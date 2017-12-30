// otoeditor.cpp: アプリケーションのエントリ ポイントを定義します。
//
#include "resource.h"
#include "stdafx.h"
#include "otoeditor.h"
#include "midi.h"
#include "CWav.h"

#define MAX_LOADSTRING 100

#define DEBUGMODE				// デバッグ出力しない場合はコメント化する
#include "DEBUG.H"

// マクロ
#define SAFE_RELEASE(x)		{ if( x ) { x->Release(); x=NULL; } }
#define SAFE_FREE(x)		{ if( x ) { free(x); x=NULL; } }

// グローバル変数:
WCHAR szClassName[] = _T("edit01");				// ウィンドウクラス
WCHAR szFileName[256];							// オープンするファイル名（パス付き）
WCHAR szFile[64];								// ファイル名

HINSTANCE hInst;                                // 現在のインターフェイス
WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名
HWND hWnd;										// メインウィンドウのハンドル
HWND hEdit;										// エディットウィンドウのハンドル
midi* pMidi;									// midiクラス

CharCode charCode = SJIS;						// 編集対象ファイルのコード系(デフォルトはSJIS）

// このコード モジュールに含まれる関数（WASAPI関連以外）
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
int					OpenMyFile(HWND);
int					SaveMyFile(HWND);
int					RegisterKeyHook(HINSTANCE hInstance);
LRESULT CALLBACK	LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

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
#pragma comment(lib, "imm32.lib")

// インターフェースのGUIDの実体(プロジェクト内のCファイルに必ず1つ必要)
const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioClock = __uuidof(IAudioClock);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

// WASAPI関連の変数
//CWindow					win;							// ウィンドウ
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

// WASAPI関連の関数
// 再生スレッド
static DWORD WINAPI PlayThread(LPVOID param);
// WASAPIの初期化
BOOL InitWasapi(int latency);
// WASAPIの終了
void ExitWasapi();

//////////////////////////////////////////////////////////////////////////////////////
// WASAPI関連
//////////////////////////////////////////////////////////////////////////////////////


// アプリケーションエントリ
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: ここにコードを挿入してください。

    // グローバル文字列を初期化しています。
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_OTOEDITOR, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);
	RegisterKeyHook(hInstance);

    // アプリケーションの初期化を実行します:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_OTOEDITOR));

	// Midiのオープン
	if (!pMidi->OpenMidi()) {
		return FALSE;
	}

	// 音色の設定
	pMidi->SetTone(0x00);

    MSG msg;

    // メイン メッセージ ループ:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

	// Midiのクローズ
	pMidi->StopNote();

	return (int) msg.wParam;

}



//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_OTOEDITOR));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_OTOEDITOR);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

// キーボードフックの設定
int RegisterKeyHook(HINSTANCE hInstance)
{
	HHOOK hHook = SetWindowsHookEx(
		WH_KEYBOARD_LL,				// idHook
		LowLevelKeyboardProc,		// lpfn
		hInstance,					// hMod
		0							// dwThreadId
	);

	return int(hHook);
}


//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します。
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // グローバル変数にインスタンス処理を格納します。

   hWnd = CreateWindow(szWindowClass,
	  szTitle, WS_OVERLAPPEDWINDOW,		// ウィンドウタイトル
      CW_USEDEFAULT,					// 
	  0,								// 
	  CW_USEDEFAULT,					// 
	  0,								//
	  nullptr,							//
	  nullptr,							//
	  hInstance,						//
	  nullptr							// 
   );

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   // midiクラス
   pMidi = new midi();
   if (!pMidi) {
	   return FALSE;
   }

   return TRUE;
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND  - アプリケーション メニューの処理
//  WM_PAINT    - メイン ウィンドウの描画
//  WM_DESTROY  - 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int id;
	RECT rc;

    switch (message)
    {
	case WM_CREATE:
		GetClientRect(hWnd, &rc);
		hEdit = CreateWindow(
			_T("EDIT"),
			NULL,
			WS_CHILD | WS_VISIBLE |
			ES_WANTRETURN | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL /* |
			ES_AUTOHSCROLL | WS_HSCROLL */,
			0, 0,
			rc.right, rc.bottom,
			hWnd,
			(HMENU)IDC_OTOEDITOR,
			hInst,
			NULL);
		SendMessage(hEdit, EM_SETLIMITTEXT, (WPARAM)1024 * 64, 0);
		break;
	
	case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 選択されたメニューの解析:
            switch (wmId)
            {
			case IDM_OPEN:
				OpenMyFile(hWnd);
				break;
			case IDM_SAVENEWNAME:
				SaveMyFile(hWnd);
				break;
			case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

	case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: HDC を使用する描画コードをここに追加してください...
            EndPaint(hWnd, &ps);
        }
        break;

	case WM_SIZE:
		GetClientRect(hWnd, &rc);
		MoveWindow(hEdit, rc.left, rc.top,
			rc.right, rc.bottom,
			TRUE);
		break;

	case WM_CLOSE:
		id = MessageBox(hWnd,
			_T("終了してもよいですか"),
			_T("終了確認"),
			MB_YESNO | MB_ICONQUESTION);
		if (id == IDYES) {
			DestroyWindow(hWnd);
		}
		break;

	case WM_DESTROY:
        PostQuitMessage(0);
        break;

	case WM_USER:						// ユーザ定義メッセージ
		// ここに処理を書く
		{
			KBDLLHOOKSTRUCT* pHookStruct = (KBDLLHOOKSTRUCT*)lParam;
			DWORD vkCode = pHookStruct->vkCode;

			switch (wParam)
			{
			case WM_KEYDOWN:
				if (vkCode == 0x41) {		// 'A'
					//Beep(131, 90);
					pMidi->SetPitch(0x3c);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x42) {	// 'B'
					//Beep(147, 90);
					pMidi->SetPitch(0x3e);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x43) {	// 'C'
					//Beep(165, 90);
					pMidi->SetPitch(0x40);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x44) {	// 'D'
					//Beep(175, 90);
					pMidi->SetPitch(0x41);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x45) {	// 'E'
					//Beep(196, 90);
					pMidi->SetPitch(0x43);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x46) {	// 'F'
					//Beep(220, 90);
					pMidi->SetPitch(0x45);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x47) {	// 'G'
					//Beep(247, 90);
					pMidi->SetPitch(0x47);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x48) {	// 'H'
					//Beep(262, 90);
					pMidi->SetPitch(0x48);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x49) {	// 'I'
					//Beep(294, 90);
					pMidi->SetPitch(0x4a);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x4a) {	// 'J'
					//Beep(330, 90);
					pMidi->SetPitch(0x4c);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x4b) {	// 'K'
					//Beep(349, 90);
					pMidi->SetPitch(0x4d);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x4c) {	// 'L'
					//Beep(392, 90);
					pMidi->SetPitch(0x4f);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x4d) {	// 'M'
					//Beep(440, 90);
					pMidi->SetPitch(0x51);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x4e) {	// 'N'
					//Beep(494, 90);
					pMidi->SetPitch(0x53);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x4f) {	// 'O'
					//Beep(523, 90);
					pMidi->SetPitch(0x54);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x50) {	// 'P'
					//Beep(587, 90);
					pMidi->SetPitch(0x56);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x51) {	// 'Q'
					//Beep(659, 90);
					pMidi->SetPitch(0x58);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x52) {	// 'R'
					//Beep(698, 90);
					pMidi->SetPitch(0x59);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x53) {	// 'S'
					//Beep(784, 90);
					pMidi->SetPitch(0x5b);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x54) {	// 'T'
					//Beep(880, 90);
					pMidi->SetPitch(0x5d);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x55) {	// 'U'
					//Beep(988, 90);
					pMidi->SetPitch(0x5f);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x56) {	// 'V'
					//Beep(1047, 90);
					pMidi->SetPitch(0x60);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x57) {	// 'W'
					//Beep(1175, 90);
					pMidi->SetPitch(0x62);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x58) {	// 'X'
					//Beep(1319, 90);
					pMidi->SetPitch(0x64);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x59) {	// 'Y'
					//Beep(1397, 90);
					pMidi->SetPitch(0x65);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x5a) {	// 'Z'
					//Beep(1568, 90);
					pMidi->SetPitch(0x67);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0x5b) {	// '左WIN'
					//Beep(1760, 90);
					pMidi->SetPitch(0x69);
					pMidi->SetVolume(0x70);
				}
   				else if (vkCode == 0x20) {	// 'SPACE'
					pMidi->SetPitch(0x6b);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0xbe) {	// '.'
					pMidi->SetPitch(0x3b);
					pMidi->SetVolume(0x70);
				}
				else if (vkCode == 0xbc) {	// ','
					pMidi->SetPitch(0x39);
					pMidi->SetVolume(0x70);
				}

				break;

			case  WM_KEYUP:
				//pMidi->SetVolume(0x00);
				break;

			case  WM_SYSKEYDOWN:
				break;

			case  WM_SYSKEYUP:
				break;

			default:
				;
			}
		}

	default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// バージョン情報ボックスのメッセージ ハンドラー
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// ファイルオープンメニューのメッセージ ハンドラー
int OpenMyFile(HWND hWnd)
{
	OPENFILENAME ofn;
	HANDLE hFile;
//	WCHAR szStr[1024 * 64];
	DWORD dwBytesRead;
	WCHAR szTitle[64], *szTitle_org = _T("OtoMemo[%s]");
	WCHAR szFileName2[512], *szFileName2_t = _T("\\\\?\\");

	HANDLE	hHeap = NULL;
	DWORD	cFileLen = 0;
	PBYTE	pBuff = NULL;
	BOOL	bUnicode = false;

	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = _T("text(*.txt)\0*.txt\0All files(*.*)\0*.*\0\0");
	ofn.lpstrFile = szFileName;
	ofn.lpstrFileTitle = szFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = _T("txt");
	ofn.lpstrTitle = _T("ファイルオープン！");

	GetOpenFileName(&ofn);
	memset(szFileName2, 0x00, sizeof(szFileName2));
	wcscpy_s(szFileName2, 512, szFileName2_t);
	wcscat_s(szFileName2, 512, szFileName);

	//
	// 読み出し用にファイルをオープンします。
	//
	hFile = CreateFile(szFileName, GENERIC_READ, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile != INVALID_HANDLE_VALUE) {
		SetFilePointer(hFile, 0, 0, FILE_BEGIN);

		// ファイルサイズを調べ、ファイルのデータを読み込むバッファを用意する
		cFileLen = GetFileSize(hFile, NULL);
		hHeap = GetProcessHeap();
		pBuff = (PBYTE)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cFileLen + sizeof(WCHAR));
		if (!pBuff) {
			CloseHandle(hFile);
			return FALSE;
		}

		// ファイルのデータを読み込みます。
		if (!ReadFile(hFile, pBuff, cFileLen, &dwBytesRead, NULL)) {
			CloseHandle(hFile);
			HeapFree(hHeap, 0, pBuff);
			return FALSE;
		}

		// ファイルをクローズします。
		CloseHandle(hFile);

		// エディットコントロールに、データをセットします。
		// このときに、Unicode のデータかどうかによって処理が変ります。
		bUnicode = IsTextUnicode(pBuff,	dwBytesRead, NULL);
		if (bUnicode) {
			// Unicode の場合はそのまま SetWindowText 関数でデータをセット
			charCode = UTF16_LE;
			SetWindowText(hEdit, (PTSTR)pBuff);
			HeapFree(hHeap, 0, pBuff);

		} else {
			// Unicode のデータではない場合、Unicode に変換しなければならない。
			// MultiByteToWideChar 関数を利用する。
			// ここでは Unicode のデータではない場合、システム既定のコードページと
			// みなして、データ変換する。(実際にはシステム既定のコードページのデータではない場合、
			// 誤ったデータ変換をしてしまうために、いわゆる文字化けを起こす。
			//
			if (charCode == SJIS) {
				// 必要なバッファサイズの取得
				INT cchRecWChar = MultiByteToWideChar(
					CP_ACP,
					0,
					(LPCSTR)pBuff,
					dwBytesRead,
					NULL,
					0);

				// メモリ割り当て
				DWORD	dwAllocByte = (cchRecWChar + 1) * sizeof(WCHAR);
				PWSTR	pUnicodeText = (PWSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwAllocByte);
				if (!pUnicodeText) {
					HeapFree(hHeap, 0, pBuff);
					return FALSE;
				}

				// データ変換
				MultiByteToWideChar(
					CP_ACP,
					0,
					(LPCSTR)pBuff,
					cFileLen,
					pUnicodeText,
					cchRecWChar);

				// エディットコントロールにデータをセットする
				SetWindowText(hEdit, (PTSTR)pUnicodeText);

				// 使ったメモリのクリーンアップ
				HeapFree(hHeap, 0, pBuff);
				HeapFree(hHeap, 0, pUnicodeText);
			}
			else if (charCode == UTF8) {
				;
			}
			else {
				;
			}
		}

		// ウィンドウタイトルの設定
		wsprintf(szTitle, szTitle_org, szFile);
		SetWindowText(hWnd, szTitle);
	}

	return 0;
}

// 名前を付けて保存メニューのメッセージ ハンドラー
int SaveMyFile(HWND hWnd)
{
	OPENFILENAME ofn;
	HANDLE	hFile;
	HANDLE	hHeap = NULL;
	LPWSTR	pBuff = NULL;
	//WCHAR szStr[1024 * 64];
	DWORD	dwBytesWrite;
	DWORD	dwAccBytes;
	WCHAR	szFileName2[512], *szFileName2_t = _T("\\\\?\\");

	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = _T("text(*.txt)\0*.txt\0All files(*.*)\0*.*\0\0");
	ofn.lpstrFile = szFileName;
	ofn.lpstrFileTitle = szFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY;
	ofn.lpstrDefExt = _T("txt");
	ofn.lpstrTitle = _T("ファイル保存");

	GetOpenFileName(&ofn);
	memset(szFileName2, 0x00, sizeof(szFileName2));
	wcscpy_s(szFileName2, 512, szFileName2_t);
	wcscat_s(szFileName2, 512, szFileName);

	// コントロールテキストサイズを調べ、データを読み込むバッファを用意する
	dwBytesWrite = GetWindowTextLengthW(hEdit);
	hHeap = GetProcessHeap();
	pBuff = (LPWSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (dwBytesWrite + 1) * sizeof(WCHAR));
	if (!pBuff) {
		HeapFree(hHeap, 0, pBuff);
		return FALSE;
	}

	// コントロールのデータを読み込みます。
	if (!GetWindowTextW(hEdit, pBuff, dwBytesWrite)) {
		HeapFree(hHeap, 0, pBuff);
		return FALSE;
	}

	// 読込み元のファイルがSJISか、ファイルを新規に作成した場合は、SJISに変換します。
	if (charCode == SJIS) {
		// 変換先のバッファを獲得
		HANDLE	hTmpHeap = GetProcessHeap();
		LPSTR	pTmpBuff = (LPSTR)HeapAlloc(hTmpHeap, HEAP_ZERO_MEMORY, (dwBytesWrite + 1) * sizeof(WCHAR));
		if (!pTmpBuff) {
			HeapFree(hTmpHeap, 0, pTmpBuff);
			return FALSE;
		}

		// データ変換
		WideCharToMultiByte(
			CP_ACP,
			0,
			(LPWSTR)pBuff,
			-1,
			pTmpBuff,
			(dwBytesWrite + 1) * sizeof(WCHAR),
			NULL,
			NULL );

		// ファイルに書き込みます（マルチバイト）
		hFile = CreateFile(szFileName, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE) {
			SetFilePointer(hFile, 0, 0, FILE_BEGIN);
			int mbLen = strlen(pTmpBuff);
			WriteFile(hFile, pTmpBuff, mbLen, &dwAccBytes, NULL);
			CloseHandle(hFile);
		}

	}
	else {
		// ファイルに書き込みます（ユニコード（UTF-16)）
		hFile = CreateFile(szFileName, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE) {
			SetFilePointer(hFile, 0, 0, FILE_BEGIN);
			WriteFile(hFile, pBuff, (dwBytesWrite + 1) * sizeof(WCHAR), &dwAccBytes, NULL);
			CloseHandle(hFile);
		}

	}

	HeapFree(hHeap, 0, pBuff);

	return 0;
}



// キーボードフックのメッセージ ハンドラー
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{

	if (nCode >= HC_ACTION)
	{
		// フックのチェーンを遅延させるといけないので、自プロセスへメッセージ
		// 投げるだけにする
		SendMessage(hWnd, WM_USER, wParam, lParam);
	}

	// 後続の処理にパラメタを渡す
	HHOOK hhk = NULL;								// 使っていないのでNULLを設定しておく
	CallNextHookEx(hhk, nCode, wParam, lParam);		// メッセージチェーンにメッセージを流す

	return 0;

}

//////////////////////////////////////////////////////////////////////////////////////
// WASAPI関連メソッド
//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////
// 再生スレッド
//////////////////////////////////////////////////////////////////////////////////////
DWORD CALLBACK PlayThread(LPVOID param)
{
	DEBUG("スレッド開始\n");

	while (bThreadLoop) {
		// 次のバッファ取得が必要になるまで待機
		DWORD retval = WaitForSingleObject(hEvent, 2000);
		if (retval != WAIT_OBJECT_0) {
			DEBUG("タイムアウト\n");
			pAudioClient->Stop();
			break;
		}

		// 今回必要なフレーム数を取得
		UINT32 frame_count;
		HRESULT ret = pAudioClient->GetBufferSize(&frame_count);
		//		ODS( "フレーム数    : %d\n",frame_count );

		// フレーム数からバッファサイズを算出
		int buf_size = frame_count * iFrameSize;
		//		ODS( "バッファサイズ : %dbyte\n",buf_size );


		// 出力バッファのポインタを取得
		LPBYTE dst;
		ret = pRenderClient->GetBuffer(frame_count, &dst);
		if (SUCCEEDED(ret)) {
			// ソースバッファのポインタを取得
			LPBYTE src = wav.GetBuffer();

			// 現在のカーソルから次に必要なバッファサイズを加算したときにWAVバッファのサイズを超えるか
			if (iAddr + buf_size>wav.GetBufferSize()) {
				////////////////////////////////////////////////////////////////////////////////////////
				// 超える場合はまず現在のカーソルから最後までをコピーし、次に残った分を先頭から補充する
				////////////////////////////////////////////////////////////////////////////////////////

				// WAVバッファサイズから現在のカーソル位置を差し引いたものが最後までのバッファサイズ
				int last_size = wav.GetBufferSize() - iAddr;

				// 現在のカーソルから最後までのバッファを出力バッファの先頭にコピーする
				memcpy(&dst[0], &src[iAddr], last_size);

				// 今回必要なサイズから今コピーしたサイズを引いたものが先頭から補充するバッファサイズ
				int begin_size = buf_size - last_size;

				// WAVバッファの先頭から補充サイズ分を出力バッファに追加コピーする
				memcpy(&dst[last_size], &src[0], begin_size);

				// 補充した分のサイズを現在のカーソルとする
				iAddr = begin_size;

				ODS("LOOP OK\n");
			}
			else {
				////////////////////////////////////////////////////////////////////////////////////////
				// 超えない場合は現在のカーソルから必要なバッファ分をコピーしてカーソルを進める
				////////////////////////////////////////////////////////////////////////////////////////

				// WAVバッファの現在のカーソルから出力バッファに指定サイズ分コピーする
				memcpy(&dst[0], &src[iAddr], buf_size);

				// カーソルをコピーした分だけ進める
				iAddr += buf_size;
			}

			// バッファを開放
			pRenderClient->ReleaseBuffer(frame_count, 0);
		}

	}

	DEBUG("スレッド終了\n");
	return 0;
}


//////////////////////////////////////////////////////////////////////////////////////
// WASAPIの初期化
//////////////////////////////////////////////////////////////////////////////////////
BOOL InitWasapi(int latency)
{
	HRESULT ret;

	// マルチメディアデバイス列挙子
	ret = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pDeviceEnumerator);
	if (FAILED(ret)) {
		DEBUG("CLSID_MMDeviceEnumeratorエラー\n");
		return FALSE;
	}

	// デフォルトのデバイスを選択
	ret = pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
	if (FAILED(ret)) {
		DEBUG("GetDefaultAudioEndpointエラー\n");
		return FALSE;
	}

	// オーディオクライアント
	ret = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
	if (FAILED(ret)) {
		DEBUG("オーディオクライアント取得失敗\n");
		return FALSE;
	}

	// フォーマットの構築
	WAVEFORMATEXTENSIBLE wf;
	ZeroMemory(&wf, sizeof(wf));
	wf.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE);
	wf.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	wf.Format.nChannels = 2;
	wf.Format.nSamplesPerSec = 44100;
	wf.Format.wBitsPerSample = 16;
	wf.Format.nBlockAlign = wf.Format.nChannels * wf.Format.wBitsPerSample / 8;
	wf.Format.nAvgBytesPerSec = wf.Format.nSamplesPerSec * wf.Format.nBlockAlign;
	wf.Samples.wValidBitsPerSample = wf.Format.wBitsPerSample;
	wf.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
	wf.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

	// 1サンプルのサイズを保存(16bitステレオなら4byte)
	iFrameSize = wf.Format.nBlockAlign;

	// フォーマットのサポートチェック
	ret = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, (WAVEFORMATEX*)&wf, NULL);
	if (FAILED(ret)) {
		DEBUG("未サポートのフォーマット\n");
		return FALSE;
	}

	// レイテンシ設定
	REFERENCE_TIME default_device_period = 0;
	REFERENCE_TIME minimum_device_period = 0;

	if (latency != 0) {
		default_device_period = (REFERENCE_TIME)latency * 10000LL;		// デフォルトデバイスピリオドとしてセット
		DEBUG("レイテンシ指定             : %I64d (%fミリ秒)\n", default_device_period, default_device_period / 10000.0);
	}
	else {
		ret = pAudioClient->GetDevicePeriod(&default_device_period, &minimum_device_period);
		DEBUG("デフォルトデバイスピリオド : %I64d (%fミリ秒)\n", default_device_period, default_device_period / 10000.0);
		DEBUG("最小デバイスピリオド       : %I64d (%fミリ秒)\n", minimum_device_period, minimum_device_period / 10000.0);
	}

	// 初期化
	UINT32 frame = 0;
	ret = pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE,
		AUDCLNT_STREAMFLAGS_NOPERSIST | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
		default_device_period,				// デフォルトデバイスピリオド値をセット
		default_device_period,				// デフォルトデバイスピリオド値をセット
		(WAVEFORMATEX*)&wf,
		NULL);
	if (FAILED(ret)) {
		if (ret == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED) {
			DEBUG("バッファサイズアライメントエラーのため修正する\n");

			// 修正後のフレーム数を取得
			ret = pAudioClient->GetBufferSize(&frame);
			DEBUG("修正後のフレーム数         : %d\n", frame);
			default_device_period = (REFERENCE_TIME)(10000.0 *						// (REFERENCE_TIME(100ns) / ms) *
				1000 *						// (ms / s) *
				frame /						// frames /
				wf.Format.nSamplesPerSec +	// (frames / s)
				0.5);							// 四捨五入？
			DEBUG("修正後のレイテンシ         : %I64d (%fミリ秒)\n", default_device_period, default_device_period / 10000.0);

			// 一度破棄してオーディオクライアントを再生成
			SAFE_RELEASE(pAudioClient);
			ret = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
			if (FAILED(ret)) {
				DEBUG("オーディオクライアント再取得失敗\n");
				return FALSE;
			}

			// 再挑戦
			ret = pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE,
				AUDCLNT_STREAMFLAGS_NOPERSIST | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
				default_device_period,
				default_device_period,
				(WAVEFORMATEX*)&wf,
				NULL);
		}

		if (FAILED(ret)) {
			DEBUG("初期化失敗 : 0x%08X\n", ret);
			switch (ret)
			{
			case AUDCLNT_E_NOT_INITIALIZED:					DEBUG("AUDCLNT_E_NOT_INITIALIZED\n");					break;
			case AUDCLNT_E_ALREADY_INITIALIZED:				DEBUG("AUDCLNT_E_ALREADY_INITIALIZED\n");				break;
			case AUDCLNT_E_WRONG_ENDPOINT_TYPE:				DEBUG("AUDCLNT_E_WRONG_ENDPOINT_TYPE\n");				break;
			case AUDCLNT_E_DEVICE_INVALIDATED:				DEBUG("AUDCLNT_E_DEVICE_INVALIDATED\n");				break;
			case AUDCLNT_E_NOT_STOPPED:						DEBUG("AUDCLNT_E_NOT_STOPPED\n");						break;
			case AUDCLNT_E_BUFFER_TOO_LARGE:				DEBUG("AUDCLNT_E_BUFFER_TOO_LARGE\n");				break;
			case AUDCLNT_E_OUT_OF_ORDER:					DEBUG("AUDCLNT_E_OUT_OF_ORDER\n");					break;
			case AUDCLNT_E_UNSUPPORTED_FORMAT:				DEBUG("AUDCLNT_E_UNSUPPORTED_FORMAT\n");				break;
			case AUDCLNT_E_INVALID_SIZE:					DEBUG("AUDCLNT_E_INVALID_SIZE\n");					break;
			case AUDCLNT_E_DEVICE_IN_USE:					DEBUG("AUDCLNT_E_DEVICE_IN_USE\n");					break;
			case AUDCLNT_E_BUFFER_OPERATION_PENDING:		DEBUG("AUDCLNT_E_BUFFER_OPERATION_PENDING\n");		break;
			case AUDCLNT_E_THREAD_NOT_REGISTERED:			DEBUG("AUDCLNT_E_THREAD_NOT_REGISTERED\n");			break;
				//			case AUDCLNT_E_NO_SINGLE_PROCESS:				DEBUG( "AUDCLNT_E_NO_SINGLE_PROCESS\n" );				break;	// VC2010では未定義？
			case AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED:		DEBUG("AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED\n");		break;
			case AUDCLNT_E_ENDPOINT_CREATE_FAILED:			DEBUG("AUDCLNT_E_ENDPOINT_CREATE_FAILED\n");			break;
			case AUDCLNT_E_SERVICE_NOT_RUNNING:				DEBUG("AUDCLNT_E_SERVICE_NOT_RUNNING\n");				break;
			case AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED:		DEBUG("AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED\n");		break;
			case AUDCLNT_E_EXCLUSIVE_MODE_ONLY:				DEBUG("AUDCLNT_E_EXCLUSIVE_MODE_ONLY\n");				break;
			case AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL:	DEBUG("AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL\n");	break;
			case AUDCLNT_E_EVENTHANDLE_NOT_SET:				DEBUG("AUDCLNT_E_EVENTHANDLE_NOT_SET\n");				break;
			case AUDCLNT_E_INCORRECT_BUFFER_SIZE:			DEBUG("AUDCLNT_E_INCORRECT_BUFFER_SIZE\n");			break;
			case AUDCLNT_E_BUFFER_SIZE_ERROR:				DEBUG("AUDCLNT_E_BUFFER_SIZE_ERROR\n");				break;
			case AUDCLNT_E_CPUUSAGE_EXCEEDED:				DEBUG("AUDCLNT_E_CPUUSAGE_EXCEEDED\n");				break;
			default:										DEBUG("UNKNOWN\n");									break;
			}
			return FALSE;
		}
	}

	// イベント生成
	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!hEvent) {
		DEBUG("イベントオブジェクト作成失敗\n");
		return FALSE;
	}

	// イベントのセット
	ret = pAudioClient->SetEventHandle(hEvent);
	if (FAILED(ret)) {
		DEBUG("イベントオブジェクト設定失敗\n");
		return FALSE;
	}

	// レンダラーの取得
	ret = pAudioClient->GetService(IID_IAudioRenderClient, (void**)&pRenderClient);
	if (FAILED(ret)) {
		DEBUG("レンダラー取得エラー\n");
		return FALSE;
	}

	// WASAPI情報取得
	ret = pAudioClient->GetBufferSize(&frame);
	DEBUG("設定されたフレーム数       : %d\n", frame);

	UINT32 size = frame * iFrameSize;
	DEBUG("設定されたバッファサイズ   : %dbyte\n", size);
	DEBUG("1サンプルの時間            : %f秒\n", (float)size / wf.Format.nSamplesPerSec);

	// ゼロクリアをしてイベントをリセット
	LPBYTE pData;
	ret = pRenderClient->GetBuffer(frame, &pData);
	if (SUCCEEDED(ret)) {
		ZeroMemory(pData, size);
		pRenderClient->ReleaseBuffer(frame, 0);
	}

	// スレッドループフラグを立てる
	bThreadLoop = TRUE;

	// 再生スレッド起動
	DWORD dwThread;
	hThread = CreateThread(NULL, 0, PlayThread, NULL, 0, &dwThread);
	if (!hThread) {
		// 失敗
		return FALSE;
	}

	// 再生開始
	pAudioClient->Start();

	DEBUG("WASAPI初期化完了\n");
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
// WASAPIの終了
//////////////////////////////////////////////////////////////////////////////////////
void ExitWasapi()
{
	// スレッドループフラグを落とす
	bThreadLoop = FALSE;

	// スレッド終了処理
	if (hThread) {
		// スレッドが完全に終了するまで待機
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
		hThread = NULL;
	}

	// イベントを開放処理
	if (hEvent) {
		CloseHandle(hEvent);
		hEvent = NULL;
	}

	// インターフェース開放
	SAFE_RELEASE(pRenderClient);
	SAFE_RELEASE(pAudioClient);
	SAFE_RELEASE(pDevice);
	SAFE_RELEASE(pDeviceEnumerator);

	DEBUG("WASAPI終了\n");
}
