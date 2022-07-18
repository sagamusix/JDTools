// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 by Johannes Schultz
// License: BSD 3-clause

#pragma once

#include <array>
#include <cstdint>

struct Tone990
{
	struct WG
	{
		uint8_t
			waveSource,
			waveformMSB,
			waveformLSB,
			fxmColor,         // 990 only, default = 0
			fxmDepth,         // 990 only, default = 0
			syncSlaveSwitch,  // 990 only, default = 0
			toneDelayMode,    // 990 only, default = 0
			toneDelayTime,    // 990 only, default = 0
			pitchCoarse,
			pitchFine,
			pitchRandom,
			keyFollow,
			envDepth,  // 990 only, default = 24
			benderSwitch;
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
			sustainLevel,  // 990 only
			time3,
			level3;
	};


	struct TVF
	{
		uint8_t
			filterMode,
			cutoffFreq,
			resonance,
			keyFollow,
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
			level,
			biasDirection,
			biasPoint,
			biasLevel,
			pan,           // 990 only, default = 50
			panKeyFollow;  // 990 only, default = 7
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

	struct Common
	{
		uint8_t
			velocityCurve,
			holdControl;
	};

	struct LFO
	{
		uint8_t
			waveform,
			rate,
			delay,
			fade,
			offset,
			keyTrigger,
			depthPitch,
			depthTVF,
			depthTVA;
	};

	struct ControlSource
	{
		uint8_t
			destination1,  // pitch, cutoff, res, level, p-lfo1, p-lfo2, f-lfo1, f-lfo2, a-lfo1, a-lfo2, lfo1-r, lfo2-r
			depth1,
			destination2,
			depth2,
			destination3,
			depth3,
			destination4,
			depth4;
	};

	WG wg;
	PitchEnv pitchEnv;
	TVF tvf;
	TVFEnv tvfEnv;
	TVA tva;
	TVAEnv tvaEnv;
	Common common;
	LFO lfo1;
	LFO lfo2;
	ControlSource cs1;
	ControlSource cs2;
};

struct Patch990
{
	struct Common
	{
		std::array<char, 16> name;
		uint8_t
			patchLevel,
			patchPan,       // 990 only, default = 50
			analogFeel,     // 990 only, default = 0
			voicePriority,  // 990 only, default = 0
			bendRangeDown,
			bendRangeUp,
			toneControlSource1,  // mod, after, exp, breath, p.bend, foot
			toneControlSource2,  // mod, after, exp, breath, p.bend, foot
			layerTone,
			activeTone;
	};

	struct KeyEffects
	{
		uint8_t
			portamentoSW,
			portamentoMode,
			portamentoType,  // 990 only, default = 1
			portamentoTime,
			soloSW,
			soloLegato,
			soloSyncMaster;  // 990 only
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

	struct StructureType
	{
		uint8_t
			structureAB,  // type 1-6
			structureCD;  // type 1-6

	};

	struct KeyRanges
	{
		uint8_t
			keyRangeLowA,
			keyRangeLowB,
			keyRangeLowC,
			keyRangeLowD,
			keyRangeHighA,
			keyRangeHighB,
			keyRangeHighC,
			keyRangeHighD;
	};

	struct Velocity
	{
		uint8_t
			velocityRange1,
			velocityRange2,
			velocityRange3,
			velocityRange4,
			velocityPoint1,
			velocityPoint2,
			velocityPoint3,
			velocityPoint4,
			velocityFade1,
			velocityFade2,
			velocityFade3,
			velocityFade4;
	};

	struct Effect
	{
		uint8_t
			effectsBalanceGroupB,
			controlSource1,
			controlDest1,
			controlDepth1,
			controlSource2,
			controlDest2,
			controlDepth2,

			groupAsequence,
			groupAblockSwitch1,
			groupAblockSwitch2,
			groupAblockSwitch3,
			groupAblockSwitch4,
			
			distortionType,
			distortionDrive,
			distortionLevel,

			phaserManual,
			phaseRate,
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

			groupBsequence,
			groupBblockSwitch1,
			groupBblockSwitch2,
			groupBblockSwitch3,

			chorusRate,
			chorusDepth,
			chorusDelayTime,
			chorusFeedback,
			chorusLevel,

			delayMode,          // 990 only
			delayCenterTapMSB,  // 990 only (delay values above 0x7D are not supported)
			delayCenterTapLSB,
			delayCenterLevel,
			delayLeftTapMSB,  // 990 only
			delayLeftTapLSB,
			delayLeftLevel,
			delayRightTapMSB,  // 990 only
			delayRightTapLSB,
			delayRightLevel,
			delayFeedback,

			reverbType,
			reverbPreDelay,
			reveryEarlyRefLevel,
			reverbHFDamp,
			reverbTime,
			reverbLevel;
	};

	Common common;
	KeyEffects keyEffects;
	EQ eq;
	StructureType structureType;
	KeyRanges keyRanges;
	Velocity velocity;  // 990 only
	Effect effect;
	uint8_t octaveSwitch;  // 990 only, default = 1
	Tone990 toneA;
	Tone990 toneB;
	Tone990 toneC;
	Tone990 toneD;
};


struct SpecialSetup990
{
	struct Common
	{
		std::array<char, 16> name;
		uint8_t
			level,
			pan,
			analogFeel,
			benderRangeDown,
			benderRangeUp,
			toneControlSource1,
			toneControlSource2;
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

	struct Effect
	{
		uint8_t
			controlSource1,
			controlDest1,
			controlDepth1,
			controlSource2,
			controlDest2,
			controlDepth2,

			chorusRate,
			chorusDepth,
			chorusDelayTime,
			chorusFeedback,
			chorusLevel,

			delayMode,
			delayCenterTapMSB,
			delayCenterTapLSB,
			delayCenterLevel,
			delayLeftTapMSB,
			delayLeftTapLSB,
			delayLeftLevel,
			delayRightTapMSB,
			delayRightTapLSB,
			delayRightLevel,
			delayFeedback,

			reverbType,
			reverbPreDelay,
			reveryEarlyRefLevel,
			reverbHFDamp,
			reverbTime,
			reverbLevel;
	};

	struct Key
	{
		std::array<uint8_t, 10> name;
		uint8_t
			envMode,
			muteGroup,  // 00-1A instead of 00-08!
			effectMode,  // different
			effectLevel;
		Tone990 tone;
	};

	Common common;
	EQ eq;
	Effect effect;
	std::array<Key, 61> keys;
};
