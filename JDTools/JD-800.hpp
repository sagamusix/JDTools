// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 - 2024 by Johannes Schultz
// License: BSD 3-clause

#pragma once

#include <array>
#include <cstdint>

struct Tone800
{
	struct Common
	{
		uint8_t
			velocityCurve,
			holdControl;
	};

	struct LFO
	{
		uint8_t
			rate,
			delay,
			fade,
			waveform,
			offset,
			keyTrigger;
	};

	struct WG
	{
		uint8_t
			waveSource,
			waveformMSB,
			waveformLSB,
			pitchCoarse,
			pitchFine,
			pitchRandom,
			keyFollow,
			benderSwitch,
			aTouchBend,
			lfo1Sens,
			lfo2Sens,
			leverSens,
			aTouchModSens;
	};

	struct PitchEnv
	{
		uint8_t
			velo,
			timeVelo,
			timeKF,
			level0,
			time1,
			level1,
			time2,
			time3,
			level2;
	};

	struct TVF
	{
		uint8_t
			filterMode,
			cutoffFreq,
			resonance,
			keyFollow,
			aTouchSens,
			lfoSelect,
			lfoDepth,
			envDepth;
	};

	struct TVFEnv
	{
		uint8_t
			velo,
			timeVelo,
			timeKF,
			time1,
			level1,
			time2,
			level2,
			time3,
			sustainLevel,
			time4,
			level4;
	};

	struct TVA
	{
		uint8_t
			biasDirection,
			biasPoint,
			biasLevel,
			level,
			aTouchSens,
			lfoSelect,
			lfoDepth;
	};

	struct TVAEnv
	{
		uint8_t
			velo,
			timeVelo,
			timeKF,
			time1,
			level1,
			time2,
			level2,
			time3,
			sustainLevel,
			time4;
	};

	Common common;
	LFO lfo1;
	LFO lfo2;
	WG wg;
	PitchEnv pitchEnv;
	TVF tvf;
	TVFEnv tvfEnv;
	TVA tva;
	TVAEnv tvaEnv;
};

struct Patch800
{
	struct Common
	{
		std::array<char, 16> name;
		uint8_t
			patchLevel,
			keyRangeLowA,
			keyRangeHighA,
			keyRangeLowB,
			keyRangeHighB,
			keyRangeLowC,
			keyRangeHighC,
			keyRangeLowD,
			keyRangeHighD,
			benderRangeDown,
			benderRangeUp,
			aTouchBend,
			soloSW,
			soloLegato,
			portamentoSW,
			portamentoMode,
			portamentoTime,
			layerTone,
			activeTone;
	};

	struct EQ
	{
		uint8_t
			lowFreq,
			lowGain,
			midFreq,
			midQ,
			midGain,
			highFreq,
			highGain;
	};

	struct MidiTx
	{
		uint8_t
			keyMode,     // 0 = whole
			splitPoint,  // default = 36
			lowerChannel,
			upperChannel,
			lowerProgramChange,
			upperProgramChange,
			holdMode,  // 2 = both
			dummy;
	};

	struct Effect
	{
		uint8_t
			groupAsequence,
			groupBsequence,
			groupAblockSwitch1,
			groupAblockSwitch2,
			groupAblockSwitch3,
			groupAblockSwitch4,
			groupBblockSwitch1,
			groupBblockSwitch2,
			groupBblockSwitch3,
			effectsBalanceGroupB,

			distortionType,
			distortionDrive,
			distortionLevel,

			phaserManual,
			phaserRate,
			phaserDepth,
			phaserResonance,
			phaserMix,

			spectrumBand1,
			spectrumBand2,
			spectrumBand3,
			spectrumBand4,
			spectrumBand5,
			spectrumBand6,
			spectrumBandwidth,

			enhancerSens,
			enhancerMix,

			delayCenterTap,
			delayCenterLevel,
			delayLeftTap,
			delayLeftLevel,
			delayRightTap,
			delayRightLevel,
			delayFeedback,

			chorusRate,
			chorusDepth,
			chorusDelayTime,
			chorusFeedback,
			chorusLevel,

			reverbType,
			reverbPreDelay,
			reverbEarlyRefLevel,
			reverbHFDamp,
			reverbTime,
			reverbLevel,
			dummy;
	};

	Common common;
	EQ eq;
	MidiTx midiTx;
	Effect effect;
	Tone800 toneA;
	Tone800 toneB;
	Tone800 toneC;
	Tone800 toneD;
};

struct SpecialSetup800
{
	struct EQ
	{
		uint8_t
			lowFreq,
			lowGain,
			midFreq,
			midQ,
			midGain,
			highFreq,
			highGain;
	};

	struct Common
	{
		uint8_t
			benderRangeDown,
			benderRangeUp,
			aTouchBendSens;
	};

	struct Key
	{
		std::array<char, 10> name;
		uint8_t
			muteGroup,
			envMode,
			pan,
			effectMode,
			effectLevel,
			dummy;
		Tone800 tone;
	};
	
	EQ eq;
	Common common;
	std::array<Key, 61> keys;
};
