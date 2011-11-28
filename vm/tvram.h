//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �e�L�X�gVRAM ]
//
//---------------------------------------------------------------------------

#if !defined(tvram_h)
#define tvram_h

#include "device.h"

//===========================================================================
//
//	�e�L�X�gVRAM�n���h��
//
//===========================================================================
class TVRAMHandler
{
public:
	TVRAMHandler(Render *rend, uint8_t *mem);
										// �R���X�g���N�^
	virtual void FASTCALL WriteByte(uint32_t addr, uint32_t data) = 0;
										// �o�C�g��������
	virtual void FASTCALL WriteWord(uint32_t addr, uint32_t data) = 0;
										// ���[�h��������

	// TVRAM���[�N�̃R�s�[
	uint32_t multi;
										// �����A�N�Z�X(bit0-bit3)
	uint32_t mask;
										// �A�N�Z�X�}�X�N(1�ŕύX�Ȃ�)
	uint32_t rev;
										// �A�N�Z�X�}�X�N���]
	uint32_t maskh;
										// �A�N�Z�X�}�X�N��ʃo�C�g
	uint32_t revh;
										// �A�N�Z�X�}�X�N��ʔ��]

protected:
	Render *render;
										// �����_��
	uint8_t *tvram;
										// �e�L�X�gVRAM
};

//===========================================================================
//
//	�e�L�X�gVRAM�n���h��(�ʏ�)
//
//===========================================================================
class TVRAMNormal : public TVRAMHandler
{
public:
	TVRAMNormal(Render *rend, uint8_t *mem);
										// �R���X�g���N�^
	void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// �o�C�g��������
	void FASTCALL WriteWord(uint32_t addr, uint32_t data);
										// ���[�h��������
};

//===========================================================================
//
//	�e�L�X�gVRAM�n���h��(�}�X�N)
//
//===========================================================================
class TVRAMMask : public TVRAMHandler
{
public:
	TVRAMMask(Render *rend, uint8_t *mem);
										// �R���X�g���N�^
	void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// �o�C�g��������
	void FASTCALL WriteWord(uint32_t addr, uint32_t data);
										// ���[�h��������
};

//===========================================================================
//
//	�e�L�X�gVRAM�n���h��(�}���`)
//
//===========================================================================
class TVRAMMulti : public TVRAMHandler
{
public:
	TVRAMMulti(Render *rend, uint8_t *mem);
										// �R���X�g���N�^
	void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// �o�C�g��������
	void FASTCALL WriteWord(uint32_t addr, uint32_t data);
										// ���[�h��������
};

//===========================================================================
//
//	�e�L�X�gVRAM�n���h��(�}�X�N�{�}���`)
//
//===========================================================================
class TVRAMBoth : public TVRAMHandler
{
public:
	TVRAMBoth(Render *rend, uint8_t *mem);
										// �R���X�g���N�^
	void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// �o�C�g��������
	void FASTCALL WriteWord(uint32_t addr, uint32_t data);
										// ���[�h��������
};

//===========================================================================
//
//	�e�L�X�gVRAM
//
//===========================================================================
class TVRAM : public MemDevice
{
public:
	// �����f�[�^��`
	typedef struct {
		uint32_t multi;					// �����A�N�Z�X(bit0-bit3)
		uint32_t mask;						// �A�N�Z�X�}�X�N(1�ŕύX�Ȃ�)
		uint32_t rev;						// �A�N�Z�X�}�X�N���]
		uint32_t maskh;					// �A�N�Z�X�}�X�N��ʃo�C�g
		uint32_t revh;						// �A�N�Z�X�}�X�N��ʔ��]
		uint32_t src;						// ���X�^�R�s�[ �����X�^
		uint32_t dst;						// ���X�^�R�s�[ �惉�X�^
		uint32_t plane;					// ���X�^�R�s�[ �Ώۃv���[��
	} tvram_t;

public:
	// ��{�t�@���N�V����
	TVRAM(VM *p);
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
	const uint8_t* FASTCALL GetTVRAM() const;
										// TVRAM�擾
	void FASTCALL SetMulti(uint32_t data);
										// �����������ݐݒ�
	void FASTCALL SetMask(uint32_t data);
										// �A�N�Z�X�}�X�N�ݒ�
	void FASTCALL SetCopyRaster(uint32_t src, uint32_t dst, uint32_t plane);
										// �R�s�[���X�^�w��
	void FASTCALL RasterCopy();
										// ���X�^�R�s�[����

private:
	void FASTCALL SelectHandler();
										// �n���h���I��
	TVRAMNormal *normal;
										// �n���h��(�ʏ�)
	TVRAMMask *mask;
										// �n���h��(�}�X�N)
	TVRAMMulti *multi;
										// �n���h��(�}���`)
	TVRAMBoth *both;
										// �n���h��(����)
	TVRAMHandler *handler;
										// �n���h��(���ݑI��)
	Render *render;
										// �����_��
	uint8_t *tvram;
										// �e�L�X�gVRAM (512KB)
	tvram_t tvdata;
										// �����f�[�^
	uint32_t tvcount;
										// TVRAM�A�N�Z�X�J�E���g(version2.04�ȍ~)
};

#endif	// tvram_h
