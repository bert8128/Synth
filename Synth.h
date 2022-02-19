#pragma once

#include <string>
#include <string_view>
#include <vector>

#ifndef FTYPE
#define FTYPE double
#endif

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
	enum HarmonicDecayType
	{
		LINEAR,
		EXPONENTIAL,
	};

	constexpr auto BaseNoteID = 64;

	WaveType strToWaveType(const std::string_view str);
	std::string waveTypeToStr(const WaveType wt);
	HarmonicDecayType strToHarmonicDecayType(const std::string_view str);
	std::string harmonicDecayTypeToStr(const HarmonicDecayType wt);

	// Converts frequency (Hz) to angular velocity
	inline FTYPE f2w(const FTYPE dHertz);
	FTYPE oscillator(const FTYPE dTime, const FTYPE dHertz, const WaveType nType);
	FTYPE oscillator(const FTYPE dTime, const FTYPE dHertz, const WaveType nType, const FTYPE dLFOHertz, const FTYPE dLFOAmplitude);
	FTYPE oscillator(const FTYPE dTime, const FTYPE dHertz, const WaveType nType, const FTYPE dLFOHertz, const FTYPE dLFOAmplitude, FTYPE dCustom);

	//////////////////////////////////////////////////////////////////////////////
	// Scale to Frequency conversion

	constexpr int SCALE_DEFAULT = 0;

	FTYPE scale(const int nNoteID, const int /*nScaleID*/ = SCALE_DEFAULT);


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

		FTYPE amplitude(const FTYPE dTime, const FTYPE dTimeOn, const FTYPE dTimeOff) const;
	};

	FTYPE env(const FTYPE dTime, const Envelope& envel, const FTYPE dTimeOn, const FTYPE dTimeOff);


	// A basic note
	struct Note
	{
		int id = 0;		// Position in scale
		FTYPE on = 0;	// Time note was activated
		FTYPE off = 0;	// Time note was deactivated
		FTYPE velocity = 1.0; // how lound is this note, relative to the base loudness of the instrument
		bool active = false;
	};

	struct Instrument
	{
		FTYPE dVolume;
		Envelope envADSR;
		FTYPE fMaxLifeTime;
		std::string name;
		virtual FTYPE sound(const FTYPE dTime, Note note, bool& bNoteFinished) const = 0;
	};

	struct NoteInstrumentPtr
	{
		Note m_Note;
		Instrument* m_pInstrument;
	};

	struct CustomInstrument : public Instrument
	{
		struct Sound
		{
			/*
						"Amp": 1.0,
						"Freq": 0,
						"Type": "SawA",
						"LFreq": 5.0,
						"LAmp": 0.001,
						"Cust": 100
						"Harmonics": 1,
						"DecayType": "Exp",
						"Decay": 50,
						"EvenOddBalance": 50
			*/
			FTYPE amp = 1;
			int freq = 0;
			WaveType type;
			FTYPE lFreq = 0;
			FTYPE lAmp = 0;
			FTYPE custom = 0;
			int harmonics = 0;
			HarmonicDecayType decayType = LINEAR;
			FTYPE decay = 0;
			int evenOddBal = 50;
		};
		CustomInstrument();
		FTYPE sound(const FTYPE dTime, Note note, bool& bNoteFinished) const override;
		std::vector<Sound> sounds;
	};

	struct Instrument_harmonica : public Instrument
	{
		Instrument_harmonica();
		FTYPE sound(const FTYPE dTime, Note note, bool& bNoteFinished) const override;
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
		Instrument_drumkick();
		FTYPE sound(const FTYPE dTime, Note note, bool& bNoteFinished) const override;
	};

	struct Instrument_drumsnare : public Instrument
	{
		Instrument_drumsnare();
		FTYPE sound(const FTYPE dTime, Note note, bool& bNoteFinished) const override;
	};


	struct Instrument_drumhihat : public Instrument
	{
		Instrument_drumhihat();
		FTYPE sound(const FTYPE dTime, Note note, bool& bNoteFinished) const override;
	};


	struct Sequencer
	{
	public:
		struct Channel
		{
			Instrument* instrument;
			std::vector<int> sBeat;
			bool bMuted = false;
		};

	public:
		Sequencer(float tempo, int beats, int subbeats);
		void Update(FTYPE fElapsedTime);

		/** Returns index of instrument */
		size_t AddInstrument(Instrument* inst);

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

	std::vector<CustomInstrument> loadInstruments();
}
