//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC コンフィグ ]
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
//	コンフィグ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CConfig::CConfig(CFrmWnd *pWnd) : CComponent(pWnd)
{
	// コンポーネントパラメータ
	m_dwID = MAKEID('C', 'F', 'G', ' ');
	m_strDesc = _T("Config Manager");
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CConfig::Init()
{
	int i;
	Filepath path;

	ASSERT(this);

	// 基本クラス
	if (!CComponent::Init()) {
		return FALSE;
	}

	// INIファイルパス決定
	path.SetPath(_T("XM6.ini"));
	path.SetBaseFile();
	_tcscpy(m_IniFile, path.GetPath());

	// 設定データ
	LoadConfig();

	// 互換性維持
	ResetSASI();
	ResetCDROM();

	// MRU
	for (i=0; i<MruTypes; i++) {
		ClearMRU(i);
		LoadMRU(i);
	}

	// キー
	LoadKey();

	// TrueKey
//	LoadTKey();

	// セーブ・ロード
	m_bApply = FALSE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::Cleanup()
{
	int i;

	ASSERT(this);

	// 設定データ
	SaveConfig();

	// MRU
	for (i=0; i<MruTypes; i++) {
		SaveMRU(i);
	}

	// キー
	SaveKey();

	// TrueKey
//	SaveTKey();

	// 基本クラス
	CComponent::Cleanup();
}

//---------------------------------------------------------------------------
//
//	設定データ実体
//
//---------------------------------------------------------------------------
Config CConfig::m_Config;

//---------------------------------------------------------------------------
//
//	設定データ取得
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::GetConfig(Config *pConfigBuf) const
{
	ASSERT(this);
	ASSERT(pConfigBuf);

	// 内部ワークをコピー
	*pConfigBuf = m_Config;
}

//---------------------------------------------------------------------------
//
//	設定データ設定
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SetConfig(Config *pConfigBuf)
{
	ASSERT(this);
	ASSERT(pConfigBuf);

	// 内部ワークへコピー
	m_Config = *pConfigBuf;
}

//---------------------------------------------------------------------------
//
//	画面拡大設定
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SetStretch(BOOL bStretch)
{
	ASSERT(this);

	m_Config.aspect_stretch = bStretch;
}

//---------------------------------------------------------------------------
//
//	MIDIデバイス設定
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SetMIDIDevice(int nDevice, BOOL bIn)
{
	ASSERT(this);
	ASSERT(nDevice >= 0);

	// InまたはOut
	if (bIn) {
		m_Config.midiin_device = nDevice;
	}
	else {
		m_Config.midiout_device = nDevice;
	}
}

//---------------------------------------------------------------------------
//
//	INIファイルテーブル
//	※ポインタ・セクション名・キー名・型・デフォルト値・最小値・最大値の順
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
//	設定データロード
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

	// テーブルの先頭に合わせる
	pIni = (const PINIKEY)&IniTable[0];
	pszSection = NULL;
	szDef[0] = _T('\0');

	// テーブルループ
	while (pIni->pBuf) {
		// セクション設定
		if (pIni->pszSection) {
			pszSection = pIni->pszSection;
		}
		ASSERT(pszSection);

		// タイプチェック
		switch (pIni->nType) {
			// 整数型(範囲を超えたらデフォルト値)
			case 0:
				nValue = ::GetPrivateProfileInt(pszSection, pIni->pszKey, pIni->nDef, m_IniFile);
				if ((nValue < pIni->nMin) || (pIni->nMax < nValue)) {
					nValue = pIni->nDef;
				}
				*((int*)pIni->pBuf) = nValue;
				break;

			// 論理型(0,1のどちらでもなければデフォルト値)
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

			// 文字列型(バッファサイズ範囲内でのターミネートを保証)
			case 2:
				ASSERT(pIni->nDef <= (sizeof(szBuf)/sizeof(TCHAR)));
				::GetPrivateProfileString(pszSection, pIni->pszKey, szDef, szBuf,
										sizeof(szBuf)/sizeof(TCHAR), m_IniFile);

				// デフォルト値にはバッファサイズを記入すること
				ASSERT(pIni->nDef > 0);
				szBuf[pIni->nDef - 1] = _T('\0');
				_tcscpy((LPTSTR)pIni->pBuf, szBuf);
				break;

			// その他
			default:
				ASSERT(FALSE);
				break;
		}

		// 次へ
		pIni++;
	}
}

//---------------------------------------------------------------------------
//
//	設定データセーブ
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SaveConfig() const
{
	PINIKEY pIni;
	CString string;
	LPCTSTR pszSection;

	ASSERT(this);

	// テーブルの先頭に合わせる
	pIni = (const PINIKEY)&IniTable[0];
	pszSection = NULL;

	// テーブルループ
	while (pIni->pBuf) {
		// セクション設定
		if (pIni->pszSection) {
			pszSection = pIni->pszSection;
		}
		ASSERT(pszSection);

		// タイプチェック
		switch (pIni->nType) {
			// 整数型
			case 0:
				string.Format(_T("%d"), *((int*)pIni->pBuf));
				::WritePrivateProfileString(pszSection, pIni->pszKey,
											string, m_IniFile);
				break;

			// 論理型
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

			// 文字列型
			case 2:
				::WritePrivateProfileString(pszSection, pIni->pszKey,
											(LPCTSTR)pIni->pBuf, m_IniFile);
				break;

			// その他
			default:
				ASSERT(FALSE);
				break;
		}

		// 次へ
		pIni++;
	}
}

//---------------------------------------------------------------------------
//
//	SASIリセット
//	※version1.44までは自動ファイル検索のため、ここで再検索と設定を行い
//	version1.45以降への移行をスムーズに行う
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::ResetSASI()
{
	int i;
	Filepath path;
	Fileio fio;
	TCHAR szPath[FILEPATH_MAX];

	ASSERT(this);

	// ドライブ数>=0の場合は不要(設定済み)
	if (m_Config.sasi_drives >= 0) {
		return;
	}

	// ドライブ数0
	m_Config.sasi_drives = 0;

	// ファイル名作成ループ
	for (i=0; i<16; i++) {
		_stprintf(szPath, _T("HD%d.HDF"), i);
		path.SetPath(szPath);
		path.SetBaseDir();
		_tcscpy(m_Config.sasi_file[i], path.GetPath());
	}

	// 最初からチェックして、有効なドライブ数を決める
	for (i=0; i<16; i++) {
		path.SetPath(m_Config.sasi_file[i]);
		if (!fio.Open(path, Fileio::ReadOnly)) {
			return;
		}

		// サイズチェック(version1.44では40MBドライブのみサポート)
		if (fio.GetFileSize() != 0x2793000) {
			fio.Close();
			return;
		}

		// カウントアップとクローズ
		m_Config.sasi_drives++;
		fio.Close();
	}
}

//---------------------------------------------------------------------------
//
//	CD-ROMリセット
//	※version2.02まではCD-ROM未サポートのため、SCSIドライブ数を+1する
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::ResetCDROM()
{
	ASSERT(this);

	// CD-ROMフラグがセットされている場合は不要(設定済み)
	if (m_bCDROM) {
		return;
	}

	// CD-ROMフラグをセット
	m_bCDROM = TRUE;

	// SCSIドライブ数が3以上6以下の場合に限り、+1
	if ((m_Config.scsi_drives >= 3) && (m_Config.scsi_drives <= 6)) {
		m_Config.scsi_drives++;
	}
}

//---------------------------------------------------------------------------
//
//	CD-ROMフラグ
//
//---------------------------------------------------------------------------
BOOL CConfig::m_bCDROM;

//---------------------------------------------------------------------------
//
//	MRUクリア
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::ClearMRU(int nType)
{
	int i;

	ASSERT(this);
	ASSERT((nType >= 0) && (nType < MruTypes));

	// 実体クリア
	for (i=0; i<9; i++) {
		memset(m_MRUFile[nType][i], 0, FILEPATH_MAX * sizeof(TCHAR));
	}

	// 個数クリア
	m_MRUNum[nType] = 0;
}

//---------------------------------------------------------------------------
//
//	MRUロード
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::LoadMRU(int nType)
{
	CString strSection;
	CString strKey;
	int i;

	ASSERT(this);
	ASSERT((nType >= 0) && (nType < MruTypes));

	// セクション作成
	strSection.Format(_T("MRU%d"), nType);

	// ループ
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
//	MRUセーブ
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SaveMRU(int nType) const
{
	CString strSection;
	CString strKey;
	int i;

	ASSERT(this);
	ASSERT((nType >= 0) && (nType < MruTypes));

	// セクション作成
	strSection.Format(_T("MRU%d"), nType);

	// ループ
	for (i=0; i<9; i++) {
		strKey.Format(_T("File%d"), i);
		::WritePrivateProfileString(strSection, strKey, m_MRUFile[nType][i],
								m_IniFile);
	}
}

//---------------------------------------------------------------------------
//
//	MRUセット
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

	// 既に同じものがないか
	nNum = GetMRUNum(nType);
	for (nMRU=0; nMRU<nNum; nMRU++) {
		if (_tcscmp(m_MRUFile[nType][nMRU], lpszFile) == 0) {
			// 先頭にあって、また同じものを追加しようとした
			if (nMRU == 0) {
				return;
			}

			// コピー
			for (nCpy=nMRU; nCpy>=1; nCpy--) {
				memcpy(m_MRUFile[nType][nCpy], m_MRUFile[nType][nCpy - 1],
						FILEPATH_MAX * sizeof(TCHAR));
			}

			// 先頭にセット
			_tcscpy(m_MRUFile[nType][0], lpszFile);
			return;
		}
	}

	// 移動
	for (nMRU=7; nMRU>=0; nMRU--) {
		memcpy(m_MRUFile[nType][nMRU + 1], m_MRUFile[nType][nMRU],
				FILEPATH_MAX * sizeof(TCHAR));
	}

	// 先頭にセット
	ASSERT(_tcslen(lpszFile) < FILEPATH_MAX);
	_tcscpy(m_MRUFile[nType][0], lpszFile);

	// 個数更新
	if (m_MRUNum[nType] < 9) {
		m_MRUNum[nType]++;
	}
}

//---------------------------------------------------------------------------
//
//	MRU取得
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::GetMRUFile(int nType, int nIndex, LPTSTR lpszFile) const
{
	ASSERT(this);
	ASSERT((nType >= 0) && (nType < MruTypes));
	ASSERT((nIndex >= 0) && (nIndex < 9));
	ASSERT(lpszFile);

	// 個数以上なら\0
	if (nIndex >= m_MRUNum[nType]) {
		lpszFile[0] = _T('\0');
		return;
	}

	// コピー
	ASSERT(_tcslen(m_MRUFile[nType][nIndex]) < FILEPATH_MAX);
	_tcscpy(lpszFile, m_MRUFile[nType][nIndex]);
}

//---------------------------------------------------------------------------
//
//	MRU個数取得
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
//	キーロード
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

	// インプット取得
//	pInput = m_pFrmWnd->GetInput();
//	ASSERT(pInput);

	// フラグOFF(有効データなし)、クリア
	bFlag = FALSE;
	memset(dwMap, 0, sizeof(dwMap));

	// ループ
	for (i=0; i<0x100; i++) {
		strName.Format(_T("Key%d"), i);
		nValue = ::GetPrivateProfileInt(_T("Keyboard"), strName, 0, m_IniFile);

		// 値が範囲内に収まっていなければ、ここで打ち切る(デフォルト値を使う)
		if ((nValue < 0) || (nValue > 0x73)) {
			return;
		}

		// 値があればセットして、フラグ立てる
		if (nValue != 0) {
			dwMap[i] = nValue;
			bFlag = TRUE;
		}
	}

	// フラグが立っていれば、マップデータ設定
	if (bFlag) {
//		pInput->SetKeyMap(dwMap);
	}
}

//---------------------------------------------------------------------------
//
//	キーセーブ
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

	// インプット取得
//	pInput = m_pFrmWnd->GetInput();
//	ASSERT(pInput);

	// マップデータ取得
//	pInput->GetKeyMap(dwMap);

	// ループ
	for (i=0; i<0x100; i++) {
		// すべて(256種類)書く
		strName.Format(_T("Key%d"), i);
		strKey.Format(_T("%d"), dwMap[i]);
		::WritePrivateProfileString(_T("Keyboard"), strName,
									strKey, m_IniFile);
	}
}
//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL CConfig::Save(Fileio *pFio, int )
{
	size_t sz;

	ASSERT(this);
	ASSERT(pFio);

	// サイズをセーブ
	sz = sizeof(m_Config);
	if (!pFio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 本体をセーブ
	if (!pFio->Write(&m_Config, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL CConfig::Load(Fileio *pFio, int nVer)
{
	size_t sz;

	ASSERT(this);
	ASSERT(pFio);

	// 以前のバージョンとの互換
	if (nVer <= 0x0201) {
		return Load200(pFio);
	}
	if (nVer <= 0x0203) {
		return Load202(pFio);
	}

	// サイズをロード、照合
	if (!pFio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(m_Config)) {
		return FALSE;
	}

	// 本体をロード
	if (!pFio->Read(&m_Config, (int)sz)) {
		return FALSE;
	}

	// ApplyCfg要求フラグを上げる
	m_bApply = TRUE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード(version2.00)
//
//---------------------------------------------------------------------------
BOOL FASTCALL CConfig::Load200(Fileio *pFio)
{
	int i;
	size_t sz;
	Config200 *pConfig200;

	ASSERT(this);
	ASSERT(pFio);

	// キャストして、version2.00部分だけロードできるようにする
	pConfig200 = (Config200*)&m_Config;

	// サイズをロード、照合
	if (!pFio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(Config200)) {
		return FALSE;
	}

	// 本体をロード
	if (!pFio->Read(pConfig200, (int)sz)) {
		return FALSE;
	}

	// 新規項目(Config202)を初期化
	m_Config.mem_type = 1;
	m_Config.scsi_ilevel = 1;
	m_Config.scsi_drives = 0;
	m_Config.scsi_sramsync = TRUE;
	m_Config.scsi_mofirst = FALSE;
	for (i=0; i<5; i++) {
		m_Config.scsi_file[i][0] = _T('\0');
	}

	// 新規項目(Config204)を初期化
	m_Config.windrv_enable = 0;
	m_Config.resume_fd = FALSE;
	m_Config.resume_mo = FALSE;
	m_Config.resume_cd = FALSE;
	m_Config.resume_state = FALSE;
	m_Config.resume_screen = FALSE;
	m_Config.resume_dir = FALSE;

	// ApplyCfg要求フラグを上げる
	m_bApply = TRUE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード(version2.02)
//
//---------------------------------------------------------------------------
BOOL FASTCALL CConfig::Load202(Fileio *pFio)
{
	size_t sz;
	Config202 *pConfig202;

	ASSERT(this);
	ASSERT(pFio);

	// キャストして、version2.02部分だけロードできるようにする
	pConfig202 = (Config202*)&m_Config;

	// サイズをロード、照合
	if (!pFio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(Config202)) {
		return FALSE;
	}

	// 本体をロード
	if (!pFio->Read(pConfig202, (int)sz)) {
		return FALSE;
	}

	// 新規項目(Config204)を初期化
	m_Config.windrv_enable = 0;
	m_Config.resume_fd = FALSE;
	m_Config.resume_mo = FALSE;
	m_Config.resume_cd = FALSE;
	m_Config.resume_state = FALSE;
	m_Config.resume_screen = FALSE;
	m_Config.resume_dir = FALSE;

	// ApplyCfg要求フラグを上げる
	m_bApply = TRUE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Apply要求チェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL CConfig::IsApply()
{
	ASSERT(this);

	// 要求なら、ここで下ろす
	if (m_bApply) {
		m_bApply = FALSE;
		return TRUE;
	}

	// 要求していない
	return FALSE;
}

//===========================================================================
//
//	コンフィグプロパティページ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CConfigPage::CConfigPage()
{
	// メンバ変数クリア
	m_dwID = 0;
	m_nTemplate = 0;
	m_uHelpID = 0;
	m_uMsgID = 0;
	m_pConfig = NULL;
	m_pSheet = NULL;
}

//---------------------------------------------------------------------------
//
//	メッセージ マップ
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CConfigPage, CPropertyPage)
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
void FASTCALL CConfigPage::Init(CConfigSheet *pSheet)
{
	int nID;

	ASSERT(this);
	ASSERT(m_dwID != 0);

	// 親シート記憶
	ASSERT(pSheet);
	m_pSheet = pSheet;

	// ID決定
	nID = m_nTemplate;
	if (!::IsJapanese()) {
		nID += 50;
	}

	// 構築
	CommonConstruct(MAKEINTRESOURCE(nID), 0);

	// 親シートに追加
	pSheet->AddPage(this);
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL CConfigPage::OnInitDialog()
{
	CConfigSheet *pSheet;

	ASSERT(this);

	// 親ウィンドウから設定データを受け取る
	pSheet = (CConfigSheet*)GetParent();
	ASSERT(pSheet);
	m_pConfig = pSheet->m_pConfig;

	// 基本クラス
	return CPropertyPage::OnInitDialog();
}

//---------------------------------------------------------------------------
//
//	ページアクティブ
//
//---------------------------------------------------------------------------
BOOL CConfigPage::OnSetActive()
{
	CStatic *pStatic;
	CString strEmpty;

	ASSERT(this);

	// 基本クラス
	if (!CPropertyPage::OnSetActive()) {
		return FALSE;
	}

	// ヘルプ初期化
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
//	マウスカーソル設定
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

	// ヘルプが指定されていること
	ASSERT(this);
	ASSERT(m_uHelpID > 0);

	// マウス位置取得
	GetCursorPos(&pt);

	// 子ウィンドウをまわって、矩形内に位置するか調べる
	nID = 0;
	rectParent.top = 0;
	pChildWnd = GetTopWindow();

	// ループ
	while (pChildWnd) {
		// ヘルプID自身ならスキップ
		if (pChildWnd->GetDlgCtrlID() == (int)m_uHelpID) {
			pChildWnd = pChildWnd->GetNextWindow();
			continue;
		}

		// 矩形を取得
		pChildWnd->GetWindowRect(&rectChild);

		// 内部にいるか
		if (rectChild.PtInRect(pt)) {
			// 既に取得した矩形があれば、それより内側か
			if (rectParent.top == 0) {
				// 最初の候補
				rectParent = rectChild;
				nID = pChildWnd->GetDlgCtrlID();
			}
			else {
				if (rectChild.Width() < rectParent.Width()) {
					// より内側の候補
					rectParent = rectChild;
					nID = pChildWnd->GetDlgCtrlID();
				}
			}
		}

		// 次へ
		pChildWnd = pChildWnd->GetNextWindow();
	}

	// nIDを比較
	if (m_uMsgID == nID) {
		// 基本クラス
		return CPropertyPage::OnSetCursor(pWnd, nHitTest, nMsg);
	}
	m_uMsgID = nID;

	// 文字列をロード、設定
	::GetMsg(m_uMsgID, strText);
	pStatic = (CStatic*)GetDlgItem(m_uHelpID);
	ASSERT(pStatic);
	pStatic->SetWindowText(strText);

	// 基本クラス
	return CPropertyPage::OnSetCursor(pWnd, nHitTest, nMsg);
}
//===========================================================================
//
//	コンフィグプロパティシート
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CConfigSheet::CConfigSheet(CWnd *pParent) : CPropertySheet(IDS_OPTIONS, pParent)
{
	// この時点では設定データはNULL
	m_pConfig = NULL;

	// 英語環境への対応
	if (!::IsJapanese()) {
		::GetMsg(IDS_OPTIONS, m_strCaption);
	}

	// Applyボタンを削除
	m_psh.dwFlags |= PSH_NOAPPLYNOW;

	// 親ウィンドウを記憶
	m_pFrmWnd = (CFrmWnd*)pParent;

	// タイマなし
	m_nTimerID = NULL;
}

//---------------------------------------------------------------------------
//
//	メッセージ マップ
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CConfigSheet, CPropertySheet)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	ページ検索
//
//---------------------------------------------------------------------------
CConfigPage* FASTCALL CConfigSheet::SearchPage(DWORD dwID) const
{
	int nPage;
	int nCount;
	CConfigPage *pPage;

	ASSERT(this);
	ASSERT(dwID != 0);

	// ページ数取得
	nCount = GetPageCount();
	ASSERT(nCount >= 0);

	// ページループ
	for (nPage=0; nPage<nCount; nPage++) {
		// ページ取得
		pPage = (CConfigPage*)GetPage(nPage);
		ASSERT(pPage);

		// IDチェック
		if (pPage->GetID() == dwID) {
			return pPage;
		}
	}

	// 見つからなかった
	return NULL;
}

//---------------------------------------------------------------------------
//
//	ウィンドウ作成
//
//---------------------------------------------------------------------------
int CConfigSheet::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	// 基本クラス
	if (CPropertySheet::OnCreate(lpCreateStruct) != 0) {
		return -1;
	}

	// タイマをインストール
	m_nTimerID = SetTimer(IDM_OPTIONS, 100, NULL);

	return 0;
}

//---------------------------------------------------------------------------
//
//	ウィンドウ削除
//
//---------------------------------------------------------------------------
void CConfigSheet::OnDestroy()
{
	// タイマ停止
	if (m_nTimerID) {
		KillTimer(m_nTimerID);
		m_nTimerID = NULL;
	}

	// 基本クラス
	CPropertySheet::OnDestroy();
}

//---------------------------------------------------------------------------
//
//	タイマ
//
//---------------------------------------------------------------------------
#if _MFC_VER >= 0x700
void CConfigSheet::OnTimer(UINT_PTR nID)
#else
void CConfigSheet::OnTimer(UINT nID)
#endif
{
	ASSERT(m_pFrmWnd);

	// IDチェック
	if (m_nTimerID != nID) {
		return;
	}

	// タイマ停止
	KillTimer(m_nTimerID);
	m_nTimerID = NULL;

	// タイマ再開(表示完了から100msあける)
	m_nTimerID = SetTimer(IDM_OPTIONS, 100, NULL);
}

#endif	// _WIN32
