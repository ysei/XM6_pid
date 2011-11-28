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
		uint32_t dreg[8];					// データレジスタ
		uint32_t areg[8];					// アドレスレジスタ
		uint32_t sp;						// スタック予備(USP or SSP)
		uint32_t pc;						// プログラムカウンタ
		uint32_t intr[8];					// 割り込み情報
		uint32_t sr;						// ステータスレジスタ
		uint32_t intreq[8];				// 割り込み要求回数
		uint32_t intack[8];				// 割り込み受理回数
		uint32_t odd;						// 実行カウンタ
	} cpu_t;

	typedef struct {
		uint32_t erraddr;					// エラーアドレス
		uint32_t errtime;					// エラー時の仮想時間
		uint32_t intreq[8];				// 割り込み要求回数
		uint32_t intack[8];				// 割り込み受理回数
	} cpusub_t;

public:
	// 基本ファンクション
	CPU(VM *p);
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

public:
	void BeginProgramRegion(int isSuper);
	int  AddProgramRegion(unsigned int lowaddr, unsigned int highaddr, unsigned int offset);
	void EndProgramRegion();

	void BeginDataRegion(int isSuper, int isWrite, int isWord);
	int  AddDataRegion(unsigned int lowaddr, unsigned int highaddr, void* memorycall, void* userdata);
	void EndDataRegion();

	// 外部API
	void FASTCALL GetCPU(cpu_t *buffer) const;
										// CPUレジスタ取得
	void FASTCALL SetCPU(const cpu_t *buffer);
										// CPUレジスタ設定
	uint32_t FASTCALL Exec(int cycle);
										// 実行
	int FASTCALL Interrupt(int level, int vector);
										// 割り込み
	void FASTCALL IntAck(int level);
										// 割り込みACK
	void FASTCALL IntCancel(int level);
										// 割り込みキャンセル
	uint32_t FASTCALL GetCycle() const;
										// サイクル数取得
	uint32_t FASTCALL GetPC() const;
										// プログラムカウンタ取得
	void FASTCALL ResetInst();
										// RESET命令
	uint32_t FASTCALL GetIOCycle()	const;	// dma.cpp
										// I/Oサイクル取得
	void FASTCALL Release();
										// CPU実行を現命令で強制終了
	void FASTCALL BusErr(uint32_t addr, int read);
										// バスエラー
	void FASTCALL AddrErr(uint32_t addr, int read);
										// アドレスエラー
	void FASTCALL BusErrLog(uint32_t addr, uint32_t stat);
										// バスエラー記録
	void FASTCALL AddrErrLog(uint32_t addr, uint32_t stat);
										// アドレスエラー記録
	void FASTCALL Wait(uint32_t cycle);
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
