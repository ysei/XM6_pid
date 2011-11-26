//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	Modified (C) 2006 co (cogood��gmail.com)
//	[ Windrv ]
//
//---------------------------------------------------------------------------

#if !defined(windrv_h)
#define windrv_h

//===========================================================================
//
//	Windrv
//
//===========================================================================
class Windrv : public MemDevice
{
public:
	Windrv(VM *p);										// �R���X�g���N�^

private:
	BOOL FASTCALL Init();								// ������
	void FASTCALL Cleanup();							// �N���[���A�b�v
	void FASTCALL Reset();								// ���Z�b�g

	// �������f�o�C�X
	DWORD FASTCALL ReadByte(DWORD addr);				// �o�C�g�ǂݍ���
	void FASTCALL WriteByte(DWORD addr, DWORD data);	// �o�C�g��������
	DWORD FASTCALL ReadOnly(DWORD addr) const;			// �ǂݍ��݂̂�
};
#endif // windrv_h
