#pragma once

#include "UI.h"

void WLabel::draw()
{
	// make the corners rounded, and do 3d (ie dark top and left, light bottom and right (or the other way around?)
	// really the height should be fixed to the text height, and the width to the text width
	if (m_bBorder)
	{
		m_Context.DrawRect(m_X, m_Y, m_Width, m_Height, olc::BLACK);
		m_Context.FillRect(m_X + 2, m_Y + 2, m_Width - 2, m_Height - 2, m_BgCol);
		m_Context.DrawString(m_X + 4, m_Y + 4, m_Label, m_FgCol);
	}
	else
	{
		m_Context.DrawString(m_X, m_Y, m_Label, m_FgCol);
	}
}
