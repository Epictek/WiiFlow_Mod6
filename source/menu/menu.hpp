#ifndef __MENU_HPP
#define __MENU_HPP
//#define SHOWMEMGECKO

#include <ogc/pad.h>
#include <vector>
#include <map>
#include <string>

#include "btnmap.h"
#include "channel/banner.h"
#include "channel/channels.h"
#include "cheats/gct.h"
#include "devicemounter/DeviceHandler.hpp"
#include "fileOps/fileOps.h"
#include "gecko/gecko.hpp"
#include "gecko/wifi_gecko.hpp"
#include "gui/coverflow.hpp"
#include "gui/cursor.hpp"
#include "gui/fanart.hpp"
#include "gui/gui.hpp"
#include "list/ListGenerator.hpp"
#include "loader/disc.h"
#include "loader/sys.h"
#include "loader/gc_disc_dump.hpp"
#include "loader/wbfs.h"
#include "music/gui_sound.h"
#include "music/MusicPlayer.hpp"
#include "plugin/plugin.hpp"
#include "sicksaxis-wrapper/sicksaxis-wrapper.h"
#include "wiiuse/wpad.h"
#include "wupc/wupc.h"
#include "wiidrc/wiidrc.h"

using std::string;
using std::vector;

class CMenu
{
public:
	CMenu();
	int main(void);
	bool init(bool usb_mounted);
	void directlaunch(const char *GameID);
	void _hideWaitMessage();
	void GC_Messenger(int message, int info, char *cinfo); // used by gc disc dumper
	const char *getBlankCoverPath(const dir_discHdr *element); // used by coverflow
	// void terror(const char *key, const wchar_t *msg) { error(_fmt(key, msg)); }
	// void TempLoadIOS(int IOS = 0);
	
	/* Proxy settings */
	bool proxyUseSystem;
	char proxyAddress[256];
	u16 proxyPort;
	char proxyUsername[33];
	char proxyPassword[33];

	/* General thread updating stuff */
	u64 m_thrdTotal;
	void update_pThread(u64 amount, bool add = true);
	
	/* Controller stuff */
	u8 activeChan = 0; // last WPAD chan used
	
private:
	char wii_games_dir[64];
	char gc_games_dir[64];
	
	u8 m_prev_view;
	u8 m_current_view;
	u8 enabledPluginsCount;
	u8 m_catStartPage;
	u8 m_max_categories;
	u8 m_screensaverDelay;
	u8 m_explorer_on_start;
	bool m_clearCats;
	bool m_getFavs;
	bool m_newGame;
	bool show_mem;
	bool cacheCovers;
	bool SF_enabled;
	bool tdb_genres;
	bool m_snapshot_loaded;
	bool customBg;
	bool enlargeButtons;
	bool sm_tier = false;
	vector<dir_discHdr> m_gameList;
	vector<string> tiers;
	vector<string> sm_numbers;

	const string general_domain = GENERAL_DOMAIN;
	const string wii_domain = WII_DOMAIN;
	const string gc_domain = GC_DOMAIN;
	const string channel_domain = CHANNEL_DOMAIN;
	const string plugin_domain = PLUGIN_DOMAIN;
	const string homebrew_domain = HOMEBREW_DOMAIN;
	const string sourceflow_domain = SOURCEFLOW_DOMAIN;
	
	const char *getBoxPath(const dir_discHdr *element, bool fullName);
	const char *getFrontPath(const dir_discHdr *element, bool fullName, bool smallBox, bool thumbNail);
	
	CCursor m_cursor[WPAD_MAX_WIIMOTES];
	
	CFanart m_fa;
	
	Config m_cfg;
	Config m_loc;
	Config m_cat;
	Config m_source;
	Config m_gcfg1;
	Config m_gcfg2;
	Config m_theme;
	Config m_coverflow;
	Config m_titles;
	Config m_platform;
	
	u8 *m_base_font;
	u32 m_base_font_size;
	u8 *m_wbf1_font;
	u8 *m_wbf2_font;
	u8 *m_file;
	u8 *m_buffer;
	u8 m_aa;
	u8 m_numCFVersions;
	u8 m_max_source_btn;
	u8 curCustBg;
	s16 m_showtimer;
	char cf_domain[16];
	volatile bool m_exit;
	bool m_use_source; // source_menu.ini found & ok to use source menu/flow
	bool m_sourceflow; // in sourceflow view
	bool m_refreshGameList;
	bool m_bnr_settings;
	bool m_directLaunch;
	bool m_locked;
	bool m_favorites;
	bool m_music_info;
	bool m_nintendont_installed;
	bool m_reload;
	bool m_use_wifi_gecko;
	bool m_use_sd_logging;
	bool m_source_autoboot;
	bool m_thumbnail; //
	
	dir_discHdr m_autoboot_hdr;
	
	string m_curLanguage;
	string m_themeName;

	/* Directories */
	string m_appDir;
	string m_dataDir;
	string m_cacheDir;
	string m_listCacheDir;
	string m_bnrCacheDir;
	string m_customBnrDir;
	string m_txtCheatDir;
	string m_cheatDir;
	string m_wipDir;
	string m_settingsDir;
	string m_languagesDir;
	string m_helpDir;
	string m_backupDir; //
	string m_screenshotDir;
	string m_boxPicDir;
	string m_picDir;
	string m_themeDir;
	string m_themeDataDir;
	string m_coverflowsDir;
	string m_musicDir;
	string m_videoDir;
	string m_fanartDir;
	string m_bckgrndsDir;
	string m_sourceDir;
	string m_pluginsDir;
	string m_pluginDataDir;
	string m_cartDir;
	string m_snapDir;
	string m_wiiTDBDir;

	/* Nand Emulation */
	char emu_nands_dir[32];
	string m_saveExtGameId;

	/* GC sound stuff */
	bool m_gc_play_banner_sound;
	
	/* Explorer stuff */
	bool m_txt_view;
	const char *m_txt_path;
	
	/* Background image stuff */
	TexData m_mainCustomBg[2];
	TexData m_mainBg;
	TexData m_mainBgLQ;
	TexData m_curBg;
	TexData m_configBg;
	const TexData *m_prevBg;
	const TexData *m_nextBg;
	const TexData *m_lqBg;
	u8 m_bgCrossFade;

	/* Main menu */
	// s16 m_mainLblCurMusic;

	s16 m_mem1FreeSize;
	s16 m_mem2FreeSize;
	
	TexData bgLQ;
	TexData texHome;
	TexData texHomeS;
	TexData texCateg;
	TexData texCategS;
	TexData texFavOff;
	TexData texFavOffS;
	TexData texFavOn;
	TexData texFavOnS;
	TexData texDVD;
	TexData texDVDS;
	TexData texFind;
	TexData texFindS;
	TexData texView;
	TexData texViewS;
	TexData texConfig;
	TexData texConfigS;
	TexData texPrev;
	TexData texPrevS;
	TexData texNext;
	TexData texNextS;
	TexData m_favoritesOff;
	TexData m_favoritesOn;
	TexData m_onePlayerOff;
	TexData m_onePlayerOn;
	TexData m_multiPlayerOff;
	TexData m_multiPlayerOn;
	TexData m_configBtn;
	TexData m_configBtnS;

	/* Game selected */
	int snapbg_x, snapbg_y, snapbg_w, snapbg_h;
	
	s16 m_gameLblSnap;
	s16 m_gameLblOverlay;

	TexData texGameCateg;
	TexData texGameCategS;
	TexData texGameFav;
	TexData texGameFavS;
	TexData texGameFavOn;
	TexData texGameFavOnS;
	TexData texCheat;
	TexData texCheatS;
	TexData texCheatOn;
	TexData texCheatOnS;
	TexData texGameConfig;
	TexData texGameConfigS;
	TexData texGameConfigOn;
	TexData texGameConfigOnS;
	TexData texVideo;
	TexData texVideoS;
	TexData texLock;
	TexData texLockS;
	TexData texLockOn;
	TexData texLockOnS;
	
	TexData texToggleBanner;
	TexData texSnapShotBg;
	TexData texBannerFrame;
	
	TexData m_game_snap;
	TexData m_game_overlay;
	
	volatile bool m_gameSelected;
	GuiSound m_gameSound;
	volatile bool m_soundThrdBusy;
	lwp_t m_gameSoundThread;
	bool m_gamesound_changed;
	u8 m_bnrSndVol;
	bool m_video_playing;
	bool m_zoom_video;

	/* Input guide */
	TexData m_game_guide; // controller input guide image
	
	/* Game info */
	TexData m_snap;
	TexData m_cart;
	TexData m_overlay;
	TexData m_rating;
	TexData m_wifi;
	TexData m_controlsreq[4];
	TexData m_controls[4];
	
#ifdef SHOWMEMGECKO
	unsigned int mem1old;
	unsigned int mem1;
	unsigned int mem2old;
	unsigned int mem2;
#endif

	/* Config menus */
	s16 m_configLblPage;
	s16 m_configBtnPageM;
	s16 m_configBtnPageP;
	s16 m_configBtnBack;
	s16 m_configBtnCenter;
	s16 m_configLblDialog;
	s16 m_configLblNotice;
	s16 m_configLblTitle;
	s16 m_configLblUser[4];
	
	s16 m_configBtn[10];
	s16 m_configLbl[10];
	s16 m_configLblVal[10];
	s16 m_configBtnM[10];
	s16 m_configBtnP[10];
	
	/* Checkboxes */
	s16 m_configChkOff[10];
	s16 m_configChkOn[10];
	s16 m_configChkReq[10];
	s16 m_configChkHid[10];
	s16 m_configBtnGo[10];
	s16 m_checkboxBtn[10];

	/* Settings */
	enum Regions
	{
		EN = 1,
		FR,
		DE,		
		ES,
		IT,
		NL,
		PT,
		AU,
		US
	};

	/* Config game menu */
	enum {
		MAIN_SETTINGS = 0,
		NANDEMU_SETTINGS,
		CHEAT_SETTINGS,
		VIDEO_SETTINGS,
		COMPAT_SETTINGS,
		NETWORK_SETTINGS,
		GAME_LIST
	};
	
	/* Game boot */
	enum
	{
		LOAD_IOS_FAILED = 0,
		LOAD_IOS_SUCCEEDED,
		LOAD_IOS_NOT_NEEDED
	};
	
	/* Progress bar */
	s16 m_wbfsLblMessage;
	s16 m_wbfsPBar;
	
	/* Zones */
	struct SZone
	{
		int x;
		int y;
		int w;
		int h;
		bool hide;
	};
	
	void ShowZone(SZone zone, bool &showZone);
	
	/* Zone Prev/Next menu Main */
	SZone m_mainPrevZone;
	SZone m_mainNextZone;
	void ShowPrevZone(void);
	void ShowNextZone(void);
	bool m_show_zone_prev;
	bool m_show_zone_next;
	
	/* Zone Header/Footer menu Home */
	SZone m_homeButtonsHeader;
	SZone m_homeButtonsFooter;
	void ShowHeaderZone(void);
	void ShowFooterZone(void);
	bool m_show_zone_header;
	bool m_show_zone_footer;

	/* Zone info icon menu Main */
	SZone m_mainInfo[7];	
	void ShowMainInfoZone(void);
	bool m_show_zone_info[7];
	
	/* Zone info icon menu Game */
	SZone m_gameInfo[6];	
	void ShowGameInfoZone(void);
	bool m_show_zone_gameinfo[6];	
	
	/* Inputs */
	WPADData *wd[WPAD_MAX_WIIMOTES];

	u32 wii_btnsPressed[WPAD_MAX_WIIMOTES];
	u32 wii_btnsHeld[WPAD_MAX_WIIMOTES];
	u32 wupc_btnsPressed[WPAD_MAX_WIIMOTES];
	u32 wupc_btnsHeld[WPAD_MAX_WIIMOTES];
	u32 gc_btnsPressed;
	u32 gc_btnsHeld;
	u32 ds3_btnsPressed;
	
	bool wBtn_Pressed(int btn, u8 ext);
	bool wBtn_PressedChan(int btn, u8 ext, int &chan);
	bool wBtn_Held(int btn, u8 ext);
	bool wBtn_HeldChan(int btn, u8 ext, int &chan);
	u32 wiidrc_to_pad(u32 btns);
	u32 ds3_to_pad(u32 btns);
	
	bool wii_btnRepeat(u8 btn);
	u8 m_wpadLeftDelay;
	u8 m_wpadDownDelay;
	u8 m_wpadRightDelay;
	u8 m_wpadUpDelay;
	u8 m_wpadADelay;
	// u8 m_wpadBDelay;

	bool gc_btnRepeat(s64 btn);
	u8 m_padLeftDelay;
	u8 m_padDownDelay;
	u8 m_padRightDelay;
	u8 m_padUpDelay;
	u8 m_padADelay;
	// u8 m_padBDelay;

	float left_stick_angle[WPAD_MAX_WIIMOTES];
	float left_stick_mag[WPAD_MAX_WIIMOTES];
	float right_stick_angle[WPAD_MAX_WIIMOTES];
	float right_stick_mag[WPAD_MAX_WIIMOTES];
	s32   right_stick_skip[WPAD_MAX_WIIMOTES];
	float wmote_roll[WPAD_MAX_WIIMOTES];
	s32	  wmote_roll_skip[WPAD_MAX_WIIMOTES];
	bool  enable_wmote_roll;
	
	void SetupInput(bool reset_pos = false);
	void ScanInput(bool enlargeButtons = false);
	
	void ButtonsPressed(void);
	void ButtonsHeld(void);

	void LeftStick();
	bool lStick_Up(void);
	bool lStick_Right(void);
	bool lStick_Down(void);
	bool lStick_Left(void);

	bool rStick_Up(void);
	bool rStick_Right(void);
	bool rStick_Down(void);
	bool rStick_Left(void);

	bool wRoll_Left(void);
	bool wRoll_Right(void);

	bool WPadIR_Valid(int chan);
	bool WPadIR_ANY(void);

	u8 pointerhidedelay[WPAD_MAX_WIIMOTES];
	u16 stickPointer_x[WPAD_MAX_WIIMOTES];
	u16 stickPointer_y[WPAD_MAX_WIIMOTES];
	bool m_show_pointer[WPAD_MAX_WIIMOTES];
	bool ShowPointer(void);

	time_t no_input_time;
	u32 NoInputTime(void);
	
	/* Threads */
	volatile bool m_thrdStop;
	volatile bool m_thrdWorking;
	float m_thrdStep;
	float m_thrdStepLen;
	mutex_t m_mutex;
	wstringEx m_thrdMessage;
	volatile float m_thrdProgress;
	volatile float m_fileProgress;
	volatile bool m_thrdMessageAdded;
	
	/* Theme */
	typedef std::map<string, TexData> TexSet;
	typedef std::map<string, GuiSound*> SoundSet;
	
	struct SThemeData
	{
		TexSet texSet;
		vector<SFont> fontSet;
		SoundSet soundSet;
		SFont btnFont;
		SFont lblFont;
		SFont titleFont;
		SFont txtFont;
		CColor btnFontColor;
		CColor lblFontColor;
		CColor txtFontColor;
		CColor titleFontColor;
		TexData bg;
		TexData btnTexL;
		TexData btnTexR;
		TexData btnTexC;
		TexData btnTexLS;
		TexData btnTexRS;
		TexData btnTexCS;
		
		TexData checkboxoff;
		TexData checkboxoffs;
		TexData checkboxon;
		TexData checkboxons;
		TexData checkboxHid;
		TexData checkboxHids;
		TexData checkboxReq;
		TexData checkboxReqs;
		TexData btnTexGo;
		TexData btnTexGoS;
		TexData pbarTexL;
		TexData pbarTexR;
		TexData pbarTexC;
		TexData pbarTexLS;
		TexData pbarTexRS;
		TexData pbarTexCS;
		TexData btnTexPlus;
		TexData btnTexPlusS;
		TexData btnTexMinus;
		TexData btnTexMinusS;
		TexData texBtnEntryS;
		
		GuiSound *clickSound;
		GuiSound *hoverSound;
		GuiSound *cameraSound;
		GuiSound *homeSound; // added
	};
	SThemeData theme;

	void _buildMenus(void);
	SFont _dfltFont(u32 fontSize, u32 lineSpacing, u32 weight, u32 index, const char *genKey);
	SFont _font(const char *domain, const char *key, SFont def_font);
	TexData _texture(const char *domain, const char *key, TexData &def, bool freeDef = true);
	vector<TexData> _textures(const char *domain, const char *key);	
	GuiSound *_sound(CMenu::SoundSet &soundSet, const char *filename, const u8 * snd, u32 len, const char *name, bool isAllocated);
	GuiSound *_sound(CMenu::SoundSet &soundSet, const char *domain, const char *key, const char *name);
	u16 _textStyle(const char *domain, const char *key, u16 def, bool coverflow = false);
	s16 _addButton(const char *domain, SFont font, const wstringEx &text, int x, int y, u32 width, u32 height, const CColor &color);
	s16 _addPicButton(const char *domain, TexData &texNormal, TexData &texSelected, int x, int y, u32 width, u32 height);
	s16 _addTitle(const char *domain, SFont font, const wstringEx &text, int x, int y, u32 width, u32 height, const CColor &color, s16 style);
	s16 _addText(const char *domain, SFont font, const wstringEx &text, int x, int y, u32 width, u32 height, const CColor &color, s16 style);
	s16 _addLabel(const char *domain, SFont font, const wstringEx &text, int x, int y, u32 width, u32 height, const CColor &color, s16 style);
	s16 _addLabel(const char *domain, SFont font, const wstringEx &text, int x, int y, u32 width, u32 height, const CColor &color, s16 style, TexData &bg);
	s16 _addProgressBar(const char *domain, int x, int y, u32 width, u32 height);
	void _setHideAnim(s16 id, const char *domain, int dx, int dy, float scaleX, float scaleY);
	void _addUserLabels(s16 *ids, u32 size, const char *domain);
	void _addUserLabels(s16 *ids, u32 start, u32 size, const char *domain);

	/* Main coverflow functions */
	void _loadCFCfg();
	void _loadCFLayout(int version, bool forceAA = false, bool otherScrnFmt = false);
	Vector3D _getCFV3D(const string &domain, const string &key, const Vector3D &def, bool otherScrnFmt = false);
	int _getCFInt(const string &domain, const string &key, int def, bool otherScrnFmt = false);
	float _getCFFloat(const string &domain, const string &key, float def, bool otherScrnFmt = false);
	void _setAA(int aa);
	void _setCFVersion(int version);
	int _getCFVersion(void);
	
	/* Menu inits */
	void _initMainMenu(void);
	void _initConfigMenu(void);
	void _initGameMenu(void);
	void _initCodeMenu(void);
	void _initAboutMenu(void);
	void _initWBFSMenu(void);
	void _initCFThemeMenu(void);
	void _initCheatSettingsMenu(void);
	void _initSourceMenu(void);
	void _initGameInfoMenu(void);
	void _initHomeAndExitToMenu(void);
	void _initExplorer(void);
	void _initTDBCategories(void);
	void _initKeyboardMenu(void);
	
	/* Menus texts */
	void _textConfig(void);
	void _textInfoMain(void);
	void _textInfoGame(void);
	void _textGame(void);
	void _textAbout(void);
	void _textGameInfo(void);
	void _textHome(void);
	void _textExitTo(void);
	
	/* Menu hides */
	void _hideCheatSettings(bool instant = false);
	void _hideMain(bool instant = false);
	void _hideGame(bool instant = false);
	void _hideAbout(bool instant = false);
	void _hideCFTheme(bool instant = false);
	void _hideSource(bool instant = false);
	void _hideGameInfo(bool instant = false);
	void _hideGameInfoPg(bool instant = false);
	void _hideHome(bool instant = false);
	void _hideExitTo(bool instant = false);
	void _hideExplorer(bool instant = false);
	void _hideConfigFull(bool instant = false);
	void _hideConfig(bool instant = false);
	void _hideConfigPage(bool instant = false);
	void _hideCheckboxes(bool instant = false);
	
	/* Menu shows */
	void _showError(void);
	void _showMain(void);
	void _showConfigWii(bool instant = false);
	void _showConfigNandEmu(bool instant = false);
	void _showConfigGC(bool instant = false);
	void _showConfigHB(bool instant = false);
	void _showConfigPlugin(bool instant = false);
	void _showConfigMain(bool instant = false);
	void _showConfigSource(bool instant = false);
	void _showConfigBoot(bool instant = false);
	void _showConfigMusic(bool instant = false);
	void _showConfigScreen(bool instant = false);
	void _showConfigGui(bool instant = false);
	void _showConfigNet(bool instant = false); //
	void _showConfigPaths(bool instant = false);
	void _showConfigMisc(bool instant = false);
	void _showProgress(void);
	void _showGame(bool fanart = false, bool anim = false);
	void _showDownload(void);
	void _showConfigDownload(bool instant = false);
	void _showAbout(void);
	void _showSource(void);
	void _showPluginSettings(void);
	void _showCategorySettings(void);
	void _showCheatSettings(bool instant = false);
	void _showGameInfo(void);
	void _showGameInfoPg(void);
	void _showWBFS(u8 op);
	void _showCFTheme(u32 curParam, int version, bool wide);
	void _showGameSettings(bool instant = false, bool dvd = false);
	void _showHome(void);
	void _showExitTo(void);
	void _showCoverBanner(void);
	void _showExplorer(void);
	void _showWad(void);
	void _showCheckboxesMenu(void);
	void _showAddGame(void); //
	void _showCF(bool refreshList = false);
	void _showCategoryConfig(bool instant = false); //
	void _showConfigPluginEditor(bool instant = false); //
	void _showViewOptions(bool instant = false); //
	
	/* Menu main functions */
	bool _error(const wstringEx &msg, bool dialog = false);
	void _config(void);
	void _configGui(void);
	void _configMisc(void);
	void _configScreen(void);
	void _configMusic(void);
	void _configPaths(void);
	void _configNet(void); //
	void _configWii(void);
	bool _configNandEmu(void);
	void _configGC(void);
	void _configHB(void);
	void _configPlugin(void);
	void _configDownload(bool fromGameSet = false);
	void _configSource(void);
	void _configBoot(void);
	void _configPluginEditor(u8 pos); //
	void _game(bool launch = false);
	void _download(string gameId = string(), int dl_type = 0);
	bool _code(char code[4]);
	void _about(bool help = false);
	bool _wbfsOp(u8 op);
	bool _cfTheme(void);
	bool _gameinfo(void);
	void _gameSettings(const dir_discHdr *GameHdr, bool disc = false);
	void _CoverBanner(void);
	void _addGame(u8 game_type); //
	void _Wad(const char *wad_path = NULL, bool folder = false);
	void _CheatSettings(const char *id);
	bool _Source(bool home = false);
	void _PluginSettings();
	void _CategorySettings(bool fromGameSet = false);
	void _checkboxesMenu(u8 md);
	char *_keyboard(void); //
	void _CategoryConfig(void); //
	void _viewOptions(void); //
	bool _Home();
	bool _ExitTo();
	
	/* Nand emu functions */
	string _SetEmuNand(s8 direction); //
	bool _NandDump(int DumpType); //
	int _ExtractGameSave(string gameId);
	int _FlashGameSave(string gameId);
	int _FindEmuPart(bool savesnand, bool skipchecks);
	bool _checkSave(string id, int nand_type);
	void _checkEmuNandSettings(int nand_type);
	void _FullNandCheck(int nand_type);
	void _listEmuNands(const char *path, vector<string> &emuNands);	
	static void * _NandDumper(void *obj);
	static void * _NandFlasher(void *obj);
	float m_progress;
	float m_fprogress;
	int m_fileprog;
	int m_filesize;
	int m_dumpsize;
	int m_filesdone;
	int m_foldersdone;
	int m_nandexentry;
	
	/* Explorer menu */
	void _refreshExplorer(s8 direction = 0);
	void _Explorer(void);
	void _FileExplorer(const char *startPath); //
	const char *_FolderExplorer(const char *startPath);
	void _wadExplorer(void);
	void _pluginExplorer(const char *startPath, u32 magic = 0, bool source = false); //

	/* Source menu + Source menu editor */
	void _setSrcOptions(void);
	void _updateSourceBtns(void);
	void _getMagicNums(void); //
	void _getAllPlugins(void); //
	void _getMaxSourceButton(void); //
	void _getSourcePage(bool home = false); //
	bool _getNewSource(u8 btn);
	// void _setPluginPath(u8 mode); //
	
	/* Plugins */
	void _updatePluginText(void);
	void _updatePluginCheckboxes(bool instant = false); // instead of true	
	
	/* Checkboxes */
	void _updateCheckboxesText(void);
	void _updateCheckboxes(bool instant = false);
	void _updateCatCheckboxes(void);
	
	/* Categories */
	void _setTDBCategories(const dir_discHdr *hdr); //
	void _getGameCategories(const dir_discHdr *hdr);
	void _setGameCategories(void);	
	
	/* Adjust coverflow */
	struct SCFParamDesc
	{
		enum
		{
			PDT_EMPTY,
			PDT_FLOAT,
			PDT_V3D,
			PDT_COLOR,
			PDT_BOOL,
			PDT_INT, 
			PDT_TXTSTYLE,
		} paramType[4];
		enum
		{
			PDD_BOTH,
			PDD_NORMAL, 
			PDD_SELECTED,
		} domain;
		bool scrnFmt;
		const char name[32];
		const char valName[4][64];
		const char key[4][48];
		float step[4];
		float minMaxVal[4][2];
	};
	void _cfParam(bool inc, int i, const SCFParamDesc &p, int cfVersion, bool wide);
	const char *_cfDomain(bool selected = false);

	/* Download */
	void _setThrdMsg(const wstringEx &msg, float progress);
	void _setDumpMsg(const wstringEx &msg, float progress, float fileprog);
	void _downloadProgress(void *obj, int size, int position);
	int _coverDownloader(bool disc = false);
	int _gametdbDownloaderAsync();
	int _bannerDownloader();
	int _downloadCheatFileAsync(const char *id);
	bool _isNetworkAvailable();
	int _initNetwork();
	void _netInit();
	// static void * _downloadUrlAsync(void *obj);
	// void _downloadUrl(const char *url, u8 **dl_file, u32 *dl_size);
	// static s32 _networkComplete(s32 result, void *usrData);
	
	static void * _pThread(void *obj);
	void _start_pThread(void);
	void _stop_pThread(void);
	lwp_t m_thrdPtr;
	volatile bool m_thrdInstalling;
	volatile bool m_thrdUpdated;
	volatile bool m_thrdDone;
	vu64 m_thrdWritten;
	
	/* WBFS functions */
	static void _addDiscProgress(int status, int total, void *user_data);
	static void _ShowProgress(int dumpstat, int dumpprog, int filestat, int fileprog, int files, int folders, const char *tmess, void *user_data);
	static void * _gameInstaller(void *obj);
	static void * _ImportFolder(void *obj); //
	bool _searchGamesByID(const char *gameId);
	int _GCgameInstaller();
	// static void * _GCcopyGame(void *obj);
	
	enum // don't change order (used in a for loop in menu_config_game.cpp)
	{
		WO_REMOVE_GAME = 0,
		WO_BACKUP_EMUSAVE,
		WO_REMOVE_EMUSAVE,
		WO_RESTORE_EMUSAVE
	};
	
	/* Game selected functions */
	void _cleanupBanner(bool gamechange = false);
	void _cleanupVideo();
	bool _startVideo(void);
	void _extractBnr(const dir_discHdr *hdr);
	void _setCurrentItem(const dir_discHdr *hdr);
	void _playGameSound(void);
	void _stopGameSoundThread(void);
	static void * _gameSoundThread(void *obj);

	/* Gamelist functions */
	bool _loadList(void);
	bool _loadWiiList(void);
	bool _loadGamecubeList(void);
	bool _loadChannelList(void);
	bool _loadPluginList(void);
	bool _loadHomebrewList(const char *HB_Dir);
	void _initCF(bool dumpGameList = false);
	
	/* Background handling functions */
	void _getCustomBgTex(void);
	void _setMainBg(void);
	void _setBg(const TexData &bgTex, const TexData &bglqTex, bool force_change = false);
	void _updateBg(void);
	void _drawBg(void);

	/* Sourceflow functions */
	void _sourceFlow();
	int _getSrcFlow();
	void _setSrcFlow(int version);
	bool _srcTierBack(bool home);
	void _getSFlowBgTex();
	
	/* Misc functions */
	void RemoveCover(const char *id);
	void _setPartition(s8 direction, u8 coverflow);
	void _cacheCovers(void);
	void _mainLoopCommon(bool withCF = false, bool adjusting = false);
	void _loadDefaultFont(void);
	void _cleanupDefaultFont();
	void cleanup(void);
	void _Theme_Cleanup();
	void _stopSounds(void);
	void exitHandler(int ExitTo);
	const char *_domainFromView(void);
	void _updateText(void);
	const wstringEx _t(const char *key, const wchar_t *def = L"") { return m_loc.getWString(m_curLanguage, key, def); }
	const wstringEx _fmt(const char *key, const wchar_t *def);
	wstringEx _optBoolToString(int b);
	void _load_installed_cioses();
	std::map<u8, u8> _installed_cios;
	typedef std::map<u8, u8>::iterator CIOSItr;
	void _dumpGameList(void); //
	bool _launchNeek2oChannel(int ExitTo, int nand_type); //
	wstringEx _sortLabel(int sort); //
	void _sortCF(bool previous = false); //	
	// void UpdateCache(u32 view = COVERFLOW_MAX);
	
	/* Game boot functions */
	void _launch(const dir_discHdr *hdr);
	void _launchWii(dir_discHdr *hdr, bool dvd, bool disc_cfg = false);
	void _launchChannel(dir_discHdr *hdr);
	void _launchHomebrew(const char *filepath, vector<string> arguments);
	void _launchGC(dir_discHdr *hdr, bool disc);
	void _launchPlugin(dir_discHdr *hdr);
	void _launchShutdown();
	vector<string> _getMetaXML(const char *bootpath);
	int _loadGameIOS(u8 ios, int userIOS, bool RealNAND_Channels = false);
	bool _loadFile(u8 * &buffer, u32 &size, const char *path, const char *file); // gameconfig.txt and cheats.gct
	void _exitWiiflow();

	/* Option arrays */
	struct SOption { const char id[11]; const wchar_t text[16]; };
	
	static const SOption _VideoModes[7];
	static const SOption _languages[12];
	static const SOption _GCvideoModes[7];
	static const SOption _GClanguages[8];
	static const SOption _ChannelsType[3];
	static const SOption _NandEmu[4];
	static const SOption _SaveEmu[5];
	static const SOption _AspectRatio[3];
	static const SOption _NinEmuCard[6];
	static const SOption _vidModePatch[4];
	static const SOption _debugger[3];
	static const SOption _hooktype[8];
	static const SOption _exitTo[4];
	static const SOption _AddRegionCover[10];
	static const SOption _privateServer[3];
	static const SOption _DeflickerOptions[7];
	static const SOption _coverColor[6];
	
	static const SCFParamDesc _cfParams[];

	/* Thread stack */
	static u8 downloadStack[8192];
	static const u32 downloadStackSize;
	
	/* FTP stuff */
	void _initFTP();
	void _showFTP(void);
	void _updateFTP(void);
	void _FTP();
	bool m_init_ftp;
	bool m_ftp_inited;

	s16 m_mainBtnFavoritesOn;
	s16 m_mainBtnFavoritesOff;
	s16 m_mainBtnOnePlayerOn;
	s16 m_mainBtnOnePlayerOff;
	s16 m_mainBtnMultiPlayerOn;
	s16 m_mainBtnMultiPlayerOff;
	s16 m_mainBtnConfig;
};

extern CMenu mainMenu;

#define ARRAY_SIZE(a)		(sizeof a / sizeof a[0])

#endif // !defined(__MENU_HPP)
