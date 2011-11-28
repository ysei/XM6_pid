//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ DMAC(HD63450) ]
//
//---------------------------------------------------------------------------

#if !defined(dmac_h)
#define dmac_h

#include "device.h"

//===========================================================================
//
//	DMAC
//
//===========================================================================
class DMAC : public MemDevice
{
public:
	// �����f�[�^��`(�`���l����)
	typedef struct {
		// ��{�p�����[�^
		uint32_t xrm;						// ���N�G�X�g���[�h
		uint32_t dtyp;						// �f�o�C�X�^�C�v
		int dps;						// �|�[�g�T�C�Y (TRUE��16bit)
		uint32_t pcl;						// PCL�Z���N�^
		int dir;						// ���� (TRUE��DAR��������)
		int btd;						// DONE�Ŏ��u���b�N��
		uint32_t size;						// �I�y�����h�T�C�Y
		uint32_t chain;					// �`�F�C������
		uint32_t reqg;						// REQ�������[�h
		uint32_t mac;						// �������A�h���X�X�V���[�h
		uint32_t dac;						// �f�o�C�X�A�h���X�X�V���[�h

		// ����t���O
		int str;						// �X�^�[�g�t���O
		int cnt;						// �R���e�B�j���[�t���O
		int hlt;						// HALT�t���O
		int sab;						// �\�t�g�E�F�A�A�{�[�g�t���O
		int intr;						// ���荞�݉\�t���O
		int coc;						// �`�����l�����슮���t���O
		int boc;						// �u���b�N���슮���t���O
		int ndt;						// ����I���t���O
		int err;						// �G���[�t���O
		int act;						// �A�N�e�B�u�t���O
		int dit;						// DONE���̓t���O
		int pct;						// PCL negedge���o�t���O
		int pcs;						// PCL�̏�� (TRUE��H���x��)
		uint32_t ecode;					// �G���[�R�[�h

		// �A�h���X�A�����O�X
		uint32_t mar;						// �������A�h���X�J�E���^
		uint32_t dar;						// �f�o�C�X�A�h���X���W�X�^
		uint32_t bar;						// �x�[�X�A�h���X���W�X�^
		uint32_t mtc;						// �������g�����X�t�@�J�E���^
		uint32_t btc;						// �x�[�X�g�����X�t�@�J�E���^
		uint32_t mfc;						// �������t�@���N�V�����R�[�h
		uint32_t dfc;						// �f�o�C�X�t�@���N�V�����R�[�h
		uint32_t bfc;						// �x�[�X�t�@���N�V�����R�[�h
		uint32_t niv;						// �m�[�}���C���^���v�g�x�N�^
		uint32_t eiv;						// �G���[�C���^���v�g�x�N�^

		// �o�[�X�g�]��
		uint32_t cp;						// �v���C�I���e�B
		uint32_t bt;						// �o�[�X�g�]���^�C��
		uint32_t br;						// �o���h��
		int type;						// �]���^�C�v

		// ����J�E���^(�f�o�b�O����)
		uint32_t startcnt;					// �X�^�[�g�J�E���^
		uint32_t errorcnt;					// �G���[�J�E���^
	} dma_t;

	// �����f�[�^��`(�O���[�o��)
	typedef struct {
		int transfer;					// �]�����t���O(�`���l�����p)
		int load;						// �`�F�C�����[�h�t���O(�`���l�����p)
		int exec;						// �I�[�g���N�G�X�g�L���t���O
		int current_ch;					// �I�[�g���N�G�X�g�����`���l��
		int cpu_cycle;					// CPU�T�C�N���J�E���^
		int vector;						// ���荞�ݗv�����x�N�^
	} dmactrl_t;

public:
	// ��{�t�@���N�V����
	DMAC(VM *p);
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
										// ���[�h�ǂݍ���
	uint32_t FASTCALL ReadOnly(uint32_t addr) const;
										// �ǂݍ��݂̂�

	// �O��API
	void FASTCALL GetDMA(int ch, dma_t *buffer) const;
										// DMA���擾
	void FASTCALL GetDMACtrl(dmactrl_t *buffer) const;
										// DMA������擾
	int FASTCALL ReqDMA(int ch);
										// DMA�]���v��
	uint32_t FASTCALL AutoDMA(uint32_t cycle);
										// DMA�I�[�g���N�G�X�g
	int FASTCALL IsDMA() const;
										// DMA�]�������₢���킹
	void FASTCALL BusErr(uint32_t addr, int read);
										// �o�X�G���[
	void FASTCALL AddrErr(uint32_t addr, int read);
										// �A�h���X�G���[
	uint32_t FASTCALL GetVector(int type) const;
										// �x�N�^�擾
	void FASTCALL IntAck();
										// ���荞��ACK
	int FASTCALL IsAct(int ch) const;
										// DMA�]���\���₢���킹

private:
	// �`���l���������A�N�Z�X
	uint32_t FASTCALL ReadDMA(int ch, uint32_t addr) const;
										// DMA�ǂݍ���
	void FASTCALL WriteDMA(int ch, uint32_t addr, uint32_t data);
										// DMA��������
	void FASTCALL SetDCR(int ch, uint32_t data);
										// DCR�Z�b�g
	uint32_t FASTCALL GetDCR(int ch) const;
										// DCR�擾
	void FASTCALL SetOCR(int ch, uint32_t data);
										// OCR�Z�b�g
	uint32_t FASTCALL GetOCR(int ch) const;
										// OCR�擾
	void FASTCALL SetSCR(int ch, uint32_t data);
										// SCR�Z�b�g
	uint32_t FASTCALL GetSCR(int ch) const;
										// SCR�擾
	void FASTCALL SetCCR(int ch, uint32_t data);
										// CCR�Z�b�g
	uint32_t FASTCALL GetCCR(int ch) const;
										// CCR�擾
	void FASTCALL SetCSR(int ch, uint32_t data);
										// CSR�Z�b�g
	uint32_t FASTCALL GetCSR(int ch) const;
										// CSR�擾
	void FASTCALL SetGCR(uint32_t data);
										// GCR�Z�b�g

	// �`���l���I�y���[�V����
	void FASTCALL ResetDMA(int ch);
										// DMA���Z�b�g
	void FASTCALL StartDMA(int ch);
										// DMA�X�^�[�g
	void FASTCALL ContDMA(int ch);
										// DMA�R���e�B�j���[
	void FASTCALL AbortDMA(int ch);
										// DMA�\�t�g�E�F�A�A�{�[�g
	void FASTCALL LoadDMA(int ch);
										// DMA�u���b�N���[�h
	void FASTCALL ErrorDMA(int ch, uint32_t code);
										// �G���[
	void FASTCALL Interrupt();
										// ���荞��
	int FASTCALL TransDMA(int ch);
										// DMA1��]��

	// �e�[�u���A�������[�N
	static const int MemDiffTable[8][4];
										// �������X�V�e�[�u��
	static const int DevDiffTable[8][4];
										// �f�o�C�X�X�V�e�[�u��
	Memory *memory;
										// ������
	FDC *fdc;
										// FDC
	dma_t dma[4];
										// �������[�N(�`���l��)
	dmactrl_t dmactrl;
										// �������[�N(�O���[�o��)
};

#endif	// dmac_h
