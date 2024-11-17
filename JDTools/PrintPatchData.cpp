// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 - 2024 by Johannes Schultz
// License: BSD 3-clause

#include "JD-08.hpp"
#include "JD-800.hpp"
#include "JD-990.hpp"
#include "PrecomputedTablesVST.hpp"
#include "WaveformNames.hpp"
#include "Utils.hpp"

static constexpr const char *KeyMode800[] = { "WHOLE", "SPLIT", "DUAL" };
static constexpr const char *HoldMode800[] = { "UPPER", "LOWER", "BOTH" };
static constexpr const char *WaveSource[] = { "INT", "CARD", "EXP" };
static constexpr const char *ToneDelayMode990[] = { "NORMAL", "HOLD", "K-OFF N", "K-OFF D", "PLAYMATE" };
static constexpr const char *FilterMode[] = { "HP", "BP", "LF" };
static constexpr const char *BiasDirection[] = { "UPPER", "LOWER", "UP&LOW" };
static constexpr const char *LFOSelect[] = { "LFO 1", "LFO 2"};
static constexpr const char *LFOWaveform800[] = { "TRI", "SAW", "SQU", "S/H", "RND" };
static constexpr const char *LFOWaveform990[] = { "TRI", "SIN", "SAW", "SQU", "TRP", "S/H", "RND", "CHS" };
static constexpr const char *LFOOffset[] = { "+", "0", "-" };
static constexpr const char *DelayMode990[] = { "NORMAL", "MIDI TEMPO", "MANUAL TEMPO" };
static constexpr const char *ControlSource990[] = { "MOD", "AFTER", "EXP", "BREATH", "P.BEND", "FOOT" };
static constexpr const char *ControlDest990[] = { "PITCH", "CUTOFF", "RES", "LEVEL", "P-LFO1", "P-LFO2", "F-LFO1", "F-LFO2", "A-LFO1", "A-LFO2", "LFO1-R", "LFO2-R" };
static constexpr const char *ControlDestFX990[] = { "FX-BAL", "DS-DRV", "PH-MAN", "PH-RAT", "PH-DPT", "PH-RES", "PH-MIX", "EN-MIX", "CH-RAT", "CH-FDB", "CH-LVL", "DL-FDB", "DL-LVL", "RV-TIM", "RV-LVL" };
static constexpr const char *FXGroupASequence[] = { "DS-PH-SP-EN", "DS-PH-EN-SP", "DS-SP-EN-PH", "DS-SP-PH-EN", "DS-EN-PH-EN", "DS-EN-PH-SP", "PH-DS-SP-EN", "PH_DS-EN-SP", "PH-SP-EN-DS", "PH-SP-DS-EN", "PH-EN-DS-SP", "PH-EN-SP-DS", "SP-PH-DS-EN", "SP-PH-EN-DS", "SP-DS-EN-PH", "SP-DS-PH-EN", "SP-EN-PH-DS", "SP-EN-DS-PH", "EN-PH-SP-DS", "EN-PH-DS-SP", "EN-SP-DS-PH", "EN-SP-PH-DS", "EN-DS-PH-SP", "EN-DS-SP-PH" };
static constexpr const char *FXGroupBSequence[] = { "CHO-DLY-REV", "CHO-REV-DLY", "DLY-CHO-REV", "DLY-REV-CHO", "REV-CHO-DLY", "REV-DLY-CHO" };
static constexpr const char *DistortionType[] = { "MELLOW DIRVE", "OVERDIVE", "CRY DRIVE", "MELLO DIST", "LIGHT DIST", "FAT DIST", "FUZZ DIST" };
static constexpr const char *DelayTime990[] = { "16th", "Triplet 8th", "8th", "Triple Quarter", "Dotted 8th", "Quarter", "Triplet half", "Dotted quarter", "Half", "Whole" };
static constexpr const char *TempoSyncVST[] = { "1/64T", "1/64", "1/32T", "1/32", "1/16T", "1/32.", "1/16", "1/8T", "1/16.", "1/8", "1/4T", "1/8.", "1/4", "1/2T", "1/4.", "1/2", "1T", "1/2.", "1", "2T", "1.", "2", "4" };
static constexpr const char *ReverbType[] = { "ROOM1", "ROOM2", "HALL1", "HALL2", "HALL3", "HALL4", "GATE", "REVERSE", "FLYING1", "FLYING2" };
static constexpr const char *ReverbHFDamp[] = { "500 Hz", "630 Hz", "800 Hz", "1 kHz", "1.25 kHz", "1.6 kHz", "2 kHz", "2.5 kHz", "3.15 kHz", "4 kHz", "5 kHz", "6.3 kHz", "8 kHz", "10 kHz", "12.5 kHz", "16 kHz", "BYPASS" };
static constexpr const char *TonePan990[] = { "RND", "ALT-L", "ALT-R" };
static constexpr const char *SoloSyncMaster990[] = { "OFF", "TONE-A", "TONE-B", "TONE-C", "TONE-D" };
static constexpr const char *VelocityRange990[] = { "ALL", "LOW", "HIGH" };
static constexpr const char *SetupEffectMode800[] = { "DRY", "REV", "CHO+REV", "DLY+REV" };
static constexpr const char *SetupEffectMode990[] = { "EQ:MIX", "EQ+R:MIX", "EQ+C+R:MIX", "EQ+D+R:MIX", "DIR1", "DIR2", "DIR3" };

static constexpr int PanKeyFollow990[] = { -100, -70, -50, -40, -30, -20, -10, 0, 10, 20, 30, 40, 50, 70, 100};

static int ATouchBendSens(int value)
{
	if (value == 0)
		return -36;
	if (value == 1)
		return -24;
	else
		return value - 14;
}

static int CutoffKeyFollow(int value)
{
	if (value <= 10)
		return (value - 10) * 10;
	else
		return (value - 10) * 5;
}

static int ToneDelay(int value)
{
	if (value <= 100)
		return value * 10;
	else if (value <= 120)
		return (value - 100) * 100;
	else if (value <= 125)
		return (value - 100) * 100;
	else if (value == 126)
		return 4500;
	else
		return 5000;
}

static int PhaserManual(int value)
{
	if (value < 26)
		return 50 + value * 10;
	else if (value == 26)
		return 320;
	else if (value < 50)
		return 350 + (value - 27) * 30;
	else if (value < 86)
		return 1100 + (value - 50) * 200;
	else
		return 8500 + (value - 86) * 500;
}

static double ChorusTime(int value)
{
	if (value < 50)
		return 0.1 + value * 0.1;
	else if (value < 60)
		return 5.0 + (value - 50) * 0.5;
	else
		return 10.0 + (value - 60);
}

static double DelayTime(int value)
{
	if (value < 100)
		return ChorusTime(value);
	else if (value < 116)
		return 40.0 + (value - 100) * 10.0;
	else
		return 200.0 + (value - 116) * 20.0;
}

static int ReverbTime(int value, uint8_t type)
{
	if (type <= 5)
	{
		if (value < 80)
			return 100 + value * 100;
		else if (value < 95)
			return 8000 + (value - 80) * 500;
		else
			return 16000 + (value - 95) * 1000;
	}
	else
	{
		return 5 + value * 5;
	}
}

static std::string KeyName(int key)
{
	static constexpr const char *KeyName[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
	std::string name = KeyName[key % 12];
	if (key < 12)
		name += "-1";
	else
		name += static_cast<char>('0' + (key - 12) / 12);
	return name;
}

static void PrintProperty(const char *name, bool value)
{
	std::cout << name << ": " << (value ? "ON" : "OFF") << std::endl;
}

static void PrintProperty(const char *name, int value, int offset = 0)
{
	std::cout << name << ": " << (value - offset) << std::endl;
}

static void PrintProperty(const char *name, double value)
{
	std::cout << name << ": " << value << std::endl;
}

static void PrintProperty(const char *name, const char *value)
{
	std::cout << name << ": " << value << std::endl;
}

static void PrintProperty(const char *name, const std::string_view value)
{
	std::cout << name << ": " << value << std::endl;
}

static void PrintLFO(const Tone800::LFO &lfo)
{
	PrintProperty("\t\t\tRate", lfo.rate);
	if (lfo.delay == 101)
		PrintProperty("\t\t\tDelay", "REL");
	else
		PrintProperty("\t\t\tDelay", lfo.delay);
	PrintProperty("\t\t\tFade", lfo.fade, 50);
	PrintProperty("\t\t\tWaveform", SafeTable(LFOWaveform800, lfo.waveform));
	PrintProperty("\t\t\tOffset", SafeTable(LFOOffset, lfo.offset));
	PrintProperty("\t\t\tKey Trigger", lfo.keyTrigger != 0);
}

static void PrintTone(const Tone800 &tone)
{
	std::cout << "\t\tCommon" << std::endl;
	PrintProperty("\t\t\tVelocity Curve", tone.common.velocityCurve + 1);
	PrintProperty("\t\t\tHold Control", tone.common.holdControl != 0);
	std::cout << "\t\tLFO 1" << std::endl;
	PrintLFO(tone.lfo1);
	std::cout << "\t\tLFO 2" << std::endl;
	PrintLFO(tone.lfo2);
	std::cout << "\t\tWG" << std::endl;
	PrintProperty("\t\t\tWave Source", SafeTable(WaveSource, tone.wg.waveSource));
	const int waveform = ((tone.wg.waveformMSB << 8) | tone.wg.waveformLSB) + 1;
	if (tone.wg.waveSource)
		PrintProperty("\t\t\tWaveform", waveform);
	else
		std::cout << "\t\t\tWaveform: " << waveform << " (" << SafeTable(WaveformNames, tone.wg.waveformLSB + 1) << ")" << std::endl;
	PrintProperty("\t\t\tPitch Coarse", tone.wg.pitchCoarse, 48);
	PrintProperty("\t\t\tPitch Fine", tone.wg.pitchFine, 50);
	PrintProperty("\t\t\tPitch Random", tone.wg.pitchRandom);
	PrintProperty("\t\t\tKey Follow", SafeTable(PitchKF, tone.wg.keyFollow));
	PrintProperty("\t\t\tBender Switch", tone.wg.benderSwitch != 0);
	PrintProperty("\t\t\tAftertouch Bend", tone.wg.aTouchBend != 0);
	PrintProperty("\t\t\tLFO 1 Amount", tone.wg.lfo1Sens, 50);
	PrintProperty("\t\t\tLFO 2 Amount", tone.wg.lfo2Sens, 50);
	PrintProperty("\t\t\tLever Destination", SafeTable(LFOSelect, (tone.wg.leverSens < 50) ? 1 : 0));
	PrintProperty("\t\t\tLever LFO Amount", std::abs(tone.wg.leverSens - 50));
	PrintProperty("\t\t\tAftertouch Destination", SafeTable(LFOSelect, (tone.wg.aTouchModSens < 50) ? 1 : 0));
	PrintProperty("\t\t\tAftertouch LFO Amount", std::abs(tone.wg.aTouchModSens - 50));
	std::cout << "\t\tPitch Envelope" << std::endl;
	PrintProperty("\t\t\tVelo", tone.pitchEnv.velo, 50);
	PrintProperty("\t\t\tTime Velo", tone.pitchEnv.timeVelo, 50);
	PrintProperty("\t\t\tTime Key Follow", tone.pitchEnv.timeKF, 10);
	PrintProperty("\t\t\tLevel 0", tone.pitchEnv.level0, 50);
	PrintProperty("\t\t\tTime 1", tone.pitchEnv.time1);
	PrintProperty("\t\t\tLevel 1", tone.pitchEnv.level1, 50);
	PrintProperty("\t\t\tTime 2", tone.pitchEnv.time2);
	PrintProperty("\t\t\tTime 3", tone.pitchEnv.time3);
	PrintProperty("\t\t\tLevel 2", tone.pitchEnv.level2, 50);
	std::cout << "\t\tTVF" << std::endl;
	PrintProperty("\t\t\tFilter Mode", SafeTable(FilterMode, tone.tvf.filterMode));
	PrintProperty("\t\t\tCutoff Frequency", tone.tvf.cutoffFreq);
	PrintProperty("\t\t\tResonance", tone.tvf.resonance);
	PrintProperty("\t\t\tKey Follow", CutoffKeyFollow(tone.tvf.keyFollow));
	PrintProperty("\t\t\tAftertouch Amount", tone.tvf.aTouchSens, 50);
	PrintProperty("\t\t\tLFO Source", SafeTable(LFOSelect, tone.tvf.lfoSelect));
	PrintProperty("\t\t\tLFO Depth", tone.tvf.lfoDepth, 50);
	PrintProperty("\t\t\tEnvelope Depth", tone.tvf.envDepth, 50);
	std::cout << "\t\tTVF Envelope" << std::endl;
	PrintProperty("\t\t\tVelo", tone.tvfEnv.velo, 50);
	PrintProperty("\t\t\tTime Velo", tone.tvfEnv.timeVelo, 50);
	PrintProperty("\t\t\tTime Key Follow", tone.tvfEnv.timeKF, 10);
	PrintProperty("\t\t\tTime 1", tone.tvfEnv.time1);
	PrintProperty("\t\t\tLevel 1", tone.tvfEnv.level1);
	PrintProperty("\t\t\tTime 2", tone.tvfEnv.time2);
	PrintProperty("\t\t\tLevel 2", tone.tvfEnv.level2);
	PrintProperty("\t\t\tTime 3", tone.tvfEnv.time3);
	PrintProperty("\t\t\tSustain Level", tone.tvfEnv.sustainLevel);
	PrintProperty("\t\t\tTime 4", tone.tvfEnv.time4);
	PrintProperty("\t\t\tLevel 4", tone.tvfEnv.level4);
	std::cout << "\t\tTVA" << std::endl;
	PrintProperty("\t\t\tBias Direction", SafeTable(BiasDirection, tone.tva.biasDirection));
	PrintProperty("\t\t\tBias Point", KeyName(tone.tva.biasPoint));
	PrintProperty("\t\t\tBias Level", tone.tva.biasLevel, 10);
	PrintProperty("\t\t\tLevel", tone.tva.level);
	PrintProperty("\t\t\tAftertouch Amount", tone.tva.aTouchSens, 50);
	PrintProperty("\t\t\tLFO Source", SafeTable(LFOSelect, tone.tva.lfoSelect));
	PrintProperty("\t\t\tLFO Depth", tone.tva.lfoDepth, 50);
	std::cout << "\t\tTVA Envelope" << std::endl;
	PrintProperty("\t\t\tVelo", tone.tvaEnv.velo, 50);
	PrintProperty("\t\t\tTime Velo", tone.tvaEnv.timeVelo, 50);
	PrintProperty("\t\t\tTime Key Follow", tone.tvaEnv.timeKF, 10);
	PrintProperty("\t\t\tTime 1", tone.tvaEnv.time1);
	PrintProperty("\t\t\tLevel 1", tone.tvaEnv.level1);
	PrintProperty("\t\t\tTime 2", tone.tvaEnv.time2);
	PrintProperty("\t\t\tLevel 2", tone.tvaEnv.level2);
	PrintProperty("\t\t\tTime 3", tone.tvaEnv.time3);
	PrintProperty("\t\t\tSustain Level", tone.tvaEnv.sustainLevel);
	PrintProperty("\t\t\tTime 4", tone.tvaEnv.time4);
}

static void PrintTone(const Tone800 &tone, bool enabled, bool selected, uint8_t keyRangeLow, uint8_t keyRangeHigh)
{
	PrintProperty("\t\tEnabled", enabled);
	PrintProperty("\t\tSelected", selected);
	PrintProperty("\t\tKey Range Low", KeyName(keyRangeLow));
	PrintProperty("\t\tKey Range High", KeyName(keyRangeHigh));
	PrintTone(tone);
}

void PrintEQ(const EQ800 &eq)
{
	std::cout << "\tEQ" << std::endl;
	PrintProperty("\t\tLow Frequency", SafeTable(EQLowFreq, eq.lowFreq));
	PrintProperty("\t\tLow Gain", eq.lowGain, 15);
	PrintProperty("\t\tMid Frequency", SafeTable(EQMidFreq, eq.midFreq));
	PrintProperty("\t\tMid Q", SafeTable(EQMidQ, eq.midQ) * 0.1);
	PrintProperty("\t\tMid Gain", eq.midGain, 15);
	PrintProperty("\t\tHigh Frequency", SafeTable(EQHighFreq, eq.highFreq));
	PrintProperty("\t\tHigh Gain", eq.highGain, 15);
}

void PrintPatch(const Patch800 &patch)
{
	std::cout << "\tCommon" << std::endl;
	PrintProperty("\t\tPatch Level", patch.common.patchLevel);
	PrintProperty("\t\tBender Range Down", patch.common.benderRangeDown);
	PrintProperty("\t\tBender Range Up", patch.common.benderRangeUp);
	PrintProperty("\t\tAftertouch Bend Amount", ATouchBendSens(patch.common.aTouchBend));
	PrintProperty("\t\tSolo Switch", patch.common.soloSW != 0);
	PrintProperty("\t\tSolo Legato", patch.common.soloLegato != 0);
	PrintProperty("\t\tPortamento Switch", patch.common.portamentoSW != 0);
	PrintProperty("\t\tPortamento Mode", patch.common.portamentoMode ? "LEGATO" : "NORMAL");
	PrintProperty("\t\tPortamento Time", patch.common.portamentoTime);
	PrintEQ(patch.eq);
	std::cout << "\tMIDI TX" << std::endl;
	PrintProperty("\t\tKey Mode", SafeTable(KeyMode800, patch.midiTx.keyMode));
	PrintProperty("\t\tSplit Point", KeyName(patch.midiTx.splitPoint + 24));
	PrintProperty("\t\tLower Channel", patch.midiTx.lowerChannel + 1);
	PrintProperty("\t\tUpper Channel", patch.midiTx.upperChannel + 1);
	PrintProperty("\t\tLower Program Change", patch.midiTx.lowerProgramChange + 1);
	PrintProperty("\t\tUpper Program Change", patch.midiTx.upperProgramChange + 1);
	PrintProperty("\t\tHold Mode", SafeTable(HoldMode800, patch.midiTx.holdMode));
	std::cout << "\tEffects" << std::endl;
	PrintProperty("\t\tGroup A Sequence", SafeTable(FXGroupASequence, patch.effect.groupAsequence));
	PrintProperty("\t\tGroup B Sequence", SafeTable(FXGroupBSequence, patch.effect.groupBsequence));
	PrintProperty("\t\tGroup A Block 1 Switch", patch.effect.groupAblockSwitch1 != 0);
	PrintProperty("\t\tGroup A Block 2 Switch", patch.effect.groupAblockSwitch2 != 0);
	PrintProperty("\t\tGroup A Block 3 Switch", patch.effect.groupAblockSwitch3 != 0);
	PrintProperty("\t\tGroup A Block 4 Switch", patch.effect.groupAblockSwitch4 != 0);
	PrintProperty("\t\tGroup B Block 1 Switch", patch.effect.groupBblockSwitch1 != 0);
	PrintProperty("\t\tGroup B Block 2 Switch", patch.effect.groupBblockSwitch2 != 0);
	PrintProperty("\t\tGroup B Block 3 Switch", patch.effect.groupBblockSwitch3 != 0);
	PrintProperty("\t\tGroup B Effects Balance", patch.effect.effectsBalanceGroupB);
	PrintProperty("\t\tDistortion Type", SafeTable(DistortionType, patch.effect.distortionType));
	PrintProperty("\t\tDistortion Drive", patch.effect.distortionDrive);
	PrintProperty("\t\tDistortion Level", patch.effect.distortionLevel);
	PrintProperty("\t\tPhaser Manual", PhaserManual(patch.effect.phaserManual));
	PrintProperty("\t\tPhaser Rate (Hz)", patch.effect.phaserRate * 0.1 + 0.1);
	PrintProperty("\t\tPhaser Depth", patch.effect.phaserDepth);
	PrintProperty("\t\tPhaser Resonance", patch.effect.phaserResonance);
	PrintProperty("\t\tPhaser Mix", patch.effect.phaserMix);
	PrintProperty("\t\tSpectrum Band 1", patch.effect.spectrumBand1);
	PrintProperty("\t\tSpectrum Band 2", patch.effect.spectrumBand2);
	PrintProperty("\t\tSpectrum Band 3", patch.effect.spectrumBand3);
	PrintProperty("\t\tSpectrum Band 4", patch.effect.spectrumBand4);
	PrintProperty("\t\tSpectrum Band 5", patch.effect.spectrumBand5);
	PrintProperty("\t\tSpectrum Band 6", patch.effect.spectrumBand6);
	PrintProperty("\t\tSpectrum Bandwidth", patch.effect.spectrumBandwidth);
	PrintProperty("\t\tEnhancer Sensitivity", patch.effect.enhancerSens);
	PrintProperty("\t\tEnhancer Mix", patch.effect.enhancerMix);
	PrintProperty("\t\tDelay Center Tap (ms)", DelayTime(patch.effect.delayCenterTap));
	PrintProperty("\t\tDelay Center Level", patch.effect.delayCenterLevel);
	PrintProperty("\t\tDelay Left Tap (ms)", DelayTime(patch.effect.delayLeftTap));
	PrintProperty("\t\tDelay Left Level", patch.effect.delayLeftLevel);
	PrintProperty("\t\tDelay Right Tap (ms)", DelayTime(patch.effect.delayRightTap));
	PrintProperty("\t\tDelay Right Level", patch.effect.delayRightLevel);
	PrintProperty("\t\tDelay Feedback", patch.effect.delayFeedback);
	PrintProperty("\t\tChorus Rate (Hz)", 0.1 + patch.effect.chorusRate * 0.1);
	PrintProperty("\t\tChorus Depth", patch.effect.chorusDepth);
	PrintProperty("\t\tChorus Delay Time (ms)", ChorusTime(patch.effect.chorusDelayTime));
	PrintProperty("\t\tChorus Feedback", -98 + patch.effect.chorusFeedback * 2);
	PrintProperty("\t\tChorus Level", patch.effect.chorusLevel);
	PrintProperty("\t\tReverb Type", SafeTable(ReverbType, patch.effect.reverbType));
	PrintProperty("\t\tReverb Pre-Delay", patch.effect.reverbPreDelay);
	PrintProperty("\t\tReverb Early Reflections Level", patch.effect.reverbEarlyRefLevel);
	PrintProperty("\t\tReverb HF Damp", SafeTable(ReverbHFDamp, patch.effect.reverbHFDamp));
	PrintProperty("\t\tReverb Time (ms)", ReverbTime(patch.effect.reverbTime, patch.effect.reverbType));
	PrintProperty("\t\tReverb Level", patch.effect.reverbLevel);
	std::cout << "\tTone A" << std::endl;
	PrintTone(patch.toneA, patch.common.layerTone & 1, patch.common.activeTone & 1, patch.common.keyRangeLowA, patch.common.keyRangeHighA);
	std::cout << "\tTone B" << std::endl;
	PrintTone(patch.toneB, patch.common.layerTone & 2, patch.common.activeTone & 2, patch.common.keyRangeLowB, patch.common.keyRangeHighB);
	std::cout << "\tTone C" << std::endl;
	PrintTone(patch.toneC, patch.common.layerTone & 4, patch.common.activeTone & 4, patch.common.keyRangeLowC, patch.common.keyRangeHighC);
	std::cout << "\tTone D" << std::endl;
	PrintTone(patch.toneD, patch.common.layerTone & 8, patch.common.activeTone & 8, patch.common.keyRangeLowD, patch.common.keyRangeHighD);
}

void PrintSetup(const SpecialSetup800 &setup)
{
	std::cout << "\tCommon" << std::endl;
	PrintProperty("\t\tBender Range Down", setup.common.benderRangeDown);
	PrintProperty("\t\tBender Range Up", setup.common.benderRangeUp);
	PrintProperty("\t\tAftertouch Bend Amount", ATouchBendSens(setup.common.aTouchBendSens));
	PrintEQ(setup.eq);
	for (int i = 0; i < 61; i++)
	{
		std::cout << "\tKey " << KeyName(i + 24) << ": " << ToString(setup.keys[i].name) << std::endl;
		PrintProperty("\t\tEnvelope Mode", setup.keys[i].envMode ? "NO SUSTAIN" : "SUSTAIN");
		PrintProperty("\t\tMute Group", setup.keys[i].muteGroup ? std::string(1, 'A' + setup.keys[i].muteGroup - 1) : "OFF");
		PrintProperty("\t\tPan", setup.keys[i].pan, 30);
		PrintProperty("\t\tEffect Mode", SafeTable(SetupEffectMode800, setup.keys[i].effectMode));
		PrintProperty("\t\tEffect Level", setup.keys[i].effectLevel);
		PrintTone(setup.keys[i].tone);
	}
}

static void PrintLFO(const Tone990::LFO &lfo)
{
	PrintProperty("\t\t\tRate", lfo.rate);
	if (lfo.delay == 101)
		PrintProperty("\t\t\tDelay", "REL");
	else
		PrintProperty("\t\t\tDelay", lfo.delay);
	PrintProperty("\t\t\tFade", lfo.fade, 50);
	PrintProperty("\t\t\tWaveform", SafeTable(LFOWaveform990, lfo.waveform));
	PrintProperty("\t\t\tOffset", SafeTable(LFOOffset, lfo.offset));
	PrintProperty("\t\t\tKey Trigger", lfo.keyTrigger != 0);
	PrintProperty("\t\t\tPitch Depth", lfo.depthPitch, 50);
	PrintProperty("\t\t\tTVF Depth", lfo.depthTVF, 50);
	PrintProperty("\t\t\tTVA Depth", lfo.depthTVA, 50);
}

static void PrintControlSource(const Tone990::ControlSource &cs)
{
	PrintProperty("\t\t\tDestination 1", SafeTable(ControlDest990, cs.destination1));
	PrintProperty("\t\t\tDepth 1", cs.depth1, 50);
	PrintProperty("\t\t\tDestination 2", SafeTable(ControlDest990, cs.destination2));
	PrintProperty("\t\t\tDepth 2", cs.depth2, 50);
	PrintProperty("\t\t\tDestination 3", SafeTable(ControlDest990, cs.destination3));
	PrintProperty("\t\t\tDepth 3", cs.depth3, 50);
	PrintProperty("\t\t\tDestination 4", SafeTable(ControlDest990, cs.destination4));
	PrintProperty("\t\t\tDepth 4", cs.depth4, 50);
}

static void PrintTone(const Tone990 &tone)
{
	std::cout << "\t\tCommon" << std::endl;
	PrintProperty("\t\t\tVelocity Curve", tone.common.velocityCurve + 1);
	PrintProperty("\t\t\tHold Control", tone.common.holdControl != 0);
	std::cout << "\t\tLFO 1" << std::endl;
	PrintLFO(tone.lfo1);
	std::cout << "\t\tLFO 2" << std::endl;
	PrintLFO(tone.lfo2);
	std::cout << "\t\tWG" << std::endl;
	PrintProperty("\t\t\tWave Source", SafeTable(WaveSource, tone.wg.waveSource));
	const int waveform = ((tone.wg.waveformMSB << 8) | tone.wg.waveformLSB) + 1;
	if (tone.wg.waveSource)
		PrintProperty("\t\t\tWaveform", waveform);
	else
		std::cout << "\t\t\tWaveform: " << waveform << " (" << SafeTable(WaveformNames, tone.wg.waveformLSB + 1) << ")" << std::endl;
	PrintProperty("\t\t\tPitch Coarse", tone.wg.pitchCoarse, 48);
	PrintProperty("\t\t\tPitch Fine", tone.wg.pitchFine, 50);
	PrintProperty("\t\t\tPitch Random", tone.wg.pitchRandom);
	PrintProperty("\t\t\tKey Follow", SafeTable(PitchKF, tone.wg.keyFollow));
	PrintProperty("\t\t\tBender Switch", tone.wg.benderSwitch != 0);
	PrintProperty("\t\t\tFXM Color", tone.wg.fxmColor + 1);
	PrintProperty("\t\t\tFXM Depth", tone.wg.fxmDepth);
	PrintProperty("\t\t\tSync Slave Switch", tone.wg.syncSlaveSwitch != 0);
	PrintProperty("\t\t\tTone Delay Mode", SafeTable(ToneDelayMode990, tone.wg.toneDelayMode));
	PrintProperty("\t\t\tTone Delay Time (ms)", ToneDelay(tone.wg.toneDelayTime));
	PrintProperty("\t\t\tEnvelope Depth", tone.wg.envDepth, 12);
	std::cout << "\t\tPitch Envelope" << std::endl;
	PrintProperty("\t\t\tVelo", tone.pitchEnv.velo, 50);
	PrintProperty("\t\t\tTime Velo", tone.pitchEnv.timeVelo, 50);
	PrintProperty("\t\t\tTime Key Follow", tone.pitchEnv.timeKF, 10);
	PrintProperty("\t\t\tLevel 0", tone.pitchEnv.level0, 50);
	PrintProperty("\t\t\tTime 1", tone.pitchEnv.time1);
	PrintProperty("\t\t\tLevel 1", tone.pitchEnv.level1, 50);
	PrintProperty("\t\t\tTime 2", tone.pitchEnv.time2);
	PrintProperty("\t\t\tTime 3", tone.pitchEnv.time3);
	PrintProperty("\t\t\tLevel 3", tone.pitchEnv.level3, 50);
	std::cout << "\t\tTVF" << std::endl;
	PrintProperty("\t\t\tFilter Mode", SafeTable(FilterMode, tone.tvf.filterMode));
	PrintProperty("\t\t\tCutoff Frequency", tone.tvf.cutoffFreq);
	PrintProperty("\t\t\tResonance", tone.tvf.resonance);
	PrintProperty("\t\t\tKey Follow", CutoffKeyFollow(tone.tvf.keyFollow));
	PrintProperty("\t\t\tEnvelope Depth", tone.tvf.envDepth, 50);
	std::cout << "\t\tTVF Envelope" << std::endl;
	PrintProperty("\t\t\tVelo", tone.tvfEnv.velo, 50);
	PrintProperty("\t\t\tTime Velo", tone.tvfEnv.timeVelo, 50);
	PrintProperty("\t\t\tTime Key Follow", tone.tvfEnv.timeKF, 10);
	PrintProperty("\t\t\tTime 1", tone.tvfEnv.time1);
	PrintProperty("\t\t\tLevel 1", tone.tvfEnv.level1);
	PrintProperty("\t\t\tTime 2", tone.tvfEnv.time2);
	PrintProperty("\t\t\tLevel 2", tone.tvfEnv.level2);
	PrintProperty("\t\t\tTime 3", tone.tvfEnv.time3);
	PrintProperty("\t\t\tSustain Level", tone.tvfEnv.sustainLevel);
	PrintProperty("\t\t\tTime 4", tone.tvfEnv.time4);
	PrintProperty("\t\t\tLevel 4", tone.tvfEnv.level4);
	std::cout << "\t\tTVA" << std::endl;
	PrintProperty("\t\t\tBias Direction", SafeTable(BiasDirection, tone.tva.biasDirection));
	PrintProperty("\t\t\tBias Point", KeyName(tone.tva.biasPoint));
	PrintProperty("\t\t\tBias Level", tone.tva.biasLevel, 10);
	PrintProperty("\t\t\tLevel", tone.tva.level);
	if (tone.tva.pan <= 100)
		PrintProperty("\t\t\tPan", tone.tva.pan, 50);
	else
		PrintProperty("\t\t\tPan", SafeTable(TonePan990, tone.tva.pan - 101));
	PrintProperty("\t\t\tPan Key Follow", SafeTable(PanKeyFollow990, tone.tva.panKeyFollow));
	std::cout << "\t\tTVA Envelope" << std::endl;
	PrintProperty("\t\t\tVelo", tone.tvaEnv.velo, 50);
	PrintProperty("\t\t\tTime Velo", tone.tvaEnv.timeVelo, 50);
	PrintProperty("\t\t\tTime Key Follow", tone.tvaEnv.timeKF, 10);
	PrintProperty("\t\t\tTime 1", tone.tvaEnv.time1);
	PrintProperty("\t\t\tLevel 1", tone.tvaEnv.level1);
	PrintProperty("\t\t\tTime 2", tone.tvaEnv.time2);
	PrintProperty("\t\t\tLevel 2", tone.tvaEnv.level2);
	PrintProperty("\t\t\tTime 3", tone.tvaEnv.time3);
	PrintProperty("\t\t\tSustain Level", tone.tvaEnv.sustainLevel);
	PrintProperty("\t\t\tTime 4", tone.tvaEnv.time4);
	std::cout << "\t\tControl Source 1" << std::endl;
	PrintControlSource(tone.cs1);
	std::cout << "\t\tControl Source 2" << std::endl;
	PrintControlSource(tone.cs2);
}

static void PrintTone(const Tone990 &tone, bool enabled, bool selected, uint8_t keyRangeLow, uint8_t keyRangeHigh, uint8_t velocityRange, uint8_t velocityPoint, uint8_t velocityFade)
{
	PrintProperty("\t\tEnabled", enabled);
	PrintProperty("\t\tSelected", selected);
	PrintProperty("\t\tKey Range Low", KeyName(keyRangeLow));
	PrintProperty("\t\tKey Range High", KeyName(keyRangeHigh));
	PrintProperty("\t\tVelocity Range", SafeTable(VelocityRange990, velocityRange));
	PrintProperty("\t\tVelocity Point", velocityPoint);
	PrintProperty("\t\tVelocity Fade", velocityFade);
	PrintTone(tone);
}

void PrintEQ(const EQ990 &eq)
{
	std::cout << "\tEQ" << std::endl;
	PrintProperty("\t\tLow Frequency", SafeTable(EQLowFreq, eq.lowFreq));
	PrintProperty("\t\tLow Gain", eq.lowGain, 15);
	PrintProperty("\t\tMid Frequency", SafeTable(EQMidFreq, eq.midFreq));
	PrintProperty("\t\tMid Q", SafeTable(EQMidQ, eq.midQ) * 0.1);
	PrintProperty("\t\tMid Gain", eq.midGain, 15);
	PrintProperty("\t\tHigh Frequency", SafeTable(EQHighFreq, eq.highFreq));
	PrintProperty("\t\tHigh Gain", eq.highGain, 15);
}

void PrintPatch(const Patch990 &patch)
{
	std::cout << "\tCommon" << std::endl;
	PrintProperty("\t\tPatch Level", patch.common.patchLevel);
	PrintProperty("\t\tPatch Pan", patch.common.patchPan, 50);
	PrintProperty("\t\tAnalog Feel", patch.common.analogFeel);
	PrintProperty("\t\tVoice Priority", patch.common.voicePriority ? "LOUDEST" : "LAST");
	PrintProperty("\t\tBender Range Down", patch.common.bendRangeDown);
	PrintProperty("\t\tBender Range Up", patch.common.bendRangeUp);
	PrintProperty("\t\tTone Control Source 1", SafeTable(ControlSource990, patch.common.toneControlSource1));
	PrintProperty("\t\tTone Control Source 2", SafeTable(ControlSource990, patch.common.toneControlSource2));
	PrintProperty("\t\tOctave Switch", patch.octaveSwitch);
	std::cout << "\tKey Effects" << std::endl;
	PrintProperty("\t\tSolo Switch", patch.keyEffects.soloSW != 0);
	PrintProperty("\t\tSolo Legato", patch.keyEffects.soloLegato != 0);
	PrintProperty("\t\tSolo Sync Master", SafeTable(SoloSyncMaster990, patch.keyEffects.soloSyncMaster));
	PrintProperty("\t\tPortamento Switch", patch.keyEffects.portamentoSW != 0);
	PrintProperty("\t\tPortamento Mode", patch.keyEffects.portamentoMode ? "LEGATO" : "NORMAL");
	PrintProperty("\t\tPortamento Type", patch.keyEffects.portamentoType ? "RATE" : "TIME");
	PrintProperty("\t\tPortamento Time", patch.keyEffects.portamentoTime);
	PrintEQ(patch.eq);
	std::cout << "\tStructure Type" << std::endl;
	PrintProperty("\t\tTone A/B Structure", patch.structureType.structureAB);
	PrintProperty("\t\tTone C/D Structure", patch.structureType.structureCD);
	std::cout << "\tEffects" << std::endl;
	PrintProperty("\t\tControl Source 1", SafeTable(ControlSource990, patch.effect.controlSource1));
	PrintProperty("\t\tControl Destination 1", SafeTable(ControlDestFX990, patch.effect.controlDest1));
	PrintProperty("\t\tControl Depth 1", patch.effect.controlDepth1, 50);
	PrintProperty("\t\tControl Source 2", SafeTable(ControlSource990, patch.effect.controlSource2));
	PrintProperty("\t\tControl Destination 2", SafeTable(ControlDestFX990, patch.effect.controlDest2));
	PrintProperty("\t\tControl Depth 2", patch.effect.controlDepth2, 50);
	PrintProperty("\t\tGroup A Sequence", SafeTable(FXGroupASequence, patch.effect.groupAsequence));
	PrintProperty("\t\tGroup B Sequence", SafeTable(FXGroupBSequence, patch.effect.groupBsequence));
	PrintProperty("\t\tGroup A Block 1 Switch", patch.effect.groupAblockSwitch1 != 0);
	PrintProperty("\t\tGroup A Block 2 Switch", patch.effect.groupAblockSwitch2 != 0);
	PrintProperty("\t\tGroup A Block 3 Switch", patch.effect.groupAblockSwitch3 != 0);
	PrintProperty("\t\tGroup A Block 4 Switch", patch.effect.groupAblockSwitch4 != 0);
	PrintProperty("\t\tGroup B Block 1 Switch", patch.effect.groupBblockSwitch1 != 0);
	PrintProperty("\t\tGroup B Block 2 Switch", patch.effect.groupBblockSwitch2 != 0);
	PrintProperty("\t\tGroup B Block 3 Switch", patch.effect.groupBblockSwitch3 != 0);
	PrintProperty("\t\tGroup B Effects Balance", patch.effect.effectsBalanceGroupB);
	PrintProperty("\t\tDistortion Type", SafeTable(DistortionType, patch.effect.distortionType));
	PrintProperty("\t\tDistortion Drive", patch.effect.distortionDrive);
	PrintProperty("\t\tDistortion Level", patch.effect.distortionLevel);
	PrintProperty("\t\tPhaser Manual", PhaserManual(patch.effect.phaserManual));
	PrintProperty("\t\tPhaser Rate (Hz)", patch.effect.phaserRate * 0.1 + 0.1);
	PrintProperty("\t\tPhaser Depth", patch.effect.phaserDepth);
	PrintProperty("\t\tPhaser Resonance", patch.effect.phaserResonance);
	PrintProperty("\t\tPhaser Mix", patch.effect.phaserMix);
	PrintProperty("\t\tSpectrum Band 1", patch.effect.spectrumBand1);
	PrintProperty("\t\tSpectrum Band 2", patch.effect.spectrumBand2);
	PrintProperty("\t\tSpectrum Band 3", patch.effect.spectrumBand3);
	PrintProperty("\t\tSpectrum Band 4", patch.effect.spectrumBand4);
	PrintProperty("\t\tSpectrum Band 5", patch.effect.spectrumBand5);
	PrintProperty("\t\tSpectrum Band 6", patch.effect.spectrumBand6);
	PrintProperty("\t\tSpectrum Bandwidth", patch.effect.spectrumBandwidth);
	PrintProperty("\t\tEnhancer Sensitivity", patch.effect.enhancerSens);
	PrintProperty("\t\tEnhancer Mix", patch.effect.enhancerMix);
	PrintProperty("\t\tDelay Mode", SafeTable(DelayMode990, patch.effect.delayMode));
	if (patch.effect.delayCenterTapMSB)
		PrintProperty("\t\tDelay Center Tap", SafeTable(DelayTime990, patch.effect.delayCenterTapLSB));
	else
		PrintProperty("\t\tDelay Center Tap (ms)", DelayTime(patch.effect.delayCenterTapLSB));
	PrintProperty("\t\tDelay Center Level", patch.effect.delayCenterLevel);
	if (patch.effect.delayLeftTapMSB)
		PrintProperty("\t\tDelay Left Tap", SafeTable(DelayTime990, patch.effect.delayLeftTapLSB));
	else
		PrintProperty("\t\tDelay Left Tap (ms)", DelayTime(patch.effect.delayLeftTapLSB));
	PrintProperty("\t\tDelay Left Level", patch.effect.delayLeftLevel);
	if (patch.effect.delayRightTapMSB)
		PrintProperty("\t\tDelay Right  Tap", SafeTable(DelayTime990, patch.effect.delayRightTapLSB));
	else
		PrintProperty("\t\tDelay Right Tap (ms)", DelayTime(patch.effect.delayRightTapLSB));
	PrintProperty("\t\tDelay Right Level", patch.effect.delayRightLevel);
	PrintProperty("\t\tDelay Feedback", patch.effect.delayFeedback);
	PrintProperty("\t\tChorus Rate (Hz)", 0.1 + patch.effect.chorusRate * 0.1);
	PrintProperty("\t\tChorus Depth", patch.effect.chorusDepth);
	PrintProperty("\t\tChorus Delay Time (ms)", ChorusTime(patch.effect.chorusDelayTime));
	PrintProperty("\t\tChorus Feedback", -98 + patch.effect.chorusFeedback * 2);
	PrintProperty("\t\tChorus Level", patch.effect.chorusLevel);
	PrintProperty("\t\tReverb Type", SafeTable(ReverbType, patch.effect.reverbType));
	PrintProperty("\t\tReverb Pre-Delay", patch.effect.reverbPreDelay);
	PrintProperty("\t\tReverb Early Reflections Level", patch.effect.reverbEarlyRefLevel);
	PrintProperty("\t\tReverb HF Damp", SafeTable(ReverbHFDamp, patch.effect.reverbHFDamp));
	PrintProperty("\t\tReverb Time (ms)", ReverbTime(patch.effect.reverbTime, patch.effect.reverbType));
	PrintProperty("\t\tReverb Level", patch.effect.reverbLevel);
	std::cout << "\tTone A" << std::endl;
	PrintTone(patch.toneA, patch.common.layerTone & 1, patch.common.activeTone & 1, patch.keyRanges.keyRangeLowA, patch.keyRanges.keyRangeHighA, patch.velocity.velocityRange1, patch.velocity.velocityPoint1, patch.velocity.velocityFade1);
	std::cout << "\tTone B" << std::endl;
	PrintTone(patch.toneB, patch.common.layerTone & 2, patch.common.activeTone & 2, patch.keyRanges.keyRangeLowB, patch.keyRanges.keyRangeHighB, patch.velocity.velocityRange2, patch.velocity.velocityPoint2, patch.velocity.velocityFade2);
	std::cout << "\tTone C" << std::endl;
	PrintTone(patch.toneC, patch.common.layerTone & 4, patch.common.activeTone & 4, patch.keyRanges.keyRangeLowC, patch.keyRanges.keyRangeHighC, patch.velocity.velocityRange3, patch.velocity.velocityPoint3, patch.velocity.velocityFade3);
	std::cout << "\tTone D" << std::endl;
	PrintTone(patch.toneD, patch.common.layerTone & 8, patch.common.activeTone & 8, patch.keyRanges.keyRangeLowD, patch.keyRanges.keyRangeHighD, patch.velocity.velocityRange4, patch.velocity.velocityPoint4, patch.velocity.velocityFade4);
}

void PrintSetup(const SpecialSetup990 &setup)
{
	std::cout << "\tCommon" << std::endl;
	PrintProperty("\t\tLevel", setup.common.level);
	PrintProperty("\t\tPan", setup.common.pan, 50);
	PrintProperty("\t\tAnalog Feel", setup.common.analogFeel);
	PrintProperty("\t\tBender Range Down", setup.common.benderRangeDown);
	PrintProperty("\t\tBender Range Up", setup.common.benderRangeUp);
	PrintProperty("\t\tTone Control Source 1", SafeTable(ControlSource990, setup.common.toneControlSource1));
	PrintProperty("\t\tTone Control Source 2", SafeTable(ControlSource990, setup.common.toneControlSource2));
	PrintEQ(setup.eq);
	std::cout << "\tEffects" << std::endl;
	PrintProperty("\t\tControl Source 1", SafeTable(ControlSource990, setup.effect.controlSource1));
	PrintProperty("\t\tControl Destination 1", SafeTable(ControlDestFX990, setup.effect.controlDest1));
	PrintProperty("\t\tControl Depth 1", setup.effect.controlDepth1, 50);
	PrintProperty("\t\tControl Source 2", SafeTable(ControlSource990, setup.effect.controlSource2));
	PrintProperty("\t\tControl Destination 2", SafeTable(ControlDestFX990, setup.effect.controlDest2));
	PrintProperty("\t\tControl Depth 2", setup.effect.controlDepth2, 50);
	PrintProperty("\t\tDelay Mode", SafeTable(DelayMode990, setup.effect.delayMode));
	if (setup.effect.delayCenterTapMSB)
		PrintProperty("\t\tDelay Center Tap", SafeTable(DelayTime990, setup.effect.delayCenterTapLSB));
	else
		PrintProperty("\t\tDelay Center Tap (ms)", DelayTime(setup.effect.delayCenterTapLSB));
	PrintProperty("\t\tDelay Center Level", setup.effect.delayCenterLevel);
	if (setup.effect.delayLeftTapMSB)
		PrintProperty("\t\tDelay Left Tap", SafeTable(DelayTime990, setup.effect.delayLeftTapLSB));
	else
		PrintProperty("\t\tDelay Left Tap (ms)", DelayTime(setup.effect.delayLeftTapLSB));
	PrintProperty("\t\tDelay Left Level", setup.effect.delayLeftLevel);
	if (setup.effect.delayRightTapMSB)
		PrintProperty("\t\tDelay Right  Tap", SafeTable(DelayTime990, setup.effect.delayRightTapLSB));
	else
		PrintProperty("\t\tDelay Right Tap (ms)", DelayTime(setup.effect.delayRightTapLSB));
	PrintProperty("\t\tDelay Right Level", setup.effect.delayRightLevel);
	PrintProperty("\t\tDelay Feedback", setup.effect.delayFeedback);
	PrintProperty("\t\tChorus Rate (Hz)", 0.1 + setup.effect.chorusRate * 0.1);
	PrintProperty("\t\tChorus Depth", setup.effect.chorusDepth);
	PrintProperty("\t\tChorus Delay Time (ms)", ChorusTime(setup.effect.chorusDelayTime));
	PrintProperty("\t\tChorus Feedback", -98 + setup.effect.chorusFeedback * 2);
	PrintProperty("\t\tChorus Level", setup.effect.chorusLevel);
	PrintProperty("\t\tReverb Type", SafeTable(ReverbType, setup.effect.reverbType));
	PrintProperty("\t\tReverb Pre-Delay", setup.effect.reverbPreDelay);
	PrintProperty("\t\tReverb Early Reflections Level", setup.effect.reverbEarlyRefLevel);
	PrintProperty("\t\tReverb HF Damp", SafeTable(ReverbHFDamp, setup.effect.reverbHFDamp));
	PrintProperty("\t\tReverb Time (ms)", ReverbTime(setup.effect.reverbTime, setup.effect.reverbType));
	PrintProperty("\t\tReverb Level", setup.effect.reverbLevel);
	for (int i = 0; i < 61; i++)
	{
		std::cout << "\tKey " << KeyName(i + 24) << ": " << ToString(setup.keys[i].name) << std::endl;
		PrintProperty("\t\tEnvelope Mode", setup.keys[i].envMode ? "NO SUSTAIN" : "SUSTAIN");
		PrintProperty("\t\tMute Group", setup.keys[i].muteGroup ? std::string(1, 'A' + setup.keys[i].muteGroup - 1) : "OFF");
		PrintProperty("\t\tEffect Mode", SafeTable(SetupEffectMode990, setup.keys[i].effectMode));
		PrintProperty("\t\tEffect Level", setup.keys[i].effectLevel);
		PrintTone(setup.keys[i].tone);
	}
}

static void PrintLFO(const ToneVST::LFO &lfo)
{
	PrintProperty("\t\t\tTempo Sync", lfo.tempoSync != 0);
	if(lfo.tempoSync)
		PrintProperty("\t\t\tRate", SafeTable(TempoSyncVST, lfo.rateWithTempoSync));
	else
		PrintProperty("\t\t\tRate", lfo.rate);
	if (lfo.delay == 101)
		PrintProperty("\t\t\tDelay", "REL");
	else
		PrintProperty("\t\t\tDelay", lfo.delay);
	PrintProperty("\t\t\tFade", lfo.fade);
	PrintProperty("\t\t\tWaveform", SafeTable(LFOWaveform800, lfo.waveform));
	PrintProperty("\t\t\tOffset", SafeTable(LFOOffset, 2 - lfo.offset));
	PrintProperty("\t\t\tKey Trigger", lfo.keyTrigger != 0);
}

static void PrintTone(const ToneVST &tone, uint8_t keyRangeLow, uint8_t keyRangeHigh)
{
	PrintProperty("\t\tEnabled", tone.common.layerEnabled != 0);
	PrintProperty("\t\tSelected", tone.common.layerSelected != 0);
	PrintProperty("\t\tKey Range Low", KeyName(keyRangeLow));
	PrintProperty("\t\tKey Range High", KeyName(keyRangeHigh));
	std::cout << "\t\tCommon" << std::endl;
	PrintProperty("\t\t\tVelocity Curve", tone.common.velocityCurve + 1);
	PrintProperty("\t\t\tHold Control", tone.common.holdControl != 0);
	std::cout << "\t\tLFO 1" << std::endl;
	PrintLFO(tone.lfo1);
	std::cout << "\t\tLFO 2" << std::endl;
	PrintLFO(tone.lfo2);
	std::cout << "\t\tWG" << std::endl;
	const char *waveformName = SafeTable(WaveformNames, tone.wg.waveformLSB);
	if (tone.wg.waveformLSB == 88)
		waveformName = WaveformNames[89];
	else if (tone.wg.waveformLSB == 89)
		waveformName = WaveformNames[88];
	std::cout << "\t\t\tWaveform: " << static_cast<int>(tone.wg.waveformLSB) << " (" << waveformName << ")" << std::endl;
	PrintProperty("\t\t\tGain (dB)", (tone.wg.gain - 3) * 6);
	PrintProperty("\t\t\tPitch Coarse", tone.wg.pitchCoarse);
	PrintProperty("\t\t\tPitch Fine", tone.wg.pitchFine);
	PrintProperty("\t\t\tPitch Random", tone.wg.pitchRandom);
	PrintProperty("\t\t\tKey Follow", SafeTable(PitchKF, tone.wg.keyFollow));
	PrintProperty("\t\t\tBender Switch", tone.wg.benderSwitch != 0);
	PrintProperty("\t\t\tAftertouch Bend", tone.wg.aTouchBend != 0);
	PrintProperty("\t\t\tLFO 1 Amount", tone.wg.lfo1Sens);
	PrintProperty("\t\t\tLFO 2 Amount", tone.wg.lfo2Sens);
	PrintProperty("\t\t\tLever Destination", SafeTable(LFOSelect, (tone.wg.leverSens < 0) ? 1 : 0));
	PrintProperty("\t\t\tLever LFO Amount", std::abs(tone.wg.leverSens));
	PrintProperty("\t\t\tAftertouch Destination", SafeTable(LFOSelect, (tone.wg.aTouchModSens < 0) ? 1 : 0));
	PrintProperty("\t\t\tAftertouch LFO Amount", std::abs(tone.wg.aTouchModSens));
	std::cout << "\t\tPitch Envelope" << std::endl;
	PrintProperty("\t\t\tVelo", tone.pitchEnv.velo);
	PrintProperty("\t\t\tTime Velo", tone.pitchEnv.timeVelo);
	PrintProperty("\t\t\tTime Key Follow", tone.pitchEnv.timeKF);
	PrintProperty("\t\t\tLevel 0", tone.pitchEnv.level0);
	PrintProperty("\t\t\tTime 1", tone.pitchEnv.time1);
	PrintProperty("\t\t\tLevel 1", tone.pitchEnv.level1);
	PrintProperty("\t\t\tTime 2", tone.pitchEnv.time2);
	PrintProperty("\t\t\tTime 3", tone.pitchEnv.time3);
	PrintProperty("\t\t\tLevel 2", tone.pitchEnv.level2);
	std::cout << "\t\tTVF" << std::endl;
	PrintProperty("\t\t\tFilter Mode", SafeTable(FilterMode, 2 - tone.tvf.filterMode));
	PrintProperty("\t\t\tCutoff Frequency", tone.tvf.cutoffFreq);
	PrintProperty("\t\t\tResonance", tone.tvf.resonance);
	PrintProperty("\t\t\tKey Follow", CutoffKeyFollow(tone.tvf.keyFollow));
	PrintProperty("\t\t\tAftertouch Amount", tone.tvf.aTouchSens);
	PrintProperty("\t\t\tLFO Source", SafeTable(LFOSelect, tone.tvf.lfoSelect));
	PrintProperty("\t\t\tLFO Depth", tone.tvf.lfoDepth);
	PrintProperty("\t\t\tEnvelope Depth", tone.tvf.envDepth);
	std::cout << "\t\tTVF Envelope" << std::endl;
	PrintProperty("\t\t\tVelo", tone.tvfEnv.velo);
	PrintProperty("\t\t\tTime Velo", tone.tvfEnv.timeVelo);
	PrintProperty("\t\t\tTime Key Follow", tone.tvfEnv.timeKF);
	PrintProperty("\t\t\tTime 1", tone.tvfEnv.time1);
	PrintProperty("\t\t\tLevel 1", tone.tvfEnv.level1);
	PrintProperty("\t\t\tTime 2", tone.tvfEnv.time2);
	PrintProperty("\t\t\tLevel 2", tone.tvfEnv.level2);
	PrintProperty("\t\t\tTime 3", tone.tvfEnv.time3);
	PrintProperty("\t\t\tSustain Level", tone.tvfEnv.sustainLevel);
	PrintProperty("\t\t\tTime 4", tone.tvfEnv.time4);
	PrintProperty("\t\t\tLevel 4", tone.tvfEnv.level4);
	std::cout << "\t\tTVA" << std::endl;
	PrintProperty("\t\t\tBias Direction", SafeTable(BiasDirection, tone.tva.biasDirection));
	PrintProperty("\t\t\tBias Point", KeyName(tone.tva.biasPoint));
	PrintProperty("\t\t\tBias Level", tone.tva.biasLevel);
	PrintProperty("\t\t\tLevel", tone.tva.level);
	PrintProperty("\t\t\tAftertouch Amount", tone.tva.aTouchSens);
	PrintProperty("\t\t\tLFO Source", SafeTable(LFOSelect, tone.tva.lfoSelect));
	PrintProperty("\t\t\tLFO Depth", tone.tva.lfoDepth);
	std::cout << "\t\tTVA Envelope" << std::endl;
	PrintProperty("\t\t\tVelo", tone.tvaEnv.velo);
	PrintProperty("\t\t\tTime Velo", tone.tvaEnv.timeVelo);
	PrintProperty("\t\t\tTime Key Follow", tone.tvaEnv.timeKF);
	PrintProperty("\t\t\tTime 1", tone.tvaEnv.time1);
	PrintProperty("\t\t\tLevel 1", tone.tvaEnv.level1);
	PrintProperty("\t\t\tTime 2", tone.tvaEnv.time2);
	PrintProperty("\t\t\tLevel 2", tone.tvaEnv.level2);
	PrintProperty("\t\t\tTime 3", tone.tvaEnv.time3);
	PrintProperty("\t\t\tSustain Level", tone.tvaEnv.sustainLevel);
	PrintProperty("\t\t\tTime 4", tone.tvaEnv.time4);
}

void PrintPatch(const PatchVST &patch)
{
	std::cout << "\tCommon" << std::endl;
	PrintProperty("\t\tPatch Level", patch.common.patchLevel);
	PrintProperty("\t\tBender Range Down", patch.common.benderRangeDown);
	PrintProperty("\t\tBender Range Up", patch.common.benderRangeUp);
	PrintProperty("\t\tAftertouch Bend Amount", ATouchBendSens(patch.common.aTouchBend));
	PrintProperty("\t\tSolo Switch", patch.common.soloSW != 0);
	PrintProperty("\t\tSolo Legato", patch.common.soloLegato != 0);
	PrintProperty("\t\tPortamento Switch", patch.common.portamentoSW != 0);
	PrintProperty("\t\tPortamento Mode", patch.common.portamentoMode ? "LEGATO" : "NORMAL");
	PrintProperty("\t\tPortamento Time", patch.common.portamentoTime);
	PrintProperty("\t\tUnison", patch.unison != 0);
	std::cout << "\tEQ" << std::endl;
	PrintProperty("\t\tEnabled", patch.eq.eqEnabled);
	PrintProperty("\t\tLow Frequency", patch.eq.lowFreq);
	PrintProperty("\t\tLow Gain", static_cast<int16_t>(patch.eq.lowGain) * 0.1);
	PrintProperty("\t\tMid Frequency", patch.eq.midFreq);
	PrintProperty("\t\tMid Q", patch.eq.midQ * 0.1);
	PrintProperty("\t\tMid Gain", static_cast<int16_t>(patch.eq.midGain) * 0.1);
	PrintProperty("\t\tHigh Frequency", patch.eq.highFreq);
	PrintProperty("\t\tHigh Gain", static_cast<int16_t>(patch.eq.highGain) * 0.1);
	std::cout << "\tEffects" << std::endl;
	PrintProperty("\t\tMFX Type", patch.effectsGroupA.mfxType);
	PrintProperty("\t\tGroup A Enabled", patch.effectsGroupA.groupAenabled);
	PrintProperty("\t\tGroup A Sequence", SafeTable(FXGroupASequence, static_cast<uint8_t>(patch.effectsGroupA.groupAsequence)));
	PrintProperty("\t\tGroup A Panning", patch.effectsGroupA.panningGroupA);
	PrintProperty("\t\tGroup A Level", patch.effectsGroupA.effectsLevelGroupA);
	PrintProperty("\t\tGroup B Sequence", SafeTable(FXGroupBSequence, patch.effectsGroupB.groupBsequence));
	PrintProperty("\t\tGroup B Effects Balance", patch.effectsGroupB.effectsBalanceGroupB);
	PrintProperty("\t\tGroup B Effects Level", patch.effectsGroupB.effectsLevelGroupB);
	PrintProperty("\t\tDistortion Enabled", patch.effectsGroupA.distortionEnabled);
	PrintProperty("\t\tDistortion Type", SafeTable(DistortionType, static_cast<uint8_t>(patch.effectsGroupA.distortionType)));
	PrintProperty("\t\tDistortion Drive", patch.effectsGroupA.distortionDrive);
	PrintProperty("\t\tDistortion Level", patch.effectsGroupA.distortionLevel);
	PrintProperty("\t\tPhaser Enabled", patch.effectsGroupA.phaserEnabled);
	PrintProperty("\t\tPhaser Manual", PhaserManual(patch.effectsGroupA.phaserManual));
	PrintProperty("\t\tPhaser Rate (Hz)", patch.effectsGroupA.phaserRate * 0.1);
	PrintProperty("\t\tPhaser Depth", patch.effectsGroupA.phaserDepth);
	PrintProperty("\t\tPhaser Resonance", patch.effectsGroupA.phaserResonance);
	PrintProperty("\t\tPhaser Mix", patch.effectsGroupA.phaserMix);
	PrintProperty("\t\tSpectrum Enabled", patch.effectsGroupA.spectrumEnabled);
	PrintProperty("\t\tSpectrum Band 1", patch.effectsGroupA.spectrumBand1);
	PrintProperty("\t\tSpectrum Band 2", patch.effectsGroupA.spectrumBand2);
	PrintProperty("\t\tSpectrum Band 3", patch.effectsGroupA.spectrumBand3);
	PrintProperty("\t\tSpectrum Band 4", patch.effectsGroupA.spectrumBand4);
	PrintProperty("\t\tSpectrum Band 5", patch.effectsGroupA.spectrumBand5);
	PrintProperty("\t\tSpectrum Band 6", patch.effectsGroupA.spectrumBand6);
	PrintProperty("\t\tSpectrum Bandwidth", patch.effectsGroupA.spectrumBandwidth);
	PrintProperty("\t\tEnhancer Enabled", patch.effectsGroupA.enhancerEnabled);
	PrintProperty("\t\tEnhancer Sensitivity", patch.effectsGroupA.enhancerSens);
	PrintProperty("\t\tEnhancer Mix", patch.effectsGroupA.enhancerMix);
	PrintProperty("\t\tDelay Enabled", patch.effectsGroupB.delayEnabled);
	PrintProperty("\t\tDelay Center Tempo Sync", patch.effectsGroupB.delayCenterTempoSync != 0);
	if (patch.effectsGroupB.delayCenterTempoSync)
		PrintProperty("\t\tDelay Center Tap", SafeTable(TempoSyncVST, patch.effectsGroupB.delayCenterTapWithSync));
	else
		PrintProperty("\t\tDelay Center Tap (ms)", DelayTime(patch.effectsGroupB.delayCenterTap));
	PrintProperty("\t\tDelay Center Level", patch.effectsGroupB.delayCenterLevel);
	PrintProperty("\t\tDelay Left Tempo Sync", patch.effectsGroupB.delayLeftTempoSync != 0);
	if (patch.effectsGroupB.delayLeftTempoSync)
		PrintProperty("\t\tDelay Left Tap", SafeTable(TempoSyncVST, patch.effectsGroupB.delayLeftTapWithSync));
	else
		PrintProperty("\t\tDelay Left Tap (ms)", DelayTime(patch.effectsGroupB.delayLeftTap));
	PrintProperty("\t\tDelay Left Level", patch.effectsGroupB.delayLeftLevel);
	PrintProperty("\t\tDelay Right Tempo Sync", patch.effectsGroupB.delayRightTempoSync != 0);
	if (patch.effectsGroupB.delayRightTempoSync)
		PrintProperty("\t\tDelay Right Tap", SafeTable(TempoSyncVST, patch.effectsGroupB.delayRightTapWithSync));
	else
		PrintProperty("\t\tDelay Right Tap (ms)", DelayTime(patch.effectsGroupB.delayRightTap));
	PrintProperty("\t\tDelay Right Level", patch.effectsGroupB.delayRightLevel);
	PrintProperty("\t\tDelay Feedback", patch.effectsGroupB.delayFeedback);
	PrintProperty("\t\tChorus Enabled", patch.effectsGroupB.chorusEnabled);
	PrintProperty("\t\tChorus Rate (Hz)", 0.1 + patch.effectsGroupB.chorusRate * 0.1);
	PrintProperty("\t\tChorus Depth", patch.effectsGroupB.chorusDepth);
	PrintProperty("\t\tChorus Delay Time (ms)", ChorusTime(patch.effectsGroupB.chorusDelayTime));
	PrintProperty("\t\tChorus Feedback", -98 + patch.effectsGroupB.chorusFeedback * 2);
	PrintProperty("\t\tChorus Level", patch.effectsGroupB.chorusLevel);
	PrintProperty("\t\tReverb Enabled", patch.effectsGroupB.reverbEnabled);
	PrintProperty("\t\tReverb Type", SafeTable(ReverbType, patch.effectsGroupB.reverbType));
	PrintProperty("\t\tReverb Pre-Delay", patch.effectsGroupB.reverbPreDelay);
	PrintProperty("\t\tReverb Early Reflections Level", patch.effectsGroupB.reverbEarlyRefLevel);
	PrintProperty("\t\tReverb HF Damp", SafeTable(ReverbHFDamp, patch.effectsGroupB.reverbHFDamp));
	PrintProperty("\t\tReverb Time (ms)", ReverbTime(patch.effectsGroupB.reverbTime, patch.effectsGroupB.reverbType));
	PrintProperty("\t\tReverb Level", patch.effectsGroupB.reverbLevel);
	std::cout << "\tTone A" << std::endl;
	PrintTone(patch.tone[0], patch.common.keyRangeLowA, patch.common.keyRangeHighA);
	std::cout << "\tTone B" << std::endl;
	PrintTone(patch.tone[1], patch.common.keyRangeLowB, patch.common.keyRangeHighB);
	std::cout << "\tTone C" << std::endl;
	PrintTone(patch.tone[2], patch.common.keyRangeLowC, patch.common.keyRangeHighC);
	std::cout << "\tTone D" << std::endl;
	PrintTone(patch.tone[3], patch.common.keyRangeLowD, patch.common.keyRangeHighD);
}
