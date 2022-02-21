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

std::vector<Synth::NoteInstrumentPtr> vecNotes;
std::mutex muxNotes;
//Synth::Instrument_bell instBell;
Synth::Instrument_harmonica instHarm;
Synth::Instrument_drumkick instKick;
Synth::Instrument_drumsnare instSnare;
Synth::Instrument_drumhihat instHiHat;

typedef bool(*lambda)(const Synth::NoteInstrumentPtr& item);
template<class T>
void safe_remove(T &v, lambda f)
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

	std::unique_ptr<olc::Sprite> m_pPencilIcon;
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

	const auto startX = 20;
	const auto startY = 20;
	const auto rowHeight = 10;
	auto row = 0;
	m_Windows.push_back(std::make_unique<WLabel>(*this, startX, startY + (row * rowHeight), 120, rowHeight, false, "Sequencer", olc::WHITE, olc::DARK_BLUE));
	++row;
	for (auto v : sequencer.vecChannel)
	{
		m_Windows.push_back(std::make_unique<WLabel>(*this, startX, startY + (row * rowHeight), 120, rowHeight, false, v.instrument->name, olc::WHITE, olc::DARK_BLUE));
		++row;
	}

	// at the end
	m_Start = sound.GetTime();

	return true;
}

olc::vi2d w2s(int x, int y)
{
	return { x * 8, y * 10 };
};

bool Synthesiser::OnUserUpdate(float fElapsedTime)
{
	// --- SOUND STUFF ---

	dWallTime += fElapsedTime;
	FTYPE dTimeNow = sound.GetTime();

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

	constexpr int colx1 = 2;
	constexpr int colx2 = 16;
	int row = 2;
	const auto seqColour = sequencer.muted()
		? olc::GREY
		: olc::WHITE;
	//DrawString(w2s(colx1, row), "SEQUENCER:", seqColour);
	for (int beats = 0; beats < sequencer.nBeats; ++beats)
	{
		DrawString(w2s(beats * sequencer.nSubBeats + colx2, row), "O");
		for (int subbeats = 1; subbeats < sequencer.nSubBeats; ++subbeats)
			DrawString(w2s((beats * sequencer.nSubBeats) + subbeats + colx2, row), ".");
	}
	// Draw Beat Cursor
	DrawString(w2s(colx2 + sequencer.nCurrentBeat, 1), "|");

	// Draw Sequences
	const auto tlSequencer = w2s(colx1, row);
	const auto tlNames = w2s(colx1, row + 1);
	const auto maxxNames = w2s(colx2 - 1, 0).x;
	const auto tlBeats = w2s(colx2, row+1);
	auto maxxBeats = 0;
	auto maxy = 0;
	for (auto v : sequencer.vecChannel)
	{
		++row;
		const auto colour = [&sequencer = sequencer, &v]()
		{
			if (sequencer.muted())
			{
				if (v.bMuted)
					return olc::VERY_DARK_GREY;
				return olc::DARK_GREY;
			}
			if (v.bMuted)
				return olc::GREY;
			return olc::WHITE;
		}();
		/*DrawString(w2s(colx1, row), v.instrument->name, colour); */
		auto col = 0;
		for (auto vol : v.sBeat)
		{
			const auto [x,y] = w2s(colx2 + col, row);

			if (vol == 0)
			{
				FillRect(x, y, 6, 8, olc::BLUE);
			}
			else if (vol < 8)
			{
				FillRect(x, y, 6, 8, olc::DARK_BLUE);
				FillRect(x, y+(8-vol), 6, vol, colour);
			}
			else
			{
				FillRect(x, y, 6, 8, colour);
			}

			if (x > maxxBeats)
				maxxBeats = x;
			if (y > maxy)
				maxy = y;
			++col;
		}
	}
	if (GetMouse(0).bReleased)
	{
		auto pos = GetMousePos();

		const auto brBeats = olc::vi2d(maxxBeats + 8, maxy + 10);
		if (pos.x >= tlBeats.x && pos.x <= brBeats.x &&
			pos.y >= tlBeats.y && pos.y <= brBeats.y)
		{
			// change volume of beat
			auto clickedCol = (pos.x - tlBeats.x) / 8;
			auto clickedRow = (pos.y - tlBeats.y) / 10;
			auto& v = sequencer.vecChannel[clickedRow];
			auto& ch = v.sBeat[clickedCol];
			ch = ch + 2;
			if (ch > 8)
				ch = 0;
		}
		else
		{
			const auto brSequencer = olc::vi2d(maxxNames + 8, 30);
			if (pos.x >= tlSequencer.x && pos.x <= brSequencer.x &&
				pos.y >= tlSequencer.y && pos.y <= brSequencer.y)
			{
				// mute/unmute entire sequencer
				sequencer.bMuted = !sequencer.bMuted;
			}
			/*else
			{
				 // mute/unmute single channel
				const auto brNames = olc::vi2d(maxxNames + 8, maxy + 10);
				if (pos.x >= tlNames.x && pos.x <= brNames.x &&
					pos.y >= tlNames.y && pos.y <= brNames.y)
				{
					// mute/unmute
					auto clickedRow = (pos.y - tlNames.y) / 10;
					auto& v = sequencer.vecChannel[clickedRow];
					v.bMuted = !v.bMuted;
				}
			}
			*/
		}
	}

	// Draw Keyboard
	row += 1;
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
	{
		w->draw();
	}

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
