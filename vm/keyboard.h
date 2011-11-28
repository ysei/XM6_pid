//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ キーボード ]
//
//---------------------------------------------------------------------------

#if !defined(keyboard_h)
#define keyboard_h

#include "device.h"
#include "event.h"
#include "sync.h"

//===========================================================================
//
//	キーボード
//
//===========================================================================
class Keyboard : public Device
{
public:
	// 内部データ定義
	typedef struct {
		int connect;					// 接続フラグ
		int status[0x80];				// 押下ステータス
		uint32_t rep_code;					// リピートコード
		uint32_t rep_count;				// リピートカウンタ
		uint32_t rep_start;				// リピート時間(hus単位)
		uint32_t rep_next;					// リピート時間(hus単位)
		int send_en;					// キーデータ送信可
		int send_wait;					// キーデータ送信差し止め
		uint32_t msctrl;					// マウス制御信号
		int tv_mode;					// X68000テレビモード
		int tv_ctrl;					// コマンドによるテレビコントロール
		int opt2_ctrl;					// OPT2によるテレビコントロール
		uint32_t bright;					// キー明るさ
		uint32_t led;						// キーLED(1で点灯)
		uint32_t cmdbuf[0x10];				// コマンドバッファ
		uint32_t cmdread;					// コマンドリードポインタ
		uint32_t cmdwrite;					// コマンドライトポインタ
		uint32_t cmdnum;					// コマンド数
	} keyboard_t;

public:
	// 基本ファンクション
	Keyboard(VM *p);
										// コンストラクタ
	int FASTCALL Init();
										// 初期化
	void FASTCALL Cleanup();
										// クリーンアップ
	void FASTCALL Reset();
										// リセット
	int FASTCALL Save(Fileio *fio, int ver);
										// セーブ
	int FASTCALL Load(Fileio *fio, int ver);
										// ロード
	void FASTCALL ApplyCfg(const Config *config);
										// 設定適用

	// 外部API
	void FASTCALL Connect(int connect);
										// 接続
	int FASTCALL IsConnect() const		{ return keyboard.connect; }
										// 接続チェック
	int FASTCALL Callback(Event *ev);
										// イベントコールバック
	void FASTCALL MakeKey(uint32_t code);
										// メイク
	void FASTCALL BreakKey(uint32_t code);
										// ブレーク
	void FASTCALL Command(uint32_t data);
										// コマンド
	int FASTCALL GetCommand(uint32_t& data);
										// コマンド取得
	void FASTCALL ClrCommand();
										// コマンドクリア
	void FASTCALL SendWait(int flag);
										// キーデータ送信ウェイト
	int FASTCALL IsSendWait() const	{ return keyboard.send_wait; }
										// キーデータ送信ウェイト取得
	void FASTCALL GetKeyboard(keyboard_t *buffer) const;
										// 内部データ取得

private:
	MFP *mfp;
										// MFP
	Mouse *mouse;
										// マウス
	keyboard_t keyboard;
										// 内部データ
	Event event;
										// イベント
	Sync *sync;
										// コマンドSync
};

#endif	// keyboard_h
