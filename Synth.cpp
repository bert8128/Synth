#include "Synth.h"
#include <assert.h>
#include <fstream>
#include <iostream>

#include "JSON.h"
namespace json = nlohmann;

namespace Synth
{
	constexpr auto PI = 3.14159265354;

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

	FTYPE scale(const int nNoteID, const int /*nScaleID = SCALE_DEFAULT*/)
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

	FTYPE Envelope::amplitude(const FTYPE dTime, const FTYPE dTimeOn, const FTYPE dTimeOff) const
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

	/*static*/ FTYPE env(const FTYPE dTime, const Envelope& envel, const FTYPE dTimeOn, const FTYPE dTimeOff)
	{
		return envel.amplitude(dTime, dTimeOn, dTimeOff);
	}

	Instrument_harmonica::Instrument_harmonica()
	{
		envADSR.dAttackTime = 0.00;
		envADSR.dDecayTime = 1.0;
		envADSR.dSustainAmplitude = 0.95;
		envADSR.dReleaseTime = 0.1;
		fMaxLifeTime = -1.0;
		name = "Harmonica";
		dVolume = 0.3;
	}

	FTYPE Instrument_harmonica::sound(const FTYPE dTime, Note note, bool& bNoteFinished) const
	{
		FTYPE dAmplitude = Synth::env(dTime, envADSR, note.on, note.off);
		if (dAmplitude <= 0.0)
			bNoteFinished = true;

		auto t1 = note.on - dTime;
		auto t2 = dTime - note.on;
		FTYPE dSound =
			+1.00 * Synth::oscillator(t1, Synth::scale(note.id - 12), Synth::OSC_SAW_ANA, 5.0, 0.001, 100)
			+ 1.00 * Synth::oscillator(t2, Synth::scale(note.id + 00), Synth::OSC_SQUARE, 5.0, 0.001)
			+ 0.50 * Synth::oscillator(t2, Synth::scale(note.id + 12), Synth::OSC_SQUARE)
			+ 0.05 * Synth::oscillator(t2, Synth::scale(note.id + 24), Synth::OSC_NOISE);

		return dAmplitude * dSound * dVolume;
	}

	Instrument_drumkick::Instrument_drumkick()
	{
		envADSR.dAttackTime = 0.01;
		envADSR.dDecayTime = 0.15;
		envADSR.dSustainAmplitude = 0.0;
		envADSR.dReleaseTime = 0.0;
		fMaxLifeTime = 1.5;
		name = "Drum Kick";
		dVolume = 1.0;
	}

	FTYPE Instrument_drumkick::sound(const FTYPE dTime, Note note, bool& bNoteFinished) const
	{
		FTYPE dAmplitude = Synth::env(dTime, envADSR, note.on, note.off);
		if (fMaxLifeTime > 0.0 && dTime - note.on >= fMaxLifeTime)
			bNoteFinished = true;

		FTYPE dSound =
			+0.99 * Synth::oscillator(dTime - note.on, Synth::scale(note.id - 36), Synth::OSC_SINE, 1.0, 1.0)
			+ 0.5 * Synth::oscillator(dTime - note.on, 0, Synth::OSC_NOISE);

		return dAmplitude * dSound * dVolume;
	}

	Instrument_drumsnare::Instrument_drumsnare()
	{
		envADSR.dAttackTime = 0.0;
		envADSR.dDecayTime = 0.2;
		envADSR.dSustainAmplitude = 0.0;
		envADSR.dReleaseTime = 0.0;
		fMaxLifeTime = 1.0;
		name = "Drum Snare";
		dVolume = 1.0;
	}

	FTYPE Instrument_drumsnare::sound(const FTYPE dTime, Note note, bool& bNoteFinished) const
	{
		FTYPE dAmplitude = Synth::env(dTime, envADSR, note.on, note.off);
		if (fMaxLifeTime > 0.0 && dTime - note.on >= fMaxLifeTime)
			bNoteFinished = true;

		FTYPE dSound =
			+0.5 * Synth::oscillator(dTime - note.on, Synth::scale(note.id - 24), Synth::OSC_SINE, 0.5, 1.0)
			+ 0.5 * Synth::oscillator(dTime - note.on, 0, Synth::OSC_NOISE);

		return dAmplitude * dSound * dVolume;
	}

	Instrument_drumhihat::Instrument_drumhihat()
	{
		envADSR.dAttackTime = 0.01;
		envADSR.dDecayTime = 0.05;
		envADSR.dSustainAmplitude = 0.0;
		envADSR.dReleaseTime = 0.0;
		fMaxLifeTime = 1.0;
		name = "Drum HiHat";
		dVolume = 0.5;
	}

	FTYPE Instrument_drumhihat::sound(const FTYPE dTime, Note note, bool& bNoteFinished) const
	{
		FTYPE dAmplitude = Synth::env(dTime, envADSR, note.on, note.off);
		if (fMaxLifeTime > 0.0 && dTime - note.on >= fMaxLifeTime)
			bNoteFinished = true;

		FTYPE dSound =
			+0.1 * Synth::oscillator(dTime - note.on, Synth::scale(note.id - 12), Synth::OSC_SQUARE, 1.5, 1)
			+ 0.9 * Synth::oscillator(dTime - note.on, 0, Synth::OSC_NOISE);

		return dAmplitude * dSound * dVolume;
	}

	Sequencer::Sequencer(float tempo, int beats, int subbeats)
	{
		nBeats = beats;
		nSubBeats = subbeats;
		fTempo = tempo;
		fBeatTime = (60.0f / fTempo) / (float)nSubBeats;
		nCurrentBeat = 0;
		nTotalBeats = nSubBeats * nBeats;
		fAccumulate = 0;
	}

	void Sequencer::Update(FTYPE fElapsedTime)
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
				if (v.bMuted)
					continue;
				const auto currentBeatVol = v.sBeat[nCurrentBeat];
				if (currentBeatVol > 0)
				{
					Note note;
					note.active = true;
					note.id = BaseNoteID;
					note.velocity = currentBeatVol / 6.0f;
					vecNotes.emplace_back(note, vecChannel[channel].instrument);
				}
				++channel;
			}
		}
	}

	size_t Sequencer::AddInstrument(Instrument* inst)
	{
		Channel c;
		c.instrument = inst;
		vecChannel.push_back(c);
		return vecChannel.size() - 1;
	}

	WaveType strToWaveType(const std::string_view str)
	{
		if (str == "Sine")
			return OSC_SINE;
		if (str == "Square")
			return OSC_SQUARE;
		if (str == "Triangle")
			return OSC_TRIANGLE;
		if (str == "SawA")
			return OSC_SAW_ANA;
		if (str == "SawD")
			return OSC_SAW_DIG;
		if (str == "Noise")
			return OSC_NOISE;

		assert(false);
		return OSC_SINE;
	}

	std::string waveTypeToStr(const WaveType wt)
	{
		switch (wt)
		{
		case OSC_SINE:
			return "Sine";
		case OSC_SQUARE:
			return "Square";
		case OSC_TRIANGLE:
			return "Triangle";
		case OSC_SAW_ANA:
			return "SawA";
		case OSC_SAW_DIG:
			return "SawD";
		case OSC_NOISE:
			return "Noise";
		}

		assert(false);
		return "";
	}

	HarmonicDecayType strToHarmonicDecayType(const std::string_view str)
	{
		if (str == "Linear")
			return LINEAR;
		if (str == "Exponential")
			return EXPONENTIAL;

		assert(false);
		return LINEAR;
	}

	std::string harmonicDecayTypeToStr(const HarmonicDecayType wt)
	{
		switch (wt)
		{
		case LINEAR:
			return "Linear";
		case EXPONENTIAL:
			return "Exponential";
		}

		assert(false);
		return "";
	}

	CustomInstrument::CustomInstrument()
	{
	}
	FTYPE CustomInstrument::sound(const FTYPE dTime, Note note, bool& bNoteFinished) const
	{
		FTYPE dAmplitude = Synth::env(dTime, envADSR, note.on, note.off);
		if (dAmplitude <= 0.0)
			bNoteFinished = true;

		//auto t1 = note.on - dTime;
		auto t2 = dTime - note.on;
		FTYPE dSound = 0.0;
		for (auto& s : sounds)
		{
			dSound += s.amp * Synth::oscillator(t2, Synth::scale(note.id - s.freq), s.type, s.lFreq, s.lAmp, s.custom);
			if (s.harmonics > 0)
			{
				auto amp = s.amp;
				const auto decay = (s.decayType == EXPONENTIAL) ? s.decay / 100.0f : s.decay;
				const auto evenOddBal = s.evenOddBal / 100.0f;
				for (auto h = 0; h < s.harmonics; ++h)
				{
					if (s.decayType == EXPONENTIAL)
						amp *= decay;
					else
						amp -= decay;
					if (amp < 0.001)
						break;
					if (h % 2 == 0)
						amp *= evenOddBal;
					else
						amp *= (1 - evenOddBal);
					dSound += amp * Synth::oscillator(t2, Synth::scale(note.id - s.freq), s.type, s.lFreq, s.lAmp, s.custom);
				}
			}
		}

		return dAmplitude * dSound * dVolume;
	}

	std::vector<CustomInstrument> loadInstruments()
	{
		json::json jInstrumentDefinitions;
		{
			std::ifstream i("Instruments.json");
			assert(i.is_open());
			assert(i.good());
			i >> jInstrumentDefinitions;
		}

		std::vector<CustomInstrument> instruments;
		for (auto instJ : jInstrumentDefinitions["Instruments"])
		{
			std::cout << std::setw(4) << instJ << '\n' << std::flush;
			CustomInstrument ci;
			ci.name = instJ["Name"];
			ci.envADSR.dAttackTime = instJ["A"];
			ci.envADSR.dDecayTime = instJ["D"];
			ci.envADSR.dSustainAmplitude = instJ["S"];
			ci.envADSR.dReleaseTime = instJ["R"];
			ci.fMaxLifeTime = instJ["MaxLife"];
			ci.dVolume = instJ["Amp"];

			auto sounds = instJ["Sounds"];
			for (auto s : sounds)
			{
				std::cout << std::setw(4) << s << '\n' << std::flush;
				CustomInstrument::Sound sound;
				sound.amp = s["Amp"];
				if (s.contains("Freq"))
					sound.freq = s["Freq"];
				sound.type = strToWaveType(s["Type"]);
				if (s.contains("LFreq"))
					sound.lFreq = s["LFreq"];
				if (s.contains("LAmp"))
					sound.lAmp = s["LAmp"];
				if (s.contains("Harmonics"))
					sound.harmonics = s["Harmonics"];
				if (s.contains("DecayType"))
					sound.decayType = strToHarmonicDecayType(s["DecayType"]);
				if (s.contains("Decay"))
					sound.decay = s["Decay"];
				if (s.contains("EvenOddbalance"))
					sound.evenOddBal = s["EvenOddbalance"];

				ci.sounds.push_back(sound);
			}
			instruments.push_back(ci);
		}
		return instruments;
	}
}
