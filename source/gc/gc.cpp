/****************************************************************************
 * Copyright (C) 2012 FIX94
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
 
#include <gccore.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ogc/machine/processor.h>
#include <ogc/lwp_threads.h>
// for directory parsing and low-level file I/O
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include "gc/gc.hpp"
#include "gui/text.hpp"
#include "devicemounter/DeviceHandler.hpp"
#include "gecko/gecko.hpp"
#include "fileOps/fileOps.h"
#include "homebrew/homebrew.h"
#include "loader/utils.h"
#include "loader/disc.h"
#include "loader/sys.h"
#include "memory/memory.h"
#include "memory/mem2.hpp"

/* Languages */
#define SRAM_ENGLISH 0
#define SRAM_GERMAN 1
#define SRAM_FRENCH 2
#define SRAM_SPANISH 3
#define SRAM_ITALIAN 4
#define SRAM_DUTCH 5

extern "C" {
syssram* __SYS_LockSram();
u32 __SYS_UnlockSram(u32 write);
u32 __SYS_SyncSram(void);
}

u8 get_wii_language()
{
	switch (CONF_GetLanguage())
	{
		case CONF_LANG_GERMAN:
			return SRAM_GERMAN;
		case CONF_LANG_FRENCH:
			return SRAM_FRENCH;
		case CONF_LANG_SPANISH:
			return SRAM_SPANISH;
		case CONF_LANG_ITALIAN:
			return SRAM_ITALIAN;
		case CONF_LANG_DUTCH:
			return SRAM_DUTCH;
		default:
			return SRAM_ENGLISH;
	}
}

// Nintendont
NIN_CFG NinCfg;
bool slippi;

/* Since Nintendont v1.98 argsboot is supported.
since v3.324 '$$Version:' string was added for loaders to detect.
since wiiflow lite doesn't support versions less than v3.358
we will use argsboot and version detection every time. */

void Nintendont_SetOptions(const char *gamePath, const char *gameID, const char *CheatPath, u8 lang, u32 n_cfg, u32 n_vm, s8 vidscale, s8 vidoffset, bool bigMC, u8 netprofile) // bigMC for 1019 blocks Memcard
{
	memset(&NinCfg, 0, sizeof(NIN_CFG));
	NinCfg.Magicbytes = 0x01070CF6;
	
	NinCfg.MaxPads = 4;

	/* Check nintendont version so we can set the proper config version */
	u32 NIN_cfg_version = NIN_CFG_VERSION;
	char NINVersion[7]= "";
	u32 NINRev = 0;
	const char *dol_path = NULL;
	for(u8 i = SD; i < MAXDEVICES; ++i)
	{
		if(slippi)
			dol_path = fmt(NIN_SLIPPI_PATH, DeviceName[i]);
		else
			dol_path = fmt(NIN_LOADER_PATH, DeviceName[i]);
		if(!fsop_FileExist(dol_path))
			continue;
		u8 *buffer = NULL;
		u32 filesize = 0;
		const char *str = "$$Version:";
		buffer = fsop_ReadFile(dol_path, &filesize);
		// if buffer is NULL then out of memory - hopefully that doesn't happen and shouldn't
		for(u32 i = 0; i < filesize; i += 32)
		{
			if(memcmp(buffer+i, str, strlen(str)) == 0)
			{
				// get nintendont version (NINVersion) from current position in nintendont boot.dol (buffer)
				snprintf(NINVersion, sizeof(NINVersion), "%s", buffer+i+strlen(str));
				// get revision from version and use it to get NinCfg version
				NINRev = atoi(strchr(NINVersion, '.')+1);
				break;
			}
		}
		MEM2_free(buffer);
		break;
	}
	if(NINRev == 0 || NINRev < 358)
		NIN_cfg_version = 2; // nintendont not found or revision is less than 358 thus too old for wiiflow lite
		
	else if(NINRev >= 358 && NINRev < 368)
		NIN_cfg_version = 5;
	else if(NINRev >= 368 && NINRev < 424)
		NIN_cfg_version = 6;
	else if(NINRev >= 424 && NINRev < 431)
		NIN_cfg_version = 7;
	else if(NINRev >= 431 && NINRev < 487)
		NIN_cfg_version = 8;	
	NinCfg.Version = NIN_cfg_version;

	/* Set config options */
	NinCfg.Config = n_cfg;

	/* VideoMode setup */
	NinCfg.VideoMode = n_vm;
	
	NinCfg.VideoScale = vidscale;
	NinCfg.VideoOffset = vidoffset;
	
	/* BBA network profile */
	NinCfg.NetworkProfile = netprofile;
	
	/* Language setup */
	if(lang == 0)
		lang = get_wii_language();
	else
		lang--;

	switch(lang)
	{
		case SRAM_GERMAN:
			NinCfg.Language = NIN_LAN_GERMAN;
			break;
		case SRAM_FRENCH:
			NinCfg.Language = NIN_LAN_FRENCH;
			break;
		case SRAM_SPANISH:
			NinCfg.Language = NIN_LAN_SPANISH;
			break;
		case SRAM_ITALIAN:
			NinCfg.Language = NIN_LAN_ITALIAN;
			break;
		case SRAM_DUTCH:
			NinCfg.Language = NIN_LAN_DUTCH;
			break;
		default:
			NinCfg.Language = NIN_LAN_ENGLISH;
			break;
	}

	/* MemCard Blocks Setup */
	if((NinCfg.Config & NIN_CFG_MC_MULTI) || bigMC) // added bigMC
		NinCfg.MemCardBlocks = 0x4; // 1019 blocks (8MB)
	else 
		NinCfg.MemCardBlocks = 0x2; // 251 blocks (2MB)

	/* CheatPath Setup */
	if(CheatPath != NULL && (NinCfg.Config & NIN_CFG_CHEATS))
		snprintf(NinCfg.CheatPath, sizeof(NinCfg.CheatPath), strchr(CheatPath, '/'));
	
	/* GamePath Setup */
	if(strlen(gamePath) == 2 && strcmp(gamePath, "di") == 0)
		strcpy(NinCfg.GamePath, gamePath);
	else
	{
		strncpy(NinCfg.GamePath, strchr(gamePath, '/'), 254);
		if(strstr(NinCfg.GamePath, "boot.bin") != NULL)
		{
			*strrchr(NinCfg.GamePath, '/') = '\0'; // boot.bin
			*(strrchr(NinCfg.GamePath, '/')+1) = '\0'; // sys
		}
	}
	
	/* GameID Setup */
	memcpy(&NinCfg.GameID, gameID, 4);
	gprintf("Nintendont Game Path: %s, ID: %08x\n", NinCfg.GamePath, NinCfg.GameID);
	
	gprintf("Writing Arguments\n");
	AddBootArgument((char*)&NinCfg, sizeof(NIN_CFG));
}

bool Nintendont_Installed()
{
	for(u8 i = SD; i < MAXDEVICES; ++i)
	{
		const char *dol_path = fmt(NIN_LOADER_PATH, DeviceName[i]);
		if(fsop_FileExist(dol_path))
		{
			gprintf("Nintendont found\n");
			return true;
		}
	}
	return false;
}

bool Nintendont_GetLoader(bool use_slippi)
{
	slippi = use_slippi;
	bool ret = false;
	const char *dol_path = NULL;
	for(u8 i = SD; i < MAXDEVICES; ++i)
	{
		if(slippi)
			dol_path = fmt(NIN_SLIPPI_PATH, DeviceName[i]);
		else
			dol_path = fmt(NIN_LOADER_PATH, DeviceName[i]);
		ret = (LoadHomebrew(dol_path) == 1);
		if(ret == true)
		{
			gprintf("Nintendont loaded: %s\n", dol_path);
			AddBootArgument(dol_path);
			break;
		}
	}
	return ret;
}