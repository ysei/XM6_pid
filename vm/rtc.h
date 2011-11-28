//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ RTC(RP5C15) ]
//
//---------------------------------------------------------------------------

#if !defined(rtc_h)
#define rtc_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	RTC
//
//===========================================================================
class RTC : public MemDevice
{
public:
	typedef struct {
		uint32_t sec;						// �b
		uint32_t min;						// ��
		uint32_t hour;						// ����
		uint32_t week;						// �T�̗j��
		uint32_t day;						// ��
		uint32_t month;					// ��
		uint32_t year;						// �N
		int carry;						// �b�L�����[

		int timer_en;					// �^�C�}�[���싖��
		int alarm_en;					// �A���[�����싖��
		uint32_t bank;						// �o���N�ԍ�
		uint32_t test;						// TEST���W�X�^
		int alarm_1hz;					// 1Hz�p���X�o�͐���
		int alarm_16hz;				// 16Hz�p���X�o�͐���
		int under_reset;				// �b�A���_�[���Z�b�g
		int alarm_reset;				// �A���[�����Z�b�g

		uint32_t clkout;					// CLKOUT���W�X�^
		int adjust;					// �A�W���X�g

		uint32_t alarm_min;				// ��
		uint32_t alarm_hour;				// ����
		uint32_t alarm_week;				// �T�̗j��
		uint32_t alarm_day;				// ��

		int fullhour;					// 24���ԃt���O
		uint32_t leap;						// �[�N�J�E���^

		int signal_1hz;				// 1Hz�V�O�i��(500ms�����ɕω�)
		int signal_16hz;				// 16Hz�V�O�i��(31.25ms�����ɕω�)
		uint32_t signal_count;				// 16Hz�J�E���^(0�`15)
		uint32_t signal_blink;				// �_�ŃV�O�i��(781.25ms�����ɕω�)
		int alarm;						// �A���[���M��
		int alarmout;					// ALARM OUT
	} rtc_t;

public:
	// ��{�t�@���N�V����
	RTC(VM *p);
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
	void FASTCALL GetRTC(rtc_t *buffer);
										// �����f�[�^�擾
	int FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	int FASTCALL GetTimerLED() const;
										// �^�C�}�[LED�擾
	int FASTCALL GetAlarmOut() const;
										// ALARM�M���擾
	int FASTCALL GetBlink(int drive) const;
										// FDD�p�_�ŐM���擾
	void FASTCALL Adjust(int alarm);
										// ���ݎ�����ݒ�

private:
	void FASTCALL AlarmOut();
										// ALARM�M���o��
	void FASTCALL SecUp();
										// �b�A�b�v
	void FASTCALL MinUp();
										// ���A�b�v
	void FASTCALL AlarmCheck();
										// �A���[���`�F�b�N
	MFP *mfp;
										// MFP
	rtc_t rtc;
										// �����f�[�^
	Event event;
										// �C�x���g
	static const uint32_t DayTable[];
										// ���t�e�[�u��
};

#endif	// rtc_h
