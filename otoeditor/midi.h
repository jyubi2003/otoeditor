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
	// �R���X�g���N�^
	midi() {
		hmo = NULL;
		midi_short_msg.dw = 0;
	};

	// �f�X�g���N�^
	~midi() {
		if (hmo != NULL) {
			midiOutClose(hmo);
		}
	};

	// midi�I�[�v��
	int OpenMidi();

	// ���F�ݒ�
	int SetTone(char tone);

	// ���ʐݒ�
	int SetVolume(char vol);

	// �����ݒ�
	int SetPitch(char pitch);

	// �����X�^�[�g
	int StartNote();

	// �����X�g�b�v
	int StopNote();

	// midi�N���[�Y
	int CloseMidi();

};