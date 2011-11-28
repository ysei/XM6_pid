//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �X�P�W���[�� ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "os.h"
#include "xm6.h"
#include "schedule.h"
#include "vm.h"
#include "log.h"
#include "cpu.h"
#include "event.h"
#include "dmac.h"
#include "core_asm.h"
#include "config.h"
#include "fileio.h"

//===========================================================================
//
//	�X�P�W���[��
//
//===========================================================================
//#define SCHEDULER_LOG

//---------------------------------------------------------------------------
//
//	�C�x���g�����E�X�V���A�Z���u����
//
//---------------------------------------------------------------------------
#if defined(_MSC_VER) && defined(_M_IX86)
#define SCHEDULER_ASM
#endif	// _MSC_VER

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Scheduler::Scheduler(VM *p) : Device(p)
{
	int i;

	// �f�o�C�XID��������
	dev.id = MAKEID('S', 'C', 'H', 'E');
	dev.desc = "Scheduler";

	// �u���[�N�|�C���g��
	for (i=0; i<BreakMax; i++) {
		breakp[i].use = FALSE;
		breakp[i].addr = 0;
		breakp[i].enable = FALSE;
		breakp[i].time = 0;
		breakp[i].count = 0;
	}

	// ����
	sch.total = 0;
	sch.one = 0;
	sch.sound = 0;

	// CPU
	sch.clock = 0;
	sch.speed = 979;
	sch.cycle = 0;
	sch.time = 0;

	// �u���[�N�|�C���g
	sch.brk = FALSE;
	sch.check = FALSE;

	// �C�x���g
	sch.first = NULL;
	sch.exec = FALSE;

	// �f�o�C�X
	cpu = NULL;
	dmac = NULL;

	// ���̑�
	dma_active = FALSE;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
int FASTCALL Scheduler::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!Device::Init()) {
		return FALSE;
	}

	// CPU�擾
	ASSERT(!cpu);
	cpu = (CPU*)vm->SearchDevice(MAKEID('C', 'P', 'U', ' '));
	ASSERT(cpu);

	// DMAC�擾
	ASSERT(!dmac);
	dmac = (DMAC*)vm->SearchDevice(MAKEID('D', 'M', 'A', 'C'));
	ASSERT(dmac);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::Cleanup()
{
	ASSERT(this);
	ASSERT_DIAG();

	// ��{�N���X��
	Device::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::Reset()
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "���Z�b�g");

	// ���ԃ��Z�b�g(sound����)
	sch.total = 0;
	sch.one = 0;

	// CPU�T�C�N�����Z�b�g
	sch.cycle = 0;
	sch.time = 0;

	// �C�x���g���s���łȂ�
	sch.exec = FALSE;

	// DMA���s�Ȃ�
	dma_active = FALSE;

	// CPU���x�ݒ�͖���s��(INFO.RAM�΍􃋁[�`���̂���)
	ASSERT((sch.clock >= 0) && (sch.clock <= 5));
	SetCPUSpeed(ClockTable[sch.clock]);
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
int FASTCALL Scheduler::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "�Z�[�u");

	// �u���[�N�|�C���g�T�C�Y���Z�[�u
	sz = sizeof(breakp);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// �u���[�N�|�C���g���̂��Z�[�u
	if (!fio->Write(breakp, (int)sz)) {
		return FALSE;
	}

	// �X�P�W���[���T�C�Y���Z�[�u
	sz = sizeof(scheduler_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// �X�P�W���[�����̂��Z�[�u
	if (!fio->Write(&sch, (int)sz)) {
		return FALSE;
	}

	// �T�C�N���e�[�u�����Z�[�u
	if (!fio->Write(CycleTable, sizeof(CycleTable))) {
		return FALSE;
	}

	// dma_active���Z�[�u(version 2.01)
	if (!fio->Write(&dma_active, sizeof(dma_active))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
int FASTCALL Scheduler::Load(Fileio *fio, int ver)
{
	size_t sz;
	Event *first;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(ver >= 0x200);
	ASSERT_DIAG();

	LOG0(Log::Normal, "���[�h");

	// �C�x���g�|�C���^��ێ�
	first = sch.first;

	// �u���[�N�|�C���g�T�C�Y�����[�h�A�ƍ�
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(breakp)) {
		return FALSE;
	}

	// �u���[�N�|�C���g���̂����[�h
	if (!fio->Read(breakp, (int)sz)) {
		return FALSE;
	}

	// �X�P�W���[���T�C�Y�����[�h�A�ƍ�
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(scheduler_t)) {
		return FALSE;
	}

	// �X�P�W���[�����̂����[�h
	if (!fio->Read(&sch, (int)sz)) {
		return FALSE;
	}

	// �T�C�N���e�[�u�������[�h
	if (!fio->Read(CycleTable, sizeof(CycleTable))) {
		return FALSE;
	}

	// �C�x���g�|�C���^�𕜋A
	sch.first = first;

	// �o�[�W����2.01�ȏ�Ȃ�Adma_active�����[�h
	if (ver >= 0x0201) {
		if (!fio->Read(&dma_active, sizeof(dma_active))) {
			return FALSE;
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);
	ASSERT_DIAG();

	LOG0(Log::Normal, "�ݒ�K�p");

	// �V�X�e���N���b�N�ݒ���r
	if (sch.clock != config->system_clock) {
		// �ݒ肪�قȂ��Ă���̂ŁA�T�C�N���e�[�u���č\�z
		sch.clock = config->system_clock;
		ASSERT((sch.clock >= 0) && (sch.clock <= 5));
		SetCPUSpeed(ClockTable[sch.clock]);
	}
}

#if defined(_DEBUG)
//---------------------------------------------------------------------------
//
//	�f�f
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::AssertDiag() const
{
	ASSERT(this);
	ASSERT(GetID() == MAKEID('S', 'C', 'H', 'E'));
	ASSERT(cpu);
	ASSERT(cpu->GetID() == MAKEID('C', 'P', 'U', ' '));
	ASSERT(dmac);
	ASSERT(dmac->GetID() == MAKEID('D', 'M', 'A', 'C'));
}
#endif	// _DEBUG

//---------------------------------------------------------------------------
//
//	�����f�[�^�擾
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::GetScheduler(scheduler_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT_DIAG();

	// �����f�[�^���R�s�[
	*buffer = sch;
}

//---------------------------------------------------------------------------
//
//	���s
//
//---------------------------------------------------------------------------
uint32_t FASTCALL Scheduler::Exec(uint32_t hus)
{
	int cycle;
	uint32_t result;
	uint32_t dcycle;

	ASSERT(this);
	ASSERT(hus > 0);
	ASSERT_DIAG();

	// �u���[�N�|�C���g�����̏ꍇ
	if (!sch.check) {
		// �ŒZ�̃C�x���g��T��
#if defined(SCHEDULER_ASM)
		sch.one = GetMinEvent(hus);
#else
		sch.one = GetMinRemain(hus);
#endif	// SCHEDULER_ASM

		// sch.one + sch.time�Ɍ������T�C�N���������Ɏ��s���Ă��邩
		ASSERT((sch.one + sch.time) < 0x1000);
		cycle = CycleTable[sch.one + sch.time];
		if (cycle > sch.cycle) {

			// ������s�ł���T�C�N������T���āA���s
			cycle -= sch.cycle;
			if (!dma_active) {
				// �ʏ�
				result = cpu->Exec(cycle);
			}
			else {
				// DMAC�I�[�g���N�G�X�g�L��
				dcycle = dmac->AutoDMA(cycle);
				if (dcycle != 0) {
					// ������ƌ덷���o��H
					result = cpu->Exec(dcycle);
				}
				else {
					// ���ׂ�DMA�ŏ���
					result = cycle;
				}
			}

			// ����I����
			if (result < 0x80000000) {
				// sch.time, sch.cycle���X�V
				sch.cycle += result;
				sch.time += sch.one;

				// ���Ԃ�i�߂�
				ExecEvent(sch.one);

				if (sch.time < 200) {
					return sch.one;
				}

				// ����Sync
				while (sch.time >= 200) {
					if ((uint32_t)sch.cycle < sch.speed) {
						break;
					}
					sch.time -= 200;
					sch.cycle -= sch.speed;
				}

				// �u���[�N�`�F�b�N
				if (!sch.brk) {
					return sch.one;
				}

#if defined(SCHEDULER_LOG)
				LOG0(Log::Normal, "�u���[�N");
#endif	// SCHEDULER_LOG
				sch.brk = FALSE;
				return (uint32_t)(sch.one | 0x80000000);
			}
			else {
				// ���s�G���[
				result &= 0x7fffffff;

				if ((int)result > cycle) {
					// sch.time�Asch.cycle���X�V
					sch.time += sch.one;
					sch.cycle += result;

					// �C�x���g���s
					ExecEvent(sch.one);

					while (sch.time >= 200) {
						if ((uint32_t)sch.cycle < sch.speed) {
							break;
						}
						sch.time -= 200;
						sch.cycle -= sch.speed;
					}
					// ���s�G���[�A�C�x���g����
					return 0x80000000;
				}
				// �S�����s����O��cpu�G���[���N����
				sch.cycle += result;
				// ���s�G���[�A�C�x���g������
				return 0x80000000;
			}
		}
		else {

			// ����͎��s�ł��Ȃ��B���Ԃ�i�߂�̂�
			sch.time += sch.one;
			ExecEvent(sch.one);

			if (sch.time < 200) {
				return sch.one;
			}

			// sch.time���X�V
			while (sch.time >= 200) {
				if ((uint32_t)sch.cycle < sch.speed) {
					break;
				}
				sch.time -= 200;
				sch.cycle -= sch.speed;
			}

			// ���s���߂Ȃ��A�C�x���g����
			return sch.one;
		}

	}

	// ���[�v
	for (;;) {
		result = Trace(hus);

		switch (result) {
			// ���s���߂Ȃ��A�C�x���g����
			case 0:
				return sch.one;

			// ���s�A�C�x���g����
			case 1:
				if (sch.brk) {
#if defined(SCHEDULER_LOG)
					LOG0(Log::Normal, "�u���[�N");
#endif	// SCHEDULER_LOG
					sch.brk = FALSE;
					return 0x80000000;
				}
				if (IsBreak(cpu->GetPC()) != -1) {
					OnBreak(cpu->GetPC());
					return 0x80000000;
				}
				return sch.one;

			// ���s����A�C�x���g������
			case 2:
				if (sch.brk) {
#if defined(SCHEDULER_LOG)
					LOG0(Log::Normal, "�u���[�N");
#endif	// SCHEDULER_LOG
					sch.brk = FALSE;
					return 0x80000000;
				}
				if (IsBreak(cpu->GetPC()) != -1) {
					OnBreak(cpu->GetPC());
					return 0x80000000;
				}
				break;

			// ���s�G���[
			case 3:
				if (sch.brk) {
#if defined(SCHEDULER_LOG)
					LOG0(Log::Normal, "�u���[�N");
#endif	// SCHEDULER_LOG
					sch.brk = FALSE;
				}
				return 0x80000000;

			// ����ȊO
			default:
				ASSERT(FALSE);
				return sch.one;
		}
	}
}

//---------------------------------------------------------------------------
//
//	�g���[�X
//
//---------------------------------------------------------------------------
uint32_t FASTCALL Scheduler::Trace(uint32_t hus)
{
	int cycle;
	uint32_t result;

	ASSERT(this);
	ASSERT(hus > 0);
	ASSERT_DIAG();

	// �ŒZ�̃C�x���g��T��
#if defined(SCHEDULER_ASM)
	sch.one = GetMinEvent(hus);
#else
	sch.one = GetMinRemain(hus);
#endif	// SCHEDULER_ASM

	// sch.one + sch.time�Ɍ������T�C�N���������Ɏ��s���Ă��邩
	ASSERT((sch.one + sch.time) < 0x1000);
	cycle = CycleTable[sch.one + sch.time];
	if (cycle <= sch.cycle) {
		// ����͎��s�ł��Ȃ��B���Ԃ����i�߂�
		sch.time += sch.one;
		ExecEvent(sch.one);

		// sch.time���X�V
		while (sch.time >= 200) {
			sch.time -= 200;
			sch.cycle -= sch.speed;
		}
		// ���s���߂Ȃ��A�C�x���g����
		return 0;
	}

	// ������s�ł���T�C�N������T��
	cycle -= sch.cycle;

	// 1�T�C�N�������^���Ď��s���Ă݂�
	if (!dma_active) {
		// �ʏ�
		result = cpu->Exec(1);
	}
	else {
		// DMAC�I�[�g���N�G�X�g�L��
		result = dmac->AutoDMA(1);
		if (result != 0) {
			result = cpu->Exec(result);
		}
		else {
			result = 1;
		}
	}
	if (result >= 0x80000000) {
		// ���s�G���[
		return 3;
	}

	// result >= cycle�Ȃ�A�C�x���g���s�ł���
	if ((int)result >= cycle) {
		// sch.time, sch.cycle���X�V
		sch.cycle += result;
		sch.time += sch.one;

		// ���Ԃ�i�߂�
		ExecEvent(sch.one);

		while (sch.time >= 200) {
			sch.time -= 200;
			sch.cycle -= sch.speed;
		}
		// ���s�A�C�x���g����
		return 1;
	}

	// �܂�����Ă��Ȃ��̂ŁA�C�x���g�܂ł͊Ԃ�����
	// sch.cycle���X�V
	sch.cycle += result;

	// ���s����A�C�x���g������
	return 2;
}

//---------------------------------------------------------------------------
//
//	CPU���x��ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::SetCPUSpeed(uint32_t speed)
{
	int i;
	uint32_t cycle;

	ASSERT(this);
	ASSERT(speed > 0);
	ASSERT_DIAG();

	LOG2(Log::Detail, "CPU���x�ݒ� %d.%02dMHz", speed / 100, (speed % 100));

	// CPU���x���L��
	sch.speed = speed;

	// 0�`2048us�܂ŁA0.5us�P�ʂł̑Ή�����T�C�N�������v�Z
	for (i=0; i<0x1000; i++) {
		cycle = (uint32_t)i;
		cycle *= speed;
		cycle /= 200;
		CycleTable[i] = cycle;
	}
}

//---------------------------------------------------------------------------
//
//	�o�ߎ��Ԃ��擾
//
//---------------------------------------------------------------------------
uint32_t FASTCALL Scheduler::GetPassedTime() const
{
	uint32_t hus;

	ASSERT(this);
	ASSERT_DIAG();

	// �C�x���g���s���Ȃ�0
	if (sch.exec) {
		return 0;
	}

	// ���s�T�C�N�����Acpu_cylcle���玞�Ԃ��Z�o
	hus = cpu->GetCycle() + sch.cycle;
	hus *= 200;
	hus /= sch.speed;
	hus -= sch.time;

	// one�����傫����΁A����
	if (sch.one < hus) {
		hus = sch.one;
	}

	// hus�P�ʂŕԂ�
	return hus;
}

//---------------------------------------------------------------------------
//
//	�u���[�N�|�C���g�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::SetBreak(uint32_t addr, int enable)
{
	int i;
	int flag;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT_DIAG();

#if defined(SCHEDULER_LOG)
	LOG2(Log::Normal, "�u���[�N�|�C���g�ݒ� $%06X enable=%d", addr, enable);
#endif	// SCHEDULER_LOG

	flag = FALSE;

	// ��v�`�F�b�N
	for (i=0; i<BreakMax; i++) {
		if (breakp[i].use) {
			if (breakp[i].addr == addr) {
				// �t���O�ύX�̂�
				breakp[i].enable = enable;
				flag = TRUE;
				break;
			}
		}
	}

	if (!flag) {
		// �󂫃T�[�`
		for (i=0; i<BreakMax; i++) {
			if (!breakp[i].use) {
				// �Z�b�g
				breakp[i].use = TRUE;
				breakp[i].addr = addr;
				breakp[i].enable = enable;
				breakp[i].time = 0;
				breakp[i].count = 0;
				break;
			}
		}
	}

	// �L���t���O��ݒ�
	flag = FALSE;
	for (i=0; i<BreakMax; i++) {
		if (breakp[i].use) {
			if (breakp[i].enable) {
				// �L���ȃu���[�N�|�C���g������
				flag = TRUE;
				break;
			}
		}
	}
	sch.check = flag;
}

//---------------------------------------------------------------------------
//
//	�u���[�N�|�C���g�폜
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::DelBreak(uint32_t addr)
{
	int i;
	int flag;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT_DIAG();

#if defined(SCHEDULER_LOG)
	LOG1(Log::Normal, "�u���[�N�|�C���g�폜 $%06X", addr);
#endif	// SCHEDULER_LOG

	// ��v�`�F�b�N
	for (i=0; i<BreakMax; i++) {
		if (breakp[i].use) {
			if (breakp[i].addr == addr) {
				// �폜
				breakp[i].use = FALSE;
				break;
			}
		}
	}

	// �L���t���O��ݒ�
	flag = FALSE;
	for (i=0; i<BreakMax; i++) {
		if (breakp[i].use) {
			if (breakp[i].enable) {
				// �L���ȃu���[�N�|�C���g������
				flag = TRUE;
				break;
			}
		}
	}
	sch.check = flag;
}

//---------------------------------------------------------------------------
//
//	�u���[�N�|�C���g�擾
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::GetBreak(int index, breakpoint_t *buf) const
{
	ASSERT(this);
	ASSERT((index >= 0) && (index < BreakMax));
	ASSERT(buf);
	ASSERT_DIAG();

	// �R�s�[
	*buf = breakp[index];
}

//---------------------------------------------------------------------------
//
//	�u���[�N�|�C���g�L���E����
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::EnableBreak(int index, int enable)
{
	ASSERT(this);
	ASSERT((index >= 0) && (index < BreakMax));
	ASSERT(breakp[index].use);
	ASSERT_DIAG();

	breakp[index].enable = enable;
}

//---------------------------------------------------------------------------
//
//	�u���[�N�񐔃N���A
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::ClearBreak(int index)
{
	ASSERT(this);
	ASSERT((index >= 0) && (index < BreakMax));
	ASSERT(breakp[index].use);
	ASSERT_DIAG();

	breakp[index].count = 0;
	breakp[index].time = 0;
}

//---------------------------------------------------------------------------
//
//	�u���[�N�A�h���X�ύX
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::AddrBreak(int index, uint32_t addr)
{
	ASSERT(this);
	ASSERT((index >= 0) && (index < BreakMax));
	ASSERT(addr <= 0xffffff);
	ASSERT(breakp[index].use);
	ASSERT_DIAG();

	breakp[index].addr = addr;
}

//---------------------------------------------------------------------------
//
//	�u���[�N�A�h���X�`�F�b�N
//
//---------------------------------------------------------------------------
int FASTCALL Scheduler::IsBreak(uint32_t addr, int any) const
{
	int i;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT_DIAG();

	// �ŏ��Ƀt���O������
	if (!sch.check) {
		return -1;
	}

	// ��v�`�F�b�N
	for (i=0; i<BreakMax; i++) {
		if (breakp[i].use) {
			if (breakp[i].addr == addr) {
				// �L���E�������C�ɂ��Ȃ����A�L��
				if (any || breakp[i].enable) {
					return i;
				}
			}
		}
	}

	// �u���[�N�|�C���g�͂��邪�A��v����
	return -1;
}

//---------------------------------------------------------------------------
//
//	�u���[�N�A�h���X�K�p
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::OnBreak(uint32_t addr)
{
	int i;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(sch.check);
	ASSERT_DIAG();

	// ��v�`�F�b�N
	for (i=0; i<BreakMax; i++) {
		if (breakp[i].use) {
			if (breakp[i].addr == addr) {
				break;
			}
		}
	}
	ASSERT(i < BreakMax);

	// ���ԃZ�b�g�A�J�E���g�A�b�v
	breakp[i].time = GetTotalTime();
	breakp[i].count++;
}

//---------------------------------------------------------------------------
//
//	�C�x���g�ǉ�
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::AddEvent(Event *event)
{
	Event *p;

	ASSERT(this);
	ASSERT(event);
	ASSERT_DIAG();

#if defined(SCHEDULER_LOG)
	LOG4(Log::Normal, "�C�x���g�ǉ� Device=%c%c%c%c",
					(char)(event->GetDevice()->GetID() >> 24),
					(char)(event->GetDevice()->GetID() >> 16),
					(char)(event->GetDevice()->GetID() >> 8),
					(char)(event->GetDevice()->GetID()));
	LOG1(Log::Normal, "�C�x���g�ǉ� %s", event->GetDesc());
#endif	// SCHEDULER_LOG

	// �ŏ��̃C�x���g��
	if (!sch.first) {
		// �ŏ��̃C�x���g
		sch.first = event;
		event->SetNextEvent(NULL);

#if defined(SCHEDULER_ASM)
		// �ʒm
		NotifyEvent(sch.first);
#endif	// SCHEDULER_ASM
		return;
	}

	// �Ō�̃C�x���g��T��
	p = sch.first;
	while (p->GetNextEvent()) {
		p = p->GetNextEvent();
	}

	// p���Ō�̃C�x���g�Ȃ̂ŁA����ɒǉ�
	p->SetNextEvent(event);
	event->SetNextEvent(NULL);

#if defined(SCHEDULER_ASM)
	// �ʒm
	NotifyEvent(sch.first);
#endif	// SCHEDULER_ASM
}

//---------------------------------------------------------------------------
//
//	�C�x���g�폜
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::DelEvent(Event *event)
{
	Event *p;
	Event *prev;

	ASSERT(this);
	ASSERT(event);
	ASSERT_DIAG();

#if defined(SCHEDULER_LOG)
	LOG4(Log::Normal, "�C�x���g�폜 Device=%c%c%c%c",
					(char)(event->GetDevice()->GetID() >> 24),
					(char)(event->GetDevice()->GetID() >> 16),
					(char)(event->GetDevice()->GetID() >> 8),
					(char)(event->GetDevice()->GetID()));
	LOG1(Log::Normal, "�C�x���g�폜 %s", event->GetDesc());
#endif	// SCHEDULER_LOG

	// �ŏ��̃C�x���g��
	if (sch.first == event) {
		// �ŏ��̃C�x���g�Bnext���ŏ��̃C�x���g�Ɋ��蓖�Ă�
		sch.first = event->GetNextEvent();
		event->SetNextEvent(NULL);

#if defined(SCHEDULER_ASM)
		// �ʒm
		NotifyEvent(sch.first);
#endif	// SCHEDULER_ASM
		return;
	}

	// ���̃C�x���g����v����܂Ō���
	p = sch.first;
	prev = p;
	while (p) {
		// ��v�`�F�b�N
		if (p == event) {
			prev->SetNextEvent(event->GetNextEvent());
			event->SetNextEvent(NULL);

#if defined(SCHEDULER_ASM)
			// �ʒm
			NotifyEvent(sch.first);
#endif	// SCHEDULER_ASM
			return;
		}

		// ����
		prev = p;
		p = p->GetNextEvent();
	}

	// ���ׂẴC�x���g����v���Ȃ�(���蓾�Ȃ�)
	ASSERT(FALSE);
}

//---------------------------------------------------------------------------
//
//	�C�x���g���L�`�F�b�N
//
//---------------------------------------------------------------------------
int FASTCALL Scheduler::HasEvent(Event *event) const
{
	Event *p;

	ASSERT(this);
	ASSERT(event);
	ASSERT_DIAG();

	// ������
	p = sch.first;

	// �S�ẴC�x���g���܂��
	while (p) {
		// ��v�`�F�b�N
		if (p == event) {
			return TRUE;
		}

		// ����
		p = p->GetNextEvent();
	}

	// ���̃C�x���g�̓`�F�C���Ɋ܂܂�Ă��Ȃ�
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	�C�x���g�̌����擾
//
//---------------------------------------------------------------------------
int FASTCALL Scheduler::GetEventNum() const
{
	int num;
	Event *p;

	ASSERT(this);
	ASSERT_DIAG();

	// ������
	num = 0;
	p = sch.first;

	// �S�ẴC�x���g���܂��
	while (p) {
		num++;

		// ����
		p = p->GetNextEvent();
	}

	// �C�x���g�̌���Ԃ�
	return num;
}

//---------------------------------------------------------------------------
//
//	�ŒZ�̃C�x���g��T��
//	���ʓr�A�Z���u���ł�p��
//
//---------------------------------------------------------------------------
uint32_t FASTCALL Scheduler::GetMinRemain(uint32_t hus)
{
	Event *p;
	uint32_t minimum;
	uint32_t remain;

	ASSERT(this);
	ASSERT(hus > 0);
	ASSERT_DIAG();

	// �C�x���g�|�C���^������
	p = sch.first;

	// ������
	minimum = hus;

	// ���[�v
	while (p) {
		// �c�莞�Ԏ擾
		remain = p->GetRemain();

		// �L����
		if (remain == 0) {
			// ����
			p = p->GetNextEvent();
			continue;
		}

		// �ŏ��`�F�b�N
		if (remain >= minimum) {
			p = p->GetNextEvent();
			continue;
		}

		// �ŏ�
		minimum = remain;
		p = p->GetNextEvent();
	}

	return minimum;
}

//---------------------------------------------------------------------------
//
//	�C�x���g���s
//	���ʓr�A�Z���u���ł�p��
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::ExecEvent(uint32_t hus)
{
#if !defined(SCHEDULER_ASM)
	Event *p;
#endif	// !SCHEDULER_ASM

	ASSERT(this);
	ASSERT(hus >= 0);
	ASSERT_DIAG();

	// �C�x���g���s�J�n
	sch.exec = TRUE;

	// �g�[�^�����ԑ����A�T�E���h���ԑ���
	sch.total += hus;
	sch.sound += hus;

#if defined(SCHEDULER_ASM)
	SubExecEvent(hus);
	sch.exec = FALSE;
#else

	// �C�x���g�|�C���^������
	p = sch.first;

	// �C�x���g������āA���s
	while (p) {
		p->Exec(hus);
		p = p->GetNextEvent();
	}

	// �C�x���g���s�I��
	sch.exec = FALSE;
#endif
}

//---------------------------------------------------------------------------
//
//	�N���b�N�e�[�u��
//
//---------------------------------------------------------------------------
const uint32_t Scheduler::ClockTable[] = {
	979,			// 10MHz
	1171,			// 12MHz
	1460,			// 15MHz
	1556,			// 16MHz
	1689,			// 17.4MHz
	1941			// 20MHz
};

//---------------------------------------------------------------------------
//
//	�T�C�N���e�[�u��
//
//---------------------------------------------------------------------------
int Scheduler::CycleTable[0x1000];
