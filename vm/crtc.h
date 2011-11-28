//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2003 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ CRTC(VICON) ]
//
//---------------------------------------------------------------------------

#if !defined(crtc_h)
#define crtc_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	CRTC
//
//===========================================================================
class CRTC : public MemDevice
{
public:
	// 内部データ定義
	typedef struct {
		uint8_t reg[24 * 2];				// CRTCレジスタ
		int hrl;						// HRL(システムポート)
		int lowres;					// 15kHzモード
		int textres;					// 768×512モード
		int changed;					// 解像度変更フラグ

		int h_sync;						// 水平同期期間
		int h_pulse;					// 水平同期パルス幅
		int h_back;						// 水平バックポーチ
		int h_front;					// 水平フロントポーチ
		int h_dots;						// 水平ドット数
		int h_mul;						// 水平倍率
		int hd;							// 256,512,768,未定義

		int v_sync;						// 垂直同期期間(H単位)
		int v_pulse;					// 垂直同期パルス幅(H単位)
		int v_back;						// 垂直バックポーチ(H単位)
		int v_front;					// 垂直フロントポーチ(H単位)
		int v_dots;						// 垂直ドット数
		int v_mul;						// 垂直倍率(0:interlace)
		int vd;							// 256,512,未定義,未定義

		uint32_t ns;						// nsカウンタ
		uint32_t hus;						// husカウンタ
		uint32_t v_synccnt;				// V-SYNCカウンタ
		uint32_t v_blankcnt;				// V-BLANKカウンタ
		int h_disp;					// 水平表示フラグ
		int v_disp;					// V-DISPフラグ
		int v_blank;					// V-BLANKフラグ
		uint32_t v_count;					// V-DISPカウンタ
		int v_scan;						// スキャンライン

		// 以下いらない
		int h_synctime;					// 水平同期(hus)
		int h_disptime;					// 水平表示(hus)
		int v_cycletime;				// 垂直周期(hus)
		int v_blanktime;				// 垂直ブランク(hus)
		int v_synctime;					// 垂直同期(hus)
		int v_backtime;					// 垂直バックポーチ(hus)

		int tmem;						// テキストVRAM非表示
		int gmem;						// グラフィックVRAM非表示
		uint32_t siz;						// グラフィックVRAM1024×1024モード
		uint32_t col;						// グラフィックVRAM色モード

		uint32_t text_scrlx;				// テキストスクロールX
		uint32_t text_scrly;				// テキストスクロールY
		uint32_t grp_scrlx[4];				// グラフィックスクロールX
		uint32_t grp_scrly[4];				// グラフィックスクロールY

		int raster_count;				// ラスタカウンタ
		int raster_int;					// ラスタ割り込み位置
		int raster_copy;				// ラスタコピーフラグ
		int raster_exec;				// ラスタコピー実行フラグ
		uint32_t fast_clr;					// グラフィック高速クリア
	} crtc_t;

public:
	// 基本ファンクション
	CRTC(VM *p);
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

	// メモリデバイス
	uint32_t FASTCALL ReadByte(uint32_t addr);
										// バイト読み込み
	void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// バイト書き込み
	uint32_t FASTCALL ReadOnly(uint32_t addr) const;
										// 読み込みのみ

	// 外部API
	void FASTCALL GetCRTC(crtc_t *buffer) const;
										// 内部データ取得
	int FASTCALL Callback(Event *ev);
										// イベントコールバック
	void FASTCALL SetHRL(int h);
										// HRL設定
	int FASTCALL GetHRL() const;
										// HRL取得
	void FASTCALL GetHVHz(uint32_t *h, uint32_t *v) const;
										// 表示周波数取得
	uint32_t FASTCALL GetDispCount() const	{ return crtc.v_count; }
										// 表示カウンタ取得
	const crtc_t* FASTCALL GetWorkAddr() const { return &crtc; }
										// ワークアドレス取得

private:
	void FASTCALL ReCalc();
										// 再計算
	void FASTCALL HSync();
										// H-SYNC開始
	void FASTCALL HDisp();
										// H-DISP開始
	void FASTCALL VSync();
										// V-SYNC開始
	void FASTCALL VBlank();
										// V-BLANK開始
	int FASTCALL Ns2Hus(int ns)			{ return ns / 500; }
										// ns→0.5us換算
	int FASTCALL Hus2Ns(int hus)		{ return hus * 500; }
										// 0.5us→ns換算
	void FASTCALL CheckRaster();
										// ラスタ割り込みチェック
	void FASTCALL TextVRAM();
										// テキストVRAM効果
	int FASTCALL Get8DotClock() const;
										// 8ドットクロックを得る
	static const int DotClockTable[16];
										// 8ドットクロックテーブル
	static const uint8_t ResetTable[26];
										// RESETレジスタテーブル
	crtc_t crtc;
										// CRTC内部データ
	Event event;
										// イベント
	TVRAM *tvram;
										// テキストVRAM
	GVRAM *gvram;
										// グラフィックVRAM
	Sprite *sprite;
										// スプライトコントローラ
	MFP *mfp;
										// MFP
	Render *render;
										// レンダラ
	Printer *printer;
										// プリンタ
};

#endif	// crtc_h
