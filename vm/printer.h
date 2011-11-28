//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �v�����^ ]
//
//---------------------------------------------------------------------------

#if !defined(printer_h)
#define printer_h

#include "device.h"

//===========================================================================
//
//	�v�����^
//
//===========================================================================
class Printer : public MemDevice
{
public:
	// �萔�l
	enum {
		BufMax = 0x1000				// �o�b�t�@�T�C�Y(2�̔{��)
	};

	// �����f�[�^��`
	typedef struct {
		int connect;					// �ڑ�
		int strobe;					// �X�g���[�u
		int ready;						// ���f�B
		uint8_t data;						// �������݃f�[�^
		uint8_t buf[BufMax];				// �o�b�t�@�f�[�^
		uint32_t read;						// �o�b�t�@�ǂݍ��݈ʒu
		uint32_t write;					// �o�b�t�@�������݈ʒu
		uint32_t num;						// �o�b�t�@�L����
	} printer_t;

public:
	// ��{�t�@���N�V����
	Printer(VM *p);
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
	int FASTCALL IsReady() const		{ return printer.ready; }
										// ���f�B�擾
	void FASTCALL HSync();
										// H-Sync�ʒm
	void FASTCALL GetPrinter(printer_t *buffer) const;
										// �����f�[�^�擾
	void FASTCALL Connect(int flag);
										// �v�����^�ڑ�
	int FASTCALL GetData(uint8_t *ptr);
										// �擪�f�[�^�擾

private:
	IOSC *iosc;
										// IOSC
	Sync *sync;
										// �����I�u�W�F�N�g
	printer_t printer;
										// �����f�[�^
};

#endif	// printer_h
