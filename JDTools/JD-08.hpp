// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 by Johannes Schultz
// License: BSD 3-clause

#pragma once

#include "Utils.hpp"

#include <array>
#include <cstdint>

// This file format is officially terrible.
// It contains two copies of most parameters - the original set of JD-800 parameters with slight modifications
// (such as using signed chars with -50...+50 range instead of 0...100 - fine by me), exclusively used for display in the UI
// - and ADDITIONALLY a set of precomputed parameters, which I assume are the ZenCore equivalents of the original parameters.
// You cannot just create a BIN file containing the display values that can be trivially converted from SysEx format.
// If you do that, all you hear is silence. No, you have to precompute every single ZenCore parameter, and many of them
// don't map linearly. Even better, for some envelopes, every time segment uses a different mapping!
// As a result of this design choice, you could actually create a file with a parameter display that is completely different
// from the sound that is actually generated until you start to wiggle some sliders, at which point the specific precomputed
// value for that slider will be updated.

// All bytes that actually differ between patches have been identified. The only remaining unknown variables are static,
// but not setting them to their expected values will break various features of a patch.
// Unknown variable names contain an offset relative to the first byte in PatchVST::CommonPrecomputed, and the expected byte value in hex.
// For structs that are repeated for all four tones, the offset is always for the first tone.

struct ToneVSTPrecomputed
{
	struct Layer
	{
		uint8_t
			layerEnabled,
			unknown137_00,
			unknown138_00,
			unknown139_00,
			lowKey,
			highKey,
			unknown142_00,
			unknown143_00,
			lowVelocity,
			highVelocity,
			unknown146_00,
			unknown147_00;
	};

	struct Common
	{
		// Similar to JD-990
		struct ControlSource
		{
			uint8_t
				source,        // 1 = Lever / CC 1, 96 = Aftertouch
				destination1,  // 1 = Pitch, 2 = Cutoff, 4 = Level, 8 = P-LFO 1, 9 = P-LFO 2
				depth1,
				destination2,
				depth2,
				destination3,
				depth3,
				destination4,
				depth4,
				unknown253_00,
				unknown254_00,
				unknown255_00;
		};

		uint8_t
			tvaLevel,
			unknown185_00,
			pitchCoarse,
			pitchFine;
		uint16le
			pitchRandom;
		uint8_t
			unknown190_00,
			unknown191_00,
			unknown192_00,
			unknown193_00,
			unknown194_01,
			unknown195_00,
			unknown196_00,
			unknown197_0C,
			unknown198_00,
			unknown199_00,
			unknown200_00,
			unknown201_00,
			benderSwitch,
			unknown203_01,
			holdControl,
			unknown205_00,
			unknown206_01,
			unknown207_00,
			unknown208_FD,
			unknown209_2A,
			waveformLSB,
			unknown211_00,
			unknown212_00,
			unknown213_00,
			gain,
			unknown215_00,
			unknown216_01,
			unknown217_00;
		uint16le
			pitchKeyFollow;
		uint8_t
			unknown220_00,
			filterType,  // +1
			unknown222_00,
			unknown223_00;
		uint16le
			cutoff,
			filterKeyFollow,  // Precomputed, i.e. 150% = 150
			velocityCurveTVF,
			resonance;
		uint8_t
			unknown232_00,
			tvaBiasLevel,
			tvaBiasPoint,
			tvaBiasDirection,  // Lower = 0, Upper = 1, Lower & Upper = 2
			velocityCurveTVA,
			tvaVelo,
			pitchTimeKF,  // -100...100
			tvfTimeKF,    // -100...100
			tvaTimeKF,    // -100...100
			unknown241_0A,
			unknown242_00,
			unknown243_00;
		ControlSource cs1;  // 244
		ControlSource cs2;  // 256
		ControlSource cs3;  // 268
		ControlSource cs4;  // 280
		ControlSource cs5;  // Probably something completely different, given that Roland typically provides 4 modulation sources
		uint8_t
			unknown304_00,
			unknown305_00,
			unknown306_00,
			unknown307_00;
	};

	struct PitchEnv
	{
		uint8_t
			unknown680_33,
			velo,
			timeVelo,
			unknown683_00;
		uint16le
			time1,
			time2,
			unknown688_0000,
			time3,
			level0,
			level1,
			unknown696_0000,
			unknown698_0000,
			level2,
			unknown702_0001;
	};

	struct TVFEnv
	{
		uint8_t
			envDepth,
			velocityCurve,
			velo,
			timeVelo;
		uint16le
			unknown780_0000,
			time1,
			time2,
			time3,
			time4,
			unknown790_0000,
			level1,
			level2,
			sustain,
			level4;
	};

	struct TVAEnv
	{
		uint8_t
			timeVelo,
			unknown873_00;
		uint16le
			time1,
			time2,
			time3,
			time4,
			level1,
			level2,
			sustain;
	};

	struct LFO
	{
		uint8_t
			waveform,
			tempoSync,
			rateWithTempoSync,
			unknown939_0F;
		uint16le
			rate;
		uint8_t
			offset,  // -100 / 0 / +100
			delayOnRelease;
		uint16le
			delay;
		uint8_t
			unknown946_00,
			negativeFade;
		uint16le
			fade;
		uint8_t
			keyTrigger,
			pitchToLFO,
			tvfToLFO,
			tvaToLFO;
		std::array<uint8_t, 34> unknown;
	};
	
	struct LFOs
	{
		ToneVSTPrecomputed::LFO lfo1;  // 936, 1040, 1144, 1248
		ToneVSTPrecomputed::LFO lfo2;  // 988, 1092, 1196, 1300
	};

	struct EQ
	{
		uint16le
			lowGain,
			midGain,
			highGain,
			lowFreq,
			midFreq,
			highFreq;
		uint8_t
			midQ,
			eqEnabled,
			unknown1366_00,
			unknown1367_00;
	};


	std::array<uint8_t, 24> unknown112;  // 112
	std::array<Layer, 4> layer;          // 136, 148, 160, 172
	std::array<Common, 4> common;        // 184, 308, 432, 556
	std::array<PitchEnv, 4> pitchEnv;    // 680, 704, 728, 752
	std::array<TVFEnv, 4> tvfEnv;        // 776, 800, 824, 848
	std::array<TVAEnv, 4> tvaEnv;        // 872, 888, 904, 920
	std::array<LFOs, 4> lfo;             // 936, 1040, 1144, 1248
	std::array<EQ, 4> eq;                // 1352, 1368, 1384, 1400
	uint8_t unison;                      // 1416
	std::array<uint8_t, 199> theRest;    // 1417
};

struct ToneVST
{
	struct Common
	{
		uint8_t
			layerEnabled,
			layerSelected,
			velocityCurve,
			holdControl;
	};

	struct LFO
	{
		uint8_t
			waveform,
			tempoSync,  // Extended feature (0 = off, 1 = on)
			rate,
			rateWithTempoSync,  // Extended feature
			delay,
			fade,  // +50
			offset,
			keyTrigger;
	};

	struct WG
	{
		uint8_t
			waveformLSB,     // 0 = no waveform
			unknown1637_00,  // Maybe MSB of waveform?
			unknown1638_00,
			gain,         // Extended feature
			pitchCoarse,  // +48
			pitchFine,    // +50
			pitchRandom,
			keyFollow,
			benderSwitch,
			aTouchBend,
			lfo1Sens,       // +50
			lfo2Sens,       // +50
			leverSens,      // +50
			aTouchModSens;  // +50
	};

	struct PitchEnv
	{
		uint8_t
			velo,      // +50
			timeVelo,  // +50
			timeKF,    // +10
			level0,    // +50
			level1,    // +50
			level2,    // +50
			time1,
			time2,
			time3;
	};

	struct TVF
	{
		uint8_t
			filterMode,
			cutoffFreq,
			resonance,
			keyFollow,
			aTouchSens,  // +50
			lfoSelect,
			lfoDepth,  // +50
			envDepth;  // +50
	};

	struct TVFEnv
	{
		uint8_t
			velo,      // +50
			timeVelo,  // +50
			timeKF,    // +10
			level1,
			level2,
			sustainLevel,
			level4,
			time1,
			time2,
			time3,
			time4;
	};

	struct TVA
	{
		uint8_t
			biasDirection,
			biasPoint,
			biasLevel,  // +10
			level,
			aTouchSens,  // +50
			lfoSelect,
			lfoDepth;  // +50
	};

	struct TVAEnv
	{
		uint8_t
			velo,      // +50
			timeVelo,  // +50
			timeKF,    // +10
			level1,
			level2,
			sustainLevel,
			time1,
			time2,
			time3,
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
	uint8_t padding;
};

struct PatchVST
{
	struct ZenHeader
	{
		uint16le
			modelID1,  // 3
			modelID2,  // 5
			rating,
			unknown_64;
		std::array<char, 8> empty;
	};

	struct CommonPrecomputed
	{
		uint8_t
			patchCategory,  // Zenology patch category. Can be seen when loading the .bin file into Zenology but not in the JD-800 plugin.
			unknown1_00,
			unknown2_00,
			unknown3_00,
			patchCommonLevel,
			unknown5_00,
			unknown6_00,
			unknown7_00,
			unknown8_00,
			unknown9_00,
			unknown10_00,
			unknown11_00,
			unknown12_00,
			unknown13_00,
			soloSW,
			soloLegato,
			unknown16_0D,
			portamentoSW,
			portamentoMode,
			unknown19_00,
			unknown20_00,
			unknown21_00;
		uint16le
			portamentoTime;
		uint8_t
			benderRangeUp,
			benderRangeDown;
		std::array<uint8_t, 22> unknown;
	};

	struct EffectsGroupA
	{
		uint8_t
			unknown48_5D,
			groupAenabled,
			unknown50_7F,
			unknown51_7F;
		std::array<uint8_t, 12> unknown52;
		uint16le
			groupAsequence,
			distortionEnabled,
			distortionType,
			distortionDrive,
			distortionLevel,
			phaserEnabled,
			phaserManual,
			phaserRate,
			phaserDepth,
			phaserResonance,
			phaserMix,
			spectrumEnabled,
			spectrumBand1,
			spectrumBand2,
			spectrumBand3,
			spectrumBand4,
			spectrumBand5,
			spectrumBand6,
			spectrumBandwidth,
			enhancerEnabled,
			enhancerSens,
			enhancerMix,
			panningGroupA,       // Extended feature, JD-08 only
			effectsLevelGroupA;  // Extended feature (0...127)
	};

	struct EffectsGroupB
	{
		uint8_t
			groupBsequence,
			delayEnabled,
			delayCenterTempoSync,  // Extended feature (0 = off, 1 = on)
			delayCenterTap,
			delayCenterTapWithSync,  // Extended feature
			delayLeftTempoSync,      // Extended feature (0 = off, 1 = on)
			delayLeftTap,
			delayLeftTapWithSync,  // Extended feature
			delayRightTempoSync,   // Extended feature (0 = off, 1 = on)
			delayRightTap,
			delayRightTapWithSync,  // Extended feature
			delayCenterLevel,
			delayLeftLevel,
			delayRightLevel,
			delayFeedback,

			chorusEnabled,
			chorusRate,
			chorusDepth,
			chorusDelayTime,
			chorusFeedback,
			chorusLevel,

			reverbEnabled,
			reverbType,
			reverbPreDelay,
			reverbEarlyRefLevel,
			reverbHFDamp,
			reverbTime,
			reverbLevel,

			effectsBalanceGroupB,
			effectsLevelGroupB,  // Extended feature (0...127)

			padding1,
			padding2;
	};

	struct Common
	{
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
			portamentoTime;
	};

	struct EQ
	{
		// Extended values, frequencies in Hz, gain in centiBel (+24 = 240), Q * 10 (16 = 160)!
		uint8_t
			midQ;
		uint16le
			lowFreq,
			midFreq,
			highFreq,
			lowGain,
			midGain,
			highGain;
		uint8_t
			eqEnabled;
	};

	ZenHeader zenHeader;  // Not present in hardware patches
	std::array<char, 16> name;

	CommonPrecomputed commonPrecomputed;  // 0
	EffectsGroupA effectsGroupA;          // 48
	ToneVSTPrecomputed tonesPrecomputed;  // 112
	std::array<ToneVST, 4> tone;          // 1616, 1696, 1776, 1856
	EffectsGroupB effectsGroupB;          // 1936
	Common common;                        // 1968
	EQ eq;                                // 1985
	uint8_t unison;                       // 1999, Extended feature

	std::array<char, 20320> empty;  // Surely we will need patches to be able to grow to ten times their current size in the future!

	static constexpr ZenHeader DEFAULT_ZEN_HEADER = { 3, 5, 0, 100, {} };
};
