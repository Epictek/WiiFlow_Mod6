
// #include <algorithm>

#include "menu.hpp"
// #include "gui/GameTDB.hpp"
// #include "plugin/plugin.hpp"
// #include "plugin/crc32.h"
// #include "unzip/ZipFile.h"
// #include "banner/BannerWindow.hpp"

bool noSynopsis = true;
bool tdb_found = false;
bool dl_tdb = false;
int synopsis_h;
int rominfo_h;
int rominfo_th = 0;
int synopsis_th = 0;
u8 cnt_controlsreq = 0, cnt_controls = 0;
static u8 curPage;

s16 m_gameinfoLblSynopsis;
s16 m_gameinfoLblRating;
s16 m_gameinfoLblWifiplayers;
s16 m_gameinfoLblUser[4];
s16 m_gameinfoLblControlsReq[4];
s16 m_gameinfoLblControls[4];
s16 m_gameinfoLblSnap;
s16 m_gameinfoLblCartDisk;
s16 m_gameinfoLblOverlay;
s16 m_gameinfoLblRomInfo;

bool CMenu::_gameinfo(void)
{
	bool launchGame = false;
	int pixels_to_skip = 10;	
	int amount_of_skips = 0;
	int xtra_skips = 0;
	curPage = 1;

	_setBg(m_configBg, m_configBg);
	SetupInput();
	_showGameInfo();
	
	while(!m_exit)
	{
		_mainLoopCommon();
		CoverFlow.tick();
		
		if(BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED)
			break;
		
		else if((wBtn_Pressed(WPAD_BUTTON_RIGHT, WPAD_EXP_NONE) && ShowPointer()) // wiimote vertical
			|| (wBtn_Pressed(WPAD_BUTTON_DOWN, WPAD_EXP_NONE) && !ShowPointer()) // wiimote sideways
			|| (wBtn_Pressed(WPAD_CLASSIC_BUTTON_RIGHT, WPAD_EXP_CLASSIC)) // classic controller
			|| GBTN_RIGHT_PRESSED) // gamecube controller
		{
			_hideGameInfoPg(true);
			CoverFlow.down();
			m_newGame = true;
			curPage = 1;
			amount_of_skips = 0;
			xtra_skips = 0;
			_showGameInfo();
			_playGameSound(); // changes banner and game sound
		}
		else if((wBtn_Pressed(WPAD_BUTTON_LEFT, WPAD_EXP_NONE) && ShowPointer()) // wiimote vertical
			|| (wBtn_Pressed(WPAD_BUTTON_UP, WPAD_EXP_NONE) && !ShowPointer()) // wiimote sideways
			|| (wBtn_Pressed(WPAD_CLASSIC_BUTTON_LEFT, WPAD_EXP_CLASSIC)) // classic controller
			|| GBTN_LEFT_PRESSED) // gamecube controller
		{
			_hideGameInfoPg(true);
			CoverFlow.up();
			m_newGame = true;
			curPage = 1;
			amount_of_skips = 0;
			xtra_skips = 0;
			_showGameInfo();
			_playGameSound();
		}
		
		else if(((wBtn_Pressed(WPAD_BUTTON_DOWN, WPAD_EXP_NONE) || wBtn_Held(WPAD_BUTTON_DOWN, WPAD_EXP_NONE)) && ShowPointer()) // wiimote vertical
			|| ((wBtn_Pressed(WPAD_BUTTON_LEFT, WPAD_EXP_NONE) || wBtn_Held(WPAD_BUTTON_LEFT, WPAD_EXP_NONE)) && !ShowPointer()) // wiimote sideways
			|| (wBtn_Pressed(WPAD_CLASSIC_BUTTON_DOWN, WPAD_EXP_CLASSIC) || wBtn_Held(WPAD_CLASSIC_BUTTON_DOWN, WPAD_EXP_CLASSIC)) // classic controller
			|| (GBTN_DOWN_PRESSED || GBTN_DOWN_HELD)) // gamecube controller
		{
			if(curPage == 2 && synopsis_th > synopsis_h)
			{
				if((synopsis_th - amount_of_skips * pixels_to_skip) >= synopsis_h)
				{
					m_btnMgr.moveBy(m_gameinfoLblSynopsis, 0, -pixels_to_skip);
					amount_of_skips++;
				}
				else if((synopsis_th - amount_of_skips * pixels_to_skip) < synopsis_h && xtra_skips == 0)
				{
					xtra_skips = pixels_to_skip - ((synopsis_th - amount_of_skips * pixels_to_skip) - synopsis_h);
					m_btnMgr.moveBy(m_gameinfoLblSynopsis, 0, -xtra_skips);
				}
			}
			else if(curPage == 1 && rominfo_th > rominfo_h)
			{
				if((rominfo_th - amount_of_skips * pixels_to_skip) >= rominfo_h)
				{
					m_btnMgr.moveBy(m_gameinfoLblRomInfo, 0, -pixels_to_skip);
					amount_of_skips++;
				}
				else if((rominfo_th - amount_of_skips * pixels_to_skip) < rominfo_h && xtra_skips == 0)
				{
					xtra_skips = pixels_to_skip - ((rominfo_th - amount_of_skips * pixels_to_skip) - rominfo_h);
					m_btnMgr.moveBy(m_gameinfoLblRomInfo, 0, -xtra_skips);
				}
			}
		}
		else if(((wBtn_Pressed(WPAD_BUTTON_UP, WPAD_EXP_NONE) || wBtn_Held(WPAD_BUTTON_UP, WPAD_EXP_NONE)) && ShowPointer()) // wiimote vertical
			|| ((wBtn_Pressed(WPAD_BUTTON_RIGHT, WPAD_EXP_NONE) || wBtn_Held(WPAD_BUTTON_RIGHT, WPAD_EXP_NONE)) && !ShowPointer()) // wiimote sideways
			|| (wBtn_Pressed(WPAD_CLASSIC_BUTTON_UP, WPAD_EXP_CLASSIC) || wBtn_Held(WPAD_CLASSIC_BUTTON_UP, WPAD_EXP_CLASSIC)) // classic controller
			|| (GBTN_UP_PRESSED || GBTN_UP_HELD)) // gamecube controller
		{
			if(xtra_skips > 0)
			{
				m_btnMgr.moveBy(curPage == 1 ? m_gameinfoLblRomInfo : m_gameinfoLblSynopsis, 0, xtra_skips);
				xtra_skips = 0;
			}
			else if(amount_of_skips > 0)
			{
				m_btnMgr.moveBy(curPage == 1 ? m_gameinfoLblRomInfo : m_gameinfoLblSynopsis, 0, pixels_to_skip);
				amount_of_skips--;
			}
		}
		else if(BTN_MINUS_PRESSED && tdb_found && !noSynopsis)
		{
			curPage = curPage == 1 ? 2 : curPage - 1;
			amount_of_skips = 0;
			xtra_skips = 0;
			m_btnMgr.click(m_configBtnPageM);
			_hideGameInfoPg(true);
			_showGameInfoPg();
		}
		else if(BTN_PLUS_PRESSED && tdb_found && !noSynopsis)
		{
			curPage = curPage == 2 ? 1 : curPage + 1;
			amount_of_skips = 0;
			xtra_skips = 0;
			m_btnMgr.click(m_configBtnPageP);
			_hideGameInfoPg(true);
			_showGameInfoPg();
		}
		else if(BTN_A_OR_2_PRESSED)
		{
			if(m_btnMgr.selected(m_configBtnBack))
				break;
			else if(m_btnMgr.selected(m_configBtnPageM) && tdb_found && !noSynopsis)
			{
				curPage = curPage == 1 ? 2 : curPage - 1;
				amount_of_skips = 0;
				xtra_skips = 0;
				_hideGameInfoPg(true);
				_showGameInfoPg();
			}
			else if(m_btnMgr.selected(m_configBtnPageP) && tdb_found && !noSynopsis)
			{
				curPage = curPage == 2 ? 1 : curPage + 1;
				amount_of_skips = 0;
				xtra_skips = 0;
				_hideGameInfoPg(true);
				_showGameInfoPg();
			}
			else if(m_btnMgr.selected(m_configBtnCenter) || !ShowPointer())
			{
				launchGame = true;
				break;
			}
		}
	}
	_hideGameInfo();
	
	TexHandle.Cleanup(m_cart);
	TexHandle.Cleanup(m_snap);
	TexHandle.Cleanup(m_overlay);
	for(u8 i = 0; i < ARRAY_SIZE(m_gameinfoLblControlsReq); i++)
		TexHandle.Cleanup(m_controlsreq[i]);
	for(u8 i = 0; i < ARRAY_SIZE(m_gameinfoLblControls); i++)
		TexHandle.Cleanup(m_controls[i]);
	TexHandle.Cleanup(m_wifi);
	TexHandle.Cleanup(m_rating);
	
	if(dl_tdb && !launchGame)
		_download();

	return launchGame;
}

void CMenu::_hideGameInfo(bool instant)
{
	for(u8 i = 0; i < ARRAY_SIZE(m_gameinfoLblUser); ++i)
		m_btnMgr.hide(m_gameinfoLblUser[i], instant);
	
	_hideConfig(instant);
	_hideGameInfoPg(instant);	
}

void CMenu::_hideGameInfoPg(bool instant)
{
	m_btnMgr.hide(m_configLblPage);
	m_btnMgr.hide(m_configBtnPageM);
	m_btnMgr.hide(m_configBtnPageP);	

	m_btnMgr.hide(m_gameinfoLblSynopsis, instant);
	m_btnMgr.hide(m_gameinfoLblRating, instant);
	m_btnMgr.hide(m_gameinfoLblWifiplayers, instant);
	m_btnMgr.hide(m_gameinfoLblSnap, instant);
	m_btnMgr.hide(m_gameinfoLblCartDisk, instant);
	m_btnMgr.hide(m_gameinfoLblOverlay, instant);
	m_btnMgr.hide(m_gameinfoLblRomInfo, instant);
	
	for(u8 i = 0; i < ARRAY_SIZE(m_gameinfoLblControls); ++i)
		if(m_gameinfoLblControls[i] != -1)
			m_btnMgr.hide(m_gameinfoLblControls[i], instant);	
		
	for(u8 i = 0; i < ARRAY_SIZE(m_gameinfoLblControlsReq); ++i)
		if(m_gameinfoLblControlsReq[i] != -1)
			m_btnMgr.hide(m_gameinfoLblControlsReq[i], instant);	
}

void CMenu::_showGameInfoPg(void)
{
	if(curPage == 1)
	{
		m_btnMgr.reset(m_gameinfoLblRomInfo);
		m_btnMgr.show(m_gameinfoLblRomInfo);
		if(!tdb_found)
			return;
		m_btnMgr.getTotalHeight(m_gameinfoLblRomInfo, rominfo_th);
		m_btnMgr.moveBy(m_gameinfoLblRomInfo, 0, -1);
		m_btnMgr.setText(m_configLblPage, L"1 / 2");
		m_btnMgr.show(m_gameinfoLblCartDisk);
		if(CoverFlow.getHdr()->type == TYPE_PLUGIN)
		{
			m_btnMgr.show(m_gameinfoLblSnap);
			m_btnMgr.show(m_gameinfoLblOverlay);				
		}
		else // wii & gamecube games
		{
			m_btnMgr.show(m_gameinfoLblWifiplayers);
			m_btnMgr.show(m_gameinfoLblRating);

			for(u8 i = 0; i < ARRAY_SIZE(m_gameinfoLblControlsReq); ++i)
				if(m_gameinfoLblControlsReq[i] != -1 && i < cnt_controlsreq)
					m_btnMgr.show(m_gameinfoLblControlsReq[i]);
				
			for(u8 i = 0; i < ARRAY_SIZE(m_gameinfoLblControls); ++i)
				if(m_gameinfoLblControls[i] != -1 && i < cnt_controls)
					m_btnMgr.show(m_gameinfoLblControls[i]);
		}
	}
	else // curPage 2
	{
		m_btnMgr.reset(m_gameinfoLblSynopsis);
		m_btnMgr.show(m_gameinfoLblSynopsis);
		m_btnMgr.getTotalHeight(m_gameinfoLblSynopsis, synopsis_th);
		m_btnMgr.moveBy(m_gameinfoLblSynopsis, 0, -1);
		
		m_btnMgr.setText(m_configLblPage, L"2 / 2");
	}
	if(!noSynopsis)
	{
		m_btnMgr.show(m_configLblPage);
		m_btnMgr.show(m_configBtnPageM);
		m_btnMgr.show(m_configBtnPageP);
	}
}

void CMenu::_showGameInfo(void)
{
	_textGameInfo();

	m_btnMgr.show(m_configBtnCenter);
	m_btnMgr.setText(m_configBtnCenter, _t("gm1", L"Play"));	
	m_btnMgr.show(m_configBtnBack);
	m_btnMgr.show(m_configLblTitle);
	for(u8 i = 0; i < ARRAY_SIZE(m_gameinfoLblUser); ++i)
			m_btnMgr.show(m_gameinfoLblUser[i]);
	
	_showGameInfoPg();
}

void CMenu::_initGameInfoMenu()
{
	TexData emptyTex;

	_addUserLabels(m_gameinfoLblUser, ARRAY_SIZE(m_gameinfoLblUser), "GAMEINFO");
	
	m_gameinfoLblRating = _addLabel("GAMEINFO/RATING", theme.txtFont, L"", 548, 320, 48, 60, theme.txtFontColor, 0, m_rating);
	m_gameinfoLblSynopsis = _addText("GAMEINFO/SYNOPSIS", theme.txtFont, L"", 40, 95, 560, 285, theme.txtFontColor, FTGX_JUSTIFY_LEFT | FTGX_ALIGN_TOP);
	m_gameinfoLblWifiplayers = _addLabel("GAMEINFO/WIFIPLAYERS", theme.txtFont, L"", 470, 320, 68, 60, theme.txtFontColor, 0, m_wifi);
	m_gameinfoLblSnap = _addLabel("GAMEINFO/SNAP", theme.txtFont, L"", 350, 90, 100, 100, theme.txtFontColor, 0, m_snap);
	m_gameinfoLblOverlay = _addLabel("GAMEINFO/OVERLAY", theme.txtFont, L"", 350, 90, 100, 100, theme.txtFontColor, 0, m_overlay);
	m_gameinfoLblCartDisk = _addLabel("GAMEINFO/CART", theme.txtFont, L"", 330, 260, 100, 100, theme.txtFontColor, 0, m_cart);
	m_gameinfoLblRomInfo = _addText("GAMEINFO/ROMINFO", theme.txtFont, L"", 40, 95, 300, 285, theme.txtFontColor, FTGX_JUSTIFY_LEFT | FTGX_ALIGN_TOP);

	for(u8 i = 0; i < ARRAY_SIZE(m_gameinfoLblControlsReq); ++i)
	{
		string dom(fmt("GAMEINFO/CONTROLSREQ%i", i + 1));
		m_gameinfoLblControlsReq[i] = _addLabel(dom.c_str(), theme.txtFont, L"", 550 - (i*60), 100, 60, 40, theme.txtFontColor, 0, emptyTex);
		_setHideAnim(m_gameinfoLblControlsReq[i], dom.c_str(), 0, -100, 0.f, 0.f);
	}

	for(u8 i = 0; i < ARRAY_SIZE(m_gameinfoLblControls); ++i)
	{
		string dom(fmt("GAMEINFO/CONTROLS%i", i + 1));
		m_gameinfoLblControls[i] = _addLabel(dom.c_str(), theme.txtFont, L"", 550 - (i*60), 170, 60, 40, theme.txtFontColor, 0, emptyTex);
		_setHideAnim(m_gameinfoLblControls[i], dom.c_str(), 0, -100, 0.f, 0.f);
	}

	_setHideAnim(m_gameinfoLblRating, "GAMEINFO/RATING", 0, -100, 0.f, 0.f);
	_setHideAnim(m_gameinfoLblSynopsis, "GAMEINFO/SYNOPSIS", 0, 700, 1.f, 1.f);
	_setHideAnim(m_gameinfoLblWifiplayers, "GAMEINFO/WIFIPLAYERS", 0, -100, 0.f, 0.f);
	_setHideAnim(m_gameinfoLblSnap, "GAMEINFO/SNAP", 0, -100, 0.f, 0.f);
	_setHideAnim(m_gameinfoLblCartDisk, "GAMEINFO/CART", 0, -100, 0.f, 0.f);
	_setHideAnim(m_gameinfoLblOverlay, "GAMEINFO/OVERLAY", 0, -100, 0.f, 0.f);
	_setHideAnim(m_gameinfoLblRomInfo, "GAMEINFO/ROMINFO", 0, -100, 0.f, 0.f);

	// _hideGameInfo(true);
	synopsis_h = m_theme.getInt("GAMEINFO/SYNOPSIS", "height", 290);
	rominfo_h = m_theme.getInt("GAMEINFO/ROMINFO", "height", 290);
}

extern const u8 gi_balanceboard_png[];
extern const u8 gi_balanceboardR_png[];
extern const u8 gi_cero_a_png[];
extern const u8 gi_cero_b_png[];
extern const u8 gi_cero_c_png[];
extern const u8 gi_cero_d_png[];
extern const u8 gi_cero_z_png[];
extern const u8 gi_classiccontroller_png[];
extern const u8 gi_dancepad_png[];
extern const u8 gi_dancepadR_png[];
extern const u8 gi_drums_png[];
extern const u8 gi_drumsR_png[];
extern const u8 gi_esrb_ao_png[];
extern const u8 gi_esrb_e_png[];
extern const u8 gi_esrb_ec_png[];
extern const u8 gi_esrb_eten_png[];
extern const u8 gi_esrb_m_png[];
extern const u8 gi_esrb_t_png[];
extern const u8 gi_gcncontroller_png[];
extern const u8 gi_grb_12_png[];
extern const u8 gi_grb_15_png[];
extern const u8 gi_grb_18_png[];
extern const u8 gi_grb_a_png[];
extern const u8 gi_guitar_png[];
extern const u8 gi_guitarR_png[];
extern const u8 gi_keyboard_png[];
extern const u8 gi_microphone_png[];
extern const u8 gi_microphoneR_png[];
extern const u8 gi_motionplus_png[];
extern const u8 gi_motionplusR_png[];
extern const u8 gi_norating_png[];
extern const u8 gi_nunchuk_png[];
extern const u8 gi_nunchukR_png[];
extern const u8 gi_pegi_3_png[];
extern const u8 gi_pegi_7_png[];
extern const u8 gi_pegi_12_png[];
extern const u8 gi_pegi_16_png[];
extern const u8 gi_pegi_18_png[];
extern const u8 gi_udraw_png[];
extern const u8 gi_udrawR_png[];
extern const u8 gi_wheel_png[];
extern const u8 gi_wifi1_png[];
extern const u8 gi_wifi2_png[];
extern const u8 gi_wifi3_png[];
extern const u8 gi_wifi4_png[];
extern const u8 gi_wifi8_png[];
extern const u8 gi_wifi10_png[];
extern const u8 gi_wifi12_png[];
extern const u8 gi_wifi16_png[];
extern const u8 gi_wifi18_png[];
extern const u8 gi_wifi32_png[];
extern const u8 gi_wiispeak_png[];
extern const u8 gi_zapper_png[];
extern const u8 gi_wiimote1_png[];
extern const u8 gi_wiimote2_png[];
extern const u8 gi_wiimote3_png[];
extern const u8 gi_wiimote4_png[];
extern const u8 gi_wiimote6_png[];
extern const u8 gi_wiimote8_png[];

void CMenu::_textGameInfo(void)
{
	cnt_controlsreq = 0;
	cnt_controls = 0;
	char GameID[7];
	GameID[6] = '\0';
	char platformName[16];
	const char *TMP_Char = NULL;
	tdb_found = false;
	dl_tdb = false;
	GameTDB gametdb;
	TexData emptyTex;
	const dir_discHdr *GameHdr = CoverFlow.getHdr();
	bool wide = m_vid.wide();
	wstringEx rom_info = L"", gameinfo_Synopsis_w;
	int wiiRegion = CONF_GetRegion();
	u32 playCount = 0;
	time_t lastPlayed;
	int PublishDate, year, day, month;
	u8 players;

/***************************************** Playcount info ***********************************************/
	
	if(GameHdr->type != TYPE_HOMEBREW)
	{
		const char *id = NULL;
		if(GameHdr->type == TYPE_PLUGIN)
		{
			string romFileName(GameHdr->path);
			romFileName = romFileName.substr(romFileName.find_last_of("/") + 1);
			romFileName = romFileName.substr(0, romFileName.find_last_of("."));
			id = fmt("%08x/%.63s", GameHdr->settings[0], romFileName.c_str());
		}
		else
			id = GameHdr->id;
		playCount = m_gcfg1.getInt("PLAYCOUNT", id, 0);
		lastPlayed = m_gcfg1.getUInt("LASTPLAYED", id, 0);
	}
	if(playCount > 0)
	{
		struct tm *timeinfo;
		char buffer[20];
		timeinfo = localtime(&lastPlayed);
		switch(wiiRegion)
		{
			case 1: // US
				strftime(buffer, 20, "%m-%d-%y", timeinfo);
				break;
			case 2: // EUR
				strftime(buffer, 20, "%d/%m/%Y", timeinfo);
				break;
			default:
				strftime(buffer, 20, "%Y-%m-%d", timeinfo);
				break;
		}
		rom_info.append(wfmt(_fmt("gameinfo98",L"Play count: %i"), playCount));
		rom_info.append(L"\n\n");
		rom_info.append(wfmt(_fmt("gameinfo99", L"Last on: %s"), buffer));
		rom_info.append(L"\n\n");
	}
	
/***************************************** Plugin game info *********************************************/
	if(GameHdr->type == TYPE_PLUGIN)
	{
		/* Get title */
		m_btnMgr.setText(m_configLblTitle, GameHdr->title);
		
		/* Check the platform name corresponding to the current magic number.
		We can't use magic # directly since it'd require hardcoding values and a # can be several systems(genplus) 
		We can't rely on coverfolder either. Different systems can share the same folder. Or combined plugins used for the same system. */
		
		/* Is platform.ini available? */
		if(!m_platform.loaded())
			goto out; // no platform.ini found
		
		/* Search platform.ini to find plugin magic to get platformName */
		strncpy(m_plugin.PluginMagicWord, fmt("%08x", GameHdr->settings[0]), 8);
		snprintf(platformName, sizeof(platformName), "%s", m_platform.getString("PLUGINS", m_plugin.PluginMagicWord).c_str());
		strcpy(GameID, GameHdr->id);
		if(strlen(platformName) == 0 || strcasecmp(GameID, "PLUGIN") == 0)
			goto out; // no platform name found to match plugin magic #
			
		/* Check COMBINED for database platform name
		some platforms have different names per country (ex. Genesis/Megadrive)
		but we use only one platform name for both */
		string newName = m_platform.getString("COMBINED", platformName);
		if(newName.empty())
			m_platform.remove("COMBINED", platformName);
		else
			snprintf(platformName, sizeof(platformName), "%s", newName.c_str());

		/* Load platform name.xml database to get game's info using the gameID (crc/serial) */
		gametdb.OpenFile(fmt("%s/%s/%s.xml", m_pluginDataDir.c_str(), platformName, platformName));
		tdb_found = gametdb.IsLoaded();
		if(!tdb_found) // no platform xml found
			goto out;
		
		gametdb.SetLanguageCode(m_loc.getString(m_curLanguage, "gametdb_code", "EN").c_str());

		/* Get roms's title without the extra ()'s or []'s */
		string ShortName;
		if(strrchr(GameHdr->path, '/') != NULL)
			ShortName = m_plugin.GetRomName(GameHdr->path);
		else
		{
			char title[64];
			wcstombs(title, GameHdr->title, 63);
			title[63] = '\0';
			ShortName = title;
		}
		
		/* Set to empty textures in case images not found */
		m_btnMgr.setTexture(m_gameinfoLblSnap, emptyTex);
		m_btnMgr.setTexture(m_gameinfoLblCartDisk, emptyTex);
		m_btnMgr.setTexture(m_gameinfoLblOverlay, emptyTex);

		const char *snap_path = NULL;
		if(strcasestr(platformName, "ARCADE") || strcasestr(platformName, "CPS") || !strncasecmp(platformName, "NEOGEO", 6))
			snap_path = fmt("%s/%s/%s.png", m_snapDir.c_str(), platformName, ShortName.c_str());
		else if(gametdb.GetName(GameID, TMP_Char))
			snap_path = fmt("%s/%s/%s.png", m_snapDir.c_str(), platformName, TMP_Char);

		if(snap_path == NULL || !fsop_FileExist(snap_path))
			snap_path = fmt("%s/%s/%s.png", m_snapDir.c_str(), platformName, GameID);

		if(fsop_FileExist(snap_path))
		{
			TexHandle.fromImageFile(m_snap, snap_path);
			m_btnMgr.setTexture(m_gameinfoLblSnap, m_snap, m_snap.width, m_snap.height);
		}

		const char *overlay_path = fmt("%s/%s_overlay.png", m_snapDir.c_str(), platformName);
		if(fsop_FileExist(overlay_path))
		{
			TexHandle.fromImageFile(m_overlay, overlay_path);
			m_btnMgr.setTexture(m_gameinfoLblOverlay, m_overlay, m_overlay.width, m_overlay.height);
		}

		const char *cart_path = NULL;
		if(strcasestr(platformName, "ARCADE") || strcasestr(platformName, "CPS") || !strncasecmp(platformName, "NEOGEO", 6))
			cart_path = fmt("%s/%s/%s_2D.png", m_cartDir.c_str(), platformName, ShortName.c_str());
		else if(gametdb.GetName(GameID, TMP_Char))
			cart_path = fmt("%s/%s/%s_2D.png", m_cartDir.c_str(), platformName, TMP_Char);

		if(cart_path == NULL || !fsop_FileExist(cart_path))
			cart_path = fmt("%s/%s/%s_2D.png", m_cartDir.c_str(), platformName, GameID);

		if(fsop_FileExist(cart_path))
		{			
			TexHandle.fromImageFile(m_cart, cart_path);
			if(m_cart.height > 112)
			{
				u8 cart_height = wide ? 128 : 114;
				m_btnMgr.setTexture(m_gameinfoLblCartDisk, m_cart, 114, cart_height);
			}
			else
				m_btnMgr.setTexture(m_gameinfoLblCartDisk, m_cart, 160, 112);
		}
		else
			TexHandle.Cleanup(m_cart);
		
		cnt_controlsreq = 0;
	}
/************************************ Wii and GameCube game info ****************************************/
	else
	{
		/* Get title, synopsis and other fields the same way as roms (set text empty if not found) */
		m_btnMgr.setText(m_configLblTitle, GameHdr->title);
		
		gametdb.OpenFile(fmt("%s/wiitdb.xml", m_wiiTDBDir.c_str()));
		tdb_found = gametdb.IsLoaded();
		if(!tdb_found)
		{
			dl_tdb = true; // prompt to download gameTDB
			goto out;
		}

		gametdb.SetLanguageCode(m_loc.getString(m_curLanguage, "gametdb_code", "EN").c_str());

		strncpy(GameID, CoverFlow.getId(), 6);

		/* Disc label */
		m_btnMgr.setTexture(m_gameinfoLblCartDisk, emptyTex);
		const char *cart_path = NULL;
		cart_path = fmt("%s/%s.png", m_cartDir.c_str(), GameID);
		if(fsop_FileExist(cart_path))
		{			
			TexHandle.fromImageFile(m_cart, cart_path);
			m_btnMgr.setTexture(m_gameinfoLblCartDisk, m_cart, 114, wide ? 128 : 114);
		}
		else
			TexHandle.Cleanup(m_cart);
		
		/* Ratings */
		TexHandle.fromPNG(m_rating, gi_norating_png);
		
		const char *RatingValue = NULL;
		
		if(gametdb.GetRatingValue(GameID, RatingValue))
		{
			switch(gametdb.GetRating(GameID))
			{
				case GAMETDB_RATING_TYPE_CERO:
					if(RatingValue[0] == 'A')
						TexHandle.fromPNG(m_rating, gi_cero_a_png);
					else if(RatingValue[0] == 'B')
						TexHandle.fromPNG(m_rating, gi_cero_b_png);
					else if(RatingValue[0] == 'D')
						TexHandle.fromPNG(m_rating, gi_cero_d_png);
					else if(RatingValue[0] == 'C')
						TexHandle.fromPNG(m_rating, gi_cero_c_png);
					else if(RatingValue[0] == 'Z')
						TexHandle.fromPNG(m_rating, gi_cero_z_png);
					break;
				case GAMETDB_RATING_TYPE_ESRB:
					if(RatingValue[0] == 'E')
						TexHandle.fromPNG(m_rating, gi_esrb_e_png);
					else if(memcmp(RatingValue, "EC", 2) == 0)
						TexHandle.fromPNG(m_rating, gi_esrb_ec_png);
					else if(memcmp(RatingValue, "E10+", 4) == 0)
						TexHandle.fromPNG(m_rating, gi_esrb_eten_png);
					else if(RatingValue[0] == 'T')
						TexHandle.fromPNG(m_rating, gi_esrb_t_png);
					else if(RatingValue[0] == 'M')
						TexHandle.fromPNG(m_rating, gi_esrb_m_png);
					else if(memcmp(RatingValue, "AO", 2) == 0)
						TexHandle.fromPNG(m_rating, gi_esrb_ao_png);
					break;
				case GAMETDB_RATING_TYPE_PEGI:
					if(RatingValue[0] == '3')
						TexHandle.fromPNG(m_rating, gi_pegi_3_png);
					else if(RatingValue[0] == '7')
						TexHandle.fromPNG(m_rating, gi_pegi_7_png);
					else if(memcmp(RatingValue, "12", 2) == 0)
						TexHandle.fromPNG(m_rating, gi_pegi_12_png);
					else if(memcmp(RatingValue, "16", 2) == 0)
						TexHandle.fromPNG(m_rating, gi_pegi_16_png);
					else if(memcmp(RatingValue, "18", 2) == 0)
						TexHandle.fromPNG(m_rating, gi_pegi_18_png);
					break;
				case GAMETDB_RATING_TYPE_GRB:
					if(RatingValue[0] == 'A')
						TexHandle.fromPNG(m_rating, gi_grb_a_png);
					else if(memcmp(RatingValue, "12", 2) == 0)
						TexHandle.fromPNG(m_rating, gi_grb_12_png);
					else if(memcmp(RatingValue, "15", 2) == 0)
						TexHandle.fromPNG(m_rating, gi_grb_15_png);
					else if(memcmp(RatingValue, "18", 2) == 0)
						TexHandle.fromPNG(m_rating, gi_grb_18_png);
					break;
				default:
					break;
			}
		}
		m_btnMgr.setTexture(m_gameinfoLblRating, m_rating);
			
		/* Wifi players */
		int WifiPlayers = gametdb.GetWifiPlayers(GameID);

		if(WifiPlayers == 1)
			TexHandle.fromPNG(m_wifi, gi_wifi1_png);
		else if(WifiPlayers == 2)
			TexHandle.fromPNG(m_wifi, gi_wifi2_png);
		else if(WifiPlayers == 4)
			TexHandle.fromPNG(m_wifi, gi_wifi4_png);
		else if(WifiPlayers == 8)
			TexHandle.fromPNG(m_wifi, gi_wifi8_png);
		else if(WifiPlayers == 10)
			TexHandle.fromPNG(m_wifi, gi_wifi10_png);
		else if(WifiPlayers == 12)
			TexHandle.fromPNG(m_wifi, gi_wifi12_png);
		else if(WifiPlayers == 16)
			TexHandle.fromPNG(m_wifi, gi_wifi16_png);
		else if(WifiPlayers == 18)
			TexHandle.fromPNG(m_wifi, gi_wifi18_png);
		else if(WifiPlayers == 32)
			TexHandle.fromPNG(m_wifi, gi_wifi32_png);

		if(WifiPlayers > 0)
			m_btnMgr.setTexture(m_gameinfoLblWifiplayers, m_wifi);
		else 
			m_btnMgr.setTexture(m_gameinfoLblWifiplayers, emptyTex);

		/* Check required controls */
		bool wiimote = false;
		bool nunchuk = false;
		bool classiccontroller = false;
		bool balanceboard = false;
		bool dancepad = false;
		bool guitar = false;
		bool gamecube = false;
		bool motionplus = false;
		bool drums = false;
		bool microphone = false;
		bool wheel = false;
		bool keyboard = false;
		bool udraw = false;
		bool zapper = false;
		
		if(GameHdr->type == TYPE_GC_GAME)
		{
			wiimote = true;
			nunchuk = true;
		}

		vector<Accessory> Accessories;
		gametdb.GetAccessories(GameID, Accessories);
		for(vector<Accessory>::iterator acc_itr = Accessories.begin(); acc_itr != Accessories.end(); acc_itr++)
		{
			if(!acc_itr->Required)
				continue;
			if(strcmp((acc_itr->Name).c_str(), "wiimote") == 0)
				wiimote = true;
			else if(strcmp((acc_itr->Name).c_str(), "nunchuk") == 0)
				nunchuk = true;
			else if(strcmp((acc_itr->Name).c_str(), "guitar") == 0)
				guitar = true;
			else if(strcmp((acc_itr->Name).c_str(), "drums") == 0)
				drums = true;
			else if(strcmp((acc_itr->Name).c_str(), "dancepad") == 0)
				dancepad = true;
			else if(strcmp((acc_itr->Name).c_str(), "motionplus") == 0)
				motionplus = true;
			else if(strcmp((acc_itr->Name).c_str(), "microphone") == 0)
				microphone = true;
			else if(strcmp((acc_itr->Name).c_str(), "balanceboard") == 0)
				balanceboard = true;
			else if(strcmp((acc_itr->Name).c_str(), "udraw") == 0)
				udraw = true;
		}
		u8 x = 0;
		u8 max_controlsReq = ARRAY_SIZE(m_gameinfoLblControlsReq);
		
		if(wiimote && x < max_controlsReq)
		{
			u8 players = gametdb.GetPlayers(GameID);

			if(players == 1)
				TexHandle.fromPNG(m_controlsreq[x], gi_wiimote1_png);
			else if(players == 2)
				TexHandle.fromPNG(m_controlsreq[x], gi_wiimote2_png);
			else if(players == 3)
				TexHandle.fromPNG(m_controlsreq[x], gi_wiimote3_png);
			else if(players < 6)
				TexHandle.fromPNG(m_controlsreq[x], gi_wiimote4_png);
			else if(players == 6)
				TexHandle.fromPNG(m_controlsreq[x], gi_wiimote6_png);
			else
				TexHandle.fromPNG(m_controlsreq[x], gi_wiimote8_png);

			m_btnMgr.setTexture(m_gameinfoLblControlsReq[x] ,m_controlsreq[x], 52, 60);
			x++;
		}
		if(nunchuk && x < max_controlsReq)
		{
			TexHandle.fromPNG(m_controlsreq[x], gi_nunchukR_png);
			m_btnMgr.setTexture(m_gameinfoLblControlsReq[x] ,m_controlsreq[x], 52, 60);
			x++;
		}
		if(guitar && x < max_controlsReq)
		{
			TexHandle.fromPNG(m_controlsreq[x], gi_guitarR_png);
			m_btnMgr.setTexture(m_gameinfoLblControlsReq[x] ,m_controlsreq[x], 52, 60);
			x++;
		}
		if(drums && x < max_controlsReq)
		{
			TexHandle.fromPNG(m_controlsreq[x], gi_drumsR_png);
			m_btnMgr.setTexture(m_gameinfoLblControlsReq[x] ,m_controlsreq[x], 52, 60);
			x++;
		}
		if(motionplus && x < max_controlsReq)
		{
			TexHandle.fromPNG(m_controlsreq[x], gi_motionplusR_png);
			m_btnMgr.setTexture(m_gameinfoLblControlsReq[x] ,m_controlsreq[x], 52, 60);
			x++;
		}
		if(dancepad && x < max_controlsReq)
		{
			TexHandle.fromPNG(m_controlsreq[x], gi_dancepadR_png);
			m_btnMgr.setTexture(m_gameinfoLblControlsReq[x] ,m_controlsreq[x], 52, 60);
			x++;
		}
		if(microphone && x < max_controlsReq)
		{
			TexHandle.fromPNG(m_controlsreq[x], gi_microphoneR_png);
			m_btnMgr.setTexture(m_gameinfoLblControlsReq[x] ,m_controlsreq[x], 52, 60);
			x++;
		}
		if(balanceboard && x < max_controlsReq)
		{
			TexHandle.fromPNG(m_controlsreq[x], gi_balanceboardR_png);
			m_btnMgr.setTexture(m_gameinfoLblControlsReq[x] ,m_controlsreq[x], 52, 60);
			x++;
		}
		if(udraw && x < max_controlsReq)
		{
			TexHandle.fromPNG(m_controlsreq[x], gi_udrawR_png);
			m_btnMgr.setTexture(m_gameinfoLblControlsReq[x] ,m_controlsreq[x], 52, 60);
			x++;
		}
		cnt_controlsreq = x;

		/* Check optional controls */
		wiimote = false;
		nunchuk = false;
		classiccontroller = false;
		balanceboard = false;
		dancepad = false;
		guitar = false;
		gamecube = false;
		motionplus = false;
		drums = false;
		microphone = false;
		wheel = false;
		keyboard = false;
		udraw = false;
		zapper = false;
		
		if(GameHdr->type == TYPE_GC_GAME)
		{
			gamecube = true;
			classiccontroller = true;
		}
		
		for(vector<Accessory>::iterator acc_itr = Accessories.begin(); acc_itr != Accessories.end(); acc_itr++)
		{
			if(acc_itr->Required)
				continue;
			if(strcmp((acc_itr->Name).c_str(), "classiccontroller") == 0)
				classiccontroller = true;
			else if(strcmp((acc_itr->Name).c_str(), "nunchuk") == 0)
				nunchuk = true;
			else if(strcmp((acc_itr->Name).c_str(), "guitar") == 0)
				guitar = true;
			else if(strcmp((acc_itr->Name).c_str(), "drums") == 0)
				drums = true;
			else if(strcmp((acc_itr->Name).c_str(), "dancepad") == 0)
				dancepad = true;
			else if(strcmp((acc_itr->Name).c_str(), "motionplus") == 0)
				motionplus = true;
			else if(strcmp((acc_itr->Name).c_str(), "balanceboard") == 0)
				balanceboard = true;
			else if(strcmp((acc_itr->Name).c_str(), "microphone") == 0)
				microphone = true;
			else if(strcmp((acc_itr->Name).c_str(), "gamecube") == 0)
				gamecube = true;
			else if(strcmp((acc_itr->Name).c_str(), "keyboard") == 0)
				keyboard = true;
			else if(strcmp((acc_itr->Name).c_str(), "zapper") == 0)
				zapper = true;
			else if(strcmp((acc_itr->Name).c_str(), "wheel") == 0)
				wheel = true;
			else if(strcmp((acc_itr->Name).c_str(), "udraw") == 0)
				udraw = true;
		}
		x = 0;
		u8 max_controls = ARRAY_SIZE(m_gameinfoLblControls);
		if(classiccontroller && x < max_controls)
		{
			TexHandle.fromPNG(m_controls[x], gi_classiccontroller_png);
			m_btnMgr.setTexture(m_gameinfoLblControls[x] ,m_controls[x], 52, 60);
			x++;
		}
		if(nunchuk && x < max_controls)
		{
			TexHandle.fromPNG(m_controls[x], gi_nunchuk_png);
			m_btnMgr.setTexture(m_gameinfoLblControls[x] ,m_controls[x], 52, 60);
			x++;
		}
		if(guitar && x < max_controls)
		{
			TexHandle.fromPNG(m_controls[x], gi_guitar_png);
			m_btnMgr.setTexture(m_gameinfoLblControls[x] ,m_controls[x], 52, 60);
			x++;
		}
		if(drums && x < max_controls)
		{
			TexHandle.fromPNG(m_controls[x], gi_drums_png);
			m_btnMgr.setTexture(m_gameinfoLblControls[x] ,m_controls[x], 52, 60);
			x++;
		}
		if(dancepad && x < max_controls)
		{
			TexHandle.fromPNG(m_controls[x], gi_dancepad_png);
			m_btnMgr.setTexture(m_gameinfoLblControls[x] ,m_controls[x], 52, 60);
			x++;
		}
		if(motionplus && x < max_controls)
		{
			TexHandle.fromPNG(m_controls[x], gi_motionplus_png);
			m_btnMgr.setTexture(m_gameinfoLblControls[x] ,m_controls[x], 52, 60);
			x++;
		}
		if(balanceboard && x < max_controls)
		{
			TexHandle.fromPNG(m_controls[x], gi_balanceboard_png);
			m_btnMgr.setTexture(m_gameinfoLblControls[x] ,m_controls[x], 52, 60);
			x++;
		}
		if(microphone && x < max_controls)
		{
			TexHandle.fromPNG(m_controls[x], gi_microphone_png);
			m_btnMgr.setTexture(m_gameinfoLblControls[x] ,m_controls[x], 48, 60);
			x++;
		}
		if(gamecube && x < max_controls)
		{
			TexHandle.fromPNG(m_controls[x], gi_gcncontroller_png);
			m_btnMgr.setTexture(m_gameinfoLblControls[x] ,m_controls[x], 48, 60);
			x++;
		}
		if(keyboard && x < max_controls)
		{
			TexHandle.fromPNG(m_controls[x], gi_keyboard_png);
			m_btnMgr.setTexture(m_gameinfoLblControls[x] ,m_controls[x], 52, 60);
			x++;
		}
		if(udraw && x < max_controls)
		{
			TexHandle.fromPNG(m_controls[x], gi_udraw_png);
			m_btnMgr.setTexture(m_gameinfoLblControls[x] ,m_controls[x], 52, 60);
			x++;
		}
		if(zapper && x < max_controls)
		{
			TexHandle.fromPNG(m_controls[x], gi_zapper_png);
			m_btnMgr.setTexture(m_gameinfoLblControls[x] ,m_controls[x], 52, 70);
			x++;
		}
		if(wheel && x < max_controls)
		{
			TexHandle.fromPNG(m_controls[x], gi_wheel_png);
			m_btnMgr.setTexture(m_gameinfoLblControls[x] ,m_controls[x], 52, 60);
			x++;
		}
		cnt_controls = x;
	}
	
/***************************************** Shared game info *********************************************/
	
	/* Synopsis */
	if(gametdb.GetSynopsis(GameID, TMP_Char))
	{
		gameinfo_Synopsis_w.fromUTF8(TMP_Char);
		noSynopsis = gameinfo_Synopsis_w.empty() ? true : false;
	}
	else
	{
		gameinfo_Synopsis_w.fromUTF8("");
		noSynopsis = true;
	}
	m_btnMgr.setText(m_gameinfoLblSynopsis, gameinfo_Synopsis_w);

	/* Create Rom / ISO Info */
	rom_info.append(wfmt(_fmt("gameinfo7",L"GameID: %s"), GameID));
		
	if(gametdb.GetRegion(GameID, TMP_Char))
	{
		rom_info += ' ';
		rom_info.append(wfmt(_fmt("gameinfo3",L"(%s)"), TMP_Char));
	}

	if(gametdb.GetGenres(GameID, TMP_Char, GameHdr->type))
	{
		rom_info.append(L"\n\n");
		vector<string> genres = stringToVector(TMP_Char, ',');
		string s;
		for(u8 i = 0; i < genres.size(); ++i)
		{
			if(i > 0)
				s.append(", "); // add comma & space between genres
			s.append(genres[i]);
		}
		rom_info.append(wfmt(_fmt("gameinfo5",L"Genre: %s"), s.c_str()));
	}
	PublishDate = gametdb.GetPublishDate(GameID);
	year = PublishDate >> 16;
	day = PublishDate & 0xFF;
	month = (PublishDate >> 8) & 0xFF;
	if(day == 0 && month == 0)
	{
		//! only display year or nothing if there's no date at all
		if(year != 0)
		{
			rom_info.append(L"\n\n");
			rom_info.append(wfmt(_fmt("gameinfo8",L"Released: %i"), year));
		}
	}
	else
	{
		switch(wiiRegion)
		{
			case 1: // US
				rom_info.append(L"\n\n");
				rom_info.append(wfmt(_fmt("gameinfo4",L"Release Date: %i-%i-%i"), month, day, year));
				break;
			case 2: // EUR
				rom_info.append(L"\n\n");
				rom_info.append(wfmt(_fmt("gameinfo4",L"Release Date: %i/%i/%i"), day, month, year));
				break;
			default:
				rom_info.append(L"\n\n");
				rom_info.append(wfmt(_fmt("gameinfo4",L"Release Date: %i-%i-%i"), year, month, day));
				break;
		}
	}
	
	players = gametdb.GetPlayers(GameID);
	if(players > 1)
	{
		rom_info.append(L"\n\n");
		rom_info.append(wfmt(_fmt("gameinfo9",L"Players: %i"), players));
	}
	
	if(gametdb.GetLanguages(GameID, TMP_Char)) // added
	{
		rom_info.append(L"\n\n");
		rom_info.append(wfmt(_fmt("gameinfo10",L"Languages: %s"), TMP_Char));
	}
	if(gametdb.GetDeveloper(GameID, TMP_Char))
	{
		rom_info.append(L"\n\n");
		rom_info.append(wfmt(_fmt("gameinfo1",L"Developer: %s"), TMP_Char));
	}
	if(gametdb.GetPublisher(GameID, TMP_Char))
	{
		rom_info.append(L"\n\n");
		rom_info.append(wfmt(_fmt("gameinfo2",L"Publisher: %s"), TMP_Char));
	}

	gametdb.CloseFile();
	
out:
	if(!tdb_found)
	{
		if(GameHdr->type == TYPE_PLUGIN)
			rom_info.append(_t("errgame18", L"No game info!"));
		else
			rom_info.append(_t("errtdb", L"Download GameTDB to use this feature."));
		m_btnMgr.setText(m_gameinfoLblRomInfo, rom_info);
	}
	m_btnMgr.setText(m_gameinfoLblRomInfo, rom_info);
}
