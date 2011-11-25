//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC Drawビュー ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_draw_h)
#define mfc_draw_h

//===========================================================================
//
//	Drawビュー
//
//===========================================================================
class CDrawView : public CView
{
public:
	// 内部ワーク定義
	typedef struct _DRAWINFO {
		BOOL bPower;					// 電源
		Render *pRender;				// レンダラ
		Render::render_t *pWork;		// レンダラワーク
        DWORD dwDrawCount;				// 描画回数

		// DIBセクション
		HBITMAP hBitmap;				// DIBセクション
		DWORD *pBits;					// ビットデータ
		int nBMPWidth;					// BMP幅
		int nBMPHeight;					// BMP高さ

		// レンダラ連携
		int nRendWidth;					// レンダラ幅
		int nRendHeight;				// レンダラ高さ
		int nRendHMul;					// レンダラ横方向倍率
		int nRendVMul;					// レンダラ縦方向倍率
		int nLeft;						// 横マージン
		int nTop;						// 縦マージン
		int nWidth;						// BitBlt幅
		int nHeight;					// BitBlt高さ

		// Blt系
		int nBltTop;					// 描画開始Y
		int nBltBottom;					// 描画終了Y
		int nBltLeft;					// 描画開始X
		int nBltRight;					// 描画終了X
		BOOL bBltAll;					// 全表示フラグ
		BOOL bBltStretch;				// アスペクト比にあわせ拡大
	} DRAWINFO, *LPDRAWINFO;

public:
	// 基本ファンクション
	CDrawView();
										// コンストラクタ
	void FASTCALL Enable(BOOL bEnable);
										// 動作制御
	BOOL FASTCALL IsEnable() const;
										// 動作フラグ取得
	BOOL FASTCALL Init(CWnd *pParent);
										// 初期化
	BOOL PreCreateWindow(CREATESTRUCT& cs);
										// ウィンドウ作成準備
	void FASTCALL Refresh();
										// リフレッシュ描画
	void FASTCALL Draw(int index);
										// 描画(あるウィンドウのみ)
	void FASTCALL Update();
										// メッセージスレッドからの更新
	void FASTCALL ApplyCfg(const Config *pConfig);
										// 設定適用
	void FASTCALL GetDrawInfo(LPDRAWINFO pDrawInfo) const;
										// 描画情報取得

	// レンダリング描画
	void OnDraw(CDC *pDC);
										// 描画
	void FASTCALL Stretch(BOOL bStretch);
										// 拡大モード
	BOOL IsStretch() const				{ return m_Info.bBltStretch; }
										// 拡大モード取得

protected:
	// WMメッセージ
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// ウィンドウ作成
	afx_msg void OnDestroy();
										// ウィンドウ削除
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// ウィンドウサイズ変更
	afx_msg void OnPaint();
										// 描画
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
										// 背景描画
	afx_msg LRESULT OnDisplayChange(WPARAM wParam, LPARAM lParam);
										// ディスプレイ変更
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
										// キー押下
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
										// システムキー押下
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
										// キー離した
	afx_msg void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
										// システムキー離した
	afx_msg void OnMove(int x, int y);
										// ウィンドウ移動

	BOOL m_bEnable;
										// 有効フラグ
	CFont m_TextFont;
										// テキストフォント

private:
	void FASTCALL SetupBitmap();
										// ビットマップ準備
	inline void FASTCALL ReCalc(CRect& rect);
										// 再計算
	inline void FASTCALL DrawRect(CDC *pDC);
										// 周囲の余白を描画
	inline BOOL FASTCALL CalcRect();
										// 描画必要領域を調べる
	int FASTCALL MakeBits();
										// ビット作成
	CFrmWnd *m_pFrmWnd;
										// フレームウィンドウ
	CScheduler *m_pScheduler;
										// スケジューラ
	DRAWINFO m_Info;
										// 内部ワーク

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

#endif	// mfc_draw_h
#endif	// _WIN32
