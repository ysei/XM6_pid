//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �X�v���C�g(CYNTHIA) ]
//
//---------------------------------------------------------------------------

#if !defined(sprite_h)
#define sprite_h

#include "device.h"

//===========================================================================
//
//	�X�v���C�g
//
//===========================================================================
class Sprite : public MemDevice
{
public:
	// �����f�[�^��`
	typedef struct {
		int connect;					// �A�N�Z�X�\�t���O
		int disp;						// �\��(�E�F�C�g)�t���O
		uint8_t *mem;						// �X�v���C�g������
		uint8_t *pcg;						// �X�v���C�gPCG�G���A

		int bg_on[2];					// BG�\��ON
		uint32_t bg_area[2];				// BG�f�[�^�G���A
		uint32_t bg_scrlx[2];				// BG�X�N���[��X
		uint32_t bg_scrly[2];				// BG�X�N���[��Y
		int bg_size;					// BG�T�C�Y

		uint32_t h_total;					// �����g�[�^������
		uint32_t h_disp;					// �����\������
		uint32_t v_disp;					// �����\������
		int lowres;					// 15kHz���[�h
		uint32_t h_res;					// �����𑜓x
		uint32_t v_res;					// �����𑜓x
	} sprite_t;

public:
	// ��{�t�@���N�V����
	Sprite(VM *p);
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
	void FASTCALL Connect(int con)		{ spr.connect = con; }
										// �ڑ�
	int FASTCALL IsConnect() const		{ return spr.connect; }
										// �ڑ��󋵎擾
	int FASTCALL IsDisplay() const		{ return spr.disp; }
										// �\���󋵎擾
	void FASTCALL GetSprite(sprite_t *buffer) const;
										// �����f�[�^�擾
	const uint8_t* FASTCALL GetMem() const;
										// �������G���A�擾
	const uint8_t* FASTCALL GetPCG() const;
										// PCG�G���A�擾 

private:
	void FASTCALL Control(uint32_t addr, uint32_t ctrl);
										// �R���g���[��
	sprite_t spr;
										// �����f�[�^
	Render *render;
										// �����_��
	uint8_t *sprite;
										// �X�v���C�gRAM(64KB)
};

#endif	// sprite_h
