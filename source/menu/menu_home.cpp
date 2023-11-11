/****************************************************************************
 * Copyright (C) 2013 FIX94
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#include "menu.hpp"
#include "loader/nk.h"
// #include "wstringEx/wstringEx.hpp"
// #include "channel/nand.hpp"
// #include "loader/cios.h"

/* home menu */
s16 m_homeLblUser[4];
s16 m_homeBtnHelp;
s16 m_homeBtnConfig;
s16 m_homeBtnClose;
s16 m_homeBtnExitTo;
s16 m_homeLblBattery;

/* exit to menu */
s16 m_exittoLblUser[4];
s16 m_homeBtnExitToHBC;
s16 m_homeBtnExitToMenu;
s16 m_homeBtnExitToPriiloader;
s16 m_homeBtnExitToBootmii;

s16 m_homeLblHeaderOff;
s16 m_homeLblHeaderOn;
s16 m_homeLblFooterOff;
s16 m_homeLblFooterOn;

static const wstringEx PLAYER_BATTERY_LABEL("P1 %003.f%% | P2 %003.f%% | P3 %003.f%% | P4 %003.f%%");

bool CMenu::_Home(void)
{
	SetupInput();
	_showHome();
	enlargeButtons = true;

	if(theme.homeSound != NULL) // added
		theme.homeSound->Play(255);

	while(!m_exit)
	{
		/* Battery gets refreshed in here... */
		_mainLoopCommon(true);
		/* and it always changes so... */
		m_btnMgr.setText(m_homeLblBattery, wfmt(PLAYER_BATTERY_LABEL, min((float)wd[0]->battery_level, 100.f), 
			min((float)wd[1]->battery_level, 100.f), min((float)wd[2]->battery_level, 100.f), min((float)wd[3]->battery_level, 100.f)));
		if(BTN_HOME_PRESSED)
		{
			if(isWiiVC)
				exitHandler(EXIT_TO_MENU);
			else
				exitHandler(WIIFLOW_DEF); // exit to option in startup and shutdown settings
			break;
		}
		else if(BTN_B_OR_1_PRESSED)
			break;
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
			m_btnMgr.up(true); // true = enlarge
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
			m_btnMgr.down(true); // true = enlarge
		else if(BTN_A_OR_2_PRESSED)
		{
			if(m_btnMgr.selected(m_homeBtnExitTo))
			{
				_hideHome();
				if(isWiiVC)
				{
					exitHandler(EXIT_TO_MENU);
					break;
				}
				if(m_locked)
				{
					exitHandler(WIIFLOW_DEF);
					break;
				}
				else 
					_ExitTo();
				_showHome();
			}
			else if(m_btnMgr.selected(m_homeBtnHelp))
			{
				_hideHome();
				enlargeButtons = false;
				_about(true);
				break;
			}
			else if(m_btnMgr.selected(m_homeBtnConfig))
			{
				_hideHome();
				enlargeButtons = false;
				CoverFlow.fade(2);
				_config();
				CoverFlow.fade(0);
				break;
			}
			else if(m_btnMgr.selected(m_homeBtnClose))
				break;
		}
		
		/* Show header and footer */
		if(m_show_zone_header)
		{
			m_btnMgr.show(m_homeLblHeaderOff, true);
			m_btnMgr.show(m_homeLblHeaderOn, true);
		}
		else
			m_btnMgr.hide(m_homeLblHeaderOn);
		
		if(m_show_zone_footer)
		{
			m_btnMgr.show(m_homeLblFooterOff, true);
			m_btnMgr.show(m_homeLblFooterOn, true);
			m_btnMgr.show(m_homeLblBattery, true);
		}
		else
			m_btnMgr.hide(m_homeLblFooterOn);
	}
	
	enlargeButtons = false;
	_hideHome();
	
	return m_exit;
}

bool CMenu::_ExitTo(void)
{
	SetupInput();
	_showExitTo();

	while(!m_exit)
	{
		_mainLoopCommon(true);
		if(BTN_HOME_PRESSED)
		{
			exitHandler(WIIFLOW_DEF);
			break;
		}
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
			m_btnMgr.up(true); // true = enlarge
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
			m_btnMgr.down(true); // true = enlarge
		else if(BTN_B_OR_1_PRESSED)
			break;
		else if(BTN_A_OR_2_PRESSED) // note exitHandler sets m_exit = true
		{
			if(m_btnMgr.selected(m_homeBtnExitToMenu))
			{
				exitHandler(EXIT_TO_MENU);
				break;
			}
			else if(m_btnMgr.selected(m_homeBtnExitToHBC))
			{
				if(neek2o())
					exitHandler(EXIT_TO_SMNK2O); // restart wii when in neek2o mode
				else
					exitHandler(EXIT_TO_HBC);
				break;
			}
			else if(m_btnMgr.selected(m_homeBtnExitToPriiloader))
			{
				if(IsOnWiiU())
					exitHandler(EXIT_TO_WIIU);
				else if(neek2o())
					break;
				else
					exitHandler(EXIT_TO_PRIILOADER);
				break;
			}
			else if(m_btnMgr.selected(m_homeBtnExitToBootmii))
			{
				if(IsOnWiiU() || neek2o())
					exitHandler(POWEROFF_CONSOLE);
				else
					exitHandler(EXIT_TO_BOOTMII);
				break;
			}
		}
	}
	_hideExitTo();
	return m_exit;
}

void CMenu::_showHome(void)
{
	m_btnMgr.show(m_homeBtnExitTo);
	m_btnMgr.show(m_homeBtnHelp);
	m_btnMgr.show(m_homeBtnConfig);
	m_btnMgr.show(m_homeBtnClose);
	if(!m_show_zone_footer)
		m_btnMgr.show(m_homeLblBattery);
	
	/* Show header and footer in "OFF" mode */
	m_btnMgr.show(m_homeLblHeaderOff);
	m_btnMgr.show(m_homeLblFooterOff);

	for(u8 i = 0; i < ARRAY_SIZE(m_homeLblUser); ++i)
		if(m_homeLblUser[i] != -1)
			m_btnMgr.show(m_homeLblUser[i]);
}

void CMenu::_showExitTo(void)
{
	m_btnMgr.show(m_homeBtnExitToMenu);
	m_btnMgr.show(m_homeBtnExitToHBC); // restart if neek
	m_btnMgr.show(m_homeBtnExitToPriiloader); // exit to WiiU if vWii - back if neek
	m_btnMgr.show(m_homeBtnExitToBootmii); // shutdown to WiiU if vWii or neek
	
	/* Keep header and footer */
	m_btnMgr.show(m_homeLblHeaderOff);
	m_btnMgr.show(m_homeLblFooterOff);	
	m_btnMgr.show(m_homeLblBattery);
	
	for(u8 i = 0; i < ARRAY_SIZE(m_exittoLblUser); ++i)
		if(m_exittoLblUser[i] != -1)
			m_btnMgr.show(m_exittoLblUser[i]);
}

void CMenu::_hideHome(bool instant)
{
	m_btnMgr.hide(m_homeBtnExitTo, instant);
	m_btnMgr.hide(m_homeBtnHelp, instant);
	m_btnMgr.hide(m_homeBtnConfig, instant);
	m_btnMgr.hide(m_homeBtnClose, instant);
	m_btnMgr.hide(m_homeLblBattery, instant);
	
	m_btnMgr.hide(m_homeLblHeaderOff, instant);
	m_btnMgr.hide(m_homeLblHeaderOn, instant);
	m_btnMgr.hide(m_homeLblFooterOff, instant);
	m_btnMgr.hide(m_homeLblFooterOn, instant);

	for(u8 i = 0; i < ARRAY_SIZE(m_homeLblUser); ++i)
		if(m_homeLblUser[i] != -1)
			m_btnMgr.hide(m_homeLblUser[i], instant);
}

void CMenu::_hideExitTo(bool instant)
{
	m_btnMgr.hide(m_homeBtnExitToMenu, instant);
	m_btnMgr.hide(m_homeBtnExitToHBC, instant);
	m_btnMgr.hide(m_homeBtnExitToPriiloader, instant);
	m_btnMgr.hide(m_homeBtnExitToBootmii, instant);
	
	for(u8 i = 0; i < ARRAY_SIZE(m_exittoLblUser); ++i)
		if(m_exittoLblUser[i] != -1)
			m_btnMgr.hide(m_exittoLblUser[i], instant);
}

void CMenu::_initHomeAndExitToMenu()
{
	/* Home Menu */
	_addUserLabels(m_homeLblUser, ARRAY_SIZE(m_homeLblUser), "HOME");

	TexData texHeader;
	TexData texHeaders;
	TexData texFooter;
	TexData texFooters;
	
	m_homeLblHeaderOff = _addLabel("HOME/HEADER_OFF", theme.lblFont, L"", 0, 0, 640, 74, theme.txtFontColor, 0, texHeader);
	m_homeLblHeaderOn = _addLabel("HOME/HEADER_ON", theme.lblFont, L"", 0, 0, 640, 74, theme.txtFontColor, 0, texHeaders);
	m_homeLblFooterOff = _addLabel("HOME/FOOTER_OFF", theme.lblFont, L"", 0, 366, 640, 114, theme.txtFontColor, 0, texFooter);
	m_homeLblFooterOn = _addLabel("HOME/FOOTER_ON", theme.lblFont, L"", 0, 366, 640, 114, theme.txtFontColor, 0, texFooters);

	m_homeBtnExitTo = _addButton("HOME/EXIT_TO", theme.btnFont, L"", 60, 180, 250, 48, theme.btnFontColor);
	m_homeBtnHelp = _addButton("HOME/HELP", theme.btnFont, L"", 330, 180, 250, 48, theme.btnFontColor);
	m_homeBtnConfig = _addButton("HOME/CONFIG", theme.btnFont, L"", 60, 260, 250, 48, theme.btnFontColor);
	m_homeBtnClose = _addButton("HOME/CLOSE", theme.btnFont, L"", 330, 260, 250, 48, theme.btnFontColor);
	
	m_homeLblBattery = _addLabel("HOME/BATTERY", theme.txtFont, L"", 0, 425, 640, 48, theme.txtFontColor, FTGX_JUSTIFY_CENTER | FTGX_ALIGN_MIDDLE);

	/* Header and footer zones */
	m_homeButtonsHeader.x = m_theme.getInt("HOME/ZONES", "header_x", 0);
	m_homeButtonsHeader.y = m_theme.getInt("HOME/ZONES", "header_y", 0);
	m_homeButtonsHeader.w = m_theme.getInt("HOME/ZONES", "header_w", 640);
	m_homeButtonsHeader.h = m_theme.getInt("HOME/ZONES", "header_h", 74);
	m_homeButtonsHeader.hide = m_theme.getBool("HOME/ZONES", "header_hide", true);

	m_homeButtonsFooter.x = m_theme.getInt("HOME/ZONES", "footer_x", 0);
	m_homeButtonsFooter.y = m_theme.getInt("HOME/ZONES", "footer_y", 366);
	m_homeButtonsFooter.w = m_theme.getInt("HOME/ZONES", "footer_w", 640);
	m_homeButtonsFooter.h = m_theme.getInt("HOME/ZONES", "footer_h", 114);
	m_homeButtonsFooter.hide = m_theme.getBool("HOME/ZONES", "footer_hide", true);

	_setHideAnim(m_homeBtnExitTo, "HOME/EXIT_TO", 50, 0, 1.f, 0.f);
	_setHideAnim(m_homeBtnHelp, "HOME/HELP", -50, 0, 1.f, 0.f);
	_setHideAnim(m_homeBtnConfig, "HOME/CONFIG", 50, 0, 1.f, 0.f);
	_setHideAnim(m_homeBtnClose, "HOME/CLOSE", -50, 0, 1.f, 0.f);
	
	_setHideAnim(m_homeLblBattery, "HOME/BATTERY", 0, 0, -2.f, 0.f);
	
	_setHideAnim(m_homeLblHeaderOff, "HOME/HEADER_OFF", 0, -200, 0.f, 0.f);
	_setHideAnim(m_homeLblHeaderOn, "HOME/HEADER_ON", 0, 0, 0.f, 0.f);
	_setHideAnim(m_homeLblFooterOff, "HOME/FOOTER_OFF", 0, 200, 0.f, 0.f);
	_setHideAnim(m_homeLblFooterOn, "HOME/FOOTER_ON", 0, 0, 0.f, 0.f);

	_textHome();
	// _hideHome(true);
	
	/* ExitTo Menu */
	_addUserLabels(m_exittoLblUser, ARRAY_SIZE(m_exittoLblUser), "EXIT_TO");
	m_homeBtnExitToMenu = _addButton("EXIT_TO/MENU", theme.btnFont, L"", 185, 120, 270, 48, theme.btnFontColor);
	m_homeBtnExitToHBC = _addButton("EXIT_TO/HBC", theme.btnFont, L"", 185, 180, 270, 48, theme.btnFontColor);
	m_homeBtnExitToPriiloader = _addButton("EXIT_TO/PRIILOADER", theme.btnFont, L"", 185, 240, 270, 48, theme.btnFontColor);
	m_homeBtnExitToBootmii = _addButton("EXIT_TO/BOOTMII", theme.btnFont, L"", 185, 300, 270, 48, theme.btnFontColor);

	_setHideAnim(m_homeBtnExitToMenu, "EXIT_TO/MENU", 0, 0, -4.f, 0.f);
	_setHideAnim(m_homeBtnExitToHBC, "EXIT_TO/HBC", 0, 0, -4.f, 0.f);
	_setHideAnim(m_homeBtnExitToPriiloader, "EXIT_TO/PRIILOADER", 0, 0, -4.f, 0.f);
	_setHideAnim(m_homeBtnExitToBootmii, "EXIT_TO/BOOTMII", 0, 0, -4.f, 0.f);

	_textExitTo();
	// _hideExitTo(true);
}

void CMenu::_textHome(void)
{
	if(isWiiVC || m_locked)
		m_btnMgr.setText(m_homeBtnExitTo, _t("home12", L"Exit"));
	else
		m_btnMgr.setText(m_homeBtnExitTo, _t("home5", L"Exit to"));
	m_btnMgr.setText(m_homeBtnHelp, _t("about10", L"Help guide"));
	m_btnMgr.setText(m_homeBtnConfig, _t("cfg1", L"Settings"));
	m_btnMgr.setText(m_homeBtnClose, _t("back", L"Back"));
}

void CMenu::_textExitTo(void)
{
	bool nk = neek2o();
	bool wu = IsOnWiiU();

	m_btnMgr.setText(m_homeBtnExitToMenu, nk ? _t("neek2", L"Neek2o system menu") : _t("menu", L"Wii Menu"));
	m_btnMgr.setText(m_homeBtnExitToHBC, nk ? _t("neek3", L"Restart Wii") : _t("hbc", L"Homebrew Channel"));
	m_btnMgr.setText(m_homeBtnExitToPriiloader, nk ? _t("back", L"Back") : wu ? _t("wiiu", L"WiiU") : _t("prii", L"Priiloader"));
	m_btnMgr.setText(m_homeBtnExitToBootmii, (nk || wu) ? _t("shutdown", L"Shutdown") : _t("bootmii", L"Bootmii"));
}

