#pragma once

#include "UI.h"

void Window::setMouseOverCallback(const CallBackFn& callBackFn, void* pThis, void* pParam)
{
	if (!m_pOnMouseOver)
		m_pOnMouseOver = std::make_unique<CallBack>(CallBack{ callBackFn, pThis, pParam });
	else
		*m_pOnMouseOver = CallBack{ callBackFn , pThis, pParam };
}

void Window::setMouseLeftButtonPressedCallBack(const CallBackFn& callBackFn, void* pThis, void* pParam)
{
	if (!m_pOnMouseLeftButtonPressed)
		m_pOnMouseLeftButtonPressed = std::make_unique<CallBack>(CallBack{ callBackFn, pThis, pParam });
	else
		*m_pOnMouseLeftButtonPressed = CallBack{ callBackFn , pThis, pParam };
}

void Window::setMouseLeftButtonReleasedCallback(const CallBackFn& callBackFn, void* pThis, void* pParam)
{
	if (!m_pOnMouseLeftButtonReleased)
		m_pOnMouseLeftButtonReleased = std::make_unique<CallBack>(CallBack{ callBackFn, pThis, pParam });
	else
		*m_pOnMouseLeftButtonReleased = CallBack{ callBackFn , pThis, pParam };
}

void Window::setMouseLeftButtonHeldCallback(const CallBackFn& callBackFn, void* pThis, void* pParam)
{
	if (!m_pOnMouseLeftButtonHeld)
		m_pOnMouseLeftButtonHeld = std::make_unique<CallBack>(CallBack{ callBackFn, pThis, pParam });
	else
		*m_pOnMouseLeftButtonHeld = CallBack{ callBackFn , pThis, pParam };
}

bool Window::mousePos(int x, int y)
{
	bool b;
	if (x <= m_X)
		b = false;
	else if (y <= m_Y)
		b = false;
	else if (x >= m_X + m_Width)
		b = false;
	else if (y >= m_Y + m_Height)
		b = false;
	else
		b = true;
	m_bMouseOver = b;
	if (m_bMouseOver)
	{
		m_MouseX = x;
		m_MouseY = y;
	}
	else
	{
		m_MouseX = -1;
		m_MouseY = -1;
	}
	if (m_pOnMouseOver)
		m_pOnMouseOver->callback(this);
	return m_bMouseOver;
}

void Window::mouseLeftButtonPressed(bool b)
{
	if (m_bMouseLeftButtonPressed != b)
	{
		m_bMouseLeftButtonPressed = b;
		if (m_bMouseLeftButtonPressed && m_pOnMouseLeftButtonPressed)
			m_pOnMouseLeftButtonPressed->callback(this);
	}
	mouseLeftButtonReleased();

}

void Window::mouseLeftButtonReleased(bool b)
{
	if (m_bMouseLeftButtonReleased != b)
	{
		m_bMouseLeftButtonReleased = b;
		if (m_bMouseLeftButtonReleased && m_pOnMouseLeftButtonReleased)
			m_pOnMouseLeftButtonReleased->callback(this);
	}
}

void Window::mouseLeftButtonHeld(bool b)
{
	m_bMouseLeftButtonHeld = b;
	if (m_pOnMouseLeftButtonHeld)
		m_pOnMouseLeftButtonHeld->callback(this);
}

void WButton::draw()
{
	// ignoring border option for now
	// if (m_bBorder)
	{
		m_Context.FillRect(m_X, m_Y, m_Width, m_Height, m_bMouseLeftButtonHeld ? m_BgCol : m_FgCol);

		// border
		m_Context.DrawRect(m_X, m_Y, m_Width, m_Height, olc::BLACK);
		// extra border when mouse over
		if (m_bMouseOver)
			m_Context.DrawRect(m_X+1, m_Y+1, m_Width-2, m_Height-2, olc::BLACK);

		m_Context.DrawString(m_X + 3, m_Y + 3, m_Label, m_bMouseLeftButtonHeld ? m_FgCol : m_BgCol, 1);
	}
}

void WLevels::mouseLeftButtonReleased()
{
	if (!m_bMouseLeftButtonReleased)
		return;

	auto clickedCol = (m_MouseX - m_X) / 8;
	auto& ch = (*m_pBeats)[clickedCol];
	ch = ch + 2;
	if (ch > 8)
		ch = 0;
}

void WLevels::draw()
{
	const auto colour = [&]()
	{
		if (m_bGlobalMute)
		{
			if (m_bLocalMute)
				return m_MutedMuted;
			return m_MutedNotMuted;
		}
		if (m_bLocalMute)
			return m_NotMutedMuted;
		return m_NotMutedNotMuted;
	}();

	for (size_t beat = 0; beat<m_pBeats->size(); ++beat)
	{
		auto x = m_X + (m_BeatWidth * static_cast<int>(beat));
		auto y = m_Y + 3;
		auto vol = (*m_pBeats)[beat];

		if (vol == 0)
		{
			m_Context.FillRect(x, y, 6, 8, m_BgOff);
		}
		else if (vol < 8)
		{
			m_Context.FillRect(x, y, 6, 8, m_BgOn);
			m_Context.FillRect(x, y + (8 - vol), 6, vol, colour);
		}
		else
		{
			m_Context.FillRect(x, y, 6, 8, colour);
		}
	}
}