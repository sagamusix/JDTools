// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 - 2024 by Johannes Schultz
// License: BSD 3-clause

#include "JD-800.hpp"
#include "JD-08.hpp"
#include "PrecomputedTablesVST.hpp"
#include "Utils.hpp"

#include <iostream>

template<typename T, size_t N>
static T SignedTable(const T (&table)[N], int8_t offset)
{
	auto value = SafeTable(table, static_cast<uint8_t>(std::abs(offset)));
	if (offset < 0)
		return -value;
	else
		return value;
}

static uint8_t ConvertPitchEnvLevel(uint8_t value)
{
	int converted = value - 50;
	// Real JD-800 has a pitch envelope range of -3 octaves to +1 octave... ZenCore only supports +/-1 octave.
	if (converted <= -46)
	{
		converted = -50;
	}
	else if (converted < 0)
	{
		converted = (converted * 50 - 23) / 46;
	}

	return static_cast<uint8_t>(converted);
}

static void ConvertTone800ToVST(const Tone800 &t800, const bool enabled, const bool selected, ToneVST &tVST)
{
	tVST.common.layerEnabled = enabled;
	tVST.common.layerSelected = selected;
	tVST.common.velocityCurve = t800.common.velocityCurve;
	tVST.common.holdControl = t800.common.holdControl;

	tVST.lfo1.waveform = t800.lfo1.waveform;
	tVST.lfo1.tempoSync = 0;  // Extended feature
	tVST.lfo1.rate = t800.lfo1.rate;
	tVST.lfo1.rateWithTempoSync = 6;  // Extended feature
	tVST.lfo1.delay = t800.lfo1.delay;
	tVST.lfo1.fade = t800.lfo1.fade - 50;
	tVST.lfo1.offset = 2 - t800.lfo1.offset;
	tVST.lfo1.keyTrigger = t800.lfo1.keyTrigger;

	tVST.lfo2.waveform = t800.lfo2.waveform;
	tVST.lfo2.tempoSync = 0;  // Extended feature
	tVST.lfo2.rate = t800.lfo2.rate;
	tVST.lfo2.rateWithTempoSync = 6;  // Extended feature
	tVST.lfo2.delay = t800.lfo2.delay;
	tVST.lfo2.fade = t800.lfo2.fade - 50;
	tVST.lfo2.offset = 2 - t800.lfo2.offset;
	tVST.lfo2.keyTrigger = t800.lfo2.keyTrigger;

	if (t800.wg.waveSource != 0 && tVST.common.layerEnabled)
	{
		std::cerr << "LOSSY CONVERSION! Waveforms from ROM cards are not supported!" << std::endl;
	}
	tVST.wg.waveformLSB = (t800.wg.waveformLSB + 1) & 0x7F;
	tVST.wg.unknown1637_00 = 0;
	tVST.wg.unknown1638_00 = 0;
	tVST.wg.gain = 3;  // Extended feature
	tVST.wg.pitchCoarse = t800.wg.pitchCoarse - 48;
	tVST.wg.pitchFine= t800.wg.pitchFine - 50;
	tVST.wg.pitchRandom = t800.wg.pitchRandom;
	if (tVST.wg.pitchRandom > 0 && tVST.wg.pitchRandom < 20)
	{
		tVST.wg.pitchRandom = 20;
		std::cerr << "LOSSY CONVERSION! Pitch Random values 1-19 do nothing, setting to 20 instead" << std::endl;
	}
	tVST.wg.keyFollow = t800.wg.keyFollow;
	tVST.wg.benderSwitch = t800.wg.benderSwitch;
	tVST.wg.aTouchBend = t800.wg.aTouchBend;
	tVST.wg.lfo1Sens = t800.wg.lfo1Sens - 50;
	tVST.wg.lfo2Sens = t800.wg.lfo2Sens - 50;
	tVST.wg.leverSens = t800.wg.leverSens - 50;
	tVST.wg.aTouchModSens = t800.wg.aTouchModSens - 50;

	// Why, Roland, why
	if (tVST.wg.waveformLSB == 88)
		tVST.wg.waveformLSB = 89;
	else if (tVST.wg.waveformLSB == 89)
		tVST.wg.waveformLSB = 88;
	else if (tVST.wg.waveformLSB == 23 || tVST.wg.waveformLSB == 28 || tVST.wg.waveformLSB == 35 || tVST.wg.waveformLSB == 36 || tVST.wg.waveformLSB == 37 || tVST.wg.waveformLSB == 50 || tVST.wg.waveformLSB == 102 || tVST.wg.waveformLSB == 107)
		tVST.wg.pitchCoarse += 12;
	else if (tVST.wg.waveformLSB == 48)
		tVST.wg.pitchCoarse -= 12;
	else if (tVST.wg.waveformLSB == 108)
		tVST.wg.pitchCoarse += 7;
	else if (tVST.wg.waveformLSB == 105)
		tVST.wg.pitchFine -= 50;

	if (static_cast<int8_t>(tVST.wg.pitchFine) < -50)
	{
		tVST.wg.pitchFine += 100;
		tVST.wg.pitchCoarse--;
	}
	if (static_cast<int8_t>(tVST.wg.pitchCoarse) < -48)
	{
		tVST.wg.pitchCoarse = static_cast<uint8_t>(-48);
		if (tVST.common.layerEnabled)
			std::cerr << "LOSSY CONVERSION! Tone coarse pitch too low (maybe due to waveform transposition)" << std::endl;
	}
	else if (static_cast<int8_t>(tVST.wg.pitchCoarse) > 48)
	{
		tVST.wg.pitchCoarse = 48;
		if (tVST.common.layerEnabled)
			std::cerr << "LOSSY CONVERSION! Tone coarse pitch too high (maybe due to waveform transposition)" << std::endl;
	}

	tVST.pitchEnv.velo = t800.pitchEnv.velo - 50;
	tVST.pitchEnv.timeVelo = t800.pitchEnv.timeVelo - 50;
	tVST.pitchEnv.timeKF = t800.pitchEnv.timeKF - 10;
	tVST.pitchEnv.level0 = ConvertPitchEnvLevel(t800.pitchEnv.level0);
	tVST.pitchEnv.level1 = ConvertPitchEnvLevel(t800.pitchEnv.level1);
	tVST.pitchEnv.level2 = ConvertPitchEnvLevel(t800.pitchEnv.level2);
	tVST.pitchEnv.time1 = t800.pitchEnv.time1;
	tVST.pitchEnv.time2 = t800.pitchEnv.time2;
	tVST.pitchEnv.time3 = t800.pitchEnv.time3;
	if (t800.pitchEnv.level0 < 4 || t800.pitchEnv.level1 < 4 || t800.pitchEnv.level2 < 4)
	{
		std::cerr << "LOSSY CONVERSION! Pitch envelope cannot go lower than one octave" << std::endl;
	}

	tVST.tvf.filterMode = 2 - t800.tvf.filterMode;
	tVST.tvf.cutoffFreq = t800.tvf.cutoffFreq;
	tVST.tvf.resonance = t800.tvf.resonance;
	tVST.tvf.keyFollow = t800.tvf.keyFollow;
	tVST.tvf.aTouchSens = t800.tvf.aTouchSens - 50;
	tVST.tvf.lfoSelect = t800.tvf.lfoSelect;
	tVST.tvf.lfoDepth = t800.tvf.lfoDepth - 50;
	tVST.tvf.envDepth = t800.tvf.envDepth - 50;

	tVST.tvfEnv.velo = t800.tvfEnv.velo - 50;
	tVST.tvfEnv.timeVelo = t800.tvfEnv.timeVelo - 50;
	tVST.tvfEnv.timeKF = t800.tvfEnv.timeKF - 10;
	tVST.tvfEnv.level1 = t800.tvfEnv.level1;
	tVST.tvfEnv.level2 = t800.tvfEnv.level2;
	tVST.tvfEnv.sustainLevel = t800.tvfEnv.sustainLevel;
	tVST.tvfEnv.level4 = t800.tvfEnv.level4;
	tVST.tvfEnv.time1 = t800.tvfEnv.time1;
	tVST.tvfEnv.time2 = t800.tvfEnv.time2;
	tVST.tvfEnv.time3 = t800.tvfEnv.time3;
	tVST.tvfEnv.time4 = t800.tvfEnv.time4;

	tVST.tva.biasDirection = t800.tva.biasDirection;
	tVST.tva.biasPoint = t800.tva.biasPoint;
	tVST.tva.biasLevel = t800.tva.biasLevel - 10;
	tVST.tva.level = t800.tva.level;
	tVST.tva.aTouchSens = t800.tva.aTouchSens - 50;
	tVST.tva.lfoSelect = t800.tva.lfoSelect;
	tVST.tva.lfoDepth = t800.tva.lfoDepth - 50;

	tVST.tvaEnv.velo = t800.tvaEnv.velo - 50;
	tVST.tvaEnv.timeVelo = t800.tvaEnv.timeVelo - 50;
	tVST.tvaEnv.timeKF = t800.tvaEnv.timeKF - 10;
	tVST.tvaEnv.level1 = t800.tvaEnv.level1;
	tVST.tvaEnv.level2 = t800.tvaEnv.level2;
	tVST.tvaEnv.sustainLevel = t800.tvaEnv.sustainLevel;
	tVST.tvaEnv.time1 = t800.tvaEnv.time1;
	tVST.tvaEnv.time2 = t800.tvaEnv.time2;
	tVST.tvaEnv.time3 = t800.tvaEnv.time3;
	tVST.tvaEnv.time4 = t800.tvaEnv.time4;

	tVST.padding = 0;
}

static void FillPrecomputedLFO(const ToneVST::LFO &tLFO, const uint8_t lfoIndex, const ToneVST &tVST, ToneVSTPrecomputed::LFO &lfo)
{
	lfo.waveform = SafeTable(LFOWaveforms, tLFO.waveform);
	lfo.tempoSync = tLFO.tempoSync;
	lfo.rateWithTempoSync = 6;  // Standard value used in preset banks
	lfo.unknown939_0F = 15;
	lfo.rate = SafeTable(LFORates, tLFO.rate);
	lfo.offset = static_cast<int8_t>(tLFO.offset - 1) * 100;
	if (tLFO.delay == 101)
		lfo.delayOnRelease = 2;
	lfo.delay = SafeTable(LFODelay, tLFO.delay);
	lfo.negativeFade = (static_cast<int8_t>(tLFO.fade) < 0) ? 1 : 0;
	lfo.fade = SafeTable(LFOFade, static_cast<uint8_t>(std::abs(static_cast<int8_t>(tLFO.fade))));
	lfo.keyTrigger = tLFO.keyTrigger;
	lfo.pitchToLFO = SignedTable(PitchToLFOSens, (lfoIndex == 0) ? tVST.wg.lfo1Sens : tVST.wg.lfo2Sens);
	if (tVST.tvf.lfoSelect == lfoIndex)
		lfo.tvfToLFO = SignedTable(TVFToLFOSens, tVST.tvf.lfoDepth);
	if (tVST.tva.lfoSelect == lfoIndex)
		lfo.tvaToLFO = SignedTable(TVAToLFOSens, tVST.tva.lfoDepth);
	for (size_t i = 0; i < 34; i++)
		lfo.unknown[i] = (i < 18) ? 0 : 1;

}

static void FillPrecomputedToneVST(const ToneVST &tVST, const PatchVST &pVST, ToneVSTPrecomputed &tpVST, const uint8_t tone)
{
	const uint8_t LowKeys[] = { pVST.common.keyRangeLowA, pVST.common.keyRangeLowB, pVST.common.keyRangeLowC, pVST.common.keyRangeLowD };
	const uint8_t HighKeys[] = { pVST.common.keyRangeHighA, pVST.common.keyRangeHighB, pVST.common.keyRangeHighC, pVST.common.keyRangeHighD };

	ToneVSTPrecomputed::Layer &layer = tpVST.layer[tone];
	layer.layerEnabled = tVST.common.layerEnabled;
	layer.lowKey = LowKeys[tone];
	layer.highKey = HighKeys[tone];
	layer.lowVelocity = 1;
	layer.highVelocity = 127;

	ToneVSTPrecomputed::Common &common = tpVST.common[tone];
	common.tvaLevel = tVST.tva.level;
	common.pitchCoarse = tVST.wg.pitchCoarse;
	common.pitchFine = tVST.wg.pitchFine;
	common.pitchRandom = SafeTable(PitchRandom, tVST.wg.pitchRandom);
	common.unknown194_01 = 1;
	common.unknown197_0C = 12;
	common.benderSwitch = tVST.wg.benderSwitch;
	common.unknown203_01 = 1;
	common.holdControl = tVST.common.holdControl;
	common.unknown206_01 = 1;
	common.unknown208_FD = 253;
	common.unknown209_2A = 42;
	common.waveformLSB = tVST.wg.waveformLSB;
	common.gain = 3;
	common.unknown216_01 = 1;

	common.pitchKeyFollow = SafeTable(PitchKF, tVST.wg.keyFollow);
	common.filterType = tVST.tvf.filterMode + 1;
	common.cutoff = SafeTable(Cutoff, tVST.tvf.cutoffFreq);
	if (tVST.tvf.keyFollow < 10)
		common.filterKeyFollow = static_cast<int8_t>(tVST.tvf.keyFollow - 10) * 10;
	else
		common.filterKeyFollow = (tVST.tvf.keyFollow - 10) * 5;
	common.velocityCurveTVF = tVST.common.velocityCurve + 1;
	common.resonance = SafeTable(Resonance, tVST.tvf.resonance);

	common.tvaBiasLevel = SafeTable(BiasLevel, tVST.tva.biasLevel + 10);
	common.tvaBiasPoint = tVST.tva.biasPoint;
	if (tVST.tva.biasDirection < 2)
		common.tvaBiasDirection = tVST.tva.biasDirection ^ 1;
	else
		common.tvaBiasDirection = 2;
	common.velocityCurveTVA = tVST.common.velocityCurve + 1;
	common.tvaVelo = SignedTable(EnvVelo, tVST.tvaEnv.velo);
	common.pitchTimeKF = static_cast<uint8_t>(static_cast<int8_t>(tVST.pitchEnv.timeKF) * 10);
	common.tvfTimeKF = static_cast<uint8_t>(static_cast<int8_t>(tVST.tvfEnv.timeKF) * 10);
	common.tvaTimeKF = static_cast<uint8_t>(static_cast<int8_t>(tVST.tvaEnv.timeKF) * 10);
	common.unknown241_0A = 10;    // ControlSource routing won't work if this has any other value
	common.cs1.source = 96;       // Aftertouch
	common.cs1.destination1 = 1;  // Pitch
	common.cs1.destination2 = 1;  // Pitch
	common.cs1.destination3 = 1;  // Pitch
	if (tVST.wg.aTouchBend)
	{
		common.cs1.depth1 = SignedTable(ATouchBend, static_cast<int8_t>(pVST.common.aTouchBend) - 14);
		common.cs1.depth2 = (pVST.common.aTouchBend < 2) ? -63 : 0;
		common.cs1.depth3 = (pVST.common.aTouchBend < 1) ? -63 : 0;
	}
	
	common.cs2.source = 1;        // Lever
	common.cs2.destination1 = 8;  // P-LFO 1 (annoyingly off-by-one compared to modulation matrix implementations in JD-990, XV-5080 and probably any other Roland hardware synth)
	common.cs2.destination2 = 9;  // P-LFO 2 (ditto)
	const int8_t leverSens = SignedTable(LeverSens, tVST.wg.leverSens);
	if (leverSens >= 0)
		common.cs2.depth1 = static_cast<uint8_t>(leverSens);
	else
		common.cs2.depth2 = static_cast<uint8_t>(-leverSens);

	common.cs3.source = 96;       // Aftertouch
	common.cs3.destination1 = 8;  // P-LFO 1
	common.cs3.destination2 = 9;  // P-LFO 2
	const int8_t aTouchModSens = SignedTable(LeverSens, tVST.wg.aTouchModSens);
	if (aTouchModSens > 0)
		common.cs3.depth1 = static_cast<uint8_t>(aTouchModSens);
	else
		common.cs3.depth2 = static_cast<uint8_t>(-aTouchModSens);

	common.cs4.source = 96;        // Aftertouch
	common.cs4.destination1 = 2;   // Cutoff
	common.cs4.depth1 = static_cast<uint8_t>(SignedTable(AtouchSensTVF, tVST.tvf.aTouchSens));
	common.cs4.destination2 = 4;   // Level
	common.cs4.depth2 = static_cast<uint8_t>(SafeTable(AtouchSensTVA, tVST.tva.aTouchSens + 50));

	common.unknown293_64 = 0x64;

	ToneVSTPrecomputed::PitchEnv &pitchEnv = tpVST.pitchEnv[tone];
	pitchEnv.unknown680_33 = 0x33;
	pitchEnv.velo = SignedTable(EnvVelo, tVST.pitchEnv.velo);
	pitchEnv.timeVelo = SignedTable(EnvVelo, tVST.pitchEnv.timeVelo);
	pitchEnv.time1 = SafeTable(PitchEnvTime, tVST.pitchEnv.time1);
	pitchEnv.time2 = SafeTable(PitchEnvTime, tVST.pitchEnv.time2);
	pitchEnv.time3 = SafeTable(PitchEnvTime, tVST.pitchEnv.time3);
	pitchEnv.level0 = SignedTable(PitchEnvLevels, tVST.pitchEnv.level0);
	pitchEnv.level1 = SignedTable(PitchEnvLevels, tVST.pitchEnv.level1);
	pitchEnv.level2 = SignedTable(PitchEnvLevels, tVST.pitchEnv.level2);
	pitchEnv.unknown702_0001 = 1;

	ToneVSTPrecomputed::TVFEnv &tvfEnv = tpVST.tvfEnv[tone];
	tvfEnv.envDepth = SignedTable(EnvVelo, tVST.tvf.envDepth);
	tvfEnv.velocityCurve = tVST.common.velocityCurve + 1;
	tvfEnv.velo = SignedTable(EnvVelo, tVST.tvfEnv.velo);
	tvfEnv.timeVelo = SignedTable(EnvVelo, tVST.tvfEnv.timeVelo);
	tvfEnv.time1 = SafeTable(TVFEnvTime1, tVST.tvfEnv.time1);
	tvfEnv.time2 = SafeTable(TVFEnvTime2, tVST.tvfEnv.time2);
	tvfEnv.time3 = SafeTable(TVFEnvTime3, tVST.tvfEnv.time3);
	tvfEnv.time4 = SafeTable(TVFEnvTime4, tVST.tvfEnv.time4);
	tvfEnv.level1 = SafeTable(TVFEnvLevels, tVST.tvfEnv.level1);
	tvfEnv.level2 = SafeTable(TVFEnvLevels, tVST.tvfEnv.level2);
	tvfEnv.sustain = SafeTable(TVFEnvLevels, tVST.tvfEnv.sustainLevel);
	tvfEnv.level4 = SafeTable(TVFEnvLevels, tVST.tvfEnv.level4);

	ToneVSTPrecomputed::TVAEnv &tvaEnv = tpVST.tvaEnv[tone];
	tvaEnv.timeVelo = SignedTable(EnvVelo, tVST.tvaEnv.timeVelo);
	tvaEnv.time1 = SafeTable(TVAEnvTime1, tVST.tvaEnv.time1);
	tvaEnv.time2 = SafeTable(TVAEnvTime2, tVST.tvaEnv.time2);
	tvaEnv.time3 = SafeTable(TVAEnvTime34, tVST.tvaEnv.time3);
	tvaEnv.time4 = SafeTable(TVAEnvTime34, tVST.tvaEnv.time4);
	tvaEnv.level1 = SafeTable(TVAEnvLevels, tVST.tvaEnv.level1);
	tvaEnv.level2 = SafeTable(TVAEnvLevels, tVST.tvaEnv.level2);
	tvaEnv.sustain = SafeTable(TVAEnvLevels, tVST.tvaEnv.sustainLevel);

	FillPrecomputedLFO(tVST.lfo1, 0, tVST, tpVST.lfo[tone].lfo1);
	FillPrecomputedLFO(tVST.lfo2, 1, tVST, tpVST.lfo[tone].lfo2);

	ToneVSTPrecomputed::EQ &eq = tpVST.eq[tone];
	eq.lowGain = pVST.eq.lowGain;
	eq.midGain = pVST.eq.midGain;
	eq.highGain = pVST.eq.highGain;
	eq.lowFreq = pVST.eq.lowFreq;
	eq.midFreq = pVST.eq.midFreq;
	eq.highFreq = pVST.eq.highFreq;
	eq.midQ = pVST.eq.midQ;
	eq.eqEnabled = pVST.eq.eqEnabled;
}

void ConvertPatch800ToVST(const Patch800 &p800, PatchVST &pVST)
{
	pVST.zenHeader = { 3, 5, 0, 100, {} };
	pVST.name = p800.common.name;

	pVST.effectsGroupA.unknown48_5D = 93;
	pVST.effectsGroupA.groupAenabled = 1;
	pVST.effectsGroupA.unknown50_7F = 127;
	pVST.effectsGroupA.unknown51_7F = 127;
	pVST.effectsGroupA.unknown52.fill(0);

	static constexpr uint8_t DistortionPos[] = { 0, 0, 0, 0, 0, 0, 1, 1, 3, 2, 2, 3, 2, 3, 1, 1, 3, 2, 3, 2, 2, 3, 1, 1 };
	static constexpr uint8_t PhaserPos[] = { 1, 1, 3, 2, 2, 3, 0, 0, 0, 0, 0, 0, 1, 1, 3, 2, 2, 3, 1, 1, 3, 2, 2, 3 };
	static constexpr uint8_t SpectrumPos[] = { 2, 3, 1, 1, 3, 2, 2, 3, 21, 1, 3, 2, 0, 0, 0, 0, 0, 0, 2, 3, 1, 1, 3, 2 };
	static constexpr uint8_t EnhancerPos[] = { 3, 2, 2, 3, 1, 1, 3, 2, 2, 3, 1, 1, 3, 2, 2, 3, 1, 1, 0, 0, 0, 0, 0, 0 };
	const uint8_t BlockEnabledA[] = { p800.effect.groupAblockSwitch1, p800.effect.groupAblockSwitch2, p800.effect.groupAblockSwitch3, p800.effect.groupAblockSwitch4 };

	pVST.effectsGroupA.groupAsequence = p800.effect.groupAsequence;
	pVST.effectsGroupA.distortionEnabled = BlockEnabledA[DistortionPos[p800.effect.groupAsequence % 24u]];
	pVST.effectsGroupA.distortionType = p800.effect.distortionType;
	pVST.effectsGroupA.distortionDrive = p800.effect.distortionDrive;
	pVST.effectsGroupA.distortionLevel = p800.effect.distortionLevel;
	pVST.effectsGroupA.phaserEnabled = BlockEnabledA[SafeTable(PhaserPos, p800.effect.groupAsequence)];
	pVST.effectsGroupA.phaserManual = p800.effect.phaserManual;
	pVST.effectsGroupA.phaserRate = p800.effect.phaserRate;
	pVST.effectsGroupA.phaserDepth = p800.effect.phaserDepth;
	pVST.effectsGroupA.phaserResonance = p800.effect.phaserResonance;
	pVST.effectsGroupA.phaserMix = p800.effect.phaserMix;
	pVST.effectsGroupA.spectrumEnabled = BlockEnabledA[SafeTable(SpectrumPos, p800.effect.groupAsequence)];
	pVST.effectsGroupA.spectrumBand1 = p800.effect.spectrumBand1;
	pVST.effectsGroupA.spectrumBand2 = p800.effect.spectrumBand2;
	pVST.effectsGroupA.spectrumBand3 = p800.effect.spectrumBand3;
	pVST.effectsGroupA.spectrumBand4 = p800.effect.spectrumBand4;
	pVST.effectsGroupA.spectrumBand5 = p800.effect.spectrumBand5;
	pVST.effectsGroupA.spectrumBand6 = p800.effect.spectrumBand6;
	pVST.effectsGroupA.spectrumBandwidth = p800.effect.spectrumBandwidth;
	pVST.effectsGroupA.enhancerEnabled = BlockEnabledA[SafeTable(EnhancerPos, p800.effect.groupAsequence)];
	pVST.effectsGroupA.enhancerSens = p800.effect.enhancerSens;
	pVST.effectsGroupA.enhancerMix = p800.effect.enhancerMix;
	pVST.effectsGroupA.panningGroupA = 64;
	pVST.effectsGroupA.effectsLevelGroupA = 127;  // Extended feature

	ConvertTone800ToVST(p800.toneA, p800.common.layerTone & 1, p800.common.activeTone & 1, pVST.tone[0]);
	ConvertTone800ToVST(p800.toneB, p800.common.layerTone & 2, p800.common.activeTone & 2, pVST.tone[1]);
	ConvertTone800ToVST(p800.toneC, p800.common.layerTone & 4, p800.common.activeTone & 4, pVST.tone[2]);
	ConvertTone800ToVST(p800.toneD, p800.common.layerTone & 8, p800.common.activeTone & 8, pVST.tone[3]);

	static constexpr uint8_t ChorusPos[] = { 0, 0, 1, 2, 1, 2 };
	static constexpr uint8_t DelayPos[] = { 1, 2, 0, 0, 2, 1 };
	static constexpr uint8_t ReverbPos[] = { 2, 1, 2, 1, 0, 0 };
	const uint8_t BlockEnabledB[] = { p800.effect.groupBblockSwitch1, p800.effect.groupBblockSwitch2, p800.effect.groupBblockSwitch3 };

	pVST.effectsGroupB.groupBsequence = p800.effect.groupBsequence;
	pVST.effectsGroupB.delayEnabled = BlockEnabledB[SafeTable(DelayPos, p800.effect.groupBsequence)];
	pVST.effectsGroupB.delayCenterTempoSync = 0;  // Extended feature
	pVST.effectsGroupB.delayCenterTap = p800.effect.delayCenterTap;
	pVST.effectsGroupB.delayCenterTapWithSync = 0;  // Extended feature
	pVST.effectsGroupB.delayLeftTempoSync = 0;      // Extended feature
	pVST.effectsGroupB.delayLeftTap = p800.effect.delayLeftTap;
	pVST.effectsGroupB.delayLeftTapWithSync = 0;  // Extended feature
	pVST.effectsGroupB.delayRightTempoSync = 0;   // Extended feature
	pVST.effectsGroupB.delayRightTap = p800.effect.delayRightTap;
	pVST.effectsGroupB.delayRightTapWithSync = 0;  // Extended feature
	pVST.effectsGroupB.delayCenterLevel = p800.effect.delayCenterLevel;
	pVST.effectsGroupB.delayLeftLevel = p800.effect.delayLeftLevel;
	pVST.effectsGroupB.delayRightLevel = p800.effect.delayRightLevel;
	pVST.effectsGroupB.delayFeedback = p800.effect.delayFeedback;

	pVST.effectsGroupB.chorusEnabled = BlockEnabledB[SafeTable(ChorusPos, p800.effect.groupBsequence)];
	pVST.effectsGroupB.chorusRate = p800.effect.chorusRate;
	pVST.effectsGroupB.chorusDepth = p800.effect.chorusDepth;
	pVST.effectsGroupB.chorusDelayTime = p800.effect.chorusDelayTime;
	pVST.effectsGroupB.chorusFeedback = p800.effect.chorusFeedback;
	pVST.effectsGroupB.chorusLevel = p800.effect.chorusLevel;

	pVST.effectsGroupB.reverbEnabled = BlockEnabledB[SafeTable(ReverbPos, p800.effect.groupBsequence)];
	pVST.effectsGroupB.reverbType = p800.effect.reverbType;
	pVST.effectsGroupB.reverbPreDelay = p800.effect.reverbPreDelay;
	pVST.effectsGroupB.reverbEarlyRefLevel = p800.effect.reverbEarlyRefLevel;
	pVST.effectsGroupB.reverbHFDamp = p800.effect.reverbHFDamp;
	pVST.effectsGroupB.reverbTime = p800.effect.reverbTime;
	pVST.effectsGroupB.reverbLevel = p800.effect.reverbLevel;

	pVST.effectsGroupB.effectsBalanceGroupB = p800.effect.effectsBalanceGroupB;
	pVST.effectsGroupB.effectsLevelGroupB = 127;  // Extended feature

	pVST.effectsGroupB.padding1 = 0;
	pVST.effectsGroupB.padding2 = 0;

	pVST.common.patchLevel = p800.common.patchLevel;
	pVST.common.keyRangeLowA = p800.common.keyRangeLowA;
	pVST.common.keyRangeHighA = p800.common.keyRangeHighA;
	pVST.common.keyRangeLowB = p800.common.keyRangeLowB;
	pVST.common.keyRangeHighB = p800.common.keyRangeHighB;
	pVST.common.keyRangeLowC = p800.common.keyRangeLowC;
	pVST.common.keyRangeHighC = p800.common.keyRangeHighC;
	pVST.common.keyRangeLowD = p800.common.keyRangeLowD;
	pVST.common.keyRangeHighD = p800.common.keyRangeHighD;
	pVST.common.benderRangeDown = p800.common.benderRangeDown;
	pVST.common.benderRangeUp = p800.common.benderRangeUp;
	pVST.common.aTouchBend = p800.common.aTouchBend;
	pVST.common.soloSW = p800.common.soloSW;
	pVST.common.soloLegato = p800.common.soloLegato;
	pVST.common.portamentoSW = p800.common.portamentoSW;
	pVST.common.portamentoMode = p800.common.portamentoMode;
	pVST.common.portamentoTime = p800.common.portamentoTime;

	pVST.eq.midQ = SafeTable(EQMidQ, p800.eq.midQ);
	pVST.eq.lowFreq = SafeTable(EQLowFreq, p800.eq.lowFreq);
	pVST.eq.midFreq = SafeTable(EQMidFreq, p800.eq.midFreq);
	pVST.eq.highFreq = SafeTable(EQHighFreq, p800.eq.highFreq);
	pVST.eq.lowGain = static_cast<uint16_t>(static_cast<int8_t>(p800.eq.lowGain - 15) * 10);
	pVST.eq.midGain = static_cast<uint16_t>(static_cast<int8_t>(p800.eq.midGain - 15) * 10);
	pVST.eq.highGain = static_cast<uint16_t>(static_cast<int8_t>(p800.eq.highGain - 15) * 10);
	pVST.eq.eqEnabled = 1;
	
	pVST.unison = 0;  // Extended feature
	pVST.empty.fill(0);

	// Copy stuff to precomputed area
	pVST.commonPrecomputed = {};  // Lots of zeros to clear
	pVST.commonPrecomputed.patchCommonLevel = pVST.common.patchLevel;
	pVST.commonPrecomputed.soloSW = pVST.common.soloSW ^ 1u;
	pVST.commonPrecomputed.soloLegato = pVST.common.soloLegato;
	pVST.commonPrecomputed.unknown16_0D = 13;
	pVST.commonPrecomputed.portamentoSW = (pVST.common.soloSW && pVST.common.portamentoSW) ? 1 : 0;
	pVST.commonPrecomputed.portamentoMode = pVST.common.portamentoMode;
	pVST.commonPrecomputed.portamentoTime = SafeTable(PortaTime, pVST.common.portamentoTime);
	pVST.commonPrecomputed.benderRangeUp = pVST.common.benderRangeUp;
	pVST.commonPrecomputed.benderRangeDown = pVST.common.benderRangeDown;

	pVST.tonesPrecomputed = {};
	pVST.tonesPrecomputed.unknown112[20] = 1;
	for (uint8_t i = 0; i < 4; i++)
	{
		FillPrecomputedToneVST(pVST.tone[i], pVST, pVST.tonesPrecomputed, i);
	}

	pVST.tonesPrecomputed.unison = pVST.unison;

	pVST.tonesPrecomputed.theRest =
	{
		0x02, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x7F, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0xB0, 0x04, 0xB0, 0x04, 0x7F, 0x00, 0x7F, 0x00, 0x01, 0x64, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0xFF, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0xFF, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0xFF, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0xFF, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
}

std::vector<PatchVST> ConvertSetup800ToVST(const SpecialSetup800 &s800)
{
	std::vector<PatchVST> patches(64);

	Patch800 p800{};
	p800.common.patchLevel = 100;
	p800.common.keyRangeLowA = 0;
	p800.common.keyRangeHighA = 127;
	p800.common.keyRangeLowB = 0;
	p800.common.keyRangeHighB = 127;
	p800.common.keyRangeLowC = 0;
	p800.common.keyRangeHighC = 127;
	p800.common.keyRangeLowD = 0;
	p800.common.keyRangeHighD = 127;
	p800.common.benderRangeDown = s800.common.benderRangeDown;
	p800.common.benderRangeUp = s800.common.benderRangeUp;
	p800.common.aTouchBend = s800.common.aTouchBendSens;
	p800.common.soloSW = 0;
	p800.common.soloLegato = 0;
	p800.common.portamentoSW = 0;
	p800.common.portamentoMode = 0;
	p800.common.portamentoTime = 0;
	p800.common.layerTone = 1;
	p800.common.activeTone = 1;

	p800.eq.lowFreq = s800.eq.lowFreq;
	p800.eq.lowGain = s800.eq.lowGain;
	p800.eq.midFreq = s800.eq.midFreq;
	p800.eq.midQ = s800.eq.midQ;
	p800.eq.midGain = s800.eq.midGain;
	p800.eq.highFreq = s800.eq.highFreq;
	p800.eq.highGain = s800.eq.highGain;

	p800.midiTx.keyMode = 0;
	p800.midiTx.splitPoint = 36;
	p800.midiTx.lowerChannel = 1;
	p800.midiTx.upperChannel = 0;
	p800.midiTx.lowerProgramChange = 0;
	p800.midiTx.upperProgramChange = 0;
	p800.midiTx.holdMode = 2;
	p800.midiTx.dummy = 0;

	static constexpr std::array<const char *, 12> KeyNames = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

	for (uint8_t key = 0; key < 61; key++)
	{
		std::string name = "Drum Key " + (KeyNames[key % 12u] + std::string(1, '2' + key / 12));
		name.resize(p800.common.name.size(), ' ');
		std::copy(name.begin(), name.end(), p800.common.name.begin());

		p800.toneA = s800.keys[key].tone;

		ConvertPatch800ToVST(p800, patches[key]);
	}
	p800.common.name.fill(' ');
	p800.toneA = {};
	for (uint8_t key = 61; key < 64; key++)
	{
		ConvertPatch800ToVST(p800, patches[key]);
	}
	return patches;
}
