#pragma once
#include <windows.h>
#include <mmsystem.h>

#pragma comment (lib, "winmm.lib")

typedef union tag_MidiShortMsg {
	char	ch[4];
	DWORD	dw;
} MIDI_SHORT_MSG, *PTR_MIDI_SHORT_MSG;

class midi {
private:
	HMIDIOUT hmo;
	MIDI_SHORT_MSG midi_short_msg;

public:
	// コンストラクタ
	midi() {
		hmo = NULL;
		midi_short_msg.dw = 0;
	};

	// デストラクタ
	~midi() {
		if (hmo != NULL) {
			midiOutClose(hmo);
		}
	};

	// midiオープン
	int OpenMidi();

	// 音色設定
	int SetTone(char tone);

	// 音量設定
	int SetVolume(char vol);

	// 音程設定
	int SetPitch(char pitch);

	// 発音スタート
	int StartNote();

	// 発音ストップ
	int StopNote();

	// midiクローズ
	int CloseMidi();

};