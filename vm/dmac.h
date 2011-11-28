//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ DMAC(HD63450) ]
//
//---------------------------------------------------------------------------

#if !defined(dmac_h)
#define dmac_h

#include "device.h"

//===========================================================================
//
//	DMAC
//
//===========================================================================
class DMAC : public MemDevice
{
public:
	// 内部データ定義(チャネル別)
	typedef struct {
		// 基本パラメータ
		uint32_t xrm;						// リクエストモード
		uint32_t dtyp;						// デバイスタイプ
		int dps;						// ポートサイズ (TRUEで16bit)
		uint32_t pcl;						// PCLセレクタ
		int dir;						// 方向 (TRUEでDAR→メモリ)
		int btd;						// DONEで次ブロックへ
		uint32_t size;						// オペランドサイズ
		uint32_t chain;					// チェイン動作
		uint32_t reqg;						// REQ生成モード
		uint32_t mac;						// メモリアドレス更新モード
		uint32_t dac;						// デバイスアドレス更新モード

		// 制御フラグ
		int str;						// スタートフラグ
		int cnt;						// コンティニューフラグ
		int hlt;						// HALTフラグ
		int sab;						// ソフトウェアアボートフラグ
		int intr;						// 割り込み可能フラグ
		int coc;						// チャンネル動作完了フラグ
		int boc;						// ブロック動作完了フラグ
		int ndt;						// 正常終了フラグ
		int err;						// エラーフラグ
		int act;						// アクティブフラグ
		int dit;						// DONE入力フラグ
		int pct;						// PCL negedge検出フラグ
		int pcs;						// PCLの状態 (TRUEでHレベル)
		uint32_t ecode;					// エラーコード

		// アドレス、レングス
		uint32_t mar;						// メモリアドレスカウンタ
		uint32_t dar;						// デバイスアドレスレジスタ
		uint32_t bar;						// ベースアドレスレジスタ
		uint32_t mtc;						// メモリトランスファカウンタ
		uint32_t btc;						// ベーストランスファカウンタ
		uint32_t mfc;						// メモリファンクションコード
		uint32_t dfc;						// デバイスファンクションコード
		uint32_t bfc;						// ベースファンクションコード
		uint32_t niv;						// ノーマルインタラプトベクタ
		uint32_t eiv;						// エラーインタラプトベクタ

		// バースト転送
		uint32_t cp;						// プライオリティ
		uint32_t bt;						// バースト転送タイム
		uint32_t br;						// バンド幅
		int type;						// 転送タイプ

		// 動作カウンタ(デバッグ向け)
		uint32_t startcnt;					// スタートカウンタ
		uint32_t errorcnt;					// エラーカウンタ
	} dma_t;

	// 内部データ定義(グローバル)
	typedef struct {
		int transfer;					// 転送中フラグ(チャネル兼用)
		int load;						// チェインロードフラグ(チャネル兼用)
		int exec;						// オートリクエスト有無フラグ
		int current_ch;					// オートリクエスト処理チャネル
		int cpu_cycle;					// CPUサイクルカウンタ
		int vector;						// 割り込み要求中ベクタ
	} dmactrl_t;

public:
	// 基本ファンクション
	DMAC(VM *p);
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
										// ワード読み込み
	uint32_t FASTCALL ReadOnly(uint32_t addr) const;
										// 読み込みのみ

	// 外部API
	void FASTCALL GetDMA(int ch, dma_t *buffer) const;
										// DMA情報取得
	void FASTCALL GetDMACtrl(dmactrl_t *buffer) const;
										// DMA制御情報取得
	int FASTCALL ReqDMA(int ch);
										// DMA転送要求
	uint32_t FASTCALL AutoDMA(uint32_t cycle);
										// DMAオートリクエスト
	int FASTCALL IsDMA() const;
										// DMA転送中か問い合わせ
	void FASTCALL BusErr(uint32_t addr, int read);
										// バスエラー
	void FASTCALL AddrErr(uint32_t addr, int read);
										// アドレスエラー
	uint32_t FASTCALL GetVector(int type) const;
										// ベクタ取得
	void FASTCALL IntAck();
										// 割り込みACK
	int FASTCALL IsAct(int ch) const;
										// DMA転送可能か問い合わせ

private:
	// チャネルメモリアクセス
	uint32_t FASTCALL ReadDMA(int ch, uint32_t addr) const;
										// DMA読み込み
	void FASTCALL WriteDMA(int ch, uint32_t addr, uint32_t data);
										// DMA書き込み
	void FASTCALL SetDCR(int ch, uint32_t data);
										// DCRセット
	uint32_t FASTCALL GetDCR(int ch) const;
										// DCR取得
	void FASTCALL SetOCR(int ch, uint32_t data);
										// OCRセット
	uint32_t FASTCALL GetOCR(int ch) const;
										// OCR取得
	void FASTCALL SetSCR(int ch, uint32_t data);
										// SCRセット
	uint32_t FASTCALL GetSCR(int ch) const;
										// SCR取得
	void FASTCALL SetCCR(int ch, uint32_t data);
										// CCRセット
	uint32_t FASTCALL GetCCR(int ch) const;
										// CCR取得
	void FASTCALL SetCSR(int ch, uint32_t data);
										// CSRセット
	uint32_t FASTCALL GetCSR(int ch) const;
										// CSR取得
	void FASTCALL SetGCR(uint32_t data);
										// GCRセット

	// チャネルオペレーション
	void FASTCALL ResetDMA(int ch);
										// DMAリセット
	void FASTCALL StartDMA(int ch);
										// DMAスタート
	void FASTCALL ContDMA(int ch);
										// DMAコンティニュー
	void FASTCALL AbortDMA(int ch);
										// DMAソフトウェアアボート
	void FASTCALL LoadDMA(int ch);
										// DMAブロックロード
	void FASTCALL ErrorDMA(int ch, uint32_t code);
										// エラー
	void FASTCALL Interrupt();
										// 割り込み
	int FASTCALL TransDMA(int ch);
										// DMA1回転送

	// テーブル、内部ワーク
	static const int MemDiffTable[8][4];
										// メモリ更新テーブル
	static const int DevDiffTable[8][4];
										// デバイス更新テーブル
	Memory *memory;
										// メモリ
	FDC *fdc;
										// FDC
	dma_t dma[4];
										// 内部ワーク(チャネル)
	dmactrl_t dmactrl;
										// 内部ワーク(グローバル)
};

#endif	// dmac_h
