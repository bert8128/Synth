#pragma once

#include <string>
#include <string_view>

#include "olcPixelGameEngine.h"

class Window
{
public:

	Window() = default;
	Window(olc::PixelGameEngine& context, int x, int y, int w, int h, bool bBorder)
		: m_Context(context), m_X(x), m_Y(y), m_Width(w), m_Height(h), m_bBorder(bBorder)
	{
	}
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	Window(Window&&) = default;
	Window& operator=(Window&&) = default;
	virtual ~Window() = default;

	virtual void draw() = 0;

	bool mousePos(int x, int y);
	void mouseLeftButtonPressed(bool b); // also calls mouseLeftButtonPressed()
	void mouseLeftButtonReleased(bool b);
	void mouseLeftButtonHeld(bool b);

	using CallBackFn = std::function<void(Window* pWnd, void* /*pObject*/, void* /*pParam*/)>;
	void setMouseOverCallback(const CallBackFn& callBackFn, void* pThis, void* pParam);
	void setMouseLeftButtonPressedCallBack(const CallBackFn& callBackFn, void* pThis, void* pParam);
	void setMouseLeftButtonReleasedCallback(const CallBackFn& callBackFn, void* pThis, void* pParam);
	void setMouseLeftButtonHeldCallback(const CallBackFn& callBackFn, void* pThis, void* pParam);

protected:
	olc::PixelGameEngine& m_Context;

	int m_X = 0;
	int m_Y = 0;
	int m_Width = 0;
	int m_Height = 0;
	bool m_bBorder = false;
	int m_MouseX = -1;
	int m_MouseY = -1;

	bool m_bMouseOver = false;
	bool m_bMouseLeftButtonPressed = false;
	bool m_bMouseLeftButtonReleased = false;
	bool m_bMouseLeftButtonHeld = false;

	struct CallBack
	{
		CallBackFn m_pCallBackFn;
		void* m_pObject;
		void* m_pParam;
		void callback(Window* pWnd)
		{
			m_pCallBackFn(pWnd, m_pObject, m_pParam);
		}
	};

	std::unique_ptr<CallBack> m_pOnMouseOver = nullptr;
	std::unique_ptr<CallBack> m_pOnMouseLeftButtonPressed = nullptr;
	std::unique_ptr<CallBack> m_pOnMouseLeftButtonReleased = nullptr;
	std::unique_ptr<CallBack> m_pOnMouseLeftButtonHeld = nullptr;

	virtual void mouseLeftButtonReleased() {}
};

class WButton : public Window
{
public:
	WButton() = delete;
	WButton(olc::PixelGameEngine& context, int x, int y, int w, int h, bool bBorder, std::string_view label, olc::Pixel fgCol, olc::Pixel bgCol)
		: Window(context, x, y, w, h, bBorder)
		, m_Label(label)
		, m_FgCol(fgCol)
		, m_BgCol(bgCol)
	{
	}
	WButton(const WButton&) = default;
	WButton& operator=(const WButton&) = default;
	WButton(WButton&&) = default;
	WButton& operator=(WButton&&) = default;
	virtual ~WButton() = default;

	void draw() override;

	void setFgCol(olc::Pixel col) { m_FgCol = col; }
private:
	std::string m_Label;
	olc::Pixel m_FgCol;
	olc::Pixel m_BgCol;
};

class WLevels : public Window
{
public:
	WLevels() = delete;
	WLevels(
		 olc::PixelGameEngine& context, int x, int y, int w, int h, bool bBorder
		,std::vector<int>& beats
		,olc::Pixel mutedMuted
		,olc::Pixel mutedNotMuted
		,olc::Pixel notMutedMuted
		,olc::Pixel notMutedNotMuted
		,olc::Pixel bgOff
		,olc::Pixel bgOn
		)
	: Window(context, x, y, static_cast<int>(w*beats.size()), h, bBorder)
		, m_BeatWidth(w)
		, m_pBeats(&beats)
		, m_MutedMuted(mutedMuted)
		, m_MutedNotMuted(mutedNotMuted)
		, m_NotMutedMuted(notMutedMuted)
		, m_NotMutedNotMuted(notMutedNotMuted)
		, m_BgOff(bgOff)
		, m_BgOn(bgOn)
	{
	}
	WLevels(const WLevels&) = default;
	WLevels& operator=(const WLevels&) = default;
	WLevels(WLevels&&) = default;
	WLevels& operator=(WLevels&&) = default;
	virtual ~WLevels() = default;

	void setGlobalMute(bool b) { m_bGlobalMute = b; }
	void setLocalMute(bool b) { m_bLocalMute = b; }

	void draw() override;

private:
	int m_BeatWidth;
	std::vector<int>* m_pBeats;
	olc::Pixel m_MutedMuted;
	olc::Pixel m_MutedNotMuted;
	olc::Pixel m_NotMutedMuted;
	olc::Pixel m_NotMutedNotMuted;
	olc::Pixel m_BgOff;
	olc::Pixel m_BgOn;
	bool m_bGlobalMute = false;
	bool m_bLocalMute = false;

	void mouseLeftButtonReleased() override;
};

