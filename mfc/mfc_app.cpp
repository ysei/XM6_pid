//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC アプリケーション ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "filepath.h"
#include "mfc_frm.h"
#include "mfc_asm.h"
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
//	グローバル ワーク
//
//---------------------------------------------------------------------------
static VM *pVM = 0;								// Virtual Machine

//---------------------------------------------------------------------------
//
//	スタティック ワーク
//
//---------------------------------------------------------------------------
static CCriticalSection csect;			// VMロック用クリティカルセクション
static LPSTR lpszInfoMsg;				// 情報メッセージバッファ
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

VM* FASTCALL CreateVM(void) {
	if(pVM == 0) {
		pVM = new VM;
	}
	return pVM;
}

void FASTCALL DestroyVM(void) {
	if(pVM) {
		pVM->Cleanup();
		delete pVM;
		pVM = 0;
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
}

//---------------------------------------------------------------------------
//
//	インスタンス初期化
//
//---------------------------------------------------------------------------
BOOL CApp::InitInstance()
{
	// デフォルトディレクトリをクリア
	Filepath::ClearDefaultDir();

	// メインウインドウ作成(すぐにm_pMainWndへ代入)
	CFrmWnd *pFrmWnd = new CFrmWnd();
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
