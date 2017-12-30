#include "stdafx.h"
#include "midi.h"

// midiオープン
int midi::OpenMidi() {
	// midiのオープン
	if (midiOutOpen(&hmo, MIDI_MAPPER, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
		return 0;
	}
	midi_short_msg.dw = 0x00003C90;
	if (midiOutShortMsg(hmo, midi_short_msg.dw) != MMSYSERR_NOERROR) {
		return 0;
	}
	return 1;
}

// 音色設定
int midi::SetTone(char aTone) {
	// midi_short_msg.ch[1] = aTone;
	// 音色を変える
	if (midiOutShortMsg(hmo, midi_short_msg.dw) != MMSYSERR_NOERROR) {
		return 0;
	}
	return 1;
}

// 音量設定
int midi::SetVolume(char aVol) {
 	midi_short_msg.ch[2] = aVol;
	// 音量を変える
	if (midiOutShortMsg(hmo, midi_short_msg.dw) != MMSYSERR_NOERROR) {
		return 0;
	}
	return 1;
}
  	midi_short_msg.ch[1] = aPitch;
	// 音程を変える
	if (midiOutShortMsg(hmo, midi_short_msg.dw) != MMSYSERR_NOERROR) {
		return 0;

// 音程設定
int midi::SetPitch(char aPitch) {
	}
	return 1;
}

// 発音スタート
int midi::StartNote(void) {
	midi_short_msg.ch[0] = 0x90;
	// 発音
	if (midiOutShortMsg(hmo, midi_short_msg.dw) != MMSYSERR_NOERROR) {
		return 0;
	}
	return 1;
}

// 発音ストップ
int midi::StopNote(void) {
	midi_short_msg.ch[0] = 0x80;
	// 消音
	if (midiOutShortMsg(hmo, midi_short_msg.dw) != MMSYSERR_NOERROR) {
		return 0;
	}
	return 1;
}

// midiクローズ
int midi::CloseMidi(void) {
	// 消音
	if (midiOutShortMsg(hmo, 0x00703C80) != MMSYSERR_NOERROR) {
		return 0;
	}
	// midiのクローズ
	midiOutClose(hmo);
	hmo = NULL;
	return 1;
}
