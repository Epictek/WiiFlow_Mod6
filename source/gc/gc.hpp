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
 
#ifndef _GC_HPP_
#define _GC_HPP_

#include <gccore.h>

// Nintendont
#include "nin_cfg.h"
#define NIN_LOADER_PATH "%s:/apps/nintendont/boot.dol"
#define NIN_SLIPPI_PATH "%s:/apps/Slippi Nintendont/boot.dol"

bool Nintendont_Installed();
bool Nintendont_GetLoader(bool use_slippi);
void Nintendont_SetOptions(const char *gamePath, const char *gameID, const char *CheatPath, u8 lang, u32 n_cfg, 
							u32 n_vm, s8 vidscale, s8 vidoffset, bool bigMC, u8 netprofile);

#endif //_GC_HPP_
