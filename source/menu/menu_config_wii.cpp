
#include "menu.hpp"
#include "channel/channels.h"

#ifdef APP_WIIFLOW_LITE
#define WFID4 "WFLA"
#else
#define WFID4 "DWFA"
#endif

static u8 curPage;

static inline int loopNum(int i, int s)
{
	return (i + s) % s;
}

void CMenu::_showConfigWii(bool instant)
{
	_hideCheckboxes(true); // reset checkboxes
	m_btnMgr.show(m_configBtnBack);
	
	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);

	int i;

	/** MAIN PAGE **/
	if(curPage == MAIN_SETTINGS)
	{
		m_btnMgr.setText(m_configLblTitle, _t("wii", L"Wii"));
		m_btnMgr.show(m_configLblTitle);
		
		//! File browser
		m_btnMgr.setText(m_configBtnCenter, _t("home8", L"File explorer"));
		m_btnMgr.show(m_configBtnCenter);

		//! Default game language
		m_btnMgr.setText(m_configLbl[2], _t("cfgb4", L"Default game language"));
		i = min(max(0, m_cfg.getInt(wii_domain, "game_language", 0)), (int)ARRAY_SIZE(CMenu::_languages) - 2);
		m_btnMgr.setText(m_configLblVal[2], _t(CMenu::_languages[i + 1].id, CMenu::_languages[i + 1].text), true);
		m_btnMgr.show(m_configLblVal[2], instant);
		m_btnMgr.show(m_configBtnM[2], instant);
		m_btnMgr.show(m_configBtnP[2], instant);
		//! Return to WiiFlow channel
		m_btnMgr.setText(m_configLbl[3], _t("cfgg21", L"Return to WiiFlow channel"));
		m_checkboxBtn[3] = m_cfg.getString(wii_domain, "returnto") == WFID4 ? m_configChkOn[3] : m_configChkOff[3];
		m_btnMgr.show(m_checkboxBtn[3], instant);
		//! Download covers and info
		m_btnMgr.setText(m_configLbl[4], _t("cfg3", L"Download covers and info"));
		//! Global video settings
		m_btnMgr.setText(m_configLbl[5], _t("cfg803", L"Global video settings"));
		//! Global nand emulation
		m_btnMgr.setText(m_configLbl[6], _t("cfg802", L"Global nand emulation"));
		//! Game location
		m_btnMgr.setText(m_configLbl[7], _t("cfg813", L"Manage Wii game list"));

		for(i = 2; i < 8; ++i)
			m_btnMgr.show(m_configLbl[i], instant);
		for(i = 4; i < 8; ++i)
			m_btnMgr.show(m_configBtnGo[i], instant);

	}
	
	/** GLOBAL VIDEO SETTINGS **/
	else if(curPage == VIDEO_SETTINGS)
	{
		m_btnMgr.setText(m_configLblTitle, _t("cfg803", L"Global video settings"));
		m_btnMgr.show(m_configLblTitle);
		
		//! Wii game video mode
		m_btnMgr.setText(m_configLbl[3], _t("cfgb3", L"Default game video mode"));
		i = min(max(0, m_cfg.getInt(wii_domain, "video_mode", 0)), (int)ARRAY_SIZE(CMenu::_VideoModes) - 2);
		m_btnMgr.setText(m_configLblVal[3], _t(CMenu::_VideoModes[i + 1].id, CMenu::_VideoModes[i + 1].text), true);
		//! Wii game video deflicker
		m_btnMgr.setText(m_configLbl[4], _t("cfgg44", L"Video deflicker"));
		i = min(max(0, m_cfg.getInt(wii_domain, "deflicker_wii", 0)), (int)ARRAY_SIZE(CMenu::_DeflickerOptions) - 2);
		m_btnMgr.setText(m_configLblVal[4], _t(CMenu::_DeflickerOptions[i + 1].id, CMenu::_DeflickerOptions[i + 1].text), true);
		//! Wii game 480p pixel patch
		m_btnMgr.setText(m_configLbl[5], _t("cfgg49", L"480p pixel patch"));
		m_checkboxBtn[5] = m_cfg.getOptBool(wii_domain, "fix480p", 0) == 0 ? m_configChkOff[5] : m_configChkOn[5];
		m_btnMgr.show(m_checkboxBtn[5], instant);
		
		for(u8 i = 3; i < 6; ++i)
			m_btnMgr.show(m_configLbl[i], instant);
		for(u8 i = 3; i < 5; ++i)
		{
			m_btnMgr.show(m_configLblVal[i], instant);
			m_btnMgr.show(m_configBtnM[i], instant);
			m_btnMgr.show(m_configBtnP[i], instant);
		}
	}

	/** GLOBAL NAND EMULATION SETTINGS **/
	else if(curPage == NANDEMU_SETTINGS)
	{
		m_btnMgr.setText(m_configLblTitle, _t("cfg802", L"Global nand emulation"));
		m_btnMgr.show(m_configLblTitle);
		
		//! Wii game saves emunand partition
		const char *partitionname = DeviceName[m_cfg.getInt(wii_domain, "savepartition")];
		m_btnMgr.setText(m_configLbl[2], _t("cfgne38", L"Wii disc emunand partition"));
		m_btnMgr.setText(m_configLblVal[2], upperCase(partitionname));
		//! Select saves emunand
		m_btnMgr.setText(m_configLbl[3], _t("cfgne32", L"Wii disc emunand folder"));
		m_btnMgr.setText(m_configLblVal[3], m_cfg.getString(wii_domain, "current_save_emunand"));
		//! Saves emunand mode
		m_btnMgr.setText(m_configLbl[4], _t("cfgne33", L"Wii disc emunand mode"));
		i = min(max(0, m_cfg.getInt(wii_domain, "save_emulation", 0)), (int)ARRAY_SIZE(CMenu::_SaveEmu) - 2);
		m_btnMgr.setText(m_configLblVal[4], _t(CMenu::_SaveEmu[i + 1].id, CMenu::_SaveEmu[i + 1].text), true);
		//! Create new emunand folder
		m_btnMgr.setText(m_configLbl[5], _t("cfg821", L"Create new emunand folder"));
		m_btnMgr.show(m_configBtnGo[5], instant);
		//! Extract all saves from nand
		m_btnMgr.setText(m_configLbl[6], _t("cfgne2", L"Extract all saves from nand"));
		m_btnMgr.setText(m_configBtn[6], _t("cfgg31", L"Extract"));
		//! Extract missing saves from nand
		m_btnMgr.setText(m_configLbl[7], _t("cfgne4", L"Extract missing saves from nand"));
		m_btnMgr.setText(m_configBtn[7], _t("cfgg31", L"Extract"));
		//! Launch neek2o saves emunand system menu
		m_btnMgr.setText(m_configLbl[8], _t("neek2", L"Neek2o system menu"));
		m_btnMgr.setText(m_configBtn[8], _t("cfgne6", L"Start"));
		
		for(u8 i = 2; i < 9; ++i)
			m_btnMgr.show(m_configLbl[i], instant);
		for(u8 i = 2; i < 5; ++i)
		{
			m_btnMgr.show(m_configLblVal[i], instant);
			m_btnMgr.show(m_configBtnM[i], instant);
			m_btnMgr.show(m_configBtnP[i], instant);
		}
		for(u8 i = 6; i < 9; ++i)
			m_btnMgr.show(m_configBtn[i], instant);
	}
	
	/** WII GAME LOCATION **/
	else if(curPage == GAME_LIST)
	{
		m_btnMgr.setText(m_configLblTitle, _t("cfg813", L"Manage Wii game list"));
		m_btnMgr.show(m_configLblTitle);
		
		//! Wii game partition
		const char *partitionname = DeviceName[m_cfg.getInt(wii_domain, "partition", 0)];
		m_btnMgr.setText(m_configLbl[2], _t("part1", L"Wii partition"));
		m_btnMgr.setText(m_configLblVal[2], upperCase(partitionname));
		//! Wii preffered partition
		s8 part = m_cfg.getInt(wii_domain, "preferred_partition", -1);
		partitionname = DeviceName[part];
		m_btnMgr.setText(m_configLbl[3], _t("part99", L"Preffered partition at boot"));
		m_btnMgr.setText(m_configLblVal[3], part == -1 ? _t("none", L"None") : upperCase(partitionname));
		//! Wii game custom path
		m_btnMgr.setText(m_configLbl[4], _t("cfg778", L"Custom path"));
		//! Install Wii game
		m_btnMgr.setText(m_configLbl[5], _t("wbfsop1", L"Install game"));
		//! Dump Wii game coverflow list
		m_btnMgr.setText(m_configLbl[6], _t("cfg783", L"Dump coverflow list"));
		m_btnMgr.setText(m_configBtn[6], _t("cfgne6", L"Start"));
		//! Refresh coverflow list and cover cache
		m_btnMgr.setText(m_configLbl[7], _t("home2", L"Refresh coverflow list"));
		m_btnMgr.setText(m_configBtn[7], _t("cfgne6", L"Start"));
		
		for(u8 i = 2; i < 8; ++i)
			m_btnMgr.show(m_configLbl[i], instant);
		for(u8 i = 2; i < 4; ++i)
		{
			m_btnMgr.show(m_configLblVal[i], instant);
			m_btnMgr.show(m_configBtnM[i], instant);
			m_btnMgr.show(m_configBtnP[i], instant);
		}
		for(u8 i = 4; i < 6; ++i)
			m_btnMgr.show(m_configBtnGo[i], instant);
		for(u8 i = 6; i < 8; ++i)
			m_btnMgr.show(m_configBtn[i], instant);
	}
}

void CMenu::_configWii(u8 startPage)
{
	if(isWiiVC)
		return;
	
	curPage = startPage;

	_setBg(m_configBg, m_configBg);
	SetupInput();
	_showConfigWii();
	
	while(!m_exit)
	{
		_mainLoopCommon();
		if(BTN_HOME_HELD || (BTN_B_OR_1_PRESSED && (curPage == MAIN_SETTINGS || startPage == GAME_LIST)))
			break;
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
			m_btnMgr.up();
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
			m_btnMgr.down();
		else if(BTN_B_OR_1_PRESSED)
		{
			_hideConfig(true);
			curPage = MAIN_SETTINGS;
			 _showConfigWii();
		}
		else if(BTN_A_OR_2_PRESSED)
		{
			if(m_btnMgr.selected(m_configBtnBack) && curPage != MAIN_SETTINGS)
			{
				if(startPage == GAME_LIST)
					break;
				else
				{
					_hideConfig(true);
					curPage = MAIN_SETTINGS;
					 _showConfigWii();
				}
			}
			
			/** MAIN PAGE **/
			else if(curPage == MAIN_SETTINGS)
			{
				if(m_btnMgr.selected(m_configBtnBack))
					break;
				//! Wii game file browser
				else if(m_btnMgr.selected(m_configBtnCenter))
				{
					_hideConfig(true);
					const char * gameDir = fmt(wii_games_dir, DeviceName[m_cfg.getInt(wii_domain, "partition", USB1)]);
					_FileExplorer(gameDir);
					_showConfigWii();
				}
				else
				{
					//! Default game language
					if(m_btnMgr.selected(m_configBtnP[2]) || m_btnMgr.selected(m_configBtnM[2]))
					{
						s8 direction = m_btnMgr.selected(m_configBtnP[2]) ? 1 : -1;
						m_cfg.setInt(wii_domain, "game_language", (int)loopNum(m_cfg.getUInt(wii_domain, "game_language", 0) + direction, ARRAY_SIZE(CMenu::_languages) - 1));
						_showConfigWii(true);
					}
					//! Return to WiiFlow channel
					else if(m_btnMgr.selected(m_checkboxBtn[3]))
					{
						if(m_cfg.getString(wii_domain, "returnto") == WFID4)
							m_cfg.remove(wii_domain, "returnto");
						else // check if channel exists
						{
							bool curNANDemuView = NANDemuView;
							NANDemuView = false;
							ChannelHandle.Init("EN");
							int amountOfChannels = ChannelHandle.Count();
							for(u16 i = 0; i < amountOfChannels; i++)
							{
								const char * WFchannel = WFID4;
								if(strncmp(WFchannel, ChannelHandle.GetId(i), 4) == 0)
								{
									m_cfg.setString(wii_domain, "returnto", WFchannel);
									break;
								}
							}
							NANDemuView = curNANDemuView;
						}
						_showConfigWii(true);
						m_btnMgr.setSelected(m_checkboxBtn[3]);
					}
					//! Download covers and info
					else if(m_btnMgr.selected(m_configBtnGo[4]))
					{
						_hideConfig(true);
						_download();
						_showConfigWii();
					}
					//! Global video settings
					else if(m_btnMgr.selected(m_configBtnGo[5]))
					{
						_hideConfig(true);
						curPage = VIDEO_SETTINGS;
						_showConfigWii();
					}
					//! Global nand emulation
					else if(m_btnMgr.selected(m_configBtnGo[6]))
					{
						_hideConfig(true);
						curPage = NANDEMU_SETTINGS;
						_showConfigWii();
					}
					//! Game location
					else if(m_btnMgr.selected(m_configBtnGo[7]))
					{
						_hideConfig(true);
						curPage = GAME_LIST;
						_showConfigWii();
					}
				}
			}
			
			/** GLOBAL VIDEO SETTINGS **/
			else if(curPage == VIDEO_SETTINGS)
			{
				//! Wii game video mode
				if(m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3]))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[3]) ? 1 : -1;
					m_cfg.setInt(wii_domain, "video_mode", (int)loopNum(m_cfg.getUInt(wii_domain, "video_mode", 0) + direction, ARRAY_SIZE(CMenu::_VideoModes) - 1));
					_showConfigWii(true);
				}
				//! Wii game video deflicker
				else if(m_btnMgr.selected(m_configBtnP[4]) || m_btnMgr.selected(m_configBtnM[4]))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[4]) ? 1 : -1;
					m_cfg.setInt(wii_domain, "deflicker_wii", (int)loopNum(m_cfg.getUInt(wii_domain, "deflicker_wii", 0) + direction, ARRAY_SIZE(CMenu::_DeflickerOptions) - 1));
					_showConfigWii(true);
				}
				//! Wii game 480p pixel patch
				else if(m_btnMgr.selected(m_checkboxBtn[5]))
				{
					m_cfg.setBool(wii_domain, "fix480p", !m_cfg.getBool(wii_domain, "fix480p"));
					_showConfigWii(true);
					m_btnMgr.setSelected(m_checkboxBtn[5]);
				}
			}
			
			/** GLOBAL NAND EMULATION **/
			else if(curPage == NANDEMU_SETTINGS)
			{
				//! Wii game saves emunand partition
				if(m_btnMgr.selected(m_configBtnP[2]) || m_btnMgr.selected(m_configBtnM[2]))
				{
					u8 prevPartition = currentPartition;
					s8 direction = m_btnMgr.selected(m_configBtnP[2]) ? 1 : -1;
					currentPartition = m_cfg.getInt(wii_domain, "savepartition");
					_setPartition(direction, true); // m_emuSaveNand = true
					_checkEmuNandSettings(SAVES_NAND); // refresh emunands in case partition was changed
					currentPartition = prevPartition;
					_showConfigWii(true);
				}
				//! Select saves emunand
				else if(m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3]))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[3]) ? 1 : -1;
					m_prev_view = m_current_view;
					m_current_view = COVERFLOW_WII;
					_SetEmuNand(direction);
					m_current_view = m_prev_view;
					_showConfigWii(true);
				}
				//! Saves emunand mode
				else if(m_btnMgr.selected(m_configBtnP[4]) || m_btnMgr.selected(m_configBtnM[4]))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[4]) ? 1 : -1;
					m_cfg.setInt(wii_domain, "save_emulation", loopNum(m_cfg.getInt(wii_domain, "save_emulation", 0) + direction, ARRAY_SIZE(CMenu::_SaveEmu) - 1)); // minus 1 because of "default" array value
					_showConfigWii(true);
				}
				//! Create new emunand folder
				else if(m_btnMgr.selected(m_configBtnGo[5]))
				{
					_hideConfig(true);
					char *c = NULL;
					c = _keyboard();
					if(strlen(c) > 0)
					{
						const char *newNand = fmt("%s:/%s/%s", DeviceName[m_cfg.getInt(wii_domain, "savepartition")], emu_nands_dir, lowerCase(c).c_str());
						if(error(wfmt(_fmt("errcfg3", L"Create %s?"), newNand), true))
						{
							fsop_MakeFolder(newNand);
							_checkEmuNandSettings(SAVES_NAND);
							error(_t("dlmsg14", L"Done."));
						}
					}
					_showConfigWii();
				}
				//! Extract all or missing saves from nand
				else if(m_btnMgr.selected(m_configBtn[6]) || m_btnMgr.selected(m_configBtn[7]))
				{
					bool all = m_btnMgr.selected(m_configBtn[6]);
					const char *currentNand = fmt("%s:/%s/%s", DeviceName[m_cfg.getInt(wii_domain, "savepartition")], emu_nands_dir, m_cfg.getString(wii_domain, "current_save_emunand").c_str());
					if(error(wfmt(_fmt("errcfg6", L"Extract saves to %s?"), currentNand), true))
					{
						m_prev_view = m_current_view;
						if(m_prev_view != COVERFLOW_WII)
						{
							m_current_view = COVERFLOW_WII; // it might be COVERFLOW_PLUGIN
							_loadList();
						}
						_NandDump(1 + all); // 1 = MISSING saves, 2 = ALL saves
						if(m_prev_view != m_current_view)
						{
							m_current_view = m_prev_view;
							m_refreshGameList = true;
						}
					}
					_showConfigWii();
				}
				//! Launch neek2o system menu
				else if(m_btnMgr.selected(m_configBtn[8]))
				{
					if(_launchNeek2oChannel(EXIT_TO_SMNK2O, SAVES_NAND))
						break;
					_showConfigWii();
				}
			}
			
			/** GAME LOCATION **/
			else if(curPage == GAME_LIST)
			{
			//! Wii partition
				if(m_btnMgr.selected(m_configBtnP[2]) || m_btnMgr.selected(m_configBtnM[2]))
				{
					m_prev_view = m_current_view;
					u8 prevPartition = currentPartition;
					s8 direction = m_btnMgr.selected(m_configBtnP[2]) ? 1 : -1;
					currentPartition = m_cfg.getInt(wii_domain, "partition");
					m_current_view = COVERFLOW_WII;
					_setPartition(direction);
					_showConfigWii(true);
					if(m_prev_view & COVERFLOW_WII || (m_prev_view & COVERFLOW_PLUGIN && m_plugin.GetEnabledStatus(m_plugin.GetPluginPosition(0x4E574949))))
						m_refreshGameList = true;
					m_current_view = m_prev_view;
					currentPartition = prevPartition;
				}
				//! Wii preffered partition
				else if(m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3]))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[3]) ? 1 : -1;
					int part = m_cfg.getInt(wii_domain, "preferred_partition", -1);
					if((part == 8 && direction == 1) || (part == 0 && direction == -1))
						part = -1;
					else if(part == -1 && direction == -1)
						part = 8;
					else
						part = loopNum(part + direction, 9);
					m_cfg.setInt(wii_domain, "preferred_partition", part);
					_showConfigWii(true);
				}
				//! Wii game custom path
				else if(m_btnMgr.selected(m_configBtnGo[4]))
				{
					_hideConfig(true);
					const char *currentPath = fmt(wii_games_dir, DeviceName[m_cfg.getInt(wii_domain, "partition", USB1)]);
					const char *path = _FolderExplorer(currentPath);
					if(strlen(path) > 0)
					{
						m_cfg.setInt(wii_domain, "partition", DeviceHandle.PathToDriveType(path));
						const char *tmpPath = fmt("%%s%s", strchr(path, ':'));
						m_cfg.setString(wii_domain, "wii_games_dir", tmpPath);
						strcpy(wii_games_dir, tmpPath);
						m_cfg.setBool(wii_domain, "update_cache", true);
						if(m_current_view & COVERFLOW_WII || (m_prev_view & COVERFLOW_PLUGIN && m_plugin.GetEnabledStatus(m_plugin.GetPluginPosition(0x4E574949))))
							m_refreshGameList = true;
					}
					_showConfigWii();
				}
				//! Install Wii game
				else if(m_btnMgr.selected(m_configBtnGo[5]))
				{
					_hideConfig(true);
					_addGame(TYPE_WII_GAME);
					_showConfigWii();
				}
				//! Dump Wii game coverflow list
				else if(m_btnMgr.selected(m_configBtn[6]))
				{
					_dumpGameList();
					_showConfigWii();
				}
				//! Refresh coverflow list and cover cache
				else if(m_btnMgr.selected(m_configBtn[7]))
				{
					m_cfg.setBool(wii_domain, "update_cache", true);
					if(m_current_view & COVERFLOW_PLUGIN)
						m_cfg.setBool(plugin_domain, "update_cache", true);
					m_refreshGameList = true;
					break;
				}
			}
		}
	}
	
	_hideConfig(true);
}
