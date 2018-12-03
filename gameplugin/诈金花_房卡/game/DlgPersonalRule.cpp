#include "Stdafx.h"
#include "Resource.h"
#include "DlgPersonalRule.h"

//////////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CDlgPersonalRule, CDialog)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////////////
const int g_nCellScore[CELLSCORE_COUNT] = {100, 200, 500, 1000, 2000};

//���캯��
CDlgPersonalRule::CDlgPersonalRule() : CDialog(IDD_PERSONAL_RULE)
{
	//���ñ���
	ZeroMemory(&m_ZaJinHuaSpecialRule,sizeof(m_ZaJinHuaSpecialRule));
	
	m_ZaJinHuaSpecialRule.lMaxCellScore_Score = 1000;
	m_ZaJinHuaSpecialRule.lTurnMaxScore_Score = 10000;
	m_ZaJinHuaSpecialRule.lTurnMaxScore_Treasure = 10000;

	return;
}

//��������
CDlgPersonalRule::~CDlgPersonalRule()
{
}

//���ú���
BOOL CDlgPersonalRule::OnInitDialog()
{
	__super::OnInitDialog();

	//���ÿؼ�
	((CEdit *)GetDlgItem(IDC_EDIT_MAXCELLSCORE_SCORE))->LimitText(9);
	((CEdit *)GetDlgItem(IDC_EDIT_TURNMAXSCORE_SCORE))->LimitText(9);
	((CEdit *)GetDlgItem(IDC_EDIT_TURNMAXSCORE_TREASURE))->LimitText(9);

	//���²���
	FillDataToControl();

	return FALSE;
}

//ȷ������
VOID CDlgPersonalRule::OnOK() 
{ 
	//Ͷ����Ϣ
	GetParent()->PostMessage(WM_COMMAND,MAKELONG(IDOK,0),0);

	return;
}

//ȡ����Ϣ
VOID CDlgPersonalRule::OnCancel() 
{ 
	//Ͷ����Ϣ
	GetParent()->PostMessage(WM_COMMAND,MAKELONG(IDCANCEL,0),0);

	return;
}

//���¿ؼ�
bool CDlgPersonalRule::FillDataToControl()
{
	//���÷����������
	int i = IDC_TIME_START_GAME5;
	for(; i <  IDC_TIME_START_GAME5 + 20;  i += 4)
	{
		SetDlgItemInt(i,		m_ZaJinHuaSpecialRule.comPersonalRule.personalRule[(i -IDC_TIME_START_GAME5) /4].nTurnCount);
		SetDlgItemInt(i + 1, m_ZaJinHuaSpecialRule.comPersonalRule.personalRule[(i -IDC_TIME_START_GAME5) /4].nPlayerCount);
		SetDlgItemInt(i + 2, m_ZaJinHuaSpecialRule.comPersonalRule.personalRule[(i -IDC_TIME_START_GAME5) /4].nFee);
		SetDlgItemInt(i + 3, m_ZaJinHuaSpecialRule.comPersonalRule.personalRule[(i -IDC_TIME_START_GAME5) /4].nIniScore);
	}

	//���÷���׷�
	for(i = IDC_EDIT1;  i < IDC_EDIT1 + CELLSCORE_COUNT; i++)
	{
		SetDlgItemInt(i,		m_ZaJinHuaSpecialRule.comPersonalRule.nCellScore[i - IDC_EDIT1]);
	}
	
	SetDlgItemInt(IDC_EDIT_MAXCELLSCORE_SCORE, m_ZaJinHuaSpecialRule.lMaxCellScore_Score);
	SetDlgItemInt(IDC_EDIT_TURNMAXSCORE_SCORE, m_ZaJinHuaSpecialRule.lTurnMaxScore_Score);
	SetDlgItemInt(IDC_EDIT_TURNMAXSCORE_TREASURE, m_ZaJinHuaSpecialRule.lTurnMaxScore_Treasure);

	return true;
}

//��������
bool CDlgPersonalRule::FillControlToData()
{
	//��������
	m_ZaJinHuaSpecialRule.comPersonalRule.cbSpecialRule = 1;
	int i = IDC_TIME_START_GAME5;
	for(; i <  IDC_TIME_START_GAME5 + 20;  i += 4)
	{
		m_ZaJinHuaSpecialRule.comPersonalRule.personalRule[(i - IDC_TIME_START_GAME5) /4].nTurnCount = GetDlgItemInt(i);
		m_ZaJinHuaSpecialRule.comPersonalRule.personalRule[(i - IDC_TIME_START_GAME5) /4].nPlayerCount = GetDlgItemInt(i + 1);
		m_ZaJinHuaSpecialRule.comPersonalRule.personalRule[(i - IDC_TIME_START_GAME5) /4].nFee = GetDlgItemInt(i + 2);
		m_ZaJinHuaSpecialRule.comPersonalRule.personalRule[(i - IDC_TIME_START_GAME5) /4].nIniScore = GetDlgItemInt(i + 3);
	}

	//��ȡ����׷�
	for(i = IDC_EDIT1;  i < IDC_EDIT1 + CELLSCORE_COUNT; i++)
	{
		m_ZaJinHuaSpecialRule.comPersonalRule.nCellScore[i - IDC_EDIT1] = GetDlgItemInt(i);
	}
	
	m_ZaJinHuaSpecialRule.lMaxCellScore_Score = (SCORE)GetDlgItemInt(IDC_EDIT_MAXCELLSCORE_SCORE);
	m_ZaJinHuaSpecialRule.lTurnMaxScore_Score = (SCORE)GetDlgItemInt(IDC_EDIT_TURNMAXSCORE_SCORE);
	m_ZaJinHuaSpecialRule.lTurnMaxScore_Treasure = (SCORE)GetDlgItemInt(IDC_EDIT_TURNMAXSCORE_TREASURE);

	return true;
}

//��ȡ����
bool CDlgPersonalRule::GetPersonalRule(tagZaJinHuaSpecial & ZaJinHuaSpecialRule)
{
	//��ȡ����
	if (FillControlToData()==true)
	{
		ZaJinHuaSpecialRule=m_ZaJinHuaSpecialRule;
		return true;
	}

	return false;
}

//��������
bool CDlgPersonalRule::SetPersonalRule(tagZaJinHuaSpecial & ZaJinHuaSpecialRule)
{
	//���ñ���
	m_ZaJinHuaSpecialRule=ZaJinHuaSpecialRule;
	
	if(m_ZaJinHuaSpecialRule.comPersonalRule.personalRule[0].nTurnCount  == 0 ||
		m_ZaJinHuaSpecialRule.comPersonalRule.personalRule[0].nFee == 0)
	{
		m_ZaJinHuaSpecialRule.comPersonalRule.personalRule[0].nTurnCount  = 5;
		m_ZaJinHuaSpecialRule.comPersonalRule.personalRule[0].nFee = 1;
		m_ZaJinHuaSpecialRule.comPersonalRule.personalRule[0].nIniScore = 1000; 
	}

	if(m_ZaJinHuaSpecialRule.comPersonalRule.nCellScore[0]  == 0)
	{
		m_ZaJinHuaSpecialRule.comPersonalRule.nCellScore[0] = 10;
		m_ZaJinHuaSpecialRule.comPersonalRule.nCellScore[1] = 20;
		m_ZaJinHuaSpecialRule.comPersonalRule.nCellScore[2] = 30;
		m_ZaJinHuaSpecialRule.comPersonalRule.nCellScore[3] = 40;
		m_ZaJinHuaSpecialRule.comPersonalRule.nCellScore[4] = 50;
	}

	//���²���
	if (m_hWnd!=NULL) FillDataToControl();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////
