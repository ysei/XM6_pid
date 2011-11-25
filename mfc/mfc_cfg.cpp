//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �R���t�B�O ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "memory.h"
#include "opmif.h"
#include "adpcm.h"
#include "config.h"
#include "render.h"
#include "sasi.h"
#include "scsi.h"
#include "disk.h"
#include "filepath.h"
#include "ppi.h"
#include "fileio.h"
#include "mfc_frm.h"
#include "mfc_com.h"
#include "mfc_res.h"
#include "mfc_cfg.h"

//===========================================================================
//
//	�R���t�B�O
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CConfig::CConfig(CFrmWnd *pWnd) : CComponent(pWnd)
{
	// �R���|�[�l���g�p�����[�^
	m_dwID = MAKEID('C', 'F', 'G', ' ');
	m_strDesc = _T("Config Manager");
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CConfig::Init()
{
	int i;
	Filepath path;

	ASSERT(this);

	// ��{�N���X
	if (!CComponent::Init()) {
		return FALSE;
	}

	// INI�t�@�C���p�X����
	path.SetPath(_T("XM6.ini"));
	path.SetBaseFile();
	_tcscpy(m_IniFile, path.GetPath());

	// �ݒ�f�[�^
	LoadConfig();

	// �݊����ێ�
	ResetSASI();
	ResetCDROM();

	// MRU
	for (i=0; i<MruTypes; i++) {
		ClearMRU(i);
		LoadMRU(i);
	}

	// �L�[
	LoadKey();

	// TrueKey
//	LoadTKey();

	// �Z�[�u�E���[�h
	m_bApply = FALSE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::Cleanup()
{
	int i;

	ASSERT(this);

	// �ݒ�f�[�^
	SaveConfig();

	// MRU
	for (i=0; i<MruTypes; i++) {
		SaveMRU(i);
	}

	// �L�[
	SaveKey();

	// TrueKey
//	SaveTKey();

	// ��{�N���X
	CComponent::Cleanup();
}

//---------------------------------------------------------------------------
//
//	�ݒ�f�[�^����
//
//---------------------------------------------------------------------------
Config CConfig::m_Config;

//---------------------------------------------------------------------------
//
//	�ݒ�f�[�^�擾
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::GetConfig(Config *pConfigBuf) const
{
	ASSERT(this);
	ASSERT(pConfigBuf);

	// �������[�N���R�s�[
	*pConfigBuf = m_Config;
}

//---------------------------------------------------------------------------
//
//	�ݒ�f�[�^�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SetConfig(Config *pConfigBuf)
{
	ASSERT(this);
	ASSERT(pConfigBuf);

	// �������[�N�փR�s�[
	m_Config = *pConfigBuf;
}

//---------------------------------------------------------------------------
//
//	��ʊg��ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SetStretch(BOOL bStretch)
{
	ASSERT(this);

	m_Config.aspect_stretch = bStretch;
}

//---------------------------------------------------------------------------
//
//	MIDI�f�o�C�X�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SetMIDIDevice(int nDevice, BOOL bIn)
{
	ASSERT(this);
	ASSERT(nDevice >= 0);

	// In�܂���Out
	if (bIn) {
		m_Config.midiin_device = nDevice;
	}
	else {
		m_Config.midiout_device = nDevice;
	}
}

//---------------------------------------------------------------------------
//
//	INI�t�@�C���e�[�u��
//	���|�C���^�E�Z�N�V�������E�L�[���E�^�E�f�t�H���g�l�E�ŏ��l�E�ő�l�̏�
//
//---------------------------------------------------------------------------
const CConfig::INIKEY CConfig::IniTable[] = {
	{ &CConfig::m_Config.system_clock					, _T("Basic")		, _T("Clock")			, 0, 0, 0, 5 },
	{ &CConfig::m_Config.mpu_fullspeed					, NULL				, _T("MPUFullSpeed")	, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.vm_fullspeed					, NULL				, _T("VMFullSpeed")		, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.ram_size						, NULL				, _T("Memory")			, 0, 0, 0, 5 },
	{ &CConfig::m_Config.ram_sramsync					, NULL				, _T("AutoMemSw")		, 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.mem_type						, NULL				, _T("Map")				, 0, 1, 1, 6 },
	{ &CConfig::m_Config.sound_device					, _T("Sound")		, _T("Device")			, 0, 0, 0, 15 },
	{ &CConfig::m_Config.sample_rate					, NULL				, _T("Rate")			, 0, 5, 0, 5 },
	{ &CConfig::m_Config.primary_buffer					, NULL				, _T("Primary")			, 0, 10, 2, 100 },
	{ &CConfig::m_Config.polling_buffer					, NULL				, _T("Polling")			, 0, 5, 1, 100 },
	{ &CConfig::m_Config.adpcm_interp					, NULL				, _T("ADPCMInterP")		, 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.aspect_stretch					, _T("Display")		, _T("Stretch")			, 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.caption_info					, NULL				, _T("Info")			, 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.master_volume					, _T("Volume")		, _T("Master")			, 0, 100, 0, 100 },
	{ &CConfig::m_Config.fm_enable						, NULL				, _T("FMEnable")		, 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.fm_volume						, NULL				, _T("FM")				, 0, 54, 0, 100 },
	{ &CConfig::m_Config.adpcm_enable					, NULL				, _T("ADPCMEnable")		, 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.adpcm_volume					, NULL				, _T("ADPCM")			, 0, 52, 0, 100 },
	{ &CConfig::m_Config.kbd_connect					, _T("Keyboard")	, _T("Connect")			, 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.mouse_speed					, _T("Mouse")		, _T("Speed")			, 0, 205, 0, 512 },
	{ &CConfig::m_Config.mouse_port						, NULL				, _T("Port")			, 0, 1, 0, 2 },
	{ &CConfig::m_Config.mouse_swap						, NULL				, _T("Swap")			, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.mouse_mid						, NULL				, _T("MidBtn")			, 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.mouse_trackb					, NULL				, _T("TrackBall")		, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.joy_type[0]					, _T("Joystick")	, _T("Port1")			, 0, 1, 0, 15 },
	{ &CConfig::m_Config.joy_type[1]					, NULL				, _T("Port2")			, 0, 1, 0, 15 },
	{ &CConfig::m_Config.joy_dev[0]						, NULL				, _T("Device1")			, 0, 1, 0, 15 },
	{ &CConfig::m_Config.joy_dev[1]						, NULL				, _T("Device2")			, 0, 2, 0, 15 },
	{ &CConfig::m_Config.joy_button0[0]					, NULL				, _T("Button11")		, 0, 1, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[1]					, NULL				, _T("Button12")		, 0, 2, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[2]					, NULL				, _T("Button13")		, 0, 3, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[3]					, NULL				, _T("Button14")		, 0, 4, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[4]					, NULL				, _T("Button15")		, 0, 5, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[5]					, NULL				, _T("Button16")		, 0, 6, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[6]					, NULL				, _T("Button17")		, 0, 7, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[7]					, NULL				, _T("Button18")		, 0, 8, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[8]					, NULL				, _T("Button19")		, 0, 0, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[9]					, NULL				, _T("Button1A")		, 0, 0, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[10]				, NULL				, _T("Button1B")		, 0, 0, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[11]				, NULL				, _T("Button1C")		, 0, 0, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[0]					, NULL				, _T("Button21")		, 0, 65537, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[1]					, NULL				, _T("Button22")		, 0, 65538, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[2]					, NULL				, _T("Button23")		, 0, 65539, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[3]					, NULL				, _T("Button24")		, 0, 65540, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[4]					, NULL				, _T("Button25")		, 0, 65541, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[5]					, NULL				, _T("Button26")		, 0, 65542, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[6]					, NULL				, _T("Button27")		, 0, 65543, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[7]					, NULL				, _T("Button28")		, 0, 65544, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[8]					, NULL				, _T("Button29")		, 0, 0, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[9]					, NULL				, _T("Button2A")		, 0, 0, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[10]				, NULL				, _T("Button2B")		, 0, 0, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[11]				, NULL				, _T("Button2C")		, 0, 0, 0, 131071 },
	{ &CConfig::m_Config.sasi_drives					, _T("SASI")		, _T("Drives")			, 0, -1, 0, 16 },
	{ &CConfig::m_Config.sasi_sramsync					, NULL				, _T("AutoMemSw")		, 1, TRUE, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 0]					, NULL				, _T("File0") 			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 1]					, NULL				, _T("File1") 			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 2]					, NULL				, _T("File2") 			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 3]					, NULL				, _T("File3") 			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 4]					, NULL				, _T("File4") 			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 5]					, NULL				, _T("File5") 			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 6]					, NULL				, _T("File6") 			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 7]					, NULL				, _T("File7") 			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 8]					, NULL				, _T("File8") 			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 9]					, NULL				, _T("File9") 			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[10]					, NULL				, _T("File10")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[11]					, NULL				, _T("File11")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[12]					, NULL				, _T("File12")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[13]					, NULL				, _T("File13")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[14]					, NULL				, _T("File14")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[15]					, NULL				, _T("File15")			, 2, FILEPATH_MAX, 0, 0 },
	{ &CConfig::m_Config.sxsi_drives					, _T("SxSI")		, _T("Drives")			, 0, 0, 0, 7 },
	{ &CConfig::m_Config.sxsi_mofirst					, NULL				, _T("FirstMO")			, 1, FALSE, 0, 0 },
	{ CConfig::m_Config.sxsi_file[ 0]					, NULL				, _T("File0")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sxsi_file[ 1]					, NULL				, _T("File1")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sxsi_file[ 2]					, NULL				, _T("File2")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sxsi_file[ 3]					, NULL				, _T("File3")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sxsi_file[ 4]					, NULL				, _T("File4")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sxsi_file[ 5]					, NULL				, _T("File5")			, 2, FILEPATH_MAX, 0, 0 },
	{ &CConfig::m_Config.scsi_ilevel					, _T("SCSI")		, _T("IntLevel")		, 0, 1, 0, 1 },
	{ &CConfig::m_Config.scsi_drives					, NULL				, _T("Drives")			, 0, 0, 0, 7 },
	{ &CConfig::m_Config.scsi_sramsync					, NULL				, _T("AutoMemSw")		, 1, TRUE, 0, 0 },
	{ &m_bCDROM											, NULL				, _T("CDROM")			, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.scsi_mofirst					, NULL				, _T("FirstMO")			, 1, FALSE, 0, 0 },
	{ CConfig::m_Config.scsi_file[ 0]					, NULL				, _T("File0")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.scsi_file[ 1]					, NULL				, _T("File1")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.scsi_file[ 2]					, NULL				, _T("File2")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.scsi_file[ 3]					, NULL				, _T("File3")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.scsi_file[ 4]					, NULL				, _T("File4")			, 2, FILEPATH_MAX, 0, 0 },
	{ &CConfig::m_Config.port_com						, _T("Port")		, _T("COM")				, 0, 0, 0, 9 },
	{ CConfig::m_Config.port_recvlog					, NULL				, _T("RecvLog")			, 2, FILEPATH_MAX, 0, 0 },
	{ &CConfig::m_Config.port_384						, NULL				, _T("Force38400")		, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.port_lpt						, NULL				, _T("LPT")				, 0, 0, 0, 9 },
	{ CConfig::m_Config.port_sendlog					, NULL				, _T("SendLog")			, 2, FILEPATH_MAX, 0, 0 },
	{ &CConfig::m_Config.midi_bid						, _T("MIDI")		, _T("ID")				, 0, 0, 0, 2 },
	{ &CConfig::m_Config.midi_ilevel					, NULL				, _T("IntLevel")		, 0, 0, 0, 1 },
	{ &CConfig::m_Config.midi_reset						, NULL				, _T("ResetCmd")		, 0, 0, 0, 3 },
	{ &CConfig::m_Config.midiin_device					, NULL				, _T("InDevice")		, 0, 0, 0, 15 },
	{ &CConfig::m_Config.midiin_delay					, NULL				, _T("InDelay")			, 0, 0, 0, 200 },
	{ &CConfig::m_Config.midiout_device					, NULL				, _T("OutDevice")		, 0, 0, 0, 15 },
	{ &CConfig::m_Config.midiout_delay					, NULL				, _T("OutDelay")		, 0, 84, 20, 200 },
	{ &CConfig::m_Config.windrv_enable					, _T("Windrv")		, _T("Enable")			, 0, 0, 0, 255 },
	{ &CConfig::m_Config.host_option					, NULL				, _T("Option")			, 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_resume					, NULL				, _T("Resume")			, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.host_drives					, NULL				, _T("Drives")			, 0, 0, 0, 10 },
	{ &CConfig::m_Config.host_flag[ 0]					, NULL				, _T("Flag0")			, 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_flag[ 1]					, NULL				, _T("Flag1")			, 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_flag[ 2]					, NULL				, _T("Flag2")			, 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_flag[ 3]					, NULL				, _T("Flag3")			, 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_flag[ 4]					, NULL				, _T("Flag4")			, 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_flag[ 5]					, NULL				, _T("Flag5")			, 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_flag[ 6]					, NULL				, _T("Flag6")			, 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_flag[ 7]					, NULL				, _T("Flag7")			, 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_flag[ 8]					, NULL				, _T("Flag8")			, 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_flag[ 9]					, NULL				, _T("Flag9")			, 0, 0, 0, 0x7FFFFFFF },
	{ CConfig::m_Config.host_path[ 0]					, NULL				, _T("Path0")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.host_path[ 1]					, NULL				, _T("Path1")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.host_path[ 2]					, NULL				, _T("Path2")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.host_path[ 3]					, NULL				, _T("Path3")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.host_path[ 4]					, NULL				, _T("Path4")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.host_path[ 5]					, NULL				, _T("Path5")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.host_path[ 6]					, NULL				, _T("Path6")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.host_path[ 7]					, NULL				, _T("Path7")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.host_path[ 8]					, NULL				, _T("Path8")			, 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.host_path[ 9]					, NULL				, _T("Path9")			, 2, FILEPATH_MAX, 0, 0 },
	{ &CConfig::m_Config.sram_64k						, _T("Alter")		, _T("SRAM64K")			, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.scc_clkup						, NULL				, _T("SCCClock")		, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.power_led						, NULL				, _T("BlueLED")			, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.dual_fdd						, NULL				, _T("DualFDD")			, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.sasi_parity					, NULL				, _T("SASIParity")		, 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.caption						, _T("Window")		, _T("Caption")			, 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.menu_bar						, NULL				, _T("MenuBar")			, 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.status_bar						, NULL				, _T("StatusBar")		, 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.window_left					, NULL				, _T("Left")			, 0, -0x8000, -0x8000, 0x7fff },
	{ &CConfig::m_Config.window_top						, NULL				, _T("Top")				, 0, -0x8000, -0x8000, 0x7fff },
	{ &CConfig::m_Config.window_full					, NULL				, _T("Full")			, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.window_mode					, NULL				, _T("Mode")			, 0, 0, 0, 0 },
	{ &CConfig::m_Config.resume_fd						, _T("Resume")		, _T("FD")				, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_fdi[0]					, NULL				, _T("FDI0")			, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_fdi[1]					, NULL				, _T("FDI1")			, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_fdw[0]					, NULL				, _T("FDW0")			, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_fdw[1]					, NULL				, _T("FDW1")			, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_fdm[0]					, NULL				, _T("FDM0")			, 0, 0, 0, 15 },
	{ &CConfig::m_Config.resume_fdm[1]					, NULL				, _T("FDM1")			, 0, 0, 0, 15 },
	{ &CConfig::m_Config.resume_mo						, NULL				, _T("MO")				, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_mos						, NULL				, _T("MOS")				, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_mow						, NULL				, _T("MOW")				, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_cd						, NULL				, _T("CD")				, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_iso						, NULL				, _T("ISO")				, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_state					, NULL				, _T("State")			, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_xm6						, NULL				, _T("XM6")				, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_screen					, NULL				, _T("Screen")			, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_dir						, NULL				, _T("Dir")				, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_path					, NULL				, _T("Path")			, 2, FILEPATH_MAX, 0, 0 },
	{ &CConfig::m_Config.tkey_mode						, _T("TrueKey")		, _T("Mode")			, 0, 1, 0, 3 },
	{ &CConfig::m_Config.tkey_com						, NULL				, _T("COM")				, 0, 0, 0, 9 },
	{ &CConfig::m_Config.tkey_rts						, NULL				, _T("RTS")				, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.floppy_speed					, _T("Misc")		, _T("FloppySpeed")		, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.floppy_led						, NULL				, _T("FloppyLED")		, 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.popup_swnd						, NULL				, _T("PopupWnd")		, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.auto_mouse						, NULL				, _T("AutoMouse")		, 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.power_off						, NULL				, _T("PowerOff")		, 1, FALSE, 0, 0 },
	{ NULL, NULL, NULL, 0, 0, 0, 0 }
};

//---------------------------------------------------------------------------
//
//	�ݒ�f�[�^���[�h
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::LoadConfig()
{
	PINIKEY pIni;
	int nValue;
	BOOL bFlag;
	LPCTSTR pszSection;
	TCHAR szDef[1];
	TCHAR szBuf[FILEPATH_MAX];

	ASSERT(this);

	// �e�[�u���̐擪�ɍ��킹��
	pIni = (const PINIKEY)&IniTable[0];
	pszSection = NULL;
	szDef[0] = _T('\0');

	// �e�[�u�����[�v
	while (pIni->pBuf) {
		// �Z�N�V�����ݒ�
		if (pIni->pszSection) {
			pszSection = pIni->pszSection;
		}
		ASSERT(pszSection);

		// �^�C�v�`�F�b�N
		switch (pIni->nType) {
			// �����^(�͈͂𒴂�����f�t�H���g�l)
			case 0:
				nValue = ::GetPrivateProfileInt(pszSection, pIni->pszKey, pIni->nDef, m_IniFile);
				if ((nValue < pIni->nMin) || (pIni->nMax < nValue)) {
					nValue = pIni->nDef;
				}
				*((int*)pIni->pBuf) = nValue;
				break;

			// �_���^(0,1�̂ǂ���ł��Ȃ���΃f�t�H���g�l)
			case 1:
				nValue = ::GetPrivateProfileInt(pszSection, pIni->pszKey, -1, m_IniFile);
				switch (nValue) {
					case 0:
						bFlag = FALSE;
						break;
					case 1:
						bFlag = TRUE;
						break;
					default:
						bFlag = (BOOL)pIni->nDef;
						break;
				}
				*((BOOL*)pIni->pBuf) = bFlag;
				break;

			// ������^(�o�b�t�@�T�C�Y�͈͓��ł̃^�[�~�l�[�g��ۏ�)
			case 2:
				ASSERT(pIni->nDef <= (sizeof(szBuf)/sizeof(TCHAR)));
				::GetPrivateProfileString(pszSection, pIni->pszKey, szDef, szBuf,
										sizeof(szBuf)/sizeof(TCHAR), m_IniFile);

				// �f�t�H���g�l�ɂ̓o�b�t�@�T�C�Y���L�����邱��
				ASSERT(pIni->nDef > 0);
				szBuf[pIni->nDef - 1] = _T('\0');
				_tcscpy((LPTSTR)pIni->pBuf, szBuf);
				break;

			// ���̑�
			default:
				ASSERT(FALSE);
				break;
		}

		// ����
		pIni++;
	}
}

//---------------------------------------------------------------------------
//
//	�ݒ�f�[�^�Z�[�u
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SaveConfig() const
{
	PINIKEY pIni;
	CString string;
	LPCTSTR pszSection;

	ASSERT(this);

	// �e�[�u���̐擪�ɍ��킹��
	pIni = (const PINIKEY)&IniTable[0];
	pszSection = NULL;

	// �e�[�u�����[�v
	while (pIni->pBuf) {
		// �Z�N�V�����ݒ�
		if (pIni->pszSection) {
			pszSection = pIni->pszSection;
		}
		ASSERT(pszSection);

		// �^�C�v�`�F�b�N
		switch (pIni->nType) {
			// �����^
			case 0:
				string.Format(_T("%d"), *((int*)pIni->pBuf));
				::WritePrivateProfileString(pszSection, pIni->pszKey,
											string, m_IniFile);
				break;

			// �_���^
			case 1:
				if (*(BOOL*)pIni->pBuf) {
					string = _T("1");
				}
				else {
					string = _T("0");
				}
				::WritePrivateProfileString(pszSection, pIni->pszKey,
											string, m_IniFile);
				break;

			// ������^
			case 2:
				::WritePrivateProfileString(pszSection, pIni->pszKey,
											(LPCTSTR)pIni->pBuf, m_IniFile);
				break;

			// ���̑�
			default:
				ASSERT(FALSE);
				break;
		}

		// ����
		pIni++;
	}
}

//---------------------------------------------------------------------------
//
//	SASI���Z�b�g
//	��version1.44�܂ł͎����t�@�C�������̂��߁A�����ōČ����Ɛݒ���s��
//	version1.45�ȍ~�ւ̈ڍs���X���[�Y�ɍs��
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::ResetSASI()
{
	int i;
	Filepath path;
	Fileio fio;
	TCHAR szPath[FILEPATH_MAX];

	ASSERT(this);

	// �h���C�u��>=0�̏ꍇ�͕s�v(�ݒ�ς�)
	if (m_Config.sasi_drives >= 0) {
		return;
	}

	// �h���C�u��0
	m_Config.sasi_drives = 0;

	// �t�@�C�����쐬���[�v
	for (i=0; i<16; i++) {
		_stprintf(szPath, _T("HD%d.HDF"), i);
		path.SetPath(szPath);
		path.SetBaseDir();
		_tcscpy(m_Config.sasi_file[i], path.GetPath());
	}

	// �ŏ�����`�F�b�N���āA�L���ȃh���C�u�������߂�
	for (i=0; i<16; i++) {
		path.SetPath(m_Config.sasi_file[i]);
		if (!fio.Open(path, Fileio::ReadOnly)) {
			return;
		}

		// �T�C�Y�`�F�b�N(version1.44�ł�40MB�h���C�u�̂݃T�|�[�g)
		if (fio.GetFileSize() != 0x2793000) {
			fio.Close();
			return;
		}

		// �J�E���g�A�b�v�ƃN���[�Y
		m_Config.sasi_drives++;
		fio.Close();
	}
}

//---------------------------------------------------------------------------
//
//	CD-ROM���Z�b�g
//	��version2.02�܂ł�CD-ROM���T�|�[�g�̂��߁ASCSI�h���C�u����+1����
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::ResetCDROM()
{
	ASSERT(this);

	// CD-ROM�t���O���Z�b�g����Ă���ꍇ�͕s�v(�ݒ�ς�)
	if (m_bCDROM) {
		return;
	}

	// CD-ROM�t���O���Z�b�g
	m_bCDROM = TRUE;

	// SCSI�h���C�u����3�ȏ�6�ȉ��̏ꍇ�Ɍ���A+1
	if ((m_Config.scsi_drives >= 3) && (m_Config.scsi_drives <= 6)) {
		m_Config.scsi_drives++;
	}
}

//---------------------------------------------------------------------------
//
//	CD-ROM�t���O
//
//---------------------------------------------------------------------------
BOOL CConfig::m_bCDROM;

//---------------------------------------------------------------------------
//
//	MRU�N���A
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::ClearMRU(int nType)
{
	int i;

	ASSERT(this);
	ASSERT((nType >= 0) && (nType < MruTypes));

	// ���̃N���A
	for (i=0; i<9; i++) {
		memset(m_MRUFile[nType][i], 0, FILEPATH_MAX * sizeof(TCHAR));
	}

	// ���N���A
	m_MRUNum[nType] = 0;
}

//---------------------------------------------------------------------------
//
//	MRU���[�h
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::LoadMRU(int nType)
{
	CString strSection;
	CString strKey;
	int i;

	ASSERT(this);
	ASSERT((nType >= 0) && (nType < MruTypes));

	// �Z�N�V�����쐬
	strSection.Format(_T("MRU%d"), nType);

	// ���[�v
	for (i=0; i<9; i++) {
		strKey.Format(_T("File%d"), i);
		::GetPrivateProfileString(strSection, strKey, _T(""), m_MRUFile[nType][i],
								FILEPATH_MAX, m_IniFile);
		if (m_MRUFile[nType][i][0] == _T('\0')) {
			break;
		}
		m_MRUNum[nType]++;
	}
}

//---------------------------------------------------------------------------
//
//	MRU�Z�[�u
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SaveMRU(int nType) const
{
	CString strSection;
	CString strKey;
	int i;

	ASSERT(this);
	ASSERT((nType >= 0) && (nType < MruTypes));

	// �Z�N�V�����쐬
	strSection.Format(_T("MRU%d"), nType);

	// ���[�v
	for (i=0; i<9; i++) {
		strKey.Format(_T("File%d"), i);
		::WritePrivateProfileString(strSection, strKey, m_MRUFile[nType][i],
								m_IniFile);
	}
}

//---------------------------------------------------------------------------
//
//	MRU�Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SetMRUFile(int nType, LPCTSTR lpszFile)
{
	int nMRU;
	int nCpy;
	int nNum;

	ASSERT(this);
	ASSERT((nType >= 0) && (nType < MruTypes));
	ASSERT(lpszFile);

	// ���ɓ������̂��Ȃ���
	nNum = GetMRUNum(nType);
	for (nMRU=0; nMRU<nNum; nMRU++) {
		if (_tcscmp(m_MRUFile[nType][nMRU], lpszFile) == 0) {
			// �擪�ɂ����āA�܂��������̂�ǉ����悤�Ƃ���
			if (nMRU == 0) {
				return;
			}

			// �R�s�[
			for (nCpy=nMRU; nCpy>=1; nCpy--) {
				memcpy(m_MRUFile[nType][nCpy], m_MRUFile[nType][nCpy - 1],
						FILEPATH_MAX * sizeof(TCHAR));
			}

			// �擪�ɃZ�b�g
			_tcscpy(m_MRUFile[nType][0], lpszFile);
			return;
		}
	}

	// �ړ�
	for (nMRU=7; nMRU>=0; nMRU--) {
		memcpy(m_MRUFile[nType][nMRU + 1], m_MRUFile[nType][nMRU],
				FILEPATH_MAX * sizeof(TCHAR));
	}

	// �擪�ɃZ�b�g
	ASSERT(_tcslen(lpszFile) < FILEPATH_MAX);
	_tcscpy(m_MRUFile[nType][0], lpszFile);

	// ���X�V
	if (m_MRUNum[nType] < 9) {
		m_MRUNum[nType]++;
	}
}

//---------------------------------------------------------------------------
//
//	MRU�擾
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::GetMRUFile(int nType, int nIndex, LPTSTR lpszFile) const
{
	ASSERT(this);
	ASSERT((nType >= 0) && (nType < MruTypes));
	ASSERT((nIndex >= 0) && (nIndex < 9));
	ASSERT(lpszFile);

	// ���ȏ�Ȃ�\0
	if (nIndex >= m_MRUNum[nType]) {
		lpszFile[0] = _T('\0');
		return;
	}

	// �R�s�[
	ASSERT(_tcslen(m_MRUFile[nType][nIndex]) < FILEPATH_MAX);
	_tcscpy(lpszFile, m_MRUFile[nType][nIndex]);
}

//---------------------------------------------------------------------------
//
//	MRU���擾
//
//---------------------------------------------------------------------------
int FASTCALL CConfig::GetMRUNum(int nType) const
{
	ASSERT(this);
	ASSERT((nType >= 0) && (nType < MruTypes));

	return m_MRUNum[nType];
}

//---------------------------------------------------------------------------
//
//	�L�[���[�h
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::LoadKey() const
{
	DWORD dwMap[0x100];
//	CInput *pInput;
	CString strName;
	int i;
	int nValue;
	BOOL bFlag;

	ASSERT(this);

	// �C���v�b�g�擾
//	pInput = m_pFrmWnd->GetInput();
//	ASSERT(pInput);

	// �t���OOFF(�L���f�[�^�Ȃ�)�A�N���A
	bFlag = FALSE;
	memset(dwMap, 0, sizeof(dwMap));

	// ���[�v
	for (i=0; i<0x100; i++) {
		strName.Format(_T("Key%d"), i);
		nValue = ::GetPrivateProfileInt(_T("Keyboard"), strName, 0, m_IniFile);

		// �l���͈͓��Ɏ��܂��Ă��Ȃ���΁A�����őł��؂�(�f�t�H���g�l���g��)
		if ((nValue < 0) || (nValue > 0x73)) {
			return;
		}

		// �l������΃Z�b�g���āA�t���O���Ă�
		if (nValue != 0) {
			dwMap[i] = nValue;
			bFlag = TRUE;
		}
	}

	// �t���O�������Ă���΁A�}�b�v�f�[�^�ݒ�
	if (bFlag) {
//		pInput->SetKeyMap(dwMap);
	}
}

//---------------------------------------------------------------------------
//
//	�L�[�Z�[�u
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SaveKey() const
{
	DWORD dwMap[0x100];
//	CInput *pInput;
	CString strName;
	CString strKey;
	int i;

	ASSERT(this);

	// �C���v�b�g�擾
//	pInput = m_pFrmWnd->GetInput();
//	ASSERT(pInput);

	// �}�b�v�f�[�^�擾
//	pInput->GetKeyMap(dwMap);

	// ���[�v
	for (i=0; i<0x100; i++) {
		// ���ׂ�(256���)����
		strName.Format(_T("Key%d"), i);
		strKey.Format(_T("%d"), dwMap[i]);
		::WritePrivateProfileString(_T("Keyboard"), strName,
									strKey, m_IniFile);
	}
}
//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL CConfig::Save(Fileio *pFio, int )
{
	size_t sz;

	ASSERT(this);
	ASSERT(pFio);

	// �T�C�Y���Z�[�u
	sz = sizeof(m_Config);
	if (!pFio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// �{�̂��Z�[�u
	if (!pFio->Write(&m_Config, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL CConfig::Load(Fileio *pFio, int nVer)
{
	size_t sz;

	ASSERT(this);
	ASSERT(pFio);

	// �ȑO�̃o�[�W�����Ƃ̌݊�
	if (nVer <= 0x0201) {
		return Load200(pFio);
	}
	if (nVer <= 0x0203) {
		return Load202(pFio);
	}

	// �T�C�Y�����[�h�A�ƍ�
	if (!pFio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(m_Config)) {
		return FALSE;
	}

	// �{�̂����[�h
	if (!pFio->Read(&m_Config, (int)sz)) {
		return FALSE;
	}

	// ApplyCfg�v���t���O���グ��
	m_bApply = TRUE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h(version2.00)
//
//---------------------------------------------------------------------------
BOOL FASTCALL CConfig::Load200(Fileio *pFio)
{
	int i;
	size_t sz;
	Config200 *pConfig200;

	ASSERT(this);
	ASSERT(pFio);

	// �L���X�g���āAversion2.00�����������[�h�ł���悤�ɂ���
	pConfig200 = (Config200*)&m_Config;

	// �T�C�Y�����[�h�A�ƍ�
	if (!pFio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(Config200)) {
		return FALSE;
	}

	// �{�̂����[�h
	if (!pFio->Read(pConfig200, (int)sz)) {
		return FALSE;
	}

	// �V�K����(Config202)��������
	m_Config.mem_type = 1;
	m_Config.scsi_ilevel = 1;
	m_Config.scsi_drives = 0;
	m_Config.scsi_sramsync = TRUE;
	m_Config.scsi_mofirst = FALSE;
	for (i=0; i<5; i++) {
		m_Config.scsi_file[i][0] = _T('\0');
	}

	// �V�K����(Config204)��������
	m_Config.windrv_enable = 0;
	m_Config.resume_fd = FALSE;
	m_Config.resume_mo = FALSE;
	m_Config.resume_cd = FALSE;
	m_Config.resume_state = FALSE;
	m_Config.resume_screen = FALSE;
	m_Config.resume_dir = FALSE;

	// ApplyCfg�v���t���O���グ��
	m_bApply = TRUE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h(version2.02)
//
//---------------------------------------------------------------------------
BOOL FASTCALL CConfig::Load202(Fileio *pFio)
{
	size_t sz;
	Config202 *pConfig202;

	ASSERT(this);
	ASSERT(pFio);

	// �L���X�g���āAversion2.02�����������[�h�ł���悤�ɂ���
	pConfig202 = (Config202*)&m_Config;

	// �T�C�Y�����[�h�A�ƍ�
	if (!pFio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(Config202)) {
		return FALSE;
	}

	// �{�̂����[�h
	if (!pFio->Read(pConfig202, (int)sz)) {
		return FALSE;
	}

	// �V�K����(Config204)��������
	m_Config.windrv_enable = 0;
	m_Config.resume_fd = FALSE;
	m_Config.resume_mo = FALSE;
	m_Config.resume_cd = FALSE;
	m_Config.resume_state = FALSE;
	m_Config.resume_screen = FALSE;
	m_Config.resume_dir = FALSE;

	// ApplyCfg�v���t���O���グ��
	m_bApply = TRUE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Apply�v���`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL CConfig::IsApply()
{
	ASSERT(this);

	// �v���Ȃ�A�����ŉ��낷
	if (m_bApply) {
		m_bApply = FALSE;
		return TRUE;
	}

	// �v�����Ă��Ȃ�
	return FALSE;
}

//===========================================================================
//
//	�R���t�B�O�v���p�e�B�y�[�W
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CConfigPage::CConfigPage()
{
	// �����o�ϐ��N���A
	m_dwID = 0;
	m_nTemplate = 0;
	m_uHelpID = 0;
	m_uMsgID = 0;
	m_pConfig = NULL;
	m_pSheet = NULL;
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CConfigPage, CPropertyPage)
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
void FASTCALL CConfigPage::Init(CConfigSheet *pSheet)
{
	int nID;

	ASSERT(this);
	ASSERT(m_dwID != 0);

	// �e�V�[�g�L��
	ASSERT(pSheet);
	m_pSheet = pSheet;

	// ID����
	nID = m_nTemplate;
	if (!::IsJapanese()) {
		nID += 50;
	}

	// �\�z
	CommonConstruct(MAKEINTRESOURCE(nID), 0);

	// �e�V�[�g�ɒǉ�
	pSheet->AddPage(this);
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL CConfigPage::OnInitDialog()
{
	CConfigSheet *pSheet;

	ASSERT(this);

	// �e�E�B���h�E����ݒ�f�[�^���󂯎��
	pSheet = (CConfigSheet*)GetParent();
	ASSERT(pSheet);
	m_pConfig = pSheet->m_pConfig;

	// ��{�N���X
	return CPropertyPage::OnInitDialog();
}

//---------------------------------------------------------------------------
//
//	�y�[�W�A�N�e�B�u
//
//---------------------------------------------------------------------------
BOOL CConfigPage::OnSetActive()
{
	CStatic *pStatic;
	CString strEmpty;

	ASSERT(this);

	// ��{�N���X
	if (!CPropertyPage::OnSetActive()) {
		return FALSE;
	}

	// �w���v������
	ASSERT(m_uHelpID > 0);
	m_uMsgID = 0;
	pStatic = (CStatic*)GetDlgItem(m_uHelpID);
	ASSERT(pStatic);
	strEmpty.Empty();
	pStatic->SetWindowText(strEmpty);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�}�E�X�J�[�\���ݒ�
//
//---------------------------------------------------------------------------
BOOL CConfigPage::OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT nMsg)
{
	CWnd *pChildWnd;
	CPoint pt;
	UINT nID;
	CRect rectParent;
	CRect rectChild;
	CString strText;
	CStatic *pStatic;

	// �w���v���w�肳��Ă��邱��
	ASSERT(this);
	ASSERT(m_uHelpID > 0);

	// �}�E�X�ʒu�擾
	GetCursorPos(&pt);

	// �q�E�B���h�E���܂���āA��`���Ɉʒu���邩���ׂ�
	nID = 0;
	rectParent.top = 0;
	pChildWnd = GetTopWindow();

	// ���[�v
	while (pChildWnd) {
		// �w���vID���g�Ȃ�X�L�b�v
		if (pChildWnd->GetDlgCtrlID() == (int)m_uHelpID) {
			pChildWnd = pChildWnd->GetNextWindow();
			continue;
		}

		// ��`���擾
		pChildWnd->GetWindowRect(&rectChild);

		// �����ɂ��邩
		if (rectChild.PtInRect(pt)) {
			// ���Ɏ擾������`������΁A�����������
			if (rectParent.top == 0) {
				// �ŏ��̌��
				rectParent = rectChild;
				nID = pChildWnd->GetDlgCtrlID();
			}
			else {
				if (rectChild.Width() < rectParent.Width()) {
					// �������̌��
					rectParent = rectChild;
					nID = pChildWnd->GetDlgCtrlID();
				}
			}
		}

		// ����
		pChildWnd = pChildWnd->GetNextWindow();
	}

	// nID���r
	if (m_uMsgID == nID) {
		// ��{�N���X
		return CPropertyPage::OnSetCursor(pWnd, nHitTest, nMsg);
	}
	m_uMsgID = nID;

	// ����������[�h�A�ݒ�
	::GetMsg(m_uMsgID, strText);
	pStatic = (CStatic*)GetDlgItem(m_uHelpID);
	ASSERT(pStatic);
	pStatic->SetWindowText(strText);

	// ��{�N���X
	return CPropertyPage::OnSetCursor(pWnd, nHitTest, nMsg);
}
//===========================================================================
//
//	�R���t�B�O�v���p�e�B�V�[�g
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CConfigSheet::CConfigSheet(CWnd *pParent) : CPropertySheet(IDS_OPTIONS, pParent)
{
	// ���̎��_�ł͐ݒ�f�[�^��NULL
	m_pConfig = NULL;

	// �p����ւ̑Ή�
	if (!::IsJapanese()) {
		::GetMsg(IDS_OPTIONS, m_strCaption);
	}

	// Apply�{�^�����폜
	m_psh.dwFlags |= PSH_NOAPPLYNOW;

	// �e�E�B���h�E���L��
	m_pFrmWnd = (CFrmWnd*)pParent;

	// �^�C�}�Ȃ�
	m_nTimerID = NULL;
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CConfigSheet, CPropertySheet)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	�y�[�W����
//
//---------------------------------------------------------------------------
CConfigPage* FASTCALL CConfigSheet::SearchPage(DWORD dwID) const
{
	int nPage;
	int nCount;
	CConfigPage *pPage;

	ASSERT(this);
	ASSERT(dwID != 0);

	// �y�[�W���擾
	nCount = GetPageCount();
	ASSERT(nCount >= 0);

	// �y�[�W���[�v
	for (nPage=0; nPage<nCount; nPage++) {
		// �y�[�W�擾
		pPage = (CConfigPage*)GetPage(nPage);
		ASSERT(pPage);

		// ID�`�F�b�N
		if (pPage->GetID() == dwID) {
			return pPage;
		}
	}

	// ������Ȃ�����
	return NULL;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�쐬
//
//---------------------------------------------------------------------------
int CConfigSheet::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	// ��{�N���X
	if (CPropertySheet::OnCreate(lpCreateStruct) != 0) {
		return -1;
	}

	// �^�C�}���C���X�g�[��
	m_nTimerID = SetTimer(IDM_OPTIONS, 100, NULL);

	return 0;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�폜
//
//---------------------------------------------------------------------------
void CConfigSheet::OnDestroy()
{
	// �^�C�}��~
	if (m_nTimerID) {
		KillTimer(m_nTimerID);
		m_nTimerID = NULL;
	}

	// ��{�N���X
	CPropertySheet::OnDestroy();
}

//---------------------------------------------------------------------------
//
//	�^�C�}
//
//---------------------------------------------------------------------------
#if _MFC_VER >= 0x700
void CConfigSheet::OnTimer(UINT_PTR nID)
#else
void CConfigSheet::OnTimer(UINT nID)
#endif
{
	ASSERT(m_pFrmWnd);

	// ID�`�F�b�N
	if (m_nTimerID != nID) {
		return;
	}

	// �^�C�}��~
	KillTimer(m_nTimerID);
	m_nTimerID = NULL;

	// �^�C�}�ĊJ(�\����������100ms������)
	m_nTimerID = SetTimer(IDM_OPTIONS, 100, NULL);
}

#endif	// _WIN32
