//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_h)
#define mfc_h

//---------------------------------------------------------------------------
//
//	#include
//
//---------------------------------------------------------------------------
#define WINVER					0x500	// Windows98,Me,2000,XP�ȍ~
#define _WIN32_WINNT			0x410	// Windows98,Me,2000,XP�ȍ~
#define VC_EXTRALEAN

// MFC
#include <afxwin.h>
#include <afxext.h>
#include <afxdlgs.h>
#include <afxcmn.h>
#include <afxmt.h>
#include <afxconv.h>

// Win32API
#include <imm.h>
#include <mmsystem.h>
#include <shlobj.h>

// DirectX
#define DIRECTSOUND_VERSION		0x500	// DirectX5���w��
#include <dsound.h>
#define DIRECTINPUT_VERSION		0x500	// DirectX5���w��
#include <dinput.h>

// C Runtime
#include <math.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

//---------------------------------------------------------------------------
//
//	#define
//
//---------------------------------------------------------------------------
#if defined(_DEBUG)
#define new		DEBUG_NEW
#endif	// _DEBUG

#if defined(_MSC_VER) && defined(_M_IX86)
#define FASTCALL	__fastcall
#else
#define FASTCALL
#endif	// _MSC_VER

//---------------------------------------------------------------------------
//
//	�N���X�錾
//
//---------------------------------------------------------------------------
class VM;
										// ���z�}�V��

class CApp;
										// �A�v���P�[�V����
class CFrmWnd;
										// �t���[���E�B���h�E
class CDrawView;
										// �`��r���[
class CStatusView;
										// �X�e�[�^�X�r���[

class CSubWnd;
										// �T�u�E�B���h�E
class CSubTextWnd;
										// �T�u�E�B���h�E(�e�L�X�g)
class CSubListWnd;
										// �T�u�E�B���h�E(���X�g�R���g���[��)

class CComponent;
										// �R���|�[�l���g����
class CConfig;
										// �R���t�B�O�R���|�[�l���g
class CScheduler;
										// �X�P�W���[���R���|�[�l���g
class CSound;
										// �T�E���h�R���|�[�l���g
class CInput;
										// �C���v�b�g�R���|�[�l���g
class CPort;
										// �|�[�g�R���|�[�l���g
class CMIDI;
										// MIDI�R���|�[�l���g
class CTKey;
										// TrueKey�R���|�[�l���g
class CHost;
										// �z�X�g�R���|�[�l���g
class CInfo;
										// ���R���|�[�l���g

//---------------------------------------------------------------------------
//
//	�O���[�o��
//
//---------------------------------------------------------------------------
extern VM *pVM;
										// ���z�}�V��
BOOL FASTCALL IsJapanese(void);
										// ���{����̔���
BOOL FASTCALL IsWinNT(void);
										// WindowsNT���̔���
BOOL FASTCALL Support932(void);
										// CP932�T�|�[�g�L���̔���
BOOL FASTCALL IsMMX(void);
										// MMX���̔���
BOOL FASTCALL IsCMOV(void);
										// CMOV�T�|�[�g�̔���
void FASTCALL GetMsg(UINT uID, CString& string);
										// ���b�Z�[�W�擾
VM* FASTCALL GetVM(void);
										// ���z�}�V�����擾
void FASTCALL LockVM(void);
										// ���z�}�V�������b�N
void FASTCALL UnlockVM(void);
										// ���z�}�V�����A�����b�N
BOOL FASTCALL FileOpenDlg(CWnd *pParent, LPTSTR lpszPath, UINT nFilterID);
										// �t�@�C���I�[�v���_�C�A���O
BOOL FASTCALL FileSaveDlg(CWnd *pParent, LPTSTR lpszPath, LPCTSTR lpszExt, UINT nFilterID);
										// �t�@�C���Z�[�u�_�C�A���O
void FASTCALL SetInfoMsg(LPCTSTR lpszBuf, BOOL bRec);
										// ��񃁃b�Z�[�W�ێ��T�u
int FASTCALL DrawTextWide(HDC hDC, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat);
										// DrawTextW���b�p

#endif	// mfc_h
#endif	// _WIN32
