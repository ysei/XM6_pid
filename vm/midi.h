//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MIDI(YM3802) ]
//
//---------------------------------------------------------------------------

#if !defined(midi_h)
#define midi_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	MIDI
//
//===========================================================================
class MIDI : public MemDevice
{
public:
	// 定数定義
	enum {
		TransMax = 0x2000,				// 送信バッファ個数
		RecvMax = 0x2000				// 受信バッファ個数
	};

	// MIDIバイトデータ定義
	typedef struct {
		uint32_t data;						// データ実体(8bit)
		uint32_t vtime;					// 仮想時間
	} mididata_t;

	// 内部データ定義
	typedef struct {
		// リセット
		int reset;						// リセットフラグ
		int access;					// アクセスフラグ

		// ボードデータ、割り込み
		uint32_t bid;						// ボードID(0:ボード無し)
		uint32_t ilevel;					// 割り込みレベル
		int vector;						// 割り込み要求ベクタ

		// MCSレジスタ(一般)
		uint32_t wdr;						// 書き込みデータレジスタ
		uint32_t rgr;						// レジスタグループレジスタ

		// MCSレジスタ(割り込み)
		uint32_t ivr;						// 割り込みベクタレジスタ
		uint32_t isr;						// 割り込みサービスレジスタ
		uint32_t imr;						// 割り込みモードレジスタ
		uint32_t ier;						// 割り込み許可レジスタ

		// MCSレジスタ(リアルタイムメッセージ)
		uint32_t dmr;						// リアルタイムメッセージモードレジスタ
		uint32_t dcr;						// リアルタイムメッセージコントロールレジスタ

		// MCSレジスタ(受信)
		uint32_t rrr;						// 受信レートレジスタ
		uint32_t rmr;						// 受信モードレジスタ
		uint32_t amr;						// アドレスハンタモードレジスタ
		uint32_t adr;						// アドレスハンタデバイスレジスタ
		uint32_t asr;						// アドレスハンタステータスレジスタ
		uint32_t rsr;						// 受信バッファステータスレジスタ
		uint32_t rcr;						// 受信バッファコントロールレジスタ
		uint32_t rcn;						// 無受信カウンタ

		// MCSレジスタ(送信)
		uint32_t trr;						// 送信レートレジスタ
		uint32_t tmr;						// 送信モードレジスタ
		int tbs;						// 送信BUSYレジスタ
		uint32_t tcr;						// 送信コントロールレジスタ
		uint32_t tcn;						// 無送信カウンタ

		// MCSレジスタ(FSK)
		uint32_t fsr;						// FSKステータスレジスタ
		uint32_t fcr;						// FSKコントロールレジスタ

		// MCSレジスタ(カウンタ)
		uint32_t ccr;						// クリックコントロールレジスタ
		uint32_t cdr;						// クリックデータレジスタ
		uint32_t ctr;						// クリックタイマレジスタ
		uint32_t srr;						// レコーディングカウンタレジスタ
		uint32_t scr;						// クロック補間レジスタ
		uint32_t sct;						// クロック補間カウンタ
		uint32_t spr;						// プレイバックカウンタレジスタ
		uint32_t str;						// プレイバックタイマレジスタ
		uint32_t gtr;						// 汎用タイマレジスタ
		uint32_t mtr;						// MIDIクロックタイマレジスタ

		// MCSレジスタ(GPIO)
		uint32_t edr;						// 外部ポートディレクションレジスタ
		uint32_t eor;						// 外部ポートOutputレジスタ
		uint32_t eir;						// 外部ポートInputレジスタ

		// 通常バッファ
		uint32_t normbuf[16];				// 通常バッファ
		uint32_t normread;					// 通常バッファRead
		uint32_t normwrite;				// 通常バッファWrite
		uint32_t normnum;					// 通常バッファ個数
		uint32_t normtotal;				// 通常バッファトータル

		// リアルタイム送信バッファ
		uint32_t rtbuf[4];					// リアルタイム送信バッファ
		uint32_t rtread;					// リアルタイム送信バッファRead
		uint32_t rtwrite;					// リアルタイム送信バッファWrite
		uint32_t rtnum;					// リアルタイム送信バッファ個数
		uint32_t rttotal;					// リアルタイム送信バッファトータル

		// 一般バッファ
		uint32_t stdbuf[0x80];				// 一般バッファ
		uint32_t stdread;					// 一般バッファRead
		uint32_t stdwrite;					// 一般バッファWrite
		uint32_t stdnum;					// 一般バッファ個数
		uint32_t stdtotal;					// 一般バッファトータル

		// リアルタイム受信バッファ
		uint32_t rrbuf[4];					// リアルタイム受信バッファ
		uint32_t rrread;					// リアルタイム受信バッファRead
		uint32_t rrwrite;					// リアルタイム受信バッファWrite
		uint32_t rrnum;					// リアルタイム受信バッファ個数
		uint32_t rrtotal;					// リアルタイム受信バッファトータル

		// 送信バッファ(デバイスとの受け渡し用)
		mididata_t *transbuf;			// 送信バッファ
		uint32_t transread;				// 送信バッファRead
		uint32_t transwrite;				// 送信バッファWrite
		uint32_t transnum;					// 送信バッファ個数

		// 受信バッファ(デバイスとの受け渡し用)
		mididata_t *recvbuf;			// 受信バッファ
		uint32_t recvread;					// 受信バッファRead
		uint32_t recvwrite;				// 受信バッファWrite
		uint32_t recvnum;					// 受信バッファ個数
	} midi_t;

public:
	// 基本ファンクション
	MIDI(VM *p);
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
#if !defined(NDEBUG)
	void FASTCALL AssertDiag() const;
										// 診断
#endif	// NDEBUG

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
	int FASTCALL IsActive() const;
										// MIDIアクティブチェック
	int FASTCALL Callback(Event *ev);
										// イベントコールバック
	void FASTCALL IntAck(int level);
										// 割り込みACK
	void FASTCALL GetMIDI(midi_t *buffer) const;
										// 内部データ取得
	uint32_t FASTCALL GetExCount(int index) const;
										// エクスクルーシブカウント取得

	// 送信(MIDI OUT)
	uint32_t FASTCALL GetTransNum() const;
										// 送信バッファ個数取得
	const mididata_t* FASTCALL GetTransData(uint32_t proceed);
										// 送信バッファデータ取得
	void FASTCALL DelTransData(uint32_t number);
										// 送信バッファ削除
	void FASTCALL ClrTransData();
										// 送信バッファクリア

	// 受信(MIDI IN)
	void FASTCALL SetRecvData(const uint8_t *ptr, uint32_t length);
										// 受信データ設定
	void FASTCALL SetRecvDelay(int delay);
										// 受信ディレイ設定

	// リセット
	int FASTCALL IsReset() const		{ return midi.reset; }
										// リセットフラグ取得
	void FASTCALL ClrReset()			{ midi.reset = FALSE; }
										// リセットフラグクリア

private:
	void FASTCALL Receive();
										// 受信コールバック
	void FASTCALL Transmit();
										// 送信コールバック
	void FASTCALL Clock();
										// MIDIクロック検出
	void FASTCALL General();
										// 汎用タイマコールバック

	void FASTCALL InsertTrans(uint32_t data);
										// 送信バッファへ挿入
	void FASTCALL InsertRecv(uint32_t data);
										// 受信バッファへ挿入
	void FASTCALL InsertNorm(uint32_t data);
										// 通常バッファへ挿入
	void FASTCALL InsertRT(uint32_t data);
										// リアルタイム送信バッファへ挿入
	void FASTCALL InsertStd(uint32_t data);
										// 一般バッファへ挿入
	void FASTCALL InsertRR(uint32_t data);
										// リアルタイム受信バッファへ挿入

	void FASTCALL ResetReg();
										// レジスタリセット
	uint32_t FASTCALL ReadReg(uint32_t reg);
										// レジスタ読み出し
	void FASTCALL WriteReg(uint32_t reg, uint32_t data);
										// レジスタ書き込み
	uint32_t FASTCALL ReadRegRO(uint32_t reg) const;
										// レジスタ読み出し(ReadOnly)

	void FASTCALL SetICR(uint32_t data);
										// ICR設定
	void FASTCALL SetIOR(uint32_t data);
										// IOR設定
	void FASTCALL SetIMR(uint32_t data);
										// IMR設定
	void FASTCALL SetIER(uint32_t data);
										// IER設定
	void FASTCALL SetDMR(uint32_t data);
										// DMR設定
	void FASTCALL SetDCR(uint32_t data);
										// DCR設定
	uint32_t FASTCALL GetDSR() const;
										// DSR取得
	void FASTCALL SetDNR(uint32_t data);
										// DNR設定
	void FASTCALL SetRRR(uint32_t data);
										// RRR設定
	void FASTCALL SetRMR(uint32_t data);
										// RMR設定
	void FASTCALL SetAMR(uint32_t data);
										// AMR設定
	void FASTCALL SetADR(uint32_t data);
										// ADR設定
	uint32_t FASTCALL GetRSR() const;
										// RSR取得
	void FASTCALL SetRCR(uint32_t data);
										// RCR設定
	uint32_t FASTCALL GetRDR();
										// RDR取得(更新あり)
	uint32_t FASTCALL GetRDRRO() const;
										// RDR取得(Read Only)
	void FASTCALL SetTRR(uint32_t data);
										// TRR設定
	void FASTCALL SetTMR(uint32_t data);
										// TMR設定
	uint32_t FASTCALL GetTSR() const;
										// TSR取得
	void FASTCALL SetTCR(uint32_t data);
										// TCR設定
	void FASTCALL SetTDR(uint32_t data);
										// TDR設定
	uint32_t FASTCALL GetFSR() const;
										// FSR取得
	void FASTCALL SetFCR(uint32_t data);
										// FCR設定
	void FASTCALL SetCCR(uint32_t data);
										// CCR設定
	void FASTCALL SetCDR(uint32_t data);
										// CDR設定
	uint32_t FASTCALL GetSRR() const;
										// SRR取得
	void FASTCALL SetSCR(uint32_t data);
										// SCR設定
	void FASTCALL SetSPR(uint32_t data, int high);
										// SPR設定
	void FASTCALL SetGTR(uint32_t data, int high);
										// GTR設定
	void FASTCALL SetMTR(uint32_t data, int high);
										// MTR設定
	void FASTCALL SetEDR(uint32_t data);
										// EDR設定
	void FASTCALL SetEOR(uint32_t data);
										// EOR設定
	uint32_t FASTCALL GetEIR() const;
										// EIR取得

	void FASTCALL CheckRR();
										// リアルタイムメッセージ受信バッファチェック
	void FASTCALL Interrupt(int type, int flag);
										// 割り込み発生
	void FASTCALL IntCheck();
										// 割り込みチェック
	Event event[3];
										// イベント
	midi_t midi;
										// 内部データ
	Sync *sync;
										// データSync
	uint32_t recvdelay;
										// 受信遅れ時間(hus)
	uint32_t ex_cnt[4];
										// エクスクルーシブカウント
};

#endif	// midi_h
