
#include "menu.hpp"

#define REPEATDELAY 3 // used to slow down button highlighting when up or down is held

s16 m_keyboardBtnKey[42];
s16 m_keyboardLblUser[4];
char text[17];

void CMenu::_hideKeyboard(bool instant)
{
	for(u8 i = 0; i < 42; ++i)
		m_btnMgr.hide(m_keyboardBtnKey[i], instant);
	m_btnMgr.hide(m_configBtnBack, instant);
	m_btnMgr.hide(m_configBtnCenter, instant);
	m_btnMgr.hide(m_configLblTitle, instant);
	for(u8 i = 0; i < ARRAY_SIZE(m_keyboardLblUser); ++i)
		if(m_keyboardLblUser[i] != -1)
			m_btnMgr.hide(m_keyboardLblUser[i], instant);
}

void CMenu::_showKeyboard(void)
{
	for(u8 i = 0; i < 42; ++i)
		m_btnMgr.show(m_keyboardBtnKey[i]);
	m_btnMgr.show(m_configBtnBack);
	m_btnMgr.show(m_configLblTitle);
	for(u8 i = 0; i < ARRAY_SIZE(m_keyboardLblUser); ++i)
		if(m_keyboardLblUser[i] != -1)
			m_btnMgr.show(m_keyboardLblUser[i]);
}

static const char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -',.<"; // 42 buttons

char *CMenu::_keyboard(bool search)
{
	u8 repeatDelay = 0;
	u8 n = 0;
	wchar_t textLbl[] = L"_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _";
	
	m_btnMgr.setText(m_configLblTitle, textLbl);
	SetupInput();
	_showKeyboard();
	if(!search)
	{
		m_btnMgr.setText(m_configBtnCenter, _t("ok", L"Ok"));
		m_btnMgr.show(m_configBtnCenter);
	}
	
	while(!m_exit)
	{
		_mainLoopCommon();
		if(BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED)
		{
			text[n * search] = '\0'; // same as button ok if search else cancel
			break;
		}
		else if(BTN_LEFT_REV_REPEAT || BTN_UP_REPEAT)
		{
			if(repeatDelay == REPEATDELAY)
			{
				repeatDelay = 0;
				m_btnMgr.up();
			}
			else
				repeatDelay++;
		}
		else if(BTN_RIGHT_REV_REPEAT || BTN_DOWN_REPEAT)
		{
			if(repeatDelay == REPEATDELAY)
			{
				repeatDelay = 0;
				m_btnMgr.down();
			}
			else
				repeatDelay++;
		}
		else if(BTN_A_OR_2_PRESSED)
		{
			if(m_btnMgr.selected(m_configBtnBack))
			{
				text[n * search] = '\0'; // same as button ok if search else cancel
				break;
			}
			else if(m_btnMgr.selected(m_configBtnCenter))
			{
				text[n] = '\0';
				break;
			}
			else if(m_btnMgr.selected(m_keyboardBtnKey[41]) && n > 0) // "<" backspace
			{
				n--;
				textLbl[n * 2] = '_';
				m_btnMgr.setText(m_configLblTitle, textLbl);
			}
			else
			{
				for(u8 i = 0; i < 41; ++i) // not including "<"
				{
					if(m_btnMgr.selected(m_keyboardBtnKey[i]) && n < 16)
					{
						text[n] = alphabet[i];
						if(text[n] == ' ')
							textLbl[n * 2] = '_';
						else
							textLbl[n * 2] = text[n];
						m_btnMgr.setText(m_configLblTitle, textLbl);
						n++;
						break;
					}
				}
			}
		}
		else
			repeatDelay = REPEATDELAY;
	}
	_hideKeyboard();
	
	return text;
}

void CMenu::_initKeyboardMenu()
{
	_addUserLabels(m_keyboardLblUser, ARRAY_SIZE(m_keyboardLblUser), "KEYBOARD");

	for(int i = 0; i < 42; ++i)
	{
		char *keyboardText = fmt_malloc("KEYBOARD/%i_BTN", i);
		if(keyboardText == NULL) 
			continue;

		int x = i;
		int y = x / 6;
		x %= 6;
		x = 32 + x * 100;
		y = 86 + y * 44;
		m_keyboardBtnKey[i] = _addButton(keyboardText, theme.btnFont, wfmt(L"%c", alphabet[i]), x, y, 80, 34, theme.btnFontColor);

		_setHideAnim(m_keyboardBtnKey[i], keyboardText, 0, 0, 0.f, 0.f);
		MEM2_free(keyboardText);
	}
	// _hideKeyboard();
}
