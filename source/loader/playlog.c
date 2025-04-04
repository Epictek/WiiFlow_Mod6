/**
	PLAYLOG.C
	This code allows to modify play_rec.dat in order to store the
	game time in Wii's log correctly.

	by Marc
	Thanks to tueidj for giving me some hints on how to do it :)
	Most of the code was taken from here:
	http://forum.wiibrew.org/read.php?27,22130
**/

#include <stdio.h>
#include <string.h>
#include <ogcsys.h>
#include "gecko/gecko.hpp"

#define PLAYRECPATH "/title/00000001/00000002/data/play_rec.dat"
#define SECONDS_TO_2000 946684800LL
#define TICKS_PER_SECOND 60750000LL

typedef union 
{
	struct 
	{
		u32 checksum;
		u16 name[0x28];
		u32 padding1;
		u64 ticks_boot;
		u64 ticks_last;
		char title_id[6];
		u16 padding2[9];
	};
	struct 
	{
		u32 _checksum;
		u32 data[0x1f];
	};
} ATTRIBUTE_PACKED playtime_t;

playtime_t playrec_buf;

// Thanks to Dr. Clipper
u64 getWiiTime(void)
{
	time_t uTime = time(NULL);
	return TICKS_PER_SECOND * (uTime - SECONDS_TO_2000);
}

int Playlog_Update(const char ID[6], const u8 title[84])
{
	// gprintf("Update Play log\n");
	u32 sum = 0;
	u8 i;

	// Open play_rec.dat
	s32 playrec_fd = IOS_Open(PLAYRECPATH, IPC_OPEN_RW);
	if(playrec_fd == -106)
	{
		gprintf("Playlog_Update: IOS_Open error ret: %i\n",playrec_fd);
		IOS_Close(playrec_fd);
		
		// In case the play_rec.dat wasn't found create one and try again
		if(ISFS_CreateFile(PLAYRECPATH,0,3,3,3) < 0 )
			goto error_2;
			
		playrec_fd = IOS_Open(PLAYRECPATH, IPC_OPEN_RW);
		if(playrec_fd < 0)
			goto error_2;
	}
	else if(playrec_fd < 0)
		goto error_2;

    u64 stime = getWiiTime();
	playrec_buf.ticks_boot = stime;
	playrec_buf.ticks_last = stime;

	// Update channel name and ID
	memcpy(playrec_buf.name, title, 84);
	strcpy(playrec_buf.title_id, ID);

	memset(playrec_buf.padding2, 0, 18);

	// Calculate and update checksum
	for(i = 0; i < 31; i++)
		sum += playrec_buf.data[i];
	playrec_buf.checksum=sum;

	// Write play_rec.dat
	if(IOS_Write(playrec_fd, &playrec_buf, sizeof(playrec_buf)) != sizeof(playrec_buf))
		goto error_1;

	IOS_Close(playrec_fd);
	return 0;

error_1:
	gprintf("Playlog_Update: error_1\n");
	IOS_Close(playrec_fd);

error_2:
	gprintf("Playlog_Update: error_2\n");
	return -1;
}

int Playlog_Delete(void) // Make Wiiflow not show in playlog
{
	// Open play_rec.dat
	s32 playrec_fd = IOS_Open(PLAYRECPATH, IPC_OPEN_RW);
	if(playrec_fd < 0)
		goto error_2;

	// Read play_rec.dat
	if(IOS_Read(playrec_fd, &playrec_buf, sizeof(playrec_buf)) != sizeof(playrec_buf))
		goto error_1;

	if(IOS_Seek(playrec_fd, 0, 0) < 0)
		goto error_1;
    
	// invalidate checksum
	playrec_buf.checksum=0;

	if(IOS_Write(playrec_fd, &playrec_buf, sizeof(playrec_buf)) != sizeof(playrec_buf))
		goto error_1;

	IOS_Close(playrec_fd);
	return 0;

error_1:
	IOS_Close(playrec_fd);

error_2:
	return -1;
}
