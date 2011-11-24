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

	// マウス
	if (!InitMouse()) {
		return FALSE;
	}

	// ジョイスティック
//	EnumJoy();
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
//	int i;

	ASSERT(this);
	ASSERT_VALID(this);

	// マウスモード
	SetMouseMode(FALSE);

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
	ASSERT(this);
	ASSERT(pConfig);
	ASSERT_VALID(this);

	// マウス中ボタン
	m_bMouseMid = 1;	//pConfig->mouse_mid;

	// 中央ボタンカウントを無効化
	m_dwMouseMid = 5;
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
//		MakeJoy(FALSE);
		return;
	}

	// ok、入力処理できる
	InputKey(TRUE);
	InputMouse(TRUE);
	InputJoy(TRUE);
//	MakeJoy(m_bJoyEnable);
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
	ASSERT_VALID(this);

	switch (nType) {
		// 1:マウス
		case 1:
			return m_dwMouseAcquire;

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

	class DiJoyStick {
	public:
		struct Entry {
			DIDEVICEINSTANCE		diDeviceInstance;
			DIDEVCAPS				diDevCaps;
			LPDIRECTINPUTDEVICE8	diDevice;
			DIJOYSTATE2				joystate;
		};

		DiJoyStick() : entry(0), maxEntry(0), nEntry(0) {
		}

		~DiJoyStick() {
			clear();
		}

		void clear() {
			if(entry) {
				delete [] entry;
				entry = 0;
			}
			maxEntry	= 0;
			nEntry		= 0;
		}

		void enumerate(LPDIRECTINPUT di, DWORD dwDevType = DI8DEVTYPE_JOYSTICK, LPCDIDATAFORMAT lpdf = &c_dfDIJoystick2, DWORD dwFlags = DIEDFL_ATTACHEDONLY, int maxEntry = 16) {
			clear();

			entry			= new Entry [maxEntry];
			callback.di		= di;
			this->maxEntry	= maxEntry;
			nEntry			= 0;
			callback.lpdf	= lpdf;

			di->EnumDevices(dwDevType, DIEnumDevicesCallback_static, this, dwFlags);

			callback.di		= 0;
			callback.lpdf	= 0;
		}

		int getEntryCount() const {
			return nEntry;
		}

		const Entry* getEntry(int index) const {
			const Entry* e = 0;
			if(index >= 0 && index < nEntry) {
				e = &entry[index];
			}
			return e;
		}

		void update() {
			for(int iEntry = 0; iEntry < nEntry; ++iEntry) {
				Entry& e = entry[iEntry];
				LPDIRECTINPUTDEVICE8 d = e.diDevice;

				if(FAILED(d->Poll())) {
					HRESULT hr = d->Acquire();
					while(hr == DIERR_INPUTLOST) {
						hr = d->Acquire();
					}
				} else {
					d->GetDeviceState(sizeof(DIJOYSTATE2), &e.joystate);
				}
			}
		}

	protected:
		static BOOL CALLBACK DIEnumDevicesCallback_static(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef) {
			return reinterpret_cast<DiJoyStick*>(pvRef)->DIEnumDevicesCallback(lpddi, pvRef);
		}

		BOOL DIEnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef) {
			if(nEntry < maxEntry) {
				Entry e = { 0 };

				memcpy(&e.diDeviceInstance, lpddi, sizeof(e.diDeviceInstance));
				e.diDevCaps.dwSize = sizeof(e.diDevCaps);

				LPDIRECTINPUTDEVICE8	did = 0;

				if(SUCCEEDED(callback.di->CreateDevice(lpddi->guidInstance, (LPDIRECTINPUTDEVICE*) &did, 0))) {
					if(SUCCEEDED(did->SetDataFormat(callback.lpdf))) {
						if(SUCCEEDED(did->GetCapabilities(&e.diDevCaps))) {
							e.diDevice = did;
							entry[nEntry++] = e;
						}
					}
				}
			}
			return DIENUM_CONTINUE;
		}

		//
		Entry*			entry;
		int				maxEntry;
		int				nEntry;
		struct {
			LPDIRECTINPUT	di;
			LPCDIDATAFORMAT lpdf;
		} callback;
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
//	ジョイスティック初期化
//	※ApplyCfgから呼び出す場合、dwDeviceが違っていた場合のみにすること
//
//---------------------------------------------------------------------------
static DiJoyStick djs;

void FASTCALL CInput::InitJoy()
{
	if(m_lpDI) {
		djs.enumerate(m_lpDI);
	}
}

//---------------------------------------------------------------------------
//
//	ジョイスティック入力
//
//---------------------------------------------------------------------------
void FASTCALL CInput::InputJoy(BOOL bEnable)
{
	PPI::joyinfo_t ji[PPI::PortMax] = { 0 };

	djs.update();

	const DiJoyStick::Entry* e = djs.getEntry(0);
	if(e) {
		const DIJOYSTATE2* js = &e->joystate;

		int	axisX	= 0;
		int axisY	= 0;
		int btn0	= 0;
		int btn1	= 0;

		{
			int pov = js->rgdwPOV[0];
			if(pov >= 0) {
				static const int V = 0x7ff;
				switch(pov / 4500) {
				default:
				case 0:		axisX	= 0;	axisY	= -V;	break;
				case 1:		axisX	= +V;	axisY	= -V;	break;
				case 2:		axisX	= +V;	axisY	= 0;	break;
				case 3:		axisX	= +V;	axisY	= +V;	break;
				case 4:		axisX	= 0;	axisY	= +V;	break;
				case 5:		axisX	= -V;	axisY	= +V;	break;
				case 6:		axisX	= -V;	axisY	= 0;	break;
				case 7:		axisX	= -V;	axisY	= -V;	break;
				}
			}
		}

		{
			btn0 = js->rgbButtons[0];
			btn1 = js->rgbButtons[1];
		}

		PPI::joyinfo_t* o = &ji[0];
		o->axis[0]		= axisX;
		o->axis[1]		= axisY;
		o->button[0]	= btn0;
		o->button[1]	= btn1;
	}

	// PPIへ送信
	for(int i=0; i<PPI::PortMax; i++) {
		m_pPPI->SetJoyInfo(i, &ji[i]);
	}
}
#endif	// _WIN32
