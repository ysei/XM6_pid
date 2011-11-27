//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#if defined(_WIN32) && !defined(_AFXDLL)

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
#include "crtc.h"
#include "keyboard.h"
#include "mouse.h"
#include "ppi.h"
#include "opmif.h"
#include "opm.h"
#include "adpcm.h"
#include "scsi.h"
#include "config.h"
#include "sram.h"

#include "mfc_nomfc.h"


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
static void OnDraw(HDC hdc);
static HWND						mainWindow		= 0;

HWND getMainWindow() {
	return mainWindow;
}



//---------------------------------------------------------------------------
//
//	VM Interface
//
//---------------------------------------------------------------------------
class CVm {
public:
	CVm() : vm(0) {
	}

	~CVm() {
		cleanup();
	}

	void init() {
		if(!vm) {
			vm = new VM;
			vm->Init();
		}
	}

	void cleanup() {
		if(vm) {
			vm->Cleanup();
			delete vm;
			vm = 0;
		}
	}

	void reset() {
		if(vm) {
			if(vm->IsPower()) {
				vm->Reset();
			}
		}
	}

	Device* getDevice(unsigned int id) {
		Device* p = 0;
		if(vm) {
			p = vm->SearchDevice(id);
		}
		return p;
	}

	void applyCfg(const Config* config) {
		if(vm) {
			vm->ApplyCfg(config);
		}
	}

	int exec(unsigned int hus) {
		int r = 0;
		if(vm) {
			r = vm->Exec(hus);
		}
		return r;
	}

	int isPower() const {
		int r = 0;
		if(vm) {
			r = vm->IsPower();
		}
		return r;
	}

protected:
	VM*					vm;
};

static CVm* cvm = 0;



//---------------------------------------------------------------------------
//
//	scheduler
//
//---------------------------------------------------------------------------
static volatile BOOL		scheduler_m_bExitReq = FALSE;			// �X���b�h�I���v��

static DWORD FASTCALL GetTime() {
	return timeGetTime();
}

static void configGetConfig(Config* c) {
	memset(c, 0, sizeof(*c));

	//	Config200
	// �V�X�e��
	c->system_clock			= 5;		// �V�X�e���N���b�N(0�`5)
//	c->system_clock			= 0;		// �V�X�e���N���b�N(0�`5)
	c->ram_size				= 0;		// ���C��RAM�T�C�Y(0�`5)
	c->ram_sramsync			= TRUE;		// �������X�C�b�`�����X�V

	// �X�P�W���[��
	c->mpu_fullspeed		= FALSE;	// MPU�t���X�s�[�h
	c->vm_fullspeed			= FALSE;	// VM�t���X�s�[�h

	// �T�E���h
	c->sound_device			= 0;		// �T�E���h�f�o�C�X(0�`15)
	c->sample_rate			= 5;		// �T���v�����O���[�g(0�`4)
	c->primary_buffer		= 10;		// �o�b�t�@�T�C�Y(2�`100)
	c->polling_buffer		= 5;		// �|�[�����O�Ԋu(0�`99)
	c->adpcm_interp			= TRUE;		// ADPCM���`��Ԃ���

	// �`��
	c->aspect_stretch		= TRUE;		// �A�X�y�N�g��ɂ��킹�g��

	// ����
	c->master_volume		= 100;		// �}�X�^����(0�`100)
	c->fm_enable			= TRUE;		// FM�L��
	c->fm_volume			= 54;		// FM����(0�`100)
	c->adpcm_enable			= TRUE;		// ADPCM�L��
	c->adpcm_volume			= 52;		// ADPCM����(0�`100)

	// �L�[�{�[�h
	c->kbd_connect			= TRUE;		// �ڑ�

	// �}�E�X
	c->mouse_speed			= 205;		// �X�s�[�h
	c->mouse_port			= 1;		// �ڑ��|�[�g
	c->mouse_swap			= FALSE;	// �{�^���X���b�v
	c->mouse_mid			= TRUE;		// ���{�^���C�l�[�u��
	c->mouse_trackb			= FALSE;	// �g���b�N�{�[�����[�h

	// �W���C�X�e�B�b�N
	c->joy_type[0]			= 1;		// �W���C�X�e�B�b�N�^�C�v
	c->joy_type[1]			= 1;		// �W���C�X�e�B�b�N�^�C�v
	c->joy_dev[0]			= 1;		// �W���C�X�e�B�b�N�f�o�C�X
	c->joy_dev[1]			= 2;		// �W���C�X�e�B�b�N�f�o�C�X
	c->joy_button0[0]		= 1;		// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[1]		= 2;		// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[2]		= 3;		// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[3]		= 4;		// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[4]		= 5;		// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[5]		= 6;		// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[6]		= 7;		// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[7]		= 8;		// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[8]		= 0;		// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[9]		= 0;		// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[10]		= 0;		// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button0[11]		= 0;		// �W���C�X�e�B�b�N�{�^��(�f�o�C�XA)
	c->joy_button1[0]		= 65537;	// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[1]		= 65538;	// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[2]		= 65539;	// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[3]		= 65540;	// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[4]		= 65541;	// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[5]		= 65542;	// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[6]		= 65543;	// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[7]		= 65544;	// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[8]		= 0;		// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[9]		= 0;		// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[10]		= 0;		// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)
	c->joy_button1[11]		= 0;		// �W���C�X�e�B�b�N�{�^��(�f�o�C�XB)


	// MIDI
	c->midi_bid				= 0;		// MIDI�{�[�hID
	c->midi_ilevel			= 0;		// MIDI���荞�݃��x��
	c->midi_reset			= 0;		// MIDI���Z�b�g�R�}���h
	c->midiin_device		= 0;		// MIDI IN�f�o�C�X
	c->midiin_delay			= 0;		// MIDI IN�f�B���C(ms)
	c->midiout_device		= 0;		// MIDI OUT�f�o�C�X
	c->midiout_delay		= 84;		// MIDI OUT�f�B���C(ms)

	// ����
	c->sram_64k				= FALSE;	// 64KB SRAM
	c->scc_clkup			= FALSE;	// SCC�N���b�N�A�b�v
	c->power_led			= FALSE;	// �F�d��LED
	c->dual_fdd				= FALSE;	// 2DD/2HD���pFDD
	c->sasi_parity			= FALSE;	// SASI�o�X�p���e�B

	// TrueKey
	c->tkey_mode			= 1;		// TrueKey���[�h(bit0:VM bit1:WinApp)
	c->tkey_com				= 0;		// �L�[�{�[�hCOM�|�[�g
	c->tkey_rts				= FALSE;	// RTS���]���[�h

	// ���̑�
	c->floppy_speed			= TRUE;		// �t���b�s�[�f�B�X�N����
	c->floppy_led			= TRUE;		// �t���b�s�[�f�B�X�NLED���[�h
	c->popup_swnd			= TRUE;		// �|�b�v�A�b�v�T�u�E�B���h�E
	c->auto_mouse			= FALSE;	// �����}�E�X���[�h����
	c->power_off			= FALSE;	// �d��OFF�ŊJ�n

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
static void processInput(HWND hWnd) {
	static LPDIRECTINPUT	lpDi		= 0;
	static CRTC*			m_pCRTC;		// CRTC
	static DWORD			m_dwDispCount;	// CRTC�\���J�E���g
	static Keyboard*		m_pKeyboard;	// �L�[�{�[�h
	static Mouse*			m_pMouse;		// �}�E�X
	static PPI*				m_pPPI;			// PPI
	static BOOL				m_bEnable	= TRUE;

	if(lpDi == 0) {
		m_dwDispCount	= 0;

		m_pCRTC		= (CRTC*)		cvm->getDevice(MAKEID('C', 'R', 'T', 'C'));
		m_pKeyboard	= (Keyboard*)	cvm->getDevice(MAKEID('K', 'E', 'Y', 'B'));
		m_pMouse	= (Mouse*)		cvm->getDevice(MAKEID('M', 'O', 'U', 'S'));
		m_pPPI		= (PPI*)		cvm->getDevice(MAKEID('P', 'P', 'I', ' '));

		DirectInput8Create(GetModuleHandle(0), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**) &lpDi, 0);

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
		{
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
//	conversion 32bit word array to 16bit word array
//
//---------------------------------------------------------------------------
static void soundPack(const DWORD* src, unsigned short* dst, int nDstBytes) {
	int i = nDstBytes >> 1;
	while(i-- > 0) {
		*dst++ = static_cast<unsigned short>(*src++);
	}
}



//---------------------------------------------------------------------------
//
//	�i�s
//
//---------------------------------------------------------------------------
static void processSound(HWND hWnd) {

	static BOOL			m_bEnable	= TRUE;

	// �Đ�
	static UINT					m_uRate;				// �T���v�����O���[�g
	static UINT					m_bufBytes;				// �o�b�t�@�T�C�Y(�o�C�g)
	static DWORD*				m_lpBuf;				// �T�E���h�o�b�t�@
	static LPDIRECTSOUND		m_lpDS;					// DirectSound
	static LPDIRECTSOUNDBUFFER	m_lpDSb;				// DirectSoundBuffer(�Z�J���_��)

	static OPMIF*				m_pOPMIF		= 0;	// OPM�C���^�t�F�[�X
	static ADPCM*				m_pADPCM		= 0;	// ADPCM
	static SCSI*				m_pSCSI			= 0;	// SCSI
	static FM::OPM*				m_pOPM			= 0;	// OPM�f�o�C�X
	static Scheduler*			m_pScheduler	= 0;	// �X�P�W���[��

	if(m_pScheduler == 0) {
		m_pScheduler	= (Scheduler*)cvm->getDevice(MAKEID('S', 'C', 'H', 'E'));
		m_pOPMIF		= (OPMIF*)cvm->getDevice(MAKEID('O', 'P', 'M', ' '));
		m_pADPCM		= (ADPCM*)cvm->getDevice(MAKEID('A', 'P', 'C', 'M'));
		m_pSCSI			= (SCSI*)cvm->getDevice(MAKEID('S', 'C', 'S', 'I'));

		// �f�o�C�X��
		class DsEnumerator {
		public:
			DsEnumerator() : lpGuid(0) {}
			LPGUID enumerate() {
				DirectSoundEnumerate(DSEnumCallback_static, this);
				return lpGuid;
			}
		protected:
			static BOOL CALLBACK DSEnumCallback_static(LPGUID lpGuid, LPCSTR, LPCSTR, LPVOID lpContext) {
				DsEnumerator* p = reinterpret_cast<DsEnumerator*>(lpContext);
				if(!p->lpGuid) {
					p->lpGuid = lpGuid;
				}
				return TRUE;
			}
			LPGUID	lpGuid;
		};

		DsEnumerator de;
		LPGUID lpGuid = de.enumerate();

		m_bEnable = TRUE;

		// �����ł͏��������Ȃ�(ApplyCfg�ɔC����)

		//VC2010//	�ʓ|�Ȃ̂ł����ŏ�����
		m_uRate				= 44100;

		UINT m_uTick		= 100;

		if(   m_uRate
		   && lpGuid
		   && SUCCEEDED(DirectSoundCreate(lpGuid, &m_lpDS, NULL))
		   && SUCCEEDED(m_lpDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY))
		) {
			DSBUFFERDESC pbbd = { 0 };
			pbbd.dwSize				= sizeof(pbbd);
			pbbd.dwFlags			= DSBCAPS_PRIMARYBUFFER;

			WAVEFORMATEX wfex = { 0 };
			wfex.wFormatTag			= WAVE_FORMAT_PCM;
			wfex.nChannels			= 2;
			wfex.nSamplesPerSec		= m_uRate;
			wfex.nBlockAlign		= 4;
			wfex.nAvgBytesPerSec	= wfex.nSamplesPerSec * wfex.nBlockAlign;
			wfex.wBitsPerSample		= 16;

			DSBUFFERDESC dsbd = { 0 };
			dsbd.dwSize				= sizeof(dsbd);
			dsbd.dwFlags			= DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME;
			dsbd.dwBufferBytes		= (wfex.nAvgBytesPerSec * m_uTick) / 1000;
			dsbd.dwBufferBytes		= ((dsbd.dwBufferBytes + 7) >> 3) << 3;	// 8�o�C�g���E
			dsbd.lpwfxFormat		= &wfex;
			m_bufBytes				= dsbd.dwBufferBytes;

			bool b = true;

			LPDIRECTSOUNDBUFFER	m_lpDSp = 0;				// DirectSoundBuffer(�v���C�}��)
			b = b && SUCCEEDED(m_lpDS->CreateSoundBuffer(&pbbd, &m_lpDSp, NULL));
			b = b && SUCCEEDED(m_lpDSp->SetFormat(&wfex));
			b = b && SUCCEEDED(m_lpDS->CreateSoundBuffer(&dsbd, &m_lpDSb, NULL));

			// �T�E���h�o�b�t�@���쐬(�Z�J���_���o�b�t�@�Ɠ���̒����A1�P��DWORD)
			if(b) {
				int nbuf = m_bufBytes / 2;
				m_lpBuf = new DWORD [nbuf];
				memset(m_lpBuf, 0, sizeof(*m_lpBuf) * nbuf);

				// OPM�f�o�C�X(�W��)���쐬
				m_pOPM = new FM::OPM;
				m_pOPM->Init(4000000, m_uRate, true);
				m_pOPM->Reset();
				m_pOPM->SetVolume(54);		//pConfig->fm_volume);

				// OPMIF�֒ʒm
				m_pOPMIF->InitBuf(m_uRate);
				m_pOPMIF->SetEngine(m_pOPM);
				m_pOPMIF->EnableFM(1);		//pConfig->fm_enable);

				m_pADPCM->EnableADPCM(1);	//pConfig->adpcm_enable);
				m_pADPCM->SetVolume(52);	//pConfig->adpcm_volume);

				m_lpDSb->Play(0, 0, DSBPLAY_LOOPING);
			}
		}
	}

	static UINT					m_uPoll			= 5;	// �|�[�����O�Ԋu(ms)
	static UINT					m_uCount		= 0;	// �|�[�����O�J�E���g
	static DWORD				m_dwWrite		= 0;	// �������݊����ʒu
	if(++m_uCount >= m_uPoll) {
		m_uCount = 0;
		if(m_bEnable) {
			// ����������Ă��Ȃ���΁A�������Ȃ�
			// �v���C��ԂłȂ���΁A�֌W�Ȃ�
			if (!m_pOPM) {
				m_pScheduler->SetSoundTime(0);
			} else {
				// ���݂̃v���C�ʒu�𓾂�(�o�C�g�P��)
				DWORD dwOffset = 0;
				DWORD dwWrite = 0;
				if(SUCCEEDED(m_lpDSb->GetCurrentPosition(&dwOffset, &dwWrite))) {
					// �O�񏑂����񂾈ʒu����A�󂫃T�C�Y���v�Z(�o�C�g�P��)
					DWORD dwRequest = 0;
					if (m_dwWrite <= dwOffset) {
						dwRequest = dwOffset - m_dwWrite;
					} else {
						dwRequest = m_bufBytes - m_dwWrite;
						dwRequest += dwOffset;
					}

					// �󂫃T�C�Y���S�̂�1/4�𒴂��Ă��Ȃ���΁A���̋@���
					if (dwRequest >= (m_bufBytes / 4)) {
						// �󂫃T���v���Ɋ��Z(L,R��1�Ɛ�����)
						ASSERT((dwRequest & 3) == 0);
						dwRequest /= 4;

						// m_lpBuf�Ƀo�b�t�@�f�[�^���쐬�B�܂�bRun�`�F�b�N
						{
							// OPM�ɑ΂��āA�����v���Ƒ��x����
							DWORD dwReady = m_pOPMIF->ProcessBuf();
							m_pOPMIF->GetBuf(m_lpBuf, (int)dwRequest);
							if (dwReady < dwRequest) {
								dwRequest = dwReady;
							}

							// ADPCM�ɑ΂��āA�f�[�^��v��(���Z���邱��)
							m_pADPCM->GetBuf(m_lpBuf, (int)dwRequest);

							// ADPCM�̓�������
							if (dwReady > dwRequest) {
								m_pADPCM->Wait(dwReady - dwRequest);
							} else {
								m_pADPCM->Wait(0);
							}

							// SCSI�ɑ΂��āA�f�[�^��v��(���Z���邱��)
							m_pSCSI->GetBuf(m_lpBuf, (int)dwRequest, m_uRate);
						}

						// �����Ń��b�N
						{
							WORD *pBuf1;
							WORD *pBuf2;
							DWORD dwSize1;
							DWORD dwSize2;
							HRESULT hr = m_lpDSb->Lock(m_dwWrite, (dwRequest * 4), (void**)&pBuf1, &dwSize1, (void**)&pBuf2, &dwSize2, 0);
							// �o�b�t�@�������Ă���΁A���X�g�A
							if (hr == DSERR_BUFFERLOST) {
								m_lpDSb->Restore();
							}

							// ���b�N�������Ȃ���΁A�����Ă��Ӗ����Ȃ�
							if (FAILED(hr)) {
								m_dwWrite = dwOffset;
							} else {
								soundPack(m_lpBuf, pBuf1, dwSize1);
								if(dwSize2 > 0) {
									soundPack(&m_lpBuf[dwSize1 / 2], pBuf2, dwSize2);
								}

								// �A�����b�N
								m_lpDSb->Unlock(pBuf1, dwSize1, pBuf2, dwSize2);

								// m_dwWrite�X�V
								m_dwWrite += dwSize1;
								m_dwWrite += dwSize2;
								if (m_dwWrite >= m_bufBytes) {
									m_dwWrite -= m_bufBytes;
								}
								ASSERT(m_dwWrite < m_bufBytes);
							}
						}
					}
				}
			}
		}
	}
}



//---------------------------------------------------------------------------
//
//	�R�}���h���C������ �T�u
//	���R�}���h���C���AWM_COPYDATA�A�h���b�O&�h���b�v�ŋ���
//
//---------------------------------------------------------------------------
static BOOL FASTCALL InitCmdSub(int nDrive, LPCTSTR lpszPath) {
	ASSERT((nDrive == 0) || (nDrive == 1));
	ASSERT(lpszPath);

	BOOL ret = FALSE;

	Filepath path;
	BOOL isExist = FALSE;
	{
		TCHAR szPath[_MAX_PATH] = { 0 };
		LPTSTR lpszFile;
		::GetFullPathName(lpszPath, _MAX_PATH, szPath, &lpszFile);
		path.SetPath(szPath);

		Fileio fio;
		isExist = fio.Open(path, Fileio::ReadOnly);
		fio.Close();
	}

	if(isExist) {
		FDI *pFDI = NULL;

		{
			FDD* pFDD = (FDD*)cvm->getDevice(MAKEID('F', 'D', 'D', ' '));
			if(pFDD && pFDD->Open(nDrive, path)) {
				pFDI = pFDD->GetFDI(nDrive);
			}
		}

		if(pFDI) {
			if (pFDI->GetID() == MAKEID('B', 'A', 'D', ' ')) {
				//	bad image
			}
			ret = TRUE;
		}
	}

	return ret;
}



//---------------------------------------------------------------------------
static void OnDraw(HDC hdc) {
	// �������[�N��`
	struct DRAWINFO {
		BOOL bPower;					// �d��
		Render *pRender;				// �����_��
		Render::render_t *pWork;		// �����_�����[�N
        DWORD dwDrawCount;				// �`���

		// DIB�Z�N�V����
		HBITMAP hBitmap;				// DIB�Z�N�V����
		DWORD *pBits;					// �r�b�g�f�[�^
		int nBMPWidth;					// BMP��
		int nBMPHeight;					// BMP����

		// �����_���A�g
		int nRendWidth;					// �����_����
		int nRendHeight;				// �����_������
		int nRendHMul;					// �����_���������{��
		int nRendVMul;					// �����_���c�����{��
		int nLeft;						// ���}�[�W��
		int nTop;						// �c�}�[�W��
		int nWidth;						// BitBlt��
		int nHeight;					// BitBlt����

		// Blt�n
		int nBltTop;					// �`��J�nY
		int nBltBottom;					// �`��I��Y
		int nBltLeft;					// �`��J�nX
		int nBltRight;					// �`��I��X
		BOOL bBltAll;					// �S�\���t���O
	};

	HWND	hWnd	= getMainWindow();

	static DRAWINFO m_Info;										// �������[�N

	m_Info.bBltAll	= TRUE;

	if(m_Info.hBitmap == 0) {
		BITMAPINFOHEADER *p;
		// �r�b�g�}�b�v������΁A��U���
		if (m_Info.hBitmap) {
			if (m_Info.pRender) {
				m_Info.pRender->SetMixBuf(NULL, 0, 0);
			}
			::DeleteObject(m_Info.hBitmap);
			m_Info.hBitmap = NULL;
			m_Info.pBits = NULL;
		}

		// �ŏ����͓��ʈ���
		RECT rect;
		GetClientRect(hWnd, &rect);
		int w = rect.right - rect.left;
		int h = rect.bottom - rect.top;
		if ((w != 0) && (h != 0)) {
			// �r�b�g�}�b�v�w�b�_�̂��߂̃��������m��
			p = (BITMAPINFOHEADER*) new BYTE[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD)];
			memset(p, 0, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD));

			// �r�b�g�}�b�v���쐬
			m_Info.nBMPWidth = w;
			m_Info.nBMPHeight = h;
			p->biSize = sizeof(BITMAPINFOHEADER);
			p->biWidth = m_Info.nBMPWidth;
			p->biHeight = -m_Info.nBMPHeight;
			p->biPlanes = 1;
			p->biBitCount = 32;
			p->biCompression = BI_RGB;
			p->biSizeImage = m_Info.nBMPWidth * m_Info.nBMPHeight * (32 >> 3);

			// DC�擾�ADIB�Z�N�V�����쐬
//			CClientDC *pDC = new CClientDC(this);
			m_Info.hBitmap = ::CreateDIBSection(hdc, (BITMAPINFO*)p, DIB_RGB_COLORS,
										(void**)&(m_Info.pBits), NULL, 0);
			// ����������A�����_���ɓ`����
			if (m_Info.hBitmap && m_Info.pRender) {
				m_Info.pRender->SetMixBuf(m_Info.pBits, m_Info.nBMPWidth, m_Info.nBMPHeight);
			}
//			delete pDC;
			delete[] p;

			// �Čv�Z
			m_Info.nRendHMul = -1;
			m_Info.nRendVMul = -1;
		}
	}

	if(m_Info.hBitmap && m_Info.pRender == 0) {
		m_Info.pRender = (Render*)cvm->getDevice(MAKEID('R', 'E', 'N', 'D'));
		ASSERT(m_Info.pRender);
		m_Info.pWork = m_Info.pRender->GetWorkAddr();
		ASSERT(m_Info.pWork);
		if (m_Info.pBits) {
			m_Info.pRender->SetMixBuf(m_Info.pBits, m_Info.nBMPWidth, m_Info.nBMPHeight);
		}
	}

	if(m_Info.hBitmap && m_Info.pRender) {
		// �r�b�g�}�b�v�̏������o���ĂȂ���Γh��Ԃ�
		RECT rect;
		GetClientRect(hWnd, &rect);
		if (!m_Info.hBitmap || !m_Info.pWork) {
//			pDC->FillSolidRect(&rect, RGB(0, 0, 0));
		} else {
			// �Čv�Z
			// �����_�����[�N�A�r�b�g�}�b�v���Ȃ����return
			if(m_Info.pWork && m_Info.hBitmap) {
				// ��r
				BOOL flag = FALSE;
				if (m_Info.nRendWidth != m_Info.pWork->width) {
					m_Info.nRendWidth = m_Info.pWork->width;
					flag = TRUE;
				}
				if (m_Info.nRendHeight != m_Info.pWork->height) {
					m_Info.nRendHeight = m_Info.pWork->height;
					flag = TRUE;
				}
				if (m_Info.nRendHMul != m_Info.pWork->h_mul) {
					m_Info.nRendHMul = m_Info.pWork->h_mul;
					flag = TRUE;
				}
				if (m_Info.nRendVMul != m_Info.pWork->v_mul) {
					m_Info.nRendVMul = m_Info.pWork->v_mul;
					flag = TRUE;
				}
				if(flag) {
					// �����_���A�r�b�g�}�b�v�̂����������ق����Ƃ�
					m_Info.nWidth = m_Info.nRendWidth;
					if (m_Info.nBMPWidth < m_Info.nWidth) {
						m_Info.nWidth = m_Info.nBMPWidth;
					}
					m_Info.nHeight = m_Info.nRendHeight;
					if (m_Info.nRendVMul == 0) {
						// 15k�C���^���[�X�̂��߂̏���
						m_Info.nHeight <<= 1;
					}
					if (m_Info.nBMPHeight < m_Info.nRendHeight) {
						m_Info.nHeight = m_Info.nBMPHeight;
					}

					// �{�����l�����ăZ���^�����O���A�]�����Z�o
					int width = m_Info.nWidth * m_Info.nRendHMul;
				//	if ((m_Info.nRendWidth < 600) && m_Info.bBltStretch) {	// if���Ǘv
				//		width = (width * 5) >> 2;
				//	}
					int height = m_Info.nHeight;
					if (m_Info.nRendVMul == 2) {
						height <<= 1;
					}

					int w = rect.right - rect.left;
					int h = rect.bottom - rect.top;

					m_Info.nLeft = 0;
					if (width < w) {
						m_Info.nLeft = (w - width) >> 1;
					}
					m_Info.nTop = 0;
					if (height < h) {
						m_Info.nTop = (h - height) >> 1;
					}

					// �̈�`����w��
					m_Info.bBltAll = TRUE;
				}
			}

			// �d��OFF�΍�
			if (cvm->isPower() != m_Info.bPower) {
				m_Info.bPower = cvm->isPower();
				if (!m_Info.bPower) {
					// �r�b�g�}�b�v�����ׂď���
					memset(m_Info.pBits, 0, m_Info.nBMPWidth * m_Info.nBMPHeight * 4);
					m_Info.bBltAll = TRUE;
				}
			}

			// �ŏI�\���{�����m��
			int hmul;
			hmul = 1;
			if (m_Info.nRendHMul == 2) {
				// ��256�Ȃ�
				hmul = 2;
			}
			{
				// �A�X�y�N�g�䓯��͂��Ȃ��B���{�g��
				hmul <<= 2;
			}

			int vmul;
			vmul = 4;
			if (m_Info.nRendVMul == 2) {
				// �c256�Ȃ�
				vmul = 8;
			}

			// ������DC�쐬�A�Z���N�g
			HDC hMemDC = CreateCompatibleDC(hdc);
			if(hMemDC) {
				HBITMAP hDefBitmap = (HBITMAP)SelectObject(hMemDC, m_Info.hBitmap);
				if(hDefBitmap) {
					// Blt
					if ((hmul == 4) && (vmul == 4)) {
						::BitBlt(hdc,
							m_Info.nLeft, m_Info.nTop,
							m_Info.nWidth, m_Info.nHeight,
							hMemDC, 0, 0,
							SRCCOPY);
					}
					else {
						::StretchBlt(hdc,
							m_Info.nLeft, m_Info.nTop,
							(m_Info.nWidth * hmul) >> 2,
							(m_Info.nHeight * vmul) >> 2,
							hMemDC, 0, 0,
							m_Info.nWidth, m_Info.nHeight,
							SRCCOPY);
					}
					::GdiFlush();
					m_Info.bBltAll = FALSE;

					// �r�b�g�}�b�v�߂�
					SelectObject(hMemDC, hDefBitmap);

					// �`��t���O���~�낷���Ƃ�Y�ꂸ��
					for (int i=0; i<m_Info.nHeight * 64; i++) {
						m_Info.pWork->drawflag[i] = FALSE;
					}
					m_Info.dwDrawCount++;
					m_Info.nBltLeft = 0;
					m_Info.nBltTop = 0;
					m_Info.nBltRight = m_Info.nWidth - 1;
					m_Info.nBltBottom = m_Info.nHeight - 1;
				}
				DeleteDC(hMemDC);
			}
		}
	}
}



//---------------------------------------------------------------------------
//
//	cpudebug.c ���[�h�ǂݏo�� (mfc_cpu.cpp)
//
//---------------------------------------------------------------------------
extern "C" WORD cpudebug_fetch(DWORD addr)
{
	static Memory* pMemory = 0;
	if(pMemory == 0) {
		pMemory = (Memory*)cvm->getDevice(MAKEID('M', 'E', 'M', ' '));
		ASSERT(pMemory);
	}

	addr &= 0xfffffe;

	WORD w = (WORD) pMemory->ReadOnly(addr);
	w <<= 8;
	w |= (WORD) pMemory->ReadOnly(addr + 1);

	return w;
}



//---------------------------------------------------------------------------
//
//	�E�B���h�E�쐬
//
//	�t���b�s�[�f�B�X�N�̓���ւ�
//
//		int		nDrive	= [0,1];
//		TCHAR	szPath	= _T("...");
//
//		Filepath path;
//		path.SetPath(szPath);
//
//		::LockVM();
//		FDI* pFDI = 0;
//		if(m_pFDD->Open(nDrive, path)) {
//			pFDI = m_pFDD->GetFDI(nDrive);
//		}
//		if(pFDI) {
//			if (pFDI->GetID() == MAKEID('B', 'A', 'D', ' ')) {
//				//	bad image
//			}
//		} else {
//			// error (bad path, fdd error)
//		}
//		::UnlockVM();
//
//
//---------------------------------------------------------------------------
INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, INT) {
	enum {
		CW_WIDTH	= 768,
		CW_HEIGHT	= 512,
		CW_STYLE	= (WS_OVERLAPPED | WS_CAPTION | WS_VISIBLE),
	};

	::timeBeginPeriod(1);

	RECT rc = { 0, 0, CW_WIDTH, CW_HEIGHT };
	AdjustWindowRect(&rc, CW_STYLE, 0);

	mainWindow = CreateWindow(_T("edit"), 0, CW_STYLE, 0, 0, rc.right-rc.left, rc.bottom-rc.top, 0, 0, 0, 0);
	{
		// VM������
		cvm = new CVm();
		cvm->init();

		// �ݒ�K�p(OnOption�Ɠ��l�AVM���b�N����)
		{
			Config config;
			configGetConfig(&config);
			cvm->applyCfg(&config);
		}

		{
			const TCHAR* err = 0;
			Memory* pMemory = (Memory*)cvm->getDevice(MAKEID('M', 'E', 'M', ' '));
			ASSERT(pMemory);
			if(err == 0 && pMemory->GetIPL()[0] == 0xff) {
				err = _T("IPL ROM LOADING ERROR (THERE IS NO 'IPLROM.DAT')");
			}
			if(err == 0 && pMemory->GetCG()[0] == 0xff) {
				err = _T("CG ROM LOADING ERROR (THERE IS NO 'CGROM.DAT')");
			}
//			if (err == 0 && !pMemory->CheckIPL()) {
//				err = _T("IPL ROM VERION ERROR");
//			}
//			if (err == 0 && !pMemory->CheckCG()) {
//				err = _T("CG ROM VERSION ERROR");
//			}
			if(err) {
				MessageBox(mainWindow, err, err, MB_OK);
				return 0;
			}
		}

		// ���Z�b�g
		cvm->reset();

		LPCTSTR	szPath0	= _T("C:\\projects\\github\\xm6_pid\\00proj.vc10_nomfc\\Debug\\bosconian.xdf");
		LPCTSTR	szPath1	= _T("");

		BOOL bReset0 = InitCmdSub(0, szPath0);
		BOOL bReset1 = InitCmdSub(1, szPath1);

		if(bReset0 || bReset1) {
			cvm->reset();
		}

		Render*		pRender		= (Render*)cvm->getDevice(MAKEID('R', 'E', 'N', 'D'));
		HWND		hFrmWnd		= getMainWindow();	//m_pFrmWnd->m_hWnd;
		DWORD		dwExecTime	= GetTime();
		DWORD		dwExecCount	= 0;

		// �I�����N�G�X�g���オ��܂Ń��[�v
		while (!scheduler_m_bExitReq) {
			if(!scheduler_m_bExitReq) {
				if(GetAsyncKeyState(VK_ESCAPE)) {
					scheduler_m_bExitReq = true;
				}
			}

			int	preSleep	= 0;
			int postSleep	= -1;

			if(preSleep >= 0) {
				::Sleep(preSleep);
			}

			bool requestRefresh	= false;

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

				if(cvm->exec(1000 * 2)) {
					if(cvm->isPower()) {
						dwExecCount++;
						dwExecTime++;

						// ���R���|�[�l���g�̏���
						processSound(hFrmWnd);
						processInput(hFrmWnd);

						// dwExecCount���K�萔�𒴂�����A��x�\�����ċ������ԍ��킹
						if (dwExecCount > 400) {
							requestRefresh = true;
							dwExecCount = 0;
							dwExecTime = GetTime();
						}
					}
				}
			}

			if(requestRefresh) {
				if(pRender->IsReady()) {
					HDC hdc = GetDC(getMainWindow());	//m_pFrmWnd->m_hWnd);
	//				m_pFrmWnd->OnDraw(hdc);
					OnDraw(hdc);
					ReleaseDC(getMainWindow(), hdc);
					pRender->Complete();
				}
			}

			if(postSleep >= 0) {
				::Sleep(postSleep);
			}
		}
	}

	// ���z�}�V�����폜
	cvm->cleanup();
	delete cvm;

	return 0;
}
#endif	// _WIN32
