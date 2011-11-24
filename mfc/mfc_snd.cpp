//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �T�E���h ]
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
//	�T�E���h
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CSound::CSound(CFrmWnd *pWnd) : CComponent(pWnd)
{
	// �R���|�[�l���g�p�����[�^
	m_dwID = MAKEID('S', 'N', 'D', ' ');
	m_strDesc = _T("Sound Renderer");

	// ���[�N������(�ݒ�p�����[�^)
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

	// ���[�N������(DirectSound�ƃI�u�W�F�N�g)
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

//	// ���[�N������(WAV�^��)
//	m_pWav = NULL;
//	m_nWav = 0;
//	m_dwWav = 0;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CSound::Init()
{
	// ��{�N���X
	if (!CComponent::Init()) {
		return FALSE;
	}

	// �X�P�W���[���擾
	m_pScheduler = (Scheduler*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pScheduler);

	// OPMIF�擾
	m_pOPMIF = (OPMIF*)::GetVM()->SearchDevice(MAKEID('O', 'P', 'M', ' '));
	ASSERT(m_pOPMIF);

	// ADPCM�擾
	m_pADPCM = (ADPCM*)::GetVM()->SearchDevice(MAKEID('A', 'P', 'C', 'M'));
	ASSERT(m_pADPCM);

	// SCSI�擾
	m_pSCSI = (SCSI*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(m_pSCSI);

	// �f�o�C�X��
	EnumDevice();

	// �����ł͏��������Ȃ�(ApplyCfg�ɔC����)

	//VC2010//	�ʓ|�Ȃ̂ł����ŏ�����
	{
		m_nSelectDevice		= 0;
		m_uRate				= 44100;
		m_uTick				= 100;

//		InitSub();
		{
			bool b = true;

			// rate==0�Ȃ�A�������Ȃ�
			if(b && m_uRate == 0) {
				b = false;
			}

			if(b) {
				ASSERT(!m_lpDS);
				ASSERT(!m_lpDSp);
				ASSERT(!m_lpDSb);
				ASSERT(!m_lpBuf);
				ASSERT(!m_pOPM);

				// �f�o�C�X���Ȃ����0�Ŏ����A����ł��Ȃ����return
				if (m_nDeviceNum <= m_nSelectDevice) {
					if (m_nDeviceNum == 0) {
						b = false;
					} else {
						m_nSelectDevice = 0;
					}
				}
			}

			// DiectSound�I�u�W�F�N�g�쐬
			if(b && FAILED(DirectSoundCreate(m_lpGUID[m_nSelectDevice], &m_lpDS, NULL))) {
				// �f�o�C�X�͎g�p��
				b = false;
			}

			// �������x����ݒ�(�D�拦��)
			if(b && FAILED(m_lpDS->SetCooperativeLevel(m_pFrmWnd->m_hWnd, DSSCL_PRIORITY))) {
				b = false;
			}

			// �v���C�}���o�b�t�@���쐬
			if(b) {
				DSBUFFERDESC dsbd = { 0 };
				dsbd.dwSize		= sizeof(dsbd);
				dsbd.dwFlags	= DSBCAPS_PRIMARYBUFFER;

				b = SUCCEEDED(m_lpDS->CreateSoundBuffer(&dsbd, &m_lpDSp, NULL));
			}

			// �v���C�}���o�b�t�@�̃t�H�[�}�b�g���w��
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

			// �Z�J���_���o�b�t�@���쐬
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
				dsbd.dwBufferBytes			= ((dsbd.dwBufferBytes + 7) >> 3) << 3;	// 8�o�C�g���E
				dsbd.lpwfxFormat			= (LPWAVEFORMATEX)&pcmwf;

				m_uBufSize					= dsbd.dwBufferBytes;

				b = SUCCEEDED(m_lpDS->CreateSoundBuffer(&dsbd, &m_lpDSb, NULL));
			}

			// �T�E���h�o�b�t�@���쐬(�Z�J���_���o�b�t�@�Ɠ���̒����A1�P��DWORD)
			if(b) {
				m_lpBuf = new DWORD [ m_uBufSize / 2 ];
				memset(m_lpBuf, sizeof(DWORD) * (m_uBufSize / 2), m_uBufSize);

				// OPM�f�o�C�X(�W��)���쐬
				m_pOPM = new FM::OPM;
				m_pOPM->Init(4000000, m_uRate, true);
				m_pOPM->Reset();
				m_pOPM->SetVolume(m_nFMVol);

				// OPMIF�֒ʒm
				m_pOPMIF->InitBuf(m_uRate);
				m_pOPMIF->SetEngine(m_pOPM);

				// �C�l�[�u���Ȃ牉�t�J�n
				if (m_bEnable) {
					Play();
				}
			}
		}

		// ��ɐݒ�
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
//	�������T�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL CSound::InitSub()
{
	PCMWAVEFORMAT pcmwf;
	DSBUFFERDESC dsbd;
	WAVEFORMATEX wfex;

	// rate==0�Ȃ�A�������Ȃ�
	if (m_uRate == 0) {
		return TRUE;
	}

	ASSERT(!m_lpDS);
	ASSERT(!m_lpDSp);
	ASSERT(!m_lpDSb);
	ASSERT(!m_lpBuf);
	ASSERT(!m_pOPM);

	// �f�o�C�X���Ȃ����0�Ŏ����A����ł��Ȃ����return
	if (m_nDeviceNum <= m_nSelectDevice) {
		if (m_nDeviceNum == 0) {
			return TRUE;
		}
		m_nSelectDevice = 0;
	}

	// DiectSound�I�u�W�F�N�g�쐬
	if (FAILED(DirectSoundCreate(m_lpGUID[m_nSelectDevice], &m_lpDS, NULL))) {
		// �f�o�C�X�͎g�p��
		return TRUE;
	}

	// �������x����ݒ�(�D�拦��)
	if (FAILED(m_lpDS->SetCooperativeLevel(m_pFrmWnd->m_hWnd, DSSCL_PRIORITY))) {
		return FALSE;
	}

	// �v���C�}���o�b�t�@���쐬
	memset(&dsbd, 0, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	if (FAILED(m_lpDS->CreateSoundBuffer(&dsbd, &m_lpDSp, NULL))) {
		return FALSE;
	}

	// �v���C�}���o�b�t�@�̃t�H�[�}�b�g���w��
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

	// �Z�J���_���o�b�t�@���쐬
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
	dsbd.dwBufferBytes = ((dsbd.dwBufferBytes + 7) >> 3) << 3;	// 8�o�C�g���E
	m_uBufSize = dsbd.dwBufferBytes;
	dsbd.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf;
	if (FAILED(m_lpDS->CreateSoundBuffer(&dsbd, &m_lpDSb, NULL))) {
		return FALSE;
	}

	// �T�E���h�o�b�t�@���쐬(�Z�J���_���o�b�t�@�Ɠ���̒����A1�P��DWORD)
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

	// OPM�f�o�C�X(�W��)���쐬
	m_pOPM = new FM::OPM;
	m_pOPM->Init(4000000, m_uRate, true);
	m_pOPM->Reset();
	m_pOPM->SetVolume(m_nFMVol);

	// OPMIF�֒ʒm
	m_pOPMIF->InitBuf(m_uRate);
	m_pOPMIF->SetEngine(m_pOPM);

	// �C�l�[�u���Ȃ牉�t�J�n
	if (m_bEnable) {
		Play();
	}

	return TRUE;
}
*/
//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CSound::Cleanup()
{
	// �N���[���A�b�v�T�u
	{
		// �T�E���h��~
		Stop();

		// OPMIF�֒ʒm
		if (m_pOPMIF) {
			m_pOPMIF->SetEngine(NULL);
		}

		// OPM�����
		if (m_pOPM) {
			delete m_pOPM;
			m_pOPM = NULL;
		}

		// �T�E���h�쐬�o�b�t�@�����
		if (m_lpBuf) {
			delete[] m_lpBuf;
			m_lpBuf = NULL;
		}

		// DirectSoundBuffer�����
		if (m_lpDSb) {
			m_lpDSb->Release();
			m_lpDSb = NULL;
		}
		if (m_lpDSp) {
			m_lpDSp->Release();
			m_lpDSp = NULL;
		}

		// DirectSound�����
		if (m_lpDS) {
			m_lpDS->Release();
			m_lpDS = NULL;
		}

		// uRate���N���A
		m_uRate = 0;
	}

	// ��{�N���X
	CComponent::Cleanup();
}
/*
//---------------------------------------------------------------------------
//
//	�N���[���A�b�v�T�u
//
//---------------------------------------------------------------------------
void FASTCALL CSound::CleanupSub()
{
	// �T�E���h��~
	Stop();

	// OPMIF�֒ʒm
	if (m_pOPMIF) {
		m_pOPMIF->SetEngine(NULL);
	}

	// OPM�����
	if (m_pOPM) {
		delete m_pOPM;
		m_pOPM = NULL;
	}

	// �T�E���h�쐬�o�b�t�@�����
	if (m_lpBuf) {
		delete[] m_lpBuf;
		m_lpBuf = NULL;
	}

	// DirectSoundBuffer�����
	if (m_lpDSb) {
		m_lpDSb->Release();
		m_lpDSb = NULL;
	}
	if (m_lpDSp) {
		m_lpDSp->Release();
		m_lpDSp = NULL;
	}

	// DirectSound�����
	if (m_lpDS) {
		m_lpDS->Release();
		m_lpDS = NULL;
	}

	// uRate���N���A
	m_uRate = 0;
}
*/
//---------------------------------------------------------------------------
//
//	�L���t���O�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL CSound::Enable(BOOL bEnable)
{
	if (bEnable) {
		// �������L�� ���t�J�n
		if (!m_bEnable) {
			m_bEnable = TRUE;
			Play();
		}
	}
	else {
		// �L�������� ���t��~
		if (m_bEnable) {
			m_bEnable = FALSE;
			Stop();
		}
	}
}

//---------------------------------------------------------------------------
//
//	���t�J�n
//
//---------------------------------------------------------------------------
void FASTCALL CSound::Play()
{
	ASSERT(m_bEnable);

	// ���ɉ��t�J�n�Ȃ�K�v�Ȃ�
	// �|�C���^���L���Ȃ牉�t�J�n
	if (!m_bPlay && m_pOPM) {
		m_lpDSb->Play(0, 0, DSBPLAY_LOOPING);
		m_bPlay = TRUE;
		m_uCount = 0;
		m_dwWrite = 0;
	}
}

//---------------------------------------------------------------------------
//
//	���t��~
//
//---------------------------------------------------------------------------
void FASTCALL CSound::Stop()
{
	// ���ɉ��t��~�Ȃ�K�v�Ȃ�
	// �|�C���^���L���Ȃ牉�t��~
	if (m_bPlay && m_pOPM) {
		m_lpDSb->Stop();
		m_bPlay = FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	�i�s
//VC2010//	mfc_sch.cpp : CScheduler::Run() ����Ă΂��B���s���� bRun=TRUE�A��~���� bRun=FALSE �ŌĂяo�����s����
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

	// �J�E���g����(m_nPoll��ɂP��A������VM��~���͏펞)
	m_uCount++;
	if ((m_uCount < m_uPoll) && bRun) {
		return;
	}
	m_uCount = 0;

	// �f�B�Z�[�u���Ȃ�A�������Ȃ�
	if (!m_bEnable) {
		return;
	}

	// ����������Ă��Ȃ���΁A�������Ȃ�
	if (!m_pOPM) {
		m_pScheduler->SetSoundTime(0);
		return;
	}

	// �v���C��ԂłȂ���΁A�֌W�Ȃ�
	if (!m_bPlay) {
		m_pScheduler->SetSoundTime(0);
		return;
	}

	// ���݂̃v���C�ʒu�𓾂�(�o�C�g�P��)
	ASSERT(m_lpDSb);
	ASSERT(m_lpBuf);
	if (FAILED(m_lpDSb->GetCurrentPosition(&dwOffset, &dwWrite))) {
		return;
	}
	ASSERT(m_lpDSb);
	ASSERT(m_lpBuf);

	// �O�񏑂����񂾈ʒu����A�󂫃T�C�Y���v�Z(�o�C�g�P��)
	if (m_dwWrite <= dwOffset) {
		dwRequest = dwOffset - m_dwWrite;
	}
	else {
		dwRequest = m_uBufSize - m_dwWrite;
		dwRequest += dwOffset;
	}

	// �󂫃T�C�Y���S�̂�1/4�𒴂��Ă��Ȃ���΁A���̋@���
	if (dwRequest < (m_uBufSize / 4)) {
		return;
	}

	// �󂫃T���v���Ɋ��Z(L,R��1�Ɛ�����)
	ASSERT((dwRequest & 3) == 0);
	dwRequest /= 4;

	// m_lpBuf�Ƀo�b�t�@�f�[�^���쐬�B�܂�bRun�`�F�b�N
	if (!bRun) {
		memset(m_lpBuf, 0, m_uBufSize * 2);
		m_pOPMIF->InitBuf(m_uRate);
	}
	else {
		// OPM�ɑ΂��āA�����v���Ƒ��x����
		dwReady = m_pOPMIF->ProcessBuf();
		m_pOPMIF->GetBuf(m_lpBuf, (int)dwRequest);
		if (dwReady < dwRequest) {
			dwRequest = dwReady;
		}

		// ADPCM�ɑ΂��āA�f�[�^��v��(���Z���邱��)
		m_pADPCM->GetBuf(m_lpBuf, (int)dwRequest);

		// ADPCM�̓�������
		if (dwReady > dwRequest) {
			m_pADPCM->Wait(dwReady - dwRequest);
		}
		else {
			m_pADPCM->Wait(0);
		}

		// SCSI�ɑ΂��āA�f�[�^��v��(���Z���邱��)
		m_pSCSI->GetBuf(m_lpBuf, (int)dwRequest, m_uRate);
	}

	// �����Ń��b�N
	hr = m_lpDSb->Lock(m_dwWrite, (dwRequest * 4),
						(void**)&pBuf1, &dwSize1,
						(void**)&pBuf2, &dwSize2,
						0);
	// �o�b�t�@�������Ă���΁A���X�g�A
	if (hr == DSERR_BUFFERLOST) {
		m_lpDSb->Restore();
	}
	// ���b�N�������Ȃ���΁A�����Ă��Ӗ����Ȃ�
	if (FAILED(hr)) {
		m_dwWrite = dwOffset;
		return;
	}

	// �ʎq��bit=16��O��Ƃ���
	ASSERT((dwSize1 & 1) == 0);
	ASSERT((dwSize2 & 1) == 0);

	// MMX���߂ɂ��p�b�N(dwSize1+dwSize2�ŁA����5000�`15000���x�͏�������)
	SoundMMX(m_lpBuf, pBuf1, dwSize1);
	if (dwSize2 > 0) {
		SoundMMX(&m_lpBuf[dwSize1 / 2], pBuf2, dwSize2);
	}
	SoundEMMS();

	// �A�����b�N
	m_lpDSb->Unlock(pBuf1, dwSize1, pBuf2, dwSize2);

	// m_dwWrite�X�V
	m_dwWrite += dwSize1;
	m_dwWrite += dwSize2;
	if (m_dwWrite >= m_uBufSize) {
		m_dwWrite -= m_uBufSize;
	}
	ASSERT(m_dwWrite < m_uBufSize);

	// ���쒆�Ȃ�WAV�X�V
//	if (bRun && m_pWav) {
//		ProcessSaveWav((int*)m_lpBuf, (dwSize1 + dwSize2));
//	}
}

//---------------------------------------------------------------------------
//
//	�f�o�C�X��
//
//---------------------------------------------------------------------------
void FASTCALL CSound::EnumDevice()
{
#if 0
	// ������
	m_nDeviceNum = 0;

	// �񋓊J�n
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
//	�f�o�C�X�񋓃R�[���o�b�N
//
//---------------------------------------------------------------------------
BOOL CALLBACK CSound::EnumCallback(LPGUID lpGuid, LPCTSTR lpDescr, LPCTSTR , LPVOID lpContext)
{
	CSound *pSound;
	int index;

	// this�|�C���^�󂯎��
	pSound = (CSound*)lpContext;
	ASSERT(pSound);

	// �J�����g��16�����Ȃ�L��
	if (pSound->m_nDeviceNum < 16) {
		index = pSound->m_nDeviceNum;

		// �o�^
		pSound->m_lpGUID[index] = lpGuid;
//		pSound->m_DeviceDescr[index] = lpDescr;
		pSound->m_nDeviceNum++;
	}

	return TRUE;
}
*/
#endif	// _WIN32
