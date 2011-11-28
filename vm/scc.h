//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2003 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ SCC(Z8530) ]
//
//---------------------------------------------------------------------------

#if !defined(scc_h)
#define scc_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	SCC
//
//===========================================================================
class SCC : public MemDevice
{
public:
	// 割り込みタイプ
	enum itype_t {
		rxi,							// 受信割り込み
		rsi,							// スペシャルRxコンディション割り込み
		txi,							// 送信割り込み
		exti							// 外部ステータス変化割り込み
	};

	// チャネル定義
	typedef struct {
		// グローバル
		uint32_t index;					// チャネル番号(0 or 1)

		// RR0
		int ba;						// Break/Abort
		int tu;						// Txアンダーラン
		int cts;						// CTS
		int sync;						// SYNC
		int dcd;						// DCD
		int zc;						// ゼロカウント

		// WR0
		uint32_t reg;						// アクセスレジスタ選択
		int ph;						// ポイントハイ(上位レジスタ選択)
		int txpend;					// 送信割り込みペンディング
		int rxno;						// 受信データなし

		// RR1
		int framing;					// フレーミングエラー
		int overrun;					// オーバーランエラー
		int parerr;					// パリティエラー
		int txsent;					// 送信完了

		// WR1
		int extie;						// 外部ステータス割り込み許可
		int txie;						// 送信割り込み許可
		int parsp;						// パリティエラーをS-Rx割り込みにする
		uint32_t rxim;						// 受信割り込みモード

		// RR3
		int rxip;						// 受信割り込みペンディング
		int rsip;						// スペシャルRx割り込みペンディング
		int txip;						// 送信割り込みペンディング
		int extip;						// 外部ステータス変化割り込みペンディング

		// WR3
		uint32_t rxbit;					// 受信キャラクタビット長(5-8)
		int aen;						// オートモードイネーブル
		int rxen;						// 受信イネーブル

		// WR4
		uint32_t clkm;						// クロックモード
		uint32_t stopbit;					// ストップビット
		uint32_t parity;					// パリティモード

		// WR5
		int dtr;						// DTR信号線
		uint32_t txbit;					// 送信キャラクタビット長(5-8)
		int brk;						// ブレーク送出
		int txen;						// 送信イネーブル
		int rts;						// RTS信号線

		// WR8
		uint32_t tdr;						// 送信データレジスタ
		int tdf;						// 送信データ有効

		// WR12, WR13
		uint32_t tc;						// ボーレート設定値

		// WR14
		int loopback;					// ループバックモード
		int aecho;						// オートエコーモード
		int dtrreq;					// DTR信号線有効
		int brgsrc;					// ボーレートジェネレータクロック源
		int brgen;						// ボーレートジェネレータイネーブル

		// WR15
		int baie;						// Break/Abort割り込みイネーブル
		int tuie;						// Txアンダーラン割り込みイネーブル
		int ctsie;						// CTS割り込みイネーブル
		int syncie;					// SYNC割り込みイネーブル
		int dcdie;						// DCD割り込みイネーブル
		int zcie;						// ゼロカウント割り込みイネーブル

		// 通信速度
		uint32_t baudrate;					// ボーレート
		uint32_t cps;						// キャラクタ/sec
		uint32_t speed;					// 速度(hus単位)

		// 受信FIFO
		uint32_t rxfifo;					// 受信FIFO有効数
		uint32_t rxdata[3];				// 受信FIFOデータ

		// 受信バッファ
		uint8_t rxbuf[0x1000];				// 受信データ
		uint32_t rxnum;					// 受信データ数
		uint32_t rxread;					// 受信読み込みポインタ
		uint32_t rxwrite;					// 受信書き込みポインタ
		uint32_t rxtotal;					// 受信トータル

		// 送信バッファ
		uint8_t txbuf[0x1000];				// 送信データ
		uint32_t txnum;					// 送信データ数
		uint32_t txread;					// 送信読み込みポインタ
		uint32_t txwrite;					// 送信書き込みポインタ
		uint32_t txtotal;					// Txトータル
		int txwait;					// Txウェイトフラグ
	} ch_t;

	// 内部データ定義
	typedef struct {
		// チャネル
		ch_t ch[2];						// チャネルデータ

		// RR2
		uint32_t request;					// 割り込みベクタ(要求中)

		// WR2
		uint32_t vbase;					// 割り込みベクタ(ベース)

		// WR9
		int shsl;						// ベクタ変化モードb4-b6/b3-b1
		int mie;						// 割り込みイネーブル
		int dlc;						// 下位チェーン禁止
		int nv;						// 割り込みベクタ出力イネーブル
		int vis;						// 割り込みベクタ変化モード

		int ireq;						// 要求中の割り込みタイプ
		int vector;						// 要求中のベクタ
	} scc_t;

public:
	// 基本ファンクション
	SCC(VM *p);
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
	void FASTCALL GetSCC(scc_t *buffer) const;
										// 内部データ取得
	const SCC::scc_t* FASTCALL GetWork() const;
										// ワーク取得 
	uint32_t FASTCALL GetVector(int type) const;
										// ベクタ取得
	int FASTCALL Callback(Event *ev);
										// イベントコールバック
	void FASTCALL IntAck();
										// 割り込み応答

	// 送信API(SCCへ送信)
	void FASTCALL Send(int channel, uint32_t data);
										// データ送信
	void FASTCALL ParityErr(int channel);
										// パリティエラーの生成
	void FASTCALL FramingErr(int channel);
										// フレーミングエラーの生成
	void FASTCALL SetBreak(int channel, int flag);
										// ブレーク状態の通知
	int FASTCALL IsRxEnable(int channel) const;
										// 受信チェック
	int FASTCALL IsBaudRate(int channel, uint32_t baudrate) const;
										// ボーレートチェック
	uint32_t FASTCALL GetRxBit(int channel) const;
										// 受信データビット数取得
	uint32_t FASTCALL GetStopBit(int channel) const;
										// ストップビット取得
	uint32_t FASTCALL GetParity(int channel) const;
										// パリティ取得
	int FASTCALL IsRxBufEmpty(int channel) const;
										// 受信バッファの空きチェック

	// 受信API(SCCから受信)
	uint32_t FASTCALL Receive(int channel);
										// データ受信
	int FASTCALL IsTxEmpty(int channel);
										// 送信バッファエンプティチェック
	int FASTCALL IsTxFull(int channel);
										// 送信バッファフルチェック
	void FASTCALL WaitTx(int channel, int wait);
										// 送信ブロック

	// ハードフロー
	void FASTCALL SetCTS(int channel, int flag);
										// CTSセット
	void FASTCALL SetDCD(int channel, int flag);
										// DCDセット
	int FASTCALL GetRTS(int channel);
										// RTS取得
	int FASTCALL GetDTR(int channel);
										// DTR取得
	int FASTCALL GetBreak(int channel);
										// ブレーク取得

private:
	void FASTCALL ResetCh(ch_t *p);
										// チャネルリセット
	uint32_t FASTCALL ReadSCC(ch_t *p, uint32_t reg);
										// チャネル読み出し
	uint32_t FASTCALL ReadRR0(const ch_t *p) const;
										// RR0読み出し
	uint32_t FASTCALL ReadRR1(const ch_t *p) const;
										// RR1読み出し
	uint32_t FASTCALL ReadRR2(ch_t *p);
										// RR2読み出し
	uint32_t FASTCALL ReadRR3(const ch_t *p) const;
										// RR3読み出し
	uint32_t FASTCALL ReadRR8(ch_t *p);
										// RR8読み出し
	uint32_t FASTCALL ReadRR15(const ch_t *p) const;
										// RR15読み出し
	uint32_t FASTCALL ROSCC(const ch_t *p, uint32_t reg) const;
										// 読み出しのみ
	void FASTCALL WriteSCC(ch_t *p, uint32_t reg, uint32_t data);
										// チャネル書き込み
	void FASTCALL WriteWR0(ch_t *p, uint32_t data);
										// WR0書き込み
	void FASTCALL WriteWR1(ch_t *p, uint32_t data);
										// WR1書き込み
	void FASTCALL WriteWR3(ch_t *p, uint32_t data);
										// WR3書き込み
	void FASTCALL WriteWR4(ch_t *p, uint32_t data);
										// WR4書き込み
	void FASTCALL WriteWR5(ch_t *p, uint32_t data);
										// WR5書き込み
	void FASTCALL WriteWR8(ch_t *p, uint32_t data);
										// WR8書き込み
	void FASTCALL WriteWR9(uint32_t data);
										// WR9書き込み
	void FASTCALL WriteWR10(ch_t *p, uint32_t data);
										// WR10書き込み
	void FASTCALL WriteWR11(ch_t *p, uint32_t data);
										// WR11書き込み
	void FASTCALL WriteWR12(ch_t *p, uint32_t data);
										// WR12書き込み
	void FASTCALL WriteWR13(ch_t *p, uint32_t data);
										// WR13書き込み
	void FASTCALL WriteWR14(ch_t *p, uint32_t data);
										// WR14書き込み
	void FASTCALL WriteWR15(ch_t *p, uint32_t data);
										// WR15書き込み
	void FASTCALL ResetSCC(ch_t *p);
										// リセット
	void FASTCALL ClockSCC(ch_t *p);
										// ボーレート再計算
	void FASTCALL IntSCC(ch_t *p, itype_t type, int flag);
										// 割り込みリクエスト
	void FASTCALL IntCheck();
										// 割り込みチェック
	void FASTCALL EventRx(ch_t *p);
										// イベント(受信)
	void FASTCALL EventTx(ch_t *p);
										// イベント(送信)
	Mouse *mouse;
										// マウス
	scc_t scc;
										// 内部データ
	Event event[2];
										// イベント
	int clkup;
										// 7.5MHzモード
};

#endif	// scc_h
