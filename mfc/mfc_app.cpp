//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC アプリケーション ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "filepath.h"
#include "mfc_frm.h"
#include "mfc_asm.h"
#include "mfc_com.h"
#include "mfc_res.h"
#include "mfc_app.h"

#include "vm.h"
#include "cpu.h"
#include "memory.h"


//---------------------------------------------------------------------------
//
//	アプリケーション インスタンス
//
//---------------------------------------------------------------------------
CApp theApp;

//---------------------------------------------------------------------------
//
//	関数ポインタ定義
//
//---------------------------------------------------------------------------
extern "C" {
typedef int (WINAPI *DRAWTEXTWIDE)(HDC, LPCWSTR, int, LPRECT, UINT);
}

//---------------------------------------------------------------------------
//
//	グローバル ワーク
//
//---------------------------------------------------------------------------
VM *pVM;								// Virtual Machine

//---------------------------------------------------------------------------
//
//	スタティック ワーク
//
//---------------------------------------------------------------------------
static CCriticalSection csect;			// VMロック用クリティカルセクション
static BOOL bJapanese;					// 日本語・英語判別フラグ
static BOOL bWinNT;						// WindowsNT・Windows9x判別フラグ
static BOOL bSupport932;				// CP932(SHIFT-JIS)サポートフラグ
static BOOL bMMX;						// MMX判別フラグ
static BOOL bCMOV;						// CMOV判別フラグ
static LPSTR lpszInfoMsg;				// 情報メッセージバッファ
static DRAWTEXTWIDE pDrawTextW;			// DrawTextW

//---------------------------------------------------------------------------
//
//	日本語環境の判定
//
//---------------------------------------------------------------------------
BOOL FASTCALL IsJapanese(void)
{
	return bJapanese;
}

//---------------------------------------------------------------------------
//
//	WindowsNTの判定
//
//---------------------------------------------------------------------------
BOOL FASTCALL IsWinNT(void)
{
	return bWinNT;
}

//---------------------------------------------------------------------------
//
//	CP932サポートの判定
//
//---------------------------------------------------------------------------
BOOL FASTCALL Support932(void)
{
	return bSupport932;
}

//---------------------------------------------------------------------------
//
//	MMXの判定
//
//---------------------------------------------------------------------------
BOOL FASTCALL IsMMX(void)
{
	return bMMX;
}

//---------------------------------------------------------------------------
//
//	CMOVの判定
//
//---------------------------------------------------------------------------
BOOL FASTCALL IsCMOV(void)
{
	return bCMOV;
}

//---------------------------------------------------------------------------
//
//	メッセージ取得
//
//---------------------------------------------------------------------------
void FASTCALL GetMsg(UINT uID, CString& string)
{
	// uID=0で飛んでくる場合がある
	if (uID == 0) {
		string.Empty();
		return;
	}

	// 日本語か
	if (IsJapanese()) {
		if (!string.LoadString(uID)) {
#if defined(_DEBUG)
			TRACE(_T("GetMsg:文字列ロードに失敗 ID:%d\n"), uID);
#endif	// _DEBUG
			string.Empty();
		}
		return;
	}

	// 英語。+5000で試す
	if (string.LoadString(uID + 5000)) {
		return;
	}

	// +0でもう一度
	if (!string.LoadString(uID)) {
#if defined(_DEBUG)
		TRACE(_T("GetMsg:文字列ロードに失敗 ID:%d\n"), uID);
#endif	// _DEBUG
		string.Empty();
	}
}

//---------------------------------------------------------------------------
//
//	仮想マシンを取得
//
//---------------------------------------------------------------------------
VM* FASTCALL GetVM(void)
{
	ASSERT(pVM);
	return pVM;
}

//---------------------------------------------------------------------------
//
//	仮想マシンをロック
//
//---------------------------------------------------------------------------
void FASTCALL LockVM(void)
{
	csect.Lock();
}

//---------------------------------------------------------------------------
//
//	仮想マシンをアンロック
//
//---------------------------------------------------------------------------
void FASTCALL UnlockVM(void)
{
	csect.Unlock();
}

//---------------------------------------------------------------------------
//
//	ファイルオープンダイアログ
//	※lpszPathは必ず初期化して呼び出すこと
//
//---------------------------------------------------------------------------
BOOL FASTCALL FileOpenDlg(CWnd *pParent, LPSTR lpszPath, UINT nFilterID)
{
	OPENFILENAME ofn;
	TCHAR szFilter[0x200];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	CString strFilter;
	int i;
	int nLen;
	WIN32_FIND_DATA wfd;
	HANDLE hFind;

	ASSERT(pParent);
	ASSERT(lpszPath);
	ASSERT(nFilterID);

	// 構造体を設定
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = pParent->m_hWnd;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile = lpszPath;
	ofn.nMaxFile = _MAX_PATH;
	ofn.lpstrInitialDir = Filepath::GetDefaultDir();
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

	// フィルタを設定
	GetMsg(nFilterID, strFilter);
	_tcscpy(szFilter, (LPCTSTR)strFilter);
	nLen = (int)_tcslen(szFilter);
	for (i=0; i<nLen; i++) {
		if (szFilter[i] == _T('|')) {
			szFilter[i] = _T('\0');
		}
	}

	// コモンダイアログ実行
	if (!GetOpenFileName(&ofn)) {
		return FALSE;
	}

	// 正式なファイル名を得る(FindFirstFileで得られるのはファイル名+拡張子のみ)
	hFind = FindFirstFile(lpszPath, &wfd);
	FindClose(hFind);
	_tsplitpath(lpszPath, szDrive, szDir, NULL, NULL);
	_tcscpy(lpszPath, szDrive);
	_tcscat(lpszPath, szDir);
	_tcscat(lpszPath, wfd.cFileName);

	// デフォルトディレクトリを保存
	Filepath::SetDefaultDir(lpszPath);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ファイルセーブダイアログ
//	※lpszPathは必ず初期化して呼び出すこと。lpszExtは先頭3文字のみ有効
//
//---------------------------------------------------------------------------
BOOL FASTCALL FileSaveDlg(CWnd *pParent, LPSTR lpszPath, LPCTSTR lpszExt, UINT nFilterID)
{
	OPENFILENAME ofn;
	TCHAR szFilter[0x200];
	CString strFilter;
	int i;
	int nLen;

	ASSERT(pParent);
	ASSERT(lpszPath);
	ASSERT(nFilterID);

	// 構造体を設定
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = pParent->m_hWnd;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile = lpszPath;
	ofn.nMaxFile = _MAX_PATH;
	ofn.lpstrInitialDir = Filepath::GetDefaultDir();
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = lpszExt;

	// フィルタを設定
	GetMsg(nFilterID, strFilter);
	_tcscpy(szFilter, (LPCTSTR)strFilter);
	nLen = (int)_tcslen(szFilter);
	for (i=0; i<nLen; i++) {
		if (szFilter[i] == _T('|')) {
			szFilter[i] = _T('\0');
		}
	}

	// コモンダイアログ実行
	if (!GetSaveFileName(&ofn)) {
		return FALSE;
	}

	// デフォルトディレクトリを保存
	Filepath::SetDefaultDir(lpszPath);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	情報メッセージ設定
//
//---------------------------------------------------------------------------
void FASTCALL SetInfoMsg(LPCTSTR lpszMsg, BOOL bRec)
{
	// 記憶フラグを見る
	if (bRec) {
		// バッファアドレスを記憶
		lpszInfoMsg = (LPSTR)lpszMsg;
		return;
	}

	// バッファアドレスが与えられていなければ、無視
	if (!lpszInfoMsg) {
		return;
	}
}

//---------------------------------------------------------------------------
//
//	DrawTextW
//	※非サポートOSでは、何もしない
//
//---------------------------------------------------------------------------
int FASTCALL DrawTextWide(HDC hDC, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat)
{
	// サポートされているか
	if (!pDrawTextW) {
		// 何もしない
		return 1;
	}

	// ワイド文字でDraw
	return pDrawTextW(hDC, lpString, nCount, lpRect, uFormat);
}

//===========================================================================
//
//	アプリケーション
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CApp::CApp() : CWinApp(_T("XM6"))
{
	m_hMutex = NULL;
	m_hUser32 = NULL;
}

//---------------------------------------------------------------------------
//
//	インスタンス初期化
//
//---------------------------------------------------------------------------
BOOL CApp::InitInstance()
{
	CFrmWnd *pFrmWnd;

	// デフォルトディレクトリをクリア
	Filepath::ClearDefaultDir();

	// 環境判定
	if (!CheckEnvironment()) {
		return FALSE;
	}

	// 二重起動チェック
	if (!CheckMutex()) {
		// コマンドラインがあれば、渡す
		if (m_lpCmdLine[0] != _T('\0')) {
			SendCmd();
		}
		return FALSE;
	}

	// メインウインドウ作成(すぐにm_pMainWndへ代入)
	pFrmWnd = new CFrmWnd();
	m_pMainWnd = (CWnd*)pFrmWnd;

	// 初期化
	if (!pFrmWnd->Init()) {
		return FALSE;
	}

	// 表示
	pFrmWnd->ShowWindow(m_nCmdShow);
	pFrmWnd->UpdateWindow();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	インスタンス終了
//
//---------------------------------------------------------------------------
BOOL CApp::ExitInstance()
{
	// Mutex削除
	if (m_hMutex) {
		::CloseHandle(m_hMutex);
		m_hMutex = NULL;
	}

	// USER32.DLL解放
	if (m_hUser32) {
		::FreeLibrary(m_hUser32);
		m_hUser32 = NULL;
	}

	// 基本クラス
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Mutexチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL CApp::CheckMutex()
{
	HANDLE hMutex;

	ASSERT(this);

	// 有無にかかわらず、作成
	hMutex = ::CreateMutex(NULL, TRUE, _T("XM6"));
	if (hMutex) {
		// 既に起動？
		if (::GetLastError() == ERROR_ALREADY_EXISTS) {
			return FALSE;
		}

		// OK
		m_hMutex = hMutex;
		return TRUE;
	}

	// なぜか失敗
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	環境の判定
//
//---------------------------------------------------------------------------
BOOL FASTCALL CApp::CheckEnvironment()
{
	OSVERSIONINFO ovi;
	CString strError;

	ASSERT(this);

	//
	//	OSの判定
	//

	// 日本語環境の判定
	::bJapanese = FALSE;
	if (PRIMARYLANGID(GetSystemDefaultLangID()) == LANG_JAPANESE) {
		if (PRIMARYLANGID(GetUserDefaultLangID()) == LANG_JAPANESE) {
			// システムデフォルト・ユーザデフォルトの双方で判定
			::bJapanese = TRUE;
		}
	}

	// WindowsNTの判定
	memset(&ovi, 0, sizeof(ovi));
	ovi.dwOSVersionInfoSize = sizeof(ovi);
	VERIFY(::GetVersionEx(&ovi));
	if (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
		::bWinNT = TRUE;
	}
	else {
		::bWinNT = FALSE;
	}

	// コードページ932サポートの判定(UNICODEサポートが前提)
	::bSupport932 = FALSE;
	::pDrawTextW = NULL;
	if (::bWinNT) {
		// UNICODEサポートOS
		if (::IsValidCodePage(932)) {
			// USER32.DLLをロード
			m_hUser32 = ::LoadLibrary(_T("USER32.DLL"));
			if (m_hUser32) {
				// DrawTextWのアドレスを得る
				pDrawTextW = (DRAWTEXTWIDE)::GetProcAddress(m_hUser32, _T("DrawTextW"));
				if (pDrawTextW) {
					// CP932への変換と表示が可能
					::bSupport932 = TRUE;
				}
			}
		}
	}

	//
	//	プロセッサの判定
	//

	// CMOVの判定
	::bCMOV = FALSE;
	if (::IsCMOVSupport()) {
		::bCMOV = TRUE;
	}

	// MMXの判定(Windows98以降のみ)
	::bMMX = FALSE;
	if (ovi.dwMajorVersion >= 4) {
		// Windows95 or WindowsNT4 以降
		if ((ovi.dwMajorVersion == 4) && (ovi.dwMinorVersion == 0)) {
			// Windows95 or WindowsNT4
			::bMMX = FALSE;
		}
		else {
			// プロセッサによる
			::bMMX = ::IsMMXSupport();
		}
	}

	// version2.05から、CMOV,MMXとも必須
	if (!::bCMOV || !::bMMX) {
		::GetMsg(IDS_PROCESSOR ,strError);
		AfxMessageBox(strError, MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	// すべてOK
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	コマンドライン送信
//
//---------------------------------------------------------------------------
void FASTCALL CApp::SendCmd()
{
	HWND hWnd;
	COPYDATASTRUCT cds;

	ASSERT(this);

	// ウィンドウ検索
	hWnd = SearchXM6Wnd();
	if (!hWnd) {
		return;
	}

	// WM_COPYDATAで送信
	memset(&cds, 0, sizeof(cds));
	cds.dwData = WM_COPYDATA;
	cds.cbData = ((DWORD)_tcslen(m_lpCmdLine) + 1) * sizeof(TCHAR);
	cds.lpData = m_lpCmdLine;
	::SendMessage(hWnd, WM_COPYDATA, NULL, (LPARAM)&cds);
}

//---------------------------------------------------------------------------
//
//	XM6ウィンドウ検索
//
//---------------------------------------------------------------------------
HWND FASTCALL CApp::SearchXM6Wnd()
{
	HWND hWnd;

	// ウィンドウはNULL
	hWnd = NULL;

	// 検索
	::EnumWindows(EnumXM6Proc, (LPARAM)&hWnd);

	// コールバック関数で結果を入れる
	return hWnd;
}

//---------------------------------------------------------------------------
//
//	XM6ウィンドウ検索コールバック
//
//---------------------------------------------------------------------------
BOOL CALLBACK CApp::EnumXM6Proc(HWND hWnd, LPARAM lParam)
{
	HWND *phWnd;
	LONG lUser;

	// パラメータ受け取り
	phWnd = (HWND*)lParam;
	ASSERT(phWnd);
	ASSERT(*phWnd == NULL);

	// 該当ウィンドウのユーザデータを得る
	lUser = ::GetWindowLong(hWnd, GWL_USERDATA);

	// XM6チェックを行う
	if (lUser == (LONG)MAKEID('X', 'M', '6', ' ')) {
		// XM6フレームウィンドウと判定し、打ち切る
		*phWnd = hWnd;
		return FALSE;
	}

	// 違っているので続ける
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	cpudebug.c ワード読み出し (mfc_cpu.cpp)
//
//---------------------------------------------------------------------------
extern "C" WORD cpudebug_fetch(DWORD addr)
{
	static Memory* cpudebug_memory = 0;

	WORD w;

	if(cpudebug_memory == 0) {
		cpudebug_memory = (Memory*)::GetVM()->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	}

	ASSERT(cpudebug_memory);

	addr &= 0xfffffe;
	w = (WORD) cpudebug_memory->ReadOnly(addr);
	w <<= 8;
	w |= cpudebug_memory->ReadOnly(addr + 1);

	return w;
}
#endif	// _WIN32
