//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFP(MC68901) ]
//
//---------------------------------------------------------------------------

#if !defined(mfp_h)
#define mfp_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	MFP
//
//===========================================================================
class MFP : public MemDevice
{
public:
	// 内部データ定義
	typedef struct {
		// 割り込み
		int ier[0x10];					// 割り込みイネーブルレジスタ
		int ipr[0x10];					// 割り込みペンディングレジスタ
		int isr[0x10];					// 割り込みインサービスレジスタ
		int imr[0x10];					// 割り込みマスクレジスタ
		int ireq[0x10];				// 割り込みリクエストレジスタ
		uint32_t vr;						// ベクタレジスタ
		int iidx;						// 割り込みインデックス

		// タイマ
		uint32_t tcr[4];					// タイマコントロールレジスタ
		uint32_t tdr[4];					// タイマデータレジスタ
		uint32_t tir[4];					// タイマインターナルレジスタ
		uint32_t tbr[2];					// タイマバックアップレジスタ
		uint32_t sram;						// si, info.ram対策フラグ
		uint32_t tecnt;					// イベントカウントモードカウンタ

		// GPIP
		uint32_t gpdr;						// GPIPデータレジスタ
		uint32_t aer;						// アクティブエッジレジスタ
		uint32_t ddr;						// データ方向レジスタ
		uint32_t ber;						// バックアップエッジレジスタ

		// USART
		uint32_t scr;						// SYNCキャラクタレジスタ
		uint32_t ucr;						// USARTコントロールレジスタ
		uint32_t rsr;						// レシーバステータスレジスタ
		uint32_t tsr;						// トランスミッタステータスレジスタ
		uint32_t rur;						// レシーバユーザレジスタ
		uint32_t tur;						// トランスミッタユーザレジスタ
		uint32_t buffer[0x10];				// USART FIFOバッファ
		int datacount;					// USART 有効データ数
		int readpoint;					// USART MFP読み取りポイント
		int writepoint;					// USART キーボード書き込みポイント
	} mfp_t;

public:
	// 基本ファンクション
	MFP(VM *p);
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
	void FASTCALL GetMFP(mfp_t *buffer) const;
										// 内部データ取得
	int FASTCALL Callback(Event *ev);
										// イベントコールバック
	void FASTCALL IntAck();
										// 割り込み応答
	void FASTCALL EventCount(int channel, int value);
										// イベントカウント
	void FASTCALL SetGPIP(int num, int value);
										// GPIP設定
	void FASTCALL KeyData(uint32_t data);
										// キーデータ設定
	uint32_t FASTCALL GetVR() const;
										// ベクタレジスタ取得

private:
	// 割り込みコントロール
	void FASTCALL Interrupt(int level, int enable);
										// 割り込み
	void FASTCALL IntCheck();
										// 割り込み優先順位チェック
	void FASTCALL SetIER(int offset, uint32_t data);
										// IER設定
	uint32_t FASTCALL GetIER(int offset) const;
										// IER取得
	void FASTCALL SetIPR(int offset, uint32_t data);
										// IPR設定
	uint32_t FASTCALL GetIPR(int offset) const;
										// IPR取得
	void FASTCALL SetISR(int offset, uint32_t data);
										// ISR設定
	uint32_t FASTCALL GetISR(int offset) const;
										// ISR取得
	void FASTCALL SetIMR(int offset, uint32_t data);
										// IMR設定
	uint32_t FASTCALL GetIMR(int offset) const;
										// IMR設定
	void FASTCALL SetVR(uint32_t data);
										// VR設定
	static const char* IntDesc[0x10];
										// 割り込み名称テーブル

	// タイマ
	void FASTCALL SetTCR(int channel, uint32_t data);
										// TCR設定
	uint32_t FASTCALL GetTCR(int channel) const;
										// TCR取得
	void FASTCALL SetTDR(int channel, uint32_t data);
										// TDR設定
	uint32_t FASTCALL GetTIR(int channel) const;
										// TIR取得
	void FASTCALL Proceed(int channel);
										// タイマを進める
	Event timer[4];
										// タイマイベント
	static const int TimerInt[4];
										// タイマ割り込みテーブル
	static const uint32_t TimerHus[8];
										// タイマ時間テーブル

	// GPIP
	void FASTCALL SetGPDR(uint32_t data);
										// GPDR設定
	void FASTCALL IntGPIP();
										// GPIP割り込み
	static const int GPIPInt[8];
										// GPIP割り込みテーブル

	// USART
	void FASTCALL SetRSR(uint32_t data);
										// RSR設定
	void FASTCALL Receive();
										// USARTデータ受信
	void FASTCALL SetTSR(uint32_t data);
										// TSR設定
	void FASTCALL Transmit(uint32_t data);
										// USARTデータ送信
	void FASTCALL USART();
										// USART処理
	Event usart;
										// USARTイベント
	Sync *sync;
										// USART Sync
	Keyboard *keyboard;
										// キーボード
	mfp_t mfp;
										// 内部データ
};

#endif	// mfp_h
