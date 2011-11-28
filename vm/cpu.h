//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
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
	// �����f�[�^��`
	typedef struct {
		uint32_t dreg[8];					// �f�[�^���W�X�^
		uint32_t areg[8];					// �A�h���X���W�X�^
		uint32_t sp;						// �X�^�b�N�\��(USP or SSP)
		uint32_t pc;						// �v���O�����J�E���^
		uint32_t intr[8];					// ���荞�ݏ��
		uint32_t sr;						// �X�e�[�^�X���W�X�^
		uint32_t intreq[8];				// ���荞�ݗv����
		uint32_t intack[8];				// ���荞�ݎ󗝉�
		uint32_t odd;						// ���s�J�E���^
	} cpu_t;

	typedef struct {
		uint32_t erraddr;					// �G���[�A�h���X
		uint32_t errtime;					// �G���[���̉��z����
		uint32_t intreq[8];				// ���荞�ݗv����
		uint32_t intack[8];				// ���荞�ݎ󗝉�
	} cpusub_t;

public:
	// ��{�t�@���N�V����
	CPU(VM *p);
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

public:
	void BeginProgramRegion(int isSuper);
	int  AddProgramRegion(unsigned int lowaddr, unsigned int highaddr, unsigned int offset);
	void EndProgramRegion();

	void BeginDataRegion(int isSuper, int isWrite, int isWord);
	int  AddDataRegion(unsigned int lowaddr, unsigned int highaddr, void* memorycall, void* userdata);
	void EndDataRegion();

	// �O��API
	void FASTCALL GetCPU(cpu_t *buffer) const;
										// CPU���W�X�^�擾
	void FASTCALL SetCPU(const cpu_t *buffer);
										// CPU���W�X�^�ݒ�
	uint32_t FASTCALL Exec(int cycle);
										// ���s
	int FASTCALL Interrupt(int level, int vector);
										// ���荞��
	void FASTCALL IntAck(int level);
										// ���荞��ACK
	void FASTCALL IntCancel(int level);
										// ���荞�݃L�����Z��
	uint32_t FASTCALL GetCycle() const;
										// �T�C�N�����擾
	uint32_t FASTCALL GetPC() const;
										// �v���O�����J�E���^�擾
	void FASTCALL ResetInst();
										// RESET����
	uint32_t FASTCALL GetIOCycle()	const;	// dma.cpp
										// I/O�T�C�N���擾
	void FASTCALL Release();
										// CPU���s�������߂ŋ����I��
	void FASTCALL BusErr(uint32_t addr, int read);
										// �o�X�G���[
	void FASTCALL AddrErr(uint32_t addr, int read);
										// �A�h���X�G���[
	void FASTCALL BusErrLog(uint32_t addr, uint32_t stat);
										// �o�X�G���[�L�^
	void FASTCALL AddrErrLog(uint32_t addr, uint32_t stat);
										// �A�h���X�G���[�L�^
	void FASTCALL Wait(uint32_t cycle);
										// CPU�E�F�C�g
private:
	cpusub_t sub;
										// �����f�[�^
	Memory *memory;
										// ������
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
										// �X�P�W���[��
	// ���[�W���� (Starscream���L)
	struct Region;
	Region* pRegion;
};

#endif	// cpu_h
