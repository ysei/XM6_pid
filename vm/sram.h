//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �X�^�e�B�b�NRAM ]
//
//---------------------------------------------------------------------------

#if !defined(sram_h)
#define sram_h

#include "device.h"
#include "filepath.h"

//===========================================================================
//
//	�X�^�e�B�b�NRAM
//
//===========================================================================
class SRAM : public MemDevice
{
public:
	// ��{�t�@���N�V����
	SRAM(VM *p);
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
#if !defined(NDEBUG)
	void FASTCALL AssertDiag() const;
										// �f�f
#endif	// NDEBUG

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
	const uint8_t* FASTCALL GetSRAM() const;
										// SRAM�G���A�擾
	int FASTCALL GetSize() const;
										// SRAM�T�C�Y�擾
	void FASTCALL WriteEnable(int enable);
										// �������݋���
	void FASTCALL SetMemSw(uint32_t offset, uint32_t data);
										// �������X�C�b�`�Z�b�g
	uint32_t FASTCALL GetMemSw(uint32_t offset) const;
										// �������X�C�b�`�擾
	void FASTCALL UpdateBoot();
										// �N���J�E���^�X�V

private:
	Filepath sram_path;
										// SRAM�t�@�C���p�X
	int sram_size;
										// SRAM�T�C�Y(16,32,48,64)
	uint8_t sram[0x10000];
										// SRAM (64KB)
	int write_en;
										// �������݋��t���O
	int mem_sync;
										// ���C��RAM�T�C�Y�����t���O
	int changed;
										// �ύX�t���O
};

#endif	// sram_h
