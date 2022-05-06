
#include "menu.hpp"
#include "network/ftp.h" // -ftp-

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
	m_btnMgr.setText(m_configLbl[2], _t("ftp97", L"FTP Server")); // -ftp-
	m_btnMgr.show(m_configBtnGo[2], instant);
	//! Allow active mode FTP
	m_btnMgr.setText(m_configLbl[3], _t("ftp99", L"Allow FTP active mode"));
	m_checkboxBtn[3] = m_cfg.getOptBool("FTP", "allow_active_mode", 0) == 0 ? m_configChkOff[3] : m_configChkOn[3];
	//! FTP on startup
	m_btnMgr.setText(m_configLbl[4], _t("ftp98", L"Start FTP with WiiFlow"));
	m_checkboxBtn[4] = m_cfg.getOptBool("FTP", "auto_start", 0) == 0 ? m_configChkOff[4] : m_configChkOn[4];
	//! Enable Gamercard
	m_btnMgr.setText(m_configLbl[5], _t("cfg806", L"Disable Gamercard"));
	m_checkboxBtn[5] = m_cfg.getOptBool("GAMERCARD", "gamercards_enable", 0) == 1 ? m_configChkOff[5] : m_configChkOn[5];
	//! Use system proxy
	m_btnMgr.setText(m_configLbl[6], _t("cfg807", L"Use system proxy"));
	m_checkboxBtn[6] = m_cfg.getOptBool("PROXY", "proxy_use_system", 1) == 0 ? m_configChkOff[6] : m_configChkOn[6];

	for(u8 i = 2; i < 7; i++)
		m_btnMgr.show(m_configLbl[i], instant);
	for(u8 i = 3; i < 7; ++i)
		m_btnMgr.show(m_checkboxBtn[i], instant);
	
#ifdef DEBUG
	//! WiFi gecko debug
	m_btnMgr.setText(m_configLbl[7], _t("cfg787", L"Enable WiFi debug next boot"));
	m_checkboxBtn[7] = m_cfg.getOptBool("DEBUG", "wifi_gecko", 0) == 0 ? m_configChkOff[7] : m_configChkOn[7];
	m_btnMgr.show(m_configLbl[7], instant);
	m_btnMgr.show(m_checkboxBtn[7], instant);
#endif
}

void CMenu::_configNet(void)
{
	bool cur_gamercard = m_cfg.getBool("GAMERCARD", "gamercards_enable", 0);
	bool prev_gamercard = cur_gamercard;
	bool cur_proxy = m_cfg.getBool("PROXY", "proxy_use_system", 1);
	bool prev_proxy = cur_proxy;
	bool rb = false; // -ftp-
	
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
				break;
			}
			else if(m_btnMgr.selected(m_configBtnGo[2])) // FTP server
			{
				_hideConfig();
				_FTP(); // -ftp-
				_showConfigNet();
			}
			else if(m_btnMgr.selected(m_checkboxBtn[3])) // ftp active mode
			{
				ftp_allow_active = !ftp_allow_active;
				m_cfg.setBool("FTP", "allow_active_mode", ftp_allow_active);
				_showConfigNet(true);
				m_btnMgr.setSelected(m_checkboxBtn[3]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[4])) // ftp on startup
			{
				m_cfg.setBool("FTP", "auto_start", !m_cfg.getBool("FTP", "auto_start"));
				_showConfigNet(true);
				m_btnMgr.setSelected(m_checkboxBtn[4]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[5])) // gamercard
			{
				cur_gamercard = !cur_gamercard;
				m_cfg.setBool("GAMERCARD", "gamercards_enable", cur_gamercard);
				_showConfigNet(true);
				m_btnMgr.setSelected(m_checkboxBtn[5]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[6])) // proxy
			{
				cur_proxy = !cur_proxy;
				m_cfg.setBool("PROXY", "proxy_use_system", cur_proxy);
				_showConfigNet(true);
				m_btnMgr.setSelected(m_checkboxBtn[6]);
			}
#ifdef DEBUG
			else if(m_btnMgr.selected(m_checkboxBtn[7])) // enable wifi debug
			{
				m_cfg.setBool("DEBUG", "wifi_gecko", !m_cfg.getBool("DEBUG", "wifi_gecko"));
				_showConfigNet(true);
				m_btnMgr.setSelected(m_checkboxBtn[7]);
			}
#endif
		}
	}
	
	if(prev_gamercard != cur_gamercard || prev_proxy != cur_proxy || rb) // gamercard or proxy values changed
	{
		if(!rb) // -ftp-
			error(_t("errboot8", L"WiiFlow needs rebooting to apply changes."));
		m_exit = true;
		m_reload = true;
	}

	_hideConfig(true);
}
