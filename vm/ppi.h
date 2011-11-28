//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ PPI(i8255A) ]
//
//---------------------------------------------------------------------------

#if !defined(ppi_h)
#define ppi_h

#include "device.h"

//===========================================================================
//
//	PPI
//
//===========================================================================
class PPI : public MemDevice
{
public:
	// �萔��`
	enum {
		PortMax = 2,					// �|�[�g�ő吔
		AxisMax = 4,					// ���ő吔
		ButtonMax = 8					// �{�^���ő吔
	};

	// �W���C�X�e�B�b�N�f�[�^��`
	typedef struct {
		uint32_t axis[AxisMax];				// �����
		int button[ButtonMax];				// �{�^�����
	} joyinfo_t;

	// �����f�[�^��`
	typedef struct {
		uint32_t portc;					// �|�[�gC
		int type[PortMax];				// �W���C�X�e�B�b�N�^�C�v
		uint32_t ctl[PortMax];				// �W���C�X�e�B�b�N�R���g���[��
		joyinfo_t info[PortMax];		// �W���C�X�e�B�b�N���
	} ppi_t;

public:
	// ��{�t�@���N�V����
	PPI(VM *p);
										// �R���X�g���N�^
	int FASTCALL Init();
										// ������
	void FASTCALL Cleanup();
										// �N���[���A�b�v
	void FASTCALL Reset();
										// ���Z�b�g
	int FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	int FASTCALL Load(Fileio *fio, int ver);
										// ���[�h
	void FASTCALL ApplyCfg(const Config *config);
										// �ݒ�K�p
#if defined(_DEBUG)
	void FASTCALL AssertDiag() const;
										// �f�f
#endif	// _DEBUG

	// �������f�o�C�X
	uint32_t FASTCALL ReadByte(uint32_t addr);
										// �o�C�g�ǂݍ���
	uint32_t FASTCALL ReadWord(uint32_t addr);
										// ���[�h�ǂݍ���
	void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// �o�C�g��������
	void FASTCALL WriteWord(uint32_t addr, uint32_t data);
										// ���[�h��������
	uint32_t FASTCALL ReadOnly(uint32_t addr) const;
										// �ǂݍ��݂̂�

	// �O��API
	void FASTCALL GetPPI(ppi_t *buffer);
										// �����f�[�^�擾
	void FASTCALL SetJoyInfo(int port, const joyinfo_t *info);
										// �W���C�X�e�B�b�N���ݒ�
	const joyinfo_t* FASTCALL GetJoyInfo(int port) const;
										// �W���C�X�e�B�b�N���擾
	JoyDevice* FASTCALL CreateJoy(int port, int type);
										// �W���C�X�e�B�b�N�f�o�C�X�쐬

private:
	void FASTCALL SetPortC(uint32_t data);
										// �|�[�gC�Z�b�g
	ADPCM *adpcm;
										// ADPCM
	JoyDevice *joy[PortMax];
										// �W���C�X�e�B�b�N
	ppi_t ppi;
										// �����f�[�^
};

//===========================================================================
//
//	�W���C�X�e�B�b�N�f�o�C�X
//
//===========================================================================
class JoyDevice
{
public:
	// ��{�t�@���N�V����
	JoyDevice(PPI *parent, int no);
										// �R���X�g���N�^
	virtual ~JoyDevice();
										// �f�X�g���N�^
	uint32_t FASTCALL GetID() const		{ return id; }
										// ID�擾
	uint32_t FASTCALL GetType() const		{ return type; }
										// �^�C�v�擾
	virtual void FASTCALL Reset();
										// ���Z�b�g
	virtual int FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	virtual int FASTCALL Load(Fileio *fio, int ver);
										// ���[�h

	// �A�N�Z�X
	virtual uint32_t FASTCALL ReadPort(uint32_t ctl);
										// �|�[�g�ǂݎ��
	virtual uint32_t FASTCALL ReadOnly(uint32_t ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	virtual void FASTCALL Control(uint32_t ctl);
										// �R���g���[��

	// �L���b�V��
	void FASTCALL Notify()				{ changed = TRUE; }
										// �e�|�[�g�ύX�ʒm
	virtual void FASTCALL MakeData();
										// �f�[�^�쐬

	// �v���p�e�B
	int FASTCALL GetAxes() const		{ return axes; }
										// �����擾
	const char* FASTCALL GetAxisDesc(int axis) const;
										// ���\���擾
	int FASTCALL GetButtons() const		{ return buttons; }
										// �{�^�����擾
	const char* FASTCALL GetButtonDesc(int button) const;
										// �{�^���\���擾
	int FASTCALL IsAnalog() const		{ return analog; }
										// �A�i���O�E�f�W�^���擾
	int FASTCALL GetDatas() const		{ return datas; }
										// �f�[�^���擾

protected:
	uint32_t type;
										// �^�C�v
	uint32_t id;
										// ID
	PPI *ppi;
										// PPI
	int port;
										// �|�[�g�ԍ�
	int axes;
										// ����
	const char **axis_desc;
										// ���\��
	int buttons;
										// �{�^����
	const char **button_desc;
										// �{�^���\��
	int analog;
										// ���(�A�i���O�E�f�W�^��)
	uint32_t *data;
										// �f�[�^����
	int datas;
										// �f�[�^��
	int changed;
										// �W���C�X�e�B�b�N�ύX�ʒm
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(ATARI�W��)
//
//===========================================================================
class JoyAtari : public JoyDevice
{
public:
	JoyAtari(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	uint32_t FASTCALL ReadOnly(uint32_t ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(ATARI�W��+START/SELECT)
//
//===========================================================================
class JoyASS : public JoyDevice
{
public:
	JoyASS(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	uint32_t FASTCALL ReadOnly(uint32_t ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(�T�C�o�[�X�e�B�b�N�E�A�i���O)
//
//===========================================================================
class JoyCyberA : public JoyDevice
{
public:
	JoyCyberA(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	void FASTCALL Reset();
										// ���Z�b�g
	uint32_t FASTCALL ReadPort(uint32_t ctl);
										// �|�[�g�ǂݎ��
	uint32_t FASTCALL ReadOnly(uint32_t ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL Control(uint32_t ctl);
										// �R���g���[��
	void FASTCALL MakeData();
										// �f�[�^�쐬
	int FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	int FASTCALL Load(Fileio *fio, int ver);
										// ���[�h

private:
	uint32_t seq;
										// �V�[�P���X
	uint32_t ctrl;
										// �O��̃R���g���[��(0 or 1)
	uint32_t hus;
										// ����p����
	Scheduler *scheduler;
										// �X�P�W���[��
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(�T�C�o�[�X�e�B�b�N�E�f�W�^��)
//
//===========================================================================
class JoyCyberD : public JoyDevice
{
public:
	JoyCyberD(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	uint32_t FASTCALL ReadOnly(uint32_t ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(MD3�{�^��)
//
//===========================================================================
class JoyMd3 : public JoyDevice
{
public:
	JoyMd3(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	uint32_t FASTCALL ReadOnly(uint32_t ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(MD6�{�^��)
//
//===========================================================================
class JoyMd6 : public JoyDevice
{
public:
	JoyMd6(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	void FASTCALL Reset();
										// ���Z�b�g
	uint32_t FASTCALL ReadOnly(uint32_t ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL Control(uint32_t ctl);
										// �R���g���[��
	void FASTCALL MakeData();
										// �f�[�^�쐬
	int FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	int FASTCALL Load(Fileio *fio, int ver);
										// ���[�h

private:
	uint32_t seq;
										// �V�[�P���X
	uint32_t ctrl;
										// �O��̃R���g���[��(0 or 1)
	uint32_t hus;
										// ����p����
	Scheduler *scheduler;
										// �X�P�W���[��
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(CPSF-SFC)
//
//===========================================================================
class JoyCpsf : public JoyDevice
{
public:
	JoyCpsf(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	uint32_t FASTCALL ReadOnly(uint32_t ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(CPSF-MD)
//
//===========================================================================
class JoyCpsfMd : public JoyDevice
{
public:
	JoyCpsfMd(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	uint32_t FASTCALL ReadOnly(uint32_t ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(�}�W�J���p�b�h)
//
//===========================================================================
class JoyMagical : public JoyDevice
{
public:
	JoyMagical(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	uint32_t FASTCALL ReadOnly(uint32_t ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(XPD-1LR)
//
//===========================================================================
class JoyLR : public JoyDevice
{
public:
	JoyLR(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	uint32_t FASTCALL ReadOnly(uint32_t ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(�p�b�N�����h��p�p�b�h)
//
//===========================================================================
class JoyPacl : public JoyDevice
{
public:
	JoyPacl(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	uint32_t FASTCALL ReadOnly(uint32_t ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(BM68��p�R���g���[��)
//
//===========================================================================
class JoyBM : public JoyDevice
{
public:
	JoyBM(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	uint32_t FASTCALL ReadOnly(uint32_t ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

#endif	// ppi_h
