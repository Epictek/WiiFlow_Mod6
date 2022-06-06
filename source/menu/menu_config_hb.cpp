
#include "menu.hpp"

static u8 curPage;

void CMenu::_showConfigHB(bool instant)
{
	_hideCheckboxes(true); // reset checkboxes
	m_btnMgr.show(m_configBtnBack, instant);

	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);
	
	/** MAIN PAGE **/
	if(curPage == MAIN_SETTINGS)
	{
		m_btnMgr.setText(m_configLblTitle, _t("homebrew", L"Homebrew apps"));
		m_btnMgr.show(m_configLblTitle);
		
		//! File browser
		m_btnMgr.setText(m_configBtnCenter, _t("home8", L"File explorer"));
		m_btnMgr.show(m_configBtnCenter);
		
		//! CF smallbox
		m_btnMgr.setText(m_configLbl[3], _t("cfghb2", L"Coverflow smallbox"));
		m_checkboxBtn[3] = m_cfg.getOptBool(homebrew_domain, "smallbox", 1) == 0 ? m_configChkOff[3] : m_configChkOn[3]; // default true
		//! CF box mode
		m_btnMgr.setText(m_configLbl[4], _t("cfg726", L"Covers box Mode"));
		m_checkboxBtn[4] = m_cfg.getOptBool(homebrew_domain, "box_mode", 0) == 0 ? m_configChkOff[4] : m_configChkOn[4]; // default false
		//! Adjust homebrew CF
		m_btnMgr.setText(m_configLbl[5], _t("cfgc4", L"Adjust coverflow"));
		//! Homebrew app list
		m_btnMgr.setText(m_configLbl[6], _t("cfg817", L"Manage Homebrew app list"));
		
		for(u8 i = 3; i < 7; ++i)
			m_btnMgr.show(m_configLbl[i], instant);
		for(u8 i = 3; i < 5; ++i)
			m_btnMgr.show(m_checkboxBtn[i], instant);
		for(u8 i = 5; i < 7; ++i)
			m_btnMgr.show(m_configBtnGo[i], instant);
	}
	/** HOMEBREW APP LIST **/
	else if(curPage == GAME_LIST)
	{
		m_btnMgr.setText(m_configLblTitle, _t("cfg817", L"Manage Homebrew app list"));
		m_btnMgr.show(m_configLblTitle);

		//! Homebrew partition
		m_btnMgr.setText(m_configLbl[3], _t("cfghb3", L"Homebrew partition"));
		const char *partitionname = DeviceName[m_cfg.getInt(homebrew_domain, "partition", 0)];
		m_btnMgr.setText(m_configLblVal[3], upperCase(partitionname));
		m_btnMgr.show(m_configLblVal[3], instant);
		m_btnMgr.show(m_configBtnM[3], instant);
		m_btnMgr.show(m_configBtnP[3], instant);
		//! Dump homebrew app coverflow list
		m_btnMgr.setText(m_configLbl[4], _t("cfg783", L"Dump coverflow list"));
		m_btnMgr.setText(m_configBtn[4], _t("cfgne6", L"Start"));
		//! Refresh coverflow list and cover cache
		m_btnMgr.setText(m_configLbl[5], _t("home2", L"Refresh coverflow list"));
		m_btnMgr.setText(m_configBtn[5], _t("cfgne6", L"Start"));
		
		for(u8 i = 3; i < 6; ++i)
			m_btnMgr.show(m_configLbl[i], instant);
		for(u8 i = 4; i < 6; ++i)
			m_btnMgr.show(m_configBtn[i], instant);
	}
}

void CMenu::_configHB(u8 startPage)
{
	curPage = startPage;
	
	_setBg(m_configBg, m_configBg);
	SetupInput();
	_showConfigHB();

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
			 _showConfigHB();
		}
		else if(BTN_A_OR_2_PRESSED)
		{
			if((m_btnMgr.selected(m_configBtnBack) && curPage != MAIN_SETTINGS))
			{
				if(startPage == GAME_LIST)
					break;
				else
				{
					_hideConfig(true);
					curPage = MAIN_SETTINGS;
					 _showConfigHB();
				}
			}
			/** MAIN PAGE **/
			else if(curPage == MAIN_SETTINGS)
			{
				if(m_btnMgr.selected(m_configBtnBack))
					break;
				else if(m_btnMgr.selected(m_configBtnCenter)) // file explorer
				{
					_hideConfig(true);
					const char * gameDir = fmt("%s:/apps", DeviceName[m_cfg.getInt(homebrew_domain, "partition", SD)]);
					_FileExplorer(gameDir);
					_showConfigHB();
				}
				else
				{
					if(m_btnMgr.selected(m_checkboxBtn[3])) // small box
					{
						m_refreshGameList = true;
						m_cfg.setBool(homebrew_domain, "update_cache", true);
						m_cfg.setBool(homebrew_domain, "smallbox", !m_cfg.getBool(homebrew_domain, "smallbox"));
						_showConfigHB(true);
						m_btnMgr.setSelected(m_checkboxBtn[3]);
					}
					else if(m_btnMgr.selected(m_checkboxBtn[4])) // box mode
					{
						m_refreshGameList = true;
						m_cfg.setBool(homebrew_domain, "box_mode", !m_cfg.getBool(homebrew_domain, "box_mode"));
						_showConfigHB(true);
						m_btnMgr.setSelected(m_checkboxBtn[4]);
					}
					else if(m_btnMgr.selected(m_configBtnGo[5])) // adjust CF
					{
						_hideConfig();
						m_prev_view = m_current_view;
						m_current_view = COVERFLOW_HOMEBREW;
						_showCF(true);
						_hideMain(true); // quick fix
						_getCustomBgTex();
						_setMainBg();
						if(_cfTheme())
						{
							m_exit = true;
							m_reload = true;
							break; // reboot if CF was modified due to possible memory leak with cf_theme
						}
						m_current_view = m_prev_view;
						_getCustomBgTex();
						_showCF(true);
						_hideMain(true); // quick fix
						_setBg(m_configBg, m_configBg); // reset background after adjusting CF
						_showConfigHB();
					}
					else if(m_btnMgr.selected(m_configBtnGo[6])) // game list
					{
						_hideConfig(true);
						curPage = GAME_LIST;
						_showConfigHB();
					}
				}
			}
			/** HOMEBREW APP LIST **/
			else if(curPage == GAME_LIST)
			{
				if(m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3])) // HB partition
				{
					m_prev_view = m_current_view;
					u8 prevPartition = currentPartition;
					s8 direction = m_btnMgr.selected(m_configBtnP[3]) ? 1 : -1;
					currentPartition = m_cfg.getInt(homebrew_domain, "partition");
					m_current_view = COVERFLOW_HOMEBREW;
					_setPartition(direction);
					_showConfigHB(true);
					if(m_prev_view & COVERFLOW_HOMEBREW || (m_prev_view & COVERFLOW_PLUGIN && m_plugin.GetEnabledStatus(m_plugin.GetPluginPosition(0x48425257))))
						m_refreshGameList = true;
					m_current_view = m_prev_view;
					currentPartition = prevPartition;
				}
				else if(m_btnMgr.selected(m_configBtn[4])) // dump list
				{
					_dumpGameList();
					_showConfigHB();
				}
				else if(m_btnMgr.selected(m_configBtn[5])) // refresh list
				{
					m_cfg.setBool(homebrew_domain, "update_cache", true);
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
