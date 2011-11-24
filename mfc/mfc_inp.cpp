//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �C���v�b�g ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "crtc.h"
#include "keyboard.h"
#include "mouse.h"
#include "ppi.h"
#include "fileio.h"
#include "mfc_frm.h"
#include "mfc_com.h"
#include "mfc_sch.h"
#include "mfc_res.h"
#include "mfc_cfg.h"
#include "mfc_inp.h"

//===========================================================================
//
//	�C���v�b�g
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CInput::CInput(CFrmWnd *pWnd) : CComponent(pWnd)
{
	int i;
	int nAxis;
	int nButton;

	// �R���|�[�l���g�p�����[�^
	m_dwID = MAKEID('I', 'N', 'P', ' ');
	m_strDesc = _T("Input Manager");

	// ���ʃ��[�N������
	m_lpDI = NULL;
	m_bActive = TRUE;
	m_bMenu = FALSE;
	m_pCRTC = NULL;
	m_dwDispCount = 0;
	m_dwProcessCount = 0;

	// �L�[�{�[�h���[�N������
	m_pKeyboard = NULL;

	// �}�E�X���[�N������
	m_pMouse = NULL;
	m_lpDIMouse = NULL;
	m_dwMouseAcquire = 0;
	m_bMouseMode = FALSE;
	m_nMouseX = 0;
	m_nMouseY = 0;
	m_bMouseB[0] = FALSE;
	m_bMouseB[1] = FALSE;
	m_dwMouseMid = 0;
	m_bMouseMid = TRUE;

	// �W���C�X�e�B�b�N���[�N������
	m_pPPI = NULL;
	m_dwJoyDevs = 0;
	m_bJoyEnable = TRUE;
	for (i=0; i<JoyDevices; i++) {
		// �f�o�C�X
		m_lpDIJoy[i] = NULL;
		m_lpDIDev2[i] = NULL;

		// �R���t�B�O
		memset(&m_JoyCfg[i], 0, sizeof(JOYCFG));
		// nDevice�̐ݒ�:
		// ���蓖�ăf�o�C�X+1 (0�͖����蓖��)
		m_JoyCfg[i].nDevice = i + 1;
		for (nAxis=0; nAxis<JoyAxes; nAxis++) {
			if (nAxis < 4) {
				// dwAxis�̐ݒ�:
				// ��ʃ��[�h ���蓖�ă|�[�g (0x00000 or 0x10000)
				// ���ʃ��[�h ���蓖�Ď�+1 (1�`4�A0�͖����蓖��)
				m_JoyCfg[i].dwAxis[nAxis] = (DWORD)((i << 16) | (nAxis + 1));
			}
			m_JoyCfg[i].bAxis[nAxis] = FALSE;
		}
		for (nButton=0; nButton<JoyButtons; nButton++) {
			if (nButton < 8) {
				// dwButton�̐ݒ�:
				// ��ʃ��[�h ���蓖�ă|�[�g (0x00000 or 0x10000)
				// ���ʃ��[�h ���蓖�ă{�^��+1 (1�`8�A0�͖����蓖��)
				m_JoyCfg[i].dwButton[nButton] = (DWORD)((i << 16) | (nButton + 1));
			}
			// �A�˂Ȃ��A�J�E���^������
			m_JoyCfg[i].dwRapid[nButton] = 0;
			m_JoyCfg[i].dwCount[nButton] = 0;
		}

		// �������W
		memset(m_lJoyAxisMin[i], 0, sizeof(m_lJoyAxisMin[i]));
		memset(m_lJoyAxisMax[i], 0, sizeof(m_lJoyAxisMax[i]));

		// �l���J�E���^
		m_dwJoyAcquire[i] = 0;

		// ���̓f�[�^
		memset(&m_JoyState[i], 0, sizeof(DIJOYSTATE));
	}
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!CComponent::Init()) {
		return FALSE;
	}

	// CRTC�擾
	ASSERT(!m_pCRTC);
	m_pCRTC = (CRTC*)::GetVM()->SearchDevice(MAKEID('C', 'R', 'T', 'C'));
	ASSERT(m_pCRTC);

	// �L�[�{�[�h�擾
	ASSERT(!m_pKeyboard);
	m_pKeyboard = (Keyboard*)::GetVM()->SearchDevice(MAKEID('K', 'E', 'Y', 'B'));
	ASSERT(m_pKeyboard);

	// �}�E�X�擾
	ASSERT(!m_pMouse);
	m_pMouse = (Mouse*)::GetVM()->SearchDevice(MAKEID('M', 'O', 'U', 'S'));
	ASSERT(m_pMouse);

	// PPI�擾
	ASSERT(!m_pPPI);
	m_pPPI = (PPI*)::GetVM()->SearchDevice(MAKEID('P', 'P', 'I', ' '));
	ASSERT(m_pPPI);

	// DirectInput�I�u�W�F�N�g���쐬
//VC2010//	if (FAILED(DirectInputCreate(AfxGetApp()->m_hInstance, DIRECTINPUT_VERSION,
//VC2010//							&m_lpDI, NULL))) {
	if (FAILED(DirectInput8Create(AfxGetApp()->m_hInstance, DIRECTINPUT_VERSION,	//VC2010//
							IID_IDirectInput8, (void**) &m_lpDI, NULL))) {			//VC2010//
		return FALSE;
	}

	// �L�[�{�[�h
//	if (!InitKey()) {
//		return FALSE;
//	}

	// �}�E�X
	if (!InitMouse()) {
		return FALSE;
	}

	// �W���C�X�e�B�b�N
	EnumJoy();
	InitJoy();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CInput::Cleanup()
{
	int i;

	ASSERT(this);
	ASSERT_VALID(this);

	// �}�E�X���[�h
	SetMouseMode(FALSE);

	// �W���C�X�e�B�b�N�f�o�C�X�����
	for (i=0; i<JoyDevices; i++) {
		if (m_lpDIDev2[i]) {
			m_lpDIDev2[i]->Release();
			m_lpDIDev2[i] = NULL;
		}

		if (m_lpDIJoy[i]) {
			m_lpDIJoy[i]->Unacquire();
			m_lpDIJoy[i]->Release();
			m_lpDIJoy[i] = NULL;
		}
	}

	// �}�E�X�f�o�C�X�����
	if (m_lpDIMouse) {
		m_lpDIMouse->Unacquire();
		m_lpDIMouse->Release();
		m_lpDIMouse = NULL;
	}

	// DirectInput�I�u�W�F�N�g�����
	if (m_lpDI) {
		m_lpDI->Release();
		m_lpDI = NULL;
	}

	// ��{�N���X
	CComponent::Cleanup();
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL CInput::ApplyCfg(const Config* pConfig)
{
	BOOL bFlag;
	int i;
	int nButton;
	int nConfig;

	ASSERT(this);
	ASSERT(pConfig);
	ASSERT_VALID(this);

	// �}�E�X���{�^��
	m_bMouseMid = 1;	//pConfig->mouse_mid;

	// �����{�^���J�E���g�𖳌���
	m_dwMouseMid = 5;

	// �W���C�X�e�B�b�N�f�o�C�X(�f�o�C�X�ύX�n)
	bFlag = FALSE;
	for (i=0; i<JoyDevices; i++) {
		// �g���f�o�C�XNo.���ς���Ă�����A�ď��������K�v
		if (pConfig->joy_dev[i] != m_JoyCfg[i].nDevice) {
			m_JoyCfg[i].nDevice = pConfig->joy_dev[i];
			bFlag = TRUE;
		}
	}
	if (bFlag) {
		// �ď�����
		InitJoy();
	}

	// �W���C�X�e�B�b�N�f�o�C�X(�{�^���n)
	for (i=0; i<JoyDevices; i++) {
		for (nButton=0; nButton<JoyButtons; nButton++) {
			// �R���t�B�O�f�[�^�擾(bit16:�|�[�g bit15-8:�A�� bit7-0:�{�^��)
			if (i == 0) {
				nConfig = pConfig->joy_button0[nButton];
			}
			else {
				nConfig = pConfig->joy_button1[nButton];
			}

			// ������
			m_JoyCfg[i].dwButton[nButton] = 0;
			m_JoyCfg[i].dwRapid[nButton] = 0;
			m_JoyCfg[i].dwCount[nButton] = 0;

			// �����蓖�Ă��`�F�b�N
			if ((nConfig & 0xff) == 0) {
				continue;
			}

			// �{�^�����������𒴂��Ă��Ȃ����`�F�b�N
			if ((nConfig & 0xff) > PPI::ButtonMax) {
				continue;
			}

			// �{�^�����蓖�Đݒ�
			m_JoyCfg[i].dwButton[nButton] = (DWORD)(nConfig & 0xff00ff);

			// �A�ːݒ�
			m_JoyCfg[i].dwRapid[nButton] = (DWORD)((nConfig >> 8) & 0xff);
			if (m_JoyCfg[i].dwRapid[nButton] > JoyRapids) {
				// �͈̓I�[�o�̏ꍇ�A�A�˂Ȃ��Ƃ���
				m_JoyCfg[i].dwRapid[nButton] = 0;
			}
		}
	}
}

#if defined(_DEBUG)
//---------------------------------------------------------------------------
//
//	�f�f
//
//---------------------------------------------------------------------------
void CInput::AssertValid() const
{
	ASSERT(this);
	ASSERT(GetID() == MAKEID('I', 'N', 'P', ' '));
	ASSERT(m_pCRTC);
	ASSERT(m_pCRTC->GetID() == MAKEID('C', 'R', 'T', 'C'));
	ASSERT(m_pKeyboard);
	ASSERT(m_pKeyboard->GetID() == MAKEID('K', 'E', 'Y', 'B'));
	ASSERT(m_pMouse);
	ASSERT(m_pMouse->GetID() == MAKEID('M', 'O', 'U', 'S'));
	ASSERT(m_pPPI);
	ASSERT(m_pPPI->GetID() == MAKEID('P', 'P', 'I', ' '));
}
#endif	// _DEBUG

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::Save(Fileio *pFio, int)
{
	BOOL bResult;

	ASSERT(this);
	ASSERT(pFio);
	ASSERT_VALID(this);

	// VM���b�N���A�ꎞ�I�ɐi�s���~�߂�
	::LockVM();

	// �Z�[�u�{��
	bResult = SaveMain(pFio);

	// VM�A�����b�N
	::UnlockVM();

	// ���ʂ������A��
	return bResult;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u�{��
//
//---------------------------------------------------------------------------
BOOL CInput::SaveMain(Fileio *pFio)
{
	ASSERT(this);
	ASSERT(pFio);
	ASSERT_VALID(this);

	//
	//	version2.00
	//

	// �S��
	if (!pFio->Write(&m_dwProcessCount, sizeof(m_dwProcessCount))) {
		return FALSE;
	}
	if (!pFio->Write(&m_dwDispCount, sizeof(m_dwDispCount))) {
		return FALSE;
	}

	// �}�E�X
	if (!pFio->Write(&m_nMouseX, sizeof(m_nMouseX))) {
		return FALSE;
	}
	if (!pFio->Write(&m_nMouseY, sizeof(m_nMouseY))) {
		return FALSE;
	}
	if (!pFio->Write(&m_dwMouseMid, sizeof(m_dwMouseMid))) {
		return FALSE;
	}

	// �W���C�X�e�B�b�N
	if (!pFio->Write(m_JoyState, sizeof(m_JoyState))) {
		return FALSE;
	}

	//
	//	version2.01
	//

	// �}�E�X
	if (!pFio->Write(m_bMouseB, sizeof(m_bMouseB))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::Load(Fileio *pFio, int nVer)
{
	ASSERT(this);
	ASSERT(pFio);
	ASSERT(nVer >= 0x0200);
	ASSERT_VALID(this);

	// VM���b�N���A�ꎞ�I�ɐi�s���~�߂�
	::LockVM();

	// ���[�h(version2.00)
	if (!Load200(pFio)) {
		::UnlockVM();
		return FALSE;
	}

	if (nVer >= 0x0201) {
		// version2.01
		if (!Load201(pFio)) {
			::UnlockVM();
			return FALSE;
		}
	}

	// VM�A�����b�N
	::UnlockVM();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h�{�� (version2.00)
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::Load200(Fileio *pFio)
{
	ASSERT(this);
	ASSERT(pFio);
	ASSERT_VALID(this);

	// �S��
	if (!pFio->Read(&m_dwProcessCount, sizeof(m_dwProcessCount))) {
		return FALSE;
	}
	if (!pFio->Read(&m_dwDispCount, sizeof(m_dwDispCount))) {
		return FALSE;
	}

	// �}�E�X
	if (!pFio->Read(&m_nMouseX, sizeof(m_nMouseX))) {
		return FALSE;
	}
	if (!pFio->Read(&m_nMouseY, sizeof(m_nMouseY))) {
		return FALSE;
	}
	if (!pFio->Read(&m_dwMouseMid, sizeof(m_dwMouseMid))) {
		return FALSE;
	}

	// �W���C�X�e�B�b�N
	if (!pFio->Read(m_JoyState, sizeof(m_JoyState))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h�{�� (version2.01)
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::Load201(Fileio *pFio)
{
	ASSERT(this);
	ASSERT(pFio);
	ASSERT_VALID(this);

	// �}�E�X
	if (!pFio->Read(m_bMouseB, sizeof(m_bMouseB))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�i�s
//
//---------------------------------------------------------------------------
void FASTCALL CInput::Process(BOOL bRun)
{
	DWORD dwDispCount;

	ASSERT(this);
	ASSERT_VALID(this);

	// �f�B�Z�[�u���Ȃ牽�����Ȃ�
	if (!m_bEnable) {
		return;
	}

	// bRun = FALSE�Ȃ�A�X�P�W���[����~��(10ms�����ɌĂ΂��)
	if (!bRun) {
		// �i�s�J�E���^Up
		m_dwProcessCount++;

		// �_�~�[����
		InputKey(FALSE);
		InputMouse(FALSE);
		InputJoy(FALSE);
		return;
	}

	// CRTC�̕\���J�E���^�����āA�t���[�����Ƃɏ�������
	ASSERT(m_pCRTC);
	dwDispCount = m_pCRTC->GetDispCount();
	if (dwDispCount == m_dwDispCount) {
		return;
	}
	m_dwDispCount = dwDispCount;

	// �i�s�J�E���^Up
	m_dwProcessCount++;

	// �A�N�e�B�u�łȂ����A�܂��̓��j���[���Ȃ�_�~�[����
	if (!m_bActive || m_bMenu) {
		// �_�~�[����
		InputKey(FALSE);
		InputMouse(FALSE);
		InputJoy(FALSE);
		MakeJoy(FALSE);
		return;
	}

	// ok�A���͏����ł���
	InputKey(TRUE);
	InputMouse(TRUE);
	InputJoy(TRUE);
	MakeJoy(m_bJoyEnable);
}

//---------------------------------------------------------------------------
//
//	�A�N�e�B�x�[�g�ʒm
//
//---------------------------------------------------------------------------
void FASTCALL CInput::Activate(BOOL bActive)
{
	ASSERT(this);
	ASSERT_VALID(this);

	// �A�N�e�B�u�t���O�ɔ��f
	m_bActive = bActive;
}

//---------------------------------------------------------------------------
//
//	���j���[�ʒm
//
//---------------------------------------------------------------------------
void FASTCALL CInput::Menu(BOOL bMenu)
{
	ASSERT(this);
	ASSERT_VALID(this);

	// ���j���[�t���O�ɔ��f
	m_bMenu = bMenu;
}

//---------------------------------------------------------------------------
//
//	�l���J�E���^�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL CInput::GetAcquireCount(int nType) const
{
	ASSERT(this);
	ASSERT(JoyDevices >= 2);
	ASSERT_VALID(this);

	switch (nType) {
		// 1:�}�E�X
		case 1:
			return m_dwMouseAcquire;

		// 2:�W���C�X�e�B�b�NA
		case 2:
			return m_dwJoyAcquire[0];

		// 3:�W���C�X�e�B�b�NB
		case 3:
			return m_dwJoyAcquire[1];

		// ���̑�
		default:
			break;
	}

	// �ʏ�A�����ɂ͂��Ȃ�
	ASSERT(FALSE);
	return 0;
}

//===========================================================================
//
//	�L�[�{�[�h
//
//===========================================================================
namespace XM6_pid {
	typedef enum X68kKeyCode {
		X68K_KEYCODE_NONE	= 0,
		X68K_KEYCODE_ESC	= 1	,	// 01 [ESC]			.			.				.
		X68K_KEYCODE_1			,	// 02 [1]			!			��				.
		X68K_KEYCODE_2			,	// 03 [2]			"			��				.
		X68K_KEYCODE_3			,	// 04 [3]			#			��				��
		X68K_KEYCODE_4			,	// 05 [4]			$			��				��
		X68K_KEYCODE_5			,	// 06 [5]			%			��				��
		X68K_KEYCODE_6			,	// 07 [6]			&			��				��
		X68K_KEYCODE_7			,	// 08 [7]			'			��				��
		X68K_KEYCODE_8			,	// 09 [8]			(			��				��
		X68K_KEYCODE_9			,	// 0A [9]			)			��				��
		X68K_KEYCODE_0			,	// 0B [0]			.			��				��
		X68K_KEYCODE_MINUS		,	// 0C [-]			=			��				.
		X68K_KEYCODE_CIRCUMFLEX	,	// 0D [^]			~			��				.
		X68K_KEYCODE_YEN		,	// 0E [��]			|			�[				.
		X68K_KEYCODE_BS			,	// 0F [BS]			.			.				.
		X68K_KEYCODE_TAB		,	// 10 [TAB]			.			.				.
		X68K_KEYCODE_Q			,	// 11 [Q]			.			��				.
		X68K_KEYCODE_W			,	// 12 [W]			.			��				.
		X68K_KEYCODE_E			,	// 13 [E]			.			��				��
		X68K_KEYCODE_R			,	// 14 [R]			.			��				.
		X68K_KEYCODE_T			,	// 15 [T]			.			��				.
		X68K_KEYCODE_Y			,	// 16 [Y]			.			��				.
		X68K_KEYCODE_U			,	// 17 [U]			.			��				.
		X68K_KEYCODE_I			,	// 18 [I]			.			��				.
		X68K_KEYCODE_O			,	// 19 [O]			.			��				.
		X68K_KEYCODE_P			,	// 1A [P]			.			��				.
		X68K_KEYCODE_AT			,	// 1B [@]			`			�J				.
		X68K_KEYCODE_LBRACKET	,	// 1C [[]			{			�K				�u
		X68K_KEYCODE_CR			,	// 1D [CR]			.			.				.
		X68K_KEYCODE_A			,	// 1E [A]			.			��				.
		X68K_KEYCODE_S			,	// 1F [S]			.			��				.
		X68K_KEYCODE_D			,	// 20 [D]			.			��				.
		X68K_KEYCODE_F			,	// 21 [F]			.			��				.
		X68K_KEYCODE_G			,	// 22 [G]			.			��				.
		X68K_KEYCODE_H			,	// 23 [H]			.			��				.
		X68K_KEYCODE_J			,	// 24 [J]			.			��				.
		X68K_KEYCODE_K			,	// 25 [K]			.			��				.
		X68K_KEYCODE_L			,	// 26 [L]			.			��				.
		X68K_KEYCODE_SEMICOLON	,	// 27 [;]			+			��				.
		X68K_KEYCODE_COLON		,	// 28 [:]			*			��				.
		X68K_KEYCODE_RBRACKET	,	// 29 []]			}			��				�v
		X68K_KEYCODE_Z			,	// 2A [Z]			.			��				��
		X68K_KEYCODE_X			,	// 2B [X]			.			��				.
		X68K_KEYCODE_C			,	// 2C [C]			.			��				.
		X68K_KEYCODE_V			,	// 2D [V]			.			��				.
		X68K_KEYCODE_B			,	// 2E [B]			.			��				.
		X68K_KEYCODE_N			,	// 2F [N]			.			��				.
		X68K_KEYCODE_M			,	// 30 [M]			.			��				.
		X68K_KEYCODE_COMMA		,	// 31 [,]			<			��				�A
		X68K_KEYCODE_PERIOD		,	// 32 [.]			>			��				�B
		X68K_KEYCODE_SLASH		,	// 33 [/]			?			��				�E
		X68K_KEYCODE_UNDERSCORE	,	// 34 .				_			��				.
		X68K_KEYCODE_SPACE		,	// 35 [SPACE]
		X68K_KEYCODE_HOME		,	// 36 [HOME]
		X68K_KEYCODE_DEL		,	// 37 [DEL]
		X68K_KEYCODE_ROLLUP 	,	// 38 [ROLL UP]
		X68K_KEYCODE_ROLLDOWN 	,	// 39 [ROLL DOWN]
		X68K_KEYCODE_UNDO		,	// 3A [UNDO]
		X68K_KEYCODE_LEFT		,	// 3B [LEFT]
		X68K_KEYCODE_UP			,	// 3C [UP]
		X68K_KEYCODE_RIGHT		,	// 3D [RIGHT]
		X68K_KEYCODE_DOWN		,	// 3E [DOWN]
		X68K_KEYCODE_TKCLR		,	// 3F [Tenkey CLR]
		X68K_KEYCODE_TKSLASH	,	// 40 [Tenkey /]
		X68K_KEYCODE_TKASTERISK	,	// 41 [Tenkey *]
		X68K_KEYCODE_TKMINUS	,	// 42 [Tenkey -]
		X68K_KEYCODE_TK7		,	// 43 [Tenkey 7]
		X68K_KEYCODE_TK8		,	// 44 [Tenkey 8]
		X68K_KEYCODE_TK9		,	// 45 [Tenkey 9]
		X68K_KEYCODE_TKPLUS		,	// 46 [Tenkey +]
		X68K_KEYCODE_TK4		,	// 47 [Tenkey 4]
		X68K_KEYCODE_TK5		,	// 48 [Tenkey 5]
		X68K_KEYCODE_TK6		,	// 49 [Tenkey 6]
		X68K_KEYCODE_TKEQUAL	,	// 4A [Tenkey =]
		X68K_KEYCODE_TK1		,	// 4B [Tenkey 1]
		X68K_KEYCODE_TK2		,	// 4C [Tenkey 2]
		X68K_KEYCODE_TK3		,	// 4D [Tenkey 3]
		X68K_KEYCODE_TKCR		,	// 4E [Tenkey CR]
		X68K_KEYCODE_TK0		,	// 4F [Tenkey 0]
		X68K_KEYCODE_TKCOMMA	,	// 50 [Tenkey ,]
		X68K_KEYCODE_TKPERIOD	,	// 51 [Tenkey .]
		X68K_KEYCODE_KIGOU		,	// 52 [�L������]
		X68K_KEYCODE_TOUROKU	,	// 53 [�o�^]
		X68K_KEYCODE_HELP		,	// 54 [HELP]
		X68K_KEYCODE_XF1		,	// 55 [XF1]
		X68K_KEYCODE_XF2		,	// 56 [XF2]
		X68K_KEYCODE_XF3		,	// 57 [XF3]
		X68K_KEYCODE_XF4		,	// 58 [XF4]
		X68K_KEYCODE_XF5		,	// 59 [XF5]
		X68K_KEYCODE_KANA		,	// 5A [����]
		X68K_KEYCODE_ROMA		,	// 5B [���[�}��]
		X68K_KEYCODE_CODE		,	// 5C [�R�[�h����]
		X68K_KEYCODE_CAPS		,	// 5D [CAPS]
		X68K_KEYCODE_INS		,	// 5E [INS]
		X68K_KEYCODE_HIRAGANA	,	// 5F [�Ђ炪��]
		X68K_KEYCODE_ZENKAKU	,	// 60 [�S�p]
		X68K_KEYCODE_BREAK		,	// 61 [BREAK]
		X68K_KEYCODE_COPY		,	// 62 [COPY]
		X68K_KEYCODE_F1			,	// 63 [F1]
		X68K_KEYCODE_F2			,	// 64 [F2]
		X68K_KEYCODE_F3			,	// 65 [F3]
		X68K_KEYCODE_F4			,	// 66 [F4]
		X68K_KEYCODE_F5			,	// 67 [F5]
		X68K_KEYCODE_F6			,	// 68 [F6]
		X68K_KEYCODE_F7			,	// 69 [F7]
		X68K_KEYCODE_F8			,	// 6A [F8]
		X68K_KEYCODE_F9			,	// 6B [F9]
		X68K_KEYCODE_F10		,	// 6C [F10]
		X68K_KEYCODE_x6d		,	// 6D (Reserved)
		X68K_KEYCODE_x6e		,	// 6E (Reserved)
		X68K_KEYCODE_x6f		,	// 6F (Reserved)
		X68K_KEYCODE_SHIFT		,	// 70 [SHIFT]
		X68K_KEYCODE_CTRL		,	// 71 [CTRL]
		X68K_KEYCODE_OPT1		,	// 72 [OPT1]
		X68K_KEYCODE_OPT2		,	// 73 [OPT2]
		X68K_KEYCODE_MAX		,
		X68K_KEYCODE_FORCE_32BIT	= 0x7fffffff
	} X68kKeycode;

	struct KeyEntry {
		int			targetKeycode;
		X68kKeyCode	x68kKeycode;
	};

	struct KeyMap {
		enum {
			KEY_ENTRY_MAX	= 256,
		};

		int			nKeyEntry;
		KeyEntry	keyEntry[KEY_ENTRY_MAX];
	};
};

using namespace XM6_pid;

//	Win32 Virtual-Key Codes
//	http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx
//
//		keymap for US standard keyboard
//
static const KeyEntry keyEntry[] = {
	{ VK_ESCAPE					,	X68K_KEYCODE_ESC		},	// 01 [ESC]			.			.				.
	{ '1'						,	X68K_KEYCODE_1			},	// 02 [1]			!			��				.
	{ '2'						,	X68K_KEYCODE_2			},	// 03 [2]			"			��				.
	{ '3'						,	X68K_KEYCODE_3			},	// 04 [3]			#			��				��
	{ '4'						,	X68K_KEYCODE_4			},	// 05 [4]			$			��				��
	{ '5'						,	X68K_KEYCODE_5			},	// 06 [5]			%			��				��
	{ '6'						,	X68K_KEYCODE_6			},	// 07 [6]			&			��				��
	{ '7'						,	X68K_KEYCODE_7			},	// 08 [7]			'			��				��
	{ '8'						,	X68K_KEYCODE_8			},	// 09 [8]			(			��				��
	{ '9'						,	X68K_KEYCODE_9			},	// 0A [9]			)			��				��
	{ '0'						,	X68K_KEYCODE_0			},	// 0B [0]			.			��				��
	{ VK_OEM_MINUS				,	X68K_KEYCODE_MINUS		},	// 0C [-]			=			��				.
	{ VK_OEM_PLUS				,	X68K_KEYCODE_CIRCUMFLEX	},	// 0D [^]			~			��				.
	{ VK_OEM_5					,	X68K_KEYCODE_YEN		},	// 0E [��]			|			�[				.
	{ VK_BACK					,	X68K_KEYCODE_BS			},	// 0F [BS]			.			.				.
	{ VK_TAB					,	X68K_KEYCODE_TAB		},	// 10 [TAB]			.			.				.
	{ 'Q'						,	X68K_KEYCODE_Q			},	// 11 [Q]			.			��				.
	{ 'W'						,	X68K_KEYCODE_W			},	// 12 [W]			.			��				.
	{ 'E'						,	X68K_KEYCODE_E			},	// 13 [E]			.			��				��
	{ 'R'						,	X68K_KEYCODE_R			},	// 14 [R]			.			��				.
	{ 'T'						,	X68K_KEYCODE_T			},	// 15 [T]			.			��				.
	{ 'Y'						,	X68K_KEYCODE_Y			},	// 16 [Y]			.			��				.
	{ 'U'						,	X68K_KEYCODE_U			},	// 17 [U]			.			��				.
	{ 'I'						,	X68K_KEYCODE_I			},	// 18 [I]			.			��				.
	{ 'O'						,	X68K_KEYCODE_O			},	// 19 [O]			.			��				.
	{ 'P'						,	X68K_KEYCODE_P			},	// 1A [P]			.			��				.
	{ 0000						,	X68K_KEYCODE_AT			},	// 1B [@]			`			�J				.
	{ VK_OEM_4					,	X68K_KEYCODE_LBRACKET	},	// 1C [[]			{			�K				�u
	{ VK_RETURN					,	X68K_KEYCODE_CR			},	// 1D [CR]			.			.				.
	{ 'A'						,	X68K_KEYCODE_A			},	// 1E [A]			.			��				.
	{ 'S'						,	X68K_KEYCODE_S			},	// 1F [S]			.			��				.
	{ 'D'						,	X68K_KEYCODE_D			},	// 20 [D]			.			��				.
	{ 'F'						,	X68K_KEYCODE_F			},	// 21 [F]			.			��				.
	{ 'G'						,	X68K_KEYCODE_G			},	// 22 [G]			.			��				.
	{ 'H'						,	X68K_KEYCODE_H			},	// 23 [H]			.			��				.
	{ 'J'						,	X68K_KEYCODE_J			},	// 24 [J]			.			��				.
	{ 'K'						,	X68K_KEYCODE_K			},	// 25 [K]			.			��				.
	{ 'L'						,	X68K_KEYCODE_L			},	// 26 [L]			.			��				.
	{ VK_OEM_1					,	X68K_KEYCODE_SEMICOLON	},	// 27 [;]			+			��				.
	{ 0000						,	X68K_KEYCODE_COLON		},	// 28 [:]			*			��				.
	{ VK_OEM_6					,	X68K_KEYCODE_RBRACKET	},	// 29 []]			}			��				�v
	{ 'Z'						,	X68K_KEYCODE_Z			},	// 2A [Z]			.			��				��
	{ 'X'						,	X68K_KEYCODE_X			},	// 2B [X]			.			��				.
	{ 'C'						,	X68K_KEYCODE_C			},	// 2C [C]			.			��				.
	{ 'V'						,	X68K_KEYCODE_V			},	// 2D [V]			.			��				.
	{ 'B'						,	X68K_KEYCODE_B			},	// 2E [B]			.			��				.
	{ 'N'						,	X68K_KEYCODE_N			},	// 2F [N]			.			��				.
	{ 'M'						,	X68K_KEYCODE_M			},	// 30 [M]			.			��				.
	{ VK_OEM_COMMA				,	X68K_KEYCODE_COMMA		},	// 31 [,]			<			��				�A
	{ VK_OEM_PERIOD				,	X68K_KEYCODE_PERIOD		},	// 32 [.]			>			��				�B
	{ VK_OEM_2					,	X68K_KEYCODE_SLASH		},	// 33 [/]			?			��				�E
	{ 0000						,	X68K_KEYCODE_UNDERSCORE	},	// 34 .				_			��				.
	{ VK_SPACE					,	X68K_KEYCODE_SPACE		},	// 35 [SPACE]
	{ VK_HOME					,	X68K_KEYCODE_HOME		},	// 36 [HOME]
	{ VK_DELETE					,	X68K_KEYCODE_DEL		},	// 37 [DEL]
	{ VK_PRIOR					,	X68K_KEYCODE_ROLLUP 	},	// 38 [ROLL UP]
	{ VK_NEXT					,	X68K_KEYCODE_ROLLDOWN 	},	// 39 [ROLL DOWN]
	{ 0000						,	X68K_KEYCODE_UNDO		},	// 3A [UNDO]
	{ VK_LEFT					,	X68K_KEYCODE_LEFT		},	// 3B [LEFT]
	{ VK_UP						,	X68K_KEYCODE_UP			},	// 3C [UP]
	{ VK_RIGHT					,	X68K_KEYCODE_RIGHT		},	// 3D [RIGHT]
	{ VK_DOWN					,	X68K_KEYCODE_DOWN		},	// 3E [DOWN]
	{ VK_NUMLOCK				,	X68K_KEYCODE_TKCLR		},	// 3F [Tenkey CLR]
	{ VK_DIVIDE					,	X68K_KEYCODE_TKSLASH	},	// 40 [Tenkey /]
	{ VK_MULTIPLY				,	X68K_KEYCODE_TKASTERISK	},	// 41 [Tenkey *]
	{ VK_SUBTRACT				,	X68K_KEYCODE_TKMINUS	},	// 42 [Tenkey -]
	{ VK_NUMPAD7				,	X68K_KEYCODE_TK7		},	// 43 [Tenkey 7]
	{ VK_NUMPAD8				,	X68K_KEYCODE_TK8		},	// 44 [Tenkey 8]
	{ VK_NUMPAD9				,	X68K_KEYCODE_TK9		},	// 45 [Tenkey 9]
	{ VK_ADD					,	X68K_KEYCODE_TKPLUS		},	// 46 [Tenkey +]
	{ VK_NUMPAD4				,	X68K_KEYCODE_TK4		},	// 47 [Tenkey 4]
	{ VK_NUMPAD5				,	X68K_KEYCODE_TK5		},	// 48 [Tenkey 5]
	{ VK_NUMPAD6				,	X68K_KEYCODE_TK6		},	// 49 [Tenkey 6]
	{ 0000						,	X68K_KEYCODE_TKEQUAL	},	// 4A [Tenkey =]
	{ VK_NUMPAD1				,	X68K_KEYCODE_TK1		},	// 4B [Tenkey 1]
	{ VK_NUMPAD2				,	X68K_KEYCODE_TK2		},	// 4C [Tenkey 2]
	{ VK_NUMPAD3				,	X68K_KEYCODE_TK3		},	// 4D [Tenkey 3]
	{ 0000						,	X68K_KEYCODE_TKCR		},	// 4E [Tenkey CR]
	{ VK_NUMPAD0				,	X68K_KEYCODE_TK0		},	// 4F [Tenkey 0]
	{ 0000						,	X68K_KEYCODE_TKCOMMA	},	// 50 [Tenkey ,]
	{ VK_DECIMAL				,	X68K_KEYCODE_TKPERIOD	},	// 51 [Tenkey .]
	{ 0000						,	X68K_KEYCODE_KIGOU		},	// 52 [�L������]
	{ 0000						,	X68K_KEYCODE_TOUROKU	},	// 53 [�o�^]
	{ 0000						,	X68K_KEYCODE_HELP		},	// 54 [HELP]
	{ 0000						,	X68K_KEYCODE_XF1		},	// 55 [XF1]
	{ 0000						,	X68K_KEYCODE_XF2		},	// 56 [XF2]
	{ 0000						,	X68K_KEYCODE_XF3		},	// 57 [XF3]
	{ 0000						,	X68K_KEYCODE_XF4		},	// 58 [XF4]
	{ 0000						,	X68K_KEYCODE_XF5		},	// 59 [XF5]
	{ 0000						,	X68K_KEYCODE_KANA		},	// 5A [����]
	{ 0000						,	X68K_KEYCODE_ROMA		},	// 5B [���[�}��]
	{ 0000						,	X68K_KEYCODE_CODE		},	// 5C [�R�[�h����]
	{ 0000						,	X68K_KEYCODE_CAPS		},	// 5D [CAPS]
	{ VK_INSERT					,	X68K_KEYCODE_INS		},	// 5E [INS]
	{ 0000						,	X68K_KEYCODE_HIRAGANA	},	// 5F [�Ђ炪��]
	{ 0000						,	X68K_KEYCODE_ZENKAKU	},	// 60 [�S�p]
	{ 0000						,	X68K_KEYCODE_BREAK		},	// 61 [BREAK]
	{ 0000						,	X68K_KEYCODE_COPY		},	// 62 [COPY]
	{ VK_F1						,	X68K_KEYCODE_F1			},	// 63 [F1]
	{ VK_F2						,	X68K_KEYCODE_F2			},	// 64 [F2]
	{ VK_F3						,	X68K_KEYCODE_F3			},	// 65 [F3]
	{ VK_F4						,	X68K_KEYCODE_F4			},	// 66 [F4]
	{ VK_F5						,	X68K_KEYCODE_F5			},	// 67 [F5]
	{ VK_F6						,	X68K_KEYCODE_F6			},	// 68 [F6]
	{ VK_F7						,	X68K_KEYCODE_F7			},	// 69 [F7]
	{ VK_F8						,	X68K_KEYCODE_F8			},	// 6A [F8]
	{ VK_F9						,	X68K_KEYCODE_F9			},	// 6B [F9]
	{ VK_F10					,	X68K_KEYCODE_F10		},	// 6C [F10]
	{ -1						,	X68K_KEYCODE_x6d		},	// 6D (Reserved)
	{ -1						,	X68K_KEYCODE_x6e		},	// 6E (Reserved)
	{ -1						,	X68K_KEYCODE_x6f		},	// 6F (Reserved)
	{ VK_SHIFT					,	X68K_KEYCODE_SHIFT		},	// 70 [SHIFT]
	{ VK_CONTROL				,	X68K_KEYCODE_CTRL		},	// 71 [CTRL]
	{ 0000						,	X68K_KEYCODE_OPT1		},	// 72 [OPT1]
	{ 0000						,	X68K_KEYCODE_OPT2		},	// 73 [OPT2]
};

static XM6_pid::X68kKeycode getX68kKeycodeByTargetKeycode(int targetKeycode) {
	XM6_pid::X68kKeycode ret = X68K_KEYCODE_NONE;

	if(targetKeycode > 0) {
		for(int i = 0, n = sizeof(keyEntry)/sizeof(keyEntry[0]); i < n; ++i) {
			const KeyEntry& e = keyEntry[i];
			if(e.targetKeycode == targetKeycode) {
				ret = e.x68kKeycode;
				break;
			}
		}
	}

	return ret;
}

class KeyMapTargetToX68k {
public:
	KeyMapTargetToX68k() {
		clear();
	}

	~KeyMapTargetToX68k() {
	}

	void clear() {
		memset(&x68k_to_target[0], 0, sizeof(x68k_to_target));
		memset(&target_to_x68k[0], 0, sizeof(target_to_x68k));
	}

	void set(int targetKeycode, X68kKeyCode x68kKeyCode) {
		x68k_to_target[x68kKeyCode]		= targetKeycode;
		target_to_x68k[targetKeycode]	= x68kKeyCode;
	}

	X68kKeyCode getX68kKeycodeByTargetKeycode(int targetKeycode) const {
		X68kKeyCode ret = X68K_KEYCODE_NONE;
		if(targetKeycode >= 0 && targetKeycode < 256) {
			ret = target_to_x68k[targetKeycode];
		}
		return ret;
	}

	int getTargetKeycodeByX68kKeycode(X68kKeyCode x68kKeyCode) const {
		int ret = 0;
		if(x68kKeyCode > X68K_KEYCODE_NONE && x68kKeyCode < X68K_KEYCODE_MAX) {
			ret = x68k_to_target[x68kKeyCode];
		}
		return ret;
	}

protected:
	int			x68k_to_target[256];
	X68kKeyCode	target_to_x68k[256];
};



//---------------------------------------------------------------------------
//
//	�L�[�{�[�h����
//
//---------------------------------------------------------------------------
void FASTCALL CInput::InputKey(BOOL bEnable)
{
	if(m_pKeyboard) {
		//
		//	keymapping
		//
		static KeyMapTargetToX68k km;
		if(km.getTargetKeycodeByX68kKeycode(X68K_KEYCODE_ESC) == 0) {
			for(int i = 0, n = sizeof(keyEntry)/sizeof(keyEntry[0]); i < n; ++i) {
				const KeyEntry& e = keyEntry[i];
				if(e.targetKeycode && e.x68kKeycode) {
					km.set(e.targetKeycode, e.x68kKeycode);
				}
			}
		}

		static BYTE prevKeys[256];
		BYTE keys[256];
		for(int i = 0; i < 256; ++i) {
			BYTE k = 0;
			if(GetAsyncKeyState(i) & 0x8000) {
				k = 1;
			}
			keys[i] = k;
		}
		for(int i = 0, n = sizeof(keys)/sizeof(keys[0]); i < n; ++i) {
			if((keys[i] ^ prevKeys[i]) & 0x01) {
				X68kKeyCode	x68kKeyCode	= km.getX68kKeycodeByTargetKeycode(i);
				if(x68kKeyCode != X68K_KEYCODE_NONE) {
					bool		isPressed	= ((keys[i] & 0x01) != 0);
					if(isPressed) {
						//	release -> press
						m_pKeyboard->MakeKey(x68kKeyCode);
					} else {
						//	press -> release
						m_pKeyboard->BreakKey(x68kKeyCode);
					}
				}
			}
		}
		memcpy(&prevKeys[0], &keys[0], sizeof(prevKeys));
	}
}



//===========================================================================
//
//	�}�E�X
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�}�E�X������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::InitMouse()
{
	ASSERT(this);
	ASSERT(m_lpDI);
	ASSERT(!m_lpDIMouse);
	ASSERT_VALID(this);

	// �}�E�X�f�o�C�X���쐬
	if (FAILED(m_lpDI->CreateDevice(GUID_SysMouse, &m_lpDIMouse, NULL))) {
		return FALSE;
	}

	// �}�E�X�f�[�^�`����ݒ�
	if (FAILED(m_lpDIMouse->SetDataFormat(&c_dfDIMouse))) {
		return FALSE;
	}

	// �������x����ݒ�(Win9x/WinNT�ŋ������قȂ邽�߁A������)
	if (::IsWinNT()) {
		// WindowsNT
		if (FAILED(m_lpDIMouse->SetCooperativeLevel(m_pFrmWnd->m_hWnd,
						DISCL_BACKGROUND | DISCL_NONEXCLUSIVE))) {
			return FALSE;
		}
	}
	else {
		// Windows9x
		if (FAILED(m_lpDIMouse->SetCooperativeLevel(m_pFrmWnd->m_hWnd,
						DISCL_FOREGROUND | DISCL_EXCLUSIVE))) {
			return FALSE;
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�}�E�X����
//
//---------------------------------------------------------------------------
void FASTCALL CInput::InputMouse(BOOL bEnable)
{
	HRESULT hr;
	DIMOUSESTATE dims;

	ASSERT(this);
	ASSERT(m_pFrmWnd);
	ASSERT(m_lpDIMouse);
	ASSERT(m_pMouse);
	ASSERT_VALID(this);

	// �������Ă悢��
	if (!bEnable) {
		// �}�E�X���[�hOFF
		if (m_bMouseMode) {
			m_pFrmWnd->PostMessage(WM_COMMAND, IDM_MOUSEMODE, 0);
		}

		// �{�^��UP��ʒm
		m_pMouse->SetMouse(m_nMouseX, m_nMouseY, FALSE, FALSE);
		return;
	}

	// �}�E�X���[�hON��
	if (!m_bMouseMode) {
		// �{�^��UP��ʒm
		m_pMouse->SetMouse(m_nMouseX, m_nMouseY, FALSE, FALSE);
		return;
	}

	// �f�o�C�X��Ԃ��擾
	hr = m_lpDIMouse->GetDeviceState(sizeof(dims), &dims);
	if (hr != DI_OK) {
		// Acquire�����݂�
		m_lpDIMouse->Acquire();
		m_dwMouseAcquire++;

		// �}�E�X���Z�b�g
		m_pMouse->ResetMouse();
		return;
	}

	// �f�[�^����������m��
	m_nMouseX += dims.lX;
	m_nMouseY += dims.lY;
	if (dims.rgbButtons[0] & 0x80) {
		m_bMouseB[0] = TRUE;
	}
	else {
		m_bMouseB[0] = FALSE;
	}
	if (dims.rgbButtons[1] & 0x80) {
		m_bMouseB[1] = TRUE;
	}
	else {
		m_bMouseB[1] = FALSE;
	}

	// �}�E�X�f�o�C�X�֒ʒm
	m_pMouse->SetMouse(m_nMouseX, m_nMouseY, m_bMouseB[0], m_bMouseB[1]);

	// �����{�^���@�\���֎~����Ă���΁A�I��
	if (!m_bMouseMid) {
		m_dwMouseMid = 5;
		return;
	}

	// �����{�^�����`�F�b�N�B�A�����ĉ����ė����ꂽ��}�E�X���[�hoff
	if (dims.rgbButtons[2] & 0x80) {
		// ������Ă���
		if (m_dwMouseMid < 4) {
			// ���Z�b�g��Ԃ���
			m_dwMouseMid++;
			if (m_dwMouseMid == 4) {
				// �\��������Â��Ă���̂ŁA�z�[���h
				m_dwMouseMid = 3;
			}
		}
	}
	else {
		// ������Ă���
		if ((m_dwMouseMid == 3) || (m_dwMouseMid == 4)) {
			// �\�������ꂽ�ォ�A���̌�̗������P�񌟏o������Ɍ���
			m_dwMouseMid++;
			if (m_dwMouseMid == 5) {
				// 3�t���[���ȏ㉟����āA���̌�A2�t���[���ȏ㗣���ꂽ
				m_pFrmWnd->PostMessage(WM_COMMAND, IDM_MOUSEMODE, 0);
				m_dwMouseMid++;
			}
		}
		else {
			// �\��������Ă��Ȃ��܂ܗ����ꂽ�B���Z�b�g
			m_dwMouseMid = 0;
		}
	}
}

//---------------------------------------------------------------------------
//
//	�}�E�X���[�h�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL CInput::SetMouseMode(BOOL bMode)
{
	ASSERT(this);
	ASSERT_VALID(this);

	// ���݂̃��[�h�ƈ���Ă����
	if (m_bMouseMode != bMode) {
		// ���[�h���m��
		m_bMouseMode = bMode;

		// �Ƃɂ���Unacquire
		if (m_lpDIMouse) {
			m_lpDIMouse->Unacquire();
		}

		// �����{�^���J�E���g�𖳌���
		m_dwMouseMid = 5;
	}
}

//---------------------------------------------------------------------------
//
//	�}�E�X���擾
//
//---------------------------------------------------------------------------
void FASTCALL CInput::GetMouseInfo(int *pPos, BOOL *pBtn) const
{
	ASSERT(this);
	ASSERT(pPos);
	ASSERT(pBtn);
	ASSERT_VALID(this);

	// ���ꂼ��3�v�f
	pPos[0] = m_nMouseX;
	pPos[1] = m_nMouseY;
	pPos[2] = (int)m_dwMouseMid;

	// �{�^��
	pBtn[0] = m_bMouseB[0];
	pBtn[1] = m_bMouseB[1];
	pBtn[2] = m_bMouseMid;
}

//===========================================================================
//
//	�W���C�X�e�B�b�N
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�W���C�X�e�B�b�N��
//
//---------------------------------------------------------------------------
void FASTCALL CInput::EnumJoy()
{
	ASSERT(this);
	ASSERT(m_lpDI);

	// �W���C�X�e�B�b�N�����N���A
	m_dwJoyDevs = 0;

	// �񋓊J�n
//VC2010//	m_lpDI->EnumDevices(DIDEVTYPE_JOYSTICK, (LPDIENUMDEVICESCALLBACK)EnumCb,
//VC2010//							this, DIEDFL_ATTACHEDONLY);
	m_lpDI->EnumDevices(DI8DEVTYPE_JOYSTICK, (LPDIENUMDEVICESCALLBACK)EnumCb,	//VC2010//
							this, DIEDFL_ATTACHEDONLY);							//VC2010//
}

//---------------------------------------------------------------------------
//
//	�W���C�X�e�B�b�N�R�[���o�b�N
//
//---------------------------------------------------------------------------
BOOL CALLBACK CInput::EnumCb(LPDIDEVICEINSTANCE pDevInst, LPVOID pvRef)
{
	CInput *pInput;

	ASSERT(pDevInst);
	ASSERT(pvRef);

	// CInput�ɕϊ�
	pInput = (CInput*)pvRef;

	// �Ăяo��
	return pInput->EnumDev(pDevInst);
}

//---------------------------------------------------------------------------
//
//	�W���C�X�e�B�b�N�ǉ�
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::EnumDev(LPDIDEVICEINSTANCE pDevInst)
{
	LPDIRECTINPUTDEVICE pInputDev;

	ASSERT(this);
	ASSERT(pDevInst);
	ASSERT(m_lpDI);

	// �ő吔�`�F�b�N�B�ő�JoyDeviceMax�f�o�C�X�̂݃T�|�[�g����
	if (m_dwJoyDevs >= JoyDeviceMax) {
		ASSERT(m_dwJoyDevs == JoyDeviceMax);
		return DIENUM_STOP;
	}

	// �C���X�^���X���m��
	memcpy(&m_JoyDevInst[m_dwJoyDevs], pDevInst, sizeof(DIDEVICEINSTANCE));

	// �f�o�C�X�쐬
	pInputDev = NULL;
	if (FAILED(m_lpDI->CreateDevice(pDevInst->guidInstance,
									&pInputDev,
									NULL))) {
		return DIENUM_CONTINUE;
	}
	ASSERT(pInputDev);

	// �f�[�^�t�H�[�}�b�g�w��
	if (FAILED(pInputDev->SetDataFormat(&c_dfDIJoystick))) {
		// �f�o�C�X���
		pInputDev->Unacquire();
		pInputDev->Release();
		return DIENUM_CONTINUE;
	}

	// Caps�擾
	memset(&m_JoyDevCaps[m_dwJoyDevs], 0, sizeof(DIDEVCAPS));
	m_JoyDevCaps[m_dwJoyDevs].dwSize = sizeof(DIDEVCAPS);
	if (FAILED(pInputDev->GetCapabilities(&m_JoyDevCaps[m_dwJoyDevs]))) {
		// �f�o�C�X���
		pInputDev->Unacquire();
		pInputDev->Release();
		return DIENUM_CONTINUE;
	}

	// �f�o�C�X����U���
	pInputDev->Unacquire();
	pInputDev->Release();

	// �ǉ��ƌp��
	m_dwJoyDevs++;
	return DIENUM_CONTINUE;
}

//---------------------------------------------------------------------------
//
//	�W���C�X�e�B�b�N������
//	��ApplyCfg����Ăяo���ꍇ�AdwDevice������Ă����ꍇ�݂̂ɂ��邱��
//
//---------------------------------------------------------------------------
void FASTCALL CInput::InitJoy()
{
	int i;
	int nDevice;
	int nAxis;
	BOOL bError[JoyDevices];
	DIPROPDWORD dpd;
	DIPROPRANGE dpr;

	ASSERT(this);
	ASSERT(m_lpDI);

	// �f�o�C�X����U���
	for (i=0; i<JoyDevices; i++) {
		if (m_lpDIDev2[i]) {
			m_lpDIDev2[i]->Release();
			m_lpDIDev2[i] = NULL;
		}

		if (m_lpDIJoy[i]) {
			m_lpDIJoy[i]->Unacquire();
			m_lpDIJoy[i]->Release();
			m_lpDIJoy[i] = NULL;
		}
	}

	// ���̓f�[�^���N���A
	for (i=0; i<JoyDevices; i++) {
		memset(&m_JoyState[i], 0, sizeof(DIJOYSTATE));
	}

	// ���������[�v
	for (i=0; i<JoyDevices; i++) {
		// �G���[�t���OOFF
		bError[i] = FALSE;

		// ���g�p�Ȃ�A�������Ȃ�
		if (m_JoyCfg[i].nDevice == 0) {
			continue;
		}

		// �f�o�C�X�쐬
		nDevice = m_JoyCfg[i].nDevice - 1;
		if (FAILED(m_lpDI->CreateDevice(m_JoyDevInst[nDevice].guidInstance,
										&m_lpDIJoy[i],
										NULL))) {
			continue;
		}

		// �G���[�t���OON
		bError[i] = TRUE;

		// �f�[�^�t�H�[�}�b�g�w��
		if (FAILED(m_lpDIJoy[i]->SetDataFormat(&c_dfDIJoystick))) {
			continue;
		}

		// �������x���ݒ�
		if (FAILED(m_lpDIJoy[i]->SetCooperativeLevel(m_pFrmWnd->m_hWnd,
							DISCL_BACKGROUND | DISCL_NONEXCLUSIVE))) {
			continue;
		}

		// �l���[�h�ݒ�(��Βl)
		memset(&dpd, 0, sizeof(dpd));
		dpd.diph.dwSize = sizeof(DIPROPDWORD); 
		dpd.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		dpd.diph.dwHow = DIPH_DEVICE;
		dpd.dwData = DIPROPAXISMODE_ABS;
		if (FAILED(m_lpDIJoy[i]->SetProperty(DIPROP_AXISMODE, (LPDIPROPHEADER)&dpd))) {
			continue;
		}

		// �f�b�h�]�[���w��(�f�b�h�]�[���Ȃ�)
		memset(&dpd, 0, sizeof(dpd));
		dpd.diph.dwSize = sizeof(DIPROPDWORD);
		dpd.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		dpd.diph.dwHow = DIPH_DEVICE;
		dpd.dwData = 0;
		if (FAILED(m_lpDIJoy[i]->SetProperty(DIPROP_DEADZONE, (LPDIPROPHEADER)&dpd))) {
			continue;
		}

		// �����Ƃ̃����W���擾(���ׂĂ̎��ɂ��Ď擾�����݂�)
		for (nAxis=0; nAxis<JoyAxes; nAxis++) {
			// �擾(�G���[�ł��悢)
			memset(&dpr, 0, sizeof(dpr));
			dpr.diph.dwSize = sizeof(DIPROPRANGE);
			dpr.diph.dwHeaderSize = sizeof(DIPROPHEADER);
			dpr.diph.dwHow = DIPH_BYOFFSET;
			dpr.diph.dwObj = JoyAxisOffsetTable[nAxis];
			m_lpDIJoy[i]->GetProperty(DIPROP_RANGE, (LPDIPROPHEADER)&dpr);

			// �m��
			m_lJoyAxisMin[i][nAxis] = dpr.lMin;
			m_lJoyAxisMax[i][nAxis] = dpr.lMax;
		}

		// IDirectInputDevice2���擾
		if (FAILED(m_lpDIJoy[i]->QueryInterface(IID_IDirectInputDevice2,
										(LPVOID*)&m_lpDIDev2[i]))) {
			// IDirectInputDevice2���擾�ł��Ȃ��ꍇ
			m_lpDIDev2[i] = NULL;
		}

		// �G���[�t���OOFF(����)
		bError[i] = FALSE;
	}

	// �G���[���N�����f�o�C�X�ɂ��āA���
	for (i=0; i<JoyDevices; i++) {
		if (bError[i]) {
			ASSERT(m_lpDIJoy[i]);
			m_lpDIJoy[i]->Unacquire();
			m_lpDIJoy[i]->Release();
			m_lpDIJoy[i] = NULL;
		}
	}
}

//---------------------------------------------------------------------------
//
//	�W���C�X�e�B�b�N����
//
//---------------------------------------------------------------------------
void FASTCALL CInput::InputJoy(BOOL bEnable)
{
	int i;
	int nAxis;
	BYTE *pOffset;
	LONG *pAxis;
	LONG lRange;
	LONG lAxis;

	ASSERT(this);

	// bEnable=FALSE�ŁA����m_bJoyEnable=TRUE(VM��)�̏ꍇ�A���̓f�[�^�N���A
	if (!bEnable && m_bJoyEnable) {
		for (i=0; i<JoyDevices; i++) {
			// ���̓f�[�^�N���A
			memset(&m_JoyState[i], 0, sizeof(DIJOYSTATE));
		}
		return;
	}

	// �f�o�C�X���[�v
	for (i=0; i<JoyDevices; i++) {
		// �f�o�C�X���Ȃ���΃X�L�b�v
		if (!m_lpDIJoy[i]) {
			continue;
		}

		// �|�[�����O
		if (m_lpDIDev2[i]) {
			if (FAILED(m_lpDIDev2[i]->Poll())) {
				// Acquire�����݂�
				m_lpDIJoy[i]->Acquire();
				m_dwJoyAcquire[i]++;
				continue;
			}
		}

		// �f�[�^�擾
		if (FAILED(m_lpDIJoy[i]->GetDeviceState(sizeof(DIJOYSTATE),
												&m_JoyState[i]))) {
			// Acquire�����݂�
			m_lpDIJoy[i]->Acquire();
			m_dwJoyAcquire[i]++;
			continue;
		}

		// ���ϊ�(-800 �` 7FF�ɕϊ�)
		for (nAxis=0; nAxis<JoyAxes; nAxis++) {
			// �|�C���^�擾
			pOffset = (BYTE*)&m_JoyState[i];
			pOffset += JoyAxisOffsetTable[nAxis];
			pAxis = (LONG*)pOffset;

			// �����Ȏ��Ȃ�X�L�b�v
			if (m_lJoyAxisMin[i][nAxis] == m_lJoyAxisMax[i][nAxis]) {
				continue;
			}

			// -lMin������グ(�[�_��0�ɑ�����)
			lAxis = *pAxis - m_lJoyAxisMin[i][nAxis];

			// �͈͂��o���A�I�[�o�[�t���[�h�~�ϊ�
			lRange = m_lJoyAxisMax[i][nAxis] - m_lJoyAxisMin[i][nAxis] + 1;
			if (lRange >= 0x100000) {
				lRange >>= 12;
				lAxis >>= 12;
			}

			// �ϊ�
			lAxis <<= 12;
			lAxis /= lRange;
			lAxis -= 0x800;
			*pAxis = lAxis;
		}
	}
}

//---------------------------------------------------------------------------
//
//	�W���C�X�e�B�b�N����
//
//---------------------------------------------------------------------------
void FASTCALL CInput::MakeJoy(BOOL bEnable)
{
	int i;
	int nAxis;
	int nButton;
	BYTE *pOffset;
	LONG *pAxis;
	LONG lAxis;
	PPI::joyinfo_t ji[PPI::PortMax];

	int dmy;

	ASSERT(this);
	ASSERT(m_pPPI);

	// �S�ăN���A
	memset(ji, 0, sizeof(ji));

	// �f�B�Z�[�u���Ȃ�
	if (!bEnable) {
		for (i=0; i<PPI::PortMax; i++) {
			// ���͂Ȃ��f�[�^�𑗐M
			m_pPPI->SetJoyInfo(i, &ji[i]);

			// �{�^���A�˂��N���A
			for (nButton=0; nButton<JoyButtons; nButton++) {
				m_JoyCfg[i].dwCount[nButton] = 0;
			}
		}
		return;
	}

	// ���̓f�[�^������
	for (i=0; i<JoyDevices; i++) {
		dmy = -1;

		// ��
		for (nAxis=0; nAxis<JoyAxes; nAxis++) {
#if 0
			// ������
			if (LOWORD(m_JoyCfg[i].dwAxis[nAxis]) == 0) {
				continue;
			}
#else
			// �����Ȏ��Ȃ�X�L�b�v
			if (m_lJoyAxisMin[i][nAxis] == m_lJoyAxisMax[i][nAxis]) {
				continue;
			}
			dmy++;
			if (dmy >= 4) {
				break;
			}
#endif

			// �|�C���^�擾
			pOffset = (BYTE*)&m_JoyState[i];
			pOffset += JoyAxisOffsetTable[nAxis];
			pAxis = (LONG*)pOffset;

			// �f�[�^�擾
			lAxis = *pAxis;

			// �[���͖���(�ŏ��ɃN���A���Ă��邽��)
			if (lAxis == 0) {
				continue;
			}

			// ���]
			if (m_JoyCfg[i].bAxis[nAxis]) {
				// 7FF��-800 -800��7FF
				lAxis = -1 - lAxis;
			}

#if 0
			// �ړI�ʒu�Ɋi�[
			ASSERT(HIWORD(m_JoyCfg[i].dwAxis[nAxis]) >= 0);
			ASSERT(HIWORD(m_JoyCfg[i].dwAxis[nAxis]) < 2);
			ASSERT(LOWORD(m_JoyCfg[i].dwAxis[nAxis]) > 0);
			ASSERT(LOWORD(m_JoyCfg[i].dwAxis[nAxis]) <= 4);
			ji[HIWORD(m_JoyCfg[i].dwAxis[nAxis])].axis[LOWORD(m_JoyCfg[i].dwAxis[nAxis]) - 1]
				 = (DWORD)lAxis;
#else
			ji[HIWORD(m_JoyCfg[i].dwAxis[nAxis])].axis[dmy] = (DWORD)lAxis;
#endif
		}

		// �{�^��
		for (nButton=0; nButton<JoyButtons; nButton++) {
			// ������
			if (LOWORD(m_JoyCfg[i].dwButton[nButton]) == 0) {
				continue;
			}

			// �I�t��
			if ((m_JoyState[i].rgbButtons[nButton] & 0x80) == 0) {
				// �A�˃J�E���^�N���A�̂�(�{�^���������́A�ŏ��ŃN���A���Ă���)
				m_JoyCfg[i].dwCount[nButton] = 0;
				continue;
			}

			ASSERT(HIWORD(m_JoyCfg[i].dwButton[nButton]) >= 0);
			ASSERT(HIWORD(m_JoyCfg[i].dwButton[nButton]) < PPI::PortMax);
			ASSERT(LOWORD(m_JoyCfg[i].dwButton[nButton]) > 0);
			ASSERT(LOWORD(m_JoyCfg[i].dwButton[nButton]) <= PPI::ButtonMax);

			// �A��0��
			if (m_JoyCfg[i].dwRapid[nButton] == 0) {
				// �ړI�ʒu�Ɋi�[
				ji[HIWORD(m_JoyCfg[i].dwButton[nButton])].button[LOWORD(m_JoyCfg[i].dwButton[nButton]) - 1]
					= TRUE;
				continue;
			}

			// �A�˂���
			if (m_JoyCfg[i].dwCount[nButton] == 0) {
				// ����Ȃ̂ŁAON�ƃJ�E���^�����[�h
				ji[HIWORD(m_JoyCfg[i].dwButton[nButton])].button[LOWORD(m_JoyCfg[i].dwButton[nButton]) - 1]
					= TRUE;
				m_JoyCfg[i].dwCount[nButton] = JoyRapidTable[m_JoyCfg[i].dwRapid[nButton]];
				continue;
			}

			// �A�˃J�E���g�_�E���B0�Ȃ�ON�ƃJ�E���^�����[�h
			m_JoyCfg[i].dwCount[nButton]--;
			if (m_JoyCfg[i].dwCount[nButton] == 0) {
				ji[HIWORD(m_JoyCfg[i].dwButton[nButton])].button[LOWORD(m_JoyCfg[i].dwButton[nButton]) - 1]
					= TRUE;
				m_JoyCfg[i].dwCount[nButton] = JoyRapidTable[m_JoyCfg[i].dwRapid[nButton]];
				continue;
			}

			// �J�E���^�̑O���E�㔼�ŁAON/OFF�ɕ�����
			if (m_JoyCfg[i].dwCount[nButton] > (JoyRapidTable[m_JoyCfg[i].dwRapid[nButton]] >> 1)) {
				ji[HIWORD(m_JoyCfg[i].dwButton[nButton])].button[LOWORD(m_JoyCfg[i].dwButton[nButton]) - 1]
					= TRUE;
			}
			else {
				ji[HIWORD(m_JoyCfg[i].dwButton[nButton])].button[LOWORD(m_JoyCfg[i].dwButton[nButton]) - 1]
					= FALSE;
			}
		}
	}

	// �L�[�{�[�h�Ƃ̍���


	// PPI�֑��M
	for (i=0; i<PPI::PortMax; i++) {
		m_pPPI->SetJoyInfo(i, &ji[i]);
	}
}

//---------------------------------------------------------------------------
//
//	�W���C�X�e�B�b�N�L�����E������
//
//---------------------------------------------------------------------------
void FASTCALL CInput::EnableJoy(BOOL bEnable)
{
	PPI::joyinfo_t ji;

	ASSERT(this);

	// �����Ȃ�K�v�Ȃ�
	if (m_bJoyEnable == bEnable) {
		return;
	}

	// �ύX
	m_bJoyEnable = bEnable;

	// FALSE�ɕύX����ꍇ�APPI�ɑ΂��ăk����񑗐M
	if (!bEnable) {
		memset(&ji, 0, sizeof(ji));
		m_pPPI->SetJoyInfo(0, &ji);
		m_pPPI->SetJoyInfo(1, &ji);
	}
}

//---------------------------------------------------------------------------
//
//	�W���C�X�e�B�b�N�f�o�C�X�擾
//
//---------------------------------------------------------------------------
int FASTCALL CInput::GetJoyDevice(int nJoy) const
{
	ASSERT(this);
	ASSERT((nJoy >= 0) && (nJoy < JoyDevices));

	// 0�͊��蓖�ĂȂ�
	if (m_JoyCfg[nJoy].nDevice == 0) {
		return 0;
	}

	// �f�o�C�X�|�C���^�������Ă��Ȃ���΁A�������G���[�Ƃ���-1
	if (!m_lpDIJoy[nJoy]) {
		return -1;
	}

	// �������������Ă���̂ŁA�f�o�C�X�ԍ�
	return m_JoyCfg[nJoy].nDevice;
}

//---------------------------------------------------------------------------
//
//	�W���C�X�e�B�b�N���擾
//
//---------------------------------------------------------------------------
LONG FASTCALL CInput::GetJoyAxis(int nJoy, int nAxis) const
{
	BYTE *pOffset;
	LONG *pAxis;

	ASSERT(this);
	ASSERT((nJoy >= 0) && (nJoy < JoyDevices));
	ASSERT((nAxis >= 0) && (nAxis < JoyAxes));

	// 0�͊��蓖�ĂȂ�
	if (m_JoyCfg[nJoy].nDevice == 0) {
		return 0x10000;
	}

	// �f�o�C�X�|�C���^�������Ă��Ȃ���΁A�������G���[
	if (!m_lpDIJoy[nJoy]) {
		return 0x10000;
	}

	// �������݂��Ȃ���΁A�w��G���[
	if (m_lJoyAxisMin[nJoy][nAxis] == m_lJoyAxisMax[nJoy][nAxis]) {
		return 0x10000;
	}

	// �l��Ԃ�
	pOffset = (BYTE*)&m_JoyState[nJoy];
	pOffset += JoyAxisOffsetTable[nAxis];
	pAxis = (LONG*)pOffset;
	return *pAxis;
}

//---------------------------------------------------------------------------
//
//	�W���C�X�e�B�b�N�{�^���擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL CInput::GetJoyButton(int nJoy, int nButton) const
{
	ASSERT(this);
	ASSERT((nJoy >= 0) && (nJoy < JoyDevices));
	ASSERT((nButton >= 0) && (nButton < JoyButtons));

	// 0�͊��蓖�ĂȂ�
	if (m_JoyCfg[nJoy].nDevice == 0) {
		return 0x10000;
	}

	// �f�o�C�X�|�C���^�������Ă��Ȃ���΁A�������G���[
	if (!m_lpDIJoy[nJoy]) {
		return 0x10000;
	}

	// �{�^����������Ȃ���΁A�w��G���[
	if (nButton >= (int)m_JoyDevCaps[m_JoyCfg[nJoy].nDevice - 1].dwButtons) {
		return 0x10000;
	}

	// �l��Ԃ�
	return (DWORD)m_JoyState[nJoy].rgbButtons[nButton];
}

//---------------------------------------------------------------------------
//
//	�W���C�X�e�B�b�NCaps�擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::GetJoyCaps(int nDevice, CString& strDesc, DIDEVCAPS *pCaps) const
{
	ASSERT(this);
	ASSERT(nDevice >= 0);
	ASSERT(pCaps);

	// �W���C�X�e�B�b�N�f�o�C�X���Ɣ�r
	if (nDevice >= (int)m_dwJoyDevs) {
		// �w��C���f�b�N�X�̃f�o�C�X�͑��݂��Ȃ�
		return FALSE;
	}

	// Desc�ݒ�
	ASSERT(nDevice < JoyDeviceMax);
	strDesc = m_JoyDevInst[nDevice].tszInstanceName;

	// Caps�R�s�[
	*pCaps = m_JoyDevCaps[nDevice];

	// ����
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�W���C�X�e�B�b�N�ݒ�擾
//
//---------------------------------------------------------------------------
void FASTCALL CInput::GetJoyCfg(int nJoy, LPJOYCFG lpJoyCfg) const
{
	ASSERT(this);
	ASSERT((nJoy >= 0) && (nJoy < JoyDevices));

	// �ݒ���R�s�[
	*lpJoyCfg = m_JoyCfg[nJoy];
}

//---------------------------------------------------------------------------
//
//	�W���C�X�e�B�b�N�ݒ�Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL CInput::SetJoyCfg(int nJoy, const LPJOYCFG lpJoyCfg)
{
	ASSERT(this);
	ASSERT((nJoy >= 0) && (nJoy < JoyDevices));

	// �ݒ���R�s�[
	m_JoyCfg[nJoy] = *lpJoyCfg;
}

//---------------------------------------------------------------------------
//
//	�W���C�X�e�B�b�N���e�[�u��
//
//---------------------------------------------------------------------------
const DWORD CInput::JoyAxisOffsetTable[JoyAxes] = {
	DIJOFS_X,
	DIJOFS_Y,
	DIJOFS_Z,
	DIJOFS_RX,
	DIJOFS_RY,
	DIJOFS_RZ,
	DIJOFS_SLIDER(0),
	DIJOFS_SLIDER(1)
};

//---------------------------------------------------------------------------
//
//	�W���C�X�e�B�b�N�A�˃e�[�u��
//	���A�ˑ��x��60�t���[��/sec�Ɖ��肵���ꍇ�̒l
//
//---------------------------------------------------------------------------
const DWORD CInput::JoyRapidTable[JoyRapids + 1] = {
	0,									// (���g�p�G���A)
	30,									// 2��
	20,									// 3��
	15,									// 4��
	12,									// 5��
	8,									// 7.5��
	6,									// 10��
	5,									// 12��
	4,									// 15��
	3,									// 20��
	2									// 30��
};

#endif	// _WIN32
