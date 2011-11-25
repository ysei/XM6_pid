//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �t���[���E�B���h�E ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "schedule.h"
#include "memory.h"
#include "sasi.h"
#include "scsi.h"
#include "fdd.h"
#include "fdc.h"
#include "fdi.h"
#include "render.h"
#include "fileio.h"
#include "mfc_frm.h"
#include "mfc_draw.h"
#include "mfc_res.h"

#include "crtc.h"
#include "keyboard.h"
#include "mouse.h"
#include "ppi.h"

#include "opmif.h"
#include "opm.h"
#include "adpcm.h"
#include "scsi.h"
#include "mfc_asm.h"

#include "config.h"
#include "sram.h"

//===========================================================================
//
//	�t���[���E�B���h�E
//
//===========================================================================

CFrmWnd*	globalFrmWnd	= 0;

//---------------------------------------------------------------------------
//
//	scheduler
//
//---------------------------------------------------------------------------
static volatile	BOOL		scheduler_mm_bEnable = FALSE;
static volatile BOOL		scheduler_m_bExitReq = FALSE;			// �X���b�h�I���v��

BOOL schedulerIsEnable() {
	return scheduler_mm_bEnable;
}

void schedulerSetEnable(BOOL b) {
	scheduler_mm_bEnable = b;
}

static DWORD FASTCALL GetTime() {
	return timeGetTime();
}

void configGetConfig(Config* c) {
	//	Config200
	// �V�X�e��
	c->system_clock			= 5;					// �V�X�e���N���b�N(0�`5)
	c->ram_size				= 0;						// ���C��RAM�T�C�Y(0�`5)
	c->ram_sramsync			= TRUE;					// �������X�C�b�`�����X�V

	// �X�P�W���[��
	c->mpu_fullspeed		= FALSE;					// MPU�t���X�s�[�h
	c->vm_fullspeed			= FALSE;					// VM�t���X�s�[�h

	// �T�E���h
	c->sound_device			= 0;					// �T�E���h�f�o�C�X(0�`15)
	c->sample_rate			= 5;					// �T���v�����O���[�g(0�`4)
	c->primary_buffer		= 10;					// �o�b�t�@�T�C�Y(2�`100)
	c->polling_buffer		= 5;					// �|�[�����O�Ԋu(0�`99)
	c->adpcm_interp			= TRUE;					// ADPCM���`��Ԃ���

	// �`��
	c->aspect_stretch		= TRUE;				// �A�X�y�N�g��ɂ��킹�g��

	// ����
	c->master_volume		= 100;					// �}�X�^����(0�`100)
	c->fm_enable			= TRUE;						// FM�L��
	c->fm_volume			= 54;						// FM����(0�`100)
	c->adpcm_enable			= TRUE;					// ADPCM�L��
	c->adpcm_volume			= 52;					// ADPCM����(0�`100)

	// �L�[�{�[�h
	c->kbd_connect			= TRUE;					// �ڑ�

	// �}�E�X
	c->mouse_speed			= 205;					// �X�s�[�h
	c->mouse_port			= 1;						// �ڑ��|�[�g
	c->mouse_swap			= FALSE;					// �{�^���X���b�v
	c->mouse_mid			= TRUE;						// ���{�^���C�l�[�u��
	c->mouse_trackb			= FALSE;					// �g���b�N�{�[�����[�h

	// �W���C�X�e�B�b�N
	c->joy_type[0]			= 1;					// �W���C�X�e�B�b�N�^�C�v
	c->joy_type[1]			= 1;					// �W���C�X�e�B�b�N�^�C�v
	c->joy_dev[0]			= 1;						// �W���C�X�e�B�b�N�f�o�C�X
	c->joy_dev[1]			= 2;						// �W���C�X�e�B�b�N�f�o�C�X
	c->joy_button0[0]		= 1;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[1]		= 2;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[2]		= 3;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[3]		= 4;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[4]		= 5;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[5]		= 6;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[6]		= 7;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[7]		= 8;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[8]		= 0;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[9]		= 0;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[10]		= 0;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[11]		= 0;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button1[0]		= 65537;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[1]		= 65538;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[2]		= 65539;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[3]		= 65540;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[4]		= 65541;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[5]		= 65542;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[6]		= 65543;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[7]		= 65544;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[8]		= 0;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[9]		= 0;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[10]		= 0;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[11]		= 0;				// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)

	// SASI
	c->sasi_drives			= 1;					// SASI�h���C�u��
	c->sasi_sramsync		= TRUE;					// SASI�������X�C�b�`�����X�V
	strcpy(&c->sasi_file[ 0][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD0.HDF"));				// SASI�C���[�W�t�@�C��
	strcpy(&c->sasi_file[ 1][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD1.HDF"));				// SASI�C���[�W�t�@�C��
	strcpy(&c->sasi_file[ 2][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD2.HDF"));				// SASI�C���[�W�t�@�C��
	strcpy(&c->sasi_file[ 3][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD3.HDF"));				// SASI�C���[�W�t�@�C��
	strcpy(&c->sasi_file[ 4][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD4.HDF"));				// SASI�C���[�W�t�@�C��
	strcpy(&c->sasi_file[ 5][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD5.HDF"));				// SASI�C���[�W�t�@�C��
	strcpy(&c->sasi_file[ 6][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD6.HDF"));				// SASI�C���[�W�t�@�C��
	strcpy(&c->sasi_file[ 7][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD7.HDF"));				// SASI�C���[�W�t�@�C��
	strcpy(&c->sasi_file[ 8][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD8.HDF"));				// SASI�C���[�W�t�@�C��
	strcpy(&c->sasi_file[ 9][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD9.HDF"));				// SASI�C���[�W�t�@�C��
	strcpy(&c->sasi_file[10][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD10.HDF"));				// SASI�C���[�W�t�@�C��
	strcpy(&c->sasi_file[11][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD11.HDF"));				// SASI�C���[�W�t�@�C��
	strcpy(&c->sasi_file[12][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD12.HDF"));				// SASI�C���[�W�t�@�C��
	strcpy(&c->sasi_file[13][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD13.HDF"));				// SASI�C���[�W�t�@�C��
	strcpy(&c->sasi_file[14][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD14.HDF"));				// SASI�C���[�W�t�@�C��
	strcpy(&c->sasi_file[15][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD15.HDF"));				// SASI�C���[�W�t�@�C��

	// SxSI
	c->sxsi_drives			= 0;							// SxSI�h���C�u��
	c->sxsi_mofirst			= FALSE;						// MO�h���C�u�D�抄�蓖��
	memset(&c->sxsi_file[0][0], 0, sizeof(c->sxsi_file));	// SxSI�C���[�W�t�@�C��

	// �|�[�g
	c->port_com				= 0;								// COMx�|�[�g
	memset(&c->port_recvlog[0], 0, sizeof(c->port_recvlog));	// �V���A����M���O
	c->port_384				= FALSE;							// �V���A��38400bps�Œ�
	c->port_lpt				= 0;								// LPTx�|�[�g
	memset(&c->port_sendlog[0], 0, sizeof(c->port_sendlog));	// �p���������M���O

	// MIDI
	c->midi_bid				= 0;							// MIDI�{�[�hID
	c->midi_ilevel			= 0;							// MIDI���荞�݃��x��
	c->midi_reset			= 0;							// MIDI���Z�b�g�R�}���h
	c->midiin_device		= 0;							// MIDI IN�f�o�C�X
	c->midiin_delay			= 0;							// MIDI IN�f�B���C(ms)
	c->midiout_device		= 0;							// MIDI OUT�f�o�C�X
	c->midiout_delay		= 84;							// MIDI OUT�f�B���C(ms)

	// ����
	c->sram_64k				= FALSE;						// 64KB SRAM
	c->scc_clkup			= FALSE;						// SCC�N���b�N�A�b�v
	c->power_led			= FALSE;						// �F�d��LED
	c->dual_fdd				= FALSE;						// 2DD/2HD���pFDD
	c->sasi_parity			= FALSE;						// SASI�o�X�p���e�B

	// TrueKey
	c->tkey_mode			= 1;							// TrueKey���[�h(bit0:VM bit1:WinApp)
	c->tkey_com				= 0;							// �L�[�{�[�hCOM�|�[�g
	c->tkey_rts				= FALSE;						// RTS���]���[�h

	// ���̑�
	c->floppy_speed			= TRUE;							// �t���b�s�[�f�B�X�N����
	c->floppy_led			= TRUE;							// �t���b�s�[�f�B�X�NLED���[�h
	c->popup_swnd			= TRUE;							// �|�b�v�A�b�v�T�u�E�B���h�E
	c->auto_mouse			= FALSE;						// �����}�E�X���[�h����
	c->power_off			= FALSE;						// �d��OFF�ŊJ�n

	//	Config202
	// �V�X�e��
	c->mem_type				= 1;		// �������}�b�v���

	// SCSI
	c->scsi_ilevel			= 1;		// SCSI���荞�݃��x��
	c->scsi_drives			= 0;		// SCSI�h���C�u��
	c->scsi_sramsync		= 1;		// SCSI�������X�C�b�`�����X�V
	c->scsi_mofirst			= 0;		// MO�h���C�u�D�抄�蓖��
	memset(&c->scsi_file[0][0], 0, sizeof(c->scsi_file));	// SCSI�C���[�W�t�@�C��

	//	Config
	// ���W���[��
	c->resume_fd			= FALSE;	// FD���W���[��
	c->resume_fdi[0]		= TRUE;		// FD�}���t���O
	c->resume_fdi[1]		= FALSE;	// FD�}���t���O
	c->resume_fdw[0]		= FALSE;	// FD�������݋֎~
	c->resume_fdw[1]		= FALSE;	// FD�������݋֎~
	c->resume_fdm[0]		= 0;		// FD���f�B�ANo.
	c->resume_fdm[1]		= 0;		// FD���f�B�ANo.
	c->resume_mo			= 0;		// MO���W���[��
	c->resume_mos			= 0;		// MO�}���t���O
	c->resume_mow			= 0;		// MO�������݋֎~
	c->resume_cd			= 0;		// CD���W���[��
	c->resume_iso			= 0;		// CD�}���t���O
	c->resume_state			= 0;		// �X�e�[�g���W���[��
	c->resume_xm6			= 0;		// �X�e�[�g�L���t���O
	c->resume_screen		= 0;		// ��ʃ��[�h���W���[��
	c->resume_dir			= 0;		// �f�t�H���g�f�B���N�g�����W���[��
	strcpy(c->resume_path, _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\"));

	// �`��
	c->caption_info			= 1;		// �L���v�V�������\��

	// �f�B�X�v���C
	c->caption				= 1;		// �L���v�V����
	c->menu_bar				= 1;		// ���j���[�o�[
	c->status_bar			= 1;		// �X�e�[�^�X�o�[
	c->window_left			= 543;		// �E�B���h�E��`
	c->window_top			= 231;		// �E�B���h�E��`
	c->window_full			= 0;		// �t���X�N���[��
	c->window_mode			= 0;		// ���C�h�X�N���[��

	// WINDRV���W���[��
	c->windrv_enable		= 0;		// Windrv�T�|�[�g 0:���� 1:WindrvXM (2:Windrv�݊�)

	// �z�X�g���t�@�C���V�X�e��
	c->host_option			= 0;		// ����t���O (class CHostFilename �Q��)
	c->host_resume			= FALSE;	// �x�[�X�p�X��ԕ����L�� FALSE���Ɩ���X�L��������
	c->host_drives			= 0;		// �L���ȃh���C�u��
	memset(&c->host_flag[0], 0, sizeof(c->host_flag));		// ����t���O (class CWinFileDrv �Q��)
	memset(&c->host_path[0][0], 0, sizeof(c->host_path));		// �x�[�X�p�X
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
namespace XM6_pid {
	typedef enum X68kKeyCode {
		X68K_KEYCODE_NONE	= 0,
		X68K_KEYCODE_ESC	= 1	,	// 01 [ESC]			.			.				.
		X68K_KEYCODE_1			,	// 02 [1]			!			��				.
		X68K_KEYCODE_2			,	// 03 [2]			"			��				.
		X68K_KEYCODE_3			,	// 04 [3]			#			��				��
		X68K_KEYCODE_4			,	// 05 [4]			$			��				��
		X68K_KEYCODE_5			,	// 06 [5]			%			��				��
		X68K_KEYCODE_6			,	// 07 [6]			&			��				��
		X68K_KEYCODE_7			,	// 08 [7]			'			��				��
		X68K_KEYCODE_8			,	// 09 [8]			(			��				��
		X68K_KEYCODE_9			,	// 0A [9]			)			��				��
		X68K_KEYCODE_0			,	// 0B [0]			.			��				��
		X68K_KEYCODE_MINUS		,	// 0C [-]			=			��				.
		X68K_KEYCODE_CIRCUMFLEX	,	// 0D [^]			~			��				.
		X68K_KEYCODE_YEN		,	// 0E [��]			|			�[				.
		X68K_KEYCODE_BS			,	// 0F [BS]			.			.				.
		X68K_KEYCODE_TAB		,	// 10 [TAB]			.			.				.
		X68K_KEYCODE_Q			,	// 11 [Q]			.			��				.
		X68K_KEYCODE_W			,	// 12 [W]			.			��				.
		X68K_KEYCODE_E			,	// 13 [E]			.			��				��
		X68K_KEYCODE_R			,	// 14 [R]			.			��				.
		X68K_KEYCODE_T			,	// 15 [T]			.			��				.
		X68K_KEYCODE_Y			,	// 16 [Y]			.			��				.
		X68K_KEYCODE_U			,	// 17 [U]			.			��				.
		X68K_KEYCODE_I			,	// 18 [I]			.			��				.
		X68K_KEYCODE_O			,	// 19 [O]			.			��				.
		X68K_KEYCODE_P			,	// 1A [P]			.			��				.
		X68K_KEYCODE_AT			,	// 1B [@]			`			�J				.
		X68K_KEYCODE_LBRACKET	,	// 1C [[]			{			�K				�u
		X68K_KEYCODE_CR			,	// 1D [CR]			.			.				.
		X68K_KEYCODE_A			,	// 1E [A]			.			��				.
		X68K_KEYCODE_S			,	// 1F [S]			.			��				.
		X68K_KEYCODE_D			,	// 20 [D]			.			��				.
		X68K_KEYCODE_F			,	// 21 [F]			.			��				.
		X68K_KEYCODE_G			,	// 22 [G]			.			��				.
		X68K_KEYCODE_H			,	// 23 [H]			.			��				.
		X68K_KEYCODE_J			,	// 24 [J]			.			��				.
		X68K_KEYCODE_K			,	// 25 [K]			.			��				.
		X68K_KEYCODE_L			,	// 26 [L]			.			��				.
		X68K_KEYCODE_SEMICOLON	,	// 27 [;]			+			��				.
		X68K_KEYCODE_COLON		,	// 28 [:]			*			��				.
		X68K_KEYCODE_RBRACKET	,	// 29 []]			}			��				�v
		X68K_KEYCODE_Z			,	// 2A [Z]			.			��				��
		X68K_KEYCODE_X			,	// 2B [X]			.			��				.
		X68K_KEYCODE_C			,	// 2C [C]			.			��				.
		X68K_KEYCODE_V			,	// 2D [V]			.			��				.
		X68K_KEYCODE_B			,	// 2E [B]			.			��				.
		X68K_KEYCODE_N			,	// 2F [N]			.			��				.
		X68K_KEYCODE_M			,	// 30 [M]			.			��				.
		X68K_KEYCODE_COMMA		,	// 31 [,]			<			��				�A
		X68K_KEYCODE_PERIOD		,	// 32 [.]			>			��				�B
		X68K_KEYCODE_SLASH		,	// 33 [/]			?			��				�E
		X68K_KEYCODE_UNDERSCORE	,	// 34 .				_			��				.
		X68K_KEYCODE_SPACE		,	// 35 [SPACE]
		X68K_KEYCODE_HOME		,	// 36 [HOME]
		X68K_KEYCODE_DEL		,	// 37 [DEL]
		X68K_KEYCODE_ROLLUP 	,	// 38 [ROLL UP]
		X68K_KEYCODE_ROLLDOWN 	,	// 39 [ROLL DOWN]
		X68K_KEYCODE_UNDO		,	// 3A [UNDO]
		X68K_KEYCODE_LEFT		,	// 3B [LEFT]
		X68K_KEYCODE_UP			,	// 3C [UP]
		X68K_KEYCODE_RIGHT		,	// 3D [RIGHT]
		X68K_KEYCODE_DOWN		,	// 3E [DOWN]
		X68K_KEYCODE_TKCLR		,	// 3F [Tenkey CLR]
		X68K_KEYCODE_TKSLASH	,	// 40 [Tenkey /]
		X68K_KEYCODE_TKASTERISK	,	// 41 [Tenkey *]
		X68K_KEYCODE_TKMINUS	,	// 42 [Tenkey -]
		X68K_KEYCODE_TK7		,	// 43 [Tenkey 7]
		X68K_KEYCODE_TK8		,	// 44 [Tenkey 8]
		X68K_KEYCODE_TK9		,	// 45 [Tenkey 9]
		X68K_KEYCODE_TKPLUS		,	// 46 [Tenkey +]
		X68K_KEYCODE_TK4		,	// 47 [Tenkey 4]
		X68K_KEYCODE_TK5		,	// 48 [Tenkey 5]
		X68K_KEYCODE_TK6		,	// 49 [Tenkey 6]
		X68K_KEYCODE_TKEQUAL	,	// 4A [Tenkey =]
		X68K_KEYCODE_TK1		,	// 4B [Tenkey 1]
		X68K_KEYCODE_TK2		,	// 4C [Tenkey 2]
		X68K_KEYCODE_TK3		,	// 4D [Tenkey 3]
		X68K_KEYCODE_TKCR		,	// 4E [Tenkey CR]
		X68K_KEYCODE_TK0		,	// 4F [Tenkey 0]
		X68K_KEYCODE_TKCOMMA	,	// 50 [Tenkey ,]
		X68K_KEYCODE_TKPERIOD	,	// 51 [Tenkey .]
		X68K_KEYCODE_KIGOU		,	// 52 [�L������]
		X68K_KEYCODE_TOUROKU	,	// 53 [�o�^]
		X68K_KEYCODE_HELP		,	// 54 [HELP]
		X68K_KEYCODE_XF1		,	// 55 [XF1]
		X68K_KEYCODE_XF2		,	// 56 [XF2]
		X68K_KEYCODE_XF3		,	// 57 [XF3]
		X68K_KEYCODE_XF4		,	// 58 [XF4]
		X68K_KEYCODE_XF5		,	// 59 [XF5]
		X68K_KEYCODE_KANA		,	// 5A [����]
		X68K_KEYCODE_ROMA		,	// 5B [���[�}��]
		X68K_KEYCODE_CODE		,	// 5C [�R�[�h����]
		X68K_KEYCODE_CAPS		,	// 5D [CAPS]
		X68K_KEYCODE_INS		,	// 5E [INS]
		X68K_KEYCODE_HIRAGANA	,	// 5F [�Ђ炪��]
		X68K_KEYCODE_ZENKAKU	,	// 60 [�S�p]
		X68K_KEYCODE_BREAK		,	// 61 [BREAK]
		X68K_KEYCODE_COPY		,	// 62 [COPY]
		X68K_KEYCODE_F1			,	// 63 [F1]
		X68K_KEYCODE_F2			,	// 64 [F2]
		X68K_KEYCODE_F3			,	// 65 [F3]
		X68K_KEYCODE_F4			,	// 66 [F4]
		X68K_KEYCODE_F5			,	// 67 [F5]
		X68K_KEYCODE_F6			,	// 68 [F6]
		X68K_KEYCODE_F7			,	// 69 [F7]
		X68K_KEYCODE_F8			,	// 6A [F8]
		X68K_KEYCODE_F9			,	// 6B [F9]
		X68K_KEYCODE_F10		,	// 6C [F10]
		X68K_KEYCODE_x6d		,	// 6D (Reserved)
		X68K_KEYCODE_x6e		,	// 6E (Reserved)
		X68K_KEYCODE_x6f		,	// 6F (Reserved)
		X68K_KEYCODE_SHIFT		,	// 70 [SHIFT]
		X68K_KEYCODE_CTRL		,	// 71 [CTRL]
		X68K_KEYCODE_OPT1		,	// 72 [OPT1]
		X68K_KEYCODE_OPT2		,	// 73 [OPT2]
		X68K_KEYCODE_MAX		,
		X68K_KEYCODE_FORCE_32BIT	= 0x7fffffff
	} X68kKeycode;

	struct KeyEntry {
		int			targetKeycode;
		X68kKeyCode	x68kKeycode;
	};

	struct KeyMap {
		enum {
			KEY_ENTRY_MAX	= 256,
		};

		int			nKeyEntry;
		KeyEntry	keyEntry[KEY_ENTRY_MAX];
	};

	class DiJoyStick {
	public:
		struct Entry {
			DIDEVICEINSTANCE		diDeviceInstance;
			DIDEVCAPS				diDevCaps;
			LPDIRECTINPUTDEVICE8	diDevice;
			DIJOYSTATE2				joystate;
		};

		DiJoyStick() : entry(0), maxEntry(0), nEntry(0) {
		}

		~DiJoyStick() {
			clear();
		}

		void clear() {
			if(entry) {
				delete [] entry;
				entry = 0;
			}
			maxEntry	= 0;
			nEntry		= 0;
		}

		void enumerate(LPDIRECTINPUT di, DWORD dwDevType = DI8DEVTYPE_JOYSTICK, LPCDIDATAFORMAT lpdf = &c_dfDIJoystick2, DWORD dwFlags = DIEDFL_ATTACHEDONLY, int maxEntry = 16) {
			clear();

			entry			= new Entry [maxEntry];
			callback.di		= di;
			this->maxEntry	= maxEntry;
			nEntry			= 0;
			callback.lpdf	= lpdf;

			di->EnumDevices(dwDevType, DIEnumDevicesCallback_static, this, dwFlags);

			callback.di		= 0;
			callback.lpdf	= 0;
		}

		int getEntryCount() const {
			return nEntry;
		}

		const Entry* getEntry(int index) const {
			const Entry* e = 0;
			if(index >= 0 && index < nEntry) {
				e = &entry[index];
			}
			return e;
		}

		void update() {
			for(int iEntry = 0; iEntry < nEntry; ++iEntry) {
				Entry& e = entry[iEntry];
				LPDIRECTINPUTDEVICE8 d = e.diDevice;

				if(FAILED(d->Poll())) {
					HRESULT hr = d->Acquire();
					while(hr == DIERR_INPUTLOST) {
						hr = d->Acquire();
					}
				} else {
					d->GetDeviceState(sizeof(DIJOYSTATE2), &e.joystate);
				}
			}
		}

	protected:
		static BOOL CALLBACK DIEnumDevicesCallback_static(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef) {
			return reinterpret_cast<DiJoyStick*>(pvRef)->DIEnumDevicesCallback(lpddi, pvRef);
		}

		BOOL DIEnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef) {
			if(nEntry < maxEntry) {
				Entry e = { 0 };

				memcpy(&e.diDeviceInstance, lpddi, sizeof(e.diDeviceInstance));
				e.diDevCaps.dwSize = sizeof(e.diDevCaps);

				LPDIRECTINPUTDEVICE8	did = 0;

				if(SUCCEEDED(callback.di->CreateDevice(lpddi->guidInstance, (LPDIRECTINPUTDEVICE*) &did, 0))) {
					if(SUCCEEDED(did->SetDataFormat(callback.lpdf))) {
						if(SUCCEEDED(did->GetCapabilities(&e.diDevCaps))) {
							e.diDevice = did;
							entry[nEntry++] = e;
						}
					}
				}
			}
			return DIENUM_CONTINUE;
		}

		//
		Entry*			entry;
		int				maxEntry;
		int				nEntry;
		struct {
			LPDIRECTINPUT	di;
			LPCDIDATAFORMAT lpdf;
		} callback;
	};
};

using namespace XM6_pid;

//	Win32 Virtual-Key Codes
//	http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx
//
//		keymap for US standard keyboard
//
static const KeyEntry keyEntry[] = {
	{ VK_ESCAPE					,	X68K_KEYCODE_ESC		},	// 01 [ESC]			.			.				.
	{ '1'						,	X68K_KEYCODE_1			},	// 02 [1]			!			��				.
	{ '2'						,	X68K_KEYCODE_2			},	// 03 [2]			"			��				.
	{ '3'						,	X68K_KEYCODE_3			},	// 04 [3]			#			��				��
	{ '4'						,	X68K_KEYCODE_4			},	// 05 [4]			$			��				��
	{ '5'						,	X68K_KEYCODE_5			},	// 06 [5]			%			��				��
	{ '6'						,	X68K_KEYCODE_6			},	// 07 [6]			&			��				��
	{ '7'						,	X68K_KEYCODE_7			},	// 08 [7]			'			��				��
	{ '8'						,	X68K_KEYCODE_8			},	// 09 [8]			(			��				��
	{ '9'						,	X68K_KEYCODE_9			},	// 0A [9]			)			��				��
	{ '0'						,	X68K_KEYCODE_0			},	// 0B [0]			.			��				��
	{ VK_OEM_MINUS				,	X68K_KEYCODE_MINUS		},	// 0C [-]			=			��				.
	{ VK_OEM_PLUS				,	X68K_KEYCODE_CIRCUMFLEX	},	// 0D [^]			~			��				.
	{ VK_OEM_5					,	X68K_KEYCODE_YEN		},	// 0E [��]			|			�[				.
	{ VK_BACK					,	X68K_KEYCODE_BS			},	// 0F [BS]			.			.				.
	{ VK_TAB					,	X68K_KEYCODE_TAB		},	// 10 [TAB]			.			.				.
	{ 'Q'						,	X68K_KEYCODE_Q			},	// 11 [Q]			.			��				.
	{ 'W'						,	X68K_KEYCODE_W			},	// 12 [W]			.			��				.
	{ 'E'						,	X68K_KEYCODE_E			},	// 13 [E]			.			��				��
	{ 'R'						,	X68K_KEYCODE_R			},	// 14 [R]			.			��				.
	{ 'T'						,	X68K_KEYCODE_T			},	// 15 [T]			.			��				.
	{ 'Y'						,	X68K_KEYCODE_Y			},	// 16 [Y]			.			��				.
	{ 'U'						,	X68K_KEYCODE_U			},	// 17 [U]			.			��				.
	{ 'I'						,	X68K_KEYCODE_I			},	// 18 [I]			.			��				.
	{ 'O'						,	X68K_KEYCODE_O			},	// 19 [O]			.			��				.
	{ 'P'						,	X68K_KEYCODE_P			},	// 1A [P]			.			��				.
	{ 0000						,	X68K_KEYCODE_AT			},	// 1B [@]			`			�J				.
	{ VK_OEM_4					,	X68K_KEYCODE_LBRACKET	},	// 1C [[]			{			�K				�u
	{ VK_RETURN					,	X68K_KEYCODE_CR			},	// 1D [CR]			.			.				.
	{ 'A'						,	X68K_KEYCODE_A			},	// 1E [A]			.			��				.
	{ 'S'						,	X68K_KEYCODE_S			},	// 1F [S]			.			��				.
	{ 'D'						,	X68K_KEYCODE_D			},	// 20 [D]			.			��				.
	{ 'F'						,	X68K_KEYCODE_F			},	// 21 [F]			.			��				.
	{ 'G'						,	X68K_KEYCODE_G			},	// 22 [G]			.			��				.
	{ 'H'						,	X68K_KEYCODE_H			},	// 23 [H]			.			��				.
	{ 'J'						,	X68K_KEYCODE_J			},	// 24 [J]			.			��				.
	{ 'K'						,	X68K_KEYCODE_K			},	// 25 [K]			.			��				.
	{ 'L'						,	X68K_KEYCODE_L			},	// 26 [L]			.			��				.
	{ VK_OEM_1					,	X68K_KEYCODE_SEMICOLON	},	// 27 [;]			+			��				.
	{ 0000						,	X68K_KEYCODE_COLON		},	// 28 [:]			*			��				.
	{ VK_OEM_6					,	X68K_KEYCODE_RBRACKET	},	// 29 []]			}			��				�v
	{ 'Z'						,	X68K_KEYCODE_Z			},	// 2A [Z]			.			��				��
	{ 'X'						,	X68K_KEYCODE_X			},	// 2B [X]			.			��				.
	{ 'C'						,	X68K_KEYCODE_C			},	// 2C [C]			.			��				.
	{ 'V'						,	X68K_KEYCODE_V			},	// 2D [V]			.			��				.
	{ 'B'						,	X68K_KEYCODE_B			},	// 2E [B]			.			��				.
	{ 'N'						,	X68K_KEYCODE_N			},	// 2F [N]			.			��				.
	{ 'M'						,	X68K_KEYCODE_M			},	// 30 [M]			.			��				.
	{ VK_OEM_COMMA				,	X68K_KEYCODE_COMMA		},	// 31 [,]			<			��				�A
	{ VK_OEM_PERIOD				,	X68K_KEYCODE_PERIOD		},	// 32 [.]			>			��				�B
	{ VK_OEM_2					,	X68K_KEYCODE_SLASH		},	// 33 [/]			?			��				�E
	{ 0000						,	X68K_KEYCODE_UNDERSCORE	},	// 34 .				_			��				.
	{ VK_SPACE					,	X68K_KEYCODE_SPACE		},	// 35 [SPACE]
	{ VK_HOME					,	X68K_KEYCODE_HOME		},	// 36 [HOME]
	{ VK_DELETE					,	X68K_KEYCODE_DEL		},	// 37 [DEL]
	{ VK_PRIOR					,	X68K_KEYCODE_ROLLUP 	},	// 38 [ROLL UP]
	{ VK_NEXT					,	X68K_KEYCODE_ROLLDOWN 	},	// 39 [ROLL DOWN]
	{ 0000						,	X68K_KEYCODE_UNDO		},	// 3A [UNDO]
	{ VK_LEFT					,	X68K_KEYCODE_LEFT		},	// 3B [LEFT]
	{ VK_UP						,	X68K_KEYCODE_UP			},	// 3C [UP]
	{ VK_RIGHT					,	X68K_KEYCODE_RIGHT		},	// 3D [RIGHT]
	{ VK_DOWN					,	X68K_KEYCODE_DOWN		},	// 3E [DOWN]
	{ VK_NUMLOCK				,	X68K_KEYCODE_TKCLR		},	// 3F [Tenkey CLR]
	{ VK_DIVIDE					,	X68K_KEYCODE_TKSLASH	},	// 40 [Tenkey /]
	{ VK_MULTIPLY				,	X68K_KEYCODE_TKASTERISK	},	// 41 [Tenkey *]
	{ VK_SUBTRACT				,	X68K_KEYCODE_TKMINUS	},	// 42 [Tenkey -]
	{ VK_NUMPAD7				,	X68K_KEYCODE_TK7		},	// 43 [Tenkey 7]
	{ VK_NUMPAD8				,	X68K_KEYCODE_TK8		},	// 44 [Tenkey 8]
	{ VK_NUMPAD9				,	X68K_KEYCODE_TK9		},	// 45 [Tenkey 9]
	{ VK_ADD					,	X68K_KEYCODE_TKPLUS		},	// 46 [Tenkey +]
	{ VK_NUMPAD4				,	X68K_KEYCODE_TK4		},	// 47 [Tenkey 4]
	{ VK_NUMPAD5				,	X68K_KEYCODE_TK5		},	// 48 [Tenkey 5]
	{ VK_NUMPAD6				,	X68K_KEYCODE_TK6		},	// 49 [Tenkey 6]
	{ 0000						,	X68K_KEYCODE_TKEQUAL	},	// 4A [Tenkey =]
	{ VK_NUMPAD1				,	X68K_KEYCODE_TK1		},	// 4B [Tenkey 1]
	{ VK_NUMPAD2				,	X68K_KEYCODE_TK2		},	// 4C [Tenkey 2]
	{ VK_NUMPAD3				,	X68K_KEYCODE_TK3		},	// 4D [Tenkey 3]
	{ 0000						,	X68K_KEYCODE_TKCR		},	// 4E [Tenkey CR]
	{ VK_NUMPAD0				,	X68K_KEYCODE_TK0		},	// 4F [Tenkey 0]
	{ 0000						,	X68K_KEYCODE_TKCOMMA	},	// 50 [Tenkey ,]
	{ VK_DECIMAL				,	X68K_KEYCODE_TKPERIOD	},	// 51 [Tenkey .]
	{ 0000						,	X68K_KEYCODE_KIGOU		},	// 52 [�L������]
	{ 0000						,	X68K_KEYCODE_TOUROKU	},	// 53 [�o�^]
	{ 0000						,	X68K_KEYCODE_HELP		},	// 54 [HELP]
	{ 0000						,	X68K_KEYCODE_XF1		},	// 55 [XF1]
	{ 0000						,	X68K_KEYCODE_XF2		},	// 56 [XF2]
	{ 0000						,	X68K_KEYCODE_XF3		},	// 57 [XF3]
	{ 0000						,	X68K_KEYCODE_XF4		},	// 58 [XF4]
	{ 0000						,	X68K_KEYCODE_XF5		},	// 59 [XF5]
	{ 0000						,	X68K_KEYCODE_KANA		},	// 5A [����]
	{ 0000						,	X68K_KEYCODE_ROMA		},	// 5B [���[�}��]
	{ 0000						,	X68K_KEYCODE_CODE		},	// 5C [�R�[�h����]
	{ 0000						,	X68K_KEYCODE_CAPS		},	// 5D [CAPS]
	{ VK_INSERT					,	X68K_KEYCODE_INS		},	// 5E [INS]
	{ 0000						,	X68K_KEYCODE_HIRAGANA	},	// 5F [�Ђ炪��]
	{ 0000						,	X68K_KEYCODE_ZENKAKU	},	// 60 [�S�p]
	{ 0000						,	X68K_KEYCODE_BREAK		},	// 61 [BREAK]
	{ 0000						,	X68K_KEYCODE_COPY		},	// 62 [COPY]
	{ VK_F1						,	X68K_KEYCODE_F1			},	// 63 [F1]
	{ VK_F2						,	X68K_KEYCODE_F2			},	// 64 [F2]
	{ VK_F3						,	X68K_KEYCODE_F3			},	// 65 [F3]
	{ VK_F4						,	X68K_KEYCODE_F4			},	// 66 [F4]
	{ VK_F5						,	X68K_KEYCODE_F5			},	// 67 [F5]
	{ VK_F6						,	X68K_KEYCODE_F6			},	// 68 [F6]
	{ VK_F7						,	X68K_KEYCODE_F7			},	// 69 [F7]
	{ VK_F8						,	X68K_KEYCODE_F8			},	// 6A [F8]
	{ VK_F9						,	X68K_KEYCODE_F9			},	// 6B [F9]
	{ VK_F10					,	X68K_KEYCODE_F10		},	// 6C [F10]
	{ -1						,	X68K_KEYCODE_x6d		},	// 6D (Reserved)
	{ -1						,	X68K_KEYCODE_x6e		},	// 6E (Reserved)
	{ -1						,	X68K_KEYCODE_x6f		},	// 6F (Reserved)
	{ VK_SHIFT					,	X68K_KEYCODE_SHIFT		},	// 70 [SHIFT]
	{ VK_CONTROL				,	X68K_KEYCODE_CTRL		},	// 71 [CTRL]
	{ 0000						,	X68K_KEYCODE_OPT1		},	// 72 [OPT1]
	{ 0000						,	X68K_KEYCODE_OPT2		},	// 73 [OPT2]
};

class KeyMapTargetToX68k {
public:
	KeyMapTargetToX68k() {
		clear();
	}

	~KeyMapTargetToX68k() {
	}

	void clear() {
		memset(&x68k_to_target[0], 0, sizeof(x68k_to_target));
		memset(&target_to_x68k[0], 0, sizeof(target_to_x68k));
	}

	void set(int targetKeycode, X68kKeyCode x68kKeyCode) {
		x68k_to_target[x68kKeyCode]		= targetKeycode;
		target_to_x68k[targetKeycode]	= x68kKeyCode;
	}

	X68kKeyCode getX68kKeycodeByTargetKeycode(int targetKeycode) const {
		X68kKeyCode ret = X68K_KEYCODE_NONE;
		if(targetKeycode >= 0 && targetKeycode < 256) {
			ret = target_to_x68k[targetKeycode];
		}
		return ret;
	}

	int getTargetKeycodeByX68kKeycode(X68kKeyCode x68kKeyCode) const {
		int ret = 0;
		if(x68kKeyCode > X68K_KEYCODE_NONE && x68kKeyCode < X68K_KEYCODE_MAX) {
			ret = x68k_to_target[x68kKeyCode];
		}
		return ret;
	}

protected:
	int			x68k_to_target[256];
	X68kKeyCode	target_to_x68k[256];
};



//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
static DiJoyStick djs;
static KeyMapTargetToX68k km;



//---------------------------------------------------------------------------
//
//	
//
//---------------------------------------------------------------------------
static void processInput(BOOL bRun, HWND hWnd) {
	static LPDIRECTINPUT	lpDi		= 0;
	static CRTC*			m_pCRTC;		// CRTC
	static DWORD			m_dwDispCount;	// CRTC�\���J�E���g
	static Keyboard*		m_pKeyboard;	// �L�[�{�[�h
	static Mouse*			m_pMouse;		// �}�E�X
	static PPI*				m_pPPI;			// PPI
	static BOOL				m_bEnable	= TRUE;

	if(lpDi == 0) {
		m_dwDispCount	= 0;

		m_pCRTC		= (CRTC*)		::GetVM()->SearchDevice(MAKEID('C', 'R', 'T', 'C'));
		m_pKeyboard	= (Keyboard*)	::GetVM()->SearchDevice(MAKEID('K', 'E', 'Y', 'B'));
		m_pMouse	= (Mouse*)		::GetVM()->SearchDevice(MAKEID('M', 'O', 'U', 'S'));
		m_pPPI		= (PPI*)		::GetVM()->SearchDevice(MAKEID('P', 'P', 'I', ' '));

		DirectInput8Create(AfxGetApp()->m_hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**) &lpDi, 0);

		djs.enumerate(lpDi);

		//	keyboard
		for(int i = 0, n = sizeof(keyEntry)/sizeof(keyEntry[0]); i < n; ++i) {
			const KeyEntry& e = keyEntry[i];
			if(e.targetKeycode && e.x68kKeycode) {
				km.set(e.targetKeycode, e.x68kKeycode);
			}
		}
	}

	if(m_bEnable) {
		int updateMode = TRUE;

		// bRun = FALSE�Ȃ�A�X�P�W���[����~��(10ms�����ɌĂ΂��)
		if (!bRun) {
			updateMode = FALSE;
		} else {
			// CRTC�̕\���J�E���^�����āA�t���[�����Ƃɏ�������
			ASSERT(m_pCRTC);

			DWORD dwDispCount = m_pCRTC->GetDispCount();
			if (dwDispCount == m_dwDispCount) {
				updateMode = -1;
			} else {
				m_dwDispCount = dwDispCount;
			}
		}

		if(updateMode >= 0) {
			BOOL bEnable = updateMode;

			if(m_pKeyboard) {
				//
				//	update
				//
				static BYTE prevKeys[256];
				BYTE keys[256];

				if(bEnable) {
					for(int i = 0; i < 256; ++i) {
						BYTE k = 0;
						if(GetAsyncKeyState(i) & 0x8000) {
							k = 1;
						}
						keys[i] = k;
					}
				} else {
					memset(&keys[0], 0, sizeof(keys));
				}

				for(int i = 0, n = sizeof(keys)/sizeof(keys[0]); i < n; ++i) {
					if((keys[i] ^ prevKeys[i]) & 0x01) {
						X68kKeyCode	x68kKeyCode	= km.getX68kKeycodeByTargetKeycode(i);
						if(x68kKeyCode != X68K_KEYCODE_NONE) {
							bool		isPressed	= ((keys[i] & 0x01) != 0);
							if(isPressed) {
								//	release -> press
								m_pKeyboard->MakeKey(x68kKeyCode);
							} else {
								//	press -> release
								m_pKeyboard->BreakKey(x68kKeyCode);
							}
						}
					}
				}

				memcpy(&prevKeys[0], &keys[0], sizeof(prevKeys));
			}

			if(m_pMouse) {
//				ASSERT(this);
//				ASSERT(m_pFrmWnd);
//				ASSERT(m_pMouse);
//				ASSERT_VALID(this);

				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(hWnd, &pt);
				int	nMouseX		= pt.x;
				int	nMouseY		= pt.y;
				int bMouseB0	= (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? 1 : 0;
				int bMouseB1	= (GetAsyncKeyState(VK_RBUTTON) & 0x8000) ? 1 : 0;
				int bMouseMode	= GetAsyncKeyState(VK_RSHIFT) & 0x8000;

				// �}�E�X���[�hON��
				if(!bMouseMode || !bEnable) {
					// �{�^��UP��ʒm
					m_pMouse->SetMouse(nMouseX, nMouseY, FALSE, FALSE);
				} else {
					m_pMouse->SetMouse(nMouseX, nMouseY, bMouseB0, bMouseB1);
				}
			}

			djs.update();
			if(m_pPPI) {
				PPI::joyinfo_t ji[PPI::PortMax] = { 0 };

				if(bEnable) {
					const DiJoyStick::Entry* e = djs.getEntry(0);
					if(e) {
						const DIJOYSTATE2* js = &e->joystate;

						int	axisX	= 0;
						int axisY	= 0;
						int btn0	= 0;
						int btn1	= 0;

						{
							int pov = js->rgdwPOV[0];
							if(pov >= 0) {
								static const int V = 0x7ff;
								switch(pov / 4500) {
								default:
								case 0:		axisX	= 0;	axisY	= -V;	break;
								case 1:		axisX	= +V;	axisY	= -V;	break;
								case 2:		axisX	= +V;	axisY	= 0;	break;
								case 3:		axisX	= +V;	axisY	= +V;	break;
								case 4:		axisX	= 0;	axisY	= +V;	break;
								case 5:		axisX	= -V;	axisY	= +V;	break;
								case 6:		axisX	= -V;	axisY	= 0;	break;
								case 7:		axisX	= -V;	axisY	= -V;	break;
								}
							}
						}

						{
							btn0 = js->rgbButtons[0];
							btn1 = js->rgbButtons[1];
						}

						PPI::joyinfo_t* o = &ji[0];
						o->axis[0]		= axisX;
						o->axis[1]		= axisY;
						o->button[0]	= btn0;
						o->button[1]	= btn1;
					}
				}

				// PPI�֑��M
				for(int i=0; i<PPI::PortMax; i++) {
					m_pPPI->SetJoyInfo(i, &ji[i]);
				}
			}
		}
	}
}



//---------------------------------------------------------------------------
//
//	�i�s
//
//---------------------------------------------------------------------------
static void processSound(BOOL bRun, HWND hWnd) {
	HRESULT hr;
	DWORD dwOffset;
	DWORD dwWrite;
	DWORD dwRequest;
	DWORD dwReady;
	WORD *pBuf1;
	WORD *pBuf2;
	DWORD dwSize1;
	DWORD dwSize2;

//	ASSERT(this);

	static BOOL			m_bEnable	= TRUE;

	// �f�o�C�X
	static LPGUID m_lpGUID[16];				// DirectSound�f�o�C�X��GUID
	static int m_nDeviceNum;				// ���o�����f�o�C�X��

	// �Đ�
	static UINT m_uRate;					// �T���v�����O���[�g
	static UINT m_uTick;					// �o�b�t�@�T�C�Y(ms)
	static UINT m_uPoll;					// �|�[�����O�Ԋu(ms)
	static UINT m_uCount;					// �|�[�����O�J�E���g
	static UINT m_uBufSize;					// �o�b�t�@�T�C�Y(�o�C�g)
	static BOOL m_bPlay;					// �Đ��t���O
	static DWORD m_dwWrite;					// �������݊����ʒu
	static int m_nMaster;					// �}�X�^����
	static int m_nFMVol;					// FM����(0�`100)
	static int m_nADPCMVol;					// ADPCM����(0�`100)
	static LPDIRECTSOUND m_lpDS;			// DirectSound
	static LPDIRECTSOUNDBUFFER m_lpDSp;		// DirectSoundBuffer(�v���C�}��)
	static LPDIRECTSOUNDBUFFER m_lpDSb;		// DirectSoundBuffer(�Z�J���_��)
	static DWORD *m_lpBuf;					// �T�E���h�o�b�t�@

	static OPMIF*		m_pOPMIF;			// OPM�C���^�t�F�[�X
	static ADPCM*		m_pADPCM;			// ADPCM
	static SCSI*		m_pSCSI;			// SCSI
	static FM::OPM*		m_pOPM;				// OPM�f�o�C�X
	static Scheduler*	m_pScheduler = 0;	// �X�P�W���[��

	if(m_pScheduler == 0) {
		// �X�P�W���[���擾
		m_pScheduler = (Scheduler*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
		ASSERT(m_pScheduler);

		// OPMIF�擾
		m_pOPMIF = (OPMIF*)::GetVM()->SearchDevice(MAKEID('O', 'P', 'M', ' '));
		ASSERT(m_pOPMIF);

		// ADPCM�擾
		m_pADPCM = (ADPCM*)::GetVM()->SearchDevice(MAKEID('A', 'P', 'C', 'M'));
		ASSERT(m_pADPCM);

		// SCSI�擾
		m_pSCSI = (SCSI*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'S', 'I'));
		ASSERT(m_pSCSI);

		// �f�o�C�X��
	//	EnumDevice();
		{
			//
			//	DirectSound Device enumerator
			//
			class DsEnumerator {
				enum {
					ENTRY_MAX	= 16,
				};

			public:
				struct Entry {
					LPGUID	lpGuid;
					LPCSTR	lpcstrDescription;
					LPCSTR	lpcstrModule;
					LPVOID	lpContext;
				};

				DsEnumerator() : nEntry(0) {
				}

				void enumerate() {
					nEntry = 0;
					DirectSoundEnumerate(DSEnumCallback_static, this);
				}

				int getEntryCount() const {
					return nEntry;
				}

				const Entry* getEntry(int index) const {
					const Entry* e = 0;
					if(index >= 0 && index < nEntry) {
						e = &entry[index];
					}
					return e;
				}

			protected:
				static BOOL CALLBACK DSEnumCallback_static(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext) {
					return reinterpret_cast<DsEnumerator*>(lpContext)->DSEnumCallback(lpGuid, lpcstrDescription, lpcstrModule, lpContext);
				}

				BOOL DSEnumCallback(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext) {
					if(nEntry < ENTRY_MAX) {
						///	@note	bad code. fix this.
						//	See "Remarks" section :
						//		DSEnumCallback
						//		http://msdn.microsoft.com/en-us/library/windows/desktop/microsoft.directx_sdk.reference.dsenumcallback(v=vs.85).aspx
						//	Callback argument memories are local memory.
						//	We must do some allocations for save these variables.
						Entry& e = entry[nEntry++];
						e.lpGuid			= lpGuid;
						e.lpcstrDescription	= lpcstrDescription;
						e.lpcstrModule		= lpcstrModule;
						e.lpContext			= lpContext;
					}
					return TRUE;
				}

				//
				int		nEntry;
				Entry	entry[ENTRY_MAX];
			};

			DsEnumerator de;
			de.enumerate();
			m_nDeviceNum = de.getEntryCount();
			for(int i = 0; i < m_nDeviceNum; ++i) {
				m_lpGUID[i] = de.getEntry(i)->lpGuid;
			}
		}

		m_bEnable = TRUE;

		// �����ł͏��������Ȃ�(ApplyCfg�ɔC����)

		//VC2010//	�ʓ|�Ȃ̂ł����ŏ�����
		{
			int m_nSelectDevice		= 0;
			m_uRate				= 44100;
			m_uTick				= 100;

	//		InitSub();
			{
				bool b = true;

				// rate==0�Ȃ�A�������Ȃ�
				if(b && m_uRate == 0) {
					b = false;
				}

				if(b) {
					ASSERT(!m_lpDS);
					ASSERT(!m_lpDSp);
					ASSERT(!m_lpDSb);
					ASSERT(!m_lpBuf);
					ASSERT(!m_pOPM);

					// �f�o�C�X���Ȃ����0�Ŏ����A����ł��Ȃ����return
					if (m_nDeviceNum <= m_nSelectDevice) {
						if (m_nDeviceNum == 0) {
							b = false;
						} else {
							m_nSelectDevice = 0;
						}
					}
				}

				// DiectSound�I�u�W�F�N�g�쐬
				if(b && FAILED(DirectSoundCreate(m_lpGUID[m_nSelectDevice], &m_lpDS, NULL))) {
					// �f�o�C�X�͎g�p��
					b = false;
				}

				// �������x����ݒ�(�D�拦��)
				if(b && FAILED(m_lpDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY))) {
					b = false;
				}

				// �v���C�}���o�b�t�@���쐬
				if(b) {
					DSBUFFERDESC dsbd = { 0 };
					dsbd.dwSize		= sizeof(dsbd);
					dsbd.dwFlags	= DSBCAPS_PRIMARYBUFFER;

					b = SUCCEEDED(m_lpDS->CreateSoundBuffer(&dsbd, &m_lpDSp, NULL));
				}

				// �v���C�}���o�b�t�@�̃t�H�[�}�b�g���w��
				if(b) {
					WAVEFORMATEX wfex = { 0 };
					wfex.wFormatTag			= WAVE_FORMAT_PCM;
					wfex.nChannels			= 2;
					wfex.nSamplesPerSec		= m_uRate;
					wfex.nBlockAlign		= 4;
					wfex.nAvgBytesPerSec	= wfex.nSamplesPerSec * wfex.nBlockAlign;
					wfex.wBitsPerSample		= 16;

					b = SUCCEEDED(m_lpDSp->SetFormat(&wfex));
				}

				// �Z�J���_���o�b�t�@���쐬
				if(b) {
					PCMWAVEFORMAT pcmwf = { 0 };
					pcmwf.wf.wFormatTag			= WAVE_FORMAT_PCM;
					pcmwf.wf.nChannels			= 2;
					pcmwf.wf.nSamplesPerSec		= m_uRate;
					pcmwf.wf.nBlockAlign		= 4;
					pcmwf.wf.nAvgBytesPerSec	= pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign;
					pcmwf.wBitsPerSample		= 16;

					DSBUFFERDESC dsbd = { 0 };
					dsbd.dwSize					= sizeof(dsbd);
					dsbd.dwFlags				= DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME;
					dsbd.dwBufferBytes			= (pcmwf.wf.nAvgBytesPerSec * m_uTick) / 1000;
					dsbd.dwBufferBytes			= ((dsbd.dwBufferBytes + 7) >> 3) << 3;	// 8�o�C�g���E
					dsbd.lpwfxFormat			= (LPWAVEFORMATEX)&pcmwf;

					m_uBufSize					= dsbd.dwBufferBytes;

					b = SUCCEEDED(m_lpDS->CreateSoundBuffer(&dsbd, &m_lpDSb, NULL));
				}

				// �T�E���h�o�b�t�@���쐬(�Z�J���_���o�b�t�@�Ɠ���̒����A1�P��DWORD)
				if(b) {
					m_lpBuf = new DWORD [ m_uBufSize / 2 ];
					memset(m_lpBuf, sizeof(DWORD) * (m_uBufSize / 2), m_uBufSize);

					// OPM�f�o�C�X(�W��)���쐬
					m_pOPM = new FM::OPM;
					m_pOPM->Init(4000000, m_uRate, true);
					m_pOPM->Reset();
					m_pOPM->SetVolume(m_nFMVol);

					// OPMIF�֒ʒm
					m_pOPMIF->InitBuf(m_uRate);
					m_pOPMIF->SetEngine(m_pOPM);

					// �C�l�[�u���Ȃ牉�t�J�n
				//	if (m_bEnable)
					{
//						Play();
						//
						m_lpDSb->Play(0, 0, DSBPLAY_LOOPING);
						m_bPlay = TRUE;
						m_uCount = 0;
						m_dwWrite = 0;
					}
				}
			}

			// ��ɐݒ�
			if (m_pOPM) {
				{
					int lVolume = 100;		//pConfig->master_volume;
					lVolume = 100 - lVolume;
					lVolume *= (DSBVOLUME_MAX - DSBVOLUME_MIN);
					lVolume /= -200;
					m_lpDSb->SetVolume(lVolume);
				}
				m_pOPMIF->EnableFM(1);		//pConfig->fm_enable);
				m_pOPM->SetVolume(54);		//pConfig->fm_volume);
				m_pADPCM->EnableADPCM(1);	//pConfig->adpcm_enable);
				m_pADPCM->SetVolume(52);	//pConfig->adpcm_volume);
			}
			m_nMaster	= 100;	//pConfig->master_volume;
			m_uPoll		= 5;	//(UINT)pConfig->polling_buffer;
		}
	}

	// �J�E���g����(m_nPoll��ɂP��A������VM��~���͏펞)
	m_uCount++;
	if ((m_uCount < m_uPoll) && bRun) {
		return;
	}
	m_uCount = 0;

	// �f�B�Z�[�u���Ȃ�A�������Ȃ�
	if (!m_bEnable) {
		return;
	}

	// ����������Ă��Ȃ���΁A�������Ȃ�
	if (!m_pOPM) {
		m_pScheduler->SetSoundTime(0);
		return;
	}

	// �v���C��ԂłȂ���΁A�֌W�Ȃ�
	if (!m_bPlay) {
		m_pScheduler->SetSoundTime(0);
		return;
	}

	// ���݂̃v���C�ʒu�𓾂�(�o�C�g�P��)
	ASSERT(m_lpDSb);
	ASSERT(m_lpBuf);
	if (FAILED(m_lpDSb->GetCurrentPosition(&dwOffset, &dwWrite))) {
		return;
	}
	ASSERT(m_lpDSb);
	ASSERT(m_lpBuf);

	// �O�񏑂����񂾈ʒu����A�󂫃T�C�Y���v�Z(�o�C�g�P��)
	if (m_dwWrite <= dwOffset) {
		dwRequest = dwOffset - m_dwWrite;
	}
	else {
		dwRequest = m_uBufSize - m_dwWrite;
		dwRequest += dwOffset;
	}

	// �󂫃T�C�Y���S�̂�1/4�𒴂��Ă��Ȃ���΁A���̋@���
	if (dwRequest < (m_uBufSize / 4)) {
		return;
	}

	// �󂫃T���v���Ɋ��Z(L,R��1�Ɛ�����)
	ASSERT((dwRequest & 3) == 0);
	dwRequest /= 4;

	// m_lpBuf�Ƀo�b�t�@�f�[�^���쐬�B�܂�bRun�`�F�b�N
	if (!bRun) {
		memset(m_lpBuf, 0, m_uBufSize * 2);
		m_pOPMIF->InitBuf(m_uRate);
	}
	else {
		// OPM�ɑ΂��āA�����v���Ƒ��x����
		dwReady = m_pOPMIF->ProcessBuf();
		m_pOPMIF->GetBuf(m_lpBuf, (int)dwRequest);
		if (dwReady < dwRequest) {
			dwRequest = dwReady;
		}

		// ADPCM�ɑ΂��āA�f�[�^��v��(���Z���邱��)
		m_pADPCM->GetBuf(m_lpBuf, (int)dwRequest);

		// ADPCM�̓�������
		if (dwReady > dwRequest) {
			m_pADPCM->Wait(dwReady - dwRequest);
		}
		else {
			m_pADPCM->Wait(0);
		}

		// SCSI�ɑ΂��āA�f�[�^��v��(���Z���邱��)
		m_pSCSI->GetBuf(m_lpBuf, (int)dwRequest, m_uRate);
	}

	// �����Ń��b�N
	hr = m_lpDSb->Lock(m_dwWrite, (dwRequest * 4),
						(void**)&pBuf1, &dwSize1,
						(void**)&pBuf2, &dwSize2,
						0);
	// �o�b�t�@�������Ă���΁A���X�g�A
	if (hr == DSERR_BUFFERLOST) {
		m_lpDSb->Restore();
	}
	// ���b�N�������Ȃ���΁A�����Ă��Ӗ����Ȃ�
	if (FAILED(hr)) {
		m_dwWrite = dwOffset;
		return;
	}

	// �ʎq��bit=16��O��Ƃ���
	ASSERT((dwSize1 & 1) == 0);
	ASSERT((dwSize2 & 1) == 0);

	// MMX���߂ɂ��p�b�N(dwSize1+dwSize2�ŁA����5000�`15000���x�͏�������)
	SoundMMX(m_lpBuf, pBuf1, dwSize1);
	if (dwSize2 > 0) {
		SoundMMX(&m_lpBuf[dwSize1 / 2], pBuf2, dwSize2);
	}
	SoundEMMS();

	// �A�����b�N
	m_lpDSb->Unlock(pBuf1, dwSize1, pBuf2, dwSize2);

	// m_dwWrite�X�V
	m_dwWrite += dwSize1;
	m_dwWrite += dwSize2;
	if (m_dwWrite >= m_uBufSize) {
		m_dwWrite -= m_uBufSize;
	}
	ASSERT(m_dwWrite < m_uBufSize);
}

UINT ThreadFunc(LPVOID pParam) {
	extern CFrmWnd*	globalFrmWnd;
	CFrmWnd*	m_pFrmWnd	= globalFrmWnd;

	VM*			pVM			= ::GetVM();
	Render*		pRender		= (Render*)pVM->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
	HWND		hFrmWnd		= m_pFrmWnd->m_hWnd;
	CDrawView*	pDrawView	= m_pFrmWnd->GetView();
	DWORD		dwExecTime	= GetTime();
	DWORD		dwExecCount	= 0;

	// �I�����N�G�X�g���オ��܂Ń��[�v
	while (!scheduler_m_bExitReq) {
		int	preSleep	= 0;
		int postSleep	= -1;

		if(preSleep >= 0) {
			::Sleep(preSleep);
		}

		bool requestRefresh	= false;

		::LockVM();

		// �L���t���O���オ���Ă��Ȃ���΁A��~��
		if(! schedulerIsEnable()) {
			// �`��
			requestRefresh = true;
			dwExecCount = 0;

			// ���R���|�[�l���g�̏����A���Ԃ��킹
			processSound(FALSE, hFrmWnd);
			processInput(FALSE, hFrmWnd);
			dwExecTime = GetTime();
			postSleep = 10;
		} else {
			DWORD dwTime = GetTime();
			if(dwExecTime > dwTime) {
				requestRefresh = true;
				dwExecCount = 0;
				if(dwExecTime > GetTime()) {
					postSleep = 1;
				}
			} else {
				// �����_�����O�ۂ𔻒�(1or36)
				pRender->EnableAct(dwExecTime >= dwTime);

				if(pVM->Exec(1000 * 2)) {
					if(pVM->IsPower()) {
						dwExecCount++;
						dwExecTime++;

						// ���R���|�[�l���g�̏���
						processSound(TRUE, hFrmWnd);
						processInput(TRUE, hFrmWnd);

						// dwExecCount���K�萔�𒴂�����A��x�\�����ċ������ԍ��킹
						if (dwExecCount > 400) {
							requestRefresh = true;
							dwExecCount = 0;
							dwExecTime = GetTime();
						}
					}
				}
			}
		}

		if(requestRefresh) {
			if(! schedulerIsEnable() || pRender->IsReady()) {
				pDrawView->Draw(-1);
				pRender->Complete();
			}
		}

		::UnlockVM();

		if(postSleep >= 0) {
			::Sleep(postSleep);
		}
	}
	return 0;
}

void schedulerInit() {
	scheduler_m_bExitReq		= FALSE;
	scheduler_mm_bEnable		= FALSE;

	::timeBeginPeriod(1);
	AfxBeginThread(ThreadFunc, 0);
}



//---------------------------------------------------------------------------
//
//	�V�F���萔��`
//	��include�t�@�C���ł͂Ȃ��A�A�v���P�[�V�������Œ�`����悤��߂��Ă���
//
//---------------------------------------------------------------------------
#define SHCNRF_InterruptLevel			0x0001
#define SHCNRF_ShellLevel				0x0002
#define SHCNRF_NewDelivery				0x8000

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CFrmWnd::CFrmWnd()
{
	globalFrmWnd = this;

	// VM�E�X�e�[�^�X�R�[�h
	::pVM = NULL;
	m_nStatus = -1;

	// �f�o�C�X
	m_pFDD = NULL;
	m_pSASI = NULL;
	m_pSCSI = NULL;
	m_pScheduler = NULL;
	m_pKeyboard = NULL;
	m_pMouse = NULL;

	// �R���|�[�l���g
//	m_pFirstComponent = NULL;
	m_pDrawView = NULL;
//	m_pSch = NULL;
//	m_pSound = NULL;
//	m_pInput = NULL;
//	m_pHost = NULL;
//	m_pConfig = NULL;

	// �t���X�N���[��
	m_bFullScreen = FALSE;
	m_hTaskBar = NULL;
	memset(&m_DevMode, 0, sizeof(m_DevMode));
	m_nWndLeft = 0;
	m_nWndTop = 0;

	// �T�u�E�B���h�E
	m_strWndClsName.Empty();

	// �X�e�[�^�X�o�[�E���j���[�E�L���v�V����
	m_bMenuBar = TRUE;

	// �V�F���ʒm
	m_uNotifyId = NULL;

	// �R���t�B�M�����[�V����
	m_bMouseMid = TRUE;
	m_bPopup = FALSE;
	m_bAutoMouse = TRUE;

	// ���̑��ϐ�
	m_bExit = FALSE;
	m_bSaved = FALSE;
	m_nFDDStatus[0] = 0;
	m_nFDDStatus[1] = 0;
	m_dwExec = 0;
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CFrmWnd, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_DISPLAYCHANGE, OnDisplayChange)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_MOVE()
	ON_WM_ACTIVATE()
	ON_WM_ACTIVATEAPP()
	ON_WM_ENTERMENULOOP()
	ON_WM_EXITMENULOOP()
	ON_WM_PARENTNOTIFY()
	ON_MESSAGE(WM_KICK, OnKick)
	ON_WM_DRAWITEM()
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_POWERBROADCAST, OnPowerBroadCast)
	ON_WM_SYSCOMMAND()
#if _MFC_VER >= 0x700
	ON_WM_COPYDATA()
#else
	ON_MESSAGE(WM_COPYDATA, OnCopyData)
#endif
	ON_WM_ENDSESSION()
	ON_MESSAGE(WM_SHELLNOTIFY, OnShellNotify)

/*
	ON_COMMAND(IDM_OPEN, OnOpen)
	ON_UPDATE_COMMAND_UI(IDM_OPEN, OnOpenUI)
	ON_COMMAND(IDM_SAVE, OnSave)
	ON_UPDATE_COMMAND_UI(IDM_SAVE, OnSaveUI)
	ON_COMMAND(IDM_SAVEAS, OnSaveAs)
	ON_UPDATE_COMMAND_UI(IDM_SAVEAS, OnSaveAsUI)
*/
	ON_COMMAND(IDM_RESET, OnReset)
	ON_UPDATE_COMMAND_UI(IDM_RESET, OnResetUI)
/*
	ON_COMMAND(IDM_INTERRUPT, OnInterrupt)
	ON_UPDATE_COMMAND_UI(IDM_INTERRUPT, OnInterruptUI)
	ON_COMMAND(IDM_POWER, OnPower)
	ON_UPDATE_COMMAND_UI(IDM_POWER, OnPowerUI)
	ON_COMMAND(IDM_EXIT, OnExit)
	ON_COMMAND_RANGE(IDM_D0OPEN, IDM_D1_MRU8, OnFD)
*/
	ON_COMMAND_RANGE(IDM_D0OPEN, IDM_D1OPEN, OnFD)
	ON_UPDATE_COMMAND_UI(IDM_D0OPEN, OnFDOpenUI)
	ON_UPDATE_COMMAND_UI(IDM_D1OPEN, OnFDOpenUI)
/*
	ON_UPDATE_COMMAND_UI(IDM_D0EJECT, OnFDEjectUI)
	ON_UPDATE_COMMAND_UI(IDM_D1EJECT, OnFDEjectUI)
	ON_UPDATE_COMMAND_UI(IDM_D0WRITEP, OnFDWritePUI)
	ON_UPDATE_COMMAND_UI(IDM_D1WRITEP, OnFDWritePUI)
	ON_UPDATE_COMMAND_UI(IDM_D0FORCE, OnFDForceUI)
	ON_UPDATE_COMMAND_UI(IDM_D1FORCE, OnFDForceUI)
	ON_UPDATE_COMMAND_UI(IDM_D0INVALID, OnFDInvalidUI)
	ON_UPDATE_COMMAND_UI(IDM_D1INVALID, OnFDInvalidUI)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_D0_MEDIA0, IDM_D0_MEDIAF, OnFDMediaUI)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_D1_MEDIA0, IDM_D1_MEDIAF, OnFDMediaUI)
//	ON_UPDATE_COMMAND_UI_RANGE(IDM_D0_MRU0, IDM_D0_MRU8, OnFDMRUUI)
//	ON_UPDATE_COMMAND_UI_RANGE(IDM_D1_MRU0, IDM_D1_MRU8, OnFDMRUUI)
*/
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL CFrmWnd::Init()
{
	// �E�B���h�E�쐬
	if (!Create(NULL, _T("XM6"),
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
			WS_BORDER | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
			rectDefault, NULL, NULL, 0, NULL)) {
		return FALSE;
	}

	// ����ȊO�̏�������OnCrate�ɔC����
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�쐬����
//
//---------------------------------------------------------------------------
BOOL CFrmWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	// ��{�N���X
	if (!CFrameWnd::PreCreateWindow(cs)) {
		return FALSE;
	}

	// �N���C�A���g�G�b�W���O��
	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�쐬
//
//---------------------------------------------------------------------------
int CFrmWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	LONG lUser;
	CMenu *pSysMenu;
	UINT nCount;
	CString string;

	// ��{�N���X
	if (CFrameWnd::OnCreate(lpCreateStruct) != 0) {
		return -1;
	}

	// ���[�U�f�[�^�w��
	lUser = (LONG)MAKEID('X', 'M', '6', ' ');
	::SetWindowLong(m_hWnd, GWL_USERDATA, lUser);

	// �A�N�Z�����[�^�w��A�A�C�R���w��AIMM�w��
	LoadAccelTable(MAKEINTRESOURCE(IDR_ACCELERATOR));
	SetIcon(AfxGetApp()->LoadIcon(IDI_APPICON), TRUE);
	::ImmAssociateContext(m_hWnd, (HIMC)NULL);

	// ���j���[(�E�B���h�E)
	if (::IsJapanese()) {
		// ���{�ꃁ�j���[
		m_Menu.LoadMenu(IDR_MENU);
		m_PopupMenu.LoadMenu(IDR_MENUPOPUP);
	}
	else {
		// �p�ꃁ�j���[
		m_Menu.LoadMenu(IDR_US_MENU);
		m_PopupMenu.LoadMenu(IDR_US_MENUPOPUP);
	}
	SetMenu(&m_Menu);
	m_bMenuBar = TRUE;
	m_bPopupMenu = FALSE;

	// ���j���[(�V�X�e��)
	::GetMsg(IDS_STDWIN, string);
	pSysMenu = GetSystemMenu(FALSE);
	ASSERT(pSysMenu);
	nCount = pSysMenu->GetMenuItemCount();

	// �u�E�B���h�E�W���ʒu�v��}��
	pSysMenu->InsertMenu(nCount - 2, MF_BYPOSITION | MF_STRING, IDM_STDWIN, string);
	pSysMenu->InsertMenu(nCount - 2, MF_BYPOSITION | MF_SEPARATOR);

	// �`���C���h�E�B���h�E������
	if (!InitChild()) {
		return -1;
	}

	// �E�B���h�E�ʒu�A��`������
	InitPos();

	// �V�F���ʒm������
	InitShell();

	// VM������
	if (!InitVM()) {
		// VM�������G���[
		m_nStatus = 1;
		PostMessage(WM_KICK, 0, 0);
		return 0;
	}

	// �o�[�W�������\�[�X����VM�փo�[�W������`�B
	InitVer();

	// �f�o�C�X�L��
	m_pFDD = (FDD*)::GetVM()->SearchDevice(MAKEID('F', 'D', 'D', ' '));
	ASSERT(m_pFDD);
	m_pSASI = (SASI*)::GetVM()->SearchDevice(MAKEID('S', 'A', 'S', 'I'));
	ASSERT(m_pSASI);
	m_pSCSI = (SCSI*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(m_pSCSI);
	m_pScheduler = (Scheduler*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pScheduler);
	m_pKeyboard = (Keyboard*)::GetVM()->SearchDevice(MAKEID('K', 'E', 'Y', 'B'));
	ASSERT(m_pKeyboard);
	m_pMouse = (Mouse*)::GetVM()->SearchDevice(MAKEID('M', 'O', 'U', 'S'));
	ASSERT(m_pMouse);

	// �R���|�[�l���g�쐬�A������
	if (!InitComponent()) {
		// �R���|�[�l���g�������G���[
		m_nStatus = 2;
		PostMessage(WM_KICK, 0, 0);
		return 0;
	}

	// �ݒ�K�p(OnOption�Ɠ��l�AVM���b�N����)
	::LockVM();
	ApplyCfg();
	::UnlockVM();

	// ���Z�b�g
	::GetVM()->Reset();

	// �E�C���h�E�ʒu�����W���[��(m_nStatus != 0�ɗ���)
	ASSERT(m_nStatus != 0);
	RestoreFrameWnd(FALSE);

	// ���b�Z�[�W���|�X�g���ďI��
	m_nStatus = 0;
	PostMessage(WM_KICK, 0, 0);
	return 0;
}

//---------------------------------------------------------------------------
//
//	�`���C���h�E�B���h�E������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::InitChild()
{
	HDC hDC;
	HFONT hFont;
	HFONT hDefFont;
	TEXTMETRIC tm;
	int i;
	int nWidth;
	UINT uIndicator[6];

	// �r���[�쐬
	m_pDrawView = new CDrawView;
	if (!m_pDrawView->Init(this)) {
		return FALSE;
	}

	uIndicator[0] = ID_SEPARATOR;
	for (i=1; i<6; i++) {
		uIndicator[i] = (UINT)i;
	}

	// �e�L�X�g���g���b�N���擾
	hDC = ::GetDC(m_hWnd);
	hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	hDefFont = (HFONT)::SelectObject(hDC, hFont);
	ASSERT(hDefFont);
	::GetTextMetrics(hDC, &tm);
	::SelectObject(hDC, hDefFont);
	::ReleaseDC(m_hWnd, hDC);

	// �T�C�Y�ݒ胋�[�v
	nWidth = 0;
	for (i=1; i<6; i++) {
		switch (i) {
			// FD0, FD1
			case 1:
			case 2:
				nWidth = tm.tmAveCharWidth * 32;
				break;

			// HD BUSY
			case 3:
				nWidth = tm.tmAveCharWidth * 10;
				break;

			// TIMER
			case 4:
				nWidth = tm.tmAveCharWidth * 9;
				break;

			// POWER
			case 5:
				nWidth = tm.tmAveCharWidth * 9;
				break;
		}
	}

	// �ă��C�A�E�g
	RecalcLayout();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ʒu�E��`������
//	��bStart=FALSE�̏ꍇ�AbFullScreen=FALSE�̎��Ɉʒu�𕜌����邱��
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::InitPos(BOOL bStart)
{
	int cx;
	int cy;
	CRect rect;
	CRect rectStatus;
	CRect rectWnd;

	ASSERT(this);

	// �X�N���[���T�C�Y�A�E�B���h�E��`���擾
	cx = ::GetSystemMetrics(SM_CXSCREEN);
	cy = ::GetSystemMetrics(SM_CYSCREEN);
	GetWindowRect(&rectWnd);

	// 800x600�ȉ��̓X�N���[���T�C�Y�����ς��ɍL����
	if ((cx <= 800) || (cy <= 600)) {
		if ((rectWnd.left != 0) || (rectWnd.top != 0)) {
			SetWindowPos(&wndTop, 0, 0, cx, cy, SWP_NOZORDER);
			return;
		}
		if ((rectWnd.Width() != cx) || (rectWnd.Height() != cy)) {
			SetWindowPos(&wndTop, 0, 0, cx, cy, SWP_NOZORDER);
			return;
		}
		return;
	}

	// 824x560(DDP2)���m���C���^�[���[�X�̍ő�T�C�Y�ƔF��
	rect.left = 0;
	rect.top = 0;
	rect.right = 824;
	rect.bottom = 560;
	::AdjustWindowRectEx(&rect, GetView()->GetStyle(), FALSE, GetView()->GetExStyle());
//	m_StatusBar.GetWindowRect(&rectStatus);
	rect.bottom += rectStatus.Height();
	::AdjustWindowRectEx(&rect, GetStyle(), TRUE, GetExStyle());

	// rect.left, rect.bottom�����ɂȂ�炵��(����ȍ~�Aright,bottom��cx,cy������)
	rect.right -= rect.left;
	rect.left = 0;
	rect.bottom -= rect.top;
	rect.top = 0;

	// �]�T������΁A�Z���^�����O
	if (rect.right < cx) {
		rect.left = (cx - rect.right) / 2;
	}
	if (rect.bottom < cy) {
		rect.top = (cy - rect.bottom) / 2;
	}

	// bStart�ŕ�����(�����J�n���A�E�B���h�E�|�t���X�N���[���̐؂�ւ���)
	if (bStart) {
		// �E�B���h�E�ʒu����U�ۑ�(���̌�A�ēxRestoreFrameWnd�̃`�����X����)
		m_nWndLeft = rect.left;
		m_nWndTop = rect.top;
	}
	else {
		// �E�B���h�E���[�h�̎��Ɍ���A�ʒu��␳
		if (!m_bFullScreen) {
			if ((rect.left == 0) && (rect.top == 0)) {
				// WM_DISPLAYCHANGE���b�Z�[�W�����āA�E�B���h�E���������Ȃ����ꍇ
				m_nWndLeft = rect.left;
				m_nWndTop = rect.top;
			}
			else {
				// ����ȊO(�t���X�N���[�����E�B���h�E�ւ̏�ԑJ�ڂ��܂�)
				rect.left = m_nWndLeft;
				rect.top = m_nWndTop;
			}
		}
	}

	// �ݒ�
	if ((rect.left != rectWnd.left) || (rect.top != rectWnd.top)) {
		SetWindowPos(&wndTop, rect.left, rect.top, rect.right, rect.bottom, 0);
		return;
	}
	if ((rect.right != rectWnd.Width()) || (rect.bottom != rectWnd.Height())) {
		SetWindowPos(&wndTop, rect.left, rect.top, rect.right, rect.bottom, 0);
		return;
	}
}

//---------------------------------------------------------------------------
//
//	�V�F���A�g������
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::InitShell()
{
	int nSources;

	// �ʒm�v����ݒ�
	if (::IsWinNT()) {
		// Windows2000/XP:shared memory�𗘗p����t���O��ǉ�
		nSources = SHCNRF_InterruptLevel | SHCNRF_ShellLevel | SHCNRF_NewDelivery;
	}
	else {
		// Windows9x:shared memory�͎g�p���Ȃ�
		nSources = SHCNRF_InterruptLevel | SHCNRF_ShellLevel;
	}

	// �G���g����������
	m_fsne[0].pidl = NULL;
	m_fsne[0].fRecursive = FALSE;

	// �V�F���ʒm���b�Z�[�W��o�^
	m_uNotifyId = ::SHChangeNotifyRegister(m_hWnd,
							nSources,
							SHCNE_MEDIAINSERTED | SHCNE_MEDIAREMOVED | SHCNE_DRIVEADD | SHCNE_DRIVEREMOVED,
							WM_SHELLNOTIFY,
							sizeof(m_fsne)/sizeof(m_fsne[0]),
							m_fsne);
	ASSERT(m_uNotifyId);
}

//---------------------------------------------------------------------------
//
//	VM������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::InitVM()
{
	::pVM = new VM;
	if (!::GetVM()->Init()) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�R���|�[�l���g������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::InitComponent()
{
	BOOL bSuccess = TRUE;
	schedulerInit();
	return bSuccess;
}

//---------------------------------------------------------------------------
//
//	�o�[�W����������
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::InitVer()
{
	TCHAR szPath[_MAX_PATH];
	DWORD dwHandle;
	DWORD dwLength;
	BYTE *pVerInfo;
	VS_FIXEDFILEINFO *pFileInfo;
	UINT uLength;
	DWORD dwMajor;
	DWORD dwMinor;

	ASSERT(this);

	// �p�X���擾
	::GetModuleFileName(NULL, szPath, _MAX_PATH);

	// �o�[�W��������ǂݎ��
	dwLength = GetFileVersionInfoSize(szPath, &dwHandle);
	if (dwLength == 0) {
		return;
	}

	pVerInfo = new BYTE[ dwLength ];
	if (::GetFileVersionInfo(szPath, dwHandle, dwLength, pVerInfo) == 0) {
		return;
	}

	// �o�[�W�����������o��
	if (::VerQueryValue(pVerInfo, _T("\\"), (LPVOID*)&pFileInfo, &uLength) == 0) {
		delete[] pVerInfo;
		return;
	}

	// �o�[�W�����𕪗��AVM�֒ʒm
	dwMajor = (DWORD)HIWORD(pFileInfo->dwProductVersionMS);
	dwMinor = (DWORD)(LOWORD(pFileInfo->dwProductVersionMS) * 16
					+ HIWORD(pFileInfo->dwProductVersionLS));
	::GetVM()->SetVersion(dwMajor, dwMinor);

	// �I��
	delete[] pVerInfo;
}

//---------------------------------------------------------------------------
//
//	�R�}���h���C������
//	���R�}���h���C���AWM_COPYDATA�ŋ���
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::InitCmd(LPCTSTR lpszCmd)
{
	LPCTSTR lpszCurrent;
	LPCTSTR lpszNext;
	TCHAR szPath[_MAX_PATH];
	int nLen;
	int i;
	BOOL bReset;

	ASSERT(this);
	ASSERT(lpszCmd);

	// �|�C���^�A�t���O������
	lpszCurrent = lpszCmd;
	bReset = FALSE;

	// ���[�v
	for (i=0; i<2; i++) {
		// �X�y�[�X�A�^�u�̓X�L�b�v
		while (lpszCurrent[0] <= _T(0x20)) {
			if (lpszCurrent[0] == _T('\0')) {
				break;
			}
			lpszCurrent++;
		}
		if (lpszCurrent[0] == _T('\0')) {
			break;
		}

		// �ŏ����_�u���N�H�[�g�Ȃ�A���̃N�H�[�g��T��
		if (lpszCurrent[0] == _T('\x22')) {
			lpszNext = _tcschr(lpszCurrent + 1, _T('\x22'));
			if (!lpszNext) {
				// �Ή�����_�u���N�H�[�g��������Ȃ�
				return;
			}
			nLen = (int)(lpszNext - (lpszCurrent + 1));
			if (nLen >= _MAX_PATH) {
				// ��������
				return;
			}

			// �N�H�[�g���ꂽ�������R�s�[
			_tcsnccpy(szPath, &lpszCurrent[1], nLen);
			szPath[nLen] = _T('\0');

			// �N�H�[�g�̎����w��
			lpszCurrent = &lpszNext[1];
		}
		else {
			// ���̃X�y�[�X��T��
			lpszNext = _tcschr(lpszCurrent + 1, _T(' '));
			if (lpszNext) {
				// �X�y�[�X�܂�
				nLen = (int)(lpszNext - lpszCurrent);
				if (nLen >= _MAX_PATH) {
					// ��������
					return;
				}

				// �X�y�[�X�܂ł̕������R�s�[
				_tcsnccpy(szPath, lpszCurrent, nLen);
				szPath[nLen] = _T('\0');

				// �X�y�[�X�̎����w��
				lpszCurrent = &lpszNext[1];
			}
			else {
				// �I�[�܂�
				_tcscpy(szPath, lpszCurrent);
				lpszCurrent = NULL;
			}
		}

		// �I�[�v�������݂�
		bReset = InitCmdSub(i, szPath);

		// �I�[�Ȃ�I��
		if (!lpszCurrent) {
			break;
		}
	}

	// ���Z�b�g�v��������΁A���Z�b�g
	if (bReset) {
		OnReset();
	}
}

//---------------------------------------------------------------------------
//
//	�R�}���h���C������ �T�u
//	���R�}���h���C���AWM_COPYDATA�A�h���b�O&�h���b�v�ŋ���
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::InitCmdSub(int nDrive, LPCTSTR lpszPath)
{
	Filepath path;
	Fileio fio;
	LPTSTR lpszFile;
	DWORD dwSize;
	TCHAR szPath[_MAX_PATH];
	FDI *pFDI;
	CString strMsg;

	ASSERT(this);
	ASSERT((nDrive == 0) || (nDrive == 1));
	ASSERT(lpszPath);

	// pFDI������
	pFDI = NULL;

	// �t�@�C���I�[�v���`�F�b�N
	path.SetPath(lpszPath);
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}
	dwSize = fio.GetFileSize();
	fio.Close();

	// �t���p�X��
	::GetFullPathName(lpszPath, _MAX_PATH, szPath, &lpszFile);
	path.SetPath(szPath);

	// VM���b�N
	::LockVM();

/*
	// 128MO or 230MO or 540MO or 640MO
	if ((dwSize == 0x797f400) || (dwSize == 0xd9eea00) ||
		(dwSize == 0x1fc8b800) || (dwSize == 0x25e28000)) {
		// MO�̊��蓖�Ă����݂�
		nDrive = 2;

		if (!m_pSASI->Open(path)) {
			// MO���蓖�Ď��s
//			GetScheduler()->Reset();
//			ResetCaption();
			::UnlockVM();
			return FALSE;
		}
	}
	else if (dwSize >= 0x200000) {
		// VM�̊��蓖�Ă����݂�
		nDrive = 4;

		// �I�[�v���O����
		if (!OnOpenPrep(path, FALSE)) {
			// �t�@�C�����Ȃ����A�o�[�W�����Ȃǂ��������Ȃ�
//				GetScheduler()->Reset();
//				ResetCaption();
			::UnlockVM();
			return FALSE;
		}

		// ���[�h���s(OnOpenSub�ɔC����)
		::UnlockVM();
		if (OnOpenSub(path)) {
			Filepath::SetDefaultDir(szPath);
		}
		// ���Z�b�g�͍s��Ȃ�
		return FALSE;
	}
	else
*/
	{
		// FD�̊��蓖�Ă����݂�
		if (!m_pFDD->Open(nDrive, path)) {
			// FD���蓖�Ď��s
//				GetScheduler()->Reset();
//				ResetCaption();
			::UnlockVM();
			return FALSE;
		}
		pFDI = m_pFDD->GetFDI(nDrive);
	}

	// VM���Z�b�g�A���b�N����
//	GetScheduler()->Reset();
//	ResetCaption();
	::UnlockVM();

	// �����B�f�B���N�g���ۑ���MRU�ǉ�
	Filepath::SetDefaultDir(szPath);
//	GetConfig()->SetMRUFile(nDrive, szPath);

	// �t���b�s�[�Ȃ�ABAD�C���[�W�x��
	if (pFDI) {
		if (pFDI->GetID() == MAKEID('B', 'A', 'D', ' ')) {
			::GetMsg(IDS_BADFDI_WARNING, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		}

		// �t���b�s�[�����蓖�Ă��Ƃ������A���Z�b�g����
		return TRUE;
	}

	// �I��
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	�R���|�[�l���g���Z�[�u
//	���X�P�W���[���͒�~���Ă��邪�ACSound,CInput�͓��쒆
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::SaveComponent(const Filepath& path, DWORD dwPos)
{
	Fileio fio;
	DWORD dwID;
	DWORD dwMajor;
	DWORD dwMinor;
	int nVer;

	ASSERT(this);
	ASSERT(dwPos > 0);

	// �o�[�W�������쐬
	::GetVM()->GetVersion(dwMajor, dwMinor);
	nVer = (int)((dwMajor << 8) | dwMinor);

	// �t�@�C���I�[�v���ƃV�[�N
	if (!fio.Open(path, Fileio::Append)) {
		return FALSE;
	}
	if (!fio.Seek(dwPos)) {
		fio.Close();
		return FALSE;
	}

	// ���C���R���|�[�l���g����ۑ�
	dwID = MAKEID('M', 'A', 'I', 'N');
	if (!fio.Write(&dwID, sizeof(dwID))) {
		fio.Close();
		return FALSE;
	}

	// �I�[��������
	dwID = MAKEID('E', 'N', 'D', ' ');
	if (!fio.Write(&dwID, sizeof(dwID))) {
		fio.Close();
		return FALSE;
	}

	// �I��
	fio.Close();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�R���|�[�l���g�����[�h
//	���X�P�W���[���͒�~���Ă��邪�ACSound,CInput�͓��쒆
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::LoadComponent(const Filepath& path, DWORD dwPos)
{
	Fileio fio;
	DWORD dwID;
	char cHeader[0x10];
	int nVer;

	ASSERT(this);
	ASSERT(dwPos > 0);

	// �t�@�C���I�[�v��
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}

	// �w�b�_�ǂݎ��
	if (!fio.Read(cHeader, sizeof(cHeader))) {
		fio.Close();
		return FALSE;
	}

	// �w�b�_�`�F�b�N�A�o�[�W�������ǂݎ��
	cHeader[0x0a] = '\0';
	nVer = ::strtoul(&cHeader[0x09], NULL, 16);
	nVer <<= 8;
	cHeader[0x0d] = '\0';
	nVer |= ::strtoul(&cHeader[0x0b], NULL, 16);
	cHeader[0x09] = '\0';
	if (strcmp(cHeader, "XM6 DATA ") != 0) {
		fio.Close();
		return FALSE;
	}

	// �V�[�N
	if (!fio.Seek(dwPos)) {
		fio.Close();
		return FALSE;
	}

	// ���C���R���|�[�l���g�ǂݎ��
	if (!fio.Read(&dwID, sizeof(dwID))) {
		fio.Close();
		return FALSE;
	}
	if (dwID != MAKEID('M', 'A', 'I', 'N')) {
		fio.Close();
		return FALSE;
	}

	// �R���|�[�l���g���[�v
	for (;;) {
		// ID�ǂݎ��
		if (!fio.Read(&dwID, sizeof(dwID))) {
			fio.Close();
			return FALSE;
		}

		// �I���`�F�b�N
		if (dwID == MAKEID('E', 'N', 'D', ' ')) {
			break;
		}

		fio.Close();
		return FALSE;
	}

	// �N���[�Y
	fio.Close();

	// �ݒ�K�p(VM���b�N���čs��)
//	if (GetConfig()->IsApply())
	{
		::LockVM();
		ApplyCfg();
		::UnlockVM();
	}

	// �E�B���h�E�ĕ`��
	GetView()->Invalidate(FALSE);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::ApplyCfg()
{
	Config config;

	// �ݒ�擾
	configGetConfig(&config);

	// �܂�VM�ɓK�p
	::GetVM()->ApplyCfg(&config);

	// ���ɃR���|�[�l���g�ɓK�p
	// ���Ƀr���[�ɓK�p
	GetView()->ApplyCfg(&config);

	// �t���[���E�B���h�E(�|�b�v�A�b�v)
	if (config.popup_swnd != m_bPopup) {
//		// �T�u�E�B���h�E�����ׂăN���A
//		GetView()->ClrSWnd();

		// �ύX
		m_bPopup = config.popup_swnd;
	}

	// �t���[���E�B���h�E(�}�E�X)
	m_bMouseMid = config.mouse_mid;
	m_bAutoMouse = config.auto_mouse;
	if (config.mouse_port == 0) {
		// �}�E�X�ڑ��Ȃ��Ȃ�A�}�E�X���[�hOFF
//		if (GetInput()->GetMouseMode()) {
//			OnMouseMode();
//		}
	}
}

//---------------------------------------------------------------------------
//
//	�L�b�N
//
//---------------------------------------------------------------------------
LONG CFrmWnd::OnKick(UINT , LONG )
{
//	CInfo *pInfo;
	Config config;
	CString strMsg;
	MSG msg;
	Memory *pMemory;
	int nIdle;
	LPSTR lpszCmd;
	LPCTSTR lpszCommand;
	BOOL bFullScreen;

	// �G���[�������ɍs��
	switch (m_nStatus) {
		// VM�G���[
		case 1:
			::GetMsg(IDS_INIT_VMERR, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
			PostMessage(WM_CLOSE, 0, 0);
			return 0;

		// �R���|�[�l���g�G���[
		case 2:
			::GetMsg(IDS_INIT_COMERR, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
			PostMessage(WM_CLOSE, 0, 0);
			return 0;
	}
	// ����̏ꍇ
	ASSERT(m_nStatus == 0);

	// ROM�`�F�b�N
	pMemory = (Memory*)::GetVM()->SearchDevice(MAKEID('M', 'E', 'M', ' '));
/*
	ASSERT(pMemory);
	if (!pMemory->CheckIPL()) {
		::GetMsg(IDS_INIT_IPLERR, strMsg);
		if (MessageBox(strMsg, NULL, MB_ICONSTOP | MB_YESNO | MB_DEFBUTTON2) != IDYES) {
			PostMessage(WM_CLOSE, 0, 0);
			return 0;
		}
	}
	if (!pMemory->CheckCG()) {
		::GetMsg(IDS_INIT_CGERR, strMsg);
		if (MessageBox(strMsg, NULL, MB_ICONSTOP | MB_YESNO | MB_DEFBUTTON2) != IDYES) {
			PostMessage(WM_CLOSE, 0, 0);
			return 0;
		}
	}
*/
	// �ݒ�擾(power_off�ݒ�̂���)
	configGetConfig(&config);
	if (config.power_off) {
		// �d��OFF�ŋN��
		::GetVM()->SetPower(FALSE);
		::GetVM()->PowerSW(FALSE);
	}

	// �T�u�E�B���h�E�̏���
	m_strWndClsName = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS);

	// �R���|�[�l���g���C�l�[�u���B������Scheduler�͐ݒ�ɂ��
	GetView()->Enable(TRUE);
	schedulerSetEnable(TRUE);

	// ���Z�b�g(�X�e�[�^�X�o�[�̂���)
	if (!config.power_off) {
		OnReset();
	}

	// �R�}���h���C������
	lpszCmd = AfxGetApp()->m_lpCmdLine;
	lpszCommand = A2T(lpszCmd);
	if (_tcslen(lpszCommand) > 0) {
		InitCmd(lpszCommand);
	}

	// �ő剻�w��ł���΁A�߂�����ɁA�t���X�N���[��
	bFullScreen = FALSE;
	if (IsZoomed()) {
		ShowWindow(SW_RESTORE);
		bFullScreen = TRUE;
	}

	// �E�C���h�E�ʒu�����W���[��
	bFullScreen = RestoreFrameWnd(bFullScreen);
	if (bFullScreen) {
		// �ő剻�w�肩�A�O����s���Ƀt���X�N���[��
		PostMessage(WM_COMMAND, IDM_FULLSCREEN);
	}

	// �f�B�X�N�E�X�e�[�g�����W���[��
	RestoreDiskState();

	// �������[�v
	nIdle = 0;
	while (!m_bExit) {
		// ���b�Z�[�W�`�F�b�N���|���v
		if (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			if (!AfxGetApp()->PumpMessage()) {
				::PostQuitMessage(0);
				return 0;
			}
			// continue���邱�ƂŁAWM_DESTROY�����m_bExit�`�F�b�N��ۏ�
			continue;
		}

		// �X���[�v
		if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			Sleep(20);

			// �X�V�J�E���^Up
			nIdle++;

			// �X�e�[�^�X�E���s��20ms
			UpdateExec();

			if ((nIdle & 1) == 0) {
				// �r���[��40ms
				GetView()->Update();
			}
		}
	}

	return 0;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�N���[�Y
//
//---------------------------------------------------------------------------
void CFrmWnd::OnClose()
{
	CString strFormat;
	CString strText;
	Filepath path;

	ASSERT(this);
	ASSERT(!m_bSaved);

	// �L���ȃX�e�[�g�t�@�C��������΁A�Z�[�u��₤
	::LockVM();
	::GetVM()->GetPath(path);
	::UnlockVM();

/*
	// �L���ȃX�e�[�g�t�@�C����������
	if (!path.IsClear()) {
		// Windows�T�C�h��20ms�ȏ�̎��s���т������
		if (m_dwExec >= 2) {
			// �m�F
			::GetMsg(IDS_SAVECLOSE, strFormat);
			strText.Format(strFormat, path.GetFileExt());
			int nResult = MessageBox(strText, NULL, MB_ICONQUESTION | MB_YESNOCANCEL);

			// �m�F���ʂɂ��
			switch (nResult) {
				// YES
				case IDYES:
					// �ۑ�
					OnSaveSub(path);
					break;

				// NO
				case IDNO:
					// �p�X���N���A(�X�e�[�g�Ȃ�)
					::GetVM()->Clear();
					break;

				// �L�����Z��
				case IDCANCEL:
					// �N���[�Y����Ȃ��������Ƃɂ���
					return;
			}
		}
	}
*/
	// �������ς݂Ȃ�
	if ((m_nStatus == 0) && !m_bSaved) {
		// �E�B���h�E��ԁE�f�B�X�N�E�X�e�[�g��ۑ�
		SaveFrameWnd();
		SaveDiskState();
		m_bSaved = TRUE;
	}

	// ��{�N���X
	CFrameWnd::OnClose();
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�폜
//
//---------------------------------------------------------------------------
void CFrmWnd::OnDestroy()
{
	ASSERT(this);

	// �������ς݂Ȃ�
	if ((m_nStatus == 0) && !m_bSaved) {
		// �E�B���h�E��ԁE�f�B�X�N�E�X�e�[�g��ۑ�
		SaveFrameWnd();
		SaveDiskState();
		m_bSaved = TRUE;
	}

	// �N���[���A�b�v(WM_ENDSESSION�Ƌ���)
	CleanSub();

	// ��{�N���X��
	CFrameWnd::OnDestroy();
}

//---------------------------------------------------------------------------
//
//	�Z�b�V�����I��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnEndSession(BOOL bEnding)
{
	ASSERT(this);

	// �I�����́A�N���[���A�b�v���s��
	if (bEnding) {
		// �������ς݂Ȃ�
		if (m_nStatus == 0) {
			// �E�B���h�E��ԁE�f�B�X�N�E�X�e�[�g��ۑ�
			if (!m_bSaved) {
				SaveFrameWnd();
				SaveDiskState();
				m_bSaved = TRUE;
			}

			// �N���[���A�b�v
			CleanSub();
		}
	}

	// ��{�N���X
	CFrameWnd::OnEndSession(bEnding);
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v����
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::CleanSub()
{
	int i;

	// �I���t���O���グ��
	m_bExit = TRUE;

	// �R���|�[�l���g���~�߂�
	GetView()->Enable(FALSE);
	schedulerSetEnable(FALSE);

	// �X�P�W���[�������s����߂�܂ő҂�
	for (i=0; i<8; i++) {
		::LockVM();
		::UnlockVM();
	}

	// ���z�}�V�����폜
	if (::pVM) {
		::LockVM();
		::GetVM()->Cleanup();
		delete ::pVM;
		::pVM = NULL;
		::UnlockVM();
	}

	// �V�F���ʒm���폜
	if (m_uNotifyId) {
		 VERIFY(::SHChangeNotifyDeregister(m_uNotifyId));
		 m_uNotifyId = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E��Ԃ�ۑ�
//
//---------------------------------------------------------------------------
void CFrmWnd::SaveFrameWnd()
{
	CRect rectWnd;
	Config config;

	ASSERT(this);
	ASSERT_VALID(this);

	// �ݒ�擾
	configGetConfig(&config);

	// �L���v�V�����E���j���[�E�X�e�[�^�X�o�[
	config.menu_bar = m_bMenuBar;

	// �E�B���h�E��`
	if (m_bFullScreen) {
		// �t���X�N���[�����́A�E�B���h�E���̈ʒu��ۑ�����
		config.window_left = m_nWndLeft;
		config.window_top = m_nWndTop;
	}
	else {
		// �E�B���h�E���́A���݂̈ʒu��ۑ�����
		GetWindowRect(&rectWnd);
		config.window_left = rectWnd.left;
		config.window_top = rectWnd.top;
	}

	// �t���X�N���[��
	config.window_full = m_bFullScreen;

	// �ݒ�ύX
//	GetConfig()->SetConfig(&config);
}

//---------------------------------------------------------------------------
//
//	�f�B�X�N�E�X�e�[�g��ۑ�
//
//---------------------------------------------------------------------------
void CFrmWnd::SaveDiskState()
{
	int nDrive;
	Filepath path;
	Config config;

	ASSERT(this);
	ASSERT_VALID(this);

	// ���b�N
	::LockVM();

	// �ݒ�擾
	configGetConfig(&config);

	// �t���b�s�[�f�B�X�N
	for (nDrive=0; nDrive<2; nDrive++) {
		// ���f�B
		config.resume_fdi[nDrive] = m_pFDD->IsReady(nDrive, FALSE);

		// ���f�B�łȂ���΁A����
		if (!config.resume_fdi[nDrive]) {
			continue;
		}

		// ���f�B�A
		config.resume_fdm[nDrive]  = m_pFDD->GetMedia(nDrive);

		// ���C�g�v���e�N�g
		config.resume_fdw[nDrive] = m_pFDD->IsWriteP(nDrive);
	}

	// MO�f�B�X�N
	config.resume_mos = m_pSASI->IsReady();
	if (config.resume_mos) {
		config.resume_mow = m_pSASI->IsWriteP();
	}

	// CD-ROM
	config.resume_iso = m_pSCSI->IsReady(FALSE);

	// �X�e�[�g
	::GetVM()->GetPath(path);
	config.resume_xm6 = !path.IsClear();

	// �f�t�H���g�f�B���N�g��
	_tcscpy(config.resume_path, Filepath::GetDefaultDir());

	// �ݒ�ύX
//	GetConfig()->SetConfig(&config);

	// �A�����b�N
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E��Ԃ𕜌�
//	��OnCreate��OnKick�ƁA2��Ă΂��
//
//---------------------------------------------------------------------------
BOOL CFrmWnd::RestoreFrameWnd(BOOL bFullScreen)
{
	int nWidth;
	int nHeight;
	int nLeft;
	int nTop;
	CRect rectWnd;
	BOOL bValid;
	Config config;

	ASSERT(this);

	// �ݒ�擾
	configGetConfig(&config);

	// �E�B���h�E�ʒu�̕������w�肳��Ă��Ȃ���΁A�f�t�H���g��Ԃœ���
	if (!config.resume_screen) {
		return bFullScreen;
	}

	// ���j���[
	m_bMenuBar = config.menu_bar;
	ShowMenu();

	// ���z��ʂ̃T�C�Y�ƌ��_���擾
	nWidth = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
	nHeight = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
	nLeft = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
	nTop = ::GetSystemMetrics(SM_YVIRTUALSCREEN);

	// �E�B���h�E��`���擾
	GetWindowRect(&rectWnd);

	// ��̓͂��Ƃ���ɂ���΁A�E�B���h�E�ʒu���ړ�����B�܂��̓`�F�b�N
	bValid = TRUE;
	if (config.window_left < nLeft) {
		if (config.window_left < nLeft - rectWnd.Width()) {
			bValid = FALSE;
		}
	}
	else {
		if (config.window_left >= (nLeft + nWidth)) {
			bValid = FALSE;
		}
	}
	if (config.window_top < nTop) {
		if (config.window_top < nTop - rectWnd.Height()) {
			bValid = FALSE;
		}
	}
	else {
		if (config.window_top >= (nTop + nHeight)) {
			bValid = FALSE;
		}
	}

	// �E�B���h�E�ʒu���ړ�
	if (bValid) {
		SetWindowPos(&wndTop, config.window_left, config.window_top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		// ���[�N�G���A�������ɕύX����
		m_nWndLeft = config.window_left;
		m_nWndTop = config.window_top;
	}

	// VM���������Ȃ�A�����܂�
	if (m_nStatus != 0) {
		return FALSE;
	}

	// �t���X�N���[����
	if (bFullScreen || config.window_full) {
		// �ő剻�N�����A�O��t���X�N���[��������
		return TRUE;
	}
	else {
		// �ő剻�N���łȂ��A���A�O��ʏ�\��������
		return FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	�f�B�X�N�E�X�e�[�g�𕜌�
//
//---------------------------------------------------------------------------
void CFrmWnd::RestoreDiskState()
{
	int nDrive;
//	TCHAR szMRU[_MAX_PATH];
	BOOL bResult;
	Filepath path;
	Config config;

	ASSERT(this);

	// �ݒ�擾
	configGetConfig(&config);

/*
	// �X�e�[�g���w�肳��Ă���΁A������ɍs��
	if (config.resume_state) {
		// �X�e�[�g��������
		if (config.resume_xm6) {
			// �p�X�擾
//			GetConfig()->GetMRUFile(4, 0, szMRU);
//			path.SetPath(szMRU);

			// �I�[�v���O����
			if (OnOpenPrep(path)) {
				// �I�[�v���T�u
				if (OnOpenSub(path)) {
					// �����Ȃ̂ŁA�f�t�H���g�f�B���N�g����������
					if (config.resume_dir) {
						Filepath::SetDefaultDir(config.resume_path);
					}

					// ����ȍ~�͏������Ȃ�(FD, MO, CD�̃A�N�Z�X���ɃZ�[�u�����ꍇ)
					return;
				}
			}
		}
	}
*/
	// �t���b�s�[�f�B�X�N
	if (config.resume_fd) {
		for (nDrive=0; nDrive<2; nDrive++) {
			// �f�B�X�N�}������Ă�����
			if (!config.resume_fdi[nDrive]) {
				// �f�B�X�N�}������Ă��Ȃ��B�X�L�b�v
				continue;
			}

			// �f�B�X�N�}��
//			GetConfig()->GetMRUFile(nDrive, 0, szMRU);
//			ASSERT(szMRU[0] != _T('\0'));
//			path.SetPath(szMRU);

			// VM���b�N���s���A�f�B�X�N���蓖�Ă����݂�
			::LockVM();
			bResult = m_pFDD->Open(nDrive, path, config.resume_fdm[nDrive]);
			::UnlockVM();

			// ���蓖�Ăł��Ȃ���΃X�L�b�v
			if (!bResult) {
				continue;
			}

			// �������݋֎~
			if (config.resume_fdw[nDrive]) {
				::LockVM();
				m_pFDD->WriteP(nDrive, TRUE);
				::UnlockVM();
			}
		}
	}

	// MO�f�B�X�N
	if (config.resume_mo) {
		// �f�B�X�N�}������Ă�����
		if (config.resume_mos) {
			// �f�B�X�N�}��
//			GetConfig()->GetMRUFile(2, 0, szMRU);
//			ASSERT(szMRU[0] != _T('\0'));
//			path.SetPath(szMRU);

			// VM���b�N���s���A�f�B�X�N���蓖�Ă����݂�
			::LockVM();
			bResult = m_pSASI->Open(path);
			::UnlockVM();

			// ���蓖�Ăł����
			if (bResult) {
				// �������݋֎~
				if (config.resume_mow) {
					::LockVM();
					m_pSASI->WriteP(TRUE);
					::UnlockVM();
				}
			}
		}
	}

	// CD-ROM
	if (config.resume_cd) {
		// �f�B�X�N�}������Ă�����
		if (config.resume_iso) {
			// �f�B�X�N�}��
//			GetConfig()->GetMRUFile(3, 0, szMRU);
//			ASSERT(szMRU[0] != _T('\0'));
//			path.SetPath(szMRU);

			// VM���b�N���s���A�f�B�X�N���蓖�Ă����݂�
			::LockVM();
			m_pSCSI->Open(path, FALSE);
			::UnlockVM();
		}
	}

	// �f�t�H���g�f�B���N�g��
	if (config.resume_dir) {
		Filepath::SetDefaultDir(config.resume_path);
	}
}

//---------------------------------------------------------------------------
//
//	�f�B�X�v���C�ύX
//
//---------------------------------------------------------------------------
LRESULT CFrmWnd::OnDisplayChange(UINT uParam, LONG lParam)
{
	LRESULT lResult;

	// ��{�N���X
	lResult = CFrameWnd::OnDisplayChange(uParam, lParam);

	// �ŏ����͉������Ȃ�
	if (IsIconic()) {
		return lResult;
	}

	// �|�W�V�����ݒ�
	InitPos(FALSE);

	return lResult;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�w�i�`��
//
//---------------------------------------------------------------------------
BOOL CFrmWnd::OnEraseBkgnd(CDC * )
{
	// �w�i�`���}��
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�`��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPaint()
{
	// �K�����b�N���čs��
	::LockVM();

	PAINTSTRUCT ps;
	BeginPaint(&ps);
	EndPaint(&ps);

	// �A�����b�N
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�ړ�
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMove(int x, int y)
{
	// ��{�N���X
	CFrameWnd::OnMove(x, y);
}

//---------------------------------------------------------------------------
//
//	�A�N�e�B�x�[�g
//
//---------------------------------------------------------------------------
void CFrmWnd::OnActivate(UINT nState, CWnd *pWnd, BOOL bMinimized)
{
//	CInput *pInput;
//	CScheduler *pScheduler;

	// �������ς݂Ȃ�
	if (m_nStatus == 0) {
		// �C���v�b�g�A�X�P�W���[���֒ʒm
//		pInput = GetInput();
//		pScheduler = GetScheduler();
//		if (pScheduler) {
//			// WA_INACTIVE���ŏ����Ȃ�A�f�B�Z�[�u��
//			if ((nState == WA_INACTIVE) || bMinimized) {
//				// ���͎󂯕t���Ȃ��A�ᑬ���s
//				pScheduler->Activate(FALSE);
//			}
//			else {
//				// ���͎󂯕t����A�ʏ���s
//				pScheduler->Activate(TRUE);
//			}
//		}
	}

	// ��{�N���X��
	CFrameWnd::OnActivate(nState, pWnd, bMinimized);
}

//---------------------------------------------------------------------------
//
//	�A�N�e�B�x�[�g�A�v���P�[�V����
//
//---------------------------------------------------------------------------
#if _MFC_VER >= 0x700
void CFrmWnd::OnActivateApp(BOOL bActive, DWORD dwThreadID)
#else
void CFrmWnd::OnActivateApp(BOOL bActive, HTASK hTask)
#endif
{
	// �������ς݂Ȃ�
	if (m_nStatus == 0) {
		// �t���X�N���[����p
		if (m_bFullScreen) {
			if (bActive) {
				// ���ꂩ��A�N�e�B�u�ɂȂ�
				HideTaskBar(TRUE, TRUE);
			}
			else {
				// �A�N�e�B�u����O�ꂽ
				HideTaskBar(FALSE, FALSE);
			}
		}
	}

	// ��{�N���X
#if _MFC_VER >= 0x700
	CFrameWnd::OnActivateApp(bActive, dwThreadID);
#else
	CFrameWnd::OnActivateApp(bActive, hTask);
#endif
}

//---------------------------------------------------------------------------
//
//	���j���[���[�v�J�n
//
//---------------------------------------------------------------------------
void CFrmWnd::OnEnterMenuLoop(BOOL bTrackPopup)
{
	::LockVM();

//	// �X�P�W���[���֒ʒm
//	CScheduler *pScheduler = GetScheduler();
//	if (pScheduler) {
//		pScheduler->Menu(TRUE);
//	}

	::UnlockVM();

	// ��{�N���X��
	CFrameWnd::OnEnterMenuLoop(bTrackPopup);
}

//---------------------------------------------------------------------------
//
//	���j���[���[�v�I��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnExitMenuLoop(BOOL bTrackPopup)
{
	::LockVM();

//	// �X�P�W���[���֒ʒm
//	CScheduler *pScheduler = GetScheduler();
//	if (pScheduler) {
//		pScheduler->Menu(FALSE);
//	}

	::UnlockVM();

	// ��{�N���X��
	CFrameWnd::OnExitMenuLoop(bTrackPopup);
}

//---------------------------------------------------------------------------
//
//	�e�E�B���h�E�ʒm
//
//---------------------------------------------------------------------------
void CFrmWnd::OnParentNotify(UINT message, LPARAM lParam)
{
	// ��{�N���X��
	CFrameWnd::OnParentNotify(message, lParam);
}

//---------------------------------------------------------------------------
//
//	�R���e�L�X�g���j���[
//
//---------------------------------------------------------------------------
void CFrmWnd::OnContextMenu(CWnd * , CPoint pos)
{
	// �|�b�v�A�b�v���j���[
	m_bPopupMenu = TRUE;

	CMenu *pMenu = m_PopupMenu.GetSubMenu(0);
	pMenu->TrackPopupMenu(TPM_CENTERALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
							pos.x, pos.y, this, 0);
	m_bPopupMenu = FALSE;
}

//---------------------------------------------------------------------------
//
//	�d�͕ύX�ʒm
//
//---------------------------------------------------------------------------
LONG CFrmWnd::OnPowerBroadCast(UINT , LONG )
{
	// �������ς݂Ȃ�
	if (m_nStatus == 0) {
		// VM���b�N�A���ԍĐݒ�
		::LockVM();
		timeEndPeriod(1);
		timeBeginPeriod(1);
		::UnlockVM();
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�V�X�e���R�}���h
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSysCommand(UINT nID, LPARAM lParam)
{
	// �W���E�B���h�E�ʒu���T�|�[�g
	if ((nID & 0xfff0) == IDM_STDWIN) {
		InitPos(TRUE);
		return;
	}

	// �ő剻�̓t���X�N���[��
	if ((nID & 0xfff0) == SC_MAXIMIZE) {
		if (!m_bFullScreen) {
			PostMessage(WM_COMMAND, IDM_FULLSCREEN);
		}
		return;
	}

	// ��{�N���X
	CFrameWnd::OnSysCommand(nID, lParam);
}

//---------------------------------------------------------------------------
//
//	�f�[�^�]��
//
//---------------------------------------------------------------------------
#if _MFC_VER >= 0x700
afx_msg BOOL CFrmWnd::OnCopyData(CWnd* , COPYDATASTRUCT* pCopyDataStruct)
#else
LONG CFrmWnd::OnCopyData(UINT , LONG pCopyDataStruct)
#endif
{
	PCOPYDATASTRUCT pCDS;

	// �p�����[�^�󂯎��
	pCDS = (PCOPYDATASTRUCT)pCopyDataStruct;

	// �R�}���h���C��������
	InitCmd((LPSTR)pCDS->lpData);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�V�F���ʒm
//
//---------------------------------------------------------------------------
LRESULT CFrmWnd::OnShellNotify(UINT uParam, LONG lParam)
{
	HANDLE hMemoryMap;
	DWORD dwProcessId;
	LPITEMIDLIST *pidls;
	HANDLE hLock;
	LONG nEvent;
//	TCHAR szPath[_MAX_PATH];

	// Windows NT��
	if (::IsWinNT()) {
		// Windows2000/XP�̏ꍇ�ASHChangeNotification_Lock�Ń��b�N����
		hMemoryMap = (HANDLE)uParam;
		dwProcessId = (DWORD)lParam;
		hLock = ::SHChangeNotification_Lock(hMemoryMap, dwProcessId, &pidls, &nEvent);
		if (hLock == NULL) {
			return 0;
		}
	}
	else {
		// Windows9x�̏ꍇ�Apidls��nEvent��uParam,lParam���璼�ړ���
		pidls = (LPITEMIDLIST*)uParam;
		nEvent = lParam;
		hLock = NULL;
	}

	// ���s���ŁACHost������΁A�ʒm
	if (m_nStatus == 0) {
//		CHost *pHost = GetHost();

#if 1
		// Windrv���܂��s����̂��߁A���ۂ�Enable�ɂ���Ă��Ȃ��ꍇ�͉������Ȃ�(version2.04)
		{
			Config config;
			configGetConfig(&config);
			if ((config.windrv_enable <= 0) || (config.windrv_enable > 3)) {
//				pHost = NULL;
			}
		}
#endif

//		if (pHost) {
//			// �p�X�擾
//			::SHGetPathFromIDList(pidls[0], szPath);
//			// �ʒm
//			pHost->ShellNotify(nEvent, szPath);
//		}
	}

	// NT�̏ꍇ�ASHCnangeNotifcation_Unlock�ŃA�����b�N����
	if (::IsWinNT()) {
		ASSERT(hLock);
		::SHChangeNotification_Unlock(hLock);
	}

	return 0;
}

//---------------------------------------------------------------------------
//
//	�X�V(���s)
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::UpdateExec()
{
	ASSERT(this);
	ASSERT_VALID(this);

	// �X�P�W���[�����L���Ȃ�A���s�J�E���^���グ��(�Z�[�u���ɃN���A�����)
	if (schedulerIsEnable()) {
		m_dwExec++;
		if (m_dwExec == 0) {
			m_dwExec--;
		}
	}
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�������
//
//---------------------------------------------------------------------------
void CFrmWnd::GetMessageString(UINT nID, CString& rMessage) const
{
	Filepath path;
//	TCHAR szPath[_MAX_PATH];
	TCHAR szName[60];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	TCHAR szFile[_MAX_FNAME];
	TCHAR szExt[_MAX_EXT];
	int nMRU;
	int nDisk;
	BOOL bValid;
//	CInfo *pInfo;

	// �t���OFALSE
	bValid = FALSE;

	// ���j���[��������ɍs��(�p���+MRU���l��)
	if ((nID >= IDM_OPEN) && (nID <= IDM_ABOUT)) {
		// �p�����
		if (!::IsJapanese()) {
			// +5000�Ŏ���
			if (rMessage.LoadString(nID + 5000)) {
				bValid = TRUE;
			}
		}
	}

	// ���j���[���������(IDM_STDWIN)
	if (nID == IDM_STDWIN) {
		// �p�����
		if (!::IsJapanese()) {
			// +5000�Ŏ���
			if (rMessage.LoadString(nID + 5000)) {
				bValid = TRUE;
			}
		}
	}

	// MRU0
	if ((nID >= IDM_D0_MRU0) && (nID <= IDM_D0_MRU8)) {
		nMRU = nID - IDM_D0_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
//		GetConfig()->GetMRUFile(0, nMRU, szPath);
//		szPath[60] = _T('\0');
//		rMessage = szPath;
//		bValid = TRUE;
	}

	// MRU1
	if ((nID >= IDM_D1_MRU0) && (nID <= IDM_D1_MRU8)) {
		nMRU = nID - IDM_D1_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
//		GetConfig()->GetMRUFile(1, nMRU, szPath);
//		szPath[60] = _T('\0');
//		rMessage = szPath;
//		bValid = TRUE;
	}

	// MRU2
	if ((nID >= IDM_MO_MRU0) && (nID <= IDM_MO_MRU8)) {
		nMRU = nID - IDM_MO_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
//		GetConfig()->GetMRUFile(2, nMRU, szPath);
//		szPath[60] = _T('\0');
//		rMessage = szPath;
//		bValid = TRUE;
	}

	// MRU3
	if ((nID >= IDM_CD_MRU0) && (nID <= IDM_CD_MRU8)) {
		nMRU = nID - IDM_CD_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
//		GetConfig()->GetMRUFile(3, nMRU, szPath);
//		szPath[60] = _T('\0');
//		rMessage = szPath;
//		bValid = TRUE;
	}

	// MRU4
	if ((nID >= IDM_XM6_MRU0) && (nID <= IDM_XM6_MRU8)) {
		nMRU = nID - IDM_XM6_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
//		GetConfig()->GetMRUFile(4, nMRU, szPath);
//		szPath[60] = _T('\0');
//		rMessage = szPath;
//		bValid = TRUE;
	}

	// �f�B�X�N��0
	if ((nID >= IDM_D0_MEDIA0) && (nID <= IDM_D0_MEDIAF)) {
		nDisk = nID - IDM_D0_MEDIA0;
		ASSERT((nDisk >= 0) && (nDisk <= 15));
		::LockVM();
		m_pFDD->GetName(0, szName, nDisk);
		m_pFDD->GetPath(0, path);
		::UnlockVM();
		_tsplitpath(path.GetPath(), szDrive, szDir, szFile, szExt);
		rMessage = szName;
		rMessage += _T(" (");
		rMessage += szFile;
		rMessage += szExt;
		rMessage += _T(")");
		bValid = TRUE;
	}

	// �f�B�X�N��1
	if ((nID >= IDM_D1_MEDIA0) && (nID <= IDM_D1_MEDIAF)) {
		nDisk = nID - IDM_D1_MEDIA0;
		ASSERT((nDisk >= 0) && (nDisk <= 15));
		::LockVM();
		m_pFDD->GetName(1, szName, nDisk);
		m_pFDD->GetPath(1, path);
		::UnlockVM();
		_tsplitpath(path.GetPath(), szDrive, szDir, szFile, szExt);
		rMessage = szName;
		rMessage += _T(" (");
		rMessage += szFile;
		rMessage += szExt;
		rMessage += _T(")");
		bValid = TRUE;
	}

	// �����܂łŊm�肵�Ă��Ȃ���΁A��{�N���X
	if (!bValid) {
		CFrameWnd::GetMessageString(nID, rMessage);
	}
}

//---------------------------------------------------------------------------
//
//	�^�X�N�o�[�B��
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::HideTaskBar(BOOL bHide, BOOL bFore)
{
	if (bHide) {
		// "��ɑO��"
		m_hTaskBar = ::FindWindow(_T("Shell_TrayWnd"), NULL);
		if (m_hTaskBar) {
			::ShowWindow(m_hTaskBar, SW_HIDE);
		}
		ModifyStyleEx(0, WS_EX_TOPMOST, 0);
	}
	else {
		// "�ʏ�"
		ModifyStyleEx(WS_EX_TOPMOST, 0, 0);
		if (m_hTaskBar) {
			::ShowWindow(m_hTaskBar, SW_SHOWNA);
		}
	}

	// �O�ʃI�v�V�����������
	if (bFore) {
		SetForegroundWindow();
	}
}

//---------------------------------------------------------------------------
//
//	�I�[�i�[�h���[
//
//---------------------------------------------------------------------------
void CFrmWnd::OnDrawItem(int nID, LPDRAWITEMSTRUCT lpDIS)
{
}

//---------------------------------------------------------------------------
//
//	���j���[�o�[�\��
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::ShowMenu()
{
	HMENU hMenu;

	ASSERT(this);

	// �K�v�ł����VM�����b�N
	if (m_nStatus == 0) {
		::LockVM();
	}

	// ���݂̃��j���[���擾
	hMenu = ::GetMenu(m_hWnd);

	// ���j���[���s�K�v�ȏꍇ
	if (m_bFullScreen || !m_bMenuBar) {
		// ���j���[�����݂��邩
		if (hMenu != NULL) {
			// ���j���[������
			SetMenu(NULL);
		}
		if (m_nStatus == 0) {
			::UnlockVM();
		}
		return;
	}

	// ���j���[���K�v�ȏꍇ
	if (hMenu != NULL) {
		// �Z�b�g���������j���[�Ɠ�����
		if (m_Menu.GetSafeHmenu() == hMenu) {
			// �ύX�̕K�v�͂Ȃ�
			if (m_nStatus == 0) {
				::UnlockVM();
			}
			return;
		}
	}

	// ���j���[���Z�b�g
	SetMenu(&m_Menu);

	// �K�v�Ȃ�VM���A�����b�N
	if (m_nStatus == 0) {
		::UnlockVM();
	}
}

//---------------------------------------------------------------------------
//
//	�`��r���[�擾
//
//---------------------------------------------------------------------------
CDrawView* FASTCALL CFrmWnd::GetView() const
{
	ASSERT(this);
	ASSERT(m_pDrawView);
	ASSERT(m_pDrawView->m_hWnd);
	return m_pDrawView;
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void CFrmWnd::OnReset()
{
	SRAM *pSRAM;
	DWORD Sw[0x100];
	DWORD dwDevice;
	DWORD dwAddr;
	CString strReset;
	CString strSub;
	BOOL bFlag;
	int i;

	// �d��OFF�Ȃ瑀��s��
	if (!::GetVM()->IsPower()) {
		return;
	}

	::LockVM();

	// ���Z�b�g���ĕ`��
	::GetVM()->Reset();
	GetView()->Refresh();
//	ResetCaption();

	// �������X�C�b�`�擾���s��
	pSRAM = (SRAM*)::GetVM()->SearchDevice(MAKEID('S', 'R', 'A', 'M'));
	ASSERT(pSRAM);
	for (i=0; i<0x100; i++) {
		Sw[i] = pSRAM->ReadOnly(0xed0000 + i);
	}

	::UnlockVM();

	// ���Z�b�g���b�Z�[�W�����[�h
	::GetMsg(IDS_RESET, strReset);

	// �������X�C�b�`�̐擪���r
	if (memcmp(Sw, SigTable, sizeof(DWORD) * 7) != 0) {
//		SetInfo(strReset);
		return;
	}

	// �u�[�g�f�o�C�X���擾
	dwDevice = Sw[0x18];
	dwDevice <<= 8;
	dwDevice |= Sw[0x19];

	// �u�[�g�f�o�C�X����
	bFlag = FALSE;
	if (dwDevice == 0x0000) {
		// STD
		strSub = _T("STD)");
		bFlag = TRUE;
	}
	if (dwDevice == 0xa000) {
		// ROM
		dwAddr = Sw[0x0c];
		dwAddr = (dwAddr << 8) | Sw[0x0d];
		dwAddr = (dwAddr << 8) | Sw[0x0e];
		dwAddr = (dwAddr << 8) | Sw[0x0f];

		// FC0000�`FC001C�ƁAEA0020�`EA003C��SCSI#
		strSub.Format(_T("ROM $%06X)"), dwAddr);
		if ((dwAddr >= 0xfc0000) && (dwAddr < 0xfc0020)) {
			strSub.Format(_T("SCSI%1d)"), (dwAddr & 0x001f) >> 2);
		}
		if ((dwAddr >= 0xea0020) && (dwAddr < 0xea0040)) {
			strSub.Format(_T("SCSI%1d)"), (dwAddr & 0x001f) >> 2);
		}
		bFlag = TRUE;
	}
	if (dwDevice == 0xb000) {
		// RAM
		dwAddr = Sw[0x10];
		dwAddr = (dwAddr << 8) | Sw[0x11];
		dwAddr = (dwAddr << 8) | Sw[0x12];
		dwAddr = (dwAddr << 8) | Sw[0x13];
		strSub.Format(_T("RAM $%06X)"), dwAddr);
		bFlag = TRUE;
	}
	if ((dwDevice & 0xf0ff) == 0x9070) {
		strSub.Format(_T("2HD%1d)"), (dwDevice & 0xf00) >> 8);
		bFlag = TRUE;
	}
	if ((dwDevice & 0xf0ff) == 0x8000) {
		strSub.Format(_T("HD%1d)"), (dwDevice & 0xf00) >> 8);
		bFlag = TRUE;
	}
	if (!bFlag) {
		strSub = _T("Unknown)");
	}

	// �\��
	strReset += _T(" (");
	strReset += strSub;
//	SetInfo(strReset);
}

//---------------------------------------------------------------------------
//
//	���Z�b�g UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnResetUI(CCmdUI *pCmdUI)
{
	// �d��ON�Ȃ瑀��ł���
	pCmdUI->Enable(::GetVM()->IsPower());
}

//---------------------------------------------------------------------------
//
//	SRAM�V�O�l�`���e�[�u��
//
//---------------------------------------------------------------------------
const DWORD CFrmWnd::SigTable[] = {
	0x82, 0x77, 0x36, 0x38, 0x30, 0x30, 0x30
};

//---------------------------------------------------------------------------
//
//	�t���b�s�[�f�B�X�N����
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFD(UINT uID)
{
	int nDrive;

	// �h���C�u����
	nDrive = 0;
	if (uID >= IDM_D1OPEN) {
		nDrive = 1;
		uID -= (IDM_D1OPEN - IDM_D0OPEN);
	}

	switch (uID) {
		// �I�[�v��
		case IDM_D0OPEN:
			OnFDOpen(nDrive);
			break;

/*
		// �C�W�F�N�g
		case IDM_D0EJECT:
			OnFDEject(nDrive);
			break;

		// �������ݕی�
		case IDM_D0WRITEP:
			OnFDWriteP(nDrive);
			break;

		// �����C�W�F�N�g
		case IDM_D0FORCE:
			OnFDForce(nDrive);
			break;

		// ��}��
		case IDM_D0INVALID:
			OnFDInvalid(nDrive);
			break;
*/
		// ����ȊO
		default:
/*
			if (uID >= IDM_D0_MRU0) {
				// MRU
				uID -= IDM_D0_MRU0;
				ASSERT(uID <= 8);
				OnFDMRU(nDrive, (int)uID);
			}
			else {
				// Media
				uID -= IDM_D0_MEDIA0;
				ASSERT(uID <= 15);
				OnFDMedia(nDrive, (int)uID);
			}
*/
		break;
	}
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�I�[�v��
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDOpen(int nDrive)
{
	Filepath path;
	CString strMsg;
	TCHAR szPath[_MAX_PATH];
	FDI *pFDI;

	ASSERT((nDrive == 0) || (nDrive == 1));
	ASSERT(m_pFDD);

	// �R�����_�C�A���O���s
	memset(szPath, 0, sizeof(szPath));
	if (!::FileOpenDlg(this, szPath, IDS_FDOPEN)) {
//		ResetCaption();
		return;
	}
	path.SetPath(szPath);

	// VM���b�N
	::LockVM();

	// �f�B�X�N���蓖��
	if (!m_pFDD->Open(nDrive, path)) {
//		GetScheduler()->Reset();
		::UnlockVM();

		// �I�[�v���G���[
		::GetMsg(IDS_FDERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
//		ResetCaption();
		return;
	}

	// VM�����X�^�[�g������O�ɁAFDI���擾���Ă���
	pFDI = m_pFDD->GetFDI(nDrive);

	// ����
//	GetScheduler()->Reset();
//	ResetCaption();
	::UnlockVM();

	// MRU�ɒǉ�
//	GetConfig()->SetMRUFile(nDrive, szPath);

	// �����Ȃ�ABAD�C���[�W�x��
	if (pFDI->GetID() == MAKEID('B', 'A', 'D', ' ')) {
		::GetMsg(IDS_BADFDI_WARNING, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
	}
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�I�[�v�� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDOpenUI(CCmdUI *pCmdUI)
{
	CMenu *pMenu;
	CMenu *pSubMenu;
	UINT nEnable;
	int nDrive;
	int nStat;
	int nDisks;
	int nMedia;
	char szShort[_MAX_PATH];
	LPTSTR lpszShort;
	int i;
//	TCHAR szMRU[_MAX_PATH];
//	TCHAR szDrive[_MAX_DRIVE];
//	TCHAR szDir[_MAX_DIR];
//	TCHAR szFile[_MAX_FNAME];
//	TCHAR szExt[_MAX_EXT];

	ASSERT(this);
	ASSERT(m_pFDD);

	// �h���C�u����
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// �C�W�F�N�g�֎~�ŁA�f�B�X�N����ȊO�̓I�[�v���ł���
	::LockVM();
	nStat = m_pFDD->GetStatus(nDrive);
	m_nFDDStatus[nDrive] = nStat;
	nDisks = m_pFDD->GetDisks(nDrive);
	nMedia = m_pFDD->GetMedia(nDrive);
	::UnlockVM();
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		pCmdUI->Enable(FALSE);
	}
	else {
		pCmdUI->Enable(TRUE);
	}

	// �T�u���j���[�擾
	if (m_bPopupMenu) {
		pMenu = m_PopupMenu.GetSubMenu(0);
	}
	else {
		pMenu = &m_Menu;
	}
	// �t�@�C��(F)�̎��Ƀt���b�s�[0�A�t���b�s�[1�ƕ���
	pSubMenu = pMenu->GetSubMenu(nDrive + 1);

	// �C�W�F�N�gUI(�ȉ��AON_UPDATE_COMMAND_UI�̃^�C�~���O�΍�)
	if ((nStat & FDST_INSERT) && (nStat & FDST_EJECT)) {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_GRAYED);
	}

	// �������ݕی�UI
	if (m_pFDD->IsReadOnly(nDrive) || !(nStat & FDST_INSERT)) {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_GRAYED);
	}
	else {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_ENABLED);
	}

	// �����C�W�F�N�gUI
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_GRAYED);
	}

	// ��}��UI
	if (!(nStat & FDST_INSERT) && !(nStat & FDST_INVALID)) {
		pSubMenu->EnableMenuItem(5, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(5, MF_BYPOSITION | MF_GRAYED);
	}

	// �ȍ~�̃��j���[�͂��ׂč폜
	while (pSubMenu->GetMenuItemCount() > 6) {
		pSubMenu->RemoveMenu(6, MF_BYPOSITION);
	}

	// �}���`�f�B�X�N����
	if (nDisks > 1) {
		// �L���E�����萔�ݒ�
		if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
			nEnable = MF_BYCOMMAND | MF_ENABLED;
		}
		else {
			nEnable = MF_BYCOMMAND | MF_GRAYED;
		}

		// �Z�p���[�^��}��
		pSubMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);

		// ���f�B�A���[�v
		ASSERT(nDisks <= 16);
		for (i=0; i<nDisks; i++) {
			// �f�B�X�N����char*�Ŋi�[����Ă���ׁATCHAR�֕ϊ�
			m_pFDD->GetName(nDrive, szShort, i);
			lpszShort = A2T(szShort);

			// �ǉ�
			if (nDrive == 0) {
				pSubMenu->AppendMenu(MF_STRING, IDM_D0_MEDIA0 + i, lpszShort);
				pSubMenu->EnableMenuItem(IDM_D0_MEDIA0 + i, nEnable);
			}
			else {
				pSubMenu->AppendMenu(MF_STRING, IDM_D1_MEDIA0 + i, lpszShort);
				pSubMenu->EnableMenuItem(IDM_D1_MEDIA0 + i, nEnable);
			}
		}

		// ���W�I�{�^���ݒ�
		if (nDrive == 0) {
			pSubMenu->CheckMenuRadioItem(IDM_D0_MEDIA0, IDM_D0_MEDIAF,
										IDM_D0_MEDIA0 + nMedia, MF_BYCOMMAND);
		}
		else {
			pSubMenu->CheckMenuRadioItem(IDM_D1_MEDIA0, IDM_D1_MEDIAF,
										IDM_D1_MEDIA0 + nMedia, MF_BYCOMMAND);
		}
	}
}
#endif	// _WIN32
