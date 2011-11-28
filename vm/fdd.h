//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ FDD(FD55GFR) ]
//
//---------------------------------------------------------------------------

#if !defined(fdd_h)
#define fdd_h

#include "device.h"
#include "event.h"
//#include "filepath.h"
class Filepath;

//---------------------------------------------------------------------------
//
//	�G���[��`
//	�����ST1, ����ST2, �󂫃r�b�g�ꕔ�g�p
//
//---------------------------------------------------------------------------
#define FDD_NOERROR			0x0000		// �G���[����
#define FDD_EOT				0x8000		// EOT�I�[�o�[
#define FDD_DDAM			0x4000		// DDAM�Z�N�^
#define FDD_DATAERR			0x2000		// IDCRC�܂��̓f�[�^CRC(ReadID������)
#define FDD_OVERRUN			0x1000		// �I�[�o�[����(���X�g�f�[�^)
#define FDD_IDCRC			0x0800		// ID�t�B�[���hCRC
#define FDD_NODATA			0x0400		// �L���ȃZ�N�^�Ȃ�
#define FDD_NOTWRITE		0x0200		// �������݋֎~�M�������o
#define FDD_MAM				0x0100		// �A�h���X�}�[�N�Ȃ�
#define FDD_NOTREADY		0x0080		// �m�b�g���f�B
#define FDD_CM				0x0040		// DDAM�܂���DAM�����o
#define FDD_DATACRC			0x0020		// �f�[�^CRC
#define FDD_NOCYL			0x0010		// �V�����_���قȂ�
#define FDD_SCANEQ			0x0008		// SCAN�ň�v����
#define FDD_SCANNOT			0x0004		// SCAN�ň�v������̂��Ȃ�
#define FDD_BADCYL			0x0002		// �V�����_���s��
#define FDD_MDAM			0x0001		// DAM��������Ȃ�

//---------------------------------------------------------------------------
//
//	�h���C�u�X�e�[�^�X��`
//
//---------------------------------------------------------------------------
#define FDST_INSERT			0x80		// �}��(��}���܂�)
#define FDST_INVALID		0x40		// ��}��
#define FDST_EJECT			0x20		// �C�W�F�N�g�ł���
#define FDST_BLINK			0x10		// �_�ŏ��
#define FDST_CURRENT		0x08		// �_�ł̂ǂ��炩�̏�Ԃ�����
#define FDST_MOTOR			0x04		// ���[�^�����]��
#define FDST_SELECT			0x02		// �Z���N�g��
#define FDST_ACCESS			0x01		// �A�N�Z�X��

//===========================================================================
//
//	FDD
//
//===========================================================================
class FDD : public Device
{
public:
	// �h���C�u�f�[�^��`
	typedef struct {
		FDI *fdi;						// �t���b�s�[�f�B�X�N�C���[�W
		FDI *next;						// ���ɑ}������C���[�W
		int seeking;					// �V�[�N��
		int cylinder;					// �V�����_
		int insert;					// �}��
		int invalid;					// ��}��
		int eject;						// �C�W�F�N�g�ł���
		int blink;						// �}������Ă��Ȃ���Γ_��
		int access;					// �A�N�Z�X��
	} drv_t;

	// �����f�[�^��`
	typedef struct {
		int motor;						// ���[�^�t���O
		int settle;					// �Z�g�����O��
		int force;						// �������f�B�t���O
		int selected;					// �Z���N�g�h���C�u
		int first;						// ���[�^ON��̏���V�[�N
		int hd;						// HD�t���O

		int fast;						// �������[�h
	} fdd_t;

public:
	// ��{�t�@���N�V����
	FDD(VM *p);
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

	// �O��API
	void FASTCALL GetDrive(int drive, drv_t *buffer) const;
										// �h���C�u���[�N�擾
	void FASTCALL GetFDD(fdd_t *buffer) const;
										// �������[�N�擾
	FDI* FASTCALL GetFDI(int drive);
										// FDI�擾
	int FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	void FASTCALL ForceReady(int flag);
										// �������f�B
	uint32_t FASTCALL GetRotationPos() const;
										// ��]�ʒu�擾
	uint32_t FASTCALL GetRotationTime() const;
										// ��]���Ԏ擾
	uint32_t FASTCALL GetSearch();
										// �������Ԏ擾
	void FASTCALL SetHD(int hd);
										// HD�t���O�ݒ�
	int FASTCALL IsHD() const;
										// HD�t���O�擾
	void FASTCALL Access(int flag);
										// �A�N�Z�XLED�ݒ�

	// �h���C�u��
	int FASTCALL Open(int drive, const Filepath& path, int media = 0);
										// �C���[�W�I�[�v��
	void FASTCALL Insert(int drive);
										// �C���T�[�g
	void FASTCALL Eject(int drive, int force);
										// �C�W�F�N�g
	void FASTCALL Invalid(int drive);
										// ��}��
	void FASTCALL Control(int drive, uint32_t func);
										// �h���C�u�R���g���[��
	int FASTCALL IsReady(int drive, int fdc = TRUE) const;
										// ���f�B�`�F�b�N
	int FASTCALL IsWriteP(int drive) const;
										// �������݋֎~�`�F�b�N
	int FASTCALL IsReadOnly(int drive) const;
										// Read Only�`�F�b�N
	void FASTCALL WriteP(int drive, int flag);
										// �������݋֎~�ݒ�
	int FASTCALL GetStatus(int drive) const;
										// �h���C�u�X�e�[�^�X�擾
	void FASTCALL SetMotor(int drive, int flag);
										// ���[�^�ݒ�{�h���C�u�Z���N�g
	int FASTCALL GetCylinder(int drive) const;
										// �V�����_�擾
	void FASTCALL GetName(int drive, char *buf, int media = -1) const;
										// �f�B�X�N���擾
	void FASTCALL GetPath(int drive, Filepath& path) const;
										// �p�X�擾
	int FASTCALL GetDisks(int drive) const;
										// �C���[�W���f�B�X�N�����擾
	int FASTCALL GetMedia(int drive) const;
										// �C���[�W���J�����g���f�B�A�擾

	// �V�[�N
	void FASTCALL Recalibrate(uint32_t srt);
										// ���L�����u���[�g
	void FASTCALL StepIn(int step, uint32_t srt);
										// �X�e�b�v�C��
	void FASTCALL StepOut(int step, uint32_t srt);
										// �X�e�b�v�A�E�g

	// �ǂݍ��݁E��������
	int FASTCALL ReadID(uint32_t *buf, int mfm, int hd);
										// ���[�hID
	int FASTCALL ReadSector(uint8_t *buf, int *len, int mfm, uint32_t *chrn, int hd);
										// ���[�h�Z�N�^
	int FASTCALL WriteSector(const uint8_t *buf, int *len, int mfm, uint32_t *chrn, int hd, int deleted);
										// ���C�g�Z�N�^
	int FASTCALL ReadDiag(uint8_t *buf, int *len, int mfm, uint32_t *chrn, int hd);
										// ���[�h�_�C�A�O
	int FASTCALL WriteID(const uint8_t *buf, uint32_t d, int sc, int mfm, int hd, int gpl);
										// ���C�gID

private:
	void FASTCALL SeekInOut(int cylinder, uint32_t srt);
										// �V�[�N����
	void FASTCALL Rotation();
										// ���[�^��]
	FDC *fdc;
										// FDC
	IOSC *iosc;
										// IOSC
	RTC *rtc;
										// RTC
	drv_t drv[4];
										// �h���C�u�f�[�^
	fdd_t fdd;
										// �����f�[�^
	Event eject;
										// �C�W�F�N�g�C�x���g
	Event seek;
										// �V�[�N�C�x���g
	Event rotation;
										// ��]���C�x���g
};

#endif	// fdd_h
