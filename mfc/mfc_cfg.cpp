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
/*
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
*/

	////////////////////////////////////////////////////////////////////////////////////////////////
	Config* c = &m_Config;
	//	Config200
	// システム
	c->system_clock			= 5;					// システムクロック(0～5)
	c->ram_size				= 0;						// メインRAMサイズ(0～5)
	c->ram_sramsync			= TRUE;					// メモリスイッチ自動更新

	// スケジューラ
	c->mpu_fullspeed		= FALSE;					// MPUフルスピード
	c->vm_fullspeed			= FALSE;					// VMフルスピード

	// サウンド
	c->sound_device			= 0;					// サウンドデバイス(0～15)
	c->sample_rate			= 5;					// サンプリングレート(0～4)
	c->primary_buffer		= 10;					// バッファサイズ(2～100)
	c->polling_buffer		= 5;					// ポーリング間隔(0～99)
	c->adpcm_interp			= TRUE;					// ADPCM線形補間あり

	// 描画
	c->aspect_stretch		= TRUE;				// アスペクト比にあわせ拡大

	// 音量
	c->master_volume		= 100;					// マスタ音量(0～100)
	c->fm_enable			= TRUE;						// FM有効
	c->fm_volume			= 54;						// FM音量(0～100)
	c->adpcm_enable			= TRUE;					// ADPCM有効
	c->adpcm_volume			= 52;					// ADPCM音量(0～100)

	// キーボード
	c->kbd_connect			= TRUE;					// 接続

	// マウス
	c->mouse_speed			= 205;					// スピード
	c->mouse_port			= 1;						// 接続ポート
	c->mouse_swap			= FALSE;					// ボタンスワップ
	c->mouse_mid			= TRUE;						// 中ボタンイネーブル
	c->mouse_trackb			= FALSE;					// トラックボールモード

	// ジョイスティック
	c->joy_type[0]			= 1;					// ジョイスティックタイプ
	c->joy_type[1]			= 1;					// ジョイスティックタイプ
	c->joy_dev[0]			= 1;						// ジョイスティックデバイス
	c->joy_dev[1]			= 2;						// ジョイスティックデバイス
	c->joy_button0[0]		= 1;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[1]		= 2;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[2]		= 3;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[3]		= 4;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[4]		= 5;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[5]		= 6;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[6]		= 7;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[7]		= 8;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[8]		= 0;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[9]		= 0;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[10]		= 0;				// ジョイスティックボタン(デバイスA)
	c->joy_button0[11]		= 0;				// ジョイスティックボタン(デバイスA)
	c->joy_button1[0]		= 65537;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[1]		= 65538;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[2]		= 65539;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[3]		= 65540;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[4]		= 65541;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[5]		= 65542;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[6]		= 65543;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[7]		= 65544;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[8]		= 0;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[9]		= 0;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[10]		= 0;				// ジョイスティックボタン(デバイスB)
	c->joy_button1[11]		= 0;				// ジョイスティックボタン(デバイスB)

	// SASI
	c->sasi_drives			= 1;					// SASIドライブ数
	c->sasi_sramsync		= TRUE;					// SASIメモリスイッチ自動更新
	strcpy(&c->sasi_file[ 0][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD0.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[ 1][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD1.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[ 2][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD2.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[ 3][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD3.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[ 4][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD4.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[ 5][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD5.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[ 6][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD6.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[ 7][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD7.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[ 8][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD8.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[ 9][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD9.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[10][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD10.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[11][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD11.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[12][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD12.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[13][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD13.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[14][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD14.HDF"));				// SASIイメージファイル
	strcpy(&c->sasi_file[15][0], _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\HD15.HDF"));				// SASIイメージファイル

	// SxSI
	c->sxsi_drives			= 0;							// SxSIドライブ数
	c->sxsi_mofirst			= FALSE;						// MOドライブ優先割り当て
	memset(&c->sxsi_file[0][0], 0, sizeof(c->sxsi_file));	// SxSIイメージファイル

	// ポート
	c->port_com				= 0;								// COMxポート
	memset(&c->port_recvlog[0], 0, sizeof(c->port_recvlog));	// シリアル受信ログ
	c->port_384				= FALSE;							// シリアル38400bps固定
	c->port_lpt				= 0;								// LPTxポート
	memset(&c->port_sendlog[0], 0, sizeof(c->port_sendlog));	// パラレル送信ログ

	// MIDI
	c->midi_bid				= 0;							// MIDIボードID
	c->midi_ilevel			= 0;							// MIDI割り込みレベル
	c->midi_reset			= 0;							// MIDIリセットコマンド
	c->midiin_device		= 0;							// MIDI INデバイス
	c->midiin_delay			= 0;							// MIDI INディレイ(ms)
	c->midiout_device		= 0;							// MIDI OUTデバイス
	c->midiout_delay		= 84;							// MIDI OUTディレイ(ms)

	// 改造
	c->sram_64k				= FALSE;						// 64KB SRAM
	c->scc_clkup			= FALSE;						// SCCクロックアップ
	c->power_led			= FALSE;						// 青色電源LED
	c->dual_fdd				= FALSE;						// 2DD/2HD兼用FDD
	c->sasi_parity			= FALSE;						// SASIバスパリティ

	// TrueKey
	c->tkey_mode			= 1;							// TrueKeyモード(bit0:VM bit1:WinApp)
	c->tkey_com				= 0;							// キーボードCOMポート
	c->tkey_rts				= FALSE;						// RTS反転モード

	// その他
	c->floppy_speed			= TRUE;							// フロッピーディスク高速
	c->floppy_led			= TRUE;							// フロッピーディスクLEDモード
	c->popup_swnd			= TRUE;							// ポップアップサブウィンドウ
	c->auto_mouse			= FALSE;						// 自動マウスモード制御
	c->power_off			= FALSE;						// 電源OFFで開始

	//	Config202
	// システム
	c->mem_type				= 1;		// メモリマップ種別

	// SCSI
	c->scsi_ilevel			= 1;		// SCSI割り込みレベル
	c->scsi_drives			= 0;		// SCSIドライブ数
	c->scsi_sramsync		= 1;		// SCSIメモリスイッチ自動更新
	c->scsi_mofirst			= 0;		// MOドライブ優先割り当て
	memset(&c->scsi_file[0][0], 0, sizeof(c->scsi_file));	// SCSIイメージファイル

	//	Config
	// レジューム
	c->resume_fd			= FALSE;	// FDレジューム
	c->resume_fdi[0]		= TRUE;		// FD挿入フラグ
	c->resume_fdi[1]		= FALSE;	// FD挿入フラグ
	c->resume_fdw[0]		= FALSE;	// FD書き込み禁止
	c->resume_fdw[1]		= FALSE;	// FD書き込み禁止
	c->resume_fdm[0]		= 0;		// FDメディアNo.
	c->resume_fdm[1]		= 0;		// FDメディアNo.
	c->resume_mo			= 0;		// MOレジューム
	c->resume_mos			= 0;		// MO挿入フラグ
	c->resume_mow			= 0;		// MO書き込み禁止
	c->resume_cd			= 0;		// CDレジューム
	c->resume_iso			= 0;		// CD挿入フラグ
	c->resume_state			= 0;		// ステートレジューム
	c->resume_xm6			= 0;		// ステート有効フラグ
	c->resume_screen		= 0;		// 画面モードレジューム
	c->resume_dir			= 0;		// デフォルトディレクトリレジューム
	strcpy(c->resume_path, _T("C:\\projects\\x68k\\xm6_205s\\00proj.vc10\\Debug\\"));

	// 描画
	c->caption_info			= 1;		// キャプション情報表示

	// ディスプレイ
	c->caption				= 1;		// キャプション
	c->menu_bar				= 1;		// メニューバー
	c->status_bar			= 1;		// ステータスバー
	c->window_left			= 543;		// ウィンドウ矩形
	c->window_top			= 231;		// ウィンドウ矩形
	c->window_full			= 0;		// フルスクリーン
	c->window_mode			= 0;		// ワイドスクリーン

	// WINDRVモジュール
	c->windrv_enable		= 0;		// Windrvサポート 0:無効 1:WindrvXM (2:Windrv互換)

	// ホスト側ファイルシステム
	c->host_option			= 0;		// 動作フラグ (class CHostFilename 参照)
	c->host_resume			= FALSE;	// ベースパス状態復元有効 FALSEだと毎回スキャンする
	c->host_drives			= 0;		// 有効なドライブ数
	memset(&c->host_flag[0], 0, sizeof(c->host_flag));		// 動作フラグ (class CWinFileDrv 参照)
	memset(&c->host_path[0][0], 0, sizeof(c->host_path));		// ベースパス

////////////////////////////////////////////////////////////////////////////////////////////////
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
#endif	// _WIN32
