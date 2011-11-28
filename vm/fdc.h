//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ FDC(uPD72065) ]
//
//---------------------------------------------------------------------------

#if !defined(fdc_h)
#define fdc_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	FDC
//
//===========================================================================
class FDC : public MemDevice
{
public:
	// �t�F�[�Y��`
	enum fdcphase {
		idle,							// �A�C�h���t�F�[�Y
		command,						// �R�}���h�t�F�[�Y
		execute,						// ���s�t�F�[�Y(�ʏ�)
		read,							// ���s�t�F�[�Y(Read)
		write,							// ���s�t�F�[�Y(Write)
		result							// ���U���g�t�F�[�Y
	};

	// �X�e�[�^�X���W�X�^��`
	enum {
		sr_rqm = 0x80,					// Request For Master
		sr_dio = 0x40,					// Data Input / Output
		sr_ndm = 0x20,					// Non-DMA Mode
		sr_cb = 0x10,					// FDC Busy
		sr_d3b = 0x08,					// Drive3 Seek
		sr_d2b = 0x04,					// Drive2 Seek
		sr_d1b = 0x02,					// Drive1 Seek
		sr_d0b = 0x01					// Drive0 Seek
	};

	// �R�}���h��`
	enum fdccmd {
		read_data,						// READ DATA
		read_del_data,					// READ DELETED DATA
		read_id,						// READ ID
		write_id,						// WRITE ID
		write_data,						// WRITE DATA
		write_del_data,					// WRITE DELETED DATA
		read_diag,						// READ DIAGNOSTIC
		scan_eq,						// SCAN EQUAL
		scan_lo_eq,						// SCAN LOW OR EQUAL
		scan_hi_eq, 					// SCAN HIGH OR EQUAL
		seek,							// SEEK
		recalibrate,					// RECALIBRATE
		sense_int_stat,					// SENSE INTERRUPT STATUS
		sense_dev_stat,					// SENSE DEVICE STATUS
		specify,						// SPECIFY
		set_stdby,						// SET STANDBY
		reset_stdby,					// RESET STANDBY
		fdc_reset,						// SOFTWARE RESET
		invalid,						// INVALID
		no_cmd							// (NO COMMAND)
	};

	// �������[�N��`
	typedef struct {
		fdcphase phase;					// �t�F�[�Y
		fdccmd cmd;						// �R�}���h

		int in_len;						// ���̓����O�X
		int in_cnt;						// ���̓J�E���g
		uint32_t in_pkt[0x10];				// ���̓p�P�b�g
		int out_len;					// �o�̓����O�X
		int out_cnt;					// �o�̓J�E���g
		uint32_t out_pkt[0x10];			// �o�̓p�P�b�g

		uint32_t dcr;						// �h���C�u�R���g���[�����W�X�^
		uint32_t dsr;						// �h���C�u�Z���N�g���W�X�^
		uint32_t sr;						// �X�e�[�^�X���W�X�^
		uint32_t dr;						// �f�[�^���W�X�^
		uint32_t st[4];					// ST0-ST3

		uint32_t srt;						// SRT
		uint32_t hut;						// HUT
		uint32_t hlt;						// HLT
		uint32_t hd;						// HD
		uint32_t us;						// US
		uint32_t cyl[4];					// �����g���b�N
		uint32_t chrn[4];					// �v�����ꂽC,H,R,N

		uint32_t eot;						// EOT
		uint32_t gsl;						// GSL
		uint32_t dtl;						// DTL
		uint32_t sc; 						// SC
		uint32_t gpl;						// GAP3
		uint32_t d;						// �t�H�[�}�b�g�f�[�^
		uint32_t err;						// �G���[�R�[�h
		int seek;						// �V�[�N�n���荞�ݗv��
		int ndm;						// Non-DMA���[�h
		int mfm;						// MFM���[�h
		int mt;						// �}���`�g���b�N
		int sk;						// Skip DDAM
		int tc;						// TC
		int load;						// �w�b�h���[�h

		int offset;						// �o�b�t�@�I�t�Z�b�g
		int len;						// �c�背���O�X
		uint8_t buffer[0x4000];			// �f�[�^�o�b�t�@

		int fast;						// �������[�h
		int dual;						// �f���A���h���C�u
	} fdc_t;

public:
	// ��{�t�@���N�V����
	FDC(VM *p);
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
	int FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	const fdc_t* FASTCALL GetWork() const;
										// �������[�N�A�h���X�擾
	void FASTCALL CompleteSeek(int drive, int result);
										// �V�[�N����
	void FASTCALL SetTC();
										// TC�A�T�[�g

private:
	void FASTCALL Idle();
										// �A�C�h���t�F�[�Y
	void FASTCALL Command(uint32_t data);
										// �R�}���h�t�F�[�Y
	void FASTCALL CommandRW(fdccmd cmd, uint32_t data);
										// �R�}���h�t�F�[�Y(R/W�n)
	void FASTCALL Execute();
										// ���s�t�F�[�Y
	void FASTCALL ReadID();
										// ���s�t�F�[�Y(Read ID)
	void FASTCALL ExecuteRW();
										// ���s�t�F�[�Y(R/W�n)
	uint8_t FASTCALL Read();
										// ���s�t�F�[�Y(Read)
	void FASTCALL Write(uint32_t data);
										// ���s�t�F�[�Y(Write)
	void FASTCALL Compare(uint32_t data);
										// ���s�t�F�[�Y(Compare)
	void FASTCALL Result();
										// ���U���g�t�F�[�Y
	void FASTCALL ResultRW();
										// ���U���g�t�F�[�Y(R/W�n)
	void FASTCALL Interrupt(int flag);
										// ���荞��
	void FASTCALL SoftReset();
										// �\�t�g�E�F�A���Z�b�g
	void FASTCALL MakeST3();
										// ST3�쐬
	int FASTCALL ReadData();
										// READ (DELETED) DATA�R�}���h
	int FASTCALL WriteData();
										// WRITE (DELETED) DATA�R�}���h
	int FASTCALL ReadDiag();
										// READ DIAGNOSTIC�R�}���h
	int FASTCALL WriteID();
										// WRITE ID�R�}���h
	int FASTCALL Scan();
										// SCAN�n�R�}���h
	void FASTCALL EventRW();
										// �C�x���g����(R/W)
	void FASTCALL EventErr(uint32_t hus);
										// �C�x���g����(�G���[)
	void FASTCALL WriteBack();
										// �������݊���
	int FASTCALL NextSector();
										// �}���`�Z�N�^����
	IOSC *iosc;
										// IOSC
	DMAC *dmac;
										// DMAC
	FDD *fdd;
										// FDD
	Event event;
										// �C�x���g
	fdc_t fdc;
										// FDC�����f�[�^
};

#endif	// fdc_h
