
#include "menu.hpp"

void CMenu::_showConfigScreen(bool instant)
{
	m_btnMgr.setText(m_configLblTitle, _t("cfg793", L"Screen adjustment"));
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_configBtnBack);

	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);

	//! TV width
	m_btnMgr.setText(m_configLbl[2], _t("cfgc2", L"Adjust TV width"));
	m_btnMgr.setText(m_configLblVal[2], wfmt(L"%i", 640 * 640 / max(1, m_cfg.getInt(general_domain, "tv_width", 640))));	
	//! TV height
	m_btnMgr.setText(m_configLbl[3], _t("cfgc3", L"Adjust TV height"));
	m_btnMgr.setText(m_configLblVal[3], wfmt(L"%i", 480 * 480 / max(1, m_cfg.getInt(general_domain, "tv_height", 480))));
	//! X offset
	m_btnMgr.setText(m_configLbl[4], _t("cfgc6", L"Horizontal offset"));
	m_btnMgr.setText(m_configLblVal[4], wfmt(L"%i", -m_cfg.getInt(general_domain, "tv_x", 0)));
	//! Y offset
	m_btnMgr.setText(m_configLbl[5], _t("cfgc7", L"Vertical offset"));
	m_btnMgr.setText(m_configLblVal[5], wfmt(L"%i", m_cfg.getInt(general_domain, "tv_y", 0)));
	//! Screensaver delay
	m_btnMgr.setText(m_configLbl[6], _t("cfg712", L"Screensaver idle seconds"));
	m_btnMgr.setText(m_configLblVal[6], m_screensaverDelay == 0 ? _t("off", L"Off") : wfmt(L"%i", m_screensaverDelay));

	for(u8 i = 2; i < 7; ++i)
	{
		m_btnMgr.show(m_configLbl[i], instant);
		m_btnMgr.show(m_configLblVal[i], instant);
		m_btnMgr.show(m_configBtnM[i], instant);
		m_btnMgr.show(m_configBtnP[i], instant);
	}
}

void CMenu::_configScreen(void)
{
	SetupInput();
	_showConfigScreen();

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
			else if(m_btnMgr.selected(m_configBtnP[2]) || m_btnMgr.selected(m_configBtnM[2])
				|| m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3])
				|| m_btnMgr.selected(m_configBtnP[4]) || m_btnMgr.selected(m_configBtnM[4])
				|| m_btnMgr.selected(m_configBtnP[5]) || m_btnMgr.selected(m_configBtnM[5]))
			{
				int step = 0;
				
				if(m_btnMgr.selected(m_configBtnM[2]) || m_btnMgr.selected(m_configBtnM[3])) // TV width / height left
					step = 2;
					
				else if(m_btnMgr.selected(m_configBtnP[2]) || m_btnMgr.selected(m_configBtnP[3])) // TV width / height right
					step = -2;
					
				else if(m_btnMgr.selected(m_configBtnP[4]) || m_btnMgr.selected(m_configBtnM[5])) // H offset right / V offset left
					step = -1;
					
				else if(m_btnMgr.selected(m_configBtnM[4]) || m_btnMgr.selected(m_configBtnP[5])) // H offset left / V offset right
					step = 1;
					
				if(m_btnMgr.selected(m_configBtnM[2]) || m_btnMgr.selected(m_configBtnP[2])) // TV width
					m_cfg.setInt(general_domain, "tv_width", min(max(512, m_cfg.getInt(general_domain, "tv_width", 640) + step), 800));
					
				else if(m_btnMgr.selected(m_configBtnM[3]) || m_btnMgr.selected(m_configBtnP[3])) // TV height
					m_cfg.setInt(general_domain, "tv_height", min(max(384, m_cfg.getInt(general_domain, "tv_height", 480) + step), 600));
					
				else if(m_btnMgr.selected(m_configBtnP[4]) || m_btnMgr.selected(m_configBtnM[4])) // horizontal offset
					m_cfg.setInt(general_domain, "tv_x", min(max(-50, m_cfg.getInt(general_domain, "tv_x", 0) + step), 50));
					
				else if(m_btnMgr.selected(m_configBtnP[5]) || m_btnMgr.selected(m_configBtnM[5])) // vertical offset
					m_cfg.setInt(general_domain, "tv_y", min(max(-30, m_cfg.getInt(general_domain, "tv_y", 0) + step), 30));
					
				_showConfigScreen(true);
				
				m_vid.set2DViewport(m_cfg.getInt(general_domain, "tv_width", 640), m_cfg.getInt(general_domain, "tv_height", 480), m_cfg.getInt(general_domain, "tv_x", 0), m_cfg.getInt(general_domain, "tv_y", 0));
			}
			else if(m_btnMgr.selected(m_configBtnP[6]) || m_btnMgr.selected(m_configBtnM[6])) // screensaver delay
			{
				s8 direction = m_btnMgr.selected(m_configBtnP[6]) ? 5 : -5;
				int val = m_screensaverDelay + direction;
				if(val >= 0 && val <= 255)
				{
					m_screensaverDelay = val;
					m_cfg.setInt(general_domain, "screensaver_idle_seconds", val);
				}
				_showConfigScreen(true);
			}
		}
	}
	_hideConfig(true);
}
