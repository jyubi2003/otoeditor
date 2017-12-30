#pragma once
///////////////////////////////////////////////////////////////////////////////////
// CWasapi : WASAPI管理クラス v1.00                                                    //
//                                                                               //
///////////////////////////////////////////////////////////////////////////////////
#include <Windows.h>

class CWasapi {

private:
	// クラス変数
	CWindow					win;							// ウィンドウ
	CWav					wav;							// WAVファイル
	int						iAddr = 0;						// WAVファイルの再生位置

	IMMDeviceEnumerator		*pDeviceEnumerator = NULL;		// マルチメディアデバイス列挙インターフェース
	IMMDevice				*pDevice = NULL;				// デバイスインターフェース
	IAudioClient			*pAudioClient = NULL;			// オーディオクライアントインターフェース
	IAudioRenderClient		*pRenderClient = NULL;			// レンダークライアントインターフェース
	int						iFrameSize = 0;					// 1サンプル分のバッファサイズ

	HANDLE					hEvent = NULL;					// イベントハンドル
	HANDLE					hThread = NULL;					// スレッドハンドル
	BOOL					bThreadLoop = FALSE;			// スレッド処理中か

public:

	// 再生スレッド
	static DWORD WINAPI PlayThread(LPVOID param);

	// WASAPIの初期化
	BOOL InitWasapi(int latency);

	// WASAPIの終了
	void ExitWasapi();



};