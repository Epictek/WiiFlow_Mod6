// #include <unistd.h>
// #include <fstream>
// #include <sys/stat.h>

#include "menu.hpp"
#include "loader/nk.h"
#include "loader/wdvd.h"
// #include "channel/nand.hpp"
// #include "devicemounter/DeviceHandler.hpp"
// #include "loader/alt_ios.h"
// #include "loader/cios.h"
// #include "loader/disc.h"
// #include "loader/wbfs.h"

#include "loader/alt_ios.h"
#include "loader/cios.h"

s16 m_mainLblInfo[7];

s16 m_mainBtnHome;
s16 m_mainBtnCateg;	
s16 m_mainBtnFavoritesOff;
s16 m_mainBtnFavoritesOn;
s16 m_mainBtnDVD;
s16 m_mainBtnFind;
s16 m_mainBtnView;
s16 m_mainBtnConfig;
s16 m_mainBtnPrev;
s16 m_mainBtnNext;

s16 m_mainLblLetter;
s16 m_mainLblNotice;
s16 m_mainLblView;
s16 m_mainLblCurMusic;

s16 m_mainLblTitle;
s16 m_mainLblUser[4];

static inline int loopNum(int i, int s)
{
	return (i + s) % s;
}

void CMenu::_hideMain(bool instant)
{
	for(u8 i = 0; i < 7; ++i)
		m_btnMgr.hide(m_mainLblInfo[i], instant);
	
	m_btnMgr.hide(m_mainBtnHome, instant);
	m_btnMgr.hide(m_mainBtnCateg, instant);
	m_btnMgr.hide(m_mainBtnFavoritesOn, instant);
	m_btnMgr.hide(m_mainBtnFavoritesOff, instant);
	m_btnMgr.hide(m_mainBtnDVD, instant);
	m_btnMgr.hide(m_mainBtnFind, instant);
	m_btnMgr.hide(m_mainBtnView, instant);
	m_btnMgr.hide(m_mainBtnConfig, instant);
	m_btnMgr.hide(m_mainBtnPrev, instant);
	m_btnMgr.hide(m_mainBtnNext, instant);

	m_btnMgr.hide(m_mainLblLetter, instant);
	m_btnMgr.hide(m_mainLblNotice, instant);
	m_btnMgr.hide(m_mainLblView, instant);
	m_btnMgr.hide(m_mainLblCurMusic, instant);

	m_btnMgr.hide(m_mainLblTitle);
	m_snapshot_loaded = false; // in case timer is still running
	m_btnMgr.hide(m_configLblDialog, instant);
	m_btnMgr.hide(m_configLbl[7], true);
	m_btnMgr.hide(m_configBtnGo[7], true);
	
	for(u8 i = 0; i < ARRAY_SIZE(m_mainLblUser); ++i)
		if(m_mainLblUser[i] != -1)
			m_btnMgr.hide(m_mainLblUser[i], instant);
}

void CMenu::_getCustomBgTex()
{
	if(m_sourceflow)
		_getSFlowBgTex();
	else
	{
		curCustBg = loopNum(curCustBg + 1, 2);
		string fn = "";
		if(m_platform.loaded())
		{
			m_plugin.PluginMagicWord[0] = '\0';
			u8 i = 0;
			bool match = true;
			string first_pf = "";
			switch(m_current_view)
			{
				case COVERFLOW_CHANNEL:
					if(m_cfg.getInt(channel_domain, "channels_type") & CHANNELS_REAL)
						strncpy(m_plugin.PluginMagicWord, NAND_PMAGIC, 9);
					else
						strncpy(m_plugin.PluginMagicWord, ENAND_PMAGIC, 9);
					break;
				case COVERFLOW_HOMEBREW:
					strncpy(m_plugin.PluginMagicWord, HB_PMAGIC, 9);
					break;
				case COVERFLOW_GAMECUBE:
					strncpy(m_plugin.PluginMagicWord, GC_PMAGIC, 9);
					break;
				case COVERFLOW_PLUGIN:
					while(m_plugin.PluginExist(i) && !m_plugin.GetEnabledStatus(i)) { ++i; }
					if(m_plugin.PluginExist(i))
					{
						strncpy(m_plugin.PluginMagicWord, fmt("%08x", m_plugin.GetPluginMagic(i)), 8);
						//! in case of multiple plugins check if all are for the same platform
						first_pf = m_platform.getString("PLUGINS", m_plugin.PluginMagicWord);
						i++;
						while(m_plugin.PluginExist(i))
						{
							if(m_plugin.GetEnabledStatus(i) && first_pf != m_platform.getString("PLUGINS", fmt("%08x", m_plugin.GetPluginMagic(i))))
							{
								match = false;
								break;
							}
							i++;
						}
						//! if multiple platforms default to HOMEBREW
						strncpy(m_plugin.PluginMagicWord, match ? m_plugin.PluginMagicWord : HB_PMAGIC, match ? 8 : 9);
					}
					break;
				default: // COVERFLOW_WII
					strncpy(m_plugin.PluginMagicWord, WII_PMAGIC, 9);
			}
			if(strlen(m_plugin.PluginMagicWord) == 8)
				fn = m_platform.getString("PLUGINS", m_plugin.PluginMagicWord, "");
		}	
		if(fn.length() > 0)
		{
			if(TexHandle.fromImageFile(m_mainCustomBg[curCustBg], fmt("%s/%s/%s.png", m_bckgrndsDir.c_str(), m_themeName.c_str(), fn.c_str())) != TE_OK)
			{	
				if(TexHandle.fromImageFile(m_mainCustomBg[curCustBg], fmt("%s/%s/%s.jpg", m_bckgrndsDir.c_str(), m_themeName.c_str(), fn.c_str())) != TE_OK)
				{
					if(TexHandle.fromImageFile(m_mainCustomBg[curCustBg], fmt("%s/%s.png", m_bckgrndsDir.c_str(), fn.c_str())) != TE_OK)
					{
						if(TexHandle.fromImageFile(m_mainCustomBg[curCustBg], fmt("%s/%s.jpg", m_bckgrndsDir.c_str(), fn.c_str())) != TE_OK)
						{
							curCustBg = loopNum(curCustBg + 1, 2); // reset it
							customBg = false;
							return;
						}
					}
				}
			}
			customBg = true;
		}
		else
		{
			curCustBg = loopNum(curCustBg + 1, 2); // reset it
			customBg = false;
		}
	}
}

void CMenu::_setMainBg()
{
	if(customBg)
		_setBg(m_mainCustomBg[curCustBg], m_mainBgLQ);
	else
		_setBg(m_mainBg, m_mainBgLQ);
}

void CMenu::_showMain()
{
	_setMainBg();
	if(m_refreshGameList)
		_showCF(m_refreshGameList);
}

void CMenu::_showCF(bool refreshList)
{
	m_refreshGameList = false;
	_hideMain();

	/* Display view name */
	if(!m_sourceflow)
	{
		wstringEx view = L"";
		switch(m_current_view)
		{
			case COVERFLOW_WII:
				view = m_loc.getWString(m_curLanguage, "wii", L"Wii");
				break;
			case COVERFLOW_CHANNEL:
				view = m_loc.getWString(m_curLanguage, "wiichannels", L"Wii channels");
				break;
			case COVERFLOW_GAMECUBE:
				view = m_loc.getWString(m_curLanguage, "gc", L"GameCube");
				break;
			case COVERFLOW_HOMEBREW:
				view = m_loc.getWString(m_curLanguage, "homebrew", L"Homebrew");
				break;
			case COVERFLOW_PLUGIN:
				if(enabledPluginsCount == 1)
				{
					u8 i = 0;
					while(m_plugin.PluginExist(i) && !m_plugin.GetEnabledStatus(i)) { ++i; }
					view = m_plugin.GetPluginName(i);
				}
				else
					view = m_loc.getWString(m_curLanguage, "plugins", L"Plugins");
				break;
		}
		//! view name
		m_btnMgr.setText(m_mainLblTitle, view);
		//! settings icon info
		view = wfmt(_fmt("infomain7", L"%s settings"), m_current_view == COVERFLOW_PLUGIN ? m_loc.getWString(m_curLanguage, "plugins", L"Plugins").toUTF8().c_str() : view.toUTF8().c_str());
		m_btnMgr.setText(m_configLbl[7], view);
		m_btnMgr.setText(m_mainLblInfo[6], view); // will show up if m_gameList empty
		
	}
	else // if m_sourceflow
	{
		m_btnMgr.setText(m_configBtnCenter, _t("cfg837", L"Options"));
		m_btnMgr.setText(m_configLblPage, _t("cfg851", L"Scroll"));
	}

	if(refreshList)
	{
		/* Create gameList based on sources selected */
		_loadList(); // wait message is hidden there
		
		/* If game list is empty display message letting user know */
		if(m_gameList.empty())
		{
			wstringEx Msg;
			string Pth;
			cacheCovers = false;
			switch(m_current_view)
			{
				case COVERFLOW_WII:
					Msg = _t("main2", L"No games found in");
					Pth = sfmt(wii_games_dir, DeviceName[currentPartition]);
					break;
				case COVERFLOW_GAMECUBE:
					Msg = _t("main2", L"No games found in");
					Pth = sfmt(gc_games_dir, DeviceName[currentPartition]);
					break;
				case COVERFLOW_CHANNEL:
					Msg = _t("main3", L"No titles found in");
					Pth = sfmt("%s:/%s/%s", DeviceName[m_cfg.getInt(channel_domain, "partition")], emu_nands_dir, m_cfg.getString(channel_domain, "current_emunand").c_str());
					break;
				case COVERFLOW_HOMEBREW:
					Msg = _t("main4", L"No apps found in");
					Pth = sfmt("%s:/%s", DeviceName[currentPartition], HOMEBREW_DIR);
					break;
				case COVERFLOW_PLUGIN:
					if(enabledPluginsCount == 0)
						Msg = _t("main6", L"No plugins selected");
					else if(enabledPluginsCount > 1)
						Msg = _t("main5", L"No items");
					else
					{
						Msg = _t("main2", L"No games found in");
						u8 i = 0;
						while(m_plugin.PluginExist(i) && !m_plugin.GetEnabledStatus(i)){ ++i; }
						int romsPartition = m_plugin.GetRomPartition(i);
						if(romsPartition < 0)
							romsPartition = m_cfg.getInt(plugin_domain, "partition", 0);

						const char *romDir = m_plugin.GetRomDir(i);
						if(strstr(romDir, "scummvm.ini") != NULL) // SCUMMVM
						{
							if(strchr(romDir, ':') != NULL)
								Pth = sfmt("%s", romDir);
							else
								Pth = sfmt("%s/%s", m_pluginsDir.c_str(), m_plugin.GetRomDir(i));
						}
						else // all other plugins
							Pth = sfmt("%s:/%s", DeviceName[romsPartition], m_plugin.GetRomDir(i));
					}
					break;
			}
			Msg.append(wstringEx(' ' + Pth));
			m_btnMgr.setText(m_configLblDialog, Msg);
			m_btnMgr.show(m_configLblDialog);
			m_showtimer = 0;
			m_btnMgr.show(m_mainLblTitle);
			//! Show shortcut to game location settings
			m_btnMgr.show(m_configLbl[7]);
			m_btnMgr.show(m_configBtnGo[7]);
			
			return;
		}
		
		/* If source menu button set to autoboot */
		if(m_source_autoboot)
		{	
			//! search game list for the requested title
			bool game_found = false;
			for(vector<dir_discHdr>::iterator element = m_gameList.begin(); element != m_gameList.end(); ++element)
			{
				switch(m_autoboot_hdr.type)
				{
					case TYPE_CHANNEL:
					case TYPE_WII_GAME:
					case TYPE_GC_GAME:
						if(strcmp(m_autoboot_hdr.id, element->id) == 0)
							game_found = true;
						break;
					case TYPE_HOMEBREW:
					case TYPE_PLUGIN:
						if(wcsncmp(m_autoboot_hdr.title, element->title, 63) == 0)
							game_found = true;
						break;
					default:
						break;
				}
				if(game_found)
				{
					memcpy(&m_autoboot_hdr, &(*(element)), sizeof(dir_discHdr));
					break;
				}
			}
			if(game_found) // title found - launch it
			{
				// gprintf("Game found, autobooting...\n");
				_launch(&m_autoboot_hdr);
			}
			//! fail
			m_source_autoboot = false;
		}
		
		/* Update covers cache if requested */
		if(cacheCovers)
		{
			cacheCovers = false;
			{
				_showProgress();
				_start_pThread();
				
				_cacheCovers();
				
				_stop_pThread();
				m_btnMgr.setText(m_configLblDialog, _t("dlmsg14", L"Done."));
				u8 pause = 50;
				while(!m_exit)
				{
					_mainLoopCommon();
					pause--;
					if(pause == 0)
					{
						_hideConfigFull();
						break;
					}
				}
			}
		}
	}

	/* Setup categories and favorites for filtering the game list below */
	if(refreshList && m_clearCats) // false on boot up and if a source menu button selects a category
	{
		//! do not clear hidden categories to keep games hidden
		m_cat.remove(general_domain, "selected_categories");
		m_cat.remove(general_domain, "required_categories");
	}
	m_clearCats = true; // set to true for next source

	m_favorites = false;
	if(m_getFavs || m_cfg.getBool(general_domain, "save_favorites_mode", true))
		m_favorites = m_cfg.getBool(_domainFromView(), "favorites", false);
	else
		m_cfg.setBool(_domainFromView(), "favorites", false);
	m_getFavs = false;
	
	/* Set CoverFlow domain to _COVERFLOW or _SMALLFLOW or _???FLOW */
	strcpy(cf_domain, "_COVERFLOW");
	bool sf_smallbox = m_cfg.getBool(sourceflow_domain, "smallbox", true);
	bool hb_smallbox = m_cfg.getBool(homebrew_domain, "smallbox", true);
	if(m_sourceflow)
	{
		if(sf_smallbox)
			strcpy(cf_domain, "_SMALLFLOW");
	}
	else if(m_current_view & COVERFLOW_HOMEBREW)
	{
		if(hb_smallbox)
			strcpy(cf_domain, "_SMALLFLOW");		
	}
	else if(m_current_view & COVERFLOW_PLUGIN)
	{
		//! check if homebrew plugin
		if(enabledPluginsCount == 1 && m_plugin.GetEnabledStatus(HB_PMAGIC) && hb_smallbox)
			strcpy(cf_domain, "_SMALLFLOW");
		else if(m_thumbnail)
			strcpy(cf_domain, "_SNAPSHOTS");
		else if(enabledPluginsCount > 0 && m_platform.loaded())
		{
			//! get first plugin flow domain
			u8 i = 0;
			while(m_plugin.PluginExist(i) && !m_plugin.GetEnabledStatus(i)){ ++i; }
			string flow_domain = m_platform.getString("FLOWS", m_platform.getString("PLUGINS", sfmt("%08x", m_plugin.GetPluginMagic(i)), ""), "_COVERFLOW");

			//! check if all plugin flow domains match
			bool match = true;
			i++;
			while(m_plugin.PluginExist(i))
			{
				if(m_plugin.GetEnabledStatus(i) &&
					flow_domain != m_platform.getString("FLOWS", m_platform.getString("PLUGINS", sfmt("%08x", m_plugin.GetPluginMagic(i)), ""), "_COVERFLOW"))
				{
					match = false;
					break;
				}
				i++;
			}
			//! if all match we use that flow domain
			if(match)
				snprintf(cf_domain, sizeof(cf_domain), "%s", flow_domain.c_str());
		}
	}	
	/* Get the number of layouts for the CoverFlow domain */
	m_numCFVersions = min(max(1, m_coverflow.getInt(cf_domain, "number_of_modes", 1)), 15); // max layouts is 15
	
	/* Get the current cf layout number and use it to load the data used for that layout */
	_loadCFLayout(min(max(1, _getCFVersion()), (int)m_numCFVersions));
	
	/* Filter game list to create the cf cover list and start coverflow coverloader */	
	_initCF();
	
	/* Set the covers and titles to the positions and angles based on the cf layout */
	CoverFlow.applySettings(m_sourceflow && sf_smallbox); // true = no cf animation for sourceflow
	// gprintf("Displaying covers\n");

	/* Display game count if not sourceflow */
	if(m_sourceflow)
		return;
	m_showtimer = 150;
	m_snapshot_loaded = true; // hide coverflow title
	u32 totalGames = CoverFlow.size();
	if(totalGames > 0)
		m_btnMgr.setText(m_mainLblView, wfmt(_fmt("main7", L"%i games"), totalGames));
	else
		m_btnMgr.setText(m_mainLblView, _t("main5", L"No items"));
	wstringEx curSort = _sortLabel(m_cfg.getInt(_domainFromView(), "sort", 0));
	m_btnMgr.setText(m_mainLblNotice, curSort);
	m_btnMgr.setText(m_mainLblLetter, neek2o() ? L"Neek2o" : isWiiVC ? L"WiiU VC" : L"");
	m_btnMgr.show(m_mainLblLetter);
	m_btnMgr.show(m_mainLblNotice);
	m_btnMgr.show(m_mainLblView);
	m_btnMgr.show(m_mainLblTitle);
}

/********************************************************************************************************/

int CMenu::main(void)
{
	bool m_source_on_start = (m_cfg.getBool(general_domain, "source_on_start", false) && m_use_source && !neek2o());
	bool bheld = false; // bheld to indicate btn b was pressed or held
	bool bUsed = false; // bused to indicate that it was actually used for something
	bool menuBar = false; // for menu bar with D-Pad
	bool change = false;
	bool cancel_bheld = false; // cancel potential return from a menu with btn b

	m_reload = false;
	u32 disc_check = 0;
	m_prev_view = 0;
	
	if(isWiiVC)
		m_current_view = COVERFLOW_GAMECUBE;
	else if(neek2o())
	{
		m_clearCats = true;
		m_current_view = COVERFLOW_CHANNEL;
	}
	else
		m_current_view = m_cfg.getUInt(general_domain, "sources", COVERFLOW_WII);
	
	m_catStartPage = m_cfg.getInt(general_domain, "cat_startpage", 1);

	m_vid.set2DViewport(m_cfg.getInt(general_domain, "tv_width", 640), m_cfg.getInt(general_domain, "tv_height", 480), m_cfg.getInt(general_domain, "tv_x", 0), m_cfg.getInt(general_domain, "tv_y", 0));
	gprintf("Bootup completed!\n");

	_hideWaitMessage();

	if(show_mem)
	{
		m_btnMgr.show(m_mem1FreeSize);
		m_btnMgr.show(m_mem2FreeSize);
	}

	SetupInput(true);

	/* Show explorer if last game was launched using explorer */
	if(m_explorer_on_start)
	{
		u32 plmagic = strtoul(m_cfg.getString(plugin_domain, "cur_magic", WII_PMAGIC).c_str(), NULL, 16);
		string plpath = m_cfg.getString(general_domain, "explorer_path", "");
		_pluginExplorer(plpath.c_str(), plmagic, m_explorer_on_start > 1);
		if(m_explorer_on_start > 1)
			m_source_on_start = true; // back to source if explorer_on_start = 2
		else
			_showCF(true); // back to main coverflow view if explorer_on_start = 1
		m_explorer_on_start = 0;
		m_cfg.setInt(general_domain, "explorer_on_start", 0);
	}
	else
	{
		if(m_source_on_start)
		{
			_getSFlowBgTex();
			_setMainBg();
		}
		else // main coverflow view
		{
			_getCustomBgTex();
			_setMainBg();
			_showCF(true);
		}
	}

	while(!m_exit)
	{
		/* Check if a disc is inserted */
		if(!isWiiVC && !neek2o())
			WDVD_GetCoverStatus(&disc_check);
		
		/* Main Loop */
		_mainLoopCommon(true);
		
		cancel_bheld = false; // reset
		
		/* This will make the source menu/flow display, what happens when a sourceflow cover is selected is taken care of later */
		if(((bheld && !BTN_B_OR_1_HELD) || m_source_on_start) && !neek2o()) // if button b was held and now released
		{
			menuBar = false;
			bheld = false;
			if(bUsed) // if b button used for something don't show source menu or sourceflow
				bUsed = false;
			else
			{
				//! button B goes back one tier or back to coverflow if 1st tier reached
				if(m_sourceflow) // back a tier or exit sourceflow
				{
					if(!_srcTierBack(false)) // back a tier
						m_sourceflow = false; // if not back a tier then exit sourceflow
					_getCustomBgTex();
					_setMainBg();
					m_clearCats = false;
					_showCF(true); // refresh coverflow or sourceflow list
					continue;
				}
				if(m_use_source) // if source_menu enabled
				{
					_hideMain();
					if(SF_enabled) // if sourceflow show it
					{
						m_sourceflow = true;
						_getCustomBgTex();
						_setMainBg();
						m_clearCats = false;
						_showCF(true);
					}
					else // show source menu
					{
						m_refreshGameList = _Source() || m_source_on_start || m_gameList.empty();
						cancel_bheld = true; // required each time we launch a menu from main coverflow
						_getCustomBgTex();
						_showMain();
					}
					m_source_on_start = false;
					continue;
				}
			}
		}

		/** Home menu or back to main view with home button **/
		else if(BTN_HOME_PRESSED)
		{
			menuBar = false;
			if(m_sourceflow)
			{			
				m_sourceflow = false;
				_getCustomBgTex();
				_setMainBg();
				m_clearCats = false;
				_showCF(true);
				continue;
			}
			else
			{
				_hideMain();
				CoverFlow.fade(1);
				if(_Home())
					break; // exit wiiflow
				CoverFlow.fade(0);
				cancel_bheld = true;
				m_refreshGameList = m_refreshGameList || m_gameList.empty();
				_showMain();
			}
		}
		
		else if(BTN_A_OR_2_PRESSED)
		{
			/** Jump to next letter **/
			if(m_btnMgr.selected(m_mainBtnNext) || m_btnMgr.selected(m_mainBtnPrev))
			{
				_sortCF(m_btnMgr.selected(m_mainBtnPrev));
			}
			/** Sourceflow back button **/
			else if(m_btnMgr.selected(m_configBtnBack))
			{
				if(!_srcTierBack(false)) // back a tier
					m_sourceflow = false;
				_getCustomBgTex();
				_setMainBg();
				m_clearCats = false;
				_showCF(true);
				continue;
			}
			/** Sourceflow previous page **/
			else if(m_btnMgr.selected(m_configBtnPageM))
			{
				CoverFlow.pageUp();
			}
			/** Sourceflow next page **/
			else if(m_btnMgr.selected(m_configBtnPageP))
			{
				CoverFlow.pageDown();
			}
			/** Sourceflow (source menu) options **/
			else if(m_btnMgr.selected(m_configBtnCenter))
			{
				_hideMain();
				m_btnMgr.hide(m_configBtnCenter);
				m_btnMgr.hide(m_configBtnPageM);
				m_btnMgr.hide(m_configBtnPageP);
				m_btnMgr.hide(m_configLblPage);
				CoverFlow.fade(2);
				_configSource();
				if(m_exit) // end loop immediately to fix green flash on reboot or neek2o launch
					break;
				CoverFlow.fade(0);
				cancel_bheld = true;
				if(!SF_enabled) // sourceflow has just been disabled
				{
					m_sourceflow = false;
					m_source_on_start = true; // only once at next loop iteration
					continue;
				}
				_getCustomBgTex();
				_showMain();				
			}
			/** Change coverflow view **/
			else if(m_btnMgr.selected(m_mainBtnHome))
			{
				//! home menu if neek
				if(neek2o())
				{
					_hideMain();
					CoverFlow.fade(1);
					if(_Home())
						break; // exit wiiflow
					CoverFlow.fade(0);
					_showMain();
				}
				//! sourceflow or source menu if they exist
				else if(m_use_source && !BTN_B_OR_1_HELD)
				{
					_hideMain();
					_srcTierBack(true); // go back to 1st tier in source menu
					if(SF_enabled) // if sourceflow show it
					{
						m_sourceflow = true;
						_getCustomBgTex();
						_setMainBg();
						m_clearCats = false;
						_showCF(true);
					}
					else // show source menu
					{
						m_refreshGameList = _Source(true) || m_gameList.empty(); // true: first tier
						cancel_bheld = true;
						_getCustomBgTex();
						_showMain();
					}			
				}
				//! cycle coverflow views
				else
				{
					bUsed = true;
					m_current_view = (m_current_view == COVERFLOW_HOMEBREW ? (isWiiVC ? COVERFLOW_GAMECUBE : COVERFLOW_WII) : m_current_view * 2);
					m_cfg.remove(sourceflow_domain, "numbers");
					m_cfg.remove(sourceflow_domain, "tiers");
					m_cfg.setUInt(general_domain, "sources", m_current_view);
					m_catStartPage = 1;
					m_thumbnail = false;
					_srcTierBack(true);
					_getCustomBgTex();
					_setMainBg();
					_showCF(true);
				}
			}
			
			/** Select categories **/
			else if(m_btnMgr.selected(m_mainBtnCateg) && !m_gameList.empty())
			{
				_hideMain();
				CoverFlow.fade(1);
				_CategorySettings();
				CoverFlow.fade(0);
				cancel_bheld = true;
				if(m_refreshGameList)
				{
					_initCF();
					m_refreshGameList = false;
					//! show new game total
					m_showtimer = 150;
					u32 totalGames = CoverFlow.size();
					if(totalGames > 0)
						m_btnMgr.setText(m_mainLblView, wfmt(_fmt("main7", L"%i games"), totalGames));
					else
						m_btnMgr.setText(m_mainLblView, _t("main5", L"No items"));
					m_btnMgr.show(m_mainLblView);
				}
			}
			
			/** Switch favorite games only on/off **/
			else if((m_btnMgr.selected(m_mainBtnFavoritesOn) || m_btnMgr.selected(m_mainBtnFavoritesOff)) && !m_gameList.empty())
			{
				change = true;
				m_favorites = !m_favorites;
				m_cfg.setBool(_domainFromView(), "favorites", m_favorites);
				_initCF();
				//! show new game total
				m_showtimer = 150;
				u32 totalGames = CoverFlow.size();
				if(totalGames > 0)
					m_btnMgr.setText(m_mainLblView, wfmt(_fmt("main7", L"%i games"), totalGames));
				else
					m_btnMgr.setText(m_mainLblView, _t("main5", L"No items"));
				m_btnMgr.show(m_mainLblView);
			}
			
			/** Toggle 1 player games filter **/
			else if((m_btnMgr.selected(m_mainBtnOnePlayerOn) || m_btnMgr.selected(m_mainBtnOnePlayerOff)) && !m_gameList.empty())
			{
				change = true;
				bool currentFilter = m_cfg.getBool(general_domain, "filter_one_player", false);
				m_cfg.setBool(general_domain, "filter_one_player", !currentFilter);
				if(currentFilter)
					m_cfg.setBool(general_domain, "filter_multi_player", false);
				_initCF();
				//! show new game total
				m_showtimer = 150;
				u32 totalGames = CoverFlow.size();
				if(totalGames > 0)
					m_btnMgr.setText(m_mainLblView, wfmt(_fmt("main7", L"%i games"), totalGames));
				else
					m_btnMgr.setText(m_mainLblView, _t("main5", L"No items"));
				m_btnMgr.show(m_mainLblView);
			}
			
			/** Toggle 2+ player games filter **/
			else if((m_btnMgr.selected(m_mainBtnMultiPlayerOn) || m_btnMgr.selected(m_mainBtnMultiPlayerOff)) && !m_gameList.empty())
			{
				change = true;
				bool currentFilter = m_cfg.getBool(general_domain, "filter_multi_player", false);
				m_cfg.setBool(general_domain, "filter_multi_player", !currentFilter);
				if(currentFilter)
					m_cfg.setBool(general_domain, "filter_one_player", false);
				_initCF();
				//! show new game total
				m_showtimer = 150;
				u32 totalGames = CoverFlow.size();
				if(totalGames > 0)
					m_btnMgr.setText(m_mainLblView, wfmt(_fmt("main7", L"%i games"), totalGames));
				else
					m_btnMgr.setText(m_mainLblView, _t("main5", L"No items"));
				m_btnMgr.show(m_mainLblView);
			}
			
			/** Boot DVD in drive **/
			else if(m_btnMgr.selected(m_mainBtnDVD))
			{
				_hideMain(true);
				CoverFlow.fade(1);
				/* Create fake Header */
				dir_discHdr hdr;
				memset(&hdr, 0, sizeof(dir_discHdr));
				/* Boot the disc */
				_launchWii(&hdr, true, !BTN_B_OR_1_HELD); // immediately if btn B held, otherwise settings
				cancel_bheld = true;
				_showCF(false);
				CoverFlow.fade(0);
			}
			
			/** View options **/
			else if(m_btnMgr.selected(m_mainBtnView) && !CoverFlow.empty())
			{
				_hideMain();
				CoverFlow.fade(1);
				_viewOptions();
				CoverFlow.fade(0);
				cancel_bheld = true;
			}
			
			/** Search by first letters / Sort coverflow list **/
			else if(m_btnMgr.selected(m_mainBtnFind) && !CoverFlow.empty())
			{
				const char *domain = _domainFromView();
				int sort = m_cfg.getInt(domain, "sort", SORT_ALPHA);
				cancel_bheld = true;
				{
					_hideMain();
					char *c = NULL;
					CoverFlow.fade(1);
					c = _keyboard();
					CoverFlow.fade(0);
					if(strlen(c) > 0)
					{
						m_showtimer = 150;
						if(CoverFlow.findTitle(c, false)) // search only
						{
							if(sort != SORT_ALPHA) // force alpha sort mode
							{
								sort = SORT_ALPHA;
								m_cfg.setInt(domain, "sort", sort);
								_initCF();
							}
							CoverFlow.findTitle(c, true); // actually jump to letter
							m_btnMgr.setText(m_mainLblLetter, wfmt(L"%c", c[0]));
							m_btnMgr.show(m_mainLblLetter);
						}
						else
						{
							m_btnMgr.setText(m_mainLblView, _t("main5", L"No items"));
							m_btnMgr.show(m_mainLblView);
						}
					}
				}
			}
			
			/** Settings menu **/
			else if(m_btnMgr.selected(m_mainBtnConfig) || m_btnMgr.selected(m_configBtnGo[7]))
			{
				_hideMain();
				CoverFlow.fade(2);
				if(BTN_B_OR_1_HELD) // shortcut to global settings
				{
					bUsed = true;
					_config();
				}
				else if(!m_locked)
				{
					if(m_current_view & COVERFLOW_WII)
						_configWii();
					else if(m_current_view & COVERFLOW_CHANNEL)
						_configNandEmu();
					else if(m_current_view & COVERFLOW_GAMECUBE)
						_configGC();
					else if(m_current_view & COVERFLOW_PLUGIN)
							_configPlugin();
					else if(m_current_view & COVERFLOW_HOMEBREW)
						_configHB();
				}
				else // locked
					_error(_t("errgame15", L"Unlock parental control to use this feature!"));
				if(m_exit) // end loop immediately to fix green flash on reboot or neek2o launch
					break;
				cancel_bheld = true;
				CoverFlow.fade(0);
				m_refreshGameList = m_refreshGameList || m_gameList.empty();
				_showMain();				
			}
			
			/** Select game cover or sourceflow cover **/
			else if(!CoverFlow.empty() && CoverFlow.select())
			{
				_hideMain();
				if(m_sourceflow)
				{
					_sourceFlow(); // set the source selected
					cancel_bheld = true;
					_getCustomBgTex();
					_setMainBg();
					_showCF(true);
					continue;
				}
				else // select game
				{
					_game(BTN_B_OR_1_HELD);
					if(m_exit)
						break;
					cancel_bheld = true;
					_setMainBg();
					if(m_refreshGameList) // // if changes were made to favorites, parental lock, or categories
					{
						_initCF();
						m_refreshGameList = false;
					}
					else
						CoverFlow.cancel();
				}
			}
		}
		else if(WROLL_LEFT)
		{
			bUsed = true;
			CoverFlow.left();
		}
		else if(WROLL_RIGHT)
		{
			bUsed = true;
			CoverFlow.right();
		}

		if(BTN_B_OR_1_HELD) // not else if
		{
			bheld = true;
			if(cancel_bheld)
				bUsed = true;
			else
			{
				/** Change song (music player) **/
				if(BTN_LEFT_PRESSED) // previous song
				{
					bUsed = true;
					MusicPlayer.Previous();
				}
				else if(BTN_RIGHT_PRESSED) // next song
				{
					bUsed = true;
					MusicPlayer.Next();
				}

				/** Random boot or select **/
				else if((BTN_MINUS_PRESSED || BTN_PLUS_PRESSED) && !CoverFlow.empty())
				{
					bUsed = true;
					_hideMain();
					srand(time(NULL));
					u16 place = (rand() + rand() + rand()) % CoverFlow.size();

					if(BTN_PLUS_PRESSED) // boot random game immediately
					{
						const dir_discHdr *gameHdr = CoverFlow.getSpecificHdr(place);
						if(gameHdr != NULL)
						{
							_setCurrentItem(gameHdr);
							_launch(gameHdr);
						}
						_showCF(false); // this shouldn't happen
					}
					else // select random game
					{
						CoverFlow.setSelected(place);
						_game(false);
						if(m_exit)
							break;
						cancel_bheld = true;
						if(m_refreshGameList) // if changes were made to favorites, parental lock, or categories
						{
							_initCF();
							m_refreshGameList = false;
						}
						else
							CoverFlow.cancel();
						_setMainBg(); // in case fanart changed the background
					}
				}

				/** Change emunand **/
				else if((BTN_UP_PRESSED || BTN_DOWN_PRESSED) && !m_sourceflow)
				{
					bUsed = true;
					s8 direction =  BTN_DOWN_PRESSED ? 1 : -1;
					string new_nand = _SetEmuNand(direction);
					if(new_nand != "")
					{
						if(m_refreshGameList)
							_showCF(true);
						m_showtimer = 150;
						m_btnMgr.setText(m_mainLblLetter, new_nand);
						m_btnMgr.show(m_mainLblLetter);
					}
				}
			}
		}
		else // btn B NOT held
		{
			/** Move coverflow up **/
			if(BTN_MINUS_PRESSED)
			{
				if(ShowPointer())
					CoverFlow.pageUp();
				else // access to menu bar
				{
					menuBar = !menuBar; // switch menu bar
					if(!menuBar) // disable menu bar
						m_btnMgr.deselect();
					else // highlight first icon
						m_btnMgr.setSelected(m_sourceflow ? m_configBtnPageM : m_mainBtnHome);
				}
			}
			
			/** Move coverflow down **/
			else if(BTN_PLUS_PRESSED)
			{
				if(ShowPointer())
					CoverFlow.pageDown();
				else // jump to next letter (also in sourceflow)
					_sortCF();
			}
			
			/** Move coverflow **/
			if(!menuBar)
			{
				if(BTN_UP_REPEAT || RIGHT_STICK_UP)
					CoverFlow.up();
				else if(BTN_RIGHT_REPEAT || RIGHT_STICK_RIGHT || BTN_RIGHT_PRESSED) // BTN_RIGHT_PRESSED added for ds3
					CoverFlow.right();
				else if(BTN_DOWN_REPEAT ||  RIGHT_STICK_DOWN)
					CoverFlow.down();
				else if(BTN_LEFT_REPEAT || RIGHT_STICK_LEFT || BTN_LEFT_PRESSED) // BTN_LEFT_PRESSED for added for ds3
					CoverFlow.left();
			}
			else // menu bar navigation
			{
				if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
					m_btnMgr.up();
				else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
					m_btnMgr.down();
			}
		}
		
		/* Hide all labels if times up */	
		if(m_showtimer > 0)
		{
			if(--m_showtimer == 0)
			{
				m_btnMgr.hide(m_mainLblLetter);
				m_btnMgr.hide(m_mainLblNotice);
				m_btnMgr.hide(m_mainLblView);
				m_btnMgr.hide(m_mainLblCurMusic);
				m_btnMgr.hide(m_mainLblTitle);
				m_snapshot_loaded = false; // show coverflow title
			}
		}
		
		/* Display icons */
		if(!m_sourceflow)
		{
			m_btnMgr.hide(m_configBtnCenter, true);
			m_btnMgr.hide(m_configLblPage, true);
			m_btnMgr.hide(m_configBtnPageP, true);
			m_btnMgr.hide(m_configBtnPageM, true);
			m_btnMgr.hide(m_configBtnBack, true);
			if(!m_gameList.empty() && m_show_zone_prev)
				m_btnMgr.show(m_mainBtnPrev);
			else
				m_btnMgr.hide(m_mainBtnPrev);
				
			if(!m_gameList.empty() && m_show_zone_next)
				m_btnMgr.show(m_mainBtnNext);
			else
				m_btnMgr.hide(m_mainBtnNext);
			
			for(u8 i = 0; i < 7; ++i)
			{
				if(m_show_zone_info[i])
					m_btnMgr.show(m_mainLblInfo[i]);
				else
					m_btnMgr.hide(m_mainLblInfo[i]);
			}
			
			m_btnMgr.show(m_mainBtnHome);
			m_btnMgr.show(m_mainBtnCateg);
			m_btnMgr.show(m_favorites ? m_mainBtnFavoritesOn : m_mainBtnFavoritesOff, change);
			m_btnMgr.hide(m_favorites ? m_mainBtnFavoritesOff : m_mainBtnFavoritesOn, change);
			change = false;
			if(disc_check & 0x2)
				m_btnMgr.show(m_mainBtnDVD);
			else
			{
				m_btnMgr.hide(m_mainBtnDVD);
				m_btnMgr.hide(m_mainLblInfo[3]);
			}
			m_btnMgr.show(m_mainBtnFind);
			m_btnMgr.show(m_mainBtnView);
			m_btnMgr.show(m_mainBtnConfig);
			
			for(u8 i = 0; i < ARRAY_SIZE(m_mainLblUser); ++i)
				if(m_mainLblUser[i] != -1)
					m_btnMgr.show(m_mainLblUser[i]);	
		}
		else // if m_sourceflow
		{
			_hideMain();
			m_btnMgr.show(m_configBtnCenter);
			m_btnMgr.show(m_configLblPage);
			m_btnMgr.show(m_configBtnPageP);
			m_btnMgr.show(m_configBtnPageM);
			m_btnMgr.show(m_configBtnBack);
		}

		
		/* Set song title and display it if music info is allowed */
		if(m_music_info && MusicPlayer.SongChanged() && !MusicPlayer.OneSong)
		{
			if(m_showtimer == 0)
				m_showtimer = 150;
			m_btnMgr.setText(m_mainLblCurMusic, MusicPlayer.GetFileName(), false); // false for word wrap
			m_btnMgr.show(m_mainLblCurMusic);
		}

		if(menuBar && ShowPointer())
			menuBar = !menuBar;
		
		for(int chan = WPAD_MAX_WIIMOTES-1; chan >= 0; chan--)
		{
			if(WPadIR_Valid(chan) || (m_show_pointer[chan] && !WPadIR_Valid(chan)))
				CoverFlow.mouse(chan, m_cursor[chan].x(), m_cursor[chan].y());
			else
				CoverFlow.mouse(chan, -1, -1);
		}
	}
	ScanInput();

	/* Rebooting wiiflow */
	if(m_reload)
	{
		vector<string> arguments = _getMetaXML(fmt("%s/boot.dol", m_appDir.c_str()));
		_launchHomebrew(fmt("%s/boot.dol", m_appDir.c_str()), arguments);
	}
	
	cleanup();
	
	/* Save configs on power off or exit wiiflow */
	m_gcfg1.save(true); 
	m_gcfg2.save(true);
	m_cat.save(true);
	m_cfg.save(true);

	return 0;
}

extern const u8 icon_menu_png[];
extern const u8 icon_menu_s_png[];
extern const u8 icon_categ_png[];
extern const u8 icon_categ_s_png[];
extern const u8 icon_fav_png[];
extern const u8 icon_fav_s_png[];
extern const u8 icon_fav_on_png[];
extern const u8 icon_dvd_png[];
extern const u8 icon_dvd_s_png[];
extern const u8 icon_find_png[];
extern const u8 icon_find_s_png[];
extern const u8 icon_view_png[];
extern const u8 icon_view_s_png[];
extern const u8 icon_config_png[];
extern const u8 icon_config_s_png[];

extern const u8 icon_prev_png[];
extern const u8 icon_prev_s_png[];
extern const u8 icon_next_png[];
extern const u8 icon_next_s_png[];

void CMenu::_initMainMenu()
{
	TexData emptyTex;
	m_mainBg = _texture("MAIN/BG", "texture", theme.bg, false);
	if(m_theme.loaded() && TexHandle.fromImageFile(bgLQ, fmt("%s/%s", m_themeDataDir.c_str(), m_theme.getString("MAIN/BG", "texture").c_str()), GX_TF_CMPR, 64, 64) == TE_OK)
		m_mainBgLQ = bgLQ;

	TexHandle.fromPNG(texHome, icon_menu_png);
	TexHandle.fromPNG(texHomeS, icon_menu_s_png);
	TexHandle.fromPNG(texCateg, icon_categ_png);
	TexHandle.fromPNG(texCategS, icon_categ_s_png);
	TexHandle.fromPNG(texFavOff, icon_fav_png);
	TexHandle.fromPNG(texFavOffS, icon_fav_s_png);
	TexHandle.fromPNG(texFavOn, icon_fav_on_png);
	TexHandle.fromPNG(texFavOnS, icon_fav_s_png);
	TexHandle.fromPNG(texDVD, icon_dvd_png);
	TexHandle.fromPNG(texDVDS, icon_dvd_s_png);
	TexHandle.fromPNG(texFind, icon_find_png);
	TexHandle.fromPNG(texFindS, icon_find_s_png);
	TexHandle.fromPNG(texView, icon_view_png);
	TexHandle.fromPNG(texViewS, icon_view_s_png);
	TexHandle.fromPNG(texConfig, icon_config_png);
	TexHandle.fromPNG(texConfigS, icon_config_s_png);
	
	TexHandle.fromPNG(texPrev, icon_prev_png);
	TexHandle.fromPNG(texPrevS, icon_prev_s_png);
	TexHandle.fromPNG(texNext, icon_next_png);
	TexHandle.fromPNG(texNextS, icon_next_s_png);

	_addUserLabels(m_mainLblUser, ARRAY_SIZE(m_mainLblUser), "MAIN");

	m_mainBtnHome = _addPicButton("MAIN/MENU_BTN", texHome, texHomeS, 113, 410, 42, 42);
	m_mainBtnCateg = _addPicButton("MAIN/CATEG_BTN", texCateg, texCategS, 175, 410, 42, 42);
	m_mainBtnFavoritesOff = _addPicButton("MAIN/FAVORITES_OFF", texFavOff, texFavOffS, 237, 410, 42, 42);
	m_mainBtnFavoritesOn = _addPicButton("MAIN/FAVORITES_ON", texFavOn, texFavOnS, 237, 410, 42, 42);
	m_mainBtnOnePlayerOff = _addPicButton("MAIN/ONE_PLAYER_OFF", texFavOff, texFavOffS, 299, 410, 42, 42);
	m_mainBtnOnePlayerOn = _addPicButton("MAIN/ONE_PLAYER_ON", texFavOn, texFavOnS, 299, 410, 42, 42);
	m_mainBtnMultiPlayerOff = _addPicButton("MAIN/MULTI_PLAYER_OFF", texFavOff, texFavOffS, 361, 410, 42, 42);
	m_mainBtnMultiPlayerOn = _addPicButton("MAIN/MULTI_PLAYER_ON", texFavOn, texFavOnS, 361, 410, 42, 42);
	m_mainBtnDVD = _addPicButton("MAIN/DVD_BTN", texDVD, texDVDS, 423, 410, 42, 42);
	m_mainBtnFind = _addPicButton("MAIN/FIND_BTN", texFind, texFindS, 485, 410, 42, 42);
	m_mainBtnView = _addPicButton("MAIN/VIEW_BTN", texView, texViewS, 547, 410, 42, 42);
	m_mainBtnConfig = _addPicButton("MAIN/CONFIG_BTN", texConfig, texConfigS, 609, 410, 42, 42);

	m_mainBtnPrev = _addPicButton("MAIN/PREV_BTN", texPrev, texPrevS, 20, 200, 80, 80);
	m_mainBtnNext = _addPicButton("MAIN/NEXT_BTN", texNext, texNextS, 540, 200, 80, 80);

	m_mainLblCurMusic = _addLabel("MAIN/MUSIC", theme.lblFont, L"", 20, 50, 620, 20, theme.lblFontColor, FTGX_JUSTIFY_LEFT | FTGX_ALIGN_MIDDLE, emptyTex);
	m_mainLblLetter = _addLabel("MAIN/LETTER", theme.lblFont, L"", 340, 50, 280, 20, theme.lblFontColor, FTGX_JUSTIFY_RIGHT | FTGX_ALIGN_MIDDLE, emptyTex);
	m_mainLblNotice = _addLabel("MAIN/NOTICE", theme.lblFont, L"", 340, 20, 280, 20, theme.lblFontColor, FTGX_JUSTIFY_RIGHT | FTGX_ALIGN_MIDDLE, emptyTex);
	m_mainLblView = _addLabel("MAIN/VIEW", theme.lblFont, L"", 20, 20, 280, 20, theme.lblFontColor, FTGX_JUSTIFY_LEFT | FTGX_ALIGN_MIDDLE, emptyTex);
	
	m_mainLblTitle = _addTitle("MAIN/TITLE", theme.titleFont, L"", 0, 10, 640, 60, theme.titleFontColor, FTGX_JUSTIFY_CENTER | FTGX_ALIGN_MIDDLE);

	m_mem1FreeSize = _addLabel("MEM1", theme.btnFont, L"", 20, 10, 400, 20, theme.btnFontColor, FTGX_JUSTIFY_LEFT, emptyTex);
	m_mem2FreeSize = _addLabel("MEM2", theme.btnFont, L"", 420, 10, 200, 20, theme.btnFontColor, FTGX_JUSTIFY_LEFT, emptyTex);

	m_mainPrevZone.x = m_theme.getInt("MAIN/ZONES", "prev_x", 10);
	m_mainPrevZone.y = m_theme.getInt("MAIN/ZONES", "prev_y", 200);
	m_mainPrevZone.w = m_theme.getInt("MAIN/ZONES", "prev_w", 100);
	m_mainPrevZone.h = m_theme.getInt("MAIN/ZONES", "prev_h", 100);
	m_mainPrevZone.hide = m_theme.getBool("MAIN/ZONES", "prev_hide", true);
	
	m_mainNextZone.x = m_theme.getInt("MAIN/ZONES", "next_x", 530);
	m_mainNextZone.y = m_theme.getInt("MAIN/ZONES", "next_y", 200);
	m_mainNextZone.w = m_theme.getInt("MAIN/ZONES", "next_w", 100);
	m_mainNextZone.h = m_theme.getInt("MAIN/ZONES", "next_h", 100);
	m_mainNextZone.hide = m_theme.getBool("MAIN/ZONES", "next_hide", true);
	
	for(u8 i = 0; i < 7; ++i)
	{
		char *infoText = fmt_malloc("MAIN/INFO%i", i);
		if(infoText == NULL) 
			continue;
		m_mainLblInfo[i] = _addLabel(infoText, theme.lblFont, L"", 34 + (i * 62), 383, 200, 20, theme.lblFontColor, FTGX_JUSTIFY_CENTER | FTGX_ALIGN_MIDDLE, emptyTex);
		
		m_mainInfo[i].x = m_theme.getInt("MAIN/ZONES", fmt("info%i_x", i), 113 + (i * 62));
		m_mainInfo[i].y = m_theme.getInt("MAIN/ZONES", fmt("info%i_y", i), 410);
		m_mainInfo[i].w = m_theme.getInt("MAIN/ZONES", fmt("info%i_w", i), 42);
		m_mainInfo[i].h = m_theme.getInt("MAIN/ZONES", fmt("info%i_h", i), 42);
		m_mainInfo[i].hide = m_theme.getBool("MAIN/ZONES", fmt("info%i_hide", i), true);
		
		_setHideAnim(m_mainLblInfo[i], infoText, 0, 0, 0.f, 0.f);
		MEM2_free(infoText);
	}
	_textInfoMain();

	_setHideAnim(m_mainBtnHome, "MAIN/MENU_BTN", 0, 40, 0.f, 0.f);
	_setHideAnim(m_mainBtnCateg, "MAIN/CATEG_BTN", 0, 40, 0.f, 0.f);
	_setHideAnim(m_mainBtnFavoritesOff, "MAIN/FAVORITES_OFF", 0, 40, 0.f, 0.f);
	_setHideAnim(m_mainBtnFavoritesOn, "MAIN/FAVORITES_ON", 0, 40, 0.f, 0.f);
	_setHideAnim(m_mainBtnDVD, "MAIN/DVD_BTN", 0, 40, 0.f, 0.f);
	_setHideAnim(m_mainBtnFind, "MAIN/FIND_BTN", 0, 40, 0.f, 0.f);
	_setHideAnim(m_mainBtnView, "MAIN/VIEW_BTN", 0, 40, 0.f, 0.f);
	_setHideAnim(m_mainBtnConfig, "MAIN/CONFIG_BTN", 0, 40, 0.f, 0.f);
	
	_setHideAnim(m_mainBtnPrev, "MAIN/PREV_BTN", 0, 0, 0.f, 0.f);
	_setHideAnim(m_mainBtnNext, "MAIN/NEXT_BTN", 0, 0, 0.f, 0.f);
	
	_setHideAnim(m_mainLblTitle, "MAIN/TITLE", 0, 0, -2.f, 0.f);
	
	_setHideAnim(m_mainLblCurMusic, "MAIN/MUSIC", 0, 0, 0.f, 0.f);
	_setHideAnim(m_mainLblLetter, "MAIN/LETTER", 0, 0, 0.f, 0.f);
	_setHideAnim(m_mainLblNotice, "MAIN/NOTICE", 0, 0, 0.f, 0.f);
	_setHideAnim(m_mainLblView, "MAIN/VIEW", 0, 0, 0.f, 0.f);
	
	_setHideAnim(m_mem1FreeSize, "MEM1", 0, 0, 0.f, 0.f);
	_setHideAnim(m_mem2FreeSize, "MEM2", 0, 0, 0.f, 0.f);

	// _hideMain(true);
}

void CMenu::_textInfoMain(void)
{
	m_btnMgr.setText(m_mainLblInfo[0], _t("infomain1", L"Source"));
	m_btnMgr.setText(m_mainLblInfo[1], _t("infomain2", L"Categories"));
	m_btnMgr.setText(m_mainLblInfo[2], _t("infomain3", L"Favorites"));
	m_btnMgr.setText(m_mainLblInfo[3], _t("infomain4", L"Launch DVD"));
	m_btnMgr.setText(m_mainLblInfo[4], _t("infomain5", L"Go to"));
	m_btnMgr.setText(m_mainLblInfo[5], _t("infomain6", L"View"));
}

/*******************************************************************************************************/

wstringEx CMenu::_sortLabel(int sort)
{
	if(sort == SORT_ALPHA)
		return m_loc.getWString(m_curLanguage, "alphabetically", L"Alphabetical");
	else if(sort == SORT_PLAYCOUNT)
		return m_loc.getWString(m_curLanguage, "byplaycount", L"Play count");
	else if(sort == SORT_LASTPLAYED)
		return m_loc.getWString(m_curLanguage, "bylastplayed", L"Last played");
	else if(sort == SORT_YEAR)
		return m_loc.getWString(m_curLanguage, "byyear", L"Release year");
	else if(sort == SORT_GAMEID)
		return m_loc.getWString(m_curLanguage, "bygameid", L"Game ID");
	else if(sort == SORT_WIFIPLAYERS)
		return m_loc.getWString(m_curLanguage, "bywifiplayers", L"Wifi players");
	else if(sort == SORT_PLAYERS)
		return m_loc.getWString(m_curLanguage, "byplayers", L"Players");
	return L"";
}

void CMenu::_sortCF(bool previous)
{
	int sorting = m_cfg.getInt(_domainFromView(), "sort", SORT_ALPHA);
	if(sorting != SORT_ALPHA && sorting != SORT_YEAR && sorting != SORT_PLAYERS && sorting != SORT_WIFIPLAYERS && sorting != SORT_GAMEID)
	{
		CoverFlow.setSorting((Sorting)SORT_ALPHA);
		sorting = SORT_ALPHA;
	}
	wchar_t c[5] = {0, 0, 0, 0, 0};
	if(previous)
		CoverFlow.prevLetter(c);
	else
		CoverFlow.nextLetter(c);
	wstringEx curLetter = wstringEx(c);
	if(sorting != SORT_ALPHA)
	{
		if(sorting == SORT_PLAYERS)
		{
			curLetter += ' ';
			curLetter += m_loc.getWString(m_curLanguage, "players", L"players");
		}
		else if(sorting == SORT_WIFIPLAYERS)
		{
			curLetter += ' ';
			curLetter += m_loc.getWString(m_curLanguage, "wifiplayers", L"wifi players");
		}
		else if(sorting == SORT_GAMEID && (m_current_view & COVERFLOW_CHANNEL)) // only if coverflow channel
		{
			if(curLetter[0] == L'C')
				curLetter = m_loc.getWString(m_curLanguage, "commodore", L"Commodore 64");
			else if(curLetter[0] == L'E')
				curLetter = m_loc.getWString(m_curLanguage, "neogeo", L"Arcade / Neo-Geo");
			else if(curLetter[0] == L'F')
				curLetter = m_loc.getWString(m_curLanguage, "nes", L"NES");
			else if(curLetter[0] == L'J')
				curLetter = m_loc.getWString(m_curLanguage, "snes", L"Super NES");
			else if(curLetter[0] == L'L')
				curLetter = m_loc.getWString(m_curLanguage, "mastersystem", L"Master System");
			else if(curLetter[0] == L'M')
				curLetter = m_loc.getWString(m_curLanguage, "genesis", L"Genesis");
			else if(curLetter[0] == L'N')
				curLetter = m_loc.getWString(m_curLanguage, "nintendo64", L"Nintendo 64");
			else if(curLetter[0] == L'P')
				curLetter = m_loc.getWString(m_curLanguage, "turbografx16", L"TurboGrafx-16");
			else if(curLetter[0] == L'Q')
				curLetter = m_loc.getWString(m_curLanguage, "turbografxcd", L"TurboGrafx-CD");
			else if(curLetter[0] == L'X')
				curLetter = m_loc.getWString(m_curLanguage, "msx", L"MSX");
			else if(curLetter[0] == L'W')
				curLetter = m_loc.getWString(m_curLanguage, "wiiware", L"WiiWare");
			else if(curLetter[0] == L'H')
				curLetter = m_loc.getWString(m_curLanguage, "wiichannels", L"Wii channels");
			else
				curLetter = m_loc.getWString(m_curLanguage, "homebrew", L"Homebrew"); //
		}
	}
	m_showtimer = 150;
	m_btnMgr.setText(m_mainLblLetter, curLetter);
	m_btnMgr.show(m_mainLblLetter);	
}

void CMenu::_setPartition(s8 direction, u8 coverflow)
{
	if((direction != 1 && direction != -1) || coverflow > COVERFLOW_HOMEBREW)
		return;
	
	m_prev_view = m_current_view;
	m_current_view = coverflow;
	u8 Partition = m_cfg.getInt(_domainFromView(), m_current_view == COVERFLOW_NONE ? "savepartition" : "partition");
	int FS_Type = 0;
	bool NeedFAT = m_current_view == COVERFLOW_CHANNEL || m_current_view == COVERFLOW_GAMECUBE || m_current_view == COVERFLOW_NONE; // COVERFLOW_NONE is for saves emunand
	u8 limiter = 0;
	
	/* Select next eligible partition */
	do
	{
		Partition = loopNum(Partition + direction, 9);
		FS_Type = DeviceHandle.GetFSType(Partition);
		limiter++;
	}
	while(limiter < 9 && (!DeviceHandle.IsInserted(Partition) ||
		(m_current_view != COVERFLOW_WII && FS_Type == PART_FS_WBFS) ||
		(NeedFAT && FS_Type != PART_FS_FAT)));

	/* Set partition (channel emunand partition will be set with _checkEmuNandSettings()) */
	if(m_current_view != COVERFLOW_CHANNEL || (FS_Type != -1 && DeviceHandle.IsInserted(Partition)))
		m_cfg.setInt(_domainFromView(), m_current_view == COVERFLOW_NONE ? "savepartition" : "partition", Partition);

	m_current_view = m_prev_view;
}

int CMenu::_getCFVersion()
{
	if(m_sourceflow)
		return _getSrcFlow();
	else if(m_current_view == COVERFLOW_PLUGIN)
	{
		int first = 0;
		for(u8 i = 0; m_plugin.PluginExist(i); ++i)
		{
			if(m_plugin.GetEnabledStatus(i))
			{
				string magic = sfmt("%08x", m_plugin.GetPluginMagic(i));
				if(m_cfg.has("PLUGIN_CFVERSION", magic))
				{
					int plugin_cfversion = m_cfg.getInt("PLUGIN_CFVERSION", magic, 1);
					if(first > 0 && plugin_cfversion != first)
						return m_cfg.getInt(_domainFromView(), "last_cf_mode", 1);
					else if(first == 0)	
						first = plugin_cfversion;
				}
			}
		}
		if(first == 0)
			first++;
		return first;
	}
	return m_cfg.getInt(_domainFromView(), "last_cf_mode", 1);
}

void CMenu::_setCFVersion(int version)
{
	if(m_sourceflow)
		_setSrcFlow(version);
	else if(m_current_view == COVERFLOW_PLUGIN)
	{
		int first = 0;
		for(u8 i = 0; m_plugin.PluginExist(i); ++i)
		{
			if(m_plugin.GetEnabledStatus(i))
			{
				string magic = sfmt("%08x", m_plugin.GetPluginMagic(i));
				if(m_cfg.has("PLUGIN_CFVERSION", magic))
				{
					int plugin_cfversion = m_cfg.getInt("PLUGIN_CFVERSION", magic, 1);
					if(first > 0 && plugin_cfversion != first)
					{
						m_cfg.setInt(_domainFromView(), "last_cf_mode", version);
						return;
					}
					else if(first == 0)	
						first = plugin_cfversion;
				}
			}
		}
		for(u8 i = 0; m_plugin.PluginExist(i); ++i)
		{
			if(m_plugin.GetEnabledStatus(i))
				m_cfg.setInt("PLUGIN_CFVERSION", sfmt("%08x", m_plugin.GetPluginMagic(i)), version);
		}
	}
	else
		m_cfg.setInt(_domainFromView(), "last_cf_mode", version);
}

void CMenu::exitHandler(int ExitTo)
{
	/* Write thumbnail view status and category start page to config */
	m_cfg.setBool(general_domain, "thumbnails", m_thumbnail);
	m_cfg.setInt(general_domain, "cat_startpage", m_catStartPage);
	
	m_exit = true;
	if(ExitTo == EXIT_TO_BOOTMII) // Bootmii, check that the files are there, or ios will hang
	{
		struct stat dummy;
		if(!DeviceHandle.IsInserted(SD) || stat("sd:/bootmii/armboot.bin", &dummy) != 0 || stat("sd:/bootmii/ppcboot.elf", &dummy) != 0)
			ExitTo = EXIT_TO_HBC;
	}
	else if(ExitTo == POWEROFF_CONSOLE)
	{
		ExitTo = m_cfg.getBool(general_domain, "force_standby", false) ? SHUTDOWN_STANDBY : POWEROFF_CONSOLE;
	}
	if(ExitTo != WIIFLOW_DEF) // if not using wiiflows exit option then go ahead and set the exit to
		Sys_ExitTo(ExitTo);
}
