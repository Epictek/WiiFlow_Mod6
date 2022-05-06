
#include "menu.hpp"

void CMenu::_showConfigMisc(bool instant)
{
	_hideCheckboxes(true); // reset checkboxes
	
	m_btnMgr.setText(m_configLblTitle, _t("cfg794", L"Misc settings"));
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_configBtnBack);
	
	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);

	//! Dump theme
	m_btnMgr.setText(m_configLbl[0], _t("cfg799", L"Dump theme config file"));
	m_btnMgr.setText(m_configBtn[0], _t("cfgne6", L"Start"));
	m_btnMgr.show(m_configBtn[0], instant);
	//! Add category
	m_btnMgr.setText(m_configLbl[1], _t("cfg820", L"Add new category"));
	m_btnMgr.show(m_configBtnGo[1], instant);
	//! Full-Scene Anti-Aliasing
	m_btnMgr.setText(m_configLbl[2], _t("cfg808", L"Full-Scene Anti-Aliasing"));
	m_btnMgr.setText(m_configLblVal[2], wfmt(L"%i", m_cfg.getInt(general_domain, "max_fsaa", 3)));
	//! Cover buffer
	m_btnMgr.setText(m_configLbl[3], _t("cfg809", L"Cover buffer"));
	m_btnMgr.setText(m_configLblVal[3], wfmt(L"%i", m_cfg.getInt(general_domain, "cover_buffer", 20)));
	//! Fanart delay
	m_btnMgr.setText(m_configLbl[4], _t("cfg810", L"Fanart animation delay"));
	m_btnMgr.setText(m_configLblVal[4], wfmt(L"%i", m_cfg.getInt(general_domain, "fanart_delay", 8)));
	//! Fanart
	m_btnMgr.setText(m_configLbl[5], _t("cfg706", L"Enable fanart"));
	m_checkboxBtn[5] = m_cfg.getOptBool(general_domain, "enable_fanart", 0) == 0 ? m_configChkOff[5] : m_configChkOn[5];
	//! Overwrite titles_dump.ini
	m_btnMgr.setText(m_configLbl[6], wfmt(_fmt("cfg819", L"Overwrite %s"), TITLES_DUMP_FILENAME));
	m_checkboxBtn[6] = m_cfg.getOptBool(general_domain, "overwrite_dump_list", 0) == 0 ? m_configChkOff[6] : m_configChkOn[6];
	//! Update Wii desktop playlog
	m_btnMgr.setText(m_configLbl[7], _t("cfg812", L"Update Wii playlog"));
	m_checkboxBtn[7] = m_cfg.getOptBool(general_domain, "playlog_update", 0) == 0 ? m_configChkOff[7] : m_configChkOn[7];
	
	for(u8 i = 0; i < 8; ++i)
		m_btnMgr.show(m_configLbl[i], instant);
	for(u8 i = 2; i < 5; ++i)
	{
		m_btnMgr.show(m_configLblVal[i], instant);
		m_btnMgr.show(m_configBtnM[i], instant);
		m_btnMgr.show(m_configBtnP[i], instant);
	}
	for(u8 i = 5; i < 8; ++i)
		m_btnMgr.show(m_checkboxBtn[i], instant);
	
#ifdef DEBUG
	m_btnMgr.setText(m_configBtnCenter, _t("reboot", L"Reboot"));
	m_btnMgr.show(m_configBtnCenter);
	//! SD gecko debug
	m_btnMgr.setText(m_configLbl[8], _t("cfg788", L"Enable SD debug next boot"));
	m_checkboxBtn[8] = m_cfg.getOptBool("DEBUG", "sd_write_log", 0) == 0 ? m_configChkOff[8] : m_configChkOn[8];
	//! Show memory usage on screen
	m_btnMgr.setText(m_configLbl[9], _t("cfg786", L"Show memory next boot"));
	m_checkboxBtn[9] = m_cfg.getOptBool("DEBUG", "show_mem", 0) == 0 ? m_configChkOff[9] : m_configChkOn[9];

	for(u8 i = 8; i < 10; ++i)
	{
		m_btnMgr.show(m_configLbl[i], instant);
		m_btnMgr.show(m_checkboxBtn[i], instant);
	}
#endif
}

void CMenu::_configMisc(void)
{	
	bool rb = false;
	bool update_cf = false;
	
	SetupInput();
	_showConfigMisc();

	while(!m_exit)
	{
		_mainLoopCommon();
		if(BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED)
			break;
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
			m_btnMgr.up();
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
			m_btnMgr.down();
		else if(BTN_A_OR_2_PRESSED)
		{
			if(m_btnMgr.selected(m_configBtnBack))
				break;
			else if(m_btnMgr.selected(m_configBtn[0])) // dump theme
			{
				m_theme.save(true);
				error(wfmt(_fmt("errdump2", L"%s/%s.ini saved"), m_themeDir.c_str(), m_themeName.c_str()));
				_showConfigMisc();
			}
			else if(m_btnMgr.selected(m_configBtnGo[1])) // add new category
			{
				_hideConfig();
				char *c = NULL;
				c = _keyboard();
				if(strlen(c) > 0)
				{
					string s = capitalize(lowerCase(c));
					m_btnMgr.setText(m_configLblDialog, wfmt(_fmt("cfg824", L"This cannot be undone. Are you sure you want to add category: %s?"), s.c_str()));
					m_btnMgr.show(m_configLblDialog);
					m_btnMgr.show(m_configBtnCenter);
					m_btnMgr.show(m_configBtnBack);
					bool choice = false;
					while(!m_exit)
					{
						_mainLoopCommon();
						if(BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED)
							break;
						else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
							m_btnMgr.up();
						else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
							m_btnMgr.down();
						else if(BTN_A_OR_2_PRESSED)
						{
							if(m_btnMgr.selected(m_configBtnBack))
								break;
							else if(m_btnMgr.selected(m_configBtnCenter))
							{
								choice = true;
								break;
							}
						}
					}
					_hideConfigFull(true);
					if(choice)
					{
						m_cat.setString(general_domain, fmt("cat%d", m_max_categories), s);
						m_max_categories++;
						m_cat.setInt(general_domain, "numcategories", m_max_categories);
						error(wfmt(_fmt("errcfg2", L"Category %s added."), s.c_str()));
					}
				}
				_showConfigMisc();
			}
			else if(m_btnMgr.selected(m_configBtnP[2]) || m_btnMgr.selected(m_configBtnM[2])) // FSAA
			{
				s8 direction = m_btnMgr.selected(m_configBtnP[2]) ? 1 : -1;
				int val = m_cfg.getInt(general_domain, "max_fsaa", 3) + direction;
				if(val >= 2 && val <= 8)
					m_cfg.setInt(general_domain, "max_fsaa", val);
				_showConfigMisc(true);
				update_cf = true;
			}
			else if(m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3])) // cover buffer
			{
				s8 direction = m_btnMgr.selected(m_configBtnP[3]) ? 1 : -1;
				int val = m_cfg.getInt(general_domain, "cover_buffer", 20) + direction;
				if(val >= 3 && val <= 30)
					m_cfg.setInt(general_domain, "cover_buffer", val);
				_showConfigMisc(true);
				update_cf = true;
			}
			else if(m_btnMgr.selected(m_configBtnP[4]) || m_btnMgr.selected(m_configBtnM[4])) // fanart delay
			{
				s8 direction = m_btnMgr.selected(m_configBtnP[4]) ? 1 : -1;
				int val = m_cfg.getInt(general_domain, "fanart_delay") + direction;
				if(val >= 3 && val <= 30)
					m_cfg.setInt(general_domain, "fanart_delay", val);
				_showConfigMisc(true);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[5])) // enable fanart
			{
				m_cfg.setBool(general_domain, "enable_fanart", !m_cfg.getBool(general_domain, "enable_fanart"));
				_showConfigMisc(true);
				m_btnMgr.setSelected(m_checkboxBtn[5]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[6])) // overwrite titles_dump.ini
			{
				m_cfg.setBool(general_domain, "overwrite_dump_list", !m_cfg.getBool(general_domain, "overwrite_dump_list"));
				_showConfigMisc(true);
				m_btnMgr.setSelected(m_checkboxBtn[6]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[7])) // update playlog
			{
				m_cfg.setBool(general_domain, "playlog_update", !m_cfg.getBool(general_domain, "playlog_update"));
				_showConfigMisc(true);
				m_btnMgr.setSelected(m_checkboxBtn[7]);
			}
#ifdef DEBUG
			else if(m_btnMgr.selected(m_checkboxBtn[8])) // enable sd debug
			{
				m_cfg.setBool("DEBUG", "sd_write_log", !m_cfg.getBool("DEBUG", "sd_write_log"));
				_showConfigMisc(true);
				m_btnMgr.setSelected(m_checkboxBtn[8]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[9])) // show mem
			{
				m_cfg.setBool("DEBUG", "show_mem", !m_cfg.getBool("DEBUG", "show_mem"));
				_showConfigMisc(true);
				m_btnMgr.setSelected(m_checkboxBtn[9]);
			}
			else if(m_btnMgr.selected(m_configBtnCenter)) // reboot
			{
				rb = true;
				break;
			}
#endif	
		}
	}

	if(rb)
	{
		m_exit = true;
		m_reload = true;
	}
	else if(update_cf)
	{
		_showCF(); // in case FSAA or cover buffer value changed
		_hideMain(true);
	}

	_hideConfig(true);
}

