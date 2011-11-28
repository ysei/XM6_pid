//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �V�X�e���|�[�g ]
//
//---------------------------------------------------------------------------

#if !defined(sysport_h)
#define sysport_h

#include "device.h"

//===========================================================================
//
//	�V�X�e���|�[�g
//
//===========================================================================
class SysPort : public MemDevice
{
public:
	// �����f�[�^��`
	typedef struct {
		uint32_t contrast;					// �R���g���X�g
		uint32_t scope_3d;					// 3D�X�R�[�v����
		uint32_t image_unit;				// �C���[�W���j�b�g����
		uint32_t power_count;				// �d������J�E���^
		uint32_t ver_count;				// �o�[�W�����Ǘ��J�E���^
	} sysport_t;

public:
	// ��{�t�@���N�V����
	SysPort(VM *p);
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

private:
	uint32_t FASTCALL GetVR();
										// �o�[�W�������W�X�^�ǂݏo��
	sysport_t sysport;
										// �������[�N
	Memory *memory;
										// ������
	SRAM *sram;
										// �X�^�e�B�b�NRAM
	Keyboard *keyboard;
										// �L�[�{�[�h
	CRTC *crtc;
										// CRTC
	Render *render;
										// �����_��
};

#endif	// sysport_h
