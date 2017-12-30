#include "stdafx.h"
#include "midi.h"

// midi�I�[�v��
int midi::OpenMidi() {
	// midi�̃I�[�v��
	if (midiOutOpen(&hmo, MIDI_MAPPER, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
		return 0;
	}
	midi_short_msg.dw = 0x00003C90;
	if (midiOutShortMsg(hmo, midi_short_msg.dw) != MMSYSERR_NOERROR) {
		return 0;
	}
	return 1;
}

// ���F�ݒ�
int midi::SetTone(char aTone) {
	// midi_short_msg.ch[1] = aTone;
	// ���F��ς���
	if (midiOutShortMsg(hmo, midi_short_msg.dw) != MMSYSERR_NOERROR) {
		return 0;
	}
	return 1;
}

// ���ʐݒ�
int midi::SetVolume(char aVol) {
 	midi_short_msg.ch[2] = aVol;
	// ���ʂ�ς���
	if (midiOutShortMsg(hmo, midi_short_msg.dw) != MMSYSERR_NOERROR) {
		return 0;
	}
	return 1;
}
  	midi_short_msg.ch[1] = aPitch;
	// ������ς���
	if (midiOutShortMsg(hmo, midi_short_msg.dw) != MMSYSERR_NOERROR) {
		return 0;

// �����ݒ�
int midi::SetPitch(char aPitch) {
	}
	return 1;
}

// �����X�^�[�g
int midi::StartNote(void) {
	midi_short_msg.ch[0] = 0x90;
	// ����
	if (midiOutShortMsg(hmo, midi_short_msg.dw) != MMSYSERR_NOERROR) {
		return 0;
	}
	return 1;
}

// �����X�g�b�v
int midi::StopNote(void) {
	midi_short_msg.ch[0] = 0x80;
	// ����
	if (midiOutShortMsg(hmo, midi_short_msg.dw) != MMSYSERR_NOERROR) {
		return 0;
	}
	return 1;
}

// midi�N���[�Y
int midi::CloseMidi(void) {
	// ����
	if (midiOutShortMsg(hmo, 0x00703C80) != MMSYSERR_NOERROR) {
		return 0;
	}
	// midi�̃N���[�Y
	midiOutClose(hmo);
	hmo = NULL;
	return 1;
}
