
#include "menu.hpp"

bool g = false;

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

void CMenu::_showConfigDownload(bool instant)
{
	_hideCheckboxes(true); // reset checkboxes
	
	m_btnMgr.setText(m_configLblTitle, _t("dl15", L"Download options"));
	m_btnMgr.show(m_configLblTitle, instant);
	m_btnMgr.show(m_configBtnBack, instant);

	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);
		
	m_btnMgr.setText(m_configLbl[2], _t("dl99", L"Cover style"));
	m_btnMgr.setText(m_configLblVal[2], m_cfg.getBool(general_domain, "dl_box_cover", true) ?  _t("dl96", L"Box") : _t("dl95", L"Flat"));
	
	m_btnMgr.setText(m_configLbl[3], _t("dl98", L"Cover type"));
	m_btnMgr.setText(m_configLblVal[3], m_cfg.getBool(general_domain, "dl_normal_cover", true) ?  _t("dl94", L"Normal") : _t("dl93", L"Alt"));	
	
	m_btnMgr.setText(m_configLbl[4], _t("dl14", L"Replace missing PAL games with"));
	int i = min(m_cfg.getUInt(general_domain, "dl_add_region", EN), ARRAY_SIZE(CMenu::_AddRegionCover) - 1u);
	m_btnMgr.setText(m_configLblVal[4], _t(CMenu::_AddRegionCover[i].id, CMenu::_AddRegionCover[i].text));

	for(u8 i = 2; i < 5; ++i)
	{
		m_btnMgr.show(m_configLbl[i], instant);
		m_btnMgr.show(m_configLblVal[i], instant);
		m_btnMgr.show(m_configBtnM[i], instant);
		m_btnMgr.show(m_configBtnP[i], instant);
	}	
	if(!g)
	{
		m_btnMgr.setText(m_configLbl[5], _t("dl91", L"Skip replaced artwork"));
		m_checkboxBtn[5] = m_cfg.getBool(general_domain, "dl_skip_replaced", 1) ? m_configChkOn[5] : m_configChkOff[5];
		m_btnMgr.show(m_checkboxBtn[5], instant);
		
		m_btnMgr.setText(m_configLbl[6], _t("dl97", L"Flag all covers as not replaced"));
		
		m_btnMgr.setText(m_configLbl[7], _t("dl90", L"Flag all discs as not replaced"));

		bool chan = (m_current_view & COVERFLOW_CHANNEL) ? true : false;
		for(u8 i = 5; i < 8 - chan; ++i)
		{
			m_btnMgr.show(m_configLbl[i], instant);
			if(i > 5)
			{
				m_btnMgr.setText(m_configBtn[i], _t("cfgne6", L"Start"));
				m_btnMgr.show(m_configBtn[i], instant);
			}
		}
	}
}

void CMenu::_configDownload(bool fromGameSet)
{
	SetupInput();
	g = fromGameSet;
	_showConfigDownload();

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
			else if(m_btnMgr.selected(m_configBtnP[2]) || m_btnMgr.selected(m_configBtnM[2])) // cover style (box / flat)
			{
				m_cfg.setBool(general_domain, "dl_box_cover", !m_cfg.getBool(general_domain, "dl_box_cover"));
				_showConfigDownload(true);
			}
			else if(m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3])) // cover type (normal / alt)
			{
				m_cfg.setBool(general_domain, "dl_normal_cover", !m_cfg.getBool(general_domain, "dl_normal_cover"));
				_showConfigDownload(true);
			}
			else if(m_btnMgr.selected(m_configBtnP[4]) || m_btnMgr.selected(m_configBtnM[4])) // additional region check for PAL games
			{
				s8 direction = m_btnMgr.selected(m_configBtnP[4]) ? 1 : -1;
				m_cfg.setInt(general_domain, "dl_add_region", (int)loopNum(m_cfg.getUInt(general_domain, "dl_add_region", 0) + direction, ARRAY_SIZE(CMenu::_AddRegionCover)));
				_showConfigDownload(true);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[5])) // skip replaced covers
			{
				m_cfg.setBool(general_domain, "dl_skip_replaced", !m_cfg.getBool(general_domain, "dl_skip_replaced"));
				_showConfigDownload(true);
				m_btnMgr.setSelected(m_checkboxBtn[5]);
			}
			else if(m_btnMgr.selected(m_configBtn[6]) || m_btnMgr.selected(m_configBtn[7])) // remove replace flags
			{
				bool disc = m_btnMgr.selected(m_configBtn[7]);
				if(error(_t("errcfg7", L"All artwork from current coverflow will be marked as not replaced!"), true))
				{
					for(u32 i = 0; i < m_gameList.size(); ++i)
					{
						if(m_gameList[i].type == TYPE_PLUGIN || m_gameList[i].type == TYPE_HOMEBREW)
							continue;
						m_gcfg2.remove(m_gameList[i].id, disc ? "alt_disc" : "alt_cover");
					}
					error(_t("dlmsg14", L"Done."));
				}
				_showConfigDownload();
			}
		}
	}
	
	_hideConfig(true);
}
