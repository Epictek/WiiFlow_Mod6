
#include "menu.hpp"
#include "network/ftp.h"

void CMenu::_showConfigNet(bool instant)
{
	_hideCheckboxes(true); // reset checkboxes
	
	m_btnMgr.setText(m_configLblTitle, _t("cfgg98", L"Network settings"));
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_configBtnBack);
	m_btnMgr.setText(m_configBtnCenter, _t("reboot", L"Reboot"));
	m_btnMgr.show(m_configBtnCenter);
	
	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);

	//! FTP Server
	m_btnMgr.setText(m_configLbl[1], _t("ftp97", L"FTP Server"));
	//! FTP Server password
	m_btnMgr.setText(m_configLbl[2], _t("ftp96", L"FTP Password"));
	//! WiFi gecko IP
	m_btnMgr.setText(m_configLbl[3], _t("cfg838", L"WiFi gecko IP"));
	//! WiFi gecko debug
	m_btnMgr.setText(m_configLbl[4], _t("cfg787", L"Enable WiFi debug next boot"));
	m_checkboxBtn[4] = m_cfg.getOptBool("DEBUG", "wifi_gecko", 0) == 0 ? m_configChkOff[4] : m_configChkOn[4];
	//! FTP on startup
	m_btnMgr.setText(m_configLbl[5], _t("ftp98", L"Start FTP with WiiFlow"));
	m_checkboxBtn[5] = m_cfg.getOptBool("FTP", "auto_start", 0) == 0 ? m_configChkOff[5] : m_configChkOn[5];
	//! Allow active mode FTP
	m_btnMgr.setText(m_configLbl[6], _t("ftp99", L"Allow FTP active mode"));
	m_checkboxBtn[6] = m_cfg.getOptBool("FTP", "allow_active_mode", 0) == 0 ? m_configChkOff[6] : m_configChkOn[6];
	//! Enable Gamercard
	m_btnMgr.setText(m_configLbl[7], _t("cfg806", L"Disable Gamercard"));
	m_checkboxBtn[7] = m_cfg.getOptBool("GAMERCARD", "gamercards_enable", 0) == 1 ? m_configChkOff[7] : m_configChkOn[7];
	//! Use system proxy
	m_btnMgr.setText(m_configLbl[8], _t("cfg807", L"Use system proxy"));
	m_checkboxBtn[8] = m_cfg.getOptBool("PROXY", "proxy_use_system", 1) == 0 ? m_configChkOff[8] : m_configChkOn[8];

	for(u8 i = 1; i < 9; i++)
	{
		m_btnMgr.show(m_configLbl[i], instant);
		if(i < 4)
			m_btnMgr.show(m_configBtnGo[i], instant);
		else
			m_btnMgr.show(m_checkboxBtn[i], instant);
	}
}

void CMenu::_configNet(void)
{
	bool cur_gamercard = m_cfg.getBool("GAMERCARD", "gamercards_enable", 0);
	bool prev_gamercard = cur_gamercard;
	bool cur_proxy = m_cfg.getBool("PROXY", "proxy_use_system", 1);
	bool prev_proxy = cur_proxy;
	bool rb = false;
	bool show_error = true; // show error message if reboot needed
	
	SetupInput();
	_showConfigNet();

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
			else if(m_btnMgr.selected(m_configBtnGo[1])) // FTP server
			{
				_hideConfig();
				_FTP();
				_showConfigNet();
			}
			else if(m_btnMgr.selected(m_configBtnGo[2])) // FTP password
			{
				_hideConfig();
				string s = m_cfg.getString("FTP", "password", "");
				bool setPass = false;
				if(s != "") // a password is already set, suggest removing it
				{
					if(_error(_t("cfg840", L"Enter current password to remove it."), true))
					{
						char *c = NULL;
						c = _keyboard();
						if(strcmp(c, s.c_str()) != 0)
							_error(_t("cfgg25",L"Password incorrect."));
						else
						{
							s = "";
							setPass = true;
						}
					}
				}
				else // no password set
				{
					char *c = NULL;
					c = _keyboard();
					if(strlen(c) > 0)
					{
						if(c[0] == ' ' || c[strlen(c)-1] == ' ')
							_error(_t("cfg842", L"Password can't begin or end with a space!"));
						else if(_error(wfmt(_fmt("cfg839", L"Set %s as FTP password?"), c), true))
						{
							s = c;
							setPass = true;
						}
					}
				}
				if(setPass)
				{
					m_cfg.setString("FTP", "password", s);
					set_ftp_password(s.c_str());
				}
				_showConfigNet();
			}
			else if(m_btnMgr.selected(m_configBtnGo[3])) // Wifi gecko IP
			{
				_hideConfig();
				char *c = NULL;
				c = _keyboard();
				if(strlen(c) > 0)
				{
					if(_error(wfmt(_fmt("cfg841", L"Set %s as Wifi gecko IP?"), c), true))
					{
						string s(c);
						m_cfg.setString("DEBUG", "wifi_gecko_ip", s);
						rb = true;
					}
				}
				_showConfigNet();
			}
			else if(m_btnMgr.selected(m_checkboxBtn[4])) // enable wifi debug
			{
				m_cfg.setBool("DEBUG", "wifi_gecko", !m_cfg.getBool("DEBUG", "wifi_gecko"));
				_showConfigNet(true);
				m_btnMgr.setSelected(m_checkboxBtn[4]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[5])) // ftp on startup
			{
				m_cfg.setBool("FTP", "auto_start", !m_cfg.getBool("FTP", "auto_start"));
				_showConfigNet(true);
				m_btnMgr.setSelected(m_checkboxBtn[5]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[6])) // ftp active mode
			{
				ftp_allow_active = !ftp_allow_active;
				m_cfg.setBool("FTP", "allow_active_mode", ftp_allow_active);
				_showConfigNet(true);
				m_btnMgr.setSelected(m_checkboxBtn[6]);
			}

			else if(m_btnMgr.selected(m_checkboxBtn[7])) // gamercard
			{
				cur_gamercard = !cur_gamercard;
				m_cfg.setBool("GAMERCARD", "gamercards_enable", cur_gamercard);
				_showConfigNet(true);
				m_btnMgr.setSelected(m_checkboxBtn[7]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[8])) // proxy
			{
				cur_proxy = !cur_proxy;
				m_cfg.setBool("PROXY", "proxy_use_system", cur_proxy);
				_showConfigNet(true);
				m_btnMgr.setSelected(m_checkboxBtn[8]);
			}
		}
	}
	
	if(prev_gamercard != cur_gamercard || prev_proxy != cur_proxy || rb) // gamercard, proxy or wifi IP values changed
	{
		if(show_error)
			_error(_t("errboot8", L"WiiFlow needs rebooting to apply changes."));
		m_exit = true;
		m_reload = true;
	}

	_hideConfig(true);
}
