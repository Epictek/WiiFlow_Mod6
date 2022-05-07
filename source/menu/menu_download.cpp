// #include <network.h>
// #include <ogc/lwp_watchdog.h>
// #include <time.h>
// #include <fstream>

#include "menu.hpp"
#include "loader/fs.h"
#include "network/https.h"
#include "unzip/ZipFile.h"
#include "network/FTP_Dir.hpp" // -ftp-
// #include "loader/wbfs.h"
// #include "loader/wdvd.h"
// #include "types.h"
// #include "lockMutex.hpp"
// #include "channel/nand.hpp"
// #include "devicemounter/usbstorage.h"
// #include "gui/GameTDB.hpp"
// #include "gui/pngu.h"

#define TAG_GAME_ID		"{gameid}"
#define TAG_LOC			"{loc}"
#define TAG_CONSOLE		"{console}"

#define GAMETDB_URL		"http://www.gametdb.com/wiitdb.zip?LANG=%s&FALLBACK=TRUE&WIIWARE=TRUE&GAMECUBE=TRUE"
#define CUSTOM_BANNER_URL	"http://banner.rc24.xyz/{gameid}.bnr"

static const char FMT_BPIC_URL[] = "https://art.gametdb.com/{console}/coverfullHQ/{loc}/{gameid}.png"\
"|https://art.gametdb.com/{console}/coverfull/{loc}/{gameid}.png";
static const char FMT_PIC_URL[] = "https://art.gametdb.com/{console}/cover/{loc}/{gameid}.png";
static const char FMT_CBPIC_URL[] = "https://art.gametdb.com/{console}/coverfullHQ2/{loc}/{gameid}.png";
static const char FMT_CPIC_URL[] = "https://art.gametdb.com/{console}/cover2/{loc}/{gameid}.png";
static const char FMT_DPIC_URL[] = "https://art.gametdb.com/{console}/disc/{loc}/{gameid}.png"; // disk

static string dl_gameID;
int count, countFlat;
u32 n;

void CMenu::_showDownload(void)
{
	m_btnMgr.setText(m_configLblTitle, _t("cfg3", L"Download covers and info"));
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_configBtnBack);
	m_btnMgr.setText(m_configBtnCenter, _t("cfg1", L"Settings"));
	m_btnMgr.show(m_configBtnCenter);
	m_btnMgr.setText(m_configLblNotice, _t("dl10", L"Please donate\nto GameTDB.com"));
	m_btnMgr.show(m_configLblNotice);

	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);

	m_btnMgr.setText(m_configLbl[3], _t("dl12", L"GameTDB game info"));
	m_btnMgr.setText(m_configLbl[4], _t("dl8", L"Current coverflow covers"));
	m_btnMgr.setText(m_configLbl[5], _t("dl92", L"Current coverflow disc labels"));
	m_btnMgr.setText(m_configLbl[6], _t("dl26", L"GameCube custom banners"));

	bool gc = false;
	if((m_current_view & COVERFLOW_GAMECUBE) || ((m_current_view & COVERFLOW_PLUGIN) && (m_plugin.GetEnabledStatus(m_plugin.GetPluginPosition(0x4E47434D))))) // plugin gamecube
		gc = true;
	for(u8 i = 3; i < 6 + gc; ++i)
	{
		m_btnMgr.show(m_configLbl[i]);
		m_btnMgr.setText(m_configBtn[i], _t("cfg4", L"Download"));
		m_btnMgr.show(m_configBtn[i]);
	}
}

void CMenu::_showProgress(void)
{	
	m_btnMgr.setProgress(m_wbfsPBar, 0.f, true);
	m_btnMgr.setText(m_wbfsLblMessage, L"0%");
	m_btnMgr.setText(m_configLblDialog, L"");
	m_btnMgr.show(m_wbfsPBar);
	m_btnMgr.show(m_wbfsLblMessage);
	m_btnMgr.show(m_configLblDialog);
}

void CMenu::_download(string gameId, int dl_type)
{
	dl_gameID = gameId;
	bool dl_finished = false;
	
	_hideConfig(true);
	
	if(strlen(dl_gameID.c_str()) == 0) // if launched from settings menu
		_showDownload();
		
	SetupInput();

	while(!m_exit)
	{
		_mainLoopCommon();
		if(BTN_HOME_HELD || BTN_B_OR_1_PRESSED || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnBack)))
		{
			if(dl_finished)
			{
				dl_finished = false;
				m_btnMgr.hide(m_wbfsPBar);
				m_btnMgr.hide(m_wbfsLblMessage);
				m_btnMgr.hide(m_configLblDialog);
				m_btnMgr.hide(m_configBtnBack, true);
				if(strlen(dl_gameID.c_str()) > 0)
					break;
				_showDownload();
			}
			else
				break;
		}
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
			m_btnMgr.up();
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
			m_btnMgr.down();
		if(BTN_A_OR_2_PRESSED || dl_type > 0)
		{
			if(m_btnMgr.selected(m_configBtn[3])) // gametdb dl
			{
				m_refreshGameList = true;
				_hideConfig();
				_showProgress();

				_start_pThread();
				int ret = _gametdbDownloaderAsync();
				_stop_pThread();
				if(ret == -1)
					m_btnMgr.setText(m_configLblDialog, _t("dlmsg27", L"Not enough memory!"));
				else if(ret == -2)
					m_btnMgr.setText(m_configLblDialog, _t("dlmsg2", L"Network initialization failed!"));
				else if(ret == -3)
					m_btnMgr.setText(m_configLblDialog, _t("dlmsg12", L"Download failed!"));
				else if(ret == -4)
					m_btnMgr.setText(m_configLblDialog, _t("dlmsg15", L"Couldn't save ZIP file."));
				else
					m_btnMgr.setText(m_configLblDialog, _t("dlmsg14", L"Done."));
				dl_finished = true;
				m_btnMgr.show(m_configBtnBack);
			}
			else if(m_btnMgr.selected(m_configBtn[4]) || dl_type == 1) // dl covers
			{
				m_refreshGameList = true;
				_hideConfig();
				_showProgress();

				_start_pThread();
				int ret = _coverDownloader(false); // disc = false
				_stop_pThread();
				if(ret == 0)
				{
					if(countFlat == 0)
					{
						m_thrdMessage = wfmt(_fmt("dlmsg5", L"%i/%i files downloaded."), count, n);
						m_btnMgr.setText(m_configLblDialog, m_thrdMessage);
					}
					else
					{
						m_thrdMessage = wfmt(_fmt("dlmsg9", L"%i/%i front covers downloaded."), countFlat, n);
						m_btnMgr.setText(m_configLblDialog, m_thrdMessage);
					}
				}
				else if(ret == -1)
					m_btnMgr.setText(m_configLblDialog, _t("dlmsg27", L"Not enough memory!"));
				else if(ret == -2)
					m_btnMgr.setText(m_configLblDialog, _t("dlmsg2", L"Network initialization failed!"));
				else if(ret == -3)
					m_btnMgr.setText(m_configLblDialog, _t("dlmsg30", L"No covers missing."));
				dl_finished = true;
				dl_type = 0;

				m_btnMgr.show(m_configBtnBack);
			}
			else if(m_btnMgr.selected(m_configBtn[5])) // disk dl
			{
				_hideConfig();
				_showProgress();

				_start_pThread();
				int ret = _coverDownloader(true); // disc = true
				_stop_pThread();
				if(ret == 0)
				{
					m_thrdMessage = wfmt(_fmt("dlmsg5", L"%i/%i files downloaded."), count, n);
					m_btnMgr.setText(m_configLblDialog, m_thrdMessage);
				}
				else if(ret == -1)
					m_btnMgr.setText(m_configLblDialog, _t("dlmsg27", L"Not enough memory!"));
				else if(ret == -2)
					m_btnMgr.setText(m_configLblDialog, _t("dlmsg2", L"Network initialization failed!"));
				else if(ret == -3)
					m_btnMgr.setText(m_configLblDialog, _t("dlmsg99", L"No discs missing."));
				dl_finished = true;
				dl_type = 0;

				m_btnMgr.show(m_configBtnBack);
			}
			else if(m_btnMgr.selected(m_configBtn[6]) || dl_type == 2) // download bnr
			{
				_hideConfig();
				_showProgress();

				_start_pThread();
				int ret = _bannerDownloader();
				_stop_pThread();
				if(ret == 0)
				{
					if(dl_gameID.empty())
					{
						m_thrdMessage = wfmt(_fmt("dlmsg5", L"%i/%i files downloaded."), count, n);
						m_btnMgr.setText(m_configLblDialog, m_thrdMessage);
					}
					else
						m_btnMgr.setText(m_configLblDialog, _t("dlmsg14", L"Done."));
				}
				else if(ret == -1)
					m_btnMgr.setText(m_configLblDialog, _t("dlmsg34", L"Banner URL not set properly!")); // banner url not set
				else if(ret == -2)
					m_btnMgr.setText(m_configLblDialog, _t("dlmsg2", L"Network initialization failed!"));
				else if(ret == -3)
					m_btnMgr.setText(m_configLblDialog, _t("dlmsg33", L"No banners missing.")); // no banners missing
				dl_finished = true;
				dl_type = 0;

				m_btnMgr.show(m_configBtnBack);
			}
			else if(m_btnMgr.selected(m_configBtnCenter)) // dl settings
			{
				_hideConfig(true);
				_configDownload();
				_showDownload();
			}
		}
	}
	_hideConfig(true);
	m_btnMgr.hide(m_configLblNotice, true);
}

/************************************ Setup network connection *******************************************/

/** _initAsyncNetwork() and _networkComplete() removed **/

void CMenu::_netInit(void) // init network + wifi gecko debug and/or ftp thread
{
	if(networkInit || !(m_use_wifi_gecko || m_init_ftp) || m_exit) // -ftp-
		return;
	if(_initNetwork() < 0)
		return;
	
	gprintf("Network init complete, enabling WifiGecko: %s\n", m_use_wifi_gecko ? "yes" : "no");
	if(m_use_wifi_gecko)
	{
		const string &ip = m_cfg.getString("DEBUG", "wifi_gecko_ip");
		u16 port = m_cfg.getInt("DEBUG", "wifi_gecko_port", 4405);
		if(ip.size() > 0 && port != 0)
			WiFiDebugger.Init(ip.c_str(), port);
	}
	if(m_init_ftp) // -ftp-
		m_ftp_inited = ftp_startThread();
}

int CMenu::_initNetwork()
{
	if(networkInit)
		return 0;
	if(!_isNetworkAvailable())
		return -2;

	char ip[16];
	int val = if_config(ip, NULL, NULL, true, 0);
	if(val == 0)
	{
		wolfSSL_Init();
		networkInit = true;
	}
	return val;
}

bool CMenu::_isNetworkAvailable()
{
	bool retval = false;
	u32 size;
	char ISFS_Filepath[32] ATTRIBUTE_ALIGN(32);
	strcpy(ISFS_Filepath, "/shared2/sys/net/02/config.dat");
	u8 *buf = ISFS_GetFile(ISFS_Filepath, &size, -1);
	if(buf && size > 4)
		retval = buf[4] > 0; // There is a valid connection defined.
	MEM2_free(buf);
	return retval;
}

/** Older method **/
/**
void CMenu::_initAsyncNetwork()
{
	if(!_isNetworkAvailable())
		return;
	m_thrdNetwork = true;
	net_init_async(_networkComplete, this);
	while(net_get_status() == -EBUSY)
		usleep(100);
}
s32 CMenu::_networkComplete(s32 ok, void *usrData)
{
	CMenu *m = (CMenu *) usrData;
	networkInit = ok == 0;
	m->m_thrdNetwork = false;

	gprintf("NET: Network init complete, enabled wifi_gecko: %s\n", m->m_use_wifi_gecko ? "yes" : "no");
	if(m->m_use_wifi_gecko)
	{
		const string &ip = m->m_cfg.getString("DEBUG", "wifi_gecko_ip");
		u16 port = m->m_cfg.getInt("DEBUG", "wifi_gecko_port", 4405);
		if(ip.size() > 0 && port != 0)
			WiFiDebugger.Init(ip.c_str(), port);
	}

	return 0;
}
**/

/***************************************** Cover downloading *********************************************/

static string countryCode(const string &gameId)
{
	switch (gameId[3])
	{
		case 'E':
			return "US";
		case 'J':
			return "JA";
		case 'W':
			return "ZH";
		case 'K':
			return "KO";
		case 'R':
			return "RU";
		case 'P':
		case 'D':
		case 'F':
		case 'I':
		case 'S':
		case 'H':
		case 'X':
		case 'Y':
		case 'Z':
			switch (CONF_GetArea())
			{
				case CONF_AREA_BRA:
					return "PT";
				case CONF_AREA_AUS:
					return "AU";
			}
			switch (CONF_GetLanguage())
			{
				case CONF_LANG_ENGLISH:
					return "EN";
				case CONF_LANG_GERMAN:
					return "DE";
				case CONF_LANG_FRENCH:
					return "FR";
				case CONF_LANG_SPANISH:
					return "ES";
				case CONF_LANG_ITALIAN:
					return "IT";
				case CONF_LANG_DUTCH:
					return "NL";
			}
			return "other";
		case 'A':
			switch (CONF_GetArea())
			{
				case CONF_AREA_USA:
					return "US";
				case CONF_AREA_JPN:
					return "JA";
				case CONF_AREA_CHN:
				case CONF_AREA_HKG:
				case CONF_AREA_TWN:
					return "ZH";
				case CONF_AREA_KOR:
					return "KO";
				case CONF_AREA_BRA:
					return "PT";
				case CONF_AREA_AUS:
					return "AU";
			}
			switch (CONF_GetLanguage())
			{
				case CONF_LANG_ENGLISH:
					return "EN";
				case CONF_LANG_GERMAN:
					return "DE";
				case CONF_LANG_FRENCH:
					return "FR";
				case CONF_LANG_SPANISH:
					return "ES";
				case CONF_LANG_ITALIAN:
					return "IT";
				case CONF_LANG_DUTCH:
					return "NL";
			}
	}
	return "other";
}

static string makeURL(const string format, const string gameId, const string country)
{
	string url = format;
	if(url.find(TAG_LOC) != url.npos)
 		url.replace(url.find(TAG_LOC), strlen(TAG_LOC), country.c_str());

	if(url.find(TAG_CONSOLE) != url.npos)
		url.replace(url.find(TAG_CONSOLE), strlen(TAG_CONSOLE), "wii");

	url.replace(url.find(TAG_GAME_ID), strlen(TAG_GAME_ID), gameId.c_str());

	return url;
}

void CMenu::_downloadProgress(void *obj, int size, int position)
{
	CMenu *m = (CMenu *)obj;
	m->m_progress = size == 0 ? 0.f : (float)position / (float)size;
	//! don't synchronize too often
	if(m->m_progress - m->m_thrdProgress >= 0.01f)
	{
		LWP_MutexLock(m->m_mutex);
		m->m_thrdProgress = m->m_progress;
		LWP_MutexUnlock(m->m_mutex);
	}
}

void * CMenu::_pThread(void *obj)
{
	CMenu *m = (CMenu*)obj;
	m->SetupInput();
	while(m->m_thrdInstalling)
	{
		m->_mainLoopCommon();
		if(m->m_thrdUpdated)
		{
			m->m_thrdUpdated = false;
			m->_downloadProgress(obj, m->m_thrdTotal, m->m_thrdWritten);
			if(m->m_thrdProgress > 0.f)
			{
				m_btnMgr.setText(m->m_wbfsLblMessage, wfmt(L"%i%%", (int)(m->m_thrdProgress * 100.f)));
				m_btnMgr.setProgress(m->m_wbfsPBar, m->m_thrdProgress);
			}
			m->m_thrdDone = true;
		}
		if(m->m_thrdMessageAdded)
		{
			m->m_thrdMessageAdded = false;
			if(!m->m_thrdMessage.empty())
				m_btnMgr.setText(m->m_configLblDialog, m->m_thrdMessage);
		}
	}
	m->m_thrdWorking = false;
	return 0;
}

void CMenu::_start_pThread(void)
{
	m_thrdPtr = LWP_THREAD_NULL;
	m_thrdWorking = true;
	m_thrdMessageAdded = false;
	m_thrdInstalling = true;
	m_thrdUpdated = false;
	m_thrdDone = true;
	m_thrdProgress = 0.f;
	m_thrdWritten = 0;
	m_thrdTotal = 0;
	LWP_CreateThread(&m_thrdPtr, _pThread, this, 0, 8 * 1024, 64);
}

void CMenu::_stop_pThread(void)
{
	if(m_thrdPtr == LWP_THREAD_NULL)
		return;

	if(LWP_ThreadIsSuspended(m_thrdPtr))
		LWP_ResumeThread(m_thrdPtr);
	m_thrdInstalling = false;
	while(m_thrdWorking)
		usleep(50);
	LWP_JoinThread(m_thrdPtr, NULL);
	m_thrdPtr = LWP_THREAD_NULL;

	m_btnMgr.setProgress(m_wbfsPBar, 1.f);
	m_btnMgr.setText(m_wbfsLblMessage, L"100%");
}

void CMenu::update_pThread(u64 amount, bool add)
{
	if(m_thrdDone)
	{
		m_thrdDone = false;
		if(add)
			m_thrdWritten = m_thrdWritten + amount;
		else
			m_thrdWritten = amount;
		m_thrdUpdated = true;
	}
}

int CMenu::_coverDownloader(bool disc) // added disc for disc labels
{
	count = 0;
	countFlat = 0;

	GameTDB c_gameTDB;
	if(m_settingsDir.size() > 0)
	{
		c_gameTDB.OpenFile(fmt("%s/wiitdb.xml", m_settingsDir.c_str()));
		c_gameTDB.SetLanguageCode(m_curLanguage.c_str());
	}
	vector<string> coverIDList;
		
	/* Create list of cover ID's that need downloading */
	if(dl_gameID.empty())
	{
		for(u32 i = 0; i < m_gameList.size(); ++i)
		{
			if(m_gameList[i].type == TYPE_PLUGIN || m_gameList[i].type == TYPE_HOMEBREW)
				continue;
			if(!disc)
			{
				if(!fsop_FileExist(fmt("%s/%s.png", m_boxPicDir.c_str(), m_gameList[i].id)))
					coverIDList.push_back(m_gameList[i].id);
			}
			else // disc dl
			{
				if(!fsop_FileExist(fmt("%s/%s.png", m_cartDir.c_str(), m_gameList[i].id)))
					coverIDList.push_back(m_gameList[i].id);
			}
		}
	}
	else
		coverIDList.push_back(dl_gameID);

	n = coverIDList.size();
	m_thrdTotal = n * 3; // 3 = download cover, save png, and make wfc

	if(m_thrdTotal == 0)
	{
		if(c_gameTDB.IsLoaded())
			c_gameTDB.CloseFile();
		coverIDList.clear();
		return -3;
	}

	/* Initialize network connection */
	m_thrdMessage = _t("dlmsg1", L"Initializing network...");
	m_thrdMessageAdded = true;
	if(_initNetwork() < 0)
	{
		if(c_gameTDB.IsLoaded())
			c_gameTDB.CloseFile();
		coverIDList.clear();
		return -2;
	}

	/* Download covers in the list */
	for(u32 i = 0; i < coverIDList.size(); ++i)
	{
		string url;
		char path[256];
		path[255] = '\0';
		string coverID = coverIDList[i];
		vector<string> fmtURL;
		bool success = false;
		bool fullCover = true;
		
		if(!disc)
		{
			//! case BOX
			if((m_cfg.getBool(general_domain, "dl_box_cover", true)) && (m_cfg.getBool(general_domain, "dl_normal_cover", true)))
				fmtURL = stringToVector(m_cfg.getString(general_domain, "url_full_covers", FMT_BPIC_URL), '|');
			//! case FLAT
			else if((!m_cfg.getBool(general_domain, "dl_box_cover", true)) && (m_cfg.getBool(general_domain, "dl_normal_cover", true)))
			{
				fmtURL = stringToVector(m_cfg.getString(general_domain, "url_flat_covers", FMT_PIC_URL), '|');
				fullCover = false;
			}
			//! case ALT
			else if(!m_cfg.getBool(general_domain, "dl_normal_cover", true))
			{
				if(c_gameTDB.IsLoaded() && c_gameTDB.GetCaseVersions(coverID.c_str()) < 2) // < 2 means there's no alt case
					continue; // don't try to download if no alt case found in gameTDB
				//! case BOX ALT
				if(m_cfg.getBool(general_domain, "dl_box_cover", true))
					fmtURL = stringToVector(m_cfg.getString(general_domain, "url_custom_full_covers", FMT_CBPIC_URL), '|');
				//! case FLAT ALT
				else
				{
					fmtURL = stringToVector(m_cfg.getString(general_domain, "url_custom_flat_covers", FMT_CPIC_URL), '|');
					fullCover = false;
				}
			}
		}
		else // disc downloader
		{
			fmtURL = stringToVector(m_cfg.getString(general_domain, "url_discs", FMT_DPIC_URL), '|');
		}

		/* Each fmtURL may have more than one URL */
		for(u8 j = 0; !success && j < fmtURL.size(); ++j)
		{
			url = makeURL(fmtURL[j], coverID, countryCode(coverID));
			
			m_thrdMessage = wfmt(_fmt("dlmsg3", L"Downloading %s.png (%i/%i)"), coverID.c_str(), i + 1, n);
			// gprintf("Downloading from %s\n", url.c_str());
			m_thrdMessageAdded = true;
			struct download file = {};
			downloadfile(url.c_str(), &file);

			for(int o = 0; o < 14; ++o)
			{
				bool tdl = false; // tdl = try download
				// if(file.size > 0) && checkPNGBuf(file.data))
				if(file.size > 0)
					break;
				int addRegion = m_cfg.getInt(general_domain, "dl_add_region", 0);
				switch( o )
				{
					case EN: //! force EN if disc for PAL games because PAL disc labels are -all- EN
						// if(((coverID[3] == 'E' || coverID[3] == 'X' || coverID[3] == 'Y' || coverID[3] == 'P') && addRegion == EN)
						if((coverID[3] != 'J' && addRegion == EN) || ((coverID[3] == 'X' || coverID[3] == 'Y' || coverID[3] == 'P' || coverID[3] == 'F' || coverID[3] == 'D' || coverID[3] == 'S' || coverID[3] == 'I') && disc)) 
						{
							url = makeURL(fmtURL[j], coverID, "EN");
							tdl = true;
						}
						break;
					case JA:
						if(coverID[3] == 'J' && addRegion == JA)
						{
							url = makeURL(fmtURL[j], coverID, "JA");
							tdl = true;
						}
						break;
					case FR:
						if((coverID[3] == 'F' || coverID[3] == 'P') && addRegion == FR)
						{
							url = makeURL(fmtURL[j], coverID, "FR");
							tdl = true;
						}
						break;
					case DE:
						if((coverID[3] == 'D' || coverID[3] == 'P') && addRegion == DE)
						{
							url = makeURL(fmtURL[j], coverID, "DE");
							tdl = true;
						}
						break;
					case ES:
						if((coverID[3] == 'S' || coverID[3] == 'P') && addRegion == ES)
						{
							url = makeURL(fmtURL[j], coverID, "ES");
							tdl = true;
						}
						break;
					case IT:
						if((coverID[3] == 'I' || coverID[3] == 'P') && addRegion == IT)
						{
							url = makeURL(fmtURL[j], coverID, "IT");
							tdl = true;
						}
						break;
					case NL:
						if(coverID[3] == 'P' && addRegion == NL)
						{
							url = makeURL(fmtURL[j], coverID, "NL");
							tdl = true;
						}
						break;
					case PT:
						if(coverID[3] == 'P' && addRegion == PT)
						{
							url = makeURL(fmtURL[j], coverID, "PT");
							tdl = true;
						}
						break;
					case RU:
						if((coverID[3] == 'R' || coverID[3] == 'P') && addRegion == RU)
						{
							url = makeURL(fmtURL[j], coverID, "RU");
							tdl = true;
						}
						break;
					case KO:
						if(coverID[3] == 'K' && addRegion == KO)
						{
							url = makeURL(fmtURL[j], coverID, "KO");
							tdl = true;
						}
						break;
					case ZHCN:
						if(coverID[3] == 'W' && addRegion == ZHCN)
						{
							url = makeURL(fmtURL[j], coverID, "ZH");
							tdl = true;
						}
						break;
					case AU:
						break;
					case US: // added
						if(coverID[3] != 'J' && addRegion == US)
						{
							string coverIDtmp = coverID;
							coverIDtmp[3] = 'E';
							url = makeURL(fmtURL[j], coverIDtmp, "US");
							tdl = true;
						}
						break;
				}
				if(tdl) // try another download
				{
					m_thrdMessage = wfmt(_fmt("dlmsg3", L"Downloading %s.png (%i/%i)"), coverID.c_str(), i + 1, n);
					// gprintf("Try downloading from %s\n", url.c_str());
					m_thrdMessageAdded = true;
					downloadfile(url.c_str(), &file);
				}
			}
			/* If none of the downloads succeeded */
			if(file.size == 0)// || !checkPNGBuf(file.data))
				continue;
			
			/* A download succeeded */
			
			/* Save cover or disc png */
			if(!disc)
			{
				if(fullCover)
					strncpy(path, fmt("%s/%s.png", m_boxPicDir.c_str(), coverID.c_str()), 255);
				else
					strncpy(path, fmt("%s/%s.png", m_picDir.c_str(), coverID.c_str()), 255);
			}
			else
				strncpy(path, fmt("%s/%s.png", m_cartDir.c_str(), coverID.c_str()), 255);				
			update_pThread(1);
			m_thrdMessage = wfmt(_fmt("dlmsg4", L"Saving %s"), path);
			m_thrdMessageAdded = true;
			fsop_WriteFile(path, file.data, file.size);
			MEM2_free(file.data);
			if(!disc)
			{
				/* Make cover cache file (wfc) */
				update_pThread(1);
				m_thrdMessage = wfmt(_fmt("dlmsg10", L"Making %s.wfc"), coverID.c_str());
				m_thrdMessageAdded = true;
				CoverFlow.cacheCoverFile(fmt("%s/%s.wfc", m_cacheDir.c_str(), coverID.c_str()), path, fullCover); // it may fail
			}
			if(fullCover) // fullCover is always true if disc is true
				++count;
			else
				++countFlat;
			update_pThread(1);
			success = true;
		}
		if(!success)
			update_pThread(3);
	}

	/* Cover list done and downloading complete */
	if(c_gameTDB.IsLoaded())
		c_gameTDB.CloseFile();
	coverIDList.clear();
	return 0;
}

/********************************************************************************************************/

int CMenu::_gametdbDownloaderAsync()
{
	const string &langCode = m_loc.getString(m_curLanguage, "gametdb_code", "EN");
	string gametdb_url = m_cfg.getString(general_domain, "url_gametdb", GAMETDB_URL); //
	m_thrdTotal = 3; // download, save, and unzip
	
	m_thrdMessage = _t("dlmsg1", L"Initializing network...");
	m_thrdMessageAdded = true;
	if(_initNetwork() < 0)
	{
		return -2;
	}
	else
	{
		m_thrdMessage = _t("dlmsg11", L"Downloading...");
		m_thrdMessageAdded = true;
		struct download file = {};
		downloadfile(fmt(gametdb_url.c_str(), langCode.c_str()), &file); //
		if(file.size <= 0)
		{
			return -3;
		}
		else
		{
			update_pThread(1); // it's downloaded
			bool res = false;
			char *zippath = fmt_malloc("%s/wiitdb.zip", m_settingsDir.c_str());
			if(zippath != NULL)
			{
				// gprintf("Writing file to '%s'\n", zippath);
				fsop_deleteFile(zippath);
				m_thrdMessage = wfmt(_fmt("dlmsg4", L"Saving %s"), "wiitdb.zip");
				m_thrdMessageAdded = true;	
				res = fsop_WriteFile(zippath, file.data, file.size);
				// if(file.size > 0)
					// free(file.data);
				MEM2_free(file.data);
			}
			if(res == false)
			{
				gprintf("Can't save zip file\n");
				if(zippath != NULL)
					MEM2_free(zippath);
				return -4;
			}
			else
			{
				update_pThread(1); // it's saved
				// gprintf("Extracting zip file: ");
				m_thrdMessage = wfmt(_fmt("dlmsg24", L"Extracting %s"), "wiitdb.zip");
				m_thrdMessageAdded = true;	
				ZipFile zFile(zippath);
				zFile.ExtractAll(m_settingsDir.c_str());
				// bool zres = zFile.ExtractAll(m_settingsDir.c_str());
				// gprintf(zres ? "success\n" : "failed\n");
				// may add if zres failed return -4 extraction failed

				/* We don't need the zipfile anymore */
				fsop_deleteFile(zippath);
				MEM2_free(zippath);

				/* We should always remove the offsets file to make sure it's reloaded */
				fsop_deleteFile(fmt("%s/gametdb_offsets.bin", m_settingsDir.c_str()));
				
				update_pThread(1); // it's extracted

				/* Update cache */
				m_cfg.setBool(wii_domain, "update_cache", true);
				m_cfg.setBool(gc_domain, "update_cache", true);
				m_cfg.setBool(channel_domain, "update_cache", true);
				m_refreshGameList = true;
			}
		}
	}
	return 0;
}

/********************************************************************************************************/

int CMenu::_bannerDownloader()
{
	vector<string> BnrIDList;
	count = 0;

	if(dl_gameID.empty())
	{
		currentPartition = m_cfg.getInt(gc_domain, "partition", USB1);
		string gameDir(fmt(gc_games_dir, DeviceName[currentPartition]));
		string cacheDir(fmt("%s/%s_gamecube.db", m_listCacheDir.c_str(), DeviceName[currentPartition]));
		m_cacheList.CreateList(COVERFLOW_GAMECUBE, currentPartition, gameDir, stringToVector(".iso|.ciso|root", '|'), cacheDir, false);
		
		for(u32 i = 0; i < m_cacheList.size(); ++i)
		{
			if(!fsop_FileExist(fmt("%s/%s.bnr", m_customBnrDir.c_str(), m_cacheList[i].id)))
				BnrIDList.push_back(m_cacheList[i].id);
		}
		m_cacheList.clear();
	}
	else
		BnrIDList.push_back(dl_gameID);
	
	n = BnrIDList.size();
	m_thrdTotal = n;
	
	if(n == 0)
	{
		BnrIDList.clear();
		return -3;
	}
	
	const char *banner_url = NULL;
	const char *banner_url_id3 = NULL;
	const char *GAME_BNR_ID = "{gameid}";
	string base_url = m_cfg.getString(general_domain, "url_custom_banner", CUSTOM_BANNER_URL);
	if(base_url.size() < 3 || base_url.find(GAME_BNR_ID) == string::npos)
	{
		BnrIDList.clear();
		return -1;
	}

	m_thrdMessage = _t("dlmsg1", L"Initializing network...");
	m_thrdMessageAdded = true;
	if(_initNetwork() < 0)
	{
		BnrIDList.clear();
		return -2;
	}
	
	for(u32 i = 0; i < BnrIDList.size(); ++i)
	{
		string base_url_id6 = base_url;
		base_url_id6.replace(base_url_id6.find(GAME_BNR_ID), strlen(GAME_BNR_ID), BnrIDList[i]);
		banner_url = base_url_id6.c_str();

		string base_url_id3 = base_url;
		base_url_id3.replace(base_url_id3.find(GAME_BNR_ID), strlen(GAME_BNR_ID), BnrIDList[i].c_str(), 3);
		banner_url_id3 = base_url_id3.c_str();

		if(dl_gameID.empty())
			m_thrdMessage = wfmt(_fmt("dlmsg32", L"Downloading banner %i/%i"), i + 1, n);
		else
			m_thrdMessage = _t("cfgbnr7", L"Downloading banner...");
		m_thrdMessageAdded = true;

		struct download file = {};
		downloadfile(banner_url, &file);
		if(file.size < 0x5000)
		{
			if(file.size > 0)
				MEM2_free(file.data); // more than 0 bytes and less than 50kb			
			downloadfile(banner_url_id3, &file);
		}

		/* Minimum 50kb */
		if(file.size > 51200 && file.data[0] != '<')
		{
			fsop_WriteFile(fmt("%s/%s.bnr", m_customBnrDir.c_str(), BnrIDList[i].c_str()), file.data, file.size);
			count++;
		}
		if(file.size > 0)
			MEM2_free(file.data);
		update_pThread(1);
	}
	return 0;
}

/*******************************************************************************************************/

/*
const char *url_dl = NULL;
void CMenu::_downloadUrl(const char *url, u8 **dl_file, u32 *dl_size) // nothing uses this
{
	m_file = NULL;
	m_filesize = 0;
	url_dl = url;

	m_btnMgr.show(m_downloadPBar);
	m_btnMgr.setProgress(m_downloadPBar, 0.f);
	m_btnMgr.show(m_downloadBtnCancel);
	m_btnMgr.setText(m_downloadBtnCancel, _t("dl1", L"Cancel"));
	m_thrdStop = false;
	m_thrdMessageAdded = false;

	m_thrdWorking = true;
	lwp_t thread = LWP_THREAD_NULL;
	LWP_CreateThread(&thread, _downloadUrlAsync, this, downloadStack, downloadStackSize, 40);

	wstringEx prevMsg;
	while(m_thrdWorking)
	{
		_mainLoopCommon();
		if ((BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED) && !m_thrdWorking)
			break;
		if ((BTN_A_OR_2_PRESSED) && !(m_thrdWorking && m_thrdStop))
		{
			if (m_btnMgr.selected(m_downloadBtnCancel))
			{
				LockMutex lock(m_mutex);
				m_thrdStop = true;
				m_thrdMessageAdded = true;
				m_thrdMessage = _t("dlmsg6", L"Canceling...");
			}
		}
		if (Sys_Exiting())
		{
			LockMutex lock(m_mutex);
			m_thrdStop = true;
			m_thrdMessageAdded = true;
			m_thrdMessage = _t("dlmsg6", L"Canceling...");
			m_thrdWorking = false;
		}
		if (m_thrdMessageAdded)
		{
			LockMutex lock(m_mutex);
			m_thrdMessageAdded = false;
			m_btnMgr.setProgress(m_downloadPBar, m_thrdProgress);
			if (prevMsg != m_thrdMessage)
			{
				prevMsg = m_thrdMessage;
				m_btnMgr.setText(m_downloadLblMessage[0], m_thrdMessage, false);
				m_btnMgr.hide(m_downloadLblMessage[0], 0, 0, -1.f, -1.f, true);
				m_btnMgr.show(m_downloadLblMessage[0]);
			}
		}
		if (m_thrdStop && !m_thrdWorking)
			break;
	}
	if (thread != LWP_THREAD_NULL)
	{
		LWP_JoinThread(thread, NULL);
		thread = LWP_THREAD_NULL;
	}
	m_btnMgr.hide(m_downloadLblMessage[0]);
	m_btnMgr.hide(m_downloadPBar);
	m_btnMgr.hide(m_downloadBtnCancel);

	*dl_file = m_file;
	*dl_size = m_filesize;

	m_file = NULL;
	m_filesize = 0;
	url_dl = url;
}

void * CMenu::_downloadUrlAsync(void *obj)
{
	CMenu *m = (CMenu *)obj;
	if (!m->m_thrdWorking)
		return 0;

	m->m_thrdStop = false;

	LWP_MutexLock(m->m_mutex);
	m->_setThrdMsg(m->_t("dlmsg11", L"Downloading..."), 0);
	LWP_MutexUnlock(m->m_mutex);

	if(m->_initNetwork() < 0 || url_dl == NULL)
	{
		m->m_thrdWorking = false;
		return 0;
	}

	u32 bufferSize = 0x400000; // 4mb max
	m->m_buffer = (u8*)MEM2_alloc(bufferSize);
	if(m->m_buffer == NULL)
	{
		m->m_thrdWorking = false;
		return 0;
	}
	//block file = downloadfile(m->m_buffer, bufferSize, url_dl, CMenu::_downloadProgress, m);
	DCFlushRange(m->m_buffer, bufferSize);
	//m->m_file = file.data;
	//m->m_filesize = file.size;
	m->m_thrdWorking = false;
	return 0;
}
*/