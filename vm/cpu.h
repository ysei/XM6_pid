//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ CPU(MC68000) ]
//
//---------------------------------------------------------------------------

#if !defined(cpu_h)
#define cpu_h

#include "device.h"
//#include "starcpu.h"

//===========================================================================
//
//	CPU
//
//===========================================================================
class CPU : public Device
{
public:
	// 内部データ定義
	typedef struct {
		DWORD dreg[8];					// データレジスタ
		DWORD areg[8];					// アドレスレジスタ
		DWORD sp;						// スタック予備(USP or SSP)
		DWORD pc;						// プログラムカウンタ
		DWORD intr[8];					// 割り込み情報
		DWORD sr;						// ステータスレジスタ
		DWORD intreq[8];				// 割り込み要求回数
		DWORD intack[8];				// 割り込み受理回数
		DWORD odd;						// 実行カウンタ
	} cpu_t;

	typedef struct {
		DWORD erraddr;					// エラーアドレス
		DWORD errtime;					// エラー時の仮想時間
		DWORD intreq[8];				// 割り込み要求回数
		DWORD intack[8];				// 割り込み受理回数
	} cpusub_t;

public:
	// 基本ファンクション
	CPU(VM *p);
										// コンストラクタ
	BOOL FASTCALL Init();
										// 初期化
	void FASTCALL Cleanup();
										// クリーンアップ
	void FASTCALL Reset();
										// リセット
	BOOL FASTCALL Save(Fileio *fio, int ver);
										// セーブ
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ロード
	void FASTCALL ApplyCfg(const Config *config);
										// 設定適用

public:
	void BeginProgramRegion(BOOL isSuper);
	int  AddProgramRegion(unsigned int lowaddr, unsigned int highaddr, unsigned int offset);
	void EndProgramRegion();

	void BeginDataRegion(BOOL isSuper, BOOL isWrite, BOOL isWord);
	int  AddDataRegion(unsigned int lowaddr, unsigned int highaddr, void* memorycall, void* userdata);
	void EndDataRegion();

	// 外部API
	void FASTCALL GetCPU(cpu_t *buffer) const;
										// CPUレジスタ取得
	void FASTCALL SetCPU(const cpu_t *buffer);
										// CPUレジスタ設定
	DWORD FASTCALL Exec(int cycle);
										// 実行
	BOOL FASTCALL Interrupt(int level, int vector);
										// 割り込み
	void FASTCALL IntAck(int level);
										// 割り込みACK
	void FASTCALL IntCancel(int level);
										// 割り込みキャンセル
	DWORD FASTCALL GetCycle() const;
										// サイクル数取得
	DWORD FASTCALL GetPC() const;
										// プログラムカウンタ取得
	void FASTCALL ResetInst();
										// RESET命令
	DWORD FASTCALL GetIOCycle()	const;	// dma.cpp
										// I/Oサイクル取得
	void FASTCALL Release();
										// CPU実行を現命令で強制終了
	void FASTCALL BusErr(DWORD addr, BOOL read);
										// バスエラー
	void FASTCALL AddrErr(DWORD addr, BOOL read);
										// アドレスエラー
	void FASTCALL BusErrLog(DWORD addr, DWORD stat);
										// バスエラー記録
	void FASTCALL AddrErrLog(DWORD addr, DWORD stat);
										// アドレスエラー記録
	void FASTCALL Wait(DWORD cycle);
										// CPUウェイト
private:
	cpusub_t sub;
										// 内部データ
	Memory *memory;
										// メモリ
	DMAC *dmac;
										// DMAC
	MFP *mfp;
										// MFP
	IOSC *iosc;
										// IOSC
	SCC *scc;
										// SCC
	MIDI *midi;
										// MIDI
	SCSI *scsi;
										// SCSI
	Scheduler *scheduler;
										// スケジューラ
	// リージョン (Starscream特有)
	struct Region;
	Region* pRegion;
};

#endif	// cpu_h
