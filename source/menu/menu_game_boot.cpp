
#include <fcntl.h>
#include <ogc/lwp_watchdog.h>
// #include <ogc/machine/processor.h>

#include "menu.hpp"
#include "booter/external_booter.hpp"
#include "channel/channel_launcher.h"
#include "channel/nand.hpp"
#include "channel/identify.h"
#include "gc/gc.hpp"
#include "homebrew/homebrew.h"
#include "loader/alt_ios.h"
#include "loader/playlog.h"
#include "loader/wip.h"
#include "loader/frag.h"
#include "loader/fst.h"
#include "loader/cios.h"
#include "loader/nk.h"
#include "memory/memory.h"
#include "network/gcard.h"
#include "loader/wdvd.h"
#include "network/FTP_Dir.hpp"
// #include "types.h"
// #include "channel/channels.h"
// #include "devicemounter/DeviceHandler.hpp"
// #include "devicemounter/sdhc.h"
// #include "devicemounter/usbstorage.h"
// #include "loader/wbfs.h"

#ifdef APP_WIIFLOW_LITE
#define WFID4 "WFLA"
#else
#define WFID4 "DWFA"
#endif

static void setLanguage(int l)
{
	if (l > 0 && l <= 10)
		configbytes[0] = l - 1;
	else
		configbytes[0] = 0xCD;
}

static int GetLanguage(const char *lang)
{
	if(strncmp(lang, "JP", 2) == 0) return CONF_LANG_JAPANESE;
	else if(strncmp(lang, "EN", 2) == 0) return CONF_LANG_ENGLISH;
	else if(strncmp(lang, "DE", 2) == 0) return CONF_LANG_GERMAN;
	else if(strncmp(lang, "FR", 2) == 0) return CONF_LANG_FRENCH;
	else if(strncmp(lang, "ES", 2) == 0) return CONF_LANG_SPANISH;
	else if(strncmp(lang, "IT", 2) == 0) return CONF_LANG_ITALIAN;
	else if(strncmp(lang, "NL", 2) == 0) return CONF_LANG_DUTCH;
	else if(strncmp(lang, "ZHTW", 4) == 0) return CONF_LANG_TRAD_CHINESE;
	else if(strncmp(lang, "ZH", 2) == 0) return CONF_LANG_SIMP_CHINESE;
	else if(strncmp(lang, "KO", 2) == 0) return CONF_LANG_KOREAN;
	
	return CONF_LANG_ENGLISH; // Default to EN
}

static u8 GetRequestedGameIOS(dir_discHdr *hdr)
{
	u8 IOS = 0;

	DeviceHandle.OpenWBFS(currentPartition);
	wbfs_disc_t *disc = WBFS_OpenDisc((u8*)&hdr->id, hdr->path);
	if(disc != NULL)
	{
		void *titleTMD = NULL;
		u32 tmd_size = wbfs_extract_file(disc, (char*)"TMD", &titleTMD);
		if(titleTMD != NULL)
		{
			if(tmd_size > 0x18B)
				IOS = *((u8*)titleTMD + 0x18B);
			MEM2_free(titleTMD);
		}
		WBFS_CloseDisc(disc);
	}
	WBFS_Close();
	return IOS;
}

/* Used to load gameconfig.txt and cheats .gct */
bool CMenu::_loadFile(u8 * &buffer, u32 &size, const char *path, const char *file)
{
	u32 fileSize = 0;
	u8 *fileBuf = fsop_ReadFile(file == NULL ? path : fmt("%s/%s", path, file), &fileSize);
	if(fileBuf == NULL)
		return false;

	if(buffer != NULL)
		MEM2_free(buffer);
	buffer = fileBuf;
	size = fileSize;
	return true;
}

/* Direct launch from boot arg for wii game only */
void CMenu::directlaunch(const char *GameID)
{
	if(neek2o()) //
		return;
	m_directLaunch = true;
	currentPartition = m_cfg.getInt(wii_domain, "partition");
	if(DeviceHandle.IsInserted(currentPartition))
	{
		DeviceHandle.OpenWBFS(currentPartition);
		string gameDir(fmt(wii_games_dir, DeviceName[currentPartition]));
		string cacheDir(fmt("%s/%s_wii.db", m_listCacheDir.c_str(), DeviceName[currentPartition]));
		m_cacheList.CreateList(COVERFLOW_WII, currentPartition, gameDir,
				stringToVector(".wbfs|.iso", '|'), cacheDir, false);
		WBFS_Close();
		for(u32 i = 0; i < m_cacheList.size(); i++)
		{
			if(strncasecmp(GameID, m_cacheList[i].id, 6) == 0)
			{
				_launchWii(&m_cacheList[i], false); // launch will exit wiiflow
				break;
			}
		}
	}
	_error(wfmt(_fmt("errgame1", L"Launching game %s failed!"), GameID));
}

void CMenu::_launchShutdown()
{
	if(m_ftp_inited)
	{
		m_init_ftp = false;
		ftp_endThread();
	}
	exitHandler(PRIILOADER_DEF); // making wiiflow ready to boot something
	cleanup();
}

void CMenu::_launch(const dir_discHdr *hdr)
{
	dir_discHdr launchHdr;
	memcpy(&launchHdr, hdr, sizeof(dir_discHdr));
	
	MusicPlayer.Stop();
	// m_gcfg2.load(fmt("%s/" GAME_SETTINGS2_FILENAME, m_settingsDir.c_str()));

	/* Change to current game's partition */
	string domain;
	switch(launchHdr.type)
	{
		case TYPE_CHANNEL:
		case TYPE_EMUCHANNEL:
			domain = channel_domain;
			break;
		case TYPE_HOMEBREW:
			domain = homebrew_domain;
			break;
		case TYPE_GC_GAME:
			domain = gc_domain;
			break;
		case TYPE_WII_GAME:
			domain = wii_domain;
			break;
		default:
			domain = plugin_domain;
			break;
	}
	currentPartition = m_cfg.getInt(domain, "partition", 1);
	if(launchHdr.type == TYPE_PLUGIN)
	{
		int romsPartition = m_plugin.GetRomPartition(m_plugin.GetPluginPosition(launchHdr.settings[0]));
		if(romsPartition >= 0)
			currentPartition = romsPartition;
	}
	
	/* Get banner title for playlog */
	if(m_cfg.getBool(general_domain, "playlog_update", false) && !isWiiVC)
	{
		if(launchHdr.type == TYPE_WII_GAME || launchHdr.type == TYPE_CHANNEL || launchHdr.type == TYPE_EMUCHANNEL)
		{
			NANDemuView = launchHdr.type == TYPE_EMUCHANNEL;
			CurrentBanner.ClearBanner();
			if(launchHdr.type == TYPE_CHANNEL || launchHdr.type == TYPE_EMUCHANNEL)
			{
				u64 chantitle = CoverFlow.getChanTitle();
				ChannelHandle.GetBanner(chantitle);
			}
			else if(launchHdr.type == TYPE_WII_GAME)
				_extractBnr(&launchHdr);
			u8 banner_title[84];
			memset(banner_title, 0, 84);
			if(CurrentBanner.IsValid())
				CurrentBanner.GetName(banner_title, GetLanguage(m_loc.getString(m_curLanguage, "gametdb_code", "EN").c_str()));
			if(Playlog_Update(launchHdr.id, banner_title) < 0)
				Playlog_Delete();
			CurrentBanner.ClearBanner();
		}
	}
	
	/* Finally boot it */
	gprintf("Launching game %s\n", launchHdr.id);

	/* Let's boot that shit */
	if(launchHdr.type == TYPE_WII_GAME)
		_launchWii(&launchHdr, false);
	else if(launchHdr.type == TYPE_GC_GAME)
		_launchGC(&launchHdr, false);
	else if(launchHdr.type == TYPE_CHANNEL || launchHdr.type == TYPE_EMUCHANNEL)
		_launchChannel(&launchHdr);
	else if(launchHdr.type == TYPE_PLUGIN)
		_launchPlugin(&launchHdr);
	else if(launchHdr.type == TYPE_HOMEBREW)
	{
		const char *bootpath = fmt("%s/boot.dol", launchHdr.path);
		if(!fsop_FileExist(bootpath))
			bootpath = fmt("%s/boot.elf", launchHdr.path);
		if(fsop_FileExist(bootpath))
		{
			m_cfg.setString(homebrew_domain, "current_item", strrchr(launchHdr.path, '/') + 1);
			vector<string> arguments = _getMetaXML(bootpath);
			// gprintf("launching homebrew app\n");
			_launchHomebrew(bootpath, arguments);
		}
	}
}

/******************************************** PLUGIN ****************************************************/

void CMenu::_launchPlugin(dir_discHdr *hdr)
{
	/* Prepare plugin ID to update Playcount and Lastplayed before path is modified */
	string romFileName(hdr->path);
	romFileName = romFileName.substr(romFileName.find_last_of("/") + 1);
	romFileName = romFileName.substr(0, romFileName.find_last_of("."));
	const char *id = fmt("%08x/%.63s", hdr->settings[0], romFileName.c_str());
	
	/* Get dol name and name length for music plugin */
	const char *plugin_dol_name = m_plugin.GetDolName(hdr->settings[0]);
	u8 plugin_dol_len = strlen(plugin_dol_name);
	
	/* Check if music player plugin, if so set wiiflow's bckgrnd music player to play this song */
	if(plugin_dol_len == 5 && strcasecmp(plugin_dol_name, "music") == 0)
	{
		if(strstr(hdr->path, ".pls") == NULL && strstr(hdr->path, ".m3u") == NULL)
			MusicPlayer.LoadFile(hdr->path, false);
		else
		{
			// m_music_info = m_cfg.getBool(general_domain, "display_music_info", false);
			MusicPlayer.InitPlaylist(m_cfg, hdr->path, currentPartition); // maybe error msg if trouble loading playlist
		}
		return;
	}
	
	/* Get title from hdr */
	u32 title_len_no_ext = 0;
	const char *title = CoverFlow.getFilenameId(hdr); // with extension
	
	/* Get path from hdr
	example rom path - dev:/roms/super mario bros.zip
	example scummvm path - kq1-coco3 */
	const char *path = NULL;
	if(strchr(hdr->path, ':') != NULL) // it's a rom path
	{
		//! if there's extension get length of title without extension
		if(strchr(hdr->path, '.') != NULL)
			title_len_no_ext = strlen(title) - strlen(strrchr(title, '.'));
		//! get path
		*strrchr(hdr->path, '/') = '\0'; // cut title off end of path
		path = strchr(hdr->path, '/') + 1; // cut dev:/ off of path
	}
	else // it's a scummvm game
		path = hdr->path; // kq1-coco3

	/* Get device */
	const char *device = (currentPartition == 0 ? "sd" : (DeviceHandle.GetFSType(currentPartition) == PART_FS_NTFS ? "ntfs" : "usb")); // some versions of not64 uses "fat" instead of "usb"...
	
	/* Get loader
	the loader arg was used and added to plugin mods that fix94 made.
	it was used because postloader 4 also used the wiiflow plugins and the emus needed to know which loader to return to.
	the wiimednafen plugin mod may requires this loader arg. most others don't use it. */
	const char *loader = fmt("%s:/%s/WiiFlowLoader.dol", device, strchr(m_pluginsDir.c_str(), '/') + 1);

	/* Set arguments */
	vector<string> arguments = m_plugin.CreateArgs(device, path, title, loader, title_len_no_ext, hdr->settings[0]);
	
	/* Find plugin dol (it does not have to be in dev:/wiiflow/plugins) */
	const char *plugin_file = plugin_dol_name; // try full path
	if(strchr(plugin_file, ':') == NULL || !fsop_FileExist(plugin_file)) // if not found try wiiflow plugin folder
	{
		plugin_file = fmt("%s/%s", m_pluginsDir.c_str(), plugin_dol_name);
		if(!fsop_FileExist(plugin_file)) // not found - try device search
		{
			for(u8 i = SD; i < MAXDEVICES; ++i)
			{
				plugin_file = fmt("%s:/%s", DeviceName[i], plugin_dol_name);
				if(fsop_FileExist(plugin_file))
					break;
			}
		}
	}

	/* Update Playcount and Lastplayed */
	m_gcfg1.setInt("PLAYCOUNT", id, m_gcfg1.getInt("PLAYCOUNT", id, 0) + 1);
	m_gcfg1.setUInt("LASTPLAYED", id, time(NULL));
	
	/* Date fixes for specific plugins */
	if(hdr->settings[0] == 1414875969) // wiituka
		settime(637962048000000000); // Aug 16, 2022
	
	/* Launch plugin with args */
	// gprintf("launching plugin app\n");
	_launchHomebrew(plugin_file, arguments);
}

/******************************************** HOMEBREW **************************************************/

void CMenu::_launchHomebrew(const char *filepath, vector<string> arguments)
{
	/* Set exit handler, stop coverflow and cleanup GUI */
	
	_launchShutdown();

	m_gcfg1.save(true);
	m_gcfg2.save(true);
	m_cat.save(true);
	m_cfg.save(true);

	// Playlog_Delete();
	
	/* Load boot.dol into memory and load app_booter.bin into memory */
	bool ret = (LoadHomebrew(filepath) && LoadAppBooter());
	if(ret == false)
	{
		gprintf("app_booter.bin or emulator plugin not found!\n");
		_exitWiiflow();
	}

	AddBootArgument(filepath);
	for(u32 i = 0; i < arguments.size(); ++i)
	{
		// gprintf("app argument: %s\n", arguments[i].c_str());
		AddBootArgument(arguments[i].c_str());
	}

	loadIOS(58, false);
	ShutdownBeforeExit(); // wifi and sd gecko doesnt work anymore after
	BootHomebrew();
	Sys_Exit();
}

vector<string> CMenu::_getMetaXML(const char *bootpath)
{
	char *meta_buf = NULL;
	vector<string> args;
	char meta_path[200];
	char *p;
	char *e, *end;
	struct stat st;
	
	/* Load meta.xml */	
	p = strrchr(bootpath, '/');
	snprintf(meta_path, sizeof(meta_path), "%.*smeta.xml", p ? p-bootpath+1 : 0, bootpath);

	if(stat(meta_path, &st) != 0)
		return args;
	if(st.st_size > 64*1024)
		return args;

	meta_buf = (char *) MEM2_alloc(st.st_size + 1); // +1 so that the buf is 0 terminated
	if (!meta_buf)
		return args;
	
	memset(meta_buf, 0, st.st_size + 1);

	int fd = open(meta_path, O_RDONLY);
	if(fd < 0)
	{
		MEM2_free(meta_buf);
		meta_buf = NULL; 
		return args;
	}
	read(fd, meta_buf, st.st_size);
	close(fd);

	/* Strip comments */
	p = meta_buf;
	int len;
	while(p && *p) 
	{
		p = strstr(p, "<!--");
		if(!p) 
			break;
		e = strstr(p, "-->");
		if(!e) 
		{
			*p = 0; // terminate
			break;
		}
		e += 3;
		len = strlen(e);
		memmove(p, e, len + 1); // +1 for 0 termination
	}
	
	/* Parse meta */
	if(strstr(meta_buf, "<app") && strstr(meta_buf, "</app>") && strstr(meta_buf, "<arguments>") && strstr(meta_buf, "</arguments>"))
	{
		p = strstr(meta_buf, "<arguments>");
		end = strstr(meta_buf, "</arguments>");

		do 
		{
			p = strstr(p, "<arg>");
			if(!p) 
				break;
				
			p += 5; // strlen("<arg>");
			e = strstr(p, "</arg>");
			if (!e) 
				break;

			string arg(p, e-p);
			args.push_back(arg);
			p = e + 6;
		} 
		while(p < end);
	}

	MEM2_free(meta_buf);
	meta_buf = NULL; 
	return args;
}

/********************************************* GAMECUBE *************************************************/

void CMenu::_launchGC(dir_discHdr *hdr, bool disc)
{
	if(!m_nintendont_installed)
	{
		_error(_t("errgame11", L"GameCube loader not found!"));
		return;
	}
	
	/* Note for a disc boot hdr->id is set to the disc id before _launchGC is called */
	const char *id = hdr->id;
	
	/* Set game path */
	char path[256];
	if(disc)
		strcpy(path, "di");
	else
		strcpy(path, hdr->path);
	path[255] = '\0';

	/* Check if game has multi Discs */
	if(!disc)
	{
		bool multiDiscs = false;
		char disc2Path[256];
		strcpy(disc2Path, path);
		disc2Path[255] = '\0';
		*strrchr(disc2Path, '/') = '\0';
		strcat(disc2Path, "/disc2.iso"); // note fst extracted /boot.bin paths will not have disc2.iso
		if(!fsop_FileExist(disc2Path)) // if .iso does not exist, try .ciso
		{
			*strrchr(disc2Path, '/') = '\0';
			strcat(disc2Path, "/disc2.ciso");
			if(fsop_FileExist(disc2Path))
				multiDiscs = true;
		}
		else
			multiDiscs = true;
		if(multiDiscs)
		{
			SetupInput();
			CoverFlow.fade(1);
			m_btnMgr.setText(m_configLbl[4], _t("disc1", L"Disc 1"));
			m_btnMgr.setText(m_configBtn[4], _t("cfgne6", L"Start"));
			m_btnMgr.setText(m_configLbl[5], _t("disc2", L"Disc 2"));
			m_btnMgr.setText(m_configBtn[5], _t("cfgne6", L"Start"));
			m_btnMgr.show(m_configLbl[4]);
			m_btnMgr.show(m_configBtn[4]);
			m_btnMgr.show(m_configLbl[5]);
			m_btnMgr.show(m_configBtn[5]);
			
			for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
				if(m_configLblUser[i] != -1)
					m_btnMgr.show(m_configLblUser[i]);

			int choice = -1;
			while(!m_exit)
			{
				_mainLoopCommon(true);
				if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
					m_btnMgr.up();
				else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
					m_btnMgr.down();
				else if(BTN_A_OR_2_PRESSED)
				{
					if(m_btnMgr.selected(m_configBtn[4]))
					{
						choice = 0;
						break;
					}
					else if(m_btnMgr.selected(m_configBtn[5]))
					{
						choice = 1;
						break;
					}
				}
			}
			_hideConfig();
			if(choice < 0)
				return;
			if(choice == 1)
				snprintf(path, sizeof(path), "%s", disc2Path);
		}
	}
	
	/* Set exit handler, stop coverflow and cleanup GUI */
	_launchShutdown();
	
	/* Update playcount and lastplayed */
	m_gcfg1.setInt("PLAYCOUNT", id, m_gcfg1.getInt("PLAYCOUNT", id, 0) + 1);
	m_gcfg1.setUInt("LASTPLAYED", id, time(NULL));

	/* Gamercard */
	if(has_enabled_providers() && _initNetwork() == 0)
		add_game_to_card(id);
	
	/* All game settings */
	
	//! LANGUAGE
	u8 GClanguage = m_gcfg2.getUInt(id, "language", 0);
	GClanguage = (GClanguage == 0) ? m_cfg.getUInt(gc_domain, "game_language", 0) : GClanguage - 1;
	if(id[3] == 'E' || id[3] == 'J') // language selection only works for PAL games
		GClanguage = 1; // = english
		
	//! VIDEO SETTINGS
	u8 videoMode = m_gcfg2.getUInt(id, "video_mode", 0);
	videoMode = (videoMode == 0) ? m_cfg.getUInt(gc_domain, "video_mode", 0) : videoMode - 1;
	bool wiiu_widescreen = m_cfg.getBool(gc_domain, "wiiu_widescreen", true); // global setting only
	bool deflicker = m_gcfg2.getBool(id, "deflicker", false);
	bool patch_pal50 = m_gcfg2.getBool(id, "patch_pal50", false);
	bool widescreen = m_gcfg2.testOptBool(id, "widescreen", m_cfg.getBool(gc_domain, "widescreen", false));
	s8 vidscale = m_gcfg2.getInt(id, "nin_width", 127);
	if(vidscale == 127) // default then use global
		vidscale = m_cfg.getInt(gc_domain, "nin_width", 0);
	s8 vidoffset = m_gcfg2.getInt(id, "nin_pos", 127);
	if(vidoffset == 127) // default then use global
		vidoffset = m_cfg.getInt(gc_domain, "nin_pos", 0);
		
	//! MEMCARD EMULATION
	u8 emuMC = m_gcfg2.getUInt(id, "emu_memcard", 0);
	emuMC = (emuMC == 0) ? m_cfg.getUInt(gc_domain, "emu_memcard", 1) : emuMC - 1;
	
	//! PERIPHERAL EMULATION AND MISC
	bool cc_rumble = m_gcfg2.testOptBool(id, "cc_rumble", m_cfg.getBool(gc_domain, "cc_rumble", false));
	bool native_ctl = m_gcfg2.testOptBool(id, "native_ctl", m_cfg.getBool(gc_domain, "native_ctl", false));
	bool tri_arcade = m_gcfg2.getBool(id, "triforce_arcade", false);
	bool ipl = m_gcfg2.getBool(id, "skip_ipl", false);
	
	//! NETWORK SETTINGS
	bool bba = m_gcfg2.getBool(id, "bba_emu", false);
	u8 netprofile = 0;
	if(!IsOnWiiU())
		netprofile = m_gcfg2.getUInt(id, "net_profile", 0);
	
	//! CHEATS
	bool NIN_Debugger = (m_gcfg2.getInt(id, "debugger", 0) == 2);
	bool cheats = m_gcfg2.getBool(id, "cheat", false);
	
	//! USE SLIPPI FORK (for Super Smash Melee)
	bool use_slippi = (m_cfg.getBool(gc_domain, "use_slippi", false) && strncasecmp(hdr->id, "GAL", 3) == 0); // project slippi is a mod of nintendont to play patched version of smash bros melee
	gprintf("Use slippi: %s\n", use_slippi ? "yes" : "no");
	
	//! ACTIVITY LED
	bool activity_led = cheats && !m_gcfg2.getBool(id, "led", false);
	
	/* Configs no longer needed */
	m_gcfg1.save(true);
	m_gcfg2.save(true);
	m_cat.save(true);
	m_cfg.save(true);

	/* Check nintendont dol and app_booter.bin */
	bool ret = (Nintendont_GetLoader(use_slippi) && LoadAppBooter());
	if(ret == false)
	{
		gprintf("app_booter.bin not found! \n");
		_exitWiiflow();
	}

	/* GameID for Video mode when booting a Disc */
	memcpy((u8*)Disc_ID, id, 6);
	DCFlushRange((u8*)Disc_ID, 6); // not 32
	
	/* Set nintendont config options */
	u32 n_config = 0;
	n_config |= NIN_CFG_AUTO_BOOT;

	// if(DeviceHandle.PathToDriveType(path) != SD)
	//! If path is "di" PathToDriveType(path) will return -1 so use currentPartition instead
	if(currentPartition > SD)
		n_config |= NIN_CFG_USB;

	char CheatPath[256];
	CheatPath[0] = '\0'; // set NULL in case cheats are off
	if(cheats)
	{
		n_config |= NIN_CFG_CHEAT_PATH;
		n_config |= NIN_CFG_CHEATS;
		
		/* Generate Game Cheat path */
		//! Use "dev:/codes" folder for GCT files if game is not on the same partition as wiiflow
		if(strncasecmp(m_cheatDir.c_str(), DeviceName[currentPartition], strlen(DeviceName[currentPartition])) != 0)
		{
			fsop_MakeFolder(fmt("%s:/codes", DeviceName[currentPartition]));
			snprintf(CheatPath, sizeof(CheatPath), "%s:/codes/%s.gct", DeviceName[currentPartition], id);
			fsop_CopyFile(fmt("%s/%s.gct", m_cheatDir.c_str(), id), CheatPath, NULL, NULL);
		}
		/**
		//! Alternative: use game folder if game not on the same partition as wiiflow
		if(strncasecmp(m_cheatDir.c_str(), DeviceName[currentPartition], strlen(DeviceName[currentPartition])) != 0)
		{
			char GC_Path[256];
			strcpy(GC_Path, path);
			GC_Path[255] = '\0';
			if(strcasestr(path, "boot.bin") != NULL) // usb1:/games/title [id]/sys/boot.bin
			{
				*strrchr(GC_Path, '/') = '\0'; // erase /boot.bin
				*(strrchr(GC_Path, '/') + 1) = '\0'; // erase sys folder
			}
			else // usb1:/games/title [id]/game.iso
				*(strrchr(GC_Path, '/') + 1) = '\0'; // erase game.iso
			
			char GC_game_dir[strlen(GC_Path) + 11];
			snprintf(GC_game_dir, sizeof(GC_game_dir), "%s%s.gct", GC_Path, id);
			fsop_CopyFile(fmt("%s/%s.gct", m_cheatDir.c_str(), id), GC_game_dir, NULL, NULL);		
		}
		**/
		//! Use wiiflow cheat folder if is on same partition as game folder
		else
			snprintf(CheatPath, sizeof(CheatPath), "%s/%s.gct", m_cheatDir.c_str(), id);
	}

	if(NIN_Debugger && !IsOnWiiU()) // wii only
		n_config |= NIN_CFG_OSREPORT;
	if(native_ctl && !IsOnWiiU()) // wii only
		n_config |= NIN_CFG_NATIVE_SI;
	if(wiiu_widescreen && IsOnWiiU()) // wiiu vwii only
		n_config |= NIN_CFG_WIIU_WIDE;
	if(widescreen)
		n_config |= NIN_CFG_FORCE_WIDE;
	if(activity_led && !IsOnWiiU()) // wii only
		n_config |= NIN_CFG_LED;
	if(tri_arcade)
		n_config |= NIN_CFG_ARCADE_MODE;
	if(cc_rumble)
		n_config |= NIN_CFG_BIT_CC_RUMBLE;
	if(ipl)
		n_config |= NIN_CFG_SKIP_IPL;
	if(bba)
		n_config |= NIN_CFG_BBA_EMU;
		
	/* Set memcard options */
	if(emuMC > 0 || IsOnWiiU()) // force memcardemu for wiiu vwii
		n_config |= NIN_CFG_MEMCARDEMU;
	bool bigMemcard = false;
	if(emuMC == 2)
		bigMemcard = true;
	else if(emuMC > 2)
		n_config |= NIN_CFG_MC_MULTI;
		
	/* Set nintendont video options */
	u32 n_videomode = 0;
	
	/* If video is not already forced and patch pal50 or deflicker are on then force to game video mode */
	if(videoMode == 0 && (patch_pal50 || deflicker))
	{
		if(id[3] == 'E' || id[3] == 'J')
			videoMode = 3; // NTSC 480i
		else if(CONF_GetEuRGB60() > 0)
			videoMode = 2; // PAL 480i
		else
			videoMode = 1; // PAL 576i
	}
	
	/* Auto or forced */
	if(videoMode == 0)
		n_videomode |= NIN_VID_AUTO;
	else
		n_videomode |= NIN_VID_FORCE;

	/* patch_pal50 only works on pal50 games forced to mpal, NTSC, or progressive */
	if(patch_pal50)
		n_videomode |= NIN_VID_PATCH_PAL50;

	/* videomode must be forced to set deflicker */
	if(deflicker)
		n_videomode |= NIN_VID_FORCE_DF;
		
	/* progressive only works with NTSC - Nintendont auto forces to NTSC if progressive on */
	if(videoMode == 5)
	{
		n_config |= NIN_CFG_FORCE_PROG;
		n_videomode |= NIN_VID_PROG;
	}

	/* rmode and rmode_reg are set by Nintendont */
	switch(videoMode) // if 0 nothing is forced
	{
		case 1: // PAL50
			n_videomode |= NIN_VID_FORCE_PAL50;
			break;
		case 2: // PAL60 480i
			n_videomode |= NIN_VID_FORCE_PAL60;
			break;
		case 3: // NTSC 480i
			n_videomode |= NIN_VID_FORCE_NTSC;
			break;
		case 4: // MPAL
			n_videomode |= NIN_VID_FORCE_MPAL;
			break;
	}

	Nintendont_SetOptions(path, id, CheatPath, GClanguage, n_config, n_videomode, vidscale, vidoffset, bigMemcard, netprofile); // added bigMemcard
	loadIOS(58, false); // nintendont NEEDS ios58 and AHBPROT disabled
	ShutdownBeforeExit();
	// should be a check for error loading IOS58 and AHBPROT disabled
	BootHomebrew(); // regular dol
	
	Sys_Exit();
}

/********************************************* LOAD IOS *************************************************/

/* Used by wii and channel games to load the cIOS to use for the game */
/* Plugins, apps, and gamecube games don't use cIOS */
int CMenu::_loadGameIOS(u8 gameIOS, int userIOS, bool RealNAND_Channels)
{
	gprintf("Game requested IOS %d.\nUser selected %d\n", gameIOS, userIOS);
	
	/* This if seems to be used if wiiflow is in neek2o mode or cios 249 is a stub and wiiflow runs on ios58 */
	if(neek2o() || (RealNAND_Channels && IOS_GetType(mainIOS) == IOS_TYPE_STUB))
	{
		/* Doesn't use cIOS so we don't check userIOS */
		bool ret = loadIOS(gameIOS, false); // load game requested IOS and do not remount sd and usb 
		_netInit(); // needed after IOS change
		if(ret == false)
		{
			gprintf("Couldn't load IOS %i\n", gameIOS);
			return LOAD_IOS_FAILED;
		}
		return LOAD_IOS_SUCCEEDED;
	}	
	
	/*  If IOS is not 'default' and set to a specific cIOS then set gameIOS to that cIOS if it's installed */
	if(userIOS) // We need to find it in case the gameconfig has been manually edited or that cios deleted
	{
		bool found = false;
		for(CIOSItr itr = _installed_cios.begin(); itr != _installed_cios.end(); itr++)
		{
			if(itr->second == userIOS || itr->first == userIOS)
			{
				found = true;
				gameIOS = itr->first;
				break;
			}
		}
		if(!found)
			gameIOS = mainIOS;
	}
	else
		gameIOS = mainIOS; // mainIOS is usually 249 unless changed by boot args or on startup settings
	gprintf("Changed requested IOS to %d.\n", gameIOS);

	/* At this point gameIOS is a cIOS */
	if(gameIOS != CurrentIOS.Version)
	{
		gprintf("Reloading IOS into %d\n", gameIOS);
		bool ret = loadIOS(gameIOS, true); // load cIOS requested and then remount sd and usb devices
		_netInit(); // always seem to do netinit after changing IOS
		if(ret == false)
		{
			gprintf("Couldn't load IOS %i\n", gameIOS);
			return LOAD_IOS_FAILED;
		}
		return LOAD_IOS_SUCCEEDED;
	}
	return LOAD_IOS_NOT_NEEDED;
}

/********************************************** CHANNEL *************************************************/

void CMenu::_launchChannel(dir_discHdr *hdr)
{
	NANDemuView = hdr->type == TYPE_EMUCHANNEL;
	if(isWiiVC && NANDemuView)
		return;
	
	/* Set exit handler, stop coverflow and cleanup GUI */
	_launchShutdown();
	
	string id = string(hdr->id);

	/* Update playcount and lastplayed */
	m_gcfg1.setInt("PLAYCOUNT", id, m_gcfg1.getInt("PLAYCOUNT", id, 0) + 1); 
	m_gcfg1.setUInt("LASTPLAYED", id, time(NULL));
	
	/* All game settings */
	
	//! EMULATION SETTINGS
	bool need_miis = m_cfg.getBool(channel_domain, "real_nand_miis", false);
	bool need_config = m_cfg.getBool(channel_domain, "real_nand_config", false);
	u64 gameTitle = TITLE_ID(hdr->settings[0],hdr->settings[1]);
	bool hbc = false;
	if(gameTitle == HBC_OHBC || gameTitle == HBC_LULZ || gameTitle == HBC_108 || gameTitle == HBC_JODI || gameTitle == HBC_HAXX)
		hbc = true;
	bool WII_Launch = ((m_gcfg2.getBool(id, "custom", false) || hbc) && (!NANDemuView || neek2o())); // used for real nand channels - no patches, cheats, or cIOS settings allowed
	u8 emulate_mode = m_gcfg2.getUInt(id, "emulate_save", 0); // per game choice for nand emulation
	emulate_mode = (emulate_mode == 0) ? m_cfg.getInt(channel_domain, "emulation", 1) : emulate_mode - 1;
	
	//! LANGUAGE
	int language = m_gcfg2.getUInt(id, "language", 0);
	language = (language == 0) ? m_cfg.getUInt(channel_domain, "game_language", 0) : language - 1;
	
	//! VIDEO SETTINGS
	bool vipatch = m_gcfg2.getBool(id, "vipatch", false);
	u8 videoMode = m_gcfg2.getUInt(id, "video_mode", 0);
	videoMode = (videoMode == 0) ? m_cfg.getUInt(channel_domain, "video_mode", 0) : videoMode - 1;
	u8 patchVidMode = m_gcfg2.getUInt(id, "patch_video_modes", 0);
	s8 aspectRatio = m_gcfg2.getUInt(id, "aspect_ratio", 0) - 1; // -1,0,1
	u8 deflicker = m_gcfg2.getUInt(id, "deflicker_wii", 0);
	deflicker = (deflicker == 0) ? m_cfg.getUInt(channel_domain, "deflicker_wii", 0) : deflicker - 1;
	bool fix480p = m_gcfg2.testOptBool(id, "fix480p", m_cfg.getBool(channel_domain, "fix480p", false));
	
	//! NETWORK SETTNGS
	u8 private_server = m_gcfg2.getUInt(id, "private_server", 0);
	string server_addr = "";
	if(private_server > ARRAY_SIZE(CMenu::_privateServer) - 1u)
	{
		vector<string> custom_servers = stringToVector(m_cfg.getString("custom_servers", "servers"), '|');
		server_addr = m_cfg.getString("custom_servers", fmt("%s_url", custom_servers[private_server - ARRAY_SIZE(CMenu::_privateServer)].c_str()), "");
	}
	
	//! COUNTRY STRING PATCH
	bool countryPatch = m_gcfg2.getBool(id, "country_patch", false);
	
	//! BOOT DOL OR APPLOADER
	bool use_dol = !m_gcfg2.getBool(id, "apploader", false);
	
	//! RETURN TO PATCH
	u32 returnTo = 0;
	const char *rtrn = isWiiVC ? "" : neek2o() ? WFID4 : m_cfg.getString(channel_domain, "returnto", "").c_str();
	if(strlen(rtrn) == 4)
		returnTo = rtrn[0] << 24 | rtrn[1] << 16 | rtrn[2] << 8 | rtrn[3];
	
	//! CHEATS
	bool cheat = m_gcfg2.getBool(id, "cheat", false);
	u8 *cheatFile = NULL;
	u32 cheatSize = 0;
	if(!WII_Launch)
	{
		hooktype = (u32)m_gcfg2.getInt(id, "hooktype", 0);
		debuggerselect = m_gcfg2.getInt(id, "debugger", 0);
		if((cheat || debuggerselect == 1) && hooktype == 0)
			hooktype = 1;
		else if(!cheat && debuggerselect != 1)
			hooktype = 0;
		if(cheat)
			_loadFile(cheatFile, cheatSize, m_cheatDir.c_str(), fmt("%s.gct", id.c_str()));
		// note: no .wip or gameconfig.txt support for channels - not sure why
		if(has_enabled_providers() && _initNetwork() == 0)
			add_game_to_card(id.c_str());
	}
	
	//! CUSTOM IOS
	int userIOS = m_gcfg2.getInt(id, "ios", 0);
	u32 gameIOS = ChannelHandle.GetRequestedIOS(gameTitle);
	
	//! ACTIVITY LED
	bool use_led = cheat && cheatFile != NULL && !m_gcfg2.getBool(id, "led", false);

	/* Configs no longer needed */
	m_gcfg1.save(true);
	m_gcfg2.save(true);
	m_cat.save(true);
	m_cfg.save(true);

	/* Copy real NAND sysconf, settings.txt and RFL_DB.dat, they are replaced if they already exist */
	if(NANDemuView && !neek2o())
		NandHandle.PreNandCfg(need_miis, need_config);

	/* Launch emunand channel in neek2o mode */
	if((NANDemuView && !neek2o()) && emulate_mode == 2)
	{
		//! prepare neek kernel, then nandcfg.bin or vwiincfg.bin, thanks Cyan !
		bool neek_ready = false;
		if(Load_Neek2o_Kernel(currentPartition))
		{
			for(u8 i = 0; i < 2; ++i) // build nandcfg.bin twice if needed
			{
				if(neek2oSetNAND(NandHandle.Get_NandPath(), currentPartition) > -1)
				{
					neek_ready = true;
					break;
				}
			}
		}
		if(!neek_ready)
		{	
			gprintf("Launching neek2o failed!\n");
			_exitWiiflow();
		}
		else
		{
			const char *nkrtrn = NULL;
			nkrtrn = IsOnWiiU() ? "HCVA" : "NK2O"; // return to WiiU system channel or nSwitch channel 	
			u32 nkreturnTo = nkrtrn[0] << 24 | nkrtrn[1] << 16 | nkrtrn[2] << 8 | nkrtrn[3];
			ShutdownBeforeExit();
			
			Launch_nk(gameTitle, NandHandle.Get_NandPath(), ((u64)(IsOnWiiU() ? 0x00010002 : 0x00010001) << 32) | (nkreturnTo & 0xFFFFFFFF));
			
			while(1) usleep(500);
		}
	}
	
	/* Check ext_loader.bin and ext_booter.bin */
	if(!WII_Launch && !ExternalBooter_LoadBins())
	{
		gprintf("Missing ext_loader.bin or ext_booter.bin!\n");
		_exitWiiflow(); // Files missing
	}
	
	/* Load cIOS or game IOS in neek2o, devices unmounted and remounted if using ios58  */
	if(_loadGameIOS(gameIOS, userIOS, !NANDemuView) == LOAD_IOS_FAILED)
		_exitWiiflow(); // Loading game IOS failed

	/* If d2x cIOS patch returnto */
	if((CurrentIOS.Type == IOS_TYPE_D2X || neek2o()) && returnTo != 0)
	{
		if(D2X_PatchReturnTo(returnTo) >= 0)
			memset(&returnTo, 0, sizeof(u32)); // already patched - no need for giantpune patch in external booter
	}

	/* Enable our Emu NAND */
		// nand emulation not available with hermes cIOS
		// waninkoko cIOS rev14 started nand emulation (must be on root of device)
		// waninkoko cIOS rev18 added full nand emulation
		// rev21 and d2x cIOS added path support
	if(NANDemuView && !neek2o())
	{
		DeviceHandle.UnMountAll();
		NandHandle.Set_FullMode(emulate_mode == 1);
		if(NandHandle.Enable_Emu() < 0)
		{
			NandHandle.Disable_Emu();
			gprintf("Enabling emu failed!\n");
			_exitWiiflow();
		}
		DeviceHandle.MountAll();
	}

	if(WII_Launch)
	{
		ShutdownBeforeExit();
		WII_Initialize();
		WII_LaunchTitle(gameTitle);
	}
	else
	{
		setLanguage(language); // set configbyte[0] for external booter
		
		if(cheatFile != NULL)
		{
			ocarina_load_code(cheatFile, cheatSize); // copy to address used by external booter
			MEM2_free(cheatFile);
		}
		
		NandHandle.Patch_AHB(); // Identify() maybe uses it so keep AHBPROT disabled
		PatchIOS(true, isWiiVC); // patch cIOS for everything
		Identify(gameTitle); // identify title with E-Ticket Service (ES) module

		ExternalBooter_ChannelSetup(gameTitle, use_dol);
		WiiFlow_ExternalBooter(videoMode, vipatch, countryPatch, patchVidMode, aspectRatio, private_server, server_addr.c_str(), deflicker, returnTo, fix480p, TYPE_CHANNEL, use_led);
	}
	Sys_Exit();
}

/************************************************* WII *************************************************/

void CMenu::_launchWii(dir_discHdr *hdr, bool dvd, bool disc_cfg)
{
	if(isWiiVC)
		return;
	string id(hdr->id);
	string path(hdr->path);
	
	/* Update playcount and lastplayed */
	m_gcfg1.setInt("PLAYCOUNT", id, m_gcfg1.getInt("PLAYCOUNT", id, 0) + 1);
	m_gcfg1.setUInt("LASTPLAYED", id, time(NULL));
	
	/* Emu NAND settings */
	u8 emulate_mode = m_gcfg2.getUInt(id, "emulate_save", 0);
	emulate_mode = (emulate_mode == 0) ? m_cfg.getInt(wii_domain, "save_emulation", 0) : emulate_mode - 1;
	
	/* Launch incompatible games with neek2o DI (Tintin, Driver SF...) */
	if(emulate_mode == 3 && !dvd)
	{
		if(currentPartition != USB1)
			_error(_t("errgame98", L"Game must be on USB1!"));
		else
			_launchNeek2oChannel(EXIT_TO_SMNK2O, SAVES_NAND);
		return;
	}	
	
	int emuPart = -1;
	if(dvd)
	{
		/* Open Disc */
		m_vid.waitMessage(1.9f);
		if(Disc_Open(true) < 0)
		{
			WDVD_Eject();
			_error(_t("wbfsoperr2", L"Reading disc failed!"));
			return;
		}
		else
			_hideWaitMessage();
		/* Check disc */
		if(Disc_IsGC() == 0) // Disc is GC
		{
			/* Read GC disc header to get id */
			Disc_ReadGCHeader(&gc_hdr);
			memcpy(hdr->id, gc_hdr.id, 6);
			hdr->type = TYPE_GC_GAME;
			mbstowcs(hdr->title, gc_hdr.title, 63);
			
			/* Launching GC Game */
			if(disc_cfg && !m_locked)
				_gameSettings(hdr, dvd);
			currentPartition = m_cfg.getInt(gc_domain, "partition", 1);
			_launchGC(hdr, dvd);				
			return;
		}
		else if(Disc_IsWii() == 0) // Disc is Wii
		{
			/* Read header */
			Disc_ReadHeader(&wii_hdr);
			memcpy(hdr->id, wii_hdr.id, 6);
			id = string((const char*)wii_hdr.id, 6);
			hdr->type = TYPE_WII_GAME;
			mbstowcs(hdr->title, wii_hdr.title, 63);
			if(disc_cfg && !m_locked)
				_gameSettings(hdr, dvd);
			currentPartition = m_cfg.getInt(wii_domain, "partition", 1);
		}
		else // should not happen
			return;
	}
	else if(emulate_mode > 0)
	{
		emuPart = _FindEmuPart(SAVES_NAND, false); // make emunand folder if it doesn't exist
		/* SD can't be used to launch Wii game after enabling emu (d2x cIOS issue?) */
		if(emuPart == SD && currentPartition == SD)
		{
			_error(_t("errgame99", L"Game and gamesave can't be both on SD card!"));
			return;
		}
	}

	/* Set exit handler, stop coverflow and cleanup GUI */
	_launchShutdown();
	
	/* All game settings */
	
	//! LANGUAGE
	int language = m_gcfg2.getUInt(id, "language", 0);
	language = (language == 0) ? m_cfg.getUInt(wii_domain, "game_language", 0) : language - 1;
	
	//! VIDEO SETTINGS
	bool vipatch = m_gcfg2.getBool(id, "vipatch", false);
	u8 videoMode = m_gcfg2.getUInt(id, "video_mode", 0);
	videoMode = (videoMode == 0) ? m_cfg.getUInt(wii_domain, "video_mode", 0) : videoMode - 1;	
	u8 patchVidMode = m_gcfg2.getUInt(id, "patch_video_modes", 0);
	s8 aspectRatio = m_gcfg2.getUInt(id, "aspect_ratio", 0) - 1; // -1;0;1
	u8 deflicker = m_gcfg2.getUInt(id, "deflicker_wii", 0);
	deflicker = (deflicker == 0) ? m_cfg.getUInt(wii_domain, "deflicker_wii", 0) : deflicker - 1;	
	bool fix480p = m_gcfg2.testOptBool(id, "fix480p", m_cfg.getBool(wii_domain, "fix480p", false));
	
	//! NETWORK SETTINGS
	u8 private_server = m_gcfg2.getUInt(id, "private_server", 0);
	string server_addr = "";
	if(private_server > ARRAY_SIZE(CMenu::_privateServer) - 1u)
	{
		vector<string> custom_servers = stringToVector(m_cfg.getString("custom_servers", "servers"), '|');
		server_addr = m_cfg.getString("custom_servers", fmt("%s_url", custom_servers[private_server - ARRAY_SIZE(CMenu::_privateServer)].c_str()), "");
	}
	
	//! RETURN TO PATCH
	u32 returnTo = 0;
	const char *rtrn = m_cfg.getString(wii_domain, "returnto").c_str();	
	if(strlen(rtrn) == 4) // this if is done in case "returnto" is set to disabled in which case rtrn would point to nothing
		returnTo = rtrn[0] << 24 | rtrn[1] << 16 | rtrn[2] << 8 | rtrn[3];
		
	//! NAND SAVE EMULATION
	if(emulate_mode && !dvd)
	{
		if(emuPart < 0) // if savepartition is unusable
		{
			gprintf("EmuNAND for gamesave not found! Using real NAND.\n");
			emulate_mode = 0;
		}
		//! partition is good so now check if save exists on savesnand
		//! it does not check to see if actual tmd exist just if the folder exist
		else if(!_checkSave(id, SAVES_NAND))
			NandHandle.CreateTitleTMD(hdr); // setup emunand for wii gamesave
	}
	else
		emulate_mode = 0; // sets to off if we are using neek2o or launching a DVD game
	
	//! CHEATS
	bool cheat = m_gcfg2.getBool(id, "cheat", false);
	debuggerselect = m_gcfg2.getInt(id, "debugger", 0); // debuggerselect is defined in fst.h
	if((id == "RPWE41" || id == "RPWZ41" || id == "SPXP41") && debuggerselect == 1) // Prince of Persia: The Forgotten Sands
		debuggerselect = 0;
	hooktype = (u32)m_gcfg2.getInt(id, "hooktype", 0); // hooktype is defined in patchcode.h
	if((cheat || debuggerselect == 1) && hooktype == 0)
		hooktype = 1;
	else if(!cheat && debuggerselect != 1)
		hooktype = 0;

	u8 *cheatFile = NULL;
	u8 *gameconfig = NULL;
	u32 cheatSize = 0, gameconfigSize = 0;
	if(has_enabled_providers() && _initNetwork() == 0)
		add_game_to_card(id.c_str());

	load_wip_patches((u8 *)m_wipDir.c_str(), (u8 *) &id);

	if(cheat)
		_loadFile(cheatFile, cheatSize, m_cheatDir.c_str(), fmt("%s.gct", id.c_str()));
	_loadFile(gameconfig, gameconfigSize, m_txtCheatDir.c_str(), "gameconfig.txt");
	
	//! COUNTRY STRING AND REAL NAND REGION PATCHES
	bool countryPatch = m_gcfg2.getBool(id, "country_patch", false);
	bool patchregion = m_gcfg2.getBool(id, "tempregionrn", false);
	
	//! CUSTOM IOS
	int userIOS = m_gcfg2.getInt(id, "ios", 0);
	int gameIOS = dvd ? userIOS : GetRequestedGameIOS(hdr);
	
	//! ACTIVITY LED
	bool use_led = cheat && cheatFile != NULL && !m_gcfg2.getBool(id, "led", false);
	
	m_gcfg1.save(true);
	m_gcfg2.save(true);
	m_cat.save(true);
	m_cfg.save(true);

	/* Temp region switch of real nand (rn) for gamesave or off or DVD if tempregionrn is set true
	 Change real nand region to game ID[3] region - Is reset when you turn Wii off */
	if(emulate_mode <= 1 && patchregion)
		patchregion = NandHandle.Do_Region_Change(id, true);
	else
		patchregion = false;
	
	/* Load ext_loader.bin and ext_booter.bin */
	if(ExternalBooter_LoadBins() == false) // load external booter bin file
	{
		gprintf("Missing ext_loader.bin or ext_booter.bin!\n");
		_exitWiiflow();
	}
	
	/* Load cIOS, devices are unmounted and remounted if using ios58 */
	if(!dvd)
	{
		if(_loadGameIOS(gameIOS, userIOS) == LOAD_IOS_FAILED)
			_exitWiiflow(); // loading game IOS failed
	}
	
	if(CurrentIOS.Type == IOS_TYPE_D2X)
	{
		if(returnTo != 0 && !m_directLaunch && D2X_PatchReturnTo(returnTo) >= 0)
			memset(&returnTo, 0, sizeof(u32)); // set to null to keep external booter from setting it again if using d2x 

		/* Enable our Emu NAND */
		if(emulate_mode)
		{
			DeviceHandle.UnMountAll();
			NandHandle.Set_FullMode(emulate_mode == 2);
			if(NandHandle.Enable_Emu() < 0)
			{
				NandHandle.Disable_Emu();
				gprintf("Enabling emu after reload failed!\n");
				Sys_Exit();
			}
			DeviceHandle.MountAll();
		}
	}

	bool wbfs_partition = false;
	if(!dvd)
	{
		DeviceHandle.OpenWBFS(currentPartition);
		wbfs_partition = (DeviceHandle.GetFSType(currentPartition) == PART_FS_WBFS); // if USB device formatted to WBFS
		// if not WBFS formatted get fragmented list
		// if SD card (currentPartition == 0) set sector size to 512 (0x200)
		if(!wbfs_partition && get_frag_list((u8 *)id.c_str(), (char*)path.c_str(), currentPartition == 0 ? 0x200 : USBStorage2_GetSectorSize()) < 0)
			Sys_Exit(); // failed to get frag list
		WBFS_Close();
	}
	
	setLanguage(language);
	
	if(cheatFile != NULL)
	{
		ocarina_load_code(cheatFile, cheatSize);
		MEM2_free(cheatFile);
	}
	if(gameconfig != NULL)
	{
		app_gameconfig_load(id.c_str(), gameconfig, gameconfigSize);
		MEM2_free(gameconfig);
	}

	ExternalBooter_WiiGameSetup(wbfs_partition, dvd, patchregion, id.c_str());
	WiiFlow_ExternalBooter(videoMode, vipatch, countryPatch, patchVidMode, aspectRatio, private_server, server_addr.c_str(), deflicker, returnTo, fix480p, TYPE_WII_GAME, use_led);
	
	Sys_Exit();
}

void CMenu::_exitWiiflow()
{
	/* Exit WiiFlow, no game booted... */
	ShutdownBeforeExit(); // unmount devices and close inputs
	Sys_Exit();
}
