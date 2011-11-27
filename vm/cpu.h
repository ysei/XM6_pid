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
#include "starcpu.h"

//---------------------------------------------------------------------------
//
//	�O����`
//
//---------------------------------------------------------------------------
#if defined(__cplusplus)
extern "C" {
#endif	// __cplusplus

extern DWORD s68000getcounter();
										// �N���b�N�J�E���^�擾
extern void s68000setcounter(DWORD c);
										// �N���b�N�J�E���^�ݒ�
extern DWORD s68000iocycle;
										// __io_cycle_counter(Starscream)
#if defined(__cplusplus)
}
#endif	// __cplusplus

#define	CPU_IOCYCLE()						::s68000iocycle
#define	CPU_IOCYCLE_GET()					CPU_IOCYCLE()
#define	CPU_IOCYCLE_SUBTRACT(c)				CPU_IOCYCLE() -= (c)
#define	CPU_WAIT(c)							::s68000wait(c)

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
		DWORD dreg[8];					// �f�[�^���W�X�^
		DWORD areg[8];					// �A�h���X���W�X�^
		DWORD sp;						// �X�^�b�N�\��(USP or SSP)
		DWORD pc;						// �v���O�����J�E���^
		DWORD intr[8];					// ���荞�ݏ��
		DWORD sr;						// �X�e�[�^�X���W�X�^
		DWORD intreq[8];				// ���荞�ݗv����
		DWORD intack[8];				// ���荞�ݎ󗝉�
		DWORD odd;						// ���s�J�E���^
	} cpu_t;

	typedef struct {
		DWORD erraddr;					// �G���[�A�h���X
		DWORD errtime;					// �G���[���̉��z����
		DWORD intreq[8];				// ���荞�ݗv����
		DWORD intack[8];				// ���荞�ݎ󗝉�
	} cpusub_t;

public:
	// ��{�t�@���N�V����
	CPU(VM *p);
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

public:
	void BeginProgramRegion(BOOL isSuper);
	int  AddProgramRegion(unsigned int lowaddr, unsigned int highaddr, unsigned int offset);
	void EndProgramRegion();

	void BeginDataRegion(BOOL isSuper, BOOL isWrite, BOOL isWord);
	int  AddDataRegion(unsigned int lowaddr, unsigned int highaddr, void* memorycall, void* userdata);
	void EndDataRegion();

	// �O��API
	void FASTCALL GetCPU(cpu_t *buffer) const;
										// CPU���W�X�^�擾
	void FASTCALL SetCPU(const cpu_t *buffer);
										// CPU���W�X�^�ݒ�
	DWORD FASTCALL Exec(int cycle) {
		DWORD result;

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
										// ���s
	BOOL FASTCALL Interrupt(int level, int vector);
										// ���荞��
	void FASTCALL IntAck(int level);
										// ���荞��ACK
	void FASTCALL IntCancel(int level);
										// ���荞�݃L�����Z��
	DWORD FASTCALL GetCycle() const		{ return ::s68000readOdometer(); }
										// �T�C�N�����擾
	DWORD FASTCALL GetPC() const		{ return ::s68000readPC(); }
										// �v���O�����J�E���^�擾
	void FASTCALL ResetInst();
										// RESET����
	DWORD FASTCALL GetIOCycle()	const	{ return ::s68000getcounter(); }
										// I/O�T�C�N���擾
	void FASTCALL SetIOCycle(DWORD c)	{ ::s68000setcounter(c); }
										// I/O�T�C�N���ݒ�
	void FASTCALL Release()				{ ::s68000releaseTimeslice(); }
										// CPU���s�������߂ŋ����I��
	void FASTCALL BusErr(DWORD addr, BOOL read);
										// �o�X�G���[
	void FASTCALL AddrErr(DWORD addr, BOOL read);
										// �A�h���X�G���[
	void FASTCALL BusErrLog(DWORD addr, DWORD stat);
										// �o�X�G���[�L�^
	void FASTCALL AddrErrLog(DWORD addr, DWORD stat);
										// �A�h���X�G���[�L�^

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
	enum {
		REGION_MAX = 10
	};
	STARSCREAM_PROGRAMREGION u_pgr[REGION_MAX];
										// �v���O�������[�W����(User)
	STARSCREAM_PROGRAMREGION s_pgr[REGION_MAX];
										// �v���O�������[�W����(Super)
	STARSCREAM_DATAREGION u_rbr[REGION_MAX];
										// Read Byte���[�W����(User)
	STARSCREAM_DATAREGION s_rbr[REGION_MAX];
										// Read Byte���[�W����(Super)
	STARSCREAM_DATAREGION u_rwr[REGION_MAX];
										// Read Word���[�W����(User)
	STARSCREAM_DATAREGION s_rwr[REGION_MAX];
										// Read Word���[�W����(Super)
	STARSCREAM_DATAREGION u_wbr[REGION_MAX];
										// Write Byte���[�W����(User)
	STARSCREAM_DATAREGION s_wbr[REGION_MAX];
										// Write Byte���[�W����(Super)
	STARSCREAM_DATAREGION u_wwr[REGION_MAX];
										// Write Word���[�W����(User)
	STARSCREAM_DATAREGION s_wwr[REGION_MAX];
										// Write Word���[�W����(Super)
	STARSCREAM_PROGRAMREGION* pProgramRegion;
	int	iProgramRegion;
	STARSCREAM_DATAREGION* pDataRegion;
	int	iDataRegion;
};

#endif	// cpu_h
