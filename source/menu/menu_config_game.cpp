
#include "menu.hpp"
#include "loader/nk.h"
// #include "types.h"
// #include "loader/wbfs.h"
// #include "libwbfs/wiidisc.h"

#define ARRAY_SIZE(a)	(sizeof a / sizeof a[0])

static u8 curPage;
int videoScale, videoOffset;
const dir_discHdr *GameHdr;

const CMenu::SOption CMenu::_VideoModes[7] = {
	{ "viddef", L"Default" },
	{ "vidgame", L"Game" },
	{ "vidsys", L"Console" },
	{ "vidp50", L"PAL 50Hz" },
	{ "vidp60", L"PAL 60Hz" },
	{ "vidntsc", L"NTSC" },
	{ "vidprog", L"Progressive" },
};

const CMenu::SOption CMenu::_GCvideoModes[7] = {
	{ "viddef", L"Default" },
	{ "vidgame", L"Game" },
	{ "vidp50", L"PAL 50Hz" },
	{ "vidp60", L"PAL 60Hz" },
	{ "vidntsc", L"NTSC" },
	{ "DMLmpal", L"MPAL" },
	{ "vidprog", L"Progressive" },
};

const CMenu::SOption CMenu::_languages[12] = {
	{ "lngdef", L"Default" },
	{ "lngsys", L"Console" }, // added
	{ "lngjap", L"Japanese" },
	{ "lngeng", L"English" },
	{ "lngger", L"German" },
	{ "lngfre", L"French" },
	{ "lngspa", L"Spanish" },
	{ "lngita", L"Italian" },
	{ "lngdut", L"Dutch" },
	{ "lngsch", L"S. Chinese" },
	{ "lngtch", L"T. Chinese" },
	{ "lngkor", L"Korean" }
};

const CMenu::SOption CMenu::_GClanguages[8] = {
	{ "lngdef", L"Default" },
	{ "lngsys", L"Console" },
	{ "lngeng", L"English" },
	{ "lngger", L"German" },
	{ "lngfre", L"French" },
	{ "lngspa", L"Spanish" },
	{ "lngita", L"Italian" },
	{ "lngdut", L"Dutch" }
};

const CMenu::SOption CMenu::_ChannelsType[3] = {
	{ "ChanReal", L"Nand" },
	{ "ChanEmu", L"Emunand" },
	{ "ChanBoth", L"Both nands" },
};

const CMenu::SOption CMenu::_NandEmu[4] = {
	{ "NANDDef", L"Default" },
	{ "NANDpart", L"Partial" },
	{ "NANDfull", L"Full" },
	{ "neek2o", L"Neek2o" },
};

const CMenu::SOption CMenu::_SaveEmu[5] = {
	{ "SaveDef", L"Default" },
	{ "SaveOff", L"Off" },
	{ "SavePart", L"Saves/DLC" },
	{ "SaveFull", L"Full" },
	{ "neek2o", L"Neek2o" },
};

const CMenu::SOption CMenu::_AspectRatio[3] = {
	{ "aspectDef", L"Default" },
	{ "aspect43", L"4:3 to 16:9" },
	{ "aspect169", L"16:9 to 4:3" },
};

const CMenu::SOption CMenu::_NinEmuCard[6] = {
	{ "NinMCDef", L"Default" },
	{ "NinMCOff", L"Disabled" },
	{ "NinMCon", L"256 blocks" },
	{ "NinMCBig", L"1019 blocks" }, // added
	{ "NinMCMulti", L"Multi saves" },
	{ "NinMCdebug", L"Debug" },
};

const CMenu::SOption CMenu::_vidModePatch[4] = {
	{ "vmpnone", L"None" },
	{ "vmpnormal", L"Normal" }, // only patch video modes that have the same resolution
	{ "vmpmore", L"Same scan" }, // patch every mode if its scan (interlaced or progressive) is the same
	{ "vmpall", L"All" } // patch every mode
};

const CMenu::SOption CMenu::_hooktype[8] = {
	{ "hook_auto", L"AUTO" },
	{ "hooktype1", L"VBI" },
	{ "hooktype2", L"KPAD read" },
	{ "hooktype3", L"Joypad" },
	{ "hooktype4", L"GXDraw" },
	{ "hooktype5", L"GXFlush" },
	{ "hooktype6", L"OSSleepThread" },
	{ "hooktype7", L"AXNextFrame" },
};

const CMenu::SOption CMenu::_debugger[3] = {
	{ "disabled", L"Disabled" },
	{ "dbg_gecko", L"Gecko" },
	{ "dbgfwrite", L"OSReport" },
};

const CMenu::SOption CMenu::_privateServer[3] = {
	{ "off", L"Off" },
	{ "ps_nossl", L"No SSL only" },
	{ "ps_wiimmfi", L"Wiimmfi" },
};

const CMenu::SOption CMenu::_DeflickerOptions[7] = {
	{ "df_def", L"Default" },
	{ "df_norm", L"Normal" },
	{ "df_off", L"Off (Safe)" },
	{ "df_ext", L"Off (Extended)" },
	{ "df_low", L"On (Low)" },
	{ "df_med", L"On (Medium)" },
	{ "df_high", L"On (High)" },
};

template <class T> static inline T loopNum(T i, T s)
{
	return (i + s) % s;
}

wstringEx CMenu::_optBoolToString(int i)
{
	switch(i)
	{
		case 0:
			return _t("off", L"Off");
		case 1:
			return _t("on", L"On");
		default:
			return _t("def", L"Default");
	}
}

void CMenu::_showGameSettings(bool instant, bool dvd)
{
	vector<string> custom_servers = stringToVector(m_cfg.getString("custom_servers", "servers"), '|');
	u32 i;	

	for(i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);
	
	_hideCheckboxes(true); // reset checkboxes

	if(!dvd)
		m_btnMgr.setText(m_configLblTitle, CoverFlow.getTitle());
	else
		m_btnMgr.setText(m_configLblTitle, GameHdr->title);
	m_btnMgr.show(m_configLblTitle);

	if(dvd && curPage == MAIN_SETTINGS)
	{
		m_btnMgr.setText(m_configBtnCenter, _t("cfgne6", L"Start"));
		m_btnMgr.show(m_configBtnCenter);
	}
	else
		m_btnMgr.show(m_configBtnBack);
	
	if(curPage == MAIN_SETTINGS)
	{
		if(!dvd)
		{
			bool d = (GameHdr->type == TYPE_PLUGIN || GameHdr->type == TYPE_HOMEBREW);
			//! DELETE GAME
			m_btnMgr.setText(m_configLbl[1+3*d], _t("wbfsop2", L"Delete game"));
			m_btnMgr.show(m_configLbl[1+3*d], instant);
			m_btnMgr.show(m_configBtnGo[1+3*d], instant);
			
			if(d)
				return;
			
			//! COVER AND BANNER
			m_btnMgr.setText(m_configLbl[2], _t("cfgg40", L"Manage cover and banner"));
			m_btnMgr.show(m_configLbl[2], instant);
			m_btnMgr.show(m_configBtnGo[2], instant);
		}
		//! CHEAT CODES
		m_btnMgr.setText(m_configLbl[3-2*dvd], _t("cfgg92", L"Cheat codes"));
		m_btnMgr.show(m_configLbl[3-2*dvd], instant);
		m_btnMgr.show(m_configBtnGo[3-2*dvd], instant);
		
		//! VIDEO SETTINGS
		m_btnMgr.setText(m_configLbl[4-2*dvd], _t("cfgg97", L"Game video settings"));
		m_btnMgr.show(m_configLbl[4-2*dvd], instant);
		m_btnMgr.show(m_configBtnGo[4-2*dvd], instant);
		
		//! NETWORK SETTINGS
		m_btnMgr.setText(m_configLbl[5-2*dvd], _t("cfgg98", L"Network settings"));
		m_btnMgr.show(m_configLbl[5-2*dvd], instant);
		m_btnMgr.show(m_configBtnGo[5-2*dvd], instant);
		
		//! position game language button line correctly
		bool a = neek2o();
		bool b = (GameHdr->type == TYPE_CHANNEL && isWiiVC);
		bool c = (GameHdr->type == TYPE_CHANNEL || (dvd && GameHdr->type != TYPE_GC_GAME));
		u8 j = 8 - (2 * dvd) - (2 * a) - b - c;
		//! PERIPHERAL EMULATION (GC) / COMPATIBILITY
		if(!a)
		{
			if(GameHdr->type == TYPE_GC_GAME)
			{
				m_btnMgr.setText(m_configLbl[6-2*dvd], _t("cfgg91", L"Emulation settings")); // peripheral
				m_btnMgr.show(m_configLbl[6-2*dvd], instant);
				m_btnMgr.show(m_configBtnGo[6-2*dvd], instant);
			}
			else if(!b)
			{
				m_btnMgr.setText(m_configLbl[6-2*dvd], _t("cfgg93", L"Compatibility")); // compatibility
				m_btnMgr.show(m_configLbl[6-2*dvd], instant);
				m_btnMgr.show(m_configBtnGo[6-2*dvd], instant);
			}
		}
		
		//! MEMCARD (GC) / NAND EMULATION
		if(GameHdr->type == TYPE_GC_GAME)
		{
			m_btnMgr.setText(m_configLbl[7-2*dvd], _t("cfgg47", L"Manage virtual MemCard")); // memcard
			m_btnMgr.show(m_configLbl[7-2*dvd], instant);
			m_btnMgr.show(m_configBtnGo[7-2*dvd], instant);
		}
		else if(GameHdr->type != TYPE_CHANNEL && !dvd && !a)
		{
			m_btnMgr.setText(m_configLbl[7-2*dvd], _t("cfgg24", L"Nand emulation")); // nand emulation
			m_btnMgr.show(m_configLbl[7-2*dvd], instant);
			m_btnMgr.show(m_configBtnGo[7-2*dvd], instant);
		}
		
		//! GAME LANGUAGE -> default global
		m_btnMgr.setText(m_configLbl[j], _t("cfgg3", L"Game language"));
		if(GameHdr->type == TYPE_GC_GAME)
		{
			i = min(m_gcfg2.getUInt(GameHdr->id, "language", 0), ARRAY_SIZE(CMenu::_GClanguages) - 1u);
			m_btnMgr.setText(m_configLblVal[j], _t(CMenu::_GClanguages[i].id, CMenu::_GClanguages[i].text), true);
		}
		else
		{
			i = min(m_gcfg2.getUInt(GameHdr->id, "language", 0), ARRAY_SIZE(CMenu::_languages) - 1u);
			m_btnMgr.setText(m_configLblVal[j], _t(CMenu::_languages[i].id, CMenu::_languages[i].text), true);
		}
		m_btnMgr.show(m_configLbl[j], instant);
		m_btnMgr.show(m_configLblVal[j], instant);
		m_btnMgr.show(m_configBtnM[j], instant);
		m_btnMgr.show(m_configBtnP[j], instant);
	}
	else if(curPage == CHEAT_SETTINGS)
	{
		m_btnMgr.setText(m_configLblTitle, _t("cfgg92", L"Cheat codes"));
		m_btnMgr.show(m_configLblTitle);

		//! ACTIVITY LED
		if(!IsOnWiiU())
		{
			m_btnMgr.setText(m_configLbl[3], _t("cfgg38", L"Disable LED"));
			m_checkboxBtn[3] = m_gcfg2.getBool(GameHdr->id, "led", 0) == 0 ? m_configChkOff[3] : m_configChkOn[3];
			m_btnMgr.show(m_configLbl[3], instant);
			m_btnMgr.show(m_checkboxBtn[3], instant);
		}
		
		//! DELETE CHEATS
		m_btnMgr.setText(m_configLbl[4], _t("cfg833", L"Delete cheat file"));
		m_btnMgr.setText(m_configBtn[4], _t("cfgbnr6", L"Delete"));
		m_btnMgr.show(m_configLbl[4], instant);
		m_btnMgr.show(m_configBtn[4], instant);
		
		//! DEBUGGER
		if(!(IsOnWiiU() && GameHdr->type == TYPE_GC_GAME))
		{
			m_btnMgr.setText(m_configLbl[5], _t("cfgg22", L"Debugger"));
			i = min(m_gcfg2.getUInt(GameHdr->id, "debugger", 0), ARRAY_SIZE(CMenu::_debugger) - 1u);
			m_btnMgr.setText(m_configLblVal[5], _t(CMenu::_debugger[i].id, CMenu::_debugger[i].text), true);
			m_btnMgr.show(m_configLbl[5], instant);
			m_btnMgr.show(m_configLblVal[5], instant);
			m_btnMgr.show(m_configBtnM[5], instant);
			m_btnMgr.show(m_configBtnP[5], instant);
		}
		
		//! HOOKTYPE
		if(GameHdr->type != TYPE_GC_GAME)
		{
			m_btnMgr.setText(m_configLbl[6], _t("cfgg18", L"Hook type"));
			i = min(m_gcfg2.getUInt(GameHdr->id, "hooktype", 0), ARRAY_SIZE(CMenu::_hooktype) - 1u);
			m_btnMgr.setText(m_configLblVal[6], _t(CMenu::_hooktype[i].id, CMenu::_hooktype[i].text), true);
			m_btnMgr.show(m_configLbl[6], instant);
			m_btnMgr.show(m_configLblVal[6], instant);
			m_btnMgr.show(m_configBtnM[6], instant);
			m_btnMgr.show(m_configBtnP[6], instant);
		}
	}
	else if(curPage == VIDEO_SETTINGS)
	{
		m_btnMgr.setText(m_configLblTitle, _t("cfgg97", L"Game video settings"));
		m_btnMgr.show(m_configLblTitle);
		if(GameHdr->type == TYPE_GC_GAME)
		{
			//! WIIU WIDESCREEN -> default global
			m_btnMgr.setText(m_configLbl[1], _t("cfgg46", L"WiiU Widescreen"));
			m_btnMgr.setText(m_configLblVal[1], _optBoolToString(m_gcfg2.getOptBool(GameHdr->id, "wiiu_widescreen", 2)));
			
			//! WIDESCREEN PATCH -> default global
			m_btnMgr.setText(m_configLbl[2], _t("cfgg36", L"Widescreen patch"));
			m_btnMgr.setText(m_configLblVal[2], _optBoolToString(m_gcfg2.getOptBool(GameHdr->id, "widescreen", 2)));
			
			//! VIDEO MODE -> default global
			m_btnMgr.setText(m_configLbl[3], _t("cfgg2", L"Video mode"));
			i = min(m_gcfg2.getUInt(GameHdr->id, "video_mode", 0), ARRAY_SIZE(CMenu::_GCvideoModes) - 1u);
			m_btnMgr.setText(m_configLblVal[3], _t(CMenu::_GCvideoModes[i].id, CMenu::_GCvideoModes[i].text), true);
			
			//! VIDEO WIDTH -> default global
			m_btnMgr.setText(m_configLbl[4], _t("cfgg54", L"Video width"));
			if(videoScale == 0)
				m_btnMgr.setText(m_configLblVal[4], _t("GC_Auto", L"Auto"));
			else if(videoScale == 127)
				m_btnMgr.setText(m_configLblVal[4], _t("def", L"Default"));
			else
				m_btnMgr.setText(m_configLblVal[4], wfmt(L"%i", max(40, min(120, videoScale)) + 600));
			
			//! VIDEO POSITION -> default global
			m_btnMgr.setText(m_configLbl[5], _t("cfgg55", L"Video position"));
			if(videoOffset == 127)
				m_btnMgr.setText(m_configLblVal[5], _t("def", L"Default"));
			else
				m_btnMgr.setText(m_configLblVal[5], wfmt(L"%i", max(-20, min(20, videoOffset))));
			
			//! DEFLICKER
			m_btnMgr.setText(m_configLbl[6], _t("cfgg44", L"Video deflicker"));
			m_checkboxBtn[6] = m_gcfg2.getBool(GameHdr->id, "deflicker", 0) == 0 ? m_configChkOff[6] : m_configChkOn[6];
			
			//! PAL50 PATCH
			m_btnMgr.setText(m_configLbl[7], _t("cfgg56", L"Patch PAL50"));
			m_checkboxBtn[7] = m_gcfg2.getBool(GameHdr->id, "patch_pal50", 0) == 0 ? m_configChkOff[7] : m_configChkOn[7];
			
			for(u8 i = (2 - IsOnWiiU()); i < 8; ++i)
			{
				m_btnMgr.show(m_configLbl[i], instant);
				if(i < 6)
				{
					m_btnMgr.show(m_configLblVal[i], instant);
					m_btnMgr.show(m_configBtnM[i], instant);
					m_btnMgr.show(m_configBtnP[i], instant);
				}
				else
					m_btnMgr.show(m_checkboxBtn[i], instant);
			}
		}
		else // wii and channels
		{
			//! ASPECT RATIO
			m_btnMgr.setText(m_configLbl[2], _t("cfgg27", L"Aspect ratio"));
			i = min(m_gcfg2.getUInt(GameHdr->id, "aspect_ratio", 0), ARRAY_SIZE(CMenu::_AspectRatio) - 1u);
			m_btnMgr.setText(m_configLblVal[2], _t(CMenu::_AspectRatio[i].id, CMenu::_AspectRatio[i].text), true);		
		
			//! VIDEO MODE -> default global
			m_btnMgr.setText(m_configLbl[3], _t("cfgg2", L"Video mode"));
			i = min(m_gcfg2.getUInt(GameHdr->id, "video_mode", 0), ARRAY_SIZE(CMenu::_VideoModes) - 1u);
			m_btnMgr.setText(m_configLblVal[3], _t(CMenu::_VideoModes[i].id, CMenu::_VideoModes[i].text), true);

			//! DEFLICKER WII -> default global
			m_btnMgr.setText(m_configLbl[4], _t("cfgg44", L"Video deflicker"));
			i = min(m_gcfg2.getUInt(GameHdr->id, "deflicker_wii", 0), ARRAY_SIZE(CMenu::_DeflickerOptions) - 1u);
			m_btnMgr.setText(m_configLblVal[4], _t(CMenu::_DeflickerOptions[i].id, CMenu::_DeflickerOptions[i].text), true);

			//! FIX 480P -> default global
			m_btnMgr.setText(m_configLbl[5], _t("cfgg49", L"480p pixel patch"));
			m_btnMgr.setText(m_configLblVal[5], _optBoolToString(m_gcfg2.getOptBool(GameHdr->id, "fix480p", 2)));
			
			//! PATCH VIDEO MODES
			m_btnMgr.setText(m_configLbl[6], _t("cfgg14", L"Patch video modes"));
			i = min(m_gcfg2.getUInt(GameHdr->id, "patch_video_modes", 0), ARRAY_SIZE(CMenu::_vidModePatch) - 1u);
			m_btnMgr.setText(m_configLblVal[6], _t(CMenu::_vidModePatch[i].id, CMenu::_vidModePatch[i].text), true);
			
			//! VI PATCH
			m_btnMgr.setText(m_configLbl[7], _t("cfgg7", L"VI patch"));
			m_checkboxBtn[7] = m_gcfg2.getBool(GameHdr->id, "vipatch", 0) == 0 ? m_configChkOff[7] : m_configChkOn[7];
			m_btnMgr.show(m_checkboxBtn[7], instant);
			
			for(u8 i = 2; i < 8; ++i)
			{
				m_btnMgr.show(m_configLbl[i], instant);
				if(i < 7)
				{
					m_btnMgr.show(m_configLblVal[i], instant);
					m_btnMgr.show(m_configBtnM[i], instant);
					m_btnMgr.show(m_configBtnP[i], instant);
				}
			}
		}
	}
	else if(curPage == NETWORK_SETTINGS)
	{
		m_btnMgr.setText(m_configLblTitle, _t("cfgg98", L"Network settings"));
		m_btnMgr.show(m_configLblTitle);
		if(GameHdr->type == TYPE_GC_GAME)
		{
			//! BBA EMULATION
			m_btnMgr.setText(m_configLbl[4], _t("cfgg59", L"BBA Emulation"));
			m_btnMgr.show(m_configLbl[4], instant);
			m_checkboxBtn[4] = m_gcfg2.getBool(GameHdr->id, "bba_emu", 0) == 0 ? m_configChkOff[4] : m_configChkOn[4];
			m_btnMgr.show(m_checkboxBtn[4], instant);
			
			//! BBA NET PROFILE
			m_btnMgr.setText(m_configLbl[5], _t("cfgg60", L"BBA Net Profile"));
			u8 netprofile = m_gcfg2.getUInt(GameHdr->id, "net_profile", 0);
			if(netprofile == 0)
				m_btnMgr.setText(m_configLblVal[5], _t("GC_Auto", L"Auto"));
			else
				m_btnMgr.setText(m_configLblVal[5], wfmt(L"%i", netprofile));
			m_btnMgr.show(m_configLbl[5], instant);
			m_btnMgr.show(m_configLblVal[5], instant);
			m_btnMgr.show(m_configBtnM[5], instant);
			m_btnMgr.show(m_configBtnP[5], instant);
		}
		else
		{
			//! PRIVATE SERVER
			m_btnMgr.setText(m_configLbl[4], _t("cfgg45", L"Private server"));
			i = min(m_gcfg2.getUInt(GameHdr->id, "private_server", 0), ARRAY_SIZE(CMenu::_privateServer) - 1u + custom_servers.size());
			if(i < ARRAY_SIZE(CMenu::_privateServer))		
				m_btnMgr.setText(m_configLblVal[4], _t(CMenu::_privateServer[i].id, CMenu::_privateServer[i].text), true);
			else
				m_btnMgr.setText(m_configLblVal[4], custom_servers[i - ARRAY_SIZE(CMenu::_privateServer)], true);
			m_btnMgr.show(m_configLbl[4], instant);
			m_btnMgr.show(m_configLblVal[4], instant);
			m_btnMgr.show(m_configBtnM[4], instant);
			m_btnMgr.show(m_configBtnP[4], instant);
		}
	}
	else if(curPage == COMPAT_SETTINGS)
	{
		m_btnMgr.show(m_configLblTitle);
		if(GameHdr->type == TYPE_GC_GAME)
		{
			m_btnMgr.setText(m_configLblTitle, _t("cfgg91", L"Emulation settings"));
			//! TRIFORCE MODE
			m_btnMgr.setText(m_configLbl[3], _t("cfgg48", L"Triforce arcade mode (insert coins)"));
			m_checkboxBtn[3] = m_gcfg2.getBool(GameHdr->id, "triforce_arcade", 0) == 0 ? m_configChkOff[3] : m_configChkOn[3];
			
			//! SKIP IPL BIOS
			m_btnMgr.setText(m_configLbl[4], _t("cfgg53", L"Skip IPL BIOS"));
			m_checkboxBtn[4] = m_gcfg2.getBool(GameHdr->id, "skip_ipl", 0) == 0 ? m_configChkOff[4] : m_configChkOn[4];

			//! WIIMOTE CC RUMBLE -> default global
			m_btnMgr.setText(m_configLbl[5], _t("cfgg52", L"Wiimote CC rumble"));
			m_btnMgr.setText(m_configLblVal[5], _optBoolToString(m_gcfg2.getOptBool(GameHdr->id, "cc_rumble", 2)));

			//! NATIVE CONTROL -> default global
			m_btnMgr.setText(m_configLbl[6], _t("cfgg43", L"Native control"));
			m_btnMgr.setText(m_configLblVal[6], _optBoolToString(m_gcfg2.getOptBool(GameHdr->id, "native_ctl", 2)));	

			for(u8 i = 3; i < (7 - IsOnWiiU()); ++i)
			{
				m_btnMgr.show(m_configLbl[i], instant);
				if(i < 5)
					m_btnMgr.show(m_checkboxBtn[i], instant);
				else
				{
					m_btnMgr.show(m_configLblVal[i], instant);
					m_btnMgr.show(m_configBtnM[i], instant);
					m_btnMgr.show(m_configBtnP[i], instant);
				}
			}
		}
		else // wii and channels
		{
			m_btnMgr.setText(m_configLblTitle, _t("cfgg93", L"Compatibility"));
			//! CIOS
			m_btnMgr.setText(m_configLbl[3], _t("cfgg10", L"cIOS"));
			int j = m_gcfg2.getInt(GameHdr->id, "ios", 0);
			if(j > 0)
				m_btnMgr.setText(m_configLblVal[3], wfmt(L"%i", j));
			else
				m_btnMgr.setText(m_configLblVal[3], _t("def", L"Default"));
			m_btnMgr.show(m_configLbl[3], instant);
			m_btnMgr.show(m_configLblVal[3], instant);
			m_btnMgr.show(m_configBtnM[3], instant);
			m_btnMgr.show(m_configBtnP[3], instant);
			
			//! COUNTRY STRING PATCH
			m_btnMgr.setText(m_configLbl[4], _t("cfgg4", L"Patch country strings"));
			m_btnMgr.show(m_configLbl[4], instant);
			m_checkboxBtn[4] = m_gcfg2.getBool(GameHdr->id, "country_patch", 0) == 0 ? m_configChkOff[4] : m_configChkOn[4];		
			
			if(GameHdr->type == TYPE_WII_GAME)
			{
				//! TEMPREGION NAND SWITCH
				m_btnMgr.setText(m_configLbl[5], _t("cfgg99", L"Adapt Wii region to game"));
				m_btnMgr.show(m_configLbl[5], instant);
				m_checkboxBtn[5] = m_gcfg2.getBool(GameHdr->id, "tempregionrn", 0) == 0 ? m_configChkOff[5] : m_configChkOn[5];
			}
			else // channels
			{
				//! APPLOADER
				m_btnMgr.setText(m_configLbl[5], _t("cfgg37", L"Apploader method"));	
				m_btnMgr.show(m_configLbl[5], instant);
				m_checkboxBtn[5] = m_gcfg2.getBool(GameHdr->id, "apploader", 0) == 0 ? m_configChkOff[5] : m_configChkOn[5];
				
				//! WII LAUNCH
				m_btnMgr.setText(m_configLbl[6], _t("custom", L"Wii launch"));
				m_btnMgr.show(m_configLbl[6], instant);
				m_checkboxBtn[6] = m_gcfg2.getBool(GameHdr->id, "custom", 0) == 0 ? m_configChkOff[6] : m_configChkOn[6];
				m_btnMgr.show(m_configLbl[6], instant);
				m_btnMgr.show(m_checkboxBtn[6], instant);
			}

			for(u8 i = 3; i < 6; ++i)
			{
				m_btnMgr.show(m_configLbl[i], instant);
				if(i > 3)
					m_btnMgr.show(m_checkboxBtn[i], instant);
			}
		}
	}
	else if(curPage == NANDEMU_SETTINGS) // nand or save emulation
	{
		m_btnMgr.setText(m_configLblTitle, _t("cfgg24", L"Nand emulation"));
		m_btnMgr.show(m_configLblTitle);
		if(GameHdr->type == TYPE_WII_GAME)
		{
			//! NAND EMULATION -> default global
			m_btnMgr.setText(m_configLbl[2], _t("cfgg24", L"Nand emulation"));
			i = min(m_gcfg2.getUInt(GameHdr->id, "emulate_save", 0), ARRAY_SIZE(CMenu::_SaveEmu) - 1u);
			m_btnMgr.setText(m_configLblVal[2], _t(CMenu::_SaveEmu[i].id, CMenu::_SaveEmu[i].text), true);			
			
			//! EXTRACT NAND SAVE
			m_btnMgr.setText(m_configLbl[6], _t("cfgg30", L"Extract Wii save to emunand"));
			m_btnMgr.setText(m_configBtn[6], _t("cfgg31", L"Extract"));
			
			//! FLASH NAND SAVE
			m_btnMgr.setText(m_configLbl[7], _t("cfgg32", L"Flash emunand save to Wii"));
			m_btnMgr.setText(m_configBtn[7], _t("cfgg33", L"Flash"));
			
			for(u8 i = 6; i < 8; ++i)
			{
				m_btnMgr.show(m_configLbl[i], instant);
				m_btnMgr.show(m_configBtn[i], instant);
			}
		}
		else if(GameHdr->type == TYPE_EMUCHANNEL)
		{
			//! NAND EMULATION -> default global
			m_btnMgr.setText(m_configLbl[2], _t("cfgg24", L"Nand emulation"));
			i = min(m_gcfg2.getUInt(GameHdr->id, "emulate_save", 0), ARRAY_SIZE(CMenu::_NandEmu) - 1u);
			m_btnMgr.setText(m_configLblVal[2], _t(CMenu::_NandEmu[i].id, CMenu::_NandEmu[i].text));
		}
		else if(GameHdr->type == TYPE_GC_GAME)
		{
			m_btnMgr.setText(m_configLblTitle, _t("cfgg47", L"Manage virtual MemCard"));
			//! MEMCARD EMULATION -> default global
			m_btnMgr.setText(m_configLbl[2], _t("cfgb11", L"Virtual MemCard mode"));
			i = min(m_gcfg2.getUInt(GameHdr->id, "emu_memcard", 0), ARRAY_SIZE(CMenu::_NinEmuCard) - 1u);
			m_btnMgr.setText(m_configLblVal[2], _t(CMenu::_NinEmuCard[i].id, CMenu::_NinEmuCard[i].text), true);
		}
		//! BACKUP SAVE
		m_btnMgr.setText(m_configLbl[3], _t("cfgsave1", L"Backup save file"));
		
		//! DELETE SAVE
		m_btnMgr.setText(m_configLbl[4], _t("cfgsave2", L"Delete save file"));
		
		//! RESTORE SAVE
		m_btnMgr.setText(m_configLbl[5], _t("cfgsave3", L"Restore save file"));
		
		m_btnMgr.show(m_configLblVal[2], instant);
		m_btnMgr.show(m_configBtnM[2], instant);
		m_btnMgr.show(m_configBtnP[2], instant);
		
		for(u8 i = 2; i < 6; ++i)
		{
			m_btnMgr.show(m_configLbl[i], instant);
			if(i > 2)
				m_btnMgr.show(m_configBtnGo[i], instant);
		}
	}
}

void CMenu::_gameSettings(const dir_discHdr *hdr, bool dvd)
{
	vector<string> custom_servers = stringToVector(m_cfg.getString("custom_servers", "servers"), '|');
	GameHdr = hdr; // set for global use in other functions
	
	curPage = MAIN_SETTINGS;

	if(GameHdr->type == TYPE_GC_GAME)
	{
		videoScale = m_gcfg2.getInt(GameHdr->id, "nin_width", 127);
		videoOffset = m_gcfg2.getInt(GameHdr->id, "nin_pos", 127);
	}

	_showGameSettings(false, dvd);
	
	while(!m_exit)
	{
		_mainLoopCommon(true);
		if(BTN_HOME_HELD || (BTN_B_OR_1_PRESSED && curPage == MAIN_SETTINGS))
			break;
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
			m_btnMgr.up();
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
			m_btnMgr.down();
		else if(BTN_B_OR_1_PRESSED)
		{
			_hideConfig(true);
			curPage = MAIN_SETTINGS;
			 _showGameSettings(false, dvd);
		}
		else if(BTN_A_OR_2_PRESSED)
		{
			if(m_btnMgr.selected(m_configBtnBack) && curPage != MAIN_SETTINGS)
			{
				_hideConfig(true);
				curPage = MAIN_SETTINGS;
				 _showGameSettings(false, dvd);
			}
			else if(curPage == MAIN_SETTINGS)
			{
				//! position game language button line correctly
				bool b = (GameHdr->type == TYPE_CHANNEL && isWiiVC);
				bool c = (GameHdr->type == TYPE_CHANNEL || (dvd && GameHdr->type != TYPE_GC_GAME));
				bool d = (GameHdr->type == TYPE_PLUGIN || GameHdr->type == TYPE_HOMEBREW);
				u8 j = 8 - (2 * dvd) - (2 * neek2o()) - b - c;
				
				if(m_btnMgr.selected(m_configBtnBack) || m_btnMgr.selected(m_configBtnCenter)) // center because of dvd
					break;
				else
				{
					//! DELETE GAME
					if(m_btnMgr.selected(m_configBtnGo[1+3*d]) && !dvd)
					{
						_hideConfig(true);
						if(GameHdr->type == TYPE_CHANNEL)
						{
							_error(_t("errgame17", L"Can't delete real nand channels!"));
							_showGameSettings(false, dvd);
						}
						else if(_wbfsOp(WO_REMOVE_GAME))
						{
							if(GameHdr->type == TYPE_PLUGIN) // if plugin delete cached cover
							{
								for(u8 i = 0; i < 2; ++i) // delete standard and snapshot ("_small") covers
									fsop_deleteFile(fmt("%s/%s/%s%s.wfc", m_cacheDir.c_str(), m_plugin.GetCoverFolderName(GameHdr->settings[0]), CoverFlow.getFilenameId(GameHdr), i == 1 ? "_small" : ""));
							}
							_setCurrentItem(CoverFlow.getNextHdr());
							_cleanupBanner();
							_loadList(); // no need to reshowCF and recache covers, just reload list and reinitCF
							_initCF();
							cacheCovers = false;
							
							m_newGame = true;
							break;
						}
						else
							_showGameSettings(false, dvd);
					}
					//! COVER AND BANNER
					else if(m_btnMgr.selected(m_configBtnGo[2]) && !dvd)
					{
						_hideConfig(true);
						_CoverBanner();
						_showGameSettings(false, dvd);
					}
					//! CHEAT CODES
					else if(m_btnMgr.selected(m_configBtnGo[3-2*dvd]))
					{
						_hideConfig(true);
						curPage = CHEAT_SETTINGS;
						_showGameSettings(false, dvd);
					}
					//! VIDEO SETTINGS
					else if(m_btnMgr.selected(m_configBtnGo[4-2*dvd]))
					{
						_hideConfig(true);
						curPage = VIDEO_SETTINGS;
						_showGameSettings(false, dvd);
					}
					//! NETWORK SETTINGS
					else if(m_btnMgr.selected(m_configBtnGo[5-2*dvd]))
					{
						_hideConfig(true);
						curPage = NETWORK_SETTINGS;
						_showGameSettings(false, dvd);
					}
					//! COMPATIBILITY
					else if(m_btnMgr.selected(m_configBtnGo[6-2*dvd]))
					{
						_hideConfig(true);
						curPage = COMPAT_SETTINGS;
						_showGameSettings(false, dvd);
					}
					//! NAND / SAVE EMULATION
					else if(m_btnMgr.selected(m_configBtnGo[7-2*dvd]))
					{
						if(!dvd || (dvd && GameHdr->type == TYPE_GC_GAME))
						{
							_hideConfig(true);
							curPage = NANDEMU_SETTINGS;
							_showGameSettings(false, dvd);
						}
					}
					//! GAME LANGUAGE
					else if(m_btnMgr.selected(m_configBtnP[j]) || m_btnMgr.selected(m_configBtnM[j]))
					{
						s8 direction = m_btnMgr.selected(m_configBtnP[j]) ? 1 : -1;
						if(GameHdr->type == TYPE_GC_GAME)
							m_gcfg2.setInt(GameHdr->id, "language", (int)loopNum(m_gcfg2.getUInt(GameHdr->id, "language", 0) + direction, ARRAY_SIZE(CMenu::_GClanguages)));
						else
							m_gcfg2.setInt(GameHdr->id, "language", (int)loopNum(m_gcfg2.getUInt(GameHdr->id, "language", 0) + direction, ARRAY_SIZE(CMenu::_languages)));
						_showGameSettings(true, dvd);
					}
				}
			}
			else if(curPage == CHEAT_SETTINGS)
			{
				//! ACTIVITY LED
				if(m_btnMgr.selected(m_checkboxBtn[3]))
				{
					m_gcfg2.setBool(GameHdr->id, "led", !m_gcfg2.getBool(GameHdr->id, "led", 0));
					_showGameSettings(true, dvd);
					m_btnMgr.setSelected(m_checkboxBtn[3]);
				}
				//! DELETE CHEATS
				if(m_btnMgr.selected(m_configBtn[4]))
				{
					if(_error(_t("errcfg5", L"Are you sure?"), true))
					{
						fsop_deleteFile(fmt("%s/%s.gct", m_cheatDir.c_str(), GameHdr->id));
						fsop_deleteFile(fmt("%s/%s.txt", m_txtCheatDir.c_str(), GameHdr->id));
						m_gcfg2.remove(GameHdr->id, "cheat");
						m_gcfg2.remove(GameHdr->id, "hooktype");
						_error(_t("dlmsg14", L"Done."));
					}
					_showGameSettings(false, dvd);
				}
				//! DEBUGGER
				else if(m_btnMgr.selected(m_configBtnP[5]) || m_btnMgr.selected(m_configBtnM[5]))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[5]) ? 1 : -1;
					m_gcfg2.setInt(GameHdr->id, "debugger", (int)loopNum(m_gcfg2.getUInt(GameHdr->id, "debugger", 0) + direction, ARRAY_SIZE(CMenu::_debugger)));
					_showGameSettings(true, dvd);
				}
				//! HOOKTYPE
				else if(m_btnMgr.selected(m_configBtnP[6]) || m_btnMgr.selected(m_configBtnM[6]))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[6]) ? 1 : -1;
					m_gcfg2.setInt(GameHdr->id, "hooktype", (int)loopNum(m_gcfg2.getUInt(GameHdr->id, "hooktype", 1) + direction, ARRAY_SIZE(CMenu::_hooktype)));
					_showGameSettings(true, dvd);
				}
			}
			else if(curPage == VIDEO_SETTINGS)
			{
				if(GameHdr->type == TYPE_GC_GAME)
				{
					//! WIIU WIDESCREEN
					if(m_btnMgr.selected(m_configBtnP[1]) || m_btnMgr.selected(m_configBtnM[1]))
					{
						s8 direction = m_btnMgr.selected(m_configBtnP[1]) ? 1 : -1;
						m_gcfg2.setOptBool(GameHdr->id, "wiiu_widescreen", loopNum(m_gcfg2.getOptBool(GameHdr->id, "wiiu_widescreen") + direction, 3));
						_showGameSettings(true, dvd);
					}
					//! WIDESCREEN PATCH (GC)
					else if(m_btnMgr.selected(m_configBtnP[2]) || m_btnMgr.selected(m_configBtnM[2]))
					{
						s8 direction = m_btnMgr.selected(m_configBtnP[2]) ? 1 : -1;
						m_gcfg2.setOptBool(GameHdr->id, "widescreen", loopNum(m_gcfg2.getOptBool(GameHdr->id, "widescreen") + direction, 3));
						_showGameSettings(true, dvd);			
					}
					//! VIDEO MODE
					else if(m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3]))
					{
						s8 direction = m_btnMgr.selected(m_configBtnP[3]) ? 1 : -1;
						m_gcfg2.setInt(GameHdr->id, "video_mode", (int)loopNum(m_gcfg2.getUInt(GameHdr->id, "video_mode", 0) + direction, ARRAY_SIZE(CMenu::_GCvideoModes)));
						_showGameSettings(true, dvd);
					}
					//! GC VIDEO WIDTH
					else if(m_btnMgr.selected(m_configBtnP[4]) || m_btnMgr.selected(m_configBtnM[4]))
					{
						if(m_btnMgr.selected(m_configBtnP[4]))
						{
							if(videoScale == 0)
								videoScale = 40;
							else if(videoScale == 127)
								videoScale = 0;
							else if(videoScale < 120)
								videoScale += 2;
						}
						else
						{
							if(videoScale == 40)
								videoScale = 0;
							else if(videoScale == 0)
								videoScale = 127;
							else if(videoScale > 40 && videoScale != 127)
								videoScale -= 2;
						}
						m_gcfg2.setInt(GameHdr->id, "nin_width", videoScale);
						_showGameSettings(true, dvd);
					}
					//! GC VIDEO POSITION
					else if(m_btnMgr.selected(m_configBtnP[5]) || m_btnMgr.selected(m_configBtnM[5]))
					{
						if(m_btnMgr.selected(m_configBtnP[5]))
						{
							if(videoOffset == 127)
								videoOffset = -20;
							else if(videoOffset < 20)
								videoOffset++;
						}
						else
						{
							if(videoOffset == -20)
								videoOffset = 127;
							else if(videoOffset > -20 && videoOffset != 127)
								videoOffset--;
						}
						m_gcfg2.setInt(GameHdr->id, "nin_pos", videoOffset);
						_showGameSettings(true, dvd);
					}
					//! DEFLICKER
					else if(m_btnMgr.selected(m_checkboxBtn[6]))
					{
						m_gcfg2.setBool(GameHdr->id, "deflicker", !m_gcfg2.getBool(GameHdr->id, "deflicker", 0));
						_showGameSettings(true, dvd);
						m_btnMgr.setSelected(m_checkboxBtn[6]);
					}
					//! PAL50 PATCH
					else if(m_btnMgr.selected(m_checkboxBtn[7]))
					{
						m_gcfg2.setBool(GameHdr->id, "patch_pal50", !m_gcfg2.getBool(GameHdr->id, "patch_pal50", 0));
						_showGameSettings(true, dvd);
						m_btnMgr.setSelected(m_checkboxBtn[7]);
					}
				}
				else // wii and channels
				{
					//! ASPECT RATIO (Wii + Channels)
					if(m_btnMgr.selected(m_configBtnP[2]) || m_btnMgr.selected(m_configBtnM[2]))
					{
						s8 direction = m_btnMgr.selected(m_configBtnP[2]) ? 1 : -1;
						m_gcfg2.setInt(GameHdr->id, "aspect_ratio", (int)loopNum(m_gcfg2.getUInt(GameHdr->id, "aspect_ratio", 0) + direction, ARRAY_SIZE(CMenu::_AspectRatio)));
						_showGameSettings(true, dvd);
					}
					//! VIDEO MODE
					else if(m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3]))
					{
						s8 direction = m_btnMgr.selected(m_configBtnP[3]) ? 1 : -1;
						m_gcfg2.setInt(GameHdr->id, "video_mode", (int)loopNum(m_gcfg2.getUInt(GameHdr->id, "video_mode", 0) + direction, ARRAY_SIZE(CMenu::_VideoModes)));
						_showGameSettings(true, dvd);
					}
					//! DEFLICKER WII
					else if(m_btnMgr.selected(m_configBtnP[4]) || m_btnMgr.selected(m_configBtnM[4]))
					{
						s8 direction = m_btnMgr.selected(m_configBtnP[4]) ? 1 : -1;
						m_gcfg2.setInt(GameHdr->id, "deflicker_wii", (int)loopNum(m_gcfg2.getUInt(GameHdr->id, "deflicker_wii", 0) + direction, ARRAY_SIZE(CMenu::_DeflickerOptions)));
						_showGameSettings(true, dvd);
					}
					//! FIX 480P
					else if(m_btnMgr.selected(m_configBtnP[5]) || m_btnMgr.selected(m_configBtnM[5]))
					{
						s8 direction = m_btnMgr.selected(m_configBtnP[5]) ? 1 : -1;
						m_gcfg2.setOptBool(GameHdr->id, "fix480p", loopNum(m_gcfg2.getOptBool(GameHdr->id, "fix480p") + direction, 3));
						_showGameSettings(true, dvd);
					}
					//! PATCH VIDEO MODES
					else if(m_btnMgr.selected(m_configBtnP[6]) || m_btnMgr.selected(m_configBtnM[6]))
					{
						s8 direction = m_btnMgr.selected(m_configBtnP[6]) ? 1 : -1;
						m_gcfg2.setInt(GameHdr->id, "patch_video_modes", (int)loopNum(m_gcfg2.getUInt(GameHdr->id, "patch_video_modes", 0) + direction, ARRAY_SIZE(CMenu::_vidModePatch)));
						_showGameSettings(true, dvd);
					}
					//! VI PATCH
					else if(m_btnMgr.selected(m_checkboxBtn[7]))
					{
						m_gcfg2.setBool(GameHdr->id, "vipatch", !m_gcfg2.getBool(GameHdr->id, "vipatch", 0));
						_showGameSettings(true, dvd);
						m_btnMgr.setSelected(m_checkboxBtn[7]);
					}
				}
			}
			else if(curPage == NETWORK_SETTINGS)
			{
				if(GameHdr->type == TYPE_GC_GAME)
				{
					//! BBA EMULATION
					if(m_btnMgr.selected(m_checkboxBtn[4]))
					{
						m_gcfg2.setBool(GameHdr->id, "bba_emu", !m_gcfg2.getBool(GameHdr->id, "bba_emu", 0));
						_showGameSettings(true, dvd);
						m_btnMgr.setSelected(m_checkboxBtn[4]);
					}
					//! BBA NET PROFILE
					else if(m_btnMgr.selected(m_configBtnP[5]) || m_btnMgr.selected(m_configBtnM[5]))
					{
						s8 direction = m_btnMgr.selected(m_configBtnP[5]) ? 1 : -1;
						m_gcfg2.setInt(GameHdr->id, "net_profile", loopNum(m_gcfg2.getInt(GameHdr->id, "net_profile") + direction, 4));
						_showGameSettings(true, dvd);
					}
				}
				else // wii and channels
				{
					//! PRIVATE SERVER
					if(m_btnMgr.selected(m_configBtnP[4]) || m_btnMgr.selected(m_configBtnM[4]))
					{
						s8 direction = m_btnMgr.selected(m_configBtnP[4]) ? 1 : -1;
						u8 val = loopNum(m_gcfg2.getUInt(GameHdr->id, "private_server") + direction, ARRAY_SIZE(CMenu::_privateServer) + custom_servers.size());
						m_gcfg2.setUInt(GameHdr->id, "private_server", val);
						_showGameSettings(true, dvd);
					}
				}
			}
			else if(curPage == COMPAT_SETTINGS)
			{
				if(GameHdr->type == TYPE_GC_GAME)
				{
					//! TRIFORCE MODE
					if(m_btnMgr.selected(m_checkboxBtn[3]))
					{
						m_gcfg2.setBool(GameHdr->id, "triforce_arcade", !m_gcfg2.getBool(GameHdr->id, "triforce_arcade", 0));
						_showGameSettings(true, dvd);
						m_btnMgr.setSelected(m_checkboxBtn[3]);
					}
					//! SKIP IPL BIOS
					else if(m_btnMgr.selected(m_checkboxBtn[4]))
					{
						m_gcfg2.setBool(GameHdr->id, "skip_ipl", !m_gcfg2.getBool(GameHdr->id, "skip_ipl", 0));
						_showGameSettings(true, dvd);
						m_btnMgr.setSelected(m_checkboxBtn[4]);
					}
					//! WIIMOTE CC RUMBLE
					else if(m_btnMgr.selected(m_configBtnP[5]) || m_btnMgr.selected(m_configBtnM[5]))
					{
						s8 direction = m_btnMgr.selected(m_configBtnP[5]) ? 1 : -1;
						m_gcfg2.setOptBool(GameHdr->id, "cc_rumble", loopNum(m_gcfg2.getOptBool(GameHdr->id, "cc_rumble") + direction, 3));
						_showGameSettings(true, dvd);
					}
					//! NATIVE CONTROL
					else if(m_btnMgr.selected(m_configBtnP[6]) || m_btnMgr.selected(m_configBtnM[6]))
					{
						s8 direction = m_btnMgr.selected(m_configBtnP[6]) ? 1 : -1;
						m_gcfg2.setOptBool(GameHdr->id, "native_ctl", loopNum(m_gcfg2.getOptBool(GameHdr->id, "native_ctl") + direction, 3));
						_showGameSettings(true, dvd);
					}
				}
				else // wii and channels
				{
					//! GAME CIOS
					if(m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3]))
					{
						if(_installed_cios.size() > 0)
						{
							bool direction = m_btnMgr.selected(m_configBtnP[3]);
							CIOSItr itr = _installed_cios.find((u32)m_gcfg2.getInt(GameHdr->id, "ios", 0));
							if(direction && itr == _installed_cios.end())
								itr = _installed_cios.begin();
							else if(!direction && itr == _installed_cios.begin())
								itr = _installed_cios.end();
							else if(direction)
								itr++;

							if(!direction)
								itr--;

							if(itr->first != 0)
								m_gcfg2.setInt(GameHdr->id, "ios", itr->first);
							else
								m_gcfg2.remove(GameHdr->id, "ios");
							_showGameSettings(true, dvd);
						}
					}
					//! COUNTRY STRING PATCH
					else if(m_btnMgr.selected(m_checkboxBtn[4]))
					{
						m_gcfg2.setBool(GameHdr->id, "country_patch", !m_gcfg2.getBool(GameHdr->id, "country_patch", 0));
						_showGameSettings(true, dvd);
						m_btnMgr.setSelected(m_checkboxBtn[4]);
					}
					else if(GameHdr->type == TYPE_WII_GAME)
					{
						//! TEMPREGION NAND SWITCH
						if(m_btnMgr.selected(m_checkboxBtn[5]))
						{
							m_gcfg2.setBool(GameHdr->id, "tempregionrn", !m_gcfg2.getBool(GameHdr->id, "tempregionrn", 0));
							_showGameSettings(true, dvd);
							m_btnMgr.setSelected(m_checkboxBtn[5]);
						}
					}
					else // channels
					{
						//! APPLOADER
						if(m_btnMgr.selected(m_checkboxBtn[5]))
						{
							m_gcfg2.setBool(GameHdr->id, "apploader", !m_gcfg2.getBool(GameHdr->id, "apploader", 0));
							_showGameSettings(true, dvd);
							m_btnMgr.setSelected(m_checkboxBtn[5]);
						}
						//! WII LAUNCH
						else if(m_btnMgr.selected(m_checkboxBtn[6]))
						{
							m_gcfg2.setBool(GameHdr->id, "custom", !m_gcfg2.getBool(GameHdr->id, "custom", 0));
							_showGameSettings(true, dvd);
							m_btnMgr.setSelected(m_checkboxBtn[6]);
						}
					}
				}
			}
			else if(curPage == NANDEMU_SETTINGS)
			{
				if(m_btnMgr.selected(m_configBtnP[2]) || m_btnMgr.selected(m_configBtnM[2]))
				{
					s8 direction = m_btnMgr.selected(m_configBtnP[2]) ? 1 : -1;
					if(GameHdr->type == TYPE_WII_GAME)
						m_gcfg2.setInt(GameHdr->id, "emulate_save", (int)loopNum(m_gcfg2.getUInt(GameHdr->id, "emulate_save", 0) + direction, ARRAY_SIZE(CMenu::_SaveEmu)));
					else if(GameHdr->type == TYPE_EMUCHANNEL)
						//! using "emulate_save" parameter (same as wii game) even if it's not actually save emulation
						m_gcfg2.setInt(GameHdr->id, "emulate_save", (int)loopNum(m_gcfg2.getUInt(GameHdr->id, "emulate_save", 0) + direction, ARRAY_SIZE(CMenu::_NandEmu)));
					else if(GameHdr->type == TYPE_GC_GAME)
						m_gcfg2.setInt(GameHdr->id, "emu_memcard", (int)loopNum(m_gcfg2.getUInt(GameHdr->id, "emu_memcard", 2) + direction, ARRAY_SIZE(CMenu::_NinEmuCard)));
					_showGameSettings(true, dvd);
				}
				//! EXTRACT - FLASH SAVE
				else if(m_btnMgr.selected(m_configBtn[6]) || m_btnMgr.selected(m_configBtn[7]))
				{
					bool extract = m_btnMgr.selected(m_configBtn[6]);
					bool nosave = false;
					if(extract) // extract save from nand to saves emunand
					{
						if(_ExtractGameSave(GameHdr->id) < 0) // -1
							nosave = true;
					}
					else // flash save from saves emunand to nand
					{
						if(_FlashGameSave(GameHdr->id) < 0) // -1
							nosave = true;
					}
					if(nosave)
						_error(_t("cfgmc10", L"File not found!"));
					_showGameSettings(false, dvd);
				}
				//! BACKUP - REMOVE - RESTORE EMUNAND MEMCARD OR SAVE
				else 
				{
					for(u8 i = WO_BACKUP_EMUSAVE; i <= WO_RESTORE_EMUSAVE; ++i) // 1, 2, 3
					{
						if(m_btnMgr.selected(m_configBtnGo[i+2])) // 3, 4, 5
						{
							_hideConfig(true);
							_wbfsOp(i); // enum in menu.hpp
							_showGameSettings(false, dvd);
						}
					}
				}
			}
		}
	}

	_hideConfig(true);	
}
