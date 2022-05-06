
#include "menu.hpp"

#define NB_BUTTONS 3

static u8 curPage;
static u8 start_pos;
static u8 WiiBtn;
static u8 GCBtn;
static u8 ChanBtn;
static u8 HBBtn;
static u8 PlBtn;

template <class T> static inline T loopNum(T i, T s)
{
	return (i + s) % s;
}

void CMenu::_showConfigPlugin(bool instant)
{
	_hideCheckboxes(true); // reset checkboxes
	m_btnMgr.show(m_configBtnBack);

	//! check if wii or channel or gc or hb plugin is enabled to get the onscreen array size	
	u8 max_line = NB_BUTTONS - 1;
	bool wii_enabled = false;
	bool chan_enabled = false;
	bool gc_enabled= false;
	bool hb_enabled = false;
	bool pl_enabled = false;
	WiiBtn = 0;
	GCBtn = 0;
	ChanBtn = 0;
	HBBtn = 0;
	PlBtn = 0;
	
	if(m_plugin.GetEnabledStatus(m_plugin.GetPluginPosition(0x4E574949))) // plugin wii
	{
		++max_line;
		wii_enabled = true;
	}
	if(m_plugin.GetEnabledStatus(m_plugin.GetPluginPosition(0x454E414E)) ||
	m_plugin.GetEnabledStatus(m_plugin.GetPluginPosition(0x4E414E44))) // plugin channels (emunand + realnand)
	{
		++max_line;
		chan_enabled = true;
	}
	if(m_plugin.GetEnabledStatus(m_plugin.GetPluginPosition(0x4E47434D))) // plugin gamecube
	{
		++max_line;
		gc_enabled = true;
	}
	if(m_plugin.GetEnabledStatus(m_plugin.GetPluginPosition(0x48425257))) // plugin homebrew
	{
		++max_line;
		hb_enabled = true;
	}
	u8 pos = 0;
	if(enabledPluginsCount == 1)
	{
		while(m_plugin.PluginExist(pos) && !m_plugin.GetEnabledStatus(pos)) { ++pos; }
		u32 magic = m_plugin.GetPluginMagic(pos);
		//! if magic is not wii, gc, emunand, realnand, scumm or hb
		if(magic != 0x4E574949 && magic != 0x4E47434D && magic != 0x454E414E && magic != 0x4E414E44 && magic != 0x5343564D && strncasecmp(fmt("%06x", magic), "484252", 6) != 0)
		{
			++max_line;
			pl_enabled = true;
		}
	}
	
	start_pos = 4 - (max_line / 2);
	if(wii_enabled)
		WiiBtn = start_pos + NB_BUTTONS;
	if(chan_enabled)
		ChanBtn = start_pos + NB_BUTTONS + wii_enabled;
	if(gc_enabled)
		GCBtn = start_pos + NB_BUTTONS + wii_enabled + chan_enabled;
	if(hb_enabled)
		HBBtn = start_pos + NB_BUTTONS + wii_enabled + chan_enabled + gc_enabled;
	if(pl_enabled)
		PlBtn = start_pos + NB_BUTTONS + wii_enabled + chan_enabled + gc_enabled + hb_enabled;

	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);
	
	/** MAIN PAGE **/
	if(curPage == MAIN_SETTINGS)
	{
		m_btnMgr.setText(m_configLblTitle, _t("plugins", L"Plugins"));
		m_btnMgr.show(m_configLblTitle);
		
		//! File browser
		m_btnMgr.setText(m_configBtnCenter, _t("home8", L"File explorer"));
		m_btnMgr.show(m_configBtnCenter);

		m_btnMgr.setText(m_configLbl[start_pos], _t("cfgpl1", L"Select plugins"));		
		m_btnMgr.setText(m_configLbl[start_pos+1], _t("cfg816", L"Manage Plugin game list"));
		m_btnMgr.setText(m_configLbl[start_pos+2], _t("cfg727", L"Use plugin database titles"));
		m_checkboxBtn[start_pos+2] = m_cfg.getOptBool(plugin_domain, "database_titles", 1) == 0 ? m_configChkOff[start_pos+2] : m_configChkOn[start_pos+2]; // default true
		m_btnMgr.show(m_checkboxBtn[start_pos+2], instant);

		for(u8 i = start_pos; i < start_pos+3; ++i)
			m_btnMgr.show(m_configLbl[i], instant);
		
		for(u8 i = start_pos; i < start_pos+2; ++i)
			m_btnMgr.show(m_configBtnGo[i], instant);

		//! The following buttons will only show up if their corresponding plugin is selected
		if(wii_enabled) // shortcut to wii settings
		{
			m_btnMgr.setText(m_configLbl[WiiBtn], _t("wii", L"Wii"));
			m_btnMgr.show(m_configLbl[WiiBtn], instant);
			m_btnMgr.show(m_configBtnGo[WiiBtn], instant);
		}
		if(chan_enabled) // shortcut to channels settings
		{
			m_btnMgr.setText(m_configLbl[ChanBtn], _t("wiichannels", L"Wii channels"));
			m_btnMgr.show(m_configLbl[ChanBtn], instant);
			m_btnMgr.show(m_configBtnGo[ChanBtn], instant);
		}
		if(gc_enabled) // shortcut to gamecube settings
		{
			m_btnMgr.setText(m_configLbl[GCBtn], _t("gc", L"GameCube"));
			m_btnMgr.show(m_configLbl[GCBtn], instant);
			m_btnMgr.show(m_configBtnGo[GCBtn], instant);
		}
		if(hb_enabled) // shortcut to homebrew settings
		{
			m_btnMgr.setText(m_configLbl[HBBtn], _t("homebrew", L"Homebrew"));
			m_btnMgr.show(m_configLbl[HBBtn], instant);
			m_btnMgr.show(m_configBtnGo[HBBtn], instant);
		}
		if(pl_enabled) // shortcut to current plugin path
		{
			wstringEx pluginName = m_plugin.GetPluginName(pos);
			m_btnMgr.setText(m_configLbl[PlBtn], wfmt(_fmt("cfg823", L"%s rom path"), pluginName.toUTF8().c_str()));
			m_btnMgr.show(m_configLbl[PlBtn], instant);
			m_btnMgr.show(m_configBtnGo[PlBtn], instant);
		}
	}
	/** PLUGIN GAME LOCATION **/
	else if(curPage == GAME_LIST)
	{
		m_btnMgr.setText(m_configLblTitle, _t("cfg816", L"Manage Plugin game list"));
		m_btnMgr.show(m_configLblTitle);
		//! Plugin default partition
		m_btnMgr.setText(m_configLbl[3], _t("part4", L"Plugins default partition"));
		const char *partitionname = DeviceName[m_cfg.getInt(plugin_domain, "partition", 0)];
		m_btnMgr.setText(m_configLblVal[3], upperCase(partitionname));
		m_btnMgr.show(m_configLblVal[3], instant);
		m_btnMgr.show(m_configBtnM[3], instant);
		m_btnMgr.show(m_configBtnP[3], instant);		
		//! Plugin rom paths
		m_btnMgr.setText(m_configLbl[4], _t("smedit7", L"Custom rom paths"));
		m_btnMgr.show(m_configBtnGo[4], instant);
		//! Dump plugin game coverflow list
		m_btnMgr.setText(m_configLbl[5], _t("cfg783", L"Dump coverflow list"));
		m_btnMgr.setText(m_configBtn[5], _t("cfgne6", L"Start"));
		//! Refresh coverflow list and cover cache
		m_btnMgr.setText(m_configLbl[6], _t("home2", L"Refresh coverflow list"));
		m_btnMgr.setText(m_configBtn[6], _t("cfgne6", L"Start"));

		for(u8 i = 3; i < 7; ++i)
			m_btnMgr.show(m_configLbl[i], instant);
		for(u8 i = 5; i < 7; ++i)
			m_btnMgr.show(m_configBtn[i], instant);
	}
}

void CMenu::_configPlugin(u8 startPage)
{
	bool cur_db_titles = m_cfg.getBool(plugin_domain, "database_titles", true);
	bool prev_db_titles = cur_db_titles;
	curPage = startPage;
	start_pos = 0;

	_setBg(m_configBg, m_configBg);
	SetupInput();
	_showConfigPlugin();
	
	while(!m_exit)
	{
		_mainLoopCommon();
		if(BTN_HOME_HELD || (BTN_B_OR_1_PRESSED && (curPage == MAIN_SETTINGS || startPage == GAME_LIST)))
			break;
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
			m_btnMgr.up();
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
			m_btnMgr.down();
		else if(BTN_B_OR_1_PRESSED)
		{
			_hideConfig(true);
			curPage = MAIN_SETTINGS;
			 _showConfigPlugin();
		}
		else if(BTN_A_OR_2_PRESSED)
		{
			if((m_btnMgr.selected(m_configBtnBack) && curPage != MAIN_SETTINGS))
			{
				if(startPage == GAME_LIST)
					break;
				else
				{
					_hideConfig(true);
					curPage = MAIN_SETTINGS;
					 _showConfigPlugin();
				}
			}
			/** MAIN PAGE **/
			else if(curPage == MAIN_SETTINGS)
			{
				if(m_btnMgr.selected(m_configBtnBack))
					break;
				else if(m_btnMgr.selected(m_configBtnCenter)) // file explorer
				{
					_hideConfig(true);
					if(enabledPluginsCount == 1)
					{
						u8 i = 0;
						while(m_plugin.PluginExist(i) && !m_plugin.GetEnabledStatus(i)) { ++i; }
						u32 plmagic =  m_plugin.GetPluginMagic(i);
						_pluginExplorer(m_plugin.GetExplorerPath(plmagic), plmagic, false); // false = not from source
					}
					else
					{
						string gameDir(fmt("%s:/", DeviceName[m_cfg.getInt(plugin_domain, "partition", SD)]));
						_FileExplorer(gameDir.c_str());
					}
					_showConfigPlugin();
				}
				else
				{
					if(m_btnMgr.selected(m_configBtnGo[start_pos])) // select plugins
					{
						_hideConfig(true);
						_PluginSettings();
						_showConfigPlugin();
					}
					else if(m_btnMgr.selected(m_configBtnGo[start_pos+1])) //  game location
					{
						_hideConfig(true);
						curPage = GAME_LIST;
						_showConfigPlugin();
					}
					else if(m_btnMgr.selected(m_checkboxBtn[start_pos+2])) // plugin database titles
					{
						cur_db_titles = !cur_db_titles;
						m_cfg.setBool(plugin_domain, "database_titles", cur_db_titles);
						_showConfigPlugin(true);
						m_btnMgr.setSelected(m_checkboxBtn[start_pos+2]);
					}
					else if(m_btnMgr.selected(m_configBtnGo[WiiBtn])) // wii settings
					{
						_hideConfig(true);
						_configWii();
						_showConfigPlugin();
					}
					else if(m_btnMgr.selected(m_configBtnGo[ChanBtn])) // channels settings
					{
						_hideConfig(true);
						_configNandEmu();
						_showConfigPlugin();
					}
					else if(m_btnMgr.selected(m_configBtnGo[GCBtn])) // gamecube settings
					{
						_hideConfig(true);
						_configGC();
						_showConfigPlugin();
					}
					else if(m_btnMgr.selected(m_configBtnGo[HBBtn])) // homebrew settings
					{
						_hideConfig(true);
						_configHB();
						_showConfigPlugin();
					}
					else if(m_btnMgr.selected(m_configBtnGo[PlBtn])) // current plugin path
					{
						_hideConfig(true);
						u8 pos = 0;
						while(m_plugin.PluginExist(pos) && !m_plugin.GetEnabledStatus(pos)) { ++pos; }
						_setPluginPath(pos);
						_showConfigPlugin();
					}
				}
			}
			/** PLUGIN GAME LOCATION **/
			else if(curPage == GAME_LIST)
			{
				if(m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3])) // plugin partition
				{
					m_prev_view = m_current_view;
					u8 prevPartition = currentPartition;
					s8 direction = m_btnMgr.selected(m_configBtnP[3]) ? 1 : -1;
					currentPartition = m_cfg.getInt(plugin_domain, "partition");
					m_current_view = COVERFLOW_PLUGIN;
					_setPartition(direction);
					_showConfigPlugin(true);
					if(m_prev_view & COVERFLOW_PLUGIN)
						m_refreshGameList = true;
					m_current_view = m_prev_view;
					currentPartition = prevPartition;
				}
				else if(m_btnMgr.selected(m_configBtnGo[4])) // custom path
				{
					_hideConfig(true);
					_checkboxesMenu(3); // SM editor mode 3 (EDIT_PLUGIN_PATH)
					_showConfigPlugin();
				}
				else if(m_btnMgr.selected(m_configBtn[5])) // dump list
				{
					_dumpGameList();
					_showConfigPlugin();
				}
				else if(m_btnMgr.selected(m_configBtn[6])) // refresh list
				{
					m_cfg.setBool(plugin_domain, "update_cache", true);
					m_refreshGameList = true;
					break;
				}
			}
		}
	}

	if(prev_db_titles != cur_db_titles)
	{
		fsop_deleteFolder(m_listCacheDir.c_str());
		fsop_MakeFolder(m_listCacheDir.c_str());
		m_refreshGameList = true;
	}
	
	_hideConfig(true);
}

