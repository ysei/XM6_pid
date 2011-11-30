//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ コンフィギュレーション ]
//
//---------------------------------------------------------------------------

#if !defined(config_h)
#define config_h

#include "filepath.h"

//===========================================================================
//
//	コンフィギュレーション(version2.00～version2.01)
//
//===========================================================================
class Config200 {
public:
	typedef XM6_pid::FiosPath Path;

	enum {
		FILE_PATH_MAX	= 260,
	};
	// システム
	int system_clock;					// システムクロック(0～5)
	int ram_size;						// メインRAMサイズ(0～5)
	int ram_sramsync;					// メモリスイッチ自動更新

	// スケジューラ
	int mpu_fullspeed;					// MPUフルスピード
	int vm_fullspeed;					// VMフルスピード

	// サウンド
	int sound_device;					// サウンドデバイス(0～15)
	int sample_rate;					// サンプリングレート(0～4)
	int primary_buffer;					// バッファサイズ(2～100)
	int polling_buffer;					// ポーリング間隔(0～99)
	int adpcm_interp;					// ADPCM線形補間あり

	// 描画
	int aspect_stretch;				// アスペクト比にあわせ拡大

	// 音量
	int master_volume;					// マスタ音量(0～100)
	int fm_enable;						// FM有効
	int fm_volume;						// FM音量(0～100)
	int adpcm_enable;					// ADPCM有効
	int adpcm_volume;					// ADPCM音量(0～100)

	// キーボード
	int kbd_connect;					// 接続

	// マウス
	int mouse_speed;					// スピード
	int mouse_port;						// 接続ポート
	int mouse_swap;					// ボタンスワップ
	int mouse_mid;						// 中ボタンイネーブル
	int mouse_trackb;					// トラックボールモード

	// ジョイスティック
	int joy_type[2];					// ジョイスティックタイプ
	int joy_dev[2];						// ジョイスティックデバイス
	int joy_button0[12];				// ジョイスティックボタン(デバイスA)
	int joy_button1[12];				// ジョイスティックボタン(デバイスB)

	// SASI
	int sasi_drives;					// SASIドライブ数
	int sasi_sramsync;					// SASIメモリスイッチ自動更新
	Path	sasi_file[16];	// SASIイメージファイル

	// SxSI
	int sxsi_drives;					// SxSIドライブ数
	int sxsi_mofirst;					// MOドライブ優先割り当て
	Path	sxsi_file[6];	// SxSIイメージファイル

	// ポート
	int port_com;						// COMxポート
	Path	port_recvlog;	// シリアル受信ログ
	int port_384;						// シリアル38400bps固定
	int port_lpt;						// LPTxポート
	Path	port_sendlog;	// パラレル送信ログ

	// MIDI
	int midi_bid;						// MIDIボードID
	int midi_ilevel;					// MIDI割り込みレベル
	int midi_reset;						// MIDIリセットコマンド
	int midiin_device;					// MIDI INデバイス
	int midiin_delay;					// MIDI INディレイ(ms)
	int midiout_device;					// MIDI OUTデバイス
	int midiout_delay;					// MIDI OUTディレイ(ms)

	// 改造
	int sram_64k;						// 64KB SRAM
	int scc_clkup;						// SCCクロックアップ
	int power_led;						// 青色電源LED
	int dual_fdd;						// 2DD/2HD兼用FDD
	int sasi_parity;					// SASIバスパリティ

	// TrueKey
	int tkey_mode;						// TrueKeyモード(bit0:VM bit1:WinApp)
	int tkey_com;						// キーボードCOMポート
	int tkey_rts;						// RTS反転モード

	// その他
	int floppy_speed;					// フロッピーディスク高速
	int floppy_led;					// フロッピーディスクLEDモード
	int popup_swnd;					// ポップアップサブウィンドウ
	int auto_mouse;					// 自動マウスモード制御
	int power_off;						// 電源OFFで開始
};

//===========================================================================
//
//	コンフィギュレーション(version2.02～version2.03)
//
//===========================================================================
class Config202 : public Config200 {
public:
	// システム
	int mem_type;						// メモリマップ種別

	// SCSI
	int scsi_ilevel;					// SCSI割り込みレベル
	int scsi_drives;					// SCSIドライブ数
	int scsi_sramsync;					// SCSIメモリスイッチ自動更新
	int scsi_mofirst;					// MOドライブ優先割り当て
	Path	scsi_file[5];	// SCSIイメージファイル
};

//===========================================================================
//
//	コンフィギュレーション
//
//===========================================================================
class Config : public Config202 {
public:
	// レジューム
	int resume_fd;						// FDレジューム
	int resume_fdi[2];					// FD挿入フラグ
	int resume_fdw[2];					// FD書き込み禁止
	int resume_fdm[2];					// FDメディアNo.
	int resume_mo;						// MOレジューム
	int resume_mos;					// MO挿入フラグ
	int resume_mow;					// MO書き込み禁止
	int resume_cd;						// CDレジューム
	int resume_iso;					// CD挿入フラグ
	int resume_state;					// ステートレジューム
	int resume_xm6;					// ステート有効フラグ
	int resume_screen;					// 画面モードレジューム
	int resume_dir;					// デフォルトディレクトリレジューム
	Path	resume_path;	// デフォルトディレクトリ

	// 描画
	int caption_info;					// キャプション情報表示

	// ディスプレイ
	int caption;						// キャプション
	int menu_bar;						// メニューバー
	int status_bar;					// ステータスバー
	int window_left;					// ウィンドウ矩形
	int window_top;						// ウィンドウ矩形
	int window_full;					// フルスクリーン
	int window_mode;					// ワイドスクリーン

	// WINDRVモジュール
	uint32_t windrv_enable;				// Windrvサポート 0:無効 1:WindrvXM (2:Windrv互換)

	// ホスト側ファイルシステム
	uint32_t host_option;					// 動作フラグ (class CHostFilename 参照)
	int host_resume;					// ベースパス状態復元有効 FALSEだと毎回スキャンする
	uint32_t host_drives;					// 有効なドライブ数
	uint32_t host_flag[10];				// 動作フラグ (class CWinFileDrv 参照)
	Path	host_path[10];		// ベースパス
};

#endif	// config_h
