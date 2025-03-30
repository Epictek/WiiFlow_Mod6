#include "menu.hpp"
#include "channel/nand_save.hpp"

u8 set_port = 0;

template <class T> static inline T loopNum(T i, T s)
{
	return (i + s) % s;
}

/* Exit to */
const CMenu::SOption CMenu::_exitTo[4] = {
	{ "menu", L"Wii Menu" },
	{ "homebrew", L"Homebrew" },	
	{ "shutdown", L"Shutdown" },
	{ "wiiu", L"WiiU" },
};

void CMenu::_showConfigBoot(bool instant)
{
	_hideCheckboxes(true); // reset checkboxes
	
	m_btnMgr.setText(m_configLblTitle, _t("cfg791", L"Startup and shutdown"));
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_configBtnBack);
	if(useMetaArgIOS)
	{
		m_btnMgr.setText(m_configLblNotice, _t("cfgbt99", L"IOS settings forced\n in meta.xml!"));
		m_btnMgr.show(m_configLblNotice);
	}
	
	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);
	
	int i;

	//! Force cIOS revision for WiiFlow startup and all games
	m_btnMgr.setText(m_configLbl[2], _t("cfgbt3", L"Force cIOS revision"));
	if(cur_ios > 0)
		m_btnMgr.setText(m_configLblVal[2], wfmt(L"%i", cur_ios));
	else
		m_btnMgr.setText(m_configLblVal[2], _t("def", L"Default")); // cIOS 249 unless the user changed it via the meta.xml
	//! Set USB port 0 or 1
	m_btnMgr.setText(m_configLbl[3], _t("cfgbt4", L"USB port"));
	m_btnMgr.setText(m_configLblVal[3], wfmt(L"%i", set_port));
	//! Default exit (if HOME pressed in exit menu or if child lock)
	m_btnMgr.setText(m_configLbl[4], _t("cfgc1", L"Default exit to")); // default exit
	i = min(max(0, m_cfg.getInt(general_domain, "exit_to", 0)), (int)ARRAY_SIZE(CMenu::_exitTo) - 1 - !IsOnWiiU());
	m_btnMgr.setText(m_configLblVal[4], _t(CMenu::_exitTo[i].id, CMenu::_exitTo[i].text));
	//! Boot cIOS on startup
	m_btnMgr.setText(m_configLbl[5], _t("cfgbt2", L"Boot Wiiflow using cIOS"));
	m_checkboxBtn[5] = cur_load == 0 ? m_configChkOff[5] : m_configChkOn[5];
	//! Mount SD only
	m_btnMgr.setText(m_configLbl[6], _t("cfg719", L"Mount SD only"));
	m_checkboxBtn[6] = cur_load == 0 ? m_configChkOff[6] : m_configChkOn[6];
	//! Disable exit
	m_btnMgr.setText(m_configLbl[7], _t("cfg720", L"Disable exit"));
	m_checkboxBtn[7] = m_cfg.getBool(general_domain, "disable_exit", false) ? m_configChkOn[7] : m_configChkOff[7];
	//! Force standby mode if WC24 enabled (ignore idle)
	m_btnMgr.setText(m_configLbl[8], _t("cfg725", L"Ignore idle shutdown"));
	m_checkboxBtn[8] = m_cfg.getOptBool(general_domain, "force_standby", 0) == 0 ? m_configChkOff[8] : m_configChkOn[8];

	for(i = 2; i < 9; ++i)
		m_btnMgr.show(m_configLbl[i], instant);
	for(i = 2; i < 5; ++i)
	{
		m_btnMgr.show(m_configLblVal[i], instant);
		m_btnMgr.show(m_configBtnM[i], instant);
		m_btnMgr.show(m_configBtnP[i], instant);
	}
	for(i = 5; i < 9; ++i)
		m_btnMgr.show(m_checkboxBtn[i], instant);
}

void CMenu::_configBoot(void)
{
	if(isWiiVC)
		return;
	
	bool prev_sd = sdOnly;
	bool prev_load = cur_load;
	u8 prev_ios = cur_ios;

	set_port = currentPort;
	
	SetupInput();
	_showConfigBoot();

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
			else if((m_btnMgr.selected(m_configBtnM[2]) || m_btnMgr.selected(m_configBtnP[2])) && !useMetaArgIOS) // cios rev
			{
				bool increase = m_btnMgr.selected(m_configBtnP[2]);
				CIOSItr itr = _installed_cios.find(cur_ios);
				if(increase)
				{
					itr++;
					if(itr == _installed_cios.end())
						itr = _installed_cios.begin();
				}
				else
				{
					if(itr == _installed_cios.begin())
						itr = _installed_cios.end();
					itr--;
				}
				cur_ios = itr->first;
				_showConfigBoot(true);
			}
			else if(m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3])) // usb port
			{
				set_port = !set_port;
				_showConfigBoot(true);
			}
			else if(m_btnMgr.selected(m_configBtnP[4]) || m_btnMgr.selected(m_configBtnM[4])) // default exit
			{
				s8 direction = m_btnMgr.selected(m_configBtnP[4]) ? 1 : -1;
				int exit_to = (int)loopNum(m_cfg.getUInt(general_domain, "exit_to", 0) + direction, ARRAY_SIZE(CMenu::_exitTo) - !IsOnWiiU());
				m_cfg.setInt(general_domain, "exit_to", exit_to);
				Sys_ExitTo(exit_to);
				_showConfigBoot(true);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[5]) && !useMetaArgIOS) // load cIOS on startup
			{
				cur_load = !cur_load;
				_showConfigBoot(true);
				m_btnMgr.setSelected(m_checkboxBtn[5]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[6])) // sd only
			{
				sdOnly = !sdOnly;
				_showConfigBoot(true);
				m_btnMgr.setSelected(m_checkboxBtn[6]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[7])) // disable exit
			{
				m_cfg.setBool(general_domain, "disable_exit", !m_cfg.getBool(general_domain, "disable_exit"));
				_showConfigBoot(true);
				m_btnMgr.setSelected(m_checkboxBtn[7]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[8])) // force standby (if wc24)
			{
				m_cfg.setBool(general_domain, "force_standby", !m_cfg.getBool(general_domain, "force_standby"));
				_showConfigBoot(true);
				m_btnMgr.setSelected(m_checkboxBtn[8]);
			}
		}
	}
	if(prev_load != cur_load || prev_ios != cur_ios)
		InternalSave.SaveIOS();
	if(set_port != currentPort)
		InternalSave.SavePort(set_port);
	if(prev_sd != sdOnly)
		InternalSave.SaveSDOnly();

	if(prev_sd == false && sdOnly == true) // sd only has been turned on, then let's set all partitions to sd
	{
		const char *domains[] = {WII_DOMAIN, GC_DOMAIN, CHANNEL_DOMAIN, PLUGIN_DOMAIN, HOMEBREW_DOMAIN};
		for(int i = 0; i < 5; i++)
			m_cfg.setInt(domains[i], "partition", SD);
	}	
	
	if(prev_load != cur_load || prev_ios != cur_ios || set_port != currentPort || prev_sd != sdOnly)
	{
		_error(_t("errboot99", L"Delete WiiFlow Wii save if it doesn't reboot."));
		m_exit = true;
		m_reload = true;
	}
	
	_hideConfig(true);
	m_btnMgr.hide(m_configLblNotice, true);
}
