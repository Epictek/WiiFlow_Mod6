
#include "menu.hpp"
#include "channel/nand.hpp"

void CMenu::_showWBFS(u8 op)
{
	switch(op)
	{
		case WO_REMOVE_GAME:
			m_btnMgr.setText(m_configLblTitle, _t("wbfsop2", L"Delete game"));
			m_btnMgr.setText(m_configLbl[7], _t("wbfsop32", L"Delete cover files too"));
			break;
		case WO_BACKUP_EMUSAVE:
			m_btnMgr.setText(m_configLblTitle, _t("cfgsave1", L"Backup game save"));
			break;
		case WO_REMOVE_EMUSAVE:
			m_btnMgr.setText(m_configLblTitle, _t("cfgsave2", L"Delete game save"));
			break;
		case WO_RESTORE_EMUSAVE:
			m_btnMgr.setText(m_configLblTitle, _t("cfgsave3", L"Restore game save"));
			break;
	};
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.setText(m_configBtnCenter, _t("cfgne6", L"Start"));
	m_btnMgr.show(m_configBtnCenter);
	m_btnMgr.show(m_configLblDialog);
	m_btnMgr.show(m_configBtnBack);
	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);
}

bool CMenu::_wbfsOp(u8 op)
{
	bool done = false;
	bool delCover = false;
	const dir_discHdr *CF_Hdr = CoverFlow.getHdr();
		
	SetupInput();
	_showWBFS(op);
	
	if(op == WO_REMOVE_GAME)
	{
		m_btnMgr.setText(m_configLblDialog, wfmt(_fmt("wbfsremdlg", L"Permanently remove %s?"), CoverFlow.getTitle().toUTF8().c_str()));
		if(CF_Hdr->type != TYPE_PLUGIN)
		{
			m_btnMgr.show(m_configLbl[7]); // delete cover
			m_checkboxBtn[7] = m_configChkOff[7];
			m_btnMgr.show(m_checkboxBtn[7]);
		}
	}
	else if(op == WO_BACKUP_EMUSAVE)
	{
		if(CF_Hdr->type == TYPE_GC_GAME)
			m_btnMgr.setText(m_configLblDialog, wfmt(_fmt("cfgmc8", L"Copy %s:/saves/%s.raw virtual MemCard to %s?"), DeviceName[currentPartition], fmt("%.4s", CF_Hdr->id), m_backupDir.c_str()));
		else
			m_btnMgr.setText(m_configLblDialog, wfmt(_fmt("cfgsave4", L"Copy %s emunand save to %s?"), CoverFlow.getTitle().toUTF8().c_str(), m_backupDir.c_str()));
	}
	else if(op == WO_REMOVE_EMUSAVE)
	{
		if(CF_Hdr->type == TYPE_GC_GAME)
			m_btnMgr.setText(m_configLblDialog, wfmt(_fmt("cfgmc6", L"Delete %s:/saves/%s.raw virtual MemCard?"), DeviceName[currentPartition], fmt("%.4s", CF_Hdr->id)));
		else
			m_btnMgr.setText(m_configLblDialog, wfmt(_fmt("cfgsave5", L"Delete %s save from emunand?"), CoverFlow.getTitle().toUTF8().c_str()));
	}
	else if(op == WO_RESTORE_EMUSAVE)
	{
		if(CF_Hdr->type == TYPE_GC_GAME)
			m_btnMgr.setText(m_configLblDialog, wfmt(_fmt("cfgmc7", L"Restore %s:/saves/%s.raw? virtual MemCard"), DeviceName[currentPartition], fmt("%.4s", CF_Hdr->id)));
		else
			m_btnMgr.setText(m_configLblDialog, wfmt(_fmt("cfgsave6", L"Restore %s save to emunand?"), CoverFlow.getTitle().toUTF8().c_str()));
	}

	m_thrdStop = false;
	m_thrdMessageAdded = false;

	while(!m_exit)
	{
		_mainLoopCommon();
		if((BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnBack))) && !m_thrdWorking)
			break;
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
			m_btnMgr.up();
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
			m_btnMgr.down();
		else if(BTN_A_OR_2_PRESSED && !m_thrdWorking)
		{
			if(m_btnMgr.selected(m_configChkOff[7]) || m_btnMgr.selected(m_configChkOn[7])) // delete cover
			{
				m_btnMgr.hide(m_checkboxBtn[7], true);
				delCover = !delCover;
				m_checkboxBtn[7] = delCover ? m_configChkOn[7] : m_configChkOff[7];
				m_btnMgr.show(m_checkboxBtn[7], true);
			}
			else if(m_btnMgr.selected(m_configBtnCenter)) // start
			{
				_hideConfig();
				_showProgress();
				_start_pThread();
				
				//! REMOVE GAME
				/****************************************************/
				if(op == WO_REMOVE_GAME)
				{
					const char *gamePath = CF_Hdr->path;
					if(CF_Hdr->type == TYPE_GC_GAME)
					{
						if(strcasestr(CF_Hdr->path, "boot.bin") != NULL) // extracted
						{
							*strrchr(gamePath, '/') = '\0'; // remove /boot.bin from path
							*strrchr(gamePath, '/') = '\0'; // remove /sys folder from path
							fsop_deleteFolder(gamePath);
						}
						else // iso
						{
							*strrchr(gamePath, '/') = '\0'; // remove /game.iso from path
							const char *cmp = fmt(gc_games_dir, DeviceName[currentPartition]);
							if(strcasecmp(gamePath, cmp) == 0)
								fsop_deleteFile(CF_Hdr->path);
							else
								fsop_deleteFolder(gamePath);
						}
						m_cfg.setBool(gc_domain, "update_cache", true);
					}
					else if(CF_Hdr->type == TYPE_PLUGIN)
					{
						bool game_deleted = false;
						if(strrchr(CF_Hdr->path, '/') != NULL && strcasestr(CF_Hdr->path, ".cue") == NULL) // Not Scummvm or CD-ROM folder
						{
							fsop_deleteFile(CF_Hdr->path);
							game_deleted = true;
						}
						else if(strcasestr(CF_Hdr->path, ".cue") != NULL) // CD-ROM folder
						{
							*strrchr(gamePath, '/') = '\0'; // remove /game.cue from path
							u8 pos = m_plugin.GetPluginPosition(CF_Hdr->settings[0]);
							const char *cmp = fmt("%s:/%s", DeviceName[currentPartition], m_plugin.GetRomDir(pos));
							if(strcasecmp(gamePath, cmp) != 0)
							{
								fsop_deleteFolder(gamePath);
								game_deleted = true;
							}
						}
						if(game_deleted)
							m_cfg.setBool(plugin_domain, "update_cache", true);
						else // Scummvm or CD-ROM file not in a specific folder
						{
							_stop_pThread();
							error(_t("wbfsoperr99", L"Can't delete game! Removing cached cover only."));
							done = true;
							break;
						}						
					}
					else if(CF_Hdr->type == TYPE_WII_GAME)
					{
						DeviceHandle.OpenWBFS(currentPartition);
						WBFS_RemoveGame((u8*)&CF_Hdr->id, (char*)&CF_Hdr->path);
						WBFS_Close();
						m_cfg.setBool(wii_domain, "update_cache", true);
					}
					else if(CF_Hdr->type == TYPE_EMUCHANNEL)
					{
						if(CF_Hdr->settings[0] != 0x00010001)
						{
							_stop_pThread();
							error(_t("wbfsoperr5", L"Deleting this channel is not allowed!"));
							done = true;
							break;
						}
						else
						{
							const char *nand_base = NandHandle.GetPath();
							fsop_deleteFolder(fmt("%s/title/%08x/%08x", nand_base, CF_Hdr->settings[0], CF_Hdr->settings[1]));
							fsop_deleteFile(fmt("%s/ticket/%08x/%08x.tik", nand_base, CF_Hdr->settings[0], CF_Hdr->settings[1]));
							m_cfg.setBool(channel_domain, "update_cache", true);
						}
					}
					else // who knows how but just block it
					{
						_stop_pThread();
						break;
					}

					if(delCover)
						RemoveCover(CF_Hdr->id); // not for plugin
					
					m_btnMgr.setText(m_configLblDialog, _t("wbfsop7", L"Game deleted."));
					done = true;
				}
				
				//! BACKUP - REMOVE - RESTORE EMUNAND MEMCARD OR SAVE
				/****************************************************/
				else
				{
					if(op != WO_REMOVE_EMUSAVE)
						m_btnMgr.setText(m_configLblDialog, _t("dlmsg13", L"Saving..."));
					const char *savePath = NULL, *backupPath = NULL;
					bool ok = false, exist = false;
					if(CF_Hdr->type == TYPE_GC_GAME) // MEMCARD
					{
						savePath = fmt("%s:/saves/%.4s.raw", DeviceName[currentPartition], CF_Hdr->id);
						backupPath = fmt("%s/gamecube/%.4s.raw", m_backupDir.c_str(), CF_Hdr->id);
						exist = fsop_FileExist(op == WO_BACKUP_EMUSAVE ? savePath : backupPath);
						if(exist)
						{
							if(op == WO_BACKUP_EMUSAVE)
							{
								// gprintf("Copying %s to %s\n", savePath, backupPath);
								fsop_MakeFolder(fmt("%s/gamecube", m_backupDir.c_str()));
								if(fsop_CopyFile(savePath, backupPath, NULL, NULL))
									ok = true;
							}
							else if (op == WO_REMOVE_EMUSAVE)
							{
								// gprintf("Deleting MemCard %s\n", savePath);
								fsop_deleteFile(savePath);
								ok = true;
							}
							else // WO_RESTORE_EMUSAVE
							{
								// gprintf("Copying %s to %s\n", backupPath, savePath);
								fsop_MakeFolder(fmt("%s:/saves", DeviceName[currentPartition]));
								if(fsop_CopyFile(backupPath, savePath, NULL, NULL))
									ok = true;
							}
							done = true;
						}
					}
					else // NAND SAVE
					{
						if(CF_Hdr->type == TYPE_WII_GAME) // Wii emunand save
						{
							int wiiSaveID = CF_Hdr->id[0] << 24 | CF_Hdr->id[1] << 16 | CF_Hdr->id[2] << 8 | CF_Hdr->id[3];
							savePath = fmt("%s:%s/title/00010000/%08x", DeviceName[_FindEmuPart(SAVES_NAND, true)], NandHandle.Get_NandPath(), wiiSaveID);
							backupPath = fmt("%s/wii/title/00010000/%08x", m_backupDir.c_str(), wiiSaveID);
						}
						else // Channel emunand save
						{
							savePath = fmt("%s:%s/title/00010001/%08x/data", DeviceName[_FindEmuPart(EMU_NAND, true)], NandHandle.Get_NandPath(), CF_Hdr->settings[1]);
							backupPath = fmt("%s/emunand/title/00010001/%08x/data", m_backupDir.c_str(), CF_Hdr->settings[1]);
						}
						exist = fsop_FolderExist(op == WO_BACKUP_EMUSAVE ? savePath : backupPath);
						if(exist)
						{
							if(op == WO_BACKUP_EMUSAVE)
							{
								// gprintf("Copying %s to %s\n", savePath, backupPath);
								if(fsop_CopyFolder(savePath, backupPath, NULL, NULL))
									ok = true;
							}
							else if (op == WO_REMOVE_EMUSAVE)
							{
								// gprintf("Deleting emunand save %s\n", savePath);
								fsop_deleteFolder(savePath);
								ok = true;
							}
							else // WO_RESTORE_EMUSAVE
							{
								// gprintf("Copying %s to %s\n", backupPath, savePath);
								if(fsop_CopyFolder(backupPath, savePath, NULL, NULL))
									ok = true;
							}
							done = true;
						}
					}
					if(ok)
						m_btnMgr.setText(m_configLblDialog, _t("dlmsg14", L"Done."));
					else if(exist)
						m_btnMgr.setText(m_configLblDialog, _t("explorer3", L"Copy failed!"));
					else
						m_btnMgr.setText(m_configLblDialog, _t("cfgmc10", L"File not found!"));
				}
				
				_stop_pThread();
				m_btnMgr.show(m_configBtnBack);
			}
		}
	}

	_hideConfigFull(true);
	
	return done;
}

void CMenu::_initWBFSMenu()
{
	m_wbfsLblMessage = _addLabel("WBFS/MESSAGE", theme.lblFont, L"", 20, 320, 600, 20, theme.lblFontColor, FTGX_JUSTIFY_CENTER | FTGX_ALIGN_TOP);
	m_wbfsPBar = _addProgressBar("WBFS/PROGRESS_BAR", 40, 240, 560, 30);

	_setHideAnim(m_wbfsPBar, "WBFS/PROGRESS_BAR", 0, 0, -2.f, 0.f);
	_setHideAnim(m_wbfsLblMessage, "WBFS/MESSAGE", 0, 0, -2.f, 0.f);
	
	// _hideWBFS(true);
}
