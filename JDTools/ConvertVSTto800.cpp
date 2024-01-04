// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 - 2024 by Johannes Schultz
// License: BSD 3-clause

#include "JD-800.hpp"
#include "JD-08.hpp"
#include "PrecomputedTablesVST.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string_view>

template<typename T, size_t N>
static bool MapToArrayIndex(const T value, const T (&values)[N], uint8_t &target)
{
	target = 0;
	uint8_t fallback = 0;
	for (const auto v : values)
	{
		if (v == value)
			return true;
		if (v < value)
			fallback = target;
		else if (value > values[fallback] && (value - values[fallback]) > (v - value))
			fallback = target;
		target++;
	}
	target = fallback;
	return false;
}

static double IndexToNoteDuration(const uint8_t index)
{
	static constexpr uint8_t Divisor[] = { 64, 64, 32, 32, 16, 32, 16, 8, 16, 8, 4, 8, 4, 2, 4, 2, 1, 2, 1, 1, 1, 1, 1 };
	static constexpr uint8_t NoteType[] = { 3, 1, 3, 1, 3, 2, 1, 3, 2, 1, 3, 2, 1, 3, 2, 1, 3, 2, 1, 3, 2, 1, 1 };
	double length = 1.0;
	if(index == 19 || index == 21)
		length = 2.0;
	else if(index == 22)
		length = 4.0;
	length /= SafeTable(Divisor, index);
	if (const auto type = SafeTable(NoteType, index); type == 2)
		length *= 1.5;
	else if (type == 3)
		length *= 2.0 / 3.0;

	length *= 2000.0;  // Assuming a tempo of 120 BPM, we now have the delay in milliseconds
	return length;
}

static uint8_t ApproximateDelayWithTempoSync(const uint8_t index)
{
	const double tapDuration = IndexToNoteDuration(index);
	int intOffset;
	double offset, factor;
	if (tapDuration < 5.5)
	{
		intOffset = 0;
		offset = 0.1;
		factor = 0.1;
	}
	else if (tapDuration < 11.0)
	{
		intOffset = 50;
		offset = 5.5;
		factor = 0.5;
	}
	else if (tapDuration < 50.0)
	{
		intOffset = 60;
		offset = 11.0;
		factor = 1.0;
	}
	else if (tapDuration < 220.0)
	{
		intOffset = 90;
		offset = 5.0;
		factor = 10.0;
	}
	else
	{
		intOffset = 106;
		offset = 220.0;
		factor = 20.0;
	}
	return static_cast<uint8_t>(std::clamp(intOffset + std::round((tapDuration - offset) / factor), 0.0, 125.0));
}

static uint8_t ApproximateLFORateWithTempoSync(const uint8_t index)
{
	const double noteDuration = IndexToNoteDuration(index);
	double bestDiff = 1'000'000.0;
	uint8_t bestIndex = 0;
	for (uint8_t i = 0; i < std::size(LFORates); i++)
	{
		const double rateDuration = 40000.0 * std::pow(2.0, LFORates[i] / -80.0 + 1.0);
		const auto diff = std::abs(rateDuration - noteDuration);
		if (diff < bestDiff)
		{
			bestDiff = diff;
			bestIndex = i;
		}
	}
	return bestIndex;
}

template<typename T, size_t N>
static void ConvertEQBand(const T(&freqTable)[N], uint8_t &freq, uint8_t &gain, uint16_t srcFreq, int16_t srcGain, const bool enabled, const std::string_view name)
{
	if (!MapToArrayIndex(srcFreq, freqTable, freq) && srcFreq != 0 && enabled)
		std::cerr << "LOSSY CONVERSION! Unsupported EQ " << name << " frequency value: " << srcFreq << " Hz, changing to " << freqTable[freq] << " Hz" << std::endl;

	gain = static_cast<uint8_t>(enabled ? std::clamp(srcGain / 10, -15, 15) + 15 : 0);

	if ((srcGain < -150 || srcGain > 150) && enabled)
		std::cerr << "LOSSY CONVERSION! Out-of-range EQ " << name << " gain value: " << srcGain * 0.1f << " dB" << std::endl;
	else if ((srcGain % 10) && enabled)
		std::cerr << "LOSSY CONVERSION! Truncating EQ " << name << " gain fractional precision: " << srcGain * 0.1f << " dB" << std::endl;
}

static uint8_t ConvertPitchEnvLevel(uint8_t value)
{
	int converted = static_cast<int8_t>(value);
	// Real JD-800 has a pitch envelope range of -3 octaves to +1 octave... ZenCore only supports +/-1 octave.
	if (converted < 0)
	{
		converted = (converted * 46 - 25) / 50;
	}

	return static_cast<uint8_t>(converted + 50);
}

static void ConvertToneVSTTo800(const ToneVST &tVST, Tone800 &t800)
{
	if (tVST.wg.gain != 3 && tVST.common.layerEnabled)
		std::cerr << "LOSSY CONVERSION! Tone uses gain != 0 dB: " << ((static_cast<int>(tVST.wg.gain) - 3) * 6) << " dB" << std::endl;

	t800.common.velocityCurve = tVST.common.velocityCurve;
	t800.common.holdControl = tVST.common.holdControl;

	if (tVST.lfo1.tempoSync && tVST.common.layerEnabled)
		std::cerr << "LOSSY CONVERSION! Tone LFO1 uses tempo sync, approximating LFO rate @ 120 BPM" << std::endl;
	t800.lfo1.rate = tVST.lfo1.tempoSync ? ApproximateLFORateWithTempoSync(tVST.lfo1.rateWithTempoSync) : tVST.lfo1.rate;
	t800.lfo1.delay = tVST.lfo1.delay;
	t800.lfo1.fade = tVST.lfo1.fade + 50;
	t800.lfo1.waveform = tVST.lfo1.waveform;
	t800.lfo1.offset = (2 - tVST.lfo1.offset) % 3;
	t800.lfo1.keyTrigger = tVST.lfo1.keyTrigger;

	if (tVST.lfo2.tempoSync && tVST.common.layerEnabled)
		std::cerr << "LOSSY CONVERSION! Tone LFO2 uses tempo sync, approximating LFO rate @ 120 BPM" << std::endl;
	t800.lfo2.rate = tVST.lfo2.tempoSync ? ApproximateLFORateWithTempoSync(tVST.lfo2.rateWithTempoSync) : tVST.lfo2.rate;
	t800.lfo2.delay = tVST.lfo2.delay;
	t800.lfo2.fade = tVST.lfo2.fade + 50;
	t800.lfo2.waveform = tVST.lfo2.waveform;
	t800.lfo2.offset = (2 - tVST.lfo2.offset) % 3;
	t800.lfo2.keyTrigger = tVST.lfo2.keyTrigger;

	t800.wg.waveSource = 0;
	t800.wg.waveformMSB = 0;
	t800.wg.waveformLSB = (tVST.wg.waveformLSB - 1) & 0x7F;
	t800.wg.pitchCoarse = tVST.wg.pitchCoarse + 48;
	t800.wg.pitchFine = tVST.wg.pitchFine + 50;
	t800.wg.pitchRandom = tVST.wg.pitchRandom;
	t800.wg.keyFollow = tVST.wg.keyFollow;
	t800.wg.benderSwitch = tVST.wg.benderSwitch;
	t800.wg.aTouchBend = tVST.wg.aTouchBend;
	t800.wg.lfo1Sens = tVST.wg.lfo1Sens + 50;
	t800.wg.lfo2Sens = tVST.wg.lfo2Sens + 50;
	t800.wg.leverSens = tVST.wg.leverSens + 50;
	t800.wg.aTouchModSens = tVST.wg.aTouchModSens + 50;

	// Why, Roland, why
	if (tVST.wg.waveformLSB == 88)
		t800.wg.waveformLSB = 89 - 1;
	else if (tVST.wg.waveformLSB == 89)
		t800.wg.waveformLSB = 88 - 1;
	else if (tVST.wg.waveformLSB == 23 || tVST.wg.waveformLSB == 28 || tVST.wg.waveformLSB == 35 || tVST.wg.waveformLSB == 36 || tVST.wg.waveformLSB == 37 || tVST.wg.waveformLSB == 50 || tVST.wg.waveformLSB == 102 || tVST.wg.waveformLSB == 107)
		t800.wg.pitchCoarse -= 12;
	else if (tVST.wg.waveformLSB == 48)
		t800.wg.pitchCoarse += 12;
	else if (tVST.wg.waveformLSB == 108)
		t800.wg.pitchCoarse -= 7;
	else if (tVST.wg.waveformLSB == 105)
		t800.wg.pitchFine += 50;

	if (t800.wg.pitchFine > 100)
	{
		t800.wg.pitchFine -= 100;
		t800.wg.pitchCoarse++;
	}
	if (static_cast<int8_t>(t800.wg.pitchCoarse) < 0)
	{
		t800.wg.pitchCoarse = 0;
		if (tVST.common.layerEnabled)
			std::cerr << "LOSSY CONVERSION! Tone coarse pitch too low (maybe due to waveform transposition)" << std::endl;
	}
	else if (t800.wg.pitchCoarse > 96)
	{
		t800.wg.pitchCoarse = 96;
		if (tVST.common.layerEnabled)
			std::cerr << "LOSSY CONVERSION! Tone coarse pitch too high (maybe due to waveform transposition)" << std::endl;
	}

	t800.pitchEnv.velo = tVST.pitchEnv.velo + 50;
	t800.pitchEnv.timeVelo = tVST.pitchEnv.timeVelo + 50;
	t800.pitchEnv.timeKF = tVST.pitchEnv.timeKF + 10;
	t800.pitchEnv.level0 = ConvertPitchEnvLevel(tVST.pitchEnv.level0);
	t800.pitchEnv.time1 = tVST.pitchEnv.time1;
	t800.pitchEnv.level1 = ConvertPitchEnvLevel(tVST.pitchEnv.level1);
	t800.pitchEnv.time2 = tVST.pitchEnv.time2;
	t800.pitchEnv.time3 = tVST.pitchEnv.time3;
	t800.pitchEnv.level2 = ConvertPitchEnvLevel(tVST.pitchEnv.level2);

	t800.tvf.filterMode = 2 - tVST.tvf.filterMode;
	t800.tvf.cutoffFreq = tVST.tvf.cutoffFreq;
	t800.tvf.resonance = tVST.tvf.resonance;
	t800.tvf.keyFollow = tVST.tvf.keyFollow;
	t800.tvf.aTouchSens = tVST.tvf.aTouchSens + 50;
	t800.tvf.lfoSelect = tVST.tvf.lfoSelect;
	t800.tvf.lfoDepth = tVST.tvf.lfoDepth + 50;
	t800.tvf.envDepth = tVST.tvf.envDepth + 50;

	t800.tvfEnv.velo = tVST.tvfEnv.velo + 50;
	t800.tvfEnv.timeVelo = tVST.tvfEnv.timeVelo + 50;
	t800.tvfEnv.timeKF = tVST.tvfEnv.timeKF + 10;
	t800.tvfEnv.time1 = tVST.tvfEnv.time1;
	t800.tvfEnv.level1 = tVST.tvfEnv.level1;
	t800.tvfEnv.time2 = tVST.tvfEnv.time2;
	t800.tvfEnv.level2 = tVST.tvfEnv.level2;
	t800.tvfEnv.time3 = tVST.tvfEnv.time3;
	t800.tvfEnv.sustainLevel = tVST.tvfEnv.sustainLevel;
	t800.tvfEnv.time4 = tVST.tvfEnv.time4;
	t800.tvfEnv.level4 = tVST.tvfEnv.level4;

	t800.tva.biasDirection = tVST.tva.biasDirection;
	t800.tva.biasPoint = tVST.tva.biasPoint;
	t800.tva.biasLevel = tVST.tva.biasLevel + 10;
	t800.tva.level = tVST.tva.level;
	t800.tva.aTouchSens = tVST.tva.aTouchSens + 50;
	t800.tva.lfoSelect = tVST.tva.lfoSelect;
	t800.tva.lfoDepth = tVST.tva.lfoDepth + 50;

	t800.tvaEnv.velo = tVST.tvaEnv.velo + 50;
	t800.tvaEnv.timeVelo = tVST.tvaEnv.timeVelo + 50;
	t800.tvaEnv.timeKF = tVST.tvaEnv.timeKF + 10;
	t800.tvaEnv.time1 = tVST.tvaEnv.time1;
	t800.tvaEnv.level1 = tVST.tvaEnv.level1;
	t800.tvaEnv.time2 = tVST.tvaEnv.time2;
	t800.tvaEnv.level2 = tVST.tvaEnv.level2;
	t800.tvaEnv.time3 = tVST.tvaEnv.time3;
	t800.tvaEnv.sustainLevel = tVST.tvaEnv.sustainLevel;
	t800.tvaEnv.time4 = tVST.tvaEnv.time4;
}

void ConvertPatchVSTTo800(const PatchVST &pVST, Patch800 &p800)
{
	if (pVST.zenHeader.modelID1 != 3 || pVST.zenHeader.modelID2 != 5)
	{
		std::cerr << "Skipping patch, appears to be for another synth model!" << std::endl;
		p800 = {};
		p800.common.name.fill(' ');
		return;
	}

	p800.common.name = pVST.name;
	p800.common.patchLevel = pVST.common.patchLevel;
	p800.common.keyRangeLowA = pVST.common.keyRangeLowA;
	p800.common.keyRangeHighA = pVST.common.keyRangeHighA;
	p800.common.keyRangeLowB = pVST.common.keyRangeLowB;
	p800.common.keyRangeHighB = pVST.common.keyRangeHighB;
	p800.common.keyRangeLowC = pVST.common.keyRangeLowC;
	p800.common.keyRangeHighC = pVST.common.keyRangeHighC;
	p800.common.keyRangeLowD = pVST.common.keyRangeLowD;
	p800.common.keyRangeHighD = pVST.common.keyRangeHighD;
	p800.common.benderRangeDown = pVST.common.benderRangeDown;
	p800.common.benderRangeUp = pVST.common.benderRangeUp;
	p800.common.aTouchBend = pVST.common.aTouchBend;
	p800.common.soloSW = pVST.common.soloSW;
	p800.common.soloLegato = pVST.common.soloLegato;
	p800.common.portamentoSW = pVST.common.portamentoSW;
	p800.common.portamentoMode = pVST.common.portamentoMode;
	p800.common.portamentoTime = pVST.common.portamentoTime;
	p800.common.layerTone = 0;
	for (uint8_t i = 0; i < 4; i++)
	{
		if (pVST.tone[i].common.layerEnabled)
			p800.common.layerTone |= (1 << i);
	}
	p800.common.activeTone = 0;
	for (uint8_t i = 0; i < 4; i++)
	{
		if (pVST.tone[i].common.layerSelected)
			p800.common.activeTone |= (1 << i);
	}

	ConvertEQBand(EQLowFreq, p800.eq.lowFreq, p800.eq.lowGain, pVST.eq.lowFreq, pVST.eq.lowGain, pVST.eq.eqEnabled, "low");
	ConvertEQBand(EQMidFreq, p800.eq.midFreq, p800.eq.midGain, pVST.eq.midFreq, pVST.eq.midGain, pVST.eq.eqEnabled, "mid");
	ConvertEQBand(EQHighFreq, p800.eq.highFreq, p800.eq.highGain, pVST.eq.highFreq, pVST.eq.highGain, pVST.eq.eqEnabled, "high");
	if (!MapToArrayIndex(pVST.eq.midQ, EQMidQ, p800.eq.midQ) && pVST.eq.midGain != 0 && pVST.eq.eqEnabled)
		std::cerr << "LOSSY CONVERSION! Unsupported EQ mid Q value: " << int(pVST.eq.midQ) << std::endl;

	p800.midiTx.keyMode = 0;
	p800.midiTx.splitPoint = 36;
	p800.midiTx.lowerChannel = 1;
	p800.midiTx.upperChannel = 0;
	p800.midiTx.lowerProgramChange = 0;
	p800.midiTx.upperProgramChange = 0;
	p800.midiTx.holdMode = 2;
	p800.midiTx.dummy = 0;

	if (pVST.effectsGroupA.effectsLevelGroupA != 127 && pVST.effectsGroupA.groupAenabled)
		std::cerr << "LOSSY CONVERSION! Effect Group A Level != 127: " << int(pVST.effectsGroupA.effectsLevelGroupA) << std::endl;
	if (pVST.effectsGroupA.panningGroupA != 64 && pVST.effectsGroupA.groupAenabled)
		std::cerr << "LOSSY CONVERSION! Effect Group A Pan != 64: " << int(pVST.effectsGroupA.panningGroupA) << std::endl;
	p800.effect.groupAsequence = pVST.effectsGroupA.groupAsequence.lsb;
	p800.effect.groupBsequence = pVST.effectsGroupB.groupBsequence;
	
	const uint8_t ds = pVST.effectsGroupA.groupAenabled ? pVST.effectsGroupA.distortionEnabled.lsb : 0;
	const uint8_t ph = pVST.effectsGroupA.groupAenabled ? pVST.effectsGroupA.phaserEnabled.lsb : 0;
	const uint8_t sp = pVST.effectsGroupA.groupAenabled ? pVST.effectsGroupA.spectrumEnabled.lsb : 0;
	const uint8_t en = pVST.effectsGroupA.groupAenabled ? pVST.effectsGroupA.enhancerEnabled.lsb : 0;
	const uint8_t GroupABlock1[] = { ds, ds, ds, ds, ds, ds, ph, ph, ph, ph, ph, ph, sp, sp, sp, sp, sp, sp, en, en, en, en, en, en };
	const uint8_t GroupABlock2[] = { ph, ph, sp, sp, en, en, ds, ds, sp, sp, en, en, ph, ph, ds, ds, en, en, ph, ph, sp, sp, ds, ds };
	const uint8_t GroupABlock3[] = { sp, en, en, ph, ph, sp, sp, en, en, ds, ds, sp, ds, en, en, ph, ph, ds, sp, ds, ds, ph, ph, sp };
	const uint8_t GroupABlock4[] = { en, sp, ph, en, sp, ph, en, sp, ds, en, sp, ds, en, ds, ph, en, ds, ph, ds, sp, ph, ds, sp, ph };
	const uint8_t cho = pVST.effectsGroupB.chorusEnabled;
	const uint8_t dly = pVST.effectsGroupB.delayEnabled;
	const uint8_t rev = pVST.effectsGroupB.reverbEnabled;
	const uint8_t GroupBBlock1[] = { cho, cho, dly, dly, rev, rev };
	const uint8_t GroupBBlock2[] = { dly, rev, cho, rev, cho, dly };
	const uint8_t GroupBBlock3[] = { rev, dly, rev, cho, dly, cho };

	p800.effect.groupAblockSwitch1 = GroupABlock1[pVST.effectsGroupA.groupAsequence % 24u];
	p800.effect.groupAblockSwitch2 = GroupABlock2[pVST.effectsGroupA.groupAsequence % 24u];
	p800.effect.groupAblockSwitch3 = GroupABlock3[pVST.effectsGroupA.groupAsequence % 24u];
	p800.effect.groupAblockSwitch4 = GroupABlock4[pVST.effectsGroupA.groupAsequence % 24u];
	p800.effect.groupBblockSwitch1 = GroupBBlock1[pVST.effectsGroupB.groupBsequence % 6u];
	p800.effect.groupBblockSwitch2 = GroupBBlock2[pVST.effectsGroupB.groupBsequence % 6u];
	p800.effect.groupBblockSwitch3 = GroupBBlock3[pVST.effectsGroupB.groupBsequence % 6u];
	p800.effect.effectsBalanceGroupB = pVST.effectsGroupB.effectsBalanceGroupB;

	p800.effect.distortionType = pVST.effectsGroupA.distortionType.lsb;
	p800.effect.distortionDrive = pVST.effectsGroupA.distortionDrive.lsb;
	p800.effect.distortionLevel = pVST.effectsGroupA.distortionLevel.lsb;

	p800.effect.phaserManual = pVST.effectsGroupA.phaserManual.lsb;
	p800.effect.phaserRate = pVST.effectsGroupA.phaserRate.lsb;
	p800.effect.phaserDepth = pVST.effectsGroupA.phaserDepth.lsb;
	p800.effect.phaserResonance = pVST.effectsGroupA.phaserResonance.lsb;
	p800.effect.phaserMix = pVST.effectsGroupA.phaserMix.lsb;

	p800.effect.spectrumBand1 = pVST.effectsGroupA.spectrumBand1.lsb;
	p800.effect.spectrumBand2 = pVST.effectsGroupA.spectrumBand2.lsb;
	p800.effect.spectrumBand3 = pVST.effectsGroupA.spectrumBand3.lsb;
	p800.effect.spectrumBand4 = pVST.effectsGroupA.spectrumBand4.lsb;
	p800.effect.spectrumBand5 = pVST.effectsGroupA.spectrumBand5.lsb;
	p800.effect.spectrumBand6 = pVST.effectsGroupA.spectrumBand6.lsb;
	p800.effect.spectrumBandwidth = pVST.effectsGroupA.spectrumBandwidth.lsb;

	p800.effect.enhancerSens = pVST.effectsGroupA.enhancerSens.lsb;
	p800.effect.enhancerMix = pVST.effectsGroupA.enhancerMix.lsb;

	if (pVST.effectsGroupB.delayCenterTempoSync)
		std::cerr << "LOSSY CONVERSION! Delay Effect Center Tap uses tempo sync, approximating delay @ 120 BPM" << std::endl;
	if (pVST.effectsGroupB.delayLeftTempoSync)
		std::cerr << "LOSSY CONVERSION! Delay Effect Left Tap uses tempo sync, approximating delay @ 120 BPM" << std::endl;
	if (pVST.effectsGroupB.delayRightTempoSync)
		std::cerr << "LOSSY CONVERSION! Delay Effect Right Tap uses tempo sync, approximating delay @ 120 BPM" << std::endl;
	p800.effect.delayCenterTap = pVST.effectsGroupB.delayCenterTempoSync ? ApproximateDelayWithTempoSync(pVST.effectsGroupB.delayCenterTapWithSync) : pVST.effectsGroupB.delayCenterTap;
	p800.effect.delayCenterLevel = pVST.effectsGroupB.delayCenterLevel;
	p800.effect.delayLeftTap = pVST.effectsGroupB.delayLeftTempoSync ? ApproximateDelayWithTempoSync(pVST.effectsGroupB.delayLeftTapWithSync) : pVST.effectsGroupB.delayLeftTap;
	p800.effect.delayLeftLevel = pVST.effectsGroupB.delayLeftLevel;
	p800.effect.delayRightTap = pVST.effectsGroupB.delayRightTempoSync ? ApproximateDelayWithTempoSync(pVST.effectsGroupB.delayRightTapWithSync) : pVST.effectsGroupB.delayRightTap;
	p800.effect.delayRightLevel = pVST.effectsGroupB.delayRightLevel;
	p800.effect.delayFeedback = pVST.effectsGroupB.delayFeedback;

	p800.effect.chorusRate = pVST.effectsGroupB.chorusRate;
	p800.effect.chorusDepth = pVST.effectsGroupB.chorusDepth;
	p800.effect.chorusDelayTime = pVST.effectsGroupB.chorusDelayTime;
	p800.effect.chorusFeedback = pVST.effectsGroupB.chorusFeedback;
	p800.effect.chorusLevel = pVST.effectsGroupB.chorusLevel;

	p800.effect.reverbType = pVST.effectsGroupB.reverbType;
	p800.effect.reverbPreDelay = pVST.effectsGroupB.reverbPreDelay;
	p800.effect.reverbEarlyRefLevel = pVST.effectsGroupB.reverbEarlyRefLevel;
	p800.effect.reverbHFDamp = pVST.effectsGroupB.reverbHFDamp;
	p800.effect.reverbTime = pVST.effectsGroupB.reverbTime;
	p800.effect.reverbLevel = pVST.effectsGroupB.reverbLevel;
	p800.effect.dummy = 0;

	ConvertToneVSTTo800(pVST.tone[0], p800.toneA);
	ConvertToneVSTTo800(pVST.tone[1], p800.toneB);
	ConvertToneVSTTo800(pVST.tone[2], p800.toneC);
	ConvertToneVSTTo800(pVST.tone[3], p800.toneD);
}
