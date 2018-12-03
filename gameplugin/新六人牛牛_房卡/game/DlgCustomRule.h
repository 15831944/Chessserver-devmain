#ifndef GAME_DLG_CUSTOM_RULE_HEAD_FILE
#define GAME_DLG_CUSTOM_RULE_HEAD_FILE

#pragma once

#include "Stdafx.h"

//////////////////////////////////////////////////////////////////////////////////

//���ýṹ
struct tagCustomRule
{
	SCORE									lRoomStorageStart;			//�����ʼֵ
	SCORE									lRoomStorageDeduct;			//˥��ֵ
	SCORE									lRoomStorageMax1;			//���ⶥֵ1
	SCORE									lRoomStorageMul1;			//Ӯ�ְٷֱ�1
	SCORE									lRoomStorageMax2;			//���ⶥֵ1
	SCORE									lRoomStorageMul2;			//Ӯ�ְٷֱ�1

	//�����˴��ȡ��
	SCORE									lRobotScoreMin;	
	SCORE									lRobotScoreMax;
	SCORE	                                lRobotBankGet; 
	SCORE									lRobotBankGetBanker; 
	SCORE									lRobotBankStoMul; 

	//��Ϸ����
	CARDTYPE_CONFIG							ctConfig;
	SENDCARDTYPE_CONFIG						stConfig;
	KING_CONFIG								gtConfig;
	BANERGAMETYPE_CONFIG					bgtConfig;
	BETTYPE_CONFIG							btConfig;
	
	//�������ö��(��Чֵ0)
	LONG									lFreeConfig[MAX_CONFIG];

	//�ٷֱ����ö��(��Чֵ0)
	LONG									lPercentConfig[MAX_CONFIG];
};

//////////////////////////////////////////////////////////////////////////////////

//���ô���
class CDlgCustomRule : public CDialog
{
	//���ñ���
protected:
	tagCustomRule					m_CustomRule;						//���ýṹ

	//��������
public:
	//���캯��
	CDlgCustomRule();
	//��������
	virtual ~CDlgCustomRule();

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
	bool GetCustomRule(tagCustomRule & CustomRule);
	//��������
	bool SetCustomRule(tagCustomRule & CustomRule);
	
	afx_msg void OnClickBTFree();

	afx_msg void OnClickBTPercent();

	DECLARE_MESSAGE_MAP()
};

//////////////////////////////////////////////////////////////////////////////////

#endif