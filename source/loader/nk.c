/***************************************************************************
 * Copyright (C) 2012  OverjoY for Wiiflow
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you
 * must not claim that you wrote the original software. If you use
 * this software in a product, an acknowledgment in the product
 * documentation would be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and
 * must not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 *
 * nk.c
 *
 ***************************************************************************/
#include <stdio.h>
#include <ogcsys.h>
#include <string.h>
#include <malloc.h>

#include "nk.h"
#include "sys.h"
// #include "armboot.h" // not needed with Crediar patch
#include "fileOps/fileOps.h"
#include "memory/mem2.hpp"
#include "gecko/gecko.hpp"
#include <ogc/machine/processor.h>

#define MEM_PROT (MEM_REG_BASE + 0x20a)
#define MEM_REG_BASE 0xd8b4000

bool checked = false;
bool neek = false;
// u32 kernelSize = 0;
// void *Kernel = NULL;
static u32 *Kernel = NULL;

void check_neek2o(void)
{
	if(checked == true)
		return;
	checked = true;

	s32 ESHandle = IOS_Open("/dev/es", 0);
	neek = (IOS_Ioctlv(ESHandle, 0xA2, 0, 0, NULL) == 0x666c6f77);
	IOS_Close(ESHandle);
	if(!neek)
	{
		s32 FSHandle = IOS_Open("/dev/fs", 0);
		neek = (IOS_Ioctlv(FSHandle, 0x21, 0, 0, NULL) == 0);
		IOS_Close(FSHandle);
	}
	if(!neek)
	{
		u32 num = 0;
		ISFS_Initialize();
		neek = (ISFS_ReadDir("/sneek", NULL, &num) == 0);
		ISFS_Deinitialize();
	}
	gprintf("WiiFlow is in %s mode\n", neek ? "neek2o" : "real nand");
}

bool neek2o(void)
{
	return neek;
}

/** code from Cyan - USB Loader GX **/
bool Load_Neek2o_Kernel(int part)
{
	if(neek2o())
		return true;
	char kernelPath[27];
	if(IsOnWiiU())
	{
		if(part == 0) // SD
			snprintf(kernelPath, sizeof(kernelPath), "sd:/sneek/vwiikernel.bin"); // doesn't work with leaked version of r96 beta 9.6
		else if(part == 1) // USB1
			snprintf(kernelPath, sizeof(kernelPath), "usb1:/sneek/vwiikernel.bin");
		else
			return false;
	}
	else
	{
		if(part == 0) // SD
			snprintf(kernelPath, sizeof(kernelPath), "sd:/sneek/kernel.bin");
		else if(part == 1) // USB1
			snprintf(kernelPath, sizeof(kernelPath), "usb1:/sneek/kernel.bin");
		else
			return false;
	}
	if(!fsop_FileExist(kernelPath))
	{
		// gprintf("File not found.\n");
		return false;
	}
	
	FILE *f = NULL;
	f = fopen(kernelPath, "rb");
	if(!f)
	{
		// gprintf("Failed loading file %s.\n", kernelPath);
		return false;
	}
	
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	rewind(f);
	
	// Allocate kernel to mem2
	Kernel = (u32 *) MEM2_alloc(fsize);
	if(!Kernel)
	{
		return false;
	}
	
	fread(Kernel, 1, fsize, f);
	// ((ioshdr*)Kernel)->argument = 0x42; // set argument size
	DCFlushRange(Kernel, fsize);
	
	// gprintf("Loaded to 0x%08x, size: %d\n", Kernel, fsize);
	// gprintf("NEEK: offset memory address: %08x\n", (u32)Kernel - 0x80000000);	// offset
	
	fclose(f);
	return true;
}

s32 Launch_nk(u64 TitleID, const char *nandpath, u64 ReturnTo)
{
	if(neek2o())
	{
		SYS_ResetSystem(SYS_RESTART, 0, 0);
		return 1;
	}	

	// memcpy((void*)0x91000000, Kernel, kernelSize);
	// DCFlushRange((void*)0x91000000, kernelSize);
	// free(Kernel);

	memcfg *MC = (memcfg*)malloc(sizeof(memcfg));
	if(MC == NULL)
		return 0;
	memset(MC, 0, sizeof(memcfg));
	MC->magic = 0x666c6f77;
	if(TitleID)
		MC->titleid = TitleID;
	if(ReturnTo)
	{
		MC->returnto = ReturnTo;
		MC->config |= NCON_EXT_RETURN_TO;
	}

	if(nandpath != NULL)
	{
		strcpy(MC->nandpath, nandpath);
		MC->config |= NCON_EXT_NAND_PATH;
	}
	memcpy((void *)0x81200000, MC, sizeof(memcfg));
	DCFlushRange((void *)(0x81200000), sizeof(memcfg));
	free(MC);

	/** Former method by giantpune to launch neek (can't make it work on vWii) **/
	/**
	void *mini = MEM1_memalign(32, armboot_size);
	if(!mini)
		return 0;
	// uses bootmii mini to run wiiflow internal armboot.bin for neek2o
	memcpy(mini, armboot, armboot_size);
	DCFlushRange(mini, armboot_size);
	*(u32*)0xc150f000 = 0x424d454d; // BMEM
	asm volatile("eieio");
	*(u32*)0xc150f004 = MEM_VIRTUAL_TO_PHYSICAL(mini);
	asm volatile("eieio");
	IOS_ReloadIOS(0xfe); // IOS254 bootmii
	MEM1_free(mini);
	**/
	
	/** Alternative method: boot mini without BootMii IOS code by Crediar taken from USB Loader GX **/
	
	write32(MEM_PROT, read32(MEM_PROT) & 0x0000FFFF);
	unsigned int i = 0x939F02F0;
	unsigned char ES_ImportBoot2[16] =
		{ 0x68, 0x4B, 0x2B, 0x06, 0xD1, 0x0C, 0x68, 0x8B, 0x2B, 0x00, 0xD1, 0x09, 0x68, 0xC8, 0x68, 0x42 };
	if( memcmp( (void*)(i), ES_ImportBoot2, sizeof(ES_ImportBoot2) ) != 0 )
		for( i = 0x939F0000; i < 0x939FE000; i+=4 )
			if( memcmp( (void*)(i), ES_ImportBoot2, sizeof(ES_ImportBoot2) ) == 0 )
				break;
	if(i >= 0x939FE000)
	{
		gprintf("ES_ImportBoot2 not patched !! Exiting...\n");
		return -1;
	}
	DCInvalidateRange( (void*)i, 0x20 );
	
	*(vu32*)(i+0x00)        = 0x48034904;   // LDR R0, 0x10, LDR R1, 0x14
	*(vu32*)(i+0x04)        = 0x477846C0;   // BX PC, NOP
	*(vu32*)(i+0x08)        = 0xE6000870;   // SYSCALL
	*(vu32*)(i+0x0C)        = 0xE12FFF1E;   // BLR
	// *(vu32*)(i+0x10)    	= 0x11000000;   // kernel offset from 0x80000000. Kernel loaded to (void *)0x91000000
	*(vu32*)(i+0x10)   		= (u32)Kernel - 0x80000000;   // kernel offset
	*(vu32*)(i+0x14)        = 0x0000FF01;   // version
	
	DCFlushRange( (void*)i, 0x20 );
	__IOS_ShutdownSubsystems();
	
	s32 fd = IOS_Open( "/dev/es", 0 );
	
	u8 *buffer = (u8*)memalign( 32, 0x100 );
	memset( buffer, 0, 0x100 );
	
	IOS_IoctlvAsync( fd, 0x1F, 0, 0, (ioctlv*)buffer, NULL, NULL );	
	
	return 1;
}

/****************************************************************************
 * Code from Cyan - USB Loader GX
 * neek2oSetNAND
 *
 * Generates nandcfg.bin if missing and adds missing EmuNAND path
 * Sets default EmuNAND path for neek2o
 *
 * @return values :
 * -1 : error
 *  x : Default path set to EmuNAND number x
 ***************************************************************************/
int neek2oSetNAND(const char* nandpath, int part) // added part
{
	// format path string for neek
	char neekNandPath[256] = "";
	if(neekPathFormat(neekNandPath, nandpath, sizeof(neekNandPath)) < 0)
		return -1;
	
	FILE *f = NULL;
	u32 ret = -1;
	bool found = false;
	u32 i = 0;
	
	char nandconfigPath[32];

	if(IsOnWiiU())
	{
		if(part == 0) // SD
			snprintf(nandconfigPath, sizeof(nandconfigPath), "sd:/sneek/vwiincfg.bin"); // doesn't work with leaked version of r96 beta 9.6
		else if(part == 1) // USB1
			snprintf(nandconfigPath, sizeof(nandconfigPath), "usb1:/sneek/vwiincfg.bin");
		else return -1;
	}
	else // real Wii
	{
		if(part == 0) // SD
			snprintf(nandconfigPath, sizeof(nandconfigPath), "sd:/sneek/nandcfg.bin");
		else if(part == 1) // USB1
			snprintf(nandconfigPath, sizeof(nandconfigPath), "usb1:/sneek/nandcfg.bin");
		else return -1;
	}	
	// gprintf("nandconfigPath : %s\n", nandconfigPath);

	// create the file if it doesn't exist
	if(!fsop_FileExist(nandconfigPath))
	{
		u8* nandConfigHeader[NANDCONFIG_HEADER_SIZE];
		memset(nandConfigHeader, 0, sizeof(nandConfigHeader));
		
		f = fopen(nandconfigPath, "wb");
		if(!f)
		{
			gprintf("Failed creating file %s.\n", nandconfigPath);
			return -1;
		}
		
		// create an empty header with 0 NAND
		fwrite(nandConfigHeader, 1, NANDCONFIG_HEADER_SIZE, f);
		fclose(f);
	}

	f = fopen(nandconfigPath, "rb");
	if(!f)
	{
		gprintf("Failed loading file %s.\n", nandconfigPath);
		return -1;
	}
	
	fseek(f, 0 , SEEK_END);
	u32 filesize = ftell(f);
	rewind(f);
	
	/* Allocate memory */
	NandConfig *nandCfg = (NandConfig *) MEM2_alloc(filesize);
	if (!nandCfg)
	{
		fclose (f);
		return -1;
	}
	
	// Read the file
	ret = fread (nandCfg, 1, filesize, f);
	if(ret != filesize)
	{
		gprintf("Failed loading file %s to Mem.\n", nandconfigPath);
		fclose (f);
		MEM2_free(nandCfg);
		return -1;
	}

	// don't parse if wrong file
	if(nandCfg->NandCnt > NANDCONFIG_MAXNAND || nandCfg->NandSel > nandCfg->NandCnt) // wrong header, delete file
	{
		fclose(f);
		MEM2_free(nandCfg);
		fsop_deleteFile(nandconfigPath);
		return -1;
	}

	// List found nands from the file
	/*
	gprintf("NandCnt = %d\n", nandCfg->NandCnt);
	gprintf("NandSel = %d\n", nandCfg->NandSel);
	for( i = 0 ; i < nandCfg->NandCnt ; i++)
		gprintf("Path %d = %s %s\n", i, &nandCfg->Nands[i], strcmp((const char *)&nandCfg->Nands[i], neekNandPath) == 0 ? "found" : "");
	*/

	for(i = 0 ; i < nandCfg->NandCnt ; i++)
	{
		if(strcmp((const char *)&nandCfg->Nands[i], neekNandPath) == 0)
		{
			found = true;
			break;
		}
	}
	
	if(found) // NAND path already present in nandcfg.bin
	{
		// set selected nand in header if different
		if(nandCfg->NandSel != i)
		{
			nandCfg->NandSel = i;
			nandCfg->Padding1 = i; // same value?
			DCFlushRange(nandCfg, NANDCONFIG_HEADER_SIZE);
			// gprintf("new nandCfg->sel = %d", nandCfg->NandSel);

			freopen(nandconfigPath, "wb", f);
			ret = fwrite(nandCfg, sizeof(char), filesize, f); // Write full file
		}
	}
	else // new NAND path to nandcfg.bin
	{
		NandInfo * newNand = (NandInfo *) MEM2_alloc(sizeof(NandInfo));
		if(newNand)
		{
			memset(newNand, 0, sizeof(NandInfo));
			snprintf(newNand->Path, sizeof(newNand->Path), neekNandPath);
			snprintf(newNand->Name, sizeof(newNand->Name), strlen(neekNandPath) == 0 ? "root" : strrchr(neekNandPath, '/')+1);
			snprintf(newNand->DiPath, sizeof(newNand->DiPath), "/sneek");
			DCFlushRange(newNand, sizeof(NandInfo));

			nandCfg->NandCnt++;
			nandCfg->NandSel = ++i;
			
			// prevent NAND selection bigger than number of NANDs.
			if(nandCfg->NandSel >= nandCfg->NandCnt)
			{
				nandCfg->NandSel = nandCfg->NandCnt -1;
				i--;
			}
			
			freopen(nandconfigPath, "wb", f);
			ret = fwrite(nandCfg, sizeof(char), filesize, f); 	// Write full file
			ret = fwrite(newNand,1,sizeof(NandInfo),f); 		// append new NANDInfo
			if(ret != sizeof(NandInfo))
				gprintf("Writing new NAND info failed\n");
			
			MEM2_free(newNand);
		}
	}
	
	// verify the header is correctly written
	freopen(nandconfigPath, "rb", f);
	ret = fread (nandCfg, 1, NANDCONFIG_HEADER_SIZE, f);
	if(ret != NANDCONFIG_HEADER_SIZE)
	{
		gprintf("Failed loading file %s to Mem.\n", nandconfigPath);
		fclose (f);
		MEM2_free(nandCfg);
		return -1;
	}
	ret = nandCfg->NandSel;
	fclose (f);
	MEM2_free(nandCfg);

	if(ret == i)
		return ret ;
	
	return -1;
}


/****************************************************************************
 * Code from Cyan - USB Loader GX
 * neekPathFormat
 *
 * Convert and trim full path to path format used by neek
 ***************************************************************************/
int neekPathFormat(char* nandpath_out, const char* nandpath_in, u32 len)
{
	const char* neekNandPathTemp = strchr(nandpath_in, '/');
	if(!neekNandPathTemp)
		return -1;
	
	snprintf(nandpath_out, len, "%s", neekNandPathTemp);
	
	if(nandpath_out[strlen(nandpath_out)-1] == '/')
		*(strrchr(nandpath_out, '/')) = '\0'; // remove trailing slash
	
	return 1;
}


/*
void NKKeyCreate(u8 *tik)
{	
	u32 *TKeyID = (u32*)MEM1_memalign(32, sizeof(u32));	
	u8 *TitleID = (u8*)MEM1_memalign(32, 0x10);
	u8 *EncTitleKey = (u8*)MEM1_memalign(32, 0x10);
	
	memset(TitleID, 0, 0x10);
	memset(EncTitleKey, 0, 0x10);

	memcpy(TitleID, tik + 0x1DC, 8);
	memcpy(EncTitleKey, tik + 0x1BF, 16);

	static ioctlv v[3] ATTRIBUTE_ALIGN(32);
	
	v[0].data = TitleID;
	v[0].len = 0x10;
	v[1].data = EncTitleKey;
	v[1].len = 0x10;
	v[2].data = TKeyID;
	v[2].len = sizeof(u32);

	s32 ESHandle = IOS_Open("/dev/es", 0);
	IOS_Ioctlv(ESHandle, 0x50, 2, 1, (ioctlv *)v);	
	IOS_Close(ESHandle);

	KeyID = *(u32*)(TKeyID);
	
	MEM1_free(TKeyID);
    MEM1_free(EncTitleKey);
    MEM1_free(TitleID);
}

void NKAESDecryptBlock(u8 *in, u8 *out)
{
	static ioctlv v[5] ATTRIBUTE_ALIGN(32);
	
	v[0].data = &KeyID;
	v[0].len = sizeof(u32);
	v[1].data = in + 0x3d0;
	v[1].len = 0x10;
	v[2].data = in + 0x400;
	v[2].len = 0x7c00;
	v[3].data = 0;
	v[3].len = 0;
	v[4].data = out;
	v[4].len = 0x7c00;

	s32 ESHandle = IOS_Open("/dev/es", 0);
	IOS_Ioctlv(ESHandle, 0x2D, 3, 2, (ioctlv *)v);	
	IOS_Close(ESHandle);
}
*/