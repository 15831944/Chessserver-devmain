#include "StdAfx.h"
#include "Resource.h"
#include "Tableframesink.h"
#include "GameServerManager.h"

//////////////////////////////////////////////////////////////////////////
//��������
#ifndef _DEBUG
#define ANDROID_SERVICE_DLL_NAME	TEXT("ZaJinHuaAndroid.dll")	//�������
#else
#define ANDROID_SERVICE_DLL_NAME	TEXT("ZaJinHuaAndroid.dll")	//�������
#endif

//////////////////////////////////////////////////////////////////////////



//���캯��
CGameServiceManager::CGameServiceManager()
{
	//��������
	m_GameServiceAttrib.wKindID=KIND_ID;
	m_GameServiceAttrib.wChairCount=GAME_PLAYER;
	m_GameServiceAttrib.wSupporType=(GAME_GENRE_GOLD|GAME_GENRE_EDUCATE|GAME_GENRE_MATCH|GAME_GENRE_PERSONAL);

	//���ܱ�־
	m_GameServiceAttrib.cbDynamicJoin=TRUE;
	m_GameServiceAttrib.cbAndroidUser=TRUE;
	m_GameServiceAttrib.cbOffLineTrustee=FALSE;

	//��������
	m_GameServiceAttrib.dwServerVersion=VERSION_SERVER;
	m_GameServiceAttrib.dwClientVersion=VERSION_CLIENT;
	lstrcpyn(m_GameServiceAttrib.szGameName,GAME_NAME,CountArray(m_GameServiceAttrib.szGameName));
	lstrcpyn(m_GameServiceAttrib.szDataBaseName,szTreasureDB,CountArray(m_GameServiceAttrib.szDataBaseName));
	lstrcpyn(m_GameServiceAttrib.szClientEXEName,TEXT("ZaJinHua.EXE"),CountArray(m_GameServiceAttrib.szClientEXEName));
	lstrcpyn(m_GameServiceAttrib.szServerDLLName,TEXT("ZaJinHuaServer.DLL"),CountArray(m_GameServiceAttrib.szServerDLLName));
	
	m_pDlgCustomRule=NULL;
	m_pDlgPersonalRule=NULL;

	return;
}

//��������
CGameServiceManager::~CGameServiceManager()
{
	//ɾ������
	SafeDelete(m_pDlgCustomRule);
}

//�ӿڲ�ѯ
void *  CGameServiceManager::QueryInterface(const IID & Guid, DWORD dwQueryVer)
{
	QUERYINTERFACE(IGameServiceManager,Guid,dwQueryVer);
	QUERYINTERFACE(IGameServiceCustomRule,Guid,dwQueryVer);
	QUERYINTERFACE(IGameServicePersonalRule,Guid,dwQueryVer);
	QUERYINTERFACE_IUNKNOWNEX(IGameServiceManager,Guid,dwQueryVer);
	return NULL;
}

//������Ϸ��
void *  CGameServiceManager::CreateTableFrameSink(const IID & Guid, DWORD dwQueryVer)
{
	//��������
	CTableFrameSink * pTableFrameSink=NULL;
	try
	{
		pTableFrameSink=new CTableFrameSink();
		if (pTableFrameSink==NULL) throw TEXT("����ʧ��");
		void * pObject=pTableFrameSink->QueryInterface(Guid,dwQueryVer);
		if (pObject==NULL) throw TEXT("�ӿڲ�ѯʧ��");
		return pObject;
	}
	catch (...) {}

	//�������
	SafeDelete(pTableFrameSink);

	return NULL;
}

//��ȡ����
bool  CGameServiceManager::GetServiceAttrib(tagGameServiceAttrib & GameServiceAttrib)
{
	GameServiceAttrib=m_GameServiceAttrib;

	return true;
}

//�����޸�
bool  CGameServiceManager::RectifyParameter(tagGameServiceOption & GameServiceOption)
{
	//Ч�����
	ASSERT(&GameServiceOption!=NULL);
	if (&GameServiceOption==NULL) return false;

	//��Ԫ����
	GameServiceOption.lCellScore=__max(1L,GameServiceOption.lCellScore);
	
	//��������
	if (GameServiceOption.wServerType & GAME_GENRE_PERSONAL)
	{
		tagZaJinHuaSpecial *pZaJinHuaSpecial = (tagZaJinHuaSpecial *)(GameServiceOption.cbPersonalRule);
		ASSERT(pZaJinHuaSpecial);

		//�ж��ǽ�ҳ�
		if (lstrcmp(GameServiceOption.szDataBaseName,  TEXT("WHJHTreasureDB")) == 0)
		{
			//��ȡ����
			LONGLONG lRoomCardTurnMaxScore = pZaJinHuaSpecial->lTurnMaxScore_Treasure;

			GameServiceOption.lMinTableScore=__max(__max(500,GameServiceOption.lMinTableScore), lRoomCardTurnMaxScore);
		}
		//�ж��ǻ��ֳ�
		else if (lstrcmp(GameServiceOption.szDataBaseName,  TEXT("WHJHGameScoreDB")) == 0)
		{
			//��ȡ����
			GameServiceOption.lMinTableScore = __max(pZaJinHuaSpecial->lTurnMaxScore_Score, GameServiceOption.lMinTableScore);
		}
	}
	else if (GameServiceOption.wServerType&(GAME_GENRE_GOLD|SCORE_GENRE_POSITIVE))
	{
		GameServiceOption.lMinTableScore=__max(1000,GameServiceOption.lMinTableScore);
	}

	//�������(ԭ���Ļ�������)
	if (GameServiceOption.lRestrictScore!=0L)
	{
		GameServiceOption.lRestrictScore=__max(GameServiceOption.lRestrictScore,GameServiceOption.lMinTableScore);
	}

	//��������
	if (GameServiceOption.wServerType==GAME_GENRE_SCORE&&GameServiceOption.wServerType!=0)
	{
		if(GameServiceOption.lMaxEnterScore<=GameServiceOption.lMinTableScore)
			GameServiceOption.lMaxEnterScore=0L;
		else if(GameServiceOption.lRestrictScore>0)
			GameServiceOption.lMaxEnterScore=__min(GameServiceOption.lMaxEnterScore,GameServiceOption.lRestrictScore);
	}

	return true;
}

//��ȡ����
bool CGameServiceManager::SaveCustomRule(LPBYTE pcbCustomRule, WORD wCustonSize)
{
	//Ч��״̬
	ASSERT(m_pDlgCustomRule!=NULL);
	if (m_pDlgCustomRule==NULL) return false;

	//��������
	ASSERT(wCustonSize>=sizeof(tagCustomRule));
	tagCustomRule * pCustomRule=(tagCustomRule *)pcbCustomRule;

	//��ȡ����
	if (m_pDlgCustomRule->GetCustomRule(*pCustomRule)==false)
	{
		return false;
	}	

	return true;
}

//Ĭ������
bool CGameServiceManager::DefaultCustomRule(LPBYTE pcbCustomRule, WORD wCustonSize)
{
	//��������
	ASSERT(wCustonSize>=sizeof(tagCustomRule));
	tagCustomRule * pCustomRule=(tagCustomRule *)pcbCustomRule;

	//���ñ���
	pCustomRule->lRoomStorageStart = 500000;
	pCustomRule->lRoomStorageDeduct = 0;
	pCustomRule->lRoomStoragePer0 = 9;
	pCustomRule->lRoomStoragePer1 = 27;
	pCustomRule->lRoomStorageMax1 = 500000;
	pCustomRule->lRoomStorageMul1 = 48;
	pCustomRule->lRoomStorageMax2 = 2000000;
	pCustomRule->lRoomStorageMul2 = 59;

	//�����˴��ȡ��
	pCustomRule->lRobotScoreMin = 100000;
	pCustomRule->lRobotScoreMax = 1000000;
	pCustomRule->lRobotBankGet = 1000000;
	pCustomRule->lRobotBankGetBanker = 10000000;
	pCustomRule->lRobotBankStoMul = 10;

	return true;
}

//��������
HWND CGameServiceManager::CreateCustomRule(CWnd * pParentWnd, CRect rcCreate, LPBYTE pcbCustomRule, WORD wCustonSize)
{
	//��������
	if (m_pDlgCustomRule==NULL)
	{
		m_pDlgCustomRule=new CDlgCustomRule;
	}

	//��������
	if (m_pDlgCustomRule->m_hWnd==NULL)
	{
		//������Դ
		AfxSetResourceHandle(GetModuleHandle(m_GameServiceAttrib.szServerDLLName));

		//��������
		m_pDlgCustomRule->Create(IDD_CUSTOM_RULE,pParentWnd);

		//��ԭ��Դ
		AfxSetResourceHandle(GetModuleHandle(NULL));
	}

	//���ñ���
	ASSERT(wCustonSize>=sizeof(tagCustomRule));
	m_pDlgCustomRule->SetCustomRule(*((tagCustomRule *)pcbCustomRule));

	//��ʾ����
	m_pDlgCustomRule->SetWindowPos(NULL,rcCreate.left,rcCreate.top,rcCreate.Width(),rcCreate.Height(),SWP_NOZORDER|SWP_SHOWWINDOW);

	return m_pDlgCustomRule->GetSafeHwnd();
}

//��ȡ����
bool CGameServiceManager::SavePersonalRule(LPBYTE pcbPersonalRule, WORD wPersonalSize)
{
	//Ч��״̬
	ASSERT(m_pDlgPersonalRule!=NULL);
	if (m_pDlgPersonalRule==NULL) return false;

	//��������
	ASSERT(wPersonalSize>=sizeof(tagZaJinHuaSpecial));
	tagZaJinHuaSpecial * pPersonalRule=(tagZaJinHuaSpecial *)pcbPersonalRule;

	//��ȡ����
	if (m_pDlgPersonalRule->GetPersonalRule(*pPersonalRule)==false)
	{
		return false;
	}	

	return true;
}

//Ĭ������
bool CGameServiceManager::DefaultPersonalRule(LPBYTE pcbPersonalRule, WORD wPersonalSize)
{
	//��������
	ASSERT(wPersonalSize>=sizeof(tagZaJinHuaSpecial));
	tagZaJinHuaSpecial * pPersonalRule=(tagZaJinHuaSpecial *)pcbPersonalRule;

	//���ñ���
	pPersonalRule->lMaxCellScore_Score = 1000;
	pPersonalRule->lTurnMaxScore_Score = 10000;
	pPersonalRule->lTurnMaxScore_Treasure = 10000;

	return true;
}

//��������
HWND CGameServiceManager::CreatePersonalRule(CWnd * pParentWnd, CRect rcCreate, LPBYTE pcbPersonalRule, WORD wPersonalSize)
{
	//��������
	if (m_pDlgPersonalRule==NULL)
	{
		m_pDlgPersonalRule=new CDlgPersonalRule;
	}

	//��������
	if (m_pDlgPersonalRule->m_hWnd==NULL)
	{
		//������Դ
		AfxSetResourceHandle(GetModuleHandle(m_GameServiceAttrib.szServerDLLName));

		//��������
		m_pDlgPersonalRule->Create(IDD_PERSONAL_RULE,pParentWnd);

		//��ԭ��Դ
		AfxSetResourceHandle(GetModuleHandle(NULL));
	}

	//���ñ���
	ASSERT(wPersonalSize>=sizeof(tagZaJinHuaSpecial));
	m_pDlgPersonalRule->SetPersonalRule(*((tagZaJinHuaSpecial *)pcbPersonalRule));

	//��ʾ����
	m_pDlgPersonalRule->SetWindowPos(NULL,rcCreate.left,rcCreate.top,rcCreate.Width(),rcCreate.Height(),SWP_NOZORDER|SWP_SHOWWINDOW);

	return m_pDlgPersonalRule->GetSafeHwnd();
}

//��������
VOID * CGameServiceManager::CreateAndroidUserItemSink(REFGUID Guid, DWORD dwQueryVer)
{
	try
	{
		//����ģ��
		if (m_hDllInstance==NULL)
		{
			m_hDllInstance=AfxLoadLibrary(ANDROID_SERVICE_DLL_NAME);
			if (m_hDllInstance==NULL) throw TEXT("�����˷���ģ�鲻����");
		}

		//Ѱ�Һ���
		ModuleCreateProc * CreateProc=(ModuleCreateProc *)GetProcAddress(m_hDllInstance,"CreateAndroidUserItemSink");
		if (CreateProc==NULL) throw TEXT("�����˷���ģ��������Ϸ�");

		//�������
		return CreateProc(Guid, dwQueryVer);
	}
	catch(...) {}

	return NULL;
}

//��������
VOID * CGameServiceManager::CreateGameDataBaseEngineSink(REFGUID Guid, DWORD dwQueryVer)
{
	return NULL;
}
//////////////////////////////////////////////////////////////////////////

//����������
extern "C" __declspec(dllexport) VOID * CreateGameServiceManager(REFGUID Guid, DWORD dwInterfaceVer)
{
	static CGameServiceManager GameServiceManager;
	return GameServiceManager.QueryInterface(Guid,dwInterfaceVer);
}

//////////////////////////////////////////////////////////////////////////
