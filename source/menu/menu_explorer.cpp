/****************************************************************************
 * Copyright (C) 2013 FIX94 - (modified by iamerror80)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#include <algorithm>
// #include <dirent.h>
// #include <unistd.h>

#include "menu.hpp"
#include "lockMutex.hpp"
// #include "channel/nand.hpp"

#define REPEATDELAY 3 // used to slow down file highlighting when up or down is held

s16 entries[15];
s16 entries_sel[15];
s16 m_explorerLblSelFolderBg;
s16 m_explorerLblSelFolder;
s16 m_explorerLblUser[4];
u32 dirs = 0;
u32 files = 0;

typedef struct {
	char name[NAME_MAX];
} list_element;
list_element *elements = NULL;

static s8 explorer_partition = 0;
static u16 curPage = 1;
static u32 magicNum = 0; // for plugin explorer
static u32 start_pos = 0;
static u32 elements_num = 0;
char dir[MAX_FAT_PATH];
char folderPath[MAX_FAT_PATH];
char startDir[MAX_FAT_PATH];
char romDirPath[MAX_FAT_PATH];
static bool folderExplorer = false;
static bool wadsOnly = false;
static bool pluginExplorer = false;
static bool fromSource = false;
static bool importRom = false;
static bool showImport = false;

void CMenu::_hideExplorer(bool instant)
{
	for(u8 i = 0; i < 15; ++i)
	{
		m_btnMgr.hide(entries[i], instant);
		m_btnMgr.hide(entries_sel[i], instant);
	}
	m_btnMgr.hide(m_explorerLblSelFolderBg, instant);
	m_btnMgr.hide(m_explorerLblSelFolder, instant);
	m_btnMgr.hide(m_configLblTitle, instant);
	m_btnMgr.hide(m_configLblPage, instant);
	m_btnMgr.hide(m_configBtnPageM, instant);
	m_btnMgr.hide(m_configBtnPageP, instant);
	m_btnMgr.hide(m_configBtnCenter, instant); // set or import
	m_btnMgr.hide(m_configBtnBack, instant);
	for(u8 i = 0; i < ARRAY_SIZE(m_explorerLblUser); ++i)
	{
		if(m_explorerLblUser[i] != -1)
			m_btnMgr.hide(m_explorerLblUser[i], instant);
	}
	if(elements != NULL)
		MEM2_free(elements);
	elements = NULL;
	elements_num = 0;
}

void CMenu::_showExplorer(void)
{
	m_btnMgr.show(m_explorerLblSelFolderBg);
	m_btnMgr.show(m_explorerLblSelFolder);
	m_btnMgr.show(m_configBtnBack);
	if(folderExplorer)
	{
		m_btnMgr.setText(m_configBtnCenter, _t("set", L"Set"));
		m_btnMgr.show(m_configBtnCenter);
	}
	for(u8 i = 0; i < ARRAY_SIZE(m_explorerLblUser); ++i)
	{
		if(m_explorerLblUser[i] != -1)
			m_btnMgr.show(m_explorerLblUser[i]);
	}
	_refreshExplorer();
}

void CMenu::_Explorer(void)
{
	u8 repeatDelay = 0;
	
	if(!pluginExplorer)
		explorer_partition = currentPartition;
	
	m_thrdStop = false;
	m_thrdMessageAdded = false;
	
	_setBg(m_configBg, m_configBg);
	
	if(!folderExplorer)
		CoverFlow.clear();
	strcpy(folderPath, dir); // dir is "explorer_path" from ini file if m_explorer_on_start is true
	
	_showExplorer();

	startDir[MAX_FAT_PATH - 1] = '\0';
	//! check current start rom directory in ini file if previous game was launched in explorer view mode
	if(m_explorer_on_start)
	{
		curPage = m_cfg.getInt(general_domain, "explorer_page", 1);
		strcpy(startDir, m_cfg.getString(general_domain, "explorer_root", "sd:/").c_str());
		_refreshExplorer(2); // "2" means refresh using explorer_page in config ini file
	}
	//! remember start directory if plugin explorer
	else
		strcpy(startDir, dir);
	while(!m_exit)
	{
		_mainLoopCommon();
		/* If dir is root or start dir for plugin explorer, close explorer */
		if((BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED) && (dir[0] == '\0' || (pluginExplorer && strcmp(dir, startDir) == 0)))
		{
			memset(folderPath, 0, MAX_FAT_PATH);
			break;
		}
		else if(BTN_HOME_PRESSED)
		{
			if(pluginExplorer) // go to rom dir
			{
				memset(dir, 0, MAX_FAT_PATH);
				strcpy(dir, startDir);
				stpcpy(folderPath, dir);
				_refreshExplorer();
			}
			else // go to root
			{
				memset(dir, 0, MAX_FAT_PATH);
				memset(folderPath, 0, MAX_FAT_PATH);
				_refreshExplorer();
			}
		}
		else if(BTN_PLUS_PRESSED) 
		{
			_refreshExplorer(1);
			m_btnMgr.click(m_configBtnPageP);
			m_btnMgr.down();
		}
		else if(BTN_MINUS_PRESSED)
		{
			_refreshExplorer(-1);
			m_btnMgr.click(m_configBtnPageM);
			m_btnMgr.down();
		}
		else if(BTN_LEFT_REV_REPEAT || BTN_UP_REPEAT)
		{
			if(repeatDelay == REPEATDELAY)
			{
				repeatDelay = 0;
				m_btnMgr.up();
			}
			else
				repeatDelay++;
		}
		else if(BTN_RIGHT_REV_REPEAT || BTN_DOWN_REPEAT)
		{
			if(repeatDelay == REPEATDELAY)
			{
				repeatDelay = 0;
				m_btnMgr.down();
			}
			else
				repeatDelay++;
		}
		/* If "..." is selected and path is not empty then go up (back) one folder */
		else if(((BTN_A_OR_2_PRESSED && m_btnMgr.selected(entries_sel[0])) || BTN_B_OR_1_PRESSED) && dir[0] != '\0')
		{
			/* If pluginExplorer last parent dir is startDir */
			if(pluginExplorer && strcmp(dir, startDir) == 0)
			{
				memset(folderPath, 0, MAX_FAT_PATH);
				break;				
			}
			else
			{
				/* Remove last folder or device+partition */
				if(strchr(dir, '/') != NULL)
				{
					*strrchr(dir, '/') = '\0';
					if(strchr(dir, '/') != NULL)
						*(strrchr(dir, '/')+1) = '\0';
				}
				strcpy(folderPath, dir);
				/* If dir is just device and : then clear path completely */
				if(strchr(dir, '/') == NULL)
				{
					memset(dir, 0, MAX_FAT_PATH);
					memset(folderPath, 0, MAX_FAT_PATH);
				}
				else if(strchr(folderPath, '/') != strrchr(folderPath, '/'))
				{
					char tmpPath[MAX_FAT_PATH];
					*strrchr(folderPath, '/') = '\0';
					while(strlen(folderPath) > 44) //
					{
						if(strchr(folderPath, '/') == strrchr(folderPath, '/'))
							break;
						memset(tmpPath, 0, MAX_FAT_PATH);
						strncpy(tmpPath, strchr(folderPath, '/') + 1, MAX_FAT_PATH - 1);
						strcpy(folderPath, tmpPath);
					}
					memset(tmpPath, 0, MAX_FAT_PATH);
					if(strchr(folderPath, ':') == NULL)
						strcpy(tmpPath, "/");
					strcat(tmpPath, folderPath);
					strcat(tmpPath, "/");
					strcpy(folderPath, tmpPath);
				}
				_refreshExplorer();
				m_btnMgr.down();
			}
		}
		else if(BTN_A_OR_2_PRESSED)
		{
			if(m_btnMgr.selected(m_configBtnPageP))
			{
				_refreshExplorer(1);
			}
			else if(m_btnMgr.selected(m_configBtnPageM))
			{
				_refreshExplorer(-1);
			}
			else if(m_btnMgr.selected(m_configBtnCenter)) // set or import
			{
				if(folderExplorer) // set
				{
					strcpy(folderPath, dir);
					break;
				}
				if(pluginExplorer) // import
				{
					if(!importRom)
					{
						importRom = true;
						m_btnMgr.setText(m_configBtnCenter, _t("dl1", L"Cancel"));
						m_btnMgr.hide(m_explorerLblSelFolder);
						m_btnMgr.hide(m_explorerLblSelFolderBg);
						m_btnMgr.setText(m_configLblTitle, _t("explorer5", L"Select file or folder"));
						m_btnMgr.show(m_configLblTitle);
					}
					else
						_refreshExplorer();
				}
			}
			else if(m_btnMgr.selected(m_configBtnBack))
			{
				memset(folderPath, 0, MAX_FAT_PATH);
				break;
			}
			for(u8 i = 1; i < 15; ++i)
			{
				if(m_btnMgr.selected(entries_sel[i]))
				{
					/* If path is empty add device+partition#:/ to start path */
					if(dir[0] == '\0')
					{
						explorer_partition = i-1;
						strcpy(dir, fmt("%s:/", DeviceName[i-1]));
						strcpy(folderPath, dir);
						_refreshExplorer();
					}
					/* If it's a folder add folder+/ to path */
					else if(!fsop_FileExist(fmt("%s%s", dir, elements[start_pos+(i-1)].name)))
					{
						char tmpPath[MAX_FAT_PATH];
						strcat(dir, elements[start_pos+(i-1)].name);
						strcpy(folderPath, dir);
						strcat(dir, "/");
						while(strlen(folderPath) > 44)
						{
							if(strchr(folderPath, '/') == strrchr(folderPath, '/'))
								break;
							memset(tmpPath, 0, MAX_FAT_PATH);
							strncpy(tmpPath, strchr(folderPath, '/') + 1, MAX_FAT_PATH - 1);
							strcpy(folderPath, tmpPath);
						}
						memset(tmpPath, 0, MAX_FAT_PATH);
						if(strchr(folderPath, ':') == NULL)
							strcpy(tmpPath, "/");
						strcat(tmpPath, folderPath);
						strcat(tmpPath, "/");
						strcpy(folderPath, tmpPath);
						/* If import button was clicked, copy folder to romdir */
						if(importRom)
						{
							_hideExplorer();
							_showProgress();
							m_thrdMessage = _t("wbfsop95", L"Importing game to coverflow folder...");
							m_thrdProgress = 0.f;
							m_thrdMessageAdded = true;
							m_thrdWorking = true;
							lwp_t thread = 0;
							LWP_CreateThread(&thread, _ImportFolder, this, 0, 32768, 40);
							while(!m_exit)
							{
								_mainLoopCommon();
								if((BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnBack))) && !m_thrdWorking)
									break;
								if(m_thrdMessageAdded)
								{
									LockMutex lock(m_mutex);
									m_thrdMessageAdded = false;
									if(!m_thrdMessage.empty())
										m_btnMgr.setText(m_configLblDialog, m_thrdMessage);
									m_btnMgr.setProgress(m_wbfsPBar, m_thrdProgress);
									m_btnMgr.setText(m_wbfsLblMessage, wfmt(L"%i%%", (int)(m_thrdProgress * 100.f)));
								}
								if(!m_thrdWorking)
									m_btnMgr.show(m_configBtnBack);
							}
							_hideConfigFull(true);
							_showExplorer();
						}
						else
						{
							_refreshExplorer();
							m_btnMgr.down();
						}
					}
					else
					{
						char file[MAX_FAT_PATH];
						memset(file, 0, MAX_FAT_PATH);
						strncpy(file, fmt("%s%s", dir, elements[start_pos+(i-1)].name), MAX_FAT_PATH - 1);
					
						/* If import button was clicked, copy file to romdir */
						if(importRom)
						{
							_hideExplorer();
							_showProgress();
							m_thrdMessage = _t("wbfsop95", L"Importing game to coverflow folder...");
							m_thrdProgress = 0.f;
							_start_pThread();
							m_thrdMessageAdded = true;
					
							const char *title = NULL;
							title = strrchr(file, '/') + 1; // "filename.ext"
							char target[MAX_FAT_PATH];
							target[MAX_FAT_PATH - 1] = '\0';
							strcpy(target, romDirPath);
							strcat(target, title);
							if(!fsop_FileExist(target))
							{
								if(fsop_CopyFile(file, target, NULL, NULL)) // no progress feedback
								{
									m_cfg.setBool(plugin_domain, "update_cache", true);
									m_cfg.setString("PLUGIN_ITEM", fmt("%08x", magicNum), title);
									_setThrdMsg(wfmt(_fmt("explorer2", L"Game copied to %s"), romDirPath), 1.f);
									m_refreshGameList = true;
								}
								else
									_setThrdMsg(_t("explorer3", L"Copy failed!"), 1.f);
							}
							else
								_setThrdMsg(_t("wbfsoperr4", L"Game already installed!"), 1.f);

							_stop_pThread();							
							m_btnMgr.show(m_configBtnBack);
							m_btnMgr.down();
							while(!m_exit)
							{
								_mainLoopCommon();
								if((BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnBack))) && !m_thrdWorking)
									break;
							}
							_hideConfigFull(true);
							_showExplorer();
						}
						/* Else try to read the file */
						else
						{
							if(strcasestr(file, ".mp3") != NULL || strcasestr(file, ".ogg") != NULL)
								MusicPlayer.LoadFile(file, false);
							else if(strcasestr(file, ".iso") != NULL || strcasestr(file, ".ciso") != NULL || strcasestr(file, ".wbfs") != NULL)
							{
								_hideExplorer();
								//! Create header for id and path
								const u8 CISO_MAGIC[8] = {'C','I','S','O',0x00,0x00,0x20,0x00};
								dir_discHdr tmpHdr;
								memset(&tmpHdr, 0, sizeof(dir_discHdr));
								memcpy(tmpHdr.path, file, 255);
								//! Check wii or gc
								FILE *fp = fopen(file, "rb");
								fseek(fp, strcasestr(file, ".wbfs") != NULL ? 512 : 0, SEEK_SET);
								fread((void*)&wii_hdr, 1, sizeof(discHdr), fp);
								//! Check for CISO disc image and change offset to read the true header
								if(!memcmp((void*)&wii_hdr, CISO_MAGIC, sizeof(CISO_MAGIC)))
								{
									fseek(fp, 0x8000, SEEK_SET);
									fread((void*)&wii_hdr, 1, sizeof(discHdr), fp);
								}
								fclose(fp);
								memcpy(tmpHdr.id, wii_hdr.id, 6);
								_setExplorerCfg();
								if(wii_hdr.magic == WII_MAGIC)
									_launchWii(&tmpHdr, false);
								else if(wii_hdr.gc_magic == GC_MAGIC)
									_launchGC(&tmpHdr, false);
								_showExplorer();
							}
							else if(strcasestr(file, ".dol") != NULL || strcasestr(file, ".elf") != NULL)
							{
								_hideExplorer();
								vector<string> arguments = _getMetaXML(file);
								_setExplorerCfg();
								_launchHomebrew(file, arguments);
								_showExplorer();
							}
							else if(strcasestr(file, ".txt") != NULL || strcasestr(file, ".nfo") != NULL
								|| strcasestr(file, ".ini") != NULL || strcasestr(file, ".conf") != NULL
								|| strcasestr(file, ".cfg") != NULL || strcasestr(file, ".xml") != NULL
								|| strcasestr(file, ".log") != NULL)
							{
								_hideExplorer(true);
								m_txt_view = true;
								m_txt_path = file;
								_about(false);
								m_txt_view = false;
								m_txt_path = NULL;
								_showExplorer();
							}
							else if(strcasestr(file, ".wad") != NULL)
							{
								_hideExplorer(true);
								_Wad(file);
								_showExplorer();
							}
							/* Plugin explorer */
							else if(pluginExplorer)
							{
								_hideExplorer();
								//! Create header for settings[0] (magic #), path and type
								dir_discHdr tmpHdr;
								memset((void*)&tmpHdr, 0, sizeof(dir_discHdr));
								memcpy(tmpHdr.path, file, 255);
								tmpHdr.settings[0] = magicNum;
								tmpHdr.type = TYPE_PLUGIN;
								_setExplorerCfg();
								m_cfg.setString(plugin_domain, "cur_magic", fmt("%08x", magicNum));
								_launchPlugin(&tmpHdr);
								_showExplorer();
							}					
						}
					}
				}
			}
		}
		else
			repeatDelay = REPEATDELAY;
	}
	_hideExplorer(true);
	if(!folderExplorer)
		_initCF();
}

void CMenu::_setExplorerCfg(void)
{
	currentPartition = explorer_partition;
	if(!fromSource)
		return;
	m_cfg.setBool(general_domain, "explorer_on_start", true);
	m_cfg.setInt(general_domain, "explorer_page", curPage);
	m_cfg.setString(general_domain, "explorer_path", dir);
	m_cfg.setString(general_domain, "explorer_root", startDir);
}

extern const u8 tex_blank_png[];
extern const u8 tex_list_s_png[];

void CMenu::_initExplorer()
{
	memset(dir, 0, MAX_FAT_PATH);
	TexData texBtnEntry;
	TexHandle.fromPNG(texBtnEntry, tex_blank_png);
	TexData texBtnEntryS;
	TexHandle.fromPNG(texBtnEntryS, tex_list_s_png);

	_addUserLabels(m_explorerLblUser, ARRAY_SIZE(m_explorerLblUser), "EXPLORER");
	m_explorerLblSelFolderBg = _addLabel("EXPLORER/SELECTED_FOLDER_BG", theme.lblFont, L"", 0, 26, 640, 48, theme.lblFontColor, FTGX_JUSTIFY_LEFT, theme.btnTexC);
	m_explorerLblSelFolder = _addText("EXPLORER/SELECTED_FOLDER", theme.txtFont, L"", 30, 34, 560, 40, theme.txtFontColor, FTGX_JUSTIFY_LEFT);
	for(u8 i = 0; i < 15; ++i)
	{
		entries_sel[i] = _addPicButton(fmt("EXPLORER/ENTRY_%i_BTN", i), texBtnEntry, texBtnEntryS, 20, 76 + (i * 22), 600, 22);
		entries[i] = _addText(fmt("EXPLORER/ENTRY_%i", i), theme.txtFont, L"", 20, 73 + (i * 22), 600, 20, theme.txtFontColor, FTGX_JUSTIFY_LEFT | FTGX_ALIGN_MIDDLE);
		_setHideAnim(entries_sel[i], fmt("EXPLORER/ENTRY_%i_BTN", i), 0, 0, 1.f, 0.f);
		_setHideAnim(entries[i], fmt("EXPLORER/ENTRY_%i", i), 0, 0, 1.f, 0.f);
	}

	_setHideAnim(m_explorerLblSelFolderBg, "EXPLORER/SELECTED_FOLDER_BG", 0, 0, -2.f, 0.f);
	_setHideAnim(m_explorerLblSelFolder, "EXPLORER/SELECTED_FOLDER", 0, 0, -2.f, 0.f);

	// _hideExplorer(true);
}

/********************************************************************************************************/

static bool list_element_cmp(list_element a, list_element b)
{
	const char *first = a.name;
	const char *second = b.name;
	return char_cmp(first, second, strlen(first), strlen(second));
}

void CMenu::_refreshExplorer(s8 direction)
{
	for(u8 i = 0; i < 15; ++i)
	{
		m_btnMgr.hide(entries[i], true);
		m_btnMgr.hide(entries_sel[i], true);
		m_btnMgr.setText(entries[i], L" ");
	}
	m_btnMgr.setText(entries[0], L". . .");
	wstringEx path(_t("cfgne36", L"Path ="));
	path.append(wfmt(L" %.48s", folderPath));
	m_btnMgr.setText(m_explorerLblSelFolder, path, true);

	if(direction == 0)
		start_pos = 0;
	if(direction == 2) // if explorer on start return to last page
		start_pos = 14 * (curPage - 1);

	/* If path is empty show device+partitions only */
	if(dir[0] == '\0')
	{
		elements_num = 0;
		for(u8 i = 1; i < 15; ++i)
		{
			if(DeviceHandle.IsInserted(i-1))
			{
				m_btnMgr.setText(entries[i], wfmt(L"%s:/", DeviceName[i-1]));
				m_btnMgr.show(entries[i]);
				m_btnMgr.show(entries_sel[i]);
			}
		}
	}
	/* Else show folders and files */
	else
	{
		m_btnMgr.show(entries[0]);
		m_btnMgr.show(entries_sel[0]);
		if(direction == 0)
		{
			dirent *pent = NULL;
			DIR *pdir = NULL;
			pdir = opendir(dir);
			/* Some sorting */
			dirs = 0;
			files = 0;
			while((pent = readdir(pdir)) != NULL)
			{
				if(pent->d_name[0] == '.')
					continue;
				if(pent->d_type == DT_DIR)
					dirs++;
				else if(pent->d_type == DT_REG)
				{
					const char *fileType = strrchr(pent->d_name, '.');
					if(!wadsOnly || (fileType != NULL && strcasecmp(fileType, ".wad") == 0))
						files++;
				}
			}
			u32 pos = 0;
			if(elements != NULL)
				MEM2_free(elements);
			elements_num = (folderExplorer ? dirs : dirs+files);
			elements = (list_element*)MEM2_alloc(elements_num*sizeof(list_element));
			if(dirs > 0)
			{
				rewinddir(pdir);
				while((pent = readdir(pdir)) != NULL)
				{
					if(pent->d_name[0] == '.')
						continue;
					if(pent->d_type == DT_DIR)
					{
						memcpy(elements[pos].name, pent->d_name, NAME_MAX);
						pos++;
					}
				}
				std::sort(elements, elements+pos, list_element_cmp);
			}
			if(folderExplorer == false && files > 0)
			{
				rewinddir(pdir);
				while((pent = readdir(pdir)) != NULL)
				{
					if(pent->d_name[0] == '.')
						continue;
					if(pent->d_type == DT_REG)
					{
						//! Here we will check pent->d_name to make sure it's a wad file and add it if it is
						const char *fileType = strrchr(pent->d_name, '.');
						if(!wadsOnly || (fileType != NULL && strcasecmp(fileType, ".wad") == 0))
						{
							memcpy(elements[pos].name, pent->d_name, NAME_MAX);
							pos++;
						}
					}
				}
				std::sort(elements+dirs, elements+pos, list_element_cmp);
			}
			closedir(pdir);
		}
		if(direction == -1)									//! don't question
			start_pos = start_pos >= 14 ? start_pos - 14 : (elements_num % 14 ? (elements_num - elements_num % 14) : elements_num - 14);
		else if(direction == 1)
			start_pos = start_pos + 14 >= elements_num ? 0 : start_pos + 14;

		for(u8 i = 1; i < 15; i++)
		{
			if(start_pos+i > elements_num)
				break;
			if(start_pos+i <= dirs)
				m_btnMgr.setText(entries[i], wfmt(L"/%.96s", elements[start_pos+i-1].name), true);
			else
				m_btnMgr.setText(entries[i], wfmt(L"%.96s", elements[start_pos+i-1].name), true);
			m_btnMgr.show(entries[i]);
			m_btnMgr.show(entries_sel[i]);
		}
	}

	if(elements_num > 0 && ((elements_num - 1) / 14 + 1) > 1)
	{
		curPage = (start_pos / 14 + 1);
		m_btnMgr.setText(m_configLblPage, wfmt(L"%i / %i", curPage, ((elements_num - 1) / 14 + 1)));
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
	
	if(pluginExplorer && showImport)
	{
		importRom = false; // cancel import rom mode
		m_btnMgr.hide(m_configLblTitle);
		m_btnMgr.show(m_explorerLblSelFolder);
		m_btnMgr.show(m_explorerLblSelFolderBg);
		m_btnMgr.setText(m_configBtnCenter, _t("explorer1", L"Import"));
		//! If explorer path and plugin romdir are the same, don't show import button
		if((strncasecmp(dir, romDirPath, strlen(romDirPath)) == 0))
			m_btnMgr.hide(m_configBtnCenter); // import
		else
			m_btnMgr.show(m_configBtnCenter);
	}
}

const char *CMenu::_FolderExplorer(const char *startPath)
{
	folderExplorer = true;
	memset(dir, 0, MAX_FAT_PATH);
	strcpy(dir, startPath);
	if(dir[strlen(dir) - 1] != '/')
		strcat(dir, "/");
	_Explorer();
	folderExplorer = false;
	if(strrchr(folderPath, '/') != NULL)
		*strrchr(folderPath, '/') = '\0';
	return folderPath;
}

void CMenu::_wadExplorer(void)
{
	wadsOnly = true;
	_Explorer();
	wadsOnly = false;
}

void CMenu::_FileExplorer(const char *startPath)
{
	memset(dir, 0, MAX_FAT_PATH);
	strcpy(dir, startPath);
	if(dir[strlen(dir) - 1] != '/')
		strcat(dir, "/");
	_Explorer();
	return;
}

/** Plugin explorer
Will restrict path to plugin rom directory and allow launch of roms
Will also allow to copy ("import") roms from "explorer_path" directory to coverflow rom directory
if launched from source menu using an "explorer" source button in source_menu.ini file
"explorer_path" has to be defined in plugin.ini file **/
void CMenu::_pluginExplorer(const char *startPath, u32 magic, bool source)
{
	u8 pos = m_plugin.GetPluginPosition(magic);
	
	//! Simple file explorer if not a valid magic number, or if magic is wii, gc, emunand, realnand, scumm or hb
	if(pos == 255 || magic == 0x4E574949 || magic == 0x4E47434D || magic == 0x454E414E || magic == 0x4E414E44 || magic == 0x5343564D || strncasecmp(fmt("%06x", magic), "484252", 6) == 0)
		pluginExplorer = false;
	else
		pluginExplorer = true;
	fromSource = source;
	
	memset(dir, 0, MAX_FAT_PATH);
	strcpy(dir, startPath);
	
	//! Get plugin romdir full path
	const char *romDir = m_plugin.GetRomDir(pos);
	explorer_partition = m_plugin.GetRomPartition(pos);
	if(explorer_partition == -1) // default rom partition
	{
		string domain;
		switch(magic)
		{
			case 0x4E574949:
				domain = wii_domain;
				break;
			case 0x4E47434D:
				domain = gc_domain;
				break;
			case 0x454E414E: // emunand
			case 0x4E414E44: // realnand
				domain = channel_domain;
				break;
			case 0x48425257:
				domain = homebrew_domain;
				break;
			default:
				domain = plugin_domain;
				break;
		}
		explorer_partition = m_cfg.getInt(domain, "partition", USB1);
	}
	romDirPath[MAX_FAT_PATH - 1] = '\0';
	strcpy(romDirPath, fmt("%s:/%s", DeviceName[explorer_partition], romDir));
	if(romDirPath[strlen(romDirPath) - 1] != '/')
		strcat(romDirPath, "/");
	
	//! If startPath is empty default to romdir full path (rompartition:/romdir)
	if(dir[0] == '\0')
		strcpy(dir, romDirPath);
	if(dir[strlen(dir) - 1] != '/')
		strcat(dir, "/");

	//! Only show import button if plugin explorer ok and startpath is different from romdir
	showImport = (pluginExplorer && strcasecmp(dir, romDirPath) != 0) ? true : false;

	importRom = false;
	magicNum = magic;

	_Explorer();
	
	pluginExplorer = false;
	fromSource = false;
}

/** Import Folder
Copy CD-ROM folder from any source to plugin rom directory
- dir: "device:/folder/game_folder/" (source)
- newFolder: "game_folder" (used to set plugin item and build target string)
- newFolderPath: "device:/coverflow_romdir/game_folder/" (target) **/
void * CMenu::_ImportFolder(void *obj)
{
	CMenu &m = *(CMenu *)obj;
		
	string newFolder(dir);
	newFolder = newFolder.substr(0, newFolder.find_last_of("/"));
	newFolder = newFolder.substr(newFolder.find_last_of("/") + 1);
	
	char newFolderPath[MAX_FAT_PATH];
	newFolderPath[MAX_FAT_PATH - 1] = '\0';
	strcpy(newFolderPath, romDirPath);
	strcat(newFolderPath, fmt("%s/", newFolder.c_str()));

	if(!fsop_FolderExist(newFolderPath))
	{
		fsop_MakeFolder(newFolderPath);
		if(fsop_CopyFolder(dir, newFolderPath, m._addDiscProgress, obj))
		{
			m.m_cfg.setBool(m.plugin_domain, "update_cache", true);
			//! we assume the file has the same name as the folder and its extension is .cue
			m.m_cfg.setString("PLUGIN_ITEM", fmt("%08x", magicNum), fmt("%s.cue", newFolder.c_str()));
			m._setThrdMsg(wfmt(m._fmt("explorer2", L"Game copied to %s"), romDirPath), 1.f);
			m.m_refreshGameList = true;
		}
		else
			m._setThrdMsg(m._t("explorer3", L"Copy failed!"), 1.f);
	}
	else
		m._setThrdMsg(m._t("wbfsoperr4", L"Game already installed!"), 1.f);
	
	m.m_thrdWorking = false;
	return NULL;
}
