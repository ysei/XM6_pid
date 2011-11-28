//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001,2002 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ レンダラ ]
//
//---------------------------------------------------------------------------

#if !defined(render_h)
#define render_h

#include "device.h"
#include "vc.h"

//===========================================================================
//
//	レンダラ
//
//===========================================================================
class Render : public Device
{
public:
	// 内部データ定義
	typedef struct {
		// 全体制御
		int act;						// 合成しているか
		int enable;					// 合成許可
		int count;						// スケジューラ連携カウンタ
		int ready;						// 描画準備できているか
		int first;						// 未処理ラスタ
		int last;						// 表示終了ラスタ

		// CRTC
		int crtc;						// CRTC変更フラグ
		int width;						// X方向ドット数(256～)
		int h_mul;						// X方向倍率(1,2)
		int height;						// Y方向ドット数(256～)
		int v_mul;						// Y方向倍率(0,1,2)
		int lowres;					// 15kHzフラグ

		// VC
		int vc;						// VC変更フラグ

		// 合成
		int mix[1024];					// 合成フラグ(ライン)
		uint32_t *mixbuf;					// 合成バッファ
		uint32_t *mixptr[8];				// 合成ポインタ
		uint32_t mixshift[8];				// 合成ポインタのYシフト
		uint32_t *mixx[8];					// 合成ポインタのXスクロールポインタ
		uint32_t *mixy[8];					// 合成ポインタのYスクロールポインタ
		uint32_t mixand[8];				// 合成ポインタのスクロールAND値
		int mixmap[3];					// 合成マップ
		int mixtype;					// 合成タイプ
		int mixpage;					// 合成グラフィックページ数
		int mixwidth;					// 合成バッファ幅
		int mixheight;					// 合成バッファ高さ
		int mixlen;						// 合成時処理長さ(x方向)

		// 描画
		int draw[1024];				// 描画フラグ(ライン)
		int *drawflag;					// 描画フラグ(16dot)

		// コントラスト
		int contrast;					// コントラスト変更フラグ
		int contlevel;					// コントラスト

		// パレット
		int palette;					// パレット変更フラグ
		int palmod[0x200];				// パレット変更フラグ
		uint32_t *palbuf;					// パレットバッファ
		uint32_t *palptr;					// パレットポインタ
		const uint16_t *palvc;				// パレットVCポインタ
		uint32_t paldata[0x200];			// パレットデータ
		uint8_t pal64k[0x200];				// パレットデータ変形

		// テキストVRAM
		int texten;					// テキスト表示フラグ
		int textpal[1024];				// テキストパレットフラグ
		int textmod[1024];				// テキスト更新フラグ(ライン)
		int *textflag;					// テキスト更新フラグ(32dot)
		uint8_t *textbuf;					// テキストバッファ(パレット前)
		uint32_t *textout;					// テキストバッファ(パレット後)
		const uint8_t *texttv;				// テキストTVRAMポインタ
		uint32_t textx;					// テキストスクロールX
		uint32_t texty;					// テキストスクロールY

		// グラフィックVRAM
		int grptype;					// グラフィックタイプ(0～4)
		int grpen[4];					// グラフィックブロック表示フラグ
		int grppal[2048];				// グラフィックパレットフラグ
		int grpmod[2048];				// グラフィック更新フラグ(ライン)
		int *grpflag;					// グラフィック更新フラグ(16dot)
		uint32_t *grpbuf[4];				// グラフィックブロックバッファ
		const uint8_t* grpgv;				// グラフィックGVRAMポインタ
		uint32_t grpx[4];					// グラフィックブロックスクロールX
		uint32_t grpy[4];					// グラフィックブロックスクロールY

		// PCG
		int pcgready[256 * 16];		// PCG準備OKフラグ
		uint32_t pcguse[256 * 16];			// PCG使用中カウント
		uint32_t pcgpal[16];				// PCGパレット使用カウント
		uint32_t *pcgbuf;					// PCGバッファ
		const uint8_t* sprmem;				// スプライトメモリ

		// スプライト
		uint32_t **spptr;					// スプライトポインタバッファ
		uint32_t spreg[0x200];				// スプライトレジスタ保存
		int spuse[128];				// スプライト使用中フラグ

		// BG
		uint32_t bgreg[2][64 * 64];		// BGレジスタ＋変更フラグ($10000)
		int bgall[2][64];				// BG変更フラグ(ブロック単位)
		int bgdisp[2];					// BG表示フラグ
		int bgarea[2];					// BG表示エリア
		int bgsize;					// BG表示サイズ(16dot=TRUE)
		uint32_t **bgptr[2];				// BGポインタ+データ
		int bgmod[2][1024];			// BG更新フラグ
		uint32_t bgx[2];					// BGスクロール(X)
		uint32_t bgy[2];					// BGスクロール(Y)

		// BG/スプライト合成
		int bgspflag;					// BG/スプライト表示フラグ
		int bgspdisp;					// BG/スプライトCPU/Videoフラグ
		int bgspmod[512];				// BG/スプライト更新フラグ
		uint32_t *bgspbuf;					// BG/スプライトバッファ
		uint32_t zero;						// スクロールダミー(0)
	} render_t;

public:
	// 基本ファンクション
	Render(VM *p);
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

	// 外部API(コントロール)
	void FASTCALL EnableAct(int enable){ render.enable = enable; }
										// 合成許可
	int FASTCALL IsActive() const		{ return render.act; }
										// アクティブか
	int FASTCALL IsReady() const		{ return (int)(render.count > 0); }
										// 描画レディ状況取得
	void FASTCALL Complete()			{ render.count = 0; }
										// 描画完了
	void FASTCALL StartFrame();
										// フレーム開始(V-DISP)
	void FASTCALL EndFrame();
										// フレーム終了(V-BLANK)
	void FASTCALL HSync(int raster)		{ render.last = raster; if (render.act) Process(); }
										// 水平同期(rasterまで終わり)
	void FASTCALL SetMixBuf(uint32_t *buf, int width, int height);
										// 合成バッファ指定
	render_t* FASTCALL GetWorkAddr() 	{ return &render; }
										// ワークアドレス取得

	// 外部API(画面)
	void FASTCALL SetCRTC();
										// CRTCセット
	void FASTCALL SetVC();
										// VCセット
	void FASTCALL SetContrast(int cont);
										// コントラスト設定
	int FASTCALL GetContrast() const;
										// コントラスト取得
	void FASTCALL SetPalette(int index);
										// パレット設定
	const uint32_t* FASTCALL GetPalette() const;
										// パレットバッファ取得
	void FASTCALL TextMem(uint32_t addr);
										// テキストVRAM変更
	void FASTCALL TextScrl(uint32_t x, uint32_t y);
										// テキストスクロール変更
	void FASTCALL TextCopy(uint32_t src, uint32_t dst, uint32_t plane);
										// ラスタコピー
	void FASTCALL GrpMem(uint32_t addr, uint32_t block);
										// グラフィックVRAM変更
	void FASTCALL GrpAll(uint32_t line, uint32_t block);
										// グラフィックVRAM変更
	void FASTCALL GrpScrl(int block, uint32_t x, uint32_t y);
										// グラフィックスクロール変更
	void FASTCALL SpriteReg(uint32_t addr, uint32_t data);
										// スプライトレジスタ変更
	void FASTCALL BGScrl(int page, uint32_t x, uint32_t y);
										// BGスクロール変更
	void FASTCALL BGCtrl(int index, int flag);
										// BGコントロール変更
	void FASTCALL BGMem(uint32_t addr,uint16_t data);
										// BG変更
	void FASTCALL PCGMem(uint32_t addr);
										// PCG変更

	const uint32_t* FASTCALL GetTextBuf() const;
										// テキストバッファ取得
	const uint32_t* FASTCALL GetGrpBuf(int index) const;
										// グラフィックバッファ取得
	const uint32_t* FASTCALL GetPCGBuf() const;
										// PCGバッファ取得
	const uint32_t* FASTCALL GetBGSpBuf() const;
										// BG/スプライトバッファ取得
	const uint32_t* FASTCALL GetMixBuf() const;
										// 合成バッファ取得

private:
	void FASTCALL Process();
										// レンダリング
	void FASTCALL Video();
										// VC処理
	void FASTCALL SetupGrp(int first);
										// グラフィックセットアップ
	void FASTCALL Contrast();
										// コントラスト処理
	void FASTCALL Palette();
										// パレット処理
	void FASTCALL MakePalette();
										// パレット作成
	uint32_t FASTCALL ConvPalette(int color, int ratio);
										// 色変換
	void FASTCALL Text(int raster);
										// テキスト
	void FASTCALL Grp(int block, int raster);
										// グラフィック
	void FASTCALL SpriteReset();
										// スプライトリセット
	void FASTCALL BGSprite(int raster);
										// BG/スプライト
	void FASTCALL BG(int page, int raster, uint32_t *buf);
										// BG
	void FASTCALL BGBlock(int page, int y);
										// BG(横ブロック)
	void FASTCALL Mix(int offset);
										// 合成
	void FASTCALL MixGrp(int y, uint32_t *buf);
										// 合成(グラフィック)
	CRTC *crtc;
										// CRTC
	VC *vc;
										// VC
	Sprite *sprite;
										// スプライト
	render_t render;
										// 内部データ
	int cmov;
										// CMOVキャッシュ
};

#endif	// render_h
