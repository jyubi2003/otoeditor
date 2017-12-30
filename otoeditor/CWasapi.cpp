///////////////////////////////////////////////////////////////////////////////////
// Main : WASAPI再生サンプル v1.00                                               //
//                                                                               //
// このソースコードは自由に改変して使用可能です。                                //
// また商用利用も可能ですが、すべての環境で正しく動作する保障はありません。      //
//                          http://www.charatsoft.com/                           //
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "CWasapi.h"
#include "CWindow.h"
#include "CWav.h"

#define DEBUGMODE				// デバッグ出力しない場合はコメント化する
#include "DEBUG.H"

// マクロ
#define SAFE_RELEASE(x)		{ if( x ) { x->Release(); x=NULL; } }
#define SAFE_FREE(x)		{ if( x ) { free(x); x=NULL; } }


//////////////////////////////////////////////////////////////////////////////////////
// WASAPI関連
//////////////////////////////////////////////////////////////////////////////////////

// ヘッダ
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <FunctionDiscoveryKeys_devpkey.h>

// ライブラリ
#pragma comment(lib, "Avrt.lib")
#pragma comment(lib, "winmm.lib")

// インターフェースのGUIDの実体(プロジェクト内のCファイルに必ず1つ必要)
const CLSID CLSID_MMDeviceEnumerator	= __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator		= __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient				= __uuidof(IAudioClient);
const IID IID_IAudioClock				= __uuidof(IAudioClock);
const IID IID_IAudioRenderClient		= __uuidof(IAudioRenderClient);

//////////////////////////////////////////////////////////////////////////////////////
// 再生スレッド
//////////////////////////////////////////////////////////////////////////////////////
DWORD CALLBACK 
CWasapi::PlayThread( LPVOID param )
{
	DEBUG( "スレッド開始\n" );

	while( bThreadLoop ) {
		// 次のバッファ取得が必要になるまで待機
		DWORD retval = WaitForSingleObject( hEvent,2000 );
		if( retval!=WAIT_OBJECT_0 ) {
			DEBUG( "タイムアウト\n" );
			pAudioClient->Stop();
			break;
		}

		// 今回必要なフレーム数を取得
		UINT32 frame_count;
		HRESULT ret = pAudioClient->GetBufferSize( &frame_count );
//		ODS( "フレーム数    : %d\n",frame_count );

		// フレーム数からバッファサイズを算出
		int buf_size = frame_count * iFrameSize;
//		ODS( "バッファサイズ : %dbyte\n",buf_size );


		// 出力バッファのポインタを取得
		LPBYTE dst;
		ret = pRenderClient->GetBuffer( frame_count,&dst );
		if( SUCCEEDED(ret) ) {
			// ソースバッファのポインタを取得
			LPBYTE src = wav.GetBuffer();

			// 現在のカーソルから次に必要なバッファサイズを加算したときにWAVバッファのサイズを超えるか
			if( iAddr+buf_size>wav.GetBufferSize() ) {
				////////////////////////////////////////////////////////////////////////////////////////
				// 超える場合はまず現在のカーソルから最後までをコピーし、次に残った分を先頭から補充する
				////////////////////////////////////////////////////////////////////////////////////////

				// WAVバッファサイズから現在のカーソル位置を差し引いたものが最後までのバッファサイズ
				int last_size = wav.GetBufferSize() - iAddr;

				// 現在のカーソルから最後までのバッファを出力バッファの先頭にコピーする
				memcpy( &dst[0],&src[iAddr],last_size );

				// 今回必要なサイズから今コピーしたサイズを引いたものが先頭から補充するバッファサイズ
				int begin_size = buf_size - last_size;

				// WAVバッファの先頭から補充サイズ分を出力バッファに追加コピーする
				memcpy( &dst[last_size],&src[0],begin_size );

				// 補充した分のサイズを現在のカーソルとする
				iAddr = begin_size;

				ODS( "LOOP OK\n" );
			} else {
				////////////////////////////////////////////////////////////////////////////////////////
				// 超えない場合は現在のカーソルから必要なバッファ分をコピーしてカーソルを進める
				////////////////////////////////////////////////////////////////////////////////////////

				// WAVバッファの現在のカーソルから出力バッファに指定サイズ分コピーする
				memcpy( &dst[0],&src[iAddr],buf_size );

				// カーソルをコピーした分だけ進める
				iAddr += buf_size;
			}

			// バッファを開放
			pRenderClient->ReleaseBuffer( frame_count,0 );
		}

	}

	DEBUG( "スレッド終了\n" );
	return 0;
}


//////////////////////////////////////////////////////////////////////////////////////
// WASAPIの初期化
//////////////////////////////////////////////////////////////////////////////////////
BOOL 
CWasapi::InitWasapi( int latency )
{
	HRESULT ret;

	// マルチメディアデバイス列挙子
	ret = CoCreateInstance( CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pDeviceEnumerator );
	if( FAILED(ret) ) {
		DEBUG( "CLSID_MMDeviceEnumeratorエラー\n" );
		return FALSE;
	}

	// デフォルトのデバイスを選択
	ret = pDeviceEnumerator->GetDefaultAudioEndpoint( eRender,eConsole,&pDevice );
	if( FAILED(ret) ) {
		DEBUG( "GetDefaultAudioEndpointエラー\n" );
		return FALSE;
	}

	// オーディオクライアント
	ret = pDevice->Activate( IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient );
	if( FAILED(ret) ) {
		DEBUG( "オーディオクライアント取得失敗\n" );
		return FALSE;
	}

	// フォーマットの構築
	WAVEFORMATEXTENSIBLE wf;
	ZeroMemory( &wf,sizeof(wf) );
	wf.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE);
	wf.Format.wFormatTag			= WAVE_FORMAT_EXTENSIBLE;
	wf.Format.nChannels				= 2;
	wf.Format.nSamplesPerSec		= 44100;
	wf.Format.wBitsPerSample		= 16;
	wf.Format.nBlockAlign			= wf.Format.nChannels * wf.Format.wBitsPerSample / 8;
	wf.Format.nAvgBytesPerSec		= wf.Format.nSamplesPerSec * wf.Format.nBlockAlign;
	wf.Samples.wValidBitsPerSample	= wf.Format.wBitsPerSample;
	wf.dwChannelMask				= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
	wf.SubFormat					= KSDATAFORMAT_SUBTYPE_PCM;

	// 1サンプルのサイズを保存(16bitステレオなら4byte)
	iFrameSize = wf.Format.nBlockAlign;

	// フォーマットのサポートチェック
	ret = pAudioClient->IsFormatSupported( AUDCLNT_SHAREMODE_EXCLUSIVE,(WAVEFORMATEX*)&wf,NULL );
	if( FAILED(ret) ) {
		DEBUG( "未サポートのフォーマット\n" );
		return FALSE;
	}

	// レイテンシ設定
	REFERENCE_TIME default_device_period = 0;
	REFERENCE_TIME minimum_device_period = 0;

	if( latency!=0 ) {
		default_device_period = (REFERENCE_TIME)latency * 10000LL;		// デフォルトデバイスピリオドとしてセット
		DEBUG( "レイテンシ指定             : %I64d (%fミリ秒)\n",default_device_period,default_device_period/10000.0 );
	} else {
		ret = pAudioClient->GetDevicePeriod( &default_device_period,&minimum_device_period );
		DEBUG( "デフォルトデバイスピリオド : %I64d (%fミリ秒)\n",default_device_period,default_device_period/10000.0 );
		DEBUG( "最小デバイスピリオド       : %I64d (%fミリ秒)\n",minimum_device_period,minimum_device_period/10000.0 );
	}

	// 初期化
	UINT32 frame = 0;
	ret = pAudioClient->Initialize( AUDCLNT_SHAREMODE_EXCLUSIVE,
									AUDCLNT_STREAMFLAGS_NOPERSIST | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
									default_device_period,				// デフォルトデバイスピリオド値をセット
									default_device_period,				// デフォルトデバイスピリオド値をセット
									(WAVEFORMATEX*)&wf,
									NULL ); 
	if( FAILED(ret) ) {
		if( ret==AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED ) {
			DEBUG( "バッファサイズアライメントエラーのため修正する\n" );

			// 修正後のフレーム数を取得
			ret = pAudioClient->GetBufferSize( &frame );
			DEBUG( "修正後のフレーム数         : %d\n",frame );
			default_device_period = (REFERENCE_TIME)( 10000.0 *						// (REFERENCE_TIME(100ns) / ms) *
													  1000 *						// (ms / s) *
													  frame /						// frames /
													  wf.Format.nSamplesPerSec +	// (frames / s)
													  0.5);							// 四捨五入？
			DEBUG( "修正後のレイテンシ         : %I64d (%fミリ秒)\n",default_device_period,default_device_period/10000.0  );

			// 一度破棄してオーディオクライアントを再生成
			SAFE_RELEASE( pAudioClient );
			ret = pDevice->Activate( IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient );
			if( FAILED(ret) ) {
				DEBUG( "オーディオクライアント再取得失敗\n" );
				return FALSE;
			}

			// 再挑戦
			ret = pAudioClient->Initialize( AUDCLNT_SHAREMODE_EXCLUSIVE,
											AUDCLNT_STREAMFLAGS_NOPERSIST | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
											default_device_period,
											default_device_period,
											(WAVEFORMATEX*)&wf,
											NULL );
		}

		if( FAILED(ret) ) {
			DEBUG( "初期化失敗 : 0x%08X\n",ret );
			switch( ret )
			{
			case AUDCLNT_E_NOT_INITIALIZED:					DEBUG( "AUDCLNT_E_NOT_INITIALIZED\n" );					break;
			case AUDCLNT_E_ALREADY_INITIALIZED:				DEBUG( "AUDCLNT_E_ALREADY_INITIALIZED\n" );				break;
			case AUDCLNT_E_WRONG_ENDPOINT_TYPE:				DEBUG( "AUDCLNT_E_WRONG_ENDPOINT_TYPE\n" );				break;
			case AUDCLNT_E_DEVICE_INVALIDATED:				DEBUG( "AUDCLNT_E_DEVICE_INVALIDATED\n" );				break;
			case AUDCLNT_E_NOT_STOPPED:						DEBUG( "AUDCLNT_E_NOT_STOPPED\n" );						break;
			case AUDCLNT_E_BUFFER_TOO_LARGE:				DEBUG( "AUDCLNT_E_BUFFER_TOO_LARGE\n" );				break;
			case AUDCLNT_E_OUT_OF_ORDER:					DEBUG( "AUDCLNT_E_OUT_OF_ORDER\n" );					break;
			case AUDCLNT_E_UNSUPPORTED_FORMAT:				DEBUG( "AUDCLNT_E_UNSUPPORTED_FORMAT\n" );				break;
			case AUDCLNT_E_INVALID_SIZE:					DEBUG( "AUDCLNT_E_INVALID_SIZE\n" );					break;
			case AUDCLNT_E_DEVICE_IN_USE:					DEBUG( "AUDCLNT_E_DEVICE_IN_USE\n" );					break;
			case AUDCLNT_E_BUFFER_OPERATION_PENDING:		DEBUG( "AUDCLNT_E_BUFFER_OPERATION_PENDING\n" );		break;
			case AUDCLNT_E_THREAD_NOT_REGISTERED:			DEBUG( "AUDCLNT_E_THREAD_NOT_REGISTERED\n" );			break;
//			case AUDCLNT_E_NO_SINGLE_PROCESS:				DEBUG( "AUDCLNT_E_NO_SINGLE_PROCESS\n" );				break;	// VC2010では未定義？
			case AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED:		DEBUG( "AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED\n" );		break;
			case AUDCLNT_E_ENDPOINT_CREATE_FAILED:			DEBUG( "AUDCLNT_E_ENDPOINT_CREATE_FAILED\n" );			break;
			case AUDCLNT_E_SERVICE_NOT_RUNNING:				DEBUG( "AUDCLNT_E_SERVICE_NOT_RUNNING\n" );				break;
			case AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED:		DEBUG( "AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED\n" );		break;
			case AUDCLNT_E_EXCLUSIVE_MODE_ONLY:				DEBUG( "AUDCLNT_E_EXCLUSIVE_MODE_ONLY\n" );				break;
			case AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL:	DEBUG( "AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL\n" );	break;
			case AUDCLNT_E_EVENTHANDLE_NOT_SET:				DEBUG( "AUDCLNT_E_EVENTHANDLE_NOT_SET\n" );				break;
			case AUDCLNT_E_INCORRECT_BUFFER_SIZE:			DEBUG( "AUDCLNT_E_INCORRECT_BUFFER_SIZE\n" );			break;
			case AUDCLNT_E_BUFFER_SIZE_ERROR:				DEBUG( "AUDCLNT_E_BUFFER_SIZE_ERROR\n" );				break;
			case AUDCLNT_E_CPUUSAGE_EXCEEDED:				DEBUG( "AUDCLNT_E_CPUUSAGE_EXCEEDED\n" );				break;
			default:										DEBUG( "UNKNOWN\n" );									break;
			}
			return FALSE;
		}
	}

	// イベント生成
	hEvent = CreateEvent( NULL,FALSE,FALSE,NULL );
	if( !hEvent ) {
		DEBUG( "イベントオブジェクト作成失敗\n" );
		return FALSE;
	}

	// イベントのセット
	ret = pAudioClient->SetEventHandle( hEvent );
	if( FAILED(ret) ) {
		DEBUG( "イベントオブジェクト設定失敗\n" );
		return FALSE;
	}

	// レンダラーの取得
	ret = pAudioClient->GetService( IID_IAudioRenderClient,(void**)&pRenderClient );
	if( FAILED(ret) ) {
		DEBUG( "レンダラー取得エラー\n" );
		return FALSE;
	}

	// WASAPI情報取得
	ret = pAudioClient->GetBufferSize( &frame );
	DEBUG( "設定されたフレーム数       : %d\n",frame );

	UINT32 size = frame * iFrameSize;
	DEBUG( "設定されたバッファサイズ   : %dbyte\n",size );
	DEBUG( "1サンプルの時間            : %f秒\n",(float)size/wf.Format.nSamplesPerSec );

	// ゼロクリアをしてイベントをリセット
	LPBYTE pData;
	ret = pRenderClient->GetBuffer( frame,&pData );
	if( SUCCEEDED(ret) ) {
		ZeroMemory( pData,size );
		pRenderClient->ReleaseBuffer( frame,0 );
	}

	// スレッドループフラグを立てる
	bThreadLoop = TRUE;

	// 再生スレッド起動
	DWORD dwThread;
	hThread = CreateThread( NULL,0, CWasapi::PlayThread, NULL,0,&dwThread );
	if( !hThread ) {
		// 失敗
		return FALSE;
	}

	// 再生開始
	pAudioClient->Start();

	DEBUG( "WASAPI初期化完了\n" );
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
// WASAPIの終了
//////////////////////////////////////////////////////////////////////////////////////
void 
CWasapi::ExitWasapi()
{
	// スレッドループフラグを落とす
	bThreadLoop = FALSE;

	// スレッド終了処理
	if( hThread ) {
		// スレッドが完全に終了するまで待機
		WaitForSingleObject( hThread,INFINITE );
		CloseHandle( hThread );
		hThread = NULL;
	}

	// イベントを開放処理
	if( hEvent ) {
		CloseHandle( hEvent );
		hEvent = NULL;
	}

	// インターフェース開放
	SAFE_RELEASE( pRenderClient );
	SAFE_RELEASE( pAudioClient );
	SAFE_RELEASE( pDevice );
	SAFE_RELEASE( pDeviceEnumerator );

	DEBUG( "WASAPI終了\n" );
}

#if(0)
//////////////////////////////////////////////////////////////////////////////////////
// メインルーチン
//////////////////////////////////////////////////////////////////////////////////////
int WINAPI WinMain( HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpszCmdParam,int nCmdShow )
{
	// デバッグ定義
	INITDEBUG( "DEBUG.TXT" );
	CLEARDEBUG;

	// ウィンドウ生成
	if( !win.Create(hInstance,L"TestWasapi") ) {
		DEBUG( "ウィンドウ生成エラー\n" );
		return -1;
	}

	// 日本語入力の無効化
	ImmAssociateContext( win.hWnd, NULL );

	// WAVをロード
	if( !wav.Load("loop.wav") ) {
		win.Delete();
		return -1;
	}

	// COMの初期化
	CoInitialize( NULL );

	// WASAPI初期化
	if( !InitWasapi(0) ) {			// 0ならデフォルトデバイスピリオドを使用
		// エラーなら終了する
		MessageBoxA( win.hWnd,"WASAPI初期化失敗","エラー",MB_OK|MB_ICONHAND );
		ExitWasapi();
		CoUninitialize();
		win.Delete();
		return -1;
	}

	// ウィンドウループ
	while(1) {
		MSG msg;
		int ret = GetMessage( &msg,NULL,0,0 );		// メッセージが登録されるまでブロック
		if( ret==0 || ret==-1 ) {
			// 終了コードなら抜ける
			break;
		}
		// メッセージを処理する
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	// WASAPI終了
	ExitWasapi();

	// COMの終了
	CoUninitialize();

	// ウィンドウ削除
	win.Delete();

	return 0;
}
#endif