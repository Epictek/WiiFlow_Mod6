
#include "menu.hpp"
#include "network/https.h"
// #include "gui/text.hpp"
// #include "lockMutex.hpp"

// #define GECKOURL "https://www.geckocodes.org/txt.php?txt=%s"
#define GECKOURL "https://codes.rc24.xyz/txt.php?txt=%s"
#define CHEATSPERPAGE 6

static u8 curPage = 0;
int txtavailable;
GCTCheats m_cheatfile;
bool enabled;

s16 m_cheatLblLarge;
s16 m_cheatLblItem[CHEATSPERPAGE];
s16 m_cheatLblUser[4];

int CMenu::_downloadCheatFileAsync(const char *id)
{
	m_thrdTotal = 2; // download and save
	
	m_thrdMessage = _t("dlmsg1", L"Initializing network...");
	m_thrdMessageAdded = true;
	if(_initNetwork() < 0)
		return -2;
	m_thrdMessage = _t("dlmsg11", L"Downloading...");
	m_thrdMessageAdded = true;
	
	struct download file = {};
	string gecko_url = m_cfg.getString(general_domain, "url_geckocodes", GECKOURL);
	downloadfile(fmt(gecko_url.c_str(), id), &file);
	if(file.size > 0 && file.data[0] != '<')
	{
		m_thrdMessage = _t("dlmsg13", L"Saving...");
		m_thrdMessageAdded = true;
		update_pThread(1); // it's downloaded
		fsop_WriteFile(fmt("%s/%s.txt", m_txtCheatDir.c_str(), id), file.data, file.size);
		free(file.data);
		return 0;
	}
	if(file.size > 0) // received a 301/302 redirect instead of a 404?
	{
		free(file.data);
		return -4; // the file doesn't exist on the server
	}
	return -3; // download failed
}

void CMenu::_CheatSettings(const char *id) 
{
	curPage = 1;
	txtavailable = m_cheatfile.openTxtfile(fmt("%s/%s.txt", m_txtCheatDir.c_str(), id));
	u32 gctSize = 0;
	u8 *gctBuf = NULL;
	if(txtavailable > 0)
		gctBuf = fsop_ReadFile(fmt("%s/%s.gct", m_cheatDir.c_str(), id), &gctSize);

	u8 chtsCnt = m_cheatfile.getCnt();
	for(u8 i = 0; i < chtsCnt; ++i)
	{
		if(gctBuf && m_cheatfile.IsCheatIncluded(i, gctBuf, gctSize))
			m_cheatfile.sCheatSelected.push_back(true);
		else
			m_cheatfile.sCheatSelected.push_back(false);
	}
	enabled = m_gcfg2.getBool(id, "cheat", 0);

	SetupInput();
	_showCheatSettings();
	
	while(!m_exit)
	{
		_mainLoopCommon(true);
		if(BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED)
			break;
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
			m_btnMgr.up();
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
			m_btnMgr.down();
		else if(txtavailable && (BTN_MINUS_PRESSED || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnPageM))))
		{
			_hideCheatSettings(true);
			curPage = curPage == 1 ? (m_cheatfile.getCnt() + CHEATSPERPAGE - 1) / CHEATSPERPAGE : curPage - 1;
			if(BTN_MINUS_PRESSED) 
				m_btnMgr.click(m_configBtnPageM);
			_showCheatSettings();
		}
		else if(txtavailable && (BTN_PLUS_PRESSED || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnPageP))))
		{
			_hideCheatSettings(true);
			curPage = curPage == (m_cheatfile.getCnt() + CHEATSPERPAGE - 1) / CHEATSPERPAGE ? 1 : curPage + 1;
			if(BTN_PLUS_PRESSED) 
				m_btnMgr.click(m_configBtnPageP);
			_showCheatSettings();
		}
		else if(BTN_A_OR_2_PRESSED)
		{
			for(u8 i = 0; i < CHEATSPERPAGE; ++i)
			{
				if(m_btnMgr.selected(m_checkboxBtn[i]))
				{
					//! handling code for clicked cheat
					m_cheatfile.sCheatSelected[(curPage - 1) * CHEATSPERPAGE + i] = !m_cheatfile.sCheatSelected[(curPage - 1) * CHEATSPERPAGE + i];
					_showCheatSettings(true);
					m_btnMgr.setSelected(m_checkboxBtn[i]);
				}
				else if(m_btnMgr.selected(m_cheatLblItem[i]))
				{
					m_btnMgr.hide(m_cheatLblLarge, true);
					wstringEx chtName;
					chtName.fromUTF8(m_cheatfile.getCheatName((curPage - 1) * CHEATSPERPAGE + i));
					m_btnMgr.setText(m_cheatLblLarge, chtName);
					m_btnMgr.show(m_cheatLblLarge, true);
				}
			}
			
			if(m_btnMgr.selected(m_configBtnBack))
				break;
			else if(m_btnMgr.selected(m_configBtnCenter)) // enable/disable or download if txt missing
			{
				if(txtavailable) // enable/disable
				{
					enabled = !enabled;
					m_gcfg2.setBool(id, "cheat", enabled);
					break;
				}
				else // download
				{
					_hideConfig(true);
					bool dl_finished = false;
					while(!m_exit)
					{
						_mainLoopCommon();
						if(((BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED) || (BTN_A_OR_2_PRESSED && m_btnMgr.selected(m_configBtnBack))) && dl_finished)
						{
							_hideConfigFull(true);
							break;
						}
						if(!dl_finished)
						{
							m_btnMgr.setProgress(m_wbfsPBar, 0.f, true);
							m_btnMgr.setText(m_wbfsLblMessage, L"0%");
							m_btnMgr.setText(m_configLblDialog, L"");
							m_btnMgr.show(m_wbfsPBar);
							m_btnMgr.show(m_wbfsLblMessage);
							m_btnMgr.show(m_configLblDialog);
							
							_start_pThread();
							int ret = _downloadCheatFileAsync(id);
							_stop_pThread();
							if(ret == -1)
								m_btnMgr.setText(m_configLblDialog, _t("dlmsg27", L"Not enough memory!"));
							else if(ret == -2)
								m_btnMgr.setText(m_configLblDialog, _t("dlmsg2", L"Network initialization failed!"));
							// else if(ret == -3)
								// m_btnMgr.setText(m_configLblDialog, _t("dlmsg12", L"Download failed!"));
							// else if(ret == -4)
							else if(ret <= -3) //
								m_btnMgr.setText(m_configLblDialog, _t("dlmsg36", L"No cheat file available to download."));
							else
								m_btnMgr.setText(m_configLblDialog, _t("dlmsg14", L"Done."));
							dl_finished = true;
							m_btnMgr.show(m_configBtnBack);
						}
					}
					txtavailable = m_cheatfile.openTxtfile(fmt("%s/%s.txt", m_txtCheatDir.c_str(), id));
					if(txtavailable > 0)
						gctBuf = fsop_ReadFile(fmt("%s/%s.gct", m_cheatDir.c_str(), id), &gctSize);

					chtsCnt = m_cheatfile.getCnt();
					for(u8 i = 0; i < chtsCnt; ++i)
					{
						if(gctBuf && m_cheatfile.IsCheatIncluded(i, gctBuf, gctSize))
							m_cheatfile.sCheatSelected.push_back(true);
						else
							m_cheatfile.sCheatSelected.push_back(false);
					}
					//! if txt file empty delete it to allow redownload
					if(txtavailable && m_cheatfile.getCnt() == 0) 
						fsop_deleteFile(fmt("%s/%s.txt", m_txtCheatDir.c_str(), id));
					_showCheatSettings();
				}
			}
		}
	}
	
	if(m_cheatfile.getCnt() > 0)
	{
		bool selected = false;
		//! checks if at least one cheat is selected
		for(unsigned int i = 0; i < m_cheatfile.getCnt(); ++i)
		{
			if(m_cheatfile.sCheatSelected[i] == true) 
			{
				selected = true;
				break;
			}
		}
		if(selected)
		{
			m_cheatfile.createGCT(fmt("%s/%s.gct", m_cheatDir.c_str(), id)); 
			m_gcfg2.setInt(id, "hooktype", m_gcfg2.getInt(id, "hooktype", 1));
		}
		else
		{
			fsop_deleteFile(fmt("%s/%s.gct", m_cheatDir.c_str(), id));
			m_gcfg2.remove(id, "cheat");
			m_gcfg2.remove(id, "hooktype");
		}
	}

	_hideCheatSettings(true);
	_hideConfig(true);
}

void CMenu::_hideCheatSettings(bool instant)
{
	m_btnMgr.setText(m_cheatLblLarge, L"");
	for(u8 i = 0; i < CHEATSPERPAGE; ++i)
	{
		m_btnMgr.hide(m_cheatLblItem[i], instant);
		m_btnMgr.hide(m_configLbl[i], instant);
	}
	
	m_btnMgr.hide(m_cheatLblLarge, instant);
	m_btnMgr.hide(m_configLblDialog, instant);
	
	for(u8 i = 0; i < ARRAY_SIZE(m_cheatLblUser); ++i)
		if(m_cheatLblUser[i] != -1)
			m_btnMgr.hide(m_cheatLblUser[i], instant);
}

void CMenu::_showCheatSettings(bool instant)
{
	u8 chtsCnt = m_cheatfile.getCnt();
	
	_hideCheckboxes(true); // reset checkboxes
	
	m_btnMgr.setText(m_configLblTitle, CoverFlow.getTitle());
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_configBtnBack);
	
	for(u8 i = 0; i < ARRAY_SIZE(m_cheatLblUser); ++i)
		if(m_cheatLblUser[i] != -1)
			m_btnMgr.show(m_cheatLblUser[i]);
	
	if(chtsCnt > 0)
	{
		/* Cheat found, show Enable or Disable */
		m_btnMgr.setText(m_configBtnCenter, enabled ? _t("cfg835", L"Disable") : _t("cfg834", L"Enable"));
		m_btnMgr.show(m_configBtnCenter);
		if(((chtsCnt + CHEATSPERPAGE - 1) / CHEATSPERPAGE) > 1)
		{
			m_btnMgr.setText(m_configLblPage, wfmt(L"%i / %i", curPage, (chtsCnt + CHEATSPERPAGE - 1) / CHEATSPERPAGE));
			m_btnMgr.show(m_configLblPage);
			m_btnMgr.show(m_configBtnPageM);
			m_btnMgr.show(m_configBtnPageP);
		}
		else
		{
			m_btnMgr.hide(m_configLblPage);
			m_btnMgr.hide(m_configBtnPageM);
			m_btnMgr.hide(m_configBtnPageP);
		}
		m_btnMgr.setText(m_cheatLblLarge, _t("cheat98", L"Click on a code to view full description"));
		m_btnMgr.show(m_cheatLblLarge);
		
		/* Show cheats if available, else hide */
		for(u32 i = 0; i < CHEATSPERPAGE; ++i)
		{
			// cheat in range?
			if(((curPage - 1) * CHEATSPERPAGE + i + 1) <= chtsCnt) 
			{
				string chtNameTemp = m_cheatfile.getCheatName((curPage - 1) * CHEATSPERPAGE + i);
				//! display 30 chars only without cheat credits to fit the onscreen array width
				chtNameTemp = chtNameTemp.substr(0, chtNameTemp.find(" ["));
				if(chtNameTemp.size() > 30)
					chtNameTemp = chtNameTemp.substr(0, 30) + "...";
				wstringEx chtName;
				chtName.fromUTF8(chtNameTemp);
				m_btnMgr.setText(m_configLbl[i], chtName);
				
				if(m_cheatfile.sCheatSelected[(curPage - 1) * CHEATSPERPAGE + i])
					m_checkboxBtn[i] = m_configChkOn[i];
				else
					m_checkboxBtn[i] = m_configChkOff[i];
				m_btnMgr.show(m_checkboxBtn[i], instant);
				m_btnMgr.show(m_cheatLblItem[i]);
				m_btnMgr.show(m_configLbl[i]);
			}
			// cheat out of range, hide elements
			else
			{
				m_btnMgr.hide(m_cheatLblItem[i], true);
				m_btnMgr.hide(m_configLbl[i], true);
			}
		}
	}
	else
	{
		/* Cheat not found, show Download */
		m_btnMgr.setText(m_configBtnCenter, _t("cfg4", L"Download"));
		m_btnMgr.show(m_configBtnCenter);
		m_btnMgr.setText(m_configLblDialog, _t("cheat3", L"Cheat file not found"));
		m_btnMgr.show(m_configLblDialog);
	}
}

extern const u8 tex_blank_png[];
extern const u8 tex_list_s_png[];

void CMenu::_initCheatSettingsMenu()
{
	TexData texBtnEntry;
	TexHandle.fromPNG(texBtnEntry, tex_blank_png);
	TexData texBtnEntryS;
	TexHandle.fromPNG(texBtnEntryS, tex_list_s_png);
	
	_addUserLabels(m_cheatLblUser, ARRAY_SIZE(m_cheatLblUser), "CHEAT");
	
	m_cheatLblLarge = _addText("CHEAT/ITEM_LARGE", theme.txtFont, L"", 60, 270, 520, 96, theme.txtFontColor, FTGX_JUSTIFY_CENTER | FTGX_ALIGN_MIDDLE);
	
	_setHideAnim(m_cheatLblLarge, fmt("CHEAT/ITEM_LARGE"), 0, 0, 1.f, -1.f);
	
	for(u8 i = 0; i < CHEATSPERPAGE; ++i) 
	{
		char *cheatText = fmt_malloc("CHEAT/ITEM_%i", i);
		if(cheatText == NULL) 
			continue;
		m_cheatLblItem[i] = _addPicButton(cheatText, texBtnEntry, texBtnEntryS, 60, 80 + (i * 32), 340, 32);
		_setHideAnim(m_cheatLblItem[i], cheatText, 0, 0, 1.f, -1.f);
		MEM2_free(cheatText);
	}
}
