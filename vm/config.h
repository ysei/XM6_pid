//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �R���t�B�M�����[�V���� ]
//
//---------------------------------------------------------------------------

#if !defined(config_h)
#define config_h

#include "filepath.h"

//===========================================================================
//
//	�R���t�B�M�����[�V����(version2.00�`version2.01)
//
//===========================================================================
class Config200 {
public:
	typedef XM6_pid::FiosPath Path;

	enum {
		FILE_PATH_MAX	= 260,
	};
	// �V�X�e��
	int system_clock;					// �V�X�e���N���b�N(0�`5)
	int ram_size;						// ���C��RAM�T�C�Y(0�`5)
	int ram_sramsync;					// �������X�C�b�`�����X�V

	// �X�P�W���[��
	int mpu_fullspeed;					// MPU�t���X�s�[�h
	int vm_fullspeed;					// VM�t���X�s�[�h

	// �T�E���h
	int sound_device;					// �T�E���h�f�o�C�X(0�`15)
	int sample_rate;					// �T���v�����O���[�g(0�`4)
	int primary_buffer;					// �o�b�t�@�T�C�Y(2�`100)
	int polling_buffer;					// �|�[�����O�Ԋu(0�`99)
	int adpcm_interp;					// ADPCM���`��Ԃ���

	// �`��
	int aspect_stretch;				// �A�X�y�N�g��ɂ��킹�g��

	// ����
	int master_volume;					// �}�X�^����(0�`100)
	int fm_enable;						// FM�L��
	int fm_volume;						// FM����(0�`100)
	int adpcm_enable;					// ADPCM�L��
	int adpcm_volume;					// ADPCM����(0�`100)

	// �L�[�{�[�h
	int kbd_connect;					// �ڑ�

	// �}�E�X
	int mouse_speed;					// �X�s�[�h
	int mouse_port;						// �ڑ��|�[�g
	int mouse_swap;					// �{�^���X���b�v
	int mouse_mid;						// ���{�^���C�l�[�u��
	int mouse_trackb;					// �g���b�N�{�[�����[�h

	// �W���C�X�e�B�b�N
	int joy_type[2];					// �W���C�X�e�B�b�N�^�C�v
	int joy_dev[2];						// �W���C�X�e�B�b�N�f�o�C�X
	int joy_button0[12];				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	int joy_button1[12];				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)

	// SASI
	int sasi_drives;					// SASI�h���C�u��
	int sasi_sramsync;					// SASI�������X�C�b�`�����X�V
	Path	sasi_file[16];	// SASI�C���[�W�t�@�C��

	// SxSI
	int sxsi_drives;					// SxSI�h���C�u��
	int sxsi_mofirst;					// MO�h���C�u�D�抄�蓖��
	Path	sxsi_file[6];	// SxSI�C���[�W�t�@�C��

	// �|�[�g
	int port_com;						// COMx�|�[�g
	Path	port_recvlog;	// �V���A����M���O
	int port_384;						// �V���A��38400bps�Œ�
	int port_lpt;						// LPTx�|�[�g
	Path	port_sendlog;	// �p���������M���O

	// MIDI
	int midi_bid;						// MIDI�{�[�hID
	int midi_ilevel;					// MIDI���荞�݃��x��
	int midi_reset;						// MIDI���Z�b�g�R�}���h
	int midiin_device;					// MIDI IN�f�o�C�X
	int midiin_delay;					// MIDI IN�f�B���C(ms)
	int midiout_device;					// MIDI OUT�f�o�C�X
	int midiout_delay;					// MIDI OUT�f�B���C(ms)

	// ����
	int sram_64k;						// 64KB SRAM
	int scc_clkup;						// SCC�N���b�N�A�b�v
	int power_led;						// �F�d��LED
	int dual_fdd;						// 2DD/2HD���pFDD
	int sasi_parity;					// SASI�o�X�p���e�B

	// TrueKey
	int tkey_mode;						// TrueKey���[�h(bit0:VM bit1:WinApp)
	int tkey_com;						// �L�[�{�[�hCOM�|�[�g
	int tkey_rts;						// RTS���]���[�h

	// ���̑�
	int floppy_speed;					// �t���b�s�[�f�B�X�N����
	int floppy_led;					// �t���b�s�[�f�B�X�NLED���[�h
	int popup_swnd;					// �|�b�v�A�b�v�T�u�E�B���h�E
	int auto_mouse;					// �����}�E�X���[�h����
	int power_off;						// �d��OFF�ŊJ�n
};

//===========================================================================
//
//	�R���t�B�M�����[�V����(version2.02�`version2.03)
//
//===========================================================================
class Config202 : public Config200 {
public:
	// �V�X�e��
	int mem_type;						// �������}�b�v���

	// SCSI
	int scsi_ilevel;					// SCSI���荞�݃��x��
	int scsi_drives;					// SCSI�h���C�u��
	int scsi_sramsync;					// SCSI�������X�C�b�`�����X�V
	int scsi_mofirst;					// MO�h���C�u�D�抄�蓖��
	Path	scsi_file[5];	// SCSI�C���[�W�t�@�C��
};

//===========================================================================
//
//	�R���t�B�M�����[�V����
//
//===========================================================================
class Config : public Config202 {
public:
	// ���W���[��
	int resume_fd;						// FD���W���[��
	int resume_fdi[2];					// FD�}���t���O
	int resume_fdw[2];					// FD�������݋֎~
	int resume_fdm[2];					// FD���f�B�ANo.
	int resume_mo;						// MO���W���[��
	int resume_mos;					// MO�}���t���O
	int resume_mow;					// MO�������݋֎~
	int resume_cd;						// CD���W���[��
	int resume_iso;					// CD�}���t���O
	int resume_state;					// �X�e�[�g���W���[��
	int resume_xm6;					// �X�e�[�g�L���t���O
	int resume_screen;					// ��ʃ��[�h���W���[��
	int resume_dir;					// �f�t�H���g�f�B���N�g�����W���[��
	Path	resume_path;	// �f�t�H���g�f�B���N�g��

	// �`��
	int caption_info;					// �L���v�V�������\��

	// �f�B�X�v���C
	int caption;						// �L���v�V����
	int menu_bar;						// ���j���[�o�[
	int status_bar;					// �X�e�[�^�X�o�[
	int window_left;					// �E�B���h�E��`
	int window_top;						// �E�B���h�E��`
	int window_full;					// �t���X�N���[��
	int window_mode;					// ���C�h�X�N���[��

	// WINDRV���W���[��
	uint32_t windrv_enable;				// Windrv�T�|�[�g 0:���� 1:WindrvXM (2:Windrv�݊�)

	// �z�X�g���t�@�C���V�X�e��
	uint32_t host_option;					// ����t���O (class CHostFilename �Q��)
	int host_resume;					// �x�[�X�p�X��ԕ����L�� FALSE���Ɩ���X�L��������
	uint32_t host_drives;					// �L���ȃh���C�u��
	uint32_t host_flag[10];				// ����t���O (class CWinFileDrv �Q��)
	Path	host_path[10];		// �x�[�X�p�X
};

#endif	// config_h
