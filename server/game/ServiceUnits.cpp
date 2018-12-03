﻿#include "StdAfx.h"
#include "ServiceUnits.h"
#include "ControlPacket.h"
#include "../utility/AsyncLog.h"

//////////////////////////////////////////////////////////////////////////////////

//静态变量
CServiceUnits *			CServiceUnits::g_pServiceUnits=NULL;			//对象指针

//////////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CServiceUnits, CWnd)
	ON_MESSAGE(WM_UICONTROL_REQUEST,OnUIControlRequest)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////////////

//构造函数
CServiceUnits::CServiceUnits()
{
	//服务参数
	m_ServiceStatus=ServiceStatus_Stop;

	//设置接口
	m_pIServiceUnitsSink=NULL;

	//设置对象
	ASSERT(g_pServiceUnits==NULL);
	if (g_pServiceUnits==NULL) g_pServiceUnits=this;

	//组件配置
	ZeroMemory(&m_GameParameter,sizeof(m_GameParameter));
	ZeroMemory(&m_DataBaseParameter,sizeof(m_DataBaseParameter));
	ZeroMemory(&m_GameServiceAttrib,sizeof(m_GameServiceAttrib));
	ZeroMemory(&m_GameServiceOption,sizeof(m_GameServiceOption));
	ZeroMemory(&m_GameMatchOption,sizeof(m_GameMatchOption));

	return;
}

//析构函数
CServiceUnits::~CServiceUnits()
{
}

//设置接口
bool CServiceUnits::SetServiceUnitsSink(IServiceUnitsSink * pIServiceUnitsSink)
{
	//设置变量
	m_pIServiceUnitsSink=pIServiceUnitsSink;

	return true;
}

//投递请求
bool CServiceUnits::PostControlRequest(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	//状态判断
	ASSERT(IsWindow(m_hWnd));
	if (IsWindow(m_hWnd)==FALSE) return false;

	//插入队列
	CWHDataLocker DataLocker(m_CriticalSection);
	if (m_DataQueue.InsertData(wIdentifier,pData,wDataSize)==false) return false;

	//发送消息
	PostMessage(WM_UICONTROL_REQUEST,wIdentifier,wDataSize);

	return true;
}

//自定义事件请求
bool CServiceUnits::PostEventCustom(WORD wRequestID, VOID * pData, WORD wDataSize)
{
	//状态效验
	ASSERT(m_AttemperEngine.GetInterface()!=NULL);
	if (m_AttemperEngine.GetInterface()==NULL) return false;

	return m_AttemperEngine->OnEventCustom(wRequestID, pData, wDataSize);
}

//启动服务
bool CServiceUnits::StartService()
{
	//效验状态
	ASSERT(m_ServiceStatus==ServiceStatus_Stop);
	if (m_ServiceStatus!=ServiceStatus_Stop) return false;

	//设置状态
	SetServiceStatus(ServiceStatus_Config);

	//创建窗口
	if (m_hWnd==NULL)
	{
		CRect rcCreate(0,0,0,0);
		Create(NULL,NULL,WS_CHILD,rcCreate,AfxGetMainWnd(),100);
	}

	//创建模块
	if (CreateServiceDLL()==false)
	{
		ConcludeService();
		return false;
	}

	//调整参数
	if (RectifyServiceParameter()==false)
	{
		ConcludeService();
		return false;
	}

	//配置服务
	if (InitializeService()==false)
	{
		ConcludeService();
		return false;
	}

	//启动内核
	if (StartKernelService()==false)
	{
		ConcludeService();
		return false;
	}		

	//加载配置
	SendControlPacket(CT_LOAD_SERVICE_CONFIG,NULL,0);

	return true;
}

//停止服务
bool CServiceUnits::ConcludeService()
{
	//开启同类型服务器数量减一
	WriteSeverCount(false);

	//设置变量
	SetServiceStatus(ServiceStatus_Stop);

	//内核组件
	if (m_TimerEngine.GetInterface()!=NULL) m_TimerEngine->ConcludeService();
	if (m_AttemperEngine.GetInterface()!=NULL) m_AttemperEngine->ConcludeService();
	if (m_TCPSocketService.GetInterface()!=NULL) m_TCPSocketService->ConcludeService();
	if (m_TCPNetworkEngine.GetInterface()!=NULL) m_TCPNetworkEngine->ConcludeService();
	if (m_PrsnlRmTCPSocketService.GetInterface()!=NULL) m_PrsnlRmTCPSocketService->ConcludeService();
	
	//数据引擎
	if (m_RecordDataBaseEngine.GetInterface()!=NULL) m_RecordDataBaseEngine->ConcludeService();
	if (m_KernelDataBaseEngine.GetInterface()!=NULL) m_KernelDataBaseEngine->ConcludeService();
	m_DBCorrespondManager.ConcludeService();

	//注销组件
	if (m_GameServiceManager.GetInterface()!=NULL) m_GameServiceManager.CloseInstance();
	if (m_MatchServiceManager.GetInterface()!=NULL)m_MatchServiceManager.CloseInstance();
	if (m_PersonalRoomServiceManager.GetInterface()!=NULL) m_PersonalRoomServiceManager.CloseInstance();

	//约战房关闭写约战数据
	if ((m_GameServiceOption.wServerType&GAME_GENRE_PERSONAL) != 0)
	{
		WritePersonalDissumeInfo();
	}
	return true;
}

//游戏配置
bool CServiceUnits::CollocateService(LPCTSTR pszGameModule, tagGameServiceOption & GameServiceOption)
{
	//效验状态
	ASSERT(m_ServiceStatus==ServiceStatus_Stop);
	if (m_ServiceStatus!=ServiceStatus_Stop) return false;

	//配置模块
	m_GameServiceOption=GameServiceOption;
	m_GameServiceManager.SetModuleCreateInfo(pszGameModule,GAME_SERVICE_CREATE_NAME);

	return true;
}

//游戏配置
bool CServiceUnits::CollocateService(tagGameMatchOption & GameMatchOption)
{
	//效验状态
	ASSERT(m_ServiceStatus==ServiceStatus_Stop);
	if (m_ServiceStatus!=ServiceStatus_Stop) return false;

	//设置变量
	m_GameMatchOption = GameMatchOption;

	return true;
}

//创建模块
bool CServiceUnits::CreateServiceDLL()
{
	//时间引擎
	if ((m_TimerEngine.GetInterface()==NULL)&&(m_TimerEngine.CreateInstance()==false))
	{
		CTraceService::TraceString(m_TimerEngine.GetErrorDescribe(),TraceLevel_Exception);
		return false;
	}

	//调度引擎
	if ((m_AttemperEngine.GetInterface()==NULL)&&(m_AttemperEngine.CreateInstance()==false))
	{
		CTraceService::TraceString(m_AttemperEngine.GetErrorDescribe(),TraceLevel_Exception);
		return false;
	}

	//网络组件
	if ((m_TCPSocketService.GetInterface()==NULL)&&(m_TCPSocketService.CreateInstance()==false))
	{
		CTraceService::TraceString(m_TCPSocketService.GetErrorDescribe(),TraceLevel_Exception);
		return false;
	}


	//网络组件
	if ((m_PrsnlRmTCPSocketService.GetInterface()==NULL)&&(m_PrsnlRmTCPSocketService.CreateInstance()==false))
	{
		CTraceService::TraceString(m_PrsnlRmTCPSocketService.GetErrorDescribe(),TraceLevel_Exception);
		return false;
	}

	//网络引擎
	if ((m_TCPNetworkEngine.GetInterface()==NULL)&&(m_TCPNetworkEngine.CreateInstance()==false))
	{
		CTraceService::TraceString(m_TCPNetworkEngine.GetErrorDescribe(),TraceLevel_Exception);
		return false;
	}
	//数据组件
	if ((m_KernelDataBaseEngine.GetInterface()==NULL)&&(m_KernelDataBaseEngine.CreateInstance()==false))
	{
		CTraceService::TraceString(m_KernelDataBaseEngine.GetErrorDescribe(),TraceLevel_Exception);
		return false;
	}

	//数据组件
	if ((m_RecordDataBaseEngine.GetInterface()==NULL)&&(m_RecordDataBaseEngine.CreateInstance()==false))
	{
		CTraceService::TraceString(m_RecordDataBaseEngine.GetErrorDescribe(),TraceLevel_Exception);
		return false;
	}

	//游戏模块
	if ((m_GameServiceManager.GetInterface()==NULL)&&(m_GameServiceManager.CreateInstance()==false))
	{
		CTraceService::TraceString(m_GameServiceManager.GetErrorDescribe(),TraceLevel_Exception);
		return false;
	}

	//比赛模块
	if ((m_GameServiceOption.wServerType&GAME_GENRE_MATCH)!=0)
	{
		if ((m_MatchServiceManager.GetInterface()==NULL)&&(m_MatchServiceManager.CreateInstance()==false))
		{
			CTraceService::TraceString(m_MatchServiceManager.GetErrorDescribe(),TraceLevel_Exception);
			return false;
		}
	}

	//约战模块
	if ((m_GameServiceOption.wServerType&GAME_GENRE_PERSONAL)!=0)
	{
		if ((m_PersonalRoomServiceManager.GetInterface()==NULL)&&(m_PersonalRoomServiceManager.CreateInstance()==false))
		{
			CTraceService::TraceString(TEXT("约战服务启动失败"),TraceLevel_Exception);
			CTraceService::TraceString(m_PersonalRoomServiceManager.GetErrorDescribe(),TraceLevel_Exception);
			return false;
		}
	}

	return true;
}

//配置组件
bool CServiceUnits::InitializeService()
{
	//设置参数
	m_InitParameter.SetServerParameter(&m_GameServiceAttrib,&m_GameServiceOption);

	//加载参数
	m_InitParameter.LoadInitParameter();

	//配置参数
	m_GameParameter.dwMedalRate=1L;
	m_GameParameter.dwRevenueRate=1L;
	m_GameParameter.dwExchangeRate=100L;

	//连接信息
	LPCTSTR pszDataBaseAddr=m_GameServiceOption.szDataBaseAddr;
	LPCTSTR pszDataBaseName=m_GameServiceOption.szDataBaseName;
	if (LoadDataBaseParameter(pszDataBaseAddr,pszDataBaseName,m_DataBaseParameter)==false) return false;

	//组件接口
	IUnknownEx * pIAttemperEngine=m_AttemperEngine.GetInterface();
	IUnknownEx * pITCPNetworkEngine=m_TCPNetworkEngine.GetInterface();
	IUnknownEx * pIAttemperEngineSink=QUERY_OBJECT_INTERFACE(m_AttemperEngineSink,IUnknownEx);

	//数据引擎
	IUnknownEx * pIDataBaseEngineRecordSink[CountArray(m_RecordDataBaseSink)];
	IUnknownEx * pIDataBaseEngineKernelSink[CountArray(m_KernelDataBaseSink)];
	for (WORD i=0;i<CountArray(pIDataBaseEngineRecordSink);i++) pIDataBaseEngineRecordSink[i]=QUERY_OBJECT_INTERFACE(m_RecordDataBaseSink[i],IUnknownEx);
	for (WORD i=0;i<CountArray(pIDataBaseEngineKernelSink);i++) pIDataBaseEngineKernelSink[i]=QUERY_OBJECT_INTERFACE(m_KernelDataBaseSink[i],IUnknownEx);

	//绑定接口
	if (m_AttemperEngine->SetAttemperEngineSink(pIAttemperEngineSink)==false) return false;
	if (m_RecordDataBaseEngine->SetDataBaseEngineSink(pIDataBaseEngineRecordSink,CountArray(pIDataBaseEngineRecordSink))==false) return false;
	if (m_KernelDataBaseEngine->SetDataBaseEngineSink(pIDataBaseEngineKernelSink,CountArray(pIDataBaseEngineKernelSink))==false) return false;

	//查询接口
	IGameServiceCustomTime * pIGameServiceSustomTime=QUERY_OBJECT_PTR_INTERFACE(m_GameServiceManager.GetInterface(),IGameServiceCustomTime);

	//时间引擎
	if (pIGameServiceSustomTime!=NULL)
	{
		//单元时间
		DWORD dwTimeCell=dwTimeCell=pIGameServiceSustomTime->GetTimerEngineTimeCell();
		if (dwTimeCell>TIME_CELL) dwTimeCell=TIME_CELL;

		//设置时间
		m_TimerEngine->SetTimeCell(dwTimeCell);
	}

	//内核组件
	if (m_TimerEngine->SetTimerEngineEvent(pIAttemperEngine)==false) return false;
	if (m_AttemperEngine->SetNetworkEngine(pITCPNetworkEngine)==false) return false;
	if (m_TCPNetworkEngine->SetTCPNetworkEngineEvent(pIAttemperEngine)==false) return false;
	
	//协调服务
	if (m_TCPSocketService->SetServiceID(NETWORK_CORRESPOND)==false) return false;
	if (m_TCPSocketService->SetTCPSocketEvent(pIAttemperEngine)==false) return false;


	//约战服务器
	if (m_PrsnlRmTCPSocketService->SetServiceID(NETWORK_PERSONAL_ROOM_CORRESPOND)==false) return false;
	if (m_PrsnlRmTCPSocketService->SetTCPSocketEvent(pIAttemperEngine)==false) return false;

	//数据协调
	m_DBCorrespondManager.InitDBCorrespondManager(m_KernelDataBaseEngine.GetInterface());

	//创建比赛
	if(m_MatchServiceManager.GetInterface()!=NULL)
	{
		if(m_MatchServiceManager->CreateGameMatch(m_GameMatchOption.cbMatchType)==false) return false;
	}

	//创建私人房
	if(m_PersonalRoomServiceManager.GetInterface()!=NULL)
	{
		if(m_PersonalRoomServiceManager->CreatePersonalRoom(0)==false) return false;
	}

	//调度回调
	m_AttemperEngineSink.m_pInitParameter=&m_InitParameter;
	m_AttemperEngineSink.m_pGameParameter=&m_GameParameter;
	m_AttemperEngineSink.m_pGameMatchOption=&m_GameMatchOption;
	m_AttemperEngineSink.m_pGameServiceAttrib=&m_GameServiceAttrib;
	m_AttemperEngineSink.m_pGameServiceOption=&m_GameServiceOption;	

	//调度回调
	m_AttemperEngineSink.m_pITimerEngine=m_TimerEngine.GetInterface();
	m_AttemperEngineSink.m_pIAttemperEngine=m_AttemperEngine.GetInterface();
	m_AttemperEngineSink.m_pITCPSocketService=m_TCPSocketService.GetInterface();
	m_AttemperEngineSink.m_pPrsnlRmITCPSocketService=m_PrsnlRmTCPSocketService.GetInterface();
	m_AttemperEngineSink.m_pITCPNetworkEngine=m_TCPNetworkEngine.GetInterface();
	m_AttemperEngineSink.m_pIGameServiceSustomTime=pIGameServiceSustomTime;
	m_AttemperEngineSink.m_pIGameServiceManager=m_GameServiceManager.GetInterface();
	m_AttemperEngineSink.m_pIRecordDataBaseEngine=m_RecordDataBaseEngine.GetInterface();
	m_AttemperEngineSink.m_pIKernelDataBaseEngine=m_KernelDataBaseEngine.GetInterface();
	m_AttemperEngineSink.m_pIMatchServiceManager=m_MatchServiceManager.GetInterface();
	m_AttemperEngineSink.m_pIPersonalRoomServiceManager=m_PersonalRoomServiceManager.GetInterface();
	
	m_AttemperEngineSink.m_pIDBCorrespondManager=(IDBCorrespondManager*)m_DBCorrespondManager.QueryInterface(IID_IDBCorrespondManager,VER_IDBCorrespondManager);

	//数据回调
	for (INT i=0;i<CountArray(m_RecordDataBaseSink);i++)
	{
		m_RecordDataBaseSink[i].m_pInitParameter=&m_InitParameter;
		m_RecordDataBaseSink[i].m_pGameParameter=&m_GameParameter;
		m_RecordDataBaseSink[i].m_pDataBaseParameter=&m_DataBaseParameter;
		m_RecordDataBaseSink[i].m_pGameServiceAttrib=&m_GameServiceAttrib;
		m_RecordDataBaseSink[i].m_pGameServiceOption=&m_GameServiceOption;
		m_RecordDataBaseSink[i].m_pIGameServiceManager=m_GameServiceManager.GetInterface();
		m_RecordDataBaseSink[i].m_pIDataBaseEngineEvent=QUERY_OBJECT_PTR_INTERFACE(pIAttemperEngine,IDataBaseEngineEvent);
	}

	//数据回调
	for (INT i=0;i<CountArray(m_KernelDataBaseSink);i++)
	{
		m_KernelDataBaseSink[i].m_pInitParameter=&m_InitParameter;
		m_KernelDataBaseSink[i].m_pGameParameter=&m_GameParameter;
		m_KernelDataBaseSink[i].m_pDataBaseParameter=&m_DataBaseParameter;
		m_KernelDataBaseSink[i].m_pGameServiceAttrib=&m_GameServiceAttrib;
		m_KernelDataBaseSink[i].m_pGameServiceOption=&m_GameServiceOption;
		m_KernelDataBaseSink[i].m_pIGameServiceManager=m_GameServiceManager.GetInterface();
		m_KernelDataBaseSink[i].m_pIDataBaseEngineEvent=QUERY_OBJECT_PTR_INTERFACE(pIAttemperEngine,IDataBaseEngineEvent);
		m_KernelDataBaseSink[i].m_pIDBCorrespondManager=(IDBCorrespondManager*)m_DBCorrespondManager.QueryInterface(IID_IDBCorrespondManager,VER_IDBCorrespondManager);
	}

	//配置网络
	m_TCPNetworkEngine->SetServiceParameter(m_GameServiceOption.wServerPort,m_GameServiceOption.wMaxPlayer,szCompilation);
	ALErr("Listen port %d", m_GameServiceOption.wServerPort);
	return true;
}

//启动内核
bool CServiceUnits::StartKernelService()
{
	//时间引擎
	if (m_TimerEngine->StartService()==false)
	{
		ASSERT(FALSE);
		return false;
	}

	//调度引擎
	if (m_AttemperEngine->StartService()==false)
	{
		ASSERT(FALSE);
		return false;
	}

	//协调引擎
	if (m_TCPSocketService->StartService()==false)
	{
		ASSERT(FALSE);
		return false;
	}

	//约战引擎
	if (m_PrsnlRmTCPSocketService->StartService()==false)
	{
		ASSERT(FALSE);
		return false;
	}

	//数据引擎
	if (m_RecordDataBaseEngine->StartService()==false)
	{
		ASSERT(FALSE);
		return false;
	}

	//数据引擎
	if (m_KernelDataBaseEngine->StartService()==false)
	{
		ASSERT(FALSE);
		return false;
	}

	//数据协调
	if (m_DBCorrespondManager.StartService()==false)
	{
		ASSERT(FALSE);
		return false;
	}

	//启动比赛
	if (m_MatchServiceManager.GetInterface()!=NULL) 
	{
		 if(m_MatchServiceManager->StartService()==false)
		 {
			 ASSERT(FALSE);
			return false;
		 }
	}

	return true;
}

//启动网络
bool CServiceUnits::StartNetworkService()
{
	//网络引擎
	if (m_TCPNetworkEngine->StartService()==false)
	{
		ASSERT(FALSE);
		return false;
	}
	
	return true;
}

//调整参数
bool CServiceUnits::RectifyServiceParameter()
{
	//读取属性
	m_GameServiceManager->GetServiceAttrib(m_GameServiceAttrib);
	if (lstrcmp(m_GameServiceAttrib.szServerDLLName,m_GameServiceManager.m_szModuleDllName)!=0)
	{
		CTraceService::TraceString(TEXT("游戏模块的绑定 DLL 名字与配置模块 DLL 名字不一致"),TraceLevel_Exception);
		return false;
	}

	//调整参数
	if (m_GameServiceManager->RectifyParameter(m_GameServiceOption)==false)
	{
		CTraceService::TraceString(TEXT("游戏模块调整配置参数失败"),TraceLevel_Exception);
		return false;
	}

	//服务类型
	if ((m_GameServiceOption.wServerType&m_GameServiceAttrib.wSupporType)==0)
	{
		CTraceService::TraceString(TEXT("游戏模块不支持配置中指定的房间模式类型"),TraceLevel_Exception);
		return false;
	}

	//占位调整
	if (m_GameServiceAttrib.wChairCount==MAX_CHAIR)
	{
		CServerRule::SetAllowAndroidSimulate(m_GameServiceOption.dwServerRule,false);
	}

	//作弊模式
	if ((m_GameServiceOption.cbDistributeRule&DISTRIBUTE_ALLOW)!=0)
	{
		//禁止规则
		CServerRule::SetForfendGameRule(m_GameServiceOption.dwServerRule,true);

		//设置作弊
		CServerRule::SetAllowAvertCheatMode(m_GameServiceOption.dwServerRule,true);		

		//最少人数
		m_GameServiceOption.wMinDistributeUser=__max(2,m_GameServiceOption.wMinDistributeUser);

		//分组间隔
		if(m_GameServiceOption.wDistributeTimeSpace==0)
		{
			m_GameServiceOption.wDistributeTimeSpace=15;
		}

		//分组局数
		if (m_GameServiceOption.wDistributeDrawCount==0)
		{
			m_GameServiceOption.wDistributeDrawCount=1;
		}

		//最少游戏人数
		if (m_GameServiceOption.wMinPartakeGameUser!=0)
		{
			m_GameServiceOption.wMinPartakeGameUser=__max(m_GameServiceOption.wMinPartakeGameUser,2);
			m_GameServiceOption.wMinPartakeGameUser=__min(m_GameServiceOption.wMinPartakeGameUser,m_GameServiceAttrib.wChairCount);
		}
		else
		{
			m_GameServiceOption.wMinPartakeGameUser=m_GameServiceAttrib.wChairCount;
		}

		//最大游戏人数
		if (m_GameServiceOption.wMaxPartakeGameUser!=0)
		{
			m_GameServiceOption.wMaxPartakeGameUser=__min(m_GameServiceOption.wMaxPartakeGameUser,m_GameServiceAttrib.wChairCount);
			m_GameServiceOption.wMaxPartakeGameUser=__max(m_GameServiceOption.wMaxPartakeGameUser,m_GameServiceOption.wMinPartakeGameUser);
		}
		else
		{
			m_GameServiceOption.wMaxPartakeGameUser=m_GameServiceAttrib.wChairCount;
		}
	}

	//游戏记录
	if (m_GameServiceOption.wServerType&(GAME_GENRE_GOLD|GAME_GENRE_MATCH))
	{
		CServerRule::SetRecordGameScore(m_GameServiceOption.dwServerRule,true);
	}

	//即时写分
	if (m_GameServiceOption.wServerType&(GAME_GENRE_GOLD|GAME_GENRE_MATCH))
	{
		CServerRule::SetImmediateWriteScore(m_GameServiceOption.dwServerRule,true);
	}

	//挂接设置
	if (m_GameServiceOption.wSortID==0) m_GameServiceOption.wSortID=500;
	if (m_GameServiceOption.wKindID==0) m_GameServiceOption.wKindID=m_GameServiceAttrib.wKindID;

	//最大人数
	WORD wMaxPlayer=m_GameServiceOption.wTableCount*m_GameServiceAttrib.wChairCount;
	m_GameServiceOption.wMaxPlayer=__max(m_GameServiceOption.wMaxPlayer,wMaxPlayer+RESERVE_USER_COUNT);

	//最小积分
	if (m_GameServiceOption.wServerType&GAME_GENRE_GOLD)
	{
		m_GameServiceOption.lMinTableScore+=m_GameServiceOption.lServiceScore;
		m_GameServiceOption.lMinTableScore=__max(m_GameServiceOption.lMinTableScore,m_GameServiceOption.lServiceScore);
	}

	//限制调整
	if (m_GameServiceOption.lMaxEnterScore!=0L)
	{
		m_GameServiceOption.lMaxEnterScore=__max(m_GameServiceOption.lMaxEnterScore,m_GameServiceOption.lMinTableScore);
	}

	//比赛配置
	if((m_GameServiceOption.wServerType&GAME_GENRE_MATCH)!=0)
	{
		if(m_GameMatchOption.dwMatchID==0)
		{
			CTraceService::TraceString(TEXT("未加载比赛配置"),TraceLevel_Exception);
			return false;
		}
	}

	return true;
}

//设置状态
bool CServiceUnits::SetServiceStatus(enServiceStatus ServiceStatus)
{
	//状态判断
	if (m_ServiceStatus!=ServiceStatus)
	{
		//错误通知
		if ((m_ServiceStatus!=ServiceStatus_Service)&&(ServiceStatus==ServiceStatus_Stop))
		{
			LPCTSTR pszString=TEXT("服务启动失败");
			CTraceService::TraceString(pszString,TraceLevel_Exception);
		}

		//设置变量
		m_ServiceStatus=ServiceStatus;

		//状态通知
		ASSERT(m_pIServiceUnitsSink!=NULL);
		if (m_pIServiceUnitsSink!=NULL) m_pIServiceUnitsSink->OnServiceUnitsStatus(m_ServiceStatus);
	}

	return true;
}

//发送控制
bool CServiceUnits::SendControlPacket(WORD wControlID, VOID * pData, WORD wDataSize)
{
	//状态效验
	ASSERT(m_AttemperEngine.GetInterface()!=NULL);
	if (m_AttemperEngine.GetInterface()==NULL) return false;

	//发送控制
	m_AttemperEngine->OnEventControl(wControlID,pData,wDataSize);

	return true;
}

//连接信息
bool CServiceUnits::LoadDataBaseParameter(LPCTSTR pszDataBaseAddr, LPCTSTR pszDataBaseName, tagDataBaseParameter & DataBaseParameter)
{
	//变量定义
	CDataBaseAide PlatformDBAide;
	CDataBaseHelper PlatformDBModule;

	//创建对象
	if ((PlatformDBModule.GetInterface()==NULL)&&(PlatformDBModule.CreateInstance()==false))
	{
		ASSERT(FALSE);
		return false;
	}

	//变量定义
	tagDataBaseParameter * pPlatformDBParameter=&m_InitParameter.m_PlatformDBParameter;

	//设置连接
	PlatformDBModule->SetConnectionInfo(pPlatformDBParameter->szDataBaseAddr,pPlatformDBParameter->wDataBasePort,
		pPlatformDBParameter->szDataBaseName,pPlatformDBParameter->szDataBaseUser,pPlatformDBParameter->szDataBasePass);

	//获取信息
	try
	{
		//发起连接
		PlatformDBModule->OpenConnection();
		PlatformDBAide.SetDataBase(PlatformDBModule.GetInterface());

		//连接信息
		PlatformDBAide.ResetParameter();
		PlatformDBAide.AddParameter(TEXT("@strDataBaseAddr"),pszDataBaseAddr);

		//执行查询
		if (PlatformDBAide.ExecuteProcess(TEXT("GSP_GS_LoadDataBaseInfo"),true)!=DB_SUCCESS)
		{
			//构造信息
			TCHAR szErrorDescribe[128]=TEXT("");
			PlatformDBAide.GetValue_String(TEXT("ErrorDescribe"),szErrorDescribe,CountArray(szErrorDescribe));

			//提示消息
			CTraceService::TraceString(szErrorDescribe,TraceLevel_Exception);

			return false;
		}

		//读取密文
		TCHAR szDBUserRead[512]=TEXT(""),szDBPassRead[512]=TEXT("");
		PlatformDBAide.GetValue_String(TEXT("DBUser"),szDBUserRead,CountArray(szDBUserRead));
		PlatformDBAide.GetValue_String(TEXT("DBPassword"),szDBPassRead,CountArray(szDBPassRead));

		//获取信息
		DataBaseParameter.wDataBasePort=PlatformDBAide.GetValue_WORD(TEXT("DBPort"));
		lstrcpyn(DataBaseParameter.szDataBaseAddr,pszDataBaseAddr,CountArray(DataBaseParameter.szDataBaseAddr));
		lstrcpyn(DataBaseParameter.szDataBaseName,pszDataBaseName,CountArray(DataBaseParameter.szDataBaseName));

		//解密密文
		TCHAR szDataBaseUser[32]=TEXT(""),szDataBasePass[32]=TEXT("");
		CWHEncrypt::XorCrevasse(szDBUserRead,DataBaseParameter.szDataBaseUser,CountArray(DataBaseParameter.szDataBaseUser));
		CWHEncrypt::XorCrevasse(szDBPassRead,DataBaseParameter.szDataBasePass,CountArray(DataBaseParameter.szDataBasePass));
	}
	catch (IDataBaseException * pIException)
	{
		//错误信息
		LPCTSTR pszDescribe=pIException->GetExceptionDescribe();
		CTraceService::TraceString(pszDescribe,TraceLevel_Exception);

		return false;
	}

	return true;
}

//控制消息
LRESULT CServiceUnits::OnUIControlRequest(WPARAM wParam, LPARAM lParam)
{
	//变量定义
	tagDataHead DataHead;
	BYTE cbBuffer[MAX_ASYNCHRONISM_DATA];

	//提取数据
	CWHDataLocker DataLocker(m_CriticalSection);
	if (m_DataQueue.DistillData(DataHead,cbBuffer,sizeof(cbBuffer))==false)
	{
		ASSERT(FALSE);
		return NULL;
	}

	//数据处理
	switch (DataHead.wIdentifier)
	{
	case UI_CORRESPOND_RESULT:		//协调成功
		{
			//效验消息
			ASSERT(DataHead.wDataSize==sizeof(CP_ControlResult));
			if (DataHead.wDataSize!=sizeof(CP_ControlResult)) return 0;

			//变量定义
			CP_ControlResult * pControlResult=(CP_ControlResult *)cbBuffer;

			//失败处理
			if ((m_ServiceStatus!=ServiceStatus_Service)&&(pControlResult->cbSuccess==ER_FAILURE))
			{
				ConcludeService();
				return 0;
			}

			//成功处理
			if ((m_ServiceStatus!=ServiceStatus_Service)&&(pControlResult->cbSuccess==ER_SUCCESS))
			{
				//设置状态
				SetServiceStatus(ServiceStatus_Service);
			}

			return 0;
		}
	case UI_PERSONAL_ROOM_CORRESPOND_RESULT:
		{
			//效验消息
			ASSERT(DataHead.wDataSize==sizeof(CP_ControlResult));
			if (DataHead.wDataSize!=sizeof(CP_ControlResult)) return 0;

			//变量定义
			CP_ControlResult * pControlResult=(CP_ControlResult *)cbBuffer;

			//失败处理
			if (pControlResult->cbSuccess==ER_FAILURE)
			{
				if (m_PrsnlRmTCPSocketService.GetInterface()!=NULL) m_PrsnlRmTCPSocketService->ConcludeService();
				return 0;
			}
			return 0;
		}
	case UI_SERVICE_CONFIG_RESULT:	//配置结果
		{
			//效验消息
			ASSERT(DataHead.wDataSize==sizeof(CP_ControlResult));
			if (DataHead.wDataSize!=sizeof(CP_ControlResult)) return 0;

			//变量定义
			CP_ControlResult * pControlResult=(CP_ControlResult *)cbBuffer;

			//失败处理
			if ((m_ServiceStatus!=ServiceStatus_Service)&&(pControlResult->cbSuccess==ER_FAILURE))
			{
				ConcludeService();
				return 0;
			}

			//成功处理
			if ((m_ServiceStatus!=ServiceStatus_Service)&&(pControlResult->cbSuccess==ER_SUCCESS))
			{
				//启动网络
				if (StartNetworkService()==false)
				{
					ConcludeService();
					return 0;
				}

				//连接协调
				SendControlPacket(CT_CONNECT_CORRESPOND,NULL,0);
				
				//如果是约战房间，连接约战服务器
				if((m_GameServiceOption.wServerType&GAME_GENRE_PERSONAL)!=0)
				{
					SendControlPacket(CT_CONNECT_PERSONAL_ROOM_CORRESPOND,NULL,0);
				}
			}

			return 0;
		}
	}

	return 0;
}

//记录启动服务器的数量
void CServiceUnits::WriteSeverCount(bool bStartOrClose)
{
	return;
	//获取路径
	TCHAR szWorkDir[MAX_PATH]=TEXT("");
	CWHService::GetWorkDirectory(szWorkDir,CountArray(szWorkDir));

	//构造路径
	TCHAR szIniFile[MAX_PATH]=TEXT("");
	_sntprintf(szIniFile,CountArray(szIniFile),TEXT("%s\\%d.ini"),szWorkDir, m_GameServiceOption.wKindID);

	//读取配置
	CWHIniData IniData;
	IniData.SetIniFilePath(szIniFile);

	//是否允许配置私人房间参数
	byte cbEnable = IniData.ReadInt(TEXT("Enable"),TEXT("Enabled"),0);

	if (bStartOrClose )
	{
		cbEnable++;
	}
	else
	{
		cbEnable--;
	}

	TCHAR szInfo[20] = {0};
	wsprintf(szInfo, TEXT("%d"), cbEnable);

	WritePrivateProfileString(TEXT("Enable"),TEXT("Enabled"), szInfo, szIniFile);

}

//写私人房间解散信息
bool	CServiceUnits::WritePersonalDissumeInfo()
{
	//变量定义
	CDataBaseAide PlatformDBAide;
	CDataBaseHelper PlatformDBModule;

	//执行逻辑
	try
	{
		//连接数据库
		if (ConnectPlatformDB(PlatformDBModule) == false)
		{
			ASSERT(FALSE);
			return false;
		}

		//设置对象
		PlatformDBAide.SetDataBase(PlatformDBModule.GetInterface());

		//插入参数
		PlatformDBAide.ResetParameter();
		PlatformDBAide.AddParameter(TEXT("@wServerID"), m_GameServiceOption.wServerID);

		//输出信息
		TCHAR szDescribeString[128] = TEXT("");
		PlatformDBAide.AddParameterOutput(TEXT("@strErrorDescribe"), szDescribeString, sizeof(szDescribeString), adParamOutput);

		//读取列表
		if (PlatformDBAide.ExecuteProcess(TEXT("GSP_GS_CloseRoomWriteDissumeTime"), true) != DB_SUCCESS)
		{
			//获取信息
			PlatformDBAide.GetParameter(TEXT("@strErrorDescribe"), szDescribeString, CountArray(szDescribeString));

			//错误提示
			CTraceService::TraceString(szDescribeString, TraceLevel_Exception);

			//错误提示
			AfxMessageBox(szDescribeString, MB_ICONERROR);

			return false;
		}
		return true;
	}
	catch (IDataBaseException * pIException)
	{
		//错误信息
		LPCTSTR pszDescribe = pIException->GetExceptionDescribe();
		CTraceService::TraceString(pszDescribe, TraceLevel_Exception);

		//错误提示
		AfxMessageBox(pszDescribe, MB_ICONERROR);

		return false;
	}

	return false;
}

bool CServiceUnits::ConnectPlatformDB(CDataBaseHelper & PlatformDBModule)
{
	//获取参数
	CModuleDBParameter * pModuleDBParameter = CModuleDBParameter::GetModuleDBParameter();
	tagDataBaseParameter * pDataBaseParameter = pModuleDBParameter->GetPlatformDBParameter();

	//创建对象
	if ((PlatformDBModule.GetInterface() == NULL) && (PlatformDBModule.CreateInstance() == false))
	{
		AfxMessageBox(TEXT("CreateGameServer 创建 PlatformDBModule 对象失败"), MB_ICONERROR);
		return false;
	}

	//设置连接
	PlatformDBModule->SetConnectionInfo(pDataBaseParameter->szDataBaseAddr, pDataBaseParameter->wDataBasePort,
		pDataBaseParameter->szDataBaseName, pDataBaseParameter->szDataBaseUser, pDataBaseParameter->szDataBasePass);

	//发起连接
	PlatformDBModule->OpenConnection();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////
