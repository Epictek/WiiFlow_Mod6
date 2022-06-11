
#include <algorithm> // to sort themes
// #include <dirent.h>

#include "menu.hpp"

void CMenu::_showConfigGui(bool instant)
{
	_hideCheckboxes(true); // reset checkboxes
	
	m_btnMgr.setText(m_configLblTitle, _t("cfg795", L"User interface"));
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_configBtnBack);

	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);
	
	//! Theme selection
	m_btnMgr.setText(m_configLbl[0], _t("cfga7", L"Select theme"));
	m_btnMgr.setText(m_configLblVal[0], m_themeName);
	//! Adjust CF
	m_btnMgr.setText(m_configLbl[1], _t("cfgc4", L"Adjust coverflow"));
	//! Show game banner and plugin snapshot (turns on/off banner sound)
	m_btnMgr.setText(m_configLbl[2], _t("cfg779", L"Show game banner"));
	m_checkboxBtn[2] = m_bnrSndVol == 0 ? m_configChkOff[2] : m_configChkOn[2];
	//! Show banner as background in game settings
	m_btnMgr.setText(m_configLbl[3], _t("cfg705", L"Show banner in game settings"));
	m_checkboxBtn[3] = m_cfg.getOptBool(general_domain, "banner_in_settings", 1) == 0 ? m_configChkOff[3] : m_configChkOn[3];
	//! CF covers box mode
	m_btnMgr.setText(m_configLbl[4], _t("cfg726", L"Covers box mode"));
	m_checkboxBtn[4] = m_cfg.getOptBool(general_domain, "box_mode", 1) == 0 ? m_configChkOff[4] : m_configChkOn[4];
	//! Use HQ covers
	m_btnMgr.setText(m_configLbl[5], _t("cfg713", L"Use HQ covers"));
	m_checkboxBtn[5] = m_cfg.getOptBool(general_domain, "cover_use_hq", 0) == 0 ? m_configChkOff[5] : m_configChkOn[5];
	//! Memorize favorite mode state when returning to CF
	m_btnMgr.setText(m_configLbl[6], _t("cfgd5", L"Save favorite mode state"));
	m_checkboxBtn[6] = m_cfg.getOptBool(general_domain, "save_favorites_mode", 1) == 0 ? m_configChkOff[6] : m_configChkOn[6];
	//! Wiimote vibration
	m_btnMgr.setText(m_configLbl[7], _t("cfg709", L"Rumble"));
	m_checkboxBtn[7] = m_cfg.getOptBool(general_domain, "rumble", 1) == 0 ? m_configChkOff[7] : m_configChkOn[7];
	//! Wiimote gestures (explore coverflow by moving Wiimote)
	m_btnMgr.setText(m_configLbl[8], _t("cfg710", L"Wiimote gestures"));
	m_checkboxBtn[8] = enable_wmote_roll ? m_configChkOn[8] : m_configChkOff[8];
	//! Fanart
	m_btnMgr.setText(m_configLbl[9], _t("cfg706", L"Enable fanart"));
	m_checkboxBtn[9] = m_cfg.getBool(general_domain, "enable_fanart", 0) == 0 ? m_configChkOff[9] : m_configChkOn[9];

	for(u8 i = 0; i < 10; ++i)
	{
		m_btnMgr.show(m_configLbl[i], instant);
		if(i == 0)
		{
			m_btnMgr.show(m_configLblVal[i], instant);
			m_btnMgr.show(m_configBtnM[i], instant);
			m_btnMgr.show(m_configBtnP[i], instant);
		}
		else if(i == 1)
			m_btnMgr.show(m_configBtnGo[i], instant);
		else
			m_btnMgr.show(m_checkboxBtn[i], instant);
	}
}

template <class T> static inline T loopNum(T i, T s)
{
	return (i + s) % s;
}

/* Theme */
static void listThemes(const char * path, vector<string> &themes)
{
	DIR *d;
	struct dirent *dir;
	bool def = false;

	themes.clear();
	d = opendir(path);
	if(d != 0)
	{
		dir = readdir(d);
		while(dir != 0)
		{
			string fileName = dir->d_name;
			def = def || (upperCase(fileName) == "DEFAULT.INI");
			if (fileName.size() > 4 && fileName.substr(fileName.size() - 4, 4) == ".ini")
				themes.push_back(fileName.substr(0, fileName.size() - 4));
			dir = readdir(d);
		}
		closedir(d);
	}
	if(!def)
		themes.push_back("Default");
	sort(themes.begin(), themes.end());
}

void CMenu::_configGui(void)
{
	/* Theme */
	string prevTheme = m_themeName;
	vector<string> themes;
	listThemes(m_themeDir.c_str(), themes);
	u32 curTheme = 0;
	for(u32 i = 0; i < themes.size(); ++i)
	{
		if(themes[i] == m_themeName)
		{
			curTheme = i;
			break;
		}
	}	

	/* HQ covers / box mode */
	bool cur_hq_covers = m_cfg.getBool(general_domain, "cover_use_hq", false);
	bool prev_hq_covers = cur_hq_covers;
	bool cur_box_mode = m_cfg.getBool(general_domain, "box_mode", true);
	bool prev_box_mode = cur_box_mode;

	SetupInput();
	_showConfigGui();

	while(!m_exit)
	{
		_mainLoopCommon();
		if(BTN_HOME_HELD || BTN_B_OR_1_PRESSED)
			break;
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
			m_btnMgr.up();
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
			m_btnMgr.down();
		else if(BTN_A_OR_2_PRESSED)
		{
			if(m_btnMgr.selected(m_configBtnBack))
				break;
			else if(m_btnMgr.selected(m_configBtnP[0]) || m_btnMgr.selected(m_configBtnM[0])) // theme
			{
				s8 direction = m_btnMgr.selected(m_configBtnP[0]) ? 1 : -1;
				curTheme = loopNum(curTheme + direction, (u32)themes.size());
				m_themeName = themes[curTheme];
				m_cfg.setString(general_domain, "theme", m_themeName);
				_showConfigGui(true);
			}
			else if(m_btnMgr.selected(m_configBtnGo[1])) // adjust CF
			{
				if(prevTheme != m_themeName)
					break;
				else
				{
					m_refreshGameList = true;
					_hideConfig();
					_setMainBg();
					if(_cfTheme())
						break; // reboot if CF was modified due to possible memory leak with cf_theme
					_setBg(m_configBg, m_configBg); // reset background after adjusting CF
					_showConfigGui();
				}
			}
			else if(m_btnMgr.selected(m_checkboxBtn[2])) // show game banner
			{
				m_bnrSndVol = m_bnrSndVol != 0 ? 0 : 255;
				m_cfg.setInt(general_domain, "sound_volume_bnr", m_bnrSndVol);
				_showConfigGui(true);
				m_btnMgr.setSelected(m_checkboxBtn[2]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[3])) // game banner in game settings
			{
				m_bnr_settings = !m_cfg.getBool(general_domain, "banner_in_settings");
				m_cfg.setBool(general_domain, "banner_in_settings", m_bnr_settings);
				_showConfigGui(true);
				m_btnMgr.setSelected(m_checkboxBtn[3]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[4])) // box mode
			{
				cur_box_mode = !cur_box_mode;
				m_cfg.setBool(general_domain, "box_mode", cur_box_mode);
				_showConfigGui(true);
				m_btnMgr.setSelected(m_checkboxBtn[4]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[5])) // use HQ cover
			{
				cur_hq_covers = !cur_hq_covers;
				m_cfg.setBool(general_domain, "cover_use_hq", cur_hq_covers);
				_showConfigGui(true);
				m_btnMgr.setSelected(m_checkboxBtn[5]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[6])) // save favorites mode
			{
				m_cfg.setBool(general_domain, "save_favorites_mode", !m_cfg.getBool(general_domain, "save_favorites_mode"));
				_showConfigGui(true);
				m_btnMgr.setSelected(m_checkboxBtn[6]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[7])) // wiimote vibration
			{
				bool rumble = !m_cfg.getBool(general_domain, "rumble");
				m_cfg.setBool(general_domain, "rumble", rumble);
				m_btnMgr.setRumble(rumble);
				_showConfigGui(true);
				m_btnMgr.setSelected(m_checkboxBtn[7]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[8])) // wiimote gestures
			{
				enable_wmote_roll = !enable_wmote_roll;
				m_cfg.setBool(general_domain, "wiimote_gestures", enable_wmote_roll);
				_showConfigGui(true);
				m_btnMgr.setSelected(m_checkboxBtn[8]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[9])) // enable fanart
			{
				m_cfg.setBool(general_domain, "enable_fanart", !m_cfg.getBool(general_domain, "enable_fanart"));
				_showConfigGui(true);
				m_btnMgr.setSelected(m_checkboxBtn[9]);
			}
		}
	}
	_hideConfig(true);

	/* HQ covers / box mode */
	if(prev_hq_covers != cur_hq_covers || prev_box_mode != cur_box_mode)
		_initCF();
	
	/* Theme */
	if(prevTheme != m_themeName)
	{
		m_vid.waitMessage(0.5f);
		//! get new theme default settings for source menu
		m_themeDataDir = fmt("%s/%s", m_themeDir.c_str(), m_themeName.c_str());
		m_theme.unload();
		m_theme.load(fmt("%s.ini", m_themeDataDir.c_str()));
		m_cfg.setBool(sourceflow_domain, "enabled", m_theme.getBool("_SOURCEFLOW", "enabled", false));
		m_cfg.setBool(sourceflow_domain, "box_mode", m_theme.getBool("_SOURCEFLOW", "box_mode", false));
		m_cfg.setBool(sourceflow_domain, "smallbox", m_theme.getBool("_SOURCEFLOW", "smallbox", true));
		//! cleanup sourceflow cache and previous settings in wiiflow.ini
		fsop_deleteFolder(fmt("%s/sourceflow", m_cacheDir.c_str()));
		m_cfg.remove(sourceflow_domain, "numbers");
		m_cfg.remove(sourceflow_domain, "tiers");
		m_cfg.removeDomain("SOURCEFLOW_CACHE");
		m_cfg.removeDomain("PLUGIN_CFVERSION");
		//! reset all coverflow layouts to first one
		for(m_current_view = COVERFLOW_WII; m_current_view <= COVERFLOW_HOMEBREW; m_current_view = m_current_view * 2)
			_setCFVersion(1);
		// _setSrcFlow(1); // not sure if really needed
		error(_t("errboot8", L"WiiFlow needs rebooting to apply changes."));
		m_exit = true;
		m_reload = true;
	}
}