// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Windows ヘッダーから使用されていない部分を除外します。
// Windows ヘッダー ファイル:
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <Winuser.h>
#include <WinSock2.h>

// C ランタイム ヘッダー ファイル
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string.h>

// C std library ヘッダー ファイル
#include <string>
#include <vector>

// TODO: プログラムに必要な追加ヘッダーをここで参照
#include "utf8conv.h"
#include "utf8except.h"
