
#include "menu.hpp"

void CMenu::_showConfigMisc(bool instant)
{
	_hideCheckboxes(true); // reset checkboxes
	
	m_btnMgr.setText(m_configLblTitle, _t("cfg794", L"Misc settings"));
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_configBtnBack);
	m_btnMgr.setText(m_configBtnCenter, _t("reboot", L"Reboot"));
	m_btnMgr.show(m_configBtnCenter);
	
	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);

	//! Credits
	m_btnMgr.setText(m_configLbl[0], _t("home4", L"Credits"));
	//! Dump theme
	m_btnMgr.setText(m_configLbl[1], _t("cfg799", L"Dump theme config file"));
	m_btnMgr.setText(m_configBtn[1], _t("cfgne6", L"Start"));
	//! Full-Scene Anti-Aliasing
	m_btnMgr.setText(m_configLbl[2], _t("cfg808", L"Full-Scene Anti-Aliasing"));
	m_btnMgr.setText(m_configLblVal[2], wfmt(L"%i", m_cfg.getInt(general_domain, "max_fsaa", 3)));
	//! Cover buffer
	m_btnMgr.setText(m_configLbl[3], _t("cfg809", L"Cover buffer"));
	m_btnMgr.setText(m_configLblVal[3], wfmt(L"%i", m_cfg.getInt(general_domain, "cover_buffer", 20)));
	//! Fanart delay
	m_btnMgr.setText(m_configLbl[4], _t("cfg810", L"Fanart animation delay"));
	m_btnMgr.setText(m_configLblVal[4], wfmt(L"%i", m_cfg.getInt(general_domain, "fanart_delay", 8)));
	//! Use nands folder even if vWii
	m_btnMgr.setText(m_configLbl[5], _t("cfg843", L"Use \"nands\" folder on vWii"));
	m_checkboxBtn[5] = m_cfg.getBool(channel_domain, "use_vwiinands", 1) == 1 ? m_configChkOff[5] : m_configChkOn[5];
	//! Overwrite titles_dump.ini
	m_btnMgr.setText(m_configLbl[6], wfmt(_fmt("cfg819", L"Overwrite %s"), TITLES_DUMP_FILENAME));
	m_checkboxBtn[6] = m_cfg.getBool(general_domain, "overwrite_dump_list", 0) == 0 ? m_configChkOff[6] : m_configChkOn[6];
	//! Update Wii desktop playlog
	m_btnMgr.setText(m_configLbl[7], _t("cfg812", L"Update Wii playlog"));
	m_checkboxBtn[7] = m_cfg.getBool(general_domain, "playlog_update", 0) == 0 ? m_configChkOff[7] : m_configChkOn[7];
	//! SD gecko debug
	m_btnMgr.setText(m_configLbl[8], _t("cfg788", L"Enable SD debug next boot"));
	m_checkboxBtn[8] = m_cfg.getBool("DEBUG", "sd_write_log", 0) == 0 ? m_configChkOff[8] : m_configChkOn[8];
	//! Show memory usage on screen
	m_btnMgr.setText(m_configLbl[9], _t("cfg786", L"Show memory next boot"));
	m_checkboxBtn[9] = m_cfg.getBool("DEBUG", "show_mem", 0) == 0 ? m_configChkOff[9] : m_configChkOn[9];
	
	for(u8 i = 0; i < 10; ++i)
	{
		m_btnMgr.show(m_configLbl[i], instant);
		if(i == 0)
			m_btnMgr.show(m_configBtnGo[i], instant);
		else if(i == 1)
			m_btnMgr.show(m_configBtn[i], instant);
		else if(i < 5)
		{
			m_btnMgr.show(m_configLblVal[i], instant);
			m_btnMgr.show(m_configBtnM[i], instant);
			m_btnMgr.show(m_configBtnP[i], instant);
		}
		else
			m_btnMgr.show(m_checkboxBtn[i], instant);
	}
}

void CMenu::_configMisc(void)
{	
	bool rb = false;
	bool update_cf = false;
	bool cur_vwiinands = m_cfg.getBool(channel_domain, "use_vwiinands", 1);
	bool prev_vwiinands = cur_vwiinands;
	bool show_error = true;
	
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
			else if(m_btnMgr.selected(m_configBtnCenter)) // reboot
			{
				rb = true;
				show_error = false;
				break;
			}
			else if(m_btnMgr.selected(m_configBtnGo[0])) // credits
			{
				_hideConfig(true);
				_about(false);
				_showConfigMisc();
			}
			else if(m_btnMgr.selected(m_configBtn[1])) // dump theme
			{
				m_theme.save(true);
				_error(wfmt(_fmt("errdump2", L"%s/%s.ini saved"), m_themeDir.c_str(), m_themeName.c_str()));
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
			else if(m_btnMgr.selected(m_checkboxBtn[5])) // use vwiinands on vWii
			{
				cur_vwiinands = !cur_vwiinands;
				m_cfg.setBool(channel_domain, "use_vwiinands", cur_vwiinands);
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
		}
	}

	if(rb || ((cur_vwiinands != prev_vwiinands) && IsOnWiiU()))
	{
		if(show_error)
			_error(_t("errboot8", L"WiiFlow needs rebooting to apply changes."));
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

