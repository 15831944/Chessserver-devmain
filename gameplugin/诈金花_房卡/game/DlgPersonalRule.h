#ifndef GAME_DLG_PERSONAL_RULE_HEAD_FILE
#define GAME_DLG_PERSONAL_RULE_HEAD_FILE

#pragma once

#include "Stdafx.h"

//////////////////////////////////////////////////////////////////////////////////
//���ýṹ
struct tagZaJinHuaSpecial
{
	tagPersonalRule					comPersonalRule;

	LONGLONG						lMaxCellScore_Score;
	LONGLONG						lTurnMaxScore_Score;
	LONGLONG						lTurnMaxScore_Treasure;

	tagZaJinHuaSpecial()
	{
		ZeroMemory(&comPersonalRule, sizeof(comPersonalRule));
		lMaxCellScore_Score = 0L;
		lTurnMaxScore_Score = 0L;
		lTurnMaxScore_Treasure = 0L;
	}
};

//////////////////////////////////////////////////////////////////////////////////

//���ô���
class CDlgPersonalRule : public CDialog
{
	//���ñ���
protected:
	tagZaJinHuaSpecial					m_ZaJinHuaSpecialRule;						//���ýṹ

	//��������
public:
	//���캯��
	CDlgPersonalRule();
	//��������
	virtual ~CDlgPersonalRule();

	//���غ���
protected:
	//���ú���
	virtual BOOL OnInitDialog();
	//ȷ������
	virtual VOID OnOK();
	//ȡ����Ϣ
	virtual VOID OnCancel();

	//���ܺ���
public:
	//���¿ؼ�
	bool FillDataToControl();
	//��������
	bool FillControlToData();

	//���ú���
public:
	//��ȡ����
	bool GetPersonalRule(tagZaJinHuaSpecial & ZaJinHuaSpecialRule);
	//��������
	bool SetPersonalRule(tagZaJinHuaSpecial & ZaJinHuaSpecialRule);

	DECLARE_MESSAGE_MAP()
};

//////////////////////////////////////////////////////////////////////////////////

#endif