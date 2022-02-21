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

protected:
	int m_X;
	int m_Y;
	int m_Width;
	int m_Height;
	bool m_bBorder;

	olc::PixelGameEngine& m_Context;
};

class WLabel : public Window
{
public:
	WLabel() = delete;
	WLabel(olc::PixelGameEngine& context, int x, int y, int w, int h, bool bBorder, std::string_view label, olc::Pixel fgCol, olc::Pixel bgCol)
		: Window(context, x, y, w, h, bBorder)
		,m_Label(label)
		,m_FgCol(fgCol)
		,m_BgCol(bgCol)
	{
	}
	WLabel(const WLabel&) = delete;
	WLabel& operator=(const WLabel&) = delete;
	WLabel(WLabel&&) = default;
	WLabel& operator=(WLabel&&) = default;
	virtual ~WLabel() = default;

	void draw() override;
private:
	std::string m_Label;
	olc::Pixel m_FgCol;
	olc::Pixel m_BgCol;
};