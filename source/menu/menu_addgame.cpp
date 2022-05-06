
#include "menu.hpp"
#include "lockMutex.hpp"
#include "loader/wdvd.h"
#include "loader/alt_ios.h"

static bool game_installed = false;

void CMenu::_showAddGame(void)
{
	m_btnMgr.setText(m_configLblTitle, _t("wbfsop1", L"Install game"));
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.setText(m_configBtnCenter, _t("cfgne6", L"Start"));
	m_btnMgr.show(m_configBtnCenter);
	m_btnMgr.show(m_configLblDialog);
	m_btnMgr.show(m_configBtnBack);
	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);
}

void CMenu::_setThrdMsg(const wstringEx &msg, float progress)
{
	if (m_thrdStop) return;
	if (msg != L"...") m_thrdMessage = msg;
	m_thrdMessageAdded = true;
	m_thrdProgress = progress;
}
/*
// Same as wiiLightOn() and wiiLightOff() in Gekko.c
static void slotLight(bool state)
{
	if(state)
		*(u32 *)0xCD0000C0 |= 0x20;
	else
		*(u32 *)0xCD0000C0 &= ~0x20;
}
*/
void CMenu::_addDiscProgress(int status, int total, void *user_data)
{
	CMenu &m = *(CMenu *)user_data;
	m.m_progress = total == 0 ? 0.f : (float)status / (float)total;
	//! Don't synchronize too often
	if(m.m_progress - m.m_thrdProgress >= 0.01f)
	{
		LWP_MutexLock(m.m_mutex);
		m._setThrdMsg(L"...", m.m_progress);
		LWP_MutexUnlock(m.m_mutex);
	}
}

bool CMenu::_searchGamesByID(const char *gameId)
{
	for(vector<dir_discHdr>::iterator itr = m_gameList.begin(); itr != m_gameList.end(); ++itr)
	{
		if(itr->type == TYPE_WII_GAME || itr->type == TYPE_GC_GAME)
			if(strncmp(itr->id, gameId, 6) == 0)
				return true;
	}
	return false;
}

void CMenu::GC_Messenger(int message, int info, char *cinfo)
{
	switch(message)
	{
		case 1:
			m_thrdMessage = wfmt(_fmt("wbfsop23", L"Calculating space needed for %s... Please insert disc %d to continue."), cinfo, info);
			break;
		case 2:
			m_thrdMessage = wfmt(_fmt("wbfsop15", L"Calculating space needed for %s"), cinfo);
			break;
		case 3:
			m_thrdMessage = wfmt(_fmt("wbfsop16", L"Installing %s"), cinfo);
			break;
		case 4:
			m_thrdMessage = wfmt(_fmt("wbfsop17", L"Installing %s disc %d/2"), cinfo, info);
			break;
		case 5:
		case 6:
		case 7:
		case 8:
			m_thrdMessage = _t("wbfsop99", L"Wrong disc!");
			break;
		case 9:
			m_thrdMessage = wfmt(_fmt("wbfsop22", L"Installing %s... Please insert disc 2 to continue."), cinfo);
			break;
		case 10:
			m_thrdMessage = _t("wbfsop25", L"Disc read error!! Please clean the disc.");
			break;
		case 11:
			m_thrdMessage = _t("wbfsop26", L"Disc ejected!! Please insert disc again.");
			break;
	}
	m_thrdMessageAdded = true;
}

/** WII DVD INSTALLER **/
void * CMenu::_gameInstaller(void *obj)
{
	CMenu &m = *(CMenu *)obj;
	int ret;

	DeviceHandle.OpenWBFS(currentPartition);
	if(!WBFS_Mounted())
	{
		m.m_thrdWorking = false;
		return NULL;
	}
	u64 comp_size = 0, real_size = 0;
	f32 free, used;
	WBFS_DiskSpace(&used, &free);
	WBFS_DVD_Size(&comp_size, &real_size);
	if((f32)comp_size + (f32)128*1024 >= free * GB_SIZE)
	{
		LWP_MutexLock(m.m_mutex);
		m._setThrdMsg(m._t("wbfsop10", L"Not enough space."), 0.f);
		LWP_MutexUnlock(m.m_mutex);
	}
	else
	{	
		LWP_MutexLock(m.m_mutex);
		m._setThrdMsg(L"", 0);
		LWP_MutexUnlock(m.m_mutex);
	
		ret = WBFS_AddGame(_addDiscProgress, obj);

		LWP_MutexLock(m.m_mutex);
		if(ret == 0)
		{
			m._setThrdMsg(m._t("wbfsop27", L"Game installed."), 1.f);
			game_installed = true;
		}
		else
			m._setThrdMsg(m._t("wbfsop9", L"An error has occurred."), 1.f);
		LWP_MutexUnlock(m.m_mutex);
	}
	WBFS_Close();
	m.m_thrdWorking = false;
	return NULL;
}

/** GAMECUBE DVD INSTALLER **/
int CMenu::_GCgameInstaller()
{
	GCDump m_gcdump;

	bool skip = m_cfg.getBool(gc_domain, "skip_on_error", false);
	bool comp = m_cfg.getBool(gc_domain, "compressed_dump", false);
	bool wexf = m_cfg.getBool(gc_domain, "write_ex_files", false);
	bool alig = m_cfg.getBool(gc_domain, "force_32k_align_files", false);
	u32 nretry = m_cfg.getUInt(gc_domain, "num_retries", 5);
	
	u32 rsize = 1048576; // 1MB
	if(skip)
		rsize = 8192; // use small chunks when skip on error is enabled

	m_gcdump.Init(skip, comp, wexf, alig, nretry, rsize, DeviceName[currentPartition], gc_games_dir);
	
	int ret;
	m_progress = 0.f;

	if(!DeviceHandle.IsInserted(currentPartition))
	{
		m_thrdWorking = false;
		return -1;
	}

	char partition[6];
	strcpy(partition, fmt("%s:/", DeviceName[currentPartition]));

	u32 needed = 0;

	ret = m_gcdump.CheckSpace(&needed, comp);
	if(ret != 0)
	{
		_setThrdMsg(_t("wbfsop9", L"An error has occurred."), 1.f);
		m_thrdWorking = false;
		return ret;
	}

	if(m_gcdump.GetFreeSpace(partition, BL) <= needed)
	{
		// gprintf("Space available: %d Mb (%d blocks)\n", m_gcdump.GetFreeSpace(partition, MB), m_gcdump.GetFreeSpace(partition, BL));
		_setThrdMsg(_t("wbfsop10", L"Not enough space."), 0.f);
		ret = -1;
	}
	else
	{
		// gprintf("Free space available: %d Mb (%d blocks)\n", m_gcdump.GetFreeSpace(partition, MB), m_gcdump.GetFreeSpace(partition, BL));
		_setThrdMsg(L"", 0);

		ret = m_gcdump.DumpGame();

		if(ret == 0)
			_setThrdMsg(_t("wbfsop27", L"Game installed."), 1.f);
		else if(ret >= 0x30200)
			_setThrdMsg(wfmt(_fmt("wbfsop12", L"DVDError(%d)."), ret), 1.f);
		else if(ret > 0)
			_setThrdMsg(wfmt(_fmt("wbfsop13", L"Game installed, but disc contains errors (%d)."), ret), 1.f);
		else
			_setThrdMsg(_t("wbfsop9", L"An error has occurred."), 1.f);

		if(ret >= 0)
			game_installed = true;
	}
	m_thrdWorking = false;
	return ret;
}

void CMenu::_addGame(u8 game_type)
{
	lwp_t thread = 0;
	char GameID[7];
	GameID[6] = '\0';
	bool rb = false;
	bool compressed_dump = m_cfg.getOptBool(gc_domain, "compressed_dump", 0);

	SetupInput();
	_showAddGame();
	
	if(game_type == TYPE_GC_GAME)
	{
		m_btnMgr.setText(m_configLbl[7], _t("cfgg96", L"Install compressed dump"));
		m_btnMgr.show(m_configLbl[7]);
		m_checkboxBtn[7] = compressed_dump ? m_configChkOn[7] : m_configChkOff[7];
		m_btnMgr.show(m_checkboxBtn[7]);
	}
	m_btnMgr.setText(m_configLblDialog, _t("wbfsadddlg", L"Insert disc to dump and click Start"));
	
	m_thrdStop = false;
	m_thrdMessageAdded = false;
	
	while(!m_exit)
	{
		_mainLoopCommon();
		if((BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnBack))) && !m_thrdWorking)
			break;
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
			m_btnMgr.up();
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
			m_btnMgr.down();
		else if(BTN_A_OR_2_PRESSED && !m_thrdWorking)
		{
			if(m_btnMgr.selected(m_configChkOff[7]) || m_btnMgr.selected(m_configChkOn[7])) // compressed dump
			{
				m_btnMgr.hide(m_checkboxBtn[7], true);
				compressed_dump = !compressed_dump;
				m_cfg.setBool(gc_domain, "compressed_dump", compressed_dump);
				m_checkboxBtn[7] = compressed_dump ? m_configChkOn[7] : m_configChkOff[7];
				m_btnMgr.show(m_checkboxBtn[7], true);
			}
			else if(m_btnMgr.selected(m_configBtnCenter)) // start
			{
				m_vid.waitMessage(2.5f);
				//! LOAD CIOS IF NEEDED AND READ DVD
				/**********************************************/
				/**/
				if(!useMainIOS)
				{
					MusicPlayer.Stop();
					rb = true; // reboot when done even if game already installed
					loadIOS(mainIOS, true); // switch to cIOS
					Sys_Init();
					Open_Inputs();
					for(int chan = WPAD_MAX_WIIMOTES-1; chan >= 0; chan--)
						WPAD_SetVRes(chan, m_vid.width() + m_cursor[chan].width(), m_vid.height() + m_cursor[chan].height());
					_netInit();
				}
				/**/
				_hideConfig(true);
				_showProgress();

				if(Disc_Wait() < 0)
				{
					error(_t("wbfsoperr1", L"Disc wait failed!"));
					break;
				}
				if(Disc_Open(false) < 0)
				{
					WDVD_Eject();
					error(_t("wbfsoperr2", L"Reading disc failed!"));
					break;
				}
				else
					_hideWaitMessage();
				
				//! WII DVD INSTALLER
				/**********************************************/
				if(Disc_IsWii() == 0)
				{
					Disc_ReadHeader(&wii_hdr);
					memcpy(GameID, wii_hdr.id, 6);
					m_prev_view = m_current_view;
					m_current_view = COVERFLOW_WII;
					game_type = TYPE_WII_GAME;
					_loadList();
					if(_searchGamesByID(GameID))
					{
						error(_t("wbfsoperr4", L"Game already installed!"));
						break;
					}
					CoverFlow.clear();
					m_btnMgr.setText(m_configLblDialog, wfmt(_fmt("wbfsop6", L"Installing %s [%s]..."), wii_hdr.title, GameID));
					currentPartition = m_cfg.getInt(wii_domain, "partition", 0);
					m_thrdWorking = true;
					m_thrdProgress = 0.f;
					m_thrdMessageAdded = false;
					LWP_CreateThread(&thread, _gameInstaller, this, 0, 8 * 1024, 64);
				}
				
				//! GAMECUBE DVD INSTALLER
				/**********************************************/
				else if(Disc_IsGC() == 0)
				{
					Disc_ReadGCHeader(&gc_hdr);
					memcpy(GameID, gc_hdr.id, 6);
					m_prev_view = m_current_view;
					m_current_view = COVERFLOW_GAMECUBE;
					game_type = TYPE_GC_GAME;
					_loadList();
					if(_searchGamesByID(GameID))
					{
						error(_t("wbfsoperr4", L"Game already installed!"));
						break;
					}
					CoverFlow.clear();
					currentPartition = m_cfg.getInt(gc_domain, "partition", 0);
					m_thrdWorking = true;
					m_thrdProgress = 0.f;
					_start_pThread();
					m_thrdMessage = wfmt(_fmt("wbfsop6", L"Installing %s [%s]..."), gc_hdr.title, GameID);
					m_thrdMessageAdded = true;
					_GCgameInstaller();
					if(game_installed)
						m_cfg.setBool(gc_domain, "update_cache", true);
					_stop_pThread();
					WDVD_Eject();
					m_btnMgr.show(m_configBtnBack);
				}
				
				else // should not happen
					break;
			}
		}
		if(m_thrdMessageAdded) // Wii DVD installer only
		{
			LockMutex lock(m_mutex);
			m_thrdMessageAdded = false;
			if(!m_thrdMessage.empty())
				m_btnMgr.setText(m_configLblDialog, m_thrdMessage);
			m_btnMgr.setProgress(m_wbfsPBar, m_thrdProgress);
			m_btnMgr.setText(m_wbfsLblMessage, wfmt(L"%i%%", (int)(m_thrdProgress * 100.f)));
			if(!m_thrdWorking)
			{
				if(game_installed)
					m_cfg.setBool(wii_domain, "update_cache", true);
				WDVD_Eject();
				m_btnMgr.show(m_configBtnBack);
			}
		}
	}
	_hideConfigFull(true);

	if(m_prev_view != m_current_view)
	{
		m_cfg.remove(sourceflow_domain, "numbers");
		m_cfg.remove(sourceflow_domain, "tiers");
		m_cfg.setUInt(general_domain, "sources", m_current_view);
	}
	m_cfg.setString(game_type == TYPE_WII_GAME ? wii_domain : gc_domain, "current_item", GameID);

	if(rb || game_installed) // safety reboot due to possible memory leak
	{
		error(_t("wbfsop98", L"Refreshing game list and rebooting"));
		_loadList(); // in case preffered_partition at boot is different from install partition
		m_exit = true;
		m_reload = true;
	}
	else
	{
		_showCF(true);
		_hideMain(true);
	}
}
