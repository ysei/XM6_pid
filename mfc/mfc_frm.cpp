//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC フレームウィンドウ ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "schedule.h"
#include "memory.h"
#include "sasi.h"
#include "scsi.h"
#include "fdd.h"
#include "fdc.h"
#include "fdi.h"
#include "render.h"
#include "fileio.h"
#include "mfc_frm.h"
#include "mfc_draw.h"
#include "mfc_res.h"

#include "crtc.h"
#include "keyboard.h"
#include "mouse.h"
#include "ppi.h"

#include "opmif.h"
#include "opm.h"
#include "adpcm.h"
#include "scsi.h"
#include "mfc_asm.h"

#include "config.h"
#include "sram.h"

//===========================================================================
//
//	フレームウィンドウ
//
//===========================================================================

CFrmWnd*	globalFrmWnd	= 0;

//---------------------------------------------------------------------------
//
//	scheduler
//
//---------------------------------------------------------------------------
static volatile	BOOL		scheduler_mm_bEnable = FALSE;
static volatile BOOL		scheduler_m_bExitReq = FALSE;			// スレッド終了要求

BOOL schedulerIsEnable() {
	return scheduler_mm_bEnable;
}

void schedulerSetEnable(BOOL b) {
	scheduler_mm_bEnable = b;
}

static DWORD FASTCALL GetTime() {
	return timeGetTime();
}

void configGetConfig(Config* c) {
	//	Config200
	// システム
	c->system_clock			= 5;					// システムクロック(0～5)
	c->ram_size				= 0;						// メインRAMサイズ(0～5)
	c->ram_sramsync			= TRUE;					// メモリスイッチ自動更新

	// スケジューラ
	c->mpu_fullspeed		= FALSE;					// MPUフルスピード
	c->vm_fullspeed			= FALSE;					// VMフルスピード

	// サウンド
	c->sound_device			= 0;					// サウンドデバイス(0～15)
	c->sample_rate			= 5;					// サンプリングレート(0～4)
	c->primary_buffer		= 10;					// バッファサイズ(2～100)
	c->polling_buffer		= 5;					// ポーリング間隔(0～99)
	c->adpcm_interp			= TRUE;					// ADPCM線形補間あり

	// 描画
	c->aspect_stretch		= TRUE;				// アスペクト比にあわせ拡大

	// 音量
	c->master_volume		= 100;					// マスタ音量(0～100)
	c->fm_enable			= TRUE;						// FM有効
	c->fm_volume			= 54;						// FM音量(0～100)
	c->adpcm_enable			= TRUE;					// ADPCM有効
	c->adpcm_volume			= 52;					// ADPCM音量(0～100)

	// キーボード
	c->kbd_connect			= TRUE;					// 接続

	// マウス
	c->mouse_speed			= 205;					// スピード
	c->mouse_port			= 1;						// 接続ポート
	c->mouse_swap			= FALSE;					// ボタンスワップ
	c->mouse_mid			= TRUE;						// 中ボタンイネーブル
	c->mouse_trackb			= FALSE;					// トラックボールモード

	// ジョイスティック
	c->joy_type[0]			= 1;					// ジョイスティックタイプ
	c->joy_type[1]			= 1;					// ジョイスティックタイプ
	c->joy_dev[0]			= 1;						// ジョイスティックデバイス
	c->joy_dev[1]			= 2;						// ジョイスティックデバイス
	c->joy_button0[0]		= 1;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[1]		= 2;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[2]		= 3;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[3]		= 4;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[4]		= 5;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[5]		= 6;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[6]		= 7;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[7]		= 8;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[8]		= 0;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[9]		= 0;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[10]		= 0;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[11]		= 0;				// ジョイスティックボタン(デバイスA)
	c->joy_button1[0]		= 65537;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[1]		= 65538;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[2]		= 65539;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[3]		= 65540;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[4]		= 65541;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[5]		= 65542;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[6]		= 65543;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[7]		= 65544;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[8]		= 0;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[9]		= 0;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[10]		= 0;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[11]		= 0;				// ジョイスティックボタン(デバイスB)

	// SASI
	c->sasi_drives			= 1;					// SASIドライブ数
	c->sasi_sramsync		= TRUE;					// SASIメモリスイッチ自動更新
	strcpy(&c->sasi_file[ 0][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD0.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[ 1][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD1.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[ 2][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD2.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[ 3][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD3.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[ 4][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD4.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[ 5][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD5.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[ 6][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD6.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[ 7][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD7.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[ 8][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD8.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[ 9][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD9.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[10][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD10.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[11][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD11.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[12][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD12.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[13][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD13.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[14][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD14.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[15][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD15.HDF"));				// SASIイメージファイル

	// SxSI
	c->sxsi_drives			= 0;							// SxSIドライブ数
	c->sxsi_mofirst			= FALSE;						// MOドライブ優先割り当て
	memset(&c->sxsi_file[0][0], 0, sizeof(c->sxsi_file));	// SxSIイメージファイル

	// ポート
	c->port_com				= 0;								// COMxポート
	memset(&c->port_recvlog[0], 0, sizeof(c->port_recvlog));	// シリアル受信ログ
	c->port_384				= FALSE;							// シリアル38400bps固定
	c->port_lpt				= 0;								// LPTxポート
	memset(&c->port_sendlog[0], 0, sizeof(c->port_sendlog));	// パラレル送信ログ

	// MIDI
	c->midi_bid				= 0;							// MIDIボードID
	c->midi_ilevel			= 0;							// MIDI割り込みレベル
	c->midi_reset			= 0;							// MIDIリセットコマンド
	c->midiin_device		= 0;							// MIDI INデバイス
	c->midiin_delay			= 0;							// MIDI INディレイ(ms)
	c->midiout_device		= 0;							// MIDI OUTデバイス
	c->midiout_delay		= 84;							// MIDI OUTディレイ(ms)

	// 改造
	c->sram_64k				= FALSE;						// 64KB SRAM
	c->scc_clkup			= FALSE;						// SCCクロックアップ
	c->power_led			= FALSE;						// 青色電源LED
	c->dual_fdd				= FALSE;						// 2DD/2HD兼用FDD
	c->sasi_parity			= FALSE;						// SASIバスパリティ

	// TrueKey
	c->tkey_mode			= 1;							// TrueKeyモード(bit0:VM bit1:WinApp)
	c->tkey_com				= 0;							// キーボードCOMポート
	c->tkey_rts				= FALSE;						// RTS反転モード

	// その他
	c->floppy_speed			= TRUE;							// フロッピーディスク高速
	c->floppy_led			= TRUE;							// フロッピーディスクLEDモード
	c->popup_swnd			= TRUE;							// ポップアップサブウィンドウ
	c->auto_mouse			= FALSE;						// 自動マウスモード制御
	c->power_off			= FALSE;						// 電源OFFで開始

	//	Config202
	// システム
	c->mem_type				= 1;		// メモリマップ種別

	// SCSI
	c->scsi_ilevel			= 1;		// SCSI割り込みレベル
	c->scsi_drives			= 0;		// SCSIドライブ数
	c->scsi_sramsync		= 1;		// SCSIメモリスイッチ自動更新
	c->scsi_mofirst			= 0;		// MOドライブ優先割り当て
	memset(&c->scsi_file[0][0], 0, sizeof(c->scsi_file));	// SCSIイメージファイル

	//	Config
	// レジューム
	c->resume_fd			= FALSE;	// FDレジューム
	c->resume_fdi[0]		= TRUE;		// FD挿入フラグ
	c->resume_fdi[1]		= FALSE;	// FD挿入フラグ
	c->resume_fdw[0]		= FALSE;	// FD書き込み禁止
	c->resume_fdw[1]		= FALSE;	// FD書き込み禁止
	c->resume_fdm[0]		= 0;		// FDメディアNo.
	c->resume_fdm[1]		= 0;		// FDメディアNo.
	c->resume_mo			= 0;		// MOレジューム
	c->resume_mos			= 0;		// MO挿入フラグ
	c->resume_mow			= 0;		// MO書き込み禁止
	c->resume_cd			= 0;		// CDレジューム
	c->resume_iso			= 0;		// CD挿入フラグ
	c->resume_state			= 0;		// ステートレジューム
	c->resume_xm6			= 0;		// ステート有効フラグ
	c->resume_screen		= 0;		// 画面モードレジューム
	c->resume_dir			= 0;		// デフォルトディレクトリレジューム
	strcpy(c->resume_path, _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\"));

	// 描画
	c->caption_info			= 1;		// キャプション情報表示

	// ディスプレイ
	c->caption				= 1;		// キャプション
	c->menu_bar				= 1;		// メニューバー
	c->status_bar			= 1;		// ステータスバー
	c->window_left			= 543;		// ウィンドウ矩形
	c->window_top			= 231;		// ウィンドウ矩形
	c->window_full			= 0;		// フルスクリーン
	c->window_mode			= 0;		// ワイドスクリーン

	// WINDRVモジュール
	c->windrv_enable		= 0;		// Windrvサポート 0:無効 1:WindrvXM (2:Windrv互換)

	// ホスト側ファイルシステム
	c->host_option			= 0;		// 動作フラグ (class CHostFilename 参照)
	c->host_resume			= FALSE;	// ベースパス状態復元有効 FALSEだと毎回スキャンする
	c->host_drives			= 0;		// 有効なドライブ数
	memset(&c->host_flag[0], 0, sizeof(c->host_flag));		// 動作フラグ (class CWinFileDrv 参照)
	memset(&c->host_path[0][0], 0, sizeof(c->host_path));		// ベースパス
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

UINT ThreadFunc(LPVOID pParam) {
	extern CFrmWnd*	globalFrmWnd;
	CFrmWnd*	m_pFrmWnd	= globalFrmWnd;

	VM*			pVM			= ::GetVM();
	Render*		pRender		= (Render*)pVM->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
	HWND		hFrmWnd		= m_pFrmWnd->m_hWnd;
	CDrawView*	pDrawView	= m_pFrmWnd->GetView();
	DWORD		dwExecTime	= GetTime();
	DWORD		dwExecCount	= 0;

	// 終了リクエストが上がるまでループ
	while (!scheduler_m_bExitReq) {
		int	preSleep	= 0;
		int postSleep	= -1;

		if(preSleep >= 0) {
			::Sleep(preSleep);
		}

		bool requestRefresh	= false;

		::LockVM();

		// 有効フラグが上がっていなければ、停止中
		if(! schedulerIsEnable()) {
			// 描画
			requestRefresh = true;
			dwExecCount = 0;

			// 他コンポーネントの処理、時間あわせ
			processSound(FALSE, hFrmWnd);
			processInput(FALSE, hFrmWnd);
			dwExecTime = GetTime();
			postSleep = 10;
		} else {
			DWORD dwTime = GetTime();
			if(dwExecTime > dwTime) {
				requestRefresh = true;
				dwExecCount = 0;
				if(dwExecTime > GetTime()) {
					postSleep = 1;
				}
			} else {
				// レンダリング可否を判定(1or36)
				pRender->EnableAct(dwExecTime >= dwTime);

				if(pVM->Exec(1000 * 2)) {
					if(pVM->IsPower()) {
						dwExecCount++;
						dwExecTime++;

						// 他コンポーネントの処理
						processSound(TRUE, hFrmWnd);
						processInput(TRUE, hFrmWnd);

						// dwExecCountが規定数を超えたら、一度表示して強制時間合わせ
						if (dwExecCount > 400) {
							requestRefresh = true;
							dwExecCount = 0;
							dwExecTime = GetTime();
						}
					}
				}
			}
		}

		if(requestRefresh) {
			if(! schedulerIsEnable() || pRender->IsReady()) {
				pDrawView->Draw(-1);
				pRender->Complete();
			}
		}

		::UnlockVM();

		if(postSleep >= 0) {
			::Sleep(postSleep);
		}
	}
	return 0;
}

void schedulerInit() {
	scheduler_m_bExitReq		= FALSE;
	scheduler_mm_bEnable		= FALSE;

	::timeBeginPeriod(1);
	AfxBeginThread(ThreadFunc, 0);
}



//---------------------------------------------------------------------------
//
//	シェル定数定義
//	※includeファイルではなく、アプリケーション側で定義するよう定められている
//
//---------------------------------------------------------------------------
#define SHCNRF_InterruptLevel			0x0001
#define SHCNRF_ShellLevel				0x0002
#define SHCNRF_NewDelivery				0x8000

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CFrmWnd::CFrmWnd()
{
	globalFrmWnd = this;

	// VM・ステータスコード
	::pVM = NULL;
	m_nStatus = -1;

	// デバイス
	m_pFDD = NULL;
	m_pSASI = NULL;
	m_pSCSI = NULL;
	m_pScheduler = NULL;
	m_pKeyboard = NULL;
	m_pMouse = NULL;

	// コンポーネント
//	m_pFirstComponent = NULL;
	m_pDrawView = NULL;
//	m_pSch = NULL;
//	m_pSound = NULL;
//	m_pInput = NULL;
//	m_pHost = NULL;
//	m_pConfig = NULL;

	// フルスクリーン
	m_bFullScreen = FALSE;
	m_hTaskBar = NULL;
	memset(&m_DevMode, 0, sizeof(m_DevMode));
	m_nWndLeft = 0;
	m_nWndTop = 0;

	// サブウィンドウ
	m_strWndClsName.Empty();

	// ステータスバー・メニュー・キャプション
	m_bMenuBar = TRUE;

	// シェル通知
	m_uNotifyId = NULL;

	// コンフィギュレーション
	m_bMouseMid = TRUE;
	m_bPopup = FALSE;
	m_bAutoMouse = TRUE;

	// その他変数
	m_bExit = FALSE;
	m_bSaved = FALSE;
	m_nFDDStatus[0] = 0;
	m_nFDDStatus[1] = 0;
	m_dwExec = 0;
}

//---------------------------------------------------------------------------
//
//	メッセージ マップ
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CFrmWnd, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_DISPLAYCHANGE, OnDisplayChange)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_MOVE()
	ON_WM_ACTIVATE()
	ON_WM_ACTIVATEAPP()
	ON_WM_ENTERMENULOOP()
	ON_WM_EXITMENULOOP()
	ON_WM_PARENTNOTIFY()
	ON_MESSAGE(WM_KICK, OnKick)
	ON_WM_DRAWITEM()
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_POWERBROADCAST, OnPowerBroadCast)
	ON_WM_SYSCOMMAND()
#if _MFC_VER >= 0x700
	ON_WM_COPYDATA()
#else
	ON_MESSAGE(WM_COPYDATA, OnCopyData)
#endif
	ON_WM_ENDSESSION()
	ON_MESSAGE(WM_SHELLNOTIFY, OnShellNotify)

/*
	ON_COMMAND(IDM_OPEN, OnOpen)
	ON_UPDATE_COMMAND_UI(IDM_OPEN, OnOpenUI)
	ON_COMMAND(IDM_SAVE, OnSave)
	ON_UPDATE_COMMAND_UI(IDM_SAVE, OnSaveUI)
	ON_COMMAND(IDM_SAVEAS, OnSaveAs)
	ON_UPDATE_COMMAND_UI(IDM_SAVEAS, OnSaveAsUI)
*/
	ON_COMMAND(IDM_RESET, OnReset)
	ON_UPDATE_COMMAND_UI(IDM_RESET, OnResetUI)
/*
	ON_COMMAND(IDM_INTERRUPT, OnInterrupt)
	ON_UPDATE_COMMAND_UI(IDM_INTERRUPT, OnInterruptUI)
	ON_COMMAND(IDM_POWER, OnPower)
	ON_UPDATE_COMMAND_UI(IDM_POWER, OnPowerUI)
	ON_COMMAND(IDM_EXIT, OnExit)
	ON_COMMAND_RANGE(IDM_D0OPEN, IDM_D1_MRU8, OnFD)
*/
	ON_COMMAND_RANGE(IDM_D0OPEN, IDM_D1OPEN, OnFD)
	ON_UPDATE_COMMAND_UI(IDM_D0OPEN, OnFDOpenUI)
	ON_UPDATE_COMMAND_UI(IDM_D1OPEN, OnFDOpenUI)
/*
	ON_UPDATE_COMMAND_UI(IDM_D0EJECT, OnFDEjectUI)
	ON_UPDATE_COMMAND_UI(IDM_D1EJECT, OnFDEjectUI)
	ON_UPDATE_COMMAND_UI(IDM_D0WRITEP, OnFDWritePUI)
	ON_UPDATE_COMMAND_UI(IDM_D1WRITEP, OnFDWritePUI)
	ON_UPDATE_COMMAND_UI(IDM_D0FORCE, OnFDForceUI)
	ON_UPDATE_COMMAND_UI(IDM_D1FORCE, OnFDForceUI)
	ON_UPDATE_COMMAND_UI(IDM_D0INVALID, OnFDInvalidUI)
	ON_UPDATE_COMMAND_UI(IDM_D1INVALID, OnFDInvalidUI)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_D0_MEDIA0, IDM_D0_MEDIAF, OnFDMediaUI)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_D1_MEDIA0, IDM_D1_MEDIAF, OnFDMediaUI)
//	ON_UPDATE_COMMAND_UI_RANGE(IDM_D0_MRU0, IDM_D0_MRU8, OnFDMRUUI)
//	ON_UPDATE_COMMAND_UI_RANGE(IDM_D1_MRU0, IDM_D1_MRU8, OnFDMRUUI)
*/
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL CFrmWnd::Init()
{
	// ウィンドウ作成
	if (!Create(NULL, _T("XM6"),
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
			WS_BORDER | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
			rectDefault, NULL, NULL, 0, NULL)) {
		return FALSE;
	}

	// それ以外の初期化はOnCrateに任せる
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ウィンドウ作成準備
//
//---------------------------------------------------------------------------
BOOL CFrmWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	// 基本クラス
	if (!CFrameWnd::PreCreateWindow(cs)) {
		return FALSE;
	}

	// クライアントエッジを外す
	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ウィンドウ作成
//
//---------------------------------------------------------------------------
int CFrmWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	LONG lUser;
	CMenu *pSysMenu;
	UINT nCount;
	CString string;

	// 基本クラス
	if (CFrameWnd::OnCreate(lpCreateStruct) != 0) {
		return -1;
	}

	// ユーザデータ指定
	lUser = (LONG)MAKEID('X', 'M', '6', ' ');
	::SetWindowLong(m_hWnd, GWL_USERDATA, lUser);

	// アクセラレータ指定、アイコン指定、IMM指定
	LoadAccelTable(MAKEINTRESOURCE(IDR_ACCELERATOR));
	SetIcon(AfxGetApp()->LoadIcon(IDI_APPICON), TRUE);
	::ImmAssociateContext(m_hWnd, (HIMC)NULL);

	// メニュー(ウィンドウ)
	if (::IsJapanese()) {
		// 日本語メニュー
		m_Menu.LoadMenu(IDR_MENU);
		m_PopupMenu.LoadMenu(IDR_MENUPOPUP);
	}
	else {
		// 英語メニュー
		m_Menu.LoadMenu(IDR_US_MENU);
		m_PopupMenu.LoadMenu(IDR_US_MENUPOPUP);
	}
	SetMenu(&m_Menu);
	m_bMenuBar = TRUE;
	m_bPopupMenu = FALSE;

	// メニュー(システム)
	::GetMsg(IDS_STDWIN, string);
	pSysMenu = GetSystemMenu(FALSE);
	ASSERT(pSysMenu);
	nCount = pSysMenu->GetMenuItemCount();

	// 「ウィンドウ標準位置」を挿入
	pSysMenu->InsertMenu(nCount - 2, MF_BYPOSITION | MF_STRING, IDM_STDWIN, string);
	pSysMenu->InsertMenu(nCount - 2, MF_BYPOSITION | MF_SEPARATOR);

	// チャイルドウィンドウ初期化
	if (!InitChild()) {
		return -1;
	}

	// ウィンドウ位置、矩形初期化
	InitPos();

	// シェル通知初期化
	InitShell();

	// VM初期化
	if (!InitVM()) {
		// VM初期化エラー
		m_nStatus = 1;
		PostMessage(WM_KICK, 0, 0);
		return 0;
	}

	// バージョンリソースからVMへバージョンを伝達
	InitVer();

	// デバイス記憶
	m_pFDD = (FDD*)::GetVM()->SearchDevice(MAKEID('F', 'D', 'D', ' '));
	ASSERT(m_pFDD);
	m_pSASI = (SASI*)::GetVM()->SearchDevice(MAKEID('S', 'A', 'S', 'I'));
	ASSERT(m_pSASI);
	m_pSCSI = (SCSI*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(m_pSCSI);
	m_pScheduler = (Scheduler*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pScheduler);
	m_pKeyboard = (Keyboard*)::GetVM()->SearchDevice(MAKEID('K', 'E', 'Y', 'B'));
	ASSERT(m_pKeyboard);
	m_pMouse = (Mouse*)::GetVM()->SearchDevice(MAKEID('M', 'O', 'U', 'S'));
	ASSERT(m_pMouse);

	// コンポーネント作成、初期化
	if (!InitComponent()) {
		// コンポーネント初期化エラー
		m_nStatus = 2;
		PostMessage(WM_KICK, 0, 0);
		return 0;
	}

	// 設定適用(OnOptionと同様、VMロックして)
	::LockVM();
	ApplyCfg();
	::UnlockVM();

	// リセット
	::GetVM()->Reset();

	// ウインドウ位置をレジューム(m_nStatus != 0に留意)
	ASSERT(m_nStatus != 0);
	RestoreFrameWnd(FALSE);

	// メッセージをポストして終了
	m_nStatus = 0;
	PostMessage(WM_KICK, 0, 0);
	return 0;
}

//---------------------------------------------------------------------------
//
//	チャイルドウィンドウ初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::InitChild()
{
	HDC hDC;
	HFONT hFont;
	HFONT hDefFont;
	TEXTMETRIC tm;
	int i;
	int nWidth;
	UINT uIndicator[6];

	// ビュー作成
	m_pDrawView = new CDrawView;
	if (!m_pDrawView->Init(this)) {
		return FALSE;
	}

	uIndicator[0] = ID_SEPARATOR;
	for (i=1; i<6; i++) {
		uIndicator[i] = (UINT)i;
	}

	// テキストメトリックを取得
	hDC = ::GetDC(m_hWnd);
	hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	hDefFont = (HFONT)::SelectObject(hDC, hFont);
	ASSERT(hDefFont);
	::GetTextMetrics(hDC, &tm);
	::SelectObject(hDC, hDefFont);
	::ReleaseDC(m_hWnd, hDC);

	// サイズ設定ループ
	nWidth = 0;
	for (i=1; i<6; i++) {
		switch (i) {
			// FD0, FD1
			case 1:
			case 2:
				nWidth = tm.tmAveCharWidth * 32;
				break;

			// HD BUSY
			case 3:
				nWidth = tm.tmAveCharWidth * 10;
				break;

			// TIMER
			case 4:
				nWidth = tm.tmAveCharWidth * 9;
				break;

			// POWER
			case 5:
				nWidth = tm.tmAveCharWidth * 9;
				break;
		}
	}

	// 再レイアウト
	RecalcLayout();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	位置・矩形初期化
//	※bStart=FALSEの場合、bFullScreen=FALSEの時に位置を復元すること
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::InitPos(BOOL bStart)
{
	int cx;
	int cy;
	CRect rect;
	CRect rectStatus;
	CRect rectWnd;

	ASSERT(this);

	// スクリーンサイズ、ウィンドウ矩形を取得
	cx = ::GetSystemMetrics(SM_CXSCREEN);
	cy = ::GetSystemMetrics(SM_CYSCREEN);
	GetWindowRect(&rectWnd);

	// 800x600以下はスクリーンサイズいっぱいに広げる
	if ((cx <= 800) || (cy <= 600)) {
		if ((rectWnd.left != 0) || (rectWnd.top != 0)) {
			SetWindowPos(&wndTop, 0, 0, cx, cy, SWP_NOZORDER);
			return;
		}
		if ((rectWnd.Width() != cx) || (rectWnd.Height() != cy)) {
			SetWindowPos(&wndTop, 0, 0, cx, cy, SWP_NOZORDER);
			return;
		}
		return;
	}

	// 824x560(DDP2)をノンインターレースの最大サイズと認定
	rect.left = 0;
	rect.top = 0;
	rect.right = 824;
	rect.bottom = 560;
	::AdjustWindowRectEx(&rect, GetView()->GetStyle(), FALSE, GetView()->GetExStyle());
//	m_StatusBar.GetWindowRect(&rectStatus);
	rect.bottom += rectStatus.Height();
	::AdjustWindowRectEx(&rect, GetStyle(), TRUE, GetExStyle());

	// rect.left, rect.bottomが負になるらしい(これ以降、right,bottomはcx,cyを示す)
	rect.right -= rect.left;
	rect.left = 0;
	rect.bottom -= rect.top;
	rect.top = 0;

	// 余裕があれば、センタリング
	if (rect.right < cx) {
		rect.left = (cx - rect.right) / 2;
	}
	if (rect.bottom < cy) {
		rect.top = (cy - rect.bottom) / 2;
	}

	// bStartで分ける(初期開始か、ウィンドウ－フルスクリーンの切り替えか)
	if (bStart) {
		// ウィンドウ位置を一旦保存(この後、再度RestoreFrameWndのチャンスあり)
		m_nWndLeft = rect.left;
		m_nWndTop = rect.top;
	}
	else {
		// ウィンドウモードの時に限り、位置を補正
		if (!m_bFullScreen) {
			if ((rect.left == 0) && (rect.top == 0)) {
				// WM_DISPLAYCHANGEメッセージが来て、ウィンドウが小さくなった場合
				m_nWndLeft = rect.left;
				m_nWndTop = rect.top;
			}
			else {
				// それ以外(フルスクリーン→ウィンドウへの状態遷移を含む)
				rect.left = m_nWndLeft;
				rect.top = m_nWndTop;
			}
		}
	}

	// 設定
	if ((rect.left != rectWnd.left) || (rect.top != rectWnd.top)) {
		SetWindowPos(&wndTop, rect.left, rect.top, rect.right, rect.bottom, 0);
		return;
	}
	if ((rect.right != rectWnd.Width()) || (rect.bottom != rectWnd.Height())) {
		SetWindowPos(&wndTop, rect.left, rect.top, rect.right, rect.bottom, 0);
		return;
	}
}

//---------------------------------------------------------------------------
//
//	シェル連携初期化
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::InitShell()
{
	int nSources;

	// 通知要因を設定
	if (::IsWinNT()) {
		// Windows2000/XP:shared memoryを利用するフラグを追加
		nSources = SHCNRF_InterruptLevel | SHCNRF_ShellLevel | SHCNRF_NewDelivery;
	}
	else {
		// Windows9x:shared memoryは使用しない
		nSources = SHCNRF_InterruptLevel | SHCNRF_ShellLevel;
	}

	// エントリを初期化
	m_fsne[0].pidl = NULL;
	m_fsne[0].fRecursive = FALSE;

	// シェル通知メッセージを登録
	m_uNotifyId = ::SHChangeNotifyRegister(m_hWnd,
							nSources,
							SHCNE_MEDIAINSERTED | SHCNE_MEDIAREMOVED | SHCNE_DRIVEADD | SHCNE_DRIVEREMOVED,
							WM_SHELLNOTIFY,
							sizeof(m_fsne)/sizeof(m_fsne[0]),
							m_fsne);
	ASSERT(m_uNotifyId);
}

//---------------------------------------------------------------------------
//
//	VM初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::InitVM()
{
	::pVM = new VM;
	if (!::GetVM()->Init()) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	コンポーネント初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::InitComponent()
{
	BOOL bSuccess = TRUE;
	schedulerInit();
	return bSuccess;
}

//---------------------------------------------------------------------------
//
//	バージョン初期化
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::InitVer()
{
	TCHAR szPath[_MAX_PATH];
	DWORD dwHandle;
	DWORD dwLength;
	BYTE *pVerInfo;
	VS_FIXEDFILEINFO *pFileInfo;
	UINT uLength;
	DWORD dwMajor;
	DWORD dwMinor;

	ASSERT(this);

	// パスを取得
	::GetModuleFileName(NULL, szPath, _MAX_PATH);

	// バージョン情報を読み取る
	dwLength = GetFileVersionInfoSize(szPath, &dwHandle);
	if (dwLength == 0) {
		return;
	}

	pVerInfo = new BYTE[ dwLength ];
	if (::GetFileVersionInfo(szPath, dwHandle, dwLength, pVerInfo) == 0) {
		return;
	}

	// バージョン情報を取り出す
	if (::VerQueryValue(pVerInfo, _T("\\"), (LPVOID*)&pFileInfo, &uLength) == 0) {
		delete[] pVerInfo;
		return;
	}

	// バージョンを分離、VMへ通知
	dwMajor = (DWORD)HIWORD(pFileInfo->dwProductVersionMS);
	dwMinor = (DWORD)(LOWORD(pFileInfo->dwProductVersionMS) * 16
					+ HIWORD(pFileInfo->dwProductVersionLS));
	::GetVM()->SetVersion(dwMajor, dwMinor);

	// 終了
	delete[] pVerInfo;
}

//---------------------------------------------------------------------------
//
//	コマンドライン処理
//	※コマンドライン、WM_COPYDATAで共通
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::InitCmd(LPCTSTR lpszCmd)
{
	LPCTSTR lpszCurrent;
	LPCTSTR lpszNext;
	TCHAR szPath[_MAX_PATH];
	int nLen;
	int i;
	BOOL bReset;

	ASSERT(this);
	ASSERT(lpszCmd);

	// ポインタ、フラグ初期化
	lpszCurrent = lpszCmd;
	bReset = FALSE;

	// ループ
	for (i=0; i<2; i++) {
		// スペース、タブはスキップ
		while (lpszCurrent[0] <= _T(0x20)) {
			if (lpszCurrent[0] == _T('\0')) {
				break;
			}
			lpszCurrent++;
		}
		if (lpszCurrent[0] == _T('\0')) {
			break;
		}

		// 最初がダブルクォートなら、次のクォートを探す
		if (lpszCurrent[0] == _T('\x22')) {
			lpszNext = _tcschr(lpszCurrent + 1, _T('\x22'));
			if (!lpszNext) {
				// 対応するダブルクォートが見つからない
				return;
			}
			nLen = (int)(lpszNext - (lpszCurrent + 1));
			if (nLen >= _MAX_PATH) {
				// 長すぎる
				return;
			}

			// クォートされた内部をコピー
			_tcsnccpy(szPath, &lpszCurrent[1], nLen);
			szPath[nLen] = _T('\0');

			// クォートの次を指す
			lpszCurrent = &lpszNext[1];
		}
		else {
			// 次のスペースを探す
			lpszNext = _tcschr(lpszCurrent + 1, _T(' '));
			if (lpszNext) {
				// スペースまで
				nLen = (int)(lpszNext - lpszCurrent);
				if (nLen >= _MAX_PATH) {
					// 長すぎる
					return;
				}

				// スペースまでの部分をコピー
				_tcsnccpy(szPath, lpszCurrent, nLen);
				szPath[nLen] = _T('\0');

				// スペースの次を指す
				lpszCurrent = &lpszNext[1];
			}
			else {
				// 終端まで
				_tcscpy(szPath, lpszCurrent);
				lpszCurrent = NULL;
			}
		}

		// オープンを試みる
		bReset = InitCmdSub(i, szPath);

		// 終端なら終了
		if (!lpszCurrent) {
			break;
		}
	}

	// リセット要求があれば、リセット
	if (bReset) {
		OnReset();
	}
}

//---------------------------------------------------------------------------
//
//	コマンドライン処理 サブ
//	※コマンドライン、WM_COPYDATA、ドラッグ&ドロップで共通
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::InitCmdSub(int nDrive, LPCTSTR lpszPath)
{
	Filepath path;
	Fileio fio;
	LPTSTR lpszFile;
	DWORD dwSize;
	TCHAR szPath[_MAX_PATH];
	FDI *pFDI;
	CString strMsg;

	ASSERT(this);
	ASSERT((nDrive == 0) || (nDrive == 1));
	ASSERT(lpszPath);

	// pFDI初期化
	pFDI = NULL;

	// ファイルオープンチェック
	path.SetPath(lpszPath);
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}
	dwSize = fio.GetFileSize();
	fio.Close();

	// フルパス化
	::GetFullPathName(lpszPath, _MAX_PATH, szPath, &lpszFile);
	path.SetPath(szPath);

	// VMロック
	::LockVM();

/*
	// 128MO or 230MO or 540MO or 640MO
	if ((dwSize == 0x797f400) || (dwSize == 0xd9eea00) ||
		(dwSize == 0x1fc8b800) || (dwSize == 0x25e28000)) {
		// MOの割り当てを試みる
		nDrive = 2;

		if (!m_pSASI->Open(path)) {
			// MO割り当て失敗
//			GetScheduler()->Reset();
//			ResetCaption();
			::UnlockVM();
			return FALSE;
		}
	}
	else if (dwSize >= 0x200000) {
		// VMの割り当てを試みる
		nDrive = 4;

		// オープン前処理
		if (!OnOpenPrep(path, FALSE)) {
			// ファイルがないか、バージョンなどが正しくない
//				GetScheduler()->Reset();
//				ResetCaption();
			::UnlockVM();
			return FALSE;
		}

		// ロード実行(OnOpenSubに任せる)
		::UnlockVM();
		if (OnOpenSub(path)) {
			Filepath::SetDefaultDir(szPath);
		}
		// リセットは行わない
		return FALSE;
	}
	else
*/
	{
		// FDの割り当てを試みる
		if (!m_pFDD->Open(nDrive, path)) {
			// FD割り当て失敗
//				GetScheduler()->Reset();
//				ResetCaption();
			::UnlockVM();
			return FALSE;
		}
		pFDI = m_pFDD->GetFDI(nDrive);
	}

	// VMリセット、ロック解除
//	GetScheduler()->Reset();
//	ResetCaption();
	::UnlockVM();

	// 成功。ディレクトリ保存＆MRU追加
	Filepath::SetDefaultDir(szPath);
//	GetConfig()->SetMRUFile(nDrive, szPath);

	// フロッピーなら、BADイメージ警告
	if (pFDI) {
		if (pFDI->GetID() == MAKEID('B', 'A', 'D', ' ')) {
			::GetMsg(IDS_BADFDI_WARNING, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		}

		// フロッピーを割り当てたときだけ、リセットする
		return TRUE;
	}

	// 終了
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	コンポーネントをセーブ
//	※スケジューラは停止しているが、CSound,CInputは動作中
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::SaveComponent(const Filepath& path, DWORD dwPos)
{
	Fileio fio;
	DWORD dwID;
	DWORD dwMajor;
	DWORD dwMinor;
	int nVer;

	ASSERT(this);
	ASSERT(dwPos > 0);

	// バージョン情報作成
	::GetVM()->GetVersion(dwMajor, dwMinor);
	nVer = (int)((dwMajor << 8) | dwMinor);

	// ファイルオープンとシーク
	if (!fio.Open(path, Fileio::Append)) {
		return FALSE;
	}
	if (!fio.Seek(dwPos)) {
		fio.Close();
		return FALSE;
	}

	// メインコンポーネント情報を保存
	dwID = MAKEID('M', 'A', 'I', 'N');
	if (!fio.Write(&dwID, sizeof(dwID))) {
		fio.Close();
		return FALSE;
	}

	// 終端書き込み
	dwID = MAKEID('E', 'N', 'D', ' ');
	if (!fio.Write(&dwID, sizeof(dwID))) {
		fio.Close();
		return FALSE;
	}

	// 終了
	fio.Close();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	コンポーネントをロード
//	※スケジューラは停止しているが、CSound,CInputは動作中
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::LoadComponent(const Filepath& path, DWORD dwPos)
{
	Fileio fio;
	DWORD dwID;
	char cHeader[0x10];
	int nVer;

	ASSERT(this);
	ASSERT(dwPos > 0);

	// ファイルオープン
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}

	// ヘッダ読み取り
	if (!fio.Read(cHeader, sizeof(cHeader))) {
		fio.Close();
		return FALSE;
	}

	// ヘッダチェック、バージョン情報読み取り
	cHeader[0x0a] = '\0';
	nVer = ::strtoul(&cHeader[0x09], NULL, 16);
	nVer <<= 8;
	cHeader[0x0d] = '\0';
	nVer |= ::strtoul(&cHeader[0x0b], NULL, 16);
	cHeader[0x09] = '\0';
	if (strcmp(cHeader, "XM6 DATA ") != 0) {
		fio.Close();
		return FALSE;
	}

	// シーク
	if (!fio.Seek(dwPos)) {
		fio.Close();
		return FALSE;
	}

	// メインコンポーネント読み取り
	if (!fio.Read(&dwID, sizeof(dwID))) {
		fio.Close();
		return FALSE;
	}
	if (dwID != MAKEID('M', 'A', 'I', 'N')) {
		fio.Close();
		return FALSE;
	}

	// コンポーネントループ
	for (;;) {
		// ID読み取り
		if (!fio.Read(&dwID, sizeof(dwID))) {
			fio.Close();
			return FALSE;
		}

		// 終了チェック
		if (dwID == MAKEID('E', 'N', 'D', ' ')) {
			break;
		}

		fio.Close();
		return FALSE;
	}

	// クローズ
	fio.Close();

	// 設定適用(VMロックして行う)
//	if (GetConfig()->IsApply())
	{
		::LockVM();
		ApplyCfg();
		::UnlockVM();
	}

	// ウィンドウ再描画
	GetView()->Invalidate(FALSE);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::ApplyCfg()
{
	Config config;

	// 設定取得
	configGetConfig(&config);

	// まずVMに適用
	::GetVM()->ApplyCfg(&config);

	// 次にコンポーネントに適用
	// 次にビューに適用
	GetView()->ApplyCfg(&config);

	// フレームウィンドウ(ポップアップ)
	if (config.popup_swnd != m_bPopup) {
//		// サブウィンドウをすべてクリア
//		GetView()->ClrSWnd();

		// 変更
		m_bPopup = config.popup_swnd;
	}

	// フレームウィンドウ(マウス)
	m_bMouseMid = config.mouse_mid;
	m_bAutoMouse = config.auto_mouse;
	if (config.mouse_port == 0) {
		// マウス接続なしなら、マウスモードOFF
//		if (GetInput()->GetMouseMode()) {
//			OnMouseMode();
//		}
	}
}

//---------------------------------------------------------------------------
//
//	キック
//
//---------------------------------------------------------------------------
LONG CFrmWnd::OnKick(UINT , LONG )
{
//	CInfo *pInfo;
	Config config;
	CString strMsg;
	MSG msg;
	Memory *pMemory;
	int nIdle;
	LPSTR lpszCmd;
	LPCTSTR lpszCommand;
	BOOL bFullScreen;

	// エラー処理を先に行う
	switch (m_nStatus) {
		// VMエラー
		case 1:
			::GetMsg(IDS_INIT_VMERR, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
			PostMessage(WM_CLOSE, 0, 0);
			return 0;

		// コンポーネントエラー
		case 2:
			::GetMsg(IDS_INIT_COMERR, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
			PostMessage(WM_CLOSE, 0, 0);
			return 0;
	}
	// 正常の場合
	ASSERT(m_nStatus == 0);

	// ROMチェック
	pMemory = (Memory*)::GetVM()->SearchDevice(MAKEID('M', 'E', 'M', ' '));
/*
	ASSERT(pMemory);
	if (!pMemory->CheckIPL()) {
		::GetMsg(IDS_INIT_IPLERR, strMsg);
		if (MessageBox(strMsg, NULL, MB_ICONSTOP | MB_YESNO | MB_DEFBUTTON2) != IDYES) {
			PostMessage(WM_CLOSE, 0, 0);
			return 0;
		}
	}
	if (!pMemory->CheckCG()) {
		::GetMsg(IDS_INIT_CGERR, strMsg);
		if (MessageBox(strMsg, NULL, MB_ICONSTOP | MB_YESNO | MB_DEFBUTTON2) != IDYES) {
			PostMessage(WM_CLOSE, 0, 0);
			return 0;
		}
	}
*/
	// 設定取得(power_off設定のため)
	configGetConfig(&config);
	if (config.power_off) {
		// 電源OFFで起動
		::GetVM()->SetPower(FALSE);
		::GetVM()->PowerSW(FALSE);
	}

	// サブウィンドウの準備
	m_strWndClsName = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS);

	// コンポーネントをイネーブル。ただしSchedulerは設定による
	GetView()->Enable(TRUE);
	schedulerSetEnable(TRUE);

	// リセット(ステータスバーのため)
	if (!config.power_off) {
		OnReset();
	}

	// コマンドライン処理
	lpszCmd = AfxGetApp()->m_lpCmdLine;
	lpszCommand = A2T(lpszCmd);
	if (_tcslen(lpszCommand) > 0) {
		InitCmd(lpszCommand);
	}

	// 最大化指定であれば、戻した後に、フルスクリーン
	bFullScreen = FALSE;
	if (IsZoomed()) {
		ShowWindow(SW_RESTORE);
		bFullScreen = TRUE;
	}

	// ウインドウ位置をレジューム
	bFullScreen = RestoreFrameWnd(bFullScreen);
	if (bFullScreen) {
		// 最大化指定か、前回実行時にフルスクリーン
		PostMessage(WM_COMMAND, IDM_FULLSCREEN);
	}

	// ディスク・ステートをレジューム
	RestoreDiskState();

	// 無限ループ
	nIdle = 0;
	while (!m_bExit) {
		// メッセージチェック＆ポンプ
		if (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			if (!AfxGetApp()->PumpMessage()) {
				::PostQuitMessage(0);
				return 0;
			}
			// continueすることで、WM_DESTROY直後のm_bExitチェックを保証
			continue;
		}

		// スリープ
		if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			Sleep(20);

			// 更新カウンタUp
			nIdle++;

			// ステータス・実行は20ms
			UpdateExec();

			if ((nIdle & 1) == 0) {
				// ビューは40ms
				GetView()->Update();
			}
		}
	}

	return 0;
}

//---------------------------------------------------------------------------
//
//	ウィンドウクローズ
//
//---------------------------------------------------------------------------
void CFrmWnd::OnClose()
{
	CString strFormat;
	CString strText;
	Filepath path;

	ASSERT(this);
	ASSERT(!m_bSaved);

	// 有効なステートファイルがあれば、セーブを問う
	::LockVM();
	::GetVM()->GetPath(path);
	::UnlockVM();

/*
	// 有効なステートファイルがあって
	if (!path.IsClear()) {
		// Windowsサイドで20ms以上の実行実績があれば
		if (m_dwExec >= 2) {
			// 確認
			::GetMsg(IDS_SAVECLOSE, strFormat);
			strText.Format(strFormat, path.GetFileExt());
			int nResult = MessageBox(strText, NULL, MB_ICONQUESTION | MB_YESNOCANCEL);

			// 確認結果による
			switch (nResult) {
				// YES
				case IDYES:
					// 保存
					OnSaveSub(path);
					break;

				// NO
				case IDNO:
					// パスをクリア(ステートなし)
					::GetVM()->Clear();
					break;

				// キャンセル
				case IDCANCEL:
					// クローズされなかったことにする
					return;
			}
		}
	}
*/
	// 初期化済みなら
	if ((m_nStatus == 0) && !m_bSaved) {
		// ウィンドウ状態・ディスク・ステートを保存
		SaveFrameWnd();
		SaveDiskState();
		m_bSaved = TRUE;
	}

	// 基本クラス
	CFrameWnd::OnClose();
}

//---------------------------------------------------------------------------
//
//	ウィンドウ削除
//
//---------------------------------------------------------------------------
void CFrmWnd::OnDestroy()
{
	ASSERT(this);

	// 初期化済みなら
	if ((m_nStatus == 0) && !m_bSaved) {
		// ウィンドウ状態・ディスク・ステートを保存
		SaveFrameWnd();
		SaveDiskState();
		m_bSaved = TRUE;
	}

	// クリーンアップ(WM_ENDSESSIONと共通)
	CleanSub();

	// 基本クラスへ
	CFrameWnd::OnDestroy();
}

//---------------------------------------------------------------------------
//
//	セッション終了
//
//---------------------------------------------------------------------------
void CFrmWnd::OnEndSession(BOOL bEnding)
{
	ASSERT(this);

	// 終了時は、クリーンアップを行う
	if (bEnding) {
		// 初期化済みなら
		if (m_nStatus == 0) {
			// ウィンドウ状態・ディスク・ステートを保存
			if (!m_bSaved) {
				SaveFrameWnd();
				SaveDiskState();
				m_bSaved = TRUE;
			}

			// クリーンアップ
			CleanSub();
		}
	}

	// 基本クラス
	CFrameWnd::OnEndSession(bEnding);
}

//---------------------------------------------------------------------------
//
//	クリーンアップ共通
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::CleanSub()
{
	int i;

	// 終了フラグを上げる
	m_bExit = TRUE;

	// コンポーネントを止める
	GetView()->Enable(FALSE);
	schedulerSetEnable(FALSE);

	// スケジューラが実行をやめるまで待つ
	for (i=0; i<8; i++) {
		::LockVM();
		::UnlockVM();
	}

	// 仮想マシンを削除
	if (::pVM) {
		::LockVM();
		::GetVM()->Cleanup();
		delete ::pVM;
		::pVM = NULL;
		::UnlockVM();
	}

	// シェル通知を削除
	if (m_uNotifyId) {
		 VERIFY(::SHChangeNotifyDeregister(m_uNotifyId));
		 m_uNotifyId = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	ウィンドウ状態を保存
//
//---------------------------------------------------------------------------
void CFrmWnd::SaveFrameWnd()
{
	CRect rectWnd;
	Config config;

	ASSERT(this);
	ASSERT_VALID(this);

	// 設定取得
	configGetConfig(&config);

	// キャプション・メニュー・ステータスバー
	config.menu_bar = m_bMenuBar;

	// ウィンドウ矩形
	if (m_bFullScreen) {
		// フルスクリーン時は、ウィンドウ時の位置を保存する
		config.window_left = m_nWndLeft;
		config.window_top = m_nWndTop;
	}
	else {
		// ウィンドウ時は、現在の位置を保存する
		GetWindowRect(&rectWnd);
		config.window_left = rectWnd.left;
		config.window_top = rectWnd.top;
	}

	// フルスクリーン
	config.window_full = m_bFullScreen;

	// 設定変更
//	GetConfig()->SetConfig(&config);
}

//---------------------------------------------------------------------------
//
//	ディスク・ステートを保存
//
//---------------------------------------------------------------------------
void CFrmWnd::SaveDiskState()
{
	int nDrive;
	Filepath path;
	Config config;

	ASSERT(this);
	ASSERT_VALID(this);

	// ロック
	::LockVM();

	// 設定取得
	configGetConfig(&config);

	// フロッピーディスク
	for (nDrive=0; nDrive<2; nDrive++) {
		// レディ
		config.resume_fdi[nDrive] = m_pFDD->IsReady(nDrive, FALSE);

		// レディでなければ、次へ
		if (!config.resume_fdi[nDrive]) {
			continue;
		}

		// メディア
		config.resume_fdm[nDrive]  = m_pFDD->GetMedia(nDrive);

		// ライトプロテクト
		config.resume_fdw[nDrive] = m_pFDD->IsWriteP(nDrive);
	}

	// MOディスク
	config.resume_mos = m_pSASI->IsReady();
	if (config.resume_mos) {
		config.resume_mow = m_pSASI->IsWriteP();
	}

	// CD-ROM
	config.resume_iso = m_pSCSI->IsReady(FALSE);

	// ステート
	::GetVM()->GetPath(path);
	config.resume_xm6 = !path.IsClear();

	// デフォルトディレクトリ
	_tcscpy(config.resume_path, Filepath::GetDefaultDir());

	// 設定変更
//	GetConfig()->SetConfig(&config);

	// アンロック
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	ウィンドウ状態を復元
//	※OnCreateとOnKickと、2回呼ばれる
//
//---------------------------------------------------------------------------
BOOL CFrmWnd::RestoreFrameWnd(BOOL bFullScreen)
{
	int nWidth;
	int nHeight;
	int nLeft;
	int nTop;
	CRect rectWnd;
	BOOL bValid;
	Config config;

	ASSERT(this);

	// 設定取得
	configGetConfig(&config);

	// ウィンドウ位置の復元が指定されていなければ、デフォルト状態で動作
	if (!config.resume_screen) {
		return bFullScreen;
	}

	// メニュー
	m_bMenuBar = config.menu_bar;
	ShowMenu();

	// 仮想画面のサイズと原点を取得
	nWidth = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
	nHeight = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
	nLeft = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
	nTop = ::GetSystemMetrics(SM_YVIRTUALSCREEN);

	// ウィンドウ矩形を取得
	GetWindowRect(&rectWnd);

	// 手の届くところにあれば、ウィンドウ位置を移動する。まずはチェック
	bValid = TRUE;
	if (config.window_left < nLeft) {
		if (config.window_left < nLeft - rectWnd.Width()) {
			bValid = FALSE;
		}
	}
	else {
		if (config.window_left >= (nLeft + nWidth)) {
			bValid = FALSE;
		}
	}
	if (config.window_top < nTop) {
		if (config.window_top < nTop - rectWnd.Height()) {
			bValid = FALSE;
		}
	}
	else {
		if (config.window_top >= (nTop + nHeight)) {
			bValid = FALSE;
		}
	}

	// ウィンドウ位置を移動
	if (bValid) {
		SetWindowPos(&wndTop, config.window_left, config.window_top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		// ワークエリアも同時に変更する
		m_nWndLeft = config.window_left;
		m_nWndTop = config.window_top;
	}

	// VM未初期化なら、ここまで
	if (m_nStatus != 0) {
		return FALSE;
	}

	// フルスクリーンか
	if (bFullScreen || config.window_full) {
		// 最大化起動か、前回フルスクリーンだった
		return TRUE;
	}
	else {
		// 最大化起動でなく、かつ、前回通常表示だった
		return FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	ディスク・ステートを復元
//
//---------------------------------------------------------------------------
void CFrmWnd::RestoreDiskState()
{
	int nDrive;
//	TCHAR szMRU[_MAX_PATH];
	BOOL bResult;
	Filepath path;
	Config config;

	ASSERT(this);

	// 設定取得
	configGetConfig(&config);

/*
	// ステートが指定されていれば、これを先に行う
	if (config.resume_state) {
		// ステートがあった
		if (config.resume_xm6) {
			// パス取得
//			GetConfig()->GetMRUFile(4, 0, szMRU);
//			path.SetPath(szMRU);

			// オープン前処理
			if (OnOpenPrep(path)) {
				// オープンサブ
				if (OnOpenSub(path)) {
					// 成功なので、デフォルトディレクトリだけ処理
					if (config.resume_dir) {
						Filepath::SetDefaultDir(config.resume_path);
					}

					// これ以降は処理しない(FD, MO, CDのアクセス中にセーブした場合)
					return;
				}
			}
		}
	}
*/
	// フロッピーディスク
	if (config.resume_fd) {
		for (nDrive=0; nDrive<2; nDrive++) {
			// ディスク挿入されていたか
			if (!config.resume_fdi[nDrive]) {
				// ディスク挿入されていない。スキップ
				continue;
			}

			// ディスク挿入
//			GetConfig()->GetMRUFile(nDrive, 0, szMRU);
//			ASSERT(szMRU[0] != _T('\0'));
//			path.SetPath(szMRU);

			// VMロックを行い、ディスク割り当てを試みる
			::LockVM();
			bResult = m_pFDD->Open(nDrive, path, config.resume_fdm[nDrive]);
			::UnlockVM();

			// 割り当てできなければスキップ
			if (!bResult) {
				continue;
			}

			// 書き込み禁止
			if (config.resume_fdw[nDrive]) {
				::LockVM();
				m_pFDD->WriteP(nDrive, TRUE);
				::UnlockVM();
			}
		}
	}

	// MOディスク
	if (config.resume_mo) {
		// ディスク挿入されていたか
		if (config.resume_mos) {
			// ディスク挿入
//			GetConfig()->GetMRUFile(2, 0, szMRU);
//			ASSERT(szMRU[0] != _T('\0'));
//			path.SetPath(szMRU);

			// VMロックを行い、ディスク割り当てを試みる
			::LockVM();
			bResult = m_pSASI->Open(path);
			::UnlockVM();

			// 割り当てできれば
			if (bResult) {
				// 書き込み禁止
				if (config.resume_mow) {
					::LockVM();
					m_pSASI->WriteP(TRUE);
					::UnlockVM();
				}
			}
		}
	}

	// CD-ROM
	if (config.resume_cd) {
		// ディスク挿入されていたか
		if (config.resume_iso) {
			// ディスク挿入
//			GetConfig()->GetMRUFile(3, 0, szMRU);
//			ASSERT(szMRU[0] != _T('\0'));
//			path.SetPath(szMRU);

			// VMロックを行い、ディスク割り当てを試みる
			::LockVM();
			m_pSCSI->Open(path, FALSE);
			::UnlockVM();
		}
	}

	// デフォルトディレクトリ
	if (config.resume_dir) {
		Filepath::SetDefaultDir(config.resume_path);
	}
}

//---------------------------------------------------------------------------
//
//	ディスプレイ変更
//
//---------------------------------------------------------------------------
LRESULT CFrmWnd::OnDisplayChange(UINT uParam, LONG lParam)
{
	LRESULT lResult;

	// 基本クラス
	lResult = CFrameWnd::OnDisplayChange(uParam, lParam);

	// 最小化は何もしない
	if (IsIconic()) {
		return lResult;
	}

	// ポジション設定
	InitPos(FALSE);

	return lResult;
}

//---------------------------------------------------------------------------
//
//	ウィンドウ背景描画
//
//---------------------------------------------------------------------------
BOOL CFrmWnd::OnEraseBkgnd(CDC * )
{
	// 背景描画を抑制
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ウィンドウ描画
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPaint()
{
	// 必ずロックして行う
	::LockVM();

	PAINTSTRUCT ps;
	BeginPaint(&ps);
	EndPaint(&ps);

	// アンロック
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	ウィンドウ移動
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMove(int x, int y)
{
	// 基本クラス
	CFrameWnd::OnMove(x, y);
}

//---------------------------------------------------------------------------
//
//	アクティベート
//
//---------------------------------------------------------------------------
void CFrmWnd::OnActivate(UINT nState, CWnd *pWnd, BOOL bMinimized)
{
//	CInput *pInput;
//	CScheduler *pScheduler;

	// 初期化済みなら
	if (m_nStatus == 0) {
		// インプット、スケジューラへ通知
//		pInput = GetInput();
//		pScheduler = GetScheduler();
//		if (pScheduler) {
//			// WA_INACTIVEか最小化なら、ディセーブル
//			if ((nState == WA_INACTIVE) || bMinimized) {
//				// 入力受け付けない、低速実行
//				pScheduler->Activate(FALSE);
//			}
//			else {
//				// 入力受け付ける、通常実行
//				pScheduler->Activate(TRUE);
//			}
//		}
	}

	// 基本クラスへ
	CFrameWnd::OnActivate(nState, pWnd, bMinimized);
}

//---------------------------------------------------------------------------
//
//	アクティベートアプリケーション
//
//---------------------------------------------------------------------------
#if _MFC_VER >= 0x700
void CFrmWnd::OnActivateApp(BOOL bActive, DWORD dwThreadID)
#else
void CFrmWnd::OnActivateApp(BOOL bActive, HTASK hTask)
#endif
{
	// 初期化済みなら
	if (m_nStatus == 0) {
		// フルスクリーン専用
		if (m_bFullScreen) {
			if (bActive) {
				// これからアクティブになる
				HideTaskBar(TRUE, TRUE);
			}
			else {
				// アクティブから外れた
				HideTaskBar(FALSE, FALSE);
			}
		}
	}

	// 基本クラス
#if _MFC_VER >= 0x700
	CFrameWnd::OnActivateApp(bActive, dwThreadID);
#else
	CFrameWnd::OnActivateApp(bActive, hTask);
#endif
}

//---------------------------------------------------------------------------
//
//	メニューループ開始
//
//---------------------------------------------------------------------------
void CFrmWnd::OnEnterMenuLoop(BOOL bTrackPopup)
{
	::LockVM();

//	// スケジューラへ通知
//	CScheduler *pScheduler = GetScheduler();
//	if (pScheduler) {
//		pScheduler->Menu(TRUE);
//	}

	::UnlockVM();

	// 基本クラスへ
	CFrameWnd::OnEnterMenuLoop(bTrackPopup);
}

//---------------------------------------------------------------------------
//
//	メニューループ終了
//
//---------------------------------------------------------------------------
void CFrmWnd::OnExitMenuLoop(BOOL bTrackPopup)
{
	::LockVM();

//	// スケジューラへ通知
//	CScheduler *pScheduler = GetScheduler();
//	if (pScheduler) {
//		pScheduler->Menu(FALSE);
//	}

	::UnlockVM();

	// 基本クラスへ
	CFrameWnd::OnExitMenuLoop(bTrackPopup);
}

//---------------------------------------------------------------------------
//
//	親ウィンドウ通知
//
//---------------------------------------------------------------------------
void CFrmWnd::OnParentNotify(UINT message, LPARAM lParam)
{
	// 基本クラスへ
	CFrameWnd::OnParentNotify(message, lParam);
}

//---------------------------------------------------------------------------
//
//	コンテキストメニュー
//
//---------------------------------------------------------------------------
void CFrmWnd::OnContextMenu(CWnd * , CPoint pos)
{
	// ポップアップメニュー
	m_bPopupMenu = TRUE;

	CMenu *pMenu = m_PopupMenu.GetSubMenu(0);
	pMenu->TrackPopupMenu(TPM_CENTERALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
							pos.x, pos.y, this, 0);
	m_bPopupMenu = FALSE;
}

//---------------------------------------------------------------------------
//
//	電力変更通知
//
//---------------------------------------------------------------------------
LONG CFrmWnd::OnPowerBroadCast(UINT , LONG )
{
	// 初期化済みなら
	if (m_nStatus == 0) {
		// VMロック、時間再設定
		::LockVM();
		timeEndPeriod(1);
		timeBeginPeriod(1);
		::UnlockVM();
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	システムコマンド
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSysCommand(UINT nID, LPARAM lParam)
{
	// 標準ウィンドウ位置をサポート
	if ((nID & 0xfff0) == IDM_STDWIN) {
		InitPos(TRUE);
		return;
	}

	// 最大化はフルスクリーン
	if ((nID & 0xfff0) == SC_MAXIMIZE) {
		if (!m_bFullScreen) {
			PostMessage(WM_COMMAND, IDM_FULLSCREEN);
		}
		return;
	}

	// 基本クラス
	CFrameWnd::OnSysCommand(nID, lParam);
}

//---------------------------------------------------------------------------
//
//	データ転送
//
//---------------------------------------------------------------------------
#if _MFC_VER >= 0x700
afx_msg BOOL CFrmWnd::OnCopyData(CWnd* , COPYDATASTRUCT* pCopyDataStruct)
#else
LONG CFrmWnd::OnCopyData(UINT , LONG pCopyDataStruct)
#endif
{
	PCOPYDATASTRUCT pCDS;

	// パラメータ受け取り
	pCDS = (PCOPYDATASTRUCT)pCopyDataStruct;

	// コマンドライン処理へ
	InitCmd((LPSTR)pCDS->lpData);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	シェル通知
//
//---------------------------------------------------------------------------
LRESULT CFrmWnd::OnShellNotify(UINT uParam, LONG lParam)
{
	HANDLE hMemoryMap;
	DWORD dwProcessId;
	LPITEMIDLIST *pidls;
	HANDLE hLock;
	LONG nEvent;
//	TCHAR szPath[_MAX_PATH];

	// Windows NTか
	if (::IsWinNT()) {
		// Windows2000/XPの場合、SHChangeNotification_Lockでロックする
		hMemoryMap = (HANDLE)uParam;
		dwProcessId = (DWORD)lParam;
		hLock = ::SHChangeNotification_Lock(hMemoryMap, dwProcessId, &pidls, &nEvent);
		if (hLock == NULL) {
			return 0;
		}
	}
	else {
		// Windows9xの場合、pidlsとnEventはuParam,lParamから直接得る
		pidls = (LPITEMIDLIST*)uParam;
		nEvent = lParam;
		hLock = NULL;
	}

	// 実行中で、CHostがあれば、通知
	if (m_nStatus == 0) {
//		CHost *pHost = GetHost();

#if 1
		// Windrvがまだ不安定のため、実際にEnableにされていない場合は何もしない(version2.04)
		{
			Config config;
			configGetConfig(&config);
			if ((config.windrv_enable <= 0) || (config.windrv_enable > 3)) {
//				pHost = NULL;
			}
		}
#endif

//		if (pHost) {
//			// パス取得
//			::SHGetPathFromIDList(pidls[0], szPath);
//			// 通知
//			pHost->ShellNotify(nEvent, szPath);
//		}
	}

	// NTの場合、SHCnangeNotifcation_Unlockでアンロックする
	if (::IsWinNT()) {
		ASSERT(hLock);
		::SHChangeNotification_Unlock(hLock);
	}

	return 0;
}

//---------------------------------------------------------------------------
//
//	更新(実行)
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::UpdateExec()
{
	ASSERT(this);
	ASSERT_VALID(this);

	// スケジューラが有効なら、実行カウンタを上げる(セーブ時にクリアされる)
	if (schedulerIsEnable()) {
		m_dwExec++;
		if (m_dwExec == 0) {
			m_dwExec--;
		}
	}
}

//---------------------------------------------------------------------------
//
//	メッセージ文字列提供
//
//---------------------------------------------------------------------------
void CFrmWnd::GetMessageString(UINT nID, CString& rMessage) const
{
	Filepath path;
//	TCHAR szPath[_MAX_PATH];
	TCHAR szName[60];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	TCHAR szFile[_MAX_FNAME];
	TCHAR szExt[_MAX_EXT];
	int nMRU;
	int nDisk;
	BOOL bValid;
//	CInfo *pInfo;

	// フラグFALSE
	bValid = FALSE;

	// メニュー文字列を先に行う(英語環境+MRUを考慮)
	if ((nID >= IDM_OPEN) && (nID <= IDM_ABOUT)) {
		// 英語環境か
		if (!::IsJapanese()) {
			// +5000で試す
			if (rMessage.LoadString(nID + 5000)) {
				bValid = TRUE;
			}
		}
	}

	// メニュー文字列特例(IDM_STDWIN)
	if (nID == IDM_STDWIN) {
		// 英語環境か
		if (!::IsJapanese()) {
			// +5000で試す
			if (rMessage.LoadString(nID + 5000)) {
				bValid = TRUE;
			}
		}
	}

	// MRU0
	if ((nID >= IDM_D0_MRU0) && (nID <= IDM_D0_MRU8)) {
		nMRU = nID - IDM_D0_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
//		GetConfig()->GetMRUFile(0, nMRU, szPath);
//		szPath[60] = _T('\0');
//		rMessage = szPath;
//		bValid = TRUE;
	}

	// MRU1
	if ((nID >= IDM_D1_MRU0) && (nID <= IDM_D1_MRU8)) {
		nMRU = nID - IDM_D1_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
//		GetConfig()->GetMRUFile(1, nMRU, szPath);
//		szPath[60] = _T('\0');
//		rMessage = szPath;
//		bValid = TRUE;
	}

	// MRU2
	if ((nID >= IDM_MO_MRU0) && (nID <= IDM_MO_MRU8)) {
		nMRU = nID - IDM_MO_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
//		GetConfig()->GetMRUFile(2, nMRU, szPath);
//		szPath[60] = _T('\0');
//		rMessage = szPath;
//		bValid = TRUE;
	}

	// MRU3
	if ((nID >= IDM_CD_MRU0) && (nID <= IDM_CD_MRU8)) {
		nMRU = nID - IDM_CD_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
//		GetConfig()->GetMRUFile(3, nMRU, szPath);
//		szPath[60] = _T('\0');
//		rMessage = szPath;
//		bValid = TRUE;
	}

	// MRU4
	if ((nID >= IDM_XM6_MRU0) && (nID <= IDM_XM6_MRU8)) {
		nMRU = nID - IDM_XM6_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
//		GetConfig()->GetMRUFile(4, nMRU, szPath);
//		szPath[60] = _T('\0');
//		rMessage = szPath;
//		bValid = TRUE;
	}

	// ディスク名0
	if ((nID >= IDM_D0_MEDIA0) && (nID <= IDM_D0_MEDIAF)) {
		nDisk = nID - IDM_D0_MEDIA0;
		ASSERT((nDisk >= 0) && (nDisk <= 15));
		::LockVM();
		m_pFDD->GetName(0, szName, nDisk);
		m_pFDD->GetPath(0, path);
		::UnlockVM();
		_tsplitpath(path.GetPath(), szDrive, szDir, szFile, szExt);
		rMessage = szName;
		rMessage += _T(" (");
		rMessage += szFile;
		rMessage += szExt;
		rMessage += _T(")");
		bValid = TRUE;
	}

	// ディスク名1
	if ((nID >= IDM_D1_MEDIA0) && (nID <= IDM_D1_MEDIAF)) {
		nDisk = nID - IDM_D1_MEDIA0;
		ASSERT((nDisk >= 0) && (nDisk <= 15));
		::LockVM();
		m_pFDD->GetName(1, szName, nDisk);
		m_pFDD->GetPath(1, path);
		::UnlockVM();
		_tsplitpath(path.GetPath(), szDrive, szDir, szFile, szExt);
		rMessage = szName;
		rMessage += _T(" (");
		rMessage += szFile;
		rMessage += szExt;
		rMessage += _T(")");
		bValid = TRUE;
	}

	// ここまでで確定していなければ、基本クラス
	if (!bValid) {
		CFrameWnd::GetMessageString(nID, rMessage);
	}
}

//---------------------------------------------------------------------------
//
//	タスクバー隠す
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::HideTaskBar(BOOL bHide, BOOL bFore)
{
	if (bHide) {
		// "常に前面"
		m_hTaskBar = ::FindWindow(_T("Shell_TrayWnd"), NULL);
		if (m_hTaskBar) {
			::ShowWindow(m_hTaskBar, SW_HIDE);
		}
		ModifyStyleEx(0, WS_EX_TOPMOST, 0);
	}
	else {
		// "通常"
		ModifyStyleEx(WS_EX_TOPMOST, 0, 0);
		if (m_hTaskBar) {
			::ShowWindow(m_hTaskBar, SW_SHOWNA);
		}
	}

	// 前面オプションがあれば
	if (bFore) {
		SetForegroundWindow();
	}
}

//---------------------------------------------------------------------------
//
//	オーナードロー
//
//---------------------------------------------------------------------------
void CFrmWnd::OnDrawItem(int nID, LPDRAWITEMSTRUCT lpDIS)
{
}

//---------------------------------------------------------------------------
//
//	メニューバー表示
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::ShowMenu()
{
	HMENU hMenu;

	ASSERT(this);

	// 必要であればVMをロック
	if (m_nStatus == 0) {
		::LockVM();
	}

	// 現在のメニューを取得
	hMenu = ::GetMenu(m_hWnd);

	// メニューが不必要な場合
	if (m_bFullScreen || !m_bMenuBar) {
		// メニューが存在するか
		if (hMenu != NULL) {
			// メニューを消去
			SetMenu(NULL);
		}
		if (m_nStatus == 0) {
			::UnlockVM();
		}
		return;
	}

	// メニューが必要な場合
	if (hMenu != NULL) {
		// セットしたいメニューと同じか
		if (m_Menu.GetSafeHmenu() == hMenu) {
			// 変更の必要はない
			if (m_nStatus == 0) {
				::UnlockVM();
			}
			return;
		}
	}

	// メニューをセット
	SetMenu(&m_Menu);

	// 必要ならVMをアンロック
	if (m_nStatus == 0) {
		::UnlockVM();
	}
}

//---------------------------------------------------------------------------
//
//	描画ビュー取得
//
//---------------------------------------------------------------------------
CDrawView* FASTCALL CFrmWnd::GetView() const
{
	ASSERT(this);
	ASSERT(m_pDrawView);
	ASSERT(m_pDrawView->m_hWnd);
	return m_pDrawView;
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void CFrmWnd::OnReset()
{
	SRAM *pSRAM;
	DWORD Sw[0x100];
	DWORD dwDevice;
	DWORD dwAddr;
	CString strReset;
	CString strSub;
	BOOL bFlag;
	int i;

	// 電源OFFなら操作不可
	if (!::GetVM()->IsPower()) {
		return;
	}

	::LockVM();

	// リセット＆再描画
	::GetVM()->Reset();
	GetView()->Refresh();
//	ResetCaption();

	// メモリスイッチ取得を行う
	pSRAM = (SRAM*)::GetVM()->SearchDevice(MAKEID('S', 'R', 'A', 'M'));
	ASSERT(pSRAM);
	for (i=0; i<0x100; i++) {
		Sw[i] = pSRAM->ReadOnly(0xed0000 + i);
	}

	::UnlockVM();

	// リセットメッセージをロード
	::GetMsg(IDS_RESET, strReset);

	// メモリスイッチの先頭を比較
	if (memcmp(Sw, SigTable, sizeof(DWORD) * 7) != 0) {
//		SetInfo(strReset);
		return;
	}

	// ブートデバイスを取得
	dwDevice = Sw[0x18];
	dwDevice <<= 8;
	dwDevice |= Sw[0x19];

	// ブートデバイス判別
	bFlag = FALSE;
	if (dwDevice == 0x0000) {
		// STD
		strSub = _T("STD)");
		bFlag = TRUE;
	}
	if (dwDevice == 0xa000) {
		// ROM
		dwAddr = Sw[0x0c];
		dwAddr = (dwAddr << 8) | Sw[0x0d];
		dwAddr = (dwAddr << 8) | Sw[0x0e];
		dwAddr = (dwAddr << 8) | Sw[0x0f];

		// FC0000～FC001Cと、EA0020～EA003CはSCSI#
		strSub.Format(_T("ROM $%06X)"), dwAddr);
		if ((dwAddr >= 0xfc0000) && (dwAddr < 0xfc0020)) {
			strSub.Format(_T("SCSI%1d)"), (dwAddr & 0x001f) >> 2);
		}
		if ((dwAddr >= 0xea0020) && (dwAddr < 0xea0040)) {
			strSub.Format(_T("SCSI%1d)"), (dwAddr & 0x001f) >> 2);
		}
		bFlag = TRUE;
	}
	if (dwDevice == 0xb000) {
		// RAM
		dwAddr = Sw[0x10];
		dwAddr = (dwAddr << 8) | Sw[0x11];
		dwAddr = (dwAddr << 8) | Sw[0x12];
		dwAddr = (dwAddr << 8) | Sw[0x13];
		strSub.Format(_T("RAM $%06X)"), dwAddr);
		bFlag = TRUE;
	}
	if ((dwDevice & 0xf0ff) == 0x9070) {
		strSub.Format(_T("2HD%1d)"), (dwDevice & 0xf00) >> 8);
		bFlag = TRUE;
	}
	if ((dwDevice & 0xf0ff) == 0x8000) {
		strSub.Format(_T("HD%1d)"), (dwDevice & 0xf00) >> 8);
		bFlag = TRUE;
	}
	if (!bFlag) {
		strSub = _T("Unknown)");
	}

	// 表示
	strReset += _T(" (");
	strReset += strSub;
//	SetInfo(strReset);
}

//---------------------------------------------------------------------------
//
//	リセット UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnResetUI(CCmdUI *pCmdUI)
{
	// 電源ONなら操作できる
	pCmdUI->Enable(::GetVM()->IsPower());
}

//---------------------------------------------------------------------------
//
//	SRAMシグネチャテーブル
//
//---------------------------------------------------------------------------
const DWORD CFrmWnd::SigTable[] = {
	0x82, 0x77, 0x36, 0x38, 0x30, 0x30, 0x30
};

//---------------------------------------------------------------------------
//
//	フロッピーディスク処理
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFD(UINT uID)
{
	int nDrive;

	// ドライブ決定
	nDrive = 0;
	if (uID >= IDM_D1OPEN) {
		nDrive = 1;
		uID -= (IDM_D1OPEN - IDM_D0OPEN);
	}

	switch (uID) {
		// オープン
		case IDM_D0OPEN:
			OnFDOpen(nDrive);
			break;

/*
		// イジェクト
		case IDM_D0EJECT:
			OnFDEject(nDrive);
			break;

		// 書き込み保護
		case IDM_D0WRITEP:
			OnFDWriteP(nDrive);
			break;

		// 強制イジェクト
		case IDM_D0FORCE:
			OnFDForce(nDrive);
			break;

		// 誤挿入
		case IDM_D0INVALID:
			OnFDInvalid(nDrive);
			break;
*/
		// それ以外
		default:
/*
			if (uID >= IDM_D0_MRU0) {
				// MRU
				uID -= IDM_D0_MRU0;
				ASSERT(uID <= 8);
				OnFDMRU(nDrive, (int)uID);
			}
			else {
				// Media
				uID -= IDM_D0_MEDIA0;
				ASSERT(uID <= 15);
				OnFDMedia(nDrive, (int)uID);
			}
*/
		break;
	}
}

//---------------------------------------------------------------------------
//
//	フロッピーオープン
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDOpen(int nDrive)
{
	Filepath path;
	CString strMsg;
	TCHAR szPath[_MAX_PATH];
	FDI *pFDI;

	ASSERT((nDrive == 0) || (nDrive == 1));
	ASSERT(m_pFDD);

	// コモンダイアログ実行
	memset(szPath, 0, sizeof(szPath));
	if (!::FileOpenDlg(this, szPath, IDS_FDOPEN)) {
//		ResetCaption();
		return;
	}
	path.SetPath(szPath);

	// VMロック
	::LockVM();

	// ディスク割り当て
	if (!m_pFDD->Open(nDrive, path)) {
//		GetScheduler()->Reset();
		::UnlockVM();

		// オープンエラー
		::GetMsg(IDS_FDERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
//		ResetCaption();
		return;
	}

	// VMをリスタートさせる前に、FDIを取得しておく
	pFDI = m_pFDD->GetFDI(nDrive);

	// 成功
//	GetScheduler()->Reset();
//	ResetCaption();
	::UnlockVM();

	// MRUに追加
//	GetConfig()->SetMRUFile(nDrive, szPath);

	// 成功なら、BADイメージ警告
	if (pFDI->GetID() == MAKEID('B', 'A', 'D', ' ')) {
		::GetMsg(IDS_BADFDI_WARNING, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
	}
}

//---------------------------------------------------------------------------
//
//	フロッピーオープン UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDOpenUI(CCmdUI *pCmdUI)
{
	CMenu *pMenu;
	CMenu *pSubMenu;
	UINT nEnable;
	int nDrive;
	int nStat;
	int nDisks;
	int nMedia;
	char szShort[_MAX_PATH];
	LPTSTR lpszShort;
	int i;
//	TCHAR szMRU[_MAX_PATH];
//	TCHAR szDrive[_MAX_DRIVE];
//	TCHAR szDir[_MAX_DIR];
//	TCHAR szFile[_MAX_FNAME];
//	TCHAR szExt[_MAX_EXT];

	ASSERT(this);
	ASSERT(m_pFDD);

	// ドライブ決定
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// イジェクト禁止で、ディスクあり以外はオープンできる
	::LockVM();
	nStat = m_pFDD->GetStatus(nDrive);
	m_nFDDStatus[nDrive] = nStat;
	nDisks = m_pFDD->GetDisks(nDrive);
	nMedia = m_pFDD->GetMedia(nDrive);
	::UnlockVM();
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		pCmdUI->Enable(FALSE);
	}
	else {
		pCmdUI->Enable(TRUE);
	}

	// サブメニュー取得
	if (m_bPopupMenu) {
		pMenu = m_PopupMenu.GetSubMenu(0);
	}
	else {
		pMenu = &m_Menu;
	}
	// ファイル(F)の次にフロッピー0、フロッピー1と並ぶ
	pSubMenu = pMenu->GetSubMenu(nDrive + 1);

	// イジェクトUI(以下、ON_UPDATE_COMMAND_UIのタイミング対策)
	if ((nStat & FDST_INSERT) && (nStat & FDST_EJECT)) {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_GRAYED);
	}

	// 書き込み保護UI
	if (m_pFDD->IsReadOnly(nDrive) || !(nStat & FDST_INSERT)) {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_GRAYED);
	}
	else {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_ENABLED);
	}

	// 強制イジェクトUI
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_GRAYED);
	}

	// 誤挿入UI
	if (!(nStat & FDST_INSERT) && !(nStat & FDST_INVALID)) {
		pSubMenu->EnableMenuItem(5, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(5, MF_BYPOSITION | MF_GRAYED);
	}

	// 以降のメニューはすべて削除
	while (pSubMenu->GetMenuItemCount() > 6) {
		pSubMenu->RemoveMenu(6, MF_BYPOSITION);
	}

	// マルチディスク処理
	if (nDisks > 1) {
		// 有効・無効定数設定
		if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
			nEnable = MF_BYCOMMAND | MF_ENABLED;
		}
		else {
			nEnable = MF_BYCOMMAND | MF_GRAYED;
		}

		// セパレータを挿入
		pSubMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);

		// メディアループ
		ASSERT(nDisks <= 16);
		for (i=0; i<nDisks; i++) {
			// ディスク名はchar*で格納されている為、TCHARへ変換
			m_pFDD->GetName(nDrive, szShort, i);
			lpszShort = A2T(szShort);

			// 追加
			if (nDrive == 0) {
				pSubMenu->AppendMenu(MF_STRING, IDM_D0_MEDIA0 + i, lpszShort);
				pSubMenu->EnableMenuItem(IDM_D0_MEDIA0 + i, nEnable);
			}
			else {
				pSubMenu->AppendMenu(MF_STRING, IDM_D1_MEDIA0 + i, lpszShort);
				pSubMenu->EnableMenuItem(IDM_D1_MEDIA0 + i, nEnable);
			}
		}

		// ラジオボタン設定
		if (nDrive == 0) {
			pSubMenu->CheckMenuRadioItem(IDM_D0_MEDIA0, IDM_D0_MEDIAF,
										IDM_D0_MEDIA0 + nMedia, MF_BYCOMMAND);
		}
		else {
			pSubMenu->CheckMenuRadioItem(IDM_D1_MEDIA0, IDM_D1_MEDIAF,
										IDM_D1_MEDIA0 + nMedia, MF_BYCOMMAND);
		}
	}
}
#endif	// _WIN32
