
#include "menu.hpp"

static inline int loopNum(int i, int s)
{
	return (i + s) % s;
}

void CMenu::_showViewOptions(bool instant)
{
	m_btnMgr.setText(m_configLblTitle, _t("view1", L"View options"));
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_configBtnBack);

	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);

	//! View style (hidden if no plugin database)
	if(m_current_view & COVERFLOW_PLUGIN && m_platform.loaded())
	{
		m_btnMgr.setText(m_configLbl[3], _t("view4", L"View style"));
		m_btnMgr.setText(m_configLblVal[3], m_thumbnail ? _t("view7", L"Snapshots") : _t("view6", L"Covers"), true);
		m_btnMgr.show(m_configLbl[3], instant);
		m_btnMgr.show(m_configLblVal[3], instant);
		m_btnMgr.show(m_configBtnM[3], instant);
		m_btnMgr.show(m_configBtnP[3], instant);
	}
	//! Coverflow layout
	m_btnMgr.setText(m_configLbl[4], _t("view3", L"Coverflow layout"));
	m_btnMgr.setText(m_configLblVal[4], wfmt(L"%i", _getCFVersion()));
	//! Sort mode
	m_btnMgr.setText(m_configLbl[5], _t("view2", L"Sort mode"));
	m_btnMgr.setText(m_configLblVal[5], _sortLabel(m_cfg.getInt(_domainFromView(), "sort", SORT_ALPHA)), true);
	//! Explorer mode (hidden if channels or more than one plugin selected)
	if((m_current_view != COVERFLOW_CHANNEL) && !(m_current_view & COVERFLOW_PLUGIN && m_plugin.GetEnabledStatus(ENAND_PMAGIC)) && !((m_current_view & COVERFLOW_PLUGIN) && enabledPluginsCount != 1))
	{
		m_btnMgr.setText(m_configLbl[6], _t("view5", L"Explorer mode"));
		m_btnMgr.show(m_configLbl[6], instant);
		m_btnMgr.show(m_configBtnGo[6], instant);
	}
	
	for(u8 i = 4; i < 6; ++i)
	{
		m_btnMgr.show(m_configLbl[i], instant);
		m_btnMgr.show(m_configLblVal[i], instant);
		m_btnMgr.show(m_configBtnM[i], instant);
		m_btnMgr.show(m_configBtnP[i], instant);
	}
}

void CMenu::_viewOptions(void)
{
	SetupInput();	
	_showViewOptions();
	
	while(!m_exit)
	{
		_mainLoopCommon(true);
		if(BTN_HOME_HELD || BTN_B_OR_1_PRESSED)
			break;
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
			m_btnMgr.up();
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
			m_btnMgr.down();
		else if(BTN_A_OR_2_PRESSED)
		{
			if(m_btnMgr.selected(m_configBtnBack))
				break;

			else if(m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3])) // view style
			{
				m_thumbnail = !m_thumbnail;
				CoverFlow.fade(0);
				_showCF(false);
				CoverFlow.fade(1);
				_hideMain(true); // quick fix
				_setCFVersion(1);
				_showViewOptions(true);
			}
			else if(m_btnMgr.selected(m_configBtnP[4]) || m_btnMgr.selected(m_configBtnM[4])) // cf layout
			{
				s8 direction = m_btnMgr.selected(m_configBtnP[4]) ? 1 : -1;
				int cfVersion = 1 + loopNum((_getCFVersion() - 1) + direction, m_numCFVersions);
				_setCFVersion(cfVersion);
				_loadCFLayout(cfVersion);
				CoverFlow.fade(0);
				CoverFlow.applySettings(false);
				CoverFlow.fade(1);
				_showViewOptions(true);
			}
			else if(m_btnMgr.selected(m_configBtnP[5]) || m_btnMgr.selected(m_configBtnM[5])) // sort mode
			{
				s8 direction = m_btnMgr.selected(m_configBtnP[5]) ? 1 : -1;
				const char *domain = _domainFromView();
				int sort = m_cfg.getInt(domain, "sort", SORT_ALPHA);
				if(m_current_view & COVERFLOW_HOMEBREW)
					sort = loopNum(sort + direction, SORT_YEAR); // alpha, playcount & lastplayed only
				else if(m_current_view & COVERFLOW_PLUGIN) // alpha, playcount, lastplayed & year only
					sort = loopNum(sort + direction, SORT_GAMEID);
				else // change all other coverflow sort mode
					sort = loopNum(sort + direction, SORT_MAX);
				m_cfg.setInt(domain, "sort", sort);
				CoverFlow.fade(0);
				_initCF(); // set coverflow to new sorting
				CoverFlow._setCurPos(0); // force first cover of new sort as coverflow current position
				CoverFlow.fade(1);
				_showViewOptions(true);
			}
			else if(m_btnMgr.selected(m_configBtnGo[6])) // explorer
			{
				const char *gameDir = NULL;
				u32 plmagic = 0;
				if(m_current_view & COVERFLOW_WII)
					gameDir = fmt(wii_games_dir, DeviceName[m_cfg.getInt(wii_domain, "partition", USB1)]);
				else if(m_current_view & COVERFLOW_GAMECUBE)
					gameDir = fmt(gc_games_dir, DeviceName[m_cfg.getInt(gc_domain, "partition", USB1)]);
				else if(m_current_view & COVERFLOW_HOMEBREW)
					gameDir = fmt("%s:/%s", DeviceName[m_cfg.getInt(homebrew_domain, "partition", SD)], HOMEBREW_DIR);
				else if(m_current_view & COVERFLOW_PLUGIN)
				{
					u8 i = 0;
					while(m_plugin.PluginExist(i) && !m_plugin.GetEnabledStatus(i)) { ++i; }
					plmagic = m_plugin.GetPluginMagic(i);
					gameDir = m_plugin.GetExplorerPath(plmagic);
				}
				_hideConfig(true);
				_pluginExplorer(gameDir, plmagic, false); // false = not from source
				_setMainBg();
				break;
			}
		}
	}
	_hideConfig(true);
}
