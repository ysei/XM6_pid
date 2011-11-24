//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC フレームウィンドウ ]
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
#include "mfc_frm.h"
#include "mfc_draw.h"
#include "mfc_res.h"
#include "mfc_com.h"
#include "mfc_sch.h"
#include "mfc_snd.h"
#include "mfc_inp.h"
#include "mfc_host.h"
#include "mfc_cfg.h"

//===========================================================================
//
//	フレームウィンドウ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	シェル定数定義
//	※includeファイルではなく、アプリケーション側で定義するよう定められている
//
//---------------------------------------------------------------------------
#define SHCNRF_InterruptLevel			0x0001
#define SHCNRF_ShellLevel				0x0002
#define SHCNRF_NewDelivery				0x8000

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CFrmWnd::CFrmWnd()
{
	// VM・ステータスコード
	::pVM = NULL;
	m_nStatus = -1;

	// デバイス
	m_pFDD = NULL;
	m_pSASI = NULL;
	m_pSCSI = NULL;
	m_pScheduler = NULL;
	m_pKeyboard = NULL;
	m_pMouse = NULL;

	// コンポーネント
	m_pFirstComponent = NULL;
	m_pDrawView = NULL;
//	m_pStatusView = NULL;
	m_pSch = NULL;
	m_pSound = NULL;
	m_pInput = NULL;
//	m_pPort = NULL;
//	m_pMIDI = NULL;
//	m_pTKey = NULL;
	m_pHost = NULL;
//	m_pInfo = NULL;
	m_pConfig = NULL;

	// フルスクリーン
	m_bFullScreen = FALSE;
	m_hTaskBar = NULL;
	memset(&m_DevMode, 0, sizeof(m_DevMode));
	m_nWndLeft = 0;
	m_nWndTop = 0;

	// サブウィンドウ
	m_strWndClsName.Empty();

	// ステータスバー・メニュー・キャプション
//	m_bStatusBar = FALSE;
	m_bMenuBar = TRUE;
//	m_bCaption = TRUE;

	// シェル通知
	m_uNotifyId = NULL;

	// コンフィギュレーション
	m_bMouseMid = TRUE;
	m_bPopup = FALSE;
	m_bAutoMouse = TRUE;

	// その他変数
	m_bExit = FALSE;
	m_bSaved = FALSE;
	m_nFDDStatus[0] = 0;
	m_nFDDStatus[1] = 0;
	m_dwExec = 0;
}

//---------------------------------------------------------------------------
//
//	メッセージ マップ
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

	ON_COMMAND(IDM_OPEN, OnOpen)
	ON_UPDATE_COMMAND_UI(IDM_OPEN, OnOpenUI)
	ON_COMMAND(IDM_SAVE, OnSave)
	ON_UPDATE_COMMAND_UI(IDM_SAVE, OnSaveUI)
	ON_COMMAND(IDM_SAVEAS, OnSaveAs)
	ON_UPDATE_COMMAND_UI(IDM_SAVEAS, OnSaveAsUI)
	ON_COMMAND(IDM_RESET, OnReset)
	ON_UPDATE_COMMAND_UI(IDM_RESET, OnResetUI)
	ON_COMMAND(IDM_INTERRUPT, OnInterrupt)
	ON_UPDATE_COMMAND_UI(IDM_INTERRUPT, OnInterruptUI)
	ON_COMMAND(IDM_POWER, OnPower)
	ON_UPDATE_COMMAND_UI(IDM_POWER, OnPowerUI)
	ON_COMMAND_RANGE(IDM_XM6_MRU0, IDM_XM6_MRU8, OnMRU)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_XM6_MRU0, IDM_XM6_MRU8, OnMRUUI)
	ON_COMMAND(IDM_EXIT, OnExit)

	ON_COMMAND_RANGE(IDM_D0OPEN, IDM_D1_MRU8, OnFD)
	ON_UPDATE_COMMAND_UI(IDM_D0OPEN, OnFDOpenUI)
	ON_UPDATE_COMMAND_UI(IDM_D1OPEN, OnFDOpenUI)
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
	ON_UPDATE_COMMAND_UI_RANGE(IDM_D0_MRU0, IDM_D0_MRU8, OnFDMRUUI)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_D1_MRU0, IDM_D1_MRU8, OnFDMRUUI)

//	ON_COMMAND(IDM_MOOPEN, OnMOOpen)
//	ON_UPDATE_COMMAND_UI(IDM_MOOPEN, OnMOOpenUI)
//	ON_COMMAND(IDM_MOEJECT, OnMOEject)
//	ON_UPDATE_COMMAND_UI(IDM_MOEJECT, OnMOEjectUI)
//	ON_COMMAND(IDM_MOWRITEP, OnMOWriteP)
//	ON_UPDATE_COMMAND_UI(IDM_MOWRITEP, OnMOWritePUI)
//	ON_COMMAND(IDM_MOFORCE, OnMOForce)
//	ON_UPDATE_COMMAND_UI(IDM_MOFORCE, OnMOForceUI)
//	ON_COMMAND_RANGE(IDM_MO_MRU0, IDM_MO_MRU8, OnMOMRU)
//	ON_UPDATE_COMMAND_UI_RANGE(IDM_MO_MRU0, IDM_MO_MRU8, OnMOMRUUI)

//	ON_COMMAND(IDM_CDOPEN, OnCDOpen)
//	ON_UPDATE_COMMAND_UI(IDM_CDOPEN, OnCDOpenUI)
//	ON_COMMAND(IDM_CDEJECT, OnCDEject)
//	ON_UPDATE_COMMAND_UI(IDM_CDEJECT, OnCDEjectUI)
//	ON_COMMAND(IDM_CDFORCE, OnCDForce)
//	ON_UPDATE_COMMAND_UI(IDM_CDFORCE, OnCDForceUI)
//	ON_COMMAND_RANGE(IDM_CD_MRU0, IDM_CD_MRU8, OnCDMRU)
//	ON_UPDATE_COMMAND_UI_RANGE(IDM_CD_MRU0, IDM_CD_MRU8, OnCDMRUUI)
//
//	ON_COMMAND(IDM_LOG, OnLog)
//	ON_UPDATE_COMMAND_UI(IDM_LOG, OnLogUI)
//	ON_COMMAND(IDM_SCHEDULER, OnScheduler)
//	ON_UPDATE_COMMAND_UI(IDM_SCHEDULER, OnSchedulerUI)
//	ON_COMMAND(IDM_DEVICE, OnDevice)
//	ON_UPDATE_COMMAND_UI(IDM_DEVICE, OnDeviceUI)
//	ON_COMMAND(IDM_CPUREG, OnCPUReg)
//	ON_UPDATE_COMMAND_UI(IDM_CPUREG, OnCPURegUI)
//	ON_COMMAND(IDM_INT, OnInt)
//	ON_UPDATE_COMMAND_UI(IDM_INT, OnIntUI)
//	ON_COMMAND(IDM_DISASM, OnDisasm)
//	ON_UPDATE_COMMAND_UI(IDM_DISASM, OnDisasmUI)
//	ON_COMMAND(IDM_MEMORY, OnMemory)
//	ON_UPDATE_COMMAND_UI(IDM_MEMORY, OnMemoryUI)
//	ON_COMMAND(IDM_BREAKP, OnBreakP)
//	ON_UPDATE_COMMAND_UI(IDM_BREAKP, OnBreakPUI)
//	ON_COMMAND(IDM_MFP, OnMFP)
//	ON_UPDATE_COMMAND_UI(IDM_MFP, OnMFPUI)
//	ON_COMMAND(IDM_DMAC, OnDMAC)
//	ON_UPDATE_COMMAND_UI(IDM_DMAC, OnDMACUI)
//	ON_COMMAND(IDM_CRTC, OnCRTC)
//	ON_UPDATE_COMMAND_UI(IDM_CRTC, OnCRTCUI)
//	ON_COMMAND(IDM_VC, OnVC)
//	ON_UPDATE_COMMAND_UI(IDM_VC, OnVCUI)
//	ON_COMMAND(IDM_RTC, OnRTC)
//	ON_UPDATE_COMMAND_UI(IDM_RTC, OnRTCUI)
//	ON_COMMAND(IDM_OPM, OnOPM)
//	ON_UPDATE_COMMAND_UI(IDM_OPM, OnOPMUI)
//	ON_COMMAND(IDM_KEYBOARD, OnKeyboard)
//	ON_UPDATE_COMMAND_UI(IDM_KEYBOARD, OnKeyboardUI)
//	ON_COMMAND(IDM_FDD, OnFDD)
//	ON_UPDATE_COMMAND_UI(IDM_FDD, OnFDDUI)
//	ON_COMMAND(IDM_FDC, OnFDC)
//	ON_UPDATE_COMMAND_UI(IDM_FDC, OnFDCUI)
//	ON_COMMAND(IDM_SCC, OnSCC)
//	ON_UPDATE_COMMAND_UI(IDM_SCC, OnSCCUI)
//	ON_COMMAND(IDM_CYNTHIA, OnCynthia)
//	ON_UPDATE_COMMAND_UI(IDM_CYNTHIA, OnCynthiaUI)
//	ON_COMMAND(IDM_SASI, OnSASI)
//	ON_UPDATE_COMMAND_UI(IDM_SASI, OnSASIUI)
//	ON_COMMAND(IDM_MIDI, OnMIDI)
//	ON_UPDATE_COMMAND_UI(IDM_MIDI, OnMIDIUI)
//	ON_COMMAND(IDM_SCSI, OnSCSI)
//	ON_UPDATE_COMMAND_UI(IDM_SCSI, OnSCSIUI)
//	ON_COMMAND(IDM_TVRAM, OnTVRAM)
//	ON_UPDATE_COMMAND_UI(IDM_TVRAM, OnTVRAMUI)
//	ON_COMMAND(IDM_G1024, OnG1024)
//	ON_UPDATE_COMMAND_UI(IDM_G1024, OnG1024UI)
//	ON_COMMAND_RANGE(IDM_G16P0, IDM_G16P3, OnG16)
//	ON_UPDATE_COMMAND_UI_RANGE(IDM_G16P0, IDM_G16P3, OnG16UI)
//	ON_COMMAND_RANGE(IDM_G256P0, IDM_G256P1, OnG256)
//	ON_UPDATE_COMMAND_UI_RANGE(IDM_G256P0, IDM_G256P1, OnG256UI)
//	ON_COMMAND(IDM_G64K, OnG64K)
//	ON_UPDATE_COMMAND_UI(IDM_G64K, OnG64KUI)
//	ON_COMMAND(IDM_PCG, OnPCG)
//	ON_UPDATE_COMMAND_UI(IDM_PCG, OnPCGUI)
//	ON_COMMAND_RANGE(IDM_BG0, IDM_BG1, OnBG)
//	ON_UPDATE_COMMAND_UI_RANGE(IDM_BG0, IDM_BG1, OnBGUI)
//	ON_COMMAND(IDM_PALET, OnPalet)
//	ON_UPDATE_COMMAND_UI(IDM_PALET, OnPaletUI)
//	ON_COMMAND(IDM_REND_TEXT, OnTextBuf)
//	ON_UPDATE_COMMAND_UI(IDM_REND_TEXT, OnTextBufUI)
//	ON_COMMAND_RANGE(IDM_REND_GP0, IDM_REND_GP3, OnGrpBuf)
//	ON_UPDATE_COMMAND_UI_RANGE(IDM_REND_GP0, IDM_REND_GP3, OnGrpBufUI)
//	ON_COMMAND(IDM_REND_PCG, OnPCGBuf)
//	ON_UPDATE_COMMAND_UI(IDM_REND_PCG, OnPCGBufUI)
//	ON_COMMAND(IDM_REND_BGSPRITE, OnBGSpBuf)
//	ON_UPDATE_COMMAND_UI(IDM_REND_BGSPRITE, OnBGSpBufUI)
//	ON_COMMAND(IDM_REND_PALET, OnPaletBuf)
//	ON_UPDATE_COMMAND_UI(IDM_REND_PALET, OnPaletBufUI)
//	ON_COMMAND(IDM_REND_MIX, OnMixBuf)
//	ON_UPDATE_COMMAND_UI(IDM_REND_MIX, OnMixBufUI)
//	ON_COMMAND(IDM_COMPONENT, OnComponent)
//	ON_UPDATE_COMMAND_UI(IDM_COMPONENT, OnComponentUI)
//	ON_COMMAND(IDM_OSINFO, OnOSInfo)
//	ON_UPDATE_COMMAND_UI(IDM_OSINFO, OnOSInfoUI)
//	ON_COMMAND(IDM_SOUND, OnSound)
//	ON_UPDATE_COMMAND_UI(IDM_SOUND, OnSoundUI)
//	ON_COMMAND(IDM_INPUT, OnInput)
//	ON_UPDATE_COMMAND_UI(IDM_INPUT, OnInputUI)
//	ON_COMMAND(IDM_PORT, OnPort)
//	ON_UPDATE_COMMAND_UI(IDM_PORT, OnPortUI)
//	ON_COMMAND(IDM_BITMAP, OnBitmap)
//	ON_UPDATE_COMMAND_UI(IDM_BITMAP, OnBitmapUI)
//	ON_COMMAND(IDM_MIDIDRV, OnMIDIDrv)
//	ON_UPDATE_COMMAND_UI(IDM_MIDIDRV, OnMIDIDrvUI)
//	ON_COMMAND(IDM_CAPTION, OnCaption)
//	ON_UPDATE_COMMAND_UI(IDM_CAPTION, OnCaptionUI)
//	ON_COMMAND(IDM_MENU, OnMenu)
//	ON_UPDATE_COMMAND_UI(IDM_MENU, OnMenuUI)
//	ON_COMMAND(IDM_STATUS, OnStatus)
//	ON_UPDATE_COMMAND_UI(IDM_STATUS, OnStatusUI)
//	ON_COMMAND(IDM_REFRESH, OnRefresh)
//	ON_COMMAND(IDM_STRETCH, OnStretch)
//	ON_UPDATE_COMMAND_UI(IDM_STRETCH, OnStretchUI)
//	ON_COMMAND(IDM_FULLSCREEN, OnFullScreen)
//	ON_UPDATE_COMMAND_UI(IDM_FULLSCREEN, OnFullScreenUI)

//	ON_COMMAND(IDM_EXEC, OnExec)
//	ON_UPDATE_COMMAND_UI(IDM_EXEC, OnExecUI)
//	ON_COMMAND(IDM_BREAK, OnBreak)
//	ON_UPDATE_COMMAND_UI(IDM_BREAK, OnBreakUI)
//	ON_COMMAND(IDM_TRACE, OnTrace)
//	ON_UPDATE_COMMAND_UI(IDM_TRACE, OnTraceUI)

//	ON_COMMAND(IDM_MOUSEMODE, OnMouseMode)
//	ON_COMMAND(IDM_SOFTKEY, OnSoftKey)
//	ON_UPDATE_COMMAND_UI(IDM_SOFTKEY, OnSoftKeyUI)
//	ON_COMMAND(IDM_TIMEADJ, OnTimeAdj)
//	ON_COMMAND(IDM_TRAP0, OnTrap)
//	ON_UPDATE_COMMAND_UI(IDM_TRAP0, OnTrapUI)
//	ON_COMMAND(IDM_SAVEWAV, OnSaveWav)
//	ON_UPDATE_COMMAND_UI(IDM_SAVEWAV, OnSaveWavUI)
//	ON_COMMAND(IDM_NEWFD, OnNewFD)
//	ON_COMMAND_RANGE(IDM_NEWSASI, IDM_NEWMO, OnNewDisk)
	ON_COMMAND(IDM_OPTIONS, OnOptions)

//	ON_COMMAND(IDM_CASCADE, OnCascade)
//	ON_UPDATE_COMMAND_UI(IDM_CASCADE, OnCascadeUI)
//	ON_COMMAND(IDM_TILE, OnTile)
//	ON_UPDATE_COMMAND_UI(IDM_TILE, OnTileUI)
//	ON_COMMAND(IDM_ICONIC, OnIconic)
//	ON_UPDATE_COMMAND_UI(IDM_ICONIC, OnIconicUI)
//	ON_COMMAND(IDM_ARRANGEICON, OnArrangeIcon)
//	ON_UPDATE_COMMAND_UI(IDM_ARRANGEICON, OnArrangeIconUI)
//	ON_COMMAND(IDM_HIDE, OnHide)
//	ON_UPDATE_COMMAND_UI(IDM_HIDE, OnHideUI)
//	ON_COMMAND(IDM_RESTORE, OnRestore)
//	ON_UPDATE_COMMAND_UI(IDM_RESTORE, OnRestoreUI)
//	ON_COMMAND_RANGE(IDM_SWND_START, IDM_SWND_END, OnWindow)

//	ON_COMMAND(IDM_ABOUT, OnAbout)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL CFrmWnd::Init()
{
	// ウィンドウ作成
	if (!Create(NULL, _T("XM6"),
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
			WS_BORDER | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
			rectDefault, NULL, NULL, 0, NULL)) {
		return FALSE;
	}

	// それ以外の初期化はOnCrateに任せる
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ウィンドウ作成準備
//
//---------------------------------------------------------------------------
BOOL CFrmWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	// 基本クラス
	if (!CFrameWnd::PreCreateWindow(cs)) {
		return FALSE;
	}

	// クライアントエッジを外す
	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ウィンドウ作成
//
//---------------------------------------------------------------------------
int CFrmWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	LONG lUser;
	CMenu *pSysMenu;
	UINT nCount;
	CString string;

	// 基本クラス
	if (CFrameWnd::OnCreate(lpCreateStruct) != 0) {
		return -1;
	}

	// ユーザデータ指定
	lUser = (LONG)MAKEID('X', 'M', '6', ' ');
	::SetWindowLong(m_hWnd, GWL_USERDATA, lUser);

	// アクセラレータ指定、アイコン指定、IMM指定
	LoadAccelTable(MAKEINTRESOURCE(IDR_ACCELERATOR));
	SetIcon(AfxGetApp()->LoadIcon(IDI_APPICON), TRUE);
	::ImmAssociateContext(m_hWnd, (HIMC)NULL);

	// メニュー(ウィンドウ)
	if (::IsJapanese()) {
		// 日本語メニュー
		m_Menu.LoadMenu(IDR_MENU);
		m_PopupMenu.LoadMenu(IDR_MENUPOPUP);
	}
	else {
		// 英語メニュー
		m_Menu.LoadMenu(IDR_US_MENU);
		m_PopupMenu.LoadMenu(IDR_US_MENUPOPUP);
	}
	SetMenu(&m_Menu);
	m_bMenuBar = TRUE;
	m_bPopupMenu = FALSE;

	// メニュー(システム)
	::GetMsg(IDS_STDWIN, string);
	pSysMenu = GetSystemMenu(FALSE);
	ASSERT(pSysMenu);
	nCount = pSysMenu->GetMenuItemCount();

	// 「ウィンドウ標準位置」を挿入
	pSysMenu->InsertMenu(nCount - 2, MF_BYPOSITION | MF_STRING, IDM_STDWIN, string);
	pSysMenu->InsertMenu(nCount - 2, MF_BYPOSITION | MF_SEPARATOR);

	// チャイルドウィンドウ初期化
	if (!InitChild()) {
		return -1;
	}

	// ウィンドウ位置、矩形初期化
	InitPos();

	// シェル通知初期化
	InitShell();

	// VM初期化
	if (!InitVM()) {
		// VM初期化エラー
		m_nStatus = 1;
		PostMessage(WM_KICK, 0, 0);
		return 0;
	}

	// バージョンリソースからVMへバージョンを伝達
	InitVer();

	// デバイス記憶
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

	// コンポーネント作成、初期化
	if (!InitComponent()) {
		// コンポーネント初期化エラー
		m_nStatus = 2;
		PostMessage(WM_KICK, 0, 0);
		return 0;
	}

	// 設定適用(OnOptionと同様、VMロックして)
	::LockVM();
	ApplyCfg();
	::UnlockVM();

	// リセット
	::GetVM()->Reset();

	// ウインドウ位置をレジューム(m_nStatus != 0に留意)
	ASSERT(m_nStatus != 0);
	RestoreFrameWnd(FALSE);

	// メッセージをポストして終了
	m_nStatus = 0;
	PostMessage(WM_KICK, 0, 0);
	return 0;
}

//---------------------------------------------------------------------------
//
//	チャイルドウィンドウ初期化
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

	// ビュー作成
	m_pDrawView = new CDrawView;
	if (!m_pDrawView->Init(this)) {
		return FALSE;
	}

	// ステータスバーワーク初期化
//	ResetStatus();
//
//	// ステータスバー作成
//	if (!m_StatusBar.Create(this, WS_CHILD | WS_VISIBLE | CBRS_BOTTOM,
//							AFX_IDW_STATUS_BAR)) {
//		return FALSE;
//	}
//	m_bStatusBar = TRUE;
	uIndicator[0] = ID_SEPARATOR;
	for (i=1; i<6; i++) {
		uIndicator[i] = (UINT)i;
	}
//	m_StatusBar.SetIndicators(uIndicator, 6);

	// テキストメトリックを取得
	hDC = ::GetDC(m_hWnd);
	hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	hDefFont = (HFONT)::SelectObject(hDC, hFont);
	ASSERT(hDefFont);
	::GetTextMetrics(hDC, &tm);
	::SelectObject(hDC, hDefFont);
	::ReleaseDC(m_hWnd, hDC);

	// サイズ設定ループ
//	m_StatusBar.SetPaneInfo(0, 0, SBPS_NOBORDERS | SBPS_STRETCH, 0);
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
//		m_StatusBar.SetPaneInfo(i, i, SBPS_NORMAL | SBPS_OWNERDRAW, nWidth);
	}

	// 再レイアウト
	RecalcLayout();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	位置・矩形初期化
//	※bStart=FALSEの場合、bFullScreen=FALSEの時に位置を復元すること
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

	// スクリーンサイズ、ウィンドウ矩形を取得
	cx = ::GetSystemMetrics(SM_CXSCREEN);
	cy = ::GetSystemMetrics(SM_CYSCREEN);
	GetWindowRect(&rectWnd);

	// 800x600以下はスクリーンサイズいっぱいに広げる
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

	// 824x560(DDP2)をノンインターレースの最大サイズと認定
	rect.left = 0;
	rect.top = 0;
	rect.right = 824;
	rect.bottom = 560;
	::AdjustWindowRectEx(&rect, GetView()->GetStyle(), FALSE, GetView()->GetExStyle());
//	m_StatusBar.GetWindowRect(&rectStatus);
	rect.bottom += rectStatus.Height();
	::AdjustWindowRectEx(&rect, GetStyle(), TRUE, GetExStyle());

	// rect.left, rect.bottomが負になるらしい(これ以降、right,bottomはcx,cyを示す)
	rect.right -= rect.left;
	rect.left = 0;
	rect.bottom -= rect.top;
	rect.top = 0;

	// 余裕があれば、センタリング
	if (rect.right < cx) {
		rect.left = (cx - rect.right) / 2;
	}
	if (rect.bottom < cy) {
		rect.top = (cy - rect.bottom) / 2;
	}

	// bStartで分ける(初期開始か、ウィンドウ－フルスクリーンの切り替えか)
	if (bStart) {
		// ウィンドウ位置を一旦保存(この後、再度RestoreFrameWndのチャンスあり)
		m_nWndLeft = rect.left;
		m_nWndTop = rect.top;
	}
	else {
		// ウィンドウモードの時に限り、位置を補正
		if (!m_bFullScreen) {
			if ((rect.left == 0) && (rect.top == 0)) {
				// WM_DISPLAYCHANGEメッセージが来て、ウィンドウが小さくなった場合
				m_nWndLeft = rect.left;
				m_nWndTop = rect.top;
			}
			else {
				// それ以外(フルスクリーン→ウィンドウへの状態遷移を含む)
				rect.left = m_nWndLeft;
				rect.top = m_nWndTop;
			}
		}
	}

	// 設定
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
//	シェル連携初期化
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::InitShell()
{
	int nSources;

	// 通知要因を設定
	if (::IsWinNT()) {
		// Windows2000/XP:shared memoryを利用するフラグを追加
		nSources = SHCNRF_InterruptLevel | SHCNRF_ShellLevel | SHCNRF_NewDelivery;
	}
	else {
		// Windows9x:shared memoryは使用しない
		nSources = SHCNRF_InterruptLevel | SHCNRF_ShellLevel;
	}

	// エントリを初期化
	m_fsne[0].pidl = NULL;
	m_fsne[0].fRecursive = FALSE;

	// シェル通知メッセージを登録
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
//	VM初期化
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
//	コンポーネント初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::InitComponent()
{
	BOOL bSuccess;
	CComponent *pComponent;

	ASSERT(!m_pFirstComponent);
	ASSERT(!m_pSch);
	ASSERT(!m_pSound);
	ASSERT(!m_pInput);
//	ASSERT(!m_pPort);
//	ASSERT(!m_pMIDI);
//	ASSERT(!m_pTKey);
	ASSERT(!m_pHost);
//	ASSERT(!m_pInfo);
	ASSERT(!m_pConfig);

	// コンストラクト(順番を考慮する必要あり。最初Config、最後Scheduler)
	m_pConfig = new CConfig(this);
	m_pFirstComponent = m_pConfig;
	m_pSound = new CSound(this);
	m_pFirstComponent->AddComponent(m_pSound);
	m_pInput = new CInput(this);
	m_pFirstComponent->AddComponent(m_pInput);
//	m_pPort = new CPort(this);
//	m_pFirstComponent->AddComponent(m_pPort);
//	m_pMIDI = new CMIDI(this);
//	m_pFirstComponent->AddComponent(m_pMIDI);
//	m_pTKey = new CTKey(this);
//	m_pFirstComponent->AddComponent(m_pTKey);
	m_pHost = new CHost(this);
	m_pFirstComponent->AddComponent(m_pHost);
	m_pSch = new CScheduler(this);
	m_pFirstComponent->AddComponent(m_pSch);

	// 初期化
	pComponent = m_pFirstComponent;
	bSuccess = TRUE;

	// ループ
	while (pComponent) {
		if (!pComponent->Init()) {
			bSuccess = FALSE;
		}
		pComponent = pComponent->GetNextComponent();
	}

	return bSuccess;
}

//---------------------------------------------------------------------------
//
//	バージョン初期化
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

	// パスを取得
	::GetModuleFileName(NULL, szPath, _MAX_PATH);

	// バージョン情報を読み取る
	dwLength = GetFileVersionInfoSize(szPath, &dwHandle);
	if (dwLength == 0) {
		return;
	}

	pVerInfo = new BYTE[ dwLength ];
	if (::GetFileVersionInfo(szPath, dwHandle, dwLength, pVerInfo) == 0) {
		return;
	}

	// バージョン情報を取り出す
	if (::VerQueryValue(pVerInfo, _T("\\"), (LPVOID*)&pFileInfo, &uLength) == 0) {
		delete[] pVerInfo;
		return;
	}

	// バージョンを分離、VMへ通知
	dwMajor = (DWORD)HIWORD(pFileInfo->dwProductVersionMS);
	dwMinor = (DWORD)(LOWORD(pFileInfo->dwProductVersionMS) * 16
					+ HIWORD(pFileInfo->dwProductVersionLS));
	::GetVM()->SetVersion(dwMajor, dwMinor);

	// 終了
	delete[] pVerInfo;
}

//---------------------------------------------------------------------------
//
//	コマンドライン処理
//	※コマンドライン、WM_COPYDATAで共通
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

	// ポインタ、フラグ初期化
	lpszCurrent = lpszCmd;
	bReset = FALSE;

	// ループ
	for (i=0; i<2; i++) {
		// スペース、タブはスキップ
		while (lpszCurrent[0] <= _T(0x20)) {
			if (lpszCurrent[0] == _T('\0')) {
				break;
			}
			lpszCurrent++;
		}
		if (lpszCurrent[0] == _T('\0')) {
			break;
		}

		// 最初がダブルクォートなら、次のクォートを探す
		if (lpszCurrent[0] == _T('\x22')) {
			lpszNext = _tcschr(lpszCurrent + 1, _T('\x22'));
			if (!lpszNext) {
				// 対応するダブルクォートが見つからない
				return;
			}
			nLen = (int)(lpszNext - (lpszCurrent + 1));
			if (nLen >= _MAX_PATH) {
				// 長すぎる
				return;
			}

			// クォートされた内部をコピー
			_tcsnccpy(szPath, &lpszCurrent[1], nLen);
			szPath[nLen] = _T('\0');

			// クォートの次を指す
			lpszCurrent = &lpszNext[1];
		}
		else {
			// 次のスペースを探す
			lpszNext = _tcschr(lpszCurrent + 1, _T(' '));
			if (lpszNext) {
				// スペースまで
				nLen = (int)(lpszNext - lpszCurrent);
				if (nLen >= _MAX_PATH) {
					// 長すぎる
					return;
				}

				// スペースまでの部分をコピー
				_tcsnccpy(szPath, lpszCurrent, nLen);
				szPath[nLen] = _T('\0');

				// スペースの次を指す
				lpszCurrent = &lpszNext[1];
			}
			else {
				// 終端まで
				_tcscpy(szPath, lpszCurrent);
				lpszCurrent = NULL;
			}
		}

		// オープンを試みる
		bReset = InitCmdSub(i, szPath);

		// 終端なら終了
		if (!lpszCurrent) {
			break;
		}
	}

	// リセット要求があれば、リセット
	if (bReset) {
		OnReset();
	}
}

//---------------------------------------------------------------------------
//
//	コマンドライン処理 サブ
//	※コマンドライン、WM_COPYDATA、ドラッグ&ドロップで共通
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

	// pFDI初期化
	pFDI = NULL;

	// ファイルオープンチェック
	path.SetPath(lpszPath);
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}
	dwSize = fio.GetFileSize();
	fio.Close();

	// フルパス化
	::GetFullPathName(lpszPath, _MAX_PATH, szPath, &lpszFile);
	path.SetPath(szPath);

	// VMロック
	::LockVM();

	// 128MO or 230MO or 540MO or 640MO
	if ((dwSize == 0x797f400) || (dwSize == 0xd9eea00) ||
		(dwSize == 0x1fc8b800) || (dwSize == 0x25e28000)) {
		// MOの割り当てを試みる
		nDrive = 2;

		if (!m_pSASI->Open(path)) {
			// MO割り当て失敗
			GetScheduler()->Reset();
//			ResetCaption();
			::UnlockVM();
			return FALSE;
		}
	}
	else {
		if (dwSize >= 0x200000) {
			// VMの割り当てを試みる
			nDrive = 4;

			// オープン前処理
			if (!OnOpenPrep(path, FALSE)) {
				// ファイルがないか、バージョンなどが正しくない
				GetScheduler()->Reset();
//				ResetCaption();
				::UnlockVM();
				return FALSE;
			}

			// ロード実行(OnOpenSubに任せる)
			::UnlockVM();
			if (OnOpenSub(path)) {
				Filepath::SetDefaultDir(szPath);
			}
			// リセットは行わない
			return FALSE;
		}
		else {
			// FDの割り当てを試みる
			if (!m_pFDD->Open(nDrive, path)) {
				// FD割り当て失敗
				GetScheduler()->Reset();
//				ResetCaption();
				::UnlockVM();
				return FALSE;
			}
			pFDI = m_pFDD->GetFDI(nDrive);
		}
	}

	// VMリセット、ロック解除
	GetScheduler()->Reset();
//	ResetCaption();
	::UnlockVM();

	// 成功。ディレクトリ保存＆MRU追加
	Filepath::SetDefaultDir(szPath);
	GetConfig()->SetMRUFile(nDrive, szPath);

	// フロッピーなら、BADイメージ警告
	if (pFDI) {
		if (pFDI->GetID() == MAKEID('B', 'A', 'D', ' ')) {
			::GetMsg(IDS_BADFDI_WARNING, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		}

		// フロッピーを割り当てたときだけ、リセットする
		return TRUE;
	}

	// 終了
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	コンポーネントをセーブ
//	※スケジューラは停止しているが、CSound,CInputは動作中
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::SaveComponent(const Filepath& path, DWORD dwPos)
{
	Fileio fio;
	DWORD dwID;
	CComponent *pComponent;
	DWORD dwMajor;
	DWORD dwMinor;
	int nVer;

	ASSERT(this);
	ASSERT(dwPos > 0);

	// バージョン情報作成
	::GetVM()->GetVersion(dwMajor, dwMinor);
	nVer = (int)((dwMajor << 8) | dwMinor);

	// ファイルオープンとシーク
	if (!fio.Open(path, Fileio::Append)) {
		return FALSE;
	}
	if (!fio.Seek(dwPos)) {
		fio.Close();
		return FALSE;
	}

	// メインコンポーネント情報を保存
	dwID = MAKEID('M', 'A', 'I', 'N');
	if (!fio.Write(&dwID, sizeof(dwID))) {
		fio.Close();
		return FALSE;
	}

	// コンポーネントループ
	pComponent = m_pFirstComponent;
	while (pComponent) {
		// IDを保存
		dwID = pComponent->GetID();
		if (!fio.Write(&dwID, sizeof(dwID))) {
			fio.Close();
			return FALSE;
		}

		// コンポーネント固有
		if (!pComponent->Save(&fio, nVer)) {
			fio.Close();
			return FALSE;
		}

		// 次へ
		pComponent = pComponent->GetNextComponent();
	}

	// 終端書き込み
	dwID = MAKEID('E', 'N', 'D', ' ');
	if (!fio.Write(&dwID, sizeof(dwID))) {
		fio.Close();
		return FALSE;
	}

	// 終了
	fio.Close();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	コンポーネントをロード
//	※スケジューラは停止しているが、CSound,CInputは動作中
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::LoadComponent(const Filepath& path, DWORD dwPos)
{
	Fileio fio;
	DWORD dwID;
	CComponent *pComponent;
	char cHeader[0x10];
	int nVer;

	ASSERT(this);
	ASSERT(dwPos > 0);

	// ファイルオープン
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}

	// ヘッダ読み取り
	if (!fio.Read(cHeader, sizeof(cHeader))) {
		fio.Close();
		return FALSE;
	}

	// ヘッダチェック、バージョン情報読み取り
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

	// シーク
	if (!fio.Seek(dwPos)) {
		fio.Close();
		return FALSE;
	}

	// メインコンポーネント読み取り
	if (!fio.Read(&dwID, sizeof(dwID))) {
		fio.Close();
		return FALSE;
	}
	if (dwID != MAKEID('M', 'A', 'I', 'N')) {
		fio.Close();
		return FALSE;
	}

	// コンポーネントループ
	for (;;) {
		// ID読み取り
		if (!fio.Read(&dwID, sizeof(dwID))) {
			fio.Close();
			return FALSE;
		}

		// 終了チェック
		if (dwID == MAKEID('E', 'N', 'D', ' ')) {
			break;
		}

		// コンポーネントを探す
		pComponent = m_pFirstComponent->SearchComponent(dwID);
		if (!pComponent) {
			// セーブ時はコンポーネントが存在したが、今は見つからない
			fio.Close();
			return FALSE;
		}

		// コンポーネント固有
		if (!pComponent->Load(&fio, nVer)) {
			fio.Close();
			return FALSE;
		}
	}

	// クローズ
	fio.Close();

	// 設定適用(VMロックして行う)
	if (GetConfig()->IsApply()) {
		::LockVM();
		ApplyCfg();
		::UnlockVM();
	}

	// ウィンドウ再描画
	GetView()->Invalidate(FALSE);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::ApplyCfg()
{
	Config config;
	CComponent *pComponent;

	// 設定取得
	GetConfig()->GetConfig(&config);

	// まずVMに適用
	::GetVM()->ApplyCfg(&config);

	// 次にコンポーネントに適用
	pComponent = m_pFirstComponent;
	while (pComponent) {
		pComponent->ApplyCfg(&config);
		pComponent = pComponent->GetNextComponent();
	}

	// 次にビューに適用
	GetView()->ApplyCfg(&config);

	// フレームウィンドウ(ポップアップ)
	if (config.popup_swnd != m_bPopup) {
//		// サブウィンドウをすべてクリア
//		GetView()->ClrSWnd();

		// 変更
		m_bPopup = config.popup_swnd;
	}

	// フレームウィンドウ(マウス)
	m_bMouseMid = config.mouse_mid;
	m_bAutoMouse = config.auto_mouse;
	if (config.mouse_port == 0) {
		// マウス接続なしなら、マウスモードOFF
//		if (GetInput()->GetMouseMode()) {
//			OnMouseMode();
//		}
	}
}

//---------------------------------------------------------------------------
//
//	キック
//
//---------------------------------------------------------------------------
LONG CFrmWnd::OnKick(UINT , LONG )
{
	CComponent *pComponent;
//	CInfo *pInfo;
	Config config;
	CString strMsg;
	MSG msg;
	Memory *pMemory;
	int nIdle;
	LPSTR lpszCmd;
	LPCTSTR lpszCommand;
	BOOL bFullScreen;

	// エラー処理を先に行う
	switch (m_nStatus) {
		// VMエラー
		case 1:
			::GetMsg(IDS_INIT_VMERR, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
			PostMessage(WM_CLOSE, 0, 0);
			return 0;

		// コンポーネントエラー
		case 2:
			::GetMsg(IDS_INIT_COMERR, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
			PostMessage(WM_CLOSE, 0, 0);
			return 0;
	}
	// 正常の場合
	ASSERT(m_nStatus == 0);

	// ROMチェック
	pMemory = (Memory*)::GetVM()->SearchDevice(MAKEID('M', 'E', 'M', ' '));
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

	// 設定取得(power_off設定のため)
	GetConfig()->GetConfig(&config);
	if (config.power_off) {
		// 電源OFFで起動
		::GetVM()->SetPower(FALSE);
		::GetVM()->PowerSW(FALSE);
	}

	// サブウィンドウの準備
	m_strWndClsName = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS);

	// コンポーネントをイネーブル。ただしSchedulerは設定による
	GetView()->Enable(TRUE);
	pComponent = m_pFirstComponent;
	while (pComponent) {
		// スケジューラか
		if (pComponent->GetID() == MAKEID('S', 'C', 'H', 'E')) {
			if (config.power_off) {
				// 電源OFFで起動
				pComponent->Enable(FALSE);
				pComponent = pComponent->GetNextComponent();
				continue;
			}
		}

		// イネーブル
		pComponent->Enable(TRUE);
		pComponent = pComponent->GetNextComponent();
	}

	// リセット(ステータスバーのため)
	if (!config.power_off) {
		OnReset();
	}

	// コマンドライン処理
	lpszCmd = AfxGetApp()->m_lpCmdLine;
	lpszCommand = A2T(lpszCmd);
	if (_tcslen(lpszCommand) > 0) {
		InitCmd(lpszCommand);
	}

	// 最大化指定であれば、戻した後に、フルスクリーン
	bFullScreen = FALSE;
	if (IsZoomed()) {
		ShowWindow(SW_RESTORE);
		bFullScreen = TRUE;
	}

	// ウインドウ位置をレジューム
	bFullScreen = RestoreFrameWnd(bFullScreen);
	if (bFullScreen) {
		// 最大化指定か、前回実行時にフルスクリーン
		PostMessage(WM_COMMAND, IDM_FULLSCREEN);
	}

	// ディスク・ステートをレジューム
	RestoreDiskState();

	// 無限ループ
	nIdle = 0;
	while (!m_bExit) {
		// メッセージチェック＆ポンプ
		if (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			if (!AfxGetApp()->PumpMessage()) {
				::PostQuitMessage(0);
				return 0;
			}
			// continueすることで、WM_DESTROY直後のm_bExitチェックを保証
			continue;
		}

		// スリープ
		if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			Sleep(20);

			// Infoを毎回取り直す
//			pInfo = GetInfo();
//			if (!pInfo) {
//				continue;
//			}

			// 更新カウンタUp
			nIdle++;

			// ステータス・実行は20ms
//			pInfo->UpdateStatus();
			UpdateExec();

			if ((nIdle & 1) == 0) {
				// ビューは40ms
				GetView()->Update();
			}

			if ((nIdle & 3) == 0) {
				// キャプション、情報は80ms
//				pInfo->UpdateCaption();
//				pInfo->UpdateInfo();
			}
		}
	}

	return 0;
}

//---------------------------------------------------------------------------
//
//	ウィンドウクラス名取得
//
//---------------------------------------------------------------------------
//	LPCTSTR FASTCALL CFrmWnd::GetWndClassName() const
//	{
//		ASSERT(this);
//		ASSERT(m_strWndClsName.GetLength() != 0);
//	
//		return (LPCTSTR)m_strWndClsName;
//	}

//---------------------------------------------------------------------------
//
//	ポップアップか
//
//---------------------------------------------------------------------------
//	BOOL FASTCALL CFrmWnd::IsPopupSWnd() const
//	{
//		ASSERT(this);
//		return m_bPopup;
//	}

//---------------------------------------------------------------------------
//
//	ウィンドウクローズ
//
//---------------------------------------------------------------------------
void CFrmWnd::OnClose()
{
	CString strFormat;
	CString strText;
	Filepath path;
	int nResult;

	ASSERT(this);
	ASSERT(!m_bSaved);

	// 有効なステートファイルがあれば、セーブを問う
	::LockVM();
	::GetVM()->GetPath(path);
	::UnlockVM();

	// 有効なステートファイルがあって
	if (!path.IsClear()) {
		// Windowsサイドで20ms以上の実行実績があれば
		if (m_dwExec >= 2) {
			// 確認
			::GetMsg(IDS_SAVECLOSE, strFormat);
			strText.Format(strFormat, path.GetFileExt());
			nResult = MessageBox(strText, NULL, MB_ICONQUESTION | MB_YESNOCANCEL);

			// 確認結果による
			switch (nResult) {
				// YES
				case IDYES:
					// 保存
					OnSaveSub(path);
					break;

				// NO
				case IDNO:
					// パスをクリア(ステートなし)
					::GetVM()->Clear();
					break;

				// キャンセル
				case IDCANCEL:
					// クローズされなかったことにする
					return;
			}
		}
	}

	// 初期化済みなら
	if ((m_nStatus == 0) && !m_bSaved) {
		// ウィンドウ状態・ディスク・ステートを保存
		SaveFrameWnd();
		SaveDiskState();
		m_bSaved = TRUE;
	}

//	// フルスクリーン時は解除
//	if (m_bFullScreen) {
//		ASSERT(m_nStatus == 0);
//		OnFullScreen();
//	}

	// 初期化済みなら
	if (m_nStatus == 0) {
		// マウス解除
//		if (GetInput()->GetMouseMode()) {
//			OnMouseMode();
//		}
	}

	// 基本クラス
	CFrameWnd::OnClose();
}

//---------------------------------------------------------------------------
//
//	ウィンドウ削除
//
//---------------------------------------------------------------------------
void CFrmWnd::OnDestroy()
{
	ASSERT(this);

	// 初期化済みなら
	if ((m_nStatus == 0) && !m_bSaved) {
		// ウィンドウ状態・ディスク・ステートを保存
		SaveFrameWnd();
		SaveDiskState();
		m_bSaved = TRUE;
	}

//	// フルスクリーン時は解除
//	if (m_bFullScreen) {
//		ASSERT(m_nStatus == 0);
//		OnFullScreen();
//	}

	// クリーンアップ(WM_ENDSESSIONと共通)
	CleanSub();

	// 基本クラスへ
	CFrameWnd::OnDestroy();
}

//---------------------------------------------------------------------------
//
//	セッション終了
//
//---------------------------------------------------------------------------
void CFrmWnd::OnEndSession(BOOL bEnding)
{
	ASSERT(this);

	// 終了時は、クリーンアップを行う
	if (bEnding) {
		// 初期化済みなら
		if (m_nStatus == 0) {
			// ウィンドウ状態・ディスク・ステートを保存
			if (!m_bSaved) {
				SaveFrameWnd();
				SaveDiskState();
				m_bSaved = TRUE;
			}

			// クリーンアップ
			CleanSub();
		}
	}

	// 基本クラス
	CFrameWnd::OnEndSession(bEnding);
}

//---------------------------------------------------------------------------
//
//	クリーンアップ共通
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::CleanSub()
{
	CComponent *pComponent;
	CComponent *pNext;
	int i;

	// 終了フラグを上げる
	m_bExit = TRUE;

	// コンポーネントを止める
	GetView()->Enable(FALSE);
	pComponent = m_pFirstComponent;
	while (pComponent) {
		pComponent->Enable(FALSE);
		pComponent = pComponent->GetNextComponent();
	}

	// マウス解除
//	if (m_nStatus == 0) {
//		if (GetInput()->GetMouseMode()) {
//			OnMouseMode();
//		}
//	}

	// スケジューラが実行をやめるまで待つ
	for (i=0; i<8; i++) {
		::LockVM();
		::UnlockVM();
	}

	// スケジューラを停止(CScheduler)
	if (m_nStatus == 0) {
		GetScheduler()->Stop();
	}

	// コンポーネントを削除
	pComponent = m_pFirstComponent;
	while (pComponent) {
		pComponent->Cleanup();
		pComponent = pComponent->GetNextComponent();
	}
	pComponent = m_pFirstComponent;
	while (pComponent) {
		pNext = pComponent->GetNextComponent();
		delete pComponent;
		pComponent = pNext;
	}

	// 仮想マシンを削除
	if (::pVM) {
		::LockVM();
		::GetVM()->Cleanup();
		delete ::pVM;
		::pVM = NULL;
		::UnlockVM();
	}

	// シェル通知を削除
	if (m_uNotifyId) {
		 VERIFY(::SHChangeNotifyDeregister(m_uNotifyId));
		 m_uNotifyId = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	ウィンドウ状態を保存
//
//---------------------------------------------------------------------------
void CFrmWnd::SaveFrameWnd()
{
	CRect rectWnd;
	Config config;

	ASSERT(this);
	ASSERT_VALID(this);

	// 設定取得
	GetConfig()->GetConfig(&config);

	// キャプション・メニュー・ステータスバー
//	config.caption = m_bCaption;
	config.menu_bar = m_bMenuBar;
//	config.status_bar = m_bStatusBar;

	// ウィンドウ矩形
	if (m_bFullScreen) {
		// フルスクリーン時は、ウィンドウ時の位置を保存する
		config.window_left = m_nWndLeft;
		config.window_top = m_nWndTop;
	}
	else {
		// ウィンドウ時は、現在の位置を保存する
		GetWindowRect(&rectWnd);
		config.window_left = rectWnd.left;
		config.window_top = rectWnd.top;
	}

	// フルスクリーン
	config.window_full = m_bFullScreen;

	// 設定変更
	GetConfig()->SetConfig(&config);
}

//---------------------------------------------------------------------------
//
//	ディスク・ステートを保存
//
//---------------------------------------------------------------------------
void CFrmWnd::SaveDiskState()
{
	int nDrive;
	Filepath path;
	Config config;

	ASSERT(this);
	ASSERT_VALID(this);

	// ロック
	::LockVM();

	// 設定取得
	GetConfig()->GetConfig(&config);

	// フロッピーディスク
	for (nDrive=0; nDrive<2; nDrive++) {
		// レディ
		config.resume_fdi[nDrive] = m_pFDD->IsReady(nDrive, FALSE);

		// レディでなければ、次へ
		if (!config.resume_fdi[nDrive]) {
			continue;
		}

		// メディア
		config.resume_fdm[nDrive]  = m_pFDD->GetMedia(nDrive);

		// ライトプロテクト
		config.resume_fdw[nDrive] = m_pFDD->IsWriteP(nDrive);
	}

	// MOディスク
	config.resume_mos = m_pSASI->IsReady();
	if (config.resume_mos) {
		config.resume_mow = m_pSASI->IsWriteP();
	}

	// CD-ROM
	config.resume_iso = m_pSCSI->IsReady(FALSE);

	// ステート
	::GetVM()->GetPath(path);
	config.resume_xm6 = !path.IsClear();

	// デフォルトディレクトリ
	_tcscpy(config.resume_path, Filepath::GetDefaultDir());

	// 設定変更
	GetConfig()->SetConfig(&config);

	// アンロック
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	ウィンドウ状態を復元
//	※OnCreateとOnKickと、2回呼ばれる
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

	// 設定取得
	GetConfig()->GetConfig(&config);

	// ウィンドウ位置の復元が指定されていなければ、デフォルト状態で動作
	if (!config.resume_screen) {
		return bFullScreen;
	}

	// キャプション
//	m_bCaption = config.caption;
//	ShowCaption();

	// メニュー
	m_bMenuBar = config.menu_bar;
	ShowMenu();

	// ステータスバー
//	m_bStatusBar = config.status_bar;
//	ShowStatus();

	// 仮想画面のサイズと原点を取得
	nWidth = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
	nHeight = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
	nLeft = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
	nTop = ::GetSystemMetrics(SM_YVIRTUALSCREEN);

	// ウィンドウ矩形を取得
	GetWindowRect(&rectWnd);

	// 手の届くところにあれば、ウィンドウ位置を移動する。まずはチェック
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

	// ウィンドウ位置を移動
	if (bValid) {
		SetWindowPos(&wndTop, config.window_left, config.window_top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		// ワークエリアも同時に変更する
		m_nWndLeft = config.window_left;
		m_nWndTop = config.window_top;
	}

	// VM未初期化なら、ここまで
	if (m_nStatus != 0) {
		return FALSE;
	}

	// フルスクリーンか
	if (bFullScreen || config.window_full) {
		// 最大化起動か、前回フルスクリーンだった
		return TRUE;
	}
	else {
		// 最大化起動でなく、かつ、前回通常表示だった
		return FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	ディスク・ステートを復元
//
//---------------------------------------------------------------------------
void CFrmWnd::RestoreDiskState()
{
	int nDrive;
	TCHAR szMRU[_MAX_PATH];
	BOOL bResult;
	Filepath path;
	Config config;

	ASSERT(this);

	// 設定取得
	GetConfig()->GetConfig(&config);

	// ステートが指定されていれば、これを先に行う
	if (config.resume_state) {
		// ステートがあった
		if (config.resume_xm6) {
			// パス取得
			GetConfig()->GetMRUFile(4, 0, szMRU);
			path.SetPath(szMRU);

			// オープン前処理
			if (OnOpenPrep(path)) {
				// オープンサブ
				if (OnOpenSub(path)) {
					// 成功なので、デフォルトディレクトリだけ処理
					if (config.resume_dir) {
						Filepath::SetDefaultDir(config.resume_path);
					}

					// これ以降は処理しない(FD, MO, CDのアクセス中にセーブした場合)
					return;
				}
			}
		}
	}

	// フロッピーディスク
	if (config.resume_fd) {
		for (nDrive=0; nDrive<2; nDrive++) {
			// ディスク挿入されていたか
			if (!config.resume_fdi[nDrive]) {
				// ディスク挿入されていない。スキップ
				continue;
			}

			// ディスク挿入
			GetConfig()->GetMRUFile(nDrive, 0, szMRU);
			ASSERT(szMRU[0] != _T('\0'));
			path.SetPath(szMRU);

			// VMロックを行い、ディスク割り当てを試みる
			::LockVM();
			bResult = m_pFDD->Open(nDrive, path, config.resume_fdm[nDrive]);
			::UnlockVM();

			// 割り当てできなければスキップ
			if (!bResult) {
				continue;
			}

			// 書き込み禁止
			if (config.resume_fdw[nDrive]) {
				::LockVM();
				m_pFDD->WriteP(nDrive, TRUE);
				::UnlockVM();
			}
		}
	}

	// MOディスク
	if (config.resume_mo) {
		// ディスク挿入されていたか
		if (config.resume_mos) {
			// ディスク挿入
			GetConfig()->GetMRUFile(2, 0, szMRU);
			ASSERT(szMRU[0] != _T('\0'));
			path.SetPath(szMRU);

			// VMロックを行い、ディスク割り当てを試みる
			::LockVM();
			bResult = m_pSASI->Open(path);
			::UnlockVM();

			// 割り当てできれば
			if (bResult) {
				// 書き込み禁止
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
		// ディスク挿入されていたか
		if (config.resume_iso) {
			// ディスク挿入
			GetConfig()->GetMRUFile(3, 0, szMRU);
			ASSERT(szMRU[0] != _T('\0'));
			path.SetPath(szMRU);

			// VMロックを行い、ディスク割り当てを試みる
			::LockVM();
			m_pSCSI->Open(path, FALSE);
			::UnlockVM();
		}
	}

	// デフォルトディレクトリ
	if (config.resume_dir) {
		Filepath::SetDefaultDir(config.resume_path);
	}
}

//---------------------------------------------------------------------------
//
//	ディスプレイ変更
//
//---------------------------------------------------------------------------
LRESULT CFrmWnd::OnDisplayChange(UINT uParam, LONG lParam)
{
	LRESULT lResult;

	// 基本クラス
	lResult = CFrameWnd::OnDisplayChange(uParam, lParam);

	// 最小化は何もしない
	if (IsIconic()) {
		return lResult;
	}

	// ポジション設定
	InitPos(FALSE);

	return lResult;
}

//---------------------------------------------------------------------------
//
//	ウィンドウ背景描画
//
//---------------------------------------------------------------------------
BOOL CFrmWnd::OnEraseBkgnd(CDC * )
{
	// 背景描画を抑制
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ウィンドウ描画
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPaint()
{
	PAINTSTRUCT ps;

	// 必ずロックして行う
	::LockVM();

	BeginPaint(&ps);

	// VMが有効ならキャプション、ステータスバーをリセット
	if (m_nStatus == 0) {
//		ResetCaption();
//		ResetStatus();
	}

	EndPaint(&ps);

	// アンロック
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	ウィンドウ移動
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMove(int x, int y)
{
	CRect rect;

	// 初期化済みなら
	if (m_nStatus == 0) {
		// マウスモードチェック
		if (GetInput()->GetMouseMode()) {
			// クリップ範囲を変更
			ClipCursor(NULL);
			GetWindowRect(&rect);
			SetCursorPos((rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2);
			ClipCursor(&rect);
		}
	}

	// 基本クラス
	CFrameWnd::OnMove(x, y);
}

//---------------------------------------------------------------------------
//
//	アクティベート
//
//---------------------------------------------------------------------------
void CFrmWnd::OnActivate(UINT nState, CWnd *pWnd, BOOL bMinimized)
{
	CInput *pInput;
	CScheduler *pScheduler;

	// 初期化済みなら
	if (m_nStatus == 0) {
		// インプット、スケジューラへ通知
		pInput = GetInput();
		pScheduler = GetScheduler();
		if (pInput && pScheduler) {
			// WA_INACTIVEか最小化なら、ディセーブル
			if ((nState == WA_INACTIVE) || bMinimized) {
				// 入力受け付けない、低速実行
				pInput->Activate(FALSE);
				pScheduler->Activate(FALSE);

//				// マウスモードOFF(POPUPウィンドウ対策)
//				if (pInput->GetMouseMode()) {
//					OnMouseMode();
//				}
			}
			else {
				// 入力受け付ける、通常実行
				pInput->Activate(TRUE);
				pScheduler->Activate(TRUE);
			}
		}
	}

	// 基本クラスへ
	CFrameWnd::OnActivate(nState, pWnd, bMinimized);
}

//---------------------------------------------------------------------------
//
//	アクティベートアプリケーション
//
//---------------------------------------------------------------------------
#if _MFC_VER >= 0x700
void CFrmWnd::OnActivateApp(BOOL bActive, DWORD dwThreadID)
#else
void CFrmWnd::OnActivateApp(BOOL bActive, HTASK hTask)
#endif
{
	// 初期化済みなら
	if (m_nStatus == 0) {
		// フルスクリーン専用
		if (m_bFullScreen) {
			if (bActive) {
				// これからアクティブになる
				HideTaskBar(TRUE, TRUE);
//				RecalcStatusView();
			}
			else {
				// アクティブから外れた
				HideTaskBar(FALSE, FALSE);
			}
		}
	}

	// 基本クラス
#if _MFC_VER >= 0x700
	CFrameWnd::OnActivateApp(bActive, dwThreadID);
#else
	CFrameWnd::OnActivateApp(bActive, hTask);
#endif
}

//---------------------------------------------------------------------------
//
//	メニューループ開始
//
//---------------------------------------------------------------------------
void CFrmWnd::OnEnterMenuLoop(BOOL bTrackPopup)
{
	CInput *pInput;
	CScheduler *pScheduler;

	// キャプションリセット
//	ResetCaption();

	::LockVM();

	// インプットへ通知
	pInput = GetInput();
	if (pInput) {
		pInput->Menu(TRUE);
	}

//	// マウスモードFALSE(マウスでメニューが操作できるように)
//	if (pInput->GetMouseMode()) {
//		OnMouseMode();
//	}

	// スケジューラへ通知
	pScheduler = GetScheduler();
	if (pScheduler) {
		pScheduler->Menu(TRUE);
	}

	::UnlockVM();

	// 基本クラスへ
	CFrameWnd::OnEnterMenuLoop(bTrackPopup);
}

//---------------------------------------------------------------------------
//
//	メニューループ終了
//
//---------------------------------------------------------------------------
void CFrmWnd::OnExitMenuLoop(BOOL bTrackPopup)
{
	CInput *pInput;
	CScheduler *pScheduler;

	::LockVM();

	// インプットへ通知
	pInput = GetInput();
	if (pInput) {
		pInput->Menu(FALSE);
	}

	// スケジューラへ通知
	pScheduler = GetScheduler();
	if (pScheduler) {
		pScheduler->Menu(FALSE);
	}

	::UnlockVM();

	// キャプションリセット
//	ResetCaption();

	// 基本クラスへ
	CFrameWnd::OnExitMenuLoop(bTrackPopup);
}

//---------------------------------------------------------------------------
//
//	親ウィンドウ通知
//
//---------------------------------------------------------------------------
void CFrmWnd::OnParentNotify(UINT message, LPARAM lParam)
{
	CInput *pInput;

	// CInputを取得、通知
	if ((message == WM_MBUTTONDOWN) && (m_nStatus == 0)) {
		// インプットを取得
		pInput = GetInput();
		if (pInput) {
			// マウス無効なら有効にする。逆はしない
			if (!pInput->GetMouseMode()) {
//				// 設定で"中ボタン禁止"にされていないことが条件
//				if (m_bMouseMid) {
//					OnMouseMode();
//				}
			}
		}
	}

	// 基本クラスへ
	CFrameWnd::OnParentNotify(message, lParam);
}

//---------------------------------------------------------------------------
//
//	コンテキストメニュー
//
//---------------------------------------------------------------------------
void CFrmWnd::OnContextMenu(CWnd * , CPoint pos)
{
	CMenu *pMenu;

	// キーボードからの入力のとき
	if ((pos.x == -1) && (pos.y == -1)) {
		// スケジューラチェック、入力チェック
		if (GetScheduler()->IsEnable()) {
			if (GetInput()->IsActive() && !GetInput()->IsMenu()) {
				// DIK_APPSがマップされているか
//				if (GetInput()->IsKeyMapped(DIK_APPS)) {
//					// SHIFT+F10が押されているか
//					SHORT sF10;
//					SHORT sShift;
//					sF10 = ::GetAsyncKeyState(VK_F10);
//					sShift = ::GetAsyncKeyState(VK_SHIFT);
//					if (((sF10 & 0x8000) == 0) || ((sShift & 0x8000) == 0)) {
//						// VK_APPSが押されたためと判定
//						return;
//					}
//				}
			}
		}

//		// マウスモードであれば、解除(キーボードからのメニュー起動)
//		if (GetInput()->GetMouseMode()) {
//			OnMouseMode();
//		}
	}
	else {
		// マウスモードであれば、無視(マウスからのメニュー起動)
		if (GetInput()->GetMouseMode()) {
			return;
		}
	}

	// ポップアップメニュー
	m_bPopupMenu = TRUE;
	pMenu = m_PopupMenu.GetSubMenu(0);
	pMenu->TrackPopupMenu(TPM_CENTERALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
							pos.x, pos.y, this, 0);
	m_bPopupMenu = FALSE;
}

//---------------------------------------------------------------------------
//
//	電力変更通知
//
//---------------------------------------------------------------------------
LONG CFrmWnd::OnPowerBroadCast(UINT , LONG )
{
	// 初期化済みなら
	if (m_nStatus == 0) {
		// VMロック、時間再設定
		::LockVM();
		timeEndPeriod(1);
		timeBeginPeriod(1);
		::UnlockVM();
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	システムコマンド
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSysCommand(UINT nID, LPARAM lParam)
{
	// 標準ウィンドウ位置をサポート
	if ((nID & 0xfff0) == IDM_STDWIN) {
		InitPos(TRUE);
		return;
	}

	// 最大化はフルスクリーン
	if ((nID & 0xfff0) == SC_MAXIMIZE) {
		if (!m_bFullScreen) {
			PostMessage(WM_COMMAND, IDM_FULLSCREEN);
		}
		return;
	}

	// 基本クラス
	CFrameWnd::OnSysCommand(nID, lParam);
}

//---------------------------------------------------------------------------
//
//	データ転送
//
//---------------------------------------------------------------------------
#if _MFC_VER >= 0x700
afx_msg BOOL CFrmWnd::OnCopyData(CWnd* , COPYDATASTRUCT* pCopyDataStruct)
#else
LONG CFrmWnd::OnCopyData(UINT , LONG pCopyDataStruct)
#endif
{
	PCOPYDATASTRUCT pCDS;

	// パラメータ受け取り
	pCDS = (PCOPYDATASTRUCT)pCopyDataStruct;

	// コマンドライン処理へ
	InitCmd((LPSTR)pCDS->lpData);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	シェル通知
//
//---------------------------------------------------------------------------
LRESULT CFrmWnd::OnShellNotify(UINT uParam, LONG lParam)
{
	HANDLE hMemoryMap;
	DWORD dwProcessId;
	LPITEMIDLIST *pidls;
	HANDLE hLock;
	LONG nEvent;
	TCHAR szPath[_MAX_PATH];
	CHost *pHost;

	// Windows NTか
	if (::IsWinNT()) {
		// Windows2000/XPの場合、SHChangeNotification_Lockでロックする
		hMemoryMap = (HANDLE)uParam;
		dwProcessId = (DWORD)lParam;
		hLock = ::SHChangeNotification_Lock(hMemoryMap, dwProcessId, &pidls, &nEvent);
		if (hLock == NULL) {
			return 0;
		}
	}
	else {
		// Windows9xの場合、pidlsとnEventはuParam,lParamから直接得る
		pidls = (LPITEMIDLIST*)uParam;
		nEvent = lParam;
		hLock = NULL;
	}

	// 実行中で、CHostがあれば、通知
	if (m_nStatus == 0) {
		pHost = GetHost();

#if 1
		// Windrvがまだ不安定のため、実際にEnableにされていない場合は何もしない(version2.04)
		{
			Config config;
			GetConfig()->GetConfig(&config);
			if ((config.windrv_enable <= 0) || (config.windrv_enable > 3)) {
				pHost = NULL;
			}
		}
#endif

		if (pHost) {
			// パス取得
			::SHGetPathFromIDList(pidls[0], szPath);

			// 通知
			pHost->ShellNotify(nEvent, szPath);
		}
	}

	// NTの場合、SHCnangeNotifcation_Unlockでアンロックする
	if (::IsWinNT()) {
		ASSERT(hLock);
		::SHChangeNotification_Unlock(hLock);
	}

	return 0;
}

//---------------------------------------------------------------------------
//
//	更新(実行)
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::UpdateExec()
{
	ASSERT(this);
	ASSERT_VALID(this);

	// スケジューラが有効なら、実行カウンタを上げる(セーブ時にクリアされる)
	if (GetScheduler()->IsEnable()) {
		m_dwExec++;
		if (m_dwExec == 0) {
			m_dwExec--;
		}
	}
}

//---------------------------------------------------------------------------
//
//	メッセージ文字列提供
//
//---------------------------------------------------------------------------
void CFrmWnd::GetMessageString(UINT nID, CString& rMessage) const
{
	Filepath path;
	TCHAR szPath[_MAX_PATH];
	TCHAR szName[60];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	TCHAR szFile[_MAX_FNAME];
	TCHAR szExt[_MAX_EXT];
	int nMRU;
	int nDisk;
	BOOL bValid;
//	CInfo *pInfo;

	// フラグFALSE
	bValid = FALSE;

	// メニュー文字列を先に行う(英語環境+MRUを考慮)
	if ((nID >= IDM_OPEN) && (nID <= IDM_ABOUT)) {
		// 英語環境か
		if (!::IsJapanese()) {
			// +5000で試す
			if (rMessage.LoadString(nID + 5000)) {
				bValid = TRUE;
			}
		}
	}

	// メニュー文字列特例(IDM_STDWIN)
	if (nID == IDM_STDWIN) {
		// 英語環境か
		if (!::IsJapanese()) {
			// +5000で試す
			if (rMessage.LoadString(nID + 5000)) {
				bValid = TRUE;
			}
		}
	}

	// MRU0
	if ((nID >= IDM_D0_MRU0) && (nID <= IDM_D0_MRU8)) {
		nMRU = nID - IDM_D0_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
		GetConfig()->GetMRUFile(0, nMRU, szPath);
		szPath[60] = _T('\0');
		rMessage = szPath;
		bValid = TRUE;
	}

	// MRU1
	if ((nID >= IDM_D1_MRU0) && (nID <= IDM_D1_MRU8)) {
		nMRU = nID - IDM_D1_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
		GetConfig()->GetMRUFile(1, nMRU, szPath);
		szPath[60] = _T('\0');
		rMessage = szPath;
		bValid = TRUE;
	}

	// MRU2
	if ((nID >= IDM_MO_MRU0) && (nID <= IDM_MO_MRU8)) {
		nMRU = nID - IDM_MO_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
		GetConfig()->GetMRUFile(2, nMRU, szPath);
		szPath[60] = _T('\0');
		rMessage = szPath;
		bValid = TRUE;
	}

	// MRU3
	if ((nID >= IDM_CD_MRU0) && (nID <= IDM_CD_MRU8)) {
		nMRU = nID - IDM_CD_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
		GetConfig()->GetMRUFile(3, nMRU, szPath);
		szPath[60] = _T('\0');
		rMessage = szPath;
		bValid = TRUE;
	}

	// MRU4
	if ((nID >= IDM_XM6_MRU0) && (nID <= IDM_XM6_MRU8)) {
		nMRU = nID - IDM_XM6_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
		GetConfig()->GetMRUFile(4, nMRU, szPath);
		szPath[60] = _T('\0');
		rMessage = szPath;
		bValid = TRUE;
	}

	// ディスク名0
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

	// ディスク名1
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

	// ここまでで確定していなければ、基本クラス
	if (!bValid) {
		CFrameWnd::GetMessageString(nID, rMessage);
	}

	// 結果をInfoへ提供(内部保持用)
//	pInfo = GetInfo();
//	if (pInfo) {
//		pInfo->SetMessageString(rMessage);
//	}

//	// 結果をステータスビューへ提供
//	if (m_pStatusView) {
//		m_pStatusView->SetMenuString(rMessage);
//	}
}

//---------------------------------------------------------------------------
//
//	タスクバー隠す
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::HideTaskBar(BOOL bHide, BOOL bFore)
{
	if (bHide) {
		// "常に前面"
		m_hTaskBar = ::FindWindow(_T("Shell_TrayWnd"), NULL);
		if (m_hTaskBar) {
			::ShowWindow(m_hTaskBar, SW_HIDE);
		}
		ModifyStyleEx(0, WS_EX_TOPMOST, 0);
	}
	else {
		// "通常"
		ModifyStyleEx(WS_EX_TOPMOST, 0, 0);
		if (m_hTaskBar) {
			::ShowWindow(m_hTaskBar, SW_SHOWNA);
		}
	}

	// 前面オプションがあれば
	if (bFore) {
		SetForegroundWindow();
	}
}

//---------------------------------------------------------------------------
//
//	ステータスバー表示
//
//---------------------------------------------------------------------------
//	void FASTCALL CFrmWnd::ShowStatus()
//	{
//		ASSERT(this);
//	
//		// 必要ならVMをロック
//		if (m_nStatus == 0) {
//			::LockVM();
//		}
//	
//		// フルスクリーンか
//		if (m_bFullScreen) {
//			// ステータスバーは常に非表示
//	//		ShowControlBar(&m_StatusBar, FALSE, FALSE);
//	
//			// ステータスバー表示か
//	//		if (m_bStatusBar)
//	//		{
//	//			// ステータスビューが存在しなければ
//	//			if (!m_pStatusView) {
//	//				// 作成して
//	//				CreateStatusView();
//	//
//	//				// 再配置
//	//				if (m_bStatusBar) {
//	//					RecalcStatusView();
//	//				}
//	//			}
//	//		}
//	//		else {
//	//			// ステータスビューが存在していれば
//	//			if (m_pStatusView) {
//	//				// 削除して
//	//				DestroyStatusView();
//	//
//	//				// 再配置
//	//				RecalcStatusView();
//	//			}
//	//		}
//	
//			// 必要があればアンロック
//			if (m_nStatus == 0) {
//				::UnlockVM();
//			}
//			return;
//		}
//	
//		// ステータスビューはフルスクリーン専用なので、削除
//		if (m_pStatusView) {
//			DestroyStatusView();
//			RecalcLayout();
//		}
//	
//		// ウィンドウなので、ShowControlBarで制御
//	//	ShowControlBar(&m_StatusBar, m_bStatusBar, FALSE);
//	
//		// 必要があればアンロック
//		if (m_nStatus == 0) {
//			::UnlockVM();
//		}
//	}
//	
//	//---------------------------------------------------------------------------
//	//
//	//	ステータスビュー作成(フルスクリーン時)
//	//
//	//---------------------------------------------------------------------------
//	void FASTCALL CFrmWnd::CreateStatusView()
//	{
//	//	CInfo *pInfo;
//	
//		ASSERT(!m_pStatusView);
//	
//		if (m_bStatusBar) {
//			m_bStatusBar = FALSE;	//VC2010//
//	//		// ステータスビュー作成(再配置は行わない)
//	//		m_pStatusView = new CStatusView;
//	//		if (m_pStatusView->Init(this)) {
//	//			// 作成成功
//	//			pInfo = GetInfo();
//	//			if (pInfo) {
//	//				// Infoが存在するので、ステータスビュー作成を通知
//	//				pInfo->SetStatusView(m_pStatusView);
//	//			}
//	//		}
//	//		else {
//	//			// 作成失敗
//	//			m_bStatusBar = FALSE;
//	//		}
//		}
//	}
//	
//	//---------------------------------------------------------------------------
//	//
//	//	ステータスビュー終了(フルスクリーン時)
//	//
//	//---------------------------------------------------------------------------
//	void FASTCALL CFrmWnd::DestroyStatusView()
//	{
//		CInfo *pInfo;
//	
//		// 有効なステータスビューが存在する場合のみ
//		if (m_pStatusView) {
//			// Info取得
//			pInfo = GetInfo();
//			if (pInfo) {
//				// Infoが存在するので、ステータスビュー削除を通知
//	//			pInfo->SetStatusView(NULL);
//			}
//	
//			// ステータスビュー削除(再配置は行わない)
//	//		m_pStatusView->DestroyWindow();
//	//		m_pStatusView = NULL;
//		}
//	}
//	
//	//---------------------------------------------------------------------------
//	//
//	//	ステータスビュー再配置
//	//
//	//---------------------------------------------------------------------------
//	void FASTCALL CFrmWnd::RecalcStatusView()
//	{
//		CRect rectFrame;
//		CRect rectDraw;
//		CRect rectStatus;
//	//	LONG lDraw;
//	//	LONG lStatus;
//		BOOL bMove;
//	
//		// フレームのサイズを取得
//		GetClientRect(&rectFrame);
//	
//		// ステータスビューの有無で分ける
//	//	if (m_pStatusView) {
//	//		// ステータスビューあり。ステータスビューの位置を優先
//	//		m_pStatusView->GetWindowRect(&rectStatus);
//	//		lStatus = rectStatus.Height();
//	//		lDraw = rectFrame.Height() - lStatus;
//	//
//	//		// Drawビューの位置を取得
//	//		m_pDrawView->GetWindowRect(&rectDraw);
//	//		ScreenToClient(&rectDraw);
//	//
//	//		// 変更チェック
//	//		bMove = FALSE;
//	//		if ((rectDraw.left != 0) || (rectDraw.top != 0)) {
//	//			bMove = TRUE;
//	//		}
//	//		if ((rectDraw.Width () != rectFrame.Width()) || (rectDraw.Height() != lDraw)) {
//	//			bMove = TRUE;
//	//		}
//	//		if (bMove) {
//	//			m_pDrawView->SetWindowPos(&wndTop, 0, 0, rectFrame.Width(), lDraw, SWP_NOZORDER);
//	//		}
//	//
//	//		// Statusビューの位置を取得
//	//		m_pStatusView->GetWindowRect(&rectStatus);
//	//		ScreenToClient(&rectStatus);
//	//
//	//		// 変更チェック
//	//		bMove = FALSE;
//	//		if ((rectStatus.left != 0) || (rectStatus.top != lDraw)) {
//	//			bMove = TRUE;
//	//		}
//	//		if ((rectStatus.Width() != rectFrame.Width()) || (rectStatus.Height() != lStatus)) {
//	//			bMove = TRUE;
//	//		}
//	//		if (bMove) {
//	//			m_pStatusView->SetWindowPos(&wndTop, 0, lDraw, rectFrame.Width(), lStatus, SWP_NOZORDER);
//	//		}
//	//	}
//	//	else
//		{
//			// ステータスビューなし。Drawビューのみ
//			m_pDrawView->GetWindowRect(&rectDraw);
//			ScreenToClient(&rectDraw);
//	
//			// 変更チェック
//			bMove = FALSE;
//			if ((rectDraw.left != 0) || (rectDraw.top != 0)) {
//				bMove = TRUE;
//			}
//			if ((rectDraw.Width () != rectFrame.Width()) || (rectDraw.Height() != rectFrame.Height())) {
//				bMove = TRUE;
//			}
//			if (bMove) {
//				m_pDrawView->SetWindowPos(&wndTop, 0, 0, rectFrame.Width(), rectFrame.Height(), SWP_NOZORDER);
//			}
//		}
//	}
//	
//	//	//---------------------------------------------------------------------------
//	//	//
//	//	//	ステータスバーリセット
//	//	//
//	//	//---------------------------------------------------------------------------
//	//	void FASTCALL CFrmWnd::ResetStatus()
//	//	{
//	//		CInfo *pInfo;
//	//	
//	//		// Infoがあればリセット
//	//		pInfo = GetInfo();
//	//		if (pInfo) {
//	//	//		pInfo->ResetStatus();
//	//		}
//	//	}

//---------------------------------------------------------------------------
//
//	オーナードロー
//
//---------------------------------------------------------------------------
void CFrmWnd::OnDrawItem(int nID, LPDRAWITEMSTRUCT lpDIS)
{
	int nPane;
	HDC hDC;
	CRect rectDraw;
//	CInfo *pInfo;

	// ウィンドウハンドルのチェック
//	if (lpDIS->hwndItem != m_StatusBar.m_hWnd) {
//		CFrameWnd::OnDrawItem(nID, lpDIS);
//		return;
//	}

	// 種別、DC、矩形を取得
	nPane = lpDIS->itemID;
	if (nPane == 0) {
		return;
	}
	nPane--;
	hDC = lpDIS->hDC;
	rectDraw = &lpDIS->rcItem;

	// Infoのチェック
//	pInfo = GetInfo();
//	if (!pInfo) {
//		// 黒で塗りつぶす
//		::SetBkColor(hDC, RGB(0, 0, 0));
//		::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rectDraw, NULL, 0, NULL);
//		return;
//	}

	// Infoに指示
//	pInfo->DrawStatus(nPane, hDC, rectDraw);
}

//---------------------------------------------------------------------------
//
//	メニューバー表示
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::ShowMenu()
{
	HMENU hMenu;

	ASSERT(this);

	// 必要であればVMをロック
	if (m_nStatus == 0) {
		::LockVM();
	}

	// 現在のメニューを取得
	hMenu = ::GetMenu(m_hWnd);

	// メニューが不必要な場合
	if (m_bFullScreen || !m_bMenuBar) {
		// メニューが存在するか
		if (hMenu != NULL) {
			// メニューを消去
			SetMenu(NULL);
		}
		if (m_nStatus == 0) {
			::UnlockVM();
		}
		return;
	}

	// メニューが必要な場合
	if (hMenu != NULL) {
		// セットしたいメニューと同じか
		if (m_Menu.GetSafeHmenu() == hMenu) {
			// 変更の必要はない
			if (m_nStatus == 0) {
				::UnlockVM();
			}
			return;
		}
	}

	// メニューをセット
	SetMenu(&m_Menu);

	// 必要ならVMをアンロック
	if (m_nStatus == 0) {
		::UnlockVM();
	}
}

//---------------------------------------------------------------------------
//
//	キャプション表示
//
//---------------------------------------------------------------------------
//	void FASTCALL CFrmWnd::ShowCaption()
//	{
//		DWORD dwStyle;
//	
//		ASSERT(this);
//	
//		// 必要であればVMをロック
//		if (m_nStatus == 0) {
//			::LockVM();
//		}
//	
//		// 現在のキャプション状態を取得
//		dwStyle = GetStyle() & WS_CAPTION;
//	
//		// キャプションが不必要な場合
//		if (m_bFullScreen) {
//			// キャプションが存在するか
//			if (dwStyle) {
//				// キャプションを消去
//				ModifyStyle(WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
//								0, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
//			}
//			if (m_nStatus == 0) {
//				::UnlockVM();
//			}
//			return;
//		}
//	
//		// キャプションが必要な場合
//		if (!dwStyle) {
//			// キャプションをセット
//			ModifyStyle(0, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
//								SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
//		}
//	
//		// 必要ならVMをアンロック
//		if (m_nStatus == 0) {
//			::UnlockVM();
//		}
//	}

//---------------------------------------------------------------------------
//
//	キャプションリセット
//
//---------------------------------------------------------------------------
//void FASTCALL CFrmWnd::ResetCaption()
//{
//	CInfo *pInfo;
//
//	// Infoがあればリセット
//	pInfo = GetInfo();
//	if (pInfo) {
//		pInfo->ResetCaption();
//	}
//}

//---------------------------------------------------------------------------
//
//	情報設定
//
//---------------------------------------------------------------------------
//void FASTCALL CFrmWnd::SetInfo(CString& strInfo)
//{
//	CInfo *pInfo;
//
//	// Infoがあれば設定
//	pInfo = GetInfo();
//	if (pInfo) {
//		pInfo->SetInfo(strInfo);
//	}
//}

//---------------------------------------------------------------------------
//
//	描画ビュー取得
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
//	最初のコンポーネントを取得
//
//---------------------------------------------------------------------------
CComponent* FASTCALL CFrmWnd::GetFirstComponent() const
{
	ASSERT(this);
	return m_pFirstComponent;
}

//---------------------------------------------------------------------------
//
//	スケジューラ取得
//
//---------------------------------------------------------------------------
CScheduler* FASTCALL CFrmWnd::GetScheduler() const
{
	ASSERT(this);
	ASSERT(m_pSch);
	return m_pSch;
}

//---------------------------------------------------------------------------
//
//	インプット取得
//
//---------------------------------------------------------------------------
CInput* FASTCALL CFrmWnd::GetInput() const
{
	ASSERT(this);
	ASSERT(m_pInput);
	return m_pInput;
}

//---------------------------------------------------------------------------
//
//	Host取得
//
//---------------------------------------------------------------------------
CHost* FASTCALL CFrmWnd::GetHost() const
{
	ASSERT(this);
	ASSERT(m_pHost);
	return m_pHost;
}

//---------------------------------------------------------------------------
//
//	コンフィグ取得
//
//---------------------------------------------------------------------------
CConfig* FASTCALL CFrmWnd::GetConfig() const
{
	ASSERT(this);
	ASSERT(m_pConfig);
	return m_pConfig;
}

#endif	// _WIN32
