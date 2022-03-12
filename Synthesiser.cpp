/*
OneLoneCoder.com - Simple Audio Noisy Thing
"Allows you to simply listen to that waveform!" - @Javidx9

License
~~~~~~~
Copyright (C) 2018  Javidx9
This program comes with ABSOLUTELY NO WARRANTY.
This is free software, and you are welcome to redistribute it
under certain conditions; See license for details. 
Original works located at:
https://www.github.com/onelonecoder
https://www.onelonecoder.com
https://www.youtube.com/javidx9

GNU GPLv3
https://github.com/OneLoneCoder/videos/blob/master/LICENSE

From Javidx9 :)
~~~~~~~~~~~~~~~
Hello! Ultimately I don't care what you use this for. It's intended to be 
educational, and perhaps to the oddly minded - a little bit of fun. 
Please hack this, change it and use it in any way you see fit. You acknowledge 
that I am not responsible for anything bad that happens as a result of 
your actions. However this code is protected by GNU GPLv3, see the license in the
github repo. This means you must attribute me if you use it. You can view this
license here: https://github.com/OneLoneCoder/videos/blob/master/LICENSE
Cheers!

Author
~~~~~~

Twitter: @javidx9
Blog: www.onelonecoder.com

Versions
~~~~~~~~
main4.cpp
Adjusted the definition of a note, and added a sequencer.
See Video: https://youtu.be/roRH3PdTajs


main3a.cpp
This adds polyphony, frequency modulation and instruments.
See Video: https://youtu.be/kDuvruJTjOs


main2.cpp
This version expands on oscillators to include other waveforms
and introduces envelopes
See Video: https://youtu.be/OSCzKOqtgcA

main1.cpp
This is the first version of the software. It presents a simple
keyboard and a sine wave oscillator.
See video: https://youtu.be/tgamhuQnOkM

*/

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

using FTYPE = double;
#include "olcNoiseMaker.h"

#include "Synth.h"
#include "UI.h"

#include <list>
#include <iostream>
#include <algorithm>
#include <assert.h>

namespace
{
	std::vector<Synth::NoteInstrumentPtr> vecNotes;
	std::mutex muxNotes;
	//Synth::Instrument_bell instBell;
	Synth::Instrument_harmonica instHarm;
	Synth::Instrument_drumkick instKick;
	Synth::Instrument_drumsnare instSnare;
	Synth::Instrument_drumhihat instHiHat;

	typedef bool(*lambda)(const Synth::NoteInstrumentPtr& item);
	template<class T>
	void safe_remove(T& v, lambda f)
	{
		auto n = v.begin();
		while (n != v.end())
		{
			if (!f(*n))
				n = v.erase(n);
			else
				++n;
		}
	}

	// Function used by olcNoiseMaker to generate sound waves
	// Returns amplitude (-1.0 to +1.0) as a function of time
	FTYPE MakeNoise(int /*nChannel*/, FTYPE dTime)
	{
		std::lock_guard  lock(muxNotes);
		FTYPE dMixedOutput = 0.0;

		// Iterate through all active notes, and mix together
		for (auto& [n, c] : vecNotes)
		{
			bool bNoteFinished = false;
			FTYPE dSound = 0;

			// Get sample for this note by using the correct instrument and envelope
			if (c != nullptr)
				dSound = c->sound(dTime, n, bNoteFinished) * n.velocity;

			// Mix into output
			dMixedOutput += dSound;

			if (bNoteFinished) // Flag note to be removed
				n.active = false;
		}
		// Remove notes which are now inactive
		safe_remove(vecNotes, [](const Synth::NoteInstrumentPtr& item) { return item.m_Note.active; });
		return dMixedOutput * 0.2;
	}
}

class Synthesiser : public olc::PixelGameEngine
{
public:
	Synthesiser()
		: sequencer(60.0f, 4, 4)
	{
		// Name your application
		sAppName = "Synth";
	}

public:
	bool OnUserCreate() override;
	bool OnUserUpdate(float fElapsedTime) override;
private:
	// call backs
	static void toggleMuteAll(Window* pWnd,void* pThis, void* pParam);
	static void toggleMuteChannel(Window* pWnd, void* pThis, void* pParam);
	static olc::vi2d w2s(int x, int y);
	static constexpr int m_startX = 20;
	static constexpr int m_startY = 20;
	static constexpr int m_rowHeight = 13;

	double dWallTime = 0.0;

	std::vector<std::string> devices;

	olcNoiseMaker<short> sound;

	Synth::Sequencer sequencer;

	int m_Frames = 0;
	FTYPE m_Start = 0;
	int m_FPS = 0;
	std::vector<Synth::CustomInstrument> customInstruments;

	Synth::Instrument* pKeyboardInstrument;

	std::vector<std::unique_ptr<Window>> m_Windows;
	std::vector<WLevels*> m_Levels;

	std::unique_ptr<olc::Sprite> m_pPencilIcon;
};

/* static */ void Synthesiser::toggleMuteAll(Window* pWnd, void* pThis, void* /*pParam*/)
{
	if (!pThis)
		return;
	auto pSynth = reinterpret_cast<Synthesiser*>(pThis);
	pSynth->sequencer.bMuted = !pSynth->sequencer.bMuted;
	if (pWnd)
	{
		auto pButton = static_cast<WButton*>(pWnd);
		if (pSynth->sequencer.bMuted)
			pButton->setFgCol(olc::GREY);
		else
			pButton->setFgCol(olc::WHITE);
	}
	for (auto pLevel : pSynth->m_Levels)
		pLevel->setGlobalMute(pSynth->sequencer.bMuted);
}

/* static */ void Synthesiser::toggleMuteChannel(Window* pWnd, void* pThis, void* pParam)
{
	if (!pThis)
		return;
	if (!pParam)
		return;

	auto pSynth = reinterpret_cast<Synthesiser*>(pThis);
	auto pName = reinterpret_cast<const std::string*>(pParam);
	for (size_t i=0; i< pSynth->sequencer.vecChannel.size(); ++i)
	{
		auto& ch = pSynth->sequencer.vecChannel[i];
		if (ch.instrument && ch.instrument->name == *pName)
		{
			ch.bMuted = !ch.bMuted;
			if (pWnd)
			{
				auto pButton = static_cast<WButton*>(pWnd);
				if (ch.bMuted)
					pButton->setFgCol(olc::GREY);
				else
					pButton->setFgCol(olc::WHITE);
			}
			pSynth->m_Levels[i]->setLocalMute(ch.bMuted);
			break;
		}
	}
}

olc::vi2d Synthesiser::w2s(int x, int y)
{
	return { m_startX + x * 8, m_startY + y * m_rowHeight };
};

bool Synthesiser::OnUserCreate()
{
	m_pPencilIcon = std::make_unique<olc::Sprite>("pencil-icon.png");
	customInstruments = Synth::loadInstruments();
	if (customInstruments.empty())
		pKeyboardInstrument = &instHarm;
	else
		pKeyboardInstrument = &customInstruments[0];

	// Get all sound hardware
	devices = olcNoiseMaker<short>::Enumerate();

	// Create sound machine!!
	if (!sound.Create(devices[0], 44100, 1, 8, 256))
	{
		std::cerr << "sound.Create failed for device " << devices[0] << std::endl;
		return false;
	}

	// Link noise function with sound machine
	sound.SetUserFunction(MakeNoise);

	// Establish Sequencer
	auto kick = sequencer.AddInstrument(&instKick);
	auto snare = sequencer.AddInstrument(&instSnare);
	auto hh = sequencer.AddInstrument(&instHiHat);

	auto stringToIntArray = [](const std::string_view str)
	{
		std::vector<int> ar;
		ar.reserve(str.size());
		auto intForChar = [](const char ch)
		{
			switch (ch)
			{
			case '_':
				return 2;
			case '-':
				return 4;
			case '^':
				return 6;
			case '#':
				return 8;
			}
			return 0;
		};

		for (auto ch : str)
			ar.push_back(intForChar(ch));
		return ar;
	};
	
	sequencer.vecChannel[kick ].sBeat = stringToIntArray("^...^...^..^.^..");
	sequencer.vecChannel[snare].sBeat = stringToIntArray("..#...#...#...#.");
	sequencer.vecChannel[hh   ].sBeat = stringToIntArray("^.-.^.-.^._.^._^");

	auto row = 0;
	{
		const auto x = m_startX;
		const auto y = m_startY + (row * m_rowHeight);
		auto pSequencer = std::make_unique<WButton>(*this, x, y, 100, m_rowHeight, false, "Sequencer", olc::WHITE, olc::DARK_BLUE);
		pSequencer->setMouseLeftButtonReleasedCallback(Synthesiser::toggleMuteAll, this, nullptr);
		m_Windows.push_back(std::move(pSequencer));
	}
	++row;

	for (auto& v : sequencer.vecChannel)
	{
		const auto x = m_startX;
		const auto y = m_startY + (row * m_rowHeight);

		auto pChannel = std::make_unique<WButton>(*this, x, y, 100, m_rowHeight, false, v.instrument->name, olc::WHITE, olc::DARK_BLUE);
		pChannel->setMouseLeftButtonReleasedCallback(Synthesiser::toggleMuteChannel, this, &v.instrument->name);
		m_Windows.push_back(std::move(pChannel));

		auto pLevels = std::make_unique<WLevels>(*this, x+112, y, 8, m_rowHeight, false, v.sBeat, olc::VERY_DARK_GREY, olc::DARK_GREY, olc::GREY, olc::WHITE, olc::BLUE, olc::DARK_BLUE);
		m_Levels.push_back(pLevels.get());
		m_Windows.push_back(std::move(pLevels));
		
		++row;
	}

	// at the end
	m_Start = sound.GetTime();

	return true;
}

namespace
{
	template <typename T>
	void log(T&& t)
	{
		std::cout << t << "\n";
	}
	template <typename T1, typename T2>
	void log(T1&& t1, T2&& t2)
	{
		std::cout << t1 << " " << t2 << "\n";
	}
}

bool Synthesiser::OnUserUpdate(float fElapsedTime)
{
	// --- SOUND STUFF ---

	dWallTime += fElapsedTime;
	FTYPE dTimeNow = sound.GetTime();

	// ui events
	{
		const auto mPos = GetMousePos();
		const auto lButtonDn = GetMouse(0).bPressed;
		const auto lButtonUp = GetMouse(0).bReleased;
		const auto lButtonHeld = GetMouse(0).bHeld;
		for (auto& w : m_Windows)
		{
			if (w->mousePos(mPos.x, mPos.y))
			{
				w->mouseLeftButtonPressed(lButtonDn);
				w->mouseLeftButtonReleased(lButtonUp);
				w->mouseLeftButtonHeld(lButtonHeld);
			}
			else
			{
				w->mouseLeftButtonPressed(false);
				w->mouseLeftButtonReleased(false);
				w->mouseLeftButtonHeld(false);
			}
		}
	}
	// sequenceruencer (generates notes, note offs applied by note lifespan) ======================================
	sequencer.Update(fElapsedTime);
	{
		std::lock_guard  lock(muxNotes);
		for (auto& note : sequencer.vecNotes)
		{
			note.m_Note.on = dTimeNow;
			vecNotes.emplace_back(note);
		}
	}

	// Keyboard (generates and removes notes depending on key state) ========================================
	// Note : olc::OEM_2 is the the /? key
	constexpr auto Keyboard = std::to_array({ olc::Z, olc::S, olc::X, olc::C, olc::F, olc::V, olc::G, olc::B, olc::N, olc::J, olc::M, olc::K, olc::COMMA, olc::L, olc::PERIOD, olc::OEM_2 });

	for (int k = 0; k < static_cast<int>(Keyboard.size()); ++k)
	{
		const auto key = GetKey(Keyboard[k]);

		// Check if note already exists in currently playing notes
		{
			std::lock_guard  lock(muxNotes);
			auto noteFound = find_if(vecNotes.begin(), vecNotes.end(), [&k, pKeyboardInstrument = pKeyboardInstrument](const Synth::NoteInstrumentPtr& item) { return (item.m_Note.id == k + Synth::BaseNoteID) && (item.m_pInstrument == pKeyboardInstrument); });
			if (noteFound == vecNotes.end())
			{
				// Note not found in vector
				if (key.bHeld)
				{
					// Key has been pressed so create a new note
					Synth::Note note;
					note.id = k + Synth::BaseNoteID;
					note.on = dTimeNow;
					note.active = true;

					// Add note to vector
					vecNotes.emplace_back(note, pKeyboardInstrument);
				}
			}
			else
			{
				// Note exists in vector
				if (key.bHeld)
				{
					// Key is still held, so do nothing
					if (noteFound->m_Note.off > noteFound->m_Note.on)
					{
						// Key has been pressed again during release phase
						noteFound->m_Note.on = dTimeNow;
						noteFound->m_Note.active = true;
					}
				}
				else
				{
					// Key has been released, so switch off
					if (noteFound->m_Note.off < noteFound->m_Note.on)
						noteFound->m_Note.off = dTimeNow;
				}
			}
		}
	}

	// --- VISUAL STUFF ---

	Clear(olc::BLACK);

	// Draw Sequencer

	constexpr int colx1 = 0;
	constexpr int colx2 = 14;
	int row = 0;
	for (int beats = 0; beats < sequencer.nBeats; ++beats)
	{
		{
			auto xy = w2s(beats * sequencer.nSubBeats + colx2, row);
			xy.y += 3;
			DrawString(xy, "O");
		}
		for (int subbeats = 1; subbeats < sequencer.nSubBeats; ++subbeats)
		{
			auto xy = w2s((beats * sequencer.nSubBeats) + subbeats + colx2, row);
			xy.y += 3;
			DrawString(xy, ".");
		}
	}
	{
		// Draw Beat Cursor
		auto xy = w2s(colx2 + sequencer.nCurrentBeat, 0);
		xy.y = xy.y - m_rowHeight + 3;
		DrawString(xy, "|");
	}

	// Draw Keyboard
	row += static_cast<int>(sequencer.vecChannel.size()+1);
	DrawString(w2s(colx1, ++row), "|   |   |   |   |   | |   |   |   |   | |   | |   |   |   |  ");
	DrawString(w2s(colx1, ++row), "|   | S |   |   | F | | G |   |   | J | | K | | L |   |   |  ");
	DrawString(w2s(colx1, ++row), "|   |___|   |   |___| |___|   |   |___| |___| |___|   |   |__");
	DrawString(w2s(colx1, ++row), "|     |     |     |     |     |     |     |     |     |     |");
	DrawString(w2s(colx1, ++row), "|  Z  |  X  |  C  |  V  |  B  |  N  |  M  |  ,  |  .  |  /  |");
	DrawString(w2s(colx1, ++row), "|_____|_____|_____|_____|_____|_____|_____|_____|_____|_____|");

	// Draw Stats
	++m_Frames;
	if ((dTimeNow - m_Start) > 1)
	{
		m_FPS = static_cast<int>(float(m_Frames) / (dTimeNow - m_Start));
		m_Frames = 0;
		m_Start = dTimeNow;
	}
	std::string stats = "Notes: " + std::to_string(vecNotes.size()) + " Wall Time: " + std::to_string(dWallTime) + " CPU Time: " + std::to_string(dTimeNow) + " Latency: " + std::to_string(dWallTime - dTimeNow) + (m_FPS ? " FPS: " : "") + (m_FPS ? std::to_string(m_FPS) : std::string());
	DrawString(w2s(colx1, ++row), stats);


	for (auto& w : m_Windows)
		w->draw();

	FillRect(140, 140, 100, 100, olc::RED);
	DrawSprite(150, 150, m_pPencilIcon.get());

	return true;
}

int main()
{
	// Shameless self-promotion
	std::cout << "www.OneLoneCoder.com - Synthesizer Part 4" << std::endl 
		      << "Multiple FM Oscillators, Sequencing, Polyphony" << std::endl << std::endl;

	Synthesiser synth;
	if (synth.Construct(700, 400, 2, 2))
		synth.Start();
	return 0;
}
