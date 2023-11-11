
#include "menu.hpp"

bool CMenu::_error(const wstringEx &msg, bool dialog)
{
	SetupInput();
	
	_hideWaitMessage();
	_hideMain();
	_hideGame();
	_hideCFTheme();
	_hideCheatSettings();
	//! instant hide of m_configBtnCenter
	_hideExplorer(true);
	_hideConfigFull(true);
	
	m_btnMgr.setText(m_configLblDialog, msg); // false = wrap
	_showError();
	if(dialog)
		m_btnMgr.show(m_configBtnBack);

	gprintf("Error msg: %s\n", msg.toUTF8().c_str());

	m_btnMgr.down(); // btn OK selected

	bool choice = false;
	while(!m_exit)
	{
		_mainLoopCommon();
		if(BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED)
			break;
		else if(BTN_A_OR_2_PRESSED)
		{
			if(m_btnMgr.selected(m_configBtnBack))
				break;
			else if(m_btnMgr.selected(m_configBtnCenter))
			{
				choice = true;
				break;
			}
		}
	}
	_hideConfigFull(true);
	return choice;
}

void CMenu::_showError(void)
{
	m_btnMgr.setText(m_configBtnCenter,  _t("ok", L"Ok"));
	m_btnMgr.show(m_configBtnCenter);
	m_btnMgr.show(m_configLblDialog);
	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);
}
