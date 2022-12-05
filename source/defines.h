
// #define APP_WIIFLOW_LITE	// uncomment this line to compile wfl as wiiflow_lite
#define SCREENSHOT // comment this line to remove screenshot feature

#ifdef APP_WIIFLOW_LITE
#define APP_NAME				"WiiFlow Lite Mod6"
#else
#define APP_NAME				"WiiFlow Mod6"
#endif
#define APP_VERSION				"1.1.0-680"

#define APP_DATA_DIR			"wiiflow"
#ifdef APP_WIIFLOW_LITE
#define APPS_DIR				"apps/wiiflow_lite"
#else
#define APPS_DIR				"apps/wiiflow"
#endif

#define GAMES_DIR				"%s:/wbfs"
#define HOMEBREW_DIR			"apps" // no more %s:/
#define DF_GC_GAMES_DIR			"%s:/games"
#define CFG_FILENAME			"wiiflow_lite.ini"
#define CAT_FILENAME			"categories_lite.ini"
#define SOURCE_FILENAME			"source_menu.ini"
#define CTITLES_FILENAME		"custom_titles.ini"
#define TITLES_DUMP_FILENAME	"titles_dump.ini"
#define GAME_SETTINGS1_FILENAME	"gameconfig1.ini"
#define GAME_SETTINGS2_FILENAME	"gameconfig2.ini"

#define IDLE_TIME				120 // wiimote standby

#define WII_DOMAIN				"WII"
#define GC_DOMAIN				"GAMECUBE"
#define CHANNEL_DOMAIN			"CHANNELS"
#define PLUGIN_DOMAIN			"PLUGINS"
#define HOMEBREW_DOMAIN			"HOMEBREW"
#define SOURCEFLOW_DOMAIN		"SOURCEFLOW"
#define GENERAL_DOMAIN			"GENERAL"

#define DEVELOPERS				"Fledge68, mod by Iamerror80"
#define PAST_DEVELOPERS			"FIX94, OverjoY, Hibernatus, Narolez, Hulk, Miigotu, r-win"
#define LOADER_AUTHOR			"Kwiirk, Waninkoko, Hermes"
#define GUI_AUTHOR				"Hibernatus"

#define THANKS \
"Lustar, CedWii, Benjay, Domi78, Oops, \
Celtiore, Jiiwah, FluffyKiwi, Roku93, Yardape8000, \
Spayrosam, Bluescreen81, Chappy23, mamule, seam, \
BlindDude, Bubba, DJTaz, OggZee, entropy, Ayatus, \
Usptactical, WiiPower, Hermes, Spidy1000, megazig, \
Dimok, Kovani, Drexyl, DvZ, Etheboss, stfour, \
GaiaKnight, nibb, NJ7, Plasma, Pakatus, giantpune, \
wolf, ravmn, spidercaleb, Ziggy34, xFede, Abz, \
Cyan, Hakaisha, Tetsuo Shima"

#define THANKS_SITES \
"devkitpro.org, wiibrew.org, gametdb.com, \
ohloh.net, wiifanart.com, wiiflowiki.com, \
tgames.fr.nf"

#define THANKS_CODE \
"CFG Loader, Wii Banner Player Project, USB Loader GX, \
uLoader, NeoGamma, Mighty Channels, WiiXplorer, Triiforce, \
postLoader"

#define RIITAG_URL 			"https://tag.rc24.xyz/wii?game={ID6}&key={KEY}"
