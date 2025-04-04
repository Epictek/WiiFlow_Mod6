
// Buttons

#ifndef __GUI_HPP
#define __GUI_HPP

#include <ogc/pad.h>
#include "wiiuse/wpad.h"
#include "wupc/wupc.h"

#include "video.hpp"
#include "FreeTypeGX.h"
#include "text.hpp"
#include "music/gui_sound.h"
#include "wstringEx/wstringEx.hpp"

struct SButtonTextureSet
{
	TexData left;
	TexData right;
	TexData center;
	TexData leftSel;
	TexData rightSel;
	TexData centerSel;
};

class CButtonsMgr
{
public:
	bool init();
	void setRumble(bool enabled) { m_rumbleEnabled = enabled; }
	void reserve(u32 capacity) { m_elts.reserve(capacity); }
	s16 addButton(SFont font, const wstringEx &text, int x, int y, u32 width, u32 height, const CColor &color,
		const SButtonTextureSet &texSet, GuiSound *clickSound = NULL, GuiSound *hoverSound = NULL);
	s16 addLabel(SFont font, const wstringEx &text, int x, int y, u32 width, u32 height, const CColor &color, 
		s16 style, const TexData &bg = _noTexture);
	s16 addPicButton(const u8 *pngNormal, const u8 *pngSelected, int x, int y, u32 width, u32 height,
		GuiSound *clickSound = NULL, GuiSound *hoverSound = NULL);
	s16 addPicButton(TexData &texNormal, TexData &texSelected, int x, int y, u32 width, u32 height,
		GuiSound *clickSound = NULL, GuiSound *hoverSound = NULL);
	s16 addProgressBar(int x, int y, u32 width, u32 height, SButtonTextureSet &texSet);
	void setText(s16 id, const wstringEx &text, bool unwrap = false);
	void setText(s16 id, const wstringEx &text, u32 startline, bool unwrap = false);
	void setBtnTexture(s16 id, TexData &texNormal, TexData &texSelected);
	void freeBtnTexture(s16 id);
	void setTexture(s16 id ,TexData &bg);
	void setTexture(s16 id, TexData &bg, int width, int height);
	void setTexture(s16 id, TexData &bg, int x_pos, int y_pos, int width, int height);
	void setProgress(s16 id, float f, bool instant = false);
	void reset(s16 id, bool instant = false);
	void moveBy(s16 id, int x, int y, bool instant = false);
	void getTotalHeight(s16 id, int &height);
	void getDimensions(s16 id, int &x, int &y, u32 &width, u32 &height);
	void hide(s16 id, int dx, int dy, float scaleX, float scaleY, bool instant = false);
	void hide(s16 id, bool instant = false);
	void show(s16 id, bool instant = false);
	void mouse(int chan, int x, int y, bool enlargeButtons = false);
	void setMouse(bool enable);
	void up(bool enlargeButtons = false);
	void down(bool enlargeButtons = false);
	void draw(void);
	void tick(void);
	void noClick(bool noclick = false);
	// void noHover(bool nohover = false);
	void click(s16 id = -1);
	bool selected(s16 button = -1);
	void setSelected(s16 button);
	void setRumble(int chan, bool wii = false, bool gc = false, bool wupc = false);
	void deselect(void){ for(int chan = WPAD_MAX_WIIMOTES-1; chan >= 0; chan--) m_selected[chan] = -1; }
	void stopSounds(void);
	void setSoundVolume(int vol);
private:
	struct SHideParam
	{
		int dx;
		int dy;
		float scaleX;
		float scaleY;
	public:
		SHideParam(void) : dx(0), dy(0), scaleX(1.f), scaleY(1.f) { }
	};
	enum EltType {
		GUIELT_BUTTON,
		GUIELT_LABEL,
		GUIELT_PROGRESS
	};
	struct SElement
	{
		SHideParam hideParam;
		EltType t;
		bool visible;
		int x; // x & y center of element (x + (width/2))
		int y;
		int w;
		int h;
		Vector3D pos; // actual current position (x,y,z) as it moves from hide x,y to targetpos or vice versa
		Vector3D targetPos; // position to move to (x,y,z) usually hide position or x,y of element
		u8 alpha;
		u8 targetAlpha;
		float scaleX;
		float scaleY;
		float targetScaleX;
		float targetScaleY;
		int moveByX; // keeps track of how much element has moved so when reset function is called x and y can be reset.
		int moveByY;
	public:
		virtual ~SElement(void) { }
		virtual void tick(void);
	protected:
		SElement(void) { }
	};
	struct SButton : public SElement
	{
		SFont font;
		SButtonTextureSet tex;
		CText text;
		CColor textColor;
		float click;
		GuiSound *clickSound;
		GuiSound *hoverSound;
	public:
		SButton(void) { t = GUIELT_BUTTON; }
		virtual void tick(void);
	};
	struct SLabel : public SElement
	{
		SFont font;
		CText text;
		CColor textColor;
		u16 textStyle;
		TexData texBg;
	public:
		SLabel(void) { t = GUIELT_LABEL; }
		virtual void tick(void);
	};
	struct SProgressBar : public SElement
	{
		SButtonTextureSet tex;
		float val;
		float targetVal;
	public:
		SProgressBar(void) { t = GUIELT_PROGRESS; }
		virtual void tick(void);
	};
private:
	vector<SElement*> m_elts;
	s32 m_selected[WPAD_MAX_WIIMOTES];
	bool m_rumbleEnabled;
	u8 m_rumble[WPAD_MAX_WIIMOTES];
	bool wii_rumble[WPAD_MAX_WIIMOTES];
	bool gc_rumble[WPAD_MAX_WIIMOTES];
	bool wupc_rumble[WPAD_MAX_WIIMOTES];
	GuiSound *m_sndHover;
	GuiSound *m_sndClick;
	u8 m_soundVolume;
	bool m_noclick;
	// bool m_nohover;
	bool m_mouse;
private:
	void _drawBtn(SButton &b, bool selected, bool click);
	void _drawLbl(SLabel &b);
	void _drawPBar(const SProgressBar &b);
	static TexData _noTexture;
};

extern CButtonsMgr m_btnMgr;

#endif // !defined(__GUI_HPP)
