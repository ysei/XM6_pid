//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ ���z�}�V�� ]
//
//---------------------------------------------------------------------------

#if !defined(vm_h)
#define vm_h

#include "log.h"
#include "schedule.h"
#include "cpu.h"
#include "filepath.h"

//===========================================================================
//
//	���z�}�V��
//
//===========================================================================
class VM
{
public:
	// ��{�t�@���N�V����
	VM();
										// �R���X�g���N�^
	~VM();
	int FASTCALL Init();
										// ������
	void FASTCALL Cleanup();
										// �N���[���A�b�v
	void FASTCALL Reset();
										// ���Z�b�g
	void FASTCALL ApplyCfg(const Config *config);
										// �ݒ�K�p

	// �X�e�[�g�ۑ�
	uint32_t FASTCALL Save(const Filepath& path);
										// �Z�[�u
	uint32_t FASTCALL Load(const Filepath& path);
										// ���[�h
	void FASTCALL GetPath(Filepath& path) const;
										// �p�X�擾
	void FASTCALL Clear();
										// �p�X���N���A

	// �f�o�C�X�Ǘ�
	void FASTCALL AddDevice(Device *device);
										// �f�o�C�X�ǉ�(�q����Ă΂��)
	void FASTCALL DelDevice(const Device *device);
										// �f�o�C�X�폜(�q����Ă΂��)
	Device* FASTCALL GetFirstDevice() const	{ return first_device; }
										// �ŏ��̃f�o�C�X���擾
	Device* FASTCALL SearchDevice(uint32_t id) const;
										// �C��ID�̃f�o�C�X���擾

	// ���s
	int FASTCALL Exec(uint32_t hus);
										// ���s
	void FASTCALL Trace();
										// �g���[�X
	void FASTCALL Break()				{ scheduler->Break(); }
										// ���s���~

	// �o�[�W����
	void FASTCALL SetVersion(uint32_t major, uint32_t minor);
										// �o�[�W�����ݒ�
	void FASTCALL GetVersion(uint32_t& major, uint32_t& minor);
										// �o�[�W�����擾

	// �V�X�e������
	void FASTCALL PowerSW(int sw);
										// �d���X�C�b�`����
	int FASTCALL IsPowerSW() const		{ return power_sw; }
										// �d���X�C�b�`��Ԏ擾
	void FASTCALL SetPower(int flag);
										// �d������
	int FASTCALL IsPower() const		{ return power; }
										// �d����Ԏ擾
	void FASTCALL Interrupt() const		{ cpu->Interrupt(7, -1); }
										// NMI���荞��
#if defined(XM6_USE_LOG)
	Log log;
										// ���O
#endif
	void FASTCALL SetHostRtcCallback(XM6_pid::XM6_RTC_CALLBACK cb);
	int FASTCALL GetHostRtc(XM6_pid::XM6_RTC* xm6_rtc);	// take a current time from host

	void FASTCALL SetHostFileSystem(XM6_pid::XM6_FILEIO_SYSTEM* fios);
	XM6_pid::XM6_FILEIO_SYSTEM* FASTCALL GetHostFileSystem();

private:
	int status;
										// �������X�e�[�^�X
	Device *first_device;
										// �ŏ��̃f�o�C�X
	Scheduler *scheduler;
										// �X�P�W���[��
	CPU *cpu;
										// CPU
	MFP *mfp;
										// MFP
	RTC *rtc;
										// RTC
	SRAM *sram;
										// SRAM
	int power_sw;
										// �d���X�C�b�`
	int power;
										// �d��
	uint32_t major_ver;
										// ���W���[�o�[�W����
	uint32_t minor_ver;
										// �}�C�i�[�o�[�W����
//	Filepath current;
										// �J�����g�f�[�^
	Filepath* pCurrent;
										// �J�����g�f�[�^

	XM6_pid::XM6_RTC_CALLBACK	xm6_rtc_cb;
	XM6_pid::XM6_FILEIO_SYSTEM*	xm6_fios;
};

#endif	// vm_h
