
#include "menu.hpp"
#include "wstringEx/wstringEx.hpp"

vector<char> m_categories;
static u8 curPage;
char id[64];
char catDomain[16];
bool gameSet = false;
bool renameCat = false;
bool exit_renameCat = false;

void CMenu::_showCategorySettings(void)
{
	m_btnMgr.setText(m_configLblTitle, renameCat ? _t("cfg832", L"Rename category") : gameSet ? CoverFlow.getTitle() : _t("cat1", L"Select categories"));
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.setText(m_configBtnCenter, gameSet ? _t("cfg837", L"Options") : _t("none", L"None"));
	if(!renameCat)
		m_btnMgr.show(m_configBtnCenter);
	m_btnMgr.show(m_configBtnBack);
	
	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);
		
	_updateCatCheckboxes();
}

void CMenu::_updateCatCheckboxes(void)
{
	for(u8 i = 0; i < 10; ++i)
	{
		m_btnMgr.hide(m_configLbl[i], true);
		m_btnMgr.hide(m_checkboxBtn[i], true);
		m_btnMgr.hide(m_configBtnGo[i], true);
	}
	
	if(m_max_categories > 11)
	{
		m_btnMgr.setText(m_configLblPage, wfmt(L"%i / %i", curPage, ((m_max_categories - 2) / 10) + 1));
		m_btnMgr.show(m_configLblPage);
		m_btnMgr.show(m_configBtnPageM);
		m_btnMgr.show(m_configBtnPageP);
	}
	for(u8 i = 0; i < 10; ++i)
	{
		int j = i + 1 + ((curPage - 1) * 10);
		if(j == m_max_categories)
			break;
		if(!renameCat)
		{
			switch(m_categories[j])
			{
				case '0':
					m_checkboxBtn[i] = m_configChkOff[i];
					break;
				case '1':
					m_checkboxBtn[i] = m_configChkOn[i];
					break;
				case '2':
					m_checkboxBtn[i] = m_configChkHid[i];
					break;
				default:
					m_checkboxBtn[i] = m_configChkReq[i];
			}
			m_btnMgr.show(m_checkboxBtn[i]);
		}
		else
			m_btnMgr.show(m_configBtnGo[i]);
		
		//! display translated category name if available
		string keyCategName = m_cat.getString(general_domain, fmt("cat%d", j));
		wstringEx defCategName;
		defCategName.fromUTF8(keyCategName);
		wstringEx categName = m_loc.getWString(m_curLanguage, keyCategName, defCategName);
		m_btnMgr.setText(m_configLbl[i], categName);
		m_btnMgr.show(m_configLbl[i]);
	}
}

void CMenu::_getGameCategories(const dir_discHdr *hdr)
{	
	memset(catDomain, 0, 16);
	switch(hdr->type)
	{
		case TYPE_CHANNEL:
			sprintf(catDomain, "NAND");
			break;
		case TYPE_EMUCHANNEL:
			sprintf(catDomain, "CHANNELS");
			break;
		case TYPE_HOMEBREW:
			sprintf(catDomain, "HOMEBREW");
			break;
		case TYPE_GC_GAME:
			sprintf(catDomain, "GAMECUBE");
			break;
		case TYPE_WII_GAME:
			sprintf(catDomain, "WII");
			break;
		default:
			//! fix PluginMagicWord when multiple plugins are selected
			strncpy(m_plugin.PluginMagicWord, fmt("%08x", hdr->settings[0]), 8); 
			sprintf(catDomain, m_plugin.PluginMagicWord);
	}

	memset(id, 0, 64);
	if(NoGameID(hdr->type))
	{
		string romFileName(hdr->path);
		romFileName = romFileName.substr(romFileName.find_last_of("/") + 1);
		romFileName = romFileName.substr(0, romFileName.find_last_of("."));
		strncpy(id, romFileName.c_str(), 63);
	}
	else
		strcpy(id, hdr->id);

	const char *gameCats = m_cat.getString(catDomain, id, "").c_str();
	if(strlen(gameCats) > 0)
	{
		for(u8 j = 0; j < strlen(gameCats); ++j)
		{
			int k = (static_cast<int>(gameCats[j])) - 32;
			m_categories.at(k) = '1';
		}
	}
	else
		m_cat.remove(catDomain, id);
}

void CMenu::_setGameCategories(void)
{
	string gameCats = "";
	for(int i = 1; i < m_max_categories; i++)
	{
		if(m_categories.at(i) == '1')
		{
			char cCh = static_cast<char>(i + 32);
			gameCats += cCh;
		}
	}
	m_cat.setString(catDomain, id, gameCats);
}

void CMenu::_CategorySettings(bool fromGameSet)
{
	exit_renameCat = false;
	gameSet = fromGameSet;
	
	_setBg(m_configBg, m_configBg);
	
	SetupInput();

	curPage = m_catStartPage;
	if(curPage < 1 || curPage > (((m_max_categories - 2)/ 10) + 1))
		curPage = 1;
	m_categories.resize(m_max_categories, '0');
	m_categories.assign(m_max_categories, '0');

	if(fromGameSet)
		_getGameCategories(CoverFlow.getHdr());
	else
	{
		string requiredCats = m_cat.getString(general_domain, "required_categories", "");
		string selectedCats = m_cat.getString(general_domain, "selected_categories", "");
		string hiddenCats = m_cat.getString(general_domain, "hidden_categories", "");
		u8 numReqCats = requiredCats.length();
		u8 numSelCats = selectedCats.length();
		u8 numHidCats = hiddenCats.length();
		
		if(numReqCats != 0)
		{
			for(u8 j = 0; j < numReqCats; ++j)
			{
				int k = (static_cast<int>(requiredCats[j])) - 32;
				m_categories.at(k) = '3';
			}
		}
		if(numSelCats != 0)
		{
			for(u8 j = 0; j < numSelCats; ++j)
			{
				int k = (static_cast<int>(selectedCats[j])) - 32;
				m_categories.at(k) = '1';
			}
		}
		if(numHidCats != 0)
		{
			for(u8 j = 0; j < numHidCats; ++j)
			{
				int k = (static_cast<int>(hiddenCats[j])) - 32;
				m_categories.at(k) = '2';
			}
		}
	}
	_showCategorySettings();
	
	while(!m_exit)
	{
		_mainLoopCommon();
		CoverFlow.tick();
			
		if(BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnBack)))
		{
			if(!fromGameSet)
			{
				string newReqCats = "";
				string newSelCats = "";
				string newHidCats = "";
				for(int i = 1; i < m_max_categories; i++)
				{
					if(m_categories.at(i) == '1')
					{
						char cCh = static_cast<char>( i + 32);
						newSelCats = newSelCats + cCh;
					}
					else if(m_categories.at(i) == '2')
					{
						char cCh = static_cast<char>( i + 32);
						newHidCats = newHidCats + cCh;
					}
					else if(m_categories.at(i) == '3')
					{
						char cCh = static_cast<char>( i + 32);
						newReqCats = newReqCats + cCh;
					}
				}
				m_cat.setString(general_domain, "selected_categories", newSelCats);
				m_cat.setString(general_domain, "hidden_categories", newHidCats);
				m_cat.setString(general_domain, "required_categories", newReqCats);
			}
			else // from game settings
			{
				_setGameCategories();
			}
			break;
		}
		else if((BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED) && !ShowPointer())
			m_btnMgr.up();
		else if((BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED) && !ShowPointer())
			m_btnMgr.down();
		
		else if(fromGameSet && !renameCat && ShowPointer() && (wBtn_Pressed(WPAD_BUTTON_RIGHT, WPAD_EXP_NONE) || wBtn_Pressed(WPAD_BUTTON_LEFT, WPAD_EXP_NONE)))
		{
			bool left = wBtn_Pressed(WPAD_BUTTON_LEFT, WPAD_EXP_NONE);
			_setGameCategories();
			_hideConfig();
			left ? CoverFlow.up() : CoverFlow.down();
			curPage = 1;
			m_newGame = true;
			m_categories.assign(m_max_categories, '0');
			_playGameSound(); // changes banner and game sound
			_getGameCategories(CoverFlow.getHdr());
			_showCategorySettings();
		}
		
		else if((BTN_MINUS_PRESSED && m_max_categories > 11) || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnPageM)))
		{
			// m_btnMgr.noHover(true);
			curPage = curPage == 1 ? ((m_max_categories - 2) / 10) + 1 : curPage - 1;
			if(BTN_MINUS_PRESSED)
				m_btnMgr.click(m_configBtnPageM);
			_updateCatCheckboxes();
		}
		else if((BTN_PLUS_PRESSED && m_max_categories > 11) || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnPageP)))
		{
			// m_btnMgr.noHover(true);
			curPage = curPage == ((m_max_categories - 2) / 10) + 1 ? 1 : curPage + 1;
			if(BTN_PLUS_PRESSED)
				m_btnMgr.click(m_configBtnPageP);
			_updateCatCheckboxes();
		}
		else if(BTN_A_OR_2_PRESSED)
		{
			if(m_btnMgr.selected(m_configBtnCenter))
			{
				if(!fromGameSet) // uncheck all categories
				{
					m_refreshGameList = true;
					bool hiddenCat = false;
					for(int j = 1; j < m_max_categories; ++j)
					{
						if(m_categories.at(j) == '2')
						{	
							hiddenCat = true;
							continue;
						}
						m_categories.at(j) = '0';
					}
					if(!hiddenCat)
						m_categories.at(0) = '1';
					_updateCatCheckboxes();
				}
				else // settings
				{
					_setGameCategories();
					_hideConfig(true);
					_CategoryConfig();
					_getGameCategories(CoverFlow.getHdr());
					_showCategorySettings();
				}
			}
			
			for(u8 i = 0; i < 10; ++i)
			{
				if(m_btnMgr.selected(m_checkboxBtn[i])) // set category
				{
					m_refreshGameList = true;
					int j = i + 1 + ((curPage - 1) * 10);
					if(fromGameSet)
					{
						m_categories.at(j) = m_categories.at(j) == '0' ? '1' : '0';
					}
					else
					{
						if(m_locked && m_categories.at(j) == '2')
							m_categories.at(j) = '3'; // instead of 1
						else if(m_locked && m_categories.at(j) == '3') // instead of 1
							m_categories.at(j) = '2';
						m_categories.at(j) = m_categories.at(j) == '0' ? '1' : m_categories.at(j) == '1' ? '3' : m_categories.at(j) == '3' ? '2' : '0'; // changed cycle, was previously 0 / 1 / 1 / 2 / 2 / 3
						if(m_categories.at(0) == '1' && m_categories.at(j) != '0')
							m_categories.at(0) = '0';
					}
					m_btnMgr.hide(m_checkboxBtn[i], true);
					switch(m_categories.at(j))
					{
						case '0': // off
							m_checkboxBtn[i] = m_configChkOff[i];
							break;
						case '1': // on
							m_checkboxBtn[i] = m_configChkOn[i];
							break;
						case '2': // hidden
							m_checkboxBtn[i] = m_configChkHid[i];
							break;
						default: // required
							m_checkboxBtn[i] = m_configChkReq[i];
							break;
					}
					m_btnMgr.show(m_checkboxBtn[i], true);
					m_btnMgr.setSelected(m_checkboxBtn[i]);
					break;
				}
				
				if(m_btnMgr.selected(m_configBtnGo[i])) // choose category to rename
				{
					_hideConfig(true);
					char *c = NULL;
					c = _keyboard();
					if(strlen(c) > 0)
					{
						string s = capitalize(lowerCase(c));
						if(_error(wfmt(_fmt("errcfg10", L"Categories used by GameTDB will be recreated if renamed. Rename category to: %s?"), s.c_str()), true))
						{
							int j = i + 1 + ((curPage - 1) * 10);
							m_cat.setString(general_domain, fmt("cat%d", j), s);
							exit_renameCat = true;
						}
					}
					_showCategorySettings();
					break;
				}
			}
		}
		if(exit_renameCat)
			break;
	}
	
	_hideConfig(true);
}

/*********************************************************************************************************/

void CMenu::_showCategoryConfig(bool instant)
{
	_hideCheckboxes(true); // reset checkboxes
	
	m_btnMgr.setText(m_configLblTitle, _t("cfg828", L"Category options"));
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_configBtnBack);
	
	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);
	
	m_btnMgr.setText(m_configLbl[2], _t("cfg829", L"Uncheck all for this game"));
	m_btnMgr.setText(m_configBtn[2], _t("set", L"Set"));
	
	m_btnMgr.setText(m_configLbl[3], _t("cfg830", L"GameTDB categories for this game"));
	m_btnMgr.setText(m_configBtn[3], _t("set", L"Set"));
	
	m_btnMgr.setText(m_configLbl[4], _t("cfg831", L"GameTDB categories for all games"));
	m_checkboxBtn[4] = m_cfg.getBool(general_domain, "tdb_genres", 0) == 0 ? m_configChkOff[4] : m_configChkOn[4];
	m_btnMgr.show(m_checkboxBtn[4], instant);
	
	m_btnMgr.setText(m_configLbl[5], _t("cfg820", L"Add new category"));
	
	m_btnMgr.setText(m_configLbl[6], _t("cfg832", L"Rename category"));
	
	for(u8 i = 2; i < 7; ++i)
	{
		m_btnMgr.show(m_configLbl[i], instant);
		if(i < 4)
			m_btnMgr.show(m_configBtn[i], instant);
		else if(i > 4)
			m_btnMgr.show(m_configBtnGo[i], instant);
	}
}

void CMenu::_CategoryConfig(void)
{
	bool prevGenres = tdb_genres;
	
	SetupInput();
	
	_showCategoryConfig();
	
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
			else if(m_btnMgr.selected(m_configBtn[2])) // erase categories for this game
			{
				if(_error(_t("errcfg5", L"Are you sure?"), true))
				{
					m_refreshGameList = true;
					m_categories.assign(m_max_categories, '0');
					_setGameCategories();
					break;
				}
				_showCategoryConfig();
			}
			else if(m_btnMgr.selected(m_configBtn[3]) || m_btnMgr.selected(m_checkboxBtn[4])) // gameTDB categories
			{
				bool all = m_btnMgr.selected(m_checkboxBtn[4]);
				bool accept = false;
				if(!all || !tdb_genres)
					if(_error(_t("errcfg11", L"GameTDB and/or plugin database are required. Missing categories will be automatically added. This cannot be undone."), true))
						accept = true;
				if(all && (tdb_genres || accept)) // gameTDB categories for all games
				{
					tdb_genres = !tdb_genres;
					m_cfg.setBool(general_domain, "tdb_genres", tdb_genres);
				}
				else if(accept) // gameTDB categories for this game
				{
					_setTDBCategories(CoverFlow.getHdr());
					break;	
				}
				_showCategoryConfig(true);
				if(all)
					m_btnMgr.setSelected(m_checkboxBtn[4]);
			}
			else if(m_btnMgr.selected(m_configBtnGo[5])) // add new category
			{
				_hideConfig();
				char *c = NULL;
				c = _keyboard();
				if(strlen(c) > 0)
				{
					string s = capitalize(lowerCase(c));
					if(_error(wfmt(_fmt("cfg824", L"This cannot be undone. Are you sure you want to add category: %s?"), s.c_str()), true))
					{
						m_cat.setString(general_domain, fmt("cat%d", m_max_categories), s);
						m_max_categories++;
						m_categories.resize(m_max_categories, '0');
						m_cat.setInt(general_domain, "numcategories", m_max_categories);
						curPage = ((m_max_categories - 2) / 10) + 1;
						break;
					}
				}
				_showCategoryConfig();
			}
			else if(m_btnMgr.selected(m_configBtnGo[6])) // rename category
			{
				_hideConfig(true);
				renameCat = true;
				_CategorySettings(true);
				renameCat = false;
				if(!exit_renameCat) // no category renamed
					_showCategoryConfig();
				else // a category has been renamed, return to category settings
				{
					exit_renameCat = false;
					break;
				}
			}
		}
	}
	
	/* Import TDB categories */
	if(prevGenres == false && tdb_genres == true) // if tdb_genres was false and now true
	{
		m_refreshGameList = true;
		m_vid.waitMessage(0.5f);
		for(vector<dir_discHdr>::iterator tmp_hdr = m_gameList.begin(); tmp_hdr != m_gameList.end(); ++tmp_hdr)
			_setTDBCategories(&(*(tmp_hdr)));
		_hideWaitMessage();
		m_categories.assign(m_max_categories, '0');
		const char *domains[] = {WII_DOMAIN, GC_DOMAIN, CHANNEL_DOMAIN, PLUGIN_DOMAIN};
		for(u8 i = 0; i < 4; i++)
			m_cfg.setBool(domains[i], "update_cache", true);
	}

	_hideConfig(true);
}

/*********************************************************************************************************/

/** Simple set of basic categories, don't modify order unless you modify _setTDBCategories() **/
static const char TDBCategories[20][16] =
{
	"Action",
	"Adventure",
	"Sports",
	"Platformer",
	"Puzzle",
	"Racing",
	"Fighting",
	"Role-playing",
	"Shooter",
	"Simulation",
	"Party",
	"Strategy",
	"Music",
	"Health",
	"Point-and-click",
	"Arcade",
	"NTSC-U",
	"NTSC-J",
	"PAL",
	"Hidden"
};

/** Assign categories from gameTDB and Wiimpathy's databases to games **/
void CMenu::_setTDBCategories(const dir_discHdr *hdr)
{
	GameTDB gametdb;
	char GameID[7];
	GameID[6] = '\0';

	if(hdr->type != TYPE_PLUGIN)
	{
		gametdb.OpenFile(fmt("%s/wiitdb.xml", m_wiiTDBDir.c_str()));
		strcpy(GameID, hdr->id);
	}
	else // type = plugin
	{	
		if(m_platform.loaded())
		{
			char platformName[16];
			strncpy(m_plugin.PluginMagicWord, fmt("%08x", hdr->settings[0]), 8);
			snprintf(platformName, sizeof(platformName), "%s", m_platform.getString("PLUGINS", m_plugin.PluginMagicWord).c_str());
			strcpy(GameID, hdr->id);
			if(!(strlen(platformName) == 0 || strcasecmp(GameID, "PLUGIN") == 0))
			{
				string newName = m_platform.getString("COMBINED", platformName);
				if(newName.empty())
					m_platform.remove("COMBINED", platformName);
				else
					snprintf(platformName, sizeof(platformName), "%s", newName.c_str());
				gametdb.OpenFile(fmt("%s/%s/%s.xml", m_pluginDataDir.c_str(), platformName, platformName));					
			}
		}
	}

	if(gametdb.IsLoaded())
	{
		const char *TMP_Char = NULL;
		vector<string> genres;
		
		if(gametdb.GetGenres(GameID, TMP_Char, hdr->type))
			genres = stringToVector(TMP_Char, ',');
		if(gametdb.GetRegion(GameID, TMP_Char)) // add region ("NTSC-U", "NTSC-J" or "PAL")
			genres.push_back(TMP_Char);
		
		gametdb.CloseFile();
		
		if(genres.size() > 0)
		{
			m_categories.resize(m_max_categories, '0');
			m_categories.assign(m_max_categories, '0');
			
			for(u8 i = 0; i < genres.size(); ++i)
			{
				//! genres in wiimpathy's base for plugins don't follow wii/gc standards, don't add new categories from it				
				bool addCat = (hdr->type != TYPE_PLUGIN);
				//! clean plugin base names a little
				if(hdr->type == TYPE_PLUGIN)
				{
					if(genres[i].find("Sport") != string::npos)
						genres[i] = TDBCategories[2]; // (Sports)
					else if(genres[i].find("Plat") != string::npos)
						genres[i] = TDBCategories[3]; // (Platformer)
					else if(genres[i].find("Puzzl") != string::npos)
						genres[i] = TDBCategories[4]; // (Puzzle)
					else if(genres[i].find("Rac") != string::npos)
						genres[i] = TDBCategories[5]; // (Racing)
					else if(genres[i].find("Fight") != string::npos)
						genres[i] = TDBCategories[6]; // (Fighting)
					else if(genres[i].find("Role") != string::npos)
						genres[i] = TDBCategories[7]; // (Role-playing)
					else if(genres[i].find("Shoot") != string::npos)
						genres[i] = TDBCategories[8]; // (Shooter)
				}				

				string tdbi = removeSpaceDash(lowerCase(genres[i]));
				for(int j = 1; j < m_max_categories; j++)
				{
					string catj = removeSpaceDash(lowerCase(m_cat.getString(general_domain, fmt("cat%d", j))));
					if(catj == tdbi)
					{
						m_categories.at(j) = '1';
						addCat = false; // category already exists, for whatever reason, don't add it
						break;
					}
				}
				if(addCat)
				{
					m_cat.setString(general_domain, fmt("cat%d", m_max_categories), capitalize(genres[i]));
					m_max_categories++;
					m_categories.resize(m_max_categories, '0');
					m_categories.at(m_max_categories - 1) = '1';
				}
			}
			m_cat.setInt(general_domain, "numcategories", m_max_categories);		
			_getGameCategories(hdr);
			_setGameCategories();
		}
	}
}

/** Initialize basic category set (called in CMenu::init() if categories_lite.ini is empty) **/
void CMenu::_initTDBCategories(void)
{
	for(u8 i = 0; i < ARRAY_SIZE(TDBCategories); i++)
		m_cat.setString(general_domain, fmt("cat%d", i + 1), TDBCategories[i]);
	m_max_categories = ARRAY_SIZE(TDBCategories) + 1;
	m_cat.setInt(general_domain, "numcategories", m_max_categories);
}
