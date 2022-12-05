
// #include <fstream>
// #include <sys/stat.h>
// #include <dirent.h>
// #include <time.h>
// #include <wchar.h>
// #include <network.h>
// #include <errno.h>

#include "menu.hpp"
#include "fonts.h"
#include "banner/BannerWindow.hpp"
#include "channel/nand.hpp"
#include "gc/gc.hpp"
#include "hw/Gekko.h"
#include "gui/WiiMovie.hpp"
#include "loader/cios.h"
#include "loader/fs.h"
#include "loader/nk.h"
#include "loader/playlog.h"
#include "music/SoundHandler.hpp"
#include "network/gcard.h"
#include "unzip/U8Archive.h"
#include "network/proxysettings.h"
#include "network/FTP_Dir.hpp"
#include "network/ftp.h"
// #include "types.h"
// #include "channel/nand_save.hpp"
// #include "loader/alt_ios.h"
// #include "loader/wbfs.h"

/* Sounds */
extern const u8 click_wav[];
extern const u32 click_wav_size;
extern const u8 hover_wav[];
extern const u32 hover_wav_size;
#ifdef SCREENSHOT
extern const u8 camera_wav[];
extern const u32 camera_wav_size;
#endif

extern const u8 font_ttf[];
extern const u32 font_ttf_size;

CMenu mainMenu;

u8 CMenu::downloadStack[8192] ATTRIBUTE_ALIGN(32);
const u32 CMenu::downloadStackSize = 8192;

CMenu::CMenu()
{
	m_ftp_inited = false;
	m_init_ftp = false;
	m_aa = 0;
	m_thrdWorking = false;
	m_thrdStop = false;
	m_thrdProgress = 0.f;
	m_thrdStep = 0.f;
	m_thrdStepLen = 0.f;
	m_locked = false;
	m_favorites = false;
	m_mutex = 0;
	m_showtimer = 0;
	m_gameSoundThread = LWP_THREAD_NULL;
	m_soundThrdBusy = false;
	m_numCFVersions = 0;
	m_bgCrossFade = 0;
	m_bnrSndVol = 0;
	m_directLaunch = false;
	m_exit = false;
	m_reload = false;
	m_gamesound_changed = false;
	m_video_playing = false;
	m_zoom_video = false;
	m_base_font = NULL;
	m_base_font_size = 0;
	m_wbf1_font = NULL;
	m_wbf2_font = NULL;
	m_current_view = COVERFLOW_WII;
	m_prevBg = NULL;
	m_nextBg = NULL;
	m_lqBg = NULL;
	m_use_sd_logging = false;
	m_use_wifi_gecko = false;
	m_use_source = true;
	m_sourceflow = false;
	m_clearCats = false;
	m_getFavs = true;
	m_catStartPage = 1;
	curCustBg = 1;
	cacheCovers = false;
	m_snapshot_loaded = false;
	/* Explorer stuff */
	m_txt_view = false;
	m_txt_path = NULL;
	/* Download stuff */
	m_file = NULL;
	m_buffer = NULL;
	m_filesize = 0;
	/* Thread stuff */
	m_thrdPtr = LWP_THREAD_NULL;
	m_thrdInstalling = false;
	m_thrdUpdated = false;
	m_thrdDone = false;
	m_thrdWritten = 0;
	m_thrdTotal = 0;
	/* Screensaver */
	no_input_time = 0;
	/* Autoboot stuff */
	m_source_autoboot = false;
}

bool CMenu::init(bool usb_mounted)
{
	/* Clear Playlog */
	Playlog_Delete(); // erases "Wiiflow" entry in playlog but will also erase last played game
	
	/* Delete temp sysconf created by region patch (nand:/sys/wiiflow.reg) */
	NandHandle.Clear_Region_Patch(); //

	/* Find the first partition with apps/wiiflow folder */
	const char *drive = NULL;
	struct stat dummy;
	for(int i = SD; i <= USB8; i++)
	{
		if(DeviceHandle.IsInserted(i) && DeviceHandle.GetFSType(i) != PART_FS_WBFS && stat(fmt("%s:/%s", DeviceName[i], APPS_DIR), &dummy) == 0)
		{
			drive = DeviceName[i];
			break;
		}
	}
	if(drive == NULL) // Could not find apps/wiiflow so we can't go on
		return false;
	
	/* Handle apps dir first, so handling wiiflow.ini does not fail */
	m_appDir = fmt("%s:/%s", drive, APPS_DIR);
	gprintf("Wiiflow boot.dol Location: %s\n", m_appDir.c_str());

	/* Set data folder on same device as the apps/wiiflow folder */
	m_dataDir = fmt("%s:/%s", drive, APP_DATA_DIR);
	gprintf("Data Directory: %s\n", m_dataDir.c_str());
	
	/* Load/Create wiiflow.ini so we can get settings to start Gecko and Network */
	m_cfg.load(fmt("%s/" CFG_FILENAME, m_appDir.c_str()));

	/* ------------------------------------------------------*/
	/* Setup debugging stuff after loading wiiflow.ini */
	show_mem = m_cfg.getBool("DEBUG", "show_mem", false);
	
	/* Check if we want WiFi Gecko */
	m_use_wifi_gecko = m_cfg.getBool("DEBUG", "wifi_gecko", false);
	WiFiDebugger.SetBuffer(m_use_wifi_gecko);
	
	/* Check if we want SD Gecko */
	m_use_sd_logging = m_cfg.getBool("DEBUG", "sd_write_log", false);
	LogToSD_SetBuffer(m_use_sd_logging);
	/* ------------------------------------------------------*/
	
	/* Init gamer tags */
	m_cfg.setString("GAMERCARD", "gamercards", "riitag"); // changed to "riitag"
	m_cfg.getString("GAMERCARD", "riitag_url", RIITAG_URL); // 
	m_cfg.getString("GAMERCARD", "riitag_key", ""); // 
	if(m_cfg.getBool("GAMERCARD", "gamercards_enable", false))
	{
		vector<string> gamercards = stringToVector(m_cfg.getString("GAMERCARD", "gamercards"), '|');
		if(gamercards.size() == 0)
			gamercards.push_back("riitag");
		for(vector<string>::iterator itr = gamercards.begin(); itr != gamercards.end(); itr++)
		{
			gprintf("Found gamercard provider: %s\n",(*itr).c_str());
			register_card_provider(
				m_cfg.getString("GAMERCARD", fmt("%s_url", (*itr).c_str())).c_str(),
				m_cfg.getString("GAMERCARD", fmt("%s_key", (*itr).c_str())).c_str()
			);
		}
	}

	/* Check if we want FTP */
	m_init_ftp = m_cfg.getBool("FTP", "auto_start", false);
	ftp_allow_active = m_cfg.getBool("FTP", "allow_active_mode", false);
	ftp_server_port = min(65535u, m_cfg.getUInt("FTP", "server_port", 21));
	set_ftp_password(m_cfg.getString("FTP", "password", "").c_str());	
	
	/* Init Network (only if wifi gecko debug or FTP on start true), slows down boot a little */
	_netInit();
	if(neek2o()) // retry (only works on 2nd try for some reason)
		_netInit();
	
	/* Set the proxy settings */
	proxyUseSystem = m_cfg.getBool("PROXY", "proxy_use_system", true);
	memset(proxyAddress, 0, sizeof(proxyAddress));
	strncpy(proxyAddress, m_cfg.getString("PROXY", "proxy_address", "").c_str(), sizeof(proxyAddress) - 1);
	proxyPort = m_cfg.getInt("PROXY", "proxy_port", 0);
	memset(proxyUsername, 0, sizeof(proxyUsername));
	strncpy(proxyUsername, m_cfg.getString("PROXY", "proxy_username", "").c_str(), sizeof(proxyUsername) - 1);
	memset(proxyPassword, 0, sizeof(proxyPassword));
	strncpy(proxyPassword, m_cfg.getString("PROXY", "proxy_password", "").c_str(), sizeof(proxyPassword) - 1);
	getProxyInfo();

	/* Set default homebrew partition on first boot */
	m_cfg.getInt(homebrew_domain, "partition", strcmp(drive, "sd") == 0 ? 0 : 1); // drive is device where wiiflow is
		
	int part = 0;
	/* Set default wii games partition on first boot */
	part = m_cfg.getInt(wii_domain, "partition", -1);
	if(part < 0)
	{
		if(!sdOnly)
		{
			for(int i = SD; i <= USB8; i++) // find first wbfs folder or a partition of wbfs file system
			{
				if(DeviceHandle.IsInserted(i) && (DeviceHandle.GetFSType(i) == PART_FS_WBFS || stat(fmt(GAMES_DIR, DeviceName[i]), &dummy) == 0))
				{
					part = i;
					break;
				}
			}
		}
		if(part < 0) // not found 
		{
			if(DeviceHandle.IsInserted(SD)) // set to SD if inserted otherwise USB1
				part = SD;
			else
				part = USB1;
		}
		m_cfg.setInt(wii_domain, "partition", part);
	}

	/* Set default gc games partition on first boot */
	part = m_cfg.getInt(gc_domain, "partition", -1);
	if(part < 0)
	{
		if(!sdOnly)
		{
			for(int i = SD; i <= USB8; i++) // find first 'games' folder
			{
				if(stat(fmt(DF_GC_GAMES_DIR, DeviceName[i]), &dummy) == 0) // should also check for FAT...
				{
					part = i;
					break;
				}
			}
		}
		if(part < 0) // not found 
		{
			if(DeviceHandle.IsInserted(SD)) // set to SD if inserted otherwise USB1
				part = SD;
			else
				part = USB1;
		}
		m_cfg.setInt(gc_domain, "partition", part);
	}

	/* Preferred partition setting - negative 1 means not set by user so skip this */
	part = m_cfg.getInt(wii_domain, "preferred_partition", -1);
	if(part >= 0)
		m_cfg.setInt(wii_domain, "partition", (usb_mounted && part > 0) ? part : SD);

	part = m_cfg.getInt(gc_domain, "preferred_partition", -1);
	if(part >= 0)
		m_cfg.setInt(gc_domain, "partition", (usb_mounted && part > 0) ? part : SD); // allow USB > 1

	part = m_cfg.getInt(channel_domain, "preferred_partition", -1);
	if(part >= 0)
		m_cfg.setInt(channel_domain, "partition", neek2o() ? USB1 : ((usb_mounted && part > 0) ? part : SD));
	
	/* Our Wii games dir */
	memset(wii_games_dir, 0, 64);
	strncpy(wii_games_dir, m_cfg.getString(wii_domain, "wii_games_dir", GAMES_DIR).c_str(), 63);
	if(strncmp(wii_games_dir, "%s:/", 4) != 0)
		strcpy(wii_games_dir, GAMES_DIR);
	// gprintf("Wii Games Directory: %s\n", wii_games_dir);
	
	/* GameCube stuff */
	memset(gc_games_dir, 0, 64);
	strncpy(gc_games_dir, m_cfg.getString(gc_domain, "gc_games_dir", DF_GC_GAMES_DIR).c_str(), 63);
	if(strncmp(gc_games_dir, "%s:/", 4) != 0)
		strcpy(gc_games_dir, DF_GC_GAMES_DIR);
	// gprintf("GameCube Games Directory: %s\n", gc_games_dir);
	
	m_nintendont_installed = Nintendont_Installed();
	m_gc_play_banner_sound = m_cfg.getBool(gc_domain, "play_banner_sound", true);

	/* Init directories */
	m_cacheDir = m_cfg.getString(general_domain, "dir_cache", fmt("%s/cache", m_dataDir.c_str()));
	m_listCacheDir = m_cfg.getString(general_domain, "dir_list_cache", fmt("%s/lists", m_cacheDir.c_str()));
	m_bnrCacheDir = m_cfg.getString(general_domain, "dir_banner_cache", fmt("%s/banners", m_cacheDir.c_str()));
	m_customBnrDir = m_cfg.getString(general_domain, "dir_custom_banners", fmt("%s/custom_banners", m_dataDir.c_str()));
	
	m_txtCheatDir = m_cfg.getString(general_domain, "dir_txtcheat", fmt("%s/codes", m_dataDir.c_str()));
	m_cheatDir = m_cfg.getString(general_domain, "dir_cheat", fmt("%s/gct", m_txtCheatDir.c_str()));
	m_wipDir = m_cfg.getString(general_domain, "dir_wip", fmt("%s/wip", m_txtCheatDir.c_str()));
	
	m_settingsDir = m_cfg.getString(general_domain, "dir_settings", fmt("%s/settings", m_dataDir.c_str()));
	m_languagesDir = m_cfg.getString(general_domain, "dir_languages", fmt("%s/languages", m_dataDir.c_str()));
	m_helpDir = m_cfg.getString(general_domain, "dir_help", fmt("%s/help", m_dataDir.c_str()));
	m_backupDir = m_cfg.getString(general_domain, "dir_backup", fmt("%s/backups", m_dataDir.c_str())); //
#ifdef SCREENSHOT
	m_screenshotDir = m_cfg.getString(general_domain, "dir_screenshot", fmt("%s/screenshots", m_dataDir.c_str()));
#endif

	m_boxPicDir = m_cfg.getString(general_domain, "dir_box_covers", fmt("%s/boxcovers", m_dataDir.c_str()));
	m_picDir = m_cfg.getString(general_domain, "dir_flat_covers", fmt("%s/covers", m_dataDir.c_str()));
	m_themeDir = m_cfg.getString(general_domain, "dir_themes_lite", fmt("%s/themes_lite", m_dataDir.c_str()));
	m_coverflowsDir = m_cfg.getString(general_domain, "dir_coverflows", fmt("%s/coverflows", m_themeDir.c_str()));
	m_musicDir = m_cfg.getString(general_domain, "dir_music", fmt("%s/music", m_dataDir.c_str())); 
	m_videoDir = m_cfg.getString(general_domain, "dir_trailers", fmt("%s/trailers", m_dataDir.c_str()));
	m_fanartDir = m_cfg.getString(general_domain, "dir_fanart", fmt("%s/fanart", m_dataDir.c_str()));
	m_bckgrndsDir = m_cfg.getString(general_domain, "dir_backgrounds", fmt("%s/backgrounds", m_dataDir.c_str()));
	
	m_sourceDir = m_cfg.getString(general_domain, "dir_source", fmt("%s/source_menu", m_dataDir.c_str()));
	m_pluginsDir = m_cfg.getString(general_domain, "dir_plugins", fmt("%s/plugins", m_dataDir.c_str()));
	m_pluginDataDir = m_cfg.getString(general_domain, "dir_plugins_data", fmt("%s/plugins_data", m_dataDir.c_str()));
	m_cartDir = m_cfg.getString(general_domain, "dir_cart", fmt("%s/cart_disk", m_dataDir.c_str()));
	m_snapDir = m_cfg.getString(general_domain, "dir_snap", fmt("%s/snapshots", m_dataDir.c_str()));
	
	/* Create our Folder Structure */
	fsop_MakeFolder(m_dataDir.c_str()); // D'OH!
	
	fsop_MakeFolder(m_cacheDir.c_str());
	fsop_MakeFolder(m_listCacheDir.c_str());
	fsop_MakeFolder(m_bnrCacheDir.c_str());
	fsop_MakeFolder(m_customBnrDir.c_str());
	
	fsop_MakeFolder(m_txtCheatDir.c_str());
	fsop_MakeFolder(m_cheatDir.c_str());
	fsop_MakeFolder(m_wipDir.c_str());

	fsop_MakeFolder(m_settingsDir.c_str());
	fsop_MakeFolder(m_languagesDir.c_str());
	fsop_MakeFolder(m_backupDir.c_str()); //
#ifdef SCREENSHOT
	fsop_MakeFolder(m_screenshotDir.c_str());
#endif

	fsop_MakeFolder(m_helpDir.c_str());
	fsop_MakeFolder(m_boxPicDir.c_str());
	fsop_MakeFolder(m_picDir.c_str());
	fsop_MakeFolder(m_themeDir.c_str());
	fsop_MakeFolder(m_coverflowsDir.c_str());
	fsop_MakeFolder(m_musicDir.c_str());
	fsop_MakeFolder(m_videoDir.c_str());
	fsop_MakeFolder(m_fanartDir.c_str());
	fsop_MakeFolder(m_bckgrndsDir.c_str());
	
	fsop_MakeFolder(m_sourceDir.c_str());
	fsop_MakeFolder(m_pluginsDir.c_str());
	fsop_MakeFolder(m_pluginDataDir.c_str());
	fsop_MakeFolder(m_cartDir.c_str());
	fsop_MakeFolder(m_snapDir.c_str());

	/* Show wait animation */
	/* only now for usb splash to last longer on screen */
	m_vid.waitMessage(0.65f);

	/* Emu nands init even if not being used */
	memset(emu_nands_dir, 0, sizeof(emu_nands_dir));
	bool vwiinands = IsOnWiiU() && m_cfg.getBool(channel_domain, "use_vwiinands", true);
	strncpy(emu_nands_dir, vwiinands ? "vwiinands" : "nands", sizeof(emu_nands_dir) - 1);
	for(u8 i = 0; i < 2; ++i)
		_checkEmuNandSettings(i);
	
	/* Misc. setup */
	SoundHandle.Init();
	m_gameSound.SetVoice(1);
	_loadDefaultFont();
	LWP_MutexInit(&m_mutex, 0);
	
	/* Load cIOS Map */
	_installed_cios.clear();
	if(!neek2o())
		_load_installed_cioses();
	else
		_installed_cios[CurrentIOS.Version] = CurrentIOS.Version;

	/* Check if wiiflow is parental locked */
	m_locked = m_cfg.getString(general_domain, "parent_code", "").size() >= 4;

	/* Set WIIFLOW_DEF exit to option */
	/* 0 thru 3 of exit to enum (EXIT_TO_MENU, EXIT_TO_HBC, POWEROFF_CONSOLE, EXIT_TO_WIIU) in sys.h */
	int exit_to = min(max(0, m_cfg.getInt(general_domain, "exit_to", 0)), (int)ARRAY_SIZE(CMenu::_exitTo) - 1 - !IsOnWiiU());
	Sys_ExitTo(exit_to);

	/* Load misc config files */
	m_cat.load(fmt("%s/" CAT_FILENAME, m_settingsDir.c_str()));
	m_gcfg1.load(fmt("%s/" GAME_SETTINGS1_FILENAME, m_settingsDir.c_str()));
	m_gcfg2.load(fmt("%s/" GAME_SETTINGS2_FILENAME, m_settingsDir.c_str())); // added
	m_platform.load(fmt("%s/platform.ini", m_pluginDataDir.c_str()));

	/* Init plugins */
	m_plugin.init(m_pluginsDir);
	vector<string> magics = m_cfg.getStrings(plugin_domain, "enabled_plugins", ',');
	if(magics.size() > 0)
	{
		enabledPluginsCount = 0;
		string enabledMagics;
		for(u8 i = 0; i < magics.size(); i++)
		{
			u8 pos = m_plugin.GetPluginPosition(strtoul(magics[i].c_str(), NULL, 16));
			if(pos < 255)
			{
				enabledPluginsCount++;
				m_plugin.SetEnablePlugin(pos, 2);
				if(enabledPluginsCount == 1)
					enabledMagics = magics[i];
				else
					enabledMagics.append(',' + magics[i]);
			}
		}
		m_cfg.setString(plugin_domain, "enabled_plugins", enabledMagics);
		magics.clear();
	}

	/* Set wiiflow language */
	const char *defLang = "Default";
	switch (CONF_GetLanguage())
	{
		case CONF_LANG_JAPANESE:
			defLang = "japanese";
			break;
		case CONF_LANG_GERMAN:
			defLang = "german";
			break;
		case CONF_LANG_FRENCH:
			defLang = "french";
			break;
		case CONF_LANG_SPANISH:
			defLang = "spanish";
			break;
		case CONF_LANG_ITALIAN:
			defLang = "italian";
			break;
		case CONF_LANG_DUTCH:
			defLang = "dutch";
			break;
		case CONF_LANG_SIMP_CHINESE:
			defLang = "chinese_s";
			break;
		case CONF_LANG_TRAD_CHINESE:
			defLang = "chinese_t";
			break;
		case CONF_LANG_KOREAN:
			defLang = "korean";
			break;
		case CONF_LANG_ENGLISH:
			defLang = "english";
			break;
	}
	if(CONF_GetArea() == CONF_AREA_BRA)
		defLang = "brazilian";

	m_curLanguage = m_cfg.getString(general_domain, "language", defLang);
	if(!m_loc.load(fmt("%s/%s.ini", m_languagesDir.c_str(), m_curLanguage.c_str())))
	{
		m_curLanguage = "Default";
		m_cfg.setString(general_domain, "language", m_curLanguage.c_str());
		m_loc.unload();
	}

	/* Init gametdb and custom titles for game list making */
	m_cacheList.Init(m_settingsDir.c_str(), m_loc.getString(m_curLanguage, "gametdb_code", "EN").c_str(), m_pluginDataDir.c_str());
	
	/* Coverflow init */
	CoverFlow.init(m_base_font, m_base_font_size, m_vid.vid_50hz());

	/* Load theme and coverflow files */
	m_themeName = m_cfg.getString(general_domain, "theme", "Default");
	m_themeDataDir = fmt("%s/%s", m_themeDir.c_str(), m_themeName.c_str());
	m_theme.load(fmt("%s.ini", m_themeDataDir.c_str()));
	m_coverflow.load(fmt("%s/%s.ini", m_coverflowsDir.c_str(), m_themeName.c_str()));
	if(!m_coverflow.loaded())
		m_coverflow.load(fmt("%s/default.ini", m_coverflowsDir.c_str()));	
	
	/* Init the onscreen pointer */
	m_aa = 3;
	CColor pShadowColor = m_theme.getColor(general_domain, "pointer_shadow_color", CColor(0x3F000000));
	float pShadowX = m_theme.getFloat(general_domain, "pointer_shadow_x", 3.f);
	float pShadowY = m_theme.getFloat(general_domain, "pointer_shadow_y", 3.f);
	bool pShadowBlur = m_theme.getBool(general_domain, "pointer_shadow_blur", false);

	for(int chan = WPAD_MAX_WIIMOTES-1; chan >= 0; chan--)
	{
		m_cursor[chan].init(fmt("%s/%s", m_themeDataDir.c_str(), m_theme.getString(general_domain, fmt("pointer%i", chan+1)).c_str()),
			m_vid.wide(), pShadowColor, pShadowX, pShadowY, pShadowBlur, chan);
		WPAD_SetVRes(chan, m_vid.width() + m_cursor[chan].width(), m_vid.height() + m_cursor[chan].height());
	}
	
	/* Init background Music Player and song info */
	MusicPlayer.Init(m_cfg, m_musicDir, fmt("%s/music", m_themeDataDir.c_str()));
	m_music_info = m_cfg.getBool(general_domain, "display_music_info", false);
	MusicPlayer.SetResampleSetting(m_cfg.getBool(general_domain, "resample_to_48khz", false));

	/* Set sound volumes */
	CoverFlow.setSoundVolume(m_cfg.getInt(general_domain, "sound_volume_coverflow", 255));
	// "sound_volume_gui" is set in _buildMenus()
	m_bnrSndVol = m_cfg.getInt(general_domain, "sound_volume_bnr", 255);
	m_bnr_settings = m_cfg.getBool(general_domain, "banner_in_settings", true);	
	
	/* Explorer on start if last game was launched from Explorer (unless Source menu on start below) */
	m_explorer_on_start = (m_cfg.getBool(general_domain, "explorer_on_start", 0) && !neek2o());
	
	/* Source Menu on start reset tiers before build menus */
	if(m_cfg.getBool(general_domain, "source_on_start", false))
	{
		m_explorer_on_start = false;
		m_cfg.remove(SOURCEFLOW_DOMAIN, "tiers");
		m_cfg.remove(SOURCEFLOW_DOMAIN, "numbers");
	}	

	/* Init Button Manager, build the menus and load coverflow config */
	_buildMenus();
	
	/* Init categories_lite.ini with gameTDB genres */
	m_max_categories = m_cat.getInt(general_domain, "numcategories", 1);
	if(m_max_categories == 1)
		_initTDBCategories();
	tdb_genres = m_cfg.getBool(general_domain, "tdb_genres", false);
	
	/* Enable sourceflow */
	SF_enabled = m_cfg.getBool(sourceflow_domain, "enabled", false);
	
	/* Enable Wiimote roll */
	enable_wmote_roll = m_cfg.getBool(general_domain, "wiimote_gestures", false);
	
	/* Screensaver delay */
	m_screensaverDelay = min(max(0, m_cfg.getInt(general_domain, "screensaver_idle_seconds", 60)), 255);

	return true;
}

bool cleaned_up = false;

void CMenu::cleanup()
{
	if(cleaned_up)
		return;
	// m_btnMgr.hide(m_mainLblCurMusic);
	_stopSounds();
	MusicPlayer.Cleanup();
	_cleanupDefaultFont();
	CoverFlow.shutdown(); // possibly plugin flow crash so cleanup early
	m_banner.DeleteBanner();
	m_plugin.Cleanup();
	m_source.unload();
	m_platform.unload();
	m_loc.unload();
	_Theme_Cleanup();
	for(int chan = WPAD_MAX_WIIMOTES-1; chan >= 0; chan--)
		m_cursor[chan].cleanup();
	m_gameSound.FreeMemory();
	SoundHandle.Cleanup();
	soundDeinit();
	m_vid.cleanup();
	wiiLightOff();
	LWP_MutexDestroy(m_mutex);
	m_mutex = 0;
	cleaned_up = true;
	gprintf("MEM1_freesize(): %i\nMEM2_freesize(): %i\n", MEM1_freesize(), MEM2_freesize());
}

void CMenu::_Theme_Cleanup(void)
{
	/* Backgrounds */
	TexHandle.Cleanup(theme.bg);
	m_prevBg = NULL;
	m_nextBg = NULL;
	TexHandle.Cleanup(m_curBg);
	m_lqBg = NULL;
	TexHandle.Cleanup(m_mainCustomBg[0]);
	TexHandle.Cleanup(m_mainCustomBg[1]);
	
	TexHandle.Cleanup(m_mainBg);
	TexHandle.Cleanup(m_configBg);
	
	/* Main icons */
	TexHandle.Cleanup(bgLQ);
	TexHandle.Cleanup(texHome);
	TexHandle.Cleanup(texHomeS);
	TexHandle.Cleanup(texCateg);
	TexHandle.Cleanup(texCategS);
	TexHandle.Cleanup(texFavOff);
	TexHandle.Cleanup(texFavOffS);
	TexHandle.Cleanup(texFavOn);
	TexHandle.Cleanup(texFavOnS);
	TexHandle.Cleanup(texDVD);
	TexHandle.Cleanup(texDVDS);
	TexHandle.Cleanup(texRandom);
	TexHandle.Cleanup(texRandomS);
	TexHandle.Cleanup(texSort);
	TexHandle.Cleanup(texSortS);
	TexHandle.Cleanup(texConfig);
	TexHandle.Cleanup(texConfigS);

	TexHandle.Cleanup(texPrev);
	TexHandle.Cleanup(texPrevS);
	TexHandle.Cleanup(texNext);
	TexHandle.Cleanup(texNextS);

	/* Game icons and other textures */
	TexHandle.Cleanup(texGameCateg);
	TexHandle.Cleanup(texGameCategS);
	TexHandle.Cleanup(texGameFav);
	TexHandle.Cleanup(texGameFavS);
	TexHandle.Cleanup(texGameFavOn);
	TexHandle.Cleanup(texGameFavOnS);
	TexHandle.Cleanup(texCheat);
	TexHandle.Cleanup(texCheatS);
	TexHandle.Cleanup(texCheatOn);
	TexHandle.Cleanup(texCheatOnS);
	TexHandle.Cleanup(texGameConfig);
	TexHandle.Cleanup(texGameConfigS);
	TexHandle.Cleanup(texGameConfigOn);
	TexHandle.Cleanup(texGameConfigOnS);
	TexHandle.Cleanup(texVideo);
	TexHandle.Cleanup(texVideoS);
	TexHandle.Cleanup(texLock);
	TexHandle.Cleanup(texLockS);
	TexHandle.Cleanup(texLockOn);
	TexHandle.Cleanup(texLockOnS);
	
	TexHandle.Cleanup(texToggleBanner);
	TexHandle.Cleanup(texSnapShotBg);
	TexHandle.Cleanup(texBannerFrame);
	
	/* Buttons */
	TexHandle.Cleanup(theme.btnTexL);
	TexHandle.Cleanup(theme.btnTexR);
	TexHandle.Cleanup(theme.btnTexC);
	TexHandle.Cleanup(theme.btnTexLS);
	TexHandle.Cleanup(theme.btnTexRS);
	TexHandle.Cleanup(theme.btnTexCS);
	TexHandle.Cleanup(theme.btnTexPlus);
	TexHandle.Cleanup(theme.btnTexPlusS);
	TexHandle.Cleanup(theme.btnTexMinus);
	TexHandle.Cleanup(theme.btnTexMinusS);
	TexHandle.Cleanup(theme.btnTexGo);
	TexHandle.Cleanup(theme.btnTexGoS);
	
	/* Checkboxes */
	TexHandle.Cleanup(theme.checkboxoff);
	TexHandle.Cleanup(theme.checkboxoffs);
	TexHandle.Cleanup(theme.checkboxon);
	TexHandle.Cleanup(theme.checkboxons);
	TexHandle.Cleanup(theme.checkboxHid);
	TexHandle.Cleanup(theme.checkboxHids);
	TexHandle.Cleanup(theme.checkboxReq);
	TexHandle.Cleanup(theme.checkboxReqs);
	
	/* Progress Bars */
	TexHandle.Cleanup(theme.pbarTexL);
	TexHandle.Cleanup(theme.pbarTexR);
	TexHandle.Cleanup(theme.pbarTexC);
	TexHandle.Cleanup(theme.pbarTexLS);
	TexHandle.Cleanup(theme.pbarTexRS);
	TexHandle.Cleanup(theme.pbarTexCS);
	
	/* Plugin game snapshot and overlay */
	TexHandle.Cleanup(m_game_snap);
	TexHandle.Cleanup(m_game_overlay);
	
	/* Other Theme Stuff */
	// for(TexSet::iterator texture = theme.texSet.begin(); texture != theme.texSet.end(); ++texture)
		// TexHandle.Cleanup(texture->second);
	// for(vector<SFont>::iterator font = theme.fontSet.begin(); font != theme.fontSet.end(); ++font)
		// font->ClearData();
	for(SoundSet::iterator sound = theme.soundSet.begin(); sound != theme.soundSet.end(); ++sound)
		sound->second->FreeMemory();
	theme.texSet.clear();
	theme.fontSet.clear();
	theme.soundSet.clear();
	m_theme.unload();
	m_coverflow.unload();
}

void CMenu::_setAA(int aa)
{
	switch(aa)
	{
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 8:
			m_aa = aa;
			break;
		case 7:
			m_aa = 6;
			break;
		default:
			m_aa = 0;
	}
}

void CMenu::_loadCFCfg()
{
	const char *domain = "_COVERFLOW";
	// gprintf("Preparing to load sounds from %s\n", m_themeDataDir.c_str());
	CoverFlow.setCachePath(m_cacheDir.c_str());
	CoverFlow.setBufferSize(m_cfg.getInt(general_domain, "cover_buffer", 20));
	
	/* Coverflow Sounds */
	CoverFlow.setSounds(
		new GuiSound(fmt("%s/%s", m_themeDataDir.c_str(), m_theme.getString(domain, "sound_flip").c_str())),
		_sound(theme.soundSet, m_theme.getString(domain, "sound_hover", "").c_str(),  hover_wav, hover_wav_size, "default_btn_hover", false),
		_sound(theme.soundSet, m_theme.getString(domain, "sound_select", "").c_str(), click_wav, click_wav_size, "default_btn_click", false),
		new GuiSound(fmt("%s/%s", m_themeDataDir.c_str(), m_theme.getString(domain, "sound_cancel").c_str()))
	);
	
	/* Textures */
	string texLoading = fmt("%s/%s", m_themeDataDir.c_str(), m_theme.getString(domain, "loading_cover_box").c_str());
	string texNoCover = fmt("%s/%s", m_themeDataDir.c_str(), m_theme.getString(domain, "missing_cover_box").c_str());
	string texLoadingFlat = fmt("%s/%s", m_themeDataDir.c_str(), m_theme.getString(domain, "loading_cover_flat").c_str());
	string texNoCoverFlat = fmt("%s/%s", m_themeDataDir.c_str(), m_theme.getString(domain, "missing_cover_flat").c_str());
	CoverFlow.setTextures(texLoading, texLoadingFlat, texNoCover, texNoCoverFlat);
	
	/* Font */
	CoverFlow.setFont(_font(domain, "font", theme.titleFont), m_theme.getColor(domain, "font_color", CColor(0xFFFFFFFF)));
}

Vector3D CMenu::_getCFV3D(const string &domain, const string &key, const Vector3D &def, bool otherScrnFmt)
{
	string key169(key);
	string key43(key);

	key43 += "_4_3";
	if(m_vid.wide() == otherScrnFmt)
		swap(key169, key43);
	if(m_coverflow.has(domain, key169))
	{
		Vector3D v(m_coverflow.getVector3D(domain, key169));
		m_coverflow.getVector3D(domain, key43, v);
		return v;
	}
	return m_coverflow.getVector3D(domain, key169, m_coverflow.getVector3D(domain, key43, def));
}

int CMenu::_getCFInt(const string &domain, const string &key, int def, bool otherScrnFmt)
{
	string key169(key);
	string key43(key);

	key43 += "_4_3";
	if(m_vid.wide() == otherScrnFmt)
		swap(key169, key43);
	if(m_coverflow.has(domain, key169))
	{
		int v = m_coverflow.getInt(domain, key169);
		m_coverflow.getInt(domain, key43, v);
		return v;
	}
	return m_coverflow.getInt(domain, key169, m_coverflow.getInt(domain, key43, def));
}

float CMenu::_getCFFloat(const string &domain, const string &key, float def, bool otherScrnFmt)
{
	string key169(key);
	string key43(key);

	key43 += "_4_3";
	if(m_vid.wide() == otherScrnFmt)
		swap(key169, key43);
	if(m_coverflow.has(domain, key169))
	{
		float v = m_coverflow.getFloat(domain, key169);
		m_coverflow.getFloat(domain, key43, v);
		return v;
	}
	return m_coverflow.getFloat(domain, key169, m_coverflow.getFloat(domain, key43, def));
}

void CMenu::_loadCFLayout(int version, bool forceAA, bool otherScrnFmt)
{
	string domain = fmt("%s_%i", cf_domain, version);
	string domainSel = fmt("%s_%i_S", cf_domain, version);
	bool smallflow = strcasecmp(cf_domain, "_SMALLFLOW") == 0;	
	bool sf = otherScrnFmt;

	int max_fsaa = m_coverflow.getInt(domain, "max_fsaa", 3);
	_setAA(forceAA ? max_fsaa : min(max_fsaa, m_cfg.getInt(general_domain, "max_fsaa", 3)));

	CoverFlow.setTextureQuality(m_coverflow.getFloat(domain, "tex_lod_bias", -3.f),
		m_coverflow.getInt(domain, "tex_aniso", 2),
		m_coverflow.getBool(domain, "tex_edge_lod", true));

	CoverFlow.setRange(_getCFInt(domain, "rows", (smallflow ? 5 : 1), sf), _getCFInt(domain, "columns", 11, sf));

	CoverFlow.setCameraPos(false,
		_getCFV3D(domain, "camera_pos", Vector3D(0.f, 0.f, 5.f), sf),
		_getCFV3D(domain, "camera_aim", Vector3D(0.f, 0.f, 0.f), sf));

	CoverFlow.setCameraPos(true,
		_getCFV3D(domainSel, "camera_pos", Vector3D(0.f, 0.f, 5.f), sf),
		_getCFV3D(domainSel, "camera_aim", Vector3D(0.f, 0.f, 0.f), sf));

	CoverFlow.setCameraOsc(false,
		_getCFV3D(domain, "camera_osc_speed", Vector3D(2.f, 0.f, 0.f), sf),
		_getCFV3D(domain, "camera_osc_amp", Vector3D(0.1f, 0.f, 0.f), sf));

	CoverFlow.setCameraOsc(true,
		_getCFV3D(domainSel, "camera_osc_speed", Vector3D(), sf),
		_getCFV3D(domainSel, "camera_osc_amp", Vector3D(), sf));

	float def_cvr_posX = smallflow ? 1.f : 1.6f;
	float def_cvr_posY = smallflow ? -0.9f : -1.f;
	CoverFlow.setCoverPos(false,
		_getCFV3D(domain, "left_pos", Vector3D(-def_cvr_posX, def_cvr_posY, 0.f), sf),
		_getCFV3D(domain, "right_pos", Vector3D(def_cvr_posX, def_cvr_posY, 0.f), sf),
		_getCFV3D(domain, "center_pos", Vector3D(0.f, def_cvr_posY, 1.f), sf),
		_getCFV3D(domain, "row_center_pos", Vector3D(0.f, def_cvr_posY, 0.f), sf));

	if(smallflow)
		CoverFlow.setCoverPos(true, 
			_getCFV3D(domainSel, "left_pos", Vector3D(-4.05f, -0.6f, -1.f), sf),
			_getCFV3D(domainSel, "right_pos", Vector3D(3.35f, -0.6f, -1.f), sf),
			_getCFV3D(domainSel, "center_pos", Vector3D(-0.5f, -0.8f, 2.6f), sf),
			_getCFV3D(domainSel, "row_center_pos", Vector3D(-3.05f, -0.6f, -1.f), sf));
	else
		CoverFlow.setCoverPos(true,
			_getCFV3D(domainSel, "left_pos", Vector3D(-4.7f, -1.f, 0.f), sf),
			_getCFV3D(domainSel, "right_pos", Vector3D(4.7f, -1.f, 0.f), sf),
			_getCFV3D(domainSel, "center_pos", Vector3D(-0.6f, -1.f, 2.6f), sf),
			_getCFV3D(domainSel, "row_center_pos", Vector3D(0.f, 0.f, 0.f), sf));

	CoverFlow.setCoverAngleOsc(false,
		m_coverflow.getVector3D(domain, "cover_osc_speed", Vector3D(2.f, 2.f, 0.f)),
		m_coverflow.getVector3D(domain, "cover_osc_amp", Vector3D(5.f, 10.f, 0.f)));

	CoverFlow.setCoverAngleOsc(true,
		m_coverflow.getVector3D(domainSel, "cover_osc_speed", Vector3D(2.1f, 2.1f, 0.f)),
		m_coverflow.getVector3D(domainSel, "cover_osc_amp", Vector3D(2.f, 5.f, 0.f)));

	CoverFlow.setCoverPosOsc(false,
		m_coverflow.getVector3D(domain, "cover_pos_osc_speed"),
		m_coverflow.getVector3D(domain, "cover_pos_osc_amp"));

	CoverFlow.setCoverPosOsc(true,
		m_coverflow.getVector3D(domainSel, "cover_pos_osc_speed"),
		m_coverflow.getVector3D(domainSel, "cover_pos_osc_amp"));

	float spacerX = smallflow ? 1.f : 0.35f;
	CoverFlow.setSpacers(false,
		m_coverflow.getVector3D(domain, "left_spacer", Vector3D(-spacerX, 0.f, 0.f)),
		m_coverflow.getVector3D(domain, "right_spacer", Vector3D(spacerX, 0.f, 0.f)));

	CoverFlow.setSpacers(true,
		m_coverflow.getVector3D(domainSel, "left_spacer", Vector3D(-spacerX, 0.f, 0.f)),
		m_coverflow.getVector3D(domainSel, "right_spacer", Vector3D(spacerX, 0.f, 0.f)));

	CoverFlow.setDeltaAngles(false,
		m_coverflow.getVector3D(domain, "left_delta_angle"),
		m_coverflow.getVector3D(domain, "right_delta_angle"));

	CoverFlow.setDeltaAngles(true,
		m_coverflow.getVector3D(domainSel, "left_delta_angle"),
		m_coverflow.getVector3D(domainSel, "right_delta_angle"));

	float angleY = smallflow ? 0.f : 70.f;
	CoverFlow.setAngles(false,
		m_coverflow.getVector3D(domain, "left_angle", Vector3D(0.f, angleY, 0.f)),
		m_coverflow.getVector3D(domain, "right_angle", Vector3D(0.f, -angleY, 0.f)),
		m_coverflow.getVector3D(domain, "center_angle", Vector3D(0.f, 0.f, 0.f)),
		m_coverflow.getVector3D(domain, "row_center_angle"));

	angleY = smallflow ? 0.f : 90.f;
	float angleY1 = smallflow ? 0.f : 360.f;
	float angleX = smallflow ? 0.f : -45.f;
	CoverFlow.setAngles(true,
		m_coverflow.getVector3D(domainSel, "left_angle", Vector3D(angleX, angleY, 0.f)),
		m_coverflow.getVector3D(domainSel, "right_angle", Vector3D(angleX, -angleY, 0.f)),
		m_coverflow.getVector3D(domainSel, "center_angle", Vector3D(0.f, angleY1, 0.f)),
		m_coverflow.getVector3D(domainSel, "row_center_angle"));

	angleX = smallflow ? 0.f : 20.f;
	CoverFlow.setTitleAngles(false,
		_getCFFloat(domain, "text_left_angle", -angleX, sf),
		_getCFFloat(domain, "text_right_angle", angleX, sf),
		_getCFFloat(domain, "text_center_angle", 0.f, sf));

	CoverFlow.setTitleAngles(true,
		_getCFFloat(domainSel, "text_left_angle", -angleX, sf),
		_getCFFloat(domainSel, "text_right_angle", angleX, sf),
		_getCFFloat(domainSel, "text_center_angle", 0.f, sf));

	def_cvr_posX = smallflow ? 2.f : 1.f;
	CoverFlow.setTitlePos(false,
		_getCFV3D(domain, "text_left_pos", Vector3D(-def_cvr_posX, 0.8f, 2.6f), sf),
		_getCFV3D(domain, "text_right_pos", Vector3D(def_cvr_posX, 0.8f, 2.6f), sf),
		_getCFV3D(domain, "text_center_pos", Vector3D(0.f, 0.8f, 2.6f), sf));

	def_cvr_posX = smallflow ? .6f : 1.8f;
	CoverFlow.setTitlePos(true,
		_getCFV3D(domainSel, "text_left_pos", Vector3D(-4.f, 0.f, 1.3f), sf),
		_getCFV3D(domainSel, "text_right_pos", Vector3D(4.f, 0.f, 1.3f), sf),
		_getCFV3D(domainSel, "text_center_pos", Vector3D(def_cvr_posX, 1.f, 1.6f), sf));

	CoverFlow.setTitleWidth(false,
		_getCFFloat(domain, "text_side_wrap_width", 500.f, sf),
		_getCFFloat(domain, "text_center_wrap_width", 500.f, sf));

	CoverFlow.setTitleWidth(true,
		_getCFFloat(domainSel, "text_side_wrap_width", 500.f, sf),
		_getCFFloat(domainSel, "text_center_wrap_width", 360.f, sf));

	CoverFlow.setTitleStyle(false,
		_textStyle(domain.c_str(), "text_side_style", FTGX_ALIGN_MIDDLE | FTGX_JUSTIFY_CENTER, true),
		_textStyle(domain.c_str(), "text_center_style", FTGX_ALIGN_MIDDLE | FTGX_JUSTIFY_CENTER, true));

	CoverFlow.setTitleStyle(true,
		_textStyle(domainSel.c_str(), "text_side_style", FTGX_ALIGN_MIDDLE | FTGX_JUSTIFY_CENTER, true),
		_textStyle(domainSel.c_str(), "text_center_style", FTGX_ALIGN_TOP | FTGX_JUSTIFY_RIGHT, true));

	CoverFlow.setColors(false,
		m_coverflow.getColor(domain, "color_beg", 0xCFFFFFFF),
		m_coverflow.getColor(domain, "color_end", 0x3FFFFFFF),
		m_coverflow.getColor(domain, "color_off", 0x7FFFFFFF));

	CoverFlow.setColors(true,
		m_coverflow.getColor(domainSel, "color_beg", 0x7FFFFFFF),
		m_coverflow.getColor(domainSel, "color_end", 0x1FFFFFFF),
		m_coverflow.getColor(domain, "color_off", 0x7FFFFFFF));	// Mouse not used once a selection has been made

	CoverFlow.setMirrorAlpha(m_coverflow.getFloat(domain, "mirror_alpha", 0.15f), m_coverflow.getFloat(domain, "title_mirror_alpha", 0.03f));	// Doesn't depend on selection

	CoverFlow.setMirrorBlur(m_coverflow.getBool(domain, "mirror_blur", true));	// Doesn't depend on selection

	CoverFlow.setShadowColors(false,
		m_coverflow.getColor(domain, "color_shadow_center", 0x00000000),
		m_coverflow.getColor(domain, "color_shadow_beg", 0x00000000),
		m_coverflow.getColor(domain, "color_shadow_end", 0x00000000),
		m_coverflow.getColor(domain, "color_shadow_off", 0x00000000));

	CoverFlow.setShadowColors(true,
		m_coverflow.getColor(domainSel, "color_shadow_center", 0x0000007F),
		m_coverflow.getColor(domainSel, "color_shadow_beg", 0x0000007F),
		m_coverflow.getColor(domainSel, "color_shadow_end", 0x0000007F),
		m_coverflow.getColor(domainSel, "color_shadow_off", 0x0000007F));

	CoverFlow.setShadowPos(m_coverflow.getFloat(domain, "shadow_scale", 1.1f),
		m_coverflow.getFloat(domain, "shadow_x"),
		m_coverflow.getFloat(domain, "shadow_y"));
	
	float spacerY = smallflow ? 0.60f : 2.f; 
	CoverFlow.setRowSpacers(false,
		m_coverflow.getVector3D(domain, "top_spacer", Vector3D(0.f, spacerY, 0.f)),
		m_coverflow.getVector3D(domain, "bottom_spacer", Vector3D(0.f, -spacerY, 0.f)));

	CoverFlow.setRowSpacers(true,
		m_coverflow.getVector3D(domainSel, "top_spacer", Vector3D(0.f, spacerY, 0.f)),
		m_coverflow.getVector3D(domainSel, "bottom_spacer", Vector3D(0.f, -spacerY, 0.f)));

	CoverFlow.setRowDeltaAngles(false,
		m_coverflow.getVector3D(domain, "top_delta_angle"),
		m_coverflow.getVector3D(domain, "bottom_delta_angle"));

	CoverFlow.setRowDeltaAngles(true,
		m_coverflow.getVector3D(domainSel, "top_delta_angle"),
		m_coverflow.getVector3D(domainSel, "bottom_delta_angle"));

	CoverFlow.setRowAngles(false,
		m_coverflow.getVector3D(domain, "top_angle"),
		m_coverflow.getVector3D(domain, "bottom_angle"));

	CoverFlow.setRowAngles(true,
		m_coverflow.getVector3D(domainSel, "top_angle"),
		m_coverflow.getVector3D(domainSel, "bottom_angle"));

	Vector3D def_cvr_scale = smallflow ? Vector3D(0.66f, 0.25f, 1.f) : Vector3D(1.f, 1.f, 1.f);	

	CoverFlow.setCoverScale(false,
		m_coverflow.getVector3D(domain, "left_scale", def_cvr_scale),
		m_coverflow.getVector3D(domain, "right_scale", def_cvr_scale),
		m_coverflow.getVector3D(domain, "center_scale", def_cvr_scale),
		m_coverflow.getVector3D(domain, "row_center_scale", def_cvr_scale));

	CoverFlow.setCoverScale(true,
		m_coverflow.getVector3D(domainSel, "left_scale", def_cvr_scale),
		m_coverflow.getVector3D(domainSel, "right_scale", def_cvr_scale),
		m_coverflow.getVector3D(domainSel, "center_scale", def_cvr_scale),
		m_coverflow.getVector3D(domainSel, "row_center_scale", def_cvr_scale));

	float flipX = smallflow ? 360.f : 180.f;
	CoverFlow.setCoverFlipping(
		_getCFV3D(domainSel, "flip_pos", Vector3D(), sf),
		_getCFV3D(domainSel, "flip_angle", Vector3D(0.f, flipX, 0.f), sf),
		_getCFV3D(domainSel, "flip_scale", Vector3D(1.f, 1.f, 1.f), sf));

	CoverFlow.setBlur(
		m_coverflow.getInt(domain, "blur_resolution", 1),
		m_coverflow.getInt(domain, "blur_radius", 2),
		m_coverflow.getFloat(domain, "blur_factor", 1.f));
}

extern const u8 btn_png[];
extern const u8 btn_s_png[];
extern const u8 btn_minus_png[];
extern const u8 btn_minus_s_png[];
extern const u8 btn_plus_png[];
extern const u8 btn_plus_s_png[];
extern const u8 btn_go_png[];
extern const u8 btn_go_s_png[];

extern const u8 btn_checkbox_png[];
extern const u8 btn_checkbox_s_png[];
extern const u8 btn_checkboxhid_png[];
extern const u8 btn_checkboxhid_s_png[];
extern const u8 btn_checkboxon_png[];
extern const u8 btn_checkboxon_s_png[];
extern const u8 btn_checkboxreq_png[];
extern const u8 btn_checkboxreq_s_png[];

extern const u8 pbar_png[];
extern const u8 pbar_s_png[];

extern const u8 tex_bg_png[];

void CMenu::_buildMenus(void)
{
	const char *galDomain = "GENERAL";
	
	m_btnMgr.init();
	m_btnMgr.setSoundVolume(m_cfg.getInt(general_domain, "sound_volume_gui", 255));
	m_btnMgr.setRumble(m_cfg.getBool(general_domain, "rumble", true));
	
	/* Default fonts */
	theme.btnFont = _dfltFont(BUTTONFONT);
	theme.btnFontColor = m_theme.getColor(general_domain, "button_font_color", 0xFFFFFFFF); // instead of 0xD0BFDFFF
	theme.lblFont = _dfltFont(LABELFONT);
	theme.lblFontColor = m_theme.getColor(general_domain, "label_font_color", 0xFFFFFFFF); // instead of 0xD0BFDFFF
	theme.titleFont = _dfltFont(TITLEFONT);
	theme.titleFontColor = m_theme.getColor(general_domain, "title_font_color", 0xFFFFFFFF);
	theme.txtFont = _dfltFont(TEXTFONT);
	theme.txtFontColor = m_theme.getColor(general_domain, "text_font_color", 0xFFFFFFFF);

	/* Default Sounds */
	theme.clickSound	= _sound(theme.soundSet, m_theme.getString(general_domain, "click_sound", "").c_str(), click_wav, click_wav_size, "default_btn_click", false);
	theme.hoverSound	= _sound(theme.soundSet, m_theme.getString(general_domain, "hover_sound", "").c_str(), hover_wav, hover_wav_size, "default_btn_hover", false);
#ifdef SCREENSHOT
	theme.cameraSound	= _sound(theme.soundSet, m_theme.getString(general_domain, "camera_sound", "").c_str(), camera_wav, camera_wav_size, "default_camera", false);
#endif
	theme.homeSound     = new GuiSound(fmt("%s/%s", m_themeDataDir.c_str(), m_theme.getString(general_domain, "home_sound").c_str()));

	TexHandle.fromPNG(theme.btnTexL, btn_png);
	TexHandle.fromPNG(theme.btnTexR, btn_png);
	TexHandle.fromPNG(theme.btnTexC, btn_png);
	TexHandle.fromPNG(theme.btnTexLS, btn_s_png);
	TexHandle.fromPNG(theme.btnTexRS, btn_s_png);
	TexHandle.fromPNG(theme.btnTexCS, btn_s_png);
	TexHandle.fromPNG(theme.btnTexPlus, btn_plus_png);
	TexHandle.fromPNG(theme.btnTexPlusS, btn_plus_s_png);
	TexHandle.fromPNG(theme.btnTexMinus, btn_minus_png);
	TexHandle.fromPNG(theme.btnTexMinusS, btn_minus_s_png);
	TexHandle.fromPNG(theme.btnTexGo, btn_go_png);
	TexHandle.fromPNG(theme.btnTexGoS, btn_go_s_png);
	
	TexHandle.fromPNG(theme.checkboxoff, btn_checkbox_png);
	TexHandle.fromPNG(theme.checkboxoffs, btn_checkbox_s_png);
	TexHandle.fromPNG(theme.checkboxon, btn_checkboxon_png);
	TexHandle.fromPNG(theme.checkboxons, btn_checkboxon_s_png);
	TexHandle.fromPNG(theme.checkboxHid, btn_checkboxhid_png);
	TexHandle.fromPNG(theme.checkboxHids, btn_checkboxhid_s_png);
	TexHandle.fromPNG(theme.checkboxReq, btn_checkboxreq_png);
	TexHandle.fromPNG(theme.checkboxReqs, btn_checkboxreq_s_png);
	
	TexHandle.fromPNG(theme.pbarTexL, pbar_png);
	TexHandle.fromPNG(theme.pbarTexR, pbar_png);
	TexHandle.fromPNG(theme.pbarTexC, pbar_png);
	TexHandle.fromPNG(theme.pbarTexLS, pbar_s_png);
	TexHandle.fromPNG(theme.pbarTexRS, pbar_s_png);
	TexHandle.fromPNG(theme.pbarTexCS, pbar_s_png);
	
	TexHandle.fromPNG(theme.bg, tex_bg_png);
	TexHandle.fromPNG(m_mainBgLQ, tex_bg_png, GX_TF_CMPR, 64, 64);


	/* Default buttons textures */
	theme.btnTexL = _texture(galDomain, "button_texture_left", theme.btnTexL); 
	theme.btnTexR = _texture(galDomain, "button_texture_right", theme.btnTexR); 
	theme.btnTexC = _texture(galDomain, "button_texture_center", theme.btnTexC); 
	theme.btnTexLS = _texture(galDomain, "button_texture_left_selected", theme.btnTexLS); 
	theme.btnTexRS = _texture(galDomain, "button_texture_right_selected", theme.btnTexRS); 
	theme.btnTexCS = _texture(galDomain, "button_texture_center_selected", theme.btnTexCS);
	theme.btnTexPlus = _texture(galDomain, "plus_button_texture", theme.btnTexPlus);
	theme.btnTexPlusS = _texture(galDomain, "plus_button_texture_selected", theme.btnTexPlusS);
	theme.btnTexMinus = _texture(galDomain, "minus_button_texture", theme.btnTexMinus);
	theme.btnTexMinusS = _texture(galDomain, "minus_button_texture_selected", theme.btnTexMinusS);
	theme.btnTexGo = _texture(galDomain, "button_texture_go", theme.btnTexGo);
	theme.btnTexGoS = _texture(galDomain, "button_texture_go_selected", theme.btnTexGoS); 	

	/* Default checkboxes textures */
	theme.checkboxoff = _texture(galDomain, "checkbox_off", theme.checkboxoff);
	theme.checkboxoffs = _texture(galDomain, "checkbox_off_selected", theme.checkboxoffs);
	theme.checkboxon = _texture(galDomain, "checkbox_on", theme.checkboxon);
	theme.checkboxons = _texture(galDomain, "checkbox_on_selected", theme.checkboxons);
	theme.checkboxHid = _texture(galDomain, "checkbox_Hid", theme.checkboxHid);
	theme.checkboxHids = _texture(galDomain, "checkbox_Hid_selected", theme.checkboxHids);
	theme.checkboxReq = _texture(galDomain, "checkbox_Req", theme.checkboxReq);
	theme.checkboxReqs = _texture(galDomain, "checkbox_Req_selected", theme.checkboxReqs);

	/* Default progress bars textures */
	theme.pbarTexL = _texture(galDomain, "progressbar_texture_left", theme.pbarTexL);
	theme.pbarTexR = _texture(galDomain, "progressbar_texture_right", theme.pbarTexR);
	theme.pbarTexC = _texture(galDomain, "progressbar_texture_center", theme.pbarTexC);
	theme.pbarTexLS = _texture(galDomain, "progressbar_texture_left_selected", theme.pbarTexLS);
	theme.pbarTexRS = _texture(galDomain, "progressbar_texture_right_selected", theme.pbarTexRS);
	theme.pbarTexCS = _texture(galDomain, "progressbar_texture_center_selected", theme.pbarTexCS);

	/* Build menus */
	_initMainMenu();	
	_initGameMenu();
	_initSourceMenu();
	_initExplorer();
	_initCheatSettingsMenu();
	_initConfigMenu();
	_initCodeMenu();
	_initAboutMenu();
	_initCFThemeMenu();
	_initGameInfoMenu();
	_initHomeAndExitToMenu();
	_initFTP();
	_initWBFSMenu();
	_initKeyboardMenu();
	
	/* Load coverflow config */
	_loadCFCfg();
}

typedef struct
{
	string ext;
	u32 min;
	u32 max;
	u32 def;
	u32 res;
} FontHolder;

SFont CMenu::_dfltFont(u32 fontSize, u32 lineSpacing, u32 weight, u32 index, const char *genKey)
{
	/* Get font info from theme.ini or use the default values */
	string filename;
	FontHolder fonts[3] = {{ "_size", 6u, 300u, fontSize, 0 }, { "_line_height", 6u, 300u, lineSpacing, 0 }, { "_weight", 1u, 32u, weight, 0 }};

	filename = m_theme.getString(general_domain, genKey, genKey);
	bool useDefault = filename == genKey;

	/* Get the resources - fontSize, lineSpacing, and weight */
	for(u32 i = 0; i < 3; i++)
	{
		string defValue = genKey;
		defValue += fonts[i].ext; // _size, _line_height, _weight
		fonts[i].res = (u32)m_theme.getInt(general_domain, defValue);

		fonts[i].res = min(max(fonts[i].min, fonts[i].res <= 0 ? fonts[i].def : fonts[i].res), fonts[i].max);
	}

	/* Check if font is already in memory and the filename, size, spacing, and weight are the same if so return this font */
	std::vector<SFont>::iterator font_itr;
	for(font_itr = theme.fontSet.begin(); font_itr != theme.fontSet.end(); ++font_itr)
	{
		if(strncmp(filename.c_str(), font_itr->name, 127) == 0 && font_itr->fSize == fonts[0].res &&
				font_itr->lineSpacing == fonts[1].res && font_itr->weight && fonts[2].res)
			break;
	}
	if(font_itr != theme.fontSet.end()) return *font_itr;

	/* Font not found in memory, load it to create a new font unless useDefault font is specified */
	SFont retFont;
	if(!useDefault && retFont.fromFile(fmt("%s/%s", m_themeDataDir.c_str(), filename.c_str()), fonts[0].res, fonts[1].res, fonts[2].res, index, filename.c_str()))
	{
		//! theme font
		theme.fontSet.push_back(retFont);
		return retFont;
	}
	
	/* Try default font in imgs folder */
	if(retFont.fromBuffer(font_ttf, font_ttf_size, fonts[0].res, fonts[1].res, fonts[2].res, index, filename.c_str()))
	{
		//! default font
		theme.fontSet.push_back(retFont);
		return retFont;
	}
	
	/* Fallback to default font default font is the wii's system font */
	if(retFont.fromBuffer(m_base_font, m_base_font_size, fonts[0].res, fonts[1].res, fonts[2].res, index, filename.c_str()))
	{
		//! default font
		theme.fontSet.push_back(retFont);
		return retFont;
	}
	return retFont;
}

SFont CMenu::_font(const char *domain, const char *key, SFont def_font)
{
	string filename;
	FontHolder fonts[3] = {{ "_size", 6u, 300u, 0, 0 }, { "_line_height", 6u, 300u, 0, 0 }, { "_weight", 1u, 32u, 0, 0 }};

	filename = m_theme.getString(domain, key);
	if(filename.empty())
		filename = def_font.name;

	/* Get the resources - fontSize, lineSpacing, and weight */
	for(u32 i = 0; i < 3; i++)
	{
		string value = key;
		value += fonts[i].ext; // _size, _line_height, _weight

		fonts[i].res = (u32)m_theme.getInt(domain, value);
		if(fonts[i].res <= 0 && i == 0)
			fonts[i].res = def_font.fSize;
		else if(fonts[i].res <= 0 && i == 1)
			fonts[i].res = def_font.lineSpacing;
		else if(fonts[i].res <= 0 && i == 2)
			fonts[i].res = def_font.weight;

		fonts[i].res = min(max(fonts[i].min, fonts[i].res), fonts[i].max);
	}

	/* Check if font is already in memory and the filename, size, spacing, and weight are the same if so return this font */
	std::vector<SFont>::iterator font_itr;
	for(font_itr = theme.fontSet.begin(); font_itr != theme.fontSet.end(); ++font_itr)
	{
		if(strncmp(filename.c_str(), font_itr->name, 127) == 0 && font_itr->fSize == fonts[0].res &&
				font_itr->lineSpacing == fonts[1].res && font_itr->weight && fonts[2].res)
			break;
	}
	if(font_itr != theme.fontSet.end()) return *font_itr;

	/* Font not found in memory, load it to create a new font unless useDefault font is specified */
	SFont retFont;
	if(retFont.fromFile(fmt("%s/%s", m_themeDataDir.c_str(), filename.c_str()), fonts[0].res, fonts[1].res, fonts[2].res, 1, filename.c_str()))
	{
		//! theme font
		theme.fontSet.push_back(retFont);
		return retFont;
	}
	return def_font;
}

vector<TexData> CMenu::_textures(const char *domain, const char *key)
{
	vector<TexData> textures;

	if(m_theme.loaded())
	{
		vector<string> filenames = m_theme.getStrings(domain, key);
		if(filenames.size() > 0)
		{
			for(vector<string>::iterator itr = filenames.begin(); itr != filenames.end(); itr++)
			{
				const string &filename = *itr;
				TexSet::iterator i = theme.texSet.find(filename);
				if(i != theme.texSet.end())
					textures.push_back(i->second);
				TexData themetex;
				if(TexHandle.fromImageFile(themetex, fmt("%s/%s", m_themeDataDir.c_str(), filename.c_str())) == TE_OK)
				{
					theme.texSet[filename] = themetex;
					textures.push_back(themetex);
				}
			}
		}
	}
	return textures;
}

TexData CMenu::_texture(const char *domain, const char *key, TexData &def, bool freeDef)
{
	string filename;

	if(m_theme.loaded())
	{
		//! load from theme
		filename = m_theme.getString(domain, key);
		if(!filename.empty())
		{
			TexSet::iterator i = theme.texSet.find(filename);
			if(i != theme.texSet.end())
				return i->second;
			//! load from image file
			TexData themetex;
			if(TexHandle.fromImageFile(themetex, fmt("%s/%s", m_themeDataDir.c_str(), filename.c_str())) == TE_OK)
			{
				if(freeDef && def.data != NULL)
				{
					MEM2_free(def.data);
					def.data = NULL;
				}
				theme.texSet[filename] = themetex;
				return themetex;
			}
		}
	}
	//! fallback to default
	theme.texSet[filename] = def;
	return def;
}

/* Only for loading defaults and GENERAL domains!! */
GuiSound *CMenu::_sound(CMenu::SoundSet &soundSet, const char *filename, const u8 * snd, u32 len, const char *name, bool isAllocated)
{
	if(filename == NULL || filename[0] == '\0')
		filename = name;

	CMenu::SoundSet::iterator i = soundSet.find(upperCase(name));
	if(i == soundSet.end())
	{
		if(filename != name && fsop_FileExist(fmt("%s/%s", m_themeDataDir.c_str(), filename))) //
		{
			u32 size = 0;
			u8 *mem = fsop_ReadFile(fmt("%s/%s", m_themeDataDir.c_str(), filename), &size);
			soundSet[upperCase(filename)] = new GuiSound(mem, size, filename, true);
		}
		else
			soundSet[upperCase(filename)] = new GuiSound(snd, len, filename, isAllocated);
		return soundSet[upperCase(filename)];
	}
	return i->second;
}

/* For buttons and labels only!! */
GuiSound *CMenu::_sound(CMenu::SoundSet &soundSet, const char *domain, const char *key, const char *name)
{
	const char *filename = m_theme.getString(domain, key).c_str();
	if(filename == NULL || filename[0] == '\0')
	{
		if(strrchr(name, '/') != NULL)
			name = strrchr(name, '/') + 1;
		return soundSet[upperCase(name)];  // General/Default are already cached!
	}

	SoundSet::iterator i = soundSet.find(upperCase(filename));
	if(i == soundSet.end())
	{
		if(fsop_FileExist(fmt("%s/%s", m_themeDataDir.c_str(), filename)))
		{
			u32 size = 0;
			u8 *mem = fsop_ReadFile(fmt("%s/%s", m_themeDataDir.c_str(), filename), &size);
			soundSet[upperCase(filename)] = new GuiSound(mem, size, filename, true);
		}
		else
			soundSet[upperCase(filename)] = new GuiSound();
		return soundSet[upperCase(filename)];
	}
	return i->second;
}

u16 CMenu::_textStyle(const char *domain, const char *key, u16 def, bool coverflow)
{
	u16 textStyle = 0;
	string style;
	if(coverflow)
		style = m_coverflow.getString(domain, key);
	else
		style = m_theme.getString(domain, key);
	if(style.empty()) 
		return def;

	if(style.find_first_of("Cc") != string::npos)
		textStyle |= FTGX_JUSTIFY_CENTER;
	else if(style.find_first_of("Rr") != string::npos)
		textStyle |= FTGX_JUSTIFY_RIGHT;
	else
		textStyle |= FTGX_JUSTIFY_LEFT;
	if(style.find_first_of("Mm") != string::npos)
		textStyle |= FTGX_ALIGN_MIDDLE;
	else if(style.find_first_of("Bb") != string::npos)
		textStyle |= FTGX_ALIGN_BOTTOM;
	else
		textStyle |= FTGX_ALIGN_TOP;
	return textStyle;
}

s16 CMenu::_addButton(const char *domain, SFont font, const wstringEx &text, int x, int y, u32 width, u32 height, const CColor &color)
{
	SButtonTextureSet btnTexSet;
	CColor c(color);

	c = m_theme.getColor(domain, "color", c);
	x = m_theme.getInt(domain, "x", x);
	y = m_theme.getInt(domain, "y", y);
	width = m_theme.getInt(domain, "width", width);
	height = m_theme.getInt(domain, "height", height);
	btnTexSet.left = _texture(domain, "texture_left", theme.btnTexL, false);
	btnTexSet.right = _texture(domain, "texture_right", theme.btnTexR, false);
	btnTexSet.center = _texture(domain, "texture_center", theme.btnTexC, false);
	btnTexSet.leftSel = _texture(domain, "texture_left_selected", theme.btnTexLS, false);
	btnTexSet.rightSel = _texture(domain, "texture_right_selected", theme.btnTexRS, false);
	btnTexSet.centerSel = _texture(domain, "texture_center_selected", theme.btnTexCS, false);
	font = _font(domain, "font", font);
	GuiSound *clickSound = _sound(theme.soundSet, domain, "click_sound", theme.clickSound->GetName());
	GuiSound *hoverSound = _sound(theme.soundSet, domain, "hover_sound", theme.hoverSound->GetName());
	return m_btnMgr.addButton(font, text, x, y, width, height, c, btnTexSet, clickSound, hoverSound);
}

s16 CMenu::_addPicButton(const char *domain, TexData &texNormal, TexData &texSelected, int x, int y, u32 width, u32 height)
{
	x = m_theme.getInt(domain, "x", x);
	y = m_theme.getInt(domain, "y", y);
	width = m_theme.getInt(domain, "width", width);
	height = m_theme.getInt(domain, "height", height);
	TexData tex1 = _texture(domain, "texture_normal", texNormal, false);
	TexData tex2 = _texture(domain, "texture_selected", texSelected, false);
	GuiSound *clickSound = _sound(theme.soundSet, domain, "click_sound", theme.clickSound->GetName());
	GuiSound *hoverSound = _sound(theme.soundSet, domain, "hover_sound", theme.hoverSound->GetName());
	return m_btnMgr.addPicButton(tex1, tex2, x, y, width, height, clickSound, hoverSound);
}

s16 CMenu::_addTitle(const char *domain, SFont font, const wstringEx &text, int x, int y, u32 width, u32 height, const CColor &color, s16 style)
{
	CColor c(color);

	c = m_theme.getColor(domain, "color", c);
	x = m_theme.getInt(domain, "x", x);
	y = m_theme.getInt(domain, "y", y);
	width = m_theme.getInt(domain, "width", width);
	height = m_theme.getInt(domain, "height", height);
	font = _font(domain, "font", font);
	style = _textStyle(domain, "style", style);
	return m_btnMgr.addLabel(font, text, x, y, width, height, c, style);
}

s16 CMenu::_addText(const char *domain, SFont font, const wstringEx &text, int x, int y, u32 width, u32 height, const CColor &color, s16 style)
{
	CColor c(color);

	c = m_theme.getColor(domain, "color", c);
	x = m_theme.getInt(domain, "x", x);
	y = m_theme.getInt(domain, "y", y);
	width = m_theme.getInt(domain, "width", width);
	height = m_theme.getInt(domain, "height", height);
	font = _font(domain, "font", font);
	style = _textStyle(domain, "style", style);
	return m_btnMgr.addLabel(font, text, x, y, width, height, c, style);
}

s16 CMenu::_addLabel(const char *domain, SFont font, const wstringEx &text, int x, int y, u32 width, u32 height, const CColor &color, s16 style)
{
	CColor c(color);

	c = m_theme.getColor(domain, "color", c);
	x = m_theme.getInt(domain, "x", x);
	y = m_theme.getInt(domain, "y", y);
	width = m_theme.getInt(domain, "width", width);
	height = m_theme.getInt(domain, "height", height);
	font = _font(domain, "font", font);
	style = _textStyle(domain, "style", style);
	return m_btnMgr.addLabel(font, text, x, y, width, height, c, style);
}

s16 CMenu::_addLabel(const char *domain, SFont font, const wstringEx &text, int x, int y, u32 width, u32 height, const CColor &color, s16 style, TexData &bg)
{
	CColor c(color);

	c = m_theme.getColor(domain, "color", c);
	x = m_theme.getInt(domain, "x", x);
	y = m_theme.getInt(domain, "y", y);
	width = m_theme.getInt(domain, "width", width);
	height = m_theme.getInt(domain, "height", height);
	font = _font(domain, "font", font);
	TexData texBg = _texture(domain, "background_texture", bg, false);
	style = _textStyle(domain, "style", style);
	return m_btnMgr.addLabel(font, text, x, y, width, height, c, style, texBg);
}

s16 CMenu::_addProgressBar(const char *domain, int x, int y, u32 width, u32 height)
{
	SButtonTextureSet btnTexSet;

	x = m_theme.getInt(domain, "x", x);
	y = m_theme.getInt(domain, "y", y);
	width = m_theme.getInt(domain, "width", width);
	height = m_theme.getInt(domain, "height", height);
	btnTexSet.left = _texture(domain, "texture_left", theme.pbarTexL, false);
	btnTexSet.right = _texture(domain, "texture_right", theme.pbarTexR, false);
	btnTexSet.center = _texture(domain, "texture_center", theme.pbarTexC, false);
	btnTexSet.leftSel = _texture(domain, "texture_left_selected", theme.pbarTexLS, false);
	btnTexSet.rightSel = _texture(domain, "texture_right_selected", theme.pbarTexRS, false);
	btnTexSet.centerSel = _texture(domain, "texture_center_selected", theme.pbarTexCS, false);
	return m_btnMgr.addProgressBar(x, y, width, height, btnTexSet);
}

void CMenu::_setHideAnim(s16 id, const char *domain, int dx, int dy, float scaleX, float scaleY)
{
	dx = m_theme.getInt(domain, "effect_x", dx);
	dy = m_theme.getInt(domain, "effect_y", dy);
	scaleX = m_theme.getFloat(domain, "effect_scale_x", scaleX);
	scaleY = m_theme.getFloat(domain, "effect_scale_y", scaleY);
	m_btnMgr.hide(id, dx, dy, scaleX, scaleY, true);
}

void CMenu::_addUserLabels(s16 *ids, u32 size, const char *domain)
{
	_addUserLabels(ids, 0, size, domain);
}

void CMenu::_addUserLabels(s16 *ids, u32 start, u32 size, const char *domain)
{
	for(u32 i = start; i < start + size; ++i)
	{
		string dom(fmt("%s/USER%i", domain, i + 1));
		if(m_theme.hasDomain(dom))
		{
			TexData emptyTex;
			ids[i] = _addLabel(dom.c_str(), theme.lblFont, L"", 40, 200, 64, 64, CColor(0xFFFFFFFF), 0, emptyTex);
			_setHideAnim(ids[i], dom.c_str(), -50, 0, 0.f, 0.f);
		}
		else
			ids[i] = -1;
	}
}

bool musicPaused = false;

void CMenu::_mainLoopCommon(bool withCF, bool adjusting)
{
	if(m_thrdWorking)
	{
		musicPaused = true;
		MusicPlayer.Pause(); // note - bg music is paused but sound thread is still running. so banner gamesound still plays
		m_btnMgr.tick();
		m_vid.prepare();
		m_vid.setup2DProjection(false, true); // false = prepare() already set view port, true = no scaling - draw at 640x480
		_updateBg();
		if(CoverFlow.getRenderTex()) //
			CoverFlow.RenderTex(); //
		m_vid.setup2DProjection(); // this time set the view port and allow scaling
		_drawBg();
		m_btnMgr.draw();
		m_vid.render();
		return;
	}
	if(musicPaused && !m_thrdWorking)
	{
		musicPaused = false;
		MusicPlayer.Resume();
	}
	
	/* Ticks - for moving and scaling covers and GUI buttons and text */
	if(withCF)
		CoverFlow.tick();
	m_btnMgr.tick();
	m_fa.tick();

	/* Video setup */
	m_vid.prepare();
	m_vid.setup2DProjection(false, true);
	
	/* Background and coverflow drawing */
	_updateBg();
	if(CoverFlow.getRenderTex()) //
		CoverFlow.RenderTex(); //
	if(withCF && m_lqBg != NULL)
		CoverFlow.makeEffectTexture(m_lqBg);
	if(withCF && m_aa > 0)
	{
		m_vid.setAA(m_aa, true);
		for(int i = 0; i < m_aa; ++i)
		{
			m_vid.prepareAAPass(i);
			m_vid.setup2DProjection(false, true);
			_drawBg();
			CoverFlow.draw();
			m_vid.setup2DProjection(false, true);
			CoverFlow.drawEffect();
			if(!m_soundThrdBusy && !m_banner.GetSelectedGame() && !m_snapshot_loaded)
				CoverFlow.drawText(adjusting);
			m_vid.renderAAPass(i);
		}
		m_vid.setup2DProjection();
		m_vid.drawAAScene();
	}
	else
	{
		m_vid.setup2DProjection();
		_drawBg();
		if(withCF)
		{
			CoverFlow.draw();
			m_vid.setup2DProjection();
			CoverFlow.drawEffect();
			if(!m_soundThrdBusy && !m_banner.GetSelectedGame() && !m_snapshot_loaded)
				CoverFlow.drawText(adjusting);
		}
	}
	
	/* Game video or banner drawing */
	if(m_gameSelected)
	{
		if(m_fa.isLoaded())
			m_fa.draw();
		else if(m_video_playing)
		{
			if(movie.Frame != NULL)
			{
				DrawTexturePos(movie.Frame, m_zoom_video);
				movie.Frame->thread = false;
			}
		}
		else if(m_banner.GetSelectedGame() && (!m_banner.GetInGameSettings() || (m_banner.GetInGameSettings() && m_bnr_settings)))
			m_banner.Draw();
	}

	/* GUI buttons and text drawing */
	m_btnMgr.draw();
	
	/* Reading controller inputs and drawing cursor pointers */	
	ScanInput(enlargeButtons);

	/* Screensaver, draw full screen black square with mild alpha */	
	if(m_screensaverDelay > 0)
		m_vid.screensaver(NoInputTime(), m_screensaverDelay);

	/* Render everything on screen */
	m_vid.render();
	
	/* Check if power or reset button is pressed and exit wiiflow */
	u8 PowerReset = Sys_Exiting();
	if(PowerReset > 0)
		exitHandler(PowerReset);
	
	/* Check if we need to start playing the game/banner sound
		m_gameSelected means we are on the game selected menu
		m_gamesound_changed means a new game sound is loaded and ready to play
		the previous game sound needs to stop before playing new sound
		and the bg music volume needs to be 0 before playing game sound */
	if(withCF && m_gameSelected && m_gamesound_changed && !m_gameSound.IsPlaying() && MusicPlayer.GetVolume() == 0)
	{
		_stopGameSoundThread(); // stop game sound loading thread
		m_gameSound.Play(m_bnrSndVol); // play game sound
		m_gamesound_changed = false;
	}
	/* Stop game/banner sound from playing if we exited game selected menu or if we move to new game */
	else if((withCF && m_gameSelected && m_gamesound_changed && m_gameSound.IsPlaying()) || (!m_gameSelected && m_gameSound.IsPlaying()))
		m_gameSound.Stop();

	/* Decrease music volume to zero if any of these are true:
		trailer video playing or||
		game/banner sound is being loaded because we are switching to a new game or||
		game/banner sound is loaded and ready to play or||
		gamesound hasn't finished - when finishes music volume back to normal - some gamesounds don't loop continuously
		also this switches to next song if current song is done */
	MusicPlayer.Tick((withCF && (m_video_playing || (m_gameSelected && m_soundThrdBusy) || 
						(m_gameSelected && m_gamesound_changed))) ||  m_gameSound.IsPlaying());

	/* Set song title and display it if music info is allowed */
	// if(MusicPlayer.SongChanged() && m_music_info && !MusicPlayer.OneSong && withCF)
	// {
		// m_btnMgr.setText(m_mainLblCurMusic, MusicPlayer.GetFileName(), false); // false for word wrap
		// m_btnMgr.show(m_mainLblCurMusic);
		// MusicPlayer.DisplayTime = time(NULL);
	// }
	/* Hide song title if it's displaying and been >3 seconds */
	// else if(MusicPlayer.DisplayTime > 0 && time(NULL) - MusicPlayer.DisplayTime > 3)
	// {
		// MusicPlayer.DisplayTime = 0;
		// m_btnMgr.hide(m_mainLblCurMusic);
		// if(MusicPlayer.OneSong) m_music_info = false;
	// }
#ifdef SCREENSHOT	
	/* Take Screenshot */
	if(WBTN_Z_PRESSED || GBTN_Z_PRESSED)
	{
		time_t rawtime;
		struct tm *timeinfo;
		char buffer[80];

		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime(buffer,80,"%Y-%m-%d-%Hh%Mm%Ss.png",timeinfo);
		// gprintf("Screenshot taken and saved to: %s/%s\n", m_screenshotDir.c_str(), buffer);
		m_vid.TakeScreenshot(fmt("%s/%s", m_screenshotDir.c_str(), buffer));
		if(theme.cameraSound != NULL)
			theme.cameraSound->Play(255);
	}
#endif
	if(show_mem)
	{
		m_btnMgr.setText(m_mem1FreeSize, wfmt(L"Mem1 lo Free:%u, Mem1 Free:%u", MEM1_lo_freesize(), MEM1_freesize()), true);
		m_btnMgr.setText(m_mem2FreeSize, wfmt(L"Mem2 Free:%u", MEM2_freesize()), true);
	}

#ifdef SHOWMEMGECKO
	mem1 = MEM1_freesize();
	mem2 = MEM2_freesize();
	if(mem1 != mem1old)
	{
		mem1old = mem1;
		gprintf("Mem1 Free: %u\n", mem1);
	}
	if(mem2 != mem2old)
	{
		mem2old = mem2;
		gprintf("Mem2 Free: %u\n", mem2);
	}
#endif
}

void CMenu::_setBg(const TexData &bgTex, const TexData &bglqTex, bool force_change)
{
	/* Not setting same bg again */
	if(!force_change && m_nextBg == &bgTex)
		return;
	m_lqBg = &bglqTex;
	
	/* Before setting new next bg set previous */
	if(m_nextBg != NULL)
		m_prevBg = m_nextBg;
	m_nextBg = &bgTex;
	m_bgCrossFade = 0xFF;
}

void CMenu::_updateBg(void)
{
	if(m_bgCrossFade == 0)
		return;
	m_bgCrossFade = max(0, (int)m_bgCrossFade - 14);

	Mtx modelViewMtx;
	GXTexObj texObj;
	GXTexObj texObj2;

	/* Last pass so remove previous bg */
	if(m_bgCrossFade == 0)
		m_prevBg = NULL;

	GX_ClearVtxDesc();
	GX_SetNumTevStages(m_prevBg == NULL ? 1 : 2);
	GX_SetNumChans(0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GX_SetNumTexGens(m_prevBg == NULL ? 1 : 2);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
	GX_SetTexCoordGen(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
	GX_SetTevKColor(GX_KCOLOR0, CColor(m_bgCrossFade, 0xFF - m_bgCrossFade, 0, 0));
	GX_SetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K0_R);
	GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_KONST, GX_CC_ZERO);
	GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
	GX_SetTevKColorSel(GX_TEVSTAGE1, GX_TEV_KCSEL_K0_G);
	GX_SetTevColorIn(GX_TEVSTAGE1, GX_CC_TEXC, GX_CC_ZERO, GX_CC_KONST, GX_CC_CPREV);
	GX_SetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
	GX_SetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLORNULL);
	GX_SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetAlphaUpdate(GX_FALSE);
	GX_SetCullMode(GX_CULL_NONE);
	GX_SetZMode(GX_DISABLE, GX_ALWAYS, GX_FALSE);
	guMtxIdentity(modelViewMtx);
	GX_LoadPosMtxImm(modelViewMtx, GX_PNMTX0);
	if(m_nextBg != NULL && m_nextBg->data != NULL)
	{
		GX_InitTexObj(&texObj, m_nextBg->data, m_nextBg->width, m_nextBg->height, m_nextBg->format, GX_CLAMP, GX_CLAMP, GX_FALSE);
		GX_LoadTexObj(&texObj, GX_TEXMAP0);
	}
	if(m_prevBg != NULL && m_prevBg->data != NULL)
	{
		GX_InitTexObj(&texObj2, m_prevBg->data, m_prevBg->width, m_prevBg->height, m_prevBg->format, GX_CLAMP, GX_CLAMP, GX_FALSE);
		GX_LoadTexObj(&texObj2, GX_TEXMAP1);
	}
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
	GX_Position3f32(0.f, 0.f, 0.f);
	GX_TexCoord2f32(0.f, 0.f);
	GX_Position3f32(640.f, 0.f, 0.f);
	GX_TexCoord2f32(1.f, 0.f);
	GX_Position3f32(640.f, 480.f, 0.f);
	GX_TexCoord2f32(1.f, 1.f);
	GX_Position3f32(0.f, 480.f, 0.f);
	GX_TexCoord2f32(0.f, 1.f);
	GX_End();
	GX_SetNumTevStages(1);
	m_curBg.width = 640;
	m_curBg.height = 480;
	m_curBg.format = GX_TF_RGBA8;
	m_curBg.maxLOD = 0;
	m_vid.renderToTexture(m_curBg, true);
}

void CMenu::_drawBg(void)
{
	Mtx modelViewMtx;
	GXTexObj texObj;

	GX_ClearVtxDesc();
	GX_SetNumTevStages(1);
	GX_SetNumChans(0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GX_SetNumTexGens(1);
	GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
	GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
	GX_SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetAlphaUpdate(GX_FALSE);
	GX_SetCullMode(GX_CULL_NONE);
	GX_SetZMode(GX_DISABLE, GX_ALWAYS, GX_FALSE);
	guMtxIdentity(modelViewMtx);
	GX_LoadPosMtxImm(modelViewMtx, GX_PNMTX0);
	GX_InitTexObj(&texObj, m_curBg.data, m_curBg.width, m_curBg.height, m_curBg.format, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_LoadTexObj(&texObj, GX_TEXMAP0);
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
	GX_Position3f32(0.f, 0.f, 0.f);
	GX_TexCoord2f32(0.f, 0.f);
	GX_Position3f32(640.f, 0.f, 0.f);
	GX_TexCoord2f32(1.f, 0.f);
	GX_Position3f32(640.f, 480.f, 0.f);
	GX_TexCoord2f32(1.f, 1.f);
	GX_Position3f32(0.f, 480.f, 0.f);
	GX_TexCoord2f32(0.f, 1.f);
	GX_End();
}

void CMenu::_updateText(void)
{
	_textConfig();
	_textInfoMain();
	_textInfoGame();
	_textGame();
	_textHome();
	_textExitTo();
}

const wstringEx CMenu::_fmt(const char *key, const wchar_t *def)
{
	wstringEx ws = m_loc.getWString(m_curLanguage, key, def);
	if(checkFmt(def, ws))
		return ws;
	return def;
}

/** Full set of game settings to be cleared if their value is default **/
static const char gameSetting[34][18] =
{
	//! [0] to [14] - default to "false" game settings:
	"useneek", // not used anymore, using emulate_save=3 instead
	"apploader", 
	"cheat", 
	"country_patch",
	"custom", 
	"led", 
	"deflicker", 
	"patch_pal50", 
	"skip_ipl", 
	"triforce_arcade", 
	"vipatch", 
	"bba_emu", 
	"tempregionrn",
	"alt_cover",
	"alt_disc",
	//! [15] to [19] - default to "2" game settings:
	"wiiu_widescreen",
	"widescreen", 
	"cc_rumble",
	"native_ctl", 
	"fix480p",
	//! [20] to [31] - default to "0" game settings:
	"aspect_ratio", 
	"debugger", 
	"hooktype", 
	"language", 
	"patch_video_modes", 
	"private_server", 
	"video_mode",
	"emu_memcard", 
	"ios", 
	"net_profile", 
	"emulate_save", 
	"deflicker_wii", 
	//! [32] to [33] - default to "127" game settings:
	"nin_pos", 
	"nin_width"
};

void CMenu::_dumpGameList(void)
{
	if(CoverFlow.empty())
		return;
	char dumpFile[48];
	memset(dumpFile, 0, 48);
	strncpy(dumpFile, fmt("%s/%s",  m_settingsDir.c_str(), TITLES_DUMP_FILENAME), 47);
	if(m_cfg.getBool(general_domain, "overwrite_dump_list", false))
		fsop_deleteFile(dumpFile);
	_initCF(true);
	_error(wfmt(_fmt("errdump1", L"%s saved. To make it a custom title file, rename it to %s."), dumpFile, CTITLES_FILENAME));
}

void CMenu::_initCF(bool dumpGameList)
{
	Config dump;
	if(dumpGameList)
		dump.load(fmt("%s/" TITLES_DUMP_FILENAME, m_settingsDir.c_str()));

	CoverFlow.clear();
	CoverFlow.reserve(m_gameList.size());

	string requiredCats = m_cat.getString(general_domain, "required_categories", "");
	string selectedCats = m_cat.getString(general_domain, "selected_categories", "");
	string hiddenCats = m_cat.getString(general_domain, "hidden_categories", "");
	u8 numReqCats = requiredCats.length();
	u8 numSelCats = selectedCats.length();
	u8 numHidCats = hiddenCats.length();
	
	char id[74];
	char catID[64];	

	//! filter list based on categories and favorites
	for(vector<dir_discHdr>::iterator hdr = m_gameList.begin(); hdr != m_gameList.end(); ++hdr)
	{
		const char *favDomain = "FAVORITES";
		const char *adultDomain = "ADULTONLY";
		memset(id, 0, 74);
		memset(catID, 0, 64);
		if(m_sourceflow)
		{
			if(m_source.getBool(sfmt("button_%i", hdr->settings[0]), "hidden", false) == false)
				CoverFlow.addItem(&(*hdr), 0, 0);
			continue;
		}
		else if(hdr->type == TYPE_HOMEBREW)
		{
			wcstombs(id, hdr->title, 63);
			strcpy(catID, id);
		}
		else if(hdr->type == TYPE_PLUGIN)
		{
			strncpy(m_plugin.PluginMagicWord, fmt("%08x", hdr->settings[0]), 8);
			string romFileName(hdr->path);
			romFileName = romFileName.substr(romFileName.find_last_of("/") + 1);
			romFileName = romFileName.substr(0, romFileName.find_last_of("."));
			strncpy(catID, romFileName.c_str(), 63);
			strcpy(id, m_plugin.PluginMagicWord);
			strcat(id, fmt("/%s", catID));
			favDomain = "FAVORITES_PLUGINS";
			adultDomain = "ADULTONLY_PLUGINS";
		}
		else // wii, gc, channels
		{
			strcpy(id, hdr->id);
			strcpy(catID, id);
		}
		
		if((!m_favorites || m_gcfg1.getBool(favDomain, id, false)) && (!m_locked || !m_gcfg1.getBool(adultDomain, id, false)))
		{
			string catDomain = "";
			if(hdr->type == TYPE_CHANNEL)
				catDomain = "NAND";
			else if(hdr->type == TYPE_EMUCHANNEL)
				catDomain = "CHANNELS";
			else if(hdr->type == TYPE_GC_GAME)
				catDomain = "GAMECUBE";
			else if(hdr->type == TYPE_WII_GAME)
				catDomain = "WII";
			else if(hdr->type == TYPE_HOMEBREW)
				catDomain = "HOMEBREW";			
			else
				catDomain = m_plugin.PluginMagicWord;

			if(numReqCats != 0 || numSelCats != 0 || numHidCats != 0) // if all 0 skip checking cats and show all games
			{
				string idCats= m_cat.getString(catDomain, catID, "");
				u8 numIdCats = idCats.length();
				
				/* Removing games from categories_lite.ini if it doesn't have any category set.
				This should keep categories_lite.ini small and less cluttered */
				if(numIdCats == 0)
					m_cat.remove(catDomain, catID);

				bool inaCat = false;
				bool inHiddenCat = false;
				int reqMatch = 0;
				if(numIdCats != 0)
				{
					for(u8 j = 0; j < numIdCats; ++j)
					{
						int k = (static_cast<int>(idCats[j])) - 32;
						if(k <= 0)
							continue;
						bool match = false;
						if(numReqCats != 0)
						{
							for(u8 l = 0; l < numReqCats; ++l)
							{
								if(k == (static_cast<int>(requiredCats[l]) - 32))
								{
									match = true;
									reqMatch++;
									inaCat = true;
								}
							}
						}
						if(match)
							continue;
						if(numSelCats != 0)
						{
							for(u8 l = 0; l < numSelCats; ++l)
							{
								if(k == (static_cast<int>(selectedCats[l]) - 32))
								{
									match = true;
									inaCat = true;
								}
							}
						}
						if(match)
							continue;
						if(numHidCats != 0)
						{
							for(u8 l = 0; l < numHidCats; ++l)
							{
								if(k == (static_cast<int>(hiddenCats[l]) - 32))
									inHiddenCat = true;
							}
						}
					}
				}
				
				if(inHiddenCat)
					continue; // means don't add game to list (don't show)
				if(numReqCats != reqMatch)
					continue;
				if(!inaCat)
				{
					if(numHidCats == 0)
						continue;
					else if(numSelCats > 0)
						continue;
				}
			}
			
			if(dumpGameList)
			{
				const char *domain = NULL;
				switch(hdr->type)
				{
					case TYPE_WII_GAME:
					case TYPE_CHANNEL:
					case TYPE_EMUCHANNEL:
					case TYPE_GC_GAME:
					case TYPE_HOMEBREW:
						domain = "TITLES";
						break;
					default:
						domain = m_plugin.PluginMagicWord;
						break;
				}
				dump.setWString(domain, catID, hdr->title);
			}
			
			if((hdr->type == TYPE_PLUGIN && m_plugin.GetEnabledStatus(m_plugin.GetPluginPosition(hdr->settings[0])))
				|| (hdr->type != TYPE_PLUGIN))
			{
				int playcount = 0;
				unsigned int lastPlayed = 0;
				if(hdr->type != TYPE_HOMEBREW)
				{
					playcount = m_gcfg1.getInt("PLAYCOUNT", id, 0);
					lastPlayed = m_gcfg1.getUInt("LASTPLAYED", id, 0);
				}				
				CoverFlow.addItem(&(*hdr), playcount, lastPlayed);				
			}		
		}

		/** Apply PAL60 video mode settings to NTSC games for RGB wiring **/
		/**
		if(m_gcfg2.getInt(id, "video_mode", 0) == 0)
		{
			if((hdr->type == TYPE_WII_GAME || hdr->type == TYPE_EMUCHANNEL) && (id[3] == 'E' || id[3] == 'J'))
				m_gcfg2.setInt(id, "video_mode", 4);
			else if(hdr->type == TYPE_GC_GAME && (id[3] == 'E' || id[3] == 'J'))
				m_gcfg2.setInt(id, "video_mode", 3);
		}
		**/

		/* Remove favorites if false to keep file short */
		if(!m_gcfg1.getBool(favDomain, id))
			m_gcfg1.remove(favDomain, id);
		if(!m_gcfg1.getBool(adultDomain, id))
			m_gcfg1.remove(adultDomain, id);
		
		/* Remove also playcount and lastplayed (won't clean games launched 
			from plugin explorer that are not listed in coverflow) */
		if(m_gcfg1.getInt("PLAYCOUNT", id) == 0)
		{
			m_gcfg1.remove("PLAYCOUNT", id);
			m_gcfg1.remove("LASTPLAYED", id);
		}
		
		/* Update former useneek game setting to new emulate_save setting for neek2o */
		if(m_gcfg2.getBool(id, gameSetting[0], false)) // if useneek=yes
			m_gcfg2.setInt(id, gameSetting[28], 3); // set emulate_save=3
				
		/* Remove default game settings */
		u8 i = 0;
		bool emptyDom = true;
		//! "yes/no" settings: remove if "no"
		for(i = 0; i < 15; i++)
		{
			if(!m_gcfg2.getBool(id, gameSetting[i]))
				m_gcfg2.remove(id, gameSetting[i]);
		}
		//! "0/1/2" (off/on/default) settings: remove if 2
		for(i = 15; i < 20; i++)
		{
			if(m_gcfg2.getOptBool(id, gameSetting[i]) == 2)
				m_gcfg2.remove(id, gameSetting[i]);
		}
		//! integer value settings: remove if 0
		for(i = 20; i < 32; i++)
		{
			if(m_gcfg2.getInt(id, gameSetting[i]) == 0)
				m_gcfg2.remove(id, gameSetting[i]);
		}
		//! nin_pos & nin_width settings: remove if 127
		for(i = 32; i < 34; i++)
		{
			if(m_gcfg2.getInt(id, gameSetting[i], 127) == 127) // (0 is "auto" for nin_width)
				m_gcfg2.remove(id, gameSetting[i]);
		}
		//! look for empty domains
		for(i = 0; i < 34; i++)
		{
			if(m_gcfg2.has(id, gameSetting[i]))
				emptyDom = false;
		}
		//! remove empty domains
		if(emptyDom)
			m_gcfg2.removeDomain(id);
	}
	
	if(CoverFlow.empty())
		return;
	
	/* Dump game list to dev:/wiiflow/settings */
	if(dumpGameList)
		dump.save(true);

/*************************************** sort coverflow list *******************************************/

	CoverFlow.setSorting((Sorting)m_cfg.getInt(_domainFromView(), "sort", 0));

	if(!m_sourceflow)
	{
		bool defaultBoxMode = m_cfg.getBool(general_domain, "box_mode", true); //
		if((m_current_view == COVERFLOW_HOMEBREW) ||
		(m_current_view == COVERFLOW_PLUGIN && enabledPluginsCount == 1 &&
		m_plugin.GetEnabledStatus(m_plugin.GetPluginPosition(0x48425257))))
		{
			CoverFlow.setBoxMode(m_cfg.getBool(homebrew_domain, "box_mode", false));
			CoverFlow.setSmallBoxMode(m_cfg.getBool(homebrew_domain, "smallbox", true));
		}
		else if(m_current_view == COVERFLOW_PLUGIN)
		{
			if(enabledPluginsCount == 1) // only one plugin enabled
			{
				s8 bm = -1;
				for(u8 i = 0; m_plugin.PluginExist(i); ++i)
				{
					if(m_plugin.GetEnabledStatus(i))
					{
						bm = m_plugin.GetBoxMode(i);
						break;
					}
				}
				if(bm < 0) // if negative then use default setting
					CoverFlow.setBoxMode(defaultBoxMode);
				else 
					CoverFlow.setBoxMode(bm == 0 ? false : true);
				CoverFlow.setSmallBoxMode(false);
			}
			else // more than 1 plugin enabled
			{
				s8 bm1 = -1;
				s8 bm2 = -1;
				bool all_same = true;
				for(u8 i = 0; m_plugin.PluginExist(i); ++i)
				{
					if(m_plugin.GetEnabledStatus(i))
					{
						if(bm1 == -1)
						{
							bm1 = m_plugin.GetBoxMode(i);
							if(bm1 < 0)
								bm1 = defaultBoxMode ? 1 : 0;
						}
						else
						{
							bm2 = m_plugin.GetBoxMode(i);
							if(bm2 < 0)
								bm2 = defaultBoxMode ? 1 : 0;
							if(bm2 != bm1)
							{
								all_same = false;
								break;
							}
						}
					}
				}
				if(!all_same)
					CoverFlow.setBoxMode(defaultBoxMode);
				else
					CoverFlow.setBoxMode(bm1 == 0 ? false : true);
				CoverFlow.setSmallBoxMode(false);
			}
		}
		else
		{
			CoverFlow.setBoxMode(defaultBoxMode);
			CoverFlow.setSmallBoxMode(false);
		}
	}
	else // sourceflow
	{
		CoverFlow.setBoxMode(m_cfg.getBool(SOURCEFLOW_DOMAIN, "box_mode", true));
		CoverFlow.setSmallBoxMode(m_cfg.getBool(SOURCEFLOW_DOMAIN, "smallbox", false));
	}
/*********************************** Setup coverflow covers settings ***********************************/
	CoverFlow.setBufferSize(m_cfg.getInt(general_domain, "cover_buffer", 20));
	CoverFlow.setHQcover(m_cfg.getBool(general_domain, "cover_use_hq", true));
	CoverFlow.start();
/**************************** Get and set game list current item to center cover ***********************/
	if(!CoverFlow.empty())
	{
		/* get ID or filename or source number of center cover */
		string ID = "", filename = "";
		u32 sourceNumber = 0;
		if(m_current_view == COVERFLOW_PLUGIN && !m_sourceflow)
		{
			if(!m_plugin.GetEnabledStatus(m_plugin.GetPluginPosition(strtoul(m_cfg.getString(plugin_domain, "cur_magic", "00000000").c_str(), NULL, 16))))
			{
				for(u8 i = 0; m_plugin.PluginExist(i); ++i)
				{
					if(m_plugin.GetEnabledStatus(i))
					{
						m_cfg.setString(plugin_domain, "cur_magic", sfmt("%08x", m_plugin.GetPluginMagic(i)));
						break;
					}
				}
			}
			
			strncpy(m_plugin.PluginMagicWord, m_cfg.getString(plugin_domain, "cur_magic").c_str(), 8);
			
			if(strncasecmp(m_plugin.PluginMagicWord, "4E47434D", 8) == 0) // NGCM
				ID = m_cfg.getString("plugin_item", m_plugin.PluginMagicWord, "");
			else if(strncasecmp(m_plugin.PluginMagicWord, "4E574949", 8) == 0) // NWII
				ID = m_cfg.getString("plugin_item", m_plugin.PluginMagicWord, "");
			else if(strncasecmp(m_plugin.PluginMagicWord, "4E414E44", 8) == 0) // NAND
				ID = m_cfg.getString("plugin_item", m_plugin.PluginMagicWord, "");
			else if(strncasecmp(m_plugin.PluginMagicWord, "454E414E", 8) == 0) // ENAN
				ID = m_cfg.getString("plugin_item", m_plugin.PluginMagicWord, "");
			else
				filename = m_cfg.getString("plugin_item", m_plugin.PluginMagicWord, "");
		}
		else if(m_sourceflow && sm_numbers.size() > 0)
			sourceNumber = stoi(sm_numbers[sm_numbers.size() - 1]);
		else if(m_current_view == COVERFLOW_HOMEBREW) 
			filename = m_cfg.getString(_domainFromView(), "current_item", "");
		else
			ID = m_cfg.getString(_domainFromView(), "current_item", "");

		/* Set center cover as coverflow current position */
		if(!CoverFlow._setCurPosToCurItem(ID.c_str(), filename.c_str(), sourceNumber, true))
			CoverFlow._setCurPos(0); // if not found set first cover as coverflow current position
/****************************** Create and start the cover loader thread *******************************/
		CoverFlow.startCoverLoader();
	}
}

bool CMenu::_loadList(void)
{
	CoverFlow.clear(); // clears filtered list (m_items), cover list (m_covers), and cover textures and stops coverloader
	m_gameList.clear();
	vector<dir_discHdr>().swap(m_gameList);
	NANDemuView = false;
	
	_hideWaitMessage();
	if(m_sourceflow)
	{
		m_cacheList.createSFList(m_max_source_btn, m_source, m_sourceDir);
		for(vector<dir_discHdr>::iterator tmp_itr = m_cacheList.begin(); tmp_itr != m_cacheList.end(); tmp_itr++)
			m_gameList.push_back(*tmp_itr);
		m_cacheList.Clear();
		string fn = tiers[tiers.size() - 1];
		if(m_cfg.getBool("SOURCEFLOW_CACHE", fn, true))
		{
			m_cfg.setBool("SOURCEFLOW_CACHE", fn, false);
			cacheCovers = true;
		}
		return true;
	}
	// gprintf("Creating Gamelist\n");
	if(m_current_view & COVERFLOW_PLUGIN)
		_loadPluginList();
	if(m_current_view & COVERFLOW_WII)
		_loadWiiList();
	if(m_current_view & COVERFLOW_CHANNEL)
		_loadChannelList();
	if(m_current_view & COVERFLOW_GAMECUBE)
		_loadGamecubeList();
	if(m_current_view & COVERFLOW_HOMEBREW)
		_loadHomebrewList(HOMEBREW_DIR);
	m_cacheList.Clear();
	_hideWaitMessage();
	// gprintf("Games found: %i\n", m_gameList.size());
	return m_gameList.size() > 0 ? true : false;
}

bool CMenu::_loadWiiList(void)
{
	currentPartition = m_cfg.getInt(wii_domain, "partition", USB1);
	if(!DeviceHandle.IsInserted(currentPartition))
		return false;
	// gprintf("Adding wii list\n");
	DeviceHandle.OpenWBFS(currentPartition);
	string gameDir(fmt(wii_games_dir, DeviceName[currentPartition]));
	string cacheDir(fmt("%s/%s_wii.db", m_listCacheDir.c_str(), DeviceName[currentPartition]));
	bool updateCache = m_cfg.getBool(wii_domain, "update_cache");
	if(updateCache || !fsop_FileExist(cacheDir.c_str()))
	{
		m_vid.loadListImage();
		cacheCovers = true;
	}
	m_cacheList.CreateList(COVERFLOW_WII, currentPartition, gameDir, stringToVector(".wbfs|.iso", '|'), cacheDir, updateCache);
	WBFS_Close();
	m_cfg.remove(wii_domain, "update_cache");
	for(vector<dir_discHdr>::iterator tmp_itr = m_cacheList.begin(); tmp_itr != m_cacheList.end(); tmp_itr++)
	{
		m_gameList.push_back(*tmp_itr);
		if(updateCache && tdb_genres)
			_setTDBCategories(&(*(tmp_itr)));
	}
	return true;
}

bool CMenu::_loadHomebrewList(const char *HB_Dir)
{
	currentPartition = m_cfg.getInt(homebrew_domain, "partition", SD);
	if(!DeviceHandle.IsInserted(currentPartition))
		return false;
	// gprintf("Adding homebrew list\n");
	string gameDir(fmt("%s:/%s", DeviceName[currentPartition], HB_Dir));
	//! small fix if HB_Dir is a subfolder replace '/' with '_' in the cache list name
	int i = 0;
	char hb_dir[64];
	strncpy(hb_dir, HB_Dir, sizeof(hb_dir)-1);
	while(hb_dir[i] != '\0')
	{
		if(hb_dir[i] == '/')
			hb_dir[i] = '_';
		i++;
	}
	string cacheDir(fmt("%s/%s_%s.db", m_listCacheDir.c_str(), DeviceName[currentPartition], hb_dir));
	bool updateCache = m_cfg.getBool(homebrew_domain, "update_cache");
	if(updateCache || !fsop_FileExist(cacheDir.c_str()))
	{
		m_vid.loadListImage();
		cacheCovers = true;
	}
	m_cacheList.CreateList(COVERFLOW_HOMEBREW, currentPartition, gameDir, stringToVector(".dol|.elf", '|'), cacheDir, updateCache);
	m_cfg.remove(homebrew_domain, "update_cache");	
	for(vector<dir_discHdr>::iterator tmp_itr = m_cacheList.begin(); tmp_itr != m_cacheList.end(); tmp_itr++)
	{
		m_gameList.push_back(*tmp_itr);
		// if(updateCache && tdb_genres)
			// _setTDBCategories(&(*(tmp_itr)));
	}
	return true;
}

bool CMenu::_loadGamecubeList()
{
	currentPartition = m_cfg.getInt(gc_domain, "partition", USB1);
	if(!DeviceHandle.IsInserted(currentPartition))
		return false;
	// gprintf("Adding gamecube list\n");
	string gameDir(fmt(gc_games_dir, DeviceName[currentPartition]));
	string cacheDir(fmt("%s/%s_gamecube.db", m_listCacheDir.c_str(), DeviceName[currentPartition]));
	bool updateCache = m_cfg.getBool(gc_domain, "update_cache");
	if(updateCache || !fsop_FileExist(cacheDir.c_str()))
	{
		m_vid.loadListImage();
		cacheCovers = true;
	}
	m_cacheList.CreateList(COVERFLOW_GAMECUBE, currentPartition, gameDir, stringToVector(".iso|.gcm|.ciso|root", '|'), cacheDir, updateCache);
	m_cfg.remove(gc_domain, "update_cache");
	for(vector<dir_discHdr>::iterator tmp_itr = m_cacheList.begin(); tmp_itr != m_cacheList.end(); tmp_itr++)
	{
		if(tmp_itr->settings[0] == 1) // disc 2
			continue; // skip gc disc 2 if it's still part of the cached list
		m_gameList.push_back(*tmp_itr);
		if(updateCache && tdb_genres)
			_setTDBCategories(&(*(tmp_itr)));
	}
	return true;
}

bool CMenu::_loadChannelList(void)
{
	u8 chantypes;
	if(neek2o())
		chantypes = CHANNELS_EMU; // force emunand in neek2o
	else if(isWiiVC)
		chantypes = CHANNELS_REAL; // force real nand in WiiVC
	else
		chantypes = m_cfg.getUInt(channel_domain, "channels_type", CHANNELS_EMU);
	if(chantypes < CHANNELS_REAL || chantypes > CHANNELS_BOTH)
	{
		m_cfg.setUInt(channel_domain, "channels_type", CHANNELS_REAL);
		chantypes = CHANNELS_REAL;
	}
	vector<string> NullVector;
	bool updateCache = m_cfg.getBool(channel_domain, "update_cache");
	if(chantypes & CHANNELS_REAL)
	{
		// gprintf("Adding real nand list\n");
		NANDemuView = false;
		if(updateCache)
		{
			m_vid.loadListImage();
			cacheCovers = true; // real nand channels list is not cached but covers may still need to be updated
		}
		m_cacheList.CreateList(COVERFLOW_CHANNEL, 9, std::string(), NullVector, std::string(), false);
		for(vector<dir_discHdr>::iterator tmp_itr = m_cacheList.begin(); tmp_itr != m_cacheList.end(); tmp_itr++)
		{
			m_gameList.push_back(*tmp_itr);
			if(tdb_genres)
				_setTDBCategories(&(*(tmp_itr)));
		}
	}
	if(chantypes & CHANNELS_EMU)
	{
		NANDemuView = true;
		int emuPartition = _FindEmuPart(EMU_NAND, true); // check if emunand folder exist and on FAT
		if(neek2o() && emuPartition == SD) // should only happen in case config file was manually modified
			emuPartition = USB1; // force USB
		if(emuPartition >= 0)
		{
			// gprintf("Adding emu nand list\n");
			currentPartition = emuPartition;
			string emunand = m_cfg.getString(channel_domain, "current_emunand");
			//! create a cache file for each emunand folder, and a different one for vwii nands
			string cacheDir = fmt("%s/%s_%s%s_channels.db", m_listCacheDir.c_str(), DeviceName[currentPartition], emunand.c_str(), IsOnWiiU() ? "_vwii" : "");
			if(updateCache || !fsop_FileExist(cacheDir.c_str()))
			{
				m_vid.loadListImage();
				cacheCovers = true;
			}
			m_cacheList.CreateList(COVERFLOW_CHANNEL, currentPartition, std::string(), NullVector, cacheDir, updateCache);
			for(vector<dir_discHdr>::iterator tmp_itr = m_cacheList.begin(); tmp_itr != m_cacheList.end(); tmp_itr++)
			{
				m_gameList.push_back(*tmp_itr);
				if(updateCache && tdb_genres)
					_setTDBCategories(&(*(tmp_itr)));
			}
		}
	}
	m_cfg.remove(channel_domain, "update_cache");
	return true;
}

bool CMenu::_loadPluginList()
{
	bool updateCache = m_cfg.getBool(plugin_domain, "update_cache");
	// gprintf("Adding plugins list\n");
	int channels_type = min(max(1, m_cfg.getInt(channel_domain, "channels_type", CHANNELS_REAL)), (int)ARRAY_SIZE(CMenu::_ChannelsType));
	for(u8 i = 0; m_plugin.PluginExist(i); ++i)
	{
		if(!m_plugin.GetEnabledStatus(i))
			continue;
		int romsPartition = m_plugin.GetRomPartition(i);
		if(romsPartition < 0)
			romsPartition = m_cfg.getInt(plugin_domain, "partition", 0);
		currentPartition = romsPartition;
		if(!DeviceHandle.IsInserted(currentPartition))
			continue;
		strncpy(m_plugin.PluginMagicWord, fmt("%08x", m_plugin.GetPluginMagic(i)), 8);
		m_cacheList.usePluginDBTitles = m_cfg.getBool(plugin_domain, "database_titles", true) && !m_plugin.GetFileNamesAsTitles(i); //
		const char *romDir = m_plugin.GetRomDir(i);
		if(strstr(romDir, "scummvm.ini") == NULL)
		{
			if(strncasecmp(m_plugin.PluginMagicWord, "484252", 6) == 0) // HBRW
			{
				if(updateCache)
					m_cfg.setBool(homebrew_domain, "update_cache", true);
				_loadHomebrewList(romDir);
			}
			else if(strncasecmp(m_plugin.PluginMagicWord, "4E47434D", 8) == 0) // NGCM
			{
				if(updateCache)
					m_cfg.setBool(gc_domain, "update_cache", true);
				_loadGamecubeList();
			}
			else if(strncasecmp(m_plugin.PluginMagicWord, "4E574949", 8) == 0) // NWII
			{
				if(updateCache)
					m_cfg.setBool(wii_domain, "update_cache", true);
				_loadWiiList();
			}
			else if(strncasecmp(m_plugin.PluginMagicWord, "4E414E44", 8) == 0) // NAND
			{
				if(updateCache)
					m_cfg.setBool(channel_domain, "update_cache", true);
				m_cfg.setInt(channel_domain, "channels_type", CHANNELS_REAL);
				_loadChannelList();
			}
			else if(strncasecmp(m_plugin.PluginMagicWord, "454E414E", 8) == 0) // ENAN
			{
				if(updateCache)
					m_cfg.setBool(channel_domain, "update_cache", true);
				m_cfg.setInt(channel_domain, "channels_type", CHANNELS_EMU);
				_loadChannelList();
			}
			else
			{
				string romsDir(fmt("%s:/%s", DeviceName[currentPartition], romDir));
				string cachedListFile(fmt("%s/%s_%s.db", m_listCacheDir.c_str(), DeviceName[currentPartition], m_plugin.PluginMagicWord));
				if(updateCache || !fsop_FileExist(cachedListFile.c_str()))
				{
					m_vid.loadListImage();
					cacheCovers = true;
				}
				vector<string> FileTypes = stringToVector(m_plugin.GetFileTypes(i), '|');
				m_cacheList.Color = m_plugin.GetCaseColor(i);
				m_cacheList.Magic = m_plugin.GetPluginMagic(i);
				m_cacheList.CreateRomList(m_platform, romsDir, FileTypes, cachedListFile, updateCache);
				for(vector<dir_discHdr>::iterator tmp_itr = m_cacheList.begin(); tmp_itr != m_cacheList.end(); tmp_itr++)
				{
					m_gameList.push_back(*tmp_itr);
					if(updateCache && tdb_genres)
						_setTDBCategories(&(*(tmp_itr)));
				}
			}
		}
		else // SCUMMVM
		{
			string cachedListFile(fmt("%s/%s_%s.db", m_listCacheDir.c_str(), DeviceName[currentPartition], m_plugin.PluginMagicWord));
			if(updateCache || !fsop_FileExist(cachedListFile.c_str()))
			{
				m_vid.loadListImage();
				cacheCovers = true;
			}
			Config scummvm;
			if(strchr(romDir, ':') == NULL) //
				scummvm.load(fmt("%s/%s", m_pluginsDir.c_str(), romDir));
			else
				scummvm.load(romDir);
			// should add error msg if loading scummvm fails or is not found
			
			string platformName = "";
			if(m_platform.loaded()) // convert plugin magic to platform name
				platformName = m_platform.getString("PLUGINS", m_plugin.PluginMagicWord);
			m_cacheList.Color = m_plugin.GetCaseColor(i);
			m_cacheList.Magic = m_plugin.GetPluginMagic(i);
			m_cacheList.ParseScummvmINI(scummvm, DeviceName[currentPartition], m_pluginDataDir.c_str(), platformName.c_str(), cachedListFile, updateCache);
			for(vector<dir_discHdr>::iterator tmp_itr = m_cacheList.begin(); tmp_itr != m_cacheList.end(); tmp_itr++)
			{
				m_gameList.push_back(*tmp_itr);
				if(updateCache && tdb_genres)
					_setTDBCategories(&(*(tmp_itr)));
			}
			scummvm.unload();			
		}
	}
		
	m_cfg.remove(plugin_domain, "update_cache");
	m_cfg.setInt(channel_domain, "channels_type", channels_type);	
	return true;
}

void CMenu::_stopSounds(void)
{
	/* Fade out sounds */
	// int fade_rate = m_cfg.getInt(general_domain, "music_fade_rate", 8);
	int fade_rate = 8;

	if(!MusicPlayer.IsStopped())
	{
		while(MusicPlayer.GetVolume() > 0 || m_gameSound.GetVolume() > 0)
		{
			MusicPlayer.Tick(true);
			if(m_gameSound.GetVolume() > 0)
				m_gameSound.SetVolume(m_gameSound.GetVolume() < fade_rate ? 0 : m_gameSound.GetVolume() - fade_rate);
			VIDEO_WaitVSync();
		}
	}
	m_btnMgr.stopSounds();
	CoverFlow.stopSound();
	m_gameSound.Stop();
}

/* Wiiflow creates a map<u8, u8> _installed_cios list for slots 200 to 253 and slot 0
	the first u8 is the slot and the second u8 is the base if its a d2x cios otherwise the slot number again.
	slot 0 is set to 1 - first = 0 and second = 1
	game config only shows the first (slot) or auto if first = 0 */
void CMenu::_load_installed_cioses()
{
	if(isWiiVC)
		return;
	gprintf("Loading cIOS map\n");
	_installed_cios[0] = 1;

	for(u8 slot = 200; slot < 254; slot++)
	{
		u8 base = 1;
		if(IOS_D2X(slot, &base))
		{
			gprintf("Found d2x base %u in slot %u\n", base, slot);
			_installed_cios[slot] = base;
		}
		else if(CustomIOS(IOS_GetType(slot)))
		{
			gprintf("Found cIOS in slot %u\n", slot);
			_installed_cios[slot] = slot;
		}
	}
}

void CMenu::_hideWaitMessage()
{
	m_vid.hideWaitMessage();
}

typedef struct map_entry
{
	char filename[8];
	u8 sha1[20];
} ATTRIBUTE_PACKED map_entry_t;

void CMenu::_loadDefaultFont(void)
{
	if(m_base_font != NULL)
		return;

	u32 size = 0;
	bool retry = false;
	bool korean = (CONF_GetLanguage() == CONF_LANG_KOREAN);
	char ISFS_Filename[32] ATTRIBUTE_ALIGN(32);

	/* Read content.map from ISFS */
	strcpy(ISFS_Filename, "/shared1/content.map");
	u8 *content = ISFS_GetFile(ISFS_Filename, &size, -1);
	if(content == NULL)
		return;

	u32 items = size / sizeof(map_entry_t);
	// gprintf("Open content.map, size %d, items %d\n", size, items);
	map_entry_t *cm = (map_entry_t *)content;

retry:
	bool kor_font = (korean && !retry) || (!korean && retry);
	for(u32 i = 0; i < items; i++)
	{
		if(m_base_font != NULL && m_wbf1_font != NULL && m_wbf2_font != NULL)
			break;
		if(memcmp(cm[i].sha1, kor_font ? WIIFONT_HASH_KOR : WIIFONT_HASH, 20) == 0 && m_base_font == NULL)
		{
			sprintf(ISFS_Filename, "/shared1/%.8s.app", cm[i].filename);  // who cares about the few ticks more?
			u8 *u8_font_archive = ISFS_GetFile(ISFS_Filename, &size, -1);
			if(u8_font_archive != NULL)
			{
				const u8 *font_file = u8_get_file_by_index(u8_font_archive, 1, &size); // There is only one file in that app
				// gprintf("Extracted font: %d\n", size);
				m_base_font = (u8*)MEM1_lo_alloc(size);
				memcpy(m_base_font, font_file, size);
				DCFlushRange(m_base_font, size);
				m_base_font_size = size;
				MEM2_free(u8_font_archive);
			}
		}
		else if(memcmp(cm[i].sha1, WFB_HASH, 20) == 0 && m_wbf1_font == NULL && m_wbf2_font == NULL)
		{
			sprintf(ISFS_Filename, "/shared1/%.8s.app", cm[i].filename); // who cares about the few ticks more?
			u8 *u8_font_archive = ISFS_GetFile(ISFS_Filename, &size, -1);
			if(u8_font_archive != NULL)
			{
				const u8 *font_file1 = u8_get_file(u8_font_archive, "wbf1.brfna", &size);
				m_wbf1_font = (u8*)MEM1_lo_alloc(size);
				memcpy(m_wbf1_font, font_file1, size);
				DCFlushRange(m_wbf1_font, size);
	
				const u8 *font_file2 = u8_get_file(u8_font_archive, "wbf2.brfna", &size);
				m_wbf2_font = (u8*)MEM1_lo_alloc(size);
				memcpy(m_wbf2_font, font_file2, size);
				DCFlushRange(m_wbf2_font, size);

				MEM2_free(u8_font_archive);
			}
		}
	}
	if(!retry && m_base_font == NULL)
	{
		retry = true;
		goto retry;
	}
	MEM2_free(content);
}

void CMenu::_cleanupDefaultFont()
{
	MEM1_lo_free(m_base_font);
	m_base_font = NULL;
	m_base_font_size = 0;
	MEM1_lo_free(m_wbf1_font);
	m_wbf1_font = NULL;
	MEM1_lo_free(m_wbf2_font);
	m_wbf2_font = NULL;
}

const char *CMenu::_domainFromView()
{
	if(m_sourceflow)
		return SOURCEFLOW_DOMAIN;

	switch(m_current_view)
	{
		case COVERFLOW_CHANNEL:
			return CHANNEL_DOMAIN;
		case COVERFLOW_HOMEBREW:
			return HOMEBREW_DOMAIN;
		case COVERFLOW_GAMECUBE:
			return GC_DOMAIN;
		case COVERFLOW_PLUGIN:
			return PLUGIN_DOMAIN;
		default:
			return WII_DOMAIN;
	}
	return "NULL";
}

void CMenu::RemoveCover(const char *id)
{
	const char *CoverPath = NULL;
	if(id == NULL)
		return;
	CoverPath = fmt("%s/%s.png", m_boxPicDir.c_str(), id);
	fsop_deleteFile(CoverPath);
	CoverPath = fmt("%s/%s.png", m_picDir.c_str(), id);
	fsop_deleteFile(CoverPath);
	CoverPath = fmt("%s/%s.wfc", m_cacheDir.c_str(), id);
	fsop_deleteFile(CoverPath);
	m_gcfg2.remove(id, "alt_cover");
}
/* If wiiflow using IOS58 this switches to cIOS for certain functions and back to IOS58 when done */
/* If wiiflow using cIOS no need to temp switch */
/*
void CMenu::TempLoadIOS(int IOS)
{
	// Only temp reload in IOS58 mode
	if(useMainIOS || neek2o())
		return;

	if(IOS == IOS_TYPE_NORMAL_IOS)
		IOS = 58;
	else if(IOS == 0)
		IOS = mainIOS;

	if(CurrentIOS.Version != IOS)
	{
		gprintf("\n\nCurrentIOS.Version != IOS \n> CurrentIOS.Version : %i \n> IOS to load : %i \n", CurrentIOS.Version, IOS);
		loadIOS(IOS, true); // switch to new IOS
		Sys_Init();
		Open_Inputs();
		for(int chan = WPAD_MAX_WIIMOTES-1; chan >= 0; chan--)
			WPAD_SetVRes(chan, m_vid.width() + m_cursor[chan].width(), m_vid.height() + m_cursor[chan].height());
		_netInit();
	}
	else
		gprintf("\n\nCurrentIOS.Version == IOS \n> CurrentIOS.Version : %i \n> IOS to load : %i \n", CurrentIOS.Version, IOS);
}
*/