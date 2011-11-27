//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �X�P�W���[�� ]
//
//---------------------------------------------------------------------------

#if !defined(scheduler_h)
#define scheduler_h

#include "device.h"
#include "cpu.h"

//---------------------------------------------------------------------------
//
//	�����E�F�C�g(Starscream��p)
//
//---------------------------------------------------------------------------
#define SCHEDULER_FASTWAIT

//===========================================================================
//
//	�X�P�W���[��
//
//===========================================================================
class Scheduler : public Device
{
public:
	// �u���[�N�|�C���g��`
	typedef struct{
		BOOL use;						// �g�p�t���O
		DWORD addr;						// �A�h���X
		BOOL enable;					// �L���t���O
		DWORD time;						// ��~���̎���
		DWORD count;					// ��~��
	} breakpoint_t;

	// �X�P�W���[����`
	typedef struct {
		// ����
		DWORD total;					// �g�[�^�����s����
		DWORD one;						// 1��̎��s����
		DWORD sound;					// �T�E���h�X�V����

		// CPU
		int clock;						// CPU�N���b�N(0�`5)
		DWORD speed;					// CPU���x(clock�ɂ�茈�܂�)
		int cycle;						// CPU�T�C�N����
		DWORD time;						// CPU�T�C�N�������p����

		// �u���[�N�|�C���g
		BOOL brk;						// �u���[�N����
		BOOL check;						// �L���ȃu���[�N�|�C���g����

		// �C�x���g
		Event *first;					// �ŏ��̃C�x���g
		BOOL exec;						// �C�x���g���s��
	} scheduler_t;

	// ����`
	enum {
		BreakMax = 8					// �u���[�N�|�C���g����
	};

public:
	// ��{�t�@���N�V����
	Scheduler(VM *p);
										// �R���X�g���N�^
	BOOL FASTCALL Init();
										// ������
	void FASTCALL Cleanup();
										// �N���[���A�b�v
	void FASTCALL Reset();
										// ���Z�b�g
	BOOL FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ���[�h
	void FASTCALL ApplyCfg(const Config *config);
										// �ݒ�K�p
#if defined(_DEBUG)
	void FASTCALL AssertDiag() const;
										// �f�f
#endif	// _DEBUG

	// �O��API
	void FASTCALL GetScheduler(scheduler_t *buffer) const;
										// �����f�[�^�擾
	DWORD FASTCALL Exec(DWORD hus);
										// ���s
	DWORD FASTCALL Trace(DWORD hus);
										// �g���[�X
	void FASTCALL Break()				{ sch.brk = TRUE; }
										// ���s���~
#ifdef SCHEDULER_FASTWAIT
	void FASTCALL Wait(DWORD cycle)		{ sch.cycle += cycle; if (CPU_IOCYCLE_GET() != (DWORD)-1) CPU_IOCYCLE_SUBTRACT(cycle); }
										// CPU�E�F�C�g(���ׂăC�����C��)
#else
	void FASTCALL Wait(DWORD cycle)		{ CPU_WAIT(cycle); sch.cycle += cycle; }
										// CPU�E�F�C�g
#endif	// SCHEDULER_FASTWAIT

	// ���ꑀ��(DMAC����)
	int FASTCALL GetCPUCycle() const	{ return sch.cycle; }
										// �E�F�C�g���擾
	void FASTCALL SetCPUCycle(int cycle) { sch.cycle = cycle; }
										// �E�F�C�g���ݒ�

	// ���ԏ��
	DWORD FASTCALL GetTotalTime() const	{ return (GetPassedTime() + sch.total); }
										// �g�[�^�����s���Ԃ��擾
	DWORD FASTCALL GetOneTime() const	{ return sch.one; }
										// �������s���Ԃ��擾
	DWORD FASTCALL GetPassedTime() const;
										// �o�ߎ��Ԃ��擾
	DWORD FASTCALL GetCPUSpeed() const	{ return sch.speed; }
										// CPU���x�擾
	void FASTCALL SetCPUSpeed(DWORD speed);
										// CPU���x�ݒ�
	DWORD FASTCALL GetSoundTime() const	{ return sch.sound; }
										// �T�E���h���Ԃ��擾
	void FASTCALL SetSoundTime(DWORD hus) { sch.sound = hus; }
										// �T�E���h���Ԃ�ݒ�

	// �u���[�N�|�C���g
	void FASTCALL SetBreak(DWORD addr, BOOL enable = TRUE);
										// �u���[�N�|�C���g�ݒ�
	void FASTCALL DelBreak(DWORD addr);
										// �u���[�N�|�C���g�폜
	void FASTCALL GetBreak(int index, breakpoint_t *buf) const;
										// �u���[�N�|�C���g�擾
	void FASTCALL EnableBreak(int index, BOOL enable = TRUE);
										// �u���[�N�|�C���g�L���E����
	void FASTCALL ClearBreak(int index);
										// �u���[�N�񐔃N���A
	void FASTCALL AddrBreak(int index, DWORD addr);
										// �u���[�N�A�h���X�ύX
	int FASTCALL IsBreak(DWORD addr, BOOL any = FALSE) const;
										// �u���[�N�A�h���X�`�F�b�N

	// �C�x���g
	void FASTCALL AddEvent(Event *event);
										// �C�x���g�ǉ�
	void FASTCALL DelEvent(Event *event);
										// �C�x���g�폜
	BOOL FASTCALL HasEvent(Event *event) const;
										// �C�x���g���L�`�F�b�N
	Event* FASTCALL GetFirstEvent()	const { return sch.first; }
										// �ŏ��̃C�x���g���擾
	int FASTCALL GetEventNum() const;
										// �C�x���g�̌����擾

	// �O������t���O
	BOOL dma_active;
										// DMAC�I�[�g���N�G�X�g�L��

private:
	DWORD FASTCALL GetMinRemain(DWORD hus);
										// �ŒZ�̃C�x���g�𓾂�
	void FASTCALL ExecEvent(DWORD hus);
										// �C�x���g���s
	void FASTCALL OnBreak(DWORD addr);
										// �u���[�N�|�C���g�K�p

	// �����f�[�^
	breakpoint_t breakp[BreakMax];
										// �u���[�N�|�C���g
	scheduler_t sch;
										// �X�P�W���[��

	// �f�o�C�X
	CPU *cpu;
										// CPU
	DMAC *dmac;
										// DMAC

	// �e�[�u��
	static const DWORD ClockTable[];
										// �N���b�N�e�[�u��
	static int CycleTable[0x1000];
										// ����(hus)���T�C�N����
};

#endif	// scheduler_h
