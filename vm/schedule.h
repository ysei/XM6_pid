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
		int use;						// �g�p�t���O
		uint32_t addr;						// �A�h���X
		int enable;					// �L���t���O
		uint32_t time;						// ��~���̎���
		uint32_t count;					// ��~��
	} breakpoint_t;

	// �X�P�W���[����`
	typedef struct {
		// ����
		uint32_t total;					// �g�[�^�����s����
		uint32_t one;						// 1��̎��s����
		uint32_t sound;					// �T�E���h�X�V����

		// CPU
		int clock;						// CPU�N���b�N(0�`5)
		uint32_t speed;					// CPU���x(clock�ɂ�茈�܂�)
		int cycle;						// CPU�T�C�N����
		uint32_t time;						// CPU�T�C�N�������p����

		// �u���[�N�|�C���g
		int brk;						// �u���[�N����
		int check;						// �L���ȃu���[�N�|�C���g����

		// �C�x���g
		Event *first;					// �ŏ��̃C�x���g
		int exec;						// �C�x���g���s��
	} scheduler_t;

	// ����`
	enum {
		BreakMax = 8					// �u���[�N�|�C���g����
	};

public:
	// ��{�t�@���N�V����
	Scheduler(VM *p);
										// �R���X�g���N�^
	int FASTCALL Init();
										// ������
	void FASTCALL Cleanup();
										// �N���[���A�b�v
	void FASTCALL Reset();
										// ���Z�b�g
	int FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	int FASTCALL Load(Fileio *fio, int ver);
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
	uint32_t FASTCALL Exec(uint32_t hus);
										// ���s
	uint32_t FASTCALL Trace(uint32_t hus);
										// �g���[�X
	void FASTCALL Break()				{ sch.brk = TRUE; }
										// ���s���~
	void FASTCALL Wait(uint32_t cycle)		{ cpu->Wait(cycle); sch.cycle += cycle; }
										// CPU�E�F�C�g

	// ���ꑀ��(DMAC����)
	int FASTCALL GetCPUCycle() const	{ return sch.cycle; }
										// �E�F�C�g���擾
	void FASTCALL SetCPUCycle(int cycle) { sch.cycle = cycle; }
										// �E�F�C�g���ݒ�

	// ���ԏ��
	uint32_t FASTCALL GetTotalTime() const	{ return (GetPassedTime() + sch.total); }
										// �g�[�^�����s���Ԃ��擾
	uint32_t FASTCALL GetOneTime() const	{ return sch.one; }
										// �������s���Ԃ��擾
	uint32_t FASTCALL GetPassedTime() const;
										// �o�ߎ��Ԃ��擾
	uint32_t FASTCALL GetCPUSpeed() const	{ return sch.speed; }
										// CPU���x�擾
	void FASTCALL SetCPUSpeed(uint32_t speed);
										// CPU���x�ݒ�
	uint32_t FASTCALL GetSoundTime() const	{ return sch.sound; }
										// �T�E���h���Ԃ��擾
	void FASTCALL SetSoundTime(uint32_t hus) { sch.sound = hus; }
										// �T�E���h���Ԃ�ݒ�

	// �u���[�N�|�C���g
	void FASTCALL SetBreak(uint32_t addr, int enable = TRUE);
										// �u���[�N�|�C���g�ݒ�
	void FASTCALL DelBreak(uint32_t addr);
										// �u���[�N�|�C���g�폜
	void FASTCALL GetBreak(int index, breakpoint_t *buf) const;
										// �u���[�N�|�C���g�擾
	void FASTCALL EnableBreak(int index, int enable = TRUE);
										// �u���[�N�|�C���g�L���E����
	void FASTCALL ClearBreak(int index);
										// �u���[�N�񐔃N���A
	void FASTCALL AddrBreak(int index, uint32_t addr);
										// �u���[�N�A�h���X�ύX
	int FASTCALL IsBreak(uint32_t addr, int any = FALSE) const;
										// �u���[�N�A�h���X�`�F�b�N

	// �C�x���g
	void FASTCALL AddEvent(Event *event);
										// �C�x���g�ǉ�
	void FASTCALL DelEvent(Event *event);
										// �C�x���g�폜
	int FASTCALL HasEvent(Event *event) const;
										// �C�x���g���L�`�F�b�N
	Event* FASTCALL GetFirstEvent()	const { return sch.first; }
										// �ŏ��̃C�x���g���擾
	int FASTCALL GetEventNum() const;
										// �C�x���g�̌����擾

	// �O������t���O
	int dma_active;
										// DMAC�I�[�g���N�G�X�g�L��

private:
	uint32_t FASTCALL GetMinRemain(uint32_t hus);
										// �ŒZ�̃C�x���g�𓾂�
	void FASTCALL ExecEvent(uint32_t hus);
										// �C�x���g���s
	void FASTCALL OnBreak(uint32_t addr);
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
	static const uint32_t ClockTable[];
										// �N���b�N�e�[�u��
	static int CycleTable[0x1000];
										// ����(hus)���T�C�N����
};

#endif	// scheduler_h
