
#include "menu.hpp"

void CMenu::_showCoverBanner(void)
{
	m_btnMgr.setText(m_configLblTitle, _t("cfgg40", L"Manage cover and banner"));
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_configBtnBack);
	m_btnMgr.setText(m_configBtnCenter, _t("cfg837", L"Options"));
	m_btnMgr.show(m_configBtnCenter);

	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);

	m_btnMgr.setText(m_configLbl[2], _t("cfgbnr1", L"Download cover"));
	m_btnMgr.setText(m_configLbl[3], _t("cfgbnr2", L"Delete cover"));
	m_btnMgr.setText(m_configLbl[4], _t("cfgbnr3", L"Download custom banner"));
	m_btnMgr.setText(m_configLbl[5], _t("cfgbnr4", L"Delete banner"));
	m_btnMgr.setText(m_configLbl[6], _t("cfgbnr99", L"Download disc label"));
	m_btnMgr.setText(m_configLbl[7], _t("cfgbnr98", L"Delete disc label"));

	for(u8 i = 2; i < 7; i = i + 2)
		m_btnMgr.setText(m_configBtn[i], _t("cfg4", L"Download"));
	for(u8 i = 3; i < 8; i = i + 2)
		m_btnMgr.setText(m_configBtn[i], _t("cfgbnr6", L"Delete"));
	
	bool chan = (m_current_view & COVERFLOW_CHANNEL) ? true : false;
	for(u8 i = 2; i < 8 - (chan*2); ++i)
	{
		m_btnMgr.show(m_configLbl[i]);
		m_btnMgr.show(m_configBtn[i]);
	}
}

void CMenu::_CoverBanner(void)
{
	const char *id = CoverFlow.getId();
	
	SetupInput();	
	_showCoverBanner();
	
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
			else if(m_btnMgr.selected(m_configBtnCenter)) // download settings
			{
				_hideConfig(true);
				_configDownload(true);
				_showCoverBanner();
			}
			else if(m_btnMgr.selected(m_configBtn[2])) // download cover
			{
				_hideConfig(true);
				_download(id, 1);
				_showCoverBanner();
			}
			else if(m_btnMgr.selected(m_configBtn[3])) // delete cover
			{
				if(error(_t("errcfg5", L"Are you sure?"), true))
				{
					RemoveCover(id);
					error(_t("dlmsg14", L"Done."));
				}
				_showCoverBanner();
			}
			else if(m_btnMgr.selected(m_configBtn[4])) // download custom banner
			{
				_hideConfig(true);
				_download(id, 2);
				m_newGame = true;
				_showCoverBanner();
			}
			else if(m_btnMgr.selected(m_configBtn[5])) // delete custom banner
			{
				if(error(_t("errcfg5", L"Are you sure?"), true))
				{
					fsop_deleteFile(fmt("%s/%s.bnr", m_bnrCacheDir.c_str(), id));
					fsop_deleteFile(fmt("%s/%s.bnr", m_customBnrDir.c_str(), id));
					fsop_deleteFile(fmt("%s/%.3s.bnr", m_bnrCacheDir.c_str(), id));
					fsop_deleteFile(fmt("%s/%.3s.bnr", m_customBnrDir.c_str(), id));
					error(_t("dlmsg14", L"Done."));
					m_newGame = true;
				}
				_showCoverBanner();
			}
			else if(m_btnMgr.selected(m_configBtn[6])) // download disc label
			{
				_hideConfig(true);
				_download(id, 3);
				_showCoverBanner();
			}
			else if(m_btnMgr.selected(m_configBtn[7])) // delete disc label
			{
				if(error(_t("errcfg5", L"Are you sure?"), true))
				{
					fsop_deleteFile(fmt("%s/%s.png", m_cartDir.c_str(), id));
					error(_t("dlmsg14", L"Done."));
				}
				_showCoverBanner();
			}
		}
	}
	_hideConfig(true);
}
