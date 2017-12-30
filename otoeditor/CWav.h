#ifndef _CWAV_H
#define _CWAV_H
///////////////////////////////////////////////////////////////////////////////////
// CWav : WAV管理クラス v1.01                                                    //
//                                                                               //
// このソースコードは自由に改変して使用可能です。                                //
// また商用利用も可能ですが、すべての環境で正しく動作する保障はありません。      //
//                          http://www.charatsoft.com/                           //
///////////////////////////////////////////////////////////////////////////////////
#include <WinSock2.h>
#include <Windows.h>
#include <stdio.h>

// 読み込みモード
#define WAVMODE_STATIC		0x00000000			// メモリ上にすべてのWAVをロードする(スタティック)
#define WAVMODE_STREAM		0x00000001			// ファイルから任意のサイズを読み取って使用する(ストリーム)

// WAVヘッダ
typedef struct _WAVHEADER {
	char	mRiff[4];				// 'RIFF'
	DWORD	dwFileSize;				// ファイルサイズ
	char	mWave[4];				// 'WAVE'
	char	mFmt[4];				// 'fmt '
	DWORD	dwChuncSize;			// チャンクサイズ(PCMなら16)
	WORD	wFormatID;				// フォーマットＩＤ(PCMなら1)
	WORD	wChannel;				// チャンネル数
	DWORD	dwSample;				// サンプリングレート
	DWORD	dwBytePerSec;			// 秒間のバイト数(wChannel*dwSample*wBitrate )
	WORD	wBlockSize;				// １つのWAVデータサイズ(wChannel*wBitrate)
	WORD	wBitrate;				// サンプリングビット値(8bit/16bit)
} WAVHEADER,*LPWAVHEADER;

// チャンク構造体
typedef struct _DATACHUNC {
	char	mData[4];				// 'data'
	DWORD	dwSize;					// データサイズ
} DATACHUNC,*LPDATACHUNC;



class CWav {
	DWORD		dwMode;				// ストリームか
	WAVHEADER	mHead;				// WAVヘッダ
	DATACHUNC	mData;				// DATAチャンク
	LPBYTE		mBuf;				// データ部
	int			iBuf;				// データ数
	int			iStreamPoint;		// ストリーム時のデータポインタ
	char		mFile[MAX_PATH];	// ファイル名
	FILE		*fp;
public:
	CWav();
	virtual ~CWav();
	BOOL Create( const char *file,int ch=2,int bit=16,int samp=44100 );		// WAVデータの新規定義
	BOOL Write( const LPVOID buf,int size );								// ファイルへの書き込み
	BOOL Close( void );														// ファイルを閉じる
	BOOL Load( const char *file,DWORD mode=WAVMODE_STATIC );				// 読み込みモードを指定してファイルをオープン
	BOOL Read( char *buf,int *size );										// ストリームモードでのロード
public:
	inline const LPBYTE GetBuffer( void ) { return mBuf; }
	inline int GetBufferSize( void ) { return iBuf; }
	inline int GetChannel( void ) { return (int)mHead.wChannel; }
	inline int GetSampleRate( void ) { return (int)mHead.dwSample; }
	inline int GetBitrate( void ) { return (int)mHead.wBitrate; }
};

#endif
