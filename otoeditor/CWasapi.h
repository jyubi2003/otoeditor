#pragma once
///////////////////////////////////////////////////////////////////////////////////
// CWasapi : WASAPI�Ǘ��N���X v1.00                                                    //
//                                                                               //
///////////////////////////////////////////////////////////////////////////////////
#include <Windows.h>

class CWasapi {

private:
	// �N���X�ϐ�
	CWindow					win;							// �E�B���h�E
	CWav					wav;							// WAV�t�@�C��
	int						iAddr = 0;						// WAV�t�@�C���̍Đ��ʒu

	IMMDeviceEnumerator		*pDeviceEnumerator = NULL;		// �}���`���f�B�A�f�o�C�X�񋓃C���^�[�t�F�[�X
	IMMDevice				*pDevice = NULL;				// �f�o�C�X�C���^�[�t�F�[�X
	IAudioClient			*pAudioClient = NULL;			// �I�[�f�B�I�N���C�A���g�C���^�[�t�F�[�X
	IAudioRenderClient		*pRenderClient = NULL;			// �����_�[�N���C�A���g�C���^�[�t�F�[�X
	int						iFrameSize = 0;					// 1�T���v�����̃o�b�t�@�T�C�Y

	HANDLE					hEvent = NULL;					// �C�x���g�n���h��
	HANDLE					hThread = NULL;					// �X���b�h�n���h��
	BOOL					bThreadLoop = FALSE;			// �X���b�h��������

public:

	// �Đ��X���b�h
	static DWORD WINAPI PlayThread(LPVOID param);

	// WASAPI�̏�����
	BOOL InitWasapi(int latency);

	// WASAPI�̏I��
	void ExitWasapi();



};