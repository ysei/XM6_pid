//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
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
	STARSCREAM_PROGRAMREGION u_pgr[REGION_MAX];		// �v���O�������[�W����(User)
	STARSCREAM_PROGRAMREGION s_pgr[REGION_MAX];		// �v���O�������[�W����(Super)
	STARSCREAM_DATAREGION u_rbr[REGION_MAX];		// Read Byte���[�W����(User)
	STARSCREAM_DATAREGION s_rbr[REGION_MAX];		// Read Byte���[�W����(Super)
	STARSCREAM_DATAREGION u_rwr[REGION_MAX];		// Read Word���[�W����(User)
	STARSCREAM_DATAREGION s_rwr[REGION_MAX];		// Read Word���[�W����(Super)
	STARSCREAM_DATAREGION u_wbr[REGION_MAX];		// Write Byte���[�W����(User)
	STARSCREAM_DATAREGION s_wbr[REGION_MAX];		// Write Byte���[�W����(Super)
	STARSCREAM_DATAREGION u_wwr[REGION_MAX];		// Write Word���[�W����(User)
	STARSCREAM_DATAREGION s_wwr[REGION_MAX];		// Write Word���[�W����(Super)
	STARSCREAM_PROGRAMREGION* pProgramRegion;
	int	iProgramRegion;
	STARSCREAM_DATAREGION* pDataRegion;
	int	iDataRegion;
};

//---------------------------------------------------------------------------
//
//	�A�Z���u���R�A�Ƃ̃C���^�t�F�[�X
//
//---------------------------------------------------------------------------
#if defined(__cplusplus)
extern "C" {
#endif	// __cplusplus

//---------------------------------------------------------------------------
//
//	�X�^�e�B�b�N ���[�N
//
//---------------------------------------------------------------------------
static CPU *cpu;						// CPU

//---------------------------------------------------------------------------
//
//	�O����`
//
//---------------------------------------------------------------------------
uint32_t s68000fbpc(void);							// PC�t�B�[�h�o�b�N
void s68000buserr(uint32_t addr, uint32_t param);	// �o�X�G���[
extern uint32_t s68000getcounter();					// �N���b�N�J�E���^�擾
extern uint32_t s68000iocycle;						// __io_cycle_counter(Starscream)

//---------------------------------------------------------------------------
//
//	RESET���߃n���h��
//
//---------------------------------------------------------------------------
static void cpu_resethandler(void)
{
	cpu->ResetInst();
}

//---------------------------------------------------------------------------
//
//	���荞��ACK
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
//	�o�X�G���[�L�^
//
//---------------------------------------------------------------------------
void s68000buserrlog(uint32_t addr, uint32_t stat)
{
	cpu->BusErrLog(addr, stat);
}

//---------------------------------------------------------------------------
//
//	�A�h���X�G���[�L�^
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
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CPU::CPU(VM *p) : Device(p)
{
	// �f�o�C�XID��������
	dev.id = XM6_MAKEID('C', 'P', 'U', ' ');
	dev.desc = "MPU (MC68000)";

	// �|�C���^������
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
//	������
//
//---------------------------------------------------------------------------
int FASTCALL CPU::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!Device::Init()) {
		return FALSE;
	}

	// CPU�L��
	::cpu = this;

	// �������擾
	memory = (Memory*)vm->SearchDevice(XM6_MAKEID('M', 'E', 'M', ' '));
	ASSERT(memory);

	// DMAC�擾
	dmac = (DMAC*)vm->SearchDevice(XM6_MAKEID('D', 'M', 'A', 'C'));
	ASSERT(dmac);

	// MFP�擾
	mfp = (MFP*)vm->SearchDevice(XM6_MAKEID('M', 'F', 'P', ' '));
	ASSERT(mfp);

	// IOSC�擾
	iosc = (IOSC*)vm->SearchDevice(XM6_MAKEID('I', 'O', 'S', 'C'));
	ASSERT(iosc);

	// SCC�擾
	scc = (SCC*)vm->SearchDevice(XM6_MAKEID('S', 'C', 'C', ' '));
	ASSERT(scc);

	// MIDI�擾
	midi = (MIDI*)vm->SearchDevice(XM6_MAKEID('M', 'I', 'D', 'I'));
	ASSERT(midi);

	// SCSI�擾
	scsi = (SCSI*)vm->SearchDevice(XM6_MAKEID('S', 'C', 'S', 'I'));
	ASSERT(scsi);

	// �X�P�W���[���擾
	scheduler = (Scheduler*)vm->SearchDevice(XM6_MAKEID('S', 'C', 'H', 'E'));
	ASSERT(scheduler);

	// ���[�W�����G���A��ݒ�
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

	// CPU�R�A�̃W�����v�e�[�u�����쐬
	::s68000init();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CPU::Cleanup()
{
	ASSERT(this);

	if(pRegion) {
		delete pRegion;
		pRegion = 0;
	}

	// ��{�N���X��
	Device::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL CPU::Reset()
{
	int i;
	S68000CONTEXT context;
	uint32_t bit;

	ASSERT(this);
	LOG0(Log::Normal, "���Z�b�g");

	// �G���[�A�h���X�A�G���[���ԃN���A
	sub.erraddr = 0;
	sub.errtime = 0;

	// ���荞�݃J�E���g�N���A
	for (i=0; i<8; i++) {
		sub.intreq[i] = 0;
		sub.intack[i] = 0;
	}

	// �������R���e�L�X�g�쐬(���Z�b�g��p)
	memory->MakeContext(TRUE);

	// ���Z�b�g
	::s68000reset();
	::s68000context.resethandler = cpu_resethandler;
	::s68000context.odometer = 0;

	// ���荞�݂����ׂĎ�����
	::s68000GetContext(&context);
	for (i=1; i<=7; i++) {
		bit = (1 << i);
		if (context.interrupts[0] & bit) {
			context.interrupts[0] &= (uint8_t)(~bit);
			context.interrupts[i] = 0;
		}
	}
	::s68000SetContext(&context);

	// �������R���e�L�X�g�쐬(�ʏ�)
	memory->MakeContext(FALSE);
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
int FASTCALL CPU::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;
	cpu_t cpu;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "�Z�[�u");

	// �R���e�L�X�g�擾
	GetCPU(&cpu);

	// �T�C�Y���Z�[�u
	sz = sizeof(cpu_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// ���̂��Z�[�u
	if (!fio->Write(&cpu, (int)sz)) {
		return FALSE;
	}

	// �T�C�Y���Z�[�u(�T�u)
	sz = sizeof(cpusub_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// ���̂��Z�[�u(�T�u)
	if (!fio->Write(&sub, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
int FASTCALL CPU::Load(Fileio *fio, int /*ver*/)
{
	cpu_t cpu;
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "���[�h");

	// �T�C�Y�����[�h�A�ƍ�
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(cpu_t)) {
		return FALSE;
	}

	// ���̂����[�h
	if (!fio->Read(&cpu, (int)sz)) {
		return FALSE;
	}

	// �K�p(���Z�b�g���Ă���s��)
	memory->MakeContext(TRUE);
	::s68000reset();
	memory->MakeContext(FALSE);
	SetCPU(&cpu);

	// �T�C�Y�����[�h�A�ƍ�(�T�u)
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(cpusub_t)) {
		return FALSE;
	}

	// ���̂����[�h(�T�u)
	if (!fio->Read(&sub, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL CPU::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);

	LOG0(Log::Normal, "�ݒ�K�p");
}

//---------------------------------------------------------------------------
//
//	CPU���W�X�^�擾
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

	// ���荞��
	for (i=0; i<8; i++) {
		buffer->intr[i] = (uint32_t)::s68000context.interrupts[i];
		buffer->intreq[i] = sub.intreq[i];
		buffer->intack[i] = sub.intack[i];
	}

	// ���̑�
	buffer->sp = ::s68000context.asp;
	buffer->pc = ::s68000context.pc;
	buffer->sr = (uint32_t)::s68000context.sr;
	buffer->odd = ::s68000context.odometer;
}

//---------------------------------------------------------------------------
//
//	CPU���W�X�^�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL CPU::SetCPU(const cpu_t *buffer)
{
	int i;
	S68000CONTEXT context;

	ASSERT(this);
	ASSERT(buffer);

	// �R���e�L�X�g�擾
	::s68000GetContext(&context);

	// Dreg, Areg
	for (i=0; i<8; i++) {
		context.dreg[i] = buffer->dreg[i];
		context.areg[i] = buffer->areg[i];
	}

	// ���荞��
	for (i=0; i<8; i++) {
		context.interrupts[i] = (uint8_t)buffer->intr[i];
		sub.intreq[i] = buffer->intreq[i];
		sub.intack[i] = buffer->intack[i];
	}

	// ���̑�
	context.asp = buffer->sp;
	context.pc = buffer->pc;
	context.sr = (uint16_t)buffer->sr;
	context.odometer = buffer->odd;

	// �R���e�L�X�g�ݒ�
	::s68000SetContext(&context);
}

//---------------------------------------------------------------------------
//
//	���荞��
//
//---------------------------------------------------------------------------
int FASTCALL CPU::Interrupt(int level, int vector)
{
	int ret;

	// INTERRUPT SWITCH�ɂ��NMI���荞�݂̓x�N�^-1
	ASSERT(this);
	ASSERT((level >= 1) && (level <= 7));
	ASSERT(vector >= -1);

	// ���N�G�X�g
	ret = ::s68000interrupt(level, vector);

	// ���ʕ]��
	if (ret == 0) {
		CPU_LOG(LOG2(Log::Normal, "���荞�ݗv���� ���x��%d �x�N�^$%02X", level, vector));
		sub.intreq[level]++;
		return TRUE;
	}

	return FALSE;
}

//---------------------------------------------------------------------------
//
//	���荞��ACK
//
//---------------------------------------------------------------------------
void FASTCALL CPU::IntAck(int level)
{
	ASSERT(this);
	ASSERT((level >= 1) && (level <= 7));

	CPU_LOG(LOG1(Log::Normal, "���荞�ݗv��ACK ���x��%d", level));

	// �J�E���g�A�b�v
	sub.intack[level]++;

	// ���荞�݃��x����
	switch (level) {
		// IOSC,SCSI(����)
		case 1:
			iosc->IntAck();
			scsi->IntAck(1);
			break;

		// MIDI,SCSI(���x��2)
		case 2:
			midi->IntAck(2);
			scsi->IntAck(2);
			break;

		// DMAC
		case 3:
			dmac->IntAck();
			break;

		// MIDI,SCSI(���x��4)
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

		// ���̑�
		default:
			break;
	}
}

//---------------------------------------------------------------------------
//
//	���荞�݃L�����Z��
//
//---------------------------------------------------------------------------
void FASTCALL CPU::IntCancel(int level)
{
	S68000CONTEXT context;
	uint32_t bit;

	ASSERT(this);
	ASSERT((level >= 1) && (level <= 7));

	// �R���e�L�X�g�𒼐ڏ���������
	::s68000GetContext(&context);

	// �Y���r�b�g���I���Ȃ�
	bit = (1 << level);
	if (context.interrupts[0] & bit) {
		CPU_LOG(LOG1(Log::Normal, "���荞�݃L�����Z�� ���x��%d", level));

		// �r�b�g���~�낷
		context.interrupts[0] &= (uint8_t)(~bit);

		// �x�N�^��0
		context.interrupts[level] = 0;

		// ���N�G�X�g��������
		sub.intreq[level]--;
	}

	// �R���e�L�X�g����������
	::s68000SetContext(&context);
}

//---------------------------------------------------------------------------
//
//	RESET����
//
//---------------------------------------------------------------------------
void FASTCALL CPU::ResetInst()
{
	Device *device;

	ASSERT(this);
	CPU_LOG(LOG0(Log::Detail, "RESET����"));

	// ���������擾
	device = (Device*)vm->SearchDevice(XM6_MAKEID('M', 'E', 'M', ' '));
	ASSERT(device);

	// �������f�o�C�X�ɑ΂��Ă��ׂă��Z�b�g�������Ă���
	// ���m�ɂ́ACPU��RESET�M�����ǂ��܂œ`����Ă��邩�ɂ��
	while (device) {
		device->Reset();
		device = device->GetNextDevice();
	}
}

//---------------------------------------------------------------------------
//
//	�o�X�G���[
//	��DMA�]���ɂ��o�X�G���[�������ɗ���
//	��CPU�R�A�����Ńo�X�G���[�Ɣ��肵���ꍇ�́A�������o�R���Ȃ�
//
//---------------------------------------------------------------------------
void FASTCALL CPU::BusErr(uint32_t addr, int read)
{
	uint32_t pc;
	uint32_t stat;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);

	// DMAC�ɓ]�����������BDMAC���Ȃ�DMAC�ɔC����
	if (dmac->IsDMA()) {
		dmac->BusErr(addr, read);
		return;
	}

	// �A�h���X���O��̃A�h���X+2�ŁA�����Ԃ������Ȃ疳������(LONG�A�N�Z�X)
	if (addr == (sub.erraddr + 2)) {
		if (scheduler->GetTotalTime() == sub.errtime) {
			return;
		}
	}

	// �A�h���X�Ǝ��Ԃ��X�V
	sub.erraddr = addr;
	sub.errtime = scheduler->GetTotalTime();

	// PC�擾(�Y�����߂̃I�y�R�[�h�Ɉʒu����)
	pc = GetPC();

	// �ǂݏo��(Word)
	stat = memory->ReadOnly(pc);
	stat <<= 8;
	stat |= memory->ReadOnly(pc + 1);
	stat <<= 16;

	// �t�@���N�V�����R�[�h�쐬(��Ƀf�[�^�A�N�Z�X�Ƃ݂Ȃ�)
	stat |= 0x09;
	if (::s68000context.sr & 0x2000) {
		stat |= 0x04;
	}
	if (read) {
		stat |= 0x10;
	}

	// �o�X�G���[���s
	::s68000buserr(addr, stat);
}

//---------------------------------------------------------------------------
//
//	�A�h���X�G���[
//	��DMA�]���ɂ��A�h���X�G���[�������ɗ���
//	��CPU�R�A�����ŃA�h���X�G���[�Ɣ��肵���ꍇ�́A�������o�R���Ȃ�
//
//---------------------------------------------------------------------------
void FASTCALL CPU::AddrErr(uint32_t addr, int read)
{
	uint32_t pc;
	uint32_t stat;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(addr & 1);

	// DMAC�ɓ]�����������BDMAC���Ȃ�DMAC�ɔC����
	if (dmac->IsDMA()) {
		dmac->AddrErr(addr, read);
		return;
	}

	// �A�h���X���O��̃A�h���X+2�ŁA�����Ԃ������Ȃ疳������(LONG�A�N�Z�X)
	if (addr == (sub.erraddr + 2)) {
		if (scheduler->GetTotalTime() == sub.errtime) {
			return;
		}
	}

	// �A�h���X�Ǝ��Ԃ��X�V
	sub.erraddr = addr;
	sub.errtime = scheduler->GetTotalTime();

	// PC�擾(�Y�����߂̃I�y�R�[�h�Ɉʒu����)
	pc = GetPC();

	// �ǂݏo��(Word)
	stat = memory->ReadOnly(pc);
	stat <<= 8;
	stat |= memory->ReadOnly(pc + 1);
	stat <<= 16;

	// �t�@���N�V�����R�[�h�쐬(��Ƀf�[�^�A�N�Z�X�Ƃ݂Ȃ�)
	stat |= 0x8009;
	if (::s68000context.sr & 0x2000) {
		stat |= 0x04;
	}
	if (read) {
		stat |= 0x10;
	}

	// �o�X�G���[���s(�����ŃA�h���X�G���[�֕���)
	::s68000buserr(addr, stat);
}

//---------------------------------------------------------------------------
//
//	�o�X�G���[�L�^
//	��CPU�R�A�����Ńo�X�G���[�Ɣ��肵���ꍇ���A������ʂ�
//
//---------------------------------------------------------------------------
void FASTCALL CPU::BusErrLog(uint32_t addr, uint32_t stat)
{
	ASSERT(this);

	// �K���}�X�N(24bit�𒴂���ꍇ������)
	addr &= 0xffffff;

	if (stat & 0x10) {
		LOG1(Log::Warning, "�o�X�G���[(�ǂݍ���) $%06X", addr);
	}
	else {
		LOG1(Log::Warning, "�o�X�G���[(��������) $%06X", addr);
	}
}

//---------------------------------------------------------------------------
//
//	�A�h���X�G���[�L�^
//	��CPU�R�A�����ŃA�h���X�G���[�Ɣ��肵���ꍇ���A������ʂ�
//
//---------------------------------------------------------------------------
void FASTCALL CPU::AddrErrLog(uint32_t addr, uint32_t stat)
{
	ASSERT(this);

	// �K���}�X�N(24bit�𒴂���ꍇ������)
	addr &= 0xffffff;

	if (stat & 0x10) {
		LOG1(Log::Warning, "�A�h���X�G���[(�ǂݍ���) $%06X", addr);
	}
	else {
		LOG1(Log::Warning, "�A�h���X�G���[(��������) $%06X", addr);
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
