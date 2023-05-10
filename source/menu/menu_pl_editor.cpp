
#include "menu.hpp"

/* Cover color */
const CMenu::SOption CMenu::_coverColor[6] = {
	{ "cc_black", L"Black" },
	{ "cc_white", L"White" },	
	{ "cc_clearcd", L"Clear CD" },
	{ "cc_red", L"Red" },
	{ "cc_yellow", L"Yellow" },
	{ "cc_green", L"Green" },
};

u8 coverColorIndex = 0;
u8 p = 255; // plugin position

void CMenu::_showConfigPluginEditor(bool instant)
{
	_hideCheckboxes(true); // reset checkboxes
	m_btnMgr.show(m_configBtnBack);
	
	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);
	
	wstringEx pluginName = m_plugin.GetPluginName(p);
	m_btnMgr.setText(m_configLblTitle, wfmt(_fmt("cfg844", L"Edit %s"), pluginName.toUTF8().c_str()));
	m_btnMgr.show(m_configLblTitle);
	//! Plugin partition
	s8 part = m_plugin.GetRomPartition(p);
	const char *partitionname = DeviceName[part];
	m_btnMgr.setText(m_configLbl[1], _t("cfg17", L"Game partition"));
	m_btnMgr.setText(m_configLblVal[1], part == -1 ? _t("def", L"Default") : upperCase(partitionname));
	//! Plugin rom path
	m_btnMgr.setText(m_configLbl[2], _t("cfg778", L"Game folder path"));
	//! Plugin explorer path
	m_btnMgr.setText(m_configLbl[3], _t("cfg826", L"Custom explorer path"));
	//! Plugin display name
	m_btnMgr.setText(m_configLbl[4], _t("cfg848", L"Plugin display name"));
	//! Plugin cover folder
	m_btnMgr.setText(m_configLbl[5], _t("cfg846", L"Cover folder name"));
	//! Plugin cover color
	m_btnMgr.setText(m_configLbl[6], _t("cfg845", L"Cover color"));
	switch(m_plugin.GetCaseColor(p))
	{
		case 0xFFFFFF: coverColorIndex = 1; break; // white
		case 0x111111: coverColorIndex = 2; break; // clear cd
		case 0xFF0000: coverColorIndex = 3; break; // red
		case 0xFCFF00: coverColorIndex = 4; break; // yellow
		case 0x01A300: coverColorIndex = 5; break; // green
		default: coverColorIndex = 0; // black
	}
	m_btnMgr.setText(m_configLblVal[6], _t(CMenu::_coverColor[coverColorIndex].id, CMenu::_coverColor[coverColorIndex].text));
	//! Use plugin database names
	m_btnMgr.setText(m_configLbl[7], _t("cfg847", L"Force file names as titles"));
	m_checkboxBtn[7] = m_plugin.GetFileNamesAsTitles(p) == false ? m_configChkOff[7] : m_configChkOn[7]; // default false

	u32 magic = m_plugin.GetPluginMagic(p);
	if(magic == WII_PMAGICN || magic == GC_PMAGICN) // explorer path only for wii and gc plugins
	{
		m_btnMgr.show(m_configLbl[4], instant);
		m_btnMgr.show(m_configBtnGo[4], instant);
	}
	else
	{
		for(u8 i = 1; i < 8; ++i)
		{
			m_btnMgr.show(m_configLbl[i], instant);
			if(i == 1 || i == 6)
			{
				m_btnMgr.show(m_configLblVal[i], instant);
				m_btnMgr.show(m_configBtnM[i], instant);
				m_btnMgr.show(m_configBtnP[i], instant);
			}
			else if(i < 6)
				m_btnMgr.show(m_configBtnGo[i], instant);
			else if(i == 7)
				m_btnMgr.show(m_checkboxBtn[i], instant);
		}
	}
}

void CMenu::_configPluginEditor(u8 pos)
{
	p = pos;
	
	u32 magic = m_plugin.GetPluginMagic(p);
	// no edition allowed if emunand, realnand, scummvm or homebrew plugin
	if(magic == ENAND_PMAGICN || magic == NAND_PMAGICN || magic == SCUMM_PMAGICN || magic == HB_PMAGICN)
	{
		_error(_t("smediterr", L"Not allowed!"));
		return;
	}

	Config m_plugin_cfg;
	m_plugin_cfg.load(m_plugin.GetPluginPath(p).c_str());
	if(!m_plugin_cfg.loaded())
		return;

	int cur_part = m_plugin.GetRomPartition(p);
	int default_rom_part = m_cfg.getInt(plugin_domain, "partition", 0);
	bool rebuildCache = false;
	string coverFolder(m_plugin.GetCoverFolderName(magic));
	u32 cur_caseColor = m_plugin.GetCaseColor(p);
	u32 prev_caseColor = cur_caseColor;
	bool cur_noDBTitles = m_plugin.GetFileNamesAsTitles(p);
	bool prev_noDBTitles = cur_noDBTitles;
	
	SetupInput();
	_showConfigPluginEditor();
	
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
			
			else if(m_btnMgr.selected(m_configBtnP[1]) || m_btnMgr.selected(m_configBtnM[1])) // partition
			{
				s8 direction = m_btnMgr.selected(m_configBtnP[1]) ? 1 : -1;
				if(cur_part == 8 && direction == 1)
					cur_part = -1;
				else if(cur_part == -1 && direction == -1)
					cur_part = 8;
				else
					cur_part = cur_part + direction;
				m_plugin.SetRomPartition(p, cur_part);
				m_plugin_cfg.setInt(PLUGIN, "rompartition", cur_part);
				if(m_current_view & COVERFLOW_PLUGIN)
					m_refreshGameList = true;
				_showConfigPluginEditor(true);
			}
			else if(m_btnMgr.selected(m_configBtnGo[2])) // rom path
			{
				_hideConfig(true);
				if(cur_part == -1)
					cur_part = default_rom_part;
				const char *currentPath = fmt("%s:/%s", DeviceName[cur_part], m_plugin.GetRomDir(p));
				const char *path = _FolderExplorer(currentPath);
				if(strlen(path) > 0)
				{
					cur_part = DeviceHandle.PathToDriveType(path);
					m_plugin_cfg.setInt(PLUGIN, "rompartition", cur_part);
					m_plugin.SetRomPartition(p, cur_part);
					string rd = sfmt("%s", strchr(path, '/') + 1);
					m_plugin_cfg.setString(PLUGIN, "romdir", rd);
					m_plugin.SetRomDir(p, rd);
					rebuildCache = true;
				}
				_showConfigPluginEditor();
			}
			else if(m_btnMgr.selected(m_configBtnGo[3])) // explorer path
			{
				_hideConfig(true);
				const char *currentPath = m_plugin.GetExplorerPath(magic);
				const char *path = _FolderExplorer(currentPath);
				if(strlen(path) > 0)
				{
					m_plugin_cfg.setString(PLUGIN, "explorerpath", path);
					m_plugin.SetExplorerPath(p, path);
				}
				_showConfigPluginEditor();
			}
			else if(m_btnMgr.selected(m_configBtnGo[4])) // plugin display name
			{
				_hideConfig(true);
				char *c = NULL;
				c = _keyboard();
				if(strlen(c) > 0)
				{
					string newDisplayName = capitalize(lowerCase(c));
					if(_error(wfmt(_fmt("errcfg15", L"New display name: %s. Are you sure?"), newDisplayName.c_str()), true))
					{
						m_plugin.SetPluginName(p, newDisplayName);
						m_plugin_cfg.setString(PLUGIN, "displayname", newDisplayName);
						_error(_t("dlmsg14", L"Done."));
					}
				}
				_showConfigPluginEditor();
			}
			else if(m_btnMgr.selected(m_configBtnGo[5])) // cover folder name
			{
				_hideConfig(true);
				char *c = NULL;
				c = _keyboard();
				if(strlen(c) > 0)
				{
					string newCoverFolder = lowerCase(c);
					if(_error(wfmt(_fmt("errcfg14", L"New cover folder name: %s. Are you sure?"), newCoverFolder.c_str()), true))
					{
						m_plugin.SetCoverFolderName(p, newCoverFolder);
						m_plugin_cfg.setString(PLUGIN, "coverfolder", newCoverFolder);
						_error(_t("dlmsg14", L"Done."));
					}
				}
				_showConfigPluginEditor();
			}
			else if(m_btnMgr.selected(m_configBtnP[6]) || m_btnMgr.selected(m_configBtnM[6])) // cover color
			{
				s8 direction = m_btnMgr.selected(m_configBtnP[6]) ? 1 : -1;
				switch(coverColorIndex)
				{
					case 1: cur_caseColor = direction == 1 ? 0x111111 : 0x000000; break; // white
					case 2: cur_caseColor = direction == 1 ? 0xFF0000 : 0xFFFFFF; break; // clear cd
					case 3: cur_caseColor = direction == 1 ? 0xFCFF00 : 0x111111; break; // red
					case 4: cur_caseColor = direction == 1 ? 0x01A300 : 0xFF0000; break; // yellow
					case 5: cur_caseColor = direction == 1 ? 0x000000 : 0xFCFF00; break; // green
					default: cur_caseColor = direction == 1 ? 0xFFFFFF : 0x01A300; // black
				}
				m_plugin.SetCaseColor(p, cur_caseColor);
				m_plugin_cfg.setString(PLUGIN, "covercolor", fmt("%06x", cur_caseColor));
				_showConfigPluginEditor(true);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[7])) // force no database titles
			{
				cur_noDBTitles = !cur_noDBTitles;
				m_plugin.SetFileNamesAsTitles(p, cur_noDBTitles);
				m_plugin_cfg.setBool(PLUGIN, "filenamesastitles", cur_noDBTitles);
				_showConfigPluginEditor(true);
				m_btnMgr.setSelected(m_checkboxBtn[7]);
			}
		}
	}

	if((cur_noDBTitles != prev_noDBTitles) || (cur_caseColor != prev_caseColor) || (strcmp(coverFolder.c_str(), m_plugin.GetCoverFolderName(magic)) != 0) || rebuildCache) // fileNamesAsTitles or covercolor or coverfolder or romdir have changed
	{
		strncpy(m_plugin.PluginMagicWord, fmt("%08x", magic), 8); //
		if(cur_part == -1)
			cur_part = default_rom_part;
		string cachedListFile(fmt("%s/%s_%s.db", m_listCacheDir.c_str(), DeviceName[cur_part], m_plugin.PluginMagicWord));
		fsop_deleteFile(cachedListFile.c_str());
		if(m_current_view & COVERFLOW_PLUGIN)
			m_refreshGameList = true;
	}
	
	m_plugin_cfg.save(true);
	_hideConfig(true);
}
