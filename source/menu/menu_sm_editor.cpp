
// #include <string.h>
// #include <gccore.h>
// #include <cmath>

#include "menu.hpp"
// #include "gui/text.hpp"

static u8 curPage;
static u8 maxPage;
static u8 max_checkbox;
static u8 mode;
static u8 curSource;
static bool allow_tiers;

enum
{
	HIDE_SOURCES = 1,
	SELECT_BUTTON = 2,
	ASSIGN_PLUGIN = 3,
	EDIT_PLUGIN = 4,
};

void CMenu::_showCheckboxesMenu(void)
{
	if(mode == HIDE_SOURCES)
		m_btnMgr.setText(m_configLblTitle, _t("smedit1", L"Hide Sources"));
	else if(mode == SELECT_BUTTON)
		m_btnMgr.setText(m_configLblTitle, _t("smedit2", L"Choose Source"));
	else if (mode == ASSIGN_PLUGIN)
		m_btnMgr.setText(m_configLblTitle, _t("cfgpl1", L"Select Plugins"));
	else // mode == EDIT_PLUGIN
		m_btnMgr.setText(m_configLblTitle, _t("smedit3", L"Choose Plugin"));

	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_configBtnBack);
	m_btnMgr.setText(m_configBtnCenter, allow_tiers ? _t("smedit98", L"Disable tiers") : _t("smedit99", L"Enable tiers"));
	if(mode == HIDE_SOURCES)
		m_btnMgr.show(m_configBtnCenter);

	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);
		
	_updateCheckboxes();
}

void CMenu::_updateCheckboxesText(void)
{
	u32 firstCheckbox = (curPage - 1) * 10;
	for(u8 i = 0; i < min(firstCheckbox + 10, (u32)max_checkbox) - firstCheckbox; i++)
	{
		if(mode == HIDE_SOURCES || mode == SELECT_BUTTON)
		{
			string button = sfmt("button_%i", firstCheckbox + i);
			m_btnMgr.setText(m_configLbl[i], m_source.getWString(button, "title", button));
		}
		else // mode == ASSIGN_PLUGIN || mode == EDIT_PLUGIN
		{
			m_btnMgr.setText(m_configLbl[i], m_plugin.GetPluginName(firstCheckbox + i));
		}
	}
}

void CMenu::_updateCheckboxes(bool instant)
{
	if(max_checkbox > 10)
	{
		m_btnMgr.setText(m_configLblPage, wfmt(L"%i / %i", curPage, maxPage));
		m_btnMgr.show(m_configLblPage);
		m_btnMgr.show(m_configBtnPageM);
		m_btnMgr.show(m_configBtnPageP);
	}
	for(u8 i = 0; i < 10; ++i)
	{
		m_btnMgr.hide(m_checkboxBtn[i], true);
		m_btnMgr.hide(m_configLbl[i], true);
	}
	
	vector<string> magicNums;
	if(mode == ASSIGN_PLUGIN)
	{
		magicNums = m_source.getStrings(sfmt("button_%i", curSource), "magic", ',');
	}
	u32 firstCheckbox = (curPage - 1) * 10;
	for(u8 i = 0; i < min(firstCheckbox + 10, (u32)max_checkbox) - firstCheckbox; ++i)
	{
		string source = m_source.getString(sfmt("button_%i", firstCheckbox + i), "source", "");
		if(mode == HIDE_SOURCES)
		{
			if(allow_tiers && source == "new_source")
				m_checkboxBtn[i] = m_configBtnGo[i];
			else if(m_source.getBool(sfmt("button_%i", firstCheckbox + i), "hidden", false))
				m_checkboxBtn[i] = m_configChkOn[i];
			else
				m_checkboxBtn[i] = m_configChkOff[i];
		}
		else if(mode == SELECT_BUTTON)
		{
			if(source == "plugin" || source == "new_source")
				m_checkboxBtn[i] = m_configBtnGo[i];
			else
				m_checkboxBtn[i] = m_configChkHid[i];
		}
		else if(mode == ASSIGN_PLUGIN)
		{
			bool found = false;
			string pluginMagic = sfmt("%08x", m_plugin.GetPluginMagic(firstCheckbox + i));
			if(magicNums.size() > 0)
			{
				for(u8 j = 0; j < magicNums.size(); j++)
				{
					string tmp = lowerCase(magicNums[j]);
					if(tmp == pluginMagic)
					{
						found = true;
						break;
					}
				}
				if(found)
					m_checkboxBtn[i] = m_configChkOn[i];
				else
					m_checkboxBtn[i] = m_configChkOff[i];
			}
		}
		else // mode == EDIT_PLUGIN
			m_checkboxBtn[i] = m_configBtnGo[i];
		m_btnMgr.show(m_checkboxBtn[i], instant);
		m_btnMgr.show(m_configLbl[i], instant);
	}
}

void CMenu::_checkboxesMenu(u8 md)
{
	allow_tiers = false;
	mode = md;
	if(mode == HIDE_SOURCES || mode == SELECT_BUTTON)
		max_checkbox = m_max_source_btn + 1;
	else
	{
		max_checkbox = 0;
		while(m_plugin.PluginExist(max_checkbox)) max_checkbox++;
	}
	
	curPage = 1;
	maxPage = ((max_checkbox - 1) / 10) + 1;
	
	SetupInput();
	_showCheckboxesMenu();
	_updateCheckboxesText();
	while(!m_exit)
	{
		_mainLoopCommon();
		if(BTN_HOME_HELD || BTN_B_OR_1_PRESSED || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnBack)))
			break;
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
			m_btnMgr.up();
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
			m_btnMgr.down();
		if(BTN_MINUS_PRESSED || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnPageM)))
		{
			curPage = curPage == 1 ? maxPage : curPage - 1;
			if(BTN_MINUS_PRESSED)
				m_btnMgr.click(m_configBtnPageM);
			_updateCheckboxes();
			_updateCheckboxesText();
		}
		else if(BTN_PLUS_PRESSED || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnPageP)))
		{
			curPage = curPage == maxPage ? 1 : curPage + 1;
			if(BTN_PLUS_PRESSED)
				m_btnMgr.click(m_configBtnPageP);
			_updateCheckboxes();
			_updateCheckboxesText();
		}
		if(BTN_A_OR_2_PRESSED)
		{
			if(m_btnMgr.selected(m_configBtnCenter))
			{
				allow_tiers = !allow_tiers;
				_showCheckboxesMenu();
				_updateCheckboxes();
				continue;
			}
			u32 firstCheckbox = (curPage - 1) * 10;
			for(u8 i = 0; i <= min(firstCheckbox + 10, (u32)max_checkbox) - firstCheckbox; ++i)
			{
				if(m_btnMgr.selected(m_checkboxBtn[i]))
				{
					if(mode == HIDE_SOURCES)
					{
						string button = sfmt("button_%i", firstCheckbox + i);
						if((m_source.getString(button, "source", "") == "new_source") && allow_tiers)
						{
							if(_getNewSource(firstCheckbox + i))
							{
								_hideConfig(true);
								curSource = firstCheckbox + i;
								_checkboxesMenu(HIDE_SOURCES);
								_srcTierBack(false);
								max_checkbox = m_max_source_btn + 1;
								curPage = (curSource / 10) + 1;
								maxPage = ((max_checkbox - 1) / 10) + 1;
								_showCheckboxesMenu();
								_updateCheckboxes();
								_updateCheckboxesText();
								break;
							}
						}
						else
						{
							bool val = !m_source.getBool(button, "hidden", false);						
							m_source.setBool(button, "hidden", val);
							_updateCheckboxes(true);
							m_btnMgr.setSelected(m_checkboxBtn[i]);
							break;
						}
					}
					else if(mode == SELECT_BUTTON)
					{
						string source = m_source.getString(sfmt("button_%i", firstCheckbox + i), "source", "");
						if(source == "new_source")
						{
							if(_getNewSource(firstCheckbox + i))
							{
								_hideConfig(true);
								curSource = firstCheckbox + i;
								_checkboxesMenu(SELECT_BUTTON);
								_srcTierBack(false);
								max_checkbox = m_max_source_btn + 1;
								curPage = (curSource / 10) + 1;
								maxPage = ((max_checkbox - 1) / 10) + 1;
								_showCheckboxesMenu();
								_updateCheckboxes();
								_updateCheckboxesText();
								break;
							}
						}
						else if(source != "plugin")
						{
							_hideConfig();
							_error(_t("smediterr", L"Not allowed!"));
							_showCheckboxesMenu();
						}
						else
						{
							_hideConfig(true);
							curSource = firstCheckbox + i;
							_checkboxesMenu(ASSIGN_PLUGIN);
							mode = SELECT_BUTTON;
							max_checkbox = m_max_source_btn + 1;
							curPage = (curSource / 10) + 1;
							maxPage = ((max_checkbox - 1) / 10) + 1;
							_showCheckboxesMenu();
							_updateCheckboxes();
							_updateCheckboxesText();
							break;
						}
					}
					else if(mode == ASSIGN_PLUGIN)
					{
						bool found = false;
						u8 pluginsCount = 0;
						string newMagics;
						string pluginMagic = sfmt("%08x", m_plugin.GetPluginMagic(firstCheckbox + i));
						string button = sfmt("button_%i", curSource);
						vector<string> magicNums = m_source.getStrings(button, "magic", ',');
						if(magicNums.size() > 0)
						{
							for(u8 j = 0; j < magicNums.size(); j++)
							{
								string tmp = lowerCase(magicNums[j]);
								if(tmp == pluginMagic)
								{
									found = true; // and don't add it
								}
								else if(m_plugin.GetPluginPosition(strtoul(magicNums[j].c_str(), NULL, 16)) < 255) // make sure plugin exist
								{
									if(pluginsCount == 0)
										newMagics = magicNums[j];
									else
										newMagics.append(',' + magicNums[j]);
									pluginsCount++;
								}
							}
						}
						if(!found) // add it if not found
						{
							if(newMagics.empty())
								newMagics = pluginMagic;
							else
								newMagics.append(',' + pluginMagic);
						}
						if(!newMagics.empty()) // to make sure at least one plugin is selected
							m_source.setString(button, "magic", newMagics);
						_updateCheckboxes(true);
						m_btnMgr.setSelected(m_checkboxBtn[i]);
					}
					else // mode == EDIT_PLUGIN
					{
						_hideConfig(true);
						u8 pos = firstCheckbox + i;
						_configPluginEditor(pos); //
						_showCheckboxesMenu();
						_updateCheckboxesText();
					}
				}
			}
		}
	}
	if(mode < EDIT_PLUGIN)
		m_source.save();
	_hideConfig(true);
}

