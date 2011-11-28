//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �r�f�I�R���g���[��(CATHY & VIPS) ]
//
//---------------------------------------------------------------------------

#if !defined(vc_h)
#define vc_h

#include "device.h"

//===========================================================================
//
//	�r�f�I�R���g���[��
//
//===========================================================================
class VC : public MemDevice
{
public:
	// �����f�[�^��`
	typedef struct {
		uint32_t vr1h;						// VR1(H)�o�b�N�A�b�v
		uint32_t vr1l;						// VR1(H)�o�b�N�A�b�v
		uint32_t vr2h;						// VR2(H)�o�b�N�A�b�v
		uint32_t vr2l;						// VR2(H)�o�b�N�A�b�v
		int siz;						// ����ʃT�C�Y
		uint32_t col;						// �F���[�h
		uint32_t sp;						// �X�v���C�g�v���C�I���e�B
		uint32_t tx;						// �e�L�X�g�v���C�I���e�B
		uint32_t gr;						// �O���t�B�b�N�v���C�I���e�B(1024)
		uint32_t gp[4];					// �O���t�B�b�N�v���C�I���e�B(512)
		int ys;						// Ys�M��
		int ah;						// �e�L�X�g�p���b�g������
		int vht;						// �O���r�f�I������
		int exon;						// ����v���C�I���e�B�E������
		int hp;						// ������
		int bp;						// �ŉ��ʃr�b�g�������t���O
		int gg;						// �O���t�B�b�N������
		int gt;						// �e�L�X�g������
		int bcon;						// �V���[�v�\��
		int son;						// �X�v���C�gON
		int ton;						// �e�L�X�gON
		int gon;						// �O���t�B�b�NON(�����1024��)
		int gs[4];						// �O���t�B�b�NON(�����512��)
	} vc_t;

public:
	// ��{�t�@���N�V����
	VC(VM *p);
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
	void FASTCALL GetVC(vc_t *buffer);
										// �����f�[�^�擾
	const uint8_t* FASTCALL GetPalette() const	{ return palette; }
										// �p���b�gRAM�擾
	const vc_t* FASTCALL GetWorkAddr() const{ return &vc; }
										// ���[�N�A�h���X�擾

private:
	// ���W�X�^�A�N�Z�X
	void FASTCALL SetVR0L(uint32_t data);
										// ���W�X�^0(L)�ݒ�
	uint32_t FASTCALL GetVR0() const;
										// ���W�X�^0�擾
	void FASTCALL SetVR1H(uint32_t data);
										// ���W�X�^1(H)�ݒ�
	void FASTCALL SetVR1L(uint32_t data);
										// ���W�X�^1(L)�ݒ�
	uint32_t FASTCALL GetVR1() const;
										// ���W�X�^1�擾
	void FASTCALL SetVR2H(uint32_t data);
										// ���W�X�^2(H)�ݒ�
	void FASTCALL SetVR2L(uint32_t data);
										// ���W�X�^2(L)�ݒ�
	uint32_t FASTCALL GetVR2() const;
										// ���W�X�^2�擾

	// �f�[�^
	Render *render;
										// �����_��
	vc_t vc;
										// �����f�[�^
	uint8_t palette[0x400];
										// �p���b�gRAM
};

#endif	// vc_h
