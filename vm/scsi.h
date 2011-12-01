//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ SCSI(MB89352) ]
//
//---------------------------------------------------------------------------

#if !defined(scsi_h)
#define scsi_h

#include "device.h"
#include "event.h"
#include "disk.h"
class Filepath;

//===========================================================================
//
//	SCSI
//
//===========================================================================
class SCSI : public MemDevice
{
public:
	// �ő吔
	enum {
		DeviceMax = 8,					// �ő�SCSI�f�o�C�X��
		HDMax = 5						// �ő�SCSI HD��
	};

	// �t�F�[�Y��`
	enum phase_t {
		busfree,						// �o�X�t���[�t�F�[�Y
		arbitration,					// �A�[�r�g���[�V�����t�F�[�Y
		selection,						// �Z���N�V�����t�F�[�Y
		reselection,					// ���Z���N�V�����t�F�[�Y
		command,						// �R�}���h�t�F�[�Y
		execute,						// ���s�t�F�[�Y
		msgin,							// ���b�Z�[�W�C���t�F�[�Y
		msgout,							// ���b�Z�[�W�A�E�g�t�F�[�Y
		datain,							// �f�[�^�C���t�F�[�Y
		dataout,						// �f�[�^�A�E�g�t�F�[�Y
		status							// �X�e�[�^�X�t�F�[�Y
	};

	// �����f�[�^��`
	typedef struct {
		// �S��
		int type;						// SCSI�^�C�v(0:�Ȃ� 1:�O�t 2:����)
		phase_t phase;					// �t�F�[�Y
		int id;							// �J�����gID(0-7)

		// ���荞��
		int vector;						// �v���x�N�^(-1�ŗv���Ȃ�)
		int ilevel;						// ���荞�݃��x��

		// �M��
		int bsy;						// Busy�M��
		int sel;						// Select�M��
		int atn;						// Attention�M��
		int msg;						// Message�M��
		int cd;						// Command/Data�M��
		int io;						// Input/Output�M��
		int req;						// Request�M��
		int ack;						// Ack�M��
		int rst;						// Reset�M��

		// ���W�X�^
		uint32_t bdid;						// BDID���W�X�^(�r�b�g�\��)
		uint32_t sctl;						// SCTL���W�X�^
		uint32_t scmd;						// SCMD���W�X�^
		uint32_t ints;						// INTS���W�X�^
		uint32_t sdgc;						// SDGC���W�X�^
		uint32_t pctl;						// PCTL���W�X�^
		uint32_t mbc;						// MBC���W�X�^
		uint32_t temp;						// TEMP���W�X�^
		uint32_t tc;						// TCH,TCM,TCL���W�X�^

		// �R�}���h
		uint32_t cmd[10];					// �R�}���h�f�[�^
		uint32_t status;					// �X�e�[�^�X�f�[�^
		uint32_t message;					// ���b�Z�[�W�f�[�^

		// �]��
		int trans;						// �]���t���O
		uint8_t buffer[0x800];				// �]���o�b�t�@
		uint32_t blocks;					// �]���u���b�N��
		uint32_t next;						// ���̃��R�[�h
		uint32_t offset;					// �]���I�t�Z�b�g
		uint32_t length;					// �]���c�蒷��

		// �R���t�B�O
		int scsi_drives;				// SCSI�h���C�u��
		int memsw;						// �������X�C�b�`�X�V
		int mo_first;					// MO�D��t���O(SxSI)

		// �f�B�X�N
		Disk *disk[DeviceMax];			// �f�o�C�X
		Disk *hd[HDMax];				// HD
		SCSIMO *mo;						// MO
		SCSICD *cdrom;					// CD-ROM
	} scsi_t;

public:
	// ��{�t�@���N�V����
	SCSI(VM *p);
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
	void FASTCALL GetSCSI(scsi_t *buffer) const;
										// �����f�[�^�擾
	int FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	void FASTCALL IntAck(int level);
										// ���荞��ACK
	int FASTCALL GetSCSIID() const;
										// SCSI-ID�擾
	int FASTCALL IsBusy() const;
										// BUSY��
	uint32_t FASTCALL GetBusyDevice() const;
										// BUSY�f�o�C�X�擾

	// MO/CD�A�N�Z�X
	int FASTCALL Open(const Filepath& path, int mo = TRUE);
										// MO/CD �I�[�v��
	void FASTCALL Eject(int force, int mo = TRUE);
										// MO/CD �C�W�F�N�g
	void FASTCALL WriteP(int writep);
										// MO �������݋֎~
	int FASTCALL IsWriteP() const;
										// MO �������݋֎~�`�F�b�N
	int FASTCALL IsReadOnly() const;
										// MO ReadOnly�`�F�b�N
	int FASTCALL IsLocked(int mo = TRUE) const;
										// MO/CD Lock�`�F�b�N
	int FASTCALL IsReady(int mo = TRUE) const;
										// MO/CD Ready�`�F�b�N
	int FASTCALL IsValid(int mo = TRUE) const;
										// MO/CD �L���`�F�b�N
	void FASTCALL GetPath(Filepath &path, int mo = TRUE) const;
										// MO/CD �p�X�擾

	// CD-DA
	void FASTCALL GetBuf(uint32_t *buffer, int samples, uint32_t rate);
										// CD-DA�o�b�t�@�擾

private:
	// ���W�X�^
	void FASTCALL ResetReg();
										// ���W�X�^���Z�b�g
	void FASTCALL ResetCtrl();
										// �]�����Z�b�g
	void FASTCALL ResetBus(int reset);
										// �o�X���Z�b�g
	void FASTCALL SetBDID(uint32_t data);
										// BDID�ݒ�
	void FASTCALL SetSCTL(uint32_t data);
										// SCTL�ݒ�
	void FASTCALL SetSCMD(uint32_t data);
										// SCMD�ݒ�
	void FASTCALL SetINTS(uint32_t data);
										// INTS�ݒ�
	uint32_t FASTCALL GetPSNS() const;
										// PSNS�擾
	void FASTCALL SetSDGC(uint32_t data);
										// SDGC�ݒ�
	uint32_t FASTCALL GetSSTS() const;
										// SSTS�擾
	uint32_t FASTCALL GetSERR() const;
										// SERR�擾
	void FASTCALL SetPCTL(uint32_t data);
										// PCTL�ݒ�
	uint32_t FASTCALL GetDREG();
										// DREG�擾
	void FASTCALL SetDREG(uint32_t data);
										// DREG�ݒ�
	void FASTCALL SetTEMP(uint32_t data);
										// TEMP�ݒ�
	void FASTCALL SetTCH(uint32_t data);
										// TCH�ݒ�
	void FASTCALL SetTCM(uint32_t data);
										// TCM�ݒ�
	void FASTCALL SetTCL(uint32_t data);
										// TCL�ݒ�

	// SPC�R�}���h
	void FASTCALL BusRelease();
										// �o�X�����[�X
	void FASTCALL Select();
										// �Z���N�V����/���Z���N�V����
	void FASTCALL ResetATN();
										// ATN���C��=0
	void FASTCALL SetATN();
										// ATN���C��=1
	void FASTCALL Transfer();
										// �]��
	void FASTCALL TransPause();
										// �]�����f
	void FASTCALL TransComplete();
										// �]������
	void FASTCALL ResetACKREQ();
										// ACK/REQ���C��=0
	void FASTCALL SetACKREQ();
										// ACK/REQ���C��=1

	// �f�[�^�]��
	void FASTCALL Xfer(uint32_t *reg);
										// �f�[�^�]��
	void FASTCALL XferNext();
										// �f�[�^�]���p��
	int FASTCALL XferIn();
										// �f�[�^�]��IN
	int FASTCALL XferOut(int cont);
										// �f�[�^�]��OUT
	int FASTCALL XferMsg(uint32_t msg);
										// �f�[�^�]��MSG

	// �t�F�[�Y
	void FASTCALL BusFree();
										// �o�X�t���[�t�F�[�Y
	void FASTCALL Arbitration();
										// �A�[�r�g���[�V�����t�F�[�Y
	void FASTCALL Selection();
										// �Z���N�V�����t�F�[�Y
	void FASTCALL SelectTime();
										// �Z���N�V�����t�F�[�Y(���Ԑݒ�)
	void FASTCALL Command();
										// �R�}���h�t�F�[�Y
	void FASTCALL Execute();
										// ���s�t�F�[�Y
	void FASTCALL MsgIn();
										// ���b�Z�[�W�C���t�F�[�Y
	void FASTCALL MsgOut();
										// ���b�Z�[�W�A�E�g�t�F�[�Y
	void FASTCALL DataIn();
										// �f�[�^�C���t�F�[�Y
	void FASTCALL DataOut();
										// �f�[�^�A�E�g�t�F�[�Y
	void FASTCALL Status();
										// �X�e�[�^�X�t�F�[�Y

	// ���荞��
	void FASTCALL Interrupt(int type, int flag);
										// ���荞�ݗv��
	void FASTCALL IntCheck();
										// ���荞�݃`�F�b�N

	// SCSI�R�}���h����
	void FASTCALL Error();
										// ���ʃG���[

	// SCSI�R�}���h��
	void FASTCALL TestUnitReady();
										// TEST UNIT READY�R�}���h
	void FASTCALL Rezero();
										// REZERO UNIT�R�}���h
	void FASTCALL RequestSense();
										// REQUEST SENSE�R�}���h
	void FASTCALL Format();
										// FORMAT UNIT�R�}���h
	void FASTCALL Reassign();
										// REASSIGN BLOCKS�R�}���h
	void FASTCALL Read6();
										// READ(6)�R�}���h
	void FASTCALL Write6();
										// WRITE(6)�R�}���h
	void FASTCALL Seek6();
										// SEEK(6)�R�}���h
	void FASTCALL Inquiry();
										// INQUIRY�R�}���h
	void FASTCALL ModeSelect();
										// MODE SELECT�R�}���h
	void FASTCALL ModeSense();
										// MODE SENSE�R�}���h
	void FASTCALL StartStop();
										// START STOP UNIT�R�}���h
	void FASTCALL SendDiag();
										// SEND DIAGNOSTIC�R�}���h
	void FASTCALL Removal();
										// PREVENT/ALLOW MEDIUM REMOVAL�R�}���h
	void FASTCALL ReadCapacity();
										// READ CAPACITY�R�}���h
	void FASTCALL Read10();
										// READ(10)�R�}���h
	void FASTCALL Write10();
										// WRITE(10)�R�}���h
	void FASTCALL Seek10();
										// SEEK(10)�R�}���h
	void FASTCALL Verify();
										// VERIFY�R�}���h
	void FASTCALL ReadToc();
										// READ TOC�R�}���h
	void FASTCALL PlayAudio10();
										// PLAY AUDIO(10)�R�}���h
	void FASTCALL PlayAudioMSF();
										// PLAY AUDIO MSF�R�}���h
	void FASTCALL PlayAudioTrack();
										// PLAY AUDIO TRACK INDEX�R�}���h

	// CD-ROM�ECD-DA
	Event cdda;
										// �t���[���C�x���g

	// �h���C�u�E�t�@�C���p�X
	void FASTCALL Construct();
										// �h���C�u�\�z
	Filepath* scsihd[DeviceMax];
										// SCSI-HD�t�@�C���p�X
	// ���̑�
	scsi_t scsi;
										// �����f�[�^
	uint8_t verifybuf[0x800];
										// �x���t�@�C�o�b�t�@
	Event event;
										// �C�x���g
	Memory *memory;
										// ������
	SRAM *sram;
										// SRAM
};

#endif	// scsi_h
