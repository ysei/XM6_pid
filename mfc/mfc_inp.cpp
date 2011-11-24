//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC インプット ]
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
//	インプット
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CInput::CInput(CFrmWnd *pWnd) : CComponent(pWnd)
{
	int i;
	int nAxis;
	int nButton;

	// コンポーネントパラメータ
	m_dwID = MAKEID('I', 'N', 'P', ' ');
	m_strDesc = _T("Input Manager");

	// 共通ワーク初期化
	m_lpDI = NULL;
	m_bActive = TRUE;
	m_bMenu = FALSE;
	m_pCRTC = NULL;
	m_dwDispCount = 0;
	m_dwProcessCount = 0;

	// キーボードワーク初期化
	m_pKeyboard = NULL;

	// マウスワーク初期化
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

	// ジョイスティックワーク初期化
	m_pPPI = NULL;
	m_dwJoyDevs = 0;
	m_bJoyEnable = TRUE;
	for (i=0; i<JoyDevices; i++) {
		// デバイス
		m_lpDIJoy[i] = NULL;
		m_lpDIDev2[i] = NULL;

		// コンフィグ
		memset(&m_JoyCfg[i], 0, sizeof(JOYCFG));
		// nDeviceの設定:
		// 割り当てデバイス+1 (0は未割り当て)
		m_JoyCfg[i].nDevice = i + 1;
		for (nAxis=0; nAxis<JoyAxes; nAxis++) {
			if (nAxis < 4) {
				// dwAxisの設定:
				// 上位ワード 割り当てポート (0x00000 or 0x10000)
				// 下位ワード 割り当て軸+1 (1～4、0は未割り当て)
				m_JoyCfg[i].dwAxis[nAxis] = (DWORD)((i << 16) | (nAxis + 1));
			}
			m_JoyCfg[i].bAxis[nAxis] = FALSE;
		}
		for (nButton=0; nButton<JoyButtons; nButton++) {
			if (nButton < 8) {
				// dwButtonの設定:
				// 上位ワード 割り当てポート (0x00000 or 0x10000)
				// 下位ワード 割り当てボタン+1 (1～8、0は未割り当て)
				m_JoyCfg[i].dwButton[nButton] = (DWORD)((i << 16) | (nButton + 1));
			}
			// 連射なし、カウンタ初期化
			m_JoyCfg[i].dwRapid[nButton] = 0;
			m_JoyCfg[i].dwCount[nButton] = 0;
		}

		// 軸レンジ
		memset(m_lJoyAxisMin[i], 0, sizeof(m_lJoyAxisMin[i]));
		memset(m_lJoyAxisMax[i], 0, sizeof(m_lJoyAxisMax[i]));

		// 獲得カウンタ
		m_dwJoyAcquire[i] = 0;

		// 入力データ
		memset(&m_JoyState[i], 0, sizeof(DIJOYSTATE));
	}
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!CComponent::Init()) {
		return FALSE;
	}

	// CRTC取得
	ASSERT(!m_pCRTC);
	m_pCRTC = (CRTC*)::GetVM()->SearchDevice(MAKEID('C', 'R', 'T', 'C'));
	ASSERT(m_pCRTC);

	// キーボード取得
	ASSERT(!m_pKeyboard);
	m_pKeyboard = (Keyboard*)::GetVM()->SearchDevice(MAKEID('K', 'E', 'Y', 'B'));
	ASSERT(m_pKeyboard);

	// マウス取得
	ASSERT(!m_pMouse);
	m_pMouse = (Mouse*)::GetVM()->SearchDevice(MAKEID('M', 'O', 'U', 'S'));
	ASSERT(m_pMouse);

	// PPI取得
	ASSERT(!m_pPPI);
	m_pPPI = (PPI*)::GetVM()->SearchDevice(MAKEID('P', 'P', 'I', ' '));
	ASSERT(m_pPPI);

	// DirectInputオブジェクトを作成
//VC2010//	if (FAILED(DirectInputCreate(AfxGetApp()->m_hInstance, DIRECTINPUT_VERSION,
//VC2010//							&m_lpDI, NULL))) {
	if (FAILED(DirectInput8Create(AfxGetApp()->m_hInstance, DIRECTINPUT_VERSION,	//VC2010//
							IID_IDirectInput8, (void**) &m_lpDI, NULL))) {			//VC2010//
		return FALSE;
	}

	// キーボード
//	if (!InitKey()) {
//		return FALSE;
//	}

	// マウス
	if (!InitMouse()) {
		return FALSE;
	}

	// ジョイスティック
	EnumJoy();
	InitJoy();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL CInput::Cleanup()
{
	int i;

	ASSERT(this);
	ASSERT_VALID(this);

	// マウスモード
	SetMouseMode(FALSE);

	// ジョイスティックデバイスを解放
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

	// マウスデバイスを解放
	if (m_lpDIMouse) {
		m_lpDIMouse->Unacquire();
		m_lpDIMouse->Release();
		m_lpDIMouse = NULL;
	}

	// DirectInputオブジェクトを解放
	if (m_lpDI) {
		m_lpDI->Release();
		m_lpDI = NULL;
	}

	// 基本クラス
	CComponent::Cleanup();
}

//---------------------------------------------------------------------------
//
//	設定適用
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

	// マウス中ボタン
	m_bMouseMid = 1;	//pConfig->mouse_mid;

	// 中央ボタンカウントを無効化
	m_dwMouseMid = 5;

	// ジョイスティックデバイス(デバイス変更系)
	bFlag = FALSE;
	for (i=0; i<JoyDevices; i++) {
		// 使うデバイスNo.が変わっていたら、再初期化が必要
		if (pConfig->joy_dev[i] != m_JoyCfg[i].nDevice) {
			m_JoyCfg[i].nDevice = pConfig->joy_dev[i];
			bFlag = TRUE;
		}
	}
	if (bFlag) {
		// 再初期化
		InitJoy();
	}

	// ジョイスティックデバイス(ボタン系)
	for (i=0; i<JoyDevices; i++) {
		for (nButton=0; nButton<JoyButtons; nButton++) {
			// コンフィグデータ取得(bit16:ポート bit15-8:連射 bit7-0:ボタン)
			if (i == 0) {
				nConfig = pConfig->joy_button0[nButton];
			}
			else {
				nConfig = pConfig->joy_button1[nButton];
			}

			// 初期化
			m_JoyCfg[i].dwButton[nButton] = 0;
			m_JoyCfg[i].dwRapid[nButton] = 0;
			m_JoyCfg[i].dwCount[nButton] = 0;

			// 未割り当てかチェック
			if ((nConfig & 0xff) == 0) {
				continue;
			}

			// ボタン数が制限を超えていないかチェック
			if ((nConfig & 0xff) > PPI::ButtonMax) {
				continue;
			}

			// ボタン割り当て設定
			m_JoyCfg[i].dwButton[nButton] = (DWORD)(nConfig & 0xff00ff);

			// 連射設定
			m_JoyCfg[i].dwRapid[nButton] = (DWORD)((nConfig >> 8) & 0xff);
			if (m_JoyCfg[i].dwRapid[nButton] > JoyRapids) {
				// 範囲オーバの場合、連射なしとする
				m_JoyCfg[i].dwRapid[nButton] = 0;
			}
		}
	}
}

#if defined(_DEBUG)
//---------------------------------------------------------------------------
//
//	診断
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
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::Save(Fileio *pFio, int)
{
	BOOL bResult;

	ASSERT(this);
	ASSERT(pFio);
	ASSERT_VALID(this);

	// VMロックし、一時的に進行を止める
	::LockVM();

	// セーブ本体
	bResult = SaveMain(pFio);

	// VMアンロック
	::UnlockVM();

	// 結果を持ち帰る
	return bResult;
}

//---------------------------------------------------------------------------
//
//	セーブ本体
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

	// 全般
	if (!pFio->Write(&m_dwProcessCount, sizeof(m_dwProcessCount))) {
		return FALSE;
	}
	if (!pFio->Write(&m_dwDispCount, sizeof(m_dwDispCount))) {
		return FALSE;
	}

	// マウス
	if (!pFio->Write(&m_nMouseX, sizeof(m_nMouseX))) {
		return FALSE;
	}
	if (!pFio->Write(&m_nMouseY, sizeof(m_nMouseY))) {
		return FALSE;
	}
	if (!pFio->Write(&m_dwMouseMid, sizeof(m_dwMouseMid))) {
		return FALSE;
	}

	// ジョイスティック
	if (!pFio->Write(m_JoyState, sizeof(m_JoyState))) {
		return FALSE;
	}

	//
	//	version2.01
	//

	// マウス
	if (!pFio->Write(m_bMouseB, sizeof(m_bMouseB))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::Load(Fileio *pFio, int nVer)
{
	ASSERT(this);
	ASSERT(pFio);
	ASSERT(nVer >= 0x0200);
	ASSERT_VALID(this);

	// VMロックし、一時的に進行を止める
	::LockVM();

	// ロード(version2.00)
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

	// VMアンロック
	::UnlockVM();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード本体 (version2.00)
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::Load200(Fileio *pFio)
{
	ASSERT(this);
	ASSERT(pFio);
	ASSERT_VALID(this);

	// 全般
	if (!pFio->Read(&m_dwProcessCount, sizeof(m_dwProcessCount))) {
		return FALSE;
	}
	if (!pFio->Read(&m_dwDispCount, sizeof(m_dwDispCount))) {
		return FALSE;
	}

	// マウス
	if (!pFio->Read(&m_nMouseX, sizeof(m_nMouseX))) {
		return FALSE;
	}
	if (!pFio->Read(&m_nMouseY, sizeof(m_nMouseY))) {
		return FALSE;
	}
	if (!pFio->Read(&m_dwMouseMid, sizeof(m_dwMouseMid))) {
		return FALSE;
	}

	// ジョイスティック
	if (!pFio->Read(m_JoyState, sizeof(m_JoyState))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード本体 (version2.01)
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::Load201(Fileio *pFio)
{
	ASSERT(this);
	ASSERT(pFio);
	ASSERT_VALID(this);

	// マウス
	if (!pFio->Read(m_bMouseB, sizeof(m_bMouseB))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	進行
//
//---------------------------------------------------------------------------
void FASTCALL CInput::Process(BOOL bRun)
{
	DWORD dwDispCount;

	ASSERT(this);
	ASSERT_VALID(this);

	// ディセーブルなら何もしない
	if (!m_bEnable) {
		return;
	}

	// bRun = FALSEなら、スケジューラ停止中(10msおきに呼ばれる)
	if (!bRun) {
		// 進行カウンタUp
		m_dwProcessCount++;

		// ダミー入力
		InputKey(FALSE);
		InputMouse(FALSE);
		InputJoy(FALSE);
		return;
	}

	// CRTCの表示カウンタを見て、フレームごとに処理する
	ASSERT(m_pCRTC);
	dwDispCount = m_pCRTC->GetDispCount();
	if (dwDispCount == m_dwDispCount) {
		return;
	}
	m_dwDispCount = dwDispCount;

	// 進行カウンタUp
	m_dwProcessCount++;

	// アクティブでないか、またはメニュー中ならダミー入力
	if (!m_bActive || m_bMenu) {
		// ダミー入力
		InputKey(FALSE);
		InputMouse(FALSE);
		InputJoy(FALSE);
		MakeJoy(FALSE);
		return;
	}

	// ok、入力処理できる
	InputKey(TRUE);
	InputMouse(TRUE);
	InputJoy(TRUE);
	MakeJoy(m_bJoyEnable);
}

//---------------------------------------------------------------------------
//
//	アクティベート通知
//
//---------------------------------------------------------------------------
void FASTCALL CInput::Activate(BOOL bActive)
{
	ASSERT(this);
	ASSERT_VALID(this);

	// アクティブフラグに反映
	m_bActive = bActive;
}

//---------------------------------------------------------------------------
//
//	メニュー通知
//
//---------------------------------------------------------------------------
void FASTCALL CInput::Menu(BOOL bMenu)
{
	ASSERT(this);
	ASSERT_VALID(this);

	// メニューフラグに反映
	m_bMenu = bMenu;
}

//---------------------------------------------------------------------------
//
//	獲得カウンタ取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL CInput::GetAcquireCount(int nType) const
{
	ASSERT(this);
	ASSERT(JoyDevices >= 2);
	ASSERT_VALID(this);

	switch (nType) {
		// 1:マウス
		case 1:
			return m_dwMouseAcquire;

		// 2:ジョイスティックA
		case 2:
			return m_dwJoyAcquire[0];

		// 3:ジョイスティックB
		case 3:
			return m_dwJoyAcquire[1];

		// その他
		default:
			break;
	}

	// 通常、ここにはこない
	ASSERT(FALSE);
	return 0;
}

//===========================================================================
//
//	キーボード
//
//===========================================================================
namespace XM6_pid {
	typedef enum X68kKeyCode {
		X68K_KEYCODE_NONE	= 0,
		X68K_KEYCODE_ESC	= 1	,	// 01 [ESC]			.			.				.
		X68K_KEYCODE_1			,	// 02 [1]			!			ぬ				.
		X68K_KEYCODE_2			,	// 03 [2]			"			ふ				.
		X68K_KEYCODE_3			,	// 04 [3]			#			あ				ぁ
		X68K_KEYCODE_4			,	// 05 [4]			$			う				ぅ
		X68K_KEYCODE_5			,	// 06 [5]			%			え				ぇ
		X68K_KEYCODE_6			,	// 07 [6]			&			お				ぉ
		X68K_KEYCODE_7			,	// 08 [7]			'			や				ゃ
		X68K_KEYCODE_8			,	// 09 [8]			(			ゆ				ゅ
		X68K_KEYCODE_9			,	// 0A [9]			)			よ				ょ
		X68K_KEYCODE_0			,	// 0B [0]			.			わ				を
		X68K_KEYCODE_MINUS		,	// 0C [-]			=			ほ				.
		X68K_KEYCODE_CIRCUMFLEX	,	// 0D [^]			~			へ				.
		X68K_KEYCODE_YEN		,	// 0E [￥]			|			ー				.
		X68K_KEYCODE_BS			,	// 0F [BS]			.			.				.
		X68K_KEYCODE_TAB		,	// 10 [TAB]			.			.				.
		X68K_KEYCODE_Q			,	// 11 [Q]			.			た				.
		X68K_KEYCODE_W			,	// 12 [W]			.			て				.
		X68K_KEYCODE_E			,	// 13 [E]			.			い				ぃ
		X68K_KEYCODE_R			,	// 14 [R]			.			す				.
		X68K_KEYCODE_T			,	// 15 [T]			.			か				.
		X68K_KEYCODE_Y			,	// 16 [Y]			.			ん				.
		X68K_KEYCODE_U			,	// 17 [U]			.			な				.
		X68K_KEYCODE_I			,	// 18 [I]			.			に				.
		X68K_KEYCODE_O			,	// 19 [O]			.			ら				.
		X68K_KEYCODE_P			,	// 1A [P]			.			せ				.
		X68K_KEYCODE_AT			,	// 1B [@]			`			゛				.
		X68K_KEYCODE_LBRACKET	,	// 1C [[]			{			゜				「
		X68K_KEYCODE_CR			,	// 1D [CR]			.			.				.
		X68K_KEYCODE_A			,	// 1E [A]			.			ち				.
		X68K_KEYCODE_S			,	// 1F [S]			.			と				.
		X68K_KEYCODE_D			,	// 20 [D]			.			し				.
		X68K_KEYCODE_F			,	// 21 [F]			.			は				.
		X68K_KEYCODE_G			,	// 22 [G]			.			き				.
		X68K_KEYCODE_H			,	// 23 [H]			.			く				.
		X68K_KEYCODE_J			,	// 24 [J]			.			ま				.
		X68K_KEYCODE_K			,	// 25 [K]			.			の				.
		X68K_KEYCODE_L			,	// 26 [L]			.			り				.
		X68K_KEYCODE_SEMICOLON	,	// 27 [;]			+			れ				.
		X68K_KEYCODE_COLON		,	// 28 [:]			*			け				.
		X68K_KEYCODE_RBRACKET	,	// 29 []]			}			む				」
		X68K_KEYCODE_Z			,	// 2A [Z]			.			つ				っ
		X68K_KEYCODE_X			,	// 2B [X]			.			さ				.
		X68K_KEYCODE_C			,	// 2C [C]			.			そ				.
		X68K_KEYCODE_V			,	// 2D [V]			.			ひ				.
		X68K_KEYCODE_B			,	// 2E [B]			.			こ				.
		X68K_KEYCODE_N			,	// 2F [N]			.			み				.
		X68K_KEYCODE_M			,	// 30 [M]			.			も				.
		X68K_KEYCODE_COMMA		,	// 31 [,]			<			ね				、
		X68K_KEYCODE_PERIOD		,	// 32 [.]			>			る				。
		X68K_KEYCODE_SLASH		,	// 33 [/]			?			め				・
		X68K_KEYCODE_UNDERSCORE	,	// 34 .				_			ろ				.
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
		X68K_KEYCODE_KIGOU		,	// 52 [記号入力]
		X68K_KEYCODE_TOUROKU	,	// 53 [登録]
		X68K_KEYCODE_HELP		,	// 54 [HELP]
		X68K_KEYCODE_XF1		,	// 55 [XF1]
		X68K_KEYCODE_XF2		,	// 56 [XF2]
		X68K_KEYCODE_XF3		,	// 57 [XF3]
		X68K_KEYCODE_XF4		,	// 58 [XF4]
		X68K_KEYCODE_XF5		,	// 59 [XF5]
		X68K_KEYCODE_KANA		,	// 5A [かな]
		X68K_KEYCODE_ROMA		,	// 5B [ローマ字]
		X68K_KEYCODE_CODE		,	// 5C [コード入力]
		X68K_KEYCODE_CAPS		,	// 5D [CAPS]
		X68K_KEYCODE_INS		,	// 5E [INS]
		X68K_KEYCODE_HIRAGANA	,	// 5F [ひらがな]
		X68K_KEYCODE_ZENKAKU	,	// 60 [全角]
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
	{ '1'						,	X68K_KEYCODE_1			},	// 02 [1]			!			ぬ				.
	{ '2'						,	X68K_KEYCODE_2			},	// 03 [2]			"			ふ				.
	{ '3'						,	X68K_KEYCODE_3			},	// 04 [3]			#			あ				ぁ
	{ '4'						,	X68K_KEYCODE_4			},	// 05 [4]			$			う				ぅ
	{ '5'						,	X68K_KEYCODE_5			},	// 06 [5]			%			え				ぇ
	{ '6'						,	X68K_KEYCODE_6			},	// 07 [6]			&			お				ぉ
	{ '7'						,	X68K_KEYCODE_7			},	// 08 [7]			'			や				ゃ
	{ '8'						,	X68K_KEYCODE_8			},	// 09 [8]			(			ゆ				ゅ
	{ '9'						,	X68K_KEYCODE_9			},	// 0A [9]			)			よ				ょ
	{ '0'						,	X68K_KEYCODE_0			},	// 0B [0]			.			わ				を
	{ VK_OEM_MINUS				,	X68K_KEYCODE_MINUS		},	// 0C [-]			=			ほ				.
	{ VK_OEM_PLUS				,	X68K_KEYCODE_CIRCUMFLEX	},	// 0D [^]			~			へ				.
	{ VK_OEM_5					,	X68K_KEYCODE_YEN		},	// 0E [￥]			|			ー				.
	{ VK_BACK					,	X68K_KEYCODE_BS			},	// 0F [BS]			.			.				.
	{ VK_TAB					,	X68K_KEYCODE_TAB		},	// 10 [TAB]			.			.				.
	{ 'Q'						,	X68K_KEYCODE_Q			},	// 11 [Q]			.			た				.
	{ 'W'						,	X68K_KEYCODE_W			},	// 12 [W]			.			て				.
	{ 'E'						,	X68K_KEYCODE_E			},	// 13 [E]			.			い				ぃ
	{ 'R'						,	X68K_KEYCODE_R			},	// 14 [R]			.			す				.
	{ 'T'						,	X68K_KEYCODE_T			},	// 15 [T]			.			か				.
	{ 'Y'						,	X68K_KEYCODE_Y			},	// 16 [Y]			.			ん				.
	{ 'U'						,	X68K_KEYCODE_U			},	// 17 [U]			.			な				.
	{ 'I'						,	X68K_KEYCODE_I			},	// 18 [I]			.			に				.
	{ 'O'						,	X68K_KEYCODE_O			},	// 19 [O]			.			ら				.
	{ 'P'						,	X68K_KEYCODE_P			},	// 1A [P]			.			せ				.
	{ 0000						,	X68K_KEYCODE_AT			},	// 1B [@]			`			゛				.
	{ VK_OEM_4					,	X68K_KEYCODE_LBRACKET	},	// 1C [[]			{			゜				「
	{ VK_RETURN					,	X68K_KEYCODE_CR			},	// 1D [CR]			.			.				.
	{ 'A'						,	X68K_KEYCODE_A			},	// 1E [A]			.			ち				.
	{ 'S'						,	X68K_KEYCODE_S			},	// 1F [S]			.			と				.
	{ 'D'						,	X68K_KEYCODE_D			},	// 20 [D]			.			し				.
	{ 'F'						,	X68K_KEYCODE_F			},	// 21 [F]			.			は				.
	{ 'G'						,	X68K_KEYCODE_G			},	// 22 [G]			.			き				.
	{ 'H'						,	X68K_KEYCODE_H			},	// 23 [H]			.			く				.
	{ 'J'						,	X68K_KEYCODE_J			},	// 24 [J]			.			ま				.
	{ 'K'						,	X68K_KEYCODE_K			},	// 25 [K]			.			の				.
	{ 'L'						,	X68K_KEYCODE_L			},	// 26 [L]			.			り				.
	{ VK_OEM_1					,	X68K_KEYCODE_SEMICOLON	},	// 27 [;]			+			れ				.
	{ 0000						,	X68K_KEYCODE_COLON		},	// 28 [:]			*			け				.
	{ VK_OEM_6					,	X68K_KEYCODE_RBRACKET	},	// 29 []]			}			む				」
	{ 'Z'						,	X68K_KEYCODE_Z			},	// 2A [Z]			.			つ				っ
	{ 'X'						,	X68K_KEYCODE_X			},	// 2B [X]			.			さ				.
	{ 'C'						,	X68K_KEYCODE_C			},	// 2C [C]			.			そ				.
	{ 'V'						,	X68K_KEYCODE_V			},	// 2D [V]			.			ひ				.
	{ 'B'						,	X68K_KEYCODE_B			},	// 2E [B]			.			こ				.
	{ 'N'						,	X68K_KEYCODE_N			},	// 2F [N]			.			み				.
	{ 'M'						,	X68K_KEYCODE_M			},	// 30 [M]			.			も				.
	{ VK_OEM_COMMA				,	X68K_KEYCODE_COMMA		},	// 31 [,]			<			ね				、
	{ VK_OEM_PERIOD				,	X68K_KEYCODE_PERIOD		},	// 32 [.]			>			る				。
	{ VK_OEM_2					,	X68K_KEYCODE_SLASH		},	// 33 [/]			?			め				・
	{ 0000						,	X68K_KEYCODE_UNDERSCORE	},	// 34 .				_			ろ				.
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
	{ 0000						,	X68K_KEYCODE_KIGOU		},	// 52 [記号入力]
	{ 0000						,	X68K_KEYCODE_TOUROKU	},	// 53 [登録]
	{ 0000						,	X68K_KEYCODE_HELP		},	// 54 [HELP]
	{ 0000						,	X68K_KEYCODE_XF1		},	// 55 [XF1]
	{ 0000						,	X68K_KEYCODE_XF2		},	// 56 [XF2]
	{ 0000						,	X68K_KEYCODE_XF3		},	// 57 [XF3]
	{ 0000						,	X68K_KEYCODE_XF4		},	// 58 [XF4]
	{ 0000						,	X68K_KEYCODE_XF5		},	// 59 [XF5]
	{ 0000						,	X68K_KEYCODE_KANA		},	// 5A [かな]
	{ 0000						,	X68K_KEYCODE_ROMA		},	// 5B [ローマ字]
	{ 0000						,	X68K_KEYCODE_CODE		},	// 5C [コード入力]
	{ 0000						,	X68K_KEYCODE_CAPS		},	// 5D [CAPS]
	{ VK_INSERT					,	X68K_KEYCODE_INS		},	// 5E [INS]
	{ 0000						,	X68K_KEYCODE_HIRAGANA	},	// 5F [ひらがな]
	{ 0000						,	X68K_KEYCODE_ZENKAKU	},	// 60 [全角]
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
//	キーボード入力
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
//	マウス
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	マウス初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::InitMouse()
{
	ASSERT(this);
	ASSERT(m_lpDI);
	ASSERT(!m_lpDIMouse);
	ASSERT_VALID(this);

	// マウスデバイスを作成
	if (FAILED(m_lpDI->CreateDevice(GUID_SysMouse, &m_lpDIMouse, NULL))) {
		return FALSE;
	}

	// マウスデータ形式を設定
	if (FAILED(m_lpDIMouse->SetDataFormat(&c_dfDIMouse))) {
		return FALSE;
	}

	// 協調レベルを設定(Win9x/WinNTで挙動が異なるため、分ける)
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
//	マウス入力
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

	// 処理してよいか
	if (!bEnable) {
		// マウスモードOFF
		if (m_bMouseMode) {
			m_pFrmWnd->PostMessage(WM_COMMAND, IDM_MOUSEMODE, 0);
		}

		// ボタンUPを通知
		m_pMouse->SetMouse(m_nMouseX, m_nMouseY, FALSE, FALSE);
		return;
	}

	// マウスモードONか
	if (!m_bMouseMode) {
		// ボタンUPを通知
		m_pMouse->SetMouse(m_nMouseX, m_nMouseY, FALSE, FALSE);
		return;
	}

	// デバイス状態を取得
	hr = m_lpDIMouse->GetDeviceState(sizeof(dims), &dims);
	if (hr != DI_OK) {
		// Acquireを試みる
		m_lpDIMouse->Acquire();
		m_dwMouseAcquire++;

		// マウスリセット
		m_pMouse->ResetMouse();
		return;
	}

	// データをいったん確保
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

	// マウスデバイスへ通知
	m_pMouse->SetMouse(m_nMouseX, m_nMouseY, m_bMouseB[0], m_bMouseB[1]);

	// 中央ボタン機能が禁止されていれば、終了
	if (!m_bMouseMid) {
		m_dwMouseMid = 5;
		return;
	}

	// 中央ボタンをチェック。連続して押して離されたらマウスモードoff
	if (dims.rgbButtons[2] & 0x80) {
		// 押されている
		if (m_dwMouseMid < 4) {
			// リセット状態から
			m_dwMouseMid++;
			if (m_dwMouseMid == 4) {
				// 十分押されつづけているので、ホールド
				m_dwMouseMid = 3;
			}
		}
	}
	else {
		// 離されている
		if ((m_dwMouseMid == 3) || (m_dwMouseMid == 4)) {
			// 十分押された後か、その後の離しを１回検出した後に限る
			m_dwMouseMid++;
			if (m_dwMouseMid == 5) {
				// 3フレーム以上押されて、その後、2フレーム以上離された
				m_pFrmWnd->PostMessage(WM_COMMAND, IDM_MOUSEMODE, 0);
				m_dwMouseMid++;
			}
		}
		else {
			// 十分押されていないまま離された。リセット
			m_dwMouseMid = 0;
		}
	}
}

//---------------------------------------------------------------------------
//
//	マウスモード設定
//
//---------------------------------------------------------------------------
void FASTCALL CInput::SetMouseMode(BOOL bMode)
{
	ASSERT(this);
	ASSERT_VALID(this);

	// 現在のモードと違っていれば
	if (m_bMouseMode != bMode) {
		// モードを確保
		m_bMouseMode = bMode;

		// とにかくUnacquire
		if (m_lpDIMouse) {
			m_lpDIMouse->Unacquire();
		}

		// 中央ボタンカウントを無効化
		m_dwMouseMid = 5;
	}
}

//---------------------------------------------------------------------------
//
//	マウス情報取得
//
//---------------------------------------------------------------------------
void FASTCALL CInput::GetMouseInfo(int *pPos, BOOL *pBtn) const
{
	ASSERT(this);
	ASSERT(pPos);
	ASSERT(pBtn);
	ASSERT_VALID(this);

	// それぞれ3要素
	pPos[0] = m_nMouseX;
	pPos[1] = m_nMouseY;
	pPos[2] = (int)m_dwMouseMid;

	// ボタン
	pBtn[0] = m_bMouseB[0];
	pBtn[1] = m_bMouseB[1];
	pBtn[2] = m_bMouseMid;
}

//===========================================================================
//
//	ジョイスティック
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ジョイスティック列挙
//
//---------------------------------------------------------------------------
void FASTCALL CInput::EnumJoy()
{
	ASSERT(this);
	ASSERT(m_lpDI);

	// ジョイスティック数をクリア
	m_dwJoyDevs = 0;

	// 列挙開始
//VC2010//	m_lpDI->EnumDevices(DIDEVTYPE_JOYSTICK, (LPDIENUMDEVICESCALLBACK)EnumCb,
//VC2010//							this, DIEDFL_ATTACHEDONLY);
	m_lpDI->EnumDevices(DI8DEVTYPE_JOYSTICK, (LPDIENUMDEVICESCALLBACK)EnumCb,	//VC2010//
							this, DIEDFL_ATTACHEDONLY);							//VC2010//
}

//---------------------------------------------------------------------------
//
//	ジョイスティックコールバック
//
//---------------------------------------------------------------------------
BOOL CALLBACK CInput::EnumCb(LPDIDEVICEINSTANCE pDevInst, LPVOID pvRef)
{
	CInput *pInput;

	ASSERT(pDevInst);
	ASSERT(pvRef);

	// CInputに変換
	pInput = (CInput*)pvRef;

	// 呼び出し
	return pInput->EnumDev(pDevInst);
}

//---------------------------------------------------------------------------
//
//	ジョイスティック追加
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::EnumDev(LPDIDEVICEINSTANCE pDevInst)
{
	LPDIRECTINPUTDEVICE pInputDev;

	ASSERT(this);
	ASSERT(pDevInst);
	ASSERT(m_lpDI);

	// 最大数チェック。最大JoyDeviceMaxデバイスのみサポートする
	if (m_dwJoyDevs >= JoyDeviceMax) {
		ASSERT(m_dwJoyDevs == JoyDeviceMax);
		return DIENUM_STOP;
	}

	// インスタンスを確保
	memcpy(&m_JoyDevInst[m_dwJoyDevs], pDevInst, sizeof(DIDEVICEINSTANCE));

	// デバイス作成
	pInputDev = NULL;
	if (FAILED(m_lpDI->CreateDevice(pDevInst->guidInstance,
									&pInputDev,
									NULL))) {
		return DIENUM_CONTINUE;
	}
	ASSERT(pInputDev);

	// データフォーマット指定
	if (FAILED(pInputDev->SetDataFormat(&c_dfDIJoystick))) {
		// デバイス解放
		pInputDev->Unacquire();
		pInputDev->Release();
		return DIENUM_CONTINUE;
	}

	// Caps取得
	memset(&m_JoyDevCaps[m_dwJoyDevs], 0, sizeof(DIDEVCAPS));
	m_JoyDevCaps[m_dwJoyDevs].dwSize = sizeof(DIDEVCAPS);
	if (FAILED(pInputDev->GetCapabilities(&m_JoyDevCaps[m_dwJoyDevs]))) {
		// デバイス解放
		pInputDev->Unacquire();
		pInputDev->Release();
		return DIENUM_CONTINUE;
	}

	// デバイスを一旦解放
	pInputDev->Unacquire();
	pInputDev->Release();

	// 追加と継続
	m_dwJoyDevs++;
	return DIENUM_CONTINUE;
}

//---------------------------------------------------------------------------
//
//	ジョイスティック初期化
//	※ApplyCfgから呼び出す場合、dwDeviceが違っていた場合のみにすること
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

	// デバイスを一旦解放
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

	// 入力データをクリア
	for (i=0; i<JoyDevices; i++) {
		memset(&m_JoyState[i], 0, sizeof(DIJOYSTATE));
	}

	// 初期化ループ
	for (i=0; i<JoyDevices; i++) {
		// エラーフラグOFF
		bError[i] = FALSE;

		// 未使用なら、何もしない
		if (m_JoyCfg[i].nDevice == 0) {
			continue;
		}

		// デバイス作成
		nDevice = m_JoyCfg[i].nDevice - 1;
		if (FAILED(m_lpDI->CreateDevice(m_JoyDevInst[nDevice].guidInstance,
										&m_lpDIJoy[i],
										NULL))) {
			continue;
		}

		// エラーフラグON
		bError[i] = TRUE;

		// データフォーマット指定
		if (FAILED(m_lpDIJoy[i]->SetDataFormat(&c_dfDIJoystick))) {
			continue;
		}

		// 協調レベル設定
		if (FAILED(m_lpDIJoy[i]->SetCooperativeLevel(m_pFrmWnd->m_hWnd,
							DISCL_BACKGROUND | DISCL_NONEXCLUSIVE))) {
			continue;
		}

		// 値モード設定(絶対値)
		memset(&dpd, 0, sizeof(dpd));
		dpd.diph.dwSize = sizeof(DIPROPDWORD); 
		dpd.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		dpd.diph.dwHow = DIPH_DEVICE;
		dpd.dwData = DIPROPAXISMODE_ABS;
		if (FAILED(m_lpDIJoy[i]->SetProperty(DIPROP_AXISMODE, (LPDIPROPHEADER)&dpd))) {
			continue;
		}

		// デッドゾーン指定(デッドゾーンなし)
		memset(&dpd, 0, sizeof(dpd));
		dpd.diph.dwSize = sizeof(DIPROPDWORD);
		dpd.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		dpd.diph.dwHow = DIPH_DEVICE;
		dpd.dwData = 0;
		if (FAILED(m_lpDIJoy[i]->SetProperty(DIPROP_DEADZONE, (LPDIPROPHEADER)&dpd))) {
			continue;
		}

		// 軸ごとのレンジを取得(すべての軸について取得を試みる)
		for (nAxis=0; nAxis<JoyAxes; nAxis++) {
			// 取得(エラーでもよい)
			memset(&dpr, 0, sizeof(dpr));
			dpr.diph.dwSize = sizeof(DIPROPRANGE);
			dpr.diph.dwHeaderSize = sizeof(DIPROPHEADER);
			dpr.diph.dwHow = DIPH_BYOFFSET;
			dpr.diph.dwObj = JoyAxisOffsetTable[nAxis];
			m_lpDIJoy[i]->GetProperty(DIPROP_RANGE, (LPDIPROPHEADER)&dpr);

			// 確保
			m_lJoyAxisMin[i][nAxis] = dpr.lMin;
			m_lJoyAxisMax[i][nAxis] = dpr.lMax;
		}

		// IDirectInputDevice2を取得
		if (FAILED(m_lpDIJoy[i]->QueryInterface(IID_IDirectInputDevice2,
										(LPVOID*)&m_lpDIDev2[i]))) {
			// IDirectInputDevice2が取得できない場合
			m_lpDIDev2[i] = NULL;
		}

		// エラーフラグOFF(成功)
		bError[i] = FALSE;
	}

	// エラーが起きたデバイスについて、解放
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
//	ジョイスティック入力
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

	// bEnable=FALSEで、かつm_bJoyEnable=TRUE(VM側)の場合、入力データクリア
	if (!bEnable && m_bJoyEnable) {
		for (i=0; i<JoyDevices; i++) {
			// 入力データクリア
			memset(&m_JoyState[i], 0, sizeof(DIJOYSTATE));
		}
		return;
	}

	// デバイスループ
	for (i=0; i<JoyDevices; i++) {
		// デバイスがなければスキップ
		if (!m_lpDIJoy[i]) {
			continue;
		}

		// ポーリング
		if (m_lpDIDev2[i]) {
			if (FAILED(m_lpDIDev2[i]->Poll())) {
				// Acquireを試みる
				m_lpDIJoy[i]->Acquire();
				m_dwJoyAcquire[i]++;
				continue;
			}
		}

		// データ取得
		if (FAILED(m_lpDIJoy[i]->GetDeviceState(sizeof(DIJOYSTATE),
												&m_JoyState[i]))) {
			// Acquireを試みる
			m_lpDIJoy[i]->Acquire();
			m_dwJoyAcquire[i]++;
			continue;
		}

		// 軸変換(-800 ～ 7FFに変換)
		for (nAxis=0; nAxis<JoyAxes; nAxis++) {
			// ポインタ取得
			pOffset = (BYTE*)&m_JoyState[i];
			pOffset += JoyAxisOffsetTable[nAxis];
			pAxis = (LONG*)pOffset;

			// 無効な軸ならスキップ
			if (m_lJoyAxisMin[i][nAxis] == m_lJoyAxisMax[i][nAxis]) {
				continue;
			}

			// -lMinだけ底上げ(端点を0に揃える)
			lAxis = *pAxis - m_lJoyAxisMin[i][nAxis];

			// 範囲を出し、オーバーフロー防止変換
			lRange = m_lJoyAxisMax[i][nAxis] - m_lJoyAxisMin[i][nAxis] + 1;
			if (lRange >= 0x100000) {
				lRange >>= 12;
				lAxis >>= 12;
			}

			// 変換
			lAxis <<= 12;
			lAxis /= lRange;
			lAxis -= 0x800;
			*pAxis = lAxis;
		}
	}
}

//---------------------------------------------------------------------------
//
//	ジョイスティック合成
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

	// 全てクリア
	memset(ji, 0, sizeof(ji));

	// ディセーブルなら
	if (!bEnable) {
		for (i=0; i<PPI::PortMax; i++) {
			// 入力なしデータを送信
			m_pPPI->SetJoyInfo(i, &ji[i]);

			// ボタン連射をクリア
			for (nButton=0; nButton<JoyButtons; nButton++) {
				m_JoyCfg[i].dwCount[nButton] = 0;
			}
		}
		return;
	}

	// 入力データを解釈
	for (i=0; i<JoyDevices; i++) {
		dmy = -1;

		// 軸
		for (nAxis=0; nAxis<JoyAxes; nAxis++) {
#if 0
			// 無効か
			if (LOWORD(m_JoyCfg[i].dwAxis[nAxis]) == 0) {
				continue;
			}
#else
			// 無効な軸ならスキップ
			if (m_lJoyAxisMin[i][nAxis] == m_lJoyAxisMax[i][nAxis]) {
				continue;
			}
			dmy++;
			if (dmy >= 4) {
				break;
			}
#endif

			// ポインタ取得
			pOffset = (BYTE*)&m_JoyState[i];
			pOffset += JoyAxisOffsetTable[nAxis];
			pAxis = (LONG*)pOffset;

			// データ取得
			lAxis = *pAxis;

			// ゼロは無視(最初にクリアしているため)
			if (lAxis == 0) {
				continue;
			}

			// 反転
			if (m_JoyCfg[i].bAxis[nAxis]) {
				// 7FF→-800 -800→7FF
				lAxis = -1 - lAxis;
			}

#if 0
			// 目的位置に格納
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

		// ボタン
		for (nButton=0; nButton<JoyButtons; nButton++) {
			// 無効か
			if (LOWORD(m_JoyCfg[i].dwButton[nButton]) == 0) {
				continue;
			}

			// オフか
			if ((m_JoyState[i].rgbButtons[nButton] & 0x80) == 0) {
				// 連射カウンタクリアのみ(ボタン押下情報は、最初でクリアしている)
				m_JoyCfg[i].dwCount[nButton] = 0;
				continue;
			}

			ASSERT(HIWORD(m_JoyCfg[i].dwButton[nButton]) >= 0);
			ASSERT(HIWORD(m_JoyCfg[i].dwButton[nButton]) < PPI::PortMax);
			ASSERT(LOWORD(m_JoyCfg[i].dwButton[nButton]) > 0);
			ASSERT(LOWORD(m_JoyCfg[i].dwButton[nButton]) <= PPI::ButtonMax);

			// 連射0か
			if (m_JoyCfg[i].dwRapid[nButton] == 0) {
				// 目的位置に格納
				ji[HIWORD(m_JoyCfg[i].dwButton[nButton])].button[LOWORD(m_JoyCfg[i].dwButton[nButton]) - 1]
					= TRUE;
				continue;
			}

			// 連射あり
			if (m_JoyCfg[i].dwCount[nButton] == 0) {
				// 初回なので、ONとカウンタリロード
				ji[HIWORD(m_JoyCfg[i].dwButton[nButton])].button[LOWORD(m_JoyCfg[i].dwButton[nButton]) - 1]
					= TRUE;
				m_JoyCfg[i].dwCount[nButton] = JoyRapidTable[m_JoyCfg[i].dwRapid[nButton]];
				continue;
			}

			// 連射カウントダウン。0ならONとカウンタリロード
			m_JoyCfg[i].dwCount[nButton]--;
			if (m_JoyCfg[i].dwCount[nButton] == 0) {
				ji[HIWORD(m_JoyCfg[i].dwButton[nButton])].button[LOWORD(m_JoyCfg[i].dwButton[nButton]) - 1]
					= TRUE;
				m_JoyCfg[i].dwCount[nButton] = JoyRapidTable[m_JoyCfg[i].dwRapid[nButton]];
				continue;
			}

			// カウンタの前半・後半で、ON/OFFに分ける
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

	// キーボードとの合成


	// PPIへ送信
	for (i=0; i<PPI::PortMax; i++) {
		m_pPPI->SetJoyInfo(i, &ji[i]);
	}
}

//---------------------------------------------------------------------------
//
//	ジョイスティック有効化・無効化
//
//---------------------------------------------------------------------------
void FASTCALL CInput::EnableJoy(BOOL bEnable)
{
	PPI::joyinfo_t ji;

	ASSERT(this);

	// 同じなら必要なし
	if (m_bJoyEnable == bEnable) {
		return;
	}

	// 変更
	m_bJoyEnable = bEnable;

	// FALSEに変更する場合、PPIに対してヌル情報送信
	if (!bEnable) {
		memset(&ji, 0, sizeof(ji));
		m_pPPI->SetJoyInfo(0, &ji);
		m_pPPI->SetJoyInfo(1, &ji);
	}
}

//---------------------------------------------------------------------------
//
//	ジョイスティックデバイス取得
//
//---------------------------------------------------------------------------
int FASTCALL CInput::GetJoyDevice(int nJoy) const
{
	ASSERT(this);
	ASSERT((nJoy >= 0) && (nJoy < JoyDevices));

	// 0は割り当てなし
	if (m_JoyCfg[nJoy].nDevice == 0) {
		return 0;
	}

	// デバイスポインタを持っていなければ、初期化エラーとして-1
	if (!m_lpDIJoy[nJoy]) {
		return -1;
	}

	// 初期化成功しているので、デバイス番号
	return m_JoyCfg[nJoy].nDevice;
}

//---------------------------------------------------------------------------
//
//	ジョイスティック軸取得
//
//---------------------------------------------------------------------------
LONG FASTCALL CInput::GetJoyAxis(int nJoy, int nAxis) const
{
	BYTE *pOffset;
	LONG *pAxis;

	ASSERT(this);
	ASSERT((nJoy >= 0) && (nJoy < JoyDevices));
	ASSERT((nAxis >= 0) && (nAxis < JoyAxes));

	// 0は割り当てなし
	if (m_JoyCfg[nJoy].nDevice == 0) {
		return 0x10000;
	}

	// デバイスポインタを持っていなければ、初期化エラー
	if (!m_lpDIJoy[nJoy]) {
		return 0x10000;
	}

	// 軸が存在しなければ、指定エラー
	if (m_lJoyAxisMin[nJoy][nAxis] == m_lJoyAxisMax[nJoy][nAxis]) {
		return 0x10000;
	}

	// 値を返す
	pOffset = (BYTE*)&m_JoyState[nJoy];
	pOffset += JoyAxisOffsetTable[nAxis];
	pAxis = (LONG*)pOffset;
	return *pAxis;
}

//---------------------------------------------------------------------------
//
//	ジョイスティックボタン取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL CInput::GetJoyButton(int nJoy, int nButton) const
{
	ASSERT(this);
	ASSERT((nJoy >= 0) && (nJoy < JoyDevices));
	ASSERT((nButton >= 0) && (nButton < JoyButtons));

	// 0は割り当てなし
	if (m_JoyCfg[nJoy].nDevice == 0) {
		return 0x10000;
	}

	// デバイスポインタを持っていなければ、初期化エラー
	if (!m_lpDIJoy[nJoy]) {
		return 0x10000;
	}

	// ボタン数が合わなければ、指定エラー
	if (nButton >= (int)m_JoyDevCaps[m_JoyCfg[nJoy].nDevice - 1].dwButtons) {
		return 0x10000;
	}

	// 値を返す
	return (DWORD)m_JoyState[nJoy].rgbButtons[nButton];
}

//---------------------------------------------------------------------------
//
//	ジョイスティックCaps取得
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::GetJoyCaps(int nDevice, CString& strDesc, DIDEVCAPS *pCaps) const
{
	ASSERT(this);
	ASSERT(nDevice >= 0);
	ASSERT(pCaps);

	// ジョイスティックデバイス数と比較
	if (nDevice >= (int)m_dwJoyDevs) {
		// 指定インデックスのデバイスは存在しない
		return FALSE;
	}

	// Desc設定
	ASSERT(nDevice < JoyDeviceMax);
	strDesc = m_JoyDevInst[nDevice].tszInstanceName;

	// Capsコピー
	*pCaps = m_JoyDevCaps[nDevice];

	// 成功
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ジョイスティック設定取得
//
//---------------------------------------------------------------------------
void FASTCALL CInput::GetJoyCfg(int nJoy, LPJOYCFG lpJoyCfg) const
{
	ASSERT(this);
	ASSERT((nJoy >= 0) && (nJoy < JoyDevices));

	// 設定をコピー
	*lpJoyCfg = m_JoyCfg[nJoy];
}

//---------------------------------------------------------------------------
//
//	ジョイスティック設定セット
//
//---------------------------------------------------------------------------
void FASTCALL CInput::SetJoyCfg(int nJoy, const LPJOYCFG lpJoyCfg)
{
	ASSERT(this);
	ASSERT((nJoy >= 0) && (nJoy < JoyDevices));

	// 設定をコピー
	m_JoyCfg[nJoy] = *lpJoyCfg;
}

//---------------------------------------------------------------------------
//
//	ジョイスティック軸テーブル
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
//	ジョイスティック連射テーブル
//	※連射速度は60フレーム/secと仮定した場合の値
//
//---------------------------------------------------------------------------
const DWORD CInput::JoyRapidTable[JoyRapids + 1] = {
	0,									// (未使用エリア)
	30,									// 2発
	20,									// 3発
	15,									// 4発
	12,									// 5発
	8,									// 7.5発
	6,									// 10発
	5,									// 12発
	4,									// 15発
	3,									// 20発
	2									// 30発
};

#endif	// _WIN32
