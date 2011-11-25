//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC スケジューラ ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "cpu.h"
#include "mouse.h"
#include "render.h"
#include "config.h"
#include "fileio.h"

#include "mfc_com.h"
#include "mfc_frm.h"
#include "mfc_draw.h"
#include "mfc_sch.h"

#include "crtc.h"
#include "keyboard.h"
#include "mouse.h"
#include "ppi.h"

#include "opmif.h"
#include "opm.h"
#include "adpcm.h"
#include "scsi.h"
#include "mfc_asm.h"

//===========================================================================
//
//	スケジューラ
//
//===========================================================================
static DWORD FASTCALL GetTime() {
	return timeGetTime();
}

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CScheduler::CScheduler(CFrmWnd *pFrmWnd) : CComponent(pFrmWnd)
{
	// コンポーネントパラメータ
	m_dwID = MAKEID('S', 'C', 'H', 'E');
	m_strDesc = _T("Scheduler");

	// ワーク初期化
	m_pThread		= NULL;
	m_bExitReq		= FALSE;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CScheduler::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!CComponent::Init()) {
		return FALSE;
	}

	// マルチメディアタイマーの時間間隔を1msに設定
	::timeBeginPeriod(1);

	// スレッドを立てる
	m_pThread = AfxBeginThread(ThreadFunc, this);
	if (!m_pThread) {
		::timeEndPeriod(1);
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL CScheduler::Cleanup()
{
	ASSERT(this);
	ASSERT_VALID(this);

	// 停止
	{
		ASSERT(this);
		ASSERT_VALID(this);

		// スレッドが上がっている場合のみ終了処理
		if (m_pThread) {
			// 終了リクエストを立てる
			m_bExitReq = TRUE;

			// 停止まで待つ
			::WaitForSingleObject(m_pThread->m_hThread, INFINITE);

			// スレッドは終了した
			m_pThread = NULL;
		}
	}

	// マルチメディアタイマーの時間間隔を戻す
	::timeEndPeriod(1);

	// 基本クラス
	CComponent::Cleanup();
}

#if defined(_DEBUG)
//---------------------------------------------------------------------------
//
//	診断
//
//---------------------------------------------------------------------------
void CScheduler::AssertValid() const
{
	ASSERT(this);
	ASSERT(GetID() == MAKEID('S', 'C', 'H', 'E'));
}
#endif	// _DEBUG

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL CScheduler::Reset()
{
	ASSERT(this);
	ASSERT_VALID(this);
}

//---------------------------------------------------------------------------
//
//	スレッド関数
//
//---------------------------------------------------------------------------
UINT CScheduler::ThreadFunc(LPVOID pParam)
{
	// パラメータを受け取る
	CScheduler *pSch = (CScheduler*)pParam;
	ASSERT(pSch);
#if defined(_DEBUG)
	pSch->AssertValid();
#endif	// _DEBUG

	// 実行
	pSch->Run();

	// 終了コードを持ってスレッドを終了
	return 0;
}



//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
static DiJoyStick djs;
static KeyMapTargetToX68k km;



//---------------------------------------------------------------------------
//
//	
//
//---------------------------------------------------------------------------
static void processInput(BOOL bRun, HWND hWnd) {
	static LPDIRECTINPUT	lpDi		= 0;
	static CRTC*			m_pCRTC;		// CRTC
	static DWORD			m_dwDispCount;	// CRTC表示カウント
	static Keyboard*		m_pKeyboard;	// キーボード
	static Mouse*			m_pMouse;		// マウス
	static PPI*				m_pPPI;			// PPI
	static BOOL				m_bEnable	= TRUE;

	if(lpDi == 0) {
		m_dwDispCount	= 0;

		m_pCRTC		= (CRTC*)		::GetVM()->SearchDevice(MAKEID('C', 'R', 'T', 'C'));
		m_pKeyboard	= (Keyboard*)	::GetVM()->SearchDevice(MAKEID('K', 'E', 'Y', 'B'));
		m_pMouse	= (Mouse*)		::GetVM()->SearchDevice(MAKEID('M', 'O', 'U', 'S'));
		m_pPPI		= (PPI*)		::GetVM()->SearchDevice(MAKEID('P', 'P', 'I', ' '));

		DirectInput8Create(AfxGetApp()->m_hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**) &lpDi, 0);

		djs.enumerate(lpDi);

		//	keyboard
		for(int i = 0, n = sizeof(keyEntry)/sizeof(keyEntry[0]); i < n; ++i) {
			const KeyEntry& e = keyEntry[i];
			if(e.targetKeycode && e.x68kKeycode) {
				km.set(e.targetKeycode, e.x68kKeycode);
			}
		}
	}

	if(m_bEnable) {
		int updateMode = TRUE;

		// bRun = FALSEなら、スケジューラ停止中(10msおきに呼ばれる)
		if (!bRun) {
			updateMode = FALSE;
		} else {
			// CRTCの表示カウンタを見て、フレームごとに処理する
			ASSERT(m_pCRTC);

			DWORD dwDispCount = m_pCRTC->GetDispCount();
			if (dwDispCount == m_dwDispCount) {
				updateMode = -1;
			} else {
				m_dwDispCount = dwDispCount;
			}
		}

		if(updateMode >= 0) {
			BOOL bEnable = updateMode;

			if(m_pKeyboard) {
				//
				//	update
				//
				static BYTE prevKeys[256];
				BYTE keys[256];

				if(bEnable) {
					for(int i = 0; i < 256; ++i) {
						BYTE k = 0;
						if(GetAsyncKeyState(i) & 0x8000) {
							k = 1;
						}
						keys[i] = k;
					}
				} else {
					memset(&keys[0], 0, sizeof(keys));
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

			if(m_pMouse) {
//				ASSERT(this);
//				ASSERT(m_pFrmWnd);
//				ASSERT(m_pMouse);
//				ASSERT_VALID(this);

				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(hWnd, &pt);
				int	nMouseX		= pt.x;
				int	nMouseY		= pt.y;
				int bMouseB0	= (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? 1 : 0;
				int bMouseB1	= (GetAsyncKeyState(VK_RBUTTON) & 0x8000) ? 1 : 0;
				int bMouseMode	= GetAsyncKeyState(VK_RSHIFT) & 0x8000;

				// マウスモードONか
				if(!bMouseMode || !bEnable) {
					// ボタンUPを通知
					m_pMouse->SetMouse(nMouseX, nMouseY, FALSE, FALSE);
				} else {
					m_pMouse->SetMouse(nMouseX, nMouseY, bMouseB0, bMouseB1);
				}
			}

			djs.update();
			if(m_pPPI) {
				PPI::joyinfo_t ji[PPI::PortMax] = { 0 };

				if(bEnable) {
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
				}

				// PPIへ送信
				for(int i=0; i<PPI::PortMax; i++) {
					m_pPPI->SetJoyInfo(i, &ji[i]);
				}
			}
		}
	}
}



//---------------------------------------------------------------------------
//
//	進行
//
//---------------------------------------------------------------------------
static void processSound(BOOL bRun, HWND hWnd) {
	HRESULT hr;
	DWORD dwOffset;
	DWORD dwWrite;
	DWORD dwRequest;
	DWORD dwReady;
	WORD *pBuf1;
	WORD *pBuf2;
	DWORD dwSize1;
	DWORD dwSize2;

//	ASSERT(this);

	static BOOL			m_bEnable	= TRUE;

	// デバイス
	static LPGUID m_lpGUID[16];				// DirectSoundデバイスのGUID
	static int m_nDeviceNum;				// 検出したデバイス数

	// 再生
	static UINT m_uRate;					// サンプリングレート
	static UINT m_uTick;					// バッファサイズ(ms)
	static UINT m_uPoll;					// ポーリング間隔(ms)
	static UINT m_uCount;					// ポーリングカウント
	static UINT m_uBufSize;					// バッファサイズ(バイト)
	static BOOL m_bPlay;					// 再生フラグ
	static DWORD m_dwWrite;					// 書き込み完了位置
	static int m_nMaster;					// マスタ音量
	static int m_nFMVol;					// FM音量(0～100)
	static int m_nADPCMVol;					// ADPCM音量(0～100)
	static LPDIRECTSOUND m_lpDS;			// DirectSound
	static LPDIRECTSOUNDBUFFER m_lpDSp;		// DirectSoundBuffer(プライマリ)
	static LPDIRECTSOUNDBUFFER m_lpDSb;		// DirectSoundBuffer(セカンダリ)
	static DWORD *m_lpBuf;					// サウンドバッファ

	static OPMIF*		m_pOPMIF;			// OPMインタフェース
	static ADPCM*		m_pADPCM;			// ADPCM
	static SCSI*		m_pSCSI;			// SCSI
	static FM::OPM*		m_pOPM;				// OPMデバイス
	static Scheduler*	m_pScheduler = 0;	// スケジューラ

	if(m_pScheduler == 0) {
		// スケジューラ取得
		m_pScheduler = (Scheduler*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
		ASSERT(m_pScheduler);

		// OPMIF取得
		m_pOPMIF = (OPMIF*)::GetVM()->SearchDevice(MAKEID('O', 'P', 'M', ' '));
		ASSERT(m_pOPMIF);

		// ADPCM取得
		m_pADPCM = (ADPCM*)::GetVM()->SearchDevice(MAKEID('A', 'P', 'C', 'M'));
		ASSERT(m_pADPCM);

		// SCSI取得
		m_pSCSI = (SCSI*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'S', 'I'));
		ASSERT(m_pSCSI);

		// デバイス列挙
	//	EnumDevice();
		{
			//
			//	DirectSound Device enumerator
			//
			class DsEnumerator {
				enum {
					ENTRY_MAX	= 16,
				};

			public:
				struct Entry {
					LPGUID	lpGuid;
					LPCSTR	lpcstrDescription;
					LPCSTR	lpcstrModule;
					LPVOID	lpContext;
				};

				DsEnumerator() : nEntry(0) {
				}

				void enumerate() {
					nEntry = 0;
					DirectSoundEnumerate(DSEnumCallback_static, this);
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

			protected:
				static BOOL CALLBACK DSEnumCallback_static(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext) {
					return reinterpret_cast<DsEnumerator*>(lpContext)->DSEnumCallback(lpGuid, lpcstrDescription, lpcstrModule, lpContext);
				}

				BOOL DSEnumCallback(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext) {
					if(nEntry < ENTRY_MAX) {
						///	@note	bad code. fix this.
						//	See "Remarks" section :
						//		DSEnumCallback
						//		http://msdn.microsoft.com/en-us/library/windows/desktop/microsoft.directx_sdk.reference.dsenumcallback(v=vs.85).aspx
						//	Callback argument memories are local memory.
						//	We must do some allocations for save these variables.
						Entry& e = entry[nEntry++];
						e.lpGuid			= lpGuid;
						e.lpcstrDescription	= lpcstrDescription;
						e.lpcstrModule		= lpcstrModule;
						e.lpContext			= lpContext;
					}
					return TRUE;
				}

				//
				int		nEntry;
				Entry	entry[ENTRY_MAX];
			};

			DsEnumerator de;
			de.enumerate();
			m_nDeviceNum = de.getEntryCount();
			for(int i = 0; i < m_nDeviceNum; ++i) {
				m_lpGUID[i] = de.getEntry(i)->lpGuid;
			}
		}

		m_bEnable = TRUE;

		// ここでは初期化しない(ApplyCfgに任せる)

		//VC2010//	面倒なのでここで初期化
		{
			int m_nSelectDevice		= 0;
			m_uRate				= 44100;
			m_uTick				= 100;

	//		InitSub();
			{
				bool b = true;

				// rate==0なら、何もしない
				if(b && m_uRate == 0) {
					b = false;
				}

				if(b) {
					ASSERT(!m_lpDS);
					ASSERT(!m_lpDSp);
					ASSERT(!m_lpDSb);
					ASSERT(!m_lpBuf);
					ASSERT(!m_pOPM);

					// デバイスがなければ0で試し、それでもなければreturn
					if (m_nDeviceNum <= m_nSelectDevice) {
						if (m_nDeviceNum == 0) {
							b = false;
						} else {
							m_nSelectDevice = 0;
						}
					}
				}

				// DiectSoundオブジェクト作成
				if(b && FAILED(DirectSoundCreate(m_lpGUID[m_nSelectDevice], &m_lpDS, NULL))) {
					// デバイスは使用中
					b = false;
				}

				// 協調レベルを設定(優先協調)
				if(b && FAILED(m_lpDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY))) {
					b = false;
				}

				// プライマリバッファを作成
				if(b) {
					DSBUFFERDESC dsbd = { 0 };
					dsbd.dwSize		= sizeof(dsbd);
					dsbd.dwFlags	= DSBCAPS_PRIMARYBUFFER;

					b = SUCCEEDED(m_lpDS->CreateSoundBuffer(&dsbd, &m_lpDSp, NULL));
				}

				// プライマリバッファのフォーマットを指定
				if(b) {
					WAVEFORMATEX wfex = { 0 };
					wfex.wFormatTag			= WAVE_FORMAT_PCM;
					wfex.nChannels			= 2;
					wfex.nSamplesPerSec		= m_uRate;
					wfex.nBlockAlign		= 4;
					wfex.nAvgBytesPerSec	= wfex.nSamplesPerSec * wfex.nBlockAlign;
					wfex.wBitsPerSample		= 16;

					b = SUCCEEDED(m_lpDSp->SetFormat(&wfex));
				}

				// セカンダリバッファを作成
				if(b) {
					PCMWAVEFORMAT pcmwf = { 0 };
					pcmwf.wf.wFormatTag			= WAVE_FORMAT_PCM;
					pcmwf.wf.nChannels			= 2;
					pcmwf.wf.nSamplesPerSec		= m_uRate;
					pcmwf.wf.nBlockAlign		= 4;
					pcmwf.wf.nAvgBytesPerSec	= pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign;
					pcmwf.wBitsPerSample		= 16;

					DSBUFFERDESC dsbd = { 0 };
					dsbd.dwSize					= sizeof(dsbd);
					dsbd.dwFlags				= DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME;
					dsbd.dwBufferBytes			= (pcmwf.wf.nAvgBytesPerSec * m_uTick) / 1000;
					dsbd.dwBufferBytes			= ((dsbd.dwBufferBytes + 7) >> 3) << 3;	// 8バイト境界
					dsbd.lpwfxFormat			= (LPWAVEFORMATEX)&pcmwf;

					m_uBufSize					= dsbd.dwBufferBytes;

					b = SUCCEEDED(m_lpDS->CreateSoundBuffer(&dsbd, &m_lpDSb, NULL));
				}

				// サウンドバッファを作成(セカンダリバッファと同一の長さ、1単位DWORD)
				if(b) {
					m_lpBuf = new DWORD [ m_uBufSize / 2 ];
					memset(m_lpBuf, sizeof(DWORD) * (m_uBufSize / 2), m_uBufSize);

					// OPMデバイス(標準)を作成
					m_pOPM = new FM::OPM;
					m_pOPM->Init(4000000, m_uRate, true);
					m_pOPM->Reset();
					m_pOPM->SetVolume(m_nFMVol);

					// OPMIFへ通知
					m_pOPMIF->InitBuf(m_uRate);
					m_pOPMIF->SetEngine(m_pOPM);

					// イネーブルなら演奏開始
				//	if (m_bEnable)
					{
//						Play();
						//
						m_lpDSb->Play(0, 0, DSBPLAY_LOOPING);
						m_bPlay = TRUE;
						m_uCount = 0;
						m_dwWrite = 0;
					}
				}
			}

			// 常に設定
			if (m_pOPM) {
				{
					int lVolume = 100;		//pConfig->master_volume;
					lVolume = 100 - lVolume;
					lVolume *= (DSBVOLUME_MAX - DSBVOLUME_MIN);
					lVolume /= -200;
					m_lpDSb->SetVolume(lVolume);
				}
				m_pOPMIF->EnableFM(1);		//pConfig->fm_enable);
				m_pOPM->SetVolume(54);		//pConfig->fm_volume);
				m_pADPCM->EnableADPCM(1);	//pConfig->adpcm_enable);
				m_pADPCM->SetVolume(52);	//pConfig->adpcm_volume);
			}
			m_nMaster	= 100;	//pConfig->master_volume;
			m_uPoll		= 5;	//(UINT)pConfig->polling_buffer;
		}
	}

	// カウント処理(m_nPoll回に１回、ただしVM停止中は常時)
	m_uCount++;
	if ((m_uCount < m_uPoll) && bRun) {
		return;
	}
	m_uCount = 0;

	// ディセーブルなら、何もしない
	if (!m_bEnable) {
		return;
	}

	// 初期化されていなければ、何もしない
	if (!m_pOPM) {
		m_pScheduler->SetSoundTime(0);
		return;
	}

	// プレイ状態でなければ、関係なし
	if (!m_bPlay) {
		m_pScheduler->SetSoundTime(0);
		return;
	}

	// 現在のプレイ位置を得る(バイト単位)
	ASSERT(m_lpDSb);
	ASSERT(m_lpBuf);
	if (FAILED(m_lpDSb->GetCurrentPosition(&dwOffset, &dwWrite))) {
		return;
	}
	ASSERT(m_lpDSb);
	ASSERT(m_lpBuf);

	// 前回書き込んだ位置から、空きサイズを計算(バイト単位)
	if (m_dwWrite <= dwOffset) {
		dwRequest = dwOffset - m_dwWrite;
	}
	else {
		dwRequest = m_uBufSize - m_dwWrite;
		dwRequest += dwOffset;
	}

	// 空きサイズが全体の1/4を超えていなければ、次の機会に
	if (dwRequest < (m_uBufSize / 4)) {
		return;
	}

	// 空きサンプルに換算(L,Rで1つと数える)
	ASSERT((dwRequest & 3) == 0);
	dwRequest /= 4;

	// m_lpBufにバッファデータを作成。まずbRunチェック
	if (!bRun) {
		memset(m_lpBuf, 0, m_uBufSize * 2);
		m_pOPMIF->InitBuf(m_uRate);
	}
	else {
		// OPMに対して、処理要求と速度制御
		dwReady = m_pOPMIF->ProcessBuf();
		m_pOPMIF->GetBuf(m_lpBuf, (int)dwRequest);
		if (dwReady < dwRequest) {
			dwRequest = dwReady;
		}

		// ADPCMに対して、データを要求(加算すること)
		m_pADPCM->GetBuf(m_lpBuf, (int)dwRequest);

		// ADPCMの同期処理
		if (dwReady > dwRequest) {
			m_pADPCM->Wait(dwReady - dwRequest);
		}
		else {
			m_pADPCM->Wait(0);
		}

		// SCSIに対して、データを要求(加算すること)
		m_pSCSI->GetBuf(m_lpBuf, (int)dwRequest, m_uRate);
	}

	// 次いでロック
	hr = m_lpDSb->Lock(m_dwWrite, (dwRequest * 4),
						(void**)&pBuf1, &dwSize1,
						(void**)&pBuf2, &dwSize2,
						0);
	// バッファが失われていれば、リストア
	if (hr == DSERR_BUFFERLOST) {
		m_lpDSb->Restore();
	}
	// ロック成功しなければ、続けても意味がない
	if (FAILED(hr)) {
		m_dwWrite = dwOffset;
		return;
	}

	// 量子化bit=16を前提とする
	ASSERT((dwSize1 & 1) == 0);
	ASSERT((dwSize2 & 1) == 0);

	// MMX命令によるパック(dwSize1+dwSize2で、平均5000～15000程度は処理する)
	SoundMMX(m_lpBuf, pBuf1, dwSize1);
	if (dwSize2 > 0) {
		SoundMMX(&m_lpBuf[dwSize1 / 2], pBuf2, dwSize2);
	}
	SoundEMMS();

	// アンロック
	m_lpDSb->Unlock(pBuf1, dwSize1, pBuf2, dwSize2);

	// m_dwWrite更新
	m_dwWrite += dwSize1;
	m_dwWrite += dwSize2;
	if (m_dwWrite >= m_uBufSize) {
		m_dwWrite -= m_uBufSize;
	}
	ASSERT(m_dwWrite < m_uBufSize);
}

//---------------------------------------------------------------------------
//
//	実行
//
//---------------------------------------------------------------------------
void FASTCALL CScheduler::Run()
{
	VM *pVM;
	Scheduler *pScheduler;
	Render *pRender;
	DWORD dwExecCount;

	ASSERT(this);
	ASSERT_VALID(this);

	// VM取得
	pVM = ::GetVM();
	ASSERT(pVM);
	pScheduler = (Scheduler*)pVM->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(pScheduler);
	pRender = (Render*)pVM->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
	ASSERT(pRender);

	// 時間カウンタ
	DWORD m_dwExecTime = GetTime();
	dwExecCount = 0;
	BOOL m_bBackup = m_bEnable;

	// 終了リクエストが上がるまでループ
	while (!m_bExitReq) {
		// 常時診断
		ASSERT_VALID(this);

		int	preSleep	= 0;
		int postSleep	= -1;

		if(preSleep >= 0) {
			::Sleep(preSleep);
		}

		bool requestRefresh	= false;

		::LockVM();

		// 有効フラグが上がっていなければ、停止中
		if (!m_bEnable) {
			// ブレークポイント、電源停止などで有効→無効になったら、必ず再描画
			if (m_bBackup) {
				m_pFrmWnd->GetView()->Invalidate(FALSE);
				m_bBackup = FALSE;
			}

			// 描画
			requestRefresh = true;
			dwExecCount = 0;

			// 他コンポーネントの処理、時間あわせ
			processSound(FALSE, m_pFrmWnd->m_hWnd);
			processInput(FALSE, m_pFrmWnd->m_hWnd);
			m_dwExecTime = GetTime();
			postSleep = 10;
		} else {
			DWORD dwTime = GetTime();
			if(m_dwExecTime > dwTime) {
				requestRefresh = true;
				dwExecCount = 0;

				if(m_dwExecTime > GetTime()) {
					postSleep = 1;
				}
			} else {
				// レンダリング可否を判定(1or36)
				if (m_dwExecTime >= dwTime) {
					pRender->EnableAct(TRUE);
				} else {
					pRender->EnableAct(FALSE);
				}

				if(pVM->Exec(1000 * 2)) {
					if (pVM->IsPower()) {
						dwExecCount++;
						m_dwExecTime++;

						// 他コンポーネントの処理
						processSound(TRUE, m_pFrmWnd->m_hWnd);
						processInput(TRUE, m_pFrmWnd->m_hWnd);

						// dwExecCountが規定数を超えたら、一度表示して強制時間合わせ
						if (dwExecCount > 400) {
							requestRefresh = true;
							dwExecCount = 0;
							m_dwExecTime = GetTime();
						}
					}
				}
			}
		}

		if(requestRefresh) {
			ASSERT(this);
			ASSERT_VALID(this);
			ASSERT(m_pFrmWnd);

			// ビューを取得
			CDrawView *pView = m_pFrmWnd->GetView();
			ASSERT(pView);

			int m_nSubWndDisp = -1;

			bool skip = false;

			if (m_bEnable) {
				// 実行中でメイン画面の番か
				if (m_nSubWndDisp < 0) {
					// レンダラの準備ができていなければ描画しない
					if (!pRender->IsReady()) {
						skip = true;
					}
				}
			}

			if(!skip) {
				// 表示(一部)
				pView->Draw(m_nSubWndDisp);

				// メイン画面表示なら、カウントダウン
				if (m_nSubWndDisp < 0) {
					pRender->Complete();
				}
			}
		}

		::UnlockVM();

		if(postSleep >= 0) {
			::Sleep(postSleep);
		}
	}
}
#endif	// _WIN32
