//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ ビデオコントローラ(CATHY & VIPS) ]
//
//---------------------------------------------------------------------------

#if !defined(vc_h)
#define vc_h

#include "device.h"

//===========================================================================
//
//	ビデオコントローラ
//
//===========================================================================
class VC : public MemDevice
{
public:
	// 内部データ定義
	typedef struct {
		uint32_t vr1h;						// VR1(H)バックアップ
		uint32_t vr1l;						// VR1(H)バックアップ
		uint32_t vr2h;						// VR2(H)バックアップ
		uint32_t vr2l;						// VR2(H)バックアップ
		int siz;						// 実画面サイズ
		uint32_t col;						// 色モード
		uint32_t sp;						// スプライトプライオリティ
		uint32_t tx;						// テキストプライオリティ
		uint32_t gr;						// グラフィックプライオリティ(1024)
		uint32_t gp[4];					// グラフィックプライオリティ(512)
		int ys;						// Ys信号
		int ah;						// テキストパレット半透明
		int vht;						// 外部ビデオ半透明
		int exon;						// 特殊プライオリティ・半透明
		int hp;						// 半透明
		int bp;						// 最下位ビット半透明フラグ
		int gg;						// グラフィック半透明
		int gt;						// テキスト半透明
		int bcon;						// シャープ予約
		int son;						// スプライトON
		int ton;						// テキストON
		int gon;						// グラフィックON(実画面1024時)
		int gs[4];						// グラフィックON(実画面512時)
	} vc_t;

public:
	// 基本ファンクション
	VC(VM *p);
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
	uint32_t FASTCALL ReadWord(uint32_t addr);
										// ワード読み込み
	void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// バイト書き込み
	void FASTCALL WriteWord(uint32_t addr, uint32_t data);
										// ワード書き込み
	uint32_t FASTCALL ReadOnly(uint32_t addr) const;
										// 読み込みのみ

	// 外部API
	void FASTCALL GetVC(vc_t *buffer);
										// 内部データ取得
	const uint8_t* FASTCALL GetPalette() const	{ return palette; }
										// パレットRAM取得
	const vc_t* FASTCALL GetWorkAddr() const{ return &vc; }
										// ワークアドレス取得

private:
	// レジスタアクセス
	void FASTCALL SetVR0L(uint32_t data);
										// レジスタ0(L)設定
	uint32_t FASTCALL GetVR0() const;
										// レジスタ0取得
	void FASTCALL SetVR1H(uint32_t data);
										// レジスタ1(H)設定
	void FASTCALL SetVR1L(uint32_t data);
										// レジスタ1(L)設定
	uint32_t FASTCALL GetVR1() const;
										// レジスタ1取得
	void FASTCALL SetVR2H(uint32_t data);
										// レジスタ2(H)設定
	void FASTCALL SetVR2L(uint32_t data);
										// レジスタ2(L)設定
	uint32_t FASTCALL GetVR2() const;
										// レジスタ2取得

	// データ
	Render *render;
										// レンダラ
	vc_t vc;
										// 内部データ
	uint8_t palette[0x400];
										// パレットRAM
};

#endif	// vc_h
