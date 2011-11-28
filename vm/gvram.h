//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �O���t�B�b�NVRAM ]
//
//---------------------------------------------------------------------------

#if !defined(gvram_h)
#define gvram_h

#include "device.h"
#include "crtc.h"

//===========================================================================
//
//	�O���t�B�b�NVRAM�n���h��
//
//===========================================================================
class GVRAMHandler
{
public:
	GVRAMHandler(Render *rend, uint8_t *mem, CPU *p);
										// �R���X�g���N�^
	virtual uint32_t FASTCALL ReadByte(uint32_t addr) = 0;
										// �o�C�g�ǂݍ���
	virtual uint32_t FASTCALL ReadWord(uint32_t addr) = 0;
										// ���[�h�ǂݍ���
	virtual void FASTCALL WriteByte(uint32_t addr, uint32_t data) = 0;
										// �o�C�g��������
	virtual void FASTCALL WriteWord(uint32_t addr, uint32_t data) = 0;
										// ���[�h��������
	virtual uint32_t FASTCALL ReadOnly(uint32_t addr) const = 0;
										// �ǂݍ��݂̂�

protected:
	Render *render;
										// �����_��
	uint8_t *gvram;
										// �O���t�B�b�NVRAM
	CPU *cpu;
										// CPU
};

//===========================================================================
//
//	�O���t�B�b�NVRAM�n���h��(1024)
//
//===========================================================================
class GVRAM1024 : public GVRAMHandler
{
public:
	GVRAM1024(Render *render, uint8_t *gvram, CPU *p);
										// �R���X�g���N�^
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
};

//===========================================================================
//
//	�O���t�B�b�NVRAM�n���h��(16�F)
//
//===========================================================================
class GVRAM16 : public GVRAMHandler
{
public:
	GVRAM16(Render *render, uint8_t *gvram, CPU *p);
										// �R���X�g���N�^
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
};

//===========================================================================
//
//	�O���t�B�b�NVRAM�n���h��(256�F)
//
//===========================================================================
class GVRAM256 : public GVRAMHandler
{
public:
	GVRAM256(Render *render, uint8_t *gvram, CPU *p);
										// �R���X�g���N�^
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
};

//===========================================================================
//
//	�O���t�B�b�NVRAM�n���h��(����)
//
//===========================================================================
class GVRAMNDef : public GVRAMHandler
{
public:
	GVRAMNDef(Render *render, uint8_t *gvram, CPU *p);
										// �R���X�g���N�^
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
};

//===========================================================================
//
//	�O���t�B�b�NVRAM�n���h��(65536�F)
//
//===========================================================================
class GVRAM64K : public GVRAMHandler
{
public:
	GVRAM64K(Render *render, uint8_t *gvram, CPU *p);
										// �R���X�g���N�^
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
};

//===========================================================================
//
//	�O���t�B�b�NVRAM
//
//===========================================================================
class GVRAM : public MemDevice
{
public:
	// �������[�N��`
	typedef struct {
		int mem;						// 512KB�P���������t���O
		uint32_t siz;						// 1024�~1024�t���O
		uint32_t col;						// 16, 256, ����`, 65536
		int type;						// �n���h���^�C�v(0�`4)
		uint32_t mask[4];					// �����N���A �}�X�N
		int plane[4];					// �����N���A �v���[��
	} gvram_t;

public:
	// ��{�t�@���N�V����
	GVRAM(VM *p);
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
	void FASTCALL SetType(uint32_t type);
										// GVRAM�^�C�v�ݒ�
	void FASTCALL FastSet(uint32_t mask);
										// �����N���A�ݒ�
	void FASTCALL FastClr(const CRTC::crtc_t *p);
										// �����N���A
	const uint8_t* FASTCALL GetGVRAM() const;
										// GVRAM�擾

private:
	void FASTCALL FastClr768(const CRTC::crtc_t *p);
										// �����N���A 1024x1024 512/768
	void FASTCALL FastClr256(const CRTC::crtc_t *p);
										// �����N���A 1024x1024 256
	void FASTCALL FastClr512(const CRTC::crtc_t *p);
										// �����N���A 512x512
	Render *render;
										// �����_��
	uint8_t *gvram;
										// �O���t�B�b�NVRAM
	GVRAMHandler *handler;
										// �������n���h��(�J�����g)
	GVRAM1024 *hand1024;
										// �������n���h��(1024)
	GVRAM16 *hand16;
										// �������n���h��(16�F)
	GVRAM256 *hand256;
										// �������n���h��(256�F)
	GVRAMNDef *handNDef;
										// �������n���h��(����)
	GVRAM64K *hand64K;
										// �������n���h��(64K�F)
	gvram_t gvdata;
										// �������[�N
	uint32_t gvcount;
										// GVRAM�A�N�Z�X�J�E���g(version2.04�ȍ~)
};

#endif	// gvram_h
