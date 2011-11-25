//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �R���t�B�O ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_cfg_h)
#define mfc_cfg_h

#include "config.h"
#include "ppi.h"

//===========================================================================
//
//	�R���t�B�O
//
//===========================================================================
class CConfig : public CComponent
{
public:
	// ��{�t�@���N�V����
	CConfig(CFrmWnd *pWnd);													// �R���X�g���N�^
	BOOL FASTCALL Init();													// ������
	void FASTCALL Cleanup();												// �N���[���A�b�v

	// �ݒ�f�[�^(�S��)
	void FASTCALL GetConfig(Config *pConfigBuf) const;						// �ݒ�f�[�^�擾
	void FASTCALL SetConfig(Config *pConfigBuf);							// �ݒ�f�[�^�ݒ�

	// �ݒ�f�[�^(��)
	void FASTCALL SetStretch(BOOL bStretch);								// ��ʊg��ݒ�
	void FASTCALL SetMIDIDevice(int nDevice, BOOL bIn);						// MIDI�f�o�C�X�ݒ�

	// MRU
	void FASTCALL SetMRUFile(int nType, LPCTSTR pszFile);					// MRU�t�@�C���ݒ�(�ł��V����)
	void FASTCALL GetMRUFile(int nType, int nIndex, LPTSTR pszFile) const;	// MRU�t�@�C���擾
	int FASTCALL GetMRUNum(int nType) const;								// MRU�t�@�C�����擾

	// �Z�[�u�E���[�h
	BOOL FASTCALL Save(Fileio *pFio, int nVer);								// �Z�[�u
	BOOL FASTCALL Load(Fileio *pFio, int nVer);								// ���[�h
	BOOL FASTCALL IsApply();												// �K�p���邩

private:
	// �ݒ�f�[�^
	typedef struct _INIKEY {
		void *pBuf;						// �|�C���^
		LPCTSTR pszSection;				// �Z�N�V������
		LPCTSTR pszKey;					// �L�[��
		int nType;						// �^
		int nDef;						// �f�t�H���g�l
		int nMin;						// �ŏ��l(�ꕔ�^�C�v�̂�)
		int nMax;						// �ő�l(�ꕔ�̂�)
	} INIKEY, *PINIKEY;

	// INI�t�@�C��
	TCHAR m_IniFile[FILEPATH_MAX];								// INI�t�@�C����

	// �ݒ�f�[�^
	void FASTCALL LoadConfig();									// �ݒ�f�[�^���[�h
	void FASTCALL SaveConfig() const;							// �ݒ�f�[�^�Z�[�u
	static const INIKEY IniTable[];								// �ݒ�f�[�^INI�e�[�u��
	static Config m_Config;										// �ݒ�f�[�^

	// �o�[�W�����݊�
	void FASTCALL ResetSASI();									// SASI�Đݒ�
	void FASTCALL ResetCDROM();									// CD-ROM�Đݒ�
	static BOOL m_bCDROM;										// CD-ROM�L��

	// MRU
	enum {
		MruTypes = 5											// MRU�^�C�v��
	};
	void FASTCALL ClearMRU(int nType);							// MRU�N���A
	void FASTCALL LoadMRU(int nType);							// MRU���[�h
	void FASTCALL SaveMRU(int nType) const;						// MRU�Z�[�u
	TCHAR m_MRUFile[MruTypes][9][FILEPATH_MAX];					// MRU�t�@�C��
	int m_MRUNum[MruTypes];										// MRU��

	// �L�[
	void FASTCALL LoadKey() const;								// �L�[���[�h
	void FASTCALL SaveKey() const;								// �L�[�Z�[�u

	// TrueKey
	void FASTCALL LoadTKey() const;								// TrueKey���[�h
	void FASTCALL SaveTKey() const;								// TrueKey�Z�[�u

	// �o�[�W�����݊�
	BOOL FASTCALL Load200(Fileio *pFio);						// version2.00 or version2.01
	BOOL FASTCALL Load202(Fileio *pFio);						// version2.02 or version2.03

	// ���[�h�E�Z�[�u
	BOOL m_bApply;												// ���[�h��ApplyCfg��v��
};

//---------------------------------------------------------------------------
//
//	�N���X��s��`
//
//---------------------------------------------------------------------------
class CConfigSheet;

//===========================================================================
//
//	�R���t�B�O�v���p�e�B�y�[�W
//
//===========================================================================
class CConfigPage : public CPropertyPage
{
public:
	CConfigPage();													// �R���X�g���N�^
	void FASTCALL Init(CConfigSheet *pSheet);						// ������
	virtual BOOL OnInitDialog();									// �_�C�A���O������
	virtual BOOL OnSetActive();										// �y�[�W�A�N�e�B�u
	DWORD FASTCALL GetID() const		{ return m_dwID; }			// ID�擾

protected:
	afx_msg BOOL OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT nMsg);	// �}�E�X�J�[�\���ݒ�
	Config *m_pConfig;												// �ݒ�f�[�^
	DWORD m_dwID;													// �y�[�WID
	int m_nTemplate;												// �e���v���[�gID
	UINT m_uHelpID;													// �w���vID
	UINT m_uMsgID;													// �w���v���b�Z�[�WID
	CConfigSheet *m_pSheet;											// �v���p�e�B�V�[�g
	DECLARE_MESSAGE_MAP()											// ���b�Z�[�W �}�b�v����
};
//===========================================================================
//
//	�R���t�B�O�v���p�e�B�V�[�g
//
//===========================================================================
class CConfigSheet : public CPropertySheet
{
public:
	CConfigSheet(CWnd *pParent);							// �R���X�g���N�^
	Config *m_pConfig;										// �ݒ�f�[�^
	CConfigPage* FASTCALL SearchPage(DWORD dwID) const;		// �y�[�W����

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);	// �E�B���h�E�쐬
	afx_msg void OnDestroy();								// �E�B���h�E�폜
	afx_msg void OnTimer(UINT_PTR nTimerID);				// �^�C�}

private:
	CFrmWnd *m_pFrmWnd;										// �t���[���E�B���h�E
	UINT_PTR m_nTimerID;
	DECLARE_MESSAGE_MAP()									// ���b�Z�[�W �}�b�v����
};

#endif	// mfc_cfg_h
#endif	// _WIN32
