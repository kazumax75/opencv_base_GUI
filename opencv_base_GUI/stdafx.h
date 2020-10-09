// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Windows ヘッダーから使用されていない部分を除外します。
// Windows ヘッダー ファイル:
#include <windows.h>

// C ランタイム ヘッダー ファイル
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <Shellapi.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>



//出力ウィンドウの初期位置とサイズ
#define POS_X -1280
#define POS_Y 170
#define WIDTH  1280
#define HEIGHT 768
//
#define BORDER_CENTER 456
/*
WINAPIでは、効果音の複数同時再生ができないようなので、
効果音はHSPで作成した外部のプログラムに再生させることにする。
効果音を再生したい時に、以下のウィンドウメッセージをその外部プログラムにsendする。
*/
#define YARI_MYMESSAGE (WM_APP + 1)
#define YARARE_MYMESSAGE (WM_APP + 2)

// TODO: プログラムに必要な追加ヘッダーをここで参照してください
