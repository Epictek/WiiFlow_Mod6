
#include "menu.hpp"

/* Source menu */
s16 m_sourceBtnSource[12];
s16 m_sourceLblUser[4];

string source;
char btn_selected[16];
static u8 i, j;
static u8 curPage;
static u8 maxPage;
int curflow = 1;
int channels_type;
vector<u8> nonHiddenSources;
vector<string> magicNums;

template <class T> static inline T loopNum(T i, T s)
{
	return (i + s) % s;
}

/** This is what happens when a sourceflow cover is clicked on **/
void CMenu::_sourceFlow()
{
	string numbers;
	string trs;
	const dir_discHdr *hdr = CoverFlow.getHdr();

	/* Save source number for return */
	sm_numbers[sm_numbers.size() - 1] = std::to_string(hdr->settings[0]);
	trs = tiers[0];
	numbers = sm_numbers[0];
	for(u8 i = 1; i < tiers.size(); i++)
	{
		trs.append(',' + tiers[i]);
		numbers.append(',' + sm_numbers[i]);
	}
	m_cfg.setString(sourceflow_domain, "tiers", trs);
	m_cfg.setString(sourceflow_domain, "numbers", numbers);
	
	memset(btn_selected, 0, 16);
	strncpy(btn_selected, fmt("BUTTON_%i", hdr->settings[0]), 15);
	source = m_source.getString(btn_selected, "source", "");
	
	if(source == "dml" || source == "gamecube")
		m_current_view = COVERFLOW_GAMECUBE;
	else if(source == "emunand")
	{
		m_cfg.setInt(channel_domain, "channels_type", CHANNELS_EMU);
		m_current_view = COVERFLOW_CHANNEL;
	}
	else if(source == "realnand")
	{
		m_cfg.setInt(channel_domain, "channels_type", CHANNELS_REAL);
		m_current_view = COVERFLOW_CHANNEL;
	}
	else if(source == "bothnand")
	{
		m_cfg.setInt(channel_domain, "channels_type", CHANNELS_BOTH);
		m_current_view = COVERFLOW_CHANNEL;
	}
	else if(source == "homebrew")
	{
		m_current_view = COVERFLOW_HOMEBREW;
	}
	else if(source == "allplugins")
	{
		_getAllPlugins();
	}
	else if((source == "plugin") || (source == "explorer"))
	{
		_getMagicNums();
		if(magicNums.size() > 0 && source == "explorer" && !m_locked) // explorer view
		{
			m_cfg.setUInt(general_domain, "sources", m_current_view);
			u32 plmagic = strtoul(m_source.getString(btn_selected, "magic", "").c_str(), NULL, 16);
			const char *plpath = m_plugin.GetExplorerPath(plmagic);
			_pluginExplorer(plpath, plmagic, true);
			return; // don't close sourceflow
		}
	}
	else if(source == "new_source")
	{
		string fn = m_source.getString(btn_selected, "magic", "");
		if(_getNewSource(hdr->settings[0]))
		{
			fn.replace(fn.find("."), 4, "_flow");
			if(m_source.has(general_domain, "flow"))
				curflow = m_source.getInt(general_domain, "flow", 1);
			else
				curflow = m_cfg.getInt(sourceflow_domain, fn, m_cfg.getInt(sourceflow_domain, "last_cf_mode", 1));
			m_clearCats = false;
			return;
		}
	}
	else if(source == "back_tier")
	{
		_srcTierBack(false);
		m_clearCats = false;
		return;
	}
	else //(source == "wii")
		m_current_view = COVERFLOW_WII;
	m_sourceflow = false;
	m_cfg.setUInt(general_domain, "sources", m_current_view);
	_setSrcOptions();
}

/** Get sourceflow cover layout number **/
int CMenu::_getSrcFlow(void)
{
	return curflow;
}

/** Set sourceflow cover layout to version number and set it in wiiflow_lite.ini **/
void CMenu::_setSrcFlow(int version)
{
	curflow = version;
	string fn = tiers[tiers.size() - 1];
	fn.replace(fn.find("."), 4, "_flow");
	m_cfg.setInt(sourceflow_domain, fn, curflow);
	if(!sm_tier)
		m_cfg.setInt(sourceflow_domain, "last_cf_mode", curflow);
}

/** Return back to previous tier or home base tier **/
bool CMenu::_srcTierBack(bool home)
{
	if(!sm_tier)
		return false;

	string fn;
	if(home)
	{
		fn = tiers[0];
		tiers.erase(tiers.begin() + 1, tiers.end());
		sm_numbers.erase(sm_numbers.begin() + 1, sm_numbers.end());
		sm_numbers[0] = "0";
	}
	else
	{
		fn = tiers[tiers.size() - 2];
		tiers.pop_back();
		sm_numbers.pop_back();
	}
	
	if(fn == SOURCE_FILENAME)
		sm_tier = false;
	else
		sm_tier = true;
	m_source.unload();
	m_source.load(fmt("%s/%s", m_sourceDir.c_str(), fn.c_str()));	
	fn.replace(fn.find("."), 4, "_flow");
	if(m_source.has(general_domain, "flow"))
		curflow = m_source.getInt(general_domain, "flow", 1);
	else
		curflow = m_cfg.getInt(sourceflow_domain, fn, m_cfg.getInt(sourceflow_domain, "last_cf_mode", 1));	
	_getMaxSourceButton();
	if(!m_sourceflow)
		_getSourcePage();
	return true;
}

/** Get custom sourceflow background image if available **/
void CMenu::_getSFlowBgTex(void)
{
	curCustBg = loopNum(curCustBg + 1, 2);
	string fn = m_source.getString(general_domain, "background", "");
	if(fn.length() > 0)
	{
		/* Backgrounds for sourceflow view */
		if(m_sourceflow)
		{
			if(TexHandle.fromImageFile(m_mainCustomBg[curCustBg], fmt("%s/backgrounds/%s/sourceflow/%s", m_sourceDir.c_str(), m_themeName.c_str(), fn.c_str())) != TE_OK)
			{
				if(TexHandle.fromImageFile(m_mainCustomBg[curCustBg], fmt("%s/backgrounds/sourceflow/%s", m_sourceDir.c_str(), fn.c_str())) != TE_OK)
				{
					if(TexHandle.fromImageFile(m_mainCustomBg[curCustBg], fmt("%s/backgrounds/%s/%s", m_sourceDir.c_str(), m_themeName.c_str(), fn.c_str())) != TE_OK)
					{
						if(TexHandle.fromImageFile(m_mainCustomBg[curCustBg], fmt("%s/backgrounds/%s", m_sourceDir.c_str(), fn.c_str())) != TE_OK)
						{
							curCustBg = loopNum(curCustBg + 1, 2); // reset it
							customBg = false;
							return;
						}
					}
				}
			}
			customBg = true;
			return;
		}
		/* Backgrounds for classic source menu */
		else
		{
			if(TexHandle.fromImageFile(m_mainCustomBg[curCustBg], fmt("%s/backgrounds/%s/%s", m_sourceDir.c_str(), m_themeName.c_str(), fn.c_str())) != TE_OK)
			{
				if(TexHandle.fromImageFile(m_mainCustomBg[curCustBg], fmt("%s/backgrounds/%s", m_sourceDir.c_str(), fn.c_str())) != TE_OK)
				{
					curCustBg = loopNum(curCustBg + 1, 2); // reset it
					customBg = false;
					return;
				}
			}
			customBg = true;
		}
	}	
	else
	{
		curCustBg = loopNum(curCustBg + 1, 2); // reset it
		customBg = false;
	}
}

/********************************************************************************************************/
/** End of sourceflow stuff - Start of classic source menu stuff **/
/********************************************************************************************************/

void CMenu::_hideSource(bool instant)
{
	m_btnMgr.hide(m_configLblTitle, instant);
	m_btnMgr.hide(m_configLblPage, instant);
	m_btnMgr.hide(m_configBtnPageM, instant);
	m_btnMgr.hide(m_configBtnPageP, instant);
	m_btnMgr.hide(m_configBtnBack, instant);

	for(i = 0; i < ARRAY_SIZE(m_sourceLblUser); ++i)
		if(m_sourceLblUser[i] != -1)
			m_btnMgr.hide(m_sourceLblUser[i], instant);

	for(i = 0; i < 12; ++i)
	{
		m_btnMgr.hide(m_sourceBtnSource[i], instant);
		m_btnMgr.freeBtnTexture(m_sourceBtnSource[i]);
	}
}

void CMenu::_showSource(void)
{
	for(i = 0; i < ARRAY_SIZE(m_sourceLblUser); ++i)
		if(m_sourceLblUser[i] != -1)
			m_btnMgr.show(m_sourceLblUser[i]);

	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_configBtnBack);
}

void CMenu::_getSourcePage(bool home)
{
	nonHiddenSources.clear();
	for(i = 0; i <= m_max_source_btn; i++)
	{
		if(!m_source.getBool(sfmt("BUTTON_%i", i), "hidden", false))
			nonHiddenSources.push_back(i);
	}
	curPage = 1;
	if(!home)
	{
		u8 num = stoi(sm_numbers[sm_numbers.size() - 1]);
		for(i = 0; i < nonHiddenSources.size(); i++)
		{
			if(nonHiddenSources[i] == num)
			{
				curPage = i / 12 + 1;
				break;
			}
		}
	}
	maxPage = ((nonHiddenSources.size() - 1) / 12) + 1;	
}

void CMenu::_updateSourceBtns(void)
{
	char current_btn[16];
	
	for(i = 0; i < 12; ++i)
		m_btnMgr.hide(m_sourceBtnSource[i], true);

	_getSFlowBgTex();
	_setMainBg();
	
	/* Get page title from source_menu.ini */
	m_btnMgr.hide(m_configLblTitle, true);
	string page_title = sfmt("pagetitle_%i", curPage);
	m_btnMgr.setText(m_configLblTitle, m_source.getWString(general_domain, page_title, _t("cfg774", L"Select source")));
	m_btnMgr.show(m_configLblTitle);
	
	if(maxPage > 1)
	{
		m_btnMgr.setText(m_configLblPage, wfmt(L"%i / %i", curPage, maxPage));
		m_btnMgr.show(m_configLblPage);
		m_btnMgr.show(m_configBtnPageM);
		m_btnMgr.show(m_configBtnPageP);
	}
	else
	{
		m_btnMgr.hide(m_configLblPage);
		m_btnMgr.hide(m_configBtnPageM);
		m_btnMgr.hide(m_configBtnPageP);
	}

	j = (curPage - 1) * 12;
	for(i = 0; i < 12; ++i)
	{
		if((i + j) >= nonHiddenSources.size())
			m_btnMgr.hide(m_sourceBtnSource[i]);
		else
		{
			memset(current_btn, 0, 16);
			strncpy(current_btn, fmt("BUTTON_%i", nonHiddenSources[i + j]), 15);
			string btnSource = m_source.getString(current_btn, "source", "");
			if(btnSource == "")
			{
				m_btnMgr.hide(m_sourceBtnSource[i]); // hide buttons with no source
				continue;
			}
			char btn_image[255];
			snprintf(btn_image, sizeof(btn_image), "%s", m_source.getString(current_btn,"image", "").c_str());			
			char btn_image_s[255];
			snprintf(btn_image_s, sizeof(btn_image_s), "%s", m_source.getString(current_btn,"image_s", "").c_str());
			
			TexData texConsoleImg;
			TexData texConsoleImgs;

			if(TexHandle.fromImageFile(texConsoleImg, fmt("%s/%s/%s", m_sourceDir.c_str(), m_themeName.c_str(), btn_image)) != TE_OK)
			{
				if(TexHandle.fromImageFile(texConsoleImg, fmt("%s/%s", m_sourceDir.c_str(), btn_image)) != TE_OK)
					continue;
			}
			if(TexHandle.fromImageFile(texConsoleImgs, fmt("%s/%s/%s", m_sourceDir.c_str(), m_themeName.c_str(), btn_image_s)) != TE_OK) // 
			{
				if(TexHandle.fromImageFile(texConsoleImgs, fmt("%s/%s", m_sourceDir.c_str(), btn_image_s)) != TE_OK)
					continue;
			}
				m_btnMgr.setBtnTexture(m_sourceBtnSource[i], texConsoleImg, texConsoleImgs);
			m_btnMgr.show(m_sourceBtnSource[i]);
		}
	}
}

bool CMenu::_Source(bool home)
{
	bool newSource = false;
	bool updateSource = false;
	bool exitSource = false;

	_getSourcePage(home);
	channels_type = m_cfg.getInt(channel_domain, "channels_type", CHANNELS_REAL);
	SetupInput();
	_showSource();
	_updateSourceBtns();

	while(!m_exit)
	{
		updateSource = false;
		_mainLoopCommon();

		if((BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnBack)) || BTN_HOME_PRESSED)
			break;
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
			m_btnMgr.up();
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
			m_btnMgr.down();
		else if((BTN_MINUS_PRESSED && maxPage > 1) || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnPageM)))
		{
			curPage = curPage == 1 ? maxPage : curPage - 1;
			if(BTN_MINUS_PRESSED)
				m_btnMgr.click(m_configBtnPageM);
			_updateSourceBtns();
		}
		else if((BTN_PLUS_PRESSED && maxPage > 1) || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnPageP)))
		{
			curPage = curPage == maxPage ? 1 : curPage + 1;
			if(BTN_PLUS_PRESSED)
				m_btnMgr.click(m_configBtnPageP);
			_updateSourceBtns();
		}
		else if(BTN_B_OR_1_PRESSED)
		{
			if(!sm_tier)
				break;
			else
			{
				_srcTierBack(false); // force false (back one tier)
				updateSource = true;
			}
		}
		else if(BTN_A_OR_2_PRESSED)
		{
			j = (curPage - 1) * 12;
			for(i = 0; i < 12; ++i)
			{
				if(m_btnMgr.selected(m_sourceBtnSource[i]))
				{
					memset(btn_selected, 0, 16);
					strncpy(btn_selected, fmt("BUTTON_%i", nonHiddenSources[i + j]), 15);
					source = m_source.getString(btn_selected, "source", "");
					break;
				}
			}
			if(i < 12)
			{
				sm_numbers[tiers.size() - 1] = std::to_string(i + j);
				
				string trs = tiers[0];
				string numbers = sm_numbers[0];
				for(u8 i = 1; i < tiers.size(); i++)
					trs.append(',' + tiers[i]);
				for(u8 i = 1; i < sm_numbers.size(); i++)
					numbers.append(',' + sm_numbers[i]);

				m_cfg.setString(sourceflow_domain, "tiers", trs);
				m_cfg.setString(sourceflow_domain, "numbers", numbers);

				exitSource = true;
				m_catStartPage = 1;
				if(source == "dml" || source == "gamecube")
				{
					m_current_view = COVERFLOW_GAMECUBE;
					_setSrcOptions();
				}
				else if(source == "emunand" || source == "realnand" || source == "bothnand")
				{
					if(source == "emunand")
						channels_type = CHANNELS_EMU;
					else if(source == "realnand")
						channels_type = CHANNELS_REAL;
					else if(source == "bothnand")
						channels_type = CHANNELS_BOTH;
					m_current_view = COVERFLOW_CHANNEL;
					_setSrcOptions();

					//! check for specific emunand folder
					if(channels_type >= CHANNELS_EMU)
					{
						string emunand = m_source.getString(btn_selected, "nandfolder", "");
						if(emunand != "")
							m_cfg.setString(channel_domain, "current_emunand", emunand);
					}
				}
				else if(source == "homebrew")
				{
					m_current_view = COVERFLOW_HOMEBREW;
					_setSrcOptions();
				}
				else if(source == "allplugins")
				{
					_getAllPlugins();
					_setSrcOptions();
				}
				else if((source == "plugin") || (source == "explorer"))
				{
					_getMagicNums();
					_setSrcOptions();
					if(magicNums.size() > 0 && source == "explorer" && !m_locked) // explorer view
					{
						m_cat.remove(general_domain, "selected_categories");
						m_cat.remove(general_domain, "required_categories");
						m_cfg.setUInt(general_domain, "sources", m_current_view);
						_hideSource(true);
						exitSource = false;
						u32 plmagic = strtoul(m_source.getString(btn_selected, "magic", "").c_str(), NULL, 16);
						const char *plpath = m_plugin.GetExplorerPath(plmagic);
						_pluginExplorer(plpath, plmagic, true);
						_showSource();
						newSource = true;
						updateSource = true;
					}						
				}
				else if(source == "new_source")
				{
					if(_getNewSource(nonHiddenSources[i + j]))
					{
						exitSource = false;
						updateSource = true;
						_getSourcePage(true);
					}
				}
				else if(source == "back_tier")
				{
					exitSource = false;
					_srcTierBack(false);
					updateSource = true;
				}				
				else // if(source == "wii") or source is invalid or empty default to wii
				{
					m_current_view = COVERFLOW_WII;
					_setSrcOptions();
				}
			}
		}
		if(exitSource)
		{
			m_cfg.setUInt(general_domain, "sources", m_current_view);
			newSource = true;
			break;
		}
		if(updateSource)
			_updateSourceBtns();
	}
	m_cfg.setInt(channel_domain, "channels_type", channels_type);
	_hideSource(true);
	
	return newSource;
}

/** _getMagicNums() looks for one or several plugin magic numbers associated with a source button,
 enables all of them and updates "enabled_plugins" in wiiflow_lite.ini **/
void CMenu::_getMagicNums(void) // added
{
	magicNums.clear();
	magicNums = m_source.getStrings(btn_selected, "magic", ',');
	if(magicNums.size() > 0)
	{
		for(u8 pos = 0; m_plugin.PluginExist(pos); pos++)
			m_plugin.SetEnablePlugin(pos, 1); // force disable all
		enabledPluginsCount = 0;
		string enabledMagics;
		for(i = 0; i < magicNums.size(); i++)
		{
			u8 pos = m_plugin.GetPluginPosition(strtoul(magicNums[i].c_str(), NULL, 16));
			if(pos < 255) // if pos == 255 then it doesn't exist
			{
				enabledPluginsCount++;
				m_plugin.SetEnablePlugin(pos, 2);
				if(enabledPluginsCount == 1)
					enabledMagics = magicNums[i];
				else
					enabledMagics.append(',' + magicNums[i]);
			}
		}
		m_cfg.setString(plugin_domain, "enabled_plugins", enabledMagics);
		m_current_view = COVERFLOW_PLUGIN;
	}
}

/** _getAllPlugins() enables all plugins and updates "enabled_plugins" in wiiflow_lite.ini **/
void CMenu::_getAllPlugins(void) // added
{
	enabledPluginsCount = 0;
	string enabledMagics;
	for(u8 pos = 0; m_plugin.PluginExist(pos); pos++)
	{
		enabledPluginsCount++;
		m_plugin.SetEnablePlugin(pos, 2); // force enable
		string pluginMagic = sfmt("%08x", m_plugin.GetPluginMagic(pos));
		if(enabledPluginsCount == 1)
			enabledMagics = pluginMagic;
		else
			enabledMagics.append(',' + pluginMagic);
	}
	m_cfg.setString(plugin_domain, "enabled_plugins", enabledMagics);
	m_current_view = COVERFLOW_PLUGIN;
}

/** _getMaxSourceButton() counts buttons in INI source file and updates m_max_source_btn **/
void CMenu::_getMaxSourceButton(void)
{
	m_max_source_btn = 0;
	const char *srcDomain = m_source.firstDomain().c_str();
	while(1)
	{
		if(strlen(srcDomain) < 2)
			break;
		if(strrchr(srcDomain, '_') != NULL)
		{
			int srcBtnNumber = atoi(strrchr(srcDomain, '_') + 1);
			if(srcBtnNumber > m_max_source_btn)
				m_max_source_btn = srcBtnNumber;
		}
		srcDomain = m_source.nextDomain().c_str();
	}
}

/** _getNewSource() is used by _sourceFlow(), _Source() and _checkboxesMenu()
it checks the new INI source file, loads it and updates m_max_source_btn **/
bool CMenu::_getNewSource(u8 btn)
{
	string fn = m_source.getString(sfmt("button_%i", btn), "magic", "");
	if(fsop_FileExist(fmt("%s/%s", m_sourceDir.c_str(), fn.c_str())))
	{
		if(fn == SOURCE_FILENAME) // the new source is "source_menu.ini"
		{
			sm_tier = false;
			tiers.erase(tiers.begin() + 1, tiers.end());
			sm_numbers.erase(sm_numbers.begin() + 1, sm_numbers.end());
		}
		else
		{
			sm_tier = true;
			tiers.push_back(fn);
			sm_numbers.push_back("0");
		}
		m_source.unload();
		m_source.load(fmt("%s/%s", m_sourceDir.c_str(), fn.c_str()));
		_getMaxSourceButton();
		return true;
	}
	return false;
}

void CMenu::_setSrcOptions(void)
{
	m_thumbnail = m_source.getBool(btn_selected, "snapshots", false);
	m_catStartPage = min(max(1, m_source.getInt(btn_selected, "cat_page", 1)), 255);
	u8 category = min(max(0, m_source.getInt(btn_selected, "category", 0)), 255);
	if(category > 0 && category < m_max_categories)
	{
		m_cat.remove(general_domain, "selected_categories");
		m_cat.remove(general_domain, "required_categories");
		char cCh = static_cast<char>(category + 32);
		string newSelCats(1, cCh);
		m_cat.setString(general_domain, "required_categories", newSelCats); // replaced selected_categories
		m_clearCats = false;
	}
	
	/* Autoboot */
	char autoboot[64];
	autoboot[63] = '\0';
	strncpy(autoboot, m_source.getString(btn_selected, "autoboot", "").c_str(), sizeof(autoboot) - 1);
	if(autoboot[0] != '\0')
	{
		m_source_autoboot = true;
		memset(&m_autoboot_hdr, 0, sizeof(dir_discHdr));
		if(source == "emunand" || source == "realnand")
		{
			m_autoboot_hdr.type = TYPE_CHANNEL;
			memcpy(m_autoboot_hdr.id, autoboot, 4);
		}
		else if(source == "wii")
		{
			m_autoboot_hdr.type = TYPE_WII_GAME;
			memcpy(m_autoboot_hdr.id, autoboot, 6);
		}
		else if(source == "dml" || source == "gamecube")
		{
			m_autoboot_hdr.type = TYPE_GC_GAME;
			memcpy(m_autoboot_hdr.id, autoboot, 6);
		}
		else if(source == "homebrew")
		{
			m_autoboot_hdr.type = TYPE_HOMEBREW;
			mbstowcs(m_autoboot_hdr.title, autoboot, 63);
		}
		else if(source == "plugin")
		{
			m_autoboot_hdr.type = TYPE_PLUGIN;
			mbstowcs(m_autoboot_hdr.title, autoboot, 63);
		}
		else
			m_source_autoboot = false;
	}
}

void CMenu::_initSourceMenu()
{
	m_use_source = false;
	//! look for source_menu/theme/source_menu.ini
	if(!fsop_FileExist(fmt("%s/%s/%s", m_sourceDir.c_str(), m_themeName.c_str(), SOURCE_FILENAME)))
	{
		//! look for source_menu/source_menu.ini
		if(!fsop_FileExist(fmt("%s/%s", m_sourceDir.c_str(), SOURCE_FILENAME)))
		return; // no source_menu.ini so we don't init nor use source menu, just return
	}
	//! if source_menu/theme/source_menu.ini found then change m_sourceDir to source_menu/theme/
	else
		m_sourceDir = fmt("%s/%s", m_sourceDir.c_str(), m_themeName.c_str());
	
	//! let wiiflow know source_menu.ini found and we will be using it
	m_use_source = true;

	sm_numbers.clear();
	tiers.clear();
	sm_numbers = m_cfg.getStrings(sourceflow_domain, "numbers");
	tiers = m_cfg.getStrings(sourceflow_domain, "tiers");
	if(tiers.size() == 0)
	{
		tiers.push_back(SOURCE_FILENAME);
		sm_numbers.push_back("0");
	}
	sm_tier = false;
	if(tiers.size() > 1)
		sm_tier = true;
	string fn = tiers[tiers.size() - 1];
	m_source.load(fmt("%s/%s", m_sourceDir.c_str(), fn.c_str()));
	fn.replace(fn.find("."), 4, "_flow");
	if(m_source.has(general_domain, "flow"))
		curflow = m_source.getInt(general_domain, "flow", 1);
	else
		curflow = m_cfg.getInt(sourceflow_domain, fn, m_cfg.getInt(sourceflow_domain, "last_cf_mode", 1));
	
	_getMaxSourceButton();

	_addUserLabels(m_sourceLblUser, ARRAY_SIZE(m_sourceLblUser), "SOURCE");

	TexData texConsoleImg;
	TexData texConsoleImgs;

	//! use basic button texture just to initialize the buttons
	int row;
	int col;	
	for(i = 0; i < 12; ++i)
	{
		char *srcBtnText = fmt_malloc("SOURCE/SOURCE_BTN_%i", i);
		if(srcBtnText == NULL) 
			continue;
		row = i / 4;
		col = i - (row * 4);
		m_sourceBtnSource[i] = _addPicButton(srcBtnText, texConsoleImg, texConsoleImgs, (35 + 150 * col), (90 + 100 * row), 120, 90);
		
		_setHideAnim(m_sourceBtnSource[i], srcBtnText, 1, 1, 0, 0);
		MEM2_free(srcBtnText);
	}
	// _hideSource(true);
}
