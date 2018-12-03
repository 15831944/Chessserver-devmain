#include "Stdafx.h"
#include "Resource.h"
#include "DlgCustomRule.h"

//////////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CDlgCustomRule, CDialog)
	ON_BN_CLICKED(IDC_RADIO_BT_FREE, OnClickBTFree)
	ON_BN_CLICKED(IDC_RADIO_BT_PERCENT, OnClickBTPercent)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////////////

//���캯��
CDlgCustomRule::CDlgCustomRule() : CDialog(IDD_CUSTOM_RULE)
{
	//���ñ���
	ZeroMemory(&m_CustomRule,sizeof(m_CustomRule));

	m_CustomRule.lRoomStorageStart = 100000;
	m_CustomRule.lRoomStorageDeduct = 0;
	m_CustomRule.lRoomStorageMax1 = 1000000;
	m_CustomRule.lRoomStorageMul1 = 50;
	m_CustomRule.lRoomStorageMax2 = 5000000;
	m_CustomRule.lRoomStorageMul2 = 80;

	//�����˴��ȡ��
	m_CustomRule.lRobotScoreMin = 100000;
	m_CustomRule.lRobotScoreMax = 1000000;
	m_CustomRule.lRobotBankGet = 1000000;
	m_CustomRule.lRobotBankGetBanker = 10000000;
	m_CustomRule.lRobotBankStoMul = 10;


	m_CustomRule.ctConfig = CT_CLASSIC_;
	m_CustomRule.stConfig = ST_SENDFOUR_;
	m_CustomRule.gtConfig = GT_HAVEKING_;
	m_CustomRule.bgtConfig = BGT_DESPOT_;
	m_CustomRule.btConfig = BT_FREE_;

	m_CustomRule.lFreeConfig[0] = 200;
	m_CustomRule.lFreeConfig[1] = 500;
	m_CustomRule.lFreeConfig[2] = 800;
	m_CustomRule.lFreeConfig[3] = 1100;
	m_CustomRule.lFreeConfig[4] = 1500;
	
	ZeroMemory(m_CustomRule.lPercentConfig, sizeof(m_CustomRule.lPercentConfig));
    
	return;
}

//��������
CDlgCustomRule::~CDlgCustomRule()
{
}

//���ú���
BOOL CDlgCustomRule::OnInitDialog()
{
	__super::OnInitDialog();
	
	//���ÿؼ�
	((CEdit *)GetDlgItem(IDC_EDIT_ROOMSTORAGE_START))->LimitText(9);
	((CEdit *)GetDlgItem(IDC_EDIT_ROOMSTORAGE_DEDUCT))->LimitText(3);
	((CEdit *)GetDlgItem(IDC_EDIT_ROOMSTORAGE_MAX1))->LimitText(9);
	((CEdit *)GetDlgItem(IDC_EDIT_ROOMSTORAGE_MUL1))->LimitText(2);
	((CEdit *)GetDlgItem(IDC_EDIT_ROOMSTORAGE_MAX2))->LimitText(9);
	((CEdit *)GetDlgItem(IDC_EDIT_ROOMSTORAGE_MUL2))->LimitText(2);

	((CEdit *)GetDlgItem(IDC_ROBOT_SCOREMIN))->LimitText(9);
	((CEdit *)GetDlgItem(IDC_ROBOT_BANKERGETBANKER))->LimitText(9);
	((CEdit *)GetDlgItem(IDC_ROBOT_SCOREMAX))->LimitText(9);
	((CEdit *)GetDlgItem(IDC_ROBOT_BANKGET))->LimitText(9);
	((CEdit *)GetDlgItem(IDC_ROBOT_STOMUL))->LimitText(2);
	
	for (WORD i=0; i<MAX_CONFIG; i++)
	{
		((CEdit *)GetDlgItem(IDC_EDIT_BT_FREE_0 + i))->LimitText(9);
	}
	
	for (WORD i=0; i<MAX_CONFIG; i++)
	{
		((CEdit *)GetDlgItem(IDC_EDIT_BT_PERCENT_0 + i))->LimitText(3);
	}

	//���²���
	FillDataToControl();

	return FALSE;
}

//ȷ������
VOID CDlgCustomRule::OnOK() 
{ 
	//Ͷ����Ϣ
	GetParent()->PostMessage(WM_COMMAND,MAKELONG(IDOK,0),0);

	return;
}

//ȡ����Ϣ
VOID CDlgCustomRule::OnCancel() 
{ 
	//Ͷ����Ϣ
	GetParent()->PostMessage(WM_COMMAND,MAKELONG(IDCANCEL,0),0);

	return;
}

//���¿ؼ�
bool CDlgCustomRule::FillDataToControl()
{
	//��������
	SetDlgItemInt(IDC_EDIT_ROOMSTORAGE_START, m_CustomRule.lRoomStorageStart);
	SetDlgItemInt(IDC_EDIT_ROOMSTORAGE_DEDUCT, m_CustomRule.lRoomStorageDeduct);
	SetDlgItemInt(IDC_EDIT_ROOMSTORAGE_MAX1, m_CustomRule.lRoomStorageMax1);
	SetDlgItemInt(IDC_EDIT_ROOMSTORAGE_MUL1, m_CustomRule.lRoomStorageMul1);
	SetDlgItemInt(IDC_EDIT_ROOMSTORAGE_MAX2, m_CustomRule.lRoomStorageMax2);
	SetDlgItemInt(IDC_EDIT_ROOMSTORAGE_MUL2, m_CustomRule.lRoomStorageMul2);

	//�����˿���
	SetDlgItemInt(IDC_ROBOT_SCOREMIN, m_CustomRule.lRobotScoreMin);
	SetDlgItemInt(IDC_ROBOT_SCOREMAX, m_CustomRule.lRobotScoreMax);
	SetDlgItemInt(IDC_ROBOT_BANKERGETBANKER, m_CustomRule.lRobotBankGetBanker);
	SetDlgItemInt(IDC_ROBOT_BANKGET, m_CustomRule.lRobotBankGet);
	SetDlgItemInt(IDC_ROBOT_STOMUL, m_CustomRule.lRobotBankStoMul);

	//��Ϸ����
	if (m_CustomRule.ctConfig == CT_CLASSIC_)
	{
		((CButton *)GetDlgItem(IDC_RADIO_CT_CLASSIC))->SetCheck(TRUE);
		((CButton *)GetDlgItem(IDC_RADIO_CT_ADDTIMES))->SetCheck(FALSE);
	}
	else if (m_CustomRule.ctConfig == CT_ADDTIMES_)
	{
		((CButton *)GetDlgItem(IDC_RADIO_CT_CLASSIC))->SetCheck(FALSE);
		((CButton *)GetDlgItem(IDC_RADIO_CT_ADDTIMES))->SetCheck(TRUE);
	}

	//����ģʽ
	if (m_CustomRule.stConfig == ST_SENDFOUR_)
	{
		((CButton *)GetDlgItem(IDC_RADIO_ST_SENDFOUR))->SetCheck(TRUE);
		((CButton *)GetDlgItem(IDC_RADIO_ST_BETFIRST))->SetCheck(FALSE);
	}
	else if (m_CustomRule.stConfig == ST_BETFIRST_)
	{
		((CButton *)GetDlgItem(IDC_RADIO_ST_SENDFOUR))->SetCheck(FALSE);
		((CButton *)GetDlgItem(IDC_RADIO_ST_BETFIRST))->SetCheck(TRUE);
	}

	//�˿��淨
	if (m_CustomRule.gtConfig == GT_HAVEKING_)
	{
		((CButton *)GetDlgItem(IDC_RADIO_HAVEKING))->SetCheck(TRUE);
		((CButton *)GetDlgItem(IDC_RADIO_NOKING))->SetCheck(FALSE);
	}
	else if (m_CustomRule.gtConfig == GT_NOKING_)
	{
		((CButton *)GetDlgItem(IDC_RADIO_HAVEKING))->SetCheck(FALSE);
		((CButton *)GetDlgItem(IDC_RADIO_NOKING))->SetCheck(TRUE);
	}

	//ׯ���淨
	if (m_CustomRule.bgtConfig == BGT_DESPOT_)
	{
		((CButton *)GetDlgItem(IDC_RADIO_DESPOT_BANKER))->SetCheck(TRUE);
		((CButton *)GetDlgItem(IDC_RADIO_ROB_BANKER))->SetCheck(FALSE);
		((CButton *)GetDlgItem(IDC_RADIO_NIUNIU_BANKER))->SetCheck(FALSE);
		((CButton *)GetDlgItem(IDC_RADIO_NONIUNIU))->SetCheck(FALSE);
	}
	else if (m_CustomRule.bgtConfig == BGT_ROB_)
	{
		((CButton *)GetDlgItem(IDC_RADIO_DESPOT_BANKER))->SetCheck(FALSE);
		((CButton *)GetDlgItem(IDC_RADIO_ROB_BANKER))->SetCheck(TRUE);
		((CButton *)GetDlgItem(IDC_RADIO_NIUNIU_BANKER))->SetCheck(FALSE);
		((CButton *)GetDlgItem(IDC_RADIO_NONIUNIU))->SetCheck(FALSE);
	}
	else if (m_CustomRule.bgtConfig == BGT_NIUNIU_)
	{
		((CButton *)GetDlgItem(IDC_RADIO_DESPOT_BANKER))->SetCheck(FALSE);
		((CButton *)GetDlgItem(IDC_RADIO_ROB_BANKER))->SetCheck(FALSE);
		((CButton *)GetDlgItem(IDC_RADIO_NIUNIU_BANKER))->SetCheck(TRUE);
		((CButton *)GetDlgItem(IDC_RADIO_NONIUNIU))->SetCheck(FALSE);
	}
	else if (m_CustomRule.bgtConfig == BGT_NONIUNIU_)
	{
		((CButton *)GetDlgItem(IDC_RADIO_DESPOT_BANKER))->SetCheck(FALSE);
		((CButton *)GetDlgItem(IDC_RADIO_ROB_BANKER))->SetCheck(FALSE);
		((CButton *)GetDlgItem(IDC_RADIO_NIUNIU_BANKER))->SetCheck(FALSE);
		((CButton *)GetDlgItem(IDC_RADIO_NONIUNIU))->SetCheck(TRUE);
	}
	
	//��ע����
	if (m_CustomRule.btConfig == BT_FREE_)
	{
		((CButton *)GetDlgItem(IDC_RADIO_BT_FREE))->SetCheck(TRUE);
		((CButton *)GetDlgItem(IDC_RADIO_BT_PERCENT))->SetCheck(FALSE);
		
		for (WORD i=0; i<MAX_CONFIG; i++)
		{
			((CEdit *)GetDlgItem(IDC_EDIT_BT_PERCENT_0 + i))->SetWindowText(TEXT(""));
			((CEdit *)GetDlgItem(IDC_EDIT_BT_PERCENT_0 + i))->EnableWindow(FALSE);


			SetDlgItemInt(IDC_EDIT_BT_FREE_0 + i, m_CustomRule.lFreeConfig[i]);
			((CEdit *)GetDlgItem(IDC_EDIT_BT_FREE_0 + i))->EnableWindow(TRUE);
		}
	}
	else if (m_CustomRule.btConfig == BT_PENCENT_)
	{
		((CButton *)GetDlgItem(IDC_RADIO_BT_FREE))->SetCheck(FALSE);
		((CButton *)GetDlgItem(IDC_RADIO_BT_PERCENT))->SetCheck(TRUE);

		for (WORD i=0; i<MAX_CONFIG; i++)
		{
			((CEdit *)GetDlgItem(IDC_EDIT_BT_FREE_0 + i))->SetWindowText(TEXT(""));
			((CEdit *)GetDlgItem(IDC_EDIT_BT_FREE_0 + i))->EnableWindow(FALSE);


			SetDlgItemInt(IDC_EDIT_BT_PERCENT_0 + i, m_CustomRule.lPercentConfig[i]);
			((CEdit *)GetDlgItem(IDC_EDIT_BT_PERCENT_0 + i))->EnableWindow(TRUE);
		}
	}

	return true;
}

//��������
bool CDlgCustomRule::FillControlToData()
{
	//��������
	m_CustomRule.lRoomStorageStart = (SCORE)GetDlgItemInt(IDC_EDIT_ROOMSTORAGE_START);
	m_CustomRule.lRoomStorageDeduct = (SCORE)GetDlgItemInt(IDC_EDIT_ROOMSTORAGE_DEDUCT);
	m_CustomRule.lRoomStorageMax1 = (SCORE)GetDlgItemInt(IDC_EDIT_ROOMSTORAGE_MAX1);
	m_CustomRule.lRoomStorageMul1 = (SCORE)GetDlgItemInt(IDC_EDIT_ROOMSTORAGE_MUL1);
	m_CustomRule.lRoomStorageMax2 = (SCORE)GetDlgItemInt(IDC_EDIT_ROOMSTORAGE_MAX2);
	m_CustomRule.lRoomStorageMul2 = (SCORE)GetDlgItemInt(IDC_EDIT_ROOMSTORAGE_MUL2);

	//�����˿���
	m_CustomRule.lRobotScoreMin = (SCORE)GetDlgItemInt(IDC_ROBOT_SCOREMIN);
	m_CustomRule.lRobotScoreMax = (SCORE)GetDlgItemInt(IDC_ROBOT_SCOREMAX);
	m_CustomRule.lRobotBankGetBanker = (SCORE)GetDlgItemInt(IDC_ROBOT_BANKERGETBANKER);
	m_CustomRule.lRobotBankGet = (SCORE)GetDlgItemInt(IDC_ROBOT_BANKGET);
	m_CustomRule.lRobotBankStoMul = (SCORE)GetDlgItemInt(IDC_ROBOT_STOMUL);
	
	//��Ϸ����
	//��Ϸ����
	if (((CButton *)GetDlgItem(IDC_RADIO_CT_CLASSIC))->GetCheck() == TRUE && ((CButton *)GetDlgItem(IDC_RADIO_CT_ADDTIMES))->GetCheck() == FALSE)
	{
		m_CustomRule.ctConfig = CT_CLASSIC_;
	}
	else if (((CButton *)GetDlgItem(IDC_RADIO_CT_CLASSIC))->GetCheck() == FALSE && ((CButton *)GetDlgItem(IDC_RADIO_CT_ADDTIMES))->GetCheck() == TRUE)
	{
		m_CustomRule.ctConfig = CT_ADDTIMES_;
	}
	
	//����ģʽ
	if (((CButton *)GetDlgItem(IDC_RADIO_ST_SENDFOUR))->GetCheck() == TRUE && ((CButton *)GetDlgItem(IDC_RADIO_ST_BETFIRST))->GetCheck() == FALSE)
	{
		m_CustomRule.stConfig = ST_SENDFOUR_;
	}
	else if (((CButton *)GetDlgItem(IDC_RADIO_ST_SENDFOUR))->GetCheck() == FALSE && ((CButton *)GetDlgItem(IDC_RADIO_ST_BETFIRST))->GetCheck() == TRUE)
	{
		m_CustomRule.stConfig = ST_BETFIRST_;
	}
	
	//�˿��淨
	if (((CButton *)GetDlgItem(IDC_RADIO_HAVEKING))->GetCheck() == TRUE && ((CButton *)GetDlgItem(IDC_RADIO_NOKING))->GetCheck() == FALSE)
	{
		m_CustomRule.gtConfig = GT_HAVEKING_;
	}
	else if (((CButton *)GetDlgItem(IDC_RADIO_HAVEKING))->GetCheck() == FALSE && ((CButton *)GetDlgItem(IDC_RADIO_NOKING))->GetCheck() == TRUE)
	{
		m_CustomRule.gtConfig = GT_NOKING_;
	}

	//ׯ���淨
	if (((CButton *)GetDlgItem(IDC_RADIO_DESPOT_BANKER))->GetCheck() == TRUE && ((CButton *)GetDlgItem(IDC_RADIO_ROB_BANKER))->GetCheck() == FALSE
		&& ((CButton *)GetDlgItem(IDC_RADIO_NIUNIU_BANKER))->GetCheck() == FALSE && ((CButton *)GetDlgItem(IDC_RADIO_NONIUNIU))->GetCheck() == FALSE)
	{
		m_CustomRule.bgtConfig = BGT_DESPOT_;
	}
	else if (((CButton *)GetDlgItem(IDC_RADIO_DESPOT_BANKER))->GetCheck() == FALSE && ((CButton *)GetDlgItem(IDC_RADIO_ROB_BANKER))->GetCheck() == TRUE
		&& ((CButton *)GetDlgItem(IDC_RADIO_NIUNIU_BANKER))->GetCheck() == FALSE && ((CButton *)GetDlgItem(IDC_RADIO_NONIUNIU))->GetCheck() == FALSE)
	{
		m_CustomRule.bgtConfig = BGT_ROB_;
	}
	else if (((CButton *)GetDlgItem(IDC_RADIO_DESPOT_BANKER))->GetCheck() == FALSE && ((CButton *)GetDlgItem(IDC_RADIO_ROB_BANKER))->GetCheck() == FALSE
		&& ((CButton *)GetDlgItem(IDC_RADIO_NIUNIU_BANKER))->GetCheck() == TRUE && ((CButton *)GetDlgItem(IDC_RADIO_NONIUNIU))->GetCheck() == FALSE)
	{
		m_CustomRule.bgtConfig = BGT_NIUNIU_;
	}
	else if (((CButton *)GetDlgItem(IDC_RADIO_DESPOT_BANKER))->GetCheck() == FALSE && ((CButton *)GetDlgItem(IDC_RADIO_ROB_BANKER))->GetCheck() == FALSE
		&& ((CButton *)GetDlgItem(IDC_RADIO_NIUNIU_BANKER))->GetCheck() == FALSE && ((CButton *)GetDlgItem(IDC_RADIO_NONIUNIU))->GetCheck() == TRUE)
	{
		m_CustomRule.bgtConfig = BGT_NONIUNIU_;
	}

	//��ע����
	if (((CButton *)GetDlgItem(IDC_RADIO_BT_FREE))->GetCheck() == TRUE && ((CButton *)GetDlgItem(IDC_RADIO_BT_PERCENT))->GetCheck() == FALSE)
	{
		m_CustomRule.btConfig = BT_FREE_;

		for (WORD i=0; i<MAX_CONFIG; i++)
		{
			m_CustomRule.lFreeConfig[i] = (LONG)GetDlgItemInt(IDC_EDIT_BT_FREE_0 + i);
		}
		
		//У��ǰ����
		if (m_CustomRule.lFreeConfig[0] == 0 || m_CustomRule.lFreeConfig[1] == 0)
		{
			AfxMessageBox(TEXT("�������ö��ǰ������ӦΪ0�����������ã�"), MB_ICONSTOP);
			return false;
		}
		
		//У���Ƿ�������ظ�
		for (WORD i=0; i<MAX_CONFIG-1; i++)
		{
			if (m_CustomRule.lFreeConfig[i + 1] <=   m_CustomRule.lFreeConfig[i])
			{
				//�Ƿ���涼��0
				bool bContinueZero = true;

				for (WORD j = i + 1; j<MAX_CONFIG; j++)
				{
					if (m_CustomRule.lFreeConfig[j] != 0)
					{
						bContinueZero = false;
						break;
					}
				}

				if (!bContinueZero)
				{
					AfxMessageBox(TEXT("�������ö��Ӧ�õ�����Ψһ�����������ã�"), MB_ICONSTOP);
					return false;
				}

			}
		}
	}
	else if (((CButton *)GetDlgItem(IDC_RADIO_BT_FREE))->GetCheck() == FALSE && ((CButton *)GetDlgItem(IDC_RADIO_BT_PERCENT))->GetCheck() == TRUE)
	{
		m_CustomRule.btConfig = BT_PENCENT_;

		for (WORD i=0; i<MAX_CONFIG; i++)
		{
			m_CustomRule.lPercentConfig[i] = (LONG)GetDlgItemInt(IDC_EDIT_BT_PERCENT_0 + i);
		}

		//У��ǰ����
		if (m_CustomRule.lPercentConfig[0] == 0 || m_CustomRule.lPercentConfig[1] == 0)
		{
			AfxMessageBox(TEXT("�ٷֱ����ö��ǰ������ӦΪ0�����������ã�"), MB_ICONSTOP);
			return false;
		}

		//У���Ƿ�������ظ�
		for (WORD i=0; i<MAX_CONFIG-1; i++)
		{
			if (m_CustomRule.lPercentConfig[i + 1] <=   m_CustomRule.lPercentConfig[i])
			{
				//�Ƿ���涼��0
				bool bContinueZero = true;

				for (WORD j = i + 1; j<MAX_CONFIG; j++)
				{
					if (m_CustomRule.lPercentConfig[j] != 0)
					{
						bContinueZero = false;
						break;
					}
				}

				if (!bContinueZero)
				{
					AfxMessageBox(TEXT("�ٷֱ����ö��Ӧ�õ�����Ψһ�����������ã�"), MB_ICONSTOP);
					return false;
				}

			}
		}

		for (WORD i=0; i<MAX_CONFIG; i++)
		{
			if (m_CustomRule.lPercentConfig[i] > 100)
			{
				AfxMessageBox(TEXT("�ٷֱ����ö�Ȳ�Ӧ�ô���100�����������ã�"), MB_ICONSTOP);
				return false;
			}
		}
	}

	//����У��
	if ((m_CustomRule.lRoomStorageMax1 > m_CustomRule.lRoomStorageMax2))
	{
		AfxMessageBox(TEXT("���ⶥֵ1ӦС�ڿ��ⶥֵ2�����������ã�"),MB_ICONSTOP);
		return false;
	}

	if ((m_CustomRule.lRoomStorageMul1 > m_CustomRule.lRoomStorageMul2))
	{
		AfxMessageBox(TEXT("Ӯ�ָ���1ӦС��Ӯ�ָ���2�����������ã�"),MB_ICONSTOP);
		return false;
	}

	if ((m_CustomRule.lRobotScoreMin > m_CustomRule.lRobotScoreMax))
	{
		AfxMessageBox(TEXT("�����˷�����СֵӦС�����ֵ�����������ã�"),MB_ICONSTOP);
		return false;
	}

	return true;
}

//��ȡ����
bool CDlgCustomRule::GetCustomRule(tagCustomRule & CustomRule)
{
	//��ȡ����
	if (FillControlToData()==true)
	{
		CustomRule=m_CustomRule;
		return true;
	}

	return false;
}

//��������
bool CDlgCustomRule::SetCustomRule(tagCustomRule & CustomRule)
{
	//���ñ���
	m_CustomRule=CustomRule;

	//���²���
	if (m_hWnd!=NULL) FillDataToControl();

	return true;
}

void CDlgCustomRule::OnClickBTFree()
{
	if (m_CustomRule.lFreeConfig[0] == 0)
	{
		m_CustomRule.lFreeConfig[0] = 200;
		m_CustomRule.lFreeConfig[1] = 500;
		m_CustomRule.lFreeConfig[2] = 800;
		m_CustomRule.lFreeConfig[3] = 1100;
		m_CustomRule.lFreeConfig[4] = 1500;
	}

	ZeroMemory(m_CustomRule.lPercentConfig, sizeof(m_CustomRule.lPercentConfig));

	for (WORD i=0; i<MAX_CONFIG; i++)
	{
		((CEdit *)GetDlgItem(IDC_EDIT_BT_PERCENT_0 + i))->SetWindowText(TEXT(""));
		((CEdit *)GetDlgItem(IDC_EDIT_BT_PERCENT_0 + i))->EnableWindow(FALSE);

		((CEdit *)GetDlgItem(IDC_EDIT_BT_FREE_0 + i))->EnableWindow(TRUE);
		SetDlgItemInt(IDC_EDIT_BT_FREE_0 + i, m_CustomRule.lFreeConfig[i]);
	}
}

void CDlgCustomRule::OnClickBTPercent()
{
	if (m_CustomRule.lPercentConfig[0] == 0)
	{
		m_CustomRule.lPercentConfig[0] = 20;
		m_CustomRule.lPercentConfig[1] = 30;
		m_CustomRule.lPercentConfig[2] = 50;
		m_CustomRule.lPercentConfig[3] = 80;
		m_CustomRule.lPercentConfig[4] = 100;
	}

	ZeroMemory(m_CustomRule.lFreeConfig, sizeof(m_CustomRule.lFreeConfig));


	for (WORD i=0; i<MAX_CONFIG; i++)
	{
		((CEdit *)GetDlgItem(IDC_EDIT_BT_FREE_0 + i))->SetWindowText(TEXT(""));
		((CEdit *)GetDlgItem(IDC_EDIT_BT_FREE_0 + i))->EnableWindow(FALSE);

		((CEdit *)GetDlgItem(IDC_EDIT_BT_PERCENT_0 + i))->EnableWindow(TRUE);
		SetDlgItemInt(IDC_EDIT_BT_PERCENT_0 + i, m_CustomRule.lPercentConfig[i]);
	}
}

//////////////////////////////////////////////////////////////////////////////////
