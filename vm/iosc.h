//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ I/O�R���g���[��(IOSC-2) ]
//
//---------------------------------------------------------------------------

#if !defined(iosc_h)
#define iosc_h

#include "device.h"

//===========================================================================
//
//	I/O�R���g���[��
//
//===========================================================================
class IOSC : public MemDevice
{
public:
	// �����f�[�^��`
	typedef struct {
		int prt_int;					// �v�����^���荞�ݗv��
		int prt_en;					// �v�����^���荞�݋���
		int fdd_int;					// FDD���荞�ݗv��
		int fdd_en;					// FDD���荞�݋���
		int fdc_int;					// FDC���荞�ݗv��
		int fdc_en;					// FDC���荞�݋���
		int hdc_int;					// HDD���荞�ݗv��
		int hdc_en;					// HDD���荞�݋���
		uint32_t vbase;					// �x�N�^�x�[�X
		int vector;						// �v�����̊��荞�݃x�N�^
	} iosc_t;

public:
	// ��{�t�@���N�V����
	IOSC(VM *p);
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

	// �������f�o�C�X
	uint32_t FASTCALL ReadByte(uint32_t addr);
										// �o�C�g�ǂݍ���
	uint32_t FASTCALL ReadWord(uint32_t addr);
										// ���[�h�ǂݍ���
	void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// �o�C�g��������
	void FASTCALL WriteWord(uint32_t addr, uint32_t data);
										// ���[�h��������
	uint32_t FASTCALL ReadOnly(uint32_t addr) const;
										// �ǂݍ��݂̂�

	// �O��API
	void FASTCALL GetIOSC(iosc_t *buffer) const;
										// �����f�[�^�擾
	uint32_t FASTCALL GetVector() const	{ return iosc.vbase; }
										// �x�N�^�x�[�X�擾
	void FASTCALL IntAck();
										// ���荞�݉���
	void FASTCALL IntFDC(int flag);
										// FDC���荞��
	void FASTCALL IntFDD(int flag);
										// FDD���荞��
	void FASTCALL IntHDC(int flag);
										// HDC���荞��
	void FASTCALL IntPRT(int flag);
										// �v�����^���荞��

private:
	void FASTCALL IntChk();
										// ���荞�݃`�F�b�N
	iosc_t iosc;
										// �����f�[�^
	Printer *printer;
										// �v�����^
};

#endif	// iosc_h
