#ifndef _CWAV_H
#define _CWAV_H
///////////////////////////////////////////////////////////////////////////////////
// CWav : WAV�Ǘ��N���X v1.01                                                    //
//                                                                               //
// ���̃\�[�X�R�[�h�͎��R�ɉ��ς��Ďg�p�\�ł��B                                //
// �܂����p���p���\�ł����A���ׂĂ̊��Ő��������삷��ۏ�͂���܂���B      //
//                          http://www.charatsoft.com/                           //
///////////////////////////////////////////////////////////////////////////////////
#include <WinSock2.h>
#include <Windows.h>
#include <stdio.h>

// �ǂݍ��݃��[�h
#define WAVMODE_STATIC		0x00000000			// ��������ɂ��ׂĂ�WAV�����[�h����(�X�^�e�B�b�N)
#define WAVMODE_STREAM		0x00000001			// �t�@�C������C�ӂ̃T�C�Y��ǂݎ���Ďg�p����(�X�g���[��)

// WAV�w�b�_
typedef struct _WAVHEADER {
	char	mRiff[4];				// 'RIFF'
	DWORD	dwFileSize;				// �t�@�C���T�C�Y
	char	mWave[4];				// 'WAVE'
	char	mFmt[4];				// 'fmt '
	DWORD	dwChuncSize;			// �`�����N�T�C�Y(PCM�Ȃ�16)
	WORD	wFormatID;				// �t�H�[�}�b�g�h�c(PCM�Ȃ�1)
	WORD	wChannel;				// �`�����l����
	DWORD	dwSample;				// �T���v�����O���[�g
	DWORD	dwBytePerSec;			// �b�Ԃ̃o�C�g��(wChannel*dwSample*wBitrate )
	WORD	wBlockSize;				// �P��WAV�f�[�^�T�C�Y(wChannel*wBitrate)
	WORD	wBitrate;				// �T���v�����O�r�b�g�l(8bit/16bit)
} WAVHEADER,*LPWAVHEADER;

// �`�����N�\����
typedef struct _DATACHUNC {
	char	mData[4];				// 'data'
	DWORD	dwSize;					// �f�[�^�T�C�Y
} DATACHUNC,*LPDATACHUNC;



class CWav {
	DWORD		dwMode;				// �X�g���[����
	WAVHEADER	mHead;				// WAV�w�b�_
	DATACHUNC	mData;				// DATA�`�����N
	LPBYTE		mBuf;				// �f�[�^��
	int			iBuf;				// �f�[�^��
	int			iStreamPoint;		// �X�g���[�����̃f�[�^�|�C���^
	char		mFile[MAX_PATH];	// �t�@�C����
	FILE		*fp;
public:
	CWav();
	virtual ~CWav();
	BOOL Create( const char *file,int ch=2,int bit=16,int samp=44100 );		// WAV�f�[�^�̐V�K��`
	BOOL Write( const LPVOID buf,int size );								// �t�@�C���ւ̏�������
	BOOL Close( void );														// �t�@�C�������
	BOOL Load( const char *file,DWORD mode=WAVMODE_STATIC );				// �ǂݍ��݃��[�h���w�肵�ăt�@�C�����I�[�v��
	BOOL Read( char *buf,int *size );										// �X�g���[�����[�h�ł̃��[�h
public:
	inline const LPBYTE GetBuffer( void ) { return mBuf; }
	inline int GetBufferSize( void ) { return iBuf; }
	inline int GetChannel( void ) { return (int)mHead.wChannel; }
	inline int GetSampleRate( void ) { return (int)mHead.dwSample; }
	inline int GetBitrate( void ) { return (int)mHead.wBitrate; }
};

#endif
