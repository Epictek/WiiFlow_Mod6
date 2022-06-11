
#include "menu.hpp"

void CMenu::_showConfigMusic(bool instant)
{
	_hideCheckboxes(true); // reset checkboxes
	
	m_btnMgr.setText(m_configLblTitle, _t("cfg792", L"Music settings"));
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_configBtnBack);

	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);
	
	//! Music volume
	m_btnMgr.setText(m_configLbl[1], _t("cfgs1", L"Music volume"));
	m_btnMgr.setText(m_configLblVal[1], wfmt(L"%i", m_cfg.getInt(general_domain, "sound_volume_music", 255)));
	//! GUI sound volume
	m_btnMgr.setText(m_configLbl[2], _t("cfgs2", L"GUI sound volume"));
	m_btnMgr.setText(m_configLblVal[2], wfmt(L"%i", m_cfg.getInt(general_domain, "sound_volume_gui", 255)));
	//! Coverflow sound volume
	m_btnMgr.setText(m_configLbl[3], _t("cfgs3", L"Coverflow sound volume"));		
	m_btnMgr.setText(m_configLblVal[3], wfmt(L"%i", m_cfg.getInt(general_domain, "sound_volume_coverflow", 255)));
	//! Banner sound volume
	m_btnMgr.setText(m_configLbl[4], _t("cfgs4", L"Banner sound volume"));
	m_btnMgr.setText(m_configLblVal[4], wfmt(L"%i", m_cfg.getInt(general_domain, "sound_volume_bnr", 255)));
	//! Music path
	m_btnMgr.setText(m_configLbl[5], _t("cfgp7", L"Music path"));
	m_btnMgr.show(m_configBtnGo[5], instant);
	//! Randomize music
	m_btnMgr.setText(m_configLbl[6], _t("cfg715", L"Randomize music"));
	m_checkboxBtn[6] = m_cfg.getOptBool(general_domain, "randomize_music", 0) == 0 ? m_configChkOff[6] : m_configChkOn[6];
	//! Display music title
	m_btnMgr.setText(m_configLbl[7], _t("cfg714", L"Display music title"));
	m_checkboxBtn[7] = m_cfg.getOptBool(general_domain, "display_music_info", 0) == 0 ? m_configChkOff[7] : m_configChkOn[7];
	//! Upsample music to 48khz
	m_btnMgr.setText(m_configLbl[8], _t("cfg728", L"Upsample music to 48khz"));
	m_checkboxBtn[8] = m_cfg.getOptBool(general_domain, "resample_to_48khz", 1) == 0 ? m_configChkOff[8] : m_configChkOn[8];

	for(u8 i = 1; i < 9; ++i)
	{
		m_btnMgr.show(m_configLbl[i], instant);
		if(i < 5)
		{
			m_btnMgr.show(m_configLblVal[i], instant);
			m_btnMgr.show(m_configBtnM[i], instant);
			m_btnMgr.show(m_configBtnP[i], instant);		
		}
		else if(i > 5)
			m_btnMgr.show(m_checkboxBtn[i], instant);
	}
}

void CMenu::_configMusic(void)
{
	bool cur_rand_music = m_cfg.getBool(general_domain, "randomize_music");
	bool prev_rand_music = cur_rand_music;
	int cur_music_vol = m_cfg.getInt(general_domain, "sound_volume_music", 255);
	int prev_music_vol = cur_music_vol;

	SetupInput();
	_showConfigMusic();

	while(!m_exit)
	{
		_mainLoopCommon();
		if(BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED)
			break;
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
			m_btnMgr.up();
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
			m_btnMgr.down();

		if(BTN_A_REPEAT || BTN_A_OR_2_PRESSED)
		{
			int step = 5;
			if(m_btnMgr.selected(m_configBtnP[1]) || m_btnMgr.selected(m_configBtnM[1])) // sound vol
			{
				s8 direction = m_btnMgr.selected(m_configBtnP[1]) ? 1 : -1;
				cur_music_vol = min(max(0, cur_music_vol + (step * direction)), 255);
				m_cfg.setInt(general_domain, "sound_volume_music", cur_music_vol);
				MusicPlayer.SetMaxVolume(cur_music_vol);
				_showConfigMusic(true);
			}
			else if(m_btnMgr.selected(m_configBtnP[2]) || m_btnMgr.selected(m_configBtnM[2])) // GUI vol
			{
				s8 direction = m_btnMgr.selected(m_configBtnP[2]) ? 1 : -1;
				int val = m_cfg.getInt(general_domain, "sound_volume_gui", 255) + (step * direction);
				if(val >= 0 && val <= 255)
				{
					m_btnMgr.setSoundVolume(val);
					m_cfg.setInt(general_domain, "sound_volume_gui", val);
				}
				_showConfigMusic(true);
			}
			else if(m_btnMgr.selected(m_configBtnP[3]) || m_btnMgr.selected(m_configBtnM[3])) // CF vol
			{
				s8 direction = m_btnMgr.selected(m_configBtnP[3]) ? 1 : -1;
				int val = m_cfg.getInt(general_domain, "sound_volume_coverflow", 255) + (step * direction);
				if(val >= 0 && val <= 255)
				{
					CoverFlow.setSoundVolume(val);
					m_cfg.setInt(general_domain, "sound_volume_coverflow", val);
				}
				_showConfigMusic(true);
			}
			else if(m_btnMgr.selected(m_configBtnP[4]) || m_btnMgr.selected(m_configBtnM[4])) // bnr vol
			{
				s8 direction = m_btnMgr.selected(m_configBtnP[4]) ? 1 : -1;
				int val = m_cfg.getInt(general_domain, "sound_volume_bnr", 255) + (step * direction);
				if(val >= 0 && val <= 255)
				{
					m_bnrSndVol = val;
					m_cfg.setInt(general_domain, "sound_volume_bnr", val);
				}
				_showConfigMusic(true);
			}
		}
		if(BTN_A_OR_2_PRESSED)
		{
			if(m_btnMgr.selected(m_configBtnBack))
				break;
			else if(m_btnMgr.selected(m_configBtnGo[5])) // path
			{
				_hideConfig(true);
				const char *path = NULL;
				path = _FolderExplorer(m_cfg.getString(general_domain, "dir_music").c_str());
				if(strlen(path) > 0)
				{
					m_cfg.setString(general_domain, "dir_music", path);
					m_musicDir = path;
					MusicPlayer.Init(m_cfg, m_musicDir, fmt("%s/music", m_themeDataDir.c_str()));
					// m_music_info = m_cfg.getBool(general_domain, "display_music_info", false);
				}
				_showConfigMusic();
			}
			else if(m_btnMgr.selected(m_checkboxBtn[6])) // randomize music
			{
				cur_rand_music = !cur_rand_music;
				m_cfg.setBool(general_domain, "randomize_music", cur_rand_music);
				_showConfigMusic(true);
				m_btnMgr.setSelected(m_checkboxBtn[6]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[7])) // display music title
			{
				m_music_info = !m_music_info;
				m_cfg.setBool(general_domain, "display_music_info", m_music_info);
				_showConfigMusic(true);
				m_btnMgr.setSelected(m_checkboxBtn[7]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[8])) // upsample music to 48khz
			{
				bool val = !m_cfg.getBool(general_domain, "resample_to_48khz");
				m_cfg.setBool(general_domain, "resample_to_48khz", val);
				MusicPlayer.SetResampleSetting(val);
				if(!MusicPlayer.IsStopped())
				{
					MusicPlayer.Stop();
					MusicPlayer.LoadCurrentFile();
				}
				_showConfigMusic(true);
				m_btnMgr.setSelected(m_checkboxBtn[8]);
			}
		}
	}
	
	if(prev_rand_music != cur_rand_music || prev_music_vol != cur_music_vol)
		MusicPlayer.Init(m_cfg, m_musicDir, fmt("%s/music", m_themeDataDir.c_str()));
	
	_hideConfig(true);
}