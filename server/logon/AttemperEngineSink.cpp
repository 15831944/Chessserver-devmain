﻿#include "StdAfx.h"
#include <memory>
#include "ServiceUnits.h"
#include "ControlPacket.h"
#include "AttemperEngineSink.h"
#include "../utility/AsyncLog.h"
#include "logon.pb.h"
#include "logon.rawrpc.h"

//////////////////////////////////////////////////////////////////////////////////

//时间标识
#define IDI_LOAD_GAME_LIST			1									//加载列表
#define IDI_CONNECT_CORRESPOND		2									//重连标识
#define IDI_COLLECT_ONLINE_INFO		3									//统计在线
#define IDI_CONNECT_PERSONAL_ROOM_CORRESPOND		4			//约战服务器重连标识

//时间定义
#define	TIME_VALID_VERIFY_CODE		0									//请求间隔
//////////////////////////////////////////////////////////////////////////////////

//构造函数
CAttemperEngineSink::CAttemperEngineSink()
{
	//状态变量
	m_bNeekCorrespond=true;
	m_bNeekPrsnlRmCorrespond = true;
	m_bShowServerStatus=false;

	//状态变量
	m_pInitParameter=NULL;
	m_pBindParameter=NULL;

	//组件变量
	m_pITimerEngine=NULL;
	m_pIDataBaseEngine=NULL;
	m_pITCPNetworkEngine=NULL;
	m_pITCPSocketService=NULL;
	m_pPrsnlRmITCPSocketService=NULL;
	
	//视频配置
	m_wAVServerPort=0;
	m_dwAVServerAddr=0;

	//缓存变量
	m_wTaskCount=0;
	m_wLevelCount=0;
	m_wMemberCount=0;	
	m_wTaskCountBuffer=0;

	ZeroMemory(&m_PlatformParameter,sizeof(&m_PlatformParameter));
	ZeroMemory(&m_BaseEnsureParameter,sizeof(&m_BaseEnsureParameter));	
	ZeroMemory(&m_AuthRealParameter,sizeof(&m_AuthRealParameter));
	ZeroMemory(m_lCheckInReward,sizeof(m_lCheckInReward));
	ZeroMemory(m_TaskParameter,sizeof(m_TaskParameter));	
	ZeroMemory(m_MemberParameter,sizeof(m_MemberParameter));
	ZeroMemory(m_GrowLevelConfig,sizeof(m_GrowLevelConfig));
	ZeroMemory(m_TaskParameterBuffer,sizeof(m_TaskParameterBuffer));	
	//清理数组
	m_VerifyCodeArray.RemoveAll();

	m_bHasPrsnlRM = false;

	HINSTANCE hInstLibrary = NULL;
	#ifdef _DEBUG
		hInstLibrary = LoadLibrary(TEXT("PersonalRoomServiceD.dll"));
	#else
		hInstLibrary = LoadLibrary(TEXT("PersonalRoomService.dll"));
	#endif

	if (hInstLibrary == NULL)
	{
		FreeLibrary(hInstLibrary); 
	}
	else
	{
		m_bHasPrsnlRM = true;
		FreeLibrary(hInstLibrary); 
	}


	return;
}

//析构函数
CAttemperEngineSink::~CAttemperEngineSink()
{
	//清理资源
	for (INT i=0; i<m_VerifyCodeArray.GetCount();i++)
	{
		tagVerifyCode *pValidateCode=m_VerifyCodeArray[i];
		if (pValidateCode!=NULL)
		{
			SafeDelete(pValidateCode);
		}
		m_VerifyCodeArray.RemoveAt(i);
	}
}

//接口查询
VOID * CAttemperEngineSink::QueryInterface(REFGUID Guid, DWORD dwQueryVer)
{
	QUERYINTERFACE(IAttemperEngineSink,Guid,dwQueryVer);
	QUERYINTERFACE_IUNKNOWNEX(IAttemperEngineSink,Guid,dwQueryVer);
	return NULL;
}

//启动事件
bool CAttemperEngineSink::OnAttemperEngineStart(IUnknownEx * pIUnknownEx)
{
	//绑定参数
	m_pBindParameter=new tagBindParameter[m_pInitParameter->m_wMaxConnect];
	ZeroMemory(m_pBindParameter,sizeof(tagBindParameter)*m_pInitParameter->m_wMaxConnect);

	//设置时间
	ASSERT(m_pITimerEngine!=NULL);
#ifndef _DEBUG
	m_pITimerEngine->SetTimer(IDI_COLLECT_ONLINE_INFO,m_pInitParameter->m_wCollectTime*1000L,TIMES_INFINITY,0);
#else
	m_pITimerEngine->SetTimer(IDI_COLLECT_ONLINE_INFO,20*1000L,TIMES_INFINITY,0);
#endif

	//获取目录
	TCHAR szPath[MAX_PATH]=TEXT("");
	CString szFileName;
	GetModuleFileName(AfxGetInstanceHandle(),szPath,sizeof(szPath));
	szFileName=szPath;
	int nIndex = szFileName.ReverseFind(TEXT('\\'));
	szFileName = szFileName.Left(nIndex);
	szFileName += TEXT("\\PlazaOptionConfig.ini");

	//读取配置
	m_bShowServerStatus = (GetPrivateProfileInt(TEXT("ServerStatus"),TEXT("ShowServerStatus"),0,szFileName) != 0);

	//获取目录
	TCHAR szServerAddr[LEN_SERVER]=TEXT("");
	GetCurrentDirectory(sizeof(szPath),szPath);

	//读取配置
	TCHAR szVideoFileName[MAX_PATH]=TEXT("");
	_sntprintf_s(szVideoFileName,CountArray(szVideoFileName),TEXT("%s\\VideoOption.ini"),szPath);
	m_wAVServerPort=GetPrivateProfileInt(TEXT("VideoOption"),TEXT("ServerPort"), 0,szVideoFileName);
	DWORD dwAddrLen=GetPrivateProfileString(TEXT("VideoOption"),TEXT("ServerAddr"), TEXT(""), szServerAddr,LEN_SERVER,szVideoFileName);
	if(dwAddrLen>0)
	{
		CT2CA strServerDomain(szServerAddr);
		m_dwAVServerAddr=inet_addr(strServerDomain);
	}
	else
	{
		m_dwAVServerAddr=0;
	}

	return true;
}

//停止事件
bool CAttemperEngineSink::OnAttemperEngineConclude(IUnknownEx * pIUnknownEx)
{
	//状态变量
	m_bNeekCorrespond=true;
	m_bNeekPrsnlRmCorrespond=true;

	//组件变量
	m_pITimerEngine=NULL;
	m_pIDataBaseEngine=NULL;
	m_pITCPNetworkEngine=NULL;
	m_pITCPSocketService=NULL;
	m_pPrsnlRmITCPSocketService=NULL;
	
	//任务参数
	m_wTaskCount=0;
	m_wTaskCountBuffer=0;
	ZeroMemory(m_TaskParameter,sizeof(m_TaskParameter));
	ZeroMemory(m_TaskParameterBuffer,sizeof(m_TaskParameterBuffer));

	//会员参数
	m_wMemberCount=0;
	ZeroMemory(m_MemberParameter,sizeof(m_MemberParameter));

	//等级配置
	m_wLevelCount=0;	
	ZeroMemory(m_GrowLevelConfig,sizeof(m_GrowLevelConfig));

	//签到配置
	ZeroMemory(m_lCheckInReward,sizeof(m_lCheckInReward));	



	//删除数据
	SafeDeleteArray(m_pBindParameter);

	//列表组件
	m_ServerListManager.ResetServerList();

	//道具组件
	m_GamePropertyListManager.ResetPropertyListManager();

	return true;
}

//控制事件
bool CAttemperEngineSink::OnEventControl(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	switch (wIdentifier)
	{
	case CT_LOAD_DB_GAME_LIST:		//加载列表
		{
			//加载列表
			m_ServerListManager.DisuseKernelItem();
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOAD_GAME_LIST,0,NULL,0);

			//平台参数
			//m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOAD_PLATFORM_PARAMETER,0,NULL,0);			

			//加载签到
			//m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOAD_CHECKIN_REWARD,0,NULL,0);

			//加载任务
			//m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOAD_TASK_LIST,0,NULL,0);

			//加载低保
			//m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOAD_BASEENSURE,0,NULL,0);

			//会员参数
			//m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOAD_MEMBER_PARAMETER,0,NULL,0);		

			//成长配置
			//m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOAD_GROWLEVEL_CONFIG,0,NULL,0);	
			
			//道具配置
			m_GamePropertyListManager.DisusePropertyTypeItem();
			m_GamePropertyListManager.DisusePropertyItem();
			//m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOAD_GAME_PROPERTY_LIST,0,NULL,0);

			//加载认证
			//m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOAD_REAL_AUTH,0,NULL,0);
			return true;
		}
	case CT_CONNECT_CORRESPOND:		//连接协调
		{
			//发起连接
			tagAddressInfo * pCorrespondAddress=&m_pInitParameter->m_CorrespondAddress;
			m_pITCPSocketService->Connect(pCorrespondAddress->szAddress,m_pInitParameter->m_wCorrespondPort);

			//构造提示
			TCHAR szString[512]=TEXT("");
			_sntprintf(szString,CountArray(szString),TEXT("正在连接协调服务器 [ %s:%d ]"),pCorrespondAddress->szAddress,m_pInitParameter->m_wCorrespondPort);

			//提示消息
			CTraceService::TraceString(szString,TraceLevel_Normal);

			return true;
		}
	case CT_CONNECT_PERSONAL_ROOM_CORRESPOND:		//连接约战
		{
			if (!m_bHasPrsnlRM)
			{
				return true;
			}
			//发起连接
			tagAddressInfo * pPrsnlRmCorrespondAddress=&m_pInitParameter->m_PrsnlRmCorrespondAddress;
			//发送数据
			if (m_pPrsnlRmITCPSocketService)
			{
				m_pPrsnlRmITCPSocketService->Connect(pPrsnlRmCorrespondAddress->szAddress,m_pInitParameter->m_wPrsnlRmCorrespondPort);
			}
			
			//构造提示
			TCHAR szString[512]=TEXT("");
			_sntprintf(szString,CountArray(szString),TEXT("正在连接约战服务器 [ %s:%d ]"),pPrsnlRmCorrespondAddress->szAddress,m_pInitParameter->m_wCorrespondPort);

			//提示消息
			CTraceService::TraceString(szString,TraceLevel_Normal);

			return true;
		}
	}

	return false;
}

//调度事件
bool CAttemperEngineSink::OnEventAttemperData(WORD wRequestID, VOID * pData, WORD wDataSize)
{
	return false;
}

//时间事件
bool CAttemperEngineSink::OnEventTimer(DWORD dwTimerID, WPARAM wBindParam)
{
	switch (dwTimerID)
	{
	case IDI_LOAD_GAME_LIST:		//加载列表
		{
			//加载列表
			m_ServerListManager.DisuseKernelItem();
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOAD_GAME_LIST,0,NULL,0);

			return true;
		}
	case IDI_CONNECT_CORRESPOND:	//连接协调
		{
			//发起连接
			tagAddressInfo * pCorrespondAddress=&m_pInitParameter->m_CorrespondAddress;
			m_pITCPSocketService->Connect(pCorrespondAddress->szAddress,m_pInitParameter->m_wCorrespondPort);

			//构造提示
			TCHAR szString[512]=TEXT("");
			_sntprintf_s(szString,CountArray(szString),TEXT("正在连接协调服务器 [ %s:%d ]"),pCorrespondAddress->szAddress,m_pInitParameter->m_wCorrespondPort);

			//提示消息
			CTraceService::TraceString(szString,TraceLevel_Normal);

			return true;
		}
	case IDI_CONNECT_PERSONAL_ROOM_CORRESPOND:// 约战服务器重连
		{
			if (!m_bHasPrsnlRM)
			{
				return true;
			}

			//发起连接
			tagAddressInfo * pPrsnlRmCorrespondAddress=&m_pInitParameter->m_PrsnlRmCorrespondAddress;
			if (m_pPrsnlRmITCPSocketService)
			{
				m_pPrsnlRmITCPSocketService->Connect(pPrsnlRmCorrespondAddress->szAddress,m_pInitParameter->m_wPrsnlRmCorrespondPort);
			}
			
			//构造提示
			TCHAR szString[512]=TEXT("");
			_sntprintf(szString,CountArray(szString),TEXT("正在连接约战服务器 [ %s:%d ]"),pPrsnlRmCorrespondAddress->szAddress,m_pInitParameter->m_wCorrespondPort);

			//提示消息
			CTraceService::TraceString(szString,TraceLevel_Normal);

			return true;
		}
	case IDI_COLLECT_ONLINE_INFO:	//统计在线
		{
			//变量定义
			DBR_GP_OnLineCountInfo OnLineCountInfo;
			ZeroMemory(&OnLineCountInfo,sizeof(OnLineCountInfo));

			//获取总数
			OnLineCountInfo.dwOnLineCountSum=m_ServerListManager.CollectOnlineInfo(false);
			OnLineCountInfo.dwAndroidCountSum=m_ServerListManager.CollectOnlineInfo(true);
			OnLineCountInfo.dwSetCountSum = m_ServerListManager.CollectSetPlayer(INVALID_WORD);
			//获取类型
			POSITION KindPosition=NULL;
			do
			{
				//获取类型
				CGameKindItem * pGameKindItem=m_ServerListManager.EmunGameKindItem(KindPosition);

				//设置变量
				if (pGameKindItem!=NULL)
				{
					WORD wKindIndex=OnLineCountInfo.wKindCount++;
					OnLineCountInfo.OnLineCountKind[wKindIndex].wKindID=pGameKindItem->m_GameKind.wKindID;

					//目录人数
					OnLineCountInfo.OnLineCountKind[wKindIndex].dwOnLineCount=pGameKindItem->m_GameKind.dwOnLineCount;
					OnLineCountInfo.OnLineCountKind[wKindIndex].dwAndroidCount=pGameKindItem->m_GameKind.dwAndroidCount;
					OnLineCountInfo.OnLineCountKind[wKindIndex].dwSetCount = pGameKindItem->m_GameKind.dwSetCount;
				}

				//溢出判断
				if (OnLineCountInfo.wKindCount>=CountArray(OnLineCountInfo.OnLineCountKind))
				{
					ASSERT(FALSE);
					break;
				}

			} while (KindPosition!=NULL);

			//发送请求
			WORD wHeadSize=sizeof(OnLineCountInfo)-sizeof(OnLineCountInfo.OnLineCountKind);
			WORD wSendSize=wHeadSize+OnLineCountInfo.wKindCount*sizeof(OnLineCountInfo.OnLineCountKind[0]);
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_ONLINE_COUNT_INFO,0,&OnLineCountInfo,wSendSize);

			return true;
		}
	}

	return false;
}

//应答事件
bool CAttemperEngineSink::OnEventTCPNetworkBind(DWORD dwClientAddr, DWORD dwSocketID)
{
	//获取索引
	ASSERT(LOWORD(dwSocketID)<m_pInitParameter->m_wMaxConnect);
	if (LOWORD(dwSocketID)>=m_pInitParameter->m_wMaxConnect) return false;

	//变量定义
	WORD wBindIndex=LOWORD(dwSocketID);
	tagBindParameter * pBindParameter=(m_pBindParameter+wBindIndex);

	//设置变量
	pBindParameter->dwSocketID=dwSocketID;
	pBindParameter->dwClientAddr=dwClientAddr;
	pBindParameter->dwActiveTime=(DWORD)time(NULL);
	ALStd("Client socket in %d-%d", wBindIndex, dwSocketID);
	return true;
}

//关闭事件
bool CAttemperEngineSink::OnEventTCPNetworkShut(DWORD dwClientAddr, DWORD dwActiveTime, DWORD dwSocketID)
{
	//清除信息
	WORD wBindIndex=LOWORD(dwSocketID);
	ZeroMemory((m_pBindParameter+wBindIndex),sizeof(tagBindParameter));
	ALStd("Client socket out %d-%d", wBindIndex, dwSocketID);
	return false;
}

//读取事件
bool CAttemperEngineSink::OnEventTCPNetworkRead(TCP_Command Command, VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	ALStd("TCPRead cmd %d-%d", Command.wMainCmdID, Command.wSubCmdID);
	switch (Command.wMainCmdID)
	{
	case MDM_GP_LOGON:			//登录命令
		{
			return OnTCPNetworkMainPCLogon(Command.wSubCmdID,pData,wDataSize,dwSocketID);
		}
	case MDM_GP_SERVER_LIST:	//列表命令
		{
			return OnTCPNetworkMainPCServerList(Command.wSubCmdID,pData,wDataSize,dwSocketID);
		}
	case MDM_GP_USER_SERVICE:	//服务命令
		{
			return OnTCPNetworkMainPCUserService(Command.wSubCmdID,pData,wDataSize,dwSocketID);
		}
	case MDM_GP_REMOTE_SERVICE:	//远程服务
		{
			return OnTCPNetworkMainPCRemoteService(Command.wSubCmdID,pData,wDataSize,dwSocketID);
		}
	case MDM_GP_ANDROID_SERVICE: //机器服务
		{
			return OnTCPNetworkMainAndroidService(Command.wSubCmdID,pData,wDataSize,dwSocketID);
		}
	case MDM_GP_PROPERTY:		//道具服务
		{
			return OnTCPNetworkMainPCProperty(Command.wSubCmdID,pData,wDataSize,dwSocketID);
		}
	case MDM_MB_LOGON:			//登录命令
		{
			return OnTCPNetworkMainMBLogon(Command.wSubCmdID,pData,wDataSize,dwSocketID);
		}
	case MDM_MB_SERVER_LIST:	//列表命令
		{
			return OnTCPNetworkMainMBServerList(Command.wSubCmdID,pData,wDataSize,dwSocketID);
		}
	case MDM_MB_PERSONAL_SERVICE:	//约战房间
		{
			return OnTCPNetworkMainMBPersonalService(Command.wSubCmdID, pData, wDataSize,dwSocketID);
		}
	case MDM_MB_VIDEO_PLAY_BACK_INFO:
		{
			return OnTCPNetworkMBVideoPlayBackInfo(Command.wSubCmdID,pData,wDataSize,dwSocketID);
		}
	}

	return false;
}

//数据库事件
bool CAttemperEngineSink::OnEventDataBase(WORD wRequestID, DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	switch (wRequestID)
	{
	case DBO_GP_LOGON_SUCCESS:			//登录成功
		{
			return OnDBPCLogonSuccess(dwContextID,pData,wDataSize);
		}
	case DBO_GP_LOGON_FAILURE:			//登录失败
		{
			return OnDBPCLogonFailure(dwContextID,pData,wDataSize);
		}
	case DBO_GP_VALIDATE_MBCARD:		//校验密保
		{
			return OnDBPCLogonValidateMBCard(dwContextID,pData,wDataSize);
		}
	case DBO_GP_VALIDATE_PASSPORT:		//校验证件
		{
			return OnDBPCLogonValidatePassPort(dwContextID,pData,wDataSize);
		}
	case DBO_GP_VERIFY_RESULT:			//验证结果
		{
			return OnDBPCLogonVerifyResult(dwContextID,pData,wDataSize);
		}
	case DBO_GP_PLATFORM_PARAMETER:		//平台参数
		{
			return OnDBPCPlatformParameter(dwContextID,pData,wDataSize);
		}
	case DBO_GP_USER_FACE_INFO:			//用户头像
		{
			return OnDBPCUserFaceInfo(dwContextID,pData,wDataSize);
		}
	case DBO_GP_USER_INDIVIDUAL:		//用户信息
		{
			return OnDBPCUserIndividual(dwContextID,pData,wDataSize);
		}
	case DBO_GP_BIND_SPREADER_RESULT:  //绑定推广
		{
			return OnDBUserBindSpreaderResult(dwContextID, pData, wDataSize);
		}
	case DBO_GP_USER_INSURE_INFO:		//银行资料
		{
			return OnDBPCUserInsureInfo(dwContextID,pData,wDataSize);
		}
	case DBO_GP_USER_INSURE_SUCCESS:	//银行成功
		{
			return OnDBPCUserInsureSuccess(dwContextID,pData,wDataSize);
		}
	case DBO_GP_USER_INSURE_FAILURE:	//银行失败
		{
			return OnDBPCUserInsureFailure(dwContextID,pData,wDataSize);
		}
	case DBO_GP_USER_INSURE_USER_INFO:  //用户信息
		{
			return OnDBPCUserInsureUserInfo(dwContextID,pData,wDataSize);
		}
	case DBO_GP_USER_INSURE_ENABLE_RESULT: //开通结果
		{
			return OnDBPCUserInsureEnableResult(dwContextID,pData,wDataSize);
		}
	case DBO_GP_QUERY_TRANSFER_REBATE_RESULT: //返利结果
		{
			return OnDBPCQueryTransferRebateResult(dwContextID,pData,wDataSize);
		}
	case DBO_GP_ANDROID_PARAMETER:		//机器参数
		{
			return OnDBAndroidParameter(dwContextID,pData,wDataSize);
		}
	case DBO_GP_INDIVIDUAL_RESULT:
		{
			return OnDBIndividualResult(dwContextID,pData,wDataSize);
		}
	case DBO_GP_OPERATE_SUCCESS:		//操作成功
		{
			return OnDBPCOperateSuccess(dwContextID,pData,wDataSize);
		}
	case DBO_GP_OPERATE_FAILURE:		//操作失败
		{
			return OnDBPCOperateFailure(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GAME_TYPE_ITEM:			//类型子项
		{
			return OnDBPCGameTypeItem(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GAME_KIND_ITEM:			//类型子项
		{
			return OnDBPCGameKindItem(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GAME_NODE_ITEM:			//节点子项
		{
			return OnDBPCGameNodeItem(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GAME_PAGE_ITEM:			//定制子项
		{
			return OnDBPCGamePageItem(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GAME_LIST_RESULT:		//加载结果
		{
			return OnDBPCGameListResult(dwContextID,pData,wDataSize);
		}
	case DBO_GP_CHECKIN_INFO:			//签到信息
		{
			return OnDBPCUserCheckInInfo(dwContextID,pData,wDataSize);
		}
	case DBO_GP_CHECKIN_RESULT:			//签到结果
		{
			return OnDBPCUserCheckInResult(dwContextID,pData,wDataSize);
		}
	case DBO_GP_CHECKIN_REWARD:			//签到奖励
		{
			return OnDBPCCheckInReward(dwContextID,pData,wDataSize);
		}
	case DBO_GP_TASK_LIST:				//任务列表
		{
			return OnDBPCTaskList(dwContextID,pData,wDataSize);
		}
	case DBO_GP_TASK_LIST_END:			//任务结束
		{
			return OnDBPCTaskListEnd(dwContextID,pData,wDataSize);
		}
	case DBO_GP_TASK_INFO:				//任务信息
		{
			return OnDBPCTaskInfo(dwContextID,pData,wDataSize);
		}
	case DBO_GP_TASK_RESULT:			//任务结果
		{
			return OnDBPCTaskResult(dwContextID,pData,wDataSize);
		}
	case DBO_MB_VIDEO_LIST:
		{
			return OnDBMBVideoList(dwContextID,pData,wDataSize);
		}
	case DBO_MB_VIDEO_LIST_END:
		{
			return OnDBMBVideoListEnd(dwContextID,pData,wDataSize);
		}
	case DBO_MB_VIDEO_DETAILS:
		{
			return OnDBMBVideoDetails(dwContextID,pData,wDataSize);
		}
	case DBO_MB_VIDEO_DETAILS_END:
		{
			return OnDBMBVideoDetailsEnd(dwContextID,pData,wDataSize);
		}
	case DBO_MB_PLAY_BACK_CODE_YZ_RESULT:
		{
			return OnDBMBPlayBackCodeYZ(dwContextID,pData,wDataSize);
		}
	case DBO_MB_PLAY_BACK_CODE_RESULT:
		{
			return OnDBMBPlayBackCode(dwContextID,pData,wDataSize);
		}
	case DBO_GP_BASEENSURE_PARAMETER:	//低保参数
		{
			return OnDBPCBaseEnsureParameter(dwContextID,pData,wDataSize);
		}
	case DBO_GP_BASEENSURE_RESULT:		//低保结果
		{
			return OnDBPCBaseEnsureResult(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GROWLEVEL_CONFIG:		//等级配置
		{
			return OnDBPCGrowLevelConfig(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GROWLEVEL_PARAMETER:	//等级参数
		{
			return OnDBPCGrowLevelParameter(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GROWLEVEL_UPGRADE:		//等级升级
		{
			return OnDBPCGrowLevelUpgrade(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GAME_PROPERTY_TYPE_ITEM:  //道具类型 
		{
			return OnDBGamePropertyTypeItem(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GAME_PROPERTY_RELAT_ITEM: //道具关系
		{
			return OnDBGamePropertyRelatItem(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GAME_PROPERTY_ITEM:		 //道具节点
		{
			return OnDBGamePropertyItem(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GAME_PROPERTY_SUB_ITEM:		 //道具节点
		{
			return OnDBGamePropertySubItem(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GAME_PROPERTY_LIST_RESULT:		//道具结果
		{
			return OnDBGamePropertyListResult(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GAME_PROPERTY_BUY:
		{
			return OnDBGamePropertyBuy(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GAME_PROPERTY_FAILURE:
		{
			return OnDBGamePropertyFailure(dwContextID,pData,wDataSize);
		}
	case DBO_GP_QUERY_BACKPACK:			//背包查询
		{
			return OnDBQueryUserBackpack(dwContextID,pData,wDataSize);
		}
	case DBO_GP_PROPERTY_QUERY_SINGLE:			//背包查询
		{
			return OnDBQueryPropertySingle(dwContextID,pData,wDataSize);
		}
	case DBO_GP_USER_PROPERTY_BUFF:		//道具Buff
		{
			return OnDBUserPropertyBuff(dwContextID,pData,wDataSize);
		}
	case DBO_GP_PROPERTY_PRESENT:		//赠送道具
		{
			return OnDBUserPropertyPresent(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GAME_PROPERTY_USE:		//道具使用
		{
			return OnDBGamePropertyUse(dwContextID,pData,wDataSize);
		}
	case DBO_GP_QUERY_SEND_PRESENT:		//查询赠送
		{
			return OnDBQuerySendPresent(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GET_SEND_PRESENT:		//获取赠送
		{
			return OnDBGetSendPresent(dwContextID,pData,wDataSize);
		}
	case DBO_GP_MEMBER_PARAMETER:		//会员参数
		{
			return OnDBPCMemberParameter(dwContextID,pData,wDataSize);
		}
	case DBO_GP_MEMBER_QUERY_INFO_RESULT://会员查询
		{
			return OnDBPCMemberDayQueryInfoResult(dwContextID,pData,wDataSize);
		}
	case DBO_GP_MEMBER_DAY_PRESENT_RESULT://会员送金
		{
			return OnDBPCMemberDayPresentResult(dwContextID,pData,wDataSize);
		}
	case DBO_GP_MEMBER_DAY_GIFT_RESULT://会员礼包
		{
			return OnDBPCMemberDayGiftResult(dwContextID,pData,wDataSize);
		}
	case DBO_GP_PURCHASE_RESULT:		//购买结果
		{
			return OnDBPCPurchaseResult(dwContextID,pData,wDataSize);
		}
	case DBO_GP_EXCHANGE_RESULT:		//兑换结果
		{
			return OnDBPCExChangeResult(dwContextID,pData,wDataSize);
		}
	case DBO_GP_ROOM_CARD_EXCHANGE_RESULT:		//兑换结果
		{
			return OnDBPCExChangeRoomCardToScoreResult(dwContextID,pData,wDataSize);
		}
	case DBO_GP_SPREAD_INFO:			//推广信息
		{
			return OnDBPCUserSpreadInfo(dwContextID,pData,wDataSize);
		}
	case DBO_GP_REAL_AUTH_PARAMETER:				//实名信息
		{
			return OnDBPCRealAuthParameter(dwContextID,pData,wDataSize);
		}
	case DBO_MB_LOGON_SUCCESS:			//登录成功
		{
			return OnDBMBLogonSuccess(dwContextID,pData,wDataSize);
		}
	case DBO_MB_LOGON_FAILURE:			//登录失败
		{
			return OnDBMBLogonFailure(dwContextID,pData,wDataSize);
		}
	case DBO_GP_LOTTERY_CONFIG:			//抽奖配置
		{
			return OnDBPCLotteryConfig(dwContextID,pData,wDataSize);
		}
	case DBO_GP_LOTTERY_USER_INFO:		//抽奖信息
		{
			return OnDBPCLotteryUserInfo(dwContextID,pData,wDataSize);
		}
	case DBO_GP_LOTTERY_RESULT:			//抽奖结果
		{
			return OnDBPCLotteryResult(dwContextID,pData,wDataSize);
		}
	case DBO_GP_QUERY_USER_GAME_DATA:	//游戏数据
		{
			return OnDBPCQueryUserGameData(dwContextID,pData,wDataSize);
		}
	case DBO_GP_AGENT_GAME_KIND_ITEM:
		{
			return OnDBPCAgentGameList(dwContextID,pData,wDataSize);
		}
	case DBO_MB_AGENT_GAME_KIND_ITEM:
		{
			return OnDBMBAgentGameList(dwContextID,pData,wDataSize);
		}
	case DBO_MB_PERSONAL_PARAMETER: //约战房间参数
		{
			return OnDBMBPersonalParameter(dwContextID,pData,wDataSize);
		}
	case DBO_MB_PERSONAL_FEE_LIST: //约战房间费用列表
		{
			return OnDBMBPersonalFeeList(dwContextID,pData,wDataSize);
		}
	case DBO_MB_PERSONAL_CELL_SCORE_LIST: //私人房间费用列表
		{
			return OnDBMBPersonalCellScore(dwContextID, pData, wDataSize);
		}
	case DBO_MB_PERSONAL_ROOM_LIST://约战房间列表
		{
			return OnDBMBPersonalRoomListInfo(dwContextID,pData,wDataSize);
		}
	case DBO_MB_GET_PERSONAL_RULE:		//私人房间自定义配置
		{
			return OnDBMBPersonalRule(dwContextID, pData, wDataSize);
		}
	case DBO_GR_QUERY_USER_ROOM_SCORE://约战房间结算信息
		{
			return OnDBQueryUserRoomScore(dwContextID, pData, wDataSize);
		}
	case DBO_GR_QUERY_PERSONAL_ROOM_USER_INFO_RESULT://约战房间结算信息
		{
			return OnDBQueryPersonalRoomUersInfoResult(dwContextID, pData, wDataSize);
		}
	case DBO_MB_GET_PARAMETER_END:					//私人获取结束
		{
			return OnDBQueryPersonalRoomEnd(dwContextID, pData, wDataSize);
		}
	}


	return false;
}

//关闭事件
bool CAttemperEngineSink::OnEventTCPSocketShut(WORD wServiceID, BYTE cbShutReason)
{
	//协调连接
	if (wServiceID==NETWORK_CORRESPOND)
	{
		//重连判断
		if (m_bNeekCorrespond==true)
		{
			//构造提示
			TCHAR szDescribe[128]=TEXT("");
			_sntprintf_s(szDescribe,CountArray(szDescribe),TEXT("与协调服务器的连接关闭了，%ld 秒后将重新连接"),m_pInitParameter->m_wConnectTime);

			//提示消息
			CTraceService::TraceString(szDescribe,TraceLevel_Warning);

			//设置时间
			ASSERT(m_pITimerEngine!=NULL);
			m_pITimerEngine->SetTimer(IDI_CONNECT_CORRESPOND,m_pInitParameter->m_wConnectTime*1000L,1,0);

			return true;
		}
	}
	//约战连接
	else if (wServiceID==NETWORK_PERSONAL_ROOM_CORRESPOND)
	{
		//重连判断
		if (m_bNeekPrsnlRmCorrespond==true)
		{
			//构造提示
			TCHAR szDescribe[128]=TEXT("");
			_sntprintf(szDescribe,CountArray(szDescribe),TEXT("与约战服务器的连接关闭了，%ld 秒后将重新连接"),m_pInitParameter->m_wConnectTime);

			//提示消息
			CTraceService::TraceString(szDescribe,TraceLevel_Warning);

			//设置时间
			ASSERT(m_pITimerEngine!=NULL);
			m_pITimerEngine->SetTimer(IDI_CONNECT_PERSONAL_ROOM_CORRESPOND,m_pInitParameter->m_wConnectTime*1000L,1,0);

			return true;
		}
	}

	return false;
}

//连接事件
bool CAttemperEngineSink::OnEventTCPSocketLink(WORD wServiceID, INT nErrorCode)
{
	//协调连接
	if (wServiceID==NETWORK_CORRESPOND)
	{
		//错误判断
		if (nErrorCode!=0)
		{
			//构造提示
			TCHAR szDescribe[128]=TEXT("");
			_sntprintf_s(szDescribe,CountArray(szDescribe),TEXT("协调服务器连接失败 [ %ld ]，%ld 秒后将重新连接"),
				nErrorCode,m_pInitParameter->m_wConnectTime);

			//提示消息
			CTraceService::TraceString(szDescribe,TraceLevel_Warning);

			//设置时间
			ASSERT(m_pITimerEngine!=NULL);
			m_pITimerEngine->SetTimer(IDI_CONNECT_CORRESPOND,m_pInitParameter->m_wConnectTime*1000L,1,0);

			return false;
		}

		//提示消息
		CTraceService::TraceString(TEXT("正在注册游戏登录服务器..."),TraceLevel_Normal);

		//变量定义
		CMD_CS_C_RegisterPlaza RegisterPlaza;
		ZeroMemory(&RegisterPlaza,sizeof(RegisterPlaza));

		//设置变量
		lstrcpyn(RegisterPlaza.szServerName,m_pInitParameter->m_szServerName,CountArray(RegisterPlaza.szServerName));
		lstrcpyn(RegisterPlaza.szServerAddr,m_pInitParameter->m_ServiceAddress.szAddress,CountArray(RegisterPlaza.szServerAddr));

		//发送数据
		m_pITCPSocketService->SendData(MDM_CS_REGISTER,SUB_CS_C_REGISTER_PLAZA,&RegisterPlaza,sizeof(RegisterPlaza));

		return true;
	}
	else if (wServiceID==NETWORK_PERSONAL_ROOM_CORRESPOND)
	{
		//错误判断
		if (nErrorCode!=0)
		{
			//构造提示
			TCHAR szDescribe[128]=TEXT("");
			_sntprintf(szDescribe,CountArray(szDescribe),TEXT("约战服务器连接失败 [ %ld ]，%ld 秒后将重新连接"),
				nErrorCode,m_pInitParameter->m_wConnectTime);

			//提示消息
			CTraceService::TraceString(szDescribe,TraceLevel_Warning);

			//设置时间
			ASSERT(m_pITimerEngine!=NULL);
			m_pITimerEngine->SetTimer(IDI_CONNECT_PERSONAL_ROOM_CORRESPOND,m_pInitParameter->m_wConnectTime*1000L,1,0);

			return false;
		}
	}

	return true;
}

//读取事件
bool CAttemperEngineSink::OnEventTCPSocketRead(WORD wServiceID, TCP_Command Command, VOID * pData, WORD wDataSize)
{
	//协调连接
	if (wServiceID==NETWORK_CORRESPOND)
	{
		switch (Command.wMainCmdID)
		{
		case MDM_CS_REGISTER:		//注册服务
			{
				return OnTCPSocketMainRegister(Command.wSubCmdID,pData,wDataSize);
			}
		case MDM_CS_SERVICE_INFO:	//服务信息
			{
				return OnTCPSocketMainServiceInfo(Command.wSubCmdID,pData,wDataSize);
			}
		case MDM_CS_REMOTE_SERVICE:	//远程服务
			{
				return OnTCPSocketMainRemoteService(Command.wSubCmdID,pData,wDataSize);
			}
		case MDM_CS_MANAGER_SERVICE: //管理服务
			{
				return OnTCPSocketMainManagerService(Command.wSubCmdID,pData,wDataSize);
			}
		}
	}
	else if (wServiceID==NETWORK_PERSONAL_ROOM_CORRESPOND)
	{
		if (wServiceID==MDM_CS_SERVICE_INFO)
		{
			return OnTCPSocketPersonalServiceInfo(Command.wSubCmdID,pData,wDataSize);
		}

		return true;
	}

	//错误断言
	ASSERT(FALSE);

	return true;
}

//注册事件
bool CAttemperEngineSink::OnTCPSocketMainRegister(WORD wSubCmdID, VOID * pData, WORD wDataSize)
{
	switch (wSubCmdID)
	{
	case SUB_CS_S_REGISTER_FAILURE:		//注册失败
		{
			//变量定义
			CMD_CS_S_RegisterFailure * pRegisterFailure=(CMD_CS_S_RegisterFailure *)pData;

			//效验参数
			ASSERT(wDataSize>=(sizeof(CMD_CS_S_RegisterFailure)-sizeof(pRegisterFailure->szDescribeString)));
			if (wDataSize<(sizeof(CMD_CS_S_RegisterFailure)-sizeof(pRegisterFailure->szDescribeString))) return false;

			//关闭处理
			m_bNeekCorrespond=false;
			m_pITCPSocketService->CloseSocket();

			//显示消息
			LPCTSTR pszDescribeString=pRegisterFailure->szDescribeString;
			if (lstrlen(pszDescribeString)>0) CTraceService::TraceString(pszDescribeString,TraceLevel_Exception);

			//事件通知
			CP_ControlResult ControlResult;
			ControlResult.cbSuccess=ER_FAILURE;
			SendUIControlPacket(UI_CORRESPOND_RESULT,&ControlResult,sizeof(ControlResult));

			return true;
		}
	}

	return true;
}

//列表事件
bool CAttemperEngineSink::OnTCPSocketMainServiceInfo(WORD wSubCmdID, VOID * pData, WORD wDataSize)
{
	switch (wSubCmdID)
	{
	case SUB_CS_S_SERVER_INFO:		//房间信息
		{
			//废弃列表
			m_ServerListManager.DisuseServerItem();

			return true;
		}
	case SUB_CS_S_SERVER_ONLINE:	//房间人数
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_CS_S_ServerOnLine));
			if (wDataSize!=sizeof(CMD_CS_S_ServerOnLine)) return false;

			//变量定义
			CMD_CS_S_ServerOnLine * pServerOnLine=(CMD_CS_S_ServerOnLine *)pData;

			//查找房间
			CGameServerItem * pGameServerItem=m_ServerListManager.SearchGameServer(pServerOnLine->wServerID);
			if(pGameServerItem == NULL) return true;

			//获取对象
			tagGameServer * pGameServer=&pGameServerItem->m_GameServer; 

			//设置人数
			DWORD dwOldOnlineCount=0,dwOldAndroidCount=0;
			dwOldOnlineCount = pGameServer->dwOnLineCount;
			dwOldAndroidCount = pGameServer->dwAndroidCount;

			//房间人数
			pGameServer->dwAndroidCount=pServerOnLine->dwAndroidCount;
			pGameServer->dwOnLineCount=pServerOnLine->dwOnLineCount;

			//目录人数
			CGameKindItem * pGameKindItem=m_ServerListManager.SearchGameKind(pGameServer->wKindID);
			if (pGameKindItem!=NULL)
			{
				//目录总数
				pGameKindItem->m_GameKind.dwOnLineCount -= dwOldOnlineCount;
				pGameKindItem->m_GameKind.dwOnLineCount += pGameServer->dwOnLineCount;

				//机器人数
				pGameKindItem->m_GameKind.dwAndroidCount -= dwOldAndroidCount;
				pGameKindItem->m_GameKind.dwAndroidCount += pServerOnLine->dwAndroidCount;
			}

			return true;
		}
	case SUB_CS_S_SERVER_INSERT:	//房间插入
		{
			//效验参数
			ASSERT(wDataSize%sizeof(tagGameServer)==0);
			if (wDataSize%sizeof(tagGameServer)!=0) return false;

			//变量定义
			WORD wItemCount=wDataSize/sizeof(tagGameServer);
			tagGameServer * pGameServer=(tagGameServer *)pData;

			//更新数据
			for (WORD i=0;i<wItemCount;i++)
			{
				m_ServerListManager.InsertGameServer(pGameServer++);
			}

			return true;
		}
	case SUB_CS_S_SERVER_MODIFY:	//房间修改
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_CS_S_ServerModify));
			if (wDataSize!=sizeof(CMD_CS_S_ServerModify)) return false;

			//变量定义
			CMD_CS_S_ServerModify * pServerModify=(CMD_CS_S_ServerModify *)pData;

			//查找房间
			ASSERT(m_ServerListManager.SearchGameServer(pServerModify->wServerID));
			CGameServerItem * pGameServerItem=m_ServerListManager.SearchGameServer(pServerModify->wServerID);

			//设置房间
			if (pGameServerItem!=NULL)
			{
				//设置人数
				DWORD dwOldOnlineCount=0,dwOldAndroidCount=0,dwOldFullCount=0;
				dwOldAndroidCount = pGameServerItem->m_GameServer.dwAndroidCount;
				dwOldOnlineCount = pGameServerItem->m_GameServer.dwOnLineCount;				
				dwOldFullCount   = pGameServerItem->m_GameServer.dwFullCount;

				//修改房间信息
				pGameServerItem->m_GameServer.wKindID=pServerModify->wKindID;
				pGameServerItem->m_GameServer.wNodeID=pServerModify->wNodeID;
				pGameServerItem->m_GameServer.wSortID=pServerModify->wSortID;
				pGameServerItem->m_GameServer.wServerPort=pServerModify->wServerPort;
				pGameServerItem->m_GameServer.dwOnLineCount=pServerModify->dwOnLineCount;
				pGameServerItem->m_GameServer.dwFullCount=pServerModify->dwFullCount;
				pGameServerItem->m_GameServer.dwAndroidCount=pServerModify->dwAndroidCount;
				pGameServerItem->m_GameServer.dwSetPlayerCount = pServerModify->dwSetPlayerCount;
				lstrcpyn(pGameServerItem->m_GameServer.szServerName,pServerModify->szServerName,CountArray(pGameServerItem->m_GameServer.szServerName));
				lstrcpyn(pGameServerItem->m_GameServer.szServerAddr,pServerModify->szServerAddr,CountArray(pGameServerItem->m_GameServer.szServerAddr));
				lstrcpyn(pGameServerItem->m_GameServer.szGameInfomation, pServerModify->szGameInfomation, CountArray(pGameServerItem->m_GameServer.szGameInfomation));

				//目录人数
				CGameKindItem * pGameKindItem=m_ServerListManager.SearchGameKind(pGameServerItem->m_GameServer.wKindID);
				if (pGameKindItem!=NULL)
				{
					tagGameServer * pGameServer=&pGameServerItem->m_GameServer;
					pGameKindItem->m_GameKind.dwOnLineCount -= dwOldOnlineCount;
					pGameKindItem->m_GameKind.dwOnLineCount += pGameServer->dwOnLineCount;

					pGameKindItem->m_GameKind.dwFullCount -= dwOldFullCount;
					pGameKindItem->m_GameKind.dwFullCount += pGameServer->dwFullCount;

					pGameKindItem->m_GameKind.dwAndroidCount -= dwOldAndroidCount;
					pGameKindItem->m_GameKind.dwAndroidCount += pServerModify->dwAndroidCount;

					pGameKindItem->m_GameKind.dwSetCount = pServerModify->dwSetPlayerCount;
				}
			}

			return true;
		}
	case SUB_CS_S_SERVER_REMOVE:	//房间删除
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_CS_S_ServerRemove));
			if (wDataSize!=sizeof(CMD_CS_S_ServerRemove)) return false;

			//变量定义
			CMD_CS_S_ServerRemove * pServerRemove=(CMD_CS_S_ServerRemove *)pData;

			//变量定义
			m_ServerListManager.DeleteGameServer(pServerRemove->wServerID);

			DBR_GR_CLOSE_ROOM_SERVER_ID closeRoomServerID;
			closeRoomServerID.dwServerID = pServerRemove->wServerID;

			//if (m_bHasPrsnlRM)
			//{
			//	//向数据可发送查询每个房间信息的请求
			//	m_pIDataBaseEngine->PostDataBaseRequest(DBR_GR_CLOSE_ROOM_WRITE_DISSUME_TIME,  0,  &closeRoomServerID, sizeof(DBR_GR_CLOSE_ROOM_SERVER_ID));	
			//}
			return true;
		}
	case SUB_CS_S_SERVER_FINISH:	//房间完成
		{
			//清理列表
			m_ServerListManager.CleanServerItem();

			//事件处理
			CP_ControlResult ControlResult;
			ControlResult.cbSuccess=ER_SUCCESS;
			SendUIControlPacket(UI_CORRESPOND_RESULT,&ControlResult,sizeof(ControlResult));

			return true;
		}
	case SUB_CS_S_MATCH_INSERT:		//比赛插入
		{
			//效验参数
			ASSERT(wDataSize%sizeof(tagGameMatch)==0);
			if (wDataSize%sizeof(tagGameMatch)!=0) return false;

			//变量定义
			WORD wItemCount=wDataSize/sizeof(tagGameMatch);
			tagGameMatch * pGameMatch=(tagGameMatch *)pData;

			//更新数据
			for (WORD i=0;i<wItemCount;i++)
			{
				CGameServerItem * pGameServerItem = m_ServerListManager.SearchGameServer(pGameMatch->wServerID);
				if(pGameServerItem!=NULL)
				{
					CopyMemory(&pGameServerItem->m_GameMatch,pGameMatch++,sizeof(pGameServerItem->m_GameMatch));
				}
			}

			return true;
		}
	}

	return true;
}

//约战服务
bool CAttemperEngineSink::OnTCPSocketPersonalServiceInfo(WORD wSubCmdID, VOID * pData, WORD wDataSize)
{
	switch (wSubCmdID)
	{
	case SUB_CS_S_QUERY_GAME_SERVER_RESULT:		//查询结果
		{
			//效验数据
			ASSERT(wDataSize==sizeof(CMD_CS_S_QueryGameServerResult));
			if (wDataSize!=sizeof(CMD_CS_S_QueryGameServerResult)) return true;

			CMD_CS_S_QueryGameServerResult *pQueryGameServerResult = (CMD_CS_S_QueryGameServerResult*)pData;

			//判断在线
			ASSERT(LOWORD(pQueryGameServerResult->dwSocketID)<m_pInitParameter->m_wMaxConnect);
			if ((m_pBindParameter+LOWORD(pQueryGameServerResult->dwSocketID))->dwSocketID!=pQueryGameServerResult->dwSocketID) return true;

			//构造数据
			CMD_MB_QueryGameServerResult QueryGameServerResult;
			ZeroMemory(&QueryGameServerResult, sizeof(CMD_MB_QueryGameServerResult));

			QueryGameServerResult.dwServerID = pQueryGameServerResult->dwServerID;
			QueryGameServerResult.bCanCreateRoom = pQueryGameServerResult->bCanCreateRoom;
			lstrcpyn(QueryGameServerResult.szErrDescrybe,  pQueryGameServerResult->szErrDescrybe, sizeof(QueryGameServerResult.szErrDescrybe));

			//发送数据
			m_pITCPNetworkEngine->SendData(pQueryGameServerResult->dwSocketID, MDM_MB_PERSONAL_SERVICE, SUB_MB_QUERY_GAME_SERVER_RESULT, &QueryGameServerResult, sizeof(CMD_MB_QueryGameServerResult));

			return true;
		}
	case SUB_CS_S_SEARCH_TABLE_RESULT:
		{
			//校验数据
			ASSERT(wDataSize == sizeof(CMD_CS_S_SearchTableResult));
			if(wDataSize != sizeof(CMD_CS_S_SearchTableResult)) return true;

			CMD_CS_S_SearchTableResult* pSearTableResult = (CMD_CS_S_SearchTableResult*)pData;

			//判断在线
			ASSERT(LOWORD(pSearTableResult->dwSocketID)<m_pInitParameter->m_wMaxConnect);
			if ((m_pBindParameter+LOWORD(pSearTableResult->dwSocketID))->dwSocketID!=pSearTableResult->dwSocketID) return true;

			//构造数据
			CMD_MB_SearchResult SearchResult;
			ZeroMemory(&SearchResult, sizeof(CMD_MB_SearchResult));

			SearchResult.dwServerID = pSearTableResult->dwServerID;
			SearchResult.dwTableID	= pSearTableResult->dwTableID;
			SearchResult.dwKindID	= pSearTableResult->dwKindID;

			//发送消息
			m_pITCPNetworkEngine->SendData(pSearTableResult->dwSocketID, MDM_MB_PERSONAL_SERVICE, SUB_MB_SEARCH_RESULT, &SearchResult, sizeof(CMD_MB_SearchResult));

			return true;
		}
	case SUB_CS_S_DISSUME_SEARCH_TABLE_RESULT:
		{
			//校验数据
			ASSERT(wDataSize == sizeof(CMD_CS_S_SearchTableResult));
			if(wDataSize != sizeof(CMD_CS_S_SearchTableResult)) return true;

			CMD_CS_S_SearchTableResult* pSearTableResult = (CMD_CS_S_SearchTableResult*)pData;

			//判断在线
			ASSERT(LOWORD(pSearTableResult->dwSocketID)<m_pInitParameter->m_wMaxConnect);
			if ((m_pBindParameter+LOWORD(pSearTableResult->dwSocketID))->dwSocketID!=pSearTableResult->dwSocketID) return true;

			//构造数据
			CMD_MB_DissumeSearchResult SearchResult;
			ZeroMemory(&SearchResult, sizeof(CMD_MB_DissumeSearchResult));

			SearchResult.dwServerID = pSearTableResult->dwServerID;
			SearchResult.dwTableID = pSearTableResult->dwTableID;

			//发送消息
			//发送消息
			m_pITCPNetworkEngine->SendData(pSearTableResult->dwSocketID, MDM_MB_PERSONAL_SERVICE, SUB_MB_DISSUME_SEARCH_RESULT, &SearchResult, sizeof(CMD_MB_DissumeSearchResult));

			return true;
		}
	case SUB_CS_S_QUERY_ROOM_PASSWORD_RESULT:
		{
			//校验数据
			ASSERT(wDataSize == sizeof(CMD_CS_S_QueryRoomPasswordResult));
			if (wDataSize != sizeof(CMD_CS_S_QueryRoomPasswordResult)) return true;

			CMD_CS_S_QueryRoomPasswordResult* pSearTableResult = (CMD_CS_S_QueryRoomPasswordResult*)pData;

			//判断在线
			ASSERT(LOWORD(pSearTableResult->dwSocketID)<m_pInitParameter->m_wMaxConnect);
			if ((m_pBindParameter + LOWORD(pSearTableResult->dwSocketID))->dwSocketID != pSearTableResult->dwSocketID) return true;

			//构造数据
			CMD_MB_QueryRoomPwdResult SearchResult;
			ZeroMemory(&SearchResult, sizeof(CMD_MB_QueryRoomPwdResult));

			SearchResult.dwRoomDwd = pSearTableResult->dwRoomPassword;
		
			//发送消息
			//发送消息
			m_pITCPNetworkEngine->SendData(pSearTableResult->dwSocketID, MDM_MB_PERSONAL_SERVICE, SUB_MB_QUERY_ROOM_PASSWORD_RESULT, &SearchResult, sizeof(CMD_MB_DissumeSearchResult));
						
			return true;
		}
	case SUB_CS_C_QUERY_PERSONAL_ROOM_LIST_RESULT:
		{
			//校验数据
			ASSERT(wDataSize == sizeof(CMD_CS_C_HostCreatRoomInfo));
			if(wDataSize != sizeof(CMD_CS_C_HostCreatRoomInfo)) return true;

			CMD_CS_C_HostCreatRoomInfo* pHostCreatRoomInfo = (CMD_CS_C_HostCreatRoomInfo*)pData;

			//向数据可发送查询每个房间信息的请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_MB_QUERY_PERSONAL_ROOM_INFO,  pHostCreatRoomInfo->wSocketID,  pHostCreatRoomInfo, sizeof(CMD_CS_C_HostCreatRoomInfo));	

			return true;
		}
	}

	return true;
}

//远程服务
bool CAttemperEngineSink::OnTCPSocketMainRemoteService(WORD wSubCmdID, VOID * pData, WORD wDataSize)
{
	switch (wSubCmdID)
	{
	case SUB_CS_S_SEARCH_CORRESPOND:	//协调查找
		{
			//变量定义
			CMD_CS_S_SearchCorrespond * pSearchCorrespond=(CMD_CS_S_SearchCorrespond *)pData;

			//效验参数
			ASSERT(wDataSize<=sizeof(CMD_CS_S_SearchCorrespond));
			ASSERT(wDataSize>=(sizeof(CMD_CS_S_SearchCorrespond)-sizeof(pSearchCorrespond->UserRemoteInfo)));
			ASSERT(wDataSize==(sizeof(CMD_CS_S_SearchCorrespond)-sizeof(pSearchCorrespond->UserRemoteInfo)+pSearchCorrespond->wUserCount*sizeof(pSearchCorrespond->UserRemoteInfo[0])));

			//效验参数
			if (wDataSize>sizeof(CMD_CS_S_SearchCorrespond)) return false;
			if (wDataSize<(sizeof(CMD_CS_S_SearchCorrespond)-sizeof(pSearchCorrespond->UserRemoteInfo))) return false;
			if (wDataSize!=(sizeof(CMD_CS_S_SearchCorrespond)-sizeof(pSearchCorrespond->UserRemoteInfo)+pSearchCorrespond->wUserCount*sizeof(pSearchCorrespond->UserRemoteInfo[0]))) return false;

			//判断在线
			ASSERT(LOWORD(pSearchCorrespond->dwSocketID)<m_pInitParameter->m_wMaxConnect);
			if ((m_pBindParameter+LOWORD(pSearchCorrespond->dwSocketID))->dwSocketID!=pSearchCorrespond->dwSocketID) return true;

			//变量定义
			CMD_GP_S_SearchCorrespond SearchCorrespond;
			ZeroMemory(&SearchCorrespond,sizeof(SearchCorrespond));

			//设置变量
			for (WORD i=0;i<pSearchCorrespond->wUserCount;i++)
			{
				//数据效验
				ASSERT(SearchCorrespond.wUserCount<CountArray(SearchCorrespond.UserRemoteInfo));
				if (SearchCorrespond.wUserCount>=CountArray(SearchCorrespond.UserRemoteInfo)) break;

				//设置变量
				WORD wIndex=SearchCorrespond.wUserCount++;
				CopyMemory(&SearchCorrespond.UserRemoteInfo[wIndex],&pSearchCorrespond->UserRemoteInfo[i],sizeof(SearchCorrespond.UserRemoteInfo[wIndex]));
			}

			//发送数据
			WORD wHeadSize=sizeof(SearchCorrespond)-sizeof(SearchCorrespond.UserRemoteInfo);
			WORD wItemSize=sizeof(SearchCorrespond.UserRemoteInfo[0])*SearchCorrespond.wUserCount;
			m_pITCPNetworkEngine->SendData(pSearchCorrespond->dwSocketID,MDM_GP_REMOTE_SERVICE,SUB_GP_S_SEARCH_CORRESPOND,&SearchCorrespond,wHeadSize+wItemSize);

			return true;
		}
	case SUB_CS_S_SEARCH_ALLCORRESPOND:
		{
			//变量定义
			CMD_CS_S_SearchAllCorrespond * pSearchAllCorrespond=(CMD_CS_S_SearchAllCorrespond *)pData;


			BYTE cbDataBuffer[SOCKET_TCP_PACKET]={0};
			WORD cbDataSize = sizeof(CMD_GP_S_SearchAllCorrespond)-sizeof(tagUserRemoteInfo);
			CMD_GP_S_SearchAllCorrespond * pSearchCorrespond=(CMD_GP_S_SearchAllCorrespond *)cbDataBuffer;
			pSearchCorrespond->dwCount = pSearchAllCorrespond->dwCount;
			int nCount =(int)(pSearchAllCorrespond->dwCount);
			for(int i = 0; i < nCount; i++)
			{
				memcpy(&pSearchCorrespond->UserRemoteInfo[i], &pSearchAllCorrespond->UserRemoteInfo[i], sizeof(tagUserRemoteInfo));

				cbDataSize += sizeof(tagUserRemoteInfo);
			}

			if(pSearchAllCorrespond->dwCount == 0) cbDataSize = sizeof(CMD_GP_S_SearchAllCorrespond);
			m_pITCPNetworkEngine->SendData(pSearchAllCorrespond->dwSocketID,MDM_GP_REMOTE_SERVICE,SUB_GP_S_SEARCH_ALLCORRESPOND,pSearchCorrespond,cbDataSize);
			return true;
		}
	}

	return true;
}

//管理服务
bool CAttemperEngineSink::OnTCPSocketMainManagerService(WORD wSubCmdID, VOID * pData, WORD wDataSize)
{
	switch (wSubCmdID)
	{
	case SUB_CS_S_PLATFORM_PARAMETER:	//平台参数
		{
			//清除数据
			m_wTaskCountBuffer = 0;
			ZeroMemory(m_TaskParameterBuffer, sizeof(m_TaskParameterBuffer));

			//平台参数
			//m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOAD_PLATFORM_PARAMETER,0,NULL,0);		

			//加载任务
			//m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOAD_TASK_LIST, 0, NULL, 0);

			//会员参数
			//m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOAD_MEMBER_PARAMETER,0,NULL,0);	

			//签到奖励
			//m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOAD_CHECKIN_REWARD, 0, NULL, 0);			

			//加载低保
			//m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOAD_BASEENSURE, 0, NULL, 0);

			//加载认证
			//m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOAD_REAL_AUTH, 0, NULL, 0);

			//道具配置
			m_GamePropertyListManager.DisusePropertyTypeItem();
			m_GamePropertyListManager.DisusePropertyItem();
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOAD_GAME_PROPERTY_LIST,0,NULL,0);

			return true;
		}
	}

	return true;
}

//登录处理
bool CAttemperEngineSink::OnTCPNetworkMainPCLogon(WORD wSubCmdID, VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	ALStd("Login cmd %d", wSubCmdID);
	switch (wSubCmdID)
	{
	case SUB_GP_LOGON_GAMEID:		//I D 登录
		{
			return OnTCPNetworkSubPCLogonGameID(pData,wDataSize,dwSocketID);
		}
	case SUB_GP_LOGON_ACCOUNTS:		//帐号登录
		{
			return OnTCPNetworkSubPCLogonAccounts(pData,wDataSize,dwSocketID);
		}
	case SUB_GP_LOGON_MANAGETOOL:	//工具登录
		{
			return OnTCPNetworkSubPCLogonManageTool(pData,wDataSize,dwSocketID);
		}
	case SUB_GP_REGISTER_ACCOUNTS:	//帐号注册
		{
			return OnTCPNetworkSubPCRegisterAccounts(pData,wDataSize,dwSocketID);
		}
	case SUB_GP_VERIFY_INDIVIDUAL:	//校验资料
		{
			return OnTCPNetworkSubPCVerifyIndividual(pData,wDataSize,dwSocketID);
		}
	case SUB_GP_LOGON_VISITOR:      //游客登录
		{
			return OnTCPNetworkSubPCLogonVisitor(pData,wDataSize,dwSocketID);
		}
	case SUB_GP_QUERY_VERIFY_CODE:	//查询验证码
		{
			return OnTCPNetworkSubPCQueryVerifyCode(pData,wDataSize,dwSocketID);
		}
	}

	return false;
}

//列表处理
bool CAttemperEngineSink::OnTCPNetworkMainPCServerList(WORD wSubCmdID, VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	switch (wSubCmdID)
	{
	case SUB_GP_GET_LIST:			//获取列表
		{
			//发送列表
			SendGameTypeInfo(dwSocketID);
			SendGameKindInfo(dwSocketID);

			//发送列表
			if (m_pInitParameter->m_cbDelayList==FALSE)
			{
				//发送列表
				SendGamePageInfo(dwSocketID,INVALID_WORD);
				SendGameNodeInfo(dwSocketID,INVALID_WORD);
				//SendGameServerInfo(dwSocketID,INVALID_WORD);
				SendGameServerInfo(dwSocketID,INVALID_WORD,DEVICE_TYPE_PC);
			}
			else
			{
				//发送页面
				SendGamePageInfo(dwSocketID,0);
			}

			//发送完成
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_FINISH);

			return true;
		}
	case SUB_GP_GET_SERVER:			//获取房间
		{
			//效验数据
			ASSERT(wDataSize%sizeof(WORD)==0);
			if (wDataSize%sizeof(WORD)!=0) return false;

			//发送列表
			UINT nKindCount=wDataSize/sizeof(WORD);
			for (UINT i=0;i<nKindCount;i++)
			{
				SendGameNodeInfo(dwSocketID,((WORD *)pData)[i]);
				SendGamePageInfo(dwSocketID,((WORD *)pData)[i]);
				//SendGameServerInfo(dwSocketID,((WORD *)pData)[i]);
				SendGameServerInfo(dwSocketID,((WORD *)pData)[i],DEVICE_TYPE_PC);
			}

			//发送完成
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_SERVER_FINISH,pData,wDataSize);

			return true;
		}
	case SUB_GP_GET_ONLINE:			//获取在线
		{
			//变量定义
			CMD_GP_GetOnline * pGetOnline=(CMD_GP_GetOnline *)pData;
			WORD wHeadSize=(sizeof(CMD_GP_GetOnline)-sizeof(pGetOnline->wOnLineServerID));

			//效验数据
			ASSERT((wDataSize>=wHeadSize)&&(wDataSize==(wHeadSize+pGetOnline->wServerCount*sizeof(WORD))));
			if ((wDataSize<wHeadSize)||(wDataSize!=(wHeadSize+pGetOnline->wServerCount*sizeof(WORD)))) return false;

			//变量定义
			CMD_GP_KindOnline KindOnline;
			CMD_GP_ServerOnline ServerOnline;
			ZeroMemory(&KindOnline,sizeof(KindOnline));
			ZeroMemory(&ServerOnline,sizeof(ServerOnline));

			//获取类型
			POSITION KindPosition=NULL;
			do
			{
				//获取类型
				CGameKindItem * pGameKindItem=m_ServerListManager.EmunGameKindItem(KindPosition);

				//设置变量
				if (pGameKindItem!=NULL)
				{
					WORD wKindIndex=KindOnline.wKindCount++;
					KindOnline.OnLineInfoKind[wKindIndex].wKindID=pGameKindItem->m_GameKind.wKindID;
					KindOnline.OnLineInfoKind[wKindIndex].dwOnLineCount = pGameKindItem->m_GameKind.dwOnLineCount + pGameKindItem->m_GameKind.dwAndroidCount;
				}

				//溢出判断
				if (KindOnline.wKindCount>=CountArray(KindOnline.OnLineInfoKind))
				{
					ASSERT(FALSE);
					break;
				}

			} while (KindPosition!=NULL);

			//获取房间
			for (WORD i=0;i<pGetOnline->wServerCount;i++)
			{
				//获取房间
				WORD wServerID=pGetOnline->wOnLineServerID[i];
				CGameServerItem * pGameServerItem=m_ServerListManager.SearchGameServer(wServerID);

				//设置变量
				if (pGameServerItem!=NULL)
				{
					WORD wServerIndex=ServerOnline.wServerCount++;
					ServerOnline.OnLineInfoServer[wServerIndex].wServerID=wServerID;
					ServerOnline.OnLineInfoServer[wServerIndex].dwOnLineCount = pGameServerItem->m_GameServer.dwOnLineCount + pGameServerItem->m_GameServer.dwAndroidCount + pGameServerItem->m_GameServer.dwSetPlayerCount;
				}

				//溢出判断
				if (ServerOnline.wServerCount>=CountArray(ServerOnline.OnLineInfoServer))
				{
					ASSERT(FALSE);
					break;
				}
			}

			//类型在线
			if (KindOnline.wKindCount>0)
			{
				WORD wHeadSize=sizeof(KindOnline)-sizeof(KindOnline.OnLineInfoKind);
				WORD wSendSize=wHeadSize+KindOnline.wKindCount*sizeof(KindOnline.OnLineInfoKind[0]);
				m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GR_KINE_ONLINE,&KindOnline,wSendSize);
			}

			//房间在线
			if (ServerOnline.wServerCount>0)
			{
				WORD wHeadSize=sizeof(ServerOnline)-sizeof(ServerOnline.OnLineInfoServer);
				WORD wSendSize=wHeadSize+ServerOnline.wServerCount*sizeof(ServerOnline.OnLineInfoServer[0]);
				m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GR_SERVER_ONLINE,&ServerOnline,wSendSize);
			}

			return true;
		}
	case SUB_GP_GET_COLLECTION:		//获取收藏
		{
			return true;
		}
	case SUB_GP_GET_PROPERTY:		//获取道具
		{
			//道具配置
			SendGamePropertyTypeInfo(dwSocketID);
			SendGamePropertyRelatInfo(dwSocketID);
			SendGamePropertyInfo(dwSocketID);
			SendGamePropertySubInfo(dwSocketID);
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_PROPERTY_FINISH);

			return true;
		}
	}

	return false;
}

//服务处理
bool CAttemperEngineSink::OnTCPNetworkMainPCUserService(WORD wSubCmdID, VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	switch (wSubCmdID)
	{
	// Obsoleted TODO:remove
	// post message DBR_GP_MODIFY_LOGON_PASS which is used by SUB_MB_RESETPASSWD now
	// case SUB_GP_MODIFY_LOGON_PASS:

	case SUB_GP_MODIFY_MACHINE:			  //绑定机器
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_ModifyMachine));
			if (wDataSize!=sizeof(CMD_GP_ModifyMachine)) return false;

			//处理消息
			CMD_GP_ModifyMachine * pModifyMachine=(CMD_GP_ModifyMachine *)pData;
			pModifyMachine->szPassword[CountArray(pModifyMachine->szPassword)-1]=0;

			//变量定义
			DBR_GP_ModifyMachine ModifyMachine;
			ZeroMemory(&ModifyMachine,sizeof(ModifyMachine));

			//构造数据
			ModifyMachine.cbBind=pModifyMachine->cbBind;
			ModifyMachine.dwUserID=pModifyMachine->dwUserID;
			ModifyMachine.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(ModifyMachine.szPassword,pModifyMachine->szPassword,CountArray(ModifyMachine.szPassword));
			lstrcpyn(ModifyMachine.szMachineID,pModifyMachine->szMachineID,CountArray(ModifyMachine.szMachineID));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_MODIFY_MACHINE,dwSocketID,&ModifyMachine,sizeof(ModifyMachine));

			return true;
		}
	case SUB_GP_MODIFY_INSURE_PASS:		  //修改密码
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_ModifyInsurePass));
			if (wDataSize!=sizeof(CMD_GP_ModifyInsurePass)) return false;

			//处理消息
			CMD_GP_ModifyInsurePass * pModifyInsurePass=(CMD_GP_ModifyInsurePass *)pData;
			pModifyInsurePass->szDesPassword[CountArray(pModifyInsurePass->szDesPassword)-1]=0;
			pModifyInsurePass->szScrPassword[CountArray(pModifyInsurePass->szScrPassword)-1]=0;

			//变量定义
			DBR_GP_ModifyInsurePass ModifyInsurePass;
			ZeroMemory(&ModifyInsurePass,sizeof(ModifyInsurePass));

			//构造数据
			ModifyInsurePass.dwUserID=pModifyInsurePass->dwUserID;
			ModifyInsurePass.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(ModifyInsurePass.szDesPassword,pModifyInsurePass->szDesPassword,CountArray(ModifyInsurePass.szDesPassword));
			lstrcpyn(ModifyInsurePass.szScrPassword,pModifyInsurePass->szScrPassword,CountArray(ModifyInsurePass.szScrPassword));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_MODIFY_INSURE_PASS,dwSocketID,&ModifyInsurePass,sizeof(ModifyInsurePass));

			return true;
		}
	case SUB_GP_MODIFY_UNDER_WRITE:		  //修改签名
		{
			//变量定义
			CMD_GP_ModifyUnderWrite * pModifyUnderWrite=(CMD_GP_ModifyUnderWrite *)pData;

			//效验参数
			ASSERT(wDataSize<=sizeof(CMD_GP_ModifyUnderWrite));
			ASSERT(wDataSize>=(sizeof(CMD_GP_ModifyUnderWrite)-sizeof(pModifyUnderWrite->szUnderWrite)));
			ASSERT(wDataSize==(sizeof(CMD_GP_ModifyUnderWrite)-sizeof(pModifyUnderWrite->szUnderWrite)+CountStringBuffer(pModifyUnderWrite->szUnderWrite)));

			//效验参数
			if (wDataSize>sizeof(CMD_GP_ModifyUnderWrite)) return false;
			if (wDataSize<(sizeof(CMD_GP_ModifyUnderWrite)-sizeof(pModifyUnderWrite->szUnderWrite))) return false;
			if (wDataSize!=(sizeof(CMD_GP_ModifyUnderWrite)-sizeof(pModifyUnderWrite->szUnderWrite)+CountStringBuffer(pModifyUnderWrite->szUnderWrite))) return false;

			//处理消息
			pModifyUnderWrite->szPassword[CountArray(pModifyUnderWrite->szPassword)-1]=0;
			pModifyUnderWrite->szUnderWrite[CountArray(pModifyUnderWrite->szUnderWrite)-1]=0;

			//变量定义
			DBR_GP_ModifyUnderWrite ModifyUnderWrite;
			ZeroMemory(&ModifyUnderWrite,sizeof(ModifyUnderWrite));

			//构造数据
			ModifyUnderWrite.dwUserID=pModifyUnderWrite->dwUserID;
			ModifyUnderWrite.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(ModifyUnderWrite.szPassword,pModifyUnderWrite->szPassword,CountArray(ModifyUnderWrite.szPassword));
			lstrcpyn(ModifyUnderWrite.szUnderWrite,pModifyUnderWrite->szUnderWrite,CountArray(ModifyUnderWrite.szUnderWrite));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_MODIFY_UNDER_WRITE,dwSocketID,&ModifyUnderWrite,sizeof(ModifyUnderWrite));

			return true;
		}
	case SUB_GP_REAL_AUTH_QUERY:		  //实名认证
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_RealAuth));

			//变量定义
			CMD_GP_RealAuth * pCmdData=(CMD_GP_RealAuth *)pData;

			//处理消息
			pCmdData->szPassword[CountArray(pCmdData->szPassword)-1]=0;
			pCmdData->szCompellation[CountArray(pCmdData->szCompellation)-1]=0;
			pCmdData->szPassPortID[CountArray(pCmdData->szPassPortID)-1]=0;

			//变量定义
			DBR_GP_RealAuth DataPara;
			ZeroMemory(&DataPara,sizeof(DataPara));

			//构造数据
			DataPara.dwUserID=pCmdData->dwUserID;
			DataPara.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(DataPara.szPassword,pCmdData->szPassword,CountArray(DataPara.szPassword));
			lstrcpyn(DataPara.szCompellation,pCmdData->szCompellation,CountArray(DataPara.szCompellation));
			lstrcpyn(DataPara.szPassPortID,pCmdData->szPassPortID,CountArray(DataPara.szPassPortID));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_MODIFY_REAL_AUTH,dwSocketID,&DataPara,sizeof(DataPara));

			return true;
		}
	case SUB_GP_SYSTEM_FACE_INFO:		  //修改头像
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_SystemFaceInfo));
			if (wDataSize!=sizeof(CMD_GP_SystemFaceInfo)) return false;

			//处理消息
			CMD_GP_SystemFaceInfo * pSystemFaceInfo=(CMD_GP_SystemFaceInfo *)pData;

			//变量定义
			DBR_GP_ModifySystemFace ModifySystemFace;
			ZeroMemory(&ModifySystemFace,sizeof(ModifySystemFace));

			//构造数据
			ModifySystemFace.wFaceID=pSystemFaceInfo->wFaceID;
			ModifySystemFace.dwUserID=pSystemFaceInfo->dwUserID;
			ModifySystemFace.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(ModifySystemFace.szPassword,pSystemFaceInfo->szPassword,CountArray(ModifySystemFace.szPassword));
			lstrcpyn(ModifySystemFace.szMachineID,pSystemFaceInfo->szMachineID,CountArray(ModifySystemFace.szMachineID));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_MODIFY_SYSTEM_FACE,dwSocketID,&ModifySystemFace,sizeof(ModifySystemFace));

			return true;
		}
	case SUB_GP_CUSTOM_FACE_INFO:		  //修改头像
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_CustomFaceInfo));
			if (wDataSize!=sizeof(CMD_GP_CustomFaceInfo)) return false;

			//处理消息
			CMD_GP_CustomFaceInfo * pCustomFaceInfo=(CMD_GP_CustomFaceInfo *)pData;

			//变量定义
			DBR_GP_ModifyCustomFace ModifyCustomFace;
			ZeroMemory(&ModifyCustomFace,sizeof(ModifyCustomFace));

			//构造数据
			ModifyCustomFace.dwUserID=pCustomFaceInfo->dwUserID;
			ModifyCustomFace.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(ModifyCustomFace.szPassword,pCustomFaceInfo->szPassword,CountArray(ModifyCustomFace.szPassword));
			lstrcpyn(ModifyCustomFace.szMachineID,pCustomFaceInfo->szMachineID,CountArray(ModifyCustomFace.szMachineID));
			CopyMemory(ModifyCustomFace.dwCustomFace,pCustomFaceInfo->dwCustomFace,sizeof(ModifyCustomFace.dwCustomFace));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_MODIFY_CUSTOM_FACE,dwSocketID,&ModifyCustomFace,sizeof(ModifyCustomFace));

			return true;
		}
	case SUB_GP_QUERY_INDIVIDUAL:		  //查询信息
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_QueryIndividual));
			if (wDataSize!=sizeof(CMD_GP_QueryIndividual)) return false;

			//处理消息
			CMD_GP_QueryIndividual * pQueryIndividual=(CMD_GP_QueryIndividual *)pData;

			//变量定义
			DBR_GP_QueryIndividual QueryIndividual;
			ZeroMemory(&QueryIndividual,sizeof(QueryIndividual));

			//构造数据
			QueryIndividual.dwUserID=pQueryIndividual->dwUserID;
			QueryIndividual.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(QueryIndividual.szPassword,pQueryIndividual->szPassword,CountArray(QueryIndividual.szPassword));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_QUERY_INDIVIDUAL,dwSocketID,&QueryIndividual,sizeof(QueryIndividual));
			
			return true;
		}
	case SUB_GP_BIND_SPREADER:		//绑定推广
		{
			//效验参数
			ASSERT(wDataSize == sizeof(CMD_GP_BindSpreader));
			if (wDataSize != sizeof(CMD_GP_BindSpreader)) return false;

			//处理消息
			CMD_GP_BindSpreader * pBindSpreader = (CMD_GP_BindSpreader *)pData;

			//变量定义
			DBR_GP_BindSpreader BindSpreader;
			ZeroMemory(&BindSpreader, sizeof(BindSpreader));

			//构造数据
			BindSpreader.dwUserID = pBindSpreader->dwUserID;
			BindSpreader.dwClientAddr = (m_pBindParameter + LOWORD(dwSocketID))->dwClientAddr;
			BindSpreader.dwSpreaderID = pBindSpreader->dwSpreaderID;
			lstrcpyn(BindSpreader.szPassword, pBindSpreader->szPassword, CountArray(BindSpreader.szPassword));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_BIND_SPREADER, dwSocketID, &BindSpreader, sizeof(BindSpreader));

			return true;
		}
	case SUB_GP_MODIFY_INDIVIDUAL:		  //修改资料
		{
			//效验参数
			ASSERT(wDataSize>=sizeof(CMD_GP_ModifyIndividual));
			if (wDataSize<sizeof(CMD_GP_ModifyIndividual)) return false;

			//处理消息
			CMD_GP_ModifyIndividual * pModifyIndividual=(CMD_GP_ModifyIndividual *)pData;
			pModifyIndividual->szPassword[CountArray(pModifyIndividual->szPassword)-1]=0;

			//变量定义
			DBR_GP_ModifyIndividual ModifyIndividual;
			ZeroMemory(&ModifyIndividual,sizeof(ModifyIndividual));

			//设置变量
			ModifyIndividual.dwUserID=pModifyIndividual->dwUserID;
			ModifyIndividual.cbGender=pModifyIndividual->cbGender;
			ModifyIndividual.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(ModifyIndividual.szPassword,pModifyIndividual->szPassword,CountArray(ModifyIndividual.szPassword));

			//变量定义
			VOID * pDataBuffer=NULL;
			tagDataDescribe DataDescribe;
			CRecvPacketHelper RecvPacket(pModifyIndividual+1,wDataSize-sizeof(CMD_GP_ModifyIndividual));

			//扩展信息
			while (true)
			{
				pDataBuffer=RecvPacket.GetData(DataDescribe);
				if (DataDescribe.wDataDescribe==DTP_NULL) break;
				switch (DataDescribe.wDataDescribe)
				{
				case DTP_GP_UI_NICKNAME:		//用户昵称
					{
						ASSERT(pDataBuffer!=NULL);
						ASSERT(DataDescribe.wDataSize<=sizeof(ModifyIndividual.szNickName));
						if (DataDescribe.wDataSize<=sizeof(ModifyIndividual.szNickName))
						{
							CopyMemory(&ModifyIndividual.szNickName,pDataBuffer,DataDescribe.wDataSize);
							ModifyIndividual.szNickName[CountArray(ModifyIndividual.szNickName)-1]=0;
						}
						break;
					}
				case DTP_GP_UI_UNDER_WRITE:		//个性签名
					{
						ASSERT(pDataBuffer!=NULL);
						ASSERT(DataDescribe.wDataSize<=sizeof(ModifyIndividual.szUnderWrite));
						if (DataDescribe.wDataSize<=sizeof(ModifyIndividual.szUnderWrite))
						{
							CopyMemory(&ModifyIndividual.szUnderWrite,pDataBuffer,DataDescribe.wDataSize);
							ModifyIndividual.szUnderWrite[CountArray(ModifyIndividual.szUnderWrite)-1]=0;
						}
						break;
					}
				case DTP_GP_UI_USER_NOTE:		//用户备注
					{
						ASSERT(pDataBuffer!=NULL);
						ASSERT(DataDescribe.wDataSize<=sizeof(ModifyIndividual.szUserNote));
						if (DataDescribe.wDataSize<=sizeof(ModifyIndividual.szUserNote))
						{
							CopyMemory(&ModifyIndividual.szUserNote,pDataBuffer,DataDescribe.wDataSize);
							ModifyIndividual.szUserNote[CountArray(ModifyIndividual.szUserNote)-1]=0;
						}
						break;
					}
				case DTP_GP_UI_COMPELLATION:	//真实名字
					{
						ASSERT(pDataBuffer!=NULL);
						ASSERT(DataDescribe.wDataSize<=sizeof(ModifyIndividual.szCompellation));
						if (DataDescribe.wDataSize<=sizeof(ModifyIndividual.szCompellation))
						{
							CopyMemory(&ModifyIndividual.szCompellation,pDataBuffer,DataDescribe.wDataSize);
							ModifyIndividual.szCompellation[CountArray(ModifyIndividual.szCompellation)-1]=0;
						}
						break;
					}
				case DTP_GP_UI_PASSPORTID:	//身份证
					{
						ASSERT(pDataBuffer!=NULL);
						ASSERT(DataDescribe.wDataSize<=sizeof(ModifyIndividual.szPassPortID));
						if (DataDescribe.wDataSize<=sizeof(ModifyIndividual.szPassPortID))
						{
							CopyMemory(&ModifyIndividual.szPassPortID,pDataBuffer,DataDescribe.wDataSize);
							ModifyIndividual.szPassPortID[CountArray(ModifyIndividual.szPassPortID)-1]=0;
						}
						break;
					}
				case DTP_GP_UI_SEAT_PHONE:		//固定电话
					{
						ASSERT(pDataBuffer!=NULL);
						ASSERT(DataDescribe.wDataSize<=sizeof(ModifyIndividual.szSeatPhone));
						if (DataDescribe.wDataSize<=sizeof(ModifyIndividual.szSeatPhone))
						{
							CopyMemory(ModifyIndividual.szSeatPhone,pDataBuffer,DataDescribe.wDataSize);
							ModifyIndividual.szSeatPhone[CountArray(ModifyIndividual.szSeatPhone)-1]=0;
						}
						break;
					}
				case DTP_GP_UI_MOBILE_PHONE:	//移动电话
					{
						ASSERT(pDataBuffer!=NULL);
						ASSERT(DataDescribe.wDataSize<=sizeof(ModifyIndividual.szMobilePhone));
						if (DataDescribe.wDataSize<=sizeof(ModifyIndividual.szMobilePhone))
						{
							CopyMemory(ModifyIndividual.szMobilePhone,pDataBuffer,DataDescribe.wDataSize);
							ModifyIndividual.szMobilePhone[CountArray(ModifyIndividual.szMobilePhone)-1]=0;
						}
						break;
					}
				case DTP_GP_UI_QQ:				//Q Q 号码
					{
						ASSERT(pDataBuffer!=NULL);
						ASSERT(DataDescribe.wDataSize<=sizeof(ModifyIndividual.szQQ));
						if (DataDescribe.wDataSize<=sizeof(ModifyIndividual.szQQ))
						{
							CopyMemory(ModifyIndividual.szQQ,pDataBuffer,DataDescribe.wDataSize);
							ModifyIndividual.szQQ[CountArray(ModifyIndividual.szQQ)-1]=0;
						}
						break;
					}
				case DTP_GP_UI_EMAIL:			//电子邮件
					{
						ASSERT(pDataBuffer!=NULL);
						ASSERT(DataDescribe.wDataSize<=sizeof(ModifyIndividual.szEMail));
						if (DataDescribe.wDataSize<=sizeof(ModifyIndividual.szEMail))
						{
							CopyMemory(ModifyIndividual.szEMail,pDataBuffer,DataDescribe.wDataSize);
							ModifyIndividual.szEMail[CountArray(ModifyIndividual.szEMail)-1]=0;
						}
						break;
					}
				case DTP_GP_UI_DWELLING_PLACE:	//联系地址
					{
						ASSERT(pDataBuffer!=NULL);
						ASSERT(DataDescribe.wDataSize<=sizeof(ModifyIndividual.szDwellingPlace));
						if (DataDescribe.wDataSize<=sizeof(ModifyIndividual.szDwellingPlace))
						{
							CopyMemory(ModifyIndividual.szDwellingPlace,pDataBuffer,DataDescribe.wDataSize);
							ModifyIndividual.szDwellingPlace[CountArray(ModifyIndividual.szDwellingPlace)-1]=0;
						}
						break;
					}
				case DTP_GP_UI_SPREADER:	//推广标识
					{
						ASSERT(pDataBuffer!=NULL);
						ASSERT(DataDescribe.wDataSize<=sizeof(ModifyIndividual.szSpreader));
						if (DataDescribe.wDataSize<=sizeof(ModifyIndividual.szSpreader))
						{
							CopyMemory(ModifyIndividual.szSpreader,pDataBuffer,DataDescribe.wDataSize);
							ModifyIndividual.szSpreader[CountArray(ModifyIndividual.szSpreader)-1]=0;
						}
						break;
					}
				}
			}

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_MODIFY_INDIVIDUAL,dwSocketID,&ModifyIndividual,sizeof(ModifyIndividual));

			return true;
		}
	case SUB_GP_USER_ENABLE_INSURE:		  //开通银行
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_UserEnableInsure));
			if (wDataSize!=sizeof(CMD_GP_UserEnableInsure)) return false;

			//处理消息
			CMD_GP_UserEnableInsure * pUserEnableInsure=(CMD_GP_UserEnableInsure *)pData;
			pUserEnableInsure->szLogonPass[CountArray(pUserEnableInsure->szLogonPass)-1]=0;
			pUserEnableInsure->szInsurePass[CountArray(pUserEnableInsure->szInsurePass)-1]=0;
			pUserEnableInsure->szMachineID[CountArray(pUserEnableInsure->szMachineID)-1]=0;

			//变量定义
			DBR_GP_UserEnableInsure UserEnableInsure;
			ZeroMemory(&UserEnableInsure,sizeof(UserEnableInsure));

			//构造数据
			UserEnableInsure.dwUserID=pUserEnableInsure->dwUserID;			
			UserEnableInsure.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(UserEnableInsure.szLogonPass,pUserEnableInsure->szLogonPass,CountArray(UserEnableInsure.szLogonPass));
			lstrcpyn(UserEnableInsure.szInsurePass,pUserEnableInsure->szInsurePass,CountArray(UserEnableInsure.szInsurePass));
			lstrcpyn(UserEnableInsure.szMachineID,pUserEnableInsure->szMachineID,CountArray(UserEnableInsure.szMachineID));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_USER_ENABLE_INSURE,dwSocketID,&UserEnableInsure,sizeof(UserEnableInsure));

			return true;
		}
	case SUB_GP_USER_SAVE_SCORE:		  //存入游戏币
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_UserSaveScore));
			if (wDataSize!=sizeof(CMD_GP_UserSaveScore)) return false;

			//处理消息
			CMD_GP_UserSaveScore * pUserSaveScore=(CMD_GP_UserSaveScore *)pData;
			pUserSaveScore->szMachineID[CountArray(pUserSaveScore->szMachineID)-1]=0;

			//变量定义
			DBR_GP_UserSaveScore UserSaveScore;
			ZeroMemory(&UserSaveScore,sizeof(UserSaveScore));

			//构造数据
			UserSaveScore.dwUserID=pUserSaveScore->dwUserID;
			UserSaveScore.lSaveScore=pUserSaveScore->lSaveScore;
			UserSaveScore.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(UserSaveScore.szMachineID,pUserSaveScore->szMachineID,CountArray(UserSaveScore.szMachineID));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_USER_SAVE_SCORE,dwSocketID,&UserSaveScore,sizeof(UserSaveScore));

			return true;
		}
	case SUB_GP_USER_TAKE_SCORE:		  //取出游戏币
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_UserTakeScore));
			if (wDataSize!=sizeof(CMD_GP_UserTakeScore)) return false;

			//处理消息
			CMD_GP_UserTakeScore * pUserTakeScore=(CMD_GP_UserTakeScore *)pData;
			pUserTakeScore->szPassword[CountArray(pUserTakeScore->szPassword)-1]=0;
			pUserTakeScore->szMachineID[CountArray(pUserTakeScore->szMachineID)-1]=0;

			//变量定义
			DBR_GP_UserTakeScore UserTakeScore;
			ZeroMemory(&UserTakeScore,sizeof(UserTakeScore));

			//构造数据
			UserTakeScore.dwUserID=pUserTakeScore->dwUserID;
			UserTakeScore.lTakeScore=pUserTakeScore->lTakeScore;
			UserTakeScore.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(UserTakeScore.szPassword,pUserTakeScore->szPassword,CountArray(UserTakeScore.szPassword));
			lstrcpyn(UserTakeScore.szMachineID,pUserTakeScore->szMachineID,CountArray(UserTakeScore.szMachineID));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_USER_TAKE_SCORE,dwSocketID,&UserTakeScore,sizeof(UserTakeScore));

			return true;
		}
	case SUB_GP_USER_TRANSFER_SCORE:	  //转帐游戏币
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_UserTransferScore));
			if (wDataSize!=sizeof(CMD_GP_UserTransferScore)) return false;
			
			//处理消息
			CMD_GP_UserTransferScore * pUserTransferScore=(CMD_GP_UserTransferScore *)pData;			
			if (pUserTransferScore->dwGameID == 0 || pUserTransferScore->dwUserID == 0) return false;

			pUserTransferScore->szPassword[CountArray(pUserTransferScore->szPassword)-1]=0;
			pUserTransferScore->szMachineID[CountArray(pUserTransferScore->szMachineID)-1]=0;

			//变量定义
			DBR_GP_UserTransferScore UserTransferScore;
			ZeroMemory(&UserTransferScore,sizeof(UserTransferScore));

			//构造数据
			UserTransferScore.dwUserID=pUserTransferScore->dwUserID;
			UserTransferScore.lTransferScore=pUserTransferScore->lTransferScore;
			UserTransferScore.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			UserTransferScore.dwGameID = pUserTransferScore->dwGameID;
			lstrcpyn(UserTransferScore.szPassword,pUserTransferScore->szPassword,CountArray(UserTransferScore.szPassword));
			lstrcpyn(UserTransferScore.szMachineID,pUserTransferScore->szMachineID,CountArray(UserTransferScore.szMachineID));
			lstrcpyn(UserTransferScore.szTransRemark,pUserTransferScore->szTransRemark,CountArray(UserTransferScore.szTransRemark));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_USER_TRANSFER_SCORE,dwSocketID,&UserTransferScore,sizeof(UserTransferScore));

			return true;
		}
	case SUB_GP_QUERY_INSURE_INFO:		  //查询银行
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_QueryInsureInfo));
			if (wDataSize!=sizeof(CMD_GP_QueryInsureInfo)) return false;

			//处理消息
			CMD_GP_QueryInsureInfo * pQueryInsureInfo=(CMD_GP_QueryInsureInfo *)pData;

			//变量定义
			DBR_GP_QueryInsureInfo QueryInsureInfo;
			ZeroMemory(&QueryInsureInfo,sizeof(QueryInsureInfo));

			//构造数据
			QueryInsureInfo.dwUserID=pQueryInsureInfo->dwUserID;
			QueryInsureInfo.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(QueryInsureInfo.szPassword,pQueryInsureInfo->szPassword,CountArray(QueryInsureInfo.szPassword));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_QUERY_INSURE_INFO,dwSocketID,&QueryInsureInfo,sizeof(QueryInsureInfo));

			return true;
		}
	case SUB_GP_QUERY_USER_INFO_REQUEST:  //查询用户
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_QueryUserInfoRequest));
			if (wDataSize!=sizeof(CMD_GP_QueryUserInfoRequest)) return false;

			//处理消息
			CMD_GP_QueryUserInfoRequest * pQueryUserInfoRequest=(CMD_GP_QueryUserInfoRequest *)pData;

			//ID判断
			if(pQueryUserInfoRequest->cbByNickName==FALSE)
			{
				//长度判断
				int nLen=lstrlen(pQueryUserInfoRequest->szAccounts);
				if(nLen>=8)
				{
					SendInsureFailure(dwSocketID,0,TEXT("请输入合法的玩家ID！"));
					return true;
				}

				//合法判断
				for(int i=0; i<nLen; i++)
				{
					if(pQueryUserInfoRequest->szAccounts[i] < TEXT('0') || pQueryUserInfoRequest->szAccounts[i] > TEXT('9'))
					{
						SendInsureFailure(dwSocketID,0,TEXT("请输入合法的玩家ID！"));
						return true;
					}
				}
			}

			//变量定义
			DBR_GP_QueryInsureUserInfo QueryInsureUserInfo;
			ZeroMemory(&QueryInsureUserInfo,sizeof(QueryInsureUserInfo));

			//构造数据
			QueryInsureUserInfo.cbByNickName=pQueryUserInfoRequest->cbByNickName;
			lstrcpyn(QueryInsureUserInfo.szAccounts,pQueryUserInfoRequest->szAccounts,CountArray(QueryInsureUserInfo.szAccounts));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_QUERY_USER_INFO,dwSocketID,&QueryInsureUserInfo,sizeof(QueryInsureUserInfo));

			return true;
		}
	case SUB_GP_QUERY_TRANSFER_REBATE:		  //查询返利
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_QueryTransferRebate));
			if(wDataSize!=sizeof(CMD_GP_QueryTransferRebate)) return false;

			//提取数据
			CMD_GP_QueryTransferRebate *pRebate = (CMD_GP_QueryTransferRebate *)pData;
			pRebate->szPassword[CountArray(pRebate->szPassword)-1]=0;

			//构造结构
			DBR_GP_QueryTransferRebate Rebate;
			Rebate.dwUserID = pRebate->dwUserID;
			lstrcpyn(Rebate.szPassword,pRebate->szPassword,CountArray(Rebate.szPassword));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_QUERY_TRANSFER_REBATE,dwSocketID,&Rebate,sizeof(Rebate));

			return true;
		}
	case SUB_GP_CHECKIN_QUERY:		  //查询签到
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_CheckInQueryInfo));
			if(wDataSize!=sizeof(CMD_GP_CheckInQueryInfo)) return false;

			//提取数据
			CMD_GP_CheckInQueryInfo *pCheckInQueryInfo = (CMD_GP_CheckInQueryInfo *)pData;
			pCheckInQueryInfo->szPassword[CountArray(pCheckInQueryInfo->szPassword)-1]=0;

			//构造结构
			DBR_GP_CheckInQueryInfo CheckInQueryInfo;
			CheckInQueryInfo.dwUserID = pCheckInQueryInfo->dwUserID;
			lstrcpyn(CheckInQueryInfo.szPassword,pCheckInQueryInfo->szPassword,CountArray(CheckInQueryInfo.szPassword));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_CHECKIN_QUERY_INFO,dwSocketID,&CheckInQueryInfo,sizeof(CheckInQueryInfo));

			return true;
		}
	case SUB_GP_CHECKIN_DONE:			  //执行签到
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_CheckInDone));
			if(wDataSize!=sizeof(CMD_GP_CheckInDone)) return false;

			//提取数据
			CMD_GP_CheckInDone *pCheckInDone = (CMD_GP_CheckInDone *)pData;
			pCheckInDone->szPassword[CountArray(pCheckInDone->szPassword)-1]=0;
			pCheckInDone->szMachineID[CountArray(pCheckInDone->szMachineID)-1]=0;

			//构造结构
			DBR_GP_CheckInDone CheckInDone;
			CheckInDone.dwUserID = pCheckInDone->dwUserID;
			CheckInDone.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(CheckInDone.szPassword,pCheckInDone->szPassword,CountArray(CheckInDone.szPassword));
			lstrcpyn(CheckInDone.szMachineID,pCheckInDone->szMachineID,CountArray(CheckInDone.szMachineID));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_CHECKIN_DONE,dwSocketID,&CheckInDone,sizeof(CheckInDone));

			return true;
		}
	case SUB_GP_TASK_LOAD:				//加载任务
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_TaskLoadInfo));
			if(wDataSize!=sizeof(CMD_GP_TaskLoadInfo)) return false;

			//提取数据
			CMD_GP_TaskLoadInfo * pTaskLoadInfo = (CMD_GP_TaskLoadInfo *)pData;
			pTaskLoadInfo->szPassword[CountArray(pTaskLoadInfo->szPassword)-1]=0;

			//构造结构
			DBR_GP_TaskQueryInfo TaskQueryInfo;
			TaskQueryInfo.dwUserID = pTaskLoadInfo->dwUserID;
			lstrcpyn(TaskQueryInfo.szPassword,pTaskLoadInfo->szPassword,CountArray(TaskQueryInfo.szPassword));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_TASK_QUERY_INFO,dwSocketID,&TaskQueryInfo,sizeof(TaskQueryInfo));

			return true;
		}
	case SUB_GP_TASK_GIVEUP:
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_TaskGiveUp));
			if(wDataSize!=sizeof(CMD_GP_TaskGiveUp)) return false;
			CMD_GP_TaskTake * pTask = (CMD_GP_TaskTake *)pData;
			pTask->szPassword[CountArray(pTask->szPassword)-1]=0;
			pTask->szMachineID[CountArray(pTask->szMachineID)-1]=0;

			//构造结构
			DBR_GP_TaskGiveUP task;
			task.dwUserID = pTask->dwUserID;
			task.wTaskID = pTask->wTaskID;
			task.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(task.szPassword,pTask->szPassword,CountArray(task.szPassword));
			lstrcpyn(task.szMachineID,pTask->szMachineID,CountArray(task.szMachineID));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_TASK_GIVEUP,dwSocketID,&task,sizeof(task));

			return true;
		}
	case SUB_GP_TASK_TAKE:			      //领取任务
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_TaskTake));
			if(wDataSize!=sizeof(CMD_GP_TaskTake)) return false;

			//提取数据
			CMD_GP_TaskTake * pTaskTake = (CMD_GP_TaskTake *)pData;
			pTaskTake->szPassword[CountArray(pTaskTake->szPassword)-1]=0;
			pTaskTake->szMachineID[CountArray(pTaskTake->szMachineID)-1]=0;

			//构造结构
			DBR_GP_TaskTake TaskTake;
			TaskTake.dwUserID = pTaskTake->dwUserID;
			TaskTake.wTaskID = pTaskTake->wTaskID;
			TaskTake.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(TaskTake.szPassword,pTaskTake->szPassword,CountArray(TaskTake.szPassword));
			lstrcpyn(TaskTake.szMachineID,pTaskTake->szMachineID,CountArray(TaskTake.szMachineID));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_TASK_TAKE,dwSocketID,&TaskTake,sizeof(TaskTake));

			return true;
		}
	case SUB_GP_TASK_REWARD:			  //领取奖励
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_TaskReward));
			if(wDataSize!=sizeof(CMD_GP_TaskReward)) return false;

			//提取数据
			CMD_GP_TaskReward * pTaskReward = (CMD_GP_TaskReward *)pData;
			pTaskReward->szPassword[CountArray(pTaskReward->szPassword)-1]=0;
			pTaskReward->szMachineID[CountArray(pTaskReward->szMachineID)-1]=0;

			//构造结构
			DBR_GP_TaskReward TaskReward;
			TaskReward.dwUserID = pTaskReward->dwUserID;
			TaskReward.wTaskID = pTaskReward->wTaskID;
			TaskReward.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(TaskReward.szPassword,pTaskReward->szPassword,CountArray(TaskReward.szPassword));
			lstrcpyn(TaskReward.szMachineID,pTaskReward->szMachineID,CountArray(TaskReward.szMachineID));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_TASK_REWARD,dwSocketID,&TaskReward,sizeof(TaskReward));

			return true;
		}
	case SUB_GP_BASEENSURE_LOAD:		//加载低保
		{
			//构造结构
			CMD_GP_BaseEnsureParamter BaseEnsureParameter;
			BaseEnsureParameter.cbTakeTimes=m_BaseEnsureParameter.cbTakeTimes;
			BaseEnsureParameter.lScoreAmount=m_BaseEnsureParameter.lScoreAmount;
			BaseEnsureParameter.lScoreCondition=m_BaseEnsureParameter.lScoreCondition;

			//投递请求
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_USER_SERVICE,SUB_GP_BASEENSURE_PARAMETER,&BaseEnsureParameter,sizeof(BaseEnsureParameter));

			return true;
		}
	case SUB_GP_BASEENSURE_TAKE:		  //领取低保
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_BaseEnsureTake));
			if(wDataSize!=sizeof(CMD_GP_BaseEnsureTake)) return false;

			//提取数据
			CMD_GP_BaseEnsureTake * pBaseEnsureTake = (CMD_GP_BaseEnsureTake *)pData;
			pBaseEnsureTake->szPassword[CountArray(pBaseEnsureTake->szPassword)-1]=0;
			pBaseEnsureTake->szMachineID[CountArray(pBaseEnsureTake->szMachineID)-1]=0;

			//构造结构
			DBR_GP_TakeBaseEnsure TakeBaseEnsure;
			TakeBaseEnsure.dwUserID = pBaseEnsureTake->dwUserID;
			TakeBaseEnsure.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(TakeBaseEnsure.szPassword,pBaseEnsureTake->szPassword,CountArray(TakeBaseEnsure.szPassword));
			lstrcpyn(TakeBaseEnsure.szMachineID,pBaseEnsureTake->szMachineID,CountArray(TakeBaseEnsure.szMachineID));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_BASEENSURE_TAKE,dwSocketID,&TakeBaseEnsure,sizeof(TakeBaseEnsure));

			return true;
		}
	case SUB_GP_SPREAD_QUERY:		//推广参数
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_UserSpreadQuery));
			if(wDataSize!=sizeof(CMD_GP_UserSpreadQuery)) return false;

			//提取数据
			CMD_GP_UserSpreadQuery * pUserSpreadQuery=(CMD_GP_UserSpreadQuery*)pData;

			//构造结构
			DBR_GP_QuerySpreadInfo QuerySpreadInfo;
			QuerySpreadInfo.dwUserID=pUserSpreadQuery->dwUserID;

			//推广奖励
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_QUERY_SPREAD_INFO,dwSocketID,&QuerySpreadInfo,sizeof(QuerySpreadInfo));

			return true;
		}
	case SUB_GP_GROWLEVEL_QUERY:		  //查询等级
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_GrowLevelQueryInfo));
			if(wDataSize!=sizeof(CMD_GP_GrowLevelQueryInfo)) return false;

			//提取数据
			CMD_GP_GrowLevelQueryInfo * pGrowLevelQueryInfo= (CMD_GP_GrowLevelQueryInfo *)pData;
			pGrowLevelQueryInfo->szPassword[CountArray(pGrowLevelQueryInfo->szPassword)-1]=0;
			pGrowLevelQueryInfo->szMachineID[CountArray(pGrowLevelQueryInfo->szMachineID)-1]=0;

			//构造结构
			DBR_GP_GrowLevelQueryInfo GrowLevelQueryInfo;
			GrowLevelQueryInfo.dwUserID = pGrowLevelQueryInfo->dwUserID;
			GrowLevelQueryInfo.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(GrowLevelQueryInfo.szPassword,pGrowLevelQueryInfo->szPassword,CountArray(GrowLevelQueryInfo.szPassword));
			lstrcpyn(GrowLevelQueryInfo.szMachineID,pGrowLevelQueryInfo->szMachineID,CountArray(GrowLevelQueryInfo.szMachineID));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_GROWLEVEL_QUERY_IFNO,dwSocketID,&GrowLevelQueryInfo,sizeof(GrowLevelQueryInfo));

			return true;
		}
	case SUB_GP_MEMBER_PARAMETER:		//加载会员
		{
			//构造结构
			CMD_GP_MemberParameterResult MemberParameter;

			MemberParameter.wMemberCount = m_wMemberCount;
			CopyMemory(MemberParameter.MemberParameter,m_MemberParameter,sizeof(tagMemberParameterNew)*m_wMemberCount);

			//投递请求
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_USER_SERVICE,SUB_GP_MEMBER_PARAMETER_RESULT,&MemberParameter,sizeof(MemberParameter));

			return true;
		}
	case SUB_GP_MEMBER_QUERY_INFO:		//会员查询
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_MemberQueryInfo));
			if(wDataSize!=sizeof(CMD_GP_MemberQueryInfo)) return false;

			//提取数据
			CMD_GP_MemberQueryInfo * pMemberInfo= (CMD_GP_MemberQueryInfo *)pData;
			pMemberInfo->szPassword[CountArray(pMemberInfo->szPassword)-1]=0;
			pMemberInfo->szMachineID[CountArray(pMemberInfo->szMachineID)-1]=0;

			//构造结构
			DBR_GP_MemberQueryInfo MemberInfo;
			MemberInfo.dwUserID = pMemberInfo->dwUserID;
			MemberInfo.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(MemberInfo.szPassword,pMemberInfo->szPassword,CountArray(MemberInfo.szPassword));
			lstrcpyn(MemberInfo.szMachineID,pMemberInfo->szMachineID,CountArray(MemberInfo.szMachineID));

			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_MEMBER_QUERY_INFO,dwSocketID,&MemberInfo,sizeof(MemberInfo));

			return true;
		}
	case SUB_GP_MEMBER_DAY_PRESENT:		//会员送金
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_MemberDayPresent));
			if(wDataSize!=sizeof(CMD_GP_MemberDayPresent)) return false;

			//提取数据
			CMD_GP_MemberDayPresent * pMemberInfo= (CMD_GP_MemberDayPresent *)pData;
			pMemberInfo->szPassword[CountArray(pMemberInfo->szPassword)-1]=0;
			pMemberInfo->szMachineID[CountArray(pMemberInfo->szMachineID)-1]=0;

			//构造结构
			DBR_GP_MemberDayPresent MemberInfo;
			MemberInfo.dwUserID = pMemberInfo->dwUserID;
			MemberInfo.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(MemberInfo.szPassword,pMemberInfo->szPassword,CountArray(MemberInfo.szPassword));
			lstrcpyn(MemberInfo.szMachineID,pMemberInfo->szMachineID,CountArray(MemberInfo.szMachineID));

			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_MEMBER_DAY_PRESENT,dwSocketID,&MemberInfo,sizeof(MemberInfo));

			return true;
		}
	case SUB_GP_MEMBER_DAY_GIFT:		//会员礼包
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_MemberDayGift));
			if(wDataSize!=sizeof(CMD_GP_MemberDayGift)) return false;

			//提取数据
			CMD_GP_MemberDayGift * pMemberInfo= (CMD_GP_MemberDayGift *)pData;
			pMemberInfo->szPassword[CountArray(pMemberInfo->szPassword)-1]=0;
			pMemberInfo->szMachineID[CountArray(pMemberInfo->szMachineID)-1]=0;

			//构造结构
			DBR_GP_MemberDayGift MemberInfo;
			MemberInfo.dwUserID = pMemberInfo->dwUserID;
			MemberInfo.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(MemberInfo.szPassword,pMemberInfo->szPassword,CountArray(MemberInfo.szPassword));
			lstrcpyn(MemberInfo.szMachineID,pMemberInfo->szMachineID,CountArray(MemberInfo.szMachineID));

			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_MEMBER_DAY_GIFT,dwSocketID,&MemberInfo,sizeof(MemberInfo));

			return true;
		}
	case SUB_GP_EXCHANGE_QUERY:		//兑换参数
		{
			//构造结构
			CMD_GP_ExchangeParameter ExchangeParameter;
			ZeroMemory(&ExchangeParameter,sizeof(ExchangeParameter));

			//设置变量
			ExchangeParameter.dwExchangeRate=m_PlatformParameter.dwExchangeRate;
			ExchangeParameter.wMemberCount=m_wMemberCount;
			ExchangeParameter.dwPresentExchangeRate=m_PlatformParameter.dwPresentExchangeRate;
			ExchangeParameter.dwRateGold=m_PlatformParameter.dwRateGold;
			CopyMemory(ExchangeParameter.MemberParameter,m_MemberParameter,sizeof(tagMemberParameter)*m_wMemberCount);

			//计算大小
			WORD wSendDataSize = sizeof(ExchangeParameter)-sizeof(ExchangeParameter.MemberParameter);
			wSendDataSize += sizeof(tagMemberParameter)*ExchangeParameter.wMemberCount;

			//发送数据
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_USER_SERVICE,SUB_GP_EXCHANGE_PARAMETER,&ExchangeParameter,wSendDataSize);

			return true;
		}
	case SUB_GP_PURCHASE_MEMBER:	//购买会员
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_PurchaseMember));
			if(wDataSize!=sizeof(CMD_GP_PurchaseMember)) return false;

			//构造结构
			CMD_GP_PurchaseMember * pPurchaseMember = (CMD_GP_PurchaseMember*)pData;
			pPurchaseMember->szMachineID[CountArray(pPurchaseMember->szMachineID)-1]=0;

			//构造结构
			DBR_GP_PurchaseMember PurchaseMember;
			ZeroMemory(&PurchaseMember,sizeof(PurchaseMember));

			//设置变量
			PurchaseMember.dwUserID=pPurchaseMember->dwUserID;
			PurchaseMember.cbMemberOrder=pPurchaseMember->cbMemberOrder;
			PurchaseMember.wPurchaseTime=pPurchaseMember->wPurchaseTime;
			PurchaseMember.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(PurchaseMember.szMachineID,pPurchaseMember->szMachineID,CountArray(PurchaseMember.szMachineID));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_PURCHASE_MEMBER,dwSocketID,&PurchaseMember,sizeof(PurchaseMember));

			return true;
		}
	case SUB_GP_EXCHANGE_SCORE_BYINGOT:		//兑换游戏币
		{
			//参数校验
			//ASSERT(wDataSize==sizeof(CMD_GP_ExchangeScoreByIngot));
			//if(wDataSize!=sizeof(CMD_GP_ExchangeScoreByIngot)) return false;

			////构造结构
			//CMD_GP_ExchangeScoreByIngot * pExchangeScore = (CMD_GP_ExchangeScoreByIngot*)pData;
			//pExchangeScore->szMachineID[CountArray(pExchangeScore->szMachineID)-1]=0;

			////构造结构
			//DBR_GP_ExchangeScoreByIngot ExchangeScore;
			//ZeroMemory(&ExchangeScore,sizeof(ExchangeScore));

			////设置变量
			//ExchangeScore.dwUserID=pExchangeScore->dwUserID;
			//ExchangeScore.lExchangeIngot=pExchangeScore->lExchangeIngot;
			//ExchangeScore.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			//lstrcpyn(ExchangeScore.szMachineID,pExchangeScore->szMachineID,CountArray(ExchangeScore.szMachineID));

			////投递请求
			//m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_EXCHANGE_SCORE_INGOT,dwSocketID,&ExchangeScore,sizeof(ExchangeScore));

			return true;
		}
	case SUB_GP_EXCHANGE_SCORE_BYBEANS:		//兑换游戏币
		{
			//参数校验
			//ASSERT(wDataSize==sizeof(CMD_GP_ExchangeScoreByBeans));
			//if(wDataSize!=sizeof(CMD_GP_ExchangeScoreByBeans)) return false;

			////构造结构
			//CMD_GP_ExchangeScoreByBeans * pExchangeScore = (CMD_GP_ExchangeScoreByBeans*)pData;
			//pExchangeScore->szMachineID[CountArray(pExchangeScore->szMachineID)-1]=0;

			////构造结构
			//DBR_GP_ExchangeScoreByBeans ExchangeScore;
			//ZeroMemory(&ExchangeScore,sizeof(ExchangeScore));

			////设置变量
			//ExchangeScore.dwUserID=pExchangeScore->dwUserID;
			//ExchangeScore.dExchangeBeans=pExchangeScore->dExchangeBeans;
			//ExchangeScore.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			//lstrcpyn(ExchangeScore.szMachineID,pExchangeScore->szMachineID,CountArray(ExchangeScore.szMachineID));

			////投递请求
			//m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_EXCHANGE_SCORE_BEANS,dwSocketID,&ExchangeScore,sizeof(ExchangeScore));

			return true;
		}
	case SUB_GP_LOTTERY_CONFIG_REQ:			//请求配置
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_LotteryConfigReq));
			if(wDataSize!=sizeof(CMD_GP_LotteryConfigReq)) return false;

			//构造结构
			CMD_GP_LotteryConfigReq * pLotteryConfigReq = (CMD_GP_LotteryConfigReq*)pData;
			pLotteryConfigReq->szLogonPass[CountArray(pLotteryConfigReq->szLogonPass)-1]=0;

			//构造结构
			DBR_GP_LotteryConfigReq LotteryConfigReq;
			ZeroMemory(&LotteryConfigReq,sizeof(LotteryConfigReq));

			//设置变量
			LotteryConfigReq.dwUserID=pLotteryConfigReq->dwUserID;
			LotteryConfigReq.wKindID=pLotteryConfigReq->wKindID;
			LotteryConfigReq.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(LotteryConfigReq.szLogonPass,pLotteryConfigReq->szLogonPass,CountArray(LotteryConfigReq.szLogonPass));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOTTERY_CONFIG_REQ,dwSocketID,&LotteryConfigReq,sizeof(LotteryConfigReq));

			return true;
		}
	case SUB_GP_LOTTERY_START:				//抽奖开始
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_LotteryStart));
			if(wDataSize!=sizeof(CMD_GP_LotteryStart)) return false;

			//构造结构
			CMD_GP_LotteryStart * pLotteryStart = (CMD_GP_LotteryStart*)pData;
			pLotteryStart->szLogonPass[CountArray(pLotteryStart->szLogonPass)-1]=0;
			pLotteryStart->szMachineID[CountArray(pLotteryStart->szMachineID)-1]=0;

			//构造结构
			DBR_GP_LotteryStart LotteryStart;
			ZeroMemory(&LotteryStart,sizeof(LotteryStart));

			//设置变量
			LotteryStart.dwUserID=pLotteryStart->dwUserID;
			LotteryStart.wKindID=pLotteryStart->wKindID;
			LotteryStart.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(LotteryStart.szLogonPass,pLotteryStart->szLogonPass,CountArray(LotteryStart.szLogonPass));
			lstrcpyn(LotteryStart.szMachineID,pLotteryStart->szMachineID,CountArray(LotteryStart.szMachineID));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOTTERY_START,dwSocketID,&LotteryStart,sizeof(LotteryStart));

			return true;
		}
	case SUB_GP_QUERY_USER_GAME_DATA:		//游戏数据
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_QueryUserGameData));
			if(wDataSize!=sizeof(CMD_GP_QueryUserGameData)) return false;

			//构造结构
			CMD_GP_QueryUserGameData * pQueryUserGameData = (CMD_GP_QueryUserGameData*)pData;
			pQueryUserGameData->szDynamicPass[CountArray(pQueryUserGameData->szDynamicPass)-1]=0;

			//构造结构
			DBR_GP_QueryUserGameData QueryUserGameData;
			ZeroMemory(&QueryUserGameData,sizeof(QueryUserGameData));

			//设置变量
			QueryUserGameData.dwUserID=pQueryUserGameData->dwUserID;
			QueryUserGameData.wKindID=pQueryUserGameData->wKindID;
			lstrcpyn(QueryUserGameData.szDynamicPass,pQueryUserGameData->szDynamicPass,CountArray(QueryUserGameData.szDynamicPass));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_QUERY_USER_GAME_DATA,dwSocketID,&QueryUserGameData,sizeof(QueryUserGameData));

			return true;
		}
	case SUB_GP_ACCOUNT_BINDING:
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_AccountBind));
			if (wDataSize!=sizeof(CMD_GP_AccountBind)) return false;

			//处理消息
			CMD_GP_AccountBind * pAccountBind=(CMD_GP_AccountBind *)pData;
			pAccountBind->szPassword[CountArray(pAccountBind->szPassword)-1]=0;
			pAccountBind->szBindNewAccounts[CountArray(pAccountBind->szBindNewAccounts)-1]=0;
			pAccountBind->szBindNewPassword[CountArray(pAccountBind->szBindNewPassword)-1]=0;
			pAccountBind->szBindNewSpreader[CountArray(pAccountBind->szBindNewSpreader)-1]=0;
			

			//变量定义
			DBR_GP_AccountBind AccountBind;
			ZeroMemory(&AccountBind,sizeof(AccountBind));

			//构造数据
			AccountBind.dwUserID = pAccountBind->dwUserID;
			AccountBind.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			AccountBind.cbDeviceType = pAccountBind->cbDeviceType;

			lstrcpyn(AccountBind.szPassword,pAccountBind->szPassword,CountArray(AccountBind.szPassword));
			lstrcpyn(AccountBind.szMachineID,pAccountBind->szMachineID,CountArray(AccountBind.szMachineID));
			
			lstrcpyn(AccountBind.szBindNewAccounts,pAccountBind->szBindNewAccounts,CountArray(AccountBind.szBindNewAccounts));
			lstrcpyn(AccountBind.szBindNewPassword,pAccountBind->szBindNewPassword,CountArray(AccountBind.szBindNewPassword));
			lstrcpyn(AccountBind.szBindNewSpreader,pAccountBind->szBindNewSpreader,CountArray(AccountBind.szBindNewSpreader));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_ACCOUNT_BIND,dwSocketID,&AccountBind,sizeof(AccountBind));

			return true;
		}
	case SUB_GP_ACCOUNT_BINDING_EXISTS:
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_AccountBind_Exists));
			if (wDataSize!=sizeof(CMD_GP_AccountBind_Exists)) return false;

			//处理消息
			CMD_GP_AccountBind_Exists * pAccountBind=(CMD_GP_AccountBind_Exists *)pData;
			pAccountBind->szPassword[CountArray(pAccountBind->szPassword)-1]=0;
			pAccountBind->szBindExistsAccounts[CountArray(pAccountBind->szBindExistsAccounts)-1]=0;
			pAccountBind->szBindExistsPassword[CountArray(pAccountBind->szBindExistsPassword)-1]=0;


			//变量定义
			DBR_GP_AccountBind_Exists AccountBind;
			ZeroMemory(&AccountBind,sizeof(AccountBind));

			//构造数据
			AccountBind.dwUserID = pAccountBind->dwUserID;
			AccountBind.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(AccountBind.szPassword,pAccountBind->szPassword,CountArray(AccountBind.szPassword));
			lstrcpyn(AccountBind.szMachineID,pAccountBind->szMachineID,CountArray(AccountBind.szMachineID));

			lstrcpyn(AccountBind.szBindExistsAccounts,pAccountBind->szBindExistsAccounts,CountArray(AccountBind.szBindExistsAccounts));
			lstrcpyn(AccountBind.szBindExistsPassword,pAccountBind->szBindExistsPassword,CountArray(AccountBind.szBindExistsPassword));


			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_ACCOUNT_BIND_EXISTS,dwSocketID,&AccountBind,sizeof(AccountBind));
			return true;
		}
	}

	return false;
}

//远程处理
bool CAttemperEngineSink::OnTCPNetworkMainPCRemoteService(WORD wSubCmdID, VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	switch (wSubCmdID)
	{
	case SUB_GP_C_SEARCH_CORRESPOND:	//协调查找
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_C_SearchCorrespond));
			if (wDataSize!=sizeof(CMD_GP_C_SearchCorrespond)) return false;

			//处理消息
			CMD_GP_C_SearchCorrespond * pSearchCorrespond=(CMD_GP_C_SearchCorrespond *)pData;
			pSearchCorrespond->szNickName[CountArray(pSearchCorrespond->szNickName)-1]=0;

			//变量定义
			CMD_CS_C_SearchCorrespond SearchCorrespond;
			ZeroMemory(&SearchCorrespond,sizeof(SearchCorrespond));

			//连接变量
			SearchCorrespond.dwSocketID=dwSocketID;
			SearchCorrespond.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;

			//查找变量
			SearchCorrespond.dwGameID=pSearchCorrespond->dwGameID;
			lstrcpyn(SearchCorrespond.szNickName,pSearchCorrespond->szNickName,CountArray(SearchCorrespond.szNickName));

			//发送数据
			m_pITCPSocketService->SendData(MDM_CS_REMOTE_SERVICE,SUB_CS_C_SEARCH_CORRESPOND,&SearchCorrespond,sizeof(SearchCorrespond));

			return true;
		}
	case SUB_GP_C_SEARCH_ALLCORRESPOND:
		{
			//发送数据
			if(wDataSize >= sizeof(CMD_GP_C_SearchAllCorrespond)) 
			{
				CMD_GP_C_SearchAllCorrespond* pSearchAllCorrespond = (CMD_GP_C_SearchAllCorrespond*)pData;

				DWORD cBuffer[512]={0};
				CMD_CS_C_AllSearchCorrespond* pSearchCorrespond = (CMD_CS_C_AllSearchCorrespond*)cBuffer;

				//连接变量
				pSearchCorrespond->dwSocketID=dwSocketID;
				pSearchCorrespond->dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;

				pSearchCorrespond->dwCount = pSearchAllCorrespond->dwCount;
				memcpy(pSearchCorrespond->dwGameID, pSearchAllCorrespond->dwGameID, sizeof(DWORD)*pSearchAllCorrespond->dwCount);

				m_pITCPSocketService->SendData(MDM_CS_REMOTE_SERVICE,SUB_CS_C_SEARCH_ALLCORRESPOND,pSearchCorrespond, sizeof(CMD_CS_C_AllSearchCorrespond)+sizeof(DWORD)*(WORD)(pSearchAllCorrespond->dwCount-1));
			}
			return true;
		}
	}

	return false;
}


//机器服务
bool CAttemperEngineSink::OnTCPNetworkMainAndroidService(WORD wSubCmdID, VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	switch (wSubCmdID)
	{
	case SUB_GP_GET_PARAMETER:		//获取参数
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_GetParameter));
			if(wDataSize!=sizeof(CMD_GP_GetParameter)) return false;

			//提取数据
			CMD_GP_GetParameter * pGetParameter = (CMD_GP_GetParameter *)pData;
			ASSERT(pGetParameter!=NULL);

			//构造结构
			DBR_GP_GetParameter GetParameter;
			GetParameter.wServerID = pGetParameter->wServerID;

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_GET_PARAMETER,dwSocketID,&GetParameter,sizeof(GetParameter));

			return true;
		}
	case SUB_GP_ADD_PARAMETER:		//添加参数
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_AddParameter));
			if(wDataSize!=sizeof(CMD_GP_AddParameter)) return false;

			//提取数据
			CMD_GP_AddParameter * pAddParameter = (CMD_GP_AddParameter *)pData;
			ASSERT(pAddParameter!=NULL);

			//构造结构
			DBR_GP_AddParameter AddParameter;
			AddParameter.wServerID = pAddParameter->wServerID;
			CopyMemory(&AddParameter.AndroidParameter,&pAddParameter->AndroidParameter,sizeof(tagAndroidParameter));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_ADD_PARAMETER,dwSocketID,&AddParameter,sizeof(AddParameter));

			return true;
		}
	case SUB_GP_MODIFY_PARAMETER:	//修改参数
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_ModifyParameter));
			if(wDataSize!=sizeof(CMD_GP_ModifyParameter)) return false;

			//提取数据
			CMD_GP_ModifyParameter * pModifyParameter = (CMD_GP_ModifyParameter *)pData;
			ASSERT(pModifyParameter!=NULL);

			//构造结构
			DBR_GP_ModifyParameter ModifyParameter;
			ModifyParameter.wServerID=pModifyParameter->wServerID;
			CopyMemory(&ModifyParameter.AndroidParameter,&pModifyParameter->AndroidParameter,sizeof(tagAndroidParameter));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_MODIFY_PARAMETER,dwSocketID,&ModifyParameter,sizeof(ModifyParameter));
			
			return true;
		}
	case SUB_GP_DELETE_PARAMETER:	//删除参数
		{
			//参数校验
			ASSERT(wDataSize==sizeof(CMD_GP_DeleteParameter));
			if(wDataSize!=sizeof(CMD_GP_DeleteParameter)) return false;

			//提取数据
			CMD_GP_DeleteParameter * pDeleteParameter = (CMD_GP_DeleteParameter *)pData;
			ASSERT(pDeleteParameter!=NULL);

			//构造结构
			DBR_GP_DeleteParameter DeleteParameter;
			DeleteParameter.wServerID = pDeleteParameter->wServerID;
			DeleteParameter.dwBatchID = pDeleteParameter->dwBatchID;

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_DELETE_PARAMETER,dwSocketID,&DeleteParameter,sizeof(DeleteParameter));
			
			return true;
		}
	}

	return false;
}

bool CAttemperEngineSink::OnTCPNetworkMainPCProperty(WORD wSubCmdID, VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	switch (wSubCmdID)
	{
	case SUB_GP_QUERY_PROPERTY:			//加载道具
		{

			//发送消息
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_PROPERTY,SUB_GP_QUERY_PROPERTY_RESULT_FINISH);
			return true;
		}
	case SUB_GP_PROPERTY_BUY:	        //道具购买
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_PropertyBuy));
			if (wDataSize!=sizeof(CMD_GP_PropertyBuy)) return false;

			//变量定义
			CMD_GP_PropertyBuy * pPropertyBuy=(CMD_GP_PropertyBuy *)pData;

			//变量定义
			DBR_GP_PropertyBuy PropertyRequest;
			ZeroMemory(&PropertyRequest,sizeof(PropertyRequest));

			//购买信息
			PropertyRequest.dwUserID=pPropertyBuy->dwUserID;
			PropertyRequest.dwPropertyID=pPropertyBuy->dwPropertyID;
			PropertyRequest.dwDiamondCount = pPropertyBuy->dwDiamondCount;			

			//系统信息
			PropertyRequest.dwClientAddr=(m_pBindParameter + LOWORD(dwSocketID))->dwClientAddr;
			lstrcpyn(PropertyRequest.szPassword,pPropertyBuy->szPassword,CountArray(PropertyRequest.szPassword));
			lstrcpyn(PropertyRequest.szMachineID,pPropertyBuy->szMachineID,CountArray(PropertyRequest.szMachineID));

			//投递数据
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_PROPERTY_BUY,dwSocketID,&PropertyRequest,sizeof(PropertyRequest));

			return true;
		}
	case SUB_GP_PROPERTY_USE:			//道具使用
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_C_PropertyUse));
			if (wDataSize!=sizeof(CMD_GP_C_PropertyUse)) return false;

			//变量定义
			CMD_GP_C_PropertyUse * pPropertyUse = (CMD_GP_C_PropertyUse *)pData;

			//数据效验
			ASSERT(pPropertyUse->wPropCount>0);
			if (pPropertyUse->wPropCount==0) return false;

			//变量定义
			DBR_GP_PropertyUse PropertyUseRequest;
			ZeroMemory(&PropertyUseRequest,sizeof(PropertyUseRequest));
			PropertyUseRequest.dwUserID = pPropertyUse->dwUserID;
			PropertyUseRequest.dwRecvUserID = pPropertyUse->dwRecvUserID;
			PropertyUseRequest.wPropCount = pPropertyUse->wPropCount;
			PropertyUseRequest.dwPropID = pPropertyUse->dwPropID;
			PropertyUseRequest.dwClientAddr=(m_pBindParameter + LOWORD(dwSocketID))->dwClientAddr; //系统信息

			//投递数据
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_PROPERTY_USE, dwSocketID, &PropertyUseRequest, sizeof(PropertyUseRequest));

			return true;
		}
	case SUB_GP_QUERY_SINGLE:
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_PropertyQuerySingle));
			if (wDataSize!=sizeof(CMD_GP_PropertyQuerySingle)) return false;

			CMD_GP_PropertyQuerySingle* pQuerySingle = (CMD_GP_PropertyQuerySingle*)pData;

			DBR_GP_PropertyQuerySingle QuerySingle;
			QuerySingle.dwUserID = pQuerySingle->dwUserID;
			QuerySingle.dwPropertyID = pQuerySingle->dwPropertyID;
			lstrcpyn(QuerySingle.szPassword,pQuerySingle->szPassword,CountArray(QuerySingle.szPassword));

			//投递数据
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_QUERY_SINGLE,dwSocketID,&QuerySingle,sizeof(QuerySingle));

			return true;
		}
	case SUB_GP_QUERY_BACKPACKET:  //查询背包
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_C_BackpackProperty));
			if (wDataSize!=sizeof(CMD_GP_C_BackpackProperty)) return false;

			CMD_GP_C_BackpackProperty* pBackpackProperty = (CMD_GP_C_BackpackProperty*)pData;

			DBR_GP_QueryBackpack QueryBackpack;
			QueryBackpack.dwUserID = pBackpackProperty->dwUserID;
			QueryBackpack.dwKindID = 0;
			QueryBackpack.dwClientAddr = (m_pBindParameter + LOWORD(dwSocketID))->dwClientAddr;

			//投递数据
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_QUERY_BACKPACK,dwSocketID,&QueryBackpack,sizeof(QueryBackpack));
			return true;
		}
	case SUB_GP_PROPERTY_BUFF:			//道具Buff
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_C_UserPropertyBuff));
			if (wDataSize!=sizeof(CMD_GP_C_UserPropertyBuff)) return false;

			//变量定义
			CMD_GP_C_UserPropertyBuff * pPropertyBuff = (CMD_GP_C_UserPropertyBuff *)pData;

			//变量定义
			DBR_GP_UserPropertyBuff PropertyBuffRequest;
			ZeroMemory(&PropertyBuffRequest,sizeof(PropertyBuffRequest));
			PropertyBuffRequest.dwUserID = pPropertyBuff->dwUserID;

			//投递数据
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_USER_PROPERTY_BUFF, dwSocketID, &PropertyBuffRequest, sizeof(PropertyBuffRequest));

			return true;
		}
	case SUB_GP_QUERY_SEND_PRESENT:		//查询赠送
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_QuerySendPresent));
			if (wDataSize!=sizeof(CMD_GP_QuerySendPresent)) return false;

			//变量定义
			CMD_GP_QuerySendPresent *pQuerySendPresent = (CMD_GP_QuerySendPresent *)pData;
			DBR_GP_QuerySendPresent QuerySendPresentRequest={0};
			QuerySendPresentRequest.dwUserID = pQuerySendPresent->dwUserID;
			QuerySendPresentRequest.dwClientAddr=(m_pBindParameter + LOWORD(dwSocketID))->dwClientAddr; //系统信息

			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_QUERY_SEND_PRESENT, dwSocketID, &QuerySendPresentRequest, sizeof(QuerySendPresentRequest));
			return true;
		}
	case SUB_GP_PROPERTY_PRESENT:		//道具赠送
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_C_PropertyPresent));
			if (wDataSize!=sizeof(CMD_GP_C_PropertyPresent)) return false;

			//变量定义
			CMD_GP_C_PropertyPresent *PropertyPresent = (CMD_GP_C_PropertyPresent *)pData;
			DBR_GP_PropertyPresent PropertyPresentRequest={0};
			PropertyPresentRequest.dwUserID = PropertyPresent->dwUserID;
			PropertyPresentRequest.dwRecvGameID = PropertyPresent->dwRecvGameID;
			PropertyPresentRequest.dwPropID = PropertyPresent->dwPropID;
			PropertyPresentRequest.wPropCount = PropertyPresent->wPropCount;
			PropertyPresentRequest.wType = PropertyPresent->wType;
			lstrcpyn(PropertyPresentRequest.szRecvNickName,PropertyPresent->szRecvNickName,CountArray(PropertyPresentRequest.szRecvNickName));
			PropertyPresentRequest.dwClientAddr=(m_pBindParameter + LOWORD(dwSocketID))->dwClientAddr; //系统信息

			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_PROPERTY_PRESENT, dwSocketID, &PropertyPresentRequest, sizeof(PropertyPresentRequest));
			return true;
		}
	case SUB_GP_GET_SEND_PRESENT:		//获得赠送
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_GP_C_GetSendPresent));
			if (wDataSize!=sizeof(CMD_GP_C_GetSendPresent)) return false;

			//变量定义
			CMD_GP_C_GetSendPresent *pGetSendPresent = (CMD_GP_C_GetSendPresent *)pData;
			DBR_GP_GetSendPresent GetSendPresentRequest={0};
			GetSendPresentRequest.dwUserID = pGetSendPresent->dwUserID;
			lstrcpyn(GetSendPresentRequest.szPassword,pGetSendPresent->szPassword,CountArray(GetSendPresentRequest.szPassword));
			GetSendPresentRequest.dwClientAddr=(m_pBindParameter + LOWORD(dwSocketID))->dwClientAddr; //系统信息

			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_GET_SEND_PRESENT, dwSocketID, &GetSendPresentRequest, sizeof(GetSendPresentRequest));
			return true;
		}
	default:
		ASSERT(false);
		break;
	}
	return false;
}

//登录处理
bool CAttemperEngineSink::OnTCPNetworkMainMBLogon(WORD wSubCmdID, VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	switch (wSubCmdID)
	{
	// Oboleted TODO:remove
	// case SUB_MB_LOGON_GAMEID:		//I D 登录
	//  {
	//    return OnTCPNetworkSubMBLogonGameID(pData,wDataSize,dwSocketID);
	//  }
	// case SUB_MB_LOGON_OTHERPLATFORM: //其他平台
	//  {
	//   return OnTCPNetworkSubMBLogonOtherPlatform(pData,wDataSize,dwSocketID);
	//  }
	
	case SUB_MB_LOGON_ACCOUNTS:		//帐号登录
		{
			return OnTCPNetworkSubMBLogonAccounts(pData,wDataSize,dwSocketID);
		}
	case SUB_MB_REGISTER_ACCOUNTS:	//帐号注册
		{
			return OnTCPNetworkSubMBRegisterAccounts(pData,wDataSize,dwSocketID);
		}
	case SUB_MB_LOGON_VISITOR:      //游客登录
		{
			return OnTCPNetworkSubMBLogonVisitor(pData,wDataSize,dwSocketID);
		}
	case SUB_MB_REGISTER_CODE: {
			return OnTCPNetworkSubMBRegisterCode(0, pData, wDataSize, dwSocketID);
		}
	case SUB_MB_RESETPASS_CODE: {
			return OnTCPNetworkSubMBRegisterCode(1, pData, wDataSize, dwSocketID);
		}
	case SUB_MB_RESETPASSWD: {
			return OnTCPNetworkSubMBResetPasswd(pData, wDataSize, dwSocketID);
		}
	}	

	return false;
}

//列表处理
bool CAttemperEngineSink::OnTCPNetworkMainMBServerList(WORD wSubCmdID, VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	switch(wSubCmdID)
	{
	case SUB_MB_GET_ONLINE:
		{
			return OnTCPNetworkSubMBGetOnline(pData,wDataSize,dwSocketID);
		}
	}
	return false;
}

//视频信息
bool CAttemperEngineSink::OnTCPNetworkMBVideoPlayBackInfo(WORD wSubCmdID, VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	switch(wSubCmdID)
	{
	case SUB_MB_QUERY_VIDEO_INFO:
		{
			return OnTCPNetworkSubMBQueryVideo(pData,wDataSize,dwSocketID);
		}
	case SUB_MB_QUERY_VIDEO_DETAILS:
		{
			return OnTCPNetworkSubMBQueryVideoDetails(pData,wDataSize,dwSocketID);
		}
	case SUB_MB_QUERY_VIDEO_DETAILS_BY_ROOMID:
		{
			return OnTCPNetworkSubMBQueryVideoDetailsByRoomID(pData, wDataSize, dwSocketID);
		}
	case SUB_MB_QUERY_PLAYBACK_CODE_YZ:
		{
			return OnTCPNetworkSubMBQueryPlayBackCodeYZ(pData,wDataSize,dwSocketID);
		}
	case SUB_MB_QUERY_PLAYBACK_CODE:
		{
			return OnTCPNetworkSubMBQueryPlayBackCode(pData,wDataSize,dwSocketID);
		}
	}
	return false;
}

//视频查询
bool CAttemperEngineSink::OnTCPNetworkSubMBQueryVideo(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//效验参数
	ASSERT(wDataSize>=sizeof(CMD_MB_C_QueryVideoInfo));
	if (wDataSize<sizeof(CMD_MB_C_QueryVideoInfo))return false;

	//处理消息
	CMD_MB_C_QueryVideoInfo * pQueryVideo=(CMD_MB_C_QueryVideoInfo *)pData;
	
	//变量定义
	DBR_MB_QueryVideoInfo QueryVideoInfo;
	ZeroMemory(&QueryVideoInfo,sizeof(QueryVideoInfo));
	
	QueryVideoInfo.iQueryType	= pQueryVideo->iQueryType;
	QueryVideoInfo.dwUserID		= pQueryVideo->dwUserID;
	QueryVideoInfo.dwPlayBack	= pQueryVideo->dwPlayBack;	
	QueryVideoInfo.dwPersonalRoomID, pQueryVideo->dwPersonalRoomID;
	QueryVideoInfo.wIndexBegin	= pQueryVideo->wIndexBegin;
	QueryVideoInfo.wIndexEnd	= pQueryVideo->wIndexEnd;

	//投递请求
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_MB_QUERY_VIDEO_INFO,dwSocketID,&QueryVideoInfo,sizeof(QueryVideoInfo));

	return true;
}
//详情查询
bool CAttemperEngineSink::OnTCPNetworkSubMBQueryVideoDetails(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//效验参数
	ASSERT(wDataSize>=sizeof(CMD_MB_C_QueryVideoDetails));
	if (wDataSize<sizeof(CMD_MB_C_QueryVideoDetails))return false;

	//处理消息
	CMD_MB_C_QueryVideoDetails * pQueryVideo=(CMD_MB_C_QueryVideoDetails *)pData;

	//变量定义
	DBR_MB_QueryVideoDetails QueryVideoDetails;
	ZeroMemory(&QueryVideoDetails,sizeof(QueryVideoDetails));

	lstrcpyn(QueryVideoDetails.szPersonalGUID,pQueryVideo->szPersonalGUID,CountArray(QueryVideoDetails.szPersonalGUID));

	//投递请求
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_MB_QUERY_VIDEO_DETAILS,dwSocketID,&QueryVideoDetails,sizeof(QueryVideoDetails));

	return true;
}

//详情查询
bool CAttemperEngineSink::OnTCPNetworkSubMBQueryVideoDetailsByRoomID(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//效验参数
	ASSERT(wDataSize >= sizeof(CMD_MB_C_QueryVideoDetailsByRoomID));
	if (wDataSize<sizeof(CMD_MB_C_QueryVideoDetailsByRoomID))return false;

	//处理消息
	CMD_MB_C_QueryVideoDetailsByRoomID * pQueryVideo = (CMD_MB_C_QueryVideoDetailsByRoomID *)pData;

	//变量定义
	DBR_MB_QueryVideoDetailsByRoomID QueryVideoDetails;
	ZeroMemory(&QueryVideoDetails,sizeof(QueryVideoDetails));

	QueryVideoDetails.dwPersonalRoomID = pQueryVideo->dwPersonalRoomID;

	//投递请求
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_MB_QUERY_VIDEO_DETAILS_BY_ROOMID, dwSocketID, &QueryVideoDetails, sizeof(QueryVideoDetails));

	return true;
}
//回放码
bool CAttemperEngineSink::OnTCPNetworkSubMBQueryPlayBackCodeYZ(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//效验参数
	ASSERT(wDataSize>=sizeof(CMD_MB_C_QueryPlayBackCodeYZ));
	if (wDataSize<sizeof(CMD_MB_C_QueryPlayBackCodeYZ))return false;

	//处理消息
	CMD_MB_C_QueryPlayBackCodeYZ * pQueryCode=(CMD_MB_C_QueryPlayBackCodeYZ *)pData;

	//变量定义
	DBR_MB_QueryPlayBackCodeYZ QueryPlayBackCode;
	ZeroMemory(&QueryPlayBackCode,sizeof(QueryPlayBackCode));

	QueryPlayBackCode.dwUserID	= pQueryCode->dwUserID;
	lstrcpyn(QueryPlayBackCode.szPersonalGUID,pQueryCode->szPersonalGUID,CountArray(QueryPlayBackCode.szPersonalGUID));

	//投递请求
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_MB_QUERY_PLAYBACK_CODE_YZ,dwSocketID,&QueryPlayBackCode,sizeof(QueryPlayBackCode));

	return true;
}

//回放码
bool CAttemperEngineSink::OnTCPNetworkSubMBQueryPlayBackCode(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//效验参数
	ASSERT(wDataSize>=sizeof(CMD_MB_C_QueryPlayBackCode));
	if (wDataSize<sizeof(CMD_MB_C_QueryPlayBackCode))return false;

	//处理消息
	CMD_MB_C_QueryPlayBackCode * pQueryCode=(CMD_MB_C_QueryPlayBackCode *)pData;

	//变量定义
	DBR_MB_QueryPlayBackCode QueryPlayBackCode;
	ZeroMemory(&QueryPlayBackCode,sizeof(QueryPlayBackCode));

	QueryPlayBackCode.dwUserID	= pQueryCode->dwUserID;

	int iLength ;  	
	iLength = WideCharToMultiByte(CP_ACP, 0, pQueryCode->szVideoNumber, -1, NULL, 0, NULL, NULL);  	
	WideCharToMultiByte(CP_ACP, 0, pQueryCode->szVideoNumber, -1, QueryPlayBackCode.szVideoNumber, iLength, NULL, NULL);   

	//投递请求
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_MB_QUERY_PLAYBACK_CODE,dwSocketID,&QueryPlayBackCode,sizeof(QueryPlayBackCode));

	return true;
}
//列表处理
bool CAttemperEngineSink::OnTCPNetworkMainMBPersonalService(WORD wSubCmdID, VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	switch(wSubCmdID)
	{
	case SUB_MB_QUERY_GAME_SERVER:
		{
			return OnTCPNetworkSubMBQueryGameServer(pData,wDataSize,dwSocketID);
		}
	case SUB_MB_SEARCH_SERVER_TABLE:
		{
			return OnTCPNetworkSubMBSearchServerTable(pData,wDataSize,dwSocketID);
		}
	case SUB_MB_DISSUME_SEARCH_SERVER_TABLE:
		{
			return OnTCPNetworkSubMBDissumeSearchServerTable(pData,wDataSize,dwSocketID);
		}
	case SUB_MB_GET_PERSONAL_PARAMETER:
		{
			return OnTCPNetworkSubMBPersonalParameter(pData,wDataSize,dwSocketID);
		}
	case SUB_MB_QUERY_ROOM_PASSWORD:
		{
			return OnTCPNetworkSubMBQueryRoomPassword(pData, wDataSize, dwSocketID);
		}
	case SUB_MB_QUERY_PERSONAL_ROOM_LIST:
		{
			return OnTCPNetworkSubMBQueryPersonalRoomList(pData,wDataSize,dwSocketID);
		}
	case SUB_GR_USER_QUERY_ROOM_SCORE: //请求房间成绩
		{
			return OnTCPNetworkSubQueryUserRoomScore(pData, wDataSize, dwSocketID);
		}
	case SUB_MB_QUERY_PERSONAL_ROOM_USER_INFO://请求玩家的游戏豆
		{
			return OnTCPNetworkSubQueryPersonalRoomUserInfo(pData, wDataSize, dwSocketID);
		}
	case SUB_MB_ROOM_CARD_EXCHANGE_TO_SCORE://请求玩家的游戏豆
		{
			return OnTCPNetworkSubRoomCardExchangeToScore(pData, wDataSize, dwSocketID);
		}
	case SUB_MB_PERSONAL_RULE:				//获取私人房定制配置
		{
			return OnTCPNetworkSubMBGetPersonalRoomRule(pData, wDataSize, dwSocketID);
		}
	}
	return true;
}

//I D 登录
bool CAttemperEngineSink::OnTCPNetworkSubPCLogonGameID(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//效验参数
	ASSERT(wDataSize>=sizeof(CMD_GP_LogonGameID));
	if (wDataSize<sizeof(CMD_GP_LogonGameID))
	{
		if (wDataSize<sizeof(CMD_GP_LogonGameID)-sizeof(BYTE))
			return false;
	}

	//变量定义
	WORD wBindIndex=LOWORD(dwSocketID);
	tagBindParameter * pBindParameter=(m_pBindParameter+wBindIndex);

	//处理消息
	CMD_GP_LogonGameID * pLogonGameID=(CMD_GP_LogonGameID *)pData;
	pLogonGameID->szPassword[CountArray(pLogonGameID->szPassword)-1]=0;
	pLogonGameID->szMachineID[CountArray(pLogonGameID->szMachineID)-1]=0;

	//设置连接
	pBindParameter->cbClientKind=CLIENT_KIND_COMPUTER;
	pBindParameter->dwPlazaVersion=pLogonGameID->dwPlazaVersion;

	//效验版本
	if (CheckPlazaVersion(DEVICE_TYPE_PC,pLogonGameID->dwPlazaVersion,dwSocketID,((pLogonGameID->cbValidateFlags&LOW_VER_VALIDATE_FLAGS)!=0))==false)
	{
		return true;
	}

	//变量定义
	DBR_GP_LogonGameID LogonGameID;
	ZeroMemory(&LogonGameID,sizeof(LogonGameID));

	//附加信息
	LogonGameID.pBindParameter=(m_pBindParameter+LOWORD(dwSocketID));

	//构造数据
	LogonGameID.dwGameID=pLogonGameID->dwGameID;
	LogonGameID.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
	lstrcpyn(LogonGameID.szPassword,pLogonGameID->szPassword,CountArray(LogonGameID.szPassword));
	lstrcpyn(LogonGameID.szMachineID,pLogonGameID->szMachineID,CountArray(LogonGameID.szMachineID));
	LogonGameID.cbNeeValidateMBCard=(pLogonGameID->cbValidateFlags&MB_VALIDATE_FLAGS);

	//投递请求
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOGON_GAMEID,dwSocketID,&LogonGameID,sizeof(LogonGameID));

	return true;
}

//帐号登录
bool CAttemperEngineSink::OnTCPNetworkSubPCLogonAccounts(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//效验参数
	ASSERT(wDataSize<=sizeof(CMD_GP_LogonAccounts));
	if (wDataSize>sizeof(CMD_GP_LogonAccounts)) return false;

	//变量定义
	WORD wBindIndex=LOWORD(dwSocketID);
	tagBindParameter * pBindParameter=(m_pBindParameter+wBindIndex);

	//处理消息
	CMD_GP_LogonAccounts * pLogonAccounts=(CMD_GP_LogonAccounts *)pData;
	pLogonAccounts->szAccounts[CountArray(pLogonAccounts->szAccounts)-1]=0;
	pLogonAccounts->szPassword[CountArray(pLogonAccounts->szPassword)-1]=0;
	pLogonAccounts->szMachineID[CountArray(pLogonAccounts->szMachineID)-1]=0;
	pLogonAccounts->szPassPortID[CountArray(pLogonAccounts->szPassPortID)-1]=0;

	//设置连接
	pBindParameter->cbClientKind=CLIENT_KIND_COMPUTER;
	pBindParameter->dwPlazaVersion=pLogonAccounts->dwPlazaVersion;

	//版本判断
	if (CheckPlazaVersion(DEVICE_TYPE_PC,pLogonAccounts->dwPlazaVersion,dwSocketID,((pLogonAccounts->cbValidateFlags&LOW_VER_VALIDATE_FLAGS)!=0))==false)
	{
		return true;
	}

	//变量定义
	DBR_GP_LogonAccounts LogonAccounts;
	ZeroMemory(&LogonAccounts,sizeof(LogonAccounts));

	//附加信息
	LogonAccounts.pBindParameter=(m_pBindParameter+LOWORD(dwSocketID));

	//构造数据
	LogonAccounts.dwCheckUserRight=0;
	LogonAccounts.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
	LogonAccounts.cbNeeValidateMBCard=(pLogonAccounts->cbValidateFlags&MB_VALIDATE_FLAGS);
	lstrcpyn(LogonAccounts.szAccounts,pLogonAccounts->szAccounts,CountArray(LogonAccounts.szAccounts));
	lstrcpyn(LogonAccounts.szPassword,pLogonAccounts->szPassword,CountArray(LogonAccounts.szPassword));
	lstrcpyn(LogonAccounts.szMachineID,pLogonAccounts->szMachineID,CountArray(LogonAccounts.szMachineID));	
	lstrcpyn(LogonAccounts.szPassPortID,pLogonAccounts->szPassPortID,CountArray(LogonAccounts.szPassPortID));

	//投递请求
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOGON_ACCOUNTS,dwSocketID,&LogonAccounts,sizeof(LogonAccounts));

	return true;
}

//工具登录
bool CAttemperEngineSink::OnTCPNetworkSubPCLogonManageTool(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//效验参数
	ASSERT(wDataSize>=sizeof(CMD_GP_LogonAccounts));
	if (wDataSize<sizeof(CMD_GP_LogonAccounts))
	{
		if (wDataSize<sizeof(CMD_GP_LogonAccounts)-sizeof(BYTE))
			return false;
	}

	//变量定义
	WORD wBindIndex=LOWORD(dwSocketID);
	tagBindParameter * pBindParameter=(m_pBindParameter+wBindIndex);

	//处理消息
	CMD_GP_LogonAccounts * pLogonAccounts=(CMD_GP_LogonAccounts *)pData;
	pLogonAccounts->szAccounts[CountArray(pLogonAccounts->szAccounts)-1]=0;
	pLogonAccounts->szPassword[CountArray(pLogonAccounts->szPassword)-1]=0;
	pLogonAccounts->szMachineID[CountArray(pLogonAccounts->szMachineID)-1]=0;

	//设置连接
	pBindParameter->cbClientKind=CLIENT_KIND_COMPUTER;
	pBindParameter->dwPlazaVersion=pLogonAccounts->dwPlazaVersion;

	//版本判断
	if (CheckPlazaVersion(DEVICE_TYPE_PC,pLogonAccounts->dwPlazaVersion,dwSocketID,((pLogonAccounts->cbValidateFlags&LOW_VER_VALIDATE_FLAGS)!=0))==false)
	{
		return true;
	}

	//变量定义
	DBR_GP_LogonAccounts LogonAccounts;
	ZeroMemory(&LogonAccounts,sizeof(LogonAccounts));

	//附加信息
	LogonAccounts.pBindParameter=(m_pBindParameter+LOWORD(dwSocketID));

	//构造数据
	LogonAccounts.dwCheckUserRight=UR_CAN_MANAGER_ANDROID;
	LogonAccounts.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
	lstrcpyn(LogonAccounts.szAccounts,pLogonAccounts->szAccounts,CountArray(LogonAccounts.szAccounts));
	lstrcpyn(LogonAccounts.szPassword,pLogonAccounts->szPassword,CountArray(LogonAccounts.szPassword));
	lstrcpyn(LogonAccounts.szMachineID,pLogonAccounts->szMachineID,CountArray(LogonAccounts.szMachineID));
	LogonAccounts.cbNeeValidateMBCard=(pLogonAccounts->cbValidateFlags&MB_VALIDATE_FLAGS);

	//投递请求
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOGON_ACCOUNTS,dwSocketID,&LogonAccounts,sizeof(LogonAccounts));

	return true;
}

//帐号注册
bool CAttemperEngineSink::OnTCPNetworkSubPCRegisterAccounts(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//效验参数
	ASSERT(wDataSize>=sizeof(CMD_GP_RegisterAccounts));
	if (wDataSize<sizeof(CMD_GP_RegisterAccounts))
	{
		if (wDataSize<sizeof(CMD_GP_RegisterAccounts)-sizeof(BYTE))
			return false;
	}

	//变量定义
	WORD wBindIndex=LOWORD(dwSocketID);
	tagBindParameter * pBindParameter=(m_pBindParameter+wBindIndex);

	//处理消息
	CMD_GP_RegisterAccounts * pRegisterAccounts=(CMD_GP_RegisterAccounts *)pData;
	pRegisterAccounts->szAccounts[CountArray(pRegisterAccounts->szAccounts)-1]=0;
	pRegisterAccounts->szNickName[CountArray(pRegisterAccounts->szNickName)-1]=0;
	pRegisterAccounts->szMachineID[CountArray(pRegisterAccounts->szMachineID)-1]=0;
	pRegisterAccounts->szLogonPass[CountArray(pRegisterAccounts->szLogonPass)-1]=0;
	pRegisterAccounts->szPassPortID[CountArray(pRegisterAccounts->szPassPortID)-1]=0;
	pRegisterAccounts->szCompellation[CountArray(pRegisterAccounts->szCompellation)-1]=0;

	//设置连接
	pBindParameter->cbClientKind=CLIENT_KIND_COMPUTER;
	pBindParameter->dwPlazaVersion=pRegisterAccounts->dwPlazaVersion;

	//效验版本
	if (CheckPlazaVersion(DEVICE_TYPE_PC,pRegisterAccounts->dwPlazaVersion,dwSocketID,((pRegisterAccounts->cbValidateFlags&LOW_VER_VALIDATE_FLAGS)!=0))==false)
	{
		return true;
	}

	//变量定义
	DBR_GP_RegisterAccounts RegisterAccounts;
	ZeroMemory(&RegisterAccounts,sizeof(RegisterAccounts));

	//附加信息
	RegisterAccounts.pBindParameter=(m_pBindParameter+LOWORD(dwSocketID));

	//构造数据
	RegisterAccounts.wFaceID=pRegisterAccounts->wFaceID;
	RegisterAccounts.cbGender=pRegisterAccounts->cbGender;
	RegisterAccounts.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
	RegisterAccounts.dwAgentID = pRegisterAccounts->dwAgentID;
	RegisterAccounts.dwSpreaderGameID = pRegisterAccounts->dwSpreaderGameID;
	lstrcpyn(RegisterAccounts.szAccounts,pRegisterAccounts->szAccounts,CountArray(RegisterAccounts.szAccounts));
	lstrcpyn(RegisterAccounts.szNickName,pRegisterAccounts->szNickName,CountArray(RegisterAccounts.szNickName));
	lstrcpyn(RegisterAccounts.szMachineID,pRegisterAccounts->szMachineID,CountArray(RegisterAccounts.szMachineID));
	lstrcpyn(RegisterAccounts.szLogonPass,pRegisterAccounts->szLogonPass,CountArray(RegisterAccounts.szLogonPass));
	lstrcpyn(RegisterAccounts.szPassPortID,pRegisterAccounts->szPassPortID,CountArray(RegisterAccounts.szPassPortID));
	lstrcpyn(RegisterAccounts.szCompellation,pRegisterAccounts->szCompellation,CountArray(RegisterAccounts.szCompellation));

	//投递请求
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_REGISTER_ACCOUNTS,dwSocketID,&RegisterAccounts,sizeof(RegisterAccounts));

	return true;
}

//验证资料
bool CAttemperEngineSink::OnTCPNetworkSubPCVerifyIndividual(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//效验参数
	ASSERT(wDataSize>=sizeof(CMD_GP_VerifyIndividual));
	if (wDataSize<sizeof(CMD_GP_VerifyIndividual)) return false;

	//处理消息
	CMD_GP_VerifyIndividual * pVerifyIndividual=(CMD_GP_VerifyIndividual *)pData;

	//效验版本
	if (CheckPlazaVersion(DEVICE_TYPE_PC,pVerifyIndividual->dwPlazaVersion,dwSocketID,true)==false)
	{
		return true;
	}

	//变量定义
	DBR_GP_VerifyIndividual VerifyIndividual;
	ZeroMemory(&VerifyIndividual,sizeof(VerifyIndividual));

	//设置变量
	VerifyIndividual.wVerifyMask=pVerifyIndividual->wVerifyMask;
	VerifyIndividual.pBindParameter=(m_pBindParameter+LOWORD(dwSocketID));

	//变量定义
	VOID * pDataBuffer=NULL;
	tagDataDescribe DataDescribe;
	CRecvPacketHelper RecvPacket(pVerifyIndividual+1,wDataSize-sizeof(CMD_GP_VerifyIndividual));

	//扩展信息
	while (true)
	{
		pDataBuffer=RecvPacket.GetData(DataDescribe);
		if (DataDescribe.wDataDescribe==DTP_NULL) break;

		if(DataDescribe.wDataDescribe==DTP_GP_UI_NICKNAME)	//用户昵称
		{
			ASSERT(pDataBuffer!=NULL);
			ASSERT(DataDescribe.wDataSize<=sizeof(VerifyIndividual.szVerifyContent));
			if (DataDescribe.wDataSize<=sizeof(VerifyIndividual.szVerifyContent))
			{
				CopyMemory(&VerifyIndividual.szVerifyContent,pDataBuffer,DataDescribe.wDataSize);
				VerifyIndividual.szVerifyContent[CountArray(VerifyIndividual.szVerifyContent)-1]=0;
			}
			break;
		}

		if(DataDescribe.wDataDescribe==DTP_GP_UI_ACCOUNTS)	//用户帐号
		{
			ASSERT(pDataBuffer!=NULL);
			ASSERT(DataDescribe.wDataSize<=sizeof(VerifyIndividual.szVerifyContent));
			if (DataDescribe.wDataSize<=sizeof(VerifyIndividual.szVerifyContent))
			{
				CopyMemory(&VerifyIndividual.szVerifyContent,pDataBuffer,DataDescribe.wDataSize);
				VerifyIndividual.szVerifyContent[CountArray(VerifyIndividual.szVerifyContent)-1]=0;
			}
			break;
		}
	}

	//投递请求
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_VERIFY_INDIVIDUAL,dwSocketID,&VerifyIndividual,sizeof(VerifyIndividual));

	return true;
}

//游客登录
bool CAttemperEngineSink::OnTCPNetworkSubPCLogonVisitor(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//效验参数
	ASSERT(wDataSize>=sizeof(CMD_GP_LogonVisitor));
	if (wDataSize<sizeof(CMD_GP_LogonVisitor))return false;

	//变量定义
	WORD wBindIndex=LOWORD(dwSocketID);
	tagBindParameter * pBindParameter=(m_pBindParameter+wBindIndex);

	//处理消息
	CMD_GP_LogonVisitor * pLogonVisitor=(CMD_GP_LogonVisitor *)pData;

	//设置连接
	pBindParameter->cbClientKind=CLIENT_KIND_COMPUTER;
	pBindParameter->dwPlazaVersion=pLogonVisitor->dwPlazaVersion;

	//版本判断
	if (CheckPlazaVersion(DEVICE_TYPE_PC,pLogonVisitor->dwPlazaVersion,dwSocketID,((pLogonVisitor->cbValidateFlags&LOW_VER_VALIDATE_FLAGS)!=0))==false)
	{
		return true;
	}

	//变量定义
	DBR_GP_LogonVisitor LogonVisitor;
	ZeroMemory(&LogonVisitor,sizeof(LogonVisitor));

	//附加信息
	LogonVisitor.pBindParameter=(m_pBindParameter+LOWORD(dwSocketID));

	//构造数据
	LogonVisitor.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
	LogonVisitor.cbPlatformID=ULMByVisitor;
	lstrcpyn(LogonVisitor.szMachineID,pLogonVisitor->szMachineID,CountArray(LogonVisitor.szMachineID));

	//投递请求
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOGON_VISITOR,dwSocketID,&LogonVisitor,sizeof(LogonVisitor));

	return true;
}

//查询验证码
bool CAttemperEngineSink::OnTCPNetworkSubPCQueryVerifyCode(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	ASSERT(wDataSize==sizeof(CMD_GP_QueryVerifyCode));
	if (wDataSize!=sizeof(CMD_GP_QueryVerifyCode)) return false;

	CMD_GP_QueryVerifyCode *pQueryVerifyCode=(CMD_GP_QueryVerifyCode *)pData;
	pQueryVerifyCode->szMachineID[CountArray(pQueryVerifyCode->szMachineID)-1]=0;
	pQueryVerifyCode->szVerifyCode[CountArray(pQueryVerifyCode->szVerifyCode)-1]=0;

	DWORD dwCurrentTime=(DWORD)time(NULL);
	DWORD dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
	TCHAR szSendVerifyCode[LEN_VERIFY_CODE]=TEXT("");

	//查找是否已申请
	INT i=0;
	for (i=0; i<m_VerifyCodeArray.GetCount();i++)
	{
		tagVerifyCode *pVerifyCode=m_VerifyCodeArray[i];
		if (pVerifyCode==NULL) continue;

		if (pVerifyCode->dwClientAddr==dwClientAddr && lstrcmp(pVerifyCode->szMachineID,pQueryVerifyCode->szMachineID)==0)
		{
			if (dwCurrentTime-pVerifyCode->dwUpdateTime<TIME_VALID_VERIFY_CODE)
			{
				CMD_GP_VerifyCodeResult VerifyCodeResult;
				ZeroMemory(&VerifyCodeResult,sizeof(VerifyCodeResult));

				VerifyCodeResult.cbResultCode=2;
				lstrcpy(VerifyCodeResult.szDescString,TEXT("抱歉，您在短时间内查询次数过于频繁，请稍后重试！"));

				//发送消息
				m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_LOGON,SUB_GP_VERIFY_CODE_RESULT,&VerifyCodeResult,sizeof(VerifyCodeResult));
				return true;
			}

			//重新生成
			pVerifyCode->dwUpdateTime=dwCurrentTime;
			RandVerifyCode(pVerifyCode->szValidateCode,CountArray(pVerifyCode->szValidateCode));
			lstrcpyn(szSendVerifyCode,pVerifyCode->szValidateCode,CountArray(szSendVerifyCode));
			break;
		}
	}

	//新增请求项
	if (i==m_VerifyCodeArray.GetCount())
	{
		tagVerifyCode * pNewVerifyCode=new tagVerifyCode;
		pNewVerifyCode->dwClientAddr=dwClientAddr;
		pNewVerifyCode->dwUpdateTime=dwCurrentTime;
		lstrcpyn(pNewVerifyCode->szMachineID,pQueryVerifyCode->szMachineID,CountArray(pNewVerifyCode->szMachineID));
		RandVerifyCode(pNewVerifyCode->szValidateCode,CountArray(pNewVerifyCode->szValidateCode));
		m_VerifyCodeArray.Add(pNewVerifyCode);

		lstrcpyn(szSendVerifyCode,pNewVerifyCode->szValidateCode,CountArray(szSendVerifyCode));
	}

	//发送验证码
	SendVerifyCode(dwSocketID,szSendVerifyCode,true);

	return true;
}

//I D 登录
bool CAttemperEngineSink::OnTCPNetworkSubMBLogonGameID(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	// Obsoleted TODO:remove
	// post message DBR_MB_LOGON_GAMEID with struct DBR_MB_LogonGameID
	return false;
}

//帐号登录
bool CAttemperEngineSink::OnTCPNetworkSubMBLogonAccounts(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//效验参数
	ASSERT(wDataSize>=sizeof(CMD_MB_LogonAccounts));
	if (wDataSize<sizeof(CMD_MB_LogonAccounts)) return false;

	//变量定义
	WORD wBindIndex=LOWORD(dwSocketID);
	tagBindParameter * pBindParameter=(m_pBindParameter+wBindIndex);

	//处理消息
	CMD_MB_LogonAccounts * pLogonAccounts=(CMD_MB_LogonAccounts *)pData;
	pLogonAccounts->szAccounts[CountArray(pLogonAccounts->szAccounts)-1]=0;
	pLogonAccounts->szPassword[CountArray(pLogonAccounts->szPassword)-1]=0;
	pLogonAccounts->szMachineID[CountArray(pLogonAccounts->szMachineID)-1]=0;
	if (lstrlen(pLogonAccounts->szMachineID) == 0) {
		return false;
	}

	//设置连接
	pBindParameter->cbClientKind=CLIENT_KIND_MOBILE;
	pBindParameter->wModuleID=pLogonAccounts->wModuleID;
	pBindParameter->dwPlazaVersion=pLogonAccounts->dwPlazaVersion;

	//版本判断
	if (CheckPlazaVersion(pLogonAccounts->cbDeviceType,pLogonAccounts->dwPlazaVersion,dwSocketID)==false) return true;

	//变量定义
	DBR_MB_LogonAccounts LogonAccounts;
	ZeroMemory(&LogonAccounts,sizeof(LogonAccounts));

	//附加信息
	LogonAccounts.pBindParameter=(m_pBindParameter+LOWORD(dwSocketID));

	//构造数据
	LogonAccounts.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
	lstrcpyn(LogonAccounts.szAccounts,pLogonAccounts->szAccounts,CountArray(LogonAccounts.szAccounts));
	lstrcpyn(LogonAccounts.szPassword,pLogonAccounts->szPassword,CountArray(LogonAccounts.szPassword));
	lstrcpyn(LogonAccounts.szMachineID,pLogonAccounts->szMachineID,CountArray(LogonAccounts.szMachineID));

	//投递请求
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_MB_LOGON_ACCOUNTS,dwSocketID,&LogonAccounts,sizeof(LogonAccounts));
	return true;
}

bool CAttemperEngineSink::OnTCPNetworkSubMBResetPasswd(VOID * pData, WORD wDataSize, DWORD dwSocketID) {
	qpcmd::ReqResetPasswd req;
	bool ok = req.ParseFromArray(pData, wDataSize);
	if (!ok) {
		ALErr("%d proto decode failed(%d)", dwSocketID, wDataSize);
		return false;
	}
	const std::string& phone = req.phone();
	const std::string& code = req.code();
	std::string* pPass = req.mutable_passwd();
	if (phone.size() != 11) {
		AT_PROTO_REPRET(dwSocketID, RRPC_ERR_PHONE);
		return true;
	}
	if (code.size() != 4) {
		AT_PROTO_REPRET(dwSocketID, RRPC_ERR_VCODEWRONG);
		return true;
	}
	RPCLOGON(rpc);
	int ret = rpc.VerifySms(1, phone.data(), code.data());
	ALStd("%d:%s passwd code ret %d", dwSocketID, phone.data(), ret);
	if (ret != RRPC_ERR_VCODEOK) {
		AT_PROTO_REPRET(dwSocketID,ret);
		return true;
	}
	if (pPass->size() > 32) {
		(*pPass)[32] = 0;
	}
	//变量定义
	DBR_GP_ModifyLogonPass ModifyLogonPass;
	ZeroMemory(&ModifyLogonPass,sizeof(ModifyLogonPass));
	//构造数据
	ModifyLogonPass.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
	Utf8ToUnicode(phone.data(), ModifyLogonPass.szAccounts);
	Utf8ToUnicode(pPass->data(), ModifyLogonPass.szPassword);
	//投递请求
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_MODIFY_LOGON_PASS,dwSocketID,&ModifyLogonPass,sizeof(ModifyLogonPass));
	return true;
}

bool CAttemperEngineSink::OnTCPNetworkSubMBRegisterCode(int kind, VOID * pData, WORD wDataSize, DWORD dwSocketID) {
	qpcmd::ReqRegisterCode req;
	bool ok = req.ParseFromArray(pData, wDataSize);
	if (!ok) {
		ALErr("%d proto decode failed(%d)", dwSocketID, wDataSize);
		return false;
	}

	int ret = RRPC_ERR_PHONE;
	const std::string& phone = req.phone();

	std::string testout;
	if (phone.size() == 11) {
		RPCLOGON(rpc);
		ret = rpc.SendSms(kind, phone.data(), testout);
	}
	testout.append(RrpcErrorString(ret));
	qpcmd::RepRet rep;
	rep.set_code(ret);
	rep.set_error(testout);
	std::string repData;
	rep.SerializeToString(&repData);
	
	ALStd("%d:%s Kind(%d) %d:%s", dwSocketID, phone.data(), kind, ret, testout.data());

	m_pITCPNetworkEngine->SendData(dwSocketID,MDM_MB_LOGON,SUB_MB_LOGON_REPRET,(void*)repData.data(),repData.size());
	m_pITCPNetworkEngine->ShutDownSocket(dwSocketID);
	return true;
}

//绑定账号
bool CAttemperEngineSink::OnTCPNetworkSubMBRegisterAccounts(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	qpcmd::ReqRegisterAccount req;
	bool ok = req.ParseFromArray(pData, wDataSize);
	if (!ok) {
		ALErr("%d proto decode failed(%d)", dwSocketID, wDataSize);
		return false;
	}
	//变量定义
	WORD wBindIndex=LOWORD(dwSocketID);
	tagBindParameter * pBindParameter=(m_pBindParameter+wBindIndex);

	BYTE cbDeviceType = req.devicetype();
	DWORD dwPlazaVersion = req.plazaversion();
	const std::string& account = req.account();
	const std::string& vcode = req.verifycode();
	const std::string& nickname = req.nickname();

	//效验版本
	if (CheckPlazaVersion(cbDeviceType, dwPlazaVersion, dwSocketID)==false) return true;

	RPCLOGON(rpc);
	int ret = rpc.RegPreVerify(account.data(), vcode.data(), nickname.data());

	ALStd("%d:%s verify code[%s] nick[%s] %d", dwSocketID, account.data(), vcode.data(), nickname.data(), ret);
	if (ret != RRPC_ERR_VCODEOK) {
		AT_PROTO_REPRET(dwSocketID, ret);
		return true;
	}
	//设置连接
	pBindParameter->cbClientKind = CLIENT_KIND_MOBILE;
	pBindParameter->wModuleID = req.moduleid();
	pBindParameter->dwPlazaVersion = dwPlazaVersion;

	//转送数据模块
	DBR_MB_RegisterAccounts RegisterAccounts;
	ZeroMemory(&RegisterAccounts,sizeof(RegisterAccounts));

	RegisterAccounts.pBindParameter=(m_pBindParameter+LOWORD(dwSocketID));
	RegisterAccounts.cbDeviceType = cbDeviceType;
	RegisterAccounts.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
	RegisterAccounts.dwUserID = req.userid();

	const std::string& machineid = req.machineid();
	const std::string& logonpass = req.logonpass();
	const std::string& spcode = req.spreadercode();
	if (account.size() == 0 || account.size() >= CountArray(RegisterAccounts.szAccounts) ||
		nickname.size() >= CountArray(RegisterAccounts.szNickName) ||
		machineid.size() == 0 || machineid.size() >= CountArray(RegisterAccounts.szMachineID) ||
		logonpass.size() >= CountArray(RegisterAccounts.szLogonPass) ||
		spcode.size() >= CountArray(RegisterAccounts.szSpreaderCode) ) {
		ALErr("%d:%s malformed %d>%d>%d>%d", dwSocketID, account.data(),
			nickname.size(), machineid.size(), logonpass.size(), spcode.size());
		return false;
	}
	Utf8ToUnicode(account.data(), RegisterAccounts.szAccounts);
	Utf8ToUnicode(nickname.data(), RegisterAccounts.szNickName);
	Utf8ToUnicode(machineid.data(), RegisterAccounts.szMachineID);
	Utf8ToUnicode(logonpass.data(), RegisterAccounts.szLogonPass);
	Utf8ToUnicode(spcode.data(), RegisterAccounts.szSpreaderCode);

	ALStd("%d:%s machine[%s] spcode[%s]", dwSocketID, account.data(), machineid.data(), spcode.data());
	//投递请求
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_MB_REGISTER_ACCOUNTS,dwSocketID,&RegisterAccounts,sizeof(RegisterAccounts));
	return true;
}

//其他登录
bool CAttemperEngineSink::OnTCPNetworkSubMBLogonOtherPlatform(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	// Obsoleted TODO:remove
	// post message DBR_MB_LOGON_OTHERPLATFORM DBR_MB_LogonOtherPlatform
	return false;
}

//游客登录
bool CAttemperEngineSink::OnTCPNetworkSubMBLogonVisitor(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//效验参数
	ASSERT(wDataSize>=sizeof(CMD_MB_LogonVisitor));
	if (wDataSize<sizeof(CMD_MB_LogonVisitor))return false;

	//变量定义
	WORD wBindIndex=LOWORD(dwSocketID);
	tagBindParameter * pBindParameter=(m_pBindParameter+wBindIndex);

	//处理消息
	CMD_MB_LogonVisitor * pLogonVisitor=(CMD_MB_LogonVisitor *)pData;
	UNI2UTF_S(pLogonVisitor->szMachineID, pMachine);
	ALStd("%d machine %s", dwSocketID, pMachine);
	if (pMachine[0] == 0) return false;

	//设置连接
	pBindParameter->cbClientKind=CLIENT_KIND_MOBILE;
	pBindParameter->wModuleID=pLogonVisitor->wModuleID;
	pBindParameter->dwPlazaVersion=pLogonVisitor->dwPlazaVersion;

	//版本判断
	if (CheckPlazaVersion(pLogonVisitor->cbDeviceType,pLogonVisitor->dwPlazaVersion,dwSocketID)==false)
	{
		return true;
	}

	//变量定义
	DBR_MB_LogonVisitor LogonVisitor;
	ZeroMemory(&LogonVisitor,sizeof(LogonVisitor));

	//附加信息
	LogonVisitor.pBindParameter=(m_pBindParameter+LOWORD(dwSocketID));

	//构造数据
	LogonVisitor.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
	LogonVisitor.cbPlatformID=ULMByVisitor;
	LogonVisitor.cbDeviceType=pLogonVisitor->cbDeviceType;
	lstrcpyn(LogonVisitor.szMachineID,pLogonVisitor->szMachineID,CountArray(LogonVisitor.szMachineID));
	lstrcpyn(LogonVisitor.szLogonPass,pLogonVisitor->szLogonPass,CountArray(LogonVisitor.szLogonPass));

	//投递请求
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_MB_LOGON_VISITOR,dwSocketID,&LogonVisitor,sizeof(LogonVisitor));

	return true;
}

//获取在线人数
bool CAttemperEngineSink::OnTCPNetworkSubMBGetOnline(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//变量定义
	CMD_MB_GetOnline * pGetOnline=(CMD_MB_GetOnline *)pData;
	WORD wHeadSize=(sizeof(CMD_MB_GetOnline)-sizeof(pGetOnline->wOnLineServerID));

	//效验数据
	ASSERT((wDataSize>=wHeadSize)&&(wDataSize==(wHeadSize+pGetOnline->wServerCount*sizeof(WORD))));
	if ((wDataSize<wHeadSize)||(wDataSize!=(wHeadSize+pGetOnline->wServerCount*sizeof(WORD)))) return false;

	//变量定义
	CMD_MB_KindOnline KindOnline;
	CMD_MB_ServerOnline ServerOnline;
	ZeroMemory(&KindOnline,sizeof(KindOnline));
	ZeroMemory(&ServerOnline,sizeof(ServerOnline));

	//获取类型
	POSITION KindPosition=NULL;
	do
	{
		//获取类型
		CGameKindItem * pGameKindItem=m_ServerListManager.EmunGameKindItem(KindPosition);

		//设置变量
		if (pGameKindItem!=NULL)
		{
			WORD wKindIndex=KindOnline.wKindCount++;
			KindOnline.wCountInfo[wKindIndex].wKindOrServerID = pGameKindItem->m_GameKind.wKindID;
			KindOnline.wCountInfo[wKindIndex].dwOnLineCount = pGameKindItem->m_GameKind.dwOnLineCount;
			KindOnline.wCountInfo[wKindIndex].dwAndroidCount = pGameKindItem->m_GameKind.dwAndroidCount;
			KindOnline.wCountInfo[wKindIndex].dwSetCount = pGameKindItem->m_GameKind.dwSetCount;
		}

		//溢出判断
		if (KindOnline.wKindCount >= CountArray(KindOnline.wCountInfo))
		{
			ASSERT(FALSE);
			break;
		}

	} while (KindPosition!=NULL);

	//获取房间
	for (WORD i=0;i<pGetOnline->wServerCount;i++)
	{
		//获取房间
		WORD wServerID=pGetOnline->wOnLineServerID[i];
		CGameServerItem * pGameServerItem=m_ServerListManager.SearchGameServer(wServerID);

		//设置变量
		if (pGameServerItem!=NULL)
		{
			WORD wServerIndex=ServerOnline.wServerCount++;
			ServerOnline.wCountInfo[wServerIndex].wKindOrServerID = wServerID;
			ServerOnline.wCountInfo[wServerIndex].dwOnLineCount = pGameServerItem->m_GameServer.dwOnLineCount;
			ServerOnline.wCountInfo[wServerIndex].dwAndroidCount = pGameServerItem->m_GameServer.dwAndroidCount;
			ServerOnline.wCountInfo[wServerIndex].dwSetCount = pGameServerItem->m_GameServer.dwSetPlayerCount;
		}

		//溢出判断
		if (ServerOnline.wServerCount >= CountArray(ServerOnline.wCountInfo))
		{
			ASSERT(FALSE);
			break;
		}
	}

	//类型在线
	if (KindOnline.wKindCount>0)
	{
		WORD wHeadSize = sizeof(KindOnline)-sizeof(KindOnline.wCountInfo);
		WORD wSendSize = wHeadSize + KindOnline.wKindCount*(sizeof(KindOnline.wCountInfo[0]));
		m_pITCPNetworkEngine->SendData(dwSocketID, MDM_MB_SERVER_LIST, SUB_MB_KINE_ONLINE, &KindOnline, wSendSize);
	}

	//房间在线
	if (ServerOnline.wServerCount>0)
	{
		WORD wHeadSize = sizeof(ServerOnline)-sizeof(ServerOnline.wCountInfo);
		WORD wSendSize = wHeadSize + ServerOnline.wServerCount*(sizeof(KindOnline.wCountInfo[0]));
		m_pITCPNetworkEngine->SendData(dwSocketID, MDM_MB_SERVER_LIST, SUB_MB_SERVER_ONLINE, &ServerOnline, wSendSize);
	}

	return true;
}

//查询房间
bool CAttemperEngineSink::OnTCPNetworkSubMBQueryGameServer(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//校验数据
	ASSERT(wDataSize == sizeof(CMD_MB_QueryGameServer));
	if(wDataSize != sizeof(CMD_MB_QueryGameServer)) return false;

	CMD_MB_QueryGameServer* pGameServer = (CMD_MB_QueryGameServer*)pData;

	//构造数据
	CMD_CS_C_QueryGameServer QueryGameServer;
	ZeroMemory(&QueryGameServer, sizeof(CMD_CS_C_QueryGameServer));
	QueryGameServer.dwUserID = pGameServer->dwUserID;
	QueryGameServer.dwKindID = pGameServer->dwKindID;
	QueryGameServer.cbIsJoinGame = pGameServer->cbIsJoinGame;
	QueryGameServer.dwSocketID = dwSocketID;
	QueryGameServer.dwClientAddr = (m_pBindParameter + LOWORD(dwSocketID))->dwClientAddr;

	//发送数据
	if (m_pPrsnlRmITCPSocketService)
	{
		m_pPrsnlRmITCPSocketService->SendData(MDM_CS_SERVICE_INFO, SUB_CS_C_QUERY_GAME_SERVER, &QueryGameServer, sizeof(CMD_CS_C_QueryGameServer));
	}


	return true;
}

//搜索房间桌号
bool CAttemperEngineSink::OnTCPNetworkSubMBSearchServerTable(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//校验数据
	ASSERT(wDataSize == sizeof(CMD_MB_SearchServerTable));
	if (wDataSize != sizeof(CMD_MB_SearchServerTable)) return false;

	CMD_MB_SearchServerTable* pSearchServerTable = (CMD_MB_SearchServerTable*)pData;

	CMD_CS_C_SearchTable SearchTable;
	ZeroMemory(&SearchTable, sizeof(CMD_CS_C_SearchTable));

	SearchTable.dwSocketID = dwSocketID;	
	SearchTable.dwClientAddr = (m_pBindParameter + LOWORD(dwSocketID))->dwClientAddr;
	SearchTable.dwPersonalRoomID = pSearchServerTable->dwPersonalRoomID;

	if (m_pPrsnlRmITCPSocketService)
	{
		m_pPrsnlRmITCPSocketService->SendData(MDM_CS_SERVICE_INFO, SUB_CS_C_SEARCH_TABLE, &SearchTable, sizeof(CMD_CS_C_SearchTable));
	}
	return true;
}

//解散搜索房间桌号
bool CAttemperEngineSink::OnTCPNetworkSubMBDissumeSearchServerTable(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//if (m_pIPersonalRoomServiceManager)
	//{
	//	return m_pIPersonalRoomServiceManager->OnTCPNetworkSubMBDissumeSearchServerTable(pData, wDataSize, dwSocketID, m_pBindParameter, m_pITCPSocketService);
	//}

	//校验数据
	ASSERT(wDataSize == sizeof(CMD_MB_SearchServerTable));
	if (wDataSize != sizeof(CMD_MB_SearchServerTable)) return false;

	CMD_MB_SearchServerTable* pSearchServerTable = (CMD_MB_SearchServerTable*)pData;

	CMD_CS_C_SearchTable SearchTable;
	ZeroMemory(&SearchTable, sizeof(CMD_CS_C_SearchTable));

	SearchTable.dwSocketID = dwSocketID;
	SearchTable.dwClientAddr = (m_pBindParameter + LOWORD(dwSocketID))->dwClientAddr;
	SearchTable.dwPersonalRoomID = pSearchServerTable->dwPersonalRoomID;

	if (m_pPrsnlRmITCPSocketService)
	{
		m_pPrsnlRmITCPSocketService->SendData(MDM_CS_SERVICE_INFO, SUB_CS_C_DISSUME_SEARCH_TABLE, &SearchTable, sizeof(CMD_CS_C_SearchTable));
	}
	

	return true;
}

//约战房间配置
bool CAttemperEngineSink::OnTCPNetworkSubMBPersonalParameter(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//if (m_pIPersonalRoomServiceManager)
	//{
	//	return m_pIPersonalRoomServiceManager->OnTCPNetworkSubMBPersonalParameter(pData, wDataSize, dwSocketID, m_pIDataBaseEngine);
	//}

	//校验数据
	ASSERT(wDataSize == sizeof(CMD_MB_GetPersonalParameter));
	if(wDataSize != sizeof(CMD_MB_GetPersonalParameter)) return false;

	CMD_MB_GetPersonalParameter* pGetPersonalParameter = (CMD_MB_GetPersonalParameter*)pData;


	//构造数据
	DBR_MB_GetPersonalParameter GetPersonalParameter;
	ZeroMemory(&GetPersonalParameter, sizeof(DBR_MB_GetPersonalParameter));
	GetPersonalParameter.dwKindID = pGetPersonalParameter->dwKindID;

	//投递数据
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_MB_GET_PERSONAL_PARAMETER,dwSocketID, &GetPersonalParameter, sizeof(DBR_MB_GetPersonalParameter));

	return true;
}
//查询私人房间定制配置
bool CAttemperEngineSink::OnTCPNetworkSubMBGetPersonalRoomRule(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	return true;
	//校验数据
	ASSERT(wDataSize == sizeof(CMD_MB_GetPersonalRule));
	if (wDataSize != sizeof(CMD_MB_GetPersonalRule)) return false;

	CMD_MB_GetPersonalRule* pGetPersonalRule = (CMD_MB_GetPersonalRule*)pData;
	//构造数据
	DBR_MB_GetPersonalRule GetPersonalRule;
	ZeroMemory(&GetPersonalRule, sizeof(DBR_MB_GetPersonalRule));
	GetPersonalRule.dwServerID = pGetPersonalRule->dwServerID;
	//投递数据
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_MB_GET_PERSONAL_RULE, dwSocketID, &GetPersonalRule, sizeof(DBR_MB_GetPersonalRule));

	return true;

}

//约战房间密码
bool CAttemperEngineSink::OnTCPNetworkSubMBQueryRoomPassword(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//校验数据
	ASSERT(wDataSize == sizeof(CMD_MB_SearchServerTable));
	if (wDataSize != sizeof(CMD_MB_SearchServerTable)) return false;

	CMD_MB_SearchServerTable* pSearchServerTable = (CMD_MB_SearchServerTable*)pData;

	CMD_CS_C_SearchTable SearchTable;
	ZeroMemory(&SearchTable, sizeof(CMD_CS_C_SearchTable));

	SearchTable.dwSocketID = dwSocketID;
	SearchTable.dwClientAddr = (m_pBindParameter + LOWORD(dwSocketID))->dwClientAddr;
	SearchTable.dwPersonalRoomID = pSearchServerTable->dwPersonalRoomID;

	if (m_pPrsnlRmITCPSocketService)
	{
		m_pPrsnlRmITCPSocketService->SendData(MDM_CS_SERVICE_INFO, SUB_CS_S_QUERY_ROOM_PASSWORD, &SearchTable, sizeof(CMD_CS_C_SearchTable));
	}
	return true;
}

//查询约战房间列表
bool	CAttemperEngineSink::OnTCPNetworkSubMBQueryPersonalRoomList(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//if (m_pIPersonalRoomServiceManager)
	//{
	//	return m_pIPersonalRoomServiceManager->OnTCPNetworkSubMBQueryPersonalRoomList(pData, wDataSize, dwSocketID, m_pITCPSocketService);
	//}

	//校验数据
	ASSERT(wDataSize == sizeof(CMD_MB_QeuryPersonalRoomList));
	if(wDataSize != sizeof(CMD_MB_QeuryPersonalRoomList)) return false;

	CMD_MB_QeuryPersonalRoomList* pQueryPersonalRoomList = (CMD_MB_QeuryPersonalRoomList*)pData;

	CMD_MB_SC_QeuryPersonalRoomList  SC_QeuryPersonalRoomList; 
	SC_QeuryPersonalRoomList.dwUserID = pQueryPersonalRoomList->dwUserID;
	SC_QeuryPersonalRoomList.dwSocketID = dwSocketID;
	
	//构造数据
	if (m_pPrsnlRmITCPSocketService)
	{
		m_pPrsnlRmITCPSocketService->SendData(MDM_CS_SERVICE_INFO, SUB_CS_S_QUERY_PERSONAL_ROOM_LIST, &SC_QeuryPersonalRoomList, sizeof(CMD_MB_SC_QeuryPersonalRoomList));
	}
	
	

	return true;
}

//玩家请求房间成绩
bool CAttemperEngineSink::OnTCPNetworkSubQueryUserRoomScore(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//if (m_pIPersonalRoomServiceManager)
	//{
	//	return m_pIPersonalRoomServiceManager->OnTCPNetworkSubQueryUserRoomScore(pData, wDataSize, dwSocketID, m_pIDataBaseEngine);
	//}

	//校验数据
	ASSERT(wDataSize == sizeof(CMD_GR_QUERY_USER_ROOM_SCORE));
	if(wDataSize != sizeof(CMD_GR_QUERY_USER_ROOM_SCORE)) return false;

	//提取数据
	CMD_GR_QUERY_USER_ROOM_SCORE * pQueryUserRoomScore = (CMD_GR_QUERY_USER_ROOM_SCORE*)pData;
	ASSERT(pQueryUserRoomScore!=NULL);

	//发送数据
	DBR_GR_QUERY_USER_ROOM_INFO QUERY_USER_ROOM_INFO;
	ZeroMemory(&QUERY_USER_ROOM_INFO, sizeof(DBR_GR_QUERY_USER_ROOM_INFO));
	QUERY_USER_ROOM_INFO.dwUserID = pQueryUserRoomScore->dwUserID;//请求房间成绩的玩家

	//投递数据
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_GR_QUERY_USER_ROOM_SCORE,dwSocketID, &QUERY_USER_ROOM_INFO, sizeof(DBR_GR_QUERY_USER_ROOM_INFO));

	return true;
}

//玩家请求房间成绩
bool CAttemperEngineSink::OnTCPNetworkSubQueryPersonalRoomUserInfo(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//校验数据
	ASSERT(wDataSize == sizeof(CMD_MB_QueryPersonalRoomUserInfo));
	if(wDataSize != sizeof(CMD_MB_QueryPersonalRoomUserInfo)) return false;

	//提取数据
	CMD_MB_QueryPersonalRoomUserInfo * pQueryUserInfo = (CMD_MB_QueryPersonalRoomUserInfo*)pData;
	ASSERT(pQueryUserInfo!=NULL);

	//发送数据
	DBR_GR_QUERY_PERSONAL_ROOM_USER_INFO USER_INFO;

	USER_INFO.dwUserID = pQueryUserInfo->dwUserID;

	//投递数据
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_MB_QUERY_PERSONAL_ROOM_USER_INFO,dwSocketID, &USER_INFO, sizeof(DBR_GR_QUERY_PERSONAL_ROOM_USER_INFO));

	return true;
}



//玩家请求房间成绩
bool CAttemperEngineSink::OnTCPNetworkSubRoomCardExchangeToScore(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//校验数据
	ASSERT(wDataSize == sizeof(CMD_GP_ExchangeScoreByRoomCard));
	if(wDataSize != sizeof(CMD_GP_ExchangeScoreByRoomCard)) return false;


	//发送数据
	//构造结构
	CMD_GP_ExchangeScoreByRoomCard * pExchangeScore = (CMD_GP_ExchangeScoreByRoomCard*)pData;
	pExchangeScore->szMachineID[CountArray(pExchangeScore->szMachineID)-1]=0;

	//构造结构
	DBR_GP_ExchangeScoreByRoomCard ExchangeScore;
	ZeroMemory(&ExchangeScore,sizeof(ExchangeScore));

	//设置变量
	ExchangeScore.dwUserID=pExchangeScore->dwUserID;
	ExchangeScore.lExchangeRoomCard=pExchangeScore->lRoomCard;
	ExchangeScore.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
	lstrcpyn(ExchangeScore.szMachineID,pExchangeScore->szMachineID,CountArray(ExchangeScore.szMachineID));

	//投递数据
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_MB_ROOM_CARD_EXCHANGE_TO_SCORE,dwSocketID, &ExchangeScore, sizeof(DBR_GP_ExchangeScoreByRoomCard));

	return true;
}



//登录成功
bool CAttemperEngineSink::OnDBPCLogonSuccess(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];
	DBO_GP_LogonSuccess * pDBOLogonSuccess=(DBO_GP_LogonSuccess *)pData;
	CMD_GP_LogonSuccess * pCMDLogonSuccess=(CMD_GP_LogonSuccess *)cbDataBuffer;

	//发送定义
	WORD wHeadSize=sizeof(CMD_GP_LogonSuccess);
	CSendPacketHelper SendPacket(cbDataBuffer+wHeadSize,sizeof(cbDataBuffer)-wHeadSize);

	//设置变量
	ZeroMemory(pCMDLogonSuccess,sizeof(CMD_GP_LogonSuccess));

	//构造数据
	pCMDLogonSuccess->wFaceID=pDBOLogonSuccess->wFaceID;
	pCMDLogonSuccess->cbGender=pDBOLogonSuccess->cbGender;
	pCMDLogonSuccess->dwGameID=pDBOLogonSuccess->dwGameID;
	pCMDLogonSuccess->dwUserID=pDBOLogonSuccess->dwUserID;
	pCMDLogonSuccess->dwCustomID=pDBOLogonSuccess->dwCustomID;
	pCMDLogonSuccess->dwExperience=pDBOLogonSuccess->dwExperience;
	pCMDLogonSuccess->lLoveLiness=pDBOLogonSuccess->lLoveLiness;
	pCMDLogonSuccess->cbMoorMachine=pDBOLogonSuccess->cbMoorMachine;
	lstrcpyn(pCMDLogonSuccess->szAccounts,pDBOLogonSuccess->szAccounts,CountArray(pCMDLogonSuccess->szAccounts));
	lstrcpyn(pCMDLogonSuccess->szNickName,pDBOLogonSuccess->szNickName,CountArray(pCMDLogonSuccess->szNickName));
	lstrcpyn(pCMDLogonSuccess->szDynamicPass,pDBOLogonSuccess->szDynamicPass,CountArray(pCMDLogonSuccess->szDynamicPass));	

	//用户成绩
	pCMDLogonSuccess->lUserScore=pDBOLogonSuccess->lUserScore;
	pCMDLogonSuccess->lUserIngot=pDBOLogonSuccess->lUserIngot;
	pCMDLogonSuccess->lUserInsure=pDBOLogonSuccess->lUserInsure;
	pCMDLogonSuccess->dUserBeans=pDBOLogonSuccess->dUserBeans;	

	//配置信息
	pCMDLogonSuccess->cbInsureEnabled=pDBOLogonSuccess->cbInsureEnabled;
	pCMDLogonSuccess->cbShowServerStatus=m_bShowServerStatus?1:0;
	pCMDLogonSuccess->cbIsAgent=pDBOLogonSuccess->cbIsAgent;

	//会员信息
	if (pDBOLogonSuccess->cbMemberOrder!=0 || pDBOLogonSuccess->dwCheckUserRight!=0)
	{
		DTP_GP_MemberInfo MemberInfo;
		ZeroMemory(&MemberInfo,sizeof(MemberInfo));
		MemberInfo.cbMemberOrder=pDBOLogonSuccess->cbMemberOrder;
		MemberInfo.MemberOverDate=pDBOLogonSuccess->MemberOverDate;
		SendPacket.AddPacket(&MemberInfo,sizeof(MemberInfo),DTP_GP_MEMBER_INFO);
	}

	//个性签名
	if (pDBOLogonSuccess->szUnderWrite[0]!=0)
	{
		SendPacket.AddPacket(pDBOLogonSuccess->szUnderWrite,CountStringBuffer(pDBOLogonSuccess->szUnderWrite),DTP_GP_UNDER_WRITE);
	}

	//登录成功
	WORD wSendSize=SendPacket.GetDataSize()+sizeof(CMD_GP_LogonSuccess);
	m_pITCPNetworkEngine->SendData(dwContextID, MDM_GP_LOGON, SUB_GP_LOGON_SUCCESS, cbDataBuffer, wSendSize);
		

	//发送列表
	if (m_pInitParameter->m_cbDelayList==FALSE || 
		pDBOLogonSuccess->dwCheckUserRight==UR_CAN_MANAGER_ANDROID)
	{
		//发送列表
		SendGameTypeInfo(dwContextID);
		SendGameKindInfo(dwContextID);
		SendGamePageInfo(dwContextID,INVALID_WORD);
		SendGameNodeInfo(dwContextID,INVALID_WORD);
		//SendGameServerInfo(dwContextID,INVALID_WORD);
		SendGameServerInfo(dwContextID,INVALID_WORD,DEVICE_TYPE_PC);
		m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_SERVER_LIST,SUB_GP_LIST_FINISH);
	}
	else
	{
		SendGameTypeInfo(dwContextID);
		SendGameKindInfo(dwContextID);
		SendGamePageInfo(dwContextID,0);
		//SendGameServerInfo(dwContextID,INVALID_WORD);
		SendGameServerInfo(dwContextID,INVALID_WORD,DEVICE_TYPE_PC);
		m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_SERVER_LIST,SUB_GP_LIST_FINISH);
	}
	
	//等级配置
	SendGrowLevelConfig(dwContextID);

	//道具配置
	SendGamePropertyTypeInfo(dwContextID);
	SendGamePropertyRelatInfo(dwContextID);
	SendGamePropertyInfo(dwContextID);
	SendGamePropertySubInfo(dwContextID);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_SERVER_LIST,SUB_GP_PROPERTY_FINISH);

	//会员配置
	//SendMemberConfig(dwContextID);

	//认证配置
	SendRealAuthConfig(dwContextID);

	//登录完成
	CMD_GP_LogonFinish LogonFinish;
	ZeroMemory(&LogonFinish,sizeof(LogonFinish));
	LogonFinish.wIntermitTime=m_pInitParameter->m_wIntermitTime;
	LogonFinish.wOnLineCountTime=m_pInitParameter->m_wOnLineCountTime;
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_LOGON,SUB_GP_LOGON_FINISH,&LogonFinish,sizeof(LogonFinish));

	return true;
}

//登录失败
bool CAttemperEngineSink::OnDBPCLogonFailure(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	CMD_GP_LogonFailure LogonFailure;
	ZeroMemory(&LogonFailure,sizeof(LogonFailure));
	DBO_GP_LogonFailure * pLogonFailure=(DBO_GP_LogonFailure *)pData;

	//构造数据
	LogonFailure.lResultCode=pLogonFailure->lResultCode;
	lstrcpyn(LogonFailure.szDescribeString,pLogonFailure->szDescribeString,CountArray(LogonFailure.szDescribeString));

	//发送数据
	WORD wStringSize=CountStringBuffer(LogonFailure.szDescribeString);
	WORD wSendSize=sizeof(LogonFailure)-sizeof(LogonFailure.szDescribeString)+wStringSize;
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_LOGON,SUB_GP_LOGON_FAILURE,&LogonFailure,wSendSize);

	//关闭连接
	m_pITCPNetworkEngine->ShutDownSocket(dwContextID);

	return true;
}

//登录失败
bool CAttemperEngineSink::OnDBPCLogonValidateMBCard(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//效验参数
	ASSERT(wDataSize==sizeof(DBO_GP_ValidateMBCard));
	if (wDataSize!=sizeof(DBO_GP_ValidateMBCard)) return false;

	DBO_GP_ValidateMBCard * pValidateMBCard=(DBO_GP_ValidateMBCard *)pData;

	//构造结构
	CMD_GP_ValidateMBCard ValidateMBCard;
	ValidateMBCard.uMBCardID=pValidateMBCard->uMBCardID;

	//发送消息
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_LOGON,SUB_GP_VALIDATE_MBCARD,&ValidateMBCard,sizeof(ValidateMBCard));

	return true;
}

//登录失败
bool CAttemperEngineSink::OnDBPCLogonValidatePassPort(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//发送消息
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_LOGON,SUB_GP_VALIDATE_PASSPORT,NULL,0);

	return true;
}

//验证结果
bool CAttemperEngineSink::OnDBPCLogonVerifyResult(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//效验参数
	ASSERT(wDataSize<=sizeof(DBO_GP_VerifyIndividualResult));
	if (wDataSize>sizeof(DBO_GP_VerifyIndividualResult)) return false;

	//提取数据
	DBO_GP_VerifyIndividualResult *pVerifyIndividualResult=(DBO_GP_VerifyIndividualResult *)pData;

	//构造结构
	CMD_GP_VerifyIndividualResult VerifyIndividualResult;
	VerifyIndividualResult.wVerifyMask=pVerifyIndividualResult->wVerifyMask;
	VerifyIndividualResult.bVerifyPassage=pVerifyIndividualResult->bVerifyPassage;
	lstrcpyn(VerifyIndividualResult.szErrorMsg,pVerifyIndividualResult->szErrorMsg,CountArray(VerifyIndividualResult.szErrorMsg));

	//发送数据
	WORD wStringSize=CountStringBuffer(VerifyIndividualResult.szErrorMsg);
	WORD wSendSize=sizeof(VerifyIndividualResult)-sizeof(VerifyIndividualResult.szErrorMsg)+wStringSize;
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_LOGON,SUB_GP_VERIFY_RESULT,&VerifyIndividualResult,wSendSize);

	return true;
}

//平台配置
bool CAttemperEngineSink::OnDBPCPlatformParameter(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//变量定义
	DBO_GP_PlatformParameter * pPlatformParameter=(DBO_GP_PlatformParameter *)pData;

	//设置变量
	m_PlatformParameter.dwExchangeRate = pPlatformParameter->dwExchangeRate;
	m_PlatformParameter.dwPresentExchangeRate = pPlatformParameter->dwPresentExchangeRate;
	m_PlatformParameter.dwRateGold = pPlatformParameter->dwRateGold;

	return true;
}

//用户头像
bool CAttemperEngineSink::OnDBPCUserFaceInfo(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	CMD_GP_UserFaceInfo UserFaceInfo;
	ZeroMemory(&UserFaceInfo,sizeof(UserFaceInfo));
	DBO_GP_UserFaceInfo * pUserFaceInfo=(DBO_GP_UserFaceInfo *)pData;

	//设置变量
	UserFaceInfo.wFaceID=pUserFaceInfo->wFaceID;
	UserFaceInfo.dwCustomID=pUserFaceInfo->dwCustomID;

	//发送消息
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_USER_FACE_INFO,&UserFaceInfo,sizeof(UserFaceInfo));

	return true;
}

//用户信息
bool CAttemperEngineSink::OnDBPCUserIndividual(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];
	DBO_GP_UserIndividual * pDBOUserIndividual=(DBO_GP_UserIndividual *)pData;
	CMD_GP_UserIndividual * pCMDUserIndividual=(CMD_GP_UserIndividual *)cbDataBuffer;
	CSendPacketHelper SendPacket(cbDataBuffer+sizeof(CMD_GP_UserIndividual),sizeof(cbDataBuffer)-sizeof(CMD_GP_UserIndividual));

	//设置变量
	ZeroMemory(pCMDUserIndividual,sizeof(CMD_GP_UserIndividual));

	//构造数据
	pCMDUserIndividual->dwUserID=pDBOUserIndividual->dwUserID;

	//用户信息
	if (pDBOUserIndividual->szUserNote[0]!=0)
	{
		WORD wBufferSize=CountStringBuffer(pDBOUserIndividual->szUserNote);
		SendPacket.AddPacket(pDBOUserIndividual->szUserNote,wBufferSize,DTP_GP_UI_USER_NOTE);
	}

	// 真实
	if (pDBOUserIndividual->szCompellation[0]!=0)
	{
		WORD wBufferSize=CountStringBuffer(pDBOUserIndividual->szCompellation);
		SendPacket.AddPacket(pDBOUserIndividual->szCompellation,wBufferSize,DTP_GP_UI_COMPELLATION);
	}

	//身份证
	if (pDBOUserIndividual->szPassPortID[0]!=0)
	{
		WORD wBufferSize=CountStringBuffer(pDBOUserIndividual->szPassPortID);
		SendPacket.AddPacket(pDBOUserIndividual->szPassPortID,wBufferSize,DTP_GP_UI_PASSPORTID);
	}

	//电话号码
	if (pDBOUserIndividual->szSeatPhone[0]!=0)
	{
		WORD wBufferSize=CountStringBuffer(pDBOUserIndividual->szSeatPhone);
		SendPacket.AddPacket(pDBOUserIndividual->szSeatPhone,wBufferSize,DTP_GP_UI_SEAT_PHONE);
	}

	//移动电话
	if (pDBOUserIndividual->szMobilePhone[0]!=0)
	{
		WORD wBufferSize=CountStringBuffer(pDBOUserIndividual->szMobilePhone);
		SendPacket.AddPacket(pDBOUserIndividual->szMobilePhone,wBufferSize,DTP_GP_UI_MOBILE_PHONE);
	}

	//联系资料
	if (pDBOUserIndividual->szQQ[0]!=0) 
	{
		WORD wBufferSize=CountStringBuffer(pDBOUserIndividual->szQQ);
		SendPacket.AddPacket(pDBOUserIndividual->szQQ,wBufferSize,DTP_GP_UI_QQ);
	}

	//电子邮件
	if (pDBOUserIndividual->szEMail[0]!=0) 
	{
		WORD wBufferSize=CountStringBuffer(pDBOUserIndividual->szEMail);
		SendPacket.AddPacket(pDBOUserIndividual->szEMail,wBufferSize,DTP_GP_UI_EMAIL);
	}

	//联系地址
	if (pDBOUserIndividual->szDwellingPlace[0]!=0)
	{
		WORD wBufferSize=CountStringBuffer(pDBOUserIndividual->szDwellingPlace);
		SendPacket.AddPacket(pDBOUserIndividual->szDwellingPlace,wBufferSize,DTP_GP_UI_DWELLING_PLACE);
	}

	//推广信息
	if (pDBOUserIndividual->szSpreader[0]!=0)
	{
		WORD wBufferSize=CountStringBuffer(pDBOUserIndividual->szSpreader);
		SendPacket.AddPacket(pDBOUserIndividual->szSpreader,wBufferSize,DTP_GP_UI_SPREADER);
	}

	//发送消息
	WORD wSendSize=sizeof(CMD_GP_UserIndividual)+SendPacket.GetDataSize();
	m_pITCPNetworkEngine->SendData(dwContextID, MDM_GP_USER_SERVICE, SUB_GP_USER_INDIVIDUAL, cbDataBuffer, wSendSize);

	return true;
}
//绑定推广
bool CAttemperEngineSink::OnDBUserBindSpreaderResult(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	if ((m_pBindParameter + LOWORD(dwContextID))->dwSocketID != dwContextID) return true;
	//变量定义
	DBO_GP_BindSpreaderResult * pBindSpreaderResult = (DBO_GP_BindSpreaderResult *)pData;
	//变量定义
	CMD_GP_BindSpreaderResult BindSpreaderResult;
	ZeroMemory(&BindSpreaderResult, sizeof(BindSpreaderResult));

	//构造数据
	BindSpreaderResult.dwDiamond = pBindSpreaderResult->dwDiamond;
	BindSpreaderResult.dwRewardDiamond = pBindSpreaderResult->dwRewardDiamond;
	lstrcpyn(BindSpreaderResult.szDescribeString, pBindSpreaderResult->szDescribeString, CountArray(BindSpreaderResult.szDescribeString));

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID, MDM_GP_USER_SERVICE, SUB_GP_BIND_SPREADER_RESULT, &BindSpreaderResult, sizeof(BindSpreaderResult));

	return true;
}

//银行信息
bool CAttemperEngineSink::OnDBPCUserInsureInfo(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_UserInsureInfo * pUserInsureInfo=(DBO_GP_UserInsureInfo *)pData;

	//变量定义
	CMD_GP_UserInsureInfo UserInsureInfo;
	ZeroMemory(&UserInsureInfo,sizeof(UserInsureInfo));

	//构造数据
	UserInsureInfo.cbEnjoinTransfer=pUserInsureInfo->cbEnjoinTransfer;
	UserInsureInfo.wRevenueTake=pUserInsureInfo->wRevenueTake;
	UserInsureInfo.wRevenueTransfer=pUserInsureInfo->wRevenueTransfer;
	UserInsureInfo.wRevenueTransferMember=pUserInsureInfo->wRevenueTransferMember;
	UserInsureInfo.wServerID=pUserInsureInfo->wServerID;
	UserInsureInfo.lUserScore=pUserInsureInfo->lUserScore;
	UserInsureInfo.lUserInsure=pUserInsureInfo->lUserInsure;
	UserInsureInfo.lTransferPrerequisite=pUserInsureInfo->lTransferPrerequisite;
		UserInsureInfo.dwUserRight=pUserInsureInfo->dwUserRight;

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_USER_INSURE_INFO,&UserInsureInfo,sizeof(UserInsureInfo));

	//关闭连接
	m_pITCPNetworkEngine->ShutDownSocket(dwContextID);

	return true;
}

//银行成功
bool CAttemperEngineSink::OnDBPCUserInsureSuccess(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_UserInsureSuccess * pUserInsureSuccess=(DBO_GP_UserInsureSuccess *)pData;

	//变量定义
	CMD_GP_UserInsureSuccess UserInsureSuccess;
	ZeroMemory(&UserInsureSuccess,sizeof(UserInsureSuccess));

	//设置变量
	UserInsureSuccess.dwUserID=pUserInsureSuccess->dwUserID;
	UserInsureSuccess.lUserScore=pUserInsureSuccess->lSourceScore+pUserInsureSuccess->lVariationScore;
	UserInsureSuccess.lUserInsure=pUserInsureSuccess->lSourceInsure+pUserInsureSuccess->lVariationInsure;
	lstrcpyn(UserInsureSuccess.szDescribeString,pUserInsureSuccess->szDescribeString,CountArray(UserInsureSuccess.szDescribeString));

	//发送消息
	WORD wDescribe=CountStringBuffer(UserInsureSuccess.szDescribeString);
	WORD wHeadSize=sizeof(UserInsureSuccess)-sizeof(UserInsureSuccess.szDescribeString);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_USER_INSURE_SUCCESS,&UserInsureSuccess,wHeadSize+wDescribe);

	return true;
}

//操作失败
bool CAttemperEngineSink::OnDBPCUserInsureFailure(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	CMD_GP_UserInsureFailure UserInsureFailure;
	ZeroMemory(&UserInsureFailure,sizeof(UserInsureFailure));

	//变量定义
	DBO_GP_UserInsureFailure * pUserInsureFailure=(DBO_GP_UserInsureFailure *)pData;

	//构造数据
	UserInsureFailure.lResultCode=pUserInsureFailure->lResultCode;
	lstrcpyn(UserInsureFailure.szDescribeString,pUserInsureFailure->szDescribeString,CountArray(UserInsureFailure.szDescribeString));

	//发送数据
	WORD wDescribe=CountStringBuffer(UserInsureFailure.szDescribeString);
	WORD wHeadSize=sizeof(UserInsureFailure)-sizeof(UserInsureFailure.szDescribeString);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_USER_INSURE_FAILURE,&UserInsureFailure,wHeadSize+wDescribe);

	//关闭连接
	m_pITCPNetworkEngine->ShutDownSocket(dwContextID);

	return true;
}

//用户信息
bool CAttemperEngineSink::OnDBPCUserInsureUserInfo(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_UserTransferUserInfo * pTransferUserInfo=(DBO_GP_UserTransferUserInfo *)pData;

	//变量定义
	CMD_GP_UserTransferUserInfo UserTransferUserInfo;
	ZeroMemory(&UserTransferUserInfo,sizeof(UserTransferUserInfo));

	//构造变量
	UserTransferUserInfo.dwTargetGameID=pTransferUserInfo->dwGameID;
	lstrcpyn(UserTransferUserInfo.szAccounts,pTransferUserInfo->szAccounts,CountArray(UserTransferUserInfo.szAccounts));

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_QUERY_USER_INFO_RESULT,&UserTransferUserInfo,sizeof(UserTransferUserInfo));

	return true;
}

//开通结果
bool CAttemperEngineSink::OnDBPCUserInsureEnableResult(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_UserInsureEnableResult * pUserInsureEnableResult=(DBO_GP_UserInsureEnableResult *)pData;

	//变量定义
	CMD_GP_UserInsureEnableResult UserInsureEnableResult;
	ZeroMemory(&UserInsureEnableResult,sizeof(UserInsureEnableResult));

	//构造变量
	UserInsureEnableResult.cbInsureEnabled=pUserInsureEnableResult->cbInsureEnabled;
	lstrcpyn(UserInsureEnableResult.szDescribeString,pUserInsureEnableResult->szDescribeString,CountArray(UserInsureEnableResult.szDescribeString));

	//发送数据
	WORD wHeadSize=CountStringBuffer(UserInsureEnableResult.szDescribeString);
	wHeadSize +=sizeof(UserInsureEnableResult)-sizeof(UserInsureEnableResult.szDescribeString);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_USER_INSURE_ENABLE_RESULT,&UserInsureEnableResult,wHeadSize);

	return true;
}

bool CAttemperEngineSink::OnDBPCQueryTransferRebateResult(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_QueryTransferRebateResult * pRebateResult=(DBO_GP_QueryTransferRebateResult *)pData;

	//变量定义
	CMD_GP_QueryTransferRebateResult RebateResult;
	ZeroMemory(&RebateResult,sizeof(RebateResult));

	RebateResult.dwUserID=pRebateResult->dwUserID;
	RebateResult.cbRebateEnabled = pRebateResult->cbRebateEnabled;
	RebateResult.lIngot = pRebateResult->lIngot;
	RebateResult.lLoveLiness = pRebateResult->lLoveLiness;

	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_QUERY_TRANSFER_REBATE_RESULT,&RebateResult,sizeof(RebateResult));
	return true;
}

//签到信息
bool CAttemperEngineSink::OnDBPCUserCheckInInfo(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_CheckInInfo * pCheckInInfo=(DBO_GP_CheckInInfo *)pData;

	//变量定义
	CMD_GP_CheckInInfo CheckInInfo;
	ZeroMemory(&CheckInInfo,sizeof(CheckInInfo));

	//构造变量
	CheckInInfo.bTodayChecked=pCheckInInfo->bTodayChecked;
	CheckInInfo.wSeriesDate=pCheckInInfo->wSeriesDate;
	CopyMemory(CheckInInfo.lRewardGold,m_lCheckInReward,sizeof(CheckInInfo.lRewardGold));

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_CHECKIN_INFO,&CheckInInfo,sizeof(CheckInInfo));

	return true;
}

//签到结果
bool CAttemperEngineSink::OnDBPCUserCheckInResult(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_CheckInResult * pCheckInResult=(DBO_GP_CheckInResult *)pData;

	//变量定义
	CMD_GP_CheckInResult CheckInResult;
	ZeroMemory(&CheckInResult,sizeof(CheckInResult));

	//构造变量
	CheckInResult.bSuccessed=pCheckInResult->bSuccessed;
	CheckInResult.lScore=pCheckInResult->lScore;
	lstrcpyn(CheckInResult.szNotifyContent,pCheckInResult->szNotifyContent,CountArray(CheckInResult.szNotifyContent));

	//发送数据
	WORD wSendSize = sizeof(CheckInResult)-sizeof(CheckInResult.szNotifyContent)+CountStringBuffer(CheckInResult.szNotifyContent);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_CHECKIN_RESULT,&CheckInResult,wSendSize);

	return true;
}

//签到奖励
bool CAttemperEngineSink::OnDBPCCheckInReward(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//参数校验
	ASSERT(wDataSize==sizeof(DBO_GP_CheckInReward));
	if(wDataSize!=sizeof(DBO_GP_CheckInReward)) return false;

	//提取数据
	DBO_GP_CheckInReward * pCheckInReward=(DBO_GP_CheckInReward *)pData;

	//拷贝数据
	CopyMemory(m_lCheckInReward,pCheckInReward->lRewardGold,sizeof(m_lCheckInReward));

	return true;
}

//任务列表
bool CAttemperEngineSink::OnDBPCTaskList(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//变量定义
	DBO_GP_TaskListInfo * pTaskListInfo=(DBO_GP_TaskListInfo *)pData;

	//拷贝数据	
	WORD wTaskCount = __min(pTaskListInfo->wTaskCount,CountArray(m_TaskParameterBuffer)-m_wTaskCountBuffer);
	CopyMemory(m_TaskParameterBuffer+m_wTaskCountBuffer,++pTaskListInfo,sizeof(tagTaskParameter)*wTaskCount);

	//设置变量
	m_wTaskCountBuffer += wTaskCount;

	return true;
}

//任务结束
bool CAttemperEngineSink::OnDBPCTaskListEnd(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//拷贝数据	
	m_wTaskCount = m_wTaskCountBuffer;
	CopyMemory(m_TaskParameter, m_TaskParameterBuffer, sizeof(tagTaskParameter)*m_wTaskCount);

	return true;
}

//任务信息
bool CAttemperEngineSink::OnDBPCTaskInfo(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_TaskInfo * pTaskInfo=(DBO_GP_TaskInfo *)pData;

	//变量定义
	WORD wTaskCount = m_wTaskCount;

	//网络数据
	WORD wSendSize=0,wParameterSize=0;
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];
	LPBYTE pDataBuffer = cbDataBuffer;

	//设置数量
	pDataBuffer += sizeof(wTaskCount);
	wSendSize = wSendSize;

	//分包发送
	for(WORD i=0;i<wTaskCount;i++)
	{
		//越界判断
		if(wSendSize+sizeof(tagTaskParameter)>SOCKET_TCP_PACKET)
		{
			//重置变量
			pDataBuffer = cbDataBuffer;
			wTaskCount -= i;
			*(WORD*)pDataBuffer = i; 

			//发送数据
			m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_TASK_LIST,&cbDataBuffer,wSendSize);

			//重置变量
			pDataBuffer += sizeof(wSendSize);
			wSendSize = wSendSize;
		}

		//计算大小
		wParameterSize = sizeof(tagTaskParameter)-sizeof(m_TaskParameter[i].szTaskDescribe)+CountStringBuffer(m_TaskParameter[i].szTaskDescribe);
		*(WORD *)pDataBuffer = wParameterSize;
		pDataBuffer += sizeof(wParameterSize);
		wSendSize += sizeof(wParameterSize);

		//拷贝数据
		CopyMemory(pDataBuffer,&m_TaskParameter[i],wParameterSize);

		//推进指针
		pDataBuffer += wParameterSize;
		wSendSize += wParameterSize;
	}

	//剩余发送
	if(wTaskCount>0 && wSendSize)
	{
		//重置变量
		pDataBuffer = cbDataBuffer;
		*(WORD *)pDataBuffer = wTaskCount; 

		//发送数据
		m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_TASK_LIST,&cbDataBuffer,wSendSize);
	}

	//构造结构
	CMD_GP_TaskInfo TaskInfo;
	TaskInfo.wTaskCount = pTaskInfo->wTaskCount;
	CopyMemory(TaskInfo.TaskStatus,pTaskInfo->TaskStatus,sizeof(TaskInfo.TaskStatus[0])*pTaskInfo->wTaskCount);

	//发送数据
	WORD wSendDataSize = sizeof(TaskInfo)-sizeof(TaskInfo.TaskStatus);
	wSendDataSize += sizeof(TaskInfo.TaskStatus[0])*TaskInfo.wTaskCount;
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_TASK_INFO,&TaskInfo,wSendDataSize);

	return true;
}

//任务结果
bool CAttemperEngineSink::OnDBPCTaskResult(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_TaskResult * pTaskResult=(DBO_GP_TaskResult *)pData;

	//变量定义
	CMD_GP_TaskResult TaskResult;
	ZeroMemory(&TaskResult,sizeof(TaskResult));

	//构造变量
	TaskResult.bSuccessed=pTaskResult->bSuccessed;
	TaskResult.wCommandID=pTaskResult->wCommandID;
	TaskResult.lCurrScore=pTaskResult->lCurrScore;
	TaskResult.lCurrIngot=pTaskResult->lCurrIngot;	
	lstrcpyn(TaskResult.szNotifyContent,pTaskResult->szNotifyContent,CountArray(TaskResult.szNotifyContent));

	//发送数据
	WORD wSendSize = sizeof(TaskResult)-sizeof(TaskResult.szNotifyContent)+CountStringBuffer(TaskResult.szNotifyContent);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_TASK_RESULT,&TaskResult,wSendSize);

	return true;
}
//视频列表
bool CAttemperEngineSink::OnDBMBVideoList(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_MB_VIDEO_PLAY_BACK_INFO,SUB_MB_QUERY_VIDEO_INFO_RESULT,pData,wDataSize);

	return true;
}

//视频结束
bool CAttemperEngineSink::OnDBMBVideoListEnd(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_MB_VIDEO_PLAY_BACK_INFO,SUB_MB_QUERY_VIDEO_INFO_END,pData,wDataSize);

	return true;
}

//视频详情
bool CAttemperEngineSink::OnDBMBVideoDetails(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_MB_VIDEO_PLAY_BACK_INFO,SUB_MB_QUERY_VIDEO_DETAILS_RESULT,pData,wDataSize);

	return true;
}

//详情结束
bool CAttemperEngineSink::OnDBMBVideoDetailsEnd(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_MB_VIDEO_PLAY_BACK_INFO,SUB_MB_QUERY_VIDEO_DETAILS_END,pData,wDataSize);

	return true;
}
//回放码
bool CAttemperEngineSink::OnDBMBPlayBackCodeYZ(DWORD dwContextID, VOID * pData, WORD wDataSize)
{	
	//参数校验
	ASSERT(wDataSize==sizeof(DBR_MB_QueryPlayBackCode_YZ_Result));
	if(wDataSize!=sizeof(DBR_MB_QueryPlayBackCode_YZ_Result)) return false;

	//变量定义
	DBR_MB_QueryPlayBackCode_YZ_Result * pCodeParameter=(DBR_MB_QueryPlayBackCode_YZ_Result *)pData;

	//构造结构
	CMD_MB_S_QueryPlayBackCode_YZ_Result CodeResult;
	CodeResult.dwUserID=pCodeParameter->dwUserID;
	CodeResult.dwPlayBackCode=pCodeParameter->dwPlayBackCode;
	lstrcpyn(CodeResult.szPersonalGUID,pCodeParameter->szPersonalGUID,CountArray(CodeResult.szPersonalGUID));

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_MB_VIDEO_PLAY_BACK_INFO,SUB_MB_QUERY_PLAYBACK_CODE_YZ_RESULT,&CodeResult,sizeof(CodeResult));

	return true;
}
//回放码
bool CAttemperEngineSink::OnDBMBPlayBackCode(DWORD dwContextID, VOID * pData, WORD wDataSize)
{	
	//参数校验
	ASSERT(wDataSize==sizeof(DBR_MB_QueryPlayBackCode_Result));
	if(wDataSize!=sizeof(DBR_MB_QueryPlayBackCode_Result)) return false;

	//变量定义
	DBR_MB_QueryPlayBackCode_Result * pCodeParameter=(DBR_MB_QueryPlayBackCode_Result *)pData;

	//构造结构
	CMD_MB_S_QueryPlayBackCode_Result CodeResult;
	CodeResult.dwUserID=pCodeParameter->dwUserID;
	CodeResult.dwPlayBackCode=pCodeParameter->dwPlayBackCode;
	lstrcpyn(CodeResult.szVideoNumber,pCodeParameter->szVideoNumber,CountArray(CodeResult.szVideoNumber));

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_MB_VIDEO_PLAY_BACK_INFO,SUB_MB_QUERY_PLAYBACK_CODE_RESULT,&CodeResult,sizeof(CodeResult));

	return true;
}

//低保参数
bool CAttemperEngineSink::OnDBPCBaseEnsureParameter(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//参数校验
	ASSERT(wDataSize==sizeof(DBO_GP_BaseEnsureParameter));
	if(wDataSize!=sizeof(DBO_GP_BaseEnsureParameter)) return false;

	//变量定义
	DBO_GP_BaseEnsureParameter * pEnsureParameter=(DBO_GP_BaseEnsureParameter *)pData;

	//设置变量
	m_BaseEnsureParameter.cbTakeTimes = pEnsureParameter->cbTakeTimes;
	m_BaseEnsureParameter.lScoreAmount = pEnsureParameter->lScoreAmount;
	m_BaseEnsureParameter.lScoreCondition = pEnsureParameter->lScoreCondition;

	return true;
}

//低保结果
bool CAttemperEngineSink::OnDBPCBaseEnsureResult(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_BaseEnsureResult * pBaseEnsureResult=(DBO_GP_BaseEnsureResult *)pData;

	//构造结构
	CMD_GP_BaseEnsureResult BaseEnsureResult;
	BaseEnsureResult.bSuccessed=pBaseEnsureResult->bSuccessed;
	BaseEnsureResult.lGameScore=pBaseEnsureResult->lGameScore;
	lstrcpyn(BaseEnsureResult.szNotifyContent,pBaseEnsureResult->szNotifyContent,CountArray(BaseEnsureResult.szNotifyContent));

	//发送数据
	WORD wSendDataSize=sizeof(BaseEnsureResult)-sizeof(BaseEnsureResult.szNotifyContent);
	wSendDataSize+=CountStringBuffer(BaseEnsureResult.szNotifyContent);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_BASEENSURE_RESULT,&BaseEnsureResult,wSendDataSize);

	return true;
}

//推广参数
bool CAttemperEngineSink::OnDBPCUserSpreadInfo(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_UserSpreadInfo * pSpreadParameter=(DBO_GP_UserSpreadInfo *)pData;

	//构造结构
	CMD_GP_UserSpreadInfo UserSpreadInfo;
	UserSpreadInfo.dwSpreadCount=pSpreadParameter->dwSpreadCount;
	UserSpreadInfo.lSpreadReward=pSpreadParameter->lSpreadReward;

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_SPREAD_INFO,&UserSpreadInfo,sizeof(UserSpreadInfo));

	return true;	
}

//实名奖励
bool CAttemperEngineSink::OnDBPCRealAuthParameter(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//变量定义
	DBO_GP_RealAuthParameter * pParameter=(DBO_GP_RealAuthParameter *)pData;

	m_AuthRealParameter.dwAuthRealAward = pParameter->dwAuthRealAward;
	m_AuthRealParameter.dwAuthentDisable= pParameter->dwAuthentDisable;

	return true;	

}


//等级配置
bool CAttemperEngineSink::OnDBPCGrowLevelConfig(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//变量定义
	DBO_GP_GrowLevelConfig * pGrowLevelConfig=(DBO_GP_GrowLevelConfig *)pData;

	//拷贝数据	
	m_wLevelCount = pGrowLevelConfig->wLevelCount;
	CopyMemory(m_GrowLevelConfig,&pGrowLevelConfig->GrowLevelConfig,sizeof(tagGrowLevelConfig)*m_wLevelCount);

	return true;
}

//等级参数
bool CAttemperEngineSink::OnDBPCGrowLevelParameter(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_GrowLevelParameter * pGrowLevelParameter=(DBO_GP_GrowLevelParameter *)pData;

	//构造结构
	CMD_GP_GrowLevelParameter GrowLevelParameter;
	GrowLevelParameter.wCurrLevelID=pGrowLevelParameter->wCurrLevelID;	
	GrowLevelParameter.dwExperience=pGrowLevelParameter->dwExperience;
	GrowLevelParameter.dwUpgradeExperience=pGrowLevelParameter->dwUpgradeExperience;
	GrowLevelParameter.lUpgradeRewardGold=pGrowLevelParameter->lUpgradeRewardGold;
	GrowLevelParameter.lUpgradeRewardIngot=pGrowLevelParameter->lUpgradeRewardIngot;

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_GROWLEVEL_PARAMETER,&GrowLevelParameter,sizeof(GrowLevelParameter));

	return true;
}

//等级升级
bool CAttemperEngineSink::OnDBPCGrowLevelUpgrade(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_GrowLevelUpgrade * pGrowLevelUpgrade=(DBO_GP_GrowLevelUpgrade *)pData;

	//构造结构
	CMD_GP_GrowLevelUpgrade GrowLevelUpgrade;
	GrowLevelUpgrade.lCurrScore=pGrowLevelUpgrade->lCurrScore;
	GrowLevelUpgrade.lCurrIngot=pGrowLevelUpgrade->lCurrIngot;
	lstrcpyn(GrowLevelUpgrade.szNotifyContent,pGrowLevelUpgrade->szNotifyContent,CountArray(GrowLevelUpgrade.szNotifyContent));

	//发送数据
	WORD wSendDataSize = sizeof(GrowLevelUpgrade)-sizeof(GrowLevelUpgrade.szNotifyContent);
	wSendDataSize += CountStringBuffer(GrowLevelUpgrade.szNotifyContent);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_GROWLEVEL_UPGRADE,&GrowLevelUpgrade,wSendDataSize);

	return true;
}

//道具信息
bool CAttemperEngineSink::OnDBGamePropertyRelatItem(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//效验参数
	ASSERT(wDataSize%sizeof(DBO_GP_GamePropertyRelatItem)==0);
	if (wDataSize%sizeof(DBO_GP_GamePropertyRelatItem)!=0) return false;

	//变量定义
	WORD wItemCount=wDataSize/sizeof(DBO_GP_GamePropertyRelatItem);
	DBO_GP_GamePropertyRelatItem * pGamePropertyRelatItem=(DBO_GP_GamePropertyRelatItem *)pData;

	//更新数据
	for (WORD i=0;i<wItemCount;i++)
	{
		//变量定义
		tagPropertyRelatItem GamePropertyRelat;
		ZeroMemory(&GamePropertyRelat,sizeof(GamePropertyRelat));

		//构造数据
		GamePropertyRelat.dwTypeID=(pGamePropertyRelatItem+i)->dwTypeID;
		GamePropertyRelat.dwPropertyID=(pGamePropertyRelatItem+i)->dwPropertyID;

		//插入列表
		m_GamePropertyListManager.InsertGamePropertyRelatItem(&GamePropertyRelat);
	}

	return true;

}

//道具信息
bool CAttemperEngineSink::OnDBGamePropertyTypeItem(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//效验参数
	ASSERT(wDataSize%sizeof(DBO_GP_GamePropertyTypeItem)==0);
	if (wDataSize%sizeof(DBO_GP_GamePropertyTypeItem)!=0) return false;

	//变量定义
	WORD wItemCount=wDataSize/sizeof(DBO_GP_GamePropertyTypeItem);
	DBO_GP_GamePropertyTypeItem * pGamePropertyTypeItem=(DBO_GP_GamePropertyTypeItem *)pData;

	//更新数据
	for (WORD i=0;i<wItemCount;i++)
	{
		//变量定义
		tagPropertyTypeItem GamePropertyType;
		ZeroMemory(&GamePropertyType,sizeof(GamePropertyType));

		//构造数据
		GamePropertyType.dwTypeID=(pGamePropertyTypeItem+i)->dwTypeID;
		GamePropertyType.dwSortID=(pGamePropertyTypeItem+i)->dwSortID;
		lstrcpyn(GamePropertyType.szTypeName,(pGamePropertyTypeItem+i)->szTypeName,CountArray(GamePropertyType.szTypeName));

		//插入列表
		m_GamePropertyListManager.InsertGamePropertyTypeItem(&GamePropertyType);
	}

	return true;

}
//道具信息
bool CAttemperEngineSink::OnDBGamePropertyItem(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//效验参数
	ASSERT(wDataSize%sizeof(DBO_GP_GamePropertyItem)==0);
	if (wDataSize%sizeof(DBO_GP_GamePropertyItem)!=0) return false;

	//变量定义
	WORD wItemCount=wDataSize/sizeof(DBO_GP_GamePropertyItem);
	DBO_GP_GamePropertyItem * pGamePropertyItem=(DBO_GP_GamePropertyItem *)pData;

	//查询存在
	for (WORD nNewIndex =0;nNewIndex<wItemCount;nNewIndex++)
	{
		DBO_GP_GamePropertyItem * pCurrGamePropertyItem = (DBO_GP_GamePropertyItem *)(pGamePropertyItem + nNewIndex);

		//变量定义
		tagPropertyItem GameProperty;
		ZeroMemory(&GameProperty,sizeof(GameProperty));

		GameProperty.dwPropertyID = pCurrGamePropertyItem->dwPropertyID;
		GameProperty.dwPropertyKind = pCurrGamePropertyItem->dwPropertyKind;
		GameProperty.cbUseArea = pCurrGamePropertyItem->cbUseArea;
		GameProperty.cbServiceArea = pCurrGamePropertyItem->cbServiceArea;
		GameProperty.cbRecommend = pCurrGamePropertyItem->cbRecommend;
		GameProperty.dwExchangeRatio = pCurrGamePropertyItem->dwExchangeRatio;
		/*GameProperty.lPropertyGold = pCurrGamePropertyItem->lPropertyGold;
		GameProperty.dPropertyCash = pCurrGamePropertyItem->dPropertyCash;
		GameProperty.lPropertyUserMedal = pCurrGamePropertyItem->lPropertyUserMedal;
		GameProperty.lPropertyLoveLiness = pCurrGamePropertyItem->lPropertyLoveLiness;*/
		GameProperty.lSendLoveLiness = pCurrGamePropertyItem->lSendLoveLiness;
		GameProperty.lRecvLoveLiness = pCurrGamePropertyItem->lRecvLoveLiness;
		GameProperty.lUseResultsGold = pCurrGamePropertyItem->lUseResultsGold;
		GameProperty.dwUseResultsValidTime = pCurrGamePropertyItem->dwUseResultsValidTime;
		GameProperty.dwUseResultsValidTimeScoreMultiple = pCurrGamePropertyItem->dwUseResultsValidTimeScoreMultiple;
		GameProperty.dwUseResultsGiftPackage = pCurrGamePropertyItem->dwUseResultsGiftPackage;
		GameProperty.dwSortID = pCurrGamePropertyItem->dwSortID;
		lstrcpyn(GameProperty.szName,pCurrGamePropertyItem->szName,CountArray(GameProperty.szName));
		lstrcpyn(GameProperty.szRegulationsInfo,pCurrGamePropertyItem->szRegulationsInfo,CountArray(GameProperty.szRegulationsInfo));

		//插入列表
		m_GamePropertyListManager.InsertGamePropertyItem(&GameProperty);
	}



	return true;
}

//道具信息
bool CAttemperEngineSink::OnDBGamePropertySubItem(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//效验参数
	ASSERT(wDataSize%sizeof(DBO_GP_GamePropertySubItem)==0);
	if (wDataSize%sizeof(DBO_GP_GamePropertySubItem)!=0) return false;

	//变量定义
	WORD wItemCount=wDataSize/sizeof(DBO_GP_GamePropertySubItem);
	DBO_GP_GamePropertySubItem * pGamePropertySubtem=(DBO_GP_GamePropertySubItem *)pData;

	//更新数据
	for (WORD i=0;i<wItemCount;i++)
	{
		//变量定义
		tagPropertySubItem GamePropertySub;
		ZeroMemory(&GamePropertySub,sizeof(GamePropertySub));

		//构造数据
		GamePropertySub.dwPropertyID=(pGamePropertySubtem+i)->dwPropertyID;
		GamePropertySub.dwPropertyCount=(pGamePropertySubtem+i)->dwPropertyCount;
		GamePropertySub.dwOwnerPropertyID = (pGamePropertySubtem+i)->dwOwnerPropertyID;
		GamePropertySub.dwSortID=(pGamePropertySubtem+i)->dwSortID;

		//插入列表
		m_GamePropertyListManager.InsertGamePropertySubItem(&GamePropertySub);
	}

	return true;

}

//道具结果
bool CAttemperEngineSink::OnDBGamePropertyListResult(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//变量定义
	DBO_GP_GamePropertyListResult * pGamePropertyInfo=(DBO_GP_GamePropertyListResult *)pData;

	//设置道具
	if (pGamePropertyInfo->cbSuccess == TRUE)
	{

	}

	return true;
}
//道具购买
bool CAttemperEngineSink::OnDBGamePropertyFailure(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_PropertyFailure * pPropertyFailure=(DBO_GP_PropertyFailure *)pData;

	//构造结构
	CMD_GP_PropertyFailure PropertyFailure;
	PropertyFailure.lErrorCode = pPropertyFailure->lErrorCode;
	lstrcpyn(PropertyFailure.szDescribeString,pPropertyFailure->szDescribeString,CountArray(PropertyFailure.szDescribeString));
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_PROPERTY,SUB_GP_PROPERTY_FAILURE,&PropertyFailure,sizeof(PropertyFailure));


	return true;
}

bool CAttemperEngineSink::OnDBQueryPropertySingle(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_PropertyQuerySingle * pPropertyBuyResult=(DBO_GP_PropertyQuerySingle *)pData;

	//构造结构
	CMD_GP_PropertyQuerySingleResult PropertyBuyResult;
	PropertyBuyResult.dwUserID = pPropertyBuyResult->dwUserID;
	PropertyBuyResult.dwPropertyID = pPropertyBuyResult->dwPropertyID;
	PropertyBuyResult.dwItemCount = pPropertyBuyResult->dwItemCount;
	m_pITCPNetworkEngine->SendData(dwContextID, MDM_GP_PROPERTY, SUB_GP_QUERY_SINGLE_RESULT, &PropertyBuyResult, sizeof(PropertyBuyResult));
	return true;
}

//道具购买
bool CAttemperEngineSink::OnDBGamePropertyBuy(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_PropertyBuyResult * pPropertyBuyResult=(DBO_GP_PropertyBuyResult *)pData;

	//构造结构
	CMD_GP_PropertyBuyResult PropertyBuyResult;
	PropertyBuyResult.dwUserID = pPropertyBuyResult->dwUserID;
	PropertyBuyResult.dwPropertyID = pPropertyBuyResult->dwPropertyID;
	PropertyBuyResult.dwItemCount = pPropertyBuyResult->dwItemCount;
	PropertyBuyResult.lDiamond = pPropertyBuyResult->lDiamond;
//	PropertyBuyResult.lInsureScore = pPropertyBuyResult->lInsureScore;
//	PropertyBuyResult.lUserMedal = pPropertyBuyResult->lUserMedal;
//	PropertyBuyResult.lLoveLiness = pPropertyBuyResult->lLoveLiness;
//	PropertyBuyResult.dCash = pPropertyBuyResult->dCash;
//	PropertyBuyResult.cbCurrMemberOrder = pPropertyBuyResult->cbCurrMemberOrder;
	lstrcpyn(PropertyBuyResult.szNotifyContent,pPropertyBuyResult->szNotifyContent,CountArray(PropertyBuyResult.szNotifyContent));
	m_pITCPNetworkEngine->SendData(dwContextID, MDM_GP_PROPERTY, SUB_GP_PROPERTY_BUY_RESULT, &PropertyBuyResult, sizeof(PropertyBuyResult));

	return true;
}


bool CAttemperEngineSink::OnDBQueryUserBackpack(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	DBO_GP_QueryBackpack* pQueryBackpackResult = (DBO_GP_QueryBackpack*)pData;


	//构造返回
	BYTE cbDataBuffer[SOCKET_TCP_PACKET]={0};
	CMD_GP_S_BackpackProperty* pBackpackPropertyResult = (CMD_GP_S_BackpackProperty*)cbDataBuffer;

	//初始化参数
	WORD dwDataBufferSize = sizeof(CMD_GP_S_BackpackProperty);
	pBackpackPropertyResult->dwUserID = pQueryBackpackResult->dwUserID;
	pBackpackPropertyResult->dwCount = pQueryBackpackResult->dwCount;
	pBackpackPropertyResult->dwStatus = pQueryBackpackResult->dwStatus;
	if(pQueryBackpackResult->dwCount > 0)
	{
		memcpy(pBackpackPropertyResult->PropertyInfo, pQueryBackpackResult->PropertyInfo, sizeof(tagBackpackProperty)*(pQueryBackpackResult->dwCount));
		dwDataBufferSize += sizeof(tagBackpackProperty)*(WORD)(pQueryBackpackResult->dwCount-1);
	}
	//发生数据
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_PROPERTY,SUB_GP_QUERY_BACKPACKET_RESULT,pBackpackPropertyResult,dwDataBufferSize);
	return true;
}

bool CAttemperEngineSink::OnDBUserPropertyBuff(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	DBO_GR_UserPropertyBuff* pUserPropertyBuff = (DBO_GR_UserPropertyBuff*)pData;

	CMD_GP_S_UserPropertyBuff  UserPropertyBuff;
	UserPropertyBuff.dwUserID = pUserPropertyBuff->dwUserID;
	UserPropertyBuff.cbBuffCount = pUserPropertyBuff->cbBuffCount;
	memcpy(UserPropertyBuff.PropertyBuff, pUserPropertyBuff->PropertyBuff, sizeof(tagPropertyBuff)*pUserPropertyBuff->cbBuffCount);

	WORD dwHeadSize=sizeof(UserPropertyBuff)-sizeof(UserPropertyBuff.PropertyBuff);
	WORD dwDataSize=UserPropertyBuff.cbBuffCount*sizeof(UserPropertyBuff.PropertyBuff[0]);

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_PROPERTY,SUB_GP_PROPERTY_BUFF_RESULT,&UserPropertyBuff,dwHeadSize+dwDataSize);
	return true;
}

bool CAttemperEngineSink::OnDBGamePropertyUse(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	DBO_GP_PropertyUse* pPropertyUseResult = (DBO_GP_PropertyUse*)pData;

	//构造结构
	CMD_GP_S_PropertyUse PropertyUseResult;

	PropertyUseResult.dwUserID = pPropertyUseResult->dwUserID;
	PropertyUseResult.dwRecvUserID = pPropertyUseResult->dwRecvUserID;
	PropertyUseResult.dwPropID = pPropertyUseResult->dwPropID;
	PropertyUseResult.dwPropKind = pPropertyUseResult->dwPropKind;
	PropertyUseResult.wPropCount = pPropertyUseResult->wPropCount;
	PropertyUseResult.Score = pPropertyUseResult->Score;
	PropertyUseResult.dwRemainderPropCount = pPropertyUseResult->dwRemainderPropCount;
	PropertyUseResult.dwScoreMultiple = pPropertyUseResult->dwScoreMultiple;
	PropertyUseResult.lRecvLoveLiness = pPropertyUseResult->lRecvLoveLiness;
	PropertyUseResult.lSendLoveLiness = pPropertyUseResult->lSendLoveLiness;
	PropertyUseResult.lUseResultsGold = pPropertyUseResult->lUseResultsGold;
	PropertyUseResult.UseResultsValidTime = pPropertyUseResult->UseResultsValidTime;
	PropertyUseResult.tUseTime = pPropertyUseResult->tUseTime;
	PropertyUseResult.dwHandleCode = pPropertyUseResult->dwHandleCode;
	PropertyUseResult.cbMemberOrder = pPropertyUseResult->cbMemberOrder;
	lstrcpyn(PropertyUseResult.szName, pPropertyUseResult->szName,CountArray(PropertyUseResult.szName));
	lstrcpyn(PropertyUseResult.szNotifyContent, pPropertyUseResult->szNotifyContent,CountArray(PropertyUseResult.szNotifyContent));
	WORD wSendSize=sizeof(PropertyUseResult)-sizeof(PropertyUseResult.szNotifyContent)+CountStringBuffer(PropertyUseResult.szNotifyContent);

	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_PROPERTY,SUB_GP_PROPERTY_USE_RESULT,&PropertyUseResult,wSendSize);
	
	return true;
}

bool CAttemperEngineSink::OnDBUserPropertyPresent(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	DBO_GP_PropertyPresent* pPropertyPresent = (DBO_GP_PropertyPresent*)pData;

	//构造结构
	CMD_GP_S_PropertyPresent PropertyPresentResult={0};
	PropertyPresentResult.dwUserID = pPropertyPresent->dwUserID;
	PropertyPresentResult.dwRecvGameID = pPropertyPresent->dwRecvGameID;
	PropertyPresentResult.dwPropID = pPropertyPresent->dwPropID;
	PropertyPresentResult.wPropCount = pPropertyPresent->wPropCount;
	PropertyPresentResult.wType = pPropertyPresent->wType;
	PropertyPresentResult.nHandleCode = pPropertyPresent->nHandleCode;
	
	lstrcpyn(PropertyPresentResult.szRecvNickName, pPropertyPresent->szRecvNickName,CountArray(PropertyPresentResult.szRecvNickName));
	lstrcpyn(PropertyPresentResult.szNotifyContent, pPropertyPresent->szNotifyContent,CountArray(PropertyPresentResult.szNotifyContent));

	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_PROPERTY,SUB_GP_PROPERTY_PRESENT_RESULT,&PropertyPresentResult,sizeof(PropertyPresentResult));
	return true;
}

bool CAttemperEngineSink::OnDBQuerySendPresent(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	DBO_GP_QuerySendPresent* pQuerySendPresent = (DBO_GP_QuerySendPresent*)pData;
	CMD_GP_S_QuerySendPresent QuerySendPresentResult={0};
	QuerySendPresentResult.wPresentCount = pQuerySendPresent->wPresentCount;
	memcpy(QuerySendPresentResult.Present, pQuerySendPresent->Present, sizeof(pQuerySendPresent->wPresentCount*sizeof(pQuerySendPresent->Present[0])));
	
	WORD dwHeadSize=sizeof(QuerySendPresentResult)-sizeof(QuerySendPresentResult.Present);
	WORD dwDataSize=QuerySendPresentResult.wPresentCount*sizeof(QuerySendPresentResult.Present[0]);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_PROPERTY,SUB_GP_QUERY_SEND_PRESENT_RESULT,&QuerySendPresentResult,dwHeadSize+dwDataSize);
	return true;
}

bool CAttemperEngineSink::OnDBGetSendPresent(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	DBO_GP_GetSendPresent* pGetSendPresent = (DBO_GP_GetSendPresent*)pData;
	CMD_GP_S_GetSendPresent GetSendPresentResult={0};
	GetSendPresentResult.wPresentCount = pGetSendPresent->wPresentCount;
	memcpy(GetSendPresentResult.Present, pGetSendPresent->Present, pGetSendPresent->wPresentCount*sizeof(SendPresent));

	WORD dwHeadSize=sizeof(GetSendPresentResult)-sizeof(GetSendPresentResult.Present);
	WORD dwDataSize=GetSendPresentResult.wPresentCount*sizeof(GetSendPresentResult.Present[0]);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_PROPERTY,SUB_GP_GET_SEND_PRESENT_RESULT,&GetSendPresentResult,dwHeadSize+dwDataSize);
	return true;
}

//会员参数
bool CAttemperEngineSink::OnDBPCMemberParameter(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//变量定义
	DBO_GP_MemberParameter * pMemberParameter=(DBO_GP_MemberParameter *)pData;

	//拷贝数据	
	m_wMemberCount = pMemberParameter->wMemberCount;
	CopyMemory(m_MemberParameter,pMemberParameter->MemberParameter,sizeof(tagMemberParameterNew)*m_wMemberCount);

	return true;
}

//会员查询
bool CAttemperEngineSink::OnDBPCMemberDayQueryInfoResult( DWORD dwContextID, VOID * pData, WORD wDataSize )
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_MemberQueryInfoResult * pMemberResult=(DBO_GP_MemberQueryInfoResult *)pData;

	//构造结构
	CMD_GP_MemberQueryInfoResult MemberResult;
	MemberResult.bPresent=pMemberResult->bPresent;
	MemberResult.bGift=pMemberResult->bGift;
	//拷贝数据	
	MemberResult.GiftSubCount = pMemberResult->GiftSubCount;
	CopyMemory(MemberResult.GiftSub,pMemberResult->GiftSub,sizeof(tagGiftPropertyInfo)*MemberResult.GiftSubCount);

	//发送数据
	//计算大小
	WORD wSendDataSize = sizeof(MemberResult)-sizeof(MemberResult.GiftSub);
	wSendDataSize += sizeof(tagGiftPropertyInfo)*(WORD)(MemberResult.GiftSubCount);

	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_MEMBER_QUERY_INFO_RESULT,&MemberResult,wSendDataSize);

	return true;
}

//会员送金
bool CAttemperEngineSink::OnDBPCMemberDayPresentResult( DWORD dwContextID, VOID * pData, WORD wDataSize )
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_MemberDayPresentResult * pMemberResult=(DBO_GP_MemberDayPresentResult *)pData;

	//构造结构
	CMD_GP_MemberDayPresentResult MemberResult;
	MemberResult.bSuccessed=pMemberResult->bSuccessed;
	MemberResult.lGameScore=pMemberResult->lGameScore;
	lstrcpyn(MemberResult.szNotifyContent,pMemberResult->szNotifyContent,CountArray(MemberResult.szNotifyContent));

	//发送数据
	WORD wSendDataSize = sizeof(MemberResult)-sizeof(MemberResult.szNotifyContent);
	wSendDataSize += CountStringBuffer(MemberResult.szNotifyContent);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_MEMBER_DAY_PRESENT_RESULT,&MemberResult,wSendDataSize);

	return true;

}

//会员礼包
bool CAttemperEngineSink::OnDBPCMemberDayGiftResult( DWORD dwContextID, VOID * pData, WORD wDataSize )
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_MemberDayGiftResult * pMemberResult=(DBO_GP_MemberDayGiftResult *)pData;

	//构造结构
	CMD_GP_MemberDayGiftResult MemberResult;
	MemberResult.bSuccessed=pMemberResult->bSuccessed;
	lstrcpyn(MemberResult.szNotifyContent,pMemberResult->szNotifyContent,CountArray(MemberResult.szNotifyContent));

	//发送数据
	WORD wSendDataSize = sizeof(MemberResult)-sizeof(MemberResult.szNotifyContent);
	wSendDataSize += CountStringBuffer(MemberResult.szNotifyContent);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_MEMBER_DAY_GIFT_RESULT,&MemberResult,wSendDataSize);
	return true;
}

//购买结果
bool CAttemperEngineSink::OnDBPCPurchaseResult(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//提取数据
	DBO_GP_PurchaseResult * pPurchaseResult=(DBO_GP_PurchaseResult *)pData;

	//构造结构
	CMD_GP_PurchaseResult PurchaseResult;
	ZeroMemory(&PurchaseResult,sizeof(PurchaseResult));

	//设置变量
	PurchaseResult.bSuccessed=pPurchaseResult->bSuccessed;
	PurchaseResult.lCurrScore=pPurchaseResult->lCurrScore;
	PurchaseResult.dCurrBeans=pPurchaseResult->dCurrBeans;
	PurchaseResult.cbMemberOrder=pPurchaseResult->cbMemberOrder;
	lstrcpyn(PurchaseResult.szNotifyContent,pPurchaseResult->szNotifyContent,CountArray(PurchaseResult.szNotifyContent));

	//发送数据
	WORD wSendDataSize = sizeof(PurchaseResult)-sizeof(PurchaseResult.szNotifyContent);
	wSendDataSize += CountStringBuffer(PurchaseResult.szNotifyContent);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_PURCHASE_RESULT,&PurchaseResult,wSendDataSize);

	return true;
}

//房卡兑换结果
bool CAttemperEngineSink::OnDBPCExChangeRoomCardToScoreResult(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//提取数据
	DBO_GP_RoomCardExchangeResult * pExchangeResult=(DBO_GP_RoomCardExchangeResult *)pData;

	//构造结构
	CMD_GP_ExchangeRoomCardResult ExchangeResult;
	ZeroMemory(&ExchangeResult,sizeof(ExchangeResult));

	//设置变量
	ExchangeResult.bSuccessed=pExchangeResult->bSuccessed;
	ExchangeResult.lCurrScore=pExchangeResult->lCurrScore;
	ExchangeResult.lRoomCard=pExchangeResult->lCurrRoomCard;
	lstrcpyn(ExchangeResult.szNotifyContent,pExchangeResult->szNotifyContent,CountArray(ExchangeResult.szNotifyContent));

	//发送数据
	WORD wSendDataSize = sizeof(ExchangeResult)-sizeof(ExchangeResult.szNotifyContent);
	wSendDataSize += CountStringBuffer(ExchangeResult.szNotifyContent);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_MB_PERSONAL_SERVICE, SUB_GP_EXCHANGE_ROOM_CARD_RESULT,&ExchangeResult,wSendDataSize);

	return true;
}

//兑换结果
bool CAttemperEngineSink::OnDBPCExChangeResult(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//提取数据
	DBO_GP_ExchangeResult * pExchangeResult=(DBO_GP_ExchangeResult *)pData;

	//构造结构
	CMD_GP_ExchangeResult ExchangeResult;
	ZeroMemory(&ExchangeResult,sizeof(ExchangeResult));

	//设置变量
	ExchangeResult.bSuccessed=pExchangeResult->bSuccessed;
	ExchangeResult.lCurrScore=pExchangeResult->lCurrScore;
	ExchangeResult.lCurrIngot=pExchangeResult->lCurrIngot;
	ExchangeResult.dCurrBeans=pExchangeResult->dCurrBeans;
	lstrcpyn(ExchangeResult.szNotifyContent,pExchangeResult->szNotifyContent,CountArray(ExchangeResult.szNotifyContent));

	//发送数据
	WORD wSendDataSize = sizeof(ExchangeResult)-sizeof(ExchangeResult.szNotifyContent);
	wSendDataSize += CountStringBuffer(ExchangeResult.szNotifyContent);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_EXCHANGE_RESULT,&ExchangeResult,wSendDataSize);

	return true;
}

//机器操作
bool CAttemperEngineSink::OnDBAndroidParameter(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;
	
	//变量定义
	DBO_GP_AndroidParameter * pAndroidParameter=(DBO_GP_AndroidParameter *)pData;

	//变量定义
	CMD_GP_AndroidParameter AndroidParameter;
	ZeroMemory(&AndroidParameter,sizeof(AndroidParameter));

	//构造变量
	AndroidParameter.wSubCommdID=pAndroidParameter->wSubCommdID;
	AndroidParameter.wParameterCount=pAndroidParameter->wParameterCount;
	CopyMemory(AndroidParameter.AndroidParameter,pAndroidParameter->AndroidParameter,sizeof(tagAndroidParameter)*AndroidParameter.wParameterCount);

	//计算大小
	WORD wSendDataSize = sizeof(AndroidParameter)-sizeof(AndroidParameter.AndroidParameter);
	wSendDataSize += sizeof(tagAndroidParameter)*AndroidParameter.wParameterCount;

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_ANDROID_SERVICE,SUB_GP_ANDROID_PARAMETER,&AndroidParameter,wSendDataSize);

	//数量判断
	if(AndroidParameter.wParameterCount==1)
	{
		//协调房间
		if(AndroidParameter.wSubCommdID==SUB_GP_ADD_PARAMETER)
		{
			//构造结构
			CMD_CS_C_AddParameter AddParameter;
			AddParameter.wServerID=pAndroidParameter->wServerID;
			CopyMemory(&AddParameter.AndroidParameter,&AndroidParameter.AndroidParameter[0],sizeof(tagAndroidParameter));

			//发送数据
			m_pITCPSocketService->SendData(MDM_CS_ANDROID_SERVICE,SUB_CS_C_ADDPARAMETER,&AddParameter,sizeof(AddParameter));
		}

		//协调房间
		if(AndroidParameter.wSubCommdID==SUB_GP_MODIFY_PARAMETER)
		{
			//构造结构
			CMD_CS_C_ModifyParameter ModifyParameter;
			ModifyParameter.wServerID=pAndroidParameter->wServerID;
			CopyMemory(&ModifyParameter.AndroidParameter,&AndroidParameter.AndroidParameter[0],sizeof(tagAndroidParameter));

			//发送数据
			m_pITCPSocketService->SendData(MDM_CS_ANDROID_SERVICE,SUB_CS_C_MODIFYPARAMETER,&ModifyParameter,sizeof(ModifyParameter));
		}

		//协调房间
		if(AndroidParameter.wSubCommdID==SUB_GP_DELETE_PARAMETER)
		{
			//构造结构
			CMD_CS_C_DeleteParameter DeleteParameter;
			DeleteParameter.wServerID=pAndroidParameter->wServerID;
			DeleteParameter.dwBatchID=AndroidParameter.AndroidParameter[0].dwBatchID;

			//发送数据
			m_pITCPSocketService->SendData(MDM_CS_ANDROID_SERVICE,SUB_CS_C_DELETEPARAMETER,&DeleteParameter,sizeof(DeleteParameter));
		}
	}

	return true;
}

bool CAttemperEngineSink::OnDBIndividualResult( DWORD dwContextID, VOID * pData, WORD wDataSize )
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//提取数据
	DBO_GP_IndividualResult * pResult=(DBO_GP_IndividualResult *)pData;

	//构造结构
	CMD_GP_IndividuaResult currResult;
	ZeroMemory(&currResult,sizeof(currResult));

	//设置变量
	currResult.bSuccessed=pResult->bSuccessed;
	currResult.lCurrDiamond =pResult->lDiamond;
	lstrcpyn(currResult.szNotifyContent,pResult->szDescribeString,CountArray(currResult.szNotifyContent));

	//发送数据
	WORD wSendDataSize = sizeof(currResult)-sizeof(currResult.szNotifyContent);
	wSendDataSize += CountStringBuffer(currResult.szNotifyContent);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_INDIVIDUAL_RESULT,&currResult,wSendDataSize);

	return true;
}

//操作成功
bool CAttemperEngineSink::OnDBPCOperateSuccess(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	CMD_GP_OperateSuccess OperateSuccess;
	ZeroMemory(&OperateSuccess,sizeof(OperateSuccess));

	//变量定义
	DBO_GP_OperateSuccess * pOperateSuccess=(DBO_GP_OperateSuccess *)pData;

	//构造数据
	OperateSuccess.lResultCode=pOperateSuccess->lResultCode;
	lstrcpyn(OperateSuccess.szDescribeString,pOperateSuccess->szDescribeString,CountArray(OperateSuccess.szDescribeString));

	//发送数据
	WORD wDescribe=CountStringBuffer(OperateSuccess.szDescribeString);
	WORD wHeadSize=sizeof(OperateSuccess)-sizeof(OperateSuccess.szDescribeString);

	m_pITCPNetworkEngine->SendData(dwContextID, MDM_GP_USER_SERVICE, SUB_GP_OPERATE_SUCCESS, &OperateSuccess, wHeadSize + wDescribe);

	//关闭连接
	if(pOperateSuccess->bCloseSocket==true)
	{
		m_pITCPNetworkEngine->ShutDownSocket(dwContextID);
	}
	return true;
}

//操作失败
bool CAttemperEngineSink::OnDBPCOperateFailure(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	CMD_GP_OperateFailure OperateFailure;
	ZeroMemory(&OperateFailure,sizeof(OperateFailure));

	//变量定义
	DBO_GP_OperateFailure * pOperateFailure=(DBO_GP_OperateFailure *)pData;

	//构造数据
	OperateFailure.lResultCode=pOperateFailure->lResultCode;
	lstrcpyn(OperateFailure.szDescribeString,pOperateFailure->szDescribeString,CountArray(OperateFailure.szDescribeString));

	//发送数据
	WORD wDescribe=CountStringBuffer(OperateFailure.szDescribeString);
	WORD wHeadSize=sizeof(OperateFailure)-sizeof(OperateFailure.szDescribeString);
	m_pITCPNetworkEngine->SendData(dwContextID, MDM_GP_USER_SERVICE, SUB_GP_OPERATE_FAILURE, &OperateFailure, wHeadSize + wDescribe);

	//关闭连接
	if(pOperateFailure->bCloseSocket==true)
	{
		m_pITCPNetworkEngine->ShutDownSocket(dwContextID);
	}
	return true;
}

//登录成功
bool CAttemperEngineSink::OnDBMBLogonSuccess(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];
	DBO_MB_LogonSuccess * pDBOLogonSuccess=(DBO_MB_LogonSuccess *)pData;
	CMD_MB_LogonSuccess * pCMDLogonSuccess=(CMD_MB_LogonSuccess *)cbDataBuffer;

	//发送定义
	WORD wHeadSize=sizeof(CMD_MB_LogonSuccess);
	CSendPacketHelper SendPacket(cbDataBuffer+wHeadSize,sizeof(cbDataBuffer)-wHeadSize);

	//设置变量
	ZeroMemory(pCMDLogonSuccess,sizeof(CMD_MB_LogonSuccess));
	//构造数据
	pCMDLogonSuccess->wFaceID=pDBOLogonSuccess->wFaceID;
	pCMDLogonSuccess->dwUserID=pDBOLogonSuccess->dwUserID;
	pCMDLogonSuccess->dwGameID=pDBOLogonSuccess->dwGameID;
	pCMDLogonSuccess->dwSpreaderID=pDBOLogonSuccess->dwSpreaderID;
	lstrcpyn(pCMDLogonSuccess->szAccounts,pDBOLogonSuccess->szAccounts,CountArray(pCMDLogonSuccess->szAccounts));
	lstrcpyn(pCMDLogonSuccess->szNickName,pDBOLogonSuccess->szNickName,CountArray(pCMDLogonSuccess->szNickName));
	lstrcpyn(pCMDLogonSuccess->szDynamicPass,pDBOLogonSuccess->szDynamicPass,CountArray(pCMDLogonSuccess->szDynamicPass));
	//用户成绩
	pCMDLogonSuccess->lUserScore=pDBOLogonSuccess->lUserScore;
	pCMDLogonSuccess->lUserInsure=pDBOLogonSuccess->lUserInsure;
	pCMDLogonSuccess->lPresent = pDBOLogonSuccess->lPresent;
	//扩展信息
	pCMDLogonSuccess->cbInsureEnabled=pDBOLogonSuccess->cbInsureEnabled;
	pCMDLogonSuccess->cbIsAgent=pDBOLogonSuccess->cbIsAgent;
	pCMDLogonSuccess->cbMoorMachine=pDBOLogonSuccess->cbMoorMachine;
	pCMDLogonSuccess->dwLockServerID = pDBOLogonSuccess->dwLockServerID;
	pCMDLogonSuccess->dwKindID = pDBOLogonSuccess->dwKindID;

	//会员信息
	if (pDBOLogonSuccess->cbMemberOrder!=0 )
	{
		DTP_GP_MemberInfo MemberInfo;
		ZeroMemory(&MemberInfo,sizeof(MemberInfo));
		MemberInfo.cbMemberOrder=pDBOLogonSuccess->cbMemberOrder;
		MemberInfo.MemberOverDate=pDBOLogonSuccess->MemberOverDate;
		SendPacket.AddPacket(&MemberInfo,sizeof(MemberInfo),DTP_GP_MEMBER_INFO);
	}

	//个性签名
	if (pDBOLogonSuccess->szUnderWrite[0]!=0)
	{
		SendPacket.AddPacket(pDBOLogonSuccess->szUnderWrite,CountStringBuffer(pDBOLogonSuccess->szUnderWrite),DTP_GP_UNDER_WRITE);
	}

	//登录成功
	WORD wSendSize=SendPacket.GetDataSize()+sizeof(CMD_MB_LogonSuccess);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_MB_LOGON,SUB_MB_LOGON_SUCCESS,cbDataBuffer,wSendSize);

	//会员配置
//	SendMemberConfig(dwContextID);

	//发送房间
	WORD wIndex=LOWORD(dwContextID);
	SendMobileKindInfo(dwContextID,(m_pBindParameter+wIndex)->wModuleID);
	SendMobileServerInfo(dwContextID,(m_pBindParameter+wIndex)->wModuleID);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_MB_SERVER_LIST,SUB_MB_LIST_FINISH);

	return true;
}

//登录失败
bool CAttemperEngineSink::OnDBMBLogonFailure(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	CMD_MB_LogonFailure LogonFailure;
	ZeroMemory(&LogonFailure,sizeof(LogonFailure));
	DBO_MB_LogonFailure * pLogonFailure=(DBO_MB_LogonFailure *)pData;

	//构造数据
	LogonFailure.lResultCode=pLogonFailure->lResultCode;
	lstrcpyn(LogonFailure.szDescribeString,pLogonFailure->szDescribeString,CountArray(LogonFailure.szDescribeString));

	//发送数据
	WORD wStringSize=CountStringBuffer(LogonFailure.szDescribeString);
	WORD wSendSize=sizeof(LogonFailure)-sizeof(LogonFailure.szDescribeString)+wStringSize;
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_MB_LOGON,SUB_MB_LOGON_FAILURE,&LogonFailure,wSendSize);

	//关闭连接
	m_pITCPNetworkEngine->ShutDownSocket(dwContextID);

	return true;
}

//抽奖配置
bool CAttemperEngineSink::OnDBPCLotteryConfig(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_LotteryConfig * pLotteryConfig=(DBO_GP_LotteryConfig *)pData;

	//变量定义
	CMD_GP_LotteryConfig LotteryConfig;
	ZeroMemory(&LotteryConfig,sizeof(LotteryConfig));

	//构造变量
	LotteryConfig.wLotteryCount=pLotteryConfig->wLotteryCount;
	CopyMemory(LotteryConfig.LotteryItem,pLotteryConfig->LotteryItem,sizeof(LotteryConfig.LotteryItem));

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_LOTTERY_CONFIG,&LotteryConfig,sizeof(LotteryConfig));

	return true;
}

//抽奖信息
bool CAttemperEngineSink::OnDBPCLotteryUserInfo(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_LotteryUserInfo * pLotteryConfig=(DBO_GP_LotteryUserInfo *)pData;

	//变量定义
	CMD_GP_LotteryUserInfo LotteryUserInfo;
	ZeroMemory(&LotteryUserInfo,sizeof(LotteryUserInfo));

	//构造变量
	LotteryUserInfo.cbFreeCount=pLotteryConfig->cbFreeCount;
	LotteryUserInfo.cbAlreadyCount=pLotteryConfig->cbAlreadyCount;
	LotteryUserInfo.wKindID=pLotteryConfig->wKindID;
	LotteryUserInfo.dwUserID=pLotteryConfig->dwUserID;
	LotteryUserInfo.lChargeFee=pLotteryConfig->lChargeFee;

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_LOTTERY_USER_INFO,&LotteryUserInfo,sizeof(LotteryUserInfo));

	return true;
}

//抽奖结果
bool CAttemperEngineSink::OnDBPCLotteryResult(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	DBO_GP_LotteryResult * pLotteryResult=(DBO_GP_LotteryResult *)pData;

	//变量定义
	CMD_GP_LotteryResult LotteryResult;
	ZeroMemory(&LotteryResult,sizeof(LotteryResult));

	//构造变量
	LotteryResult.bWined=pLotteryResult->bWined;
	LotteryResult.wKindID=pLotteryResult->wKindID;
	LotteryResult.dwUserID=pLotteryResult->dwUserID;
	LotteryResult.lUserScore=pLotteryResult->lUserScore;
	LotteryResult.dUserBeans=pLotteryResult->dUserBeans;
	CopyMemory(&LotteryResult.LotteryItem,&pLotteryResult->LotteryItem,sizeof(LotteryResult.LotteryItem));

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_LOTTERY_RESULT,&LotteryResult,sizeof(LotteryResult));

	return true;
}

//游戏数据
bool CAttemperEngineSink::OnDBPCQueryUserGameData(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParameter->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];
	DBO_GP_QueryUserGameData * pDBOQueryUserGameData=(DBO_GP_QueryUserGameData *)pData;
	CMD_GP_QueryUserGameData * pCMDQueryUserGameData=(CMD_GP_QueryUserGameData *)cbDataBuffer;
	CSendPacketHelper SendPacket(cbDataBuffer+sizeof(CMD_GP_QueryUserGameData),sizeof(cbDataBuffer)-sizeof(CMD_GP_QueryUserGameData));

	//设置变量
	ZeroMemory(pCMDQueryUserGameData,sizeof(CMD_GP_QueryUserGameData));

	//构造变量
	pCMDQueryUserGameData->wKindID=pDBOQueryUserGameData->wKindID;
	pCMDQueryUserGameData->dwUserID=pDBOQueryUserGameData->dwUserID;

	//联系地址
	if (pDBOQueryUserGameData->szUserGameData[0]!=0)
	{
		WORD wBufferSize=CountStringBuffer(pDBOQueryUserGameData->szUserGameData);
		SendPacket.AddPacket(pDBOQueryUserGameData->szUserGameData,wBufferSize,DTP_GP_UI_USER_GAME_DATA);
	}

	//发送消息
	WORD wSendSize=sizeof(CMD_GP_QueryUserGameData)+SendPacket.GetDataSize();
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_QUERY_USER_GAME_DATA,cbDataBuffer,wSendSize);

	return true;
}

//代理列表
bool CAttemperEngineSink::OnDBPCAgentGameList(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//效验参数
	ASSERT(wDataSize%sizeof(tagAgentGameKind)==0);
	if (wDataSize%sizeof(tagAgentGameKind)!=0) return false;
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_SERVER_LIST,SUB_GP_AGENT_KIND,pData,wDataSize);

	return true;
}

//代理列表
bool CAttemperEngineSink::OnDBMBAgentGameList(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//效验参数
	ASSERT(wDataSize%sizeof(tagAgentGameKind)==0);
	if (wDataSize%sizeof(tagAgentGameKind)!=0) return false;
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_MB_SERVER_LIST,SUB_MB_AGENT_KIND,pData,wDataSize);

	return true;
}

//约战房间配置
bool CAttemperEngineSink::OnDBMBPersonalParameter(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//效验参数
	ASSERT(wDataSize%sizeof(tagPersonalRoomOption)==0);
	if (wDataSize%sizeof(tagPersonalRoomOption)!=0) return false;

	m_pITCPNetworkEngine->SendData(dwContextID, MDM_MB_PERSONAL_SERVICE, SUB_MB_PERSONAL_PARAMETER, pData, wDataSize);

	return true;
}

//约战房间配置
bool CAttemperEngineSink::OnDBMBPersonalFeeList(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//效验参数
	ASSERT(wDataSize%sizeof(tagPersonalTableFeeList)==0);
	if (wDataSize%sizeof(tagPersonalTableFeeList)!=0) return false;

	m_pITCPNetworkEngine->SendData(dwContextID, MDM_MB_PERSONAL_SERVICE, SUB_MB_PERSONAL_FEE_PARAMETER, pData, wDataSize);

	return true;
}
//私人房间配置
bool CAttemperEngineSink::OnDBMBPersonalCellScore(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//效验参数
	ASSERT(wDataSize%sizeof(tagPersonalCellScore) == 0);
	if (wDataSize%sizeof(tagPersonalCellScore) != 0) return false;

	m_pITCPNetworkEngine->SendData(dwContextID, MDM_MB_PERSONAL_SERVICE, SUB_MB_PERSONAL_CELL_SCORE, pData, wDataSize);

	return true;
}
//私人房间定制配置
bool CAttemperEngineSink::OnDBMBPersonalRule(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//效验参数
	ASSERT(wDataSize==sizeof(tagGetPersonalRule));
	if (wDataSize != sizeof(tagGetPersonalRule)) return false;

	m_pITCPNetworkEngine->SendData(dwContextID, MDM_MB_PERSONAL_SERVICE, SUB_MB_PERSONAL_RULE_RESULT, pData, wDataSize);

	return true;
}
//请求约战房间列表
bool CAttemperEngineSink::OnDBMBPersonalRoomListInfo(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//效验参数
	ASSERT(wDataSize%sizeof(DBO_MB_PersonalRoomInfoList)==0);
	if (wDataSize%sizeof(DBO_MB_PersonalRoomInfoList)!=0) return false;
	DBO_MB_PersonalRoomInfoList * pDBOPersonalRoomInfoList = (DBO_MB_PersonalRoomInfoList *)pData;
	CMD_MB_PersonalRoomInfoList  PersonalRoomInfoList;
	memcpy(&PersonalRoomInfoList, pDBOPersonalRoomInfoList,  sizeof(PersonalRoomInfoList));
	
	m_pITCPNetworkEngine->SendData(dwContextID, MDM_MB_PERSONAL_SERVICE, SUB_MB_QUERY_PERSONAL_ROOM_LIST_RESULT, &PersonalRoomInfoList, sizeof(CMD_MB_PersonalRoomInfoList));

	return true;
}

//约战房间玩家请求房间信息
bool CAttemperEngineSink::OnDBQueryUserRoomScore(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	if (wDataSize == 0)
	{
		m_pITCPNetworkEngine->SendData(dwContextID, MDM_MB_PERSONAL_SERVICE, SUB_GR_USER_QUERY_ROOM_SCORE_RESULT, NULL, 0);
		return true;
	}
	//效验参数
	ASSERT(wDataSize%sizeof(tagQueryPersonalRoomUserScore)==0);
	if (wDataSize%sizeof(tagQueryPersonalRoomUserScore)!=0) return false;

	m_pITCPNetworkEngine->SendData(dwContextID, MDM_MB_PERSONAL_SERVICE, SUB_GR_USER_QUERY_ROOM_SCORE_RESULT, pData, wDataSize);

	return true;

}

//约战房间玩家请求房间信息
bool CAttemperEngineSink::OnDBQueryPersonalRoomUersInfoResult(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//效验参数
	ASSERT(wDataSize==sizeof(DBO_MB_PersonalRoomUserInfo));
	if (wDataSize!=sizeof(DBO_MB_PersonalRoomUserInfo)) return false;
	DBO_MB_PersonalRoomUserInfo * pUserInfo = (DBO_MB_PersonalRoomUserInfo *)pData;
	CMD_MB_PersonalRoomUserInfo userInfo;
	//userInfo.dBeans = pUserInfo->dBeans;
	userInfo.lDiamond = pUserInfo->lDiamond;

	m_pITCPNetworkEngine->SendData(dwContextID, MDM_MB_PERSONAL_SERVICE, SUB_MB_QUERY_PERSONAL_ROOM_USER_INFO_RESULT, &userInfo, sizeof(userInfo));

	return true;

}

//游戏种类
bool CAttemperEngineSink::OnDBPCGameTypeItem(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//效验参数
	ASSERT(wDataSize%sizeof(DBO_GP_GameType)==0);
	if (wDataSize%sizeof(DBO_GP_GameType)!=0) return false;

	//变量定义
	WORD wItemCount=wDataSize/sizeof(DBO_GP_GameType);
	DBO_GP_GameType * pGameType=(DBO_GP_GameType *)pData;

	//更新数据
	for (WORD i=0;i<wItemCount;i++)
	{
		//变量定义
		tagGameType GameType;
		ZeroMemory(&GameType,sizeof(GameType));

		//构造数据
		GameType.wTypeID=(pGameType+i)->wTypeID;
		GameType.wJoinID=(pGameType+i)->wJoinID;
		GameType.wSortID=(pGameType+i)->wSortID;
		lstrcpyn(GameType.szTypeName,(pGameType+i)->szTypeName,CountArray(GameType.szTypeName));

		//插入列表
		m_ServerListManager.InsertGameType(&GameType);
	}

	return true;
}

//游戏类型
bool CAttemperEngineSink::OnDBPCGameKindItem(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//效验参数
	ASSERT(wDataSize%sizeof(DBO_GP_GameKind)==0);
	if (wDataSize%sizeof(DBO_GP_GameKind)!=0) return false;

	//变量定义
	WORD wItemCount=wDataSize/sizeof(DBO_GP_GameKind);
	DBO_GP_GameKind * pGameKind=(DBO_GP_GameKind *)pData;

	//更新数据
	for (WORD i=0;i<wItemCount;i++)
	{
		//变量定义
		tagGameKind GameKind;
		ZeroMemory(&GameKind,sizeof(GameKind));

		//构造数据
		GameKind.wTypeID=(pGameKind+i)->wTypeID;
		GameKind.wJoinID=(pGameKind+i)->wJoinID;
		GameKind.wSortID=(pGameKind+i)->wSortID;
		GameKind.wKindID=(pGameKind+i)->wKindID;
		GameKind.wGameID=(pGameKind+i)->wGameID;
		GameKind.wRecommend=(pGameKind+i)->wRecommend;
		GameKind.wGameFlag=(pGameKind+i)->wGameFlag;
		GameKind.dwOnLineCount=m_ServerListManager.CollectOnlineInfo((pGameKind+i)->wKindID,false);
		GameKind.dwAndroidCount=m_ServerListManager.CollectOnlineInfo((pGameKind+i)->wKindID,true);
		GameKind.dwSetCount = m_ServerListManager.CollectSetPlayer((pGameKind + i)->wKindID);
		lstrcpyn(GameKind.szKindName,(pGameKind+i)->szKindName,CountArray(GameKind.szKindName));
		lstrcpyn(GameKind.szProcessName,(pGameKind+i)->szProcessName,CountArray(GameKind.szProcessName));

		//插入列表
		m_ServerListManager.InsertGameKind(&GameKind);
	}

	return true;
}

//游戏节点
bool CAttemperEngineSink::OnDBPCGameNodeItem(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//效验参数
	ASSERT(wDataSize%sizeof(DBO_GP_GameNode)==0);
	if (wDataSize%sizeof(DBO_GP_GameNode)!=0) return false;

	//变量定义
	WORD wItemCount=wDataSize/sizeof(DBO_GP_GameNode);
	DBO_GP_GameNode * pGameNode=(DBO_GP_GameNode *)pData;

	//更新数据
	for (WORD i=0;i<wItemCount;i++)
	{
		//变量定义
		tagGameNode GameNode;
		ZeroMemory(&GameNode,sizeof(GameNode));

		//构造数据
		GameNode.wKindID=(pGameNode+i)->wKindID;
		GameNode.wJoinID=(pGameNode+i)->wJoinID;
		GameNode.wSortID=(pGameNode+i)->wSortID;
		GameNode.wNodeID=(pGameNode+i)->wNodeID;
		lstrcpyn(GameNode.szNodeName,(pGameNode+i)->szNodeName,CountArray(GameNode.szNodeName));

		//插入列表
		m_ServerListManager.InsertGameNode(&GameNode);
	}

	return true;
}

//游戏定制
bool CAttemperEngineSink::OnDBPCGamePageItem(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//效验参数
	ASSERT(wDataSize%sizeof(DBO_GP_GamePage)==0);
	if (wDataSize%sizeof(DBO_GP_GamePage)!=0) return false;

	//变量定义
	WORD wItemCount=wDataSize/sizeof(DBO_GP_GamePage);
	DBO_GP_GamePage * pGamePage=(DBO_GP_GamePage *)pData;

	//更新数据
	for (WORD i=0;i<wItemCount;i++)
	{
		//变量定义
		tagGamePage GamePage;
		ZeroMemory(&GamePage,sizeof(GamePage));

		//构造数据
		GamePage.wKindID=(pGamePage+i)->wKindID;
		GamePage.wNodeID=(pGamePage+i)->wNodeID;
		GamePage.wSortID=(pGamePage+i)->wSortID;
		GamePage.wPageID=(pGamePage+i)->wPageID;
		GamePage.wOperateType=(pGamePage+i)->wOperateType;
		lstrcpyn(GamePage.szDisplayName,(pGamePage+i)->szDisplayName,CountArray(GamePage.szDisplayName));

		//插入列表
		m_ServerListManager.InsertGamePage(&GamePage);
	}

	return true;
}

//游戏列表
bool CAttemperEngineSink::OnDBPCGameListResult(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//效验参数
	ASSERT(wDataSize==sizeof(DBO_GP_GameListResult));
	if (wDataSize!=sizeof(DBO_GP_GameListResult)) return false;

	//变量定义
	DBO_GP_GameListResult * pGameListResult=(DBO_GP_GameListResult *)pData;

	//消息处理
	if (pGameListResult->cbSuccess==TRUE)
	{
		//清理列表
		m_ServerListManager.CleanKernelItem();

		//事件通知
		CP_ControlResult ControlResult;
		ControlResult.cbSuccess=ER_SUCCESS;
		SendUIControlPacket(UI_LOAD_DB_LIST_RESULT,&ControlResult,sizeof(ControlResult));

		//设置时间
		ASSERT(m_pITimerEngine!=NULL);
		m_pITimerEngine->SetTimer(IDI_LOAD_GAME_LIST,m_pInitParameter->m_wLoadListTime*1000L,1,0);
	}
	else
	{
		//构造提示
		TCHAR szDescribe[128]=TEXT("");
		_sntprintf_s(szDescribe,CountArray(szDescribe),TEXT("服务器列表加载失败，%ld 秒后将重新加载"),m_pInitParameter->m_wReLoadListTime);

		//提示消息
		CTraceService::TraceString(szDescribe,TraceLevel_Warning);

		//设置时间
		ASSERT(m_pITimerEngine!=NULL);
		m_pITimerEngine->SetTimer(IDI_LOAD_GAME_LIST,m_pInitParameter->m_wReLoadListTime*1000L,1,0);
	}

	return true;
}

//版本检测
bool CAttemperEngineSink::CheckPlazaVersion(BYTE cbDeviceType, DWORD dwPlazaVersion, DWORD dwSocketID, bool bCheckLowVer)
{
	//变量定义
	bool bMustUpdate=false;
	bool bAdviceUpdate=false;
	DWORD dwVersion=VERSION_PLAZA;

	//手机版本
	if(cbDeviceType >= DEVICE_TYPE_IPAD) dwVersion=VERSION_MOBILE_IOS;
	else if(cbDeviceType >= DEVICE_TYPE_IPHONE) dwVersion=VERSION_MOBILE_IOS;
	else if(cbDeviceType >= DEVICE_TYPE_ITOUCH) dwVersion=VERSION_MOBILE_IOS;
	else if(cbDeviceType >= DEVICE_TYPE_ANDROID) dwVersion=VERSION_MOBILE_ANDROID;
	else if(cbDeviceType == DEVICE_TYPE_PC) dwVersion=VERSION_PLAZA;

	//版本判断
	if (bCheckLowVer && GetSubVer(dwPlazaVersion)<GetSubVer(dwVersion)) bAdviceUpdate=true;
	if (GetMainVer(dwPlazaVersion)!=GetMainVer(dwVersion)) bMustUpdate=true;
	if (GetProductVer(dwPlazaVersion)!=GetProductVer(dwVersion)) bMustUpdate=true;

	//升级判断
	if ((bMustUpdate==true)||(bAdviceUpdate==true))
	{
		//变量定义
		CMD_GP_UpdateNotify UpdateNotify;
		ZeroMemory(&UpdateNotify,sizeof(UpdateNotify));

		//变量定义
		UpdateNotify.cbMustUpdate=bMustUpdate;
		UpdateNotify.cbAdviceUpdate=bAdviceUpdate;
		UpdateNotify.dwCurrentVersion=dwVersion;

		//发送消息
		m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_LOGON,SUB_GP_UPDATE_NOTIFY,&UpdateNotify,sizeof(UpdateNotify));

		//中断判断
		if (bMustUpdate==true) 
		{
			m_pITCPNetworkEngine->ShutDownSocket(dwSocketID);
			return false;
		}
	}

	return true;
}

//发送请求
bool CAttemperEngineSink::SendUIControlPacket(WORD wRequestID, VOID * pData, WORD wDataSize)
{
	//发送数据
	CServiceUnits * pServiceUnits=CServiceUnits::g_pServiceUnits;
	pServiceUnits->PostControlRequest(wRequestID,pData,wDataSize);

	return true;
}

//发送类型
VOID CAttemperEngineSink::SendGameTypeInfo(DWORD dwSocketID)
{
	//网络数据
	WORD wSendSize=0;
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];

	//枚举数据
	POSITION Position=NULL;
	CGameTypeItem * pGameTypeItem=NULL;

	//枚举数据
	for (DWORD i=0;i<m_ServerListManager.GetGameTypeCount();i++)
	{
		//发送数据
		if ((wSendSize+sizeof(tagGameType))>sizeof(cbDataBuffer))
		{
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_TYPE,cbDataBuffer,wSendSize);
			wSendSize=0;
		}

		//获取数据
		pGameTypeItem=m_ServerListManager.EmunGameTypeItem(Position);
		if (pGameTypeItem==NULL) break;

		//拷贝数据
		CopyMemory(cbDataBuffer+wSendSize,&pGameTypeItem->m_GameType,sizeof(tagGameType));
		wSendSize+=sizeof(tagGameType);
	}

	//发送剩余
	if (wSendSize>0) m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_TYPE,cbDataBuffer,wSendSize);

	return;
}

//发送种类
VOID CAttemperEngineSink::SendGameKindInfo(DWORD dwSocketID)
{
	//网络数据
	WORD wSendSize=0;
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];

	//枚举数据
	POSITION Position=NULL;
	CGameKindItem * pGameKindItem=NULL;

	//枚举数据
	for (DWORD i=0;i<m_ServerListManager.GetGameKindCount();i++)
	{
		//发送数据
		if ((wSendSize+sizeof(tagGameKind))>sizeof(cbDataBuffer))
		{
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_KIND,cbDataBuffer,wSendSize);
			wSendSize=0;
		}

		//获取数据
		pGameKindItem=m_ServerListManager.EmunGameKindItem(Position);
		if (pGameKindItem==NULL) break;

		//拷贝数据
		CopyMemory(cbDataBuffer+wSendSize,&pGameKindItem->m_GameKind,sizeof(tagGameKind));
		wSendSize+=sizeof(tagGameKind);
	}

	//发送剩余
	if (wSendSize>0) m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_KIND,cbDataBuffer,wSendSize);

	return;
}

//发送节点
VOID CAttemperEngineSink::SendGameNodeInfo(DWORD dwSocketID, WORD wKindID)
{
	//网络数据
	WORD wSendSize=0;
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];

	//枚举数据
	POSITION Position=NULL;
	CGameNodeItem * pGameNodeItem=NULL;

	//枚举数据
	for (DWORD i=0;i<m_ServerListManager.GetGameNodeCount();i++)
	{
		//发送数据
		if ((wSendSize+sizeof(tagGameNode))>sizeof(cbDataBuffer))
		{
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_NODE,cbDataBuffer,wSendSize);
			wSendSize=0;
		}

		//获取数据
		pGameNodeItem=m_ServerListManager.EmunGameNodeItem(Position);
		if (pGameNodeItem==NULL) break;

		//拷贝数据
		if ((wKindID==INVALID_WORD)||(pGameNodeItem->m_GameNode.wKindID==wKindID))
		{
			CopyMemory(cbDataBuffer+wSendSize,&pGameNodeItem->m_GameNode,sizeof(tagGameNode));
			wSendSize+=sizeof(tagGameNode);
		}
	}

	//发送剩余
	if (wSendSize>0) m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_NODE,cbDataBuffer,wSendSize);

	return;
}

//发送定制
VOID CAttemperEngineSink::SendGamePageInfo(DWORD dwSocketID, WORD wKindID)
{
	//网络数据
	WORD wSendSize=0;
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];

	//枚举数据
	POSITION Position=NULL;
	CGamePageItem * pGamePageItem=NULL;

	//枚举数据
	for (DWORD i=0;i<m_ServerListManager.GetGamePageCount();i++)
	{
		//发送数据
		if ((wSendSize+sizeof(tagGamePage))>sizeof(cbDataBuffer))
		{
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_PAGE,cbDataBuffer,wSendSize);
			wSendSize=0;
		}

		//获取数据
		pGamePageItem=m_ServerListManager.EmunGamePageItem(Position);
		if (pGamePageItem==NULL) break;

		//拷贝数据
		if ((wKindID==INVALID_WORD)||(pGamePageItem->m_GamePage.wKindID==wKindID))
		{
			CopyMemory(cbDataBuffer+wSendSize,&pGamePageItem->m_GamePage,sizeof(tagGamePage));
			wSendSize+=sizeof(tagGamePage);
		}
	}

	//发送剩余
	if (wSendSize>0) m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_PAGE,cbDataBuffer,wSendSize);

	return;
}

//发送房间
VOID CAttemperEngineSink::SendGameServerInfo(DWORD dwSocketID, WORD wKindID, BYTE cbDeviceType)
{
	//网络数据
	WORD wSendSize=0;
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];

	//枚举数据
	POSITION Position=NULL;
	CGameServerItem * pGameServerItem=NULL;

	//枚举数据
	for (DWORD i=0;i<m_ServerListManager.GetGameServerCount();i++)
	{
		//发送数据
		if ((wSendSize+sizeof(tagGameServer))>sizeof(cbDataBuffer))
		{
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_SERVER,cbDataBuffer,wSendSize);
			wSendSize=0;
		}

		//获取数据
		pGameServerItem=m_ServerListManager.EmunGameServerItem(Position);
		if (pGameServerItem==NULL) break;
		//支持类型
		bool bSuportMobile = CServerRule::IsSuportMobile(pGameServerItem->m_GameServer.dwServerRule);
		bool bSurportPC = CServerRule::IsSuportPC(pGameServerItem->m_GameServer.dwServerRule);

		if(cbDeviceType == DEVICE_TYPE_PC)
		{
			if(bSurportPC == false) continue; 
		}
		else if(cbDeviceType != DEVICE_TYPE_PC)
		{
			if(bSuportMobile == false) continue;
		}

		//拷贝数据
		if ((wKindID==INVALID_WORD)||(pGameServerItem->m_GameServer.wKindID==wKindID))
		{
			CopyMemory(cbDataBuffer+wSendSize,&pGameServerItem->m_GameServer,sizeof(tagGameServer));
			wSendSize+=sizeof(tagGameServer);
		}
	}

	//发送剩余
	if (wSendSize>0) m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_SERVER,cbDataBuffer,wSendSize);
	//设置变量
	wSendSize=0;
	ZeroMemory(cbDataBuffer,sizeof(cbDataBuffer));

	//枚举数据
	for (DWORD i=0;i<m_ServerListManager.GetGameServerCount();i++)
	{
		//发送数据
		if ((wSendSize+sizeof(tagGameMatch))>sizeof(cbDataBuffer))
		{
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_MATCH,cbDataBuffer,wSendSize);
			wSendSize=0;
		}

		//获取数据
		pGameServerItem=m_ServerListManager.EmunGameServerItem(Position);
		if (pGameServerItem==NULL) break;
		if (pGameServerItem->IsMatchServer()==false) continue;

		//拷贝数据
		if ((wKindID==INVALID_WORD)||(pGameServerItem->m_GameServer.wKindID==wKindID))
		{
			CopyMemory(cbDataBuffer+wSendSize,&pGameServerItem->m_GameMatch,sizeof(tagGameMatch));
			wSendSize+=sizeof(tagGameMatch);
		}
	}

	//发送剩余
	if (wSendSize>0) m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_MATCH,cbDataBuffer,wSendSize);

	if(m_wAVServerPort!=0 && m_dwAVServerAddr!=0)
	{
		//变量定义
		tagAVServerOption AVServerOption;
		AVServerOption.wAVServerPort=m_wAVServerPort;
		AVServerOption.dwAVServerAddr=m_dwAVServerAddr;

		//发送配置
		m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_VIDEO_OPTION,&AVServerOption,sizeof(AVServerOption));
	};

	return;
}

//等级配置
VOID CAttemperEngineSink::SendGrowLevelConfig(DWORD dwSocketID)
{
	//构造结构
	CMD_GP_GrowLevelConfig GrowLevelConfig;
	GrowLevelConfig.wLevelCount = m_wLevelCount;
	CopyMemory(GrowLevelConfig.GrowLevelItem,m_GrowLevelConfig,sizeof(tagGrowLevelConfig)*GrowLevelConfig.wLevelCount);

	//发送数据
	WORD wDataSize = sizeof(GrowLevelConfig)-sizeof(GrowLevelConfig.GrowLevelItem);
	wDataSize += sizeof(tagGrowLevelConfig)*GrowLevelConfig.wLevelCount;
	m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_LOGON,SUB_GP_GROWLEVEL_CONFIG,&GrowLevelConfig,wDataSize);

	return;
}

//道具类型
VOID CAttemperEngineSink::SendGamePropertyTypeInfo(DWORD dwSocketID)
{
	//网络数据
	WORD wSendSize=0;
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];

	//枚举数据
	POSITION Position=NULL;
	CGamePropertyTypeItem * pGamePropertyTypeItem=NULL;
	DWORD dwCount = m_GamePropertyListManager.GetGamePropertyTypeCount();

	//枚举数据
	for (DWORD i=0;i<dwCount;i++)
	{
		//发送数据
		if ((wSendSize+sizeof(tagPropertyTypeItem))>sizeof(cbDataBuffer))
		{
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_PROPERTY_TYPE,cbDataBuffer,wSendSize);
			wSendSize=0;
		}

		//获取数据
		pGamePropertyTypeItem=m_GamePropertyListManager.EmunGamePropertyTypeItem(Position);
		if (pGamePropertyTypeItem==NULL) break;

		//拷贝数据
		CopyMemory(cbDataBuffer+wSendSize,&pGamePropertyTypeItem->m_PropertyTypeItem,sizeof(tagPropertyTypeItem));
		wSendSize+=sizeof(tagPropertyTypeItem);
	}

	//发送剩余
	if (wSendSize>0) m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_PROPERTY_TYPE,cbDataBuffer,wSendSize);

	return;
}

//道具关系
VOID CAttemperEngineSink::SendGamePropertyRelatInfo(DWORD dwSocketID)
{
	//网络数据
	WORD wSendSize=0;
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];

	//枚举数据
	POSITION Position=NULL;
	CGamePropertyRelatItem * pGamePropertyRelatItem=NULL;
	DWORD dwCount = m_GamePropertyListManager.GetGamePropertyRelatCount();

	//枚举数据
	for (DWORD i=0;i<dwCount;i++)
	{
		//发送数据
		if ((wSendSize+sizeof(tagPropertyRelatItem))>sizeof(cbDataBuffer))
		{
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_PROPERTY_RELAT,cbDataBuffer,wSendSize);
			wSendSize=0;
		}

		//获取数据
		pGamePropertyRelatItem=m_GamePropertyListManager.EmunGamePropertyRelatItem(Position);
		if (pGamePropertyRelatItem==NULL) break;

		//拷贝数据
		CopyMemory(cbDataBuffer+wSendSize,&pGamePropertyRelatItem->m_PropertyRelatItem,sizeof(tagPropertyRelatItem));
		wSendSize+=sizeof(tagPropertyRelatItem);
	}

	//发送剩余
	if (wSendSize>0) m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_PROPERTY_RELAT,cbDataBuffer,wSendSize);


	return;
}

//发送道具
VOID CAttemperEngineSink::SendGamePropertyInfo(DWORD dwSocketID)
{
	//网络数据
	WORD wSendSize=0;
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];

	//枚举数据
	POSITION Position=NULL;
	CGamePropertyItem * pGamePropertyItem=NULL;
	DWORD dwCount = m_GamePropertyListManager.GetGamePropertyCount();

	//枚举数据
	for (DWORD i=0;i<dwCount;i++)
	{
		//发送数据
		if ((wSendSize+sizeof(tagPropertyItem))>sizeof(cbDataBuffer))
		{
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_PROPERTY,cbDataBuffer,wSendSize);
			wSendSize=0;
		}

		//获取数据
		pGamePropertyItem=m_GamePropertyListManager.EmunGamePropertyItem(Position);
		if (pGamePropertyItem==NULL) break;

		//拷贝数据
		CopyMemory(cbDataBuffer+wSendSize,&pGamePropertyItem->m_PropertyItem,sizeof(tagPropertyItem));
		wSendSize+=sizeof(tagPropertyItem);
	}

	//发送剩余
	if (wSendSize>0) m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_PROPERTY,cbDataBuffer,wSendSize);


	return;
}


//发送道具
VOID CAttemperEngineSink::SendGamePropertySubInfo(DWORD dwSocketID)
{
	//网络数据
	WORD wSendSize=0;
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];

	//枚举数据
	POSITION Position=NULL;
	CGamePropertySubItem * pGamePropertySubItem=NULL;
	DWORD dwCount = m_GamePropertyListManager.GetGamePropertySubCount();

	//枚举数据
	for (DWORD i=0;i<dwCount;i++)
	{
		//发送数据
		if ((wSendSize+sizeof(CGamePropertySubItem))>sizeof(cbDataBuffer))
		{
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_PROPERTY_SUB,cbDataBuffer,wSendSize);
			wSendSize=0;
		}

		//获取数据
		pGamePropertySubItem=m_GamePropertyListManager.EmunGamePropertySubItem(Position);
		if (pGamePropertySubItem==NULL) break;

		//拷贝数据
		CopyMemory(cbDataBuffer+wSendSize,&pGamePropertySubItem->m_PropertySubItem,sizeof(tagPropertySubItem));
		wSendSize+=sizeof(tagPropertySubItem);
	}

	//发送剩余
	if (wSendSize>0) m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_PROPERTY_SUB,cbDataBuffer,wSendSize);


	return;
}

//会员配置
VOID CAttemperEngineSink::SendMemberConfig(DWORD dwContextID)
{
	//会员配置
	CMD_GP_MemberParameterResult MemberParameterResult;
	MemberParameterResult.wMemberCount = m_wMemberCount;
	CopyMemory(MemberParameterResult.MemberParameter,m_MemberParameter,sizeof(tagMemberParameterNew)*m_wMemberCount);

	WORD wConfigMemberHead=sizeof(MemberParameterResult)-sizeof(MemberParameterResult.MemberParameter);
	WORD wConfigMemberInfo=MemberParameterResult.wMemberCount*sizeof(MemberParameterResult.MemberParameter[0]);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_LOGON,SUB_GP_MEMBER_PARAMETER_RESULT,&MemberParameterResult,wConfigMemberHead+wConfigMemberInfo);

	return;
}

VOID CAttemperEngineSink::SendRealAuthConfig(DWORD dwContextID)
{
	//实名认证
	CMD_GP_RealAuthParameter CmdParameter;
	CopyMemory(&CmdParameter,&m_AuthRealParameter,sizeof(CmdParameter));
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_LOGON,SUB_GP_REAL_AUTH_CONFIG,&CmdParameter,sizeof(CmdParameter));
	return;
}

//发送类型
VOID CAttemperEngineSink::SendMobileKindInfo(DWORD dwSocketID, WORD wModuleID)
{
	return;
}

//发送房间
VOID CAttemperEngineSink::SendMobileServerInfo(DWORD dwSocketID, WORD wModuleID)
{
	//网络数据
	WORD wSendSize=0;
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];

	//枚举数据
	POSITION Position=NULL;
	CGameServerItem * pGameServerItem=NULL;
	CGameKindItem * pGameKindItem=NULL;
	//枚举数据
	for (DWORD i=0;i<m_ServerListManager.GetGameServerCount();i++)
	{
		//发送数据
		if ((wSendSize+sizeof(tagGameServer))>sizeof(cbDataBuffer))
		{
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_MB_SERVER_LIST,SUB_MB_LIST_SERVER,cbDataBuffer,wSendSize);
			wSendSize=0;
		}

		//获取数据
		pGameServerItem=m_ServerListManager.EmunGameServerItem(Position);
		if (pGameServerItem==NULL) break;



		//支持类型
		bool bServerSuportMobile = CServerRule::IsSuportMobile(pGameServerItem->m_GameServer.dwServerRule);

		//拷贝数据
		if ((bServerSuportMobile==true)&&((wModuleID==INVALID_WORD)||(pGameServerItem->m_GameServer.wKindID==wModuleID)))
		{
			CopyMemory(cbDataBuffer+wSendSize,&pGameServerItem->m_GameServer,sizeof(tagGameServer));
			wSendSize+=sizeof(tagGameServer);
		}
	}

	//发送剩余
	if (wSendSize>0) 
	{
		m_pITCPNetworkEngine->SendData(dwSocketID,MDM_MB_SERVER_LIST,SUB_MB_LIST_SERVER,cbDataBuffer,wSendSize);
	}

	//设置变量
	wSendSize=0;
	ZeroMemory(cbDataBuffer,sizeof(cbDataBuffer));

	//枚举数据
	for (DWORD i=0;i<m_ServerListManager.GetGameServerCount();i++)
	{
		//发送数据
		if ((wSendSize+sizeof(tagGameMatch))>sizeof(cbDataBuffer))
		{
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_MB_SERVER_LIST,SUB_MB_LIST_MATCH,cbDataBuffer,wSendSize);
			wSendSize=0;
		}

		//获取数据
		pGameServerItem=m_ServerListManager.EmunGameServerItem(Position);
		if (pGameServerItem==NULL) break;
		if (pGameServerItem->IsMatchServer()==false) continue;

		//拷贝数据
		if ((wModuleID==INVALID_WORD)||(pGameServerItem->m_GameServer.wKindID==wModuleID))
		{
			CopyMemory(cbDataBuffer+wSendSize,&pGameServerItem->m_GameMatch,sizeof(tagGameMatch));
			wSendSize+=sizeof(tagGameMatch);
		}
	}

	//发送剩余
	if (wSendSize > 0)
	{
		m_pITCPNetworkEngine->SendData(dwSocketID, MDM_MB_SERVER_LIST, SUB_MB_LIST_MATCH, cbDataBuffer, wSendSize);
	}

	return;
}

//银行失败
bool CAttemperEngineSink::SendInsureFailure(DWORD dwSocketID, LONG lResultCode, LPCTSTR pszDescribe)
{
	//变量定义
	CMD_GP_UserInsureFailure UserInsureFailure;
	ZeroMemory(&UserInsureFailure,sizeof(UserInsureFailure));

	//构造数据
	UserInsureFailure.lResultCode=lResultCode;
	lstrcpyn(UserInsureFailure.szDescribeString,pszDescribe,CountArray(UserInsureFailure.szDescribeString));

	//发送数据
	WORD wDescribe=CountStringBuffer(UserInsureFailure.szDescribeString);
	WORD wHeadSize=sizeof(UserInsureFailure)-sizeof(UserInsureFailure.szDescribeString);
	m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_USER_SERVICE,SUB_GP_USER_INSURE_FAILURE,&UserInsureFailure,wHeadSize+wDescribe);

	//关闭连接
	m_pITCPNetworkEngine->ShutDownSocket(dwSocketID);

	return true;
}

//操作成功
VOID CAttemperEngineSink::SendOperateSuccess(DWORD dwContextID, LONG lResultCode, LPCTSTR pszDescribe)
{
	//效验参数
	ASSERT(pszDescribe != NULL);
	if(pszDescribe == NULL) return;

	//变量定义
	CMD_GP_OperateSuccess OperateSuccess;
	ZeroMemory(&OperateSuccess,sizeof(OperateSuccess));

	//构造数据
	OperateSuccess.lResultCode=lResultCode;
	lstrcpyn(OperateSuccess.szDescribeString,pszDescribe,CountArray(OperateSuccess.szDescribeString));

	//发送数据
	WORD wDescribe=CountStringBuffer(OperateSuccess.szDescribeString);
	WORD wHeadSize=sizeof(OperateSuccess)-sizeof(OperateSuccess.szDescribeString);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_OPERATE_SUCCESS,&OperateSuccess,wHeadSize+wDescribe);

	//关闭连接
	m_pITCPNetworkEngine->ShutDownSocket(dwContextID);

	return;
}

//操作失败
VOID CAttemperEngineSink::SendOperateFailure(DWORD dwContextID, LONG lResultCode, LPCTSTR pszDescribe)
{
	//效验参数
	ASSERT(pszDescribe != NULL);
	if(pszDescribe == NULL) return;

	//变量定义
	CMD_GP_OperateFailure OperateFailure;
	ZeroMemory(&OperateFailure,sizeof(OperateFailure));

	//构造数据
	OperateFailure.lResultCode=lResultCode;
	lstrcpyn(OperateFailure.szDescribeString,pszDescribe,CountArray(OperateFailure.szDescribeString));

	//发送数据
	WORD wDescribe=CountStringBuffer(OperateFailure.szDescribeString);
	WORD wHeadSize=sizeof(OperateFailure)-sizeof(OperateFailure.szDescribeString);
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_USER_SERVICE,SUB_GP_OPERATE_FAILURE,&OperateFailure,wHeadSize+wDescribe);

	//关闭连接
	m_pITCPNetworkEngine->ShutDownSocket(dwContextID);

	return;
}

//生成验证码
VOID CAttemperEngineSink::RandVerifyCode(LPTSTR pszVerifyCode, UINT nMaxCount)
{
	ASSERT(pszVerifyCode!=NULL && nMaxCount>=LEN_VERIFY_CODE);
	if (pszVerifyCode==NULL || nMaxCount<LEN_VERIFY_CODE) return;

	for (UINT i=0; i<nMaxCount-1; i++)  
	{  
		INT nflag=rand()%3;  
		switch (nflag)  
		{  
		case 0:  
			pszVerifyCode[i]=TEXT('A')+rand()%26;  
			break;  
		case 1:  
			pszVerifyCode[i]=TEXT('a')+rand()%26;  
			break;  
		case 2:  
			pszVerifyCode[i]=TEXT('0')+rand()%10;  
			break;  
		default:  
			pszVerifyCode[i]=TEXT('x');  
			break;  
		}  
	}  
	pszVerifyCode[nMaxCount-1]=TEXT('\0');  

	return;
}

//效验验证
bool CAttemperEngineSink::CheckVerifyCode(LPCTSTR pszValidateCode, LPCTSTR pszValidateCodeBak)
{
	//长度验证
	if (pszValidateCode==NULL || pszValidateCodeBak==NULL) return false;

	//长度对比
	INT nTargetLen=lstrlen(pszValidateCode);
	INT nSourceLen=lstrlen(pszValidateCodeBak);

	//密码对比
	if (nTargetLen!=nSourceLen) return false;
	if (CompareString(LOCALE_SYSTEM_DEFAULT,NORM_IGNORECASE,pszValidateCode,nTargetLen,pszValidateCodeBak,nSourceLen)!=CSTR_EQUAL) return false;

	return true;
}

//发送验证码
VOID CAttemperEngineSink::SendVerifyCode(DWORD dwSocketID, LPCTSTR pszVerifyCode, bool bCloseSocket)
{
	//效验参数
	ASSERT(pszVerifyCode != NULL);
	if(pszVerifyCode == NULL) return;

	//变量定义
	CMD_GP_QueryVerifyCode QueryVerifyCode;
	ZeroMemory(&QueryVerifyCode,sizeof(QueryVerifyCode));

	//构造数据
	QueryVerifyCode.bCloseSocket=bCloseSocket;
	lstrcpyn(QueryVerifyCode.szVerifyCode,pszVerifyCode,CountArray(QueryVerifyCode.szVerifyCode));

	//发送数据
	m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_LOGON,SUB_GP_QUERY_VERIFY_CODE,&QueryVerifyCode,sizeof(QueryVerifyCode));

	return;
}

//私人房间请求结束
bool CAttemperEngineSink::OnDBQueryPersonalRoomEnd(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID, MDM_MB_PERSONAL_SERVICE, SUB_MB_QUERY_PERSONAL_END, NULL, 0);

	return true;
}
//////////////////////////////////////////////////////////////////////////////////
