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
 
#ifndef _CFG_H_
#define _CFG_H_

#include "loader/cios.h"
#include "loader/frag.h"
#include "loader/wip.h"
//! This struct should match exactly the one in wiiflow_game_booter/source/Config.hpp
struct the_CFG {
	/* Needed for wii games */
	char gameID[7];
	FragList *fragments;
	s32 wbfsDevice;
	u32 wbfsPart;
	u8 GameBootType;
	WIP_Code *wip_list;
	u32 wip_count;
	u32 *gameconf;
	u32 gameconfsize;
	void *codelist;
	u8 *codelistend;
	bool patchregion;
	/* Needed for channels */
	u64 title;
	bool use_dol;
	/* Needed for both channels and wii games */
	IOS_Info IOS;
	u8 BootType;
	u8 configbytes[2]; // [0] used for language. [1] not used
	u8 countryString; // u8?
	u8 vidMode;
	u8 patchVidMode;
	u8 vipatch;
	s8 aspectRatio;
	u8 deflicker;
	bool patchFix480p;
	u8 private_server;
	char server_addr[24];
	u8 *cheats;
	u32 cheatSize;
	u8 debugger;
	u32 hooktype;
	u32 returnTo;
	bool use_led;
} ATTRIBUTE_PACKED;

#endif /* _CFG_HPP_ */
