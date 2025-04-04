/*****************************************************************************************

fsop contains comprehensive set of function for file and folder handling

an exposed s_fsop fsop structure can be used by callback to update operation status

*****************************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <ogcsys.h>
#include <ogc/lwp_watchdog.h>
#include <malloc.h>
#include <sys/statvfs.h>
#include <sys/stat.h>

#include "fileOps/fileOps.h"
#include "gecko/gecko.hpp"
#include "loader/utils.h"

#define SET(a, b) a = b; DCFlushRange(&a, sizeof(a));
#define STACKSIZE 8192

#define BLOCKSIZE			   70*1024	  //70KB

static u8 *buff = NULL;
static FILE *fs = NULL, *ft = NULL;
static u32 block = 32768;
static u32 blockIdx = 0;
static u32 blockInfo[2] = {0,0};
static u32 blockReady = 0;
static s32 stopThread;
static u64 folderSize = 0;
u64 FolderProgressBytes;

/* Return false if the file doesn't exist */
bool fsop_GetFileSizeBytes(const char *path, u32 *filesize)	// for me stats st_size report always 0 :(
{
	FILE *f;
	size_t size = 0;

	f = fopen(path, "rb");
	if(!f)
	{
		if(filesize)
			*filesize = size;
		return false;
	}

	/* Get file size */
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	if(filesize)
		*filesize = size;
	fclose(f);

	return true;
}

/*
Recursive fsop_GetFolderBytes
*/
u64 fsop_GetFolderBytes(const char *source)
{
	DIR *pdir;
	struct dirent *pent;
	char newSource[1024];
	u64 bytes = 0;

	pdir = opendir(source);

	while((pent = readdir(pdir)) != NULL) 
	{
		// Skip it
		if(pent->d_name[0] == '.')
			continue;
		snprintf(newSource, sizeof(newSource), "%s/%s", source, pent->d_name);
		// If it is a folder... recurse...
		if(fsop_FolderExist(newSource))
			bytes += fsop_GetFolderBytes(newSource);
		else // It is a file!
		{
			u32 s;
			fsop_GetFileSizeBytes(newSource, &s);
			bytes += s;
		}
	}
	closedir(pdir);

	return bytes;
}

u32 fsop_GetFolderKb(const char *source)
{
	u32 ret = (u32)round((double)fsop_GetFolderBytes (source) / 1000.0);

	return ret;
}

u32 fsop_GetFreeSpaceKb(const char *path) // return free kb on the device passed
{
	struct statvfs s;

	statvfs(path, &s);

	u32 ret = (u32)round(((double)s.f_bfree / 1000.0) * s.f_bsize);

	return ret ;
}

static void *thread_CopyFileReader()
{
	u32 rb;
	stopThread = 0;
	DCFlushRange(&stopThread, sizeof(stopThread));
	do
	{
		SET(rb, fread(&buff[blockIdx*block], 1, block, fs));
		SET(blockInfo[blockIdx], rb);
		SET(blockReady, 1);

		while(blockReady && !stopThread)
			usleep(1);
	}
	while(stopThread == 0);

	stopThread = -1;
	DCFlushRange(&stopThread, sizeof(stopThread));

	return 0;
}

bool fsop_CopyFile(const char *source, const char *target, progress_callback_t spinner, void *spinner_data)
{
	// gprintf("Creating file: %s\n", target);
	int err = 0;

	u32 size;
	u32 rb, wb;

	fs = fopen(source, "rb");
	if(!fs)
		return false;

	ft = fopen(target, "wb");
	if(!ft)
	{
		fclose(fs);
		return false;
	}

	// Get file size
	fseek(fs, 0, SEEK_END);
	size = ftell(fs);

	if(size == 0)
	{
		fclose(fs);
		fclose(ft);
		return true;
	}

	// Return to beginning...
	fseek(fs, 0, SEEK_SET);

	u8 *threadStack = NULL;
	lwp_t hthread = LWP_THREAD_NULL;

	buff = malloc(block * 2);
	if(buff == NULL)
		return false;

	blockIdx = 0;
	blockReady = 0;
	blockInfo[0] = 0;
	blockInfo[1] = 0;
	u32 bytes = 0;

	threadStack = malloc(STACKSIZE);
	if(threadStack == NULL)
	{
		free(buff);
		return false;
	}

	LWP_CreateThread(&hthread, thread_CopyFileReader, NULL, threadStack, STACKSIZE, 30);

	while(stopThread != 0)
		usleep(5);

	u32 bi;
	do
	{
		while(!blockReady)
			usleep(1); // let's wait for incoming block from the thread

		bi = blockIdx;

		// let's th thread to read the next buff
		SET(blockIdx, 1 - blockIdx);
		SET(blockReady, 0);

		rb = blockInfo[bi];
		// write current block
		wb = fwrite(&buff[bi*block], 1, rb, ft);

		if(wb != rb || rb == 0)
			err = 1;

		bytes += rb;
		if(spinner)
		{
			FolderProgressBytes += rb;
			spinner(FolderProgressBytes, folderSize, spinner_data);
		}
	}
	while(bytes < size && err == 0);

	stopThread = 1;
	DCFlushRange(&stopThread, sizeof(stopThread));

	while(stopThread != -1)
		usleep(5);

	LWP_JoinThread(hthread, NULL);
	free(threadStack);

	stopThread = 1;
	DCFlushRange(&stopThread, sizeof(stopThread));

	fclose(fs);
	fclose(ft);
	free(buff);

	if(err)
	{
		unlink(target);
		return false;
	}

	return true;
}

/*
Recursive copyfolder
*/
static bool doCopyFolder(const char *source, const char *target, progress_callback_t spinner, void *spinner_data)
{
	DIR *pdir;
	struct dirent *pent;
	char newSource[1024], newTarget[1024];
	bool ret = true;

	// If target folder doesn't exist, create it !
	fsop_MakeFolder(target);

	pdir = opendir(source);

	while((pent = readdir(pdir)) != NULL && ret == true) 
	{
		// Skip it
		if(pent->d_name[0] == '.')
			continue;
		snprintf(newSource, sizeof(newSource), "%s/%s", source, pent->d_name);
		snprintf(newTarget, sizeof(newTarget), "%s/%s", target, pent->d_name);

		// If it is a folder... recurse...
		if(fsop_FolderExist(newSource))
			ret = doCopyFolder(newSource, newTarget, spinner, spinner_data);
		else // It is a file!
			ret = fsop_CopyFile(newSource, newTarget, spinner, spinner_data);
	}

	closedir(pdir);

	return ret;
}

bool fsop_CopyFolder(const char *source, const char *target, progress_callback_t spinner, void *spinner_data)
{
	// gprintf("DML game USB->SD job started!\n");

	FolderProgressBytes = 0;
	folderSize = fsop_GetFolderBytes(source);
	return doCopyFolder(source, target, spinner, spinner_data);
}

void fsop_deleteFolder(const char *source)
{
	DIR *pdir;
	struct dirent *pent;
	char newSource[1024];

	pdir = opendir(source);

	/* First delete all subfolders and files in the folder */
	while((pent = readdir(pdir)) != NULL) 
	{
		// Skip it
		if(pent->d_name[0] == '.')
			continue;
		snprintf(newSource, sizeof(newSource), "%s/%s", source, pent->d_name);
		// If it is a folder... recurse...
		if(fsop_FolderExist(newSource))
		{
			closedir(pdir);
			fsop_deleteFolder(newSource);
			pdir = opendir(source);
		}
		else // It is a file!
		{
			closedir(pdir);
			fsop_deleteFile(newSource);
			pdir = opendir(source);
		}
	}
	closedir(pdir);
	/* Now actually delete the folder */
	gprintf("Deleting directory: %s\n", source);
	unlink(source); // using POSIX unlink to delete the folder
}

bool fsop_FileExist(const char *fn)
{
	FILE *f;
	f = fopen(fn, "rb");
	if(f)
	{
		fclose(f);
		return true;
	}
	return false;
}

void fsop_ReadFileLoc(const char *path, const u32 size, void *loc)
{
	FILE *f = fopen(path, "rb");
	fread(loc, size, 1, f);
	fclose(f);
}

u8 *fsop_ReadFile(const char *path, u32 *size)
{
	*size = 0;
	u32 filesize = 0;
	u8 *mem = NULL;
	fsop_GetFileSizeBytes(path, &filesize);
	if(filesize > 0)
	{
		mem = (u8*)MEM2_alloc(filesize);
		if(mem != NULL)
		{
			// gprintf("Reading file: %s\n", path);
			fsop_ReadFileLoc(path, filesize, mem);
			*size = filesize;
		}
	}
	return mem;
}

bool fsop_WriteFile(const char *path, const void *mem, const u32 size)
{
	if(mem == NULL || size == 0)
		return false;

	FILE *f = fopen(path, "wb");
	if(f == NULL)
		return false;
	// gprintf("Writing file: %s\n", path);
	fwrite(mem, size, 1, f);
	fclose(f);
	return true;
}

void fsop_deleteFile(const char *source)
{
	if(!fsop_FileExist(source))
		return;
	remove(source);
}

bool fsop_FolderExist(const char *path)
{
	DIR *dir;
	dir = opendir(path);
	if(dir)
	{
		closedir(dir);
		return true;
	}
	return false;
}

/*
void fsop_MakeFolder(const char *path)
{
	if(fsop_FolderExist(path))
		return;
	//gprintf("Folder path to create: %s\n", path);
	mkdir(path, S_IREAD | S_IWRITE);
}
*/

bool fsop_MakeFolder(const char *fullpath)
{
	if(!fullpath)
		return false;

	bool result  = false;

	char dirnoslash[strlen(fullpath)+1];
	strcpy(dirnoslash, fullpath);

	int pos = strlen(dirnoslash)-1;
	while(dirnoslash[pos] == '/')
	{
		dirnoslash[pos] = '\0';
		pos--;
	}

	if(fsop_FolderExist(dirnoslash))
		return true;

	else
	{
		char parentpath[strlen(dirnoslash)+2];
		strcpy(parentpath, dirnoslash);
		char * ptr = strrchr(parentpath, '/');

		if(!ptr)
		{
			//! Device root directory (must be with '/')
			strcat(parentpath, "/");
			struct stat filestat;
			if (stat(parentpath, &filestat) == 0)
				return true;

			return false;
		}

		ptr++;
		ptr[0] = '\0';

		result = fsop_MakeFolder(parentpath);
	}

	if(!result)
		return false;

	if(mkdir(dirnoslash, 0777) == -1)
		return false;

	return true;
}
