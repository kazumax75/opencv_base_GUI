// stdafx.h : �W���̃V�X�e�� �C���N���[�h �t�@�C���̃C���N���[�h �t�@�C���A�܂���
// �Q�Ɖ񐔂������A�����܂�ύX����Ȃ��A�v���W�F�N�g��p�̃C���N���[�h �t�@�C��
// ���L�q���܂��B
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Windows �w�b�_�[����g�p����Ă��Ȃ����������O���܂��B
// Windows �w�b�_�[ �t�@�C��:
#include <windows.h>

// C �����^�C�� �w�b�_�[ �t�@�C��
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



//�o�̓E�B���h�E�̏����ʒu�ƃT�C�Y
#define POS_X -1280
#define POS_Y 170
#define WIDTH  1280
#define HEIGHT 768
//
#define BORDER_CENTER 456
/*
WINAPI�ł́A���ʉ��̕��������Đ����ł��Ȃ��悤�Ȃ̂ŁA
���ʉ���HSP�ō쐬�����O���̃v���O�����ɍĐ������邱�Ƃɂ���B
���ʉ����Đ����������ɁA�ȉ��̃E�B���h�E���b�Z�[�W�����̊O���v���O������send����B
*/
#define YARI_MYMESSAGE (WM_APP + 1)
#define YARARE_MYMESSAGE (WM_APP + 2)

// TODO: �v���O�����ɕK�v�Ȓǉ��w�b�_�[�������ŎQ�Ƃ��Ă�������
