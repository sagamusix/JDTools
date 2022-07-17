// JDTools - Patch conversion tools for Roland JD-800 / JD-990
// 2022 by Johannes Schultz
// License: BSD 3-clause

#include "jd800.h"
#include "jd990.h"

#include <iostream>

static void ConvertToneControl(const uint8_t source, const uint8_t dest, const uint8_t depth, Patch800 &p800, Tone800 &t800)
{
	if (source == 0 && dest == 4)
	{
		if(depth >= 50)
			t800.wg.leverSens = 50 + (depth - 50);
		else
			std::cerr << "LOSSY CONVERSION! Mod Wheel to LFO1 mod matrix routing with negative modulation!" << std::endl;
	}
	else if (source == 0 && dest == 5)
	{
		if (depth >= 50)
			t800.wg.leverSens = 50 - (depth - 50);
		else
			std::cerr << "LOSSY CONVERSION! Mod Wheel to LFO2 mod matrix routing with negative modulation!" << std::endl;
	}
	else if (source == 1 && dest == 4)
	{
		if (depth >= 50)
			t800.wg.aTouchModSens = 50 + (depth - 50);
		else
			std::cerr << "LOSSY CONVERSION! Aftertouch to LFO1 mod matrix routing with negative modulation!" << std::endl;
	}
	else if (source == 1 && dest == 5)
	{
		if (depth >= 50)
			t800.wg.aTouchModSens = 50 - (depth - 50);
		else
			std::cerr << "LOSSY CONVERSION! Aftertouch to LFO2 mod matrix routing with negative modulation!" << std::endl;
	}
	else if (source == 1 && dest == 0 && depth != 50)
	{
		t800.wg.aTouchBend = 1;
		if(depth == -36 + 50)
			p800.common.aTouchBend = 0;
		else if (depth == -24 + 50)
			p800.common.aTouchBend = 1;
		else if (depth >= -12 + 50 && depth <= 12 + 50)
			p800.common.aTouchBend = depth - (-12 + 50) + 2;
		else
			std::cerr << "LOSSY CONVERSION! Aftertouch to pitch bend modulation has incompatible value:" << int(depth) << std::endl;
	}
	else if (source == 1 && dest == 1)
	{
		t800.tvf.aTouchSens = depth;
	}
	else if (source == 1 && dest == 3)
	{
		t800.tva.aTouchSens = depth;
	}
	else if(depth != 50)
	{
		std::cerr << "LOSSY CONVERSION! Unknown mod matrix routing: source = " << int(source) << ", dest = " << int(dest) << std::endl;
	}
}

static void ConvertTone990To800(const Patch990 &p990, const Tone990 &t990, Patch800 &p800, Tone800 &t800)
{
	if (p990.structureType.structureAB != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch AB has unsupported structure type: " << int(p990.structureType.structureAB) << std::endl;
	if (p990.structureType.structureCD != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch CD has unsupported structure type: " << int(p990.structureType.structureCD) << std::endl;

	if (p990.velocity.velocityRange1 != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch velocity range 1 is enabled: " << int(p990.velocity.velocityRange1) << std::endl;
	if (p990.velocity.velocityRange2 != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch velocity range 2 is enabled: " << int(p990.velocity.velocityRange2) << std::endl;
	if (p990.velocity.velocityRange3 != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch velocity range 3 is enabled: " << int(p990.velocity.velocityRange3) << std::endl;
	if (p990.velocity.velocityRange4 != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch velocity range 4 is enabled: " << int(p990.velocity.velocityRange4) << std::endl;

	t800.common.velocityCurve = t990.common.velocityCurve;
	t800.common.holdControl = t990.common.holdControl;

	static constexpr uint8_t LfoWaveform990to800[] = { 0, 0x7F, 1, 2, 0x7F, 3, 4, 0x7F };

	t800.lfo1.rate = t990.lfo1.rate;
	t800.lfo1.delay = t990.lfo1.delay;
	t800.lfo1.fade = t990.lfo1.fade;
	t800.lfo1.waveform = LfoWaveform990to800[t990.lfo1.waveform % std::size(LfoWaveform990to800)];
	t800.lfo1.offset = t990.lfo1.offset;
	t800.lfo1.keyTrigger = t990.lfo1.keyTrigger;
	if (t800.lfo1.waveform == 0x7F)
	{
		t800.lfo1.waveform = 0;
		std::cerr << "LOSSY CONVERSION! JD-990 tone LFO1 has unsupported LFO waveform: " << int(t990.lfo1.waveform) << std::endl;
	}

	t800.lfo2.rate = t990.lfo2.rate;
	t800.lfo2.delay = t990.lfo2.delay;
	t800.lfo2.fade = t990.lfo2.fade;
	t800.lfo2.waveform = LfoWaveform990to800[t990.lfo2.waveform % std::size(LfoWaveform990to800)];
	t800.lfo2.offset = t990.lfo2.offset;
	t800.lfo2.keyTrigger = t990.lfo2.keyTrigger;
	if (t800.lfo2.waveform == 0x7F)
	{
		t800.lfo2.waveform = 0;
		std::cerr << "LOSSY CONVERSION! JD-990 tone LFO2 has unsupported LFO waveform: " << int(t990.lfo2.waveform) << std::endl;
	}

	t800.wg.waveSource = t990.wg.waveSource;
	t800.wg.waveformMSB = t990.wg.waveformMSB;
	t800.wg.waveformLSB = t990.wg.waveformLSB;
	t800.wg.pitchCoarse = t990.wg.pitchCoarse;
	t800.wg.pitchFine = t990.wg.pitchFine;
	t800.wg.pitchRandom = t990.wg.pitchRandom;
	t800.wg.keyFollow = t990.wg.keyFollow;
	t800.wg.benderSwitch = t990.wg.benderSwitch;
	t800.wg.aTouchBend = 0;  // Will be populated by tone control conversion
	t800.wg.lfo1Sens = t990.lfo1.depthPitch;
	t800.wg.lfo2Sens = t990.lfo2.depthPitch;
	if (t990.wg.fxmColor != 0 || t990.wg.fxmDepth != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 tone has FXM enabled!" << std::endl;
	if (t990.wg.syncSlaveSwitch != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 tone has sync slave switch enabled!" << std::endl;
	if (t990.wg.toneDelayTime != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 tone has tone delay enabled!" << std::endl;
	if (t990.wg.envDepth != 24)
		std::cerr << "LOSSY CONVERSION! JD-990 tone has pitch envelope depth level != 24: " << int(t990.wg.envDepth) << std::endl;

	t800.pitchEnv.velo = t990.pitchEnv.velo;
	t800.pitchEnv.timeVelo = t990.pitchEnv.timeVelo;
	t800.pitchEnv.timeKF = t990.pitchEnv.timeKF;
	t800.pitchEnv.level0 = t990.pitchEnv.level0;
	t800.pitchEnv.time1 = t990.pitchEnv.time1;
	t800.pitchEnv.level1 = t990.pitchEnv.level1;
	t800.pitchEnv.time2 = t990.pitchEnv.time2;
	t800.pitchEnv.time3 = t990.pitchEnv.time3;
	t800.pitchEnv.level2 = t990.pitchEnv.level3;
	if (t990.pitchEnv.sustainLevel != 50)
		std::cerr << "LOSSY CONVERSION! JD-990 tone has pitch envelope sustain level != 50: " << int(t990.pitchEnv.sustainLevel) << std::endl;

	t800.tvf.filterMode = t990.tvf.filterMode;
	t800.tvf.cutoffFreq = t990.tvf.cutoffFreq;
	t800.tvf.resonance = t990.tvf.resonance;
	t800.tvf.keyFollow = t990.tvf.keyFollow;
	if (t990.lfo2.depthTVF != 50)
	{
		t800.tvf.lfoSelect = 1;
		t800.tvf.lfoDepth = t990.lfo2.depthTVF;
		if(t990.lfo1.depthTVF != 50)
			std::cerr << "LOSSY CONVERSION! JD-990 tone has both LFOs contorlling TVF!" << std::endl;
	}
	else
	{
		t800.tvf.lfoSelect = 0;
		t800.tvf.lfoDepth = t990.lfo1.depthTVF;
	}
	t800.tvf.envDepth = t990.tvf.envDepth;

	t800.tvfEnv.velo = t990.tvfEnv.velo;
	t800.tvfEnv.timeVelo = t990.tvfEnv.timeVelo;
	t800.tvfEnv.timeKF = t990.tvfEnv.timeKF;
	t800.tvfEnv.time1 = t990.tvfEnv.time1;
	t800.tvfEnv.level1 = t990.tvfEnv.level1;
	t800.tvfEnv.time2 = t990.tvfEnv.time2;
	t800.tvfEnv.level2 = t990.tvfEnv.level2;
	t800.tvfEnv.time3 = t990.tvfEnv.time3;
	t800.tvfEnv.sustainLevel = t990.tvfEnv.sustainLevel;
	t800.tvfEnv.time4 = t990.tvfEnv.time4;
	t800.tvfEnv.level4 = t990.tvfEnv.level4;

	t800.tva.biasDirection = t990.tva.biasDirection;
	t800.tva.biasPoint = t990.tva.biasPoint;
	t800.tva.biasLevel = t990.tva.biasLevel;
	t800.tva.level = t990.tva.level;
	if (t990.lfo2.depthTVA != 50)
	{
		t800.tva.lfoSelect = 1;
		t800.tva.lfoDepth = t990.lfo2.depthTVA;
		if (t990.lfo1.depthTVA != 50)
			std::cerr << "LOSSY CONVERSION! JD-990 tone has both LFOs contorlling TVA!" << std::endl;
	}
	else
	{
		t800.tva.lfoSelect = 0;
		t800.tva.lfoDepth = t990.lfo1.depthTVA;
	}
	if (t990.tva.pan != 50)
	{
		std::cerr << "LOSSY CONVERSION! JD-990 tone has pan position != 50: " << int(t990.tva.pan) << std::endl;
	}
	if (t990.tva.panKeyFollow != 7)
	{
		std::cerr << "LOSSY CONVERSION! JD-990 tone uses pan key follow: " << int(t990.tva.panKeyFollow) << std::endl;
	}

	t800.tvaEnv.velo = t990.tvaEnv.velo;
	t800.tvaEnv.timeVelo = t990.tvaEnv.timeVelo;
	t800.tvaEnv.timeKF = t990.tvaEnv.timeKF;
	t800.tvaEnv.time1 = t990.tvaEnv.time1;
	t800.tvaEnv.level1 = t990.tvaEnv.level1;
	t800.tvaEnv.time2 = t990.tvaEnv.time2;
	t800.tvaEnv.level2 = t990.tvaEnv.level2;
	t800.tvaEnv.time3 = t990.tvaEnv.time3;
	t800.tvaEnv.sustainLevel = t990.tvaEnv.sustainLevel;
	t800.tvaEnv.time4 = t990.tvaEnv.time4;

	if (p990.common.toneControlSource1 > 1)
	{
		std::cerr << "LOSSY CONVERSION! JD-990 patch uses tone control source 1 other than mod wheel or aftertouch: " << int(p990.common.toneControlSource1) << std::endl;
	}
	if (p990.common.toneControlSource2 > 1)
	{
		std::cerr << "LOSSY CONVERSION! JD-990 patch uses tone control source 2 other than mod wheel or aftertouch: " << int(p990.common.toneControlSource1) << std::endl;
	}

	ConvertToneControl(p990.common.toneControlSource1, t990.cs1.destination1, t990.cs1.depth1, p800, t800);
	ConvertToneControl(p990.common.toneControlSource1, t990.cs1.destination2, t990.cs1.depth2, p800, t800);
	ConvertToneControl(p990.common.toneControlSource1, t990.cs1.destination3, t990.cs1.depth3, p800, t800);
	ConvertToneControl(p990.common.toneControlSource1, t990.cs1.destination4, t990.cs1.depth4, p800, t800);
	ConvertToneControl(p990.common.toneControlSource2, t990.cs2.destination1, t990.cs2.depth1, p800, t800);
	ConvertToneControl(p990.common.toneControlSource2, t990.cs2.destination2, t990.cs2.depth2, p800, t800);
	ConvertToneControl(p990.common.toneControlSource2, t990.cs2.destination3, t990.cs2.depth3, p800, t800);
	ConvertToneControl(p990.common.toneControlSource2, t990.cs2.destination4, t990.cs2.depth4, p800, t800);
}

void ConvertPatch990To800(const Patch990 &p990, Patch800 &p800)
{
	p800.common.name = p990.common.name;
	p800.common.patchLevel = p990.common.patchLevel;
	p800.common.keyRangeLowA = p990.keyRanges.keyRangeLowA;
	p800.common.keyRangeHighA = p990.keyRanges.keyRangeHighA;
	p800.common.keyRangeLowB = p990.keyRanges.keyRangeLowB;
	p800.common.keyRangeHighB = p990.keyRanges.keyRangeHighB;
	p800.common.keyRangeLowC = p990.keyRanges.keyRangeLowC;
	p800.common.keyRangeHighC = p990.keyRanges.keyRangeHighC;
	p800.common.keyRangeLowD = p990.keyRanges.keyRangeLowD;
	p800.common.keyRangeHighD = p990.keyRanges.keyRangeHighD;
	p800.common.benderRangeDown = p990.common.bendRangeDown;
	p800.common.benderRangeUp = p990.common.bendRangeUp;
	p800.common.aTouchBend = 0;  // Will be populated by tone conversion
	p800.common.soloSW = p990.keyEffects.soloSW;
	p800.common.soloLegato = p990.keyEffects.soloLegato;
	p800.common.portamentoSW = p990.keyEffects.portamentoSW;
	p800.common.portamentoMode = p990.keyEffects.portamentoMode;
	p800.common.portamentoTime = p990.keyEffects.portamentoTime;
	p800.common.layerTone = p990.common.layerTone;
	p800.common.activeTone = p990.common.activeTone;

	if (p990.common.patchPan != 50)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has pan != 50: " << int(p990.common.patchPan) << std::endl;
	if (p990.common.analogFeel != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has analog feel != 0: " << int(p990.common.analogFeel) << std::endl;
	if (p990.common.voicePriority != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has voice priority != 0: " << int(p990.common.voicePriority) << std::endl;
	if (p990.keyEffects.portamentoType != 1)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has portamento type != 1: " << int(p990.keyEffects.portamentoType) << std::endl;
	if (p990.keyEffects.soloSyncMaster != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has solo sync master != 0: " << int(p990.keyEffects.soloSyncMaster) << std::endl;
	if (p990.octaveSwitch != 1)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has octave switch != 1: " << int(p990.octaveSwitch) << std::endl;

	p800.eq.lowFreq = p990.eq.lowFreq;
	p800.eq.lowGain = p990.eq.lowGain;
	p800.eq.midFreq = p990.eq.midFreq;
	p800.eq.midQ = p990.eq.midQ;
	p800.eq.midGain = p990.eq.midGain;
	p800.eq.highFreq = p990.eq.highFreq;
	p800.eq.highGain = p990.eq.highGain;

	p800.midiTx.keyMode = 0;
	p800.midiTx.splitPoint = 36;
	p800.midiTx.lowerChannel = 0;
	p800.midiTx.upperChannel = 0;
	p800.midiTx.lowerProgramChange = 0;
	p800.midiTx.upperProgramChange = 0;
	p800.midiTx.holdMode = 2;
	p800.midiTx.dummy = 0;

	p800.effect.groupAsequence = p990.effect.groupAsequence;
	p800.effect.groupBsequence = p990.effect.groupBsequence;
	p800.effect.groupAblockSwitch1 = p990.effect.groupAblockSwitch1;
	p800.effect.groupAblockSwitch2 = p990.effect.groupAblockSwitch2;
	p800.effect.groupAblockSwitch3 = p990.effect.groupAblockSwitch3;
	p800.effect.groupAblockSwitch4 = p990.effect.groupAblockSwitch4;
	p800.effect.groupBblockSwitch1 = p990.effect.groupBblockSwitch1;
	p800.effect.groupBblockSwitch2 = p990.effect.groupBblockSwitch2;
	p800.effect.groupBblockSwitch3 = p990.effect.groupBblockSwitch3;
	p800.effect.effectsBalanceGroupB = p990.effect.effectsBalanceGroupB;

	p800.effect.distortionType = p990.effect.distortionType;
	p800.effect.distortionDrive = p990.effect.distortionDrive;
	p800.effect.distortionLevel = p990.effect.distortionLevel;

	p800.effect.phaserManual = p990.effect.phaserManual;
	p800.effect.phaseRate = p990.effect.phaseRate;
	p800.effect.phaserDepth = p990.effect.phaserDepth;
	p800.effect.phaserResonance = p990.effect.phaserResonance;
	p800.effect.phaserMix = p990.effect.phaserMix;

	p800.effect.spectrumBand1 = p990.effect.spectrumBand1;
	p800.effect.spectrumBand2 = p990.effect.spectrumBand2;
	p800.effect.spectrumBand3 = p990.effect.spectrumBand3;
	p800.effect.spectrumBand4 = p990.effect.spectrumBand4;
	p800.effect.spectrumBand5 = p990.effect.spectrumBand5;
	p800.effect.spectrumBand6 = p990.effect.spectrumBand6;
	p800.effect.spectrumBandwidth = p990.effect.spectrumBandwidth;

	p800.effect.enhancerSens = p990.effect.enhancerSens;
	p800.effect.enhancerMix = p990.effect.enhancerMix;

	p800.effect.delayCenterTap = p990.effect.delayCenterTapLSB;
	p800.effect.delayCenterLevel = p990.effect.delayCenterLevel;
	p800.effect.delayLeftTap = p990.effect.delayLeftTapLSB;
	p800.effect.delayLeftLevel = p990.effect.delayLeftLevel;
	p800.effect.delayRightTap = p990.effect.delayRightTapLSB;
	p800.effect.delayRightLevel = p990.effect.delayRightLevel;
	p800.effect.delayFeedback = p990.effect.delayFeedback;
	if (p990.effect.delayCenterTapMSB != 0 || p990.effect.delayCenterTapLSB > 0x7D)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has unsupported delay center tap: " << int(p990.effect.delayCenterTapMSB) << "/" << int(p990.effect.delayCenterTapLSB) << std::endl;
	if (p990.effect.delayLeftTapMSB != 0 || p990.effect.delayLeftTapLSB > 0x7D)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has unsupported delay left tap: " << int(p990.effect.delayLeftTapMSB) << "/" << int(p990.effect.delayLeftTapLSB) << std::endl;
	if (p990.effect.delayRightTapMSB != 0 || p990.effect.delayRightTapLSB > 0x7D)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has unsupported delay right tap: " << int(p990.effect.delayRightTapMSB) << "/" << int(p990.effect.delayRightTapLSB) << std::endl;
	if (p990.effect.delayMode != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has delay effect mode != 0: " << int(p990.effect.delayMode) << std::endl;

	p800.effect.chorusRate = p990.effect.chorusRate;
	p800.effect.chorusDepth = p990.effect.chorusDepth;
	p800.effect.chorusDelayTime = p990.effect.chorusDelayTime;
	p800.effect.chorusFeedback = p990.effect.chorusFeedback;
	p800.effect.chorusLevel = p990.effect.chorusLevel;

	p800.effect.reverbType = p990.effect.reverbType;
	p800.effect.reverbPreDelay = p990.effect.reverbPreDelay;
	p800.effect.reveryEarlyRefLevel = p990.effect.reveryEarlyRefLevel;
	p800.effect.reverbHFDamp = p990.effect.reverbHFDamp;
	p800.effect.reverbTime = p990.effect.reverbTime;
	p800.effect.reverbLevel = p990.effect.reverbLevel;
	p800.effect.dummy = 0;

	ConvertTone990To800(p990, p990.toneA, p800, p800.toneA);
	ConvertTone990To800(p990, p990.toneB, p800, p800.toneB);
	ConvertTone990To800(p990, p990.toneC, p800, p800.toneC);
	ConvertTone990To800(p990, p990.toneD, p800, p800.toneD);
}
