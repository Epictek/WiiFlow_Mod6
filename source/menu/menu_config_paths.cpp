
#include "menu.hpp"

void CMenu::_showConfigPaths(bool instant)
{
	m_btnMgr.setText(m_configLblTitle, _t("cfgd4", L"Path manager"));
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_configBtnBack);
	
	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);

	m_btnMgr.setText(m_configLbl[0], _t("cfgp8", L"Box covers path"));
	m_btnMgr.setText(m_configLbl[1], _t("cfgp2", L"Flat covers path"));
	m_btnMgr.setText(m_configLbl[2], _t("cfgp9", L"Custom banners path"));
	m_btnMgr.setText(m_configLbl[3], _t("cfgp4", L"Banners cache path"));
	m_btnMgr.setText(m_configLbl[4], _t("cfgp99", L"Fanart path"));
	m_btnMgr.setText(m_configLbl[5], _t("cfgp98", L"Trailer path"));
	m_btnMgr.setText(m_configLbl[6], _t("cfgp97", L"Cheat codes (TXT) path"));
	m_btnMgr.setText(m_configLbl[7], _t("cfgp96", L"Plugin cartridge image path"));
	m_btnMgr.setText(m_configLbl[8], _t("cfgp95", L"Plugin game snapshot path"));
	m_btnMgr.setText(m_configLbl[9], _t("cfgp94", L"Game save backup path"));
	
	for(u8 i = 0; i < 10; ++i)
	{
		m_btnMgr.show(m_configLbl[i], instant);
		m_btnMgr.show(m_configBtnGo[i], instant);
	}
}

void CMenu::_configPaths(void)
{
	SetupInput();
	_showConfigPaths();

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
			
			else if(m_btnMgr.selected(m_configBtnGo[0])) // box covers
			{
				_hideConfig(true);
				const char *path = NULL;
				path = _FolderExplorer(m_boxPicDir.c_str());
				if(strlen(path) > 0)
				{
					m_cfg.setString(general_domain, "dir_box_covers", path);
					m_boxPicDir = path;
					_initCF();
				}
				_showConfigPaths();
			}
			else if(m_btnMgr.selected(m_configBtnGo[1])) // flat covers
			{
				_hideConfig(true);
				const char *path = NULL;
				path = _FolderExplorer(m_picDir.c_str());
				if(strlen(path) > 0)
				{
					m_cfg.setString(general_domain, "dir_flat_covers", path);
					m_picDir = path;
					_initCF();
				}
				_showConfigPaths();
			}
			else if(m_btnMgr.selected(m_configBtnGo[2])) // custom banner
			{
				_hideConfig(true);
				const char *path = NULL;
				path = _FolderExplorer(m_customBnrDir.c_str());
				if(strlen(path) > 0)
				{
					m_cfg.setString(general_domain, "dir_custom_banners", path);
					m_customBnrDir = path;
				}
				_showConfigPaths();
			}
			else if(m_btnMgr.selected(m_configBtnGo[3])) // cache banner
			{
				_hideConfig(true);
				const char *path = NULL;
				path = _FolderExplorer(m_bnrCacheDir.c_str());
				if(strlen(path) > 0)
				{
					m_cfg.setString(general_domain, "dir_banner_cache", path);
					m_bnrCacheDir = path;
				}
				_showConfigPaths();
			}			
			else if(m_btnMgr.selected(m_configBtnGo[4])) // fanart
			{
				_hideConfig(true);
				const char *path = NULL;
				path = _FolderExplorer(m_fanartDir.c_str());
				if(strlen(path) > 0)
				{
					m_cfg.setString(general_domain, "dir_fanart", path);
					m_fanartDir = path;
				}
				_showConfigPaths();
			}
			else if(m_btnMgr.selected(m_configBtnGo[5])) // trailer
			{
				_hideConfig(true);
				const char *path = NULL;
				path = _FolderExplorer(m_videoDir.c_str());
				if(strlen(path) > 0)
				{
					m_cfg.setString(general_domain, "dir_trailers", path);
					m_videoDir = path;
				}
				_showConfigPaths();
			}
			else if(m_btnMgr.selected(m_configBtnGo[6])) // cheat codes
			{
				_hideConfig(true);
				const char *path = NULL;
				path = _FolderExplorer(m_txtCheatDir.c_str());
				if(strlen(path) > 0)
				{
					m_cfg.setString(general_domain, "dir_txtcheat", path);
					m_txtCheatDir = path;
				}
				_showConfigPaths();
			}
			else if(m_btnMgr.selected(m_configBtnGo[7])) // cartdisk
			{
				_hideConfig(true);
				const char *path = NULL;
				path = _FolderExplorer(m_cartDir.c_str());
				if(strlen(path) > 0)
				{
					m_cfg.setString(general_domain, "dir_cart", path);
					m_cartDir = path;
				}
				_showConfigPaths();
			}			
			else if(m_btnMgr.selected(m_configBtnGo[8])) // plugin snapshots
			{
				_hideConfig(true);
				const char *path = NULL;
				path = _FolderExplorer(m_snapDir.c_str());
				if(strlen(path) > 0)
				{
					m_cfg.setString(general_domain, "dir_snap", path);
					m_snapDir = path;
				}
				_showConfigPaths();
			}
			else if(m_btnMgr.selected(m_configBtnGo[9])) // backup folder
			{
				_hideConfig(true);
				const char *path = NULL;
				path = _FolderExplorer(m_backupDir.c_str());
				if(strlen(path) > 0)
				{
					m_cfg.setString(general_domain, "dir_backup", path);
					m_backupDir = path;
				}
				_showConfigPaths();
			}
		}
	}
	_hideConfig(true);
}

