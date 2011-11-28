//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ ������ ]
//
//---------------------------------------------------------------------------

#if !defined(memory_h)
#define memory_h

#include "device.h"

//===========================================================================
//
//	�O���֐�
//
//===========================================================================
#if defined(__cplusplus)
extern "C" {
#endif	// __cplusplus
void ReadBusErr(uint32_t addr);
										// �ǂݍ��݃o�X�G���[
void WriteBusErr(uint32_t addr);
										// �������݃o�X�G���[
#if defined(__cplusplus)
}
#endif	// __cplusplus

//===========================================================================
//
//	������
//
//===========================================================================
class Memory : public MemDevice
{
public:
	// ���������(=�V�X�e�����)
	enum memtype {
		None,							// ���[�h����Ă��Ȃ�
		SASI,							// v1.00-SASI(����/ACE/EXPERT/PRO)
		SCSIInt,						// v1.00-SCSI����(SUPER)
		SCSIExt,						// v1.00-SCSI�O�t�{�[�h(����/ACE/EXPERT/PRO)
		XVI,							// v1.10-SCSI����(XVI)
		Compact,						// v1.20-SCSI����(Compact)
		X68030							// v1.50-SCSI����(X68030)
	};

	// �����f�[�^��`
	typedef struct {
		MemDevice* table[0x180];		// �W�����v�e�[�u��
		int size;						// RAM�T�C�Y(2,4,6,8,10,12)
		int config;						// RAM�ݒ�l(0�`5)
		uint32_t length;					// RAM�ŏI�o�C�g+1
		uint8_t *ram;						// ���C��RAM
		uint8_t *ipl;						// IPL ROM (128KB)
		uint8_t *cg;						// CG ROM(768KB)
		uint8_t *scsi;						// SCSI ROM (8KB)
		memtype type;					// ���������(���Z�b�g��)
		memtype now;					// ���������(�J�����g)
		int memsw;						// �������X�C�b�`�����X�V
	} memory_t;

public:
	// ��{�t�@���N�V����
	Memory(VM *p);
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
	void FASTCALL MakeContext(int reset);
										// �������R���e�L�X�g�쐬
	int FASTCALL CheckIPL() const;
										// IPL�o�[�W�����`�F�b�N
	int FASTCALL CheckCG() const;
										// CG�`�F�b�N
	const uint8_t* FASTCALL GetCG() const;
										// CG�擾
	const uint8_t* FASTCALL GetSCSI() const;
										// SCSI�擾
	const uint8_t* FASTCALL GetIPL() const;
										// IPL�擾
	memtype FASTCALL GetMemType() const { return mem.now; }
										// ��������ʎ擾

private:
	int FASTCALL LoadROM(memtype target);
										// ROM���[�h
	void FASTCALL InitTable();
										// �f�R�[�h�e�[�u��������
	AreaSet *areaset;
										// �G���A�Z�b�g
	SRAM *sram;
										// SRAM
	memory_t mem;
										// �����f�[�^
};

#endif	// memory_h
