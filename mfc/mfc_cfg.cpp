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
//	基本ページ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CBasicPage::CBasicPage()
{
	// ID,Helpを必ず設定
	m_dwID = MAKEID('B', 'A', 'S', 'C');
	m_nTemplate = IDD_BASICPAGE;
	m_uHelpID = IDC_BASIC_HELP;
}

//---------------------------------------------------------------------------
//
//	メッセージ マップ
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CBasicPage, CConfigPage)
	ON_BN_CLICKED(IDC_BASIC_CPUFULLB, OnMPUFull)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL CBasicPage::OnInitDialog()
{
	CString string;
	CButton *pButton;
	CComboBox *pComboBox;
	int i;

	// 基本クラス
	CConfigPage::OnInitDialog();

	// システムクロック
	pComboBox = (CComboBox*)GetDlgItem(IDC_BASIC_CLOCKC);
	ASSERT(pComboBox);
	for (i=0; i<6; i++) {
		::GetMsg((IDS_BASIC_CLOCK0 + i), string);
		pComboBox->AddString(string);
	}
	pComboBox->SetCurSel(m_pConfig->system_clock);

	// MPUフルスピード
	pButton = (CButton*)GetDlgItem(IDC_BASIC_CPUFULLB);
	ASSERT(pButton);
	pButton->SetCheck(m_pConfig->mpu_fullspeed);

	// VMフルスピード
	pButton = (CButton*)GetDlgItem(IDC_BASIC_ALLFULLB);
	ASSERT(pButton);
	pButton->SetCheck(m_pConfig->vm_fullspeed);

	// メインメモリ
	pComboBox = (CComboBox*)GetDlgItem(IDC_BASIC_MEMORYC);
	ASSERT(pComboBox);
	for (i=0; i<6; i++) {
		::GetMsg((IDS_BASIC_MEMORY0 + i), string);
		pComboBox->AddString(string);
	}
	pComboBox->SetCurSel(m_pConfig->ram_size);

	// SRAM同期
	pButton = (CButton*)GetDlgItem(IDC_BASIC_MEMSWB);
	ASSERT(pButton);
	pButton->SetCheck(m_pConfig->ram_sramsync);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	決定
//
//---------------------------------------------------------------------------
void CBasicPage::OnOK()
{
	CButton *pButton;
	CComboBox *pComboBox;

	// システムクロック
	pComboBox = (CComboBox*)GetDlgItem(IDC_BASIC_CLOCKC);
	ASSERT(pComboBox);
	m_pConfig->system_clock = pComboBox->GetCurSel();

	// MPUフルスピード
	pButton = (CButton*)GetDlgItem(IDC_BASIC_CPUFULLB);
	ASSERT(pButton);
	m_pConfig->mpu_fullspeed = pButton->GetCheck();

	// VMフルスピード
	pButton = (CButton*)GetDlgItem(IDC_BASIC_ALLFULLB);
	ASSERT(pButton);
	m_pConfig->vm_fullspeed = pButton->GetCheck();

	// メインメモリ
	pComboBox = (CComboBox*)GetDlgItem(IDC_BASIC_MEMORYC);
	ASSERT(pComboBox);
	m_pConfig->ram_size = pComboBox->GetCurSel();

	// SRAM同期
	pButton = (CButton*)GetDlgItem(IDC_BASIC_MEMSWB);
	ASSERT(pButton);
	m_pConfig->ram_sramsync = pButton->GetCheck();

	// 基本クラス
	CConfigPage::OnOK();
}

//---------------------------------------------------------------------------
//
//	MPUフルスピード
//
//---------------------------------------------------------------------------
void CBasicPage::OnMPUFull()
{
	CSxSIPage *pSxSIPage;
	CButton *pButton;
	CString strWarn;

	// ボタン取得
	pButton = (CButton*)GetDlgItem(IDC_BASIC_CPUFULLB);
	ASSERT(pButton);

	// オフなら何もしない
	if (pButton->GetCheck() == 0) {
		return;
	}

	// SxSI無効なら何もしない
	pSxSIPage = (CSxSIPage*)m_pSheet->SearchPage(MAKEID('S', 'X', 'S', 'I'));
	ASSERT(pSxSIPage);
	if (pSxSIPage->GetDrives(m_pConfig) == 0) {
		return;
	}

	// 警告
	::GetMsg(IDS_MPUSXSI, strWarn);
	MessageBox(strWarn, NULL, MB_ICONINFORMATION | MB_OK);
}

//===========================================================================
//
//	サウンドページ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CSoundPage::CSoundPage()
{
	// ID,Helpを必ず設定
	m_dwID = MAKEID('S', 'N', 'D', ' ');
	m_nTemplate = IDD_SOUNDPAGE;
	m_uHelpID = IDC_SOUND_HELP;
}

//---------------------------------------------------------------------------
//
//	メッセージ マップ
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CSoundPage, CConfigPage)
	ON_WM_VSCROLL()
	ON_CBN_SELCHANGE(IDC_SOUND_DEVICEC, OnSelChange)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL CSoundPage::OnInitDialog()
{
	CFrmWnd *pFrmWnd;
//	CSound *pSound;
	CComboBox *pComboBox;
	CButton *pButton;
	CEdit *pEdit;
	CSpinButtonCtrl *pSpin;
	CString strName;
	CString strEdit;
	int i;

	// 基本クラス
	CConfigPage::OnInitDialog();

	// サウンドコンポーネントを取得
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrmWnd);
//	pSound = pFrmWnd->GetSound();
//	ASSERT(pSound);

	// デバイスコンボボックス初期化
	pComboBox = (CComboBox*)GetDlgItem(IDC_SOUND_DEVICEC);
	ASSERT(pComboBox);
	pComboBox->ResetContent();
	::GetMsg(IDS_SOUND_NOASSIGN, strName);
	pComboBox->AddString(strName);
//	for (i=0; i<pSound->m_nDeviceNum; i++) {
//		pComboBox->AddString(pSound->m_DeviceDescr[i]);
//	}

	// コンボボックスのカーソル位置
	if (m_pConfig->sample_rate == 0) {
		pComboBox->SetCurSel(0);
	}
	else {
//		if (pSound->m_nDeviceNum <= m_pConfig->sound_device) {
//			pComboBox->SetCurSel(0);
//		}
//		else
		{
			pComboBox->SetCurSel(m_pConfig->sound_device + 1);
		}
	}

	// サンプリングレート初期化
	for (i=0; i<5; i++) {
		pButton = (CButton*)GetDlgItem(IDC_SOUND_RATE0 + i);
		ASSERT(pButton);
		pButton->SetCheck(0);
	}
	if (m_pConfig->sample_rate > 0) {
		pButton = (CButton*)GetDlgItem(IDC_SOUND_RATE0 + m_pConfig->sample_rate - 1);
		ASSERT(pButton);
		pButton->SetCheck(1);
	}

	// バッファサイズ初期化
	pEdit = (CEdit*)GetDlgItem(IDC_SOUND_BUF1E);
	ASSERT(pEdit);
	strEdit.Format(_T("%d"), m_pConfig->primary_buffer * 10);
	pEdit->SetWindowText(strEdit);
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SOUND_BUF1S);
	pSpin->SetBase(10);
	pSpin->SetRange(2, 100);
	pSpin->SetPos(m_pConfig->primary_buffer);

	// ポーリング間隔初期化
	pEdit = (CEdit*)GetDlgItem(IDC_SOUND_BUF2E);
	ASSERT(pEdit);
	strEdit.Format(_T("%d"), m_pConfig->polling_buffer);
	pEdit->SetWindowText(strEdit);
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SOUND_BUF2S);
	pSpin->SetBase(10);
	pSpin->SetRange(1, 100);
	pSpin->SetPos(m_pConfig->polling_buffer);

	// ADPCM線形補間初期化
	pButton = (CButton*)GetDlgItem(IDC_SOUND_INTERP);
	ASSERT(pButton);
	pButton->SetCheck(m_pConfig->adpcm_interp);

	// コントロール有効・無効
	m_bEnableCtrl = TRUE;
	if (m_pConfig->sample_rate == 0) {
		EnableControls(FALSE);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	決定
//
//---------------------------------------------------------------------------
void CSoundPage::OnOK()
{
	CComboBox *pComboBox;
	CButton *pButton;
	CSpinButtonCtrl *pSpin;
	int i;

	// デバイス取得
	pComboBox = (CComboBox*)GetDlgItem(IDC_SOUND_DEVICEC);
	ASSERT(pComboBox);
	if (pComboBox->GetCurSel() == 0) {
		// デバイス選択なし
		m_pConfig->sample_rate = 0;
	}
	else {
		// デバイス選択あり
		m_pConfig->sound_device = pComboBox->GetCurSel() - 1;

		// サンプリングレート取得
		for (i=0; i<5; i++) {
			pButton = (CButton*)GetDlgItem(IDC_SOUND_RATE0 + i);
			ASSERT(pButton);
			if (pButton->GetCheck() == 1) {
				m_pConfig->sample_rate = i + 1;
				break;
			}
		}
	}

	// バッファ取得
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SOUND_BUF1S);
	ASSERT(pSpin);
	m_pConfig->primary_buffer = LOWORD(pSpin->GetPos());
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SOUND_BUF2S);
	ASSERT(pSpin);
	m_pConfig->polling_buffer = LOWORD(pSpin->GetPos());

	// ADPCM線形補間取得
	pButton = (CButton*)GetDlgItem(IDC_SOUND_INTERP);
	ASSERT(pButton);
	m_pConfig->adpcm_interp = pButton->GetCheck();

	// 基本クラス
	CConfigPage::OnOK();
}

//---------------------------------------------------------------------------
//
//	縦スクロール
//
//---------------------------------------------------------------------------
void CSoundPage::OnVScroll(UINT , UINT nPos, CScrollBar* pBar)
{
	CEdit *pEdit;
	CSpinButtonCtrl *pSpin;
	CString strEdit;

	// スピンコントロールと一致か
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SOUND_BUF1S);
	if ((CWnd*)pBar == (CWnd*)pSpin) {
		// エディットに反映
		pEdit = (CEdit*)GetDlgItem(IDC_SOUND_BUF1E);
		strEdit.Format(_T("%d"), nPos * 10);
		pEdit->SetWindowText(strEdit);
	}

	// スピンコントロールと一致か
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SOUND_BUF2S);
	if ((CWnd*)pBar == (CWnd*)pSpin) {
		// エディットに反映
		pEdit = (CEdit*)GetDlgItem(IDC_SOUND_BUF2E);
		strEdit.Format(_T("%d"), nPos);
		pEdit->SetWindowText(strEdit);
	}
}

//---------------------------------------------------------------------------
//
//	コンボボックス変更
//
//---------------------------------------------------------------------------
void CSoundPage::OnSelChange()
{
	int i;
	CComboBox *pComboBox;
	CButton *pButton;

	pComboBox = (CComboBox*)GetDlgItem(IDC_SOUND_DEVICEC);
	ASSERT(pComboBox);
	if (pComboBox->GetCurSel() == 0) {
		EnableControls(FALSE);
	}
	else {
		EnableControls(TRUE);
	}

	// サンプングレートの設定を考慮
	if (m_bEnableCtrl) {
		// 有効の場合、どれかにチェックがついていればよい
		for (i=0; i<5; i++) {
			pButton = (CButton*)GetDlgItem(IDC_SOUND_RATE0 + i);
			ASSERT(pButton);
			if (pButton->GetCheck() != 0) {
				return;
			}
		}

		// どれもチェックがついていないので、62.5kHzにチェック
		pButton = (CButton*)GetDlgItem(IDC_SOUND_RATE4);
		ASSERT(pButton);
		pButton->SetCheck(1);
		return;
	}

	// 無効の場合、すべてのチェックをOFFに
	for (i=0; i<5; i++) {
		pButton = (CButton*)GetDlgItem(IDC_SOUND_RATE0 + i);
		ASSERT(pButton);
		pButton->SetCheck(0);
	}
}

//---------------------------------------------------------------------------
//
//	コントロール状態変更
//
//---------------------------------------------------------------------------
void FASTCALL CSoundPage::EnableControls(BOOL bEnable) 
{
	int i;
	CWnd *pWnd;

	ASSERT(this);

	// フラグチェック
	if (m_bEnableCtrl == bEnable) {
		return;
	}
	m_bEnableCtrl = bEnable;

	// デバイス、Help以外の全コントロールを設定
	for(i=0; ; i++) {
		// 終了チェック
		if (ControlTable[i] == NULL) {
			break;
		}

		// コントロール取得
		pWnd = GetDlgItem(ControlTable[i]);
		ASSERT(pWnd);
		pWnd->EnableWindow(bEnable);
	}
}

//---------------------------------------------------------------------------
//
//	コントロールIDテーブル
//
//---------------------------------------------------------------------------
const UINT CSoundPage::ControlTable[] = {
	IDC_SOUND_RATEG,
	IDC_SOUND_RATE0,
	IDC_SOUND_RATE1,
	IDC_SOUND_RATE2,
	IDC_SOUND_RATE3,
	IDC_SOUND_RATE4,
	IDC_SOUND_BUFFERG,
	IDC_SOUND_BUF1L,
	IDC_SOUND_BUF1E,
	IDC_SOUND_BUF1S,
	IDC_SOUND_BUF1MS,
	IDC_SOUND_BUF2L,
	IDC_SOUND_BUF2E,
	IDC_SOUND_BUF2S,
	IDC_SOUND_BUF2MS,
	IDC_SOUND_OPTIONG,
	IDC_SOUND_INTERP,
	NULL
};

//===========================================================================
//
//	音量ページ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CVolPage::CVolPage()
{
	// ID,Helpを必ず設定
	m_dwID = MAKEID('V', 'O', 'L', ' ');
	m_nTemplate = IDD_VOLPAGE;
	m_uHelpID = IDC_VOL_HELP;

	// オブジェクト
//	m_pSound = NULL;
	m_pOPMIF = NULL;
	m_pADPCM = NULL;
//	m_pMIDI = NULL;

	// タイマー
	m_nTimerID = NULL;
}

//---------------------------------------------------------------------------
//
//	メッセージ マップ
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CVolPage, CConfigPage)
	ON_WM_HSCROLL()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_VOL_FMC, OnFMCheck)
	ON_BN_CLICKED(IDC_VOL_ADPCMC, OnADPCMCheck)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL CVolPage::OnInitDialog()
{
	CFrmWnd *pFrmWnd;
	CSliderCtrl *pSlider;
	CStatic *pStatic;
	CString strLabel;
	CButton *pButton;
	int nPos;
	int nMax = 100;

	// 基本クラス
	CConfigPage::OnInitDialog();

	// サウンドコンポーネントを取得
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrmWnd);
//	m_pSound = pFrmWnd->GetSound();
//	ASSERT(m_pSound);

	// OPMIFを取得
	m_pOPMIF = (OPMIF*)::GetVM()->SearchDevice(MAKEID('O', 'P', 'M', ' '));
	ASSERT(m_pOPMIF);

	// ADPCMを取得
	m_pADPCM = (ADPCM*)::GetVM()->SearchDevice(MAKEID('A', 'P', 'C', 'M'));
	ASSERT(m_pADPCM);

//		// MIDIを取得
//		m_pMIDI = pFrmWnd->GetMIDI();
//		ASSERT(m_pMIDI);

	// マスタボリューム
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_VOLS);
	ASSERT(pSlider);
	pSlider->SetRange(0, 100);
	nPos = -1;
//	nPos = m_pSound->GetMasterVol(nMax);
	if (nPos >= 0) {
		// 音量調整できる
		pSlider->SetRange(0, nMax);
		pSlider->SetPos(nPos);
		pSlider->EnableWindow(TRUE);
		strLabel.Format(_T(" %d"), (nPos * 100) / nMax);
	}
	else {
		// 音量調整できない
		pSlider->SetPos(0);
		pSlider->EnableWindow(FALSE);
		strLabel.Empty();
	}
	pStatic = (CStatic*)GetDlgItem(IDC_VOL_VOLN);
	pStatic->SetWindowText(strLabel);
	m_nMasterVol = nPos;
	m_nMasterOrg = nPos;

	// WAVEレベル
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_MASTERS);
	ASSERT(pSlider);
	pSlider->SetRange(0, 100);
	::LockVM();
//	nPos = m_pSound->GetVolume();
	::UnlockVM();
	pSlider->SetPos(nPos);
	strLabel.Format(_T(" %d"), nPos);
	pStatic = (CStatic*)GetDlgItem(IDC_VOL_MASTERN);
	pStatic->SetWindowText(strLabel);

	// MIDIレベル
//	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_SEPS);
//	ASSERT(pSlider);
//	pSlider->SetRange(0, 0xffff);
//	nPos = m_pMIDI->GetOutVolume();
//	if (nPos >= 0) {
//		// MIDI出力デバイスはアクティブかつ音量調整できる
//		pSlider->SetPos(nPos);
//		pSlider->EnableWindow(TRUE);
//		strLabel.Format(_T(" %d"), ((nPos + 1) * 100) >> 16);
//	}
//	else {
//		// MIDI出力デバイスはアクティブでない、又は音量調整できない
//		pSlider->SetPos(0);
//		pSlider->EnableWindow(FALSE);
//		strLabel.Empty();
//	}
//	pStatic = (CStatic*)GetDlgItem(IDC_VOL_SEPN);
//	pStatic->SetWindowText(strLabel);
//	m_nMIDIVol = nPos;
//	m_nMIDIOrg = nPos;

	// FM音源
	pButton = (CButton*)GetDlgItem(IDC_VOL_FMC);
	ASSERT(pButton);
	pButton->SetCheck(m_pConfig->fm_enable);
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_FMS);
	ASSERT(pSlider);
	pSlider->SetRange(0, 100);
	pSlider->SetPos(m_pConfig->fm_volume);
	strLabel.Format(_T(" %d"), m_pConfig->fm_volume);
	pStatic = (CStatic*)GetDlgItem(IDC_VOL_FMN);
	pStatic->SetWindowText(strLabel);

	// ADPCM音源
	pButton = (CButton*)GetDlgItem(IDC_VOL_ADPCMC);
	ASSERT(pButton);
	pButton->SetCheck(m_pConfig->adpcm_enable);
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_ADPCMS);
	ASSERT(pSlider);
	pSlider->SetRange(0, 100);
	pSlider->SetPos(m_pConfig->adpcm_volume);
	strLabel.Format(_T(" %d"), m_pConfig->adpcm_volume);
	pStatic = (CStatic*)GetDlgItem(IDC_VOL_ADPCMN);
	pStatic->SetWindowText(strLabel);

	// タイマを開始(100msでファイヤ)
	m_nTimerID = SetTimer(IDD_VOLPAGE, 100, NULL);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	水平スクロール
//
//---------------------------------------------------------------------------
void CVolPage::OnHScroll(UINT , UINT nPos, CScrollBar *pBar)
{
	UINT uID;
	CSliderCtrl *pSlider;
	CStatic *pStatic;
	CString strLabel;

	ASSERT(this);
	ASSERT(pBar);

	// 変換
	pSlider = (CSliderCtrl*)pBar;
	ASSERT(pSlider);

	// チェック
	switch (pSlider->GetDlgCtrlID()) {
		// マスタボリューム変更
		case IDC_VOL_VOLS:
			nPos = pSlider->GetPos();
//			m_pSound->SetMasterVol(nPos);
			// 更新はOnTimerに任せる
			OnTimer(m_nTimerID);
			return;

		// WAVEレベル変更
		case IDC_VOL_MASTERS:
			// 変更
			nPos = pSlider->GetPos();
			::LockVM();
//			m_pSound->SetVolume(nPos);
			::UnlockVM();

			// 更新
			uID = IDC_VOL_MASTERN;
			strLabel.Format(_T(" %d"), nPos);
			break;

//		// MIDIレベル変更
//		case IDC_VOL_SEPS:
//			nPos = pSlider->GetPos();
//			m_pMIDI->SetOutVolume(nPos);
//			// 更新はOnTimerに任せる
//			OnTimer(m_nTimerID);
//			return;

		// FM音量変更
		case IDC_VOL_FMS:
			// 変更
			nPos = pSlider->GetPos();
			::LockVM();
//			m_pSound->SetFMVol(nPos);
			::UnlockVM();

			// 更新
			uID = IDC_VOL_FMN;
			strLabel.Format(_T(" %d"), nPos);
			break;

		// ADPCM音量変更
		case IDC_VOL_ADPCMS:
			// 変更
			nPos = pSlider->GetPos();
			::LockVM();
//			m_pSound->SetADPCMVol(nPos);
			::UnlockVM();

			// 更新
			uID = IDC_VOL_ADPCMN;
			strLabel.Format(_T(" %d"), nPos);
			break;

		// その他
		default:
			ASSERT(FALSE);
			return;
	}

	// 変更
	pStatic = (CStatic*)GetDlgItem(uID);
	ASSERT(pStatic);
	pStatic->SetWindowText(strLabel);
}

//---------------------------------------------------------------------------
//
//	タイマ
//
//---------------------------------------------------------------------------
void CVolPage::OnTimer(UINT )
{
	CSliderCtrl *pSlider;
	CStatic *pStatic;
	CString strLabel;
	int nPos = -1;
	int nMax = 100;

	// メインボリューム取得
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_VOLS);
	ASSERT(pSlider);
//	nPos = m_pSound->GetMasterVol(nMax);

	// ボリューム比較
	if (nPos != m_nMasterVol) {
		m_nMasterVol = nPos;

		// 処理
		if (nPos >= 0) {
			// 有効化
			pSlider->SetPos(nPos);
			pSlider->EnableWindow(TRUE);
			strLabel.Format(_T(" %d"), (nPos * 100) / nMax);
		}
		else {
			// 無効化
			pSlider->SetPos(0);
			pSlider->EnableWindow(FALSE);
			strLabel.Empty();
		}

		pStatic = (CStatic*)GetDlgItem(IDC_VOL_VOLN);
		pStatic->SetWindowText(strLabel);
	}

//	// MIDI
//	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_SEPS);
//	nPos = m_pMIDI->GetOutVolume();
//
//	// MIDI比較
//	if (nPos != m_nMIDIVol) {
//		m_nMIDIVol = nPos;
//
//		// 処理
//		if (nPos >= 0) {
//			// 有効化
//			pSlider->SetPos(nPos);
//			pSlider->EnableWindow(TRUE);
//			strLabel.Format(_T(" %d"), ((nPos + 1) * 100) >> 16);
//		}
//		else {
//			// 無効化
//			pSlider->SetPos(0);
//			pSlider->EnableWindow(FALSE);
//			strLabel.Empty();
//		}
//
//		pStatic = (CStatic*)GetDlgItem(IDC_VOL_SEPN);
//		pStatic->SetWindowText(strLabel);
//	}
}

//---------------------------------------------------------------------------
//
//	FM音源チェック
//
//---------------------------------------------------------------------------
void CVolPage::OnFMCheck()
{
	CButton *pButton;

	pButton = (CButton*)GetDlgItem(IDC_VOL_FMC);
	ASSERT(pButton);
	if (pButton->GetCheck()) {
		m_pOPMIF->EnableFM(TRUE);
	}
	else {
		m_pOPMIF->EnableFM(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	ADPCM音源チェック
//
//---------------------------------------------------------------------------
void CVolPage::OnADPCMCheck()
{
	CButton *pButton;

	pButton = (CButton*)GetDlgItem(IDC_VOL_ADPCMC);
	ASSERT(pButton);
	if (pButton->GetCheck()) {
		m_pADPCM->EnableADPCM(TRUE);
	}
	else {
		m_pADPCM->EnableADPCM(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	決定
//
//---------------------------------------------------------------------------
void CVolPage::OnOK()
{
	CSliderCtrl *pSlider;
	CButton *pButton;

	// タイマ停止
	if (m_nTimerID) {
		KillTimer(m_nTimerID);
		m_nTimerID = NULL;
	}

	// WAVEレベル
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_MASTERS);
	ASSERT(pSlider);
	m_pConfig->master_volume = pSlider->GetPos();

	// FM有効
	pButton = (CButton*)GetDlgItem(IDC_VOL_FMC);
	ASSERT(pButton);
	m_pConfig->fm_enable = pButton->GetCheck();

	// FM音量
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_FMS);
	ASSERT(pSlider);
	m_pConfig->fm_volume = pSlider->GetPos();

	// ADPCM有効
	pButton = (CButton*)GetDlgItem(IDC_VOL_ADPCMC);
	ASSERT(pButton);
	m_pConfig->adpcm_enable = pButton->GetCheck();

	// ADPCM音量
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_ADPCMS);
	ASSERT(pSlider);
	m_pConfig->adpcm_volume = pSlider->GetPos();

	// 基本クラス
	CConfigPage::OnOK();
}

//---------------------------------------------------------------------------
//
//	キャンセル
//
//---------------------------------------------------------------------------
void CVolPage::OnCancel()
{
	// タイマ停止
	if (m_nTimerID) {
		KillTimer(m_nTimerID);
		m_nTimerID = NULL;
	}

	// 元の値に再設定(CONFIGデータ)
	::LockVM();
//	m_pSound->SetVolume(m_pConfig->master_volume);
	m_pOPMIF->EnableFM(m_pConfig->fm_enable);
//	m_pSound->SetFMVol(m_pConfig->fm_volume);
	m_pADPCM->EnableADPCM(m_pConfig->adpcm_enable);
//	m_pSound->SetADPCMVol(m_pConfig->adpcm_volume);
	::UnlockVM();

	// 元の値に再設定(ミキサ)
	if (m_nMasterOrg >= 0) {
//		m_pSound->SetMasterVol(m_nMasterOrg);
	}
//	if (m_nMIDIOrg >= 0) {
//		m_pMIDI->SetOutVolume(m_nMIDIOrg);
//	}

	// 基本クラス
	CConfigPage::OnCancel();
}
//===========================================================================
//
//	SASIページ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CSASIPage::CSASIPage()
{
	int i;

	// ID,Helpを必ず設定
	m_dwID = MAKEID('S', 'A', 'S', 'I');
	m_nTemplate = IDD_SASIPAGE;
	m_uHelpID = IDC_SASI_HELP;

	// SASIデバイス取得
	m_pSASI = (SASI*)::GetVM()->SearchDevice(MAKEID('S', 'A', 'S', 'I'));
	ASSERT(m_pSASI);

	// 未初期化
	m_bInit = FALSE;
	m_nDrives = -1;

	ASSERT(SASI::SASIMax <= 16);
	for (i=0; i<SASI::SASIMax; i++) {
		m_szFile[i][0] = _T('\0');
	}
}

//---------------------------------------------------------------------------
//
//	メッセージ マップ
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CSASIPage, CConfigPage)
	ON_WM_VSCROLL()
	ON_NOTIFY(NM_CLICK, IDC_SASI_LIST, OnClick)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL CSASIPage::OnInitDialog()
{
	CSpinButtonCtrl *pSpin;
	CButton *pButton;
	CListCtrl *pListCtrl;
	CClientDC *pDC;
	CString strCaption;
	CString strFile;
	TEXTMETRIC tm;
	LONG cx;
	int i;

	// 基本クラス
	CConfigPage::OnInitDialog();

	// 初期化フラグUp、ドライブ数取得
	m_bInit = TRUE;
	m_nDrives = m_pConfig->sasi_drives;
	ASSERT((m_nDrives >= 0) && (m_nDrives <= SASI::SASIMax));

	// 文字列ロード
	::GetMsg(IDS_SASI_DEVERROR, m_strError);

	// ドライブ数
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SASI_DRIVES);
	ASSERT(pSpin);
	pSpin->SetBase(10);
	pSpin->SetRange(0, SASI::SASIMax);
	pSpin->SetPos(m_nDrives);

	// メモリスイッチ自動更新
	pButton = (CButton*)GetDlgItem(IDC_SASI_MEMSWB);
	ASSERT(pButton);
	if (m_pConfig->sasi_sramsync) {
		pButton->SetCheck(1);
	}
	else {
		pButton->SetCheck(0);
	}

	// ファイル名取得
	for (i=0; i<SASI::SASIMax; i++) {
		_tcscpy(m_szFile[i], m_pConfig->sasi_file[i]);
	}

	// テキストメトリックを得る
	pDC = new CClientDC(this);
	::GetTextMetrics(pDC->m_hDC, &tm);
	delete pDC;
	cx = tm.tmAveCharWidth;

	// リストコントロール設定
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SASI_LIST);
	ASSERT(pListCtrl);
	pListCtrl->DeleteAllItems();
	::GetMsg(IDS_SASI_CAPACITY, strCaption);
	::GetMsg(IDS_SASI_FILENAME, strFile);
	if (::IsJapanese()) {
		pListCtrl->InsertColumn(0, _T("No."), LVCFMT_LEFT, cx * 4, 0);
	}
	else {
		pListCtrl->InsertColumn(0, _T("No."), LVCFMT_LEFT, cx * 5, 0);
	}
	pListCtrl->InsertColumn(1, strCaption, LVCFMT_CENTER,  cx * 6, 0);
	pListCtrl->InsertColumn(2, strFile, LVCFMT_LEFT, cx * 28, 0);

	// リストコントロール1行全体オプション(COMCTL32.DLL v4.71以降)
	pListCtrl->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

	// リストコントロール更新
	UpdateList();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ページアクティブ
//
//---------------------------------------------------------------------------
BOOL CSASIPage::OnSetActive()
{
	CSpinButtonCtrl *pSpin;
	CSCSIPage *pSCSIPage;
	BOOL bEnable;

	// 基本クラス
	if (!CConfigPage::OnSetActive()) {
		return FALSE;
	}

	// SCSIインタフェースを動的に取得
	ASSERT(m_pSheet);
	pSCSIPage = (CSCSIPage*)m_pSheet->SearchPage(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(pSCSIPage);
	if (pSCSIPage->GetInterface(m_pConfig) == 2) {
		// 内蔵SCSIインタフェース(SASIは使用できない)
		bEnable = FALSE;
	}
	else {
		// SASIまたは外付SCSIインタフェース
		bEnable = TRUE;
	}

	// コントロール有効・無効
	if (bEnable) {
		// 有効の場合、スピンボタンから現在のドライブ数を取得
		pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SASI_DRIVES);
		ASSERT(pSpin);
		if (pSpin->GetPos() > 0 ) {
			// リスト有効・ドライブ有効
			EnableControls(TRUE, TRUE);
		}
		else {
			// リスト無効・ドライブ有効
			EnableControls(FALSE, TRUE);
		}
	}
	else {
		// リスト無効・ドライブ無効
		EnableControls(FALSE, FALSE);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	決定
//
//---------------------------------------------------------------------------
void CSASIPage::OnOK()
{
	int i;
	TCHAR szPath[FILEPATH_MAX];
	CButton *pButton;
	CListCtrl *pListCtrl;

	// ドライブ数
	ASSERT((m_nDrives >= 0) && (m_nDrives <= SASI::SASIMax));
	m_pConfig->sasi_drives = m_nDrives;

	// ファイル名
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SASI_LIST);
	ASSERT(pListCtrl);
	for (i=0; i<m_nDrives; i++) {
		pListCtrl->GetItemText(i, 2, szPath, FILEPATH_MAX);
		_tcscpy(m_pConfig->sasi_file[i], szPath);
	}

	// チェックボックス(SASI・SCSIとも共通設定)
	pButton = (CButton*)GetDlgItem(IDC_SASI_MEMSWB);
	ASSERT(pButton);
	if (pButton->GetCheck() == 1) {
		m_pConfig->sasi_sramsync = TRUE;
		m_pConfig->scsi_sramsync = TRUE;
	}
	else {
		m_pConfig->sasi_sramsync = FALSE;
		m_pConfig->scsi_sramsync = FALSE;
	}

	// 基本クラス
	CConfigPage::OnOK();
}

//---------------------------------------------------------------------------
//
//	縦スクロール
//
//---------------------------------------------------------------------------
void CSASIPage::OnVScroll(UINT , UINT nPos, CScrollBar* )
{
	ASSERT(this);
	ASSERT(nPos <= SASI::SASIMax);

	// ドライブ数更新
	m_nDrives = nPos;

	// コントロール有効・無効
	if (m_nDrives > 0) {
		EnableControls(TRUE);
	}
	else {
		EnableControls(FALSE);
	}

	// リストコントロール更新
	UpdateList();
}

//---------------------------------------------------------------------------
//
//	リストコントロールクリック
//
//---------------------------------------------------------------------------
void CSASIPage::OnClick(NMHDR* , LRESULT* )
{
	CListCtrl *pListCtrl;
	int i;
	int nID;
	int nCount;
	TCHAR szPath[FILEPATH_MAX];

	// リストコントロール取得
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SASI_LIST);
	ASSERT(pListCtrl);

	// カウント数を取得
	nCount = pListCtrl->GetItemCount();

	// セレクトされているIDを取得
	nID = -1;
	for (i=0; i<nCount; i++) {
		if (pListCtrl->GetItemState(i, LVIS_SELECTED)) {
			nID = i;
			break;
		}
	}
	if (nID < 0) {
		return;
	}

	// オープンを試みる
	_tcscpy(szPath, m_szFile[nID]);
	if (!::FileOpenDlg(this, szPath, IDS_SASIOPEN)) {
		return;
	}

	// パスを更新
	_tcscpy(m_szFile[nID], szPath);

	// リストコントロール更新
	UpdateList();
}

//---------------------------------------------------------------------------
//
//	リストコントロール更新
//
//---------------------------------------------------------------------------
void FASTCALL CSASIPage::UpdateList()
{
	CListCtrl *pListCtrl;
	int nCount;
	int i;
	CString strID;
	CString strDisk;
	CString strCtrl;
	DWORD dwDisk[SASI::SASIMax];

	// リストコントロールの現在数を取得
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SASI_LIST);
	ASSERT(pListCtrl);
	nCount = pListCtrl->GetItemCount();

	// リストコントロールの方が多い場合、後半を削る
	while (nCount > m_nDrives) {
		pListCtrl->DeleteItem(nCount - 1);
		nCount--;
	}

	// リストコントロールが足りない部分は、追加する
	while (m_nDrives > nCount) {
		strID.Format(_T("%d"), nCount + 1);
		pListCtrl->InsertItem(nCount, strID);
		nCount++;
	}

	// レディチェック(m_nDriveだけまとめて行なう)
	CheckSASI(dwDisk);

	// 比較ループ
	for (i=0; i<nCount; i++) {
		// レディチェックの結果により、文字列作成
		if (dwDisk[i] == 0) {
			// 不明
			strDisk = m_strError;
		}
		else {
			// MB表示
			strDisk.Format(_T("%uMB"), dwDisk[i]);
		}

		// 比較およびセット
		strCtrl = pListCtrl->GetItemText(i, 1);
		if (strDisk != strCtrl) {
			pListCtrl->SetItemText(i, 1, strDisk);
		}

		// ファイル名
		strDisk = m_szFile[i];
		strCtrl = pListCtrl->GetItemText(i, 2);
		if (strDisk != strCtrl) {
			pListCtrl->SetItemText(i, 2, strDisk);
		}
	}
}

//---------------------------------------------------------------------------
//
//	SASIドライブチェック
//
//---------------------------------------------------------------------------
void FASTCALL CSASIPage::CheckSASI(DWORD *pDisk)
{
	int i;
	DWORD dwSize;
	Fileio fio;

	ASSERT(this);
	ASSERT(pDisk);

	// VMロック
	::LockVM();

	// ドライブループ
	for (i=0; i<m_nDrives; i++) {
		// サイズ0
		pDisk[i] = 0;

		// オープンを試みる
		if (!fio.Open(m_szFile[i], Fileio::ReadOnly)) {
			continue;
		}

		// サイズ取得、クローズ
		dwSize = fio.GetFileSize();
		fio.Close();

		// サイズチェック
		switch (dwSize) {
			case 0x9f5400:
				pDisk[i] = 10;
				break;

			// 20MB
			case 0x13c9800:
				pDisk[i] = 20;
				break;

			// 40MB
			case 0x2793000:
				pDisk[i] = 40;
				break;

			default:
				break;
		}
	}

	// アンロック
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	SASIドライブ数取得
//
//---------------------------------------------------------------------------
int FASTCALL CSASIPage::GetDrives(const Config *pConfig) const
{
	ASSERT(this);
	ASSERT(pConfig);

	// 初期化されていなければ、与えられたConfigから
	if (!m_bInit) {
		return pConfig->sasi_drives;
	}

	// 初期化済みなら、現在の値を
	return m_nDrives;
}

//---------------------------------------------------------------------------
//
//	コントロール状態変更
//
//---------------------------------------------------------------------------
void FASTCALL CSASIPage::EnableControls(BOOL bEnable, BOOL bDrive)
{
	CListCtrl *pListCtrl;
	CWnd *pWnd;

	ASSERT(this);

	// リストコントロール(bEnable)
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SASI_LIST);
	ASSERT(pListCtrl);
	pListCtrl->EnableWindow(bEnable);

	// ドライブ数(bDrive)
	pWnd = GetDlgItem(IDC_SASI_DRIVEL);
	ASSERT(pWnd);
	pWnd->EnableWindow(bDrive);
	pWnd = GetDlgItem(IDC_SASI_DRIVEE);
	ASSERT(pWnd);
	pWnd->EnableWindow(bDrive);
	pWnd = GetDlgItem(IDC_SASI_DRIVES);
	ASSERT(pWnd);
	pWnd->EnableWindow(bDrive);
}

//===========================================================================
//
//	SxSIページ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CSxSIPage::CSxSIPage()
{
	int i;

	// ID,Helpを必ず設定
	m_dwID = MAKEID('S', 'X', 'S', 'I');
	m_nTemplate = IDD_SXSIPAGE;
	m_uHelpID = IDC_SXSI_HELP;

	// 初期化(その他データ)
	m_nSASIDrives = 0;
	for (i=0; i<8; i++) {
		m_DevMap[i] = DevNone;
	}
	ASSERT(SASI::SCSIMax == 6);
	for (i=0; i<SASI::SCSIMax; i++) {
		m_szFile[i][0] = _T('\0');
	}

	// 未初期化
	m_bInit = FALSE;
}

//---------------------------------------------------------------------------
//
//	メッセージ マップ
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CSxSIPage, CConfigPage)
	ON_WM_VSCROLL()
	ON_NOTIFY(NM_CLICK, IDC_SXSI_LIST, OnClick)
	ON_BN_CLICKED(IDC_SXSI_MOCHECK, OnCheck)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	ページ初期化
//
//---------------------------------------------------------------------------
BOOL CSxSIPage::OnInitDialog()
{
	int i;
	int nMax;
	int nDrives;
	CSASIPage *pSASIPage;
	CSpinButtonCtrl *pSpin;
	CButton *pButton;
	CListCtrl *pListCtrl;
	CDC *pDC;
	TEXTMETRIC tm;
	LONG cx;
	CString strCap;
	CString strFile;

	// 基本クラス
	CConfigPage::OnInitDialog();

	// 初期化フラグUp
	m_bInit = TRUE;

	// SASIページ取得
	ASSERT(m_pSheet);
	pSASIPage = (CSASIPage*)m_pSheet->SearchPage(MAKEID('S', 'A', 'S', 'I'));
	ASSERT(pSASIPage);

	// SASIの設定ドライブ数から、SCSIに設定できる最大ドライブ数を得る
	m_nSASIDrives = pSASIPage->GetDrives(m_pConfig);
	nMax = m_nSASIDrives;
	nMax = (nMax + 1) >> 1;
	ASSERT((nMax >= 0) && (nMax <= 8));
	if (nMax >= 7) {
		nMax = 0;
	}
	else {
		nMax = 7 - nMax;
	}

	// SCSIの最大ドライブ数を制限
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SXSI_DRIVES);
	pSpin->SetBase(10);
	nDrives = m_pConfig->sxsi_drives;
	if (nDrives > nMax) {
		nDrives = nMax;
	}
	pSpin->SetRange(0, (short)nMax);
	pSpin->SetPos(nDrives);

	// SCSIのファイル名を取得
	for (i=0; i<6; i++) {
		_tcscpy(m_szFile[i], m_pConfig->sxsi_file[i]);
	}

	// MO優先フラグ設定
	pButton = (CButton*)GetDlgItem(IDC_SXSI_MOCHECK);
	if (m_pConfig->sxsi_mofirst) {
		pButton->SetCheck(1);
	}
	else {
		pButton->SetCheck(0);
	}

	// テキストメトリックを得る
	pDC = new CClientDC(this);
	::GetTextMetrics(pDC->m_hDC, &tm);
	delete pDC;
	cx = tm.tmAveCharWidth;

	// リストコントロール設定
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SXSI_LIST);
	ASSERT(pListCtrl);
	pListCtrl->DeleteAllItems();
	if (::IsJapanese()) {
		pListCtrl->InsertColumn(0, _T("ID"), LVCFMT_LEFT, cx * 3, 0);
	}
	else {
		pListCtrl->InsertColumn(0, _T("ID"), LVCFMT_LEFT, cx * 4, 0);
	}
	::GetMsg(IDS_SXSI_CAPACITY, strCap);
	pListCtrl->InsertColumn(1, strCap, LVCFMT_CENTER,  cx * 7, 0);
	::GetMsg(IDS_SXSI_FILENAME, strFile);
	pListCtrl->InsertColumn(2, strFile, LVCFMT_LEFT, cx * 26, 0);

	// リストコントロール1行全体オプション(COMCTL32.DLL v4.71以降)
	pListCtrl->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

	// リストコントロールで使う文字列を取得
	::GetMsg(IDS_SXSI_SASI, m_strSASI);
	::GetMsg(IDS_SXSI_MO, m_strMO);
	::GetMsg(IDS_SXSI_INIT, m_strInit);
	::GetMsg(IDS_SXSI_NONE, m_strNone);
	::GetMsg(IDS_SXSI_DEVERROR, m_strError);

	// リストコントロール更新
	UpdateList();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ページアクティブ
//
//---------------------------------------------------------------------------
BOOL CSxSIPage::OnSetActive()
{
	int nMax;
	int nPos;
	CSpinButtonCtrl *pSpin;
	BOOL bEnable;
	CSASIPage *pSASIPage;
	CSCSIPage *pSCSIPage;
	CAlterPage *pAlterPage;

	// 基本クラス
	if (!CConfigPage::OnSetActive()) {
		return FALSE;
	}

	// ページ取得
	ASSERT(m_pSheet);
	pSASIPage = (CSASIPage*)m_pSheet->SearchPage(MAKEID('S', 'A', 'S', 'I'));
	ASSERT(pSASIPage);
	pSCSIPage = (CSCSIPage*)m_pSheet->SearchPage(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(pSCSIPage);
	pAlterPage = (CAlterPage*)m_pSheet->SearchPage(MAKEID('A', 'L', 'T', ' '));
	ASSERT(pAlterPage);

	// SxSIイネーブルフラグを動的に取得
	bEnable = TRUE;
	if (!pAlterPage->HasParity(m_pConfig)) {
		// パリティを設定しない。SxSIは使用できない
		bEnable = FALSE;
	}
	if (pSCSIPage->GetInterface(m_pConfig) != 0) {
		// 内蔵または外付SCSIインタフェース。SxSIは使用できない
		bEnable = FALSE;
	}

	// SASIのドライブ数を取得し、SCSIの最大ドライブ数を得る
	m_nSASIDrives = pSASIPage->GetDrives(m_pConfig);
	nMax = m_nSASIDrives;
	nMax = (nMax + 1) >> 1;
	ASSERT((nMax >= 0) && (nMax <= 8));
	if (nMax >= 7) {
		nMax = 0;
	}
	else {
		nMax = 7 - nMax;
	}

	// SCSIの最大ドライブ数を制限
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SXSI_DRIVES);
	ASSERT(pSpin);
	nPos = LOWORD(pSpin->GetPos());
	if (nPos > nMax) {
		nPos = nMax;
		pSpin->SetPos(nPos);
	}
	pSpin->SetRange(0, (short)nMax);

	// リストコントロール更新
	UpdateList();

	// コントロール有効・無効
	if (bEnable) {
		if (nPos > 0) {
			// リスト有効・ドライブ有効
			EnableControls(TRUE, TRUE);
		}
		else {
			// リスト有効・ドライブ無効
			EnableControls(FALSE, TRUE);
		}
	}
	else {
		// リスト無効・ドライブ無効
		EnableControls(FALSE, FALSE);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	縦スクロール
//
//---------------------------------------------------------------------------
void CSxSIPage::OnVScroll(UINT , UINT nPos, CScrollBar* )
{
	// リストコントロール更新(内部でBuildMapを行う)
	UpdateList();

	// コントロール有効・無効
	if (nPos > 0) {
		EnableControls(TRUE);
	}
	else {
		EnableControls(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	リストコントロールクリック
//
//---------------------------------------------------------------------------
void CSxSIPage::OnClick(NMHDR* , LRESULT* )
{
	CListCtrl *pListCtrl;
	int i;
	int nID;
	int nCount;
	int nDrive;
	TCHAR szPath[FILEPATH_MAX];

	// リストコントロール取得
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SXSI_LIST);
	ASSERT(pListCtrl);

	// カウント数を取得
	nCount = pListCtrl->GetItemCount();

	// セレクトされているIDを取得
	nID = -1;
	for (i=0; i<nCount; i++) {
		if (pListCtrl->GetItemState(i, LVIS_SELECTED)) {
			nID = i;
			break;
		}
	}
	if (nID < 0) {
		return;
	}

	// マップを見て、タイプを判別
	if (m_DevMap[nID] != DevSCSI) {
		return;
	}

	// IDからドライブインデックス取得(MOは考慮しない)
	nDrive = 0;
	for (i=0; i<8; i++) {
		if (i == nID) {
			break;
		}
		if (m_DevMap[i] == DevSCSI) {
			nDrive++;
		}
	}
	ASSERT((nDrive >= 0) && (nDrive < SASI::SCSIMax));

	// オープンを試みる
	_tcscpy(szPath, m_szFile[nDrive]);
	if (!::FileOpenDlg(this, szPath, IDS_SCSIOPEN)) {
		return;
	}

	// パスを更新
	_tcscpy(m_szFile[nDrive], szPath);

	// リストコントロール更新
	UpdateList();
}

//---------------------------------------------------------------------------
//
//	チェックボックス変更
//
//---------------------------------------------------------------------------
void CSxSIPage::OnCheck()
{
	// リストコントロール更新(内部でBuildMapを行う)
	UpdateList();
}

//---------------------------------------------------------------------------
//
//	決定
//
//---------------------------------------------------------------------------
void CSxSIPage::OnOK()
{
	CSpinButtonCtrl *pSpin;
	CButton *pButton;
	int i;

	// ドライブ数
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SXSI_DRIVES);
	ASSERT(pSpin);
	m_pConfig->sxsi_drives = LOWORD(pSpin->GetPos());

	// MO優先フラグ
	pButton = (CButton*)GetDlgItem(IDC_SXSI_MOCHECK);
	ASSERT(pButton);
	if (pButton->GetCheck() == 1) {
		m_pConfig->sxsi_mofirst = TRUE;
	}
	else {
		m_pConfig->sxsi_mofirst = FALSE;
	}

	// ファイル名
	for (i=0; i<SASI::SCSIMax; i++) {
		_tcscpy(m_pConfig->sxsi_file[i], m_szFile[i]);
	}

	// 基本クラス
	CConfigPage::OnOK();
}

//---------------------------------------------------------------------------
//
//	リストコントロール更新
//
//---------------------------------------------------------------------------
void FASTCALL CSxSIPage::UpdateList()
{
	int i;
	int nDrive;
	int nDev;
	int nCount;
	int nCap;
	CListCtrl *pListCtrl;
	CString strCtrl;
	CString strID;
	CString strSize;
	CString strFile;

	ASSERT(this);

	// マップをビルド
	BuildMap();

	// リストコントロール取得、カウント取得
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SXSI_LIST);
	ASSERT(pListCtrl);
	nCount = pListCtrl->GetItemCount();

	// マップのうちNoneでないものの数を数える
	nDev = 0;
	for (i=0; i<8; i++) {
		if (m_DevMap[i] != DevNone) {
			nDev++;
		}
	}

	// nDevだけアイテムをつくる
	while (nCount > nDev) {
		pListCtrl->DeleteItem(nCount - 1);
		nCount--;
	}
	while (nDev > nCount) {
		strID.Format(_T("%d"), nCount + 1);
		pListCtrl->InsertItem(nCount, strID);
		nCount++;
	}

	// 比較ループ
	nDrive = 0;
	nDev = 0;
	for (i=0; i<8; i++) {
		// タイプに応じて文字列を作る
		switch (m_DevMap[i]) {
			// SASI ハードディスク
			case DevSASI:
				strSize = m_strNone;
				strFile = m_strSASI;
				break;

			// SCSI ハードディスク
			case DevSCSI:
				nCap = CheckSCSI(nDrive);
				if (nCap > 0) {
					strSize.Format("%dMB", nCap);
				}
				else {
					strSize = m_strError;
				}
				strFile = m_szFile[nDrive];
				nDrive++;
				break;

			// SCSI MOディスク
			case DevMO:
				strSize = m_strNone;
				strFile = m_strMO;
				break;

			// イニシエータ(ホスト)
			case DevInit:
				strSize = m_strNone;
				strFile = m_strInit;
				break;

			// デバイスなし
			case DevNone:
				// 次に進む
				continue;

			// その他(あり得ない)
			default:
				ASSERT(FALSE);
				return;
		}

		// ID
		strID.Format(_T("%d"), i);
		strCtrl = pListCtrl->GetItemText(nDev, 0);
		if (strID != strCtrl) {
			pListCtrl->SetItemText(nDev, 0, strID);
		}

		// 容量
		strCtrl = pListCtrl->GetItemText(nDev, 1);
		if (strSize != strCtrl) {
			pListCtrl->SetItemText(nDev, 1, strSize);
		}

		// ファイル名
		strCtrl = pListCtrl->GetItemText(nDev, 2);
		if (strFile != strCtrl) {
			pListCtrl->SetItemText(nDev, 2, strFile);
		}

		// 次へ
		nDev++;
	}
}

//---------------------------------------------------------------------------
//
//	マップ作成
//
//---------------------------------------------------------------------------
void FASTCALL CSxSIPage::BuildMap()
{
	int nSASI;
	int nMO;
	int nSCSI;
	int nInit;
	int nMax;
	int nID;
	int i;
	BOOL bMOFirst;
	CButton *pButton;
	CSpinButtonCtrl *pSpin;

	ASSERT(this);

	// 初期化
	nSASI = 0;
	nMO = 0;
	nSCSI = 0;
	nInit = 0;

	// MO優先フラグを取得
	pButton = (CButton*)GetDlgItem(IDC_SXSI_MOCHECK);
	ASSERT(pButton);
	bMOFirst = FALSE;
	if (pButton->GetCheck() != 0) {
		bMOFirst = TRUE;
	}

	// SASIドライブ数から、SASIの占有ID数を得る
	ASSERT((m_nSASIDrives >= 0) && (m_nSASIDrives <= 0x10));
	nSASI = m_nSASIDrives;
	nSASI = (nSASI + 1) >> 1;

	// SASIから、MO,SCSI,INITの最大数を得る
	if (nSASI <= 6) {
		nMO = 1;
		nSCSI = 6 - nSASI;
	}
	if (nSASI <= 7) {
		nInit = 1;
	}

	// SxSIドライブ数の設定を見て、値を調整
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SXSI_DRIVES);
	ASSERT(pSpin);
	nMax = LOWORD(pSpin->GetPos());
	ASSERT((nMax >= 0) && (nMax <= (nSCSI + nMO)));
	if (nMax == 0) {
		// SxSIドライブ数は0
		nMO = 0;
		nSCSI = 0;
	}
	else {
		// とりあえずnSCSIにHD+MOを集める
		nSCSI = nMax;

		// 1の場合はMOのみ
		if (nMax == 1) {
			nMO = 1;
			nSCSI = 0;
		}
		else {
			// 2以上の場合は、1つをMOに割り当てる
			nSCSI--;
			nMO = 1;
		}
	}

	// IDをリセット
	nID = 0;

	// オールクリア
	for (i=0; i<8; i++) {
		m_DevMap[i] = DevNone;
	}

	// SASIをセット
	for (i=0; i<nSASI; i++) {
		m_DevMap[nID] = DevSASI;
		nID++;
	}

	// SCSI,MOセット
	if (bMOFirst) {
		// MO優先
		for (i=0; i<nMO; i++) {
			m_DevMap[nID] = DevMO;
			nID++;
		}
		for (i=0; i<nSCSI; i++) {
			m_DevMap[nID] = DevSCSI;
			nID++;
		}
	}
	else {
		// HD優先
		for (i=0; i<nSCSI; i++) {
			m_DevMap[nID] = DevSCSI;
			nID++;
		}
		for (i=0; i<nMO; i++) {
			m_DevMap[nID] = DevMO;
			nID++;
		}
	}

	// イニシエータセット
	for (i=0; i<nInit; i++) {
		ASSERT(nID <= 7);
		m_DevMap[7] = DevInit;
	}
}

//---------------------------------------------------------------------------
//
//	SCSIハードディスク容量チェック
//	※デバイスエラーで0を返す
//
//---------------------------------------------------------------------------
int FASTCALL CSxSIPage::CheckSCSI(int nDrive)
{
	Fileio fio;
	DWORD dwSize;

	ASSERT(this);
	ASSERT((nDrive >= 0) && (nDrive <= 5));

	// ロック
	::LockVM();

	// ファイルオープン
	if (!fio.Open(m_szFile[nDrive], Fileio::ReadOnly)) {
		// エラーなので0を返す
		fio.Close();
		::UnlockVM();
		return 0;
	}

	// 容量取得
	dwSize = fio.GetFileSize();

	// アンロック
	fio.Close();
	::UnlockVM();

	// ファイルサイズをチェック(512バイト単位)
	if ((dwSize & 0x1ff) != 0) {
		return 0;
	}

	// ファイルサイズをチェック(10MB以上)
	if (dwSize < 10 * 0x400 * 0x400) {
		return 0;
	}

	// ファイルサイズをチェック(1016MB以下)
	if (dwSize > 1016 * 0x400 * 0x400) {
		return 0;
	}

	// サイズを持ち帰る
	dwSize >>= 20;
	return dwSize;
}

//---------------------------------------------------------------------------
//
//	コントロール状態変更
//
//---------------------------------------------------------------------------
void CSxSIPage::EnableControls(BOOL bEnable, BOOL bDrive)
{
	int i;
	CWnd *pWnd;
	CListCtrl *pListCtrl;

	ASSERT(this);

	// リストコントロール・MOチェック以外の全コントロールを設定
	for (i=0; ; i++) {
		// コントロール取得
		if (!ControlTable[i]) {
			break;
		}
		pWnd = GetDlgItem(ControlTable[i]);
		ASSERT(pWnd);

		// 設定
		pWnd->EnableWindow(bDrive);
	}

	// リストコントロールを設定
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SXSI_LIST);
	ASSERT(pListCtrl);
	pListCtrl->EnableWindow(bEnable);

	// MOチェックを設定
	pWnd = GetDlgItem(IDC_SXSI_MOCHECK);
	ASSERT(pWnd);
	pWnd->EnableWindow(bEnable);
}

//---------------------------------------------------------------------------
//
//	ドライブ数取得
//
//---------------------------------------------------------------------------
int FASTCALL CSxSIPage::GetDrives(const Config *pConfig) const
{
	BOOL bEnable;
	CSASIPage *pSASIPage;
	CSCSIPage *pSCSIPage;
	CAlterPage *pAlterPage;
	CSpinButtonCtrl *pSpin;
	int nPos;

	ASSERT(this);
	ASSERT(pConfig);

	// ページ取得
	ASSERT(m_pSheet);
	pSASIPage = (CSASIPage*)m_pSheet->SearchPage(MAKEID('S', 'A', 'S', 'I'));
	ASSERT(pSASIPage);
	pSCSIPage = (CSCSIPage*)m_pSheet->SearchPage(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(pSCSIPage);
	pAlterPage = (CAlterPage*)m_pSheet->SearchPage(MAKEID('A', 'L', 'T', ' '));
	ASSERT(pAlterPage);

	// SxSIイネーブルフラグを動的に取得
	bEnable = TRUE;
	if (!pAlterPage->HasParity(pConfig)) {
		// パリティを設定しない。SxSIは使用できない
		bEnable = FALSE;
	}
	if (pSCSIPage->GetInterface(pConfig) != 0) {
		// 内蔵または外付SCSIインタフェース。SxSIは使用できない
		bEnable = FALSE;
	}
	if (pSASIPage->GetDrives(pConfig) >= 12) {
		// SASIドライブ数が多すぎる。SxSIは使用できない
		bEnable = FALSE;
	}

	// 使用できない場合は0
	if (!bEnable) {
		return 0;
	}

	// 未初期化の場合、設定値を返す
	if (!m_bInit) {
		return pConfig->sxsi_drives;
	}

	// 現在編集中の値を返す
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SXSI_DRIVES);
	ASSERT(pSpin);
	nPos = LOWORD(pSpin->GetPos());
	return nPos;
}

//---------------------------------------------------------------------------
//
//	コントロールテーブル
//
//---------------------------------------------------------------------------
const UINT CSxSIPage::ControlTable[] = {
	IDC_SXSI_GROUP,
	IDC_SXSI_DRIVEL,
	IDC_SXSI_DRIVEE,
	IDC_SXSI_DRIVES,
	NULL
};

//===========================================================================
//
//	SCSIページ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CSCSIPage::CSCSIPage()
{
	int i;

	// ID,Helpを必ず設定
	m_dwID = MAKEID('S', 'C', 'S', 'I');
	m_nTemplate = IDD_SCSIPAGE;
	m_uHelpID = IDC_SCSI_HELP;

	// SCSI取得
	m_pSCSI = (SCSI*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(m_pSCSI);

	// 初期化(その他データ)
	m_bInit = FALSE;
	m_nDrives = 0;
	m_bMOFirst = FALSE;

	// デバイスマップ
	ASSERT(SCSI::DeviceMax == 8);
	for (i=0; i<SCSI::DeviceMax; i++) {
		m_DevMap[i] = DevNone;
	}

	// ファイルパス
	ASSERT(SCSI::HDMax == 5);
	for (i=0; i<SCSI::HDMax; i++) {
		m_szFile[i][0] = _T('\0');
	}
}

//---------------------------------------------------------------------------
//
//	メッセージ マップ
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CSCSIPage, CConfigPage)
	ON_WM_VSCROLL()
	ON_NOTIFY(NM_CLICK, IDC_SCSI_LIST, OnClick)
	ON_BN_CLICKED(IDC_SCSI_NONEB, OnButton)
	ON_BN_CLICKED(IDC_SCSI_INTB, OnButton)
	ON_BN_CLICKED(IDC_SCSI_EXTB, OnButton)
	ON_BN_CLICKED(IDC_SCSI_MOCHECK, OnCheck)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	ページ初期化
//
//---------------------------------------------------------------------------
BOOL CSCSIPage::OnInitDialog()
{
	int i;
	BOOL bAvail;
	BOOL bEnable[2];
	CButton *pButton;
	CSpinButtonCtrl *pSpin;
	CDC *pDC;
	TEXTMETRIC tm;
	LONG cx;
	CListCtrl *pListCtrl;
	CString strCap;
	CString strFile;

	// 基本クラス
	CConfigPage::OnInitDialog();

	// 初期化フラグUp
	m_bInit = TRUE;

	// ROMの有無に応じて、インタフェースラジオボタンを禁止
	pButton = (CButton*)GetDlgItem(IDC_SCSI_EXTB);
	ASSERT(pButton);
	bEnable[0] = CheckROM(1);
	pButton->EnableWindow(bEnable[0]);
	pButton = (CButton*)GetDlgItem(IDC_SCSI_INTB);
	ASSERT(pButton);
	bEnable[1] = CheckROM(2);
	pButton->EnableWindow(bEnable[1]);

	// インタフェース種別
	pButton = (CButton*)GetDlgItem(IDC_SCSI_NONEB);
	bAvail = FALSE;
	switch (m_pConfig->mem_type) {
		// 装着しない
		case Memory::None:
		case Memory::SASI:
			break;

		// 外付
		case Memory::SCSIExt:
			// 外付ROMが存在する場合のみ
			if (bEnable[0]) {
				pButton = (CButton*)GetDlgItem(IDC_SCSI_EXTB);
				bAvail = TRUE;
			}
			break;

		// その他(内蔵)
		default:
			// 内蔵ROMが存在する場合のみ
			if (bEnable[1]) {
				pButton = (CButton*)GetDlgItem(IDC_SCSI_INTB);
				bAvail = TRUE;
			}
			break;
	}
	ASSERT(pButton);
	pButton->SetCheck(1);

	// ドライブ数
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SCSI_DRIVES);
	pSpin->SetBase(10);
	pSpin->SetRange(0, 7);
	m_nDrives = m_pConfig->scsi_drives;
	ASSERT((m_nDrives >= 0) && (m_nDrives <= 7));
	pSpin->SetPos(m_nDrives);

	// MO優先フラグ
	pButton = (CButton*)GetDlgItem(IDC_SCSI_MOCHECK);
	ASSERT(pButton);
	if (m_pConfig->scsi_mofirst) {
		pButton->SetCheck(1);
		m_bMOFirst = TRUE;
	}
	else {
		pButton->SetCheck(0);
		m_bMOFirst = FALSE;
	}

	// SCSI-HDファイルパス
	for (i=0; i<SCSI::HDMax; i++) {
		_tcscpy(m_szFile[i], m_pConfig->scsi_file[i]);
	}

	// テキストメトリックを得る
	pDC = new CClientDC(this);
	::GetTextMetrics(pDC->m_hDC, &tm);
	delete pDC;
	cx = tm.tmAveCharWidth;

	// リストコントロール設定
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SCSI_LIST);
	ASSERT(pListCtrl);
	pListCtrl->DeleteAllItems();
	if (::IsJapanese()) {
		pListCtrl->InsertColumn(0, _T("ID"), LVCFMT_LEFT, cx * 3, 0);
	}
	else {
		pListCtrl->InsertColumn(0, _T("ID"), LVCFMT_LEFT, cx * 4, 0);
	}
	::GetMsg(IDS_SCSI_CAPACITY, strCap);
	pListCtrl->InsertColumn(1, strCap, LVCFMT_CENTER,  cx * 7, 0);
	::GetMsg(IDS_SCSI_FILENAME, strFile);
	pListCtrl->InsertColumn(2, strFile, LVCFMT_LEFT, cx * 26, 0);

	// リストコントロール1行全体オプション(COMCTL32.DLL v4.71以降)
	pListCtrl->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

	// リストコントロールで使う文字列を取得
	::GetMsg(IDS_SCSI_MO, m_strMO);
	::GetMsg(IDS_SCSI_CD, m_strCD);
	::GetMsg(IDS_SCSI_INIT, m_strInit);
	::GetMsg(IDS_SCSI_NONE, m_strNone);
	::GetMsg(IDS_SCSI_DEVERROR, m_strError);

	// リストコントロール更新(内部でBuildMapを行う)
	UpdateList();

	// コントロール有効・無効
	if (bAvail) {
		if (m_nDrives > 0) {
			// リスト有効・ドライブ有効
			EnableControls(TRUE, TRUE);
		}
		else {
			// リスト無効・ドライブ有効
			EnableControls(FALSE, TRUE);
		}
	}
	else {
		// リスト無効・ドライブ無効
		EnableControls(FALSE, FALSE);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	決定
//
//---------------------------------------------------------------------------
void CSCSIPage::OnOK()
{
	int i;

	// インタフェース種別からメモリ種別設定
	switch (GetIfCtrl()) {
		// 装着しない
		case 0:
			m_pConfig->mem_type = Memory::SASI;
			break;

		// 外付
		case 1:
			m_pConfig->mem_type = Memory::SCSIExt;
			break;

		// 内蔵
		case 2:
			// タイプが違う場合のみ、SCSIIntに変更
			if ((m_pConfig->mem_type == Memory::SASI) || (m_pConfig->mem_type == Memory::SCSIExt)) {
				m_pConfig->mem_type = Memory::SCSIInt;
			}
			break;

		// その他(ありえない)
		default:
			ASSERT(FALSE);
	}

	// ドライブ数
	m_pConfig->scsi_drives = m_nDrives;

	// MO優先フラグ
	m_pConfig->scsi_mofirst = m_bMOFirst;

	// SCSI-HDファイルパス
	for (i=0; i<SCSI::HDMax; i++) {
		_tcscpy(m_pConfig->scsi_file[i], m_szFile[i]);
	}

	// 基本クラス
	CConfigPage::OnOK();
}

//---------------------------------------------------------------------------
//
//	縦スクロール
//
//---------------------------------------------------------------------------
void CSCSIPage::OnVScroll(UINT , UINT nPos, CScrollBar* )
{
	// ドライブ数取得
	m_nDrives = nPos;

	// リストコントロール更新(内部でBuildMapを行う)
	UpdateList();

	// コントロール有効・無効
	if (nPos > 0) {
		EnableControls(TRUE);
	}
	else {
		EnableControls(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	リストコントロールクリック
//
//---------------------------------------------------------------------------
void CSCSIPage::OnClick(NMHDR* , LRESULT* )
{
	CListCtrl *pListCtrl;
	int i;
	int nID;
	int nCount;
	int nDrive;
	TCHAR szPath[FILEPATH_MAX];

	// リストコントロール取得
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SCSI_LIST);
	ASSERT(pListCtrl);

	// カウント数を取得
	nCount = pListCtrl->GetItemCount();

	// セレクトされているアイテムを取得
	nID = -1;
	for (i=0; i<nCount; i++) {
		if (pListCtrl->GetItemState(i, LVIS_SELECTED)) {
			nID = i;
			break;
		}
	}
	if (nID < 0) {
		return;
	}

	// アイテムデータからIDを取得
	nID = (int)pListCtrl->GetItemData(nID);

	// マップを見て、タイプを判別
	if (m_DevMap[nID] != DevSCSI) {
		return;
	}

	// IDからドライブインデックス取得(MOは考慮しない)
	nDrive = 0;
	for (i=0; i<SCSI::DeviceMax; i++) {
		if (i == nID) {
			break;
		}
		if (m_DevMap[i] == DevSCSI) {
			nDrive++;
		}
	}
	ASSERT((nDrive >= 0) && (nDrive < SCSI::HDMax));

	// オープンを試みる
	_tcscpy(szPath, m_szFile[nDrive]);
	if (!::FileOpenDlg(this, szPath, IDS_SCSIOPEN)) {
		return;
	}

	// パスを更新
	_tcscpy(m_szFile[nDrive], szPath);

	// リストコントロール更新
	UpdateList();
}

//---------------------------------------------------------------------------
//
//	ラジオボタン変更
//
//---------------------------------------------------------------------------
void CSCSIPage::OnButton()
{
	CButton *pButton;

	// インタフェース無効にチェックされているか
	pButton = (CButton*)GetDlgItem(IDC_SCSI_NONEB);
	ASSERT(pButton);
	if (pButton->GetCheck() != 0) {
		// リスト無効・ドライブ無効
		EnableControls(FALSE, FALSE);
		return;
	}

	if (m_nDrives > 0) {
		// リスト有効・ドライブ有効
		EnableControls(TRUE, TRUE);
	}
	else {
		// リスト無効・ドライブ有効
		EnableControls(FALSE, TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	チェックボックス変更
//
//---------------------------------------------------------------------------
void CSCSIPage::OnCheck()
{
	CButton *pButton;

	// 現在の状態を得る
	pButton = (CButton*)GetDlgItem(IDC_SCSI_MOCHECK);
	ASSERT(pButton);
	if (pButton->GetCheck() != 0) {
		m_bMOFirst = TRUE;
	}
	else {
		m_bMOFirst = FALSE;
	}

	// リストコントロール更新(内部でBuildMapを行う)
	UpdateList();
}

//---------------------------------------------------------------------------
//
//	インタフェース種別取得
//
//---------------------------------------------------------------------------
int FASTCALL CSCSIPage::GetInterface(const Config *pConfig) const
{
	ASSERT(this);
	ASSERT(pConfig);

	// 初期化フラグ
	if (!m_bInit) {
		// 初期化されていないので、Configから取得
		switch (pConfig->mem_type) {
			// 装着しない
			case Memory::None:
			case Memory::SASI:
				return 0;

			// 外付
			case Memory::SCSIExt:
				return 1;

			// その他(内蔵)
			default:
				return 2;
		}
	}

	// 初期化されているので、コントロールから取得
	return GetIfCtrl();
}

//---------------------------------------------------------------------------
//
//	インタフェース種別取得(コントロールより)
//
//---------------------------------------------------------------------------
int FASTCALL CSCSIPage::GetIfCtrl() const
{
	CButton *pButton;

	ASSERT(this);

	// 装着しない
	pButton = (CButton*)GetDlgItem(IDC_SCSI_NONEB);
	ASSERT(pButton);
	if (pButton->GetCheck() != 0) {
		return 0;
	}

	// 外付
	pButton = (CButton*)GetDlgItem(IDC_SCSI_EXTB);
	ASSERT(pButton);
	if (pButton->GetCheck() != 0) {
		return 1;
	}

	// 内蔵
	pButton = (CButton*)GetDlgItem(IDC_SCSI_INTB);
	ASSERT(pButton);
	ASSERT(pButton->GetCheck() != 0);
	return 2;
}

//---------------------------------------------------------------------------
//
//	ROMチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL CSCSIPage::CheckROM(int nType) const
{
	Filepath path;
	Fileio fio;
	DWORD dwSize;

	ASSERT(this);
	ASSERT((nType >= 0) && (nType <= 2));

	// 0:内蔵の場合は無条件にOK
	if (nType == 0) {
		return TRUE;
	}

	// ファイルパス作成
	if (nType == 1) {
		// 外付
		path.SysFile(Filepath::SCSIExt);
	}
	else {
		// 内蔵
		path.SysFile(Filepath::SCSIInt);
	}

	// ロック
	::LockVM();

	// オープンを試みる
	if (!fio.Open(path, Fileio::ReadOnly)) {
		::UnlockVM();
		return FALSE;
	}

	// ファイルサイズ取得
	dwSize = fio.GetFileSize();
	fio.Close();
	::UnlockVM();

	if (nType == 1) {
		// 外付は、0x2000バイトまたは0x1fe0バイト(WinX68k高速版と互換をとる)
		if ((dwSize == 0x2000) || (dwSize == 0x1fe0)) {
			return TRUE;
		}
	}
	else {
		// 内蔵は、0x2000バイトのみ
		if (dwSize == 0x2000) {
			return TRUE;
		}
	}

	return FALSE;
}

//---------------------------------------------------------------------------
//
//	リストコントロール更新
//
//---------------------------------------------------------------------------
void FASTCALL CSCSIPage::UpdateList()
{
	int i;
	int nDrive;
	int nDev;
	int nCount;
	int nCap;
	CListCtrl *pListCtrl;
	CString strCtrl;
	CString strID;
	CString strSize;
	CString strFile;

	ASSERT(this);

	// マップをビルド
	BuildMap();

	// リストコントロール取得、カウント取得
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SCSI_LIST);
	ASSERT(pListCtrl);
	nCount = pListCtrl->GetItemCount();

	// マップのうちNoneでないものの数を数える
	nDev = 0;
	for (i=0; i<8; i++) {
		if (m_DevMap[i] != DevNone) {
			nDev++;
		}
	}

	// nDevだけアイテムをつくる
	while (nCount > nDev) {
		pListCtrl->DeleteItem(nCount - 1);
		nCount--;
	}
	while (nDev > nCount) {
		strID.Format(_T("%d"), nCount + 1);
		pListCtrl->InsertItem(nCount, strID);
		nCount++;
	}

	// 比較ループ
	nDrive = 0;
	nDev = 0;
	for (i=0; i<SCSI::DeviceMax; i++) {
		// タイプに応じて文字列を作る
		switch (m_DevMap[i]) {
			// SCSI ハードディスク
			case DevSCSI:
				nCap = CheckSCSI(nDrive);
				if (nCap > 0) {
					strSize.Format("%dMB", nCap);
				}
				else {
					strSize = m_strError;
				}
				strFile = m_szFile[nDrive];
				nDrive++;
				break;

			// SCSI MOディスク
			case DevMO:
				strSize = m_strNone;
				strFile = m_strMO;
				break;

			// SCSI CD-ROM
			case DevCD:
				strSize = m_strNone;
				strFile = m_strCD;
				break;

			// イニシエータ(ホスト)
			case DevInit:
				strSize = m_strNone;
				strFile = m_strInit;
				break;

			// デバイスなし
			case DevNone:
				// 次に進む
				continue;

			// その他(あり得ない)
			default:
				ASSERT(FALSE);
				return;
		}

		// アイテムデータ
		if ((int)pListCtrl->GetItemData(nDev) != i) {
			pListCtrl->SetItemData(nDev, (DWORD)i);
		}

		// ID
		strID.Format(_T("%d"), i);
		strCtrl = pListCtrl->GetItemText(nDev, 0);
		if (strID != strCtrl) {
			pListCtrl->SetItemText(nDev, 0, strID);
		}

		// 容量
		strCtrl = pListCtrl->GetItemText(nDev, 1);
		if (strSize != strCtrl) {
			pListCtrl->SetItemText(nDev, 1, strSize);
		}

		// ファイル名
		strCtrl = pListCtrl->GetItemText(nDev, 2);
		if (strFile != strCtrl) {
			pListCtrl->SetItemText(nDev, 2, strFile);
		}

		// 次へ
		nDev++;
	}
}

//---------------------------------------------------------------------------
//
//	マップ作成
//
//---------------------------------------------------------------------------
void FASTCALL CSCSIPage::BuildMap()
{
	int i;
	int nID;
	int nInit;
	int nHD;
	BOOL bMO;
	BOOL bCD;

	ASSERT(this);

	// 初期化
	nHD = 0;
	bMO = FALSE;
	bCD = FALSE;

	// ディスク数を決定
	switch (m_nDrives) {
		// 0台
		case 0:
			break;

		// 1台
		case 1:
			// MO優先か、HD優先かで分ける
			if (m_bMOFirst) {
				bMO = TRUE;
			}
			else {
				nHD = 1;
			}
			break;

		// 2台
		case 2:
			// HD,MOとも1台
			nHD = 1;
			bMO = TRUE;
			break;

		// 3台
		case 3:
			// HD,MO,CDとも1台
			nHD = 1;
			bMO = TRUE;
			bCD = TRUE;
			break;

		// 4台以上
		default:
			ASSERT(m_nDrives <= 7);
			nHD= m_nDrives - 2;
			bMO = TRUE;
			bCD = TRUE;
			break;
	}

	// オールクリア
	for (i=0; i<8; i++) {
		m_DevMap[i] = DevNone;
	}

	// イニシエータを先に設定
	ASSERT(m_pSCSI);
	nInit = m_pSCSI->GetSCSIID();
	ASSERT((nInit >= 0) && (nInit <= 7));
	m_DevMap[nInit] = DevInit;

	// MO設定(優先フラグ時のみ)
	if (bMO && m_bMOFirst) {
		for (nID=0; nID<SCSI::DeviceMax; nID++) {
			if (m_DevMap[nID] == DevNone) {
				m_DevMap[nID] = DevMO;
				bMO = FALSE;
				break;
			}
		}
	}

	// HD設定
	for (i=0; i<nHD; i++) {
		for (nID=0; nID<SCSI::DeviceMax; nID++) {
			if (m_DevMap[nID] == DevNone) {
				m_DevMap[nID] = DevSCSI;
				break;
			}
		}
	}

	// MO設定
	if (bMO) {
		for (nID=0; nID<SCSI::DeviceMax; nID++) {
			if (m_DevMap[nID] == DevNone) {
				m_DevMap[nID] = DevMO;
				break;
			}
		}
	}

	// CD設定(ID=6固定、もし使われていたら7)
	if (bCD) {
		if (m_DevMap[6] == DevNone) {
			m_DevMap[6] = DevCD;
		}
		else {
			ASSERT(m_DevMap[7] == DevNone);
			m_DevMap[7] = DevCD;
		}
	}
}

//---------------------------------------------------------------------------
//
//	SCSIハードディスク容量チェック
//	※デバイスエラーで0を返す
//
//---------------------------------------------------------------------------
int FASTCALL CSCSIPage::CheckSCSI(int nDrive)
{
	Fileio fio;
	DWORD dwSize;

	ASSERT(this);
	ASSERT((nDrive >= 0) && (nDrive <= SCSI::HDMax));

	// ロック
	::LockVM();

	// ファイルオープン
	if (!fio.Open(m_szFile[nDrive], Fileio::ReadOnly)) {
		// エラーなので0を返す
		fio.Close();
		::UnlockVM();
		return 0;
	}

	// 容量取得
	dwSize = fio.GetFileSize();

	// アンロック
	fio.Close();
	::UnlockVM();

	// ファイルサイズをチェック(512バイト単位)
	if ((dwSize & 0x1ff) != 0) {
		return 0;
	}

	// ファイルサイズをチェック(10MB以上)
	if (dwSize < 10 * 0x400 * 0x400) {
		return 0;
	}

	// ファイルサイズをチェック(4095MB以下)
	if (dwSize > 0xfff00000) {
		return 0;
	}

	// サイズを持ち帰る
	dwSize >>= 20;
	return dwSize;
}

//---------------------------------------------------------------------------
//
//	コントロール状態変更
//
//---------------------------------------------------------------------------
void FASTCALL CSCSIPage::EnableControls(BOOL bEnable, BOOL bDrive)
{
	int i;
	CWnd *pWnd;
	CListCtrl *pListCtrl;

	ASSERT(this);

	// リストコントロール・MOチェック以外の全コントロールを設定
	for (i=0; ; i++) {
		// コントロール取得
		if (!ControlTable[i]) {
			break;
		}
		pWnd = GetDlgItem(ControlTable[i]);
		ASSERT(pWnd);

		// 設定
		pWnd->EnableWindow(bDrive);
	}

	// リストコントロールを設定
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SCSI_LIST);
	ASSERT(pListCtrl);
	pListCtrl->EnableWindow(bEnable);

	// MOチェックを設定
	pWnd = GetDlgItem(IDC_SCSI_MOCHECK);
	ASSERT(pWnd);
	pWnd->EnableWindow(bEnable);
}

//---------------------------------------------------------------------------
//
//	コントロールテーブル
//
//---------------------------------------------------------------------------
const UINT CSCSIPage::ControlTable[] = {
	IDC_SCSI_GROUP,
	IDC_SCSI_DRIVEL,
	IDC_SCSI_DRIVEE,
	IDC_SCSI_DRIVES,
	NULL
};

//===========================================================================
//
//	ポートページ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CPortPage::CPortPage()
{
	// ID,Helpを必ず設定
	m_dwID = MAKEID('P', 'O', 'R', 'T');
	m_nTemplate = IDD_PORTPAGE;
	m_uHelpID = IDC_PORT_HELP;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL CPortPage::OnInitDialog()
{
	int i;
	CComboBox *pComboBox;
	CString strText;
	CButton *pButton;
	CEdit *pEdit;

	// 基本クラス
	CConfigPage::OnInitDialog();

	// COMコンボボックス
	pComboBox = (CComboBox*)GetDlgItem(IDC_PORT_COMC);
	ASSERT(pComboBox);
	pComboBox->ResetContent();
	::GetMsg(IDS_PORT_NOASSIGN, strText);
	pComboBox->AddString(strText);
	for (i=1; i<=9; i++) {
		strText.Format(_T("COM%d"), i);
		pComboBox->AddString(strText);
	}
	pComboBox->SetCurSel(m_pConfig->port_com);

	// 受信ログ
	pEdit = (CEdit*)GetDlgItem(IDC_PORT_RECVE);
	ASSERT(pEdit);
	pEdit->SetWindowText(m_pConfig->port_recvlog);

	// 強制38400bps
	pButton = (CButton*)GetDlgItem(IDC_PORT_BAUDRATE);
	ASSERT(pButton);
	pButton->SetCheck(m_pConfig->port_384);

	// LPTコンボボックス
	pComboBox = (CComboBox*)GetDlgItem(IDC_PORT_LPTC);
	ASSERT(pComboBox);
	pComboBox->ResetContent();
	::GetMsg(IDS_PORT_NOASSIGN, strText);
	pComboBox->AddString(strText);
	for (i=1; i<=9; i++) {
		strText.Format(_T("LPT%d"), i);
		pComboBox->AddString(strText);
	}
	pComboBox->SetCurSel(m_pConfig->port_lpt);

	// 送信ログ
	pEdit = (CEdit*)GetDlgItem(IDC_PORT_SENDE);
	ASSERT(pEdit);
	pEdit->SetWindowText(m_pConfig->port_sendlog);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	決定
//
//---------------------------------------------------------------------------
void CPortPage::OnOK()
{
	CComboBox *pComboBox;
	CEdit *pEdit;
	CButton *pButton;

	// COMコンボボックス
	pComboBox = (CComboBox*)GetDlgItem(IDC_PORT_COMC);
	ASSERT(pComboBox);
	m_pConfig->port_com = pComboBox->GetCurSel();

	// 受信ログ
	pEdit = (CEdit*)GetDlgItem(IDC_PORT_RECVE);
	ASSERT(pEdit);
	pEdit->GetWindowText(m_pConfig->port_recvlog, sizeof(m_pConfig->port_recvlog));

	// 強制38400bps
	pButton = (CButton*)GetDlgItem(IDC_PORT_BAUDRATE);
	ASSERT(pButton);
	m_pConfig->port_384 = pButton->GetCheck();

	// LPTコンボボックス
	pComboBox = (CComboBox*)GetDlgItem(IDC_PORT_LPTC);
	ASSERT(pComboBox);
	m_pConfig->port_lpt = pComboBox->GetCurSel();

	// 送信ログ
	pEdit = (CEdit*)GetDlgItem(IDC_PORT_SENDE);
	ASSERT(pEdit);
	pEdit->GetWindowText(m_pConfig->port_sendlog, sizeof(m_pConfig->port_sendlog));

	// 基本クラス
	CConfigPage::OnOK();
}

//===========================================================================
//
//	MIDIページ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CMIDIPage::CMIDIPage()
{
	// ID,Helpを必ず設定
	m_dwID = MAKEID('M', 'I', 'D', 'I');
	m_nTemplate = IDD_MIDIPAGE;
	m_uHelpID = IDC_MIDI_HELP;

	// オブジェクト
//	m_pMIDI = NULL;
}

//---------------------------------------------------------------------------
//
//	メッセージ マップ
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CMIDIPage, CConfigPage)
	ON_WM_VSCROLL()
	ON_BN_CLICKED(IDC_MIDI_BID0, OnBIDClick)
	ON_BN_CLICKED(IDC_MIDI_BID1, OnBIDClick)
	ON_BN_CLICKED(IDC_MIDI_BID2, OnBIDClick)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL CMIDIPage::OnInitDialog()
{
//	CButton *pButton;
//	CComboBox *pComboBox;
//	CSpinButtonCtrl *pSpin;
//	CFrmWnd *pFrmWnd;
	CString strDesc;
//	int nNum;
//	int i;

	// 基本クラス
	CConfigPage::OnInitDialog();

//	// MIDIコンポーネント取得
//	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
//	ASSERT(pFrmWnd);
//	m_pMIDI = pFrmWnd->GetMIDI();
//	ASSERT(m_pMIDI);
//
//	// コントロール有効・無効
//	m_bEnableCtrl = TRUE;
//	EnableControls(FALSE);
//	if (m_pConfig->midi_bid != 0) {
//		EnableControls(TRUE);
//	}
//
//	// ボードID
//	pButton = (CButton*)GetDlgItem(IDC_MIDI_BID0 + m_pConfig->midi_bid);
//	ASSERT(pButton);
//	pButton->SetCheck(1);
//
//	// 割り込みレベル
//	pButton = (CButton*)GetDlgItem(IDC_MIDI_ILEVEL4 + m_pConfig->midi_ilevel);
//	ASSERT(pButton);
//	pButton->SetCheck(1);
//
//	// 音源リセット
//	pButton = (CButton*)GetDlgItem(IDC_MIDI_RSTGM + m_pConfig->midi_reset);
//	ASSERT(pButton);
//	pButton->SetCheck(1);
//
//	// デバイス(IN)
//	pComboBox = (CComboBox*)GetDlgItem(IDC_MIDI_INC);
//	ASSERT(pComboBox);
//	pComboBox->ResetContent();
//	::GetMsg(IDS_MIDI_NOASSIGN, strDesc);
//	pComboBox->AddString(strDesc);
//	nNum = (int)m_pMIDI->GetInDevs();
//	for (i=0; i<nNum; i++) {
//		m_pMIDI->GetInDevDesc(i, strDesc);
//		pComboBox->AddString(strDesc);
//	}
//
//	// コンボボックスのカーソルを設定
//	if (m_pConfig->midiin_device <= nNum) {
//		pComboBox->SetCurSel(m_pConfig->midiin_device);
//	}
//	else {
//		pComboBox->SetCurSel(0);
//	}
//
//	// デバイス(OUT)
//	pComboBox = (CComboBox*)GetDlgItem(IDC_MIDI_OUTC);
//	ASSERT(pComboBox);
//	pComboBox->ResetContent();
//	::GetMsg(IDS_MIDI_NOASSIGN, strDesc);
//	pComboBox->AddString(strDesc);
//	nNum = (int)m_pMIDI->GetOutDevs();
//	if (nNum >= 1) {
//		::GetMsg(IDS_MIDI_MAPPER, strDesc);
//		pComboBox->AddString(strDesc);
//		for (i=0; i<nNum; i++) {
//			m_pMIDI->GetOutDevDesc(i, strDesc);
//			pComboBox->AddString(strDesc);
//		}
//	}
//
//	// コンボボックスのカーソルを設定
//	if (m_pConfig->midiout_device < (nNum + 2)) {
//		pComboBox->SetCurSel(m_pConfig->midiout_device);
//	}
//	else {
//		pComboBox->SetCurSel(0);
//	}
//
//	// 遅延(IN)
//	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_MIDI_DLYIS);
//	ASSERT(pSpin);
//	pSpin->SetBase(10);
//	pSpin->SetRange(0, 200);
//	pSpin->SetPos(m_pConfig->midiin_delay);
//
//	// 遅延(OUT)
//	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_MIDI_DLYOS);
//	ASSERT(pSpin);
//	pSpin->SetBase(10);
//	pSpin->SetRange(20, 200);
//	pSpin->SetPos(m_pConfig->midiout_delay);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	決定
//
//---------------------------------------------------------------------------
void CMIDIPage::OnOK()
{
	int i;
	CButton *pButton;
	CComboBox *pComboBox;
	CSpinButtonCtrl *pSpin;

	// ボードID
	for (i=0; i<3; i++) {
		pButton = (CButton*)GetDlgItem(IDC_MIDI_BID0 + i);
		ASSERT(pButton);
		if (pButton->GetCheck() == 1) {
			m_pConfig->midi_bid = i;
			break;
		}
	}

	// 割り込みレベル
	for (i=0; i<2; i++) {
		pButton = (CButton*)GetDlgItem(IDC_MIDI_ILEVEL4 + i);
		ASSERT(pButton);
		if (pButton->GetCheck() == 1) {
			m_pConfig->midi_ilevel = i;
			break;
		}
	}

	// 音源リセット
	for (i=0; i<4; i++) {
		pButton = (CButton*)GetDlgItem(IDC_MIDI_RSTGM + i);
		ASSERT(pButton);
		if (pButton->GetCheck() == 1) {
			m_pConfig->midi_reset = i;
			break;
		}
	}

	// デバイス(IN)
	pComboBox = (CComboBox*)GetDlgItem(IDC_MIDI_INC);
	ASSERT(pComboBox);
	m_pConfig->midiin_device = pComboBox->GetCurSel();

	// デバイス(OUT)
	pComboBox = (CComboBox*)GetDlgItem(IDC_MIDI_OUTC);
	ASSERT(pComboBox);
	m_pConfig->midiout_device = pComboBox->GetCurSel();

	// 遅延(IN)
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_MIDI_DLYIS);
	ASSERT(pSpin);
	m_pConfig->midiin_delay = LOWORD(pSpin->GetPos());

	// 遅延(OUT)
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_MIDI_DLYOS);
	ASSERT(pSpin);
	m_pConfig->midiout_delay = LOWORD(pSpin->GetPos());

	// 基本クラス
	CConfigPage::OnOK();
}

//---------------------------------------------------------------------------
//
//	キャンセル
//
//---------------------------------------------------------------------------
void CMIDIPage::OnCancel()
{
//	// MIDIディレイを戻す(IN)
//	m_pMIDI->SetInDelay(m_pConfig->midiin_delay);
//
//	// MIDIディレイを戻す(OUT)
//	m_pMIDI->SetOutDelay(m_pConfig->midiout_delay);
//
	// 基本クラス
	CConfigPage::OnCancel();
}

//---------------------------------------------------------------------------
//
//	縦スクロール
//
//---------------------------------------------------------------------------
void CMIDIPage::OnVScroll(UINT , UINT nPos, CScrollBar *pBar)
{
	CSpinButtonCtrl *pSpin;

	// IN
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_MIDI_DLYIS);
	ASSERT(pSpin);
	if ((CWnd*)pSpin == (CWnd*)pBar) {
//		m_pMIDI->SetInDelay(nPos);
	}

	// OUT
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_MIDI_DLYOS);
	ASSERT(pSpin);
	if ((CWnd*)pSpin == (CWnd*)pBar) {
//		m_pMIDI->SetOutDelay(nPos);
	}
}

//---------------------------------------------------------------------------
//
//	ボードIDクリック
//
//---------------------------------------------------------------------------
void CMIDIPage::OnBIDClick()
{
	CButton *pButton;

	// ボードID「なし」のコントロールを取得
	pButton = (CButton*)GetDlgItem(IDC_MIDI_BID0);
	ASSERT(pButton);

	// チェックがついているかどうかで調べる
	if (pButton->GetCheck() == 1) {
		EnableControls(FALSE);
	}
	else {
		EnableControls(TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	コントロール状態変更
//
//---------------------------------------------------------------------------
void FASTCALL CMIDIPage::EnableControls(BOOL bEnable) 
{
	int i;
	CWnd *pWnd;

	ASSERT(this);

	// フラグチェック
	if (m_bEnableCtrl == bEnable) {
		return;
	}
	m_bEnableCtrl = bEnable;

	// ボードID、Help以外の全コントロールを設定
	for(i=0; ; i++) {
		// 終了チェック
		if (ControlTable[i] == NULL) {
			break;
		}

		// コントロール取得
		pWnd = GetDlgItem(ControlTable[i]);
		ASSERT(pWnd);
		pWnd->EnableWindow(bEnable);
	}
}

//---------------------------------------------------------------------------
//
//	コントロールIDテーブル
//
//---------------------------------------------------------------------------
const UINT CMIDIPage::ControlTable[] = {
	IDC_MIDI_ILEVELG,
	IDC_MIDI_ILEVEL4,
	IDC_MIDI_ILEVEL2,
	IDC_MIDI_RSTG,
	IDC_MIDI_RSTGM,
	IDC_MIDI_RSTGS,
	IDC_MIDI_RSTXG,
	IDC_MIDI_RSTLA,
	IDC_MIDI_DEVG,
	IDC_MIDI_INS,
	IDC_MIDI_INC,
	IDC_MIDI_DLYIL,
	IDC_MIDI_DLYIE,
	IDC_MIDI_DLYIS,
	IDC_MIDI_DLYIG,
	IDC_MIDI_OUTS,
	IDC_MIDI_OUTC,
	IDC_MIDI_DLYOL,
	IDC_MIDI_DLYOE,
	IDC_MIDI_DLYOS,
	IDC_MIDI_DLYOG,
	NULL
};

//===========================================================================
//
//	改造ページ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CAlterPage::CAlterPage()
{
	// ID,Helpを必ず設定
	m_dwID = MAKEID('A', 'L', 'T', ' ');
	m_nTemplate = IDD_ALTERPAGE;
	m_uHelpID = IDC_ALTER_HELP;

	// 初期化
	m_bInit = FALSE;
	m_bParity = FALSE;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL CAlterPage::OnInitDialog()
{
	// 基本クラス
	CConfigPage::OnInitDialog();

	// 初期化済み、パリティフラグを取得しておく
	m_bInit = TRUE;
	m_bParity = m_pConfig->sasi_parity;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ページ移動
//
//---------------------------------------------------------------------------
BOOL CAlterPage::OnKillActive()
{
	CButton *pButton;

	ASSERT(this);

	// チェックボックスをパリティフラグに反映させる
	pButton = (CButton*)GetDlgItem(IDC_ALTER_PARITY);
	ASSERT(pButton);
	if (pButton->GetCheck() == 1) {
		m_bParity = TRUE;
	}
	else {
		m_bParity = FALSE;
	}

	// 基底クラス
	return CConfigPage::OnKillActive();
}

//---------------------------------------------------------------------------
//
//	データ交換
//
//---------------------------------------------------------------------------
void CAlterPage::DoDataExchange(CDataExchange *pDX)
{
	ASSERT(this);
	ASSERT(pDX);

	// 基本クラス
	CConfigPage::DoDataExchange(pDX);

	// データ交換
	DDX_Check(pDX, IDC_ALTER_SRAM, m_pConfig->sram_64k);
	DDX_Check(pDX, IDC_ALTER_SCC, m_pConfig->scc_clkup);
	DDX_Check(pDX, IDC_ALTER_POWERLED, m_pConfig->power_led);
	DDX_Check(pDX, IDC_ALTER_2DD, m_pConfig->dual_fdd);
	DDX_Check(pDX, IDC_ALTER_PARITY, m_pConfig->sasi_parity);
}

//---------------------------------------------------------------------------
//
//	SASIパリティ機能チェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL CAlterPage::HasParity(const Config *pConfig) const
{
	ASSERT(this);
	ASSERT(pConfig);

	// 初期化されていなければ、与えれたConfigデータから
	if (!m_bInit) {
		return pConfig->sasi_parity;
	}

	// 初期化済みなら、最新の編集結果を知らせる
	return m_bParity;
}

//===========================================================================
//
//	レジュームページ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CResumePage::CResumePage()
{
	// ID,Helpを必ず設定
	m_dwID = MAKEID('R', 'E', 'S', 'M');
	m_nTemplate = IDD_RESUMEPAGE;
	m_uHelpID = IDC_RESUME_HELP;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL CResumePage::OnInitDialog()
{
	// 基本クラス
	CConfigPage::OnInitDialog();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	データ交換
//
//---------------------------------------------------------------------------
void CResumePage::DoDataExchange(CDataExchange *pDX)
{
	ASSERT(this);
	ASSERT(pDX);

	// 基本クラス
	CConfigPage::DoDataExchange(pDX);

	// データ交換
	DDX_Check(pDX, IDC_RESUME_FDC, m_pConfig->resume_fd);
	DDX_Check(pDX, IDC_RESUME_MOC, m_pConfig->resume_mo);
	DDX_Check(pDX, IDC_RESUME_CDC, m_pConfig->resume_cd);
	DDX_Check(pDX, IDC_RESUME_XM6C, m_pConfig->resume_state);
	DDX_Check(pDX, IDC_RESUME_SCREENC, m_pConfig->resume_screen);
	DDX_Check(pDX, IDC_RESUME_DIRC, m_pConfig->resume_dir);
}
//===========================================================================
//
//	その他ページ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CMiscPage::CMiscPage()
{
	// ID,Helpを必ず設定
	m_dwID = MAKEID('M', 'I', 'S', 'C');
	m_nTemplate = IDD_MISCPAGE;
	m_uHelpID = IDC_MISC_HELP;
}

//---------------------------------------------------------------------------
//
//	データ交換
//
//---------------------------------------------------------------------------
void CMiscPage::DoDataExchange(CDataExchange *pDX)
{
	ASSERT(this);
	ASSERT(pDX);

	// 基本クラス
	CConfigPage::DoDataExchange(pDX);

	// データ交換
	DDX_Check(pDX, IDC_MISC_FDSPEED, m_pConfig->floppy_speed);
	DDX_Check(pDX, IDC_MISC_FDLED, m_pConfig->floppy_led);
	DDX_Check(pDX, IDC_MISC_POPUP, m_pConfig->popup_swnd);
	DDX_Check(pDX, IDC_MISC_AUTOMOUSE, m_pConfig->auto_mouse);
	DDX_Check(pDX, IDC_MISC_POWEROFF, m_pConfig->power_off);
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

	// ページ初期化
	m_Basic.Init(this);
	m_Sound.Init(this);
	m_Vol.Init(this);
//	m_Kbd.Init(this);
//	m_Mouse.Init(this);
//	m_Joy.Init(this);
	m_SASI.Init(this);
	m_SxSI.Init(this);
	m_SCSI.Init(this);
	m_Port.Init(this);
	m_MIDI.Init(this);
	m_Alter.Init(this);
	m_Resume.Init(this);
//	m_TKey.Init(this);
	m_Misc.Init(this);
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
