
#include "menu.hpp"
#include "gc/gcdisc.hpp"
#include "gui/WiiMovie.hpp"
// #include "banner/BannerWindow.hpp"

#define FA_BG_DELAY 100 // number of loops before fanart background update

/* Sounds */
extern const u8 gc_ogg[];
extern const u32 gc_ogg_size;

bool m_banner_loaded = false;
bool m_defaultSndPlayed = false;

s16 m_gameBtnPlayFull;
s16 m_gameBtnBackFull;
s16 m_gameBtnToggle;
s16 m_gameBtnToggleFull;
s16 m_gameLblSnapBg;
// s16 m_gameLblSnapFrame; // a different frame for rom snapshots
s16 m_gameLblBannerFrame;
s16 m_gameBtnInfo;

s16 m_gameBtnCategories;
s16 m_gameBtnFavoriteOff;
s16 m_gameBtnFavoriteOn;
s16 m_gameBtnCheatOff;
s16 m_gameBtnCheatOn;	
s16 m_gameBtnSettingsOff;
s16 m_gameBtnSettingsOn;
s16 m_gameBtnVideo;
s16 m_gameBtnLockOff;
s16 m_gameBtnLockOn;
s16 m_gameBtnPlay;
s16 m_gameBtnBack;
s16 m_gameLblUser[4];

s16 m_gameLblGuide;
s16 m_gameGuideLblUser[4];

s16 m_gameLblInfo[6];

void CMenu::_extractBnr(const dir_discHdr *hdr)
{
	u32 size = 0;
	DeviceHandle.OpenWBFS(currentPartition);
	wbfs_disc_t *disc = WBFS_OpenDisc((u8 *) &hdr->id, (char *) hdr->path);
	if(disc != NULL)
	{
		void *bnr = NULL;
		size = wbfs_extract_file(disc, (char*)"opening.bnr", &bnr);
		if(size > 0)
			CurrentBanner.SetBanner((u8*)bnr, size, false, true);
		WBFS_CloseDisc(disc);
	}
	WBFS_Close();
}

void CMenu::_setCurrentItem(const dir_discHdr *hdr)
{
	const char *title = CoverFlow.getFilenameId(hdr);
	if(m_current_view == COVERFLOW_PLUGIN)
	{
		if(hdr->type == TYPE_PLUGIN)
			strncpy(m_plugin.PluginMagicWord, fmt("%08x", hdr->settings[0]), 8);
		else
		{
			if(hdr->type == TYPE_WII_GAME)
				strncpy(m_plugin.PluginMagicWord, "4E574949", 9);
			else if(hdr->type == TYPE_GC_GAME)
				strncpy(m_plugin.PluginMagicWord, "4E47434D", 9);
			else if(hdr->type == TYPE_CHANNEL)
				strncpy(m_plugin.PluginMagicWord, "4E414E44", 9);
			else if(hdr->type == TYPE_EMUCHANNEL)
				strncpy(m_plugin.PluginMagicWord, "454E414E", 9);
			else // HOMEBREW
				strncpy(m_plugin.PluginMagicWord, "48425257", 9);
		}
		m_cfg.setString(plugin_domain, "cur_magic", m_plugin.PluginMagicWord);
		m_cfg.setString("PLUGIN_ITEM", m_plugin.PluginMagicWord, title);
	}
	else
		m_cfg.setString(_domainFromView(), "current_item", title);
}

void CMenu::_hideGame(bool instant)
{
	_cleanupVideo();
	m_fa.unload();

	m_btnMgr.hide(m_gameBtnPlay, instant);
	m_btnMgr.hide(m_gameBtnBack, instant);
	m_btnMgr.hide(m_gameBtnPlayFull, instant);
	m_btnMgr.hide(m_gameBtnBackFull, instant);
	m_btnMgr.hide(m_gameBtnCheatOff, instant);
	m_btnMgr.hide(m_gameBtnCheatOn, instant);
	m_btnMgr.hide(m_gameBtnSettingsOff, instant);
	m_btnMgr.hide(m_gameBtnSettingsOn, instant);
	m_btnMgr.hide(m_gameBtnToggle, instant);
	m_btnMgr.hide(m_gameBtnToggleFull, instant);
	m_btnMgr.hide(m_gameBtnFavoriteOn, instant);
	m_btnMgr.hide(m_gameBtnFavoriteOff, instant);
	m_btnMgr.hide(m_gameBtnCategories, instant);
	m_btnMgr.hide(m_gameBtnVideo, instant);
	m_btnMgr.hide(m_gameBtnLockOn, instant);
	m_btnMgr.hide(m_gameBtnLockOff, instant);
	m_btnMgr.hide(m_gameBtnInfo, instant);
	m_btnMgr.hide(m_gameLblSnapBg, instant);
	m_btnMgr.hide(m_gameLblSnap, instant);
	m_btnMgr.hide(m_gameLblOverlay, instant);
	// m_btnMgr.hide(m_gameLblSnapFrame, instant);
	m_btnMgr.hide(m_gameLblBannerFrame, instant);
	for(u8 i = 0; i < 6; ++i)
		m_btnMgr.hide(m_gameLblInfo[i]);
	for(u8 i = 0; i < ARRAY_SIZE(m_gameLblUser); ++i)
		if(m_gameLblUser[i] != -1)
			m_btnMgr.hide(m_gameLblUser[i], instant);
}

void CMenu::_cleanupBanner(bool gamechange)
{
	//! banner
	m_gameSound.FreeMemory();
	_stopGameSoundThread(); // stop banner and gamesound loading
	m_banner.DeleteBanner(gamechange);
	//! movie
	_cleanupVideo();
}

void CMenu::_cleanupVideo()
{
	m_video_playing = false;
	movie.DeInit();
}

static const char *getVideoPath(const string &videoDir, const char *videoId)
{
	const char *videoPath = NULL;

	if(CoverFlow.getHdr()->type == TYPE_PLUGIN)
		videoPath = fmt("%s/%s/%s", videoDir.c_str(), m_plugin.GetCoverFolderName(CoverFlow.getHdr()->settings[0]), videoId);
	else
		videoPath = fmt("%s/%s", videoDir.c_str(), videoId);

	return videoPath;
}

static const char *getVideoDefaultPath(const string &videoDir)
{
	const char *videoPath = fmt("%s/%s", videoDir.c_str(), m_plugin.PluginMagicWord);
	return videoPath;
}

bool CMenu::_startVideo(void)
{
	const dir_discHdr *GameHdr = CoverFlow.getHdr();
	const char *videoId = NULL;
	char curId3[4];
	memset(curId3, 0, 4);

	if(!NoGameID(GameHdr->type))
	{	//! id3
		memcpy(curId3, GameHdr->id, 3);
		videoId = curId3;
	}
	else
		//! title.ext
		videoId = CoverFlow.getFilenameId(GameHdr);	
		
	//! dev:/wiiflow/trailers/{coverfolder}/title.ext.thp or dev:/wiiflow/trailers/id3.thp
	const char *videoPath = getVideoPath(m_videoDir, videoId);
	const char *THP_Path = fmt("%s.thp", videoPath);
	
	if(!fsop_FileExist(THP_Path))
	{
		if(GameHdr->type == TYPE_PLUGIN)
		{
			//! dev:/wiiflow/trailers/magic#.thp
			videoPath = getVideoDefaultPath(m_videoDir);
			THP_Path = fmt("%s.thp", videoPath);
		}
		else if(!NoGameID(GameHdr->type))
		{
			//! id6
			videoPath = getVideoPath(m_videoDir, GameHdr->id);
			THP_Path = fmt("%s.thp", videoPath);
		}
	}
	if(fsop_FileExist(THP_Path))
	{
		m_gameSound.FreeMemory();
		_stopGameSoundThread();
		m_banner.SetShowBanner(false);
		//! Let's play the movie
		movie.Init(THP_Path);
		m_gameSound.Load(fmt("%s.ogg", videoPath));
		u8 prevBnrSound = m_bnrSndVol;
		m_bnrSndVol = m_bnrSndVol == 0 ? 255 : m_bnrSndVol;
		m_gameSound.SetVolume(m_bnrSndVol);
		m_bnrSndVol = prevBnrSound;
		m_video_playing = true;
		m_gameSound.Play();
		movie.Play(true); // video loops sound doesn't
		return true;
	}
	return false;
}

void CMenu::_showGame(bool fanart, bool anim) // get fanart animation
{
	if(fanart)
	{
		const dir_discHdr *GameHdr = CoverFlow.getHdr();
		const char *FanartPath = NULL;

		if(GameHdr->type == TYPE_PLUGIN)
			FanartPath = fmt("%s/%s", m_fanartDir.c_str(), m_plugin.GetCoverFolderName(GameHdr->settings[0]));
		else
			FanartPath = fmt("%s", m_fanartDir.c_str());

		if(m_fa.load(FanartPath, GameHdr, !anim))
		{
			const TexData *bg = NULL;
			const TexData *bglq = NULL;
			m_fa.getBackground(bg, bglq, !anim);
			_setBg(*bg, *bglq, !anim); // true = force change to remove previous fanart background
			if(anim)
				CoverFlow.hideCover();
		}
		else
			_setMainBg();
	}
	else
		_setMainBg();
}

void CMenu::_game(bool launch)
{
	bool coverFlipped = false;
	bool m_zoom_banner = false;
	bool menuBar = false;
	bool newLabels = true;
	bool adult_on = false;
	bool cheat_on = false;
	bool mc_on = false;
	bool fav_on = false;
	bool change = false;
	bool fanart = m_cfg.getBool(general_domain, "enable_fanart", false);
	s16 fanartAnimDelay = m_cfg.getInt(general_domain, "fanart_delay", 8) * 60;
	s16 startFanartAnim = fanart ? -fanartAnimDelay : 1;
	s16 startFanartBg = fanart ? -FA_BG_DELAY : 1;
	s8 startGameSound = -7;
	int cf_version = 1;
	string domain;
	string key;
	Vector3D v;
	Vector3D savedv;
	
	dir_discHdr *hdr = (dir_discHdr*)MEM2_alloc(sizeof(dir_discHdr));
	memcpy(hdr, CoverFlow.getHdr(), sizeof(dir_discHdr));
	_setCurrentItem(hdr); // sets PluginMagicWord
	
	char id[74];
	memset(id, 0, 74);
	
	if(isWiiVC && (hdr->type == TYPE_WII_GAME || hdr->type == TYPE_EMUCHANNEL))
		return;

	if(hdr->type == TYPE_HOMEBREW)
		wcstombs(id, hdr->title, 63);
	else if(hdr->type == TYPE_PLUGIN)
	{
		string romFileName(hdr->path);
		romFileName = romFileName.substr(romFileName.find_last_of("/") + 1);
		romFileName = romFileName.substr(0, romFileName.find_last_of("."));
		strcpy(id, fmt("%08x/%.63s", hdr->settings[0], romFileName.c_str()));
	}
	else
		strcpy(id, hdr->id);
		
	m_banner_loaded = false;
	
	m_zoom_banner = m_cfg.getBool(_domainFromView(), "show_full_banner", false);
	if(m_banner.GetZoomSetting() != m_zoom_banner)
		m_banner.ToggleZoom();
	if(m_banner.GetInGameSettings())
		m_banner.ToggleGameSettings();
	
	m_gameSelected = true;
	m_defaultSndPlayed = false;
	
	SetupInput();

	while(!m_exit)
	{
		if(!launch)
		{
			/* Replay fanart animation or hide it if no loop */
			if(m_fa.isLoaded() && m_fa.isAnimationComplete())
			{
				if(m_fa.noLoop())
				{
					m_fa.unload();
					CoverFlow.showCover();
				}
				else // loop fanart
					m_fa.reset();
			}

			_mainLoopCommon(true);

			/* Game sound and banner timer */
			if(startGameSound < 0)
				startGameSound++;
			else if(startGameSound == 0)
			{
				startGameSound = 1; // stops timer
				_playGameSound();
			}
			/* Fanart background timer */
			if(startFanartBg < 0) // always false if fanart disabled
				startFanartBg++;
			else if(startFanartBg == 0)
			{
				startFanartBg = 1;
				_showGame(fanart, false);
			}
			/* Fanart animation timer */
			if(startFanartAnim < 0) // always false if fanart disabled
				startFanartAnim++;
			else if(startFanartAnim == 0 && !m_zoom_banner && !m_video_playing && !m_fa.isLoaded())
			{
				startFanartAnim = 1;
				_showGame(fanart, true); // true starts fanart animation with unloading previous fanart
			}
		}
		/* Move and zoom flipped cover */
		if(coverFlipped)
		{
			float step = 0.05f;
			if(BTN_PLUS_PRESSED || BTN_MINUS_PRESSED)
			{
				if(BTN_MINUS_PRESSED)
					step = -step;
				v.z = min(max(-15.f, v.z + step), 15.f);
			}
			else if(BTN_LEFT_PRESSED || BTN_RIGHT_PRESSED)
			{
				if(BTN_RIGHT_PRESSED)
					step = -step;
				v.x = min(max(-15.f, v.x + step), 15.f);
			}
			else if(BTN_UP_PRESSED || BTN_DOWN_PRESSED)
			{
				if(BTN_UP_PRESSED)
					step = -step;
				v.y = min(max(-15.f, v.y + step), 15.f);
			}
			CoverFlow.setCoverFlipPos(v);
		}
		if(BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED)
		{
			startFanartAnim = fanart ? -fanartAnimDelay : 1; // reset fanart animation timer

			/* Close fanart animation */
			if(m_fa.isLoaded())
			{
				m_fa.unload();
				CoverFlow.showCover();
			}
			/* Stop or unzoom trailer */
			else if(m_video_playing)
			{
				if(m_zoom_video)
					m_zoom_video = false;
				else
				{
					m_video_playing = false;
					movie.DeInit();
					m_gameSound.FreeMemory();
					m_banner.SetShowBanner(true);
					if(!m_gameSound.IsPlaying())
						startGameSound = -4; // not -6
				}
			}
			/* Unzoom banner */
			else if(m_zoom_banner)
			{
				m_zoom_banner = m_banner.ToggleZoom();
				m_cfg.setBool(_domainFromView(), "show_full_banner", m_zoom_banner);
			}
			/* De flip cover */
			else if(coverFlipped)
			{
				CoverFlow.flip();
				coverFlipped = false;
			}
			/* Exit game menu */
			else
			{
				_cleanupBanner(); // also cleans up trailer movie including trailer sound
				break;
			}			
		}
		/* Menu bar navigation */
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
		{
			startFanartAnim = fanart ? -fanartAnimDelay : 1;
			if(menuBar && !coverFlipped)
				m_btnMgr.up();
		}
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
		{
			startFanartAnim = fanart ? -fanartAnimDelay : 1;
			if(menuBar && !coverFlipped)
				m_btnMgr.down();
		}
		/* Menu bar activation */
		else if(BTN_MINUS_PRESSED && !m_video_playing && !coverFlipped)
		{
			startFanartAnim = fanart ? -fanartAnimDelay : 1;
			if(!m_fa.isLoaded())
			{
				menuBar = !menuBar;
				if(!menuBar) // disable menu bar
				{
					_hideGame();
					_showGame(fanart, false);
				}
				else
					m_btnMgr.setSelected(m_gameBtnCategories);
			}
		}
		/* Flip cover */
		else if(BTN_PLUS_PRESSED && !m_video_playing)
		{
			startFanartAnim = fanart ? -fanartAnimDelay : 1;
			if(!coverFlipped && !m_zoom_banner && !m_fa.isLoaded())
			{
				cf_version = _getCFVersion();
				domain = fmt("%s_%i_S", cf_domain, cf_version);
				key = "flip_pos";
				if(!m_vid.wide())
					key += "_4_3";
				v = m_coverflow.getVector3D(domain, key);
				coverFlipped = true;
				CoverFlow.flip();
			}
		}
		else if(launch || BTN_A_OR_2_PRESSED)
		{			
			startFanartAnim = fanart ? -fanartAnimDelay : 1;
					
			/* De flip cover before entering any other menu */
			if(coverFlipped)
			{
				CoverFlow.flip();
				coverFlipped = false;
			}
			/* Close fanart animation */
			if(m_fa.isLoaded())
			{
				m_fa.unload();
				CoverFlow.showCover();
				continue;
			}
			/* Unzoom video */
			if(m_video_playing && m_zoom_video)
				m_zoom_video = false;
			
			/* Play button */
			if(launch || m_btnMgr.selected(m_gameBtnPlay) || m_btnMgr.selected(m_gameBtnPlayFull) || (!ShowPointer() && !m_video_playing && !menuBar))
			{
				_hideGame();
				_cleanupBanner();
				_launch(hdr);
				break;
			}				
			/* Exit game or quit trailer */
			else if(m_btnMgr.selected(m_gameBtnBack))
			{
				if(m_video_playing)
				{
					m_video_playing = false;
					movie.DeInit();
					m_gameSound.FreeMemory();
					m_banner.SetShowBanner(true);
					if(!m_gameSound.IsPlaying())
						startGameSound = -4; // not -6
				}
				else
				{
					_cleanupBanner();
					break;
				}
			}
			/* Categories settings button */
			else if(m_btnMgr.selected(m_gameBtnCategories))
			{
				if(m_locked)
				{
					m_banner.SetShowBanner(false);
					error(_t("errgame15", L"Unlock parental control to use this feature!"));
					m_banner.SetShowBanner(true);
				}
				else
				{
					_hideGame();
					//! the mainloop handles drawing banner while in settings
					m_banner.ToggleZoom(); // zoom to full
					m_banner.ToggleGameSettings(); // dim brightness
					_CategorySettings(true); // fromgameset is true
					m_banner.ToggleGameSettings(); // reset brightness
					m_banner.ToggleZoom(); // de zoom to small
				}
				if(m_newGame) // if categories settings moved coverflow right or left
					startGameSound = -10;
				else
					_showGame(fanart, false);
			}
			/* Favorites settings button */
			else if(m_btnMgr.selected(m_gameBtnFavoriteOn) || m_btnMgr.selected(m_gameBtnFavoriteOff))
			{
				change = true;
				m_gcfg1.setBool(fmt("FAVORITES%s", hdr->type == TYPE_PLUGIN ? "_PLUGINS" : ""), id, !m_gcfg1.getBool(fmt("FAVORITES%s", hdr->type == TYPE_PLUGIN ? "_PLUGINS" : ""), id, false));
				if(m_favorites)
					m_refreshGameList = true;
				newLabels = true;
			}
			/* Adult only lock */
			else if((m_btnMgr.selected(m_gameBtnLockOff) || m_btnMgr.selected(m_gameBtnLockOn)) && !m_locked)
			{
				change = true;
				m_gcfg1.setBool(fmt("ADULTONLY%s", hdr->type == TYPE_PLUGIN ? "_PLUGINS" : ""), id, !m_gcfg1.getBool(fmt("ADULTONLY%s", hdr->type == TYPE_PLUGIN ? "_PLUGINS" : ""), id, false));
				newLabels = true;
			}
			/* Game info */
			else if(m_btnMgr.selected(m_gameBtnInfo))
			{
				_hideGame(); // stops trailer movie and unloads fanart
				m_banner.ToggleZoom(); // zoom to full
				m_banner.ToggleGameSettings(); // dim brightness
				launch = _gameinfo();
				m_banner.ToggleGameSettings(); // dim brightness
				m_banner.ToggleZoom(); // zoom to full
				if(m_newGame) // if game info moved coverflow right or left
					startGameSound = -10;
				else
					_showGame(fanart, false);
			}
			/* Video trailer */
			else if(m_btnMgr.selected(m_gameBtnVideo))
			{
				_startVideo();
			}
			/* Controller input guide (plugin) / Cheat codes (wii + channels + gamecube) */
			else if(m_btnMgr.selected(m_gameBtnCheatOff) || m_btnMgr.selected(m_gameBtnCheatOn))
			{
				if(hdr->type == TYPE_PLUGIN) // controller input guide
				{
					string guideBG = m_plugin.GetGuideName(hdr->settings[0]);
					if(TexHandle.fromImageFile(m_game_guide, fmt("%s/inputs/%s", m_pluginsDir.c_str(), guideBG.c_str())) != TE_OK)
					{
						TexHandle.Cleanup(m_game_guide);
						continue;
					}
					else
						m_btnMgr.setTexture(m_gameLblGuide, m_game_guide);
					_hideGame();
					_setBg(m_configBg, m_configBg);
					m_banner.ToggleZoom(); // zoom to full
					m_banner.ToggleGameSettings(); // dim brightness
					for(u8 i = 0; i < ARRAY_SIZE(m_gameGuideLblUser); ++i)
						m_btnMgr.show(m_gameGuideLblUser[i]);
					m_btnMgr.show(m_gameLblGuide);
					m_btnMgr.show(m_configBtnBack);
					string gTitle = m_plugin.GetGuideTitle(hdr->settings[0]); 
					m_btnMgr.setText(m_configLblTitle, gTitle);
					m_btnMgr.show(m_configLblTitle);
					while(!m_exit)
					{
						_mainLoopCommon();
						if(BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnBack)))
							break;
					}
					for(u8 i = 0; i < ARRAY_SIZE(m_gameGuideLblUser); ++i)
						m_btnMgr.hide(m_gameGuideLblUser[i]);
					m_btnMgr.hide(m_gameLblGuide);
					m_btnMgr.hide(m_configBtnBack);
					m_btnMgr.hide(m_configLblTitle);
					m_banner.ToggleZoom(); // zoom to full
					m_banner.ToggleGameSettings(); // dim brightness
					_showGame(fanart, false);
				}
				else if(hdr->type != TYPE_HOMEBREW) // cheat codes
				{
					_hideGame();
					//! the mainloop handles drawing banner while in settings
					m_banner.ToggleZoom(); // zoom to full
					m_banner.ToggleGameSettings(); // dim brightness
					_CheatSettings(id);
					newLabels = true;
					m_banner.ToggleGameSettings(); // reset brightness
					m_banner.ToggleZoom(); // de zoom to small
					_showGame(fanart, false);
				}
			}
			/* Settings buttons */
			else if(m_btnMgr.selected(m_gameBtnSettingsOff) || m_btnMgr.selected(m_gameBtnSettingsOn))
			{
				_hideGame();
				if(m_locked)
				{
					m_banner.SetShowBanner(false);
					error(_t("errgame15", L"Unlock parental control to use this feature!"));
					m_banner.SetShowBanner(true);
				}
				else
				{
					//! the mainloop handles drawing banner while in settings
					m_banner.ToggleZoom(); // zoom to full
					m_banner.ToggleGameSettings(); // dim brightness
					_gameSettings(hdr, false); // disc is false
					m_banner.ToggleGameSettings(); // reset brightness
					m_banner.ToggleZoom(); // de zoom to small					
				}
				newLabels = true;
				if(m_newGame) // (actually game deleted) exit menu game to avoid a crash if no game left in coverflow
				{
					m_newGame = false;
					break;					
				}
				_showGame(fanart, false);
			}
			/* Video full screen */
			else if(m_btnMgr.selected(m_gameBtnToggle) && m_video_playing && !m_zoom_video)
			{
				m_zoom_video = true;
			}
			/* Zoom and unzoom banner */
			else if(m_btnMgr.selected(m_gameBtnToggle) || m_btnMgr.selected(m_gameBtnBackFull))
			{
				m_zoom_banner = m_banner.ToggleZoom();
				m_cfg.setBool(_domainFromView(), "show_full_banner", m_zoom_banner);
			}
			/* Flip cover */
			else if(!coverFlipped && !m_zoom_banner && !m_video_playing)
			{
				for(int chan = WPAD_MAX_WIIMOTES-1; chan >= 0; chan--)
				{
					if(CoverFlow.mouseOver(m_cursor[chan].x(), m_cursor[chan].y()))
					{
						cf_version = _getCFVersion();
						domain = fmt("%s_%i_S", cf_domain, cf_version);
						key = "flip_pos";
						if(!m_vid.wide())
							key += "_4_3";
						v = m_coverflow.getVector3D(domain, key);
						coverFlipped = true;
						CoverFlow.flip();
					}
				}
			}
		}
/********************************************************************************************************/
		/* Skip to next game in coverflow */
		if(!m_video_playing && !m_fa.isLoaded())
		{
			if(!menuBar && !coverFlipped)
			{
				if((startGameSound == 1 || startGameSound < -8) && (BTN_UP_REPEAT || RIGHT_STICK_UP))
				{
					CoverFlow.up();
					startGameSound = -10;
				}
				if((startGameSound == 1 || startGameSound < -8) && (BTN_RIGHT_REPEAT || RIGHT_STICK_RIGHT || BTN_RIGHT_PRESSED)) // BTN_RIGHT_PRESSED for ds3
				{
					CoverFlow.right();
					startGameSound = -10;
				}
				if((startGameSound == 1 || startGameSound < -8) && (BTN_DOWN_REPEAT || RIGHT_STICK_DOWN))
				{
					CoverFlow.down();
					startGameSound = -10;
				}
				if((startGameSound == 1 || startGameSound < -8) && (BTN_LEFT_REPEAT || RIGHT_STICK_LEFT || BTN_LEFT_PRESSED)) // BTN_LEFT_PRESSED for ds3
				{
					CoverFlow.left();
					startGameSound = -10;
				}
			}

			if(startGameSound == -10) // if -10 then we moved to new cover
			{
				_setMainBg(); // remove previous fanart background immediately
				coverFlipped = false; // coverFlipped may still be (wrongly) true if coverflow moved right or left
				newLabels = true; // reset icon status
				startFanartAnim = fanart ? -fanartAnimDelay : 1; // reset fanart timer
				startFanartBg = fanart ? -FA_BG_DELAY : 1;
				change = true;
		
				memcpy(hdr, CoverFlow.getHdr(), sizeof(dir_discHdr)); // get new game header
				_setCurrentItem(hdr); // sets new PluginMagicWord
				
				memset(id, 0, 74);

				if(hdr->type == TYPE_HOMEBREW)
					wcstombs(id, hdr->title, 63);
				else if(hdr->type == TYPE_PLUGIN)
				{
					string romFileName(hdr->path);
					romFileName = romFileName.substr(romFileName.find_last_of("/") + 1);
					romFileName = romFileName.substr(0, romFileName.find_last_of("."));
					strcpy(id, fmt("%08x/%.63s", hdr->settings[0], romFileName.c_str()));
				}
				else
					strcpy(id, hdr->id);
				
				if(m_newGame)
				{
					m_newGame = false;
					startGameSound = 1;
					_playGameSound();
				}
			}
		}
/********************************************************************************************************/
		/* Modify lock, cheat, GC memcard (settings), and favorite icons based on gcfg1 and gcfg2 */
		if(newLabels)
		{
			newLabels = false;
			//! reset all labels
			adult_on = false;
			cheat_on = false;
			mc_on = false;
			fav_on = false;
			//! adult only?
			adult_on = m_gcfg1.getBool(fmt("ADULTONLY%s", hdr->type == TYPE_PLUGIN ? "_PLUGINS" : ""), id, false);
			if(hdr->type != TYPE_PLUGIN && hdr->type != TYPE_HOMEBREW)
			{
				//! cheats/controls icon info
				m_btnMgr.setText(m_gameLblInfo[4], _t("infogame5a", L"Cheats"));
				//! cheat codes?
				cheat_on = m_gcfg2.getBool(id, "cheat", false);
				//! real GC memcard needed?
				if(hdr->type == TYPE_GC_GAME)
				{
					u8 emuMC = m_gcfg2.getUInt(id, "emu_memcard", 0);
					emuMC = (emuMC == 0) ? m_cfg.getUInt(gc_domain, "emu_memcard", 1) : emuMC - 1;
					mc_on = emuMC == 0;
				}
			}
			else
				//! cheats/controls icon info
				m_btnMgr.setText(m_gameLblInfo[4], _t("infogame5b", L"Controls"));
			//! favorite?
			fav_on = m_gcfg1.getBool(fmt("FAVORITES%s", hdr->type == TYPE_PLUGIN ? "_PLUGINS" : ""), id, false);
		}
/********************************************************************************************************/
		/* Showing and hiding buttons based on banner zoomed state */
		if(!m_fa.isLoaded() && !m_video_playing)
		{
			if(!m_zoom_banner || (m_zoom_banner && !m_banner_loaded && !m_soundThrdBusy))
			{
				/* always hide full banner buttons */
				m_btnMgr.hide(m_gameBtnPlayFull);
				m_btnMgr.hide(m_gameBtnBackFull);
				m_btnMgr.hide(m_gameBtnToggleFull);
				
				/* Show icons and buttons */
				for(u8 i = 0; i < ARRAY_SIZE(m_gameLblUser); ++i)
					if(m_gameLblUser[i] != -1)
						m_btnMgr.show(m_gameLblUser[i]);
					
				m_btnMgr.show(m_gameBtnPlay);
				m_btnMgr.show(m_gameBtnBack);
				m_btnMgr.show(m_gameBtnInfo);
				
				m_btnMgr.show(m_gameBtnCategories, change);
				m_btnMgr.show(fav_on ? m_gameBtnFavoriteOn : m_gameBtnFavoriteOff, change);
				m_btnMgr.hide(fav_on ? m_gameBtnFavoriteOff : m_gameBtnFavoriteOn, change);
				m_btnMgr.show(adult_on ? m_gameBtnLockOn : m_gameBtnLockOff, change);
				m_btnMgr.hide(adult_on ? m_gameBtnLockOff : m_gameBtnLockOn, change);
				m_btnMgr.show(cheat_on ? m_gameBtnCheatOn : m_gameBtnCheatOff, change);
				m_btnMgr.hide(cheat_on ? m_gameBtnCheatOff : m_gameBtnCheatOn, change);
				m_btnMgr.show(m_gameBtnVideo, change);
				m_btnMgr.show(mc_on ? m_gameBtnSettingsOn : m_gameBtnSettingsOff, change);
				m_btnMgr.hide(mc_on ? m_gameBtnSettingsOff : m_gameBtnSettingsOn, change);
				change = false;
				
				for(u8 i = 0; i < 6; ++i)
				{
					if(m_show_zone_gameinfo[i])
						m_btnMgr.show(m_gameLblInfo[i]);
					else
						m_btnMgr.hide(m_gameLblInfo[i]);
				}
				
				/* Show or hide small banner toggle btn and frame */
				if(m_banner_loaded && !m_soundThrdBusy)
				{
					//! show only if the game has a banner
					m_btnMgr.show(m_gameBtnToggle);
					m_btnMgr.show(m_gameLblBannerFrame);
				}
				else if(m_snapshot_loaded && !m_soundThrdBusy)
				{
					m_btnMgr.show(m_gameLblSnapBg);
					m_btnMgr.show(m_gameLblSnap);
					m_btnMgr.show(m_gameLblOverlay);
					// m_btnMgr.show(m_gameLblSnapFrame);
					m_btnMgr.show(m_gameLblBannerFrame); // to be removed if m_gameLblSnapFrame is used instead
				}
				else
				{
					m_btnMgr.hide(m_gameBtnToggle);
					m_btnMgr.hide(m_gameLblSnapBg);
					m_btnMgr.hide(m_gameLblSnap);
					m_btnMgr.hide(m_gameLblOverlay);
					// m_btnMgr.hide(m_gameLblSnapFrame);
					m_btnMgr.hide(m_gameLblBannerFrame);
				}

			}
			else if(m_banner_loaded && !m_soundThrdBusy) // banner zoomed
			{
				m_btnMgr.show(m_gameBtnPlayFull);
				m_btnMgr.show(m_gameBtnBackFull);
				m_btnMgr.show(m_gameBtnToggleFull);
				
				m_btnMgr.hide(m_gameBtnFavoriteOn);
				m_btnMgr.hide(m_gameBtnFavoriteOff);
				m_btnMgr.hide(m_gameBtnCategories);
				m_btnMgr.hide(m_gameBtnSettingsOff);
				m_btnMgr.hide(m_gameBtnSettingsOn);
				m_btnMgr.hide(m_gameBtnCheatOff);
				m_btnMgr.hide(m_gameBtnCheatOn);
				m_btnMgr.hide(m_gameBtnVideo);
				m_btnMgr.hide(m_gameBtnLockOn);
				m_btnMgr.hide(m_gameBtnLockOff);
				m_btnMgr.hide(m_gameBtnPlay);
				m_btnMgr.hide(m_gameBtnBack);
				m_btnMgr.hide(m_gameBtnInfo);
				m_btnMgr.hide(m_gameLblBannerFrame);
				m_btnMgr.hide(m_gameBtnToggle);
				m_btnMgr.hide(m_gameLblSnapBg);
				for(u8 i = 0; i < 6; ++i)
					m_btnMgr.hide(m_gameLblInfo[i]);
				for(u8 i = 0; i < ARRAY_SIZE(m_gameLblUser); ++i)
					if(m_gameLblUser[i] != -1)
						m_btnMgr.hide(m_gameLblUser[i]);
			}
		}
		/* Fanart or trailer is loaded, hide all buttons */
		else
		{
			m_btnMgr.hide(m_gameBtnFavoriteOn);
			m_btnMgr.hide(m_gameBtnFavoriteOff);
			m_btnMgr.hide(m_gameBtnCategories);
			m_btnMgr.hide(m_gameBtnSettingsOff);
			m_btnMgr.hide(m_gameBtnSettingsOn);
			m_btnMgr.hide(m_gameBtnCheatOff);
			m_btnMgr.hide(m_gameBtnCheatOn);
			m_btnMgr.hide(m_gameBtnVideo);
			m_btnMgr.hide(m_gameBtnLockOn);
			m_btnMgr.hide(m_gameBtnLockOff);
			m_btnMgr.hide(m_gameBtnPlay);
			m_btnMgr.hide(m_gameBtnInfo);
			m_btnMgr.hide(m_gameLblSnapBg);
			m_btnMgr.hide(m_gameLblSnap);
			m_btnMgr.hide(m_gameLblOverlay);
			// m_btnMgr.hide(m_gameLblSnapFrame);
			if(m_video_playing && !m_zoom_video)
			{
				m_btnMgr.show(m_gameBtnToggle);
				m_btnMgr.show(m_gameLblBannerFrame);
				m_btnMgr.show(m_gameBtnBack);
			}
			else
			{
				m_btnMgr.hide(m_gameBtnToggle);
				m_btnMgr.hide(m_gameLblBannerFrame);
				m_btnMgr.hide(m_gameBtnBack);
			}
			for(u8 i = 0; i < 6; ++i)
				m_btnMgr.hide(m_gameLblInfo[i]);
			for(u8 i = 0; i < ARRAY_SIZE(m_gameLblUser); ++i)
				if (m_gameLblUser[i] != -1)
					m_btnMgr.hide(m_gameLblUser[i]);
		}
	}
	if(coverFlipped) // should not happen
	{
		m_coverflow.setVector3D(domain, key, savedv);
		_loadCFLayout(cf_version, true); // true?
		CoverFlow.applySettings(true);
	}
	m_snapshot_loaded = false;
	TexData emptyTex;
	m_btnMgr.setTexture(m_gameLblSnap, emptyTex);
	m_btnMgr.setTexture(m_gameLblOverlay, emptyTex);
	TexHandle.Cleanup(m_game_snap);
	TexHandle.Cleanup(m_game_overlay);
	m_btnMgr.setTexture(m_gameLblGuide, emptyTex);
	TexHandle.Cleanup(m_game_guide);
	m_gameSelected = false;
	MEM2_free(hdr);
	_hideGame(); // stops trailer movie and unloads fanart
}

extern const u8 icon_categ_png[];
extern const u8 icon_categ_s_png[];

extern const u8 icon_fav_png[];
extern const u8 icon_fav_s_png[];
extern const u8 icon_fav_on_png[];

extern const u8 icon_cheat_png[];
extern const u8 icon_cheat_s_png[];
extern const u8 icon_cheat_on_png[];

extern const u8 icon_config_png[];
extern const u8 icon_config_s_png[];
extern const u8 icon_config_on_png[];

extern const u8 icon_lock_png[];
extern const u8 icon_lock_s_png[];
extern const u8 icon_lock_on_png[];

extern const u8 icon_video_png[];
extern const u8 icon_video_s_png[];

extern const u8 tex_blank_png[];

void CMenu::_initGameMenu()
{
	TexHandle.fromPNG(texGameCateg, icon_categ_png);
	TexHandle.fromPNG(texGameCategS, icon_categ_s_png);
	
	TexHandle.fromPNG(texGameFav, icon_fav_png);
	TexHandle.fromPNG(texGameFavS, icon_fav_s_png);
	TexHandle.fromPNG(texGameFavOn, icon_fav_on_png);
	TexHandle.fromPNG(texGameFavOnS, icon_fav_s_png);
	
	TexHandle.fromPNG(texCheat, icon_cheat_png);
	TexHandle.fromPNG(texCheatS, icon_cheat_s_png);
	TexHandle.fromPNG(texCheatOn, icon_cheat_on_png);
	TexHandle.fromPNG(texCheatOnS, icon_cheat_s_png);
	
	TexHandle.fromPNG(texGameConfig, icon_config_png);
	TexHandle.fromPNG(texGameConfigS, icon_config_s_png);
	TexHandle.fromPNG(texGameConfigOn, icon_config_on_png);
	TexHandle.fromPNG(texGameConfigOnS, icon_config_s_png);
	
	TexHandle.fromPNG(texLock, icon_lock_png);
	TexHandle.fromPNG(texLockS, icon_lock_s_png);
	TexHandle.fromPNG(texLockOn, icon_lock_on_png);
	TexHandle.fromPNG(texLockOnS, icon_lock_s_png);
	
	TexHandle.fromPNG(texVideo, icon_video_png);
	TexHandle.fromPNG(texVideoS, icon_video_s_png);
	
	TexHandle.fromPNG(texToggleBanner, tex_blank_png);
	TexHandle.fromPNG(texSnapShotBg, tex_blank_png);
	TexHandle.fromPNG(texBannerFrame, tex_blank_png);
	
	_addUserLabels(m_gameLblUser, ARRAY_SIZE(m_gameLblUser), "GAME");

	m_gameBtnCategories = _addPicButton("GAME/CATEGORIES_BTN", texGameCateg, texGameCategS, 395, 215, 32, 32);
	m_gameBtnFavoriteOff = _addPicButton("GAME/FAVORITE_OFF", texGameFav, texGameFavS, 435, 215, 32, 32);
	m_gameBtnFavoriteOn = _addPicButton("GAME/FAVORITE_ON", texGameFavOn, texGameFavOnS, 435, 215, 32, 32);
	m_gameBtnLockOff = _addPicButton("GAME/LOCK_OFF", texLock, texLockS, 475, 215, 32, 32);
	m_gameBtnLockOn = _addPicButton("GAME/LOCK_ON", texLockOn, texLockOnS, 475, 215, 32, 32);
	m_gameBtnVideo = _addPicButton("GAME/VIDEO_BTN", texVideo, texVideoS, 515, 215, 32, 32);
	m_gameBtnCheatOff = _addPicButton("GAME/CHEAT_OFF", texCheat, texCheatS, 555, 215, 32, 32);
	m_gameBtnCheatOn = _addPicButton("GAME/CHEAT_ON", texCheatOn, texCheatOnS, 555, 215, 32, 32);
	m_gameBtnSettingsOff = _addPicButton("GAME/SETTINGS_OFF", texGameConfig, texGameConfigS, 595, 215, 32, 32);
	m_gameBtnSettingsOn = _addPicButton("GAME/SETTINGS_ON", texGameConfigOn, texGameConfigOnS, 595, 215, 32, 32);
	
	m_gameBtnPlay = _addButton("GAME/PLAY_BTN", theme.btnFont, L"", 415, 240+30, 200, 48, theme.btnFontColor);
	m_gameBtnInfo = _addButton("GAME/INFO_BTN", theme.btnFont, L"", 415, 296+30, 200, 48, theme.btnFontColor);
	m_gameBtnBack = _addButton("GAME/BACK_BTN", theme.btnFont, L"", 415, 352+30, 200, 48, theme.btnFontColor);
		
	m_gameBtnPlayFull = _addButton("GAME/PLAY_FULL_BTN", theme.btnFont, L"", 340, 390, 200, 56, theme.btnFontColor);
	m_gameBtnBackFull = _addButton("GAME/BACK_FULL_BTN", theme.btnFont, L"", 100, 390, 200, 56, theme.btnFontColor);
	
	m_gameBtnToggle = _addPicButton("GAME/TOGGLE_BTN", texToggleBanner, texToggleBanner, 385, 31, 236, 127);
	m_gameBtnToggleFull = _addPicButton("GAME/TOGGLE_FULL_BTN", texToggleBanner, texToggleBanner, 20, 12, 608, 344);
	m_gameLblSnapBg = _addLabel("GAME/SNAP_BG", theme.txtFont, L"", 390, 31, 246, 127, theme.txtFontColor, 0, texSnapShotBg);
	
	m_gameLblSnap = _addLabel("GAME/SNAP", theme.txtFont, L"", 385, 31, 100, 100, theme.txtFontColor, 0, m_game_snap);
	m_gameLblOverlay = _addLabel("GAME/OVERLAY", theme.txtFont, L"", 385, 31, 100, 100, theme.txtFontColor, 0, m_game_overlay);
	//! 8 pixel width frames
	// m_gameLblSnapFrame = _addLabel("GAME/SNAP_FRAME", theme.txtFont, L"", 377, 23, 262, 186, theme.txtFontColor, 0, texSnapShotFrame);
	m_gameLblBannerFrame = _addLabel("GAME/BANNER_FRAME", theme.txtFont, L"", 377, 23, 262, 151, theme.txtFontColor, 0, texBannerFrame);

	_setHideAnim(m_gameBtnCategories, "GAME/CATEGORIES_BTN", 0, 0, 1.f, -1.f);
	_setHideAnim(m_gameBtnFavoriteOff, "GAME/FAVORITE_OFF", 0, 0, 1.f, -1.f);
	_setHideAnim(m_gameBtnFavoriteOn, "GAME/FAVORITE_ON", 0, 0, 1.f, -1.f);
	_setHideAnim(m_gameBtnLockOff, "GAME/LOCK_OFF", 0, 0, 1.f, -1.f);
	_setHideAnim(m_gameBtnLockOn, "GAME/LOCK_ON", 0, 0, 1.f, -1.f);
	_setHideAnim(m_gameBtnVideo, "GAME/VIDEO_BTN", 0, 0, 1.f, -1.f);
	_setHideAnim(m_gameBtnCheatOff, "GAME/CHEAT_OFF", 0, 0, 1.f, -1.f);
	_setHideAnim(m_gameBtnCheatOn, "GAME/CHEAT_ON", 0, 0, 1.f, -1.f);
	_setHideAnim(m_gameBtnSettingsOff, "GAME/SETTINGS_OFF", 0, 0, 1.f, -1.f);
	_setHideAnim(m_gameBtnSettingsOn, "GAME/SETTINGS_ON", 0, 0, 1.f, -1.f);
	
	TexData emptyTex;
	for(u8 i = 0; i < 6; ++i)
	{
		char *infoText = fmt_malloc("GAME/INFO%i", i);
		if(infoText == NULL) 
			continue;
		m_gameLblInfo[i] = _addLabel(infoText, theme.lblFont, L"", 415, 180, 200, 20, theme.lblFontColor, FTGX_JUSTIFY_CENTER | FTGX_ALIGN_MIDDLE, emptyTex);
		
		m_gameInfo[i].x = m_theme.getInt("GAME/ZONES", fmt("info%i_x", i), 395 + (i * 40));
		m_gameInfo[i].y = m_theme.getInt("GAME/ZONES", fmt("info%i_y", i), 215);
		m_gameInfo[i].w = m_theme.getInt("GAME/ZONES", fmt("info%i_w", i), 32);
		m_gameInfo[i].h = m_theme.getInt("GAME/ZONES", fmt("info%i_h", i), 32);
		m_gameInfo[i].hide = m_theme.getBool("GAME/ZONES", fmt("info%i_hide", i), true);
		
		_setHideAnim(m_gameLblInfo[i], infoText, 0, 0, 0.f, 0.f);
		MEM2_free(infoText);
	}
	_textInfoGame();
	
	_setHideAnim(m_gameBtnPlay, "GAME/PLAY_BTN", 0, 0, 1.f, -1.f);
	_setHideAnim(m_gameBtnInfo, "GAME/INFO_BTN", 0, 0, 1.f, -1.f);
	_setHideAnim(m_gameBtnBack, "GAME/BACK_BTN", 0, 0, 1.f, -1.f);
	
	_setHideAnim(m_gameBtnPlayFull, "GAME/PLAY_FULL_BTN", 0, 0, 1.f, 0.f);
	_setHideAnim(m_gameBtnBackFull, "GAME/BACK_FULL_BTN", 0, 0, 1.f, 0.f);

	_setHideAnim(m_gameBtnToggle, "GAME/TOGGLE_BTN", 200, 0, 1.f, 0.f);
	_setHideAnim(m_gameBtnToggleFull, "GAME/TOGGLE_FULL_BTN", 200, 0, 1.f, 0.f);
	_setHideAnim(m_gameLblSnapBg, "GAME/SNAP_BG", 0, 0, 1.f, 1.f);
	_setHideAnim(m_gameLblSnap, "GAME/SNAP", 0, 0, 1.f, 1.f);
	_setHideAnim(m_gameLblOverlay, "GAME/OVERLAY", 0, 0, 1.f, 1.f);
	// _setHideAnim(m_gameLblSnapFrame, "GAME/SNAP_FRAME", 0, 0, 1.f, 1.f);
	_setHideAnim(m_gameLblBannerFrame, "GAME/BANNER_FRAME", 0, 0, 1.f, 1.f);

	/* Plugin game controller input guide */
	_addUserLabels(m_gameGuideLblUser, ARRAY_SIZE(m_gameGuideLblUser), "GAMEGUIDE");
	m_gameLblGuide = _addLabel("GAMEGUIDE/GUIDE", theme.txtFont, L"", 0, 0, 640, 480, theme.txtFontColor, 0, m_game_guide);
	_setHideAnim(m_gameLblGuide, "GAMEGUIDE/GUIDE", 1, 1, 0, 0);
	
	// _hideGame(true);
	_textGame();
	
	snapbg_x = m_theme.getInt("GAME/SNAP_BG", "x", 385);
	snapbg_y = m_theme.getInt("GAME/SNAP_BG", "y", 31);
	snapbg_w = m_theme.getInt("GAME/SNAP_BG", "width", 246);
	snapbg_h = m_theme.getInt("GAME/SNAP_BG", "height", 127);
}

void CMenu::_textInfoGame(void)
{
	m_btnMgr.setText(m_gameLblInfo[0], _t("infogame1", L"Set categories"));
	m_btnMgr.setText(m_gameLblInfo[1], _t("infogame2", L"Set as favorite"));
	m_btnMgr.setText(m_gameLblInfo[2], _t("infogame3", L"Set adult only"));
	m_btnMgr.setText(m_gameLblInfo[3], _t("infogame4", L"Trailer"));

	m_btnMgr.setText(m_gameLblInfo[5], _t("infogame6", L"Config"));
}

void CMenu::_textGame(void)
{
	m_btnMgr.setText(m_gameBtnPlay, _t("gm1", L"Play"));
	m_btnMgr.setText(m_gameBtnBack, _t("gm2", L"Back"));
	m_btnMgr.setText(m_gameBtnInfo, _t("gm99", L"Game info"));
	m_btnMgr.setText(m_gameBtnPlayFull, _t("gm1", L"Play"));
	m_btnMgr.setText(m_gameBtnBackFull, _t("gm2", L"Back"));
}

/*******************************************************************************************************/

struct IMD5Header
{
	u32 fcc;
	u32 filesize;
	u8 zeroes[8];
	u8 crypto[16];
} ATTRIBUTE_PACKED;

/* Loads game banner and sound to be played by mainloop */
void * CMenu::_gameSoundThread(void *obj)
{
	CMenu *m = (CMenu*)obj;
	m->m_soundThrdBusy = true;
	m->m_gamesound_changed = false;
	m->m_snapshot_loaded = false;
	m_banner_loaded = false;
	CurrentBanner.ClearBanner(); // clear current banner from memory
	
	/* Set to empty textures to clear current snapshot from screen */
	TexData emptyTex;
	m_btnMgr.setTexture(m->m_gameLblSnap, emptyTex);
	m_btnMgr.setTexture(m->m_gameLblOverlay, emptyTex);
	
	u8 *custom_bnr_file = NULL;
	u32 custom_bnr_size = 0;
	char custom_banner[256];
	custom_banner[255] = '\0';

	u8 *cached_bnr_file = NULL;
	u32 cached_bnr_size = 0;
	char cached_banner[256];
	cached_banner[255] = '\0';

	const dir_discHdr *GameHdr = CoverFlow.getHdr();	
	if(GameHdr->type == TYPE_PLUGIN)
	{
		/* Plugin individual game sound */
		char game_sound[256];
		game_sound[255] = '\0';
		const char *coverDir  = NULL;
		coverDir = m_plugin.GetCoverFolderName(GameHdr->settings[0]);
		
		strncpy(custom_banner, fmt("%s/%s/%s.bnr", m->m_customBnrDir.c_str(), coverDir, CoverFlow.getFilenameId(GameHdr)), 255);
		strncpy(game_sound, fmt("%s/gamesounds/%s/%s", m->m_dataDir.c_str(), coverDir, CoverFlow.getFilenameId(GameHdr)), 251);
		
		/* Get plugin rom custom banner */
		fsop_GetFileSizeBytes(custom_banner, &custom_bnr_size);
		if(custom_bnr_size > 0)
		{
			custom_bnr_file = (u8*)MEM2_lo_alloc(custom_bnr_size);
			if(custom_bnr_file != NULL)
			{
				fsop_ReadFileLoc(custom_banner, custom_bnr_size, (void*)custom_bnr_file);
				m_banner_loaded = true;
			}
		}
		/* If no banner try getting snapshot or plugin gamesound */
		else if(custom_bnr_size == 0 || custom_bnr_file == NULL)
		{
			m_banner.DeleteBanner();
			/* Try to get snapshot */
			if(m->m_platform.loaded())
			{
				char GameID[7];
				char platformName[16];
				const char *TMP_Char = NULL;
				GameTDB gametdb;
				
				strncpy(m_plugin.PluginMagicWord, fmt("%08x", GameHdr->settings[0]), 8);
				snprintf(platformName, sizeof(platformName), "%s", m->m_platform.getString("PLUGINS", m_plugin.PluginMagicWord, "").c_str());
				strcpy(GameID, GameHdr->id); // GameHdr->id is null terminated

				if(strlen(platformName) != 0 && strcasecmp(GameID, "PLUGIN") != 0)
				{	
					string newName = m->m_platform.getString("COMBINED", platformName);
					if(newName.empty())
						m->m_platform.remove("COMBINED", platformName);
					else
						snprintf(platformName, sizeof(platformName), "%s", newName.c_str());

					/* Load platform name.xml database to get game's info using the gameID */
					gametdb.OpenFile(fmt("%s/%s/%s.xml", m->m_pluginDataDir.c_str(), platformName, platformName));
					if(gametdb.IsLoaded())
					{
						gametdb.SetLanguageCode(m->m_loc.getString(m->m_curLanguage, "gametdb_code", "EN").c_str());

						/* Get roms's title without the extra ()'s or []'s */
						string ShortName;
						if(strrchr(GameHdr->path, '/') != NULL)
							ShortName = m_plugin.GetRomName(GameHdr->path);
						else
						{
							char title[64];
							title[63] = '\0';
							wcstombs(title, GameHdr->title, sizeof(title) - 1);
							ShortName = title;
						}

						const char *snap_path = NULL;
						if(strcasestr(platformName, "ARCADE") || strcasestr(platformName, "CPS") || !strncasecmp(platformName, "NEOGEO", 6))
							snap_path = fmt("%s/%s/%s.png", m->m_snapDir.c_str(), platformName, ShortName.c_str());
						else if(gametdb.GetName(GameID, TMP_Char))
							snap_path = fmt("%s/%s/%s.png", m->m_snapDir.c_str(), platformName, TMP_Char);

						gametdb.CloseFile();
						if(snap_path == NULL || !fsop_FileExist(snap_path))
							snap_path = fmt("%s/%s/%s.png", m->m_snapDir.c_str(), platformName, GameID);

						if(fsop_FileExist(snap_path))
						{
							m->m_snapshot_loaded = true;
							TexHandle.fromImageFile(m->m_game_snap, snap_path);
							/* Get snapshot position and dimensions to center it on the snap background */
							int snap_w = m->m_game_snap.width;
							int snap_h = m->m_game_snap.height;
							int width_over = snap_w - m->snapbg_w;
							int height_over = snap_h - m->snapbg_h;
							float aspect_ratio = (float)snap_w / (float)snap_h;
							if(width_over > 0 || height_over > 0)
							{
								if(width_over > height_over)
								{
									snap_w = m->snapbg_w;
									snap_h = (int)((float)snap_w / aspect_ratio);
								}
								else
								{
									snap_h = m->snapbg_h;
									snap_w = (int)((float)snap_h * aspect_ratio);
								}
							}

							int x_pos = (m->snapbg_w - snap_w) / 2 + m->snapbg_x;
							int y_pos = (m->snapbg_h - snap_h) / 2 + m->snapbg_y;
							m_btnMgr.setTexture(m->m_gameLblSnap, m->m_game_snap, x_pos, y_pos, snap_w, snap_h);

							/* Get possible overlay */
							const char *overlay_path = fmt("%s/%s_overlay.png", m->m_snapDir.c_str(), platformName);
							if(fsop_FileExist(overlay_path))
							{
								TexHandle.fromImageFile(m->m_game_overlay, overlay_path);
								m_btnMgr.setTexture(m->m_gameLblOverlay, m->m_game_overlay, x_pos, y_pos, snap_w, snap_h);
							}
							else
								TexHandle.Cleanup(m->m_game_overlay);
						}
					}
				}
				if(!m->m_snapshot_loaded)
				{
					TexHandle.Cleanup(m->m_game_snap);
					TexHandle.Cleanup(m->m_game_overlay);
				}
			}

			/* Try to get plugin rom gamesound or just the default plugin gamesound */
			bool found = false;
			if(fsop_FileExist(fmt("%s.mp3", game_sound)))
			{
				strcat(game_sound, ".mp3");
				found = true;
			}
			else if(fsop_FileExist(fmt("%s.wav", game_sound)))
			{
				strcat(game_sound, ".wav");
				found = true;
			}
			else if(fsop_FileExist(fmt("%s.ogg", game_sound)))
			{
				strcat(game_sound, ".ogg");
				found = true;
			}
			if(found)
			{
				u32 size = 0;
				u8 *sf = fsop_ReadFile(game_sound, &size);
				m->m_gameSound.Load(sf, size);
			}
			else if(!m_defaultSndPlayed)
			{
				m->m_gameSound.Load(m_plugin.GetBannerSound(GameHdr->settings[0]), m_plugin.GetBannerSoundSize());
				m_defaultSndPlayed = true;
			}
			if(m->m_gameSound.IsLoaded())
				m->m_gamesound_changed = true;

			/* No custom banner so we are done, exit sound thread */
			m->m_soundThrdBusy = false;
			return NULL;
		}
	}
	else // Not plugin type
	{
		/* Try to get custom banner for wii, gc, and channels
		 Check custom ID6 first */
		strncpy(custom_banner, fmt("%s/%s.bnr", m->m_customBnrDir.c_str(), GameHdr->id), 255);
		fsop_GetFileSizeBytes(custom_banner, &custom_bnr_size);
		if(custom_bnr_size > 0)
		{
			custom_bnr_file = (u8*)MEM2_lo_alloc(custom_bnr_size);
			if(custom_bnr_file != NULL)
			{
				fsop_ReadFileLoc(custom_banner, custom_bnr_size, (void*)custom_bnr_file);
				m_banner_loaded = true;
			}
		}
		/* No custom ID6 or too big, try ID3 */
		else 
		{
			strncpy(custom_banner, fmt("%s/%.3s.bnr", m->m_customBnrDir.c_str(), GameHdr->id), 255);
			fsop_GetFileSizeBytes(custom_banner, &custom_bnr_size);
			if(custom_bnr_size > 0)
			{
				custom_bnr_file = (u8*)MEM2_lo_alloc(custom_bnr_size);
				if(custom_bnr_file != NULL)
				{
					fsop_ReadFileLoc(custom_banner, custom_bnr_size, (void*)custom_bnr_file);
					m_banner_loaded = true;
				}
			}
		}
	}
	if(GameHdr->type == TYPE_GC_GAME && custom_bnr_file == NULL)
	{
		/* GC game but no custom banner, so we make one ourselves, and exit sound thread
		Get the GC game's opening.bnr from ISO - a 96x32 image to add to the GC banner included with wiiflow */
		GC_Disc_Reader.init(GameHdr->path);
		u8 *opening_bnr = GC_Disc_Reader.GetGameCubeBanner();
		if(opening_bnr != NULL)
		{
			//! creategcbanner adds the opening.bnr image and game title to the wiiflow GC banner
			m_banner.CreateGCBanner(opening_bnr, m->m_wbf1_font, m->m_wbf2_font, GameHdr->title);
			m_banner_loaded = true;
		}
		else
			m_banner.DeleteBanner();
		GC_Disc_Reader.clear();
		if(!m_defaultSndPlayed)
		{
			//! get wiiflow GC ogg sound to play with banner
			m->m_gameSound.Load(gc_ogg, gc_ogg_size, false);
			m_defaultSndPlayed = true;
			if(m->m_gameSound.IsLoaded())
				m->m_gamesound_changed = true;
		}
		m->m_soundThrdBusy = false;
		return NULL;
	}
	/* No custom banner load and if wii or channel game try cached banner id6 only */
	if(custom_bnr_file == NULL)
	{
		strncpy(cached_banner, fmt("%s/%s.bnr", m->m_bnrCacheDir.c_str(), GameHdr->id), 255);
		fsop_GetFileSizeBytes(cached_banner, &cached_bnr_size);
		if(cached_bnr_size > 0)
		{
			cached_bnr_file = (u8*)MEM2_lo_alloc(cached_bnr_size);
			if(cached_bnr_file != NULL)
			{
				fsop_ReadFileLoc(cached_banner, cached_bnr_size, (void*)cached_bnr_file);
				m_banner_loaded = true;
			}
		}
	}

	if(custom_bnr_file != NULL)
		CurrentBanner.SetBanner(custom_bnr_file, custom_bnr_size, true, true);
	else if(cached_bnr_file != NULL)
		CurrentBanner.SetBanner(cached_bnr_file, cached_bnr_size, false, true);
	else if(GameHdr->type == TYPE_WII_GAME)
	{
		m->_extractBnr(GameHdr);
		m_banner_loaded = true;
	}
	else if(GameHdr->type == TYPE_CHANNEL || GameHdr->type == TYPE_EMUCHANNEL)
	{
		ChannelHandle.GetBanner(TITLE_ID(GameHdr->settings[0], GameHdr->settings[1]));
		m_banner_loaded = true;
	}
		
	if(!CurrentBanner.IsValid())
	{
		m_banner_loaded = false;
		m->m_gameSound.FreeMemory();
		m_banner.DeleteBanner();
		CurrentBanner.ClearBanner();
		m->m_soundThrdBusy = false;
		return NULL;
	}
	/* Save new wii or channel banner to cache folder, gc and custom banners are not cached */
	if(cached_bnr_file == NULL && custom_bnr_file == NULL)
		fsop_WriteFile(cached_banner, CurrentBanner.GetBannerFile(), CurrentBanner.GetBannerFileSize());

	/* Load and init banner */
	m_banner.LoadBanner(m->m_wbf1_font, m->m_wbf2_font);
	
	/* Get sound from wii, channel, or custom banner and load it to play with the banner */
	u32 sndSize = 0;
	u8 *soundBin = CurrentBanner.GetFile("sound.bin", &sndSize);
	CurrentBanner.ClearBanner(); // got sound.bin and banner for displaying is loaded so no longer need current banner.

	if(soundBin != NULL && (GameHdr->type != TYPE_GC_GAME || m->m_gc_play_banner_sound))
	{
		if(memcmp(&((IMD5Header *)soundBin)->fcc, "IMD5", 4) == 0)
		{
			u32 newSize = 0;
			u8 *newSound = DecompressCopy(soundBin, sndSize, &newSize);
			free(soundBin); // no longer needed, now using decompressed newSound
			if(newSound == NULL || newSize == 0 || !m->m_gameSound.Load(newSound, newSize))
			{
				m->m_gameSound.FreeMemory(); // frees newSound
				m_banner.DeleteBanner(); // the same as UnloadBanner
				m->m_soundThrdBusy = false;
				return NULL;
			}
		}
		else
			m->m_gameSound.Load(soundBin, sndSize);

		if(m->m_gameSound.IsLoaded())
			m->m_gamesound_changed = true;
		else
		{
			m->m_gameSound.FreeMemory();
			m_banner.DeleteBanner();
		}
	}
	else
	{
		if(soundBin != NULL)
			free(soundBin);
		// gprintf("WARNING: No sound found in banner!\n");
		m->m_gamesound_changed = true;
		m->m_gameSound.FreeMemory();
	}
	m->m_soundThrdBusy = false;
	return NULL;
}

u8 *GameSoundStack = NULL;
u32 GameSoundSize = 0x10000; // 64kb
void CMenu::_playGameSound(void) // starts banner and gamesound loading thread
{
	_cleanupBanner(true);
	m_gamesound_changed = false;
	if(m_bnrSndVol == 0)
		return;

	if(m_gameSoundThread != LWP_THREAD_NULL)
		_stopGameSoundThread();
	GameSoundStack = (u8*)MEM2_lo_alloc(GameSoundSize);
	LWP_CreateThread(&m_gameSoundThread, _gameSoundThread, this, GameSoundStack, GameSoundSize, 60);
}

void CMenu::_stopGameSoundThread() // stops banner and gamesound loading thread
{
	if(m_gameSoundThread == LWP_THREAD_NULL)
		return;

	if(LWP_ThreadIsSuspended(m_gameSoundThread))
		LWP_ResumeThread(m_gameSoundThread);

	while(m_soundThrdBusy)
		usleep(500);

	LWP_JoinThread(m_gameSoundThread, NULL);
	m_gameSoundThread = LWP_THREAD_NULL;

	if(GameSoundStack)
		MEM2_lo_free(GameSoundStack);
	GameSoundStack = NULL;
}
