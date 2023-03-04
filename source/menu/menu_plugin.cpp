
// #include <string.h>
// #include <gccore.h>
// #include <cmath>

#include "menu.hpp"

static u8 m_max_plugins = 0;
static u8 curPage = 1;
static u8 maxPage = 1;

void CMenu::_showPluginSettings(void)
{
	m_btnMgr.setText(m_configLblTitle, _t("cfgpl1", L"Select plugins"));
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_configBtnBack);
	
	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);
	
	_updatePluginCheckboxes();
}

void CMenu::_updatePluginText(void)
{
	u32 firstCheckbox = (curPage - 1) * 10;
	for(u8 i = 0; i < min(firstCheckbox + 10, (u32)m_max_plugins) - firstCheckbox; i++)
		m_btnMgr.setText(m_configLbl[i], m_plugin.GetPluginName(firstCheckbox + i));
}

void CMenu::_updatePluginCheckboxes(bool instant)
{
	if(m_max_plugins > 10)
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
	
	const vector<bool> &EnabledPlugins = m_plugin.GetEnabledPlugins(&enabledPluginsCount);

	/* Single Plugins */
	u32 firstCheckbox = (curPage - 1) * 10;
	for(u8 i = 0; i < min(firstCheckbox + 10, (u32)m_max_plugins) - firstCheckbox; ++i)
	{
		if(m_current_view == COVERFLOW_PLUGIN && (EnabledPlugins.size() == 0 || EnabledPlugins.at(firstCheckbox + i) == true))
			m_checkboxBtn[i] = m_configChkOn[i];
		else
			m_checkboxBtn[i] = m_configChkOff[i];
		m_btnMgr.show(m_checkboxBtn[i], instant);
		m_btnMgr.show(m_configLbl[i], instant);
	}

	if(enabledPluginsCount == 0)
		m_btnMgr.setText(m_configBtnCenter, _t("dl25", L"All"));
	else
		m_btnMgr.setText(m_configBtnCenter, _t("vmpnone", L"None"));
	m_btnMgr.show(m_configBtnCenter);
}

void CMenu::_PluginSettings()
{
	u8 i = 0;
	while(m_plugin.PluginExist(i)) i++;
	maxPage = static_cast<int>(ceil(static_cast<float>(i)/static_cast<float>(10)));
	m_max_plugins = i;
	// gprintf("Plugins found: %i, Pages: %i\n", m_max_plugins, maxPage);
	
	/* Only use Plugin Settings if plugins are found */
	if(maxPage == 0)
	{
		_error(_t("errplugin", L"No plugin files found!"));
		return;
	}
	
	curPage = 1;
	
	SetupInput();
	_showPluginSettings();
	_updatePluginText();
	
	while(!m_exit)
	{
		_mainLoopCommon();
		if(BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnBack)))
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
			_updatePluginCheckboxes();
			_updatePluginText();
		}
		else if(BTN_PLUS_PRESSED || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnPageP)))
		{
			curPage = curPage == maxPage ? 1 : curPage + 1;
			if(BTN_PLUS_PRESSED)
				m_btnMgr.click(m_configBtnPageP);
			_updatePluginCheckboxes();
			_updatePluginText();
		}
		if(BTN_A_OR_2_PRESSED)
		{
			u32 firstCheckbox = (curPage - 1) * 10;
			for(u8 i = 0; i < min(firstCheckbox + 10, (u32)m_max_plugins) - firstCheckbox; ++i) // (not 0)
			{
				if(m_btnMgr.selected(m_checkboxBtn[i]) || m_btnMgr.selected(m_configBtnCenter))
				{
					bool all = m_btnMgr.selected(m_configBtnCenter);
					m_refreshGameList = true;
					if(m_current_view != COVERFLOW_PLUGIN)
					{
						/* Clear all plugins */
						for(u8 j = 0; m_plugin.PluginExist(j); j++)
							m_plugin.SetEnablePlugin(j, 1);
						m_current_view = COVERFLOW_PLUGIN;
						enabledPluginsCount = 0;
					}
					if(all) // all button to clear all or set all (instead of i == 0)
					{
						/* If all clear then set(2) them else clear(1) them all */
						for(u8 j = 0; m_plugin.PluginExist(j); j++)
							m_plugin.SetEnablePlugin(j, (enabledPluginsCount == 0) ? 2 : 1);
					}
					else
						m_plugin.SetEnablePlugin(firstCheckbox + i); // switch plugin from off to on or vice versa
					_updatePluginCheckboxes(true); // instead of false
					if(!all)
						m_btnMgr.setSelected(m_checkboxBtn[i]);
					break;
				}
			}
		}
	}
	_hideConfig(true);
	
	int channels_type = 0;
	string enabledMagics;
	enabledPluginsCount = 0;
	for(u8 i = 0; m_plugin.PluginExist(i); i++)
	{
		if(m_plugin.GetEnabledStatus(i))
		{
			enabledPluginsCount++;
			string magic = sfmt("%08x", m_plugin.GetPluginMagic(i));
			if(enabledPluginsCount == 1)
				enabledMagics = magic;
			else
				enabledMagics.append(',' + magic);
			
			if(upperCase(magic) == ENAND_PMAGIC)
				channels_type |= CHANNELS_EMU;
			else if(upperCase(magic) == NAND_PMAGIC)
				channels_type |= CHANNELS_REAL;
		}
	}
	m_cfg.setString(plugin_domain, "enabled_plugins", enabledMagics);
	
	if(m_refreshGameList)
	{
		_srcTierBack(true);
		_getCustomBgTex();
		m_cfg.setUInt(general_domain, "sources", m_current_view);
		m_catStartPage = 1;
		if(channels_type > 0) //
			m_cfg.setInt(channel_domain, "channels_type", channels_type);
	}
	else
		m_current_view = m_cfg.getUInt(general_domain, "sources");
}
