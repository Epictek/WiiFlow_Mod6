
#include <algorithm> // for languages

#include "menu.hpp"
#include "loader/nk.h"

void CMenu::_hideConfigFull(bool instant)
{
	m_btnMgr.hide(m_configLblDialog, instant);
	m_btnMgr.hide(m_wbfsPBar, instant);
	m_btnMgr.hide(m_wbfsLblMessage, instant);
	m_btnMgr.hide(m_configLblNotice, instant);
	
	_hideConfig(instant);
}
void CMenu::_hideConfig(bool instant)
{
	m_btnMgr.hide(m_configLblTitle, instant);
	m_btnMgr.hide(m_configBtnBack, instant);
	m_btnMgr.hide(m_configLblPage, instant);
	m_btnMgr.hide(m_configBtnPageM, instant);
	m_btnMgr.hide(m_configBtnPageP, instant);
	m_btnMgr.hide(m_configBtnCenter, instant);
	
	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.hide(m_configLblUser[i], instant);
	
	_hideConfigPage(instant);
}

void CMenu::_hideConfigPage(bool instant)
{
	for(u8 i = 0; i < 10; ++i)
	{
		m_btnMgr.hide(m_configLbl[i], instant);
		m_btnMgr.hide(m_configBtn[i], instant);
		m_btnMgr.hide(m_configLblVal[i], instant);
		m_btnMgr.hide(m_configBtnM[i], instant);
		m_btnMgr.hide(m_configBtnP[i], instant);
		m_btnMgr.hide(m_configBtnGo[i], instant);
	}
	_hideCheckboxes(instant);
}

void CMenu::_hideCheckboxes(bool instant)
{
	for(u8 i = 0; i < 10; ++i)
		m_btnMgr.hide(m_checkboxBtn[i], instant);
}

void CMenu::_showConfigMain(bool instant)
{
	// bool a = isWiiVC || neek2o();
	
	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);
		
	m_btnMgr.show(m_configBtnBack);

	m_btnMgr.setText(m_configLblTitle, _t("cfg776", L"WiiFlow settings"));
	m_btnMgr.show(m_configLblTitle);

	m_btnMgr.setText(m_configLbl[m_locked ? 4 : 0], _t("cfg5", L"Child lock"));
	m_btnMgr.show(m_configLbl[m_locked ? 4 : 0]); // child lock
	m_btnMgr.show(m_configBtnGo[m_locked ? 4 : 0]);
	
	if(m_locked)
		return;

	m_btnMgr.setText(m_configLbl[1], _t("cfg795", L"User interface"));
	m_btnMgr.setText(m_configLbl[2], _t("cfg793", L"Screen adjustment"));
	m_btnMgr.setText(m_configLbl[3], _t("cfg791", L"Startup and shutdown"));
	m_btnMgr.setText(m_configLbl[4], _t("cfg792", L"Music settings"));
	m_btnMgr.setText(m_configLbl[5], _t("cfgd4", L"Path manager"));
	m_btnMgr.setText(m_configLbl[6], _t("home8", L"File explorer"));
	m_btnMgr.setText(m_configLbl[7], _t("cfgg98", L"Network settings"));
	m_btnMgr.setText(m_configLbl[8], _t("cfg794", L"Misc settings"));
	m_btnMgr.setText(m_configLbl[9], _t("cfgc9", L"WiiFlow language"));
	m_btnMgr.setText(m_configLblVal[9], m_curLanguage);

	for(u8 i = 1; i < 9; ++i)
	{
		m_btnMgr.show(m_configLbl[i], instant);
		m_btnMgr.show(m_configBtnGo[i], instant);
	}
	m_btnMgr.show(m_configLbl[9], instant);
	m_btnMgr.show(m_configLblVal[9], instant);
	m_btnMgr.show(m_configBtnM[9], instant);
	m_btnMgr.show(m_configBtnP[9], instant);
}

template <class T> static inline T loopNum(T i, T s)
{
	return (i + s) % s;
}

/* WF language */
u32 available_pos = 0;
vector<string> languages_available;
void AddLanguage(char *Path)
{
	char lng[32];
	memset(lng, 0, 32);
	char *lang_chr = strrchr(Path, '/') + 1;
	memcpy(lng, lang_chr, min(31u, (unsigned int)(strrchr(lang_chr, '.') - lang_chr)));
	languages_available.push_back(lng);
}

void CMenu::_config(void)
{
	/* WF language */
	languages_available.clear();
	languages_available.push_back("Default");
	GetFiles(m_languagesDir.c_str(), stringToVector(".ini", '|'), AddLanguage, false, 0);
	sort(languages_available.begin(), languages_available.end());
	u32 curLang = 0;
	for(u32 i = 0; i < languages_available.size(); ++i)
	{
		if(m_curLanguage == languages_available[i])
		{
			curLang = i;
			break;
		}
	}
	string prevLanguage = m_curLanguage;

	_setBg(m_configBg, m_configBg);
	SetupInput();
	_showConfigMain();
	
	while(!m_exit)
	{
		_mainLoopCommon(true);
		if(BTN_HOME_HELD || BTN_B_OR_1_PRESSED)
			break;
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
			m_btnMgr.up();
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
			m_btnMgr.down();

		if(BTN_A_OR_2_PRESSED)
		{
			if(m_btnMgr.selected(m_configBtnBack))
				break;

			//! CHILD LOCK
			else if(m_btnMgr.selected(m_configBtnGo[m_locked ? 4 : 0]))
			{
				char code[4];
				_hideConfig(true);
				if(!m_locked)
				{
					if(_code(code))
					{
						m_refreshGameList = true;
						m_cfg.setString(general_domain, "parent_code", string(code, 4));
						m_locked = true;
					}
				}
				else
				{
					if(_code(code))
					{
						if(memcmp(code, m_cfg.getString(general_domain, "parent_code", "").c_str(), 4) == 0)
						{
							m_refreshGameList = true;
							m_locked = false;
							_error(_t("errcfg12",L"WiiFlow unlocked until next reboot."));
						}
						else
							_error(_t("cfgg25",L"Password incorrect."));
					}
				}
				_showConfigMain();
			}
			//! GUI SETTINGS
			else if(m_btnMgr.selected(m_configBtnGo[1]))
			{
				_hideConfig(true);
				_configGui();
				_showConfigMain();
			}
			//! SCREEN SETTINGS
			else if(m_btnMgr.selected(m_configBtnGo[2]))
			{
				_hideConfig(true);
				_configScreen();
				_showConfigMain();
			}
			//! STARTUP AND SHUTDOWN SETTINGS
			else if(m_btnMgr.selected(m_configBtnGo[3]) && !(isWiiVC || neek2o()))
				
			{
				_hideConfig(true);
				_configBoot();
				_showConfigMain();
			}
			//! MUSIC SETTINGS
			else if(m_btnMgr.selected(m_configBtnGo[4]))
			{
				_hideConfig(true);
				_configMusic();
				_showConfigMain();
			}
			//! CUSTOM PATHS
			else if(m_btnMgr.selected(m_configBtnGo[5]))
			{
				_hideConfig(true);
				_configPaths();
				_showConfigMain();
			}
			//! FILE EXPLORER
			else if(m_btnMgr.selected(m_configBtnGo[6]))
			{
				_hideConfig(true);
				_Explorer();
				_showConfigMain();
			}
			//! NETWORK SETTINGS
			else if(m_btnMgr.selected(m_configBtnGo[7]))
			{
				_hideConfig(true);
				_configNet();
				_showConfigMain();
			}
			//! MISC SETTINGS
			else if(m_btnMgr.selected(m_configBtnGo[8]))
			{
				_hideConfig(true);
				_configMisc();
				_showConfigMain();
			}
			//! WIIFLOW LANGUAGE
			else if(m_btnMgr.selected(m_configBtnP[9]) || m_btnMgr.selected(m_configBtnM[9]))
			{
				s8 direction = m_btnMgr.selected(m_configBtnP[9]) ? 1 : -1;
				if(languages_available.size() > 1)
				{
					m_loc.unload();
					curLang = loopNum(curLang + direction, (u32)languages_available.size());
					m_curLanguage = languages_available[curLang];
					if(m_curLanguage != "Default")
						m_loc.load(fmt("%s/%s.ini", m_languagesDir.c_str(), m_curLanguage.c_str()));
					m_cfg.setString(general_domain, "language", m_curLanguage);
					_updateText();
					_showConfigMain(true);
				}
			}
		}
	}
	
	/* WF language */
	if(m_curLanguage != prevLanguage)
	{
		m_cacheList.Init(m_settingsDir.c_str(), m_loc.getString(m_curLanguage, "gametdb_code", "EN").c_str(), m_pluginDataDir.c_str());
		//! delete cache lists folder and remake it so all lists update
		fsop_deleteFolder(m_listCacheDir.c_str());
		fsop_MakeFolder(m_listCacheDir.c_str());
		m_refreshGameList = true;
	}
	
	_hideConfig(true);
}

void CMenu::_initConfigMenu()
{
	_addUserLabels(m_configLblUser, ARRAY_SIZE(m_configLblUser), "CONFIG");

	m_configBg = _texture("CONFIG/BG", "texture", theme.bg, false);
	m_configLblTitle = _addTitle("CONFIG/TITLE", theme.titleFont, L"", 0, 10, 640, 60, theme.titleFontColor, FTGX_JUSTIFY_CENTER | FTGX_ALIGN_MIDDLE);
	
	for(u8 i = 0; i < 10; ++i)
	{
		char *configText = fmt_malloc("CONFIG/LINE%i_%%s", i);
		if(configText == NULL) 
			continue;
		m_configLbl[i] = _addLabel(fmt(configText, "LBL"), theme.lblFont, L"", 60, 80 + (i * 32), 340, 32, theme.lblFontColor, FTGX_JUSTIFY_CENTER | FTGX_ALIGN_MIDDLE, theme.btnTexC);
		m_configBtn[i] = _addButton(fmt(configText, "BTN"), theme.btnFont, L"", 400, 80 + (i * 32), 180, 32, theme.btnFontColor);
		m_configLblVal[i] = _addLabel(fmt(configText, "VAL"), theme.btnFont, L"", 440, 80 + (i * 32), 100, 32, theme.btnFontColor, FTGX_JUSTIFY_CENTER | FTGX_ALIGN_MIDDLE, theme.btnTexC);
		m_configBtnM[i] = _addPicButton(fmt(configText, "MINUS"), theme.btnTexMinus, theme.btnTexMinusS, 400, 80 + (i * 32), 40, 32);
		m_configBtnP[i] = _addPicButton(fmt(configText, "PLUS"), theme.btnTexPlus, theme.btnTexPlusS, 540, 80 + (i * 32), 40, 32);
		m_configChkOff[i] = _addPicButton(fmt(configText, "CHECKBOX_OFF"), theme.checkboxoff, theme.checkboxoffs, 400, 80 + (i * 32), 180, 32);
		m_configChkOn[i] = _addPicButton(fmt(configText, "CHECKBOX_ON"), theme.checkboxon, theme.checkboxons, 400, 80 + (i * 32), 180, 32);
		m_configChkReq[i] = _addPicButton(fmt(configText, "CHECKBOX_REQ"), theme.checkboxReq, theme.checkboxReqs, 400, 80 + (i * 32), 180, 32);
		m_configChkHid[i] = _addPicButton(fmt(configText, "CHECKBOX_HID"), theme.checkboxHid, theme.checkboxHids, 400, 80 + (i * 32), 180, 32);
		m_configBtnGo[i] = _addPicButton(fmt(configText, "GO"), theme.btnTexGo, theme.btnTexGoS, 400, 80 + (i * 32), 180, 32);

		_setHideAnim(m_configLbl[i], fmt(configText, "LBL"), 0, 0, 1.f, -1.f);
		_setHideAnim(m_configBtn[i], fmt(configText, "BTN"), 0, 0, 1.f, -1.f);
		_setHideAnim(m_configLblVal[i], fmt(configText, "VAL"), 0, 0, 1.f, -1.f);
		_setHideAnim(m_configBtnM[i], fmt(configText, "MINUS"), 0, 0, 1.f, -1.f);
		_setHideAnim(m_configBtnP[i], fmt(configText, "PLUS"), 0, 0, 1.f, -1.f);
		_setHideAnim(m_configChkOff[i], fmt(configText, "CHECKBOX_OFF"), 0, 0, 1.f, -1.f);
		_setHideAnim(m_configChkOn[i], fmt(configText, "CHECKBOX_ON"), 0, 0, 1.f, -1.f);
		_setHideAnim(m_configChkReq[i], fmt(configText, "CHECKBOX_REQ"), 0, 0, 1.f, -1.f);
		_setHideAnim(m_configChkHid[i], fmt(configText, "CHECKBOX_HID"), 0, 0, 1.f, -1.f);
		_setHideAnim(m_configBtnGo[i], fmt(configText, "GO"), 0, 0, 1.f, -1.f);		
		m_checkboxBtn[i] = m_configChkOff[i];
		
		MEM2_free(configText);
	}	

	m_configLblPage = _addLabel("CONFIG/PAGE_BTN", theme.btnFont, L"", 68, 410, 104, 50, theme.btnFontColor, FTGX_JUSTIFY_CENTER | FTGX_ALIGN_MIDDLE, theme.btnTexC);
	m_configBtnPageM = _addPicButton("CONFIG/PAGE_MINUS", theme.btnTexMinus, theme.btnTexMinusS, 20, 410, 48, 50);
	m_configBtnPageP = _addPicButton("CONFIG/PAGE_PLUS", theme.btnTexPlus, theme.btnTexPlusS, 172, 410, 48, 50);

	m_configBtnCenter = _addButton("CONFIG/CENTER_BTN", theme.btnFont, L"", 220, 410, 200, 50, theme.btnFontColor); // Start, OK, Delete...
	m_configLblDialog = _addLabel("CONFIG/DIALOG", theme.lblFont, L"", 20, 170, 600, 20, theme.lblFontColor, FTGX_JUSTIFY_CENTER | FTGX_ALIGN_MIDDLE);
	m_configBtnBack = _addButton("CONFIG/BACK_BTN", theme.btnFont, L"", 420, 410, 200, 50, theme.btnFontColor);
	
	m_configLblNotice = _addLabel("CONFIG/NOTICE", theme.lblFont, L"", 20, 400, 200, 60, theme.lblFontColor, FTGX_JUSTIFY_LEFT | FTGX_ALIGN_MIDDLE);

	
	_setHideAnim(m_configLblTitle, "CONFIG/TITLE", 0, 0, -2.f, 0.f);

	_setHideAnim(m_configLblPage, "CONFIG/PAGE_BTN", 0, 0, 1.f, -1.f);
	_setHideAnim(m_configBtnPageM, "CONFIG/PAGE_MINUS", 0, 0, 1.f, -1.f);
	_setHideAnim(m_configBtnPageP, "CONFIG/PAGE_PLUS", 0, 0, 1.f, -1.f);

	_setHideAnim(m_configBtnCenter, "CONFIG/CENTER_BTN", 0, 0, 1.f, -1.f);
	_setHideAnim(m_configLblDialog, "CONFIG/DIALOG", 0, 0, -2.f, 0.f); // error, wad, wbfs, cache covers...
	_setHideAnim(m_configBtnBack, "CONFIG/BACK_BTN", 0, 0, 1.f, -1.f);
	
	_setHideAnim(m_configLblNotice, "CONFIG/NOTICE", 50, 0, -2.f, 0.f);

	// _hideConfig(true);
	_textConfig();
}

void CMenu::_textConfig(void)
{
	m_btnMgr.setText(m_configBtnBack, _t("back", L"Back")); // must be set early (if source menu on start)
}

/********************************************************************************************************/

void CMenu::_cacheCovers()
{
	CoverFlow.stopCoverLoader(true);
	
	char coverPath[MAX_FAT_PATH];
	char wfcPath[MAX_FAT_PATH];
	char cachePath[64];
	
	u32 total = m_gameList.size();
	m_thrdTotal = total;
	u32 index = 0;
	
	bool smallBox = false;
	if(m_sourceflow)
		smallBox = m_cfg.getBool(sourceflow_domain, "smallbox", true);
	else if(m_current_view == COVERFLOW_HOMEBREW || (m_current_view == COVERFLOW_PLUGIN && enabledPluginsCount == 1 && m_plugin.GetEnabledStatus(HB_PMAGIC)))
		smallBox = m_cfg.getBool(homebrew_domain, "smallbox", true);
	
	for(vector<dir_discHdr>::iterator hdr = m_gameList.begin(); hdr != m_gameList.end(); ++hdr)
	{
		index++;
		update_pThread(index, false);
		m_thrdMessage = wfmt(_fmt("dlmsg31", L"Loading item %i of %i"), index, total);
		m_thrdMessageAdded = true;
		
		bool blankCover;
		bool fullCover;
		bool thumbNail = (hdr->type == TYPE_PLUGIN && m_platform.loaded());
		u8 i = 0;

		for(i = 0; i < thumbNail + 1; ++i) // 2 passes, first for normal cover, second for thumbnail
		{
			blankCover = false;
			fullCover = true;
			/* Get cover PNG path: example for the Atari ST game "Another World" */
			strlcpy(coverPath, getBoxPath(&(*hdr), true), sizeof(coverPath)); // (fullname = true)
			//! result should be: ".../boxcovers/[coverfolder]/Another World (1991)(Delphine)(Disk 1 of 2)[cr Elite].st.png"
			
			if(i == 0) // normal box & front cover pass, test multiple cover paths in case first one does not exist
			{
				//! 1) first test if ".../boxcovers/[coverfolder]/Another World (1991)(Delphine)(Disk 1 of 2)[cr Elite].st.png" exists
				if(!fsop_FileExist(coverPath) || smallBox)
				{
					//! 2) then test if ".../covers/[coverfolder]/Another World (1991)(Delphine)(Disk 1 of 2)[cr Elite].st.png" exists
					fullCover = false;
					strlcpy(coverPath, getFrontPath(&(*hdr), true, smallBox, false), sizeof(coverPath)); // (fullname = true)
					if(!fsop_FileExist(coverPath) || smallBox)
					{
						//! 3) then test if ".../boxcovers/[coverfolder]/Another World.png" exists (added)
						fullCover = true;
						strlcpy(coverPath, getBoxPath(&(*hdr), false), sizeof(coverPath)); // (fullname = false)
						if(!fsop_FileExist(coverPath) || smallBox)
						{
							//! 4) then test if ".../covers/[coverfolder]/Another World.png" exists (added)
							fullCover = false;
							strlcpy(coverPath, getFrontPath(&(*hdr), false, smallBox, false), sizeof(coverPath)); // (fullname = false)
							if(!fsop_FileExist(coverPath) && !smallBox)
							{
								//! 5) finally test if ".../boxcovers/blank_covers/ATARIST.png" exists
								fullCover = true;				
								strlcpy(coverPath, getBlankCoverPath(&(*hdr)), sizeof(coverPath));
								blankCover = true;
							}
						}
					}
				}
			}
			else // i == 1, thumbnail pass, replace coverPath with "/snapshots/ATARIST/[gameTDB_name].png" using platform
			{
				fullCover = false;
				strlcpy(coverPath, getFrontPath(&(*hdr), false, smallBox, true), sizeof(coverPath));
				//! then test blank cover
				if(!fsop_FileExist(coverPath))
				{
					strlcpy(coverPath, getBlankCoverPath(&(*hdr)), sizeof(coverPath));
					blankCover = true;
				}
			}
			if(!fsop_FileExist(coverPath))
				continue;

			/* Get cache folder path */
			if(blankCover) // added
				snprintf(cachePath, sizeof(cachePath), "%s/blank_covers", m_cacheDir.c_str());
			else if(hdr->type == TYPE_PLUGIN)
				snprintf(cachePath, sizeof(cachePath), "%s/%s", m_cacheDir.c_str(), m_plugin.GetCoverFolderName(hdr->settings[0]));
			else if(m_sourceflow)
				snprintf(cachePath, sizeof(cachePath), "%s/sourceflow", m_cacheDir.c_str());
			else if(hdr->type == TYPE_HOMEBREW)
				snprintf(cachePath, sizeof(cachePath), "%s/homebrew", m_cacheDir.c_str());
			else
				snprintf(cachePath, sizeof(cachePath), "%s", m_cacheDir.c_str());

			/* Get game name or ID */
			const char *gameNameOrID = NULL;
			if(!blankCover)
				gameNameOrID = CoverFlow.getFilenameId(&(*hdr)); // &(*hdr) converts iterator to pointer to mem address
			else
				gameNameOrID = strrchr(coverPath, '/') + 1;
				
			/* Get cover wfc path */
			if(smallBox || i == 1)
				snprintf(wfcPath, sizeof(wfcPath), "%s/%s_small.wfc", cachePath, gameNameOrID);
			else
				snprintf(wfcPath, sizeof(wfcPath), "%s/%s.wfc", cachePath, gameNameOrID);

			/* If wfc doesn't exist or is flat and have full cover */
			if(!fsop_FileExist(wfcPath) || (!CoverFlow.fullCoverCached(wfcPath) && fullCover))
			{
				//! Create cache subfolders if needed
				fsop_MakeFolder(cachePath);
				//! Create cover texture
				CoverFlow.cacheCoverFile(wfcPath, coverPath, fullCover);
			}
		}
		
		/* Cache wii and channel banners */
		if(hdr->type == TYPE_WII_GAME || hdr->type == TYPE_CHANNEL || hdr->type == TYPE_EMUCHANNEL)
		{
			CurrentBanner.ClearBanner();
			char cached_banner[256];
			strlcpy(cached_banner, fmt("%s/%s.bnr", m_bnrCacheDir.c_str(), hdr->id), sizeof(cached_banner));
			if(fsop_FileExist(cached_banner))
				continue;

			if(hdr->type == TYPE_WII_GAME)
				_extractBnr(&(*hdr));
			else // channel or emuchannel
				ChannelHandle.GetBanner(TITLE_ID(hdr->settings[0], hdr->settings[1]));
			
			if(CurrentBanner.IsValid())
				fsop_WriteFile(cached_banner, CurrentBanner.GetBannerFile(), CurrentBanner.GetBannerFileSize());
		}
	}
	CurrentBanner.ClearBanner();
	CoverFlow.startCoverLoader();
}

const char *CMenu::getBoxPath(const dir_discHdr *element, bool fullName)
{
	if(element->type == TYPE_PLUGIN)
	{
		string CleanName(element->path);
			CleanName = CleanName.substr(CleanName.find_last_of("/") + 1);
		if(!fullName)
		{
			CleanName = CleanName.substr(0, CleanName.find_last_of("."));
			//! Remove common suffixes (parenthesis, brackets, disk #)
			CleanName = CleanName.substr(0, CleanName.find(" (")).substr(0, CleanName.find(" [")).substr(0, CleanName.find("_Disk"));
		}
		const char *coverFolder = m_plugin.GetCoverFolderName(element->settings[0]);
		return fmt("%s/%s/%s.png", m_boxPicDir.c_str(), coverFolder, CleanName.c_str());
	}
	else if(element->type == TYPE_HOMEBREW)
		return fmt("%s/homebrew/%s.png", m_boxPicDir.c_str(), strrchr(element->path, '/') + 1);
	else if(element->type == TYPE_SOURCE) // sourceflow
	{
		const char *coverImg = strrchr(element->path, '/') + 1;
		if(coverImg == NULL)
			return NULL;
		return fmt("%s/full_covers/%s", m_sourceDir.c_str(), coverImg);
	}
	return fmt("%s/%s.png", m_boxPicDir.c_str(), element->id);
}

const char *CMenu::getFrontPath(const dir_discHdr *element, bool fullName, bool smallBox, bool thumbNail) // added smallbox & thumbNail
{
	if(element->type == TYPE_PLUGIN)
	{
		if(thumbNail)
		{
			if(m_platform.loaded())
			{
				GameTDB gametdb;
				char GameID[7];
				GameID[6] = '\0';
				char platformName[16];
				const char *TMP_Char = NULL;
				strncpy(m_plugin.PluginMagicWord, fmt("%08x", element->settings[0]), 8);
				snprintf(platformName, sizeof(platformName), "%s", m_platform.getString("PLUGINS", m_plugin.PluginMagicWord).c_str());
				strcpy(GameID, element->id);
				
				//! Check COMBINED
				string newName = m_platform.getString("COMBINED", platformName);
				if(newName.empty())
					m_platform.remove("COMBINED", platformName);
				else
					snprintf(platformName, sizeof(platformName), "%s", newName.c_str());
				
				gametdb.OpenFile(fmt("%s/%s/%s.xml", m_pluginDataDir.c_str(), platformName, platformName));
				if(gametdb.IsLoaded())
				{
					gametdb.SetLanguageCode(m_loc.getString(m_curLanguage, "gametdb_code", "EN").c_str());
					const char *coverPath = NULL;
					//! Check ARCADE systems
					if(strcasestr(platformName, "ARCADE") || strcasestr(platformName, "CPS") || !strncasecmp(platformName, "NEOGEO", 6))
					{
						string ShortName;
						if(strrchr(element->path, '/') != NULL)
							ShortName = m_plugin.GetRomName(element->path);
						else
						{
							char title[64];
							wcstombs(title, element->title, 63);
							title[63] = '\0';
							ShortName = title;
						}
						
						coverPath = fmt("%s/%s/%s.png", m_snapDir.c_str(), platformName, ShortName.c_str());
					}
					else if(gametdb.GetName(GameID, TMP_Char))
						coverPath = fmt("%s/%s/%s.png", m_snapDir.c_str(), platformName, TMP_Char);
					
					gametdb.CloseFile();
					
					if(coverPath == NULL || !fsop_FileExist(coverPath))
						coverPath = fmt("%s/%s/%s.png", m_snapDir.c_str(), platformName, GameID);
					
					return coverPath;
				}
			}
		}
		else // not thumbnail
		{
			string CleanName(element->path);
				CleanName = CleanName.substr(CleanName.find_last_of("/") + 1);
			if(!fullName)
			{
				CleanName = CleanName.substr(0, CleanName.find_last_of("."));
				//! Remove common suffixes (parenthesis, brackets, disk #)
				CleanName = CleanName.substr(0, CleanName.find(" (")).substr(0, CleanName.find(" [")).substr(0, CleanName.find("_Disk"));
			}
			const char *coverFolder = m_plugin.GetCoverFolderName(element->settings[0]);
			return fmt("%s/%s/%s.png", m_picDir.c_str(), coverFolder, CleanName.c_str());
		}
	}
	else if(element->type == TYPE_HOMEBREW)
	{
		if(smallBox)
		{
			const char *coverPath = fmt("%s/homebrew_small/%s.png", m_picDir.c_str(), strrchr(element->path, '/') + 1);
			if(!fsop_FileExist(coverPath))
				return fmt("%s/icon.png", element->path);
			else
				return coverPath;
		}
		else
			return fmt("%s/homebrew/%s.png", m_picDir.c_str(), strrchr(element->path, '/') + 1);
	}
	else if(element->type == TYPE_SOURCE) // sourceflow
	{
		const char *coverImg = strrchr(element->path, '/') + 1;
		if(coverImg == NULL)
			return NULL;
		const char *coverPath = fmt("%s/front_covers/%s", m_sourceDir.c_str(), coverImg);
		if(smallBox || !fsop_FileExist(coverPath))
		{
			string themeName = m_cfg.getString(general_domain, "theme", "default");
			coverPath = fmt("%s/small_covers/%s/%s", m_sourceDir.c_str(), themeName.c_str(), coverImg);
			if(!fsop_FileExist(coverPath))
			{
				coverPath = fmt("%s/small_covers/%s", m_sourceDir.c_str(), coverImg);
				if(!fsop_FileExist(coverPath))
					return element->path;
			}
		}
		return coverPath;
	}
	return fmt("%s/%s.png", m_picDir.c_str(), element->id);
}

static char blankCoverPath[MAX_FAT_PATH];
const char *CMenu::getBlankCoverPath(const dir_discHdr *element)
{
	string blankCoverTitle = "wii";
	if(m_platform.loaded())
	{
		switch(element->type)
		{
			case TYPE_CHANNEL:
				strncpy(m_plugin.PluginMagicWord, NAND_PMAGIC, 9);
				break;
			case TYPE_EMUCHANNEL:
				strncpy(m_plugin.PluginMagicWord, ENAND_PMAGIC, 9);
				break;
			case TYPE_HOMEBREW:
				strncpy(m_plugin.PluginMagicWord, HB_PMAGIC, 9);
				break;
			case TYPE_GC_GAME:
				strncpy(m_plugin.PluginMagicWord, GC_PMAGIC, 9);
				break;
			case TYPE_PLUGIN:
				strncpy(m_plugin.PluginMagicWord, fmt("%08x", element->settings[0]), 8);
				break;
			default: // wii
				strncpy(m_plugin.PluginMagicWord, WII_PMAGIC, 9);
		}
		blankCoverTitle = m_platform.getString("PLUGINS", m_plugin.PluginMagicWord, "wii");
	}
	snprintf(blankCoverPath, sizeof(blankCoverPath), "%s/blank_covers/%s.png", m_boxPicDir.c_str(), blankCoverTitle.c_str());
	if(!fsop_FileExist(blankCoverPath))
		snprintf(blankCoverPath, sizeof(blankCoverPath), "%s/blank_covers/%s.jpg", m_boxPicDir.c_str(), blankCoverTitle.c_str());
	return blankCoverPath;
}

