
#include "menu.hpp"

static u8 curPage;

template <class T> static inline T loopNum(T i, T s)
{
	return (i + s) % s;
}

void CMenu::_showConfigGC(bool instant)
{
	int i;
	
	_hideCheckboxes(true); // reset checkboxes
	m_btnMgr.show(m_configBtnBack);

	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);
		
	/** MAIN PAGE **/
	if(curPage == MAIN_SETTINGS)
	{
		m_btnMgr.setText(m_configLblTitle, _t("gc", L"GameCube"));
		m_btnMgr.show(m_configLblTitle);
		
		//! File browser
		m_btnMgr.setText(m_configBtnCenter, _t("home8", L"File explorer"));
		m_btnMgr.show(m_configBtnCenter);

		//! Game location
		m_btnMgr.setText(m_configLbl[2], _t("cfg815", L"Manage GameCube game list"));
		//! Download covers and info
		m_btnMgr.setText(m_configLbl[3], _t("cfg3", L"Download covers and info"));
		//! Global video settings
		m_btnMgr.setText(m_configLbl[4], _t("cfg803", L"Global video settings"));
		//! Global emulation settings
		m_btnMgr.setText(m_configLbl[5], _t("cfg804", L"Global emulation settings"));
		//! Gamecube banner sound
		m_btnMgr.setText(m_configLbl[6], _t("cfg720", L"GameCube banner sounds"));
		m_checkboxBtn[6] = m_cfg.getOptBool(gc_domain, "play_banner_sound", 1) == 0 ? m_configChkOff[6] : m_configChkOn[6];
		//! Default game language
		m_btnMgr.setText(m_configLbl[7], _t("cfgb4", L"Default game language"));
		i = min(max(0, m_cfg.getInt(gc_domain, "game_language", 0)), (int)ARRAY_SIZE(CMenu::_GClanguages) - 2);
		m_btnMgr.setText(m_configLblVal[7], _t(CMenu::_GClanguages[i + 1].id, CMenu::_GClanguages[i + 1].text), true);

		for(i = 2; i < 8; ++i)
		{
			m_btnMgr.show(m_configLbl[i]);
			if(i < 6)
				m_btnMgr.show(m_configBtnGo[i], instant);
			else if(i < 7)
				m_btnMgr.show(m_checkboxBtn[i], instant);
			else
			{
				m_btnMgr.show(m_configLblVal[i], instant);
				m_btnMgr.show(m_configBtnM[i], instant);
				m_btnMgr.show(m_configBtnP[i], instant);
			}
		}
	}

	/** GAME LOCATION **/
	else if(curPage == GAME_LIST)
	{
		m_btnMgr.setText(m_configLblTitle, _t("cfg815", L"Manage GameCube game list"));
		m_btnMgr.show(m_configLblTitle);
		
		//! GC game partition
		const char *partitionname = DeviceName[m_cfg.getInt(gc_domain, "partition", 0)];
		m_btnMgr.setText(m_configLbl[2], _t("part2", L"GameCube partition"));
		m_btnMgr.setText(m_configLblVal[2], upperCase(partitionname));
		//! GC preffered partition
		s8 part = m_cfg.getInt(gc_domain, "preferred_partition", -1);
		partitionname = DeviceName[part];
		m_btnMgr.setText(m_configLbl[3], _t("part99", L"Preffered partition at boot"));
		m_btnMgr.setText(m_configLblVal[3], part == -1 ? _t("none", L"None") : upperCase(partitionname));
		//! GC game custom path
		m_btnMgr.setText(m_configLbl[4], _t("cfg778", L"Custom path"));
		m_btnMgr.show(m_configBtnGo[4], instant);
		//! Install GC game
		m_btnMgr.setText(m_configLbl[5], _t("wbfsop1", L"Install game"));
		m_btnMgr.show(m_configBtnGo[5], instant);
		//! Dump GC game coverflow list
		m_btnMgr.setText(m_configLbl[6], _t("cfg783", L"Dump coverflow list"));
		m_btnMgr.setText(m_configBtn[6], _t("cfgne6", L"Start"));
		//! Refresh coverflow list and cover cache
		m_btnMgr.setText(m_configLbl[7], _t("home2", L"Refresh coverflow list"));
		m_btnMgr.setText(m_configBtn[7], _t("cfgne6", L"Start"));
		
		for(i = 2; i < 8; ++i)
		{
			m_btnMgr.show(m_configLbl[i], instant);
			if(i < 4)
			{
				m_btnMgr.show(m_configLblVal[i], instant);
				m_btnMgr.show(m_configBtnM[i], instant);
				m_btnMgr.show(m_configBtnP[i], instant);
			}
			else if(i < 6)
				m_btnMgr.show(m_configBtnGo[i], instant);
			else
				m_btnMgr.show(m_configBtn[i], instant);
		}
	}
	
	/** GLOBAL VIDEO SETTINGS **/
	else if(curPage == VIDEO_SETTINGS)
	{	
		m_btnMgr.setText(m_configLblTitle, _t("cfg803", L"Global video settings"));
		m_btnMgr.show(m_configLblTitle);

		//! GC WiiU widescreen patch
		if(IsOnWiiU())
		{
			m_btnMgr.setText(m_configLbl[2], _t("cfgg46", L"WiiU Widescreen"));
			m_checkboxBtn[2] = m_cfg.getOptBool(gc_domain, "wiiu_widescreen", 0) == 0 ? m_configChkOff[2] : m_configChkOn[2];
			m_btnMgr.show(m_configLbl[2], instant);
			m_btnMgr.show(m_checkboxBtn[2], instant);
		}
		//! GC widescreen patch
		m_btnMgr.setText(m_configLbl[3], _t("cfgg36", L"Widescreen patch"));
		m_checkboxBtn[3] = m_cfg.getOptBool(gc_domain, "widescreen", 0) == 0 ? m_configChkOff[3] : m_configChkOn[3];
		//! GC game video mode
		i = min(max(0, m_cfg.getInt(gc_domain, "video_mode", 0)), (int)ARRAY_SIZE(CMenu::_GCvideoModes) - 2);
		m_btnMgr.setText(m_configLbl[4], _t("cfgb3", L"Default game video mode"));
		m_btnMgr.setText(m_configLblVal[4], _t(CMenu::_GCvideoModes[i + 1].id, CMenu::_GCvideoModes[i + 1].text), true);
		//! GC video width
		i = m_cfg.getInt(gc_domain, "nin_width", 0);
		m_btnMgr.setText(m_configLbl[5], _t("cfgg54", L"Video width"));
		if(i == 0)
			m_btnMgr.setText(m_configLblVal[5], _t("GC_Auto", L"Auto"));
		else
			m_btnMgr.setText(m_configLblVal[5], wfmt(L"%i", max(40, min(120, i)) + 600));
		//! GC video position
		m_btnMgr.setText(m_configLbl[6], _t("cfgg55", L"Video position"));
		i = m_cfg.getInt(gc_domain, "nin_pos", 0);
		m_btnMgr.setText(m_configLblVal[6], wfmt(L"%i", max(-20, min(20, i))));
		
		for(i = 3; i < 7; ++i)
		{
			m_btnMgr.show(m_configLbl[i], instant);
			if(i == 3)
				m_btnMgr.show(m_checkboxBtn[i], instant);
			else
			{
				m_btnMgr.show(m_configLblVal[i], instant);
				m_btnMgr.show(m_configBtnM[i], instant);
				m_btnMgr.show(m_configBtnP[i], instant);
			}
		}
	}
	
	/** GLOBAL EMULATION SETTINGS **/
	else if(curPage == COMPAT_SETTINGS)
	{
		m_btnMgr.setText(m_configLblTitle, _t("cfg804", L"Global emulation settings"));
		m_btnMgr.show(m_configLblTitle);

		//! GC MemCard emulation
		// minus 2 and [i + 1] to ignore "default" array value
		i = min(max(0, m_cfg.getInt(gc_domain, "emu_memcard", 1)), (int)ARRAY_SIZE(CMenu::_NinEmuCard) - 2);
		m_btnMgr.setText(m_configLbl[3], _t("cfgb11", L"Virtual MemCard mode"));
		m_btnMgr.setText(m_configLblVal[3], _t(CMenu::_NinEmuCard[i + 1].id, CMenu::_NinEmuCard[i + 1].text), true);
		//! Use Nintendont Slippi for Super Smash Bros Melee
		m_btnMgr.setText(m_configLbl[4], _t("cfg805", L"Use Slippi for SSBM"));
		m_checkboxBtn[4] = m_cfg.getBool(gc_domain, "use_slippi", 0) == 0 ? m_configChkOff[4] : m_configChkOn[4];
		//! Wiimote / classic controller rumble
		m_btnMgr.setText(m_configLbl[5], _t("cfgg52", L"Wiimote CC rumble"));
		m_checkboxBtn[5] = m_cfg.getOptBool(gc_domain, "cc_rumble", 0) == 0 ? m_configChkOff[5] : m_configChkOn[5];
		//! Native control
		m_btnMgr.setText(m_configLbl[6], _t("cfgg43", L"Native control"));
		m_checkboxBtn[6] = m_cfg.getOptBool(gc_domain, "native_ctl", 0) == 0 ? m_configChkOff[6] : m_configChkOn[6];

		for(i = 3; i < (7 - IsOnWiiU()); ++i)
		{
			m_btnMgr.show(m_configLbl[i], instant);
			if(i == 3)
			{
				m_btnMgr.show(m_configLblVal[i], instant);
				m_btnMgr.show(m_configBtnM[i], instant);
				m_btnMgr.show(m_configBtnP[i], instant);
			}
			else
				m_btnMgr.show(m_checkboxBtn[i], instant);
		}
	}
}

void CMenu::_configGC(u8 startPage)
{
	if(!m_nintendont_installed)
		_error(_t("errgame11", L"GameCube loader not found!"));
	
	curPage = startPage;
	
	_setBg(m_configBg, m_configBg);
	SetupInput();
	_showConfigGC();
	
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
			 _showConfigGC();
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
					 _showConfigGC();
				}
			}
			
			/** MAIN PAGE **/
			else if(curPage == MAIN_SETTINGS)
			{
				if(m_btnMgr.selected(m_configBtnBack))
					break;
				//! GC game file browser
				else if(m_btnMgr.selected(m_configBtnCenter))
				{
					_hideConfig(true);
					const char *gameDir = fmt(gc_games_dir, DeviceName[m_cfg.getInt(gc_domain, "partition", USB1)]);
					_pluginExplorer(gameDir);
					_showConfigGC();
				}
				else
				{
					//! Game location
					if(m_btnMgr.selected(m_configBtnGo[2]))
					{
						_hideConfig(true);
						curPage = GAME_LIST;
						_showConfigGC();
					}
					//! Download covers and info
					else if(m_btnMgr.selected(m_configBtnGo[3]))
					{
						_hideConfig(true);
						_download();
						_showConfigGC();
					}
					//! Global video settings
					else if(m_btnMgr.selected(m_configBtnGo[4]))
					{
						_hideConfig(true);
						curPage = VIDEO_SETTINGS;
						_showConfigGC();
					}
					//! Global emulation settings
					else if(m_btnMgr.selected(m_configBtnGo[5]))
					{
						_hideConfig(true);
						curPage = COMPAT_SETTINGS;
						_showConfigGC();
					}
					//! GC banner (& default) sound
					else if(m_btnMgr.selected(m_checkboxBtn[6]))
					{
						m_gc_play_banner_sound = !m_gc_play_banner_sound;
						m_cfg.setBool(gc_domain, "play_banner_sound", m_gc_play_banner_sound);
						_showConfigGC(true);
						m_btnMgr.setSelected(m_checkboxBtn[6]);
					}
					//! Default game language
					else if(m_btnMgr.selected(m_configBtnP[7]) || m_btnMgr.selected(m_configBtnM[7]))
					{
						s8 direction = m_btnMgr.selected(m_configBtnP[7]) ? 1 : -1;
						int i = loopNum(m_cfg.getInt(gc_domain, "game_language", 0) + direction, (int)ARRAY_SIZE(CMenu::_GClanguages) - 1);
						m_cfg.setInt(gc_domain, "game_language", i);
						_showConfigGC(true);
					}
				}
			}

			/** GAME LOCATION **/
			else if(curPage == GAME_LIST)
			{
				//! GC game partition
				if(m_btnMgr.selected(m_configBtnP[2]) || m_btnMgr.selected(m_configBtnM[2]))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[2]) ? 1 : -1;
					_setPartition(direction, COVERFLOW_GAMECUBE);
					if(m_current_view & COVERFLOW_GAMECUBE || (m_current_view & COVERFLOW_PLUGIN && m_plugin.GetEnabledStatus(GC_PMAGIC)))
						m_refreshGameList = true;
					_showConfigGC(true);
				}
				//! GC preffered partition
				else if(m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3]))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[3]) ? 1 : -1;
					int part = m_cfg.getInt(gc_domain, "preferred_partition", -1);
					if((part == 8 && direction == 1) || (part == 0 && direction == -1))
						part = -1;
					else if(part == -1 && direction == -1)
						part = 8;
					else
						part = loopNum(part + direction, 9);
					m_cfg.setInt(gc_domain, "preferred_partition", part);
					_showConfigGC(true);
				}
				//! GC game custom path
				else if(m_btnMgr.selected(m_configBtnGo[4]))
				{
					_hideConfig(true);
					const char *currentPath = fmt(gc_games_dir, DeviceName[m_cfg.getInt(gc_domain, "partition", USB1)]);
					const char *path = _FolderExplorer(currentPath);
					if(strlen(path) > 0)
					{
						m_cfg.setInt(gc_domain, "partition", DeviceHandle.PathToDriveType(path));
						const char *tmpPath = fmt("%%s%s", strchr(path, ':'));
						m_cfg.setString(gc_domain, "gc_games_dir", tmpPath);
						strcpy(gc_games_dir, tmpPath);
						m_cfg.setBool(gc_domain, "update_cache", true);
						if(m_current_view & COVERFLOW_GAMECUBE || (m_prev_view & COVERFLOW_PLUGIN && m_plugin.GetEnabledStatus(GC_PMAGIC)))
							m_refreshGameList = true;
					}
					_showConfigGC();
				}
				//! Install GC game
				else if(m_btnMgr.selected(m_configBtnGo[5]))
				{
					_hideConfig(true);
					_addGame(TYPE_GC_GAME);
					_showConfigGC();
				}
				//! Dump GC game coverflow list
				else if(m_btnMgr.selected(m_configBtn[6]))
				{
					_dumpGameList();
					_showConfigGC();
				}
				//! Refresh coverflow list and cover cache
				else if(m_btnMgr.selected(m_configBtn[7]))
				{
					m_cfg.setBool(gc_domain, "update_cache", true);
					if(m_current_view & COVERFLOW_PLUGIN)
						m_cfg.setBool(plugin_domain, "update_cache", true);
					m_refreshGameList = true;
					break;
				}
			}
			
			/** GLOBAL VIDEO SETTINGS **/
			else if(curPage == VIDEO_SETTINGS)
			{
				//! GC WiiU widescreen patch
				if(m_btnMgr.selected(m_checkboxBtn[2]))
				{
					m_cfg.setBool(gc_domain, "wiiu_widescreen", !m_cfg.getBool(gc_domain, "wiiu_widescreen"));
					_showConfigGC(true);
					m_btnMgr.setSelected(m_checkboxBtn[2]);
				}
				//! GC widescreen patch
				else if(m_btnMgr.selected(m_checkboxBtn[3]))
				{
					m_cfg.setBool(gc_domain, "widescreen", !m_cfg.getBool(gc_domain, "widescreen"));
					_showConfigGC(true);
					m_btnMgr.setSelected(m_checkboxBtn[3]);
				}
				//! GC game video mode
				else if(m_btnMgr.selected(m_configBtnP[4]) || m_btnMgr.selected(m_configBtnM[4]))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[4]) ? 1 : -1;
					int i = loopNum(m_cfg.getInt(gc_domain, "video_mode", 0) + direction, (int)ARRAY_SIZE(CMenu::_GCvideoModes) - 1); // minus 1 because of "default" array value
					m_cfg.setInt(gc_domain, "video_mode", i);
					_showConfigGC(true);
				}
				//! GC video width
				else if(m_btnMgr.selected(m_configBtnP[5]) || m_btnMgr.selected(m_configBtnM[5]))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[5]) ? 1 : -1;
					int val = m_cfg.getInt(gc_domain, "nin_width");
					val = val == 0 ? (direction == 1 ? 40 : 120) : val + direction * 2;
					if(val >= 40 && val <= 120)
						m_cfg.setInt(gc_domain, "nin_width", val);
					else
						m_cfg.setInt(gc_domain, "nin_width", 0);
					_showConfigGC(true);
				}
				//! GC video position
				else if(m_btnMgr.selected(m_configBtnP[6]) || m_btnMgr.selected(m_configBtnM[6]))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[6]) ? 1 : -1;
					int val = m_cfg.getInt(gc_domain, "nin_pos") + direction;
					if(val >= -20 && val <= 20)
						m_cfg.setInt(gc_domain, "nin_pos", val);
					_showConfigGC(true);
				}
			}
			
			/** GLOBAL EMULATION SETTINGS **/
			else if(curPage == COMPAT_SETTINGS)
			{
				//! GC MemCard emulation
				if(m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3]))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[3]) ? 1 : -1;
					int i = loopNum(m_cfg.getInt(gc_domain, "emu_memcard", 1) + direction, (int)ARRAY_SIZE(CMenu::_NinEmuCard) - 1);
					m_cfg.setInt(gc_domain, "emu_memcard", i);
					_showConfigGC(true);
				}
				//! Use Nintendont Slippi for Super Smash Bros Melee
				else if(m_btnMgr.selected(m_checkboxBtn[4]))
				{
					m_cfg.setBool(gc_domain, "use_slippi", !m_cfg.getBool(gc_domain, "use_slippi"));
					_showConfigGC(true);
					m_btnMgr.setSelected(m_checkboxBtn[4]);
				}
				//! Wiimote / classic controller rumble
				else if(m_btnMgr.selected(m_checkboxBtn[5]))
				{
					m_cfg.setBool(gc_domain, "cc_rumble", !m_cfg.getBool(gc_domain, "cc_rumble"));
					_showConfigGC(true);
					m_btnMgr.setSelected(m_checkboxBtn[5]);
				}
				//! Native control
				else if(m_btnMgr.selected(m_checkboxBtn[6]))
				{
					m_cfg.setBool(gc_domain, "native_ctl", !m_cfg.getBool(gc_domain, "native_ctl"));
					_showConfigGC(true);
					m_btnMgr.setSelected(m_checkboxBtn[6]);
				}
			}
		}
	}
	
	_hideConfig(true);
}
