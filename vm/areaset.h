//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2003 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �G���A�Z�b�g ]
//
//---------------------------------------------------------------------------

#if !defined(areaset_h)
#define areaset_h

#include "device.h"

//===========================================================================
//
//	�G���A�Z�b�g
//
//===========================================================================
class AreaSet : public MemDevice
{
public:
	// ��{�t�@���N�V����
	AreaSet(VM *p);
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
	void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// �o�C�g��������
	uint32_t FASTCALL ReadOnly(uint32_t addr) const;
										// �ǂݍ��݂̂�

	// �O��API
	uint32_t FASTCALL GetArea() const;
										// �ݒ�l�擾

private:
	Memory *memory;
										// ������
	uint32_t area;
										// �G���A�Z�b�g���W�X�^
};

#endif	// areaset_h
