// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 by Johannes Schultz
// License: BSD 3-clause

#include "jd800.h"
#include "jd990.h"

static void ConvertTone800To990(const Patch800 &p800, const Tone800 &t800, Tone990 &t990)
{
	t990.wg.waveSource = t800.wg.waveSource;
	t990.wg.waveformMSB = t800.wg.waveformMSB;
	t990.wg.waveformLSB = t800.wg.waveformLSB;
	t990.wg.fxmColor = 0;         // 990 only
	t990.wg.fxmDepth = 0;         // 990 only
	t990.wg.syncSlaveSwitch = 0;  // 990 only
	t990.wg.toneDelayMode = 0;    // 990 only
	t990.wg.toneDelayTime = 0;    // 990 only
	t990.wg.pitchCoarse = t800.wg.pitchCoarse;
	t990.wg.pitchFine = t800.wg.pitchFine;
	t990.wg.pitchRandom = t800.wg.pitchRandom;
	t990.wg.keyFollow = t800.wg.keyFollow;
	t990.wg.envDepth = 24;  // 990 only
	t990.wg.benderSwitch = t800.wg.benderSwitch;

	t990.pitchEnv.velo = t800.pitchEnv.velo;
	t990.pitchEnv.timeVelo = t800.pitchEnv.timeVelo;
	t990.pitchEnv.timeKF = t800.pitchEnv.timeKF;
	t990.pitchEnv.level0 = t800.pitchEnv.level0;
	t990.pitchEnv.time1 = t800.pitchEnv.time1;
	t990.pitchEnv.level1 = t800.pitchEnv.level1;
	t990.pitchEnv.time2 = t800.pitchEnv.time2;
	t990.pitchEnv.sustainLevel = 50;  // 990 only
	t990.pitchEnv.time3 = t800.pitchEnv.time3;
	t990.pitchEnv.level3 = t800.pitchEnv.level2;

	t990.tvf.filterMode = t800.tvf.filterMode;
	t990.tvf.cutoffFreq = t800.tvf.cutoffFreq;
	t990.tvf.resonance = t800.tvf.resonance;
	t990.tvf.keyFollow = t800.tvf.keyFollow;
	t990.tvf.envDepth = t800.tvf.envDepth;

	t990.tvfEnv.velo = t800.tvfEnv.velo;
	t990.tvfEnv.timeVelo = t800.tvfEnv.timeVelo;
	t990.tvfEnv.timeKF = t800.tvfEnv.timeKF;
	t990.tvfEnv.time1 = t800.tvfEnv.time1;
	t990.tvfEnv.level1 = t800.tvfEnv.level1;
	t990.tvfEnv.time2 = t800.tvfEnv.time2;
	t990.tvfEnv.level2 = t800.tvfEnv.level2;
	t990.tvfEnv.time3 = t800.tvfEnv.time3;
	t990.tvfEnv.sustainLevel = t800.tvfEnv.sustainLevel;
	t990.tvfEnv.time4 = t800.tvfEnv.time4;
	t990.tvfEnv.level4 = t800.tvfEnv.level4;

	t990.tva.level = t800.tva.level;
	t990.tva.biasDirection = t800.tva.biasDirection;
	t990.tva.biasPoint = t800.tva.biasPoint;
	t990.tva.biasLevel = t800.tva.biasLevel;
	t990.tva.pan = 50;          // 990 only
	t990.tva.panKeyFollow = 7;  // 990 only

	t990.tvaEnv.velo = t800.tvaEnv.velo;
	t990.tvaEnv.timeVelo = t800.tvaEnv.timeVelo;
	t990.tvaEnv.timeKF = t800.tvaEnv.timeKF;
	t990.tvaEnv.time1 = t800.tvaEnv.time1;
	t990.tvaEnv.level1 = t800.tvaEnv.level1;
	t990.tvaEnv.time2 = t800.tvaEnv.time2;
	t990.tvaEnv.level2 = t800.tvaEnv.level2;
	t990.tvaEnv.time3 = t800.tvaEnv.time3;
	t990.tvaEnv.sustainLevel = t800.tvaEnv.sustainLevel;
	t990.tvaEnv.time4 = t800.tvaEnv.time4;

	t990.common.velocityCurve = t800.common.velocityCurve;
	t990.common.holdControl = t800.common.holdControl;

	static constexpr uint8_t LfoWaveform800to990[] = { 0, 2, 3, 5, 6 };

	t990.lfo1.waveform = LfoWaveform800to990[t800.lfo1.waveform % std::size(LfoWaveform800to990)];
	t990.lfo1.rate = t800.lfo1.rate;
	t990.lfo1.delay = t800.lfo1.delay;
	t990.lfo1.fade = t800.lfo1.fade;
	t990.lfo1.offset = t800.lfo1.offset;
	t990.lfo1.keyTrigger = t800.lfo1.keyTrigger;
	t990.lfo1.depthPitch = t800.wg.lfo1Sens;
	t990.lfo1.depthTVF = (t800.tvf.lfoSelect == 0) ? t800.tvf.lfoDepth : 50;
	t990.lfo1.depthTVA = (t800.tva.lfoSelect == 0) ? t800.tva.lfoDepth : 50;

	t990.lfo2.waveform = LfoWaveform800to990[t800.lfo2.waveform % std::size(LfoWaveform800to990)];
	t990.lfo2.rate = t800.lfo2.rate;
	t990.lfo2.delay = t800.lfo2.delay;
	t990.lfo2.fade = t800.lfo2.fade;
	t990.lfo2.offset = t800.lfo2.offset;
	t990.lfo2.keyTrigger = t800.lfo2.keyTrigger;
	t990.lfo2.depthPitch = t800.wg.lfo2Sens;
	t990.lfo2.depthTVF = (t800.tvf.lfoSelect == 1) ? t800.tvf.lfoDepth : 50;
	t990.lfo2.depthTVA = (t800.tva.lfoSelect == 1) ? t800.tva.lfoDepth : 50;

	// Mod Wheel
	t990.cs1.destination1 = t800.wg.leverSens < 50 ? 5 : 4;  // Mod Wheel to Pitch via LFO
	t990.cs1.depth1 = t800.wg.leverSens < 50 ? (100 - t800.wg.leverSens) : t800.wg.leverSens;
	t990.cs1.destination2 = 6;  // unused
	t990.cs1.depth2 = 50;
	t990.cs1.destination3 = 8;  // unused
	t990.cs1.depth3 = 50;
	t990.cs1.destination4 = 10;  // unused
	t990.cs1.depth4 = 50;

	// Aftertouch
	// Destinations: pitch, cutoff, res, level, p-lfo1, p-lfo2, f-lfo1, f-lfo2, a-lfo1, a-lfo2, lfo1-r, lfo2-r
	t990.cs2.destination1 = 0;  // Aftertouch to Pitch
	if (t800.wg.aTouchBend)
	{
		if (p800.common.aTouchBend == 0)
			t990.cs2.depth1 = -36 + 50;
		else if (p800.common.aTouchBend == 1)
			t990.cs2.depth1 = -24 + 50;
		else
			t990.cs2.depth1 = -12 + 50 + (p800.common.aTouchBend - 2);
	}
	else
	{
		t990.cs2.depth1 = 50;
	}
	t990.cs2.destination2 = t800.wg.aTouchModSens < 50 ? 5 : 4;  // Aftertouch to Pitch via LFO
	t990.cs2.depth2 = t800.wg.aTouchModSens < 50 ? (100 - t800.wg.aTouchModSens) : t800.wg.aTouchModSens;
	t990.cs2.destination3 = 1;  // Aftertouch to TVF
	t990.cs2.depth3 = t800.tvf.aTouchSens;
	t990.cs2.destination4 = 3;  // Aftertouch to TVA
	t990.cs2.depth4 = t800.tva.aTouchSens;
}

void ConvertPatch800To990(const Patch800 &p800, Patch990 &p990)
{
	p990.common.name = p800.common.name;
	p990.common.patchLevel = p800.common.patchLevel;
	p990.common.patchPan = 50;      // 990 only
	p990.common.analogFeel = 0;     // 990 only
	p990.common.voicePriority = 0;  // 990 only
	p990.common.bendRangeDown = p800.common.benderRangeDown;
	p990.common.bendRangeUp = p800.common.benderRangeUp;
	p990.common.toneControlSource1 = 0;
	p990.common.toneControlSource2 = 1;
	p990.common.layerTone = p800.common.layerTone;
	p990.common.activeTone = p800.common.activeTone;

	p990.keyEffects.portamentoSW = p800.common.portamentoSW;
	p990.keyEffects.portamentoMode = p800.common.portamentoMode;
	p990.keyEffects.portamentoType = 1;  // 990 only
	p990.keyEffects.portamentoTime = p800.common.portamentoTime;
	p990.keyEffects.soloSW = p800.common.soloSW;
	p990.keyEffects.soloLegato = p800.common.soloLegato;
	p990.keyEffects.soloSyncMaster = 0;  // 990 only

	p990.eq.lowFreq = p800.eq.lowFreq;
	p990.eq.lowGain = p800.eq.lowGain;
	p990.eq.midFreq = p800.eq.midFreq;
	p990.eq.midQ = p800.eq.midQ;
	p990.eq.midGain = p800.eq.midGain;
	p990.eq.highFreq = p800.eq.highFreq;
	p990.eq.highGain = p800.eq.highGain;

	p990.structureType.structureAB = 0;
	p990.structureType.structureCD = 0;

	p990.keyRanges.keyRangeLowA = p800.common.keyRangeLowA;
	p990.keyRanges.keyRangeLowB = p800.common.keyRangeLowB;
	p990.keyRanges.keyRangeLowC = p800.common.keyRangeLowC;
	p990.keyRanges.keyRangeLowD = p800.common.keyRangeLowD;
	p990.keyRanges.keyRangeHighA = p800.common.keyRangeHighA;
	p990.keyRanges.keyRangeHighB = p800.common.keyRangeHighB;
	p990.keyRanges.keyRangeHighC = p800.common.keyRangeHighC;
	p990.keyRanges.keyRangeHighD = p800.common.keyRangeHighD;

	p990.velocity.velocityRange1 = 0;
	p990.velocity.velocityRange2 = 0;
	p990.velocity.velocityRange3 = 0;
	p990.velocity.velocityRange4 = 0;
	p990.velocity.velocityPoint1 = 64;
	p990.velocity.velocityPoint2 = 64;
	p990.velocity.velocityPoint3 = 64;
	p990.velocity.velocityPoint4 = 64;
	p990.velocity.velocityFade1 = 0;
	p990.velocity.velocityFade2 = 0;
	p990.velocity.velocityFade3 = 0;
	p990.velocity.velocityFade4 = 0;

	p990.effect.effectsBalanceGroupB = p800.effect.effectsBalanceGroupB;
	p990.effect.controlSource1 = 0;
	p990.effect.controlDest1 = 0;
	p990.effect.controlDepth1 = 50;
	p990.effect.controlSource2 = 0;
	p990.effect.controlDest2 = 0;
	p990.effect.controlDepth2 = 50;

	p990.effect.groupAsequence = p800.effect.groupAsequence;
	p990.effect.groupAblockSwitch1 = p800.effect.groupAblockSwitch1;
	p990.effect.groupAblockSwitch2 = p800.effect.groupAblockSwitch2;
	p990.effect.groupAblockSwitch3 = p800.effect.groupAblockSwitch3;
	p990.effect.groupAblockSwitch4 = p800.effect.groupAblockSwitch4;

	p990.effect.distortionType = p800.effect.distortionType;
	p990.effect.distortionDrive = p800.effect.distortionDrive;
	p990.effect.distortionLevel = p800.effect.distortionLevel;

	p990.effect.phaserManual = p800.effect.phaserManual;
	p990.effect.phaseRate = p800.effect.phaseRate;
	p990.effect.phaserDepth = p800.effect.phaserDepth;
	p990.effect.phaserResonance = p800.effect.phaserResonance;
	p990.effect.phaserMix = p800.effect.phaserMix;

	p990.effect.spectrumBand1 = p800.effect.spectrumBand1;
	p990.effect.spectrumBand2 = p800.effect.spectrumBand2;
	p990.effect.spectrumBand3 = p800.effect.spectrumBand3;
	p990.effect.spectrumBand4 = p800.effect.spectrumBand4;
	p990.effect.spectrumBand5 = p800.effect.spectrumBand5;
	p990.effect.spectrumBand6 = p800.effect.spectrumBand6;
	p990.effect.spectrumBandwidth = p800.effect.spectrumBandwidth;

	p990.effect.enhancerSens = p800.effect.enhancerSens;
	p990.effect.enhancerMix = p800.effect.enhancerMix;

	p990.effect.groupBsequence = p800.effect.groupBsequence;
	p990.effect.groupBblockSwitch1 = p800.effect.groupBblockSwitch1;
	p990.effect.groupBblockSwitch2 = p800.effect.groupBblockSwitch2;
	p990.effect.groupBblockSwitch3 = p800.effect.groupBblockSwitch3;

	p990.effect.chorusRate = p800.effect.chorusRate;
	p990.effect.chorusDepth = p800.effect.chorusDepth;
	p990.effect.chorusDelayTime = p800.effect.chorusDelayTime;
	p990.effect.chorusFeedback = p800.effect.chorusFeedback;
	p990.effect.chorusLevel = p800.effect.chorusLevel;

	p990.effect.delayMode = 0;  // 990 only
	p990.effect.delayCenterTapMSB = 0;
	p990.effect.delayCenterTapLSB = p800.effect.delayCenterTap;
	p990.effect.delayCenterLevel = p800.effect.delayCenterLevel;
	p990.effect.delayLeftTapMSB = 0;
	p990.effect.delayLeftTapLSB = p800.effect.delayLeftTap;
	p990.effect.delayLeftLevel = p800.effect.delayLeftLevel;
	p990.effect.delayRightTapMSB = 0;
	p990.effect.delayRightTapLSB = p800.effect.delayRightTap;
	p990.effect.delayRightLevel = p800.effect.delayRightLevel;
	p990.effect.delayFeedback = p800.effect.delayFeedback;

	p990.effect.reverbType = p800.effect.reverbType;
	p990.effect.reverbPreDelay = p800.effect.reverbPreDelay;
	p990.effect.reveryEarlyRefLevel = p800.effect.reveryEarlyRefLevel;
	p990.effect.reverbHFDamp = p800.effect.reverbHFDamp;
	p990.effect.reverbTime = p800.effect.reverbTime;
	p990.effect.reverbLevel = p800.effect.reverbLevel;

	p990.octaveSwitch = 1;

	ConvertTone800To990(p800, p800.toneA, p990.toneA);
	ConvertTone800To990(p800, p800.toneB, p990.toneB);
	ConvertTone800To990(p800, p800.toneC, p990.toneC);
	ConvertTone800To990(p800, p800.toneD, p990.toneD);
}
