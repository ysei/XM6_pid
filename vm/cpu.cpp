//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ CPU(MC68000) ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "os.h"
#include "xm6.h"
#include "iosc.h"
#include "mfp.h"
#include "vm.h"
#include "log.h"
#include "memory_xm6.h"
#include "dmac.h"
#include "scc.h"
#include "midi.h"
#include "scsi.h"
#include "fileio.h"
#include "cpu.h"
#include "starcpu.h"

struct CPU::Region {
	enum {
		REGION_MAX = 10
	};
	STARSCREAM_PROGRAMREGION u_pgr[REGION_MAX];		// プログラムリージョン(User)
	STARSCREAM_PROGRAMREGION s_pgr[REGION_MAX];		// プログラムリージョン(Super)
	STARSCREAM_DATAREGION u_rbr[REGION_MAX];		// Read Byteリージョン(User)
	STARSCREAM_DATAREGION s_rbr[REGION_MAX];		// Read Byteリージョン(Super)
	STARSCREAM_DATAREGION u_rwr[REGION_MAX];		// Read Wordリージョン(User)
	STARSCREAM_DATAREGION s_rwr[REGION_MAX];		// Read Wordリージョン(Super)
	STARSCREAM_DATAREGION u_wbr[REGION_MAX];		// Write Byteリージョン(User)
	STARSCREAM_DATAREGION s_wbr[REGION_MAX];		// Write Byteリージョン(Super)
	STARSCREAM_DATAREGION u_wwr[REGION_MAX];		// Write Wordリージョン(User)
	STARSCREAM_DATAREGION s_wwr[REGION_MAX];		// Write Wordリージョン(Super)
	STARSCREAM_PROGRAMREGION* pProgramRegion;
	int	iProgramRegion;
	STARSCREAM_DATAREGION* pDataRegion;
	int	iDataRegion;
};

//---------------------------------------------------------------------------
//
//	アセンブラコアとのインタフェース
//
//---------------------------------------------------------------------------
#if defined(__cplusplus)
extern "C" {
#endif	// __cplusplus

//---------------------------------------------------------------------------
//
//	スタティック ワーク
//
//---------------------------------------------------------------------------
static CPU *cpu;						// CPU

//---------------------------------------------------------------------------
//
//	外部定義
//
//---------------------------------------------------------------------------
uint32_t s68000fbpc(void);							// PCフィードバック
void s68000buserr(uint32_t addr, uint32_t param);	// バスエラー
extern uint32_t s68000getcounter();					// クロックカウンタ取得
extern uint32_t s68000iocycle;						// __io_cycle_counter(Starscream)

//---------------------------------------------------------------------------
//
//	RESET命令ハンドラ
//
//---------------------------------------------------------------------------
static void cpu_resethandler(void)
{
	cpu->ResetInst();
}

//---------------------------------------------------------------------------
//
//	割り込みACK
//
//---------------------------------------------------------------------------
void s68000intack(void)
{
	int sr;

	sr = ::s68000context.sr;
	sr >>= 8;
	sr &= 0x0007;

	cpu->IntAck(sr);
}

//---------------------------------------------------------------------------
//
//	バスエラー記録
//
//---------------------------------------------------------------------------
void s68000buserrlog(uint32_t addr, uint32_t stat)
{
	cpu->BusErrLog(addr, stat);
}

//---------------------------------------------------------------------------
//
//	アドレスエラー記録
//
//---------------------------------------------------------------------------
void s68000addrerrlog(uint32_t addr, uint32_t stat)
{
	cpu->AddrErrLog(addr, stat);
}

#if defined(__cplusplus)
}
#endif	// __cplusplus

//===========================================================================
//
//	CPU
//
//===========================================================================
#if defined(CPU_LOG)
#undef  CPU_LOG
#define CPU_LOG(...)	__VA_ARGS__
#else
#define	CPU_LOG(...)
#endif

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CPU::CPU(VM *p) : Device(p)
{
	// デバイスIDを初期化
	dev.id = XM6_MAKEID('C', 'P', 'U', ' ');
	dev.desc = "MPU (MC68000)";

	// ポインタ初期化
	memory = NULL;
	dmac = NULL;
	mfp = NULL;
	iosc = NULL;
	scc = NULL;
	midi = NULL;
	scsi = NULL;
	scheduler = NULL;

	pRegion = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
int FASTCALL CPU::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!Device::Init()) {
		return FALSE;
	}

	// CPU記憶
	::cpu = this;

	// メモリ取得
	memory = (Memory*)vm->SearchDevice(XM6_MAKEID('M', 'E', 'M', ' '));
	ASSERT(memory);

	// DMAC取得
	dmac = (DMAC*)vm->SearchDevice(XM6_MAKEID('D', 'M', 'A', 'C'));
	ASSERT(dmac);

	// MFP取得
	mfp = (MFP*)vm->SearchDevice(XM6_MAKEID('M', 'F', 'P', ' '));
	ASSERT(mfp);

	// IOSC取得
	iosc = (IOSC*)vm->SearchDevice(XM6_MAKEID('I', 'O', 'S', 'C'));
	ASSERT(iosc);

	// SCC取得
	scc = (SCC*)vm->SearchDevice(XM6_MAKEID('S', 'C', 'C', ' '));
	ASSERT(scc);

	// MIDI取得
	midi = (MIDI*)vm->SearchDevice(XM6_MAKEID('M', 'I', 'D', 'I'));
	ASSERT(midi);

	// SCSI取得
	scsi = (SCSI*)vm->SearchDevice(XM6_MAKEID('S', 'C', 'S', 'I'));
	ASSERT(scsi);

	// スケジューラ取得
	scheduler = (Scheduler*)vm->SearchDevice(XM6_MAKEID('S', 'C', 'H', 'E'));
	ASSERT(scheduler);

	// リージョンエリアを設定
	ASSERT(pRegion == 0);
	pRegion = new Region;
	::s68000context.u_fetch			= pRegion->u_pgr;
	::s68000context.s_fetch			= pRegion->s_pgr;
	::s68000context.u_readbyte		= pRegion->u_rbr;
	::s68000context.s_readbyte		= pRegion->s_rbr;
	::s68000context.u_readword		= pRegion->u_rwr;
	::s68000context.s_readword		= pRegion->s_rwr;
	::s68000context.u_writebyte		= pRegion->u_wbr;
	::s68000context.s_writebyte		= pRegion->s_wbr;
	::s68000context.u_writeword		= pRegion->u_wwr;
	::s68000context.s_writeword		= pRegion->s_wwr;
	pRegion->pProgramRegion = NULL;
	pRegion->iProgramRegion = -1;
	pRegion->pDataRegion = NULL;
	pRegion->iDataRegion = -1;

	// CPUコアのジャンプテーブルを作成
	::s68000init();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL CPU::Cleanup()
{
	ASSERT(this);

	if(pRegion) {
		delete pRegion;
		pRegion = 0;
	}

	// 基本クラスへ
	Device::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL CPU::Reset()
{
	int i;
	S68000CONTEXT context;
	uint32_t bit;

	ASSERT(this);
	LOG0(Log::Normal, "リセット");

	// エラーアドレス、エラー時間クリア
	sub.erraddr = 0;
	sub.errtime = 0;

	// 割り込みカウントクリア
	for (i=0; i<8; i++) {
		sub.intreq[i] = 0;
		sub.intack[i] = 0;
	}

	// メモリコンテキスト作成(リセット専用)
	memory->MakeContext(TRUE);

	// リセット
	::s68000reset();
	::s68000context.resethandler = cpu_resethandler;
	::s68000context.odometer = 0;

	// 割り込みをすべて取り消す
	::s68000GetContext(&context);
	for (i=1; i<=7; i++) {
		bit = (1 << i);
		if (context.interrupts[0] & bit) {
			context.interrupts[0] &= (uint8_t)(~bit);
			context.interrupts[i] = 0;
		}
	}
	::s68000SetContext(&context);

	// メモリコンテキスト作成(通常)
	memory->MakeContext(FALSE);
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
int FASTCALL CPU::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;
	cpu_t cpu;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "セーブ");

	// コンテキスト取得
	GetCPU(&cpu);

	// サイズをセーブ
	sz = sizeof(cpu_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 実体をセーブ
	if (!fio->Write(&cpu, (int)sz)) {
		return FALSE;
	}

	// サイズをセーブ(サブ)
	sz = sizeof(cpusub_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 実体をセーブ(サブ)
	if (!fio->Write(&sub, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
int FASTCALL CPU::Load(Fileio *fio, int /*ver*/)
{
	cpu_t cpu;
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "ロード");

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(cpu_t)) {
		return FALSE;
	}

	// 実体をロード
	if (!fio->Read(&cpu, (int)sz)) {
		return FALSE;
	}

	// 適用(リセットしてから行う)
	memory->MakeContext(TRUE);
	::s68000reset();
	memory->MakeContext(FALSE);
	SetCPU(&cpu);

	// サイズをロード、照合(サブ)
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(cpusub_t)) {
		return FALSE;
	}

	// 実体をロード(サブ)
	if (!fio->Read(&sub, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL CPU::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);

	LOG0(Log::Normal, "設定適用");
}

//---------------------------------------------------------------------------
//
//	CPUレジスタ取得
//
//---------------------------------------------------------------------------
void FASTCALL CPU::GetCPU(cpu_t *buffer) const
{
	int i;

	ASSERT(this);
	ASSERT(buffer);

	// Dreg, Areg
	for (i=0; i<8; i++) {
		buffer->dreg[i] = ::s68000context.dreg[i];
		buffer->areg[i] = ::s68000context.areg[i];
	}

	// 割り込み
	for (i=0; i<8; i++) {
		buffer->intr[i] = (uint32_t)::s68000context.interrupts[i];
		buffer->intreq[i] = sub.intreq[i];
		buffer->intack[i] = sub.intack[i];
	}

	// その他
	buffer->sp = ::s68000context.asp;
	buffer->pc = ::s68000context.pc;
	buffer->sr = (uint32_t)::s68000context.sr;
	buffer->odd = ::s68000context.odometer;
}

//---------------------------------------------------------------------------
//
//	CPUレジスタ設定
//
//---------------------------------------------------------------------------
void FASTCALL CPU::SetCPU(const cpu_t *buffer)
{
	int i;
	S68000CONTEXT context;

	ASSERT(this);
	ASSERT(buffer);

	// コンテキスト取得
	::s68000GetContext(&context);

	// Dreg, Areg
	for (i=0; i<8; i++) {
		context.dreg[i] = buffer->dreg[i];
		context.areg[i] = buffer->areg[i];
	}

	// 割り込み
	for (i=0; i<8; i++) {
		context.interrupts[i] = (uint8_t)buffer->intr[i];
		sub.intreq[i] = buffer->intreq[i];
		sub.intack[i] = buffer->intack[i];
	}

	// その他
	context.asp = buffer->sp;
	context.pc = buffer->pc;
	context.sr = (uint16_t)buffer->sr;
	context.odometer = buffer->odd;

	// コンテキスト設定
	::s68000SetContext(&context);
}

//---------------------------------------------------------------------------
//
//	割り込み
//
//---------------------------------------------------------------------------
int FASTCALL CPU::Interrupt(int level, int vector)
{
	int ret;

	// INTERRUPT SWITCHによるNMI割り込みはベクタ-1
	ASSERT(this);
	ASSERT((level >= 1) && (level <= 7));
	ASSERT(vector >= -1);

	// リクエスト
	ret = ::s68000interrupt(level, vector);

	// 結果評価
	if (ret == 0) {
		CPU_LOG(LOG2(Log::Normal, "割り込み要求受理 レベル%d ベクタ$%02X", level, vector));
		sub.intreq[level]++;
		return TRUE;
	}

	return FALSE;
}

//---------------------------------------------------------------------------
//
//	割り込みACK
//
//---------------------------------------------------------------------------
void FASTCALL CPU::IntAck(int level)
{
	ASSERT(this);
	ASSERT((level >= 1) && (level <= 7));

	CPU_LOG(LOG1(Log::Normal, "割り込み要求ACK レベル%d", level));

	// カウントアップ
	sub.intack[level]++;

	// 割り込みレベル別
	switch (level) {
		// IOSC,SCSI(内蔵)
		case 1:
			iosc->IntAck();
			scsi->IntAck(1);
			break;

		// MIDI,SCSI(レベル2)
		case 2:
			midi->IntAck(2);
			scsi->IntAck(2);
			break;

		// DMAC
		case 3:
			dmac->IntAck();
			break;

		// MIDI,SCSI(レベル4)
		case 4:
			midi->IntAck(4);
			scsi->IntAck(4);
			break;

		// SCC
		case 5:
			scc->IntAck();
			break;

		// MFP
		case 6:
			mfp->IntAck();
			break;

		// その他
		default:
			break;
	}
}

//---------------------------------------------------------------------------
//
//	割り込みキャンセル
//
//---------------------------------------------------------------------------
void FASTCALL CPU::IntCancel(int level)
{
	S68000CONTEXT context;
	uint32_t bit;

	ASSERT(this);
	ASSERT((level >= 1) && (level <= 7));

	// コンテキストを直接書き換える
	::s68000GetContext(&context);

	// 該当ビットがオンなら
	bit = (1 << level);
	if (context.interrupts[0] & bit) {
		CPU_LOG(LOG1(Log::Normal, "割り込みキャンセル レベル%d", level));

		// ビットを降ろす
		context.interrupts[0] &= (uint8_t)(~bit);

		// ベクタは0
		context.interrupts[level] = 0;

		// リクエストを下げる
		sub.intreq[level]--;
	}

	// コンテキストを書き込む
	::s68000SetContext(&context);
}

//---------------------------------------------------------------------------
//
//	RESET命令
//
//---------------------------------------------------------------------------
void FASTCALL CPU::ResetInst()
{
	Device *device;

	ASSERT(this);
	CPU_LOG(LOG0(Log::Detail, "RESET命令"));

	// メモリを取得
	device = (Device*)vm->SearchDevice(XM6_MAKEID('M', 'E', 'M', ' '));
	ASSERT(device);

	// メモリデバイスに対してすべてリセットをかけておく
	// 正確には、CPUのRESET信号がどこまで伝わっているかによる
	while (device) {
		device->Reset();
		device = device->GetNextDevice();
	}
}

//---------------------------------------------------------------------------
//
//	バスエラー
//	※DMA転送によるバスエラーもここに来る
//	※CPUコア内部でバスエラーと判定した場合は、ここを経由しない
//
//---------------------------------------------------------------------------
void FASTCALL CPU::BusErr(uint32_t addr, int read)
{
	uint32_t pc;
	uint32_t stat;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);

	// DMACに転送中か聞く。DMAC中ならDMACに任せる
	if (dmac->IsDMA()) {
		dmac->BusErr(addr, read);
		return;
	}

	// アドレスが前回のアドレス+2で、かつ時間が同じなら無視する(LONGアクセス)
	if (addr == (sub.erraddr + 2)) {
		if (scheduler->GetTotalTime() == sub.errtime) {
			return;
		}
	}

	// アドレスと時間を更新
	sub.erraddr = addr;
	sub.errtime = scheduler->GetTotalTime();

	// PC取得(該当命令のオペコードに位置する)
	pc = GetPC();

	// 読み出し(Word)
	stat = memory->ReadOnly(pc);
	stat <<= 8;
	stat |= memory->ReadOnly(pc + 1);
	stat <<= 16;

	// ファンクションコード作成(常にデータアクセスとみなす)
	stat |= 0x09;
	if (::s68000context.sr & 0x2000) {
		stat |= 0x04;
	}
	if (read) {
		stat |= 0x10;
	}

	// バスエラー発行
	::s68000buserr(addr, stat);
}

//---------------------------------------------------------------------------
//
//	アドレスエラー
//	※DMA転送によるアドレスエラーもここに来る
//	※CPUコア内部でアドレスエラーと判定した場合は、ここを経由しない
//
//---------------------------------------------------------------------------
void FASTCALL CPU::AddrErr(uint32_t addr, int read)
{
	uint32_t pc;
	uint32_t stat;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(addr & 1);

	// DMACに転送中か聞く。DMAC中ならDMACに任せる
	if (dmac->IsDMA()) {
		dmac->AddrErr(addr, read);
		return;
	}

	// アドレスが前回のアドレス+2で、かつ時間が同じなら無視する(LONGアクセス)
	if (addr == (sub.erraddr + 2)) {
		if (scheduler->GetTotalTime() == sub.errtime) {
			return;
		}
	}

	// アドレスと時間を更新
	sub.erraddr = addr;
	sub.errtime = scheduler->GetTotalTime();

	// PC取得(該当命令のオペコードに位置する)
	pc = GetPC();

	// 読み出し(Word)
	stat = memory->ReadOnly(pc);
	stat <<= 8;
	stat |= memory->ReadOnly(pc + 1);
	stat <<= 16;

	// ファンクションコード作成(常にデータアクセスとみなす)
	stat |= 0x8009;
	if (::s68000context.sr & 0x2000) {
		stat |= 0x04;
	}
	if (read) {
		stat |= 0x10;
	}

	// バスエラー発行(内部でアドレスエラーへ分岐)
	::s68000buserr(addr, stat);
}

//---------------------------------------------------------------------------
//
//	バスエラー記録
//	※CPUコア内部でバスエラーと判定した場合も、ここを通る
//
//---------------------------------------------------------------------------
void FASTCALL CPU::BusErrLog(uint32_t addr, uint32_t stat)
{
	ASSERT(this);

	// 必ずマスク(24bitを超える場合がある)
	addr &= 0xffffff;

	if (stat & 0x10) {
		LOG1(Log::Warning, "バスエラー(読み込み) $%06X", addr);
	}
	else {
		LOG1(Log::Warning, "バスエラー(書き込み) $%06X", addr);
	}
}

//---------------------------------------------------------------------------
//
//	アドレスエラー記録
//	※CPUコア内部でアドレスエラーと判定した場合も、ここを通る
//
//---------------------------------------------------------------------------
void FASTCALL CPU::AddrErrLog(uint32_t addr, uint32_t stat)
{
	ASSERT(this);

	// 必ずマスク(24bitを超える場合がある)
	addr &= 0xffffff;

	if (stat & 0x10) {
		LOG1(Log::Warning, "アドレスエラー(読み込み) $%06X", addr);
	}
	else {
		LOG1(Log::Warning, "アドレスエラー(書き込み) $%06X", addr);
	}
}

void CPU::BeginProgramRegion(int isSuper) {
	ASSERT(pRegion->iProgramRegion == -1);
	ASSERT(pRegion->pProgramRegion == 0);

	pRegion->iProgramRegion = 0;
	if(isSuper) {
		pRegion->pProgramRegion = &pRegion->s_pgr[0];
	} else {
		pRegion->pProgramRegion = &pRegion->u_pgr[0];
	}
}

int  CPU::AddProgramRegion(unsigned int lowaddr, unsigned int highaddr, unsigned int offset) {
	ASSERT(pRegion->iProgramRegion >= 0 && pRegion->iProgramRegion < Region::REGION_MAX);
	ASSERT(pRegion->pProgramRegion);

	int i = pRegion->iProgramRegion++;
	STARSCREAM_PROGRAMREGION* p = &pRegion->pProgramRegion[i];
	p->lowaddr	= lowaddr;
	p->highaddr	= highaddr;
	p->offset	= offset;
	return i;
}

void CPU::EndProgramRegion() {
	ASSERT(pRegion->iProgramRegion >= 0 && pRegion->iProgramRegion < Region::REGION_MAX);
	ASSERT(pRegion->pProgramRegion);

	AddProgramRegion((unsigned int)-1, (unsigned int)-1, 0);

	pRegion->iProgramRegion = -1;
	pRegion->pProgramRegion = 0;
}

void CPU::BeginDataRegion(int isSuper, int isWrite, int isWord) {
	ASSERT(pRegion->iDataRegion == -1);
	ASSERT(pRegion->pDataRegion == 0);

	STARSCREAM_DATAREGION* p = 0;

	pRegion->iDataRegion = 0;
	if(isSuper) {
		//	super
		if(! isWrite) {
			//	super, read
			if(! isWord) {
				//	super, read, byte
				p = &pRegion->s_rbr[0];
			} else {
				//	super, read, word
				p = &pRegion->s_rwr[0];
			}
		} else {
			//	super, write
			if(! isWord) {
				//	super, write, byte
				p = &pRegion->s_wbr[0];
			} else {
				//	super, write, word
				p = &pRegion->s_wwr[0];
			}
		}
	} else {
		//	user
		if(! isWrite) {
			//	user, read
			if(! isWord) {
				//	user, read, byte
				p = &pRegion->u_rbr[0];
			} else {
				//	user, read, word
				p = &pRegion->u_rwr[0];
			}
		} else {
			//	user, write
			if(! isWord) {
				//	user, write, byte
				p = &pRegion->u_wbr[0];
			} else {
				//	user, write, word
				p = &pRegion->u_wwr[0];
			}
		}
	}

	ASSERT(p);
	pRegion->pDataRegion = p;
}

int  CPU::AddDataRegion(unsigned int lowaddr, unsigned int highaddr, void* memorycall, void* userdata) {
	ASSERT(pRegion->iDataRegion >= 0 && pRegion->iDataRegion < Region::REGION_MAX);
	ASSERT(pRegion->pDataRegion);

	int i = pRegion->iDataRegion++;
	STARSCREAM_DATAREGION* p = &pRegion->pDataRegion[i];
	p->lowaddr		= lowaddr;
	p->highaddr		= highaddr;
	p->memorycall	= memorycall;
	p->userdata		= userdata;
	return i;
}

void CPU::EndDataRegion() {
	ASSERT(pRegion->iDataRegion >= 0 && pRegion->iDataRegion < Region::REGION_MAX);
	ASSERT(pRegion->pDataRegion);

	AddDataRegion((unsigned int)-1, (unsigned int)-1, 0, 0);

	pRegion->iDataRegion = -1;
	pRegion->pDataRegion = 0;
}

uint32_t FASTCALL CPU::Exec(int cycle) {
	uint32_t result;

	if (::s68000exec(cycle) <= 0x80000000) {
		result = ::s68000context.odometer;
		::s68000context.odometer = 0;
		return result;
	}

	result = ::s68000context.odometer;
	result |= 0x80000000;
	::s68000context.odometer = 0;
	return result;
}

void FASTCALL CPU::Wait(uint32_t cycle) {
	//	TODO : This function is called very frequently. So, original Scheduler::Wait() calls StarScream directly. Like this :
	//
	//		void FASTCALL Wait(uint32_t cycle)		{ sch.cycle += cycle; if (CPU_IOCYCLE_GET() != (uint32_t)-1) CPU_IOCYCLE_SUBTRACT(cycle); }
	//
	::s68000wait(cycle);
}

uint32_t FASTCALL CPU::GetIOCycle() const {
	return ::s68000getcounter();
}

void FASTCALL CPU::Release() {
	::s68000releaseTimeslice();
}

uint32_t FASTCALL CPU::GetCycle() const {
	return ::s68000readOdometer();
}

uint32_t FASTCALL CPU::GetPC() const {
	return ::s68000readPC();
}
