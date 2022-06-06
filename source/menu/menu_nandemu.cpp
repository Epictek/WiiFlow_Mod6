
#include <algorithm>
// #include <dirent.h>
// #include <unistd.h>
// #include <sys/stat.h>

#include "menu.hpp"
#include "lockMutex.hpp"
#include "channel/nand.hpp"
#include "loader/nk.h"
// #include "loader/cios.h"

#define NANDPATHLEN 64

#ifdef APP_WIIFLOW_LITE
#define WFID4 "WFLA"
#else
#define WFID4 "DWFA"
#endif

static u8 curPage;
int curEmuNand = 0;
int curSavesNand = 0;
int channelsType = 0;
vector<string> emuNands;
vector<string> savesNands;
bool m_nandext;
bool m_nanddump;
bool m_sgdump;
bool m_saveall;

static inline int loopNum(int i, int s)
{
	return (i + s) % s;
}

void CMenu::_showNandEmu(bool instant)
{
	int i;
	
	_hideCheckboxes(true); // reset checkboxes
	m_btnMgr.show(m_configBtnBack);

	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);

	/** MAIN PAGE **/
	if(curPage == MAIN_SETTINGS)
	{
		m_btnMgr.setText(m_configLblTitle, _t("wiichannels", L"Wii channels"));
		m_btnMgr.show(m_configLblTitle);

		//! Default game language
		m_btnMgr.setText(m_configLbl[2], _t("cfgb4", L"Default game language"));
		i = min(max(0, m_cfg.getInt(channel_domain, "game_language", 0)), (int)ARRAY_SIZE(CMenu::_languages) - 2);
		m_btnMgr.setText(m_configLblVal[2], _t(CMenu::_languages[i + 1].id, CMenu::_languages[i + 1].text), true);
		m_btnMgr.show(m_configLblVal[2], instant);
		m_btnMgr.show(m_configBtnM[2], instant);
		m_btnMgr.show(m_configBtnP[2], instant);
		//! Return to Wiiflow channel
		if(!isWiiVC)
		{
			m_btnMgr.setText(m_configLbl[3], _t("cfgg21", L"Return to WiiFlow channel"));
			m_checkboxBtn[3] = m_cfg.getString(channel_domain, "returnto") == WFID4 ? m_configChkOn[3] : m_configChkOff[3];
			m_btnMgr.show(m_configLbl[3], instant);
			m_btnMgr.show(m_checkboxBtn[3], instant);
		}
		//! Download covers and info
		m_btnMgr.setText(m_configLbl[4-isWiiVC], _t("cfg3", L"Download covers and info"));
		//! Global video settings
		m_btnMgr.setText(m_configLbl[5-isWiiVC], _t("cfg803", L"Global video settings"));
	
		for(u8 i = 2; i < 6-isWiiVC; ++i)
			m_btnMgr.show(m_configLbl[i], instant);
		for(u8 i = 4-isWiiVC; i < 6-isWiiVC; ++i)
			m_btnMgr.show(m_configBtnGo[i], instant);

		if(!neek2o() && !isWiiVC)
		{
			//! Global nand emulation
			m_btnMgr.setText(m_configLbl[6], _t("cfg802", L"Global nand emulation"));
			//! Game location
			m_btnMgr.setText(m_configLbl[7], _t("cfg814", L"Manage Wii channel list"));
			
			for(u8 i = 6; i < 8; ++i)
			{
				m_btnMgr.show(m_configLbl[i], instant);
				m_btnMgr.show(m_configBtnGo[i], instant);
			}
		}	
	}

	/** GLOBAL VIDEO SETTINGS **/
	else if(curPage == VIDEO_SETTINGS)
	{
		m_btnMgr.setText(m_configLblTitle, _t("cfg803", L"Global video settings"));
		m_btnMgr.show(m_configLblTitle);
		
		//! Wii channel video mode
		m_btnMgr.setText(m_configLbl[3], _t("cfgb3", L"Default game video mode"));
		i = min(max(0, m_cfg.getInt(channel_domain, "video_mode", 0)), (int)ARRAY_SIZE(CMenu::_VideoModes) - 2);
		m_btnMgr.setText(m_configLblVal[3], _t(CMenu::_VideoModes[i + 1].id, CMenu::_VideoModes[i + 1].text), true);
		//! Wii channel video deflicker
		m_btnMgr.setText(m_configLbl[4], _t("cfgg44", L"Video deflicker"));
		i = min(max(0, m_cfg.getInt(channel_domain, "deflicker_wii", 0)), (int)ARRAY_SIZE(CMenu::_DeflickerOptions) - 2);
		m_btnMgr.setText(m_configLblVal[4], _t(CMenu::_DeflickerOptions[i + 1].id, CMenu::_DeflickerOptions[i + 1].text), true);
		//! Wii channel 480p pixel patch
		m_btnMgr.setText(m_configLbl[5], _t("cfgg49", L"480p pixel patch"));
		m_checkboxBtn[5] = m_cfg.getOptBool(channel_domain, "fix480p", 0) == 0 ? m_configChkOff[5] : m_configChkOn[5];
		m_btnMgr.show(m_checkboxBtn[5], instant);
		
		for(u8 i = 3; i < 6; ++i)
			m_btnMgr.show(m_configLbl[i], instant);
		for(u8 i = 3; i < 5; ++i)
		{
			m_btnMgr.show(m_configLblVal[i], instant);
			m_btnMgr.show(m_configBtnM[i], instant);
			m_btnMgr.show(m_configBtnP[i], instant);
		}
	}

	/** GLOBAL NAND EMULATION SETTINGS **/
	else if(curPage == NANDEMU_SETTINGS)
	{
		m_btnMgr.setText(m_configLblTitle, _t("cfg802", L"Global nand emulation"));
		m_btnMgr.show(m_configLblTitle);
		
		//! Wii channel emunand mode
		m_btnMgr.setText(m_configLbl[3], _t("cfgne1", L"Emunand mode"));
		// Minus 2 and [i + 1] to ignore "default" array value
		i = min(max(0, m_cfg.getInt(channel_domain, "emulation", 0)), (int)ARRAY_SIZE(CMenu::_NandEmu) - 2);
		m_btnMgr.setText(m_configLblVal[3], _t(CMenu::_NandEmu[i + 1].id, CMenu::_NandEmu[i + 1].text));
		m_btnMgr.show(m_configLblVal[3], instant);
		m_btnMgr.show(m_configBtnM[3], instant);
		m_btnMgr.show(m_configBtnP[3], instant);
		//! Keep emunand updated with latest nand config
		m_btnMgr.setText(m_configLbl[4], _t("cfgne40", L"Update emunand to latest config"));
		m_checkboxBtn[4] = m_cfg.getOptBool(channel_domain, "real_nand_config", 0) == 0 ? m_configChkOff[4] : m_configChkOn[4];
		//! Keep emunand updated with latest nand miis
		m_btnMgr.setText(m_configLbl[5], _t("cfgne41", L"Update emunand to latest Miis"));
		m_checkboxBtn[5] = m_cfg.getOptBool(channel_domain, "real_nand_miis", 0) == 0 ? m_configChkOff[5] : m_configChkOn[5];
		//! Launch neek2o channel emunand system menu
		m_btnMgr.setText(m_configLbl[6], _t("neek2", L"Neek2o system menu"));
		m_btnMgr.setText(m_configBtn[6], _t("cfgne6", L"Start"));
		m_btnMgr.show(m_configBtn[6], instant);
		
		for(u8 i = 3; i < 7; ++i)
			m_btnMgr.show(m_configLbl[i], instant);
		for(u8 i = 4; i < 6; ++i)
			m_btnMgr.show(m_checkboxBtn[i], instant);
		
		//! Launch WiiFlow channel on emunand in neek2o mode
		if(!IsOnWiiU())
		{
			m_btnMgr.setText(m_configLbl[7], _t("neek4", L"Neek2o Wiiflow channel"));
			m_btnMgr.setText(m_configBtn[7], _t("cfgne6", L"Start"));
			m_btnMgr.show(m_configLbl[7], instant);
			m_btnMgr.show(m_configBtn[7], instant);
		}
	}
	
	/** WII CHANNEL GAME LOCATION **/
	else if(curPage == GAME_LIST)
	{
		m_btnMgr.setText(m_configLblTitle, _t("cfg814", L"Manage Wii channel list"));
		m_btnMgr.show(m_configLblTitle);
		
		//! Wii channel game partition
		const char *partitionname = DeviceName[m_cfg.getInt(channel_domain, "partition", 0)];
		m_btnMgr.setText(m_configLbl[0], _t("part3", L"Emunands partition"));
		m_btnMgr.setText(m_configLblVal[0], upperCase(partitionname));
		//! Wii channel preffered partition
		s8 part = m_cfg.getInt(channel_domain, "preferred_partition", -1);
		partitionname = DeviceName[part];
		m_btnMgr.setText(m_configLbl[1], _t("part99", L"Preffered partition at boot"));
		m_btnMgr.setText(m_configLblVal[1], part == -1 ? _t("none", L"None") : upperCase(partitionname));
		//! Select wii channel emunand
		m_btnMgr.setText(m_configLbl[2], _t("cfgne37", L"Channel emunand folder"));
		m_btnMgr.setText(m_configLblVal[2], emuNands[curEmuNand]);	
		//! Channel mode: emunand/realnand/bothnands
		m_btnMgr.setText(m_configLbl[3], _t("cfgb7", L"Channel mode"));
		if(m_current_view & COVERFLOW_PLUGIN) // disable channel type selection if coverflow plugin
		{
			m_btnMgr.setText(m_configBtn[3], _t("plugins", L"Plugins"));
			m_btnMgr.show(m_configBtn[3], instant);
		}
		else
		{
			i = min(max(1, channelsType), (int)ARRAY_SIZE(CMenu::_ChannelsType)) - 1;
			m_btnMgr.setText(m_configLblVal[3], _t(CMenu::_ChannelsType[i].id, CMenu::_ChannelsType[i].text), true);
			m_btnMgr.show(m_configLblVal[3], instant);
			m_btnMgr.show(m_configBtnM[3], instant);
			m_btnMgr.show(m_configBtnP[3], instant);
		}
		//! Create new emunand folder
		m_btnMgr.setText(m_configLbl[4], _t("cfg821", L"Create new emunand folder"));
		//! Install Wad file
		m_btnMgr.setText(m_configLbl[5], _t("wad98", L"Install Wii channel (Wad file)"));
		//! Install Wad folder
		m_btnMgr.setText(m_configLbl[6], _t("wad96", L"Batch install folder"));
		//! Extract nand to emunand
		m_btnMgr.setText(m_configLbl[7], _t("cfgne5", L"Extract nand to emunand"));
		m_btnMgr.setText(m_configBtn[7], _t("cfgg31", L"Extract"));
		//! Dump Wii channel coverflow list
		m_btnMgr.setText(m_configLbl[8], _t("cfg783", L"Dump coverflow list"));
		m_btnMgr.setText(m_configBtn[8], _t("cfgne6", L"Start"));
		//! Refresh coverflow list and cover cache
		m_btnMgr.setText(m_configLbl[9], _t("home2", L"Refresh coverflow list"));
		m_btnMgr.setText(m_configBtn[9], _t("cfgne6", L"Start"));
		
		for(u8 i = 0; i < 10; ++i)
			m_btnMgr.show(m_configLbl[i], instant);
		for(u8 i = 0; i < 3; ++i)
		{
			m_btnMgr.show(m_configLblVal[i], instant);
			m_btnMgr.show(m_configBtnM[i], instant);
			m_btnMgr.show(m_configBtnP[i], instant);
		}
		for(u8 i = 4; i < 7; ++i)
			m_btnMgr.show(m_configBtnGo[i], instant);
		for(u8 i = 7; i < 10; ++i)
			m_btnMgr.show(m_configBtn[i], instant);
	}
}

bool CMenu::_configNandEmu(u8 startPage)
{
	int prev_channelsType = channelsType;
	bool ExtNand = false;
	string emuNand = m_cfg.getString(channel_domain, "current_emunand");
	
	channelsType = m_cfg.getInt(channel_domain, "channels_type", CHANNELS_REAL);
	curPage = startPage;

	_setBg(m_configBg, m_configBg);
	SetupInput();
	_showNandEmu();
	
	while(!m_exit)
	{
		_mainLoopCommon();
		if((BTN_HOME_HELD || (BTN_B_OR_1_PRESSED && (curPage == MAIN_SETTINGS || startPage == GAME_LIST))) && !m_thrdWorking)
			break;
		else if((BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED) && !m_thrdWorking)
			m_btnMgr.up();
		else if((BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED) && !m_thrdWorking)
			m_btnMgr.down();
		else if(BTN_B_OR_1_PRESSED && !m_thrdWorking)
		{
			_hideConfig(true);
			curPage = MAIN_SETTINGS;
			 _showNandEmu();
		}
		else if(BTN_A_OR_2_PRESSED)
		{
			if((m_btnMgr.selected(m_configBtnBack) && curPage != MAIN_SETTINGS) && !m_thrdWorking)
			{
				if(startPage == GAME_LIST)
					break;
				else
				{
					_hideConfig(true);
					curPage = MAIN_SETTINGS;
					 _showNandEmu();
				}
			}
			
			/** MAIN PAGE **/
			else if(curPage == MAIN_SETTINGS)
			{
				if(m_btnMgr.selected(m_configBtnBack))
					break;
				else
				{
					//! Default game language
					if(m_btnMgr.selected(m_configBtnP[2]) || m_btnMgr.selected(m_configBtnM[2]))
					{
						s8 direction = m_btnMgr.selected(m_configBtnP[2]) ? 1 : -1;
						m_cfg.setInt(channel_domain, "game_language", (int)loopNum(m_cfg.getUInt(channel_domain, "game_language", 0) + direction, ARRAY_SIZE(CMenu::_languages) - 1));
						_showNandEmu(true);
					}
					//! Return to WiiFlow channel
					else if(m_btnMgr.selected(m_checkboxBtn[3]))
					{
						if(m_cfg.getString(channel_domain, "returnto") == WFID4)
							m_cfg.remove(channel_domain, "returnto");
						else // check if channel exists
						{
							bool found = true;
							if(!neek2o()) // channel exists if neek2o
							{
								found = false;
								bool curNANDemuView = NANDemuView;
								NANDemuView = false;
								ChannelHandle.Init("EN");
								int amountOfChannels = ChannelHandle.Count();
								for(int i = 0; i < amountOfChannels; i++)
								{
									if(strncmp(WFID4, ChannelHandle.GetId(i), 4) == 0)
									{
										found = true;
										break;
									}
								}
								NANDemuView = curNANDemuView;
							}
							if(found)
								m_cfg.setString(channel_domain, "returnto", WFID4);
						}
						_showNandEmu(true);
						m_btnMgr.setSelected(m_checkboxBtn[3]);
					}
					//! Download covers and info
					else if(m_btnMgr.selected(m_configBtnGo[4-isWiiVC]))
					{
						_hideConfig(true);
						_download();
						_showNandEmu();
					}
					//! Global video settings
					else if(m_btnMgr.selected(m_configBtnGo[5-isWiiVC]))
					{
						_hideConfig(true);
						curPage = VIDEO_SETTINGS;
						_showNandEmu();
					}
					//! Global nand emulation
					else if(m_btnMgr.selected(m_configBtnGo[6]))
					{
						_hideConfig(true);
						curPage = NANDEMU_SETTINGS;
						_showNandEmu();
					}
					//! Game location
					else if(m_btnMgr.selected(m_configBtnGo[7]))
					{
						_hideConfig(true);
						curPage = GAME_LIST;
						_showNandEmu();
					}
				}
			}

			/** GLOBAL VIDEO SETTINGS **/
			else if(curPage == VIDEO_SETTINGS)
			{
				//! Wii channel video mode
				if(m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3]))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[3]) ? 1 : -1;
					m_cfg.setInt(channel_domain, "video_mode", (int)loopNum(m_cfg.getUInt(channel_domain, "video_mode", 0) + direction, ARRAY_SIZE(CMenu::_VideoModes) - 1)); // minus 1 because of "default" array value
					_showNandEmu(true);
				}
				//! Wii channel video deflicker
				else if(m_btnMgr.selected(m_configBtnP[4]) || m_btnMgr.selected(m_configBtnM[4]))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[4]) ? 1 : -1;
					m_cfg.setInt(channel_domain, "deflicker_wii", (int)loopNum(m_cfg.getUInt(channel_domain, "deflicker_wii", 0) + direction, ARRAY_SIZE(CMenu::_DeflickerOptions) - 1));
					_showNandEmu(true);
				}
				//! Wii channel 480p pixel patch
				else if(m_btnMgr.selected(m_checkboxBtn[5]))
				{
					m_cfg.setBool(channel_domain, "fix480p", !m_cfg.getBool(channel_domain, "fix480p"));
					_showNandEmu(true);
					m_btnMgr.setSelected(m_checkboxBtn[5]);
				}
			}
			
			/** GLOBAL NAND EMULATION SETTINGS **/
			else if(curPage == NANDEMU_SETTINGS)
			{
				//! Wii channel emunand mode
				if((m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3])))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[3]) ? 1 : -1;
					m_cfg.setInt(channel_domain, "emulation", loopNum(m_cfg.getInt(channel_domain, "emulation", 0) + direction, ARRAY_SIZE(CMenu::_NandEmu) - 1));
					_showNandEmu(true);
				}
				//! Keep emunand updated with latest nand config
				else if(m_btnMgr.selected(m_checkboxBtn[4]))
				{
					m_cfg.setBool(channel_domain, "real_nand_config", !m_cfg.getBool(channel_domain, "real_nand_config"));
					_showNandEmu(true);
					m_btnMgr.setSelected(m_checkboxBtn[4]);
				}
				//! Keep emunand updated with latest nand miis
				else if(m_btnMgr.selected(m_checkboxBtn[5]))
				{
					m_cfg.setBool(channel_domain, "real_nand_miis", !m_cfg.getBool(channel_domain, "real_nand_miis"));
					_showNandEmu(true);
					m_btnMgr.setSelected(m_checkboxBtn[5]);
				}
				//! Launch neek2o channel emunand system menu
				else if(m_btnMgr.selected(m_configBtn[6]))
				{
					if(_launchNeek2oChannel(EXIT_TO_SMNK2O, EMU_NAND))
						break;
					_showNandEmu();
				}
				//! Launch WiiFlow channel on emunand in neek2o mode
				else if(m_btnMgr.selected(m_configBtn[7]))
				{
					if(_launchNeek2oChannel(EXIT_TO_WFNK2O, EMU_NAND))
						break;
					_showNandEmu();
				}
			}
			
			/** GAME LOCATION **/
			else if(curPage == GAME_LIST)
			{
				//! Wii channel partition
				if(m_btnMgr.selected(m_configBtnP[0]) || m_btnMgr.selected(m_configBtnM[0]))
				{
					m_prev_view = m_current_view;
					u8 prevPartition = currentPartition;
					s8 direction = m_btnMgr.selected(m_configBtnP[0]) ? 1 : -1;
					currentPartition = m_cfg.getInt(channel_domain, "partition");
					m_current_view = COVERFLOW_CHANNEL;
					_setPartition(direction);
					_checkEmuNandSettings(EMU_NAND); // refresh emunands in case the partition was changed
					_showNandEmu(true);
					if((m_prev_view & COVERFLOW_CHANNEL) || (m_prev_view & COVERFLOW_PLUGIN && m_plugin.GetEnabledStatus(m_plugin.GetPluginPosition(0x454E414E))))
						m_refreshGameList = true;
					m_current_view = m_prev_view;
					currentPartition = prevPartition;
				}
				//! Wii channel preffered partition
				else if(m_btnMgr.selected(m_configBtnP[1]) || m_btnMgr.selected(m_configBtnM[1]))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[1]) ? 1 : -1;
					int part = m_cfg.getInt(channel_domain, "preferred_partition", -1);
					if((part == 8 && direction == 1) || (part == 0 && direction == -1))
						part = -1;
					else if(part == -1 && direction == -1)
						part = 8;
					else
						part = loopNum(part + direction, 9);
					m_cfg.setInt(channel_domain, "preferred_partition", part);
					_showNandEmu(true);
				}
				//! Select wii channel emunand
				else if((m_btnMgr.selected(m_configBtnP[2]) || m_btnMgr.selected(m_configBtnM[2])))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[2]) ? 1 : -1;
					curEmuNand = loopNum(curEmuNand + direction, emuNands.size());
					m_cfg.setString(channel_domain, "current_emunand", emuNands[curEmuNand]);
					_FullNandCheck(EMU_NAND);
					_showNandEmu(true);
				}
				//! Channel mode: emunand/realnand/bothnands
				else if((m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3])))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[3]) ? 1 : -1;
					channelsType = 1 + (int)loopNum((channelsType) - 1 + direction, ARRAY_SIZE(CMenu::_ChannelsType));
					m_cfg.setInt(channel_domain, "channels_type", channelsType);
					_showNandEmu(true);
					m_refreshGameList = true;
				}
				//! Create new emunand folder
				else if(m_btnMgr.selected(m_configBtnGo[4]))
				{
					_hideConfig(true);
					char *c = NULL;
					c = _keyboard();
					if(strlen(c) > 0)
					{
						const char *newNand = fmt("%s:/%s/%s", DeviceName[m_cfg.getInt(channel_domain, "partition")], emu_nands_dir, lowerCase(c).c_str());
						if(error(wfmt(_fmt("errcfg3", L"Create %s?"), newNand), true))
						{
							fsop_MakeFolder(newNand);
							_checkEmuNandSettings(EMU_NAND);
							error(_t("dlmsg14", L"Done."));
						}
					}
					_showNandEmu();
				}
				//! Install wad file
				else if(m_btnMgr.selected(m_configBtnGo[5]))
				{
					_hideConfig(true);
					_wadExplorer();
					_showNandEmu();
				}
				//! Install wad folder
				else if(m_btnMgr.selected(m_configBtnGo[6]))
				{
					_hideConfig(true);
					const char *path = NULL;
					path = _FolderExplorer(fmt("%s:/", DeviceName[currentPartition]));
					if(strlen(path) > 0)
						_Wad(path, true);
					_showNandEmu();
				}
				//! Extract nand to emunand
				else if(m_btnMgr.selected(m_configBtn[7]))
				{
					const char *currentNand = fmt("%s:/%s/%s", DeviceName[m_cfg.getInt(channel_domain, "partition")], emu_nands_dir, emuNands[curEmuNand].c_str());
					if(error(wfmt(_fmt("errcfg4", L"Extract nand to %s?"), currentNand), true))
					{
						if(_NandDump(0)) // 0 = Full nand dump
							ExtNand = true;
					}
					_showNandEmu();
				}
				//! Dump Wii channel coverflow list
				else if(m_btnMgr.selected(m_configBtn[8]))
				{
					_dumpGameList();
					_showNandEmu();
				}
				//! Refresh coverflow list and cover cache
				else if(m_btnMgr.selected(m_configBtn[9]))
				{
					m_cfg.setBool(channel_domain, "update_cache", true);
					if(m_current_view & COVERFLOW_PLUGIN)
						m_cfg.setBool(plugin_domain, "update_cache", true);
					m_refreshGameList = true;
					break;
				}
			}
		}
	}
	_hideConfig(true);
	
	/* If changed emunand choice or nand has just been extracted */
	if(!isWiiVC && (emuNand != emuNands[curEmuNand] || ExtNand))
	{
		//! we only need to force update cache in case nand has just been extracted
		if(ExtNand)
			m_cfg.setBool(channel_domain, "update_cache", true);
		if((m_current_view & COVERFLOW_CHANNEL) || (m_current_view & COVERFLOW_PLUGIN && m_plugin.GetEnabledStatus(m_plugin.GetPluginPosition(0x454E414E))))
			m_refreshGameList = true;
	}
	
	if(channelsType != prev_channelsType)
	{
		_srcTierBack(true);
		_getCustomBgTex();
	}
	
	return 0;
}

/********************************************************************************************************/

/** Used in main() and _configWii() **/
string CMenu::_SetEmuNand(s8 direction)
{
	if(isWiiVC || neek2o())
		return "";
	//! if wii games currently displayed in coverflow set savesnand
	if(m_current_view & COVERFLOW_WII ||
	(m_current_view & COVERFLOW_PLUGIN && m_plugin.GetEnabledStatus(m_plugin.GetPluginPosition(0x4E574949))))
	{
		curSavesNand = loopNum(curSavesNand + direction, savesNands.size());
		m_cfg.setString(wii_domain, "current_save_emunand", savesNands[curSavesNand]);
		m_refreshGameList = false;
		_FullNandCheck(SAVES_NAND);
		return savesNands[curSavesNand];
	}
	//! else if emunand currently displayed in coverflow, along with realnand or not
	else if((m_current_view & COVERFLOW_CHANNEL && 
	(m_cfg.getInt(channel_domain, "channels_type") >= CHANNELS_EMU || neek2o())) || 
	(m_current_view & COVERFLOW_PLUGIN && m_plugin.GetEnabledStatus(m_plugin.GetPluginPosition(0x454E414E))))
	{
		curEmuNand = loopNum(curEmuNand + direction, emuNands.size());
		m_cfg.setString(channel_domain, "current_emunand", emuNands[curEmuNand]);
		m_refreshGameList = true;
		_FullNandCheck(EMU_NAND);		
		return emuNands[curEmuNand];
	}
	return "";
}

void CMenu::_listEmuNands(const char *path, vector<string> &emuNands)
{
	DIR *d;
	struct dirent *dir;
	bool add_def = true;
	
	emuNands.clear();
	
	d = opendir(path);
	if(d != 0)
	{
		while((dir = readdir(d)) != 0)
		{
			if(dir->d_name[0] == '.')
				continue;
			if(dir->d_type == DT_DIR)
			{
				emuNands.push_back(dir->d_name);
				if(strlen(dir->d_name) == 7 && strcasecmp(dir->d_name, "default") == 0)
					add_def = false;
			}
		}
		closedir(d);
	}

	if(add_def)
		emuNands.push_back("default");
	sort(emuNands.begin(), emuNands.end());
}

void CMenu::_checkEmuNandSettings(int nand_type)
{
	if(isWiiVC)
		return;
		
	u8 i;
	string emuNand;
	int emuPart;
	
	if(nand_type == EMU_NAND)
	{
		emuNand = m_cfg.getString(channel_domain, "current_emunand", "default");
		emuPart = m_cfg.getInt(channel_domain, "partition", 0);
	}
	else
	{
		emuNand = m_cfg.getString(wii_domain, "current_save_emunand", "default");
		emuPart = m_cfg.getInt(wii_domain, "savepartition", 0);
	}
		

	if(!DeviceHandle.PartitionUsableForNandEmu(emuPart)) // current partition no good
	{
		for(i = SD; i < MAXDEVICES; i++) // find first usable partition
		{
			if(DeviceHandle.PartitionUsableForNandEmu(i))
			{
				emuPart = i;
				break;
			}
		}
		if(i == MAXDEVICES) // if no usable partition found set to SD for now
			emuPart = SD;
	}
	
	/* emu Nands */
	if(nand_type == EMU_NAND)
	{
		_listEmuNands(fmt("%s:/%s", DeviceName[emuPart],  emu_nands_dir), emuNands);

		curEmuNand = 0;
		for(i = 0; i < emuNands.size(); ++i) // find current emunand folder
		{
			if(emuNands[i] == emuNand)
			{
				curEmuNand = i;
				break;
			}
		}
		if(i == emuNands.size()) // didn't find emunand folder so set to default
		{
			for(i = 0; i < emuNands.size(); ++i)
			{
				if(emuNands[i] == "default")
				{
					curEmuNand = i;
					break;
				}
			}
		}
		m_cfg.setString(channel_domain, "current_emunand", emuNands[curEmuNand]);
		m_cfg.setInt(channel_domain, "partition", emuPart);
	}
	/* saves Nands */
	else
	{
		_listEmuNands(fmt("%s:/%s", DeviceName[emuPart],  emu_nands_dir), savesNands);
		
		curSavesNand = 0;
		for(i = 0; i < savesNands.size(); ++i) // find current savesnand folder
		{
			if(savesNands[i] == emuNand)
			{
				curSavesNand = i;
				break;
			}
		}
		if(i == savesNands.size()) // didn't find savesnand folder set to default
		{ 
			for(i = 0; i < savesNands.size(); ++i)
			{
				if(savesNands[i] == "default")
				{
					curSavesNand = i;
					break;
				}
			}
		}
		m_cfg.setString(wii_domain, "current_save_emunand", savesNands[curSavesNand]);
		m_cfg.setInt(wii_domain, "savepartition", emuPart);
	}
	
	_FullNandCheck(nand_type);
}

/** This checks if nand emulation is set to Full
if it is then it copies SYSCONF, setting.txt, and RFL_DB.dat only if they don't already exist
this is helpful if you use a modmii created nand, wiiflow dumped nand will already have these files 
if you wish to overwrite these files then use the real nand mii's and config options **/
void CMenu::_FullNandCheck(int nand_type)
{
	if(isWiiVC || neek2o())
		return;
		
	if(_FindEmuPart(nand_type, true) < 0)
		return;
	
	bool need_config = false;
	bool need_miis = false;
	const char *emuPath = NandHandle.GetPath();
	gprintf("%s path = %s\n", nand_type == EMU_NAND ? "emunand" : "saves nand", emuPath);
	char testpath[NANDPATHLEN + 42];
	
	/* Add folders */
	NandHandle.CreatePath("%s/import", emuPath);
	NandHandle.CreatePath("%s/meta", emuPath);
	NandHandle.CreatePath("%s/shared1", emuPath);
	NandHandle.CreatePath("%s/shared2", emuPath);
	NandHandle.CreatePath("%s/sys", emuPath);
	NandHandle.CreatePath("%s/title", emuPath);
	NandHandle.CreatePath("%s/ticket", emuPath);
	NandHandle.CreatePath("%s/tmp", emuPath);
	
	/* Check config file - time and date, video settings, etc... */
	snprintf(testpath, sizeof(testpath), "%s/shared2/sys/SYSCONF", emuPath);
	if(!fsop_FileExist(testpath))
		need_config = true;
		
	/* System info like model and serial numbers (Modmii creates this file) */
	snprintf(testpath, sizeof(testpath), "%s/title/00000001/00000002/data/setting.txt", emuPath);
	if(!fsop_FileExist(testpath))
		need_config = true;
		
	/* Check Mii's */
	snprintf(testpath, sizeof(testpath), "%s/shared2/menu/FaceLib/RFL_DB.dat", emuPath);
	if(!fsop_FileExist(testpath))
		need_miis = true;
		
	NandHandle.PreNandCfg(need_miis, need_config); // copy to emunand if needed
}

/** This finds current emunand/savenand partition and path and SETS THEM **/
int CMenu::_FindEmuPart(bool savesnand, bool skipchecks)
{
	int emuPart;
	char tmpPath[NANDPATHLEN];
	tmpPath[NANDPATHLEN - 1] = '\0';
	bool folder_exist = false;

	if(savesnand)
	{
		emuPart = m_cfg.getInt(wii_domain, "savepartition");
		strncpy(tmpPath, fmt("/%s/%s", emu_nands_dir, m_cfg.getString(wii_domain, "current_save_emunand").c_str()), sizeof(tmpPath) - 1);
	}
	else
	{
		emuPart = m_cfg.getInt(channel_domain, "partition");
		strncpy(tmpPath, fmt("/%s/%s", emu_nands_dir, m_cfg.getString(channel_domain, "current_emunand").c_str()), sizeof(tmpPath) - 1);
	}
	if(!DeviceHandle.PartitionUsableForNandEmu(emuPart)) // check if device is mounted and partition is FAT
		return -1;	
	else if(!skipchecks)
		folder_exist = fsop_MakeFolder(fmt("%s:%s", DeviceName[emuPart], tmpPath));
	else
		folder_exist = fsop_FolderExist(fmt("%s:%s", DeviceName[emuPart], tmpPath));
	if(folder_exist)
	{
		NandHandle.SetNANDEmu(emuPart);
		NandHandle.SetPaths(tmpPath, DeviceName[emuPart]);
		return emuPart;
		//! now emunand is ready, games will boot using this path
	}
	return -2; // make emunand folder failed
}

/********************************************************************************************************/

bool CMenu::_checkSave(string id, int nand_type)
{
	int savePath = id.c_str()[0] << 24 | id.c_str()[1] << 16 | id.c_str()[2] << 8 | id.c_str()[3];
	if(nand_type == REAL_NAND)
	{
		u32 temp = 0;
		if(ISFS_ReadDir(fmt("/title/00010000/%08x", savePath), NULL, &temp) < 0)
			if(ISFS_ReadDir(fmt("/title/00010004/%08x", savePath), NULL, &temp) < 0)
				return false;
	}
	else // SAVES_NAND
	{
		const char *emuPath = NandHandle.GetPath();
		if(emuPath == NULL)
			return false;
		struct stat fstat;
		if((stat(fmt("%s/title/00010000/%08x", emuPath, savePath), &fstat) != 0) && 
		(stat(fmt("%s/title/00010004/%08x", emuPath, savePath), &fstat) != 0))
			return false;
	}
	return true;
}

void CMenu::_setDumpMsg(const wstringEx &msg, float totprog, float fileprog)
{
	if(m_thrdStop) return;
	if(msg != L"...") m_thrdMessage = msg;
	m_thrdMessageAdded = true;
	m_thrdProgress = totprog;
	m_fileProgress = fileprog;
}

void CMenu::_ShowProgress(int dumpstat, int dumpprog, int filesize, int fileprog, int files, int folders, const char *tmess, void *user_data)
{
	CMenu &m = *(CMenu *)user_data;
	m.m_progress = dumpprog == 0 ? 0.f : (float)dumpstat / (float)dumpprog;
	m.m_fprogress = filesize == 0 ? 0.f : (float)fileprog / (float)filesize;
	m.m_fileprog = fileprog;
	m.m_filesize = filesize;
	m.m_filesdone = files;
	m.m_foldersdone = folders;
	LWP_MutexLock(m.m_mutex);
	if(m_nandext)
		m._setDumpMsg(wfmt(m._fmt("cfgne9", L"Current file: %s"), tmess), m.m_progress, m.m_fprogress);
	else
		m._setDumpMsg(L"...", m.m_progress, m.m_fprogress);
	LWP_MutexUnlock(m.m_mutex);
}

bool CMenu::_NandDump(int DumpType)
{
	lwp_t thread = 0;
	m_thrdStop = false;
	m_thrdMessageAdded = false;
	m_nandext = false;
	m_saveExtGameId = ""; // in case an individual game save has just been extracted from game settings
	m_nanddump = DumpType == 0;
	m_saveall = DumpType == 2;

	if(_FindEmuPart(!m_nanddump, false) < 0) // make emunand folder if it doesn't exist
	{
		error(_t("cfgne8", L"No valid FAT partition found for nand emulation!"));
		return 0;
	}
	
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_wbfsPBar);
	m_btnMgr.show(m_wbfsLblMessage);
	m_btnMgr.show(m_configLblDialog);
	m_btnMgr.setText(m_wbfsLblMessage, L"");
	if(m_nanddump)
		m_btnMgr.setText(m_configLblTitle, _t("cfgne12", L"Nand extractor"));
	else // saves dump
		m_btnMgr.setText(m_configLblTitle, _t("cfgne13", L"Game save extractor"));
	m_thrdStop = false;
	m_thrdProgress = 0.f;
	m_thrdWorking = true;
	LWP_CreateThread(&thread, _NandDumper, this, 0, 32768, 40);
	
	while(!m_exit)
	{
		_mainLoopCommon();
		if(!m_thrdWorking && (BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnBack))))
			break;
		if(m_thrdMessageAdded)
		{
			LockMutex lock(m_mutex);
			m_thrdMessageAdded = false;
			if(!m_thrdMessage.empty())
				m_btnMgr.setText(m_configLblDialog, m_thrdMessage);
			m_btnMgr.setProgress(m_wbfsPBar, m_thrdProgress);
			m_btnMgr.setText(m_wbfsLblMessage, wfmt(_fmt("dumpprogress", L"%i%%"), (int)(m_thrdProgress*100.f)));

			if(!m_thrdWorking)
			{
				if(m_sgdump) // save game dump
					m_btnMgr.setText(m_configLblDialog, wfmt(_fmt("cfgne14", L"Processed: %d save(s) / %d block(s)"), m_nandexentry, (m_dumpsize / 0x8000) >> 2));
				else // nand dump
					m_btnMgr.setText(m_configLblDialog, wfmt(_fmt("cfgne15", L"Processed: %d files, %d folders, %d %sB, %d blocks"), m_filesdone, m_foldersdone, (m_dumpsize / 0x400 > 0x270f) ? (m_dumpsize / 0x100000) : (m_dumpsize / 0x400), (m_dumpsize / 0x400 > 0x270f) ? "M" : "K", (m_dumpsize / 0x8000) >> 2));
				m_btnMgr.show(m_configBtnBack);
			}
		}
	}
	_hideConfigFull(true);
	return 1;
}

int CMenu::_FlashSave(string gameId)
{
	if(_FindEmuPart(SAVES_NAND, true) < 0) //
		return 0;

	if(!_checkSave(gameId, SAVES_NAND)) // if save not on saves emunand
		return 0;

	lwp_t thread = 0;
	m_thrdStop = false;
	m_thrdMessageAdded = false;
	m_nandext = false;
	m_saveExtGameId = gameId;

	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_wbfsPBar);
	m_btnMgr.show(m_wbfsLblMessage);
	m_btnMgr.show(m_configLblDialog);
	m_btnMgr.setText(m_wbfsLblMessage, L"");
	m_btnMgr.setText(m_configLblTitle, _t("cfgne28", L"Game save flasher"));
	m_thrdProgress = 0.f;
	m_thrdWorking = true;
	LWP_CreateThread(&thread, _NandFlasher, this, 0, 32768, 40);
			
	SetupInput();

	while(!m_exit)
	{
		_mainLoopCommon();
		if(!m_thrdWorking && (BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnBack))))
			break;

		if(m_thrdMessageAdded)
		{
			LockMutex lock(m_mutex);
			m_thrdMessageAdded = false;
			if(!m_thrdMessage.empty())
				m_btnMgr.setText(m_configLblDialog, m_thrdMessage);
			m_btnMgr.setProgress(m_wbfsPBar, m_thrdProgress);
			m_btnMgr.setText(m_wbfsLblMessage, wfmt(_fmt("dumpprogress", L"%i%%"), (int)(m_thrdProgress*100.f)));

			if(!m_thrdWorking)
			{
				m_btnMgr.setText(m_configLblDialog, wfmt(_fmt("cfgne14", L"Processed: %d save(s), %d block(s)"), m_nandexentry, (m_dumpsize / 0x8000) >> 2));
				m_btnMgr.show(m_configBtnBack);
			}
		}
	}
	_hideConfigFull(true);
	
	return 1;
}

/** Extract a gamesave from real nand to savesnand, used in _gameSettings() **/
int CMenu::_AutoExtractSave(string gameId)
{
	if(_FindEmuPart(SAVES_NAND, false) < 0) // make emunand folder if it doesn't exist
	{
		error(_t("cfgne8", L"No valid FAT partition found for nand emulation!"));
		return 0;
	}
	
	if(!_checkSave(gameId, REAL_NAND)) // if save not on real nand
		return 0;

	lwp_t thread = 0;
	m_thrdStop = false;
	m_thrdMessageAdded = false;
	m_nandext = false;
	m_saveExtGameId = gameId;
	bool finished = false;
	m_nanddump = false;

	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_wbfsPBar);
	m_btnMgr.show(m_wbfsLblMessage);
	m_btnMgr.show(m_configLblDialog);
	m_btnMgr.setText(m_wbfsLblMessage, L"");
	m_btnMgr.setText(m_configLblTitle, _t("cfgne13", L"Game save extractor"));
	m_thrdProgress = 0.f;
	m_thrdWorking = true;
	LWP_CreateThread(&thread, _NandDumper, this, 0, 32768, 40);
	
	SetupInput();

	while(!m_exit)
	{
		_mainLoopCommon();
		if(finished && (BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnBack))))
			break;

		if(m_thrdMessageAdded)
		{
			LockMutex lock(m_mutex);
			m_thrdMessageAdded = false;
			if(!m_thrdMessage.empty())
				m_btnMgr.setText(m_configLblDialog, m_thrdMessage);
			m_btnMgr.setProgress(m_wbfsPBar, m_thrdProgress);
			m_btnMgr.setText(m_wbfsLblMessage, wfmt(_fmt("dumpprogress", L"%i%%"), (int)(m_thrdProgress*100.f)));

			if(!m_thrdWorking)
			{
				finished = true;
				m_btnMgr.setText(m_configLblDialog, wfmt(_fmt("cfgne14", L"Processed: %d save(s), %d block(s)"), m_nandexentry, (m_dumpsize / 0x8000) >> 2));
				m_btnMgr.show(m_configBtnBack);
			}
		}
	}
	_hideConfigFull(true);
	return 1;
}

void * CMenu::_NandFlasher(void *obj)
{
	CMenu &m = *(CMenu *)obj;

	char source[NANDPATHLEN + 32];
	char dest[ISFS_MAXPATH];
	const char *emuPath = NandHandle.GetPath();
	const char *SaveGameID = m.m_saveExtGameId.c_str();	
	int flashID = SaveGameID[0] << 24 | SaveGameID[1] << 16 | SaveGameID[2] << 8 | SaveGameID[3];
	
	/* We know it exists on emunand just need to figure out which folder */
	if(fsop_FolderExist(fmt("%s/title/00010000/%08x", emuPath, flashID)))
	{
		snprintf(source, sizeof(source), "%s/title/00010000/%08x", emuPath, flashID);
		snprintf(dest, sizeof(dest), "/title/00010000/%08x", flashID);
	}
	else // if(fsop_FolderExist(fmt("%s/title/00010004/%08x", emuPath, flashID)))
	{
		snprintf(source, sizeof(source), "%s/title/00010004/%08x", emuPath, flashID);
		snprintf(dest, sizeof(dest), "/title/00010004/%08x", flashID);
	}
	NandHandle.ResetCounters();
	m.m_nandexentry = 1;
	m.m_dumpsize = NandHandle.CalcFlashSize(source, _ShowProgress, obj);
	m_nandext = true;
	NandHandle.FlashToNAND(source, dest, _ShowProgress, obj);

	LWP_MutexLock(m.m_mutex);
	m_btnMgr.hide(m.m_wbfsPBar);
	m_btnMgr.hide(m.m_wbfsLblMessage);
	m._setDumpMsg(L"...", 1.f, 1.f);
	LWP_MutexUnlock(m.m_mutex);
	m.m_thrdWorking = false;
	return 0;
}

void * CMenu::_NandDumper(void *obj)
{
	CMenu &m = *(CMenu *)obj;

	m_nandext = false;
	m_sgdump = false;
	m.m_dumpsize = 0;
	m.m_filesdone = 0;
	m.m_foldersdone = 0;

	NandHandle.ResetCounters();
	
	const char *emuPath = NandHandle.GetPath();
	NandHandle.CreatePath("%s", emuPath);
	
	LWP_MutexLock(m.m_mutex);
	m._setDumpMsg(m._t("cfgne27", L"Calculating space needed for extraction..."), 0.f, 0.f);
	LWP_MutexUnlock(m.m_mutex);

	/* Full nand dump */
	if(m_nanddump) 
	{
		m.m_dumpsize = NandHandle.CalcDumpSpace("/", CMenu::_ShowProgress, obj);
		m_nandext = true;
		NandHandle.DoNandDump("/", emuPath, CMenu::_ShowProgress, obj);
	}
	/* Gamesave(s) dump */
	else 
	{
		vector<string> saveList;
		m_sgdump = true;
		if(m.m_saveExtGameId.empty()) // if not a specified gamesave from game config menu or launching wii game
		{
			/* Extract all or missing gamesaves - main emunand settings menu */
			LWP_MutexLock(m.m_mutex);
			m._setDumpMsg(m._t("cfgne18", L"Listing game saves to extract..."), 0.f, 0.f);
			LWP_MutexUnlock(m.m_mutex);
			m.m_nandexentry = 0;
			saveList.reserve(m.m_gameList.size());
			for(u32 i = 0; i < m.m_gameList.size() && !m.m_thrdStop; ++i)
			{
				if(m.m_gameList[i].type == TYPE_WII_GAME)
				{
					string id((const char *)m.m_gameList[i].id, 4);
					if(m_saveall || !m._checkSave(id, SAVES_NAND)) // if all or the gamesave is not already on saves emunand
					{
						if(m._checkSave(id, REAL_NAND)) // if save on real nand
						{
							gprintf(fmt("Game %s is added to emunand.\n", id.c_str()));
							m.m_nandexentry++;
							saveList.push_back(id);
						}
					}
				}
			}
		}
		/* One gamesave extract from game config menu or launching wii game */
		else 
		{
			m.m_nandexentry = 1;
			saveList.push_back(m.m_saveExtGameId);
		}

		/* For loop to calculate SD or HDD space NandHandle will need for the actual savegame(s) dump */
		for(u32 i = 0; i < saveList.size() && !m.m_thrdStop; ++i)
		{
			char source[ISFS_MAXPATH];
			int savePath = saveList[i].c_str()[0] << 24 | saveList[i].c_str()[1] << 16 | saveList[i].c_str()[2] << 8 | saveList[i].c_str()[3];
			snprintf(source, sizeof(source), "/title/00010000/%08x", savePath);
			if(!m._checkSave(saveList[i], REAL_NAND))
				snprintf(source, sizeof(source), "/title/00010004/%08x", savePath);

			m.m_dumpsize = NandHandle.CalcDumpSpace(source, CMenu::_ShowProgress, obj);	
		}
		
		/* For loop to do the actual savegame(s) dump */
		for(u32 i = 0; i < saveList.size() && !m.m_thrdStop; ++i)
		{
			char source[ISFS_MAXPATH];
			int savePath = saveList[i].c_str()[0] << 24 | saveList[i].c_str()[1] << 16 | saveList[i].c_str()[2] << 8 | saveList[i].c_str()[3];
			snprintf(source, sizeof(source), "/title/00010000/%08x", savePath);
			if(!m._checkSave(saveList[i], REAL_NAND))
				snprintf(source, sizeof(source), "/title/00010004/%08x", savePath);

			m_nandext = true;
			NandHandle.DoNandDump(source, emuPath, CMenu::_ShowProgress, obj);
		}
	}

	LWP_MutexLock(m.m_mutex);
	m_btnMgr.hide(m.m_wbfsPBar);
	m_btnMgr.hide(m.m_wbfsLblMessage);
	m._setDumpMsg(L"...", 1.f, 1.f);
	LWP_MutexUnlock(m.m_mutex);
	m.m_thrdWorking = false;
	return 0;
}

/********************************************************************************************************/

bool CMenu::_launchNeek2oChannel(int ExitTo, int nand_type)
{
	if(isWiiVC)
		return false;

	int emupart = _FindEmuPart(nand_type, true);

	/* if exit to neek2o SM, make sure there's not too many channels to launch
	otherwise it would take ages to boot and even fail */
	if(ExitTo == EXIT_TO_SMNK2O)
	{
		if(emupart != SD && emupart != USB1) // required
		{
			error(_t("errneek4", L"Emunand must be on SD or USB1!"));
			return false;
		}
		
		m_prev_view = m_current_view;
		channelsType = m_cfg.getInt(channel_domain, "channels_type", CHANNELS_REAL);
		int prev_emupart = m_cfg.getInt(channel_domain, "partition", SD);

		//! _loadList() gets channels_type in config, we can't use NANDemuView
		if(channelsType != CHANNELS_EMU)
			m_cfg.setInt(channel_domain, "channels_type", CHANNELS_EMU);
		if(m_current_view != COVERFLOW_CHANNEL) // it might be COVERFLOW_PLUGIN or COVERFLOW_WII
		{
			m_current_view = COVERFLOW_CHANNEL;
			if(nand_type == SAVES_NAND)
			{
				m_cfg.setInt(channel_domain, "partition", emupart);
				m_cfg.setString(channel_domain, "current_emunand", savesNands[curSavesNand]);
			}
		}
		
		_loadList(); // to get actual emunand m_gameList.size()
		
		if(channelsType != CHANNELS_EMU)
			m_cfg.setInt(channel_domain, "channels_type", channelsType);
		if(m_prev_view != COVERFLOW_CHANNEL)
		{
			m_current_view = m_prev_view;
			if(nand_type == SAVES_NAND)
			{
				m_cfg.setInt(channel_domain, "partition", prev_emupart);
				m_cfg.setString(channel_domain, "current_emunand", emuNands[curEmuNand]);
			}
		}
		m_refreshGameList = true;
		
		if(m_gameList.size() > 48) // max number of channels displayed in system menu
		{
			if(!error(wfmt(_fmt("errneek6", L"Neek2o may fail to launch %i channels, try anyway?"), m_gameList.size()), true))
			{
				if(nand_type == SAVES_NAND)
					_loadList();
				return false;
			}
		}
	}
	/* if exit to neek2o WF channel, make sure the channel exists */
	else if(ExitTo == EXIT_TO_WFNK2O)
	{
		if(emupart != USB1)
		{
			error(_t("errneek3", L"Emunand must be on USB1!"));
			return false;
		}
		else
		{
			bool curNANDemuView = NANDemuView;
			NANDemuView = true;
			u32 size = 0;
#ifdef APP_WIIFLOW_LITE
			if(!NandHandle.GetTMD(TITLE_ID(0x00010001, 0x57464C41), &size)) // WFLA
#else
			if(!NandHandle.GetTMD(TITLE_ID(0x00010001, 0x44574641), &size)) // DWFA
#endif
			{
				error(_t("errneek2", L"Channel not found on emunand!"));
				NANDemuView = curNANDemuView;
				return false;
			}
		}
	}
	else
		return false;

	/* make sure neek2o kernel exists */
	if(!Load_Neek2o_Kernel(emupart))
	{
		error(_t("errneek1", L"Neek2o kernel not found!"));
		return false;
	}
	else
	{
		//! build nandcfg.bin twice if needed
		for(u8 i = 0; i < 2; ++i)
			if(neek2oSetNAND(NandHandle.Get_NandPath(), emupart) > -1)
				break;

		//! finally boot it
		Sys_SetNeekPath(NandHandle.Get_NandPath());
		exitHandler(ExitTo);
		return true;
	}
}
