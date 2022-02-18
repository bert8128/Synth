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

#include <list>
#include <iostream>
#include <algorithm>
#include <assert.h>


namespace Synth
{
	enum WaveType 
	{
		  OSC_SINE
		, OSC_SQUARE
		, OSC_TRIANGLE
		, OSC_SAW_ANA
		, OSC_SAW_DIG
		, OSC_NOISE
	};

	FTYPE oscillator2(
		const FTYPE dTime,
		const FTYPE dHertz,
		const WaveType nType,
		const FTYPE dFreq,
		FTYPE dCustom)
	{
		switch (nType)
		{
		case OSC_SINE: // Sine wave bewteen -1 and +1
			return sin(dFreq);

		case OSC_SQUARE: // Square wave between -1 and +1
			return sin(dFreq) > 0 ? 1.0 : -1.0;

		case OSC_TRIANGLE: // Triangle wave between -1 and +1
			return asin(sin(dFreq)) * (2.0 / PI);

		case OSC_SAW_ANA: // Saw wave (analogue / warm / slow)
		{
			FTYPE dOutput = 0.0;
			for (FTYPE n = 1.0; n < dCustom; ++n)
				dOutput += (sin(n * dFreq)) / n;
			return dOutput * (2.0 / PI);
		}

		case OSC_SAW_DIG:
			return (2.0 / PI) * (dHertz * PI * fmod(dTime, 1.0 / dHertz) - (PI / 2.0));

		case OSC_NOISE:
			return 2.0 * ((FTYPE)rand() / (FTYPE)RAND_MAX) - 1.0;
		}

		assert(false);
		return 0.0;
	}

	// Converts frequency (Hz) to angular velocity
	inline FTYPE f2w(const FTYPE dHertz)
	{
		return dHertz * 2.0 * PI;
	}
	FTYPE oscillator(const FTYPE dTime, const FTYPE dHertz, const WaveType nType)
	{
		FTYPE dFreq = f2w(dHertz) * dTime;
		return oscillator2(dTime, dHertz, nType, dFreq, 50);
	}
	FTYPE oscillator(const FTYPE dTime, const FTYPE dHertz, const WaveType nType, const FTYPE dLFOHertz, const FTYPE dLFOAmplitude)
	{
		FTYPE dFreq = f2w(dHertz) * dTime + dLFOAmplitude * dHertz * (sin(f2w(dLFOHertz) * dTime));
		return oscillator2(dTime, dHertz, nType, dFreq, 50);
	}
	FTYPE oscillator(const FTYPE dTime, const FTYPE dHertz, const WaveType nType, const FTYPE dLFOHertz, const FTYPE dLFOAmplitude, FTYPE dCustom)
	{
		FTYPE dFreq = f2w(dHertz) * dTime + dLFOAmplitude * dHertz * (sin(f2w(dLFOHertz) * dTime));
		return oscillator2(dTime, dHertz, nType, dFreq, dCustom);
	}

	//////////////////////////////////////////////////////////////////////////////
	// Scale to Frequency conversion

	const int SCALE_DEFAULT = 0;

	FTYPE scale(const int nNoteID, const int /*nScaleID*/ = SCALE_DEFAULT)
	{
		//constexpr double twoPowerOneTwelth = pow(2.0, (1.0 / 12.0));
		constexpr double twoPowerOneTwelth = 1.0594630943592952645618252949463;
		return 8 * pow(twoPowerOneTwelth, nNoteID);
		/*switch (nScaleID)
		{
		case SCALE_DEFAULT: 
		default:
			return 8 * pow(twoPowerOneTwelth, nNoteID);
		}
		*/
	}


	//////////////////////////////////////////////////////////////////////////////
	// Envelopes
	// Don't need a base class (yet)
	//struct Envelope { virtual FTYPE amplitude(const FTYPE dTime, const FTYPE dTimeOn, const FTYPE dTimeOff) const = 0;};
	//struct EnvelopeADSR : public Envelope

	struct Envelope
	{
		FTYPE dAttackTime = 0;
		FTYPE dDecayTime = 0;
		FTYPE dSustainAmplitude = 0;
		FTYPE dReleaseTime = 0;
		FTYPE dStartAmplitude = 1.0;

		FTYPE amplitude(const FTYPE dTime, const FTYPE dTimeOn, const FTYPE dTimeOff) const
		{
			FTYPE dAmplitude = 0.0;
			FTYPE dReleaseAmplitude = 0.0;

			if (dTimeOn > dTimeOff) // Note is on
			{
				FTYPE dLifeTime = dTime - dTimeOn;

				if (dLifeTime <= dAttackTime)
					dAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;

				if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
					dAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;

				if (dLifeTime > (dAttackTime + dDecayTime))
					dAmplitude = dSustainAmplitude;
			}
			else // Note is off
			{
				FTYPE dLifeTime = dTimeOff - dTimeOn;

				if (dLifeTime <= dAttackTime)
					dReleaseAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;

				if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
					dReleaseAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;

				if (dLifeTime > (dAttackTime + dDecayTime))
					dReleaseAmplitude = dSustainAmplitude;

				dAmplitude = ((dTime - dTimeOff) / dReleaseTime) * (0.0 - dReleaseAmplitude) + dReleaseAmplitude;
			}

			// Amplitude should not be negative
			if (dAmplitude <= 0.01)
				dAmplitude = 0.0;

			return dAmplitude;
		}
	};

	static FTYPE env(const FTYPE dTime, const Envelope &envel, const FTYPE dTimeOn, const FTYPE dTimeOff)
	{
		return envel.amplitude(dTime, dTimeOn, dTimeOff);
	}


	// A basic note
	struct Note
	{
		int id = 0;		// Position in scale
		FTYPE on = 0;	// Time note was activated
		FTYPE off = 0;	// Time note was deactivated
		bool active = false;
	};

	struct Instrument
	{
		FTYPE dVolume;
		Envelope envADSR;
		FTYPE fMaxLifeTime;
		std::string name;
		virtual FTYPE sound(const FTYPE dTime, Note note, bool &bNoteFinished) const = 0;
	};

	struct NoteInstrumentPtr
	{
		Note m_Note;
		Instrument* m_pInstrument;
	};

	struct Instrument_harmonica : public Instrument
	{
		Instrument_harmonica()
		{
			envADSR.dAttackTime = 0.00;
			envADSR.dDecayTime = 1.0;
			envADSR.dSustainAmplitude = 0.95;
			envADSR.dReleaseTime = 0.1;
			fMaxLifeTime = -1.0;
			name = "Harmonica";
			dVolume = 0.3;
		}

		FTYPE sound(const FTYPE dTime, Note note, bool& bNoteFinished) const override
		{
			FTYPE dAmplitude = Synth::env(dTime, envADSR, note.on, note.off);
			if (dAmplitude <= 0.0)
				bNoteFinished = true;

			auto t1 = note.on - dTime;
			auto t2 = dTime - note.on;
			FTYPE dSound =
				+ 1.00 * Synth::oscillator(t1, Synth::scale(note.id - 12), Synth::OSC_SAW_ANA, 5.0, 0.001, 100)
				+ 1.00 * Synth::oscillator(t2, Synth::scale(note.id + 00), Synth::OSC_SQUARE, 5.0, 0.001)
				+ 0.50 * Synth::oscillator(t2, Synth::scale(note.id + 12), Synth::OSC_SQUARE)
				+ 0.05 * Synth::oscillator(t2, Synth::scale(note.id + 24), Synth::OSC_NOISE);

			return dAmplitude * dSound * dVolume;
		}

	};
/* Currently not in use
	struct Instrument_bell : public Instrument
	{
		Instrument_bell()
		{
			envADSR.dAttackTime = 0.01;
			envADSR.dDecayTime = 1.0;
			envADSR.dSustainAmplitude = 0.0;
			envADSR.dReleaseTime = 1.0;
			fMaxLifeTime = 3.0;
			dVolume = 1.0;
			name = "Bell";
		}

		FTYPE sound(const FTYPE dTime, Note note, bool& bNoteFinished) const override
		{
			FTYPE dAmplitude = Synth::env(dTime, envADSR, note.on, note.off);
			if (dAmplitude <= 0.0)
			bNoteFinished = true;

			FTYPE dSound =
				+ 1.00 * Synth::oscillator(dTime - note.on, Synth::scale(note.id + 12), Synth::OSC_SINE, 5.0, 0.001)
				+ 0.50 * Synth::oscillator(dTime - note.on, Synth::scale(note.id + 24), Synth::OSC_SINE)
				+ 0.25 * Synth::oscillator(dTime - note.on, Synth::scale(note.id + 36), Synth::OSC_SINE);

			return dAmplitude * dSound * dVolume;
		}

	};
*/
/* Currently not in use
	struct Instrument_bell8 : public Instrument
	{
		Instrument_bell8()
		{
			envADSR.dAttackTime = 0.01;
			envADSR.dDecayTime = 0.5;
			envADSR.dSustainAmplitude = 0.8;
			envADSR.dReleaseTime = 1.0;
			fMaxLifeTime = 3.0;
			dVolume = 1.0;
			name = "8-Bit Bell";
		}

		FTYPE sound(const FTYPE dTime, Note note, bool& bNoteFinished) const override
		{
			FTYPE dAmplitude = Synth::env(dTime, envADSR, note.on, note.off);
			if (dAmplitude <= 0.0)
			bNoteFinished = true;

			FTYPE dSound =
				+1.00 * Synth::oscillator(dTime - note.on, Synth::scale(note.id), Synth::OSC_SQUARE, 5.0, 0.001)
				+ 0.50 * Synth::oscillator(dTime - note.on, Synth::scale(note.id + 12), Synth::OSC_SINE)
				+ 0.25 * Synth::oscillator(dTime - note.on, Synth::scale(note.id + 24), Synth::OSC_SINE);

			return dAmplitude * dSound * dVolume;
		}

	};
*/

	struct Instrument_drumkick : public Instrument
	{
		Instrument_drumkick()
		{
			envADSR.dAttackTime = 0.01;
			envADSR.dDecayTime = 0.15;
			envADSR.dSustainAmplitude = 0.0;
			envADSR.dReleaseTime = 0.0;
			fMaxLifeTime = 1.5;
			name = "Drum Kick";
			dVolume = 1.0;
		}

		FTYPE sound(const FTYPE dTime, Note note, bool& bNoteFinished) const override
		{
			FTYPE dAmplitude = Synth::env(dTime, envADSR, note.on, note.off);
			if (fMaxLifeTime > 0.0 && dTime - note.on >= fMaxLifeTime)
				bNoteFinished = true;

			FTYPE dSound =
				+ 0.99 * Synth::oscillator(dTime - note.on, Synth::scale(note.id - 36), Synth::OSC_SINE, 1.0, 1.0)
				+ 0.5 * Synth::oscillator(dTime - note.on, 0,                          Synth::OSC_NOISE);

			return dAmplitude * dSound * dVolume;
		}

	};

	struct Instrument_drumsnare : public Instrument
	{
		Instrument_drumsnare()
		{
			envADSR.dAttackTime = 0.0;
			envADSR.dDecayTime = 0.2;
			envADSR.dSustainAmplitude = 0.0;
			envADSR.dReleaseTime = 0.0;
			fMaxLifeTime = 1.0;
			name = "Drum Snare";
			dVolume = 1.0;
		}

		FTYPE sound(const FTYPE dTime, Note note, bool& bNoteFinished) const override
		{
			FTYPE dAmplitude = Synth::env(dTime, envADSR, note.on, note.off);
			if (fMaxLifeTime > 0.0 && dTime - note.on >= fMaxLifeTime)
				bNoteFinished = true;

			FTYPE dSound =
				+ 0.5 * Synth::oscillator(dTime - note.on, Synth::scale(note.id - 24), Synth::OSC_SINE, 0.5, 1.0)
				+ 0.5 * Synth::oscillator(dTime - note.on, 0,                          Synth::OSC_NOISE);

			return dAmplitude * dSound * dVolume;
		}

	};


	struct Instrument_drumhihat : public Instrument
	{
		Instrument_drumhihat()
		{
			envADSR.dAttackTime = 0.01;
			envADSR.dDecayTime = 0.05;
			envADSR.dSustainAmplitude = 0.0;
			envADSR.dReleaseTime = 0.0;
			fMaxLifeTime = 1.0;
			name = "Drum HiHat";
			dVolume = 0.5;
		}

		FTYPE sound(const FTYPE dTime, Note note, bool& bNoteFinished) const override
		{
			FTYPE dAmplitude = Synth::env(dTime, envADSR, note.on, note.off);
			if (fMaxLifeTime > 0.0 && dTime - note.on >= fMaxLifeTime)	bNoteFinished = true;

			FTYPE dSound =
				+ 0.1 * Synth::oscillator(dTime - note.on, Synth::scale(note.id -12), Synth::OSC_SQUARE, 1.5, 1)
				+ 0.9 * Synth::oscillator(dTime - note.on, 0, Synth::OSC_NOISE);

			return dAmplitude * dSound * dVolume;
		}

	};


	struct Sequencer
	{
	public:
		struct Channel
		{
			Instrument* instrument;
			std::string sBeat;
		};

	public:
		Sequencer(float tempo, int beats, int subbeats)
		{
			nBeats = beats;
			nSubBeats = subbeats;
			fTempo = tempo;
			fBeatTime = (60.0f / fTempo) / (float)nSubBeats;
			nCurrentBeat = 0;
			nTotalBeats = nSubBeats * nBeats;
			fAccumulate = 0;
		}


		void Update(FTYPE fElapsedTime)
		{
			vecNotes.clear();

			fAccumulate += fElapsedTime;
			while (fAccumulate >= fBeatTime)
			{
				fAccumulate -= fBeatTime;
				++nCurrentBeat;

				if (nCurrentBeat >= nTotalBeats)
					nCurrentBeat = 0;

				int channel = 0;
				for (auto v : vecChannel)
				{
					if (v.sBeat[nCurrentBeat] == 'X')
					{
						Note note;
						note.active = true;
						note.id = 64;
						vecNotes.emplace_back(note, vecChannel[channel].instrument);
					}
					++channel;
				}
			}
		}

		/** Returns index of instrument */
		size_t AddInstrument(Instrument* inst)
		{
			Channel c;
			c.instrument = inst;
			vecChannel.push_back(c);
			return vecChannel.size() - 1;
		}

		public:
		int nBeats;
		int nSubBeats;
		FTYPE fTempo;
		FTYPE fBeatTime;
		FTYPE fAccumulate;
		int nCurrentBeat;
		int nTotalBeats;

	public:
		std::vector<Channel> vecChannel;
		std::vector<NoteInstrumentPtr> vecNotes;
		

	private:
	
	};
	
}

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
	std::unique_lock<std::mutex> lm(muxNotes);
	FTYPE dMixedOutput = 0.0;

	// Iterate through all active notes, and mix together
	for (auto& [n, c] : vecNotes)
	{
		bool bNoteFinished = false;
		FTYPE dSound = 0;

		// Get sample for this note by using the correct instrument and envelope
		if (c != nullptr)
			dSound = c->sound(dTime, n, bNoteFinished);
		
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
		: sequencer(90.0f, 4, 4)
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
};


bool Synthesiser::OnUserCreate()
{

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

	sequencer.vecChannel[kick ].sBeat = "X...X...X..X.X..";
	sequencer.vecChannel[snare].sBeat = "..X...X...X...X.";
	sequencer.vecChannel[hh   ].sBeat = "X.X.X.X.X.X.X.XX";

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
	muxNotes.lock();
	for (auto& note : sequencer.vecNotes)
	{
		note.m_Note.on = dTimeNow;
		vecNotes.emplace_back(note);
	}
	muxNotes.unlock();

	// Keyboard (generates and removes notes depending on key state) ========================================
	// Note : olc::OEM_2 is the the /? key
	constexpr auto Keyboard = std::to_array({ olc::Z, olc::S, olc::X, olc::C, olc::F, olc::V, olc::G, olc::B, olc::N, olc::J, olc::M, olc::K, olc::COMMA, olc::L, olc::PERIOD, olc::OEM_2 });
	for (int k = 0; k < Keyboard.size(); ++k)
	{
		auto key = GetKey(Keyboard[k]);

		// Check if note already exists in currently playing notes
		muxNotes.lock();
		auto noteFound = find_if(vecNotes.begin(), vecNotes.end(), [&k](const Synth::NoteInstrumentPtr& item) { return item.m_Note.id == k + 64 && item.m_pInstrument == &instHarm; });
		if (noteFound == vecNotes.end())
		{
			// Note not found in vector
			if (key.bHeld)
			{
				// Key has been pressed so create a new note
				Synth::Note note;
				note.id = k + 64;
				note.on = dTimeNow;
				note.active = true;

				// Add note to vector
				vecNotes.emplace_back(note, &instHarm);
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
		muxNotes.unlock();
	}

	// --- VISUAL STUFF ---

	Clear(olc::BLACK);

	// Draw Sequencer

	constexpr int colx1 = 2;
	constexpr int colx2 = 16;
	int row = 2;
	DrawString(w2s(colx1, row), "SEQUENCER:");
	for (int beats = 0; beats < sequencer.nBeats; ++beats)
	{
		DrawString(w2s(beats * sequencer.nSubBeats + colx2, row), "O");
		for (int subbeats = 1; subbeats < sequencer.nSubBeats; ++subbeats)
			DrawString(w2s((beats * sequencer.nSubBeats) + subbeats + colx2, row), ".");
	}
	// Draw Sequences
	for (auto v : sequencer.vecChannel)
	{
		++row;
		DrawString(w2s(colx1, row), v.instrument->name);
		DrawString(w2s(colx2, row), v.sBeat);
	}
	// Draw Beat Cursor
	DrawString(w2s(colx2 + sequencer.nCurrentBeat, 1), "|");

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
