
#include "menu.hpp"

template <class T> static inline T loopNum(T i, T s)
{
	return (i + s) % s;
}

const CMenu::SOption CMenu::_AddRegionCover[10] = {
	{ "none", L"None" },
	{ "EN", L"UK" },
	{ "FR", L"France" },
	{ "DE", L"Germany" },
	{ "ES", L"Spain" },
	{ "IT", L"Italy" },
	{ "NL", L"Netherlands" },
	{ "PT", L"Portugal" },
	{ "AU", L"Australia" },
	{ "US", L"USA" }
};

void CMenu::_showConfigDownload(bool instant, bool fromGameSet)
{
	_hideCheckboxes(true); // reset checkboxes
	
	m_btnMgr.setText(m_configLblTitle, _t("dl15", L"Download options"));
	m_btnMgr.show(m_configLblTitle, instant);
	m_btnMgr.show(m_configBtnBack, instant);

	m_btnMgr.setText(m_configLbl[3], _t("dl99", L"Cover style"));
	m_btnMgr.setText(m_configLblVal[3], m_cfg.getBool(general_domain, "dl_box_cover", true) ?  _t("dl96", L"Box") : _t("dl95", L"Flat"));
	
	m_btnMgr.setText(m_configLbl[4], _t("dl98", L"Cover type"));
	m_btnMgr.setText(m_configLblVal[4], m_cfg.getBool(general_domain, "dl_normal_cover", true) ?  _t("dl94", L"Normal") : _t("dl93", L"Alt"));	
	
	m_btnMgr.setText(m_configLbl[5], _t("dl14", L"Replace missing PAL games with"));
	int i = min(m_cfg.getUInt(general_domain, "dl_add_region", EN), ARRAY_SIZE(CMenu::_AddRegionCover) - 1u);
	m_btnMgr.setText(m_configLblVal[5], _t(CMenu::_AddRegionCover[i].id, CMenu::_AddRegionCover[i].text));

	for(u8 i = 3; i < 6; ++i)
	{
		m_btnMgr.show(m_configLbl[i], instant);
		m_btnMgr.show(m_configLblVal[i], instant);
		m_btnMgr.show(m_configBtnM[i], instant);
		m_btnMgr.show(m_configBtnP[i], instant);
	}	
	if(!fromGameSet)
	{
		m_btnMgr.setText(m_configLbl[6], _t("dl91", L"Skip replaced artwork"));
		m_checkboxBtn[6] = m_cfg.getBool(general_domain, "dl_skip_replaced", 1) ? m_configChkOn[6] : m_configChkOff[6];
		m_btnMgr.show(m_configLbl[6], instant);
		m_btnMgr.show(m_checkboxBtn[6], instant);
		
		m_btnMgr.setText(m_configLbl[7], _t("dl97", L"Reset all replace flags"));
		m_btnMgr.setText(m_configBtn[7], _t("cfgne6", L"Start"));
		m_btnMgr.show(m_configLbl[7], instant);
		m_btnMgr.show(m_configBtn[7], instant);
	}
}

void CMenu::_configDownload(bool fromGameSet)
{
	SetupInput();
	_showConfigDownload(false, fromGameSet);

	while(!m_exit)
	{
		_mainLoopCommon();
		if(BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED)
			break;
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
			m_btnMgr.up();
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
			m_btnMgr.down();
		else if(BTN_A_OR_2_PRESSED)
		{
			if(m_btnMgr.selected(m_configBtnBack))
				break;
			else if(m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3])) // cover style (box / flat)
			{
				m_cfg.setBool(general_domain, "dl_box_cover", !m_cfg.getBool(general_domain, "dl_box_cover"));
				_showConfigDownload(true);
			}
			else if(m_btnMgr.selected(m_configBtnP[4]) || m_btnMgr.selected(m_configBtnM[4])) // cover type (normal / alt)
			{
				m_cfg.setBool(general_domain, "dl_normal_cover", !m_cfg.getBool(general_domain, "dl_normal_cover"));
				_showConfigDownload(true);
			}
			else if(m_btnMgr.selected(m_configBtnP[5]) || m_btnMgr.selected(m_configBtnM[5])) // additional region check for PAL games
			{
				s8 direction = m_btnMgr.selected(m_configBtnP[5]) ? 1 : -1;
				m_cfg.setInt(general_domain, "dl_add_region", (int)loopNum(m_cfg.getUInt(general_domain, "dl_add_region", 0) + direction, ARRAY_SIZE(CMenu::_AddRegionCover)));
				_showConfigDownload(true);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[6])) // skip replaced covers
			{
				m_cfg.setBool(general_domain, "dl_skip_replaced", !m_cfg.getBool(general_domain, "dl_skip_replaced"));
				_showConfigDownload(true);
				m_btnMgr.setSelected(m_checkboxBtn[6]);
			}
			else if(m_btnMgr.selected(m_configBtn[7])) // remove all replace flags
			{
				const char *GameIdDomain = m_gcfg2.firstDomain().c_str();
				while(1)
				{
					if(strlen(GameIdDomain) < 2)
						break;
					m_gcfg2.remove(GameIdDomain, "alt_cover");
					m_gcfg2.remove(GameIdDomain, "alt_disc");
					GameIdDomain = m_gcfg2.nextDomain().c_str();
				}
				error(_t("dlmsg14", L"Done."));
				_showConfigDownload();
			}
		}
	}
	
	_hideConfig(true);
}
