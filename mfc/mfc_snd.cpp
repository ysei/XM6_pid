//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC サウンド ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "opmif.h"
#include "opm.h"
#include "adpcm.h"
#include "scsi.h"
#include "schedule.h"
#include "config.h"
#include "mfc_frm.h"
#include "mfc_com.h"
#include "mfc_asm.h"
#include "mfc_snd.h"

//===========================================================================
//
//	サウンド
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CSound::CSound(CFrmWnd *pWnd) : CComponent(pWnd)
{
	// コンポーネントパラメータ
	m_dwID = MAKEID('S', 'N', 'D', ' ');
	m_strDesc = _T("Sound Renderer");

	// ワーク初期化(設定パラメータ)
	m_uRate = 0;
	m_uTick = 90;
	m_uPoll = 7;
	m_uBufSize = 0;
	m_bPlay = FALSE;
	m_uCount = 0;
	m_dwWrite = 0;
	m_nMaster = 90;
	m_nFMVol = 54;
	m_nADPCMVol = 52;

	// ワーク初期化(DirectSoundとオブジェクト)
	m_lpDS = NULL;
	m_lpDSp = NULL;
	m_lpDSb = NULL;
	m_lpBuf = NULL;
	m_pScheduler = NULL;
	m_pOPM = NULL;
	m_pOPMIF = NULL;
	m_pADPCM = NULL;
	m_pSCSI = NULL;
	m_nDeviceNum = 0;
	m_nSelectDevice = 0;

//	// ワーク初期化(WAV録音)
//	m_pWav = NULL;
//	m_nWav = 0;
//	m_dwWav = 0;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CSound::Init()
{
	// 基本クラス
	if (!CComponent::Init()) {
		return FALSE;
	}

	// スケジューラ取得
	m_pScheduler = (Scheduler*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pScheduler);

	// OPMIF取得
	m_pOPMIF = (OPMIF*)::GetVM()->SearchDevice(MAKEID('O', 'P', 'M', ' '));
	ASSERT(m_pOPMIF);

	// ADPCM取得
	m_pADPCM = (ADPCM*)::GetVM()->SearchDevice(MAKEID('A', 'P', 'C', 'M'));
	ASSERT(m_pADPCM);

	// SCSI取得
	m_pSCSI = (SCSI*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(m_pSCSI);

	// デバイス列挙
	EnumDevice();

	// ここでは初期化しない(ApplyCfgに任せる)

	//VC2010//	面倒なのでここで初期化
	{
		m_nSelectDevice		= 0;
		m_uRate				= 44100;
		m_uTick				= 100;

//		InitSub();
		{
			bool b = true;

			// rate==0なら、何もしない
			if(b && m_uRate == 0) {
				b = false;
			}

			if(b) {
				ASSERT(!m_lpDS);
				ASSERT(!m_lpDSp);
				ASSERT(!m_lpDSb);
				ASSERT(!m_lpBuf);
				ASSERT(!m_pOPM);

				// デバイスがなければ0で試し、それでもなければreturn
				if (m_nDeviceNum <= m_nSelectDevice) {
					if (m_nDeviceNum == 0) {
						b = false;
					} else {
						m_nSelectDevice = 0;
					}
				}
			}

			// DiectSoundオブジェクト作成
			if(b && FAILED(DirectSoundCreate(m_lpGUID[m_nSelectDevice], &m_lpDS, NULL))) {
				// デバイスは使用中
				b = false;
			}

			// 協調レベルを設定(優先協調)
			if(b && FAILED(m_lpDS->SetCooperativeLevel(m_pFrmWnd->m_hWnd, DSSCL_PRIORITY))) {
				b = false;
			}

			// プライマリバッファを作成
			if(b) {
				DSBUFFERDESC dsbd = { 0 };
				dsbd.dwSize		= sizeof(dsbd);
				dsbd.dwFlags	= DSBCAPS_PRIMARYBUFFER;

				b = SUCCEEDED(m_lpDS->CreateSoundBuffer(&dsbd, &m_lpDSp, NULL));
			}

			// プライマリバッファのフォーマットを指定
			if(b) {
				WAVEFORMATEX wfex = { 0 };
				wfex.wFormatTag			= WAVE_FORMAT_PCM;
				wfex.nChannels			= 2;
				wfex.nSamplesPerSec		= m_uRate;
				wfex.nBlockAlign		= 4;
				wfex.nAvgBytesPerSec	= wfex.nSamplesPerSec * wfex.nBlockAlign;
				wfex.wBitsPerSample		= 16;

				b = SUCCEEDED(m_lpDSp->SetFormat(&wfex));
			}

			// セカンダリバッファを作成
			if(b) {
				PCMWAVEFORMAT pcmwf = { 0 };
				pcmwf.wf.wFormatTag			= WAVE_FORMAT_PCM;
				pcmwf.wf.nChannels			= 2;
				pcmwf.wf.nSamplesPerSec		= m_uRate;
				pcmwf.wf.nBlockAlign		= 4;
				pcmwf.wf.nAvgBytesPerSec	= pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign;
				pcmwf.wBitsPerSample		= 16;

				DSBUFFERDESC dsbd = { 0 };
				dsbd.dwSize					= sizeof(dsbd);
				dsbd.dwFlags				= DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME;
				dsbd.dwBufferBytes			= (pcmwf.wf.nAvgBytesPerSec * m_uTick) / 1000;
				dsbd.dwBufferBytes			= ((dsbd.dwBufferBytes + 7) >> 3) << 3;	// 8バイト境界
				dsbd.lpwfxFormat			= (LPWAVEFORMATEX)&pcmwf;

				m_uBufSize					= dsbd.dwBufferBytes;

				b = SUCCEEDED(m_lpDS->CreateSoundBuffer(&dsbd, &m_lpDSb, NULL));
			}

			// サウンドバッファを作成(セカンダリバッファと同一の長さ、1単位DWORD)
			if(b) {
				m_lpBuf = new DWORD [ m_uBufSize / 2 ];
				memset(m_lpBuf, sizeof(DWORD) * (m_uBufSize / 2), m_uBufSize);

				// OPMデバイス(標準)を作成
				m_pOPM = new FM::OPM;
				m_pOPM->Init(4000000, m_uRate, true);
				m_pOPM->Reset();
				m_pOPM->SetVolume(m_nFMVol);

				// OPMIFへ通知
				m_pOPMIF->InitBuf(m_uRate);
				m_pOPMIF->SetEngine(m_pOPM);

				// イネーブルなら演奏開始
				if (m_bEnable) {
					Play();
				}
			}
		}

		// 常に設定
		if (m_pOPM) {
			{
				int lVolume = 100;		//pConfig->master_volume;
				lVolume = 100 - lVolume;
				lVolume *= (DSBVOLUME_MAX - DSBVOLUME_MIN);
				lVolume /= -200;
				m_lpDSb->SetVolume(lVolume);
			}
			m_pOPMIF->EnableFM(1);		//pConfig->fm_enable);
			m_pOPM->SetVolume(54);		//pConfig->fm_volume);
			m_pADPCM->EnableADPCM(1);	//pConfig->adpcm_enable);
			m_pADPCM->SetVolume(52);	//pConfig->adpcm_volume);
		}
		m_nMaster	= 100;	//pConfig->master_volume;
		m_uPoll		= 5;	//(UINT)pConfig->polling_buffer;
	}

	return TRUE;
}
/*
//---------------------------------------------------------------------------
//
//	初期化サブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL CSound::InitSub()
{
	PCMWAVEFORMAT pcmwf;
	DSBUFFERDESC dsbd;
	WAVEFORMATEX wfex;

	// rate==0なら、何もしない
	if (m_uRate == 0) {
		return TRUE;
	}

	ASSERT(!m_lpDS);
	ASSERT(!m_lpDSp);
	ASSERT(!m_lpDSb);
	ASSERT(!m_lpBuf);
	ASSERT(!m_pOPM);

	// デバイスがなければ0で試し、それでもなければreturn
	if (m_nDeviceNum <= m_nSelectDevice) {
		if (m_nDeviceNum == 0) {
			return TRUE;
		}
		m_nSelectDevice = 0;
	}

	// DiectSoundオブジェクト作成
	if (FAILED(DirectSoundCreate(m_lpGUID[m_nSelectDevice], &m_lpDS, NULL))) {
		// デバイスは使用中
		return TRUE;
	}

	// 協調レベルを設定(優先協調)
	if (FAILED(m_lpDS->SetCooperativeLevel(m_pFrmWnd->m_hWnd, DSSCL_PRIORITY))) {
		return FALSE;
	}

	// プライマリバッファを作成
	memset(&dsbd, 0, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	if (FAILED(m_lpDS->CreateSoundBuffer(&dsbd, &m_lpDSp, NULL))) {
		return FALSE;
	}

	// プライマリバッファのフォーマットを指定
	memset(&wfex, 0, sizeof(wfex));
	wfex.wFormatTag = WAVE_FORMAT_PCM;
	wfex.nChannels = 2;
	wfex.nSamplesPerSec = m_uRate;
	wfex.nBlockAlign = 4;
	wfex.nAvgBytesPerSec = wfex.nSamplesPerSec * wfex.nBlockAlign;
	wfex.wBitsPerSample = 16;
	if (FAILED(m_lpDSp->SetFormat(&wfex))) {
		return FALSE;
	}

	// セカンダリバッファを作成
	memset(&pcmwf, 0, sizeof(pcmwf));
	pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
	pcmwf.wf.nChannels = 2;
	pcmwf.wf.nSamplesPerSec = m_uRate;
	pcmwf.wf.nBlockAlign = 4;
	pcmwf.wf.nAvgBytesPerSec = pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign;
	pcmwf.wBitsPerSample = 16;
	memset(&dsbd, 0, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME;
	dsbd.dwBufferBytes = (pcmwf.wf.nAvgBytesPerSec * m_uTick) / 1000;
	dsbd.dwBufferBytes = ((dsbd.dwBufferBytes + 7) >> 3) << 3;	// 8バイト境界
	m_uBufSize = dsbd.dwBufferBytes;
	dsbd.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf;
	if (FAILED(m_lpDS->CreateSoundBuffer(&dsbd, &m_lpDSb, NULL))) {
		return FALSE;
	}

	// サウンドバッファを作成(セカンダリバッファと同一の長さ、1単位DWORD)
	try {
		m_lpBuf = new DWORD [ m_uBufSize / 2 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!m_lpBuf) {
		return FALSE;
	}
	memset(m_lpBuf, sizeof(DWORD) * (m_uBufSize / 2), m_uBufSize);

	// OPMデバイス(標準)を作成
	m_pOPM = new FM::OPM;
	m_pOPM->Init(4000000, m_uRate, true);
	m_pOPM->Reset();
	m_pOPM->SetVolume(m_nFMVol);

	// OPMIFへ通知
	m_pOPMIF->InitBuf(m_uRate);
	m_pOPMIF->SetEngine(m_pOPM);

	// イネーブルなら演奏開始
	if (m_bEnable) {
		Play();
	}

	return TRUE;
}
*/
//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL CSound::Cleanup()
{
	// クリーンアップサブ
	{
		// サウンド停止
		Stop();

		// OPMIFへ通知
		if (m_pOPMIF) {
			m_pOPMIF->SetEngine(NULL);
		}

		// OPMを解放
		if (m_pOPM) {
			delete m_pOPM;
			m_pOPM = NULL;
		}

		// サウンド作成バッファを解放
		if (m_lpBuf) {
			delete[] m_lpBuf;
			m_lpBuf = NULL;
		}

		// DirectSoundBufferを解放
		if (m_lpDSb) {
			m_lpDSb->Release();
			m_lpDSb = NULL;
		}
		if (m_lpDSp) {
			m_lpDSp->Release();
			m_lpDSp = NULL;
		}

		// DirectSoundを解放
		if (m_lpDS) {
			m_lpDS->Release();
			m_lpDS = NULL;
		}

		// uRateをクリア
		m_uRate = 0;
	}

	// 基本クラス
	CComponent::Cleanup();
}
/*
//---------------------------------------------------------------------------
//
//	クリーンアップサブ
//
//---------------------------------------------------------------------------
void FASTCALL CSound::CleanupSub()
{
	// サウンド停止
	Stop();

	// OPMIFへ通知
	if (m_pOPMIF) {
		m_pOPMIF->SetEngine(NULL);
	}

	// OPMを解放
	if (m_pOPM) {
		delete m_pOPM;
		m_pOPM = NULL;
	}

	// サウンド作成バッファを解放
	if (m_lpBuf) {
		delete[] m_lpBuf;
		m_lpBuf = NULL;
	}

	// DirectSoundBufferを解放
	if (m_lpDSb) {
		m_lpDSb->Release();
		m_lpDSb = NULL;
	}
	if (m_lpDSp) {
		m_lpDSp->Release();
		m_lpDSp = NULL;
	}

	// DirectSoundを解放
	if (m_lpDS) {
		m_lpDS->Release();
		m_lpDS = NULL;
	}

	// uRateをクリア
	m_uRate = 0;
}
*/
//---------------------------------------------------------------------------
//
//	有効フラグ設定
//
//---------------------------------------------------------------------------
void FASTCALL CSound::Enable(BOOL bEnable)
{
	if (bEnable) {
		// 無効→有効 演奏開始
		if (!m_bEnable) {
			m_bEnable = TRUE;
			Play();
		}
	}
	else {
		// 有効→無効 演奏停止
		if (m_bEnable) {
			m_bEnable = FALSE;
			Stop();
		}
	}
}

//---------------------------------------------------------------------------
//
//	演奏開始
//
//---------------------------------------------------------------------------
void FASTCALL CSound::Play()
{
	ASSERT(m_bEnable);

	// 既に演奏開始なら必要なし
	// ポインタが有効なら演奏開始
	if (!m_bPlay && m_pOPM) {
		m_lpDSb->Play(0, 0, DSBPLAY_LOOPING);
		m_bPlay = TRUE;
		m_uCount = 0;
		m_dwWrite = 0;
	}
}

//---------------------------------------------------------------------------
//
//	演奏停止
//
//---------------------------------------------------------------------------
void FASTCALL CSound::Stop()
{
	// 既に演奏停止なら必要なし
	// ポインタが有効なら演奏停止
	if (m_bPlay && m_pOPM) {
		m_lpDSb->Stop();
		m_bPlay = FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	進行
//VC2010//	mfc_sch.cpp : CScheduler::Run() から呼ばれる。実行時は bRun=TRUE、停止時は bRun=FALSE で呼び出しが行われる
//
//---------------------------------------------------------------------------
void FASTCALL CSound::Process(BOOL bRun)
{
	HRESULT hr;
	DWORD dwOffset;
	DWORD dwWrite;
	DWORD dwRequest;
	DWORD dwReady;
	WORD *pBuf1;
	WORD *pBuf2;
	DWORD dwSize1;
	DWORD dwSize2;

	ASSERT(this);

	// カウント処理(m_nPoll回に１回、ただしVM停止中は常時)
	m_uCount++;
	if ((m_uCount < m_uPoll) && bRun) {
		return;
	}
	m_uCount = 0;

	// ディセーブルなら、何もしない
	if (!m_bEnable) {
		return;
	}

	// 初期化されていなければ、何もしない
	if (!m_pOPM) {
		m_pScheduler->SetSoundTime(0);
		return;
	}

	// プレイ状態でなければ、関係なし
	if (!m_bPlay) {
		m_pScheduler->SetSoundTime(0);
		return;
	}

	// 現在のプレイ位置を得る(バイト単位)
	ASSERT(m_lpDSb);
	ASSERT(m_lpBuf);
	if (FAILED(m_lpDSb->GetCurrentPosition(&dwOffset, &dwWrite))) {
		return;
	}
	ASSERT(m_lpDSb);
	ASSERT(m_lpBuf);

	// 前回書き込んだ位置から、空きサイズを計算(バイト単位)
	if (m_dwWrite <= dwOffset) {
		dwRequest = dwOffset - m_dwWrite;
	}
	else {
		dwRequest = m_uBufSize - m_dwWrite;
		dwRequest += dwOffset;
	}

	// 空きサイズが全体の1/4を超えていなければ、次の機会に
	if (dwRequest < (m_uBufSize / 4)) {
		return;
	}

	// 空きサンプルに換算(L,Rで1つと数える)
	ASSERT((dwRequest & 3) == 0);
	dwRequest /= 4;

	// m_lpBufにバッファデータを作成。まずbRunチェック
	if (!bRun) {
		memset(m_lpBuf, 0, m_uBufSize * 2);
		m_pOPMIF->InitBuf(m_uRate);
	}
	else {
		// OPMに対して、処理要求と速度制御
		dwReady = m_pOPMIF->ProcessBuf();
		m_pOPMIF->GetBuf(m_lpBuf, (int)dwRequest);
		if (dwReady < dwRequest) {
			dwRequest = dwReady;
		}

		// ADPCMに対して、データを要求(加算すること)
		m_pADPCM->GetBuf(m_lpBuf, (int)dwRequest);

		// ADPCMの同期処理
		if (dwReady > dwRequest) {
			m_pADPCM->Wait(dwReady - dwRequest);
		}
		else {
			m_pADPCM->Wait(0);
		}

		// SCSIに対して、データを要求(加算すること)
		m_pSCSI->GetBuf(m_lpBuf, (int)dwRequest, m_uRate);
	}

	// 次いでロック
	hr = m_lpDSb->Lock(m_dwWrite, (dwRequest * 4),
						(void**)&pBuf1, &dwSize1,
						(void**)&pBuf2, &dwSize2,
						0);
	// バッファが失われていれば、リストア
	if (hr == DSERR_BUFFERLOST) {
		m_lpDSb->Restore();
	}
	// ロック成功しなければ、続けても意味がない
	if (FAILED(hr)) {
		m_dwWrite = dwOffset;
		return;
	}

	// 量子化bit=16を前提とする
	ASSERT((dwSize1 & 1) == 0);
	ASSERT((dwSize2 & 1) == 0);

	// MMX命令によるパック(dwSize1+dwSize2で、平均5000～15000程度は処理する)
	SoundMMX(m_lpBuf, pBuf1, dwSize1);
	if (dwSize2 > 0) {
		SoundMMX(&m_lpBuf[dwSize1 / 2], pBuf2, dwSize2);
	}
	SoundEMMS();

	// アンロック
	m_lpDSb->Unlock(pBuf1, dwSize1, pBuf2, dwSize2);

	// m_dwWrite更新
	m_dwWrite += dwSize1;
	m_dwWrite += dwSize2;
	if (m_dwWrite >= m_uBufSize) {
		m_dwWrite -= m_uBufSize;
	}
	ASSERT(m_dwWrite < m_uBufSize);

	// 動作中ならWAV更新
//	if (bRun && m_pWav) {
//		ProcessSaveWav((int*)m_lpBuf, (dwSize1 + dwSize2));
//	}
}

//---------------------------------------------------------------------------
//
//	デバイス列挙
//
//---------------------------------------------------------------------------
void FASTCALL CSound::EnumDevice()
{
#if 0
	// 初期化
	m_nDeviceNum = 0;

	// 列挙開始
	DirectSoundEnumerate(EnumCallback, this);
#else
	//
	//	DirectSound Device enumerator
	//
	class DsEnumerator {
		enum {
			ENTRY_MAX	= 16,
		};

	public:
		struct Entry {
			LPGUID	lpGuid;
			LPCSTR	lpcstrDescription;
			LPCSTR	lpcstrModule;
			LPVOID	lpContext;
		};

		DsEnumerator() : nEntry(0) {
		}

		void enumerate() {
			nEntry = 0;
			DirectSoundEnumerate(DSEnumCallback_static, this);
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

	protected:
		static BOOL CALLBACK DSEnumCallback_static(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext) {
			return reinterpret_cast<DsEnumerator*>(lpContext)->DSEnumCallback(lpGuid, lpcstrDescription, lpcstrModule, lpContext);
		}

		BOOL DSEnumCallback(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext) {
			if(nEntry < ENTRY_MAX) {
				///	@note	bad code. fix this.
				//	See "Remarks" section :
				//		DSEnumCallback
				//		http://msdn.microsoft.com/en-us/library/windows/desktop/microsoft.directx_sdk.reference.dsenumcallback(v=vs.85).aspx
				//	Callback argument memories are local memory.
				//	We must do some allocations for save these variables.
				Entry& e = entry[nEntry++];
				e.lpGuid			= lpGuid;
				e.lpcstrDescription	= lpcstrDescription;
				e.lpcstrModule		= lpcstrModule;
				e.lpContext			= lpContext;
			}
			return TRUE;
		}

		//
		int		nEntry;
		Entry	entry[ENTRY_MAX];
	};

	DsEnumerator de;
	de.enumerate();
	m_nDeviceNum = de.getEntryCount();
	for(int i = 0; i < m_nDeviceNum; ++i) {
		m_lpGUID[i] = de.getEntry(i)->lpGuid;
	}
#endif
}
/*
//---------------------------------------------------------------------------
//
//	デバイス列挙コールバック
//
//---------------------------------------------------------------------------
BOOL CALLBACK CSound::EnumCallback(LPGUID lpGuid, LPCTSTR lpDescr, LPCTSTR , LPVOID lpContext)
{
	CSound *pSound;
	int index;

	// thisポインタ受け取り
	pSound = (CSound*)lpContext;
	ASSERT(pSound);

	// カレントが16未満なら記憶
	if (pSound->m_nDeviceNum < 16) {
		index = pSound->m_nDeviceNum;

		// 登録
		pSound->m_lpGUID[index] = lpGuid;
//		pSound->m_DeviceDescr[index] = lpDescr;
		pSound->m_nDeviceNum++;
	}

	return TRUE;
}
*/
#endif	// _WIN32
