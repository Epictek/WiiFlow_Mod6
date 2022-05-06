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
#include "network/ftp.h"
#include "network/https.h" // s
#include "network/FTP_Dir.hpp"
// #include "network/net.h"
// #include "network/gcard.h"

s16 m_ftpLblInfo;

void CMenu::_updateFTP(void)
{
	m_btnMgr.hide(m_ftpLblInfo, true);
	_hideConfig(true);
	if(m_ftp_inited)
	{
		in_addr addr;
		addr.s_addr = net_gethostip();
		m_btnMgr.setText(m_configLblTitle, wfmt(_t("dlmsg28", L"Running FTP Server on %s:%d"), inet_ntoa(addr), ftp_server_port));
		m_btnMgr.setText(m_configBtnCenter, _t("ftp2", L"Stop"));
		m_btnMgr.show(m_ftpLblInfo);
	}
	else
	{
		m_btnMgr.setText(m_configLblTitle, _t("dlmsg29", L"FTP Server is currently stopped"));
		m_btnMgr.setText(m_configBtnCenter, _t("ftp1", L"Start"));
	}
	_showFTP();
}

void CMenu::_FTP(void)
{
	_updateFTP();
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
				if(m_ftp_inited)
				{
					ftp_endThread();
					m_init_ftp = false;
					m_ftp_inited = false;
					ftp_dbg_print_update();
				}
				else
				{
					m_init_ftp = true;
					if(!networkInit)
						_netInit();
					else
						m_ftp_inited = ftp_startThread();
				}
				_updateFTP();
			}
		}
		if(ftp_dbg_print_update())
		{
			m_btnMgr.setText(m_ftpLblInfo, wfmt(L"%s%s%s%s%s%s", ftp_get_prints(5), ftp_get_prints(4), 
				ftp_get_prints(3), ftp_get_prints(2), ftp_get_prints(1), ftp_get_prints(0)));
		}
	}
	m_btnMgr.hide(m_ftpLblInfo, true);
	_hideConfig(true);
}

void CMenu::_showFTP(void)
{
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_configBtnCenter);
	m_btnMgr.show(m_configBtnBack);
	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);
}

void CMenu::_initFTP(void)
{
	m_ftpLblInfo = _addText("FTP/INFO", theme.txtFont, L"", 40, 165, 560, 270, theme.txtFontColor, FTGX_JUSTIFY_LEFT | FTGX_ALIGN_TOP);

	_setHideAnim(m_ftpLblInfo, "FTP/INFO", 0, 100, 0.f, 0.f);

	// _hideFTP(true);
}

