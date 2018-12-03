#include "StdAfx.h"
#include "TableFrameSink.h"
#include "DlgCustomRule.h"
#include <conio.h>
#include <locale>
#include <list>

//////////////////////////////////////////////////////////////////////////

//房间玩家信息
CMap<DWORD, DWORD, ROOMUSERINFO, ROOMUSERINFO> g_MapRoomUserInfo;	//玩家USERID映射玩家信息
//房间用户控制
CList<ROOMUSERCONTROL, ROOMUSERCONTROL&> g_ListRoomUserControl;		//房间用户控制链表
//操作控制记录
CList<CString, CString&> g_ListOperationRecord;						//操作控制记录

ROOMUSERINFO	g_CurrentQueryUserInfo;								//当前查询用户信息

//全局变量
LONGLONG						g_lRoomStorageStart = 0LL;								//房间起始库存
LONGLONG						g_lRoomStorageCurrent = 0LL;							//总输赢分
LONGLONG						g_lStorageDeductRoom = 0LL;								//回扣变量
LONGLONG						g_lStorageMax1Room = 0LL;								//库存封顶
LONGLONG						g_lStorageMul1Room = 0LL;								//系统输钱比例
LONGLONG						g_lStorageMax2Room = 0LL;								//库存封顶
LONGLONG						g_lStorageMul2Room = 0LL;								//系统输钱比例
//////////////////////////////////////////////////////////////////////////

#define	IDI_SO_OPERATE							2							//代打定时器
#define	TIME_SO_OPERATE							40000						//代打定时器

//////////////////////////////////////////////////////////////////////////

//构造函数
CTableFrameSink::CTableFrameSink()
{
	//游戏变量	
	m_wPlayerCount=GAME_PLAYER;	

	m_lExitScore=0;	

	m_wBankerUser=INVALID_CHAIR;
	m_wFirstEnterUser = INVALID_CHAIR;

	//用户状态
	ZeroMemory(m_cbDynamicJoin,sizeof(m_cbDynamicJoin));
	ZeroMemory(m_lTableScore,sizeof(m_lTableScore));
	ZeroMemory(m_cbPlayStatus,sizeof(m_cbPlayStatus));
	ZeroMemory(m_cbCallBankerStatus,sizeof(m_cbCallBankerStatus));
	ZeroMemory(m_cbCallBankerTimes,sizeof(m_cbCallBankerTimes));
	
	ZeroMemory(m_bOpenCard,sizeof(m_bOpenCard));

	//扑克变量
	ZeroMemory(m_cbHandCardData,sizeof(m_cbHandCardData));
	ZeroMemory(m_bSpecialCard,sizeof(m_bSpecialCard));
	ZeroMemory(m_cbOriginalCardType,sizeof(m_cbOriginalCardType));
	ZeroMemory(m_cbCombineCardType,sizeof(m_cbCombineCardType));

	//下注信息
	ZeroMemory(m_lTurnMaxScore,sizeof(m_lTurnMaxScore));

	ZeroMemory(m_bBuckleServiceCharge,sizeof(m_bBuckleServiceCharge));

	//组件变量
	m_pITableFrame=NULL;
	m_pGameServiceOption=NULL;

	m_ctConfig = CT_CLASSIC_;
	m_stConfig = ST_SENDFOUR_;
	m_gtConfig = GT_HAVEKING_;
	m_bgtConfig = BGT_DESPOT_;
	m_btConfig = BT_FREE_;

	m_lFreeConfig[0] = 200;
	m_lFreeConfig[1] = 500;
	m_lFreeConfig[2] = 800;
	m_lFreeConfig[3] = 1100;
	m_lFreeConfig[4] = 1500;

	ZeroMemory(m_lPercentConfig, sizeof(m_lPercentConfig));
	
	m_lMaxCardTimes	= 0;
	
	ZeroMemory(&m_stRecord, sizeof(m_stRecord));

	time(NULL);
	
#ifdef CARD_CONFIG
	ZeroMemory(m_cbconfigCard, sizeof(m_cbconfigCard));
#endif

	//清空链表
	g_ListRoomUserControl.RemoveAll();
	g_ListOperationRecord.RemoveAll();
	ZeroMemory(&g_CurrentQueryUserInfo, sizeof(g_CurrentQueryUserInfo));

	//服务控制
	m_hInst = NULL;
	m_pServerControl = NULL;
	m_hInst = LoadLibrary(TEXT("OxSixXServerControl.dll"));
	if ( m_hInst )
	{
		typedef void * (*CREATE)(); 
		CREATE ServerControl = (CREATE)GetProcAddress(m_hInst,"CreateServerControl"); 
		if ( ServerControl )
		{
			m_pServerControl = static_cast<IServerControl*>(ServerControl());
		}
	}

	//游戏视频
	m_hVideoInst = NULL;
	m_pGameVideo = NULL;
	m_hVideoInst = LoadLibrary(TEXT("OxSixXGameVideo.dll"));
	if (m_hVideoInst)
	{
		typedef void * (*CREATE)();
		CREATE GameVideo = (CREATE)GetProcAddress(m_hVideoInst, "CreateGameVideo");
		if (GameVideo)
		{
			m_pGameVideo = static_cast<IGameVideo*>(GameVideo());
		}
	}

	ZeroMemory(&m_RoomCardRecord, sizeof(m_RoomCardRecord));

	return;
}

//析构函数
CTableFrameSink::~CTableFrameSink(void)
{
	if( m_pServerControl )
	{
		delete m_pServerControl;
		m_pServerControl = NULL;
	}

	if( m_hInst )
	{
		FreeLibrary(m_hInst);
		m_hInst = NULL;
	}

	if (m_pGameVideo)
	{
		delete m_pGameVideo;
		m_pGameVideo = NULL;
	}

	if (m_hVideoInst)
	{
		FreeLibrary(m_hVideoInst);
		m_hVideoInst = NULL;
	}
}

//接口查询--检测相关信息版本
void * CTableFrameSink::QueryInterface(const IID & Guid, DWORD dwQueryVer)
{
	QUERYINTERFACE(ITableFrameSink,Guid,dwQueryVer);
	QUERYINTERFACE(ITableUserAction,Guid,dwQueryVer);
	QUERYINTERFACE_IUNKNOWNEX(ITableFrameSink,Guid,dwQueryVer);
	return NULL;
}

//初始化
bool CTableFrameSink::Initialization(IUnknownEx * pIUnknownEx)
{
	//查询接口
	ASSERT(pIUnknownEx!=NULL);
	m_pITableFrame=QUERY_OBJECT_PTR_INTERFACE(pIUnknownEx,ITableFrame);
	if (m_pITableFrame==NULL) return false;

	m_pITableFrame->SetStartMode(START_MODE_ALL_READY);

	//游戏配置
	m_pGameServiceAttrib=m_pITableFrame->GetGameServiceAttrib();
	m_pGameServiceOption=m_pITableFrame->GetGameServiceOption();
	
	//读取配置
	ReadConfigInformation();
	
	BYTE cbHandCardDataA[MAX_CARDCOUNT];
	cbHandCardDataA[0] = 0x14;
	cbHandCardDataA[1] = 0x28;
	cbHandCardDataA[2] = 0x4E;
	cbHandCardDataA[3] = 0x07;
	cbHandCardDataA[4] = 0x15;

	BYTE cbHandCardDataB[MAX_CARDCOUNT];
	cbHandCardDataB[0] = 0x05;
	cbHandCardDataB[1] = 0x07;
	cbHandCardDataB[2] = 0x26;
	cbHandCardDataB[3] = 0x31;
	cbHandCardDataB[4] = 0x1C;
	
	BYTE cbType = m_GameLogic.GetCardType(cbHandCardDataA, MAX_CARDCOUNT);
		
	return true;
}

//复位桌子
void CTableFrameSink::RepositionSink()
{
	//游戏变量
	m_lExitScore=0;	

	//用户状态
	ZeroMemory(m_cbDynamicJoin,sizeof(m_cbDynamicJoin));
	ZeroMemory(m_lTableScore,sizeof(m_lTableScore));
	ZeroMemory(m_bBuckleServiceCharge,sizeof(m_bBuckleServiceCharge));
	ZeroMemory(m_cbPlayStatus,sizeof(m_cbPlayStatus));
	ZeroMemory(m_cbCallBankerStatus,sizeof(m_cbCallBankerStatus));
	ZeroMemory(m_cbCallBankerTimes,sizeof(m_cbCallBankerTimes));

	ZeroMemory(m_bOpenCard,sizeof(m_bOpenCard));

	//扑克变量
	ZeroMemory(m_cbHandCardData,sizeof(m_cbHandCardData));
	ZeroMemory(m_bSpecialCard,sizeof(m_bSpecialCard));
	ZeroMemory(m_cbOriginalCardType,sizeof(m_cbOriginalCardType));
	ZeroMemory(m_cbCombineCardType,sizeof(m_cbCombineCardType));

	//下注信息
	ZeroMemory(m_lTurnMaxScore,sizeof(m_lTurnMaxScore));

	return;
}

//用户断线
bool CTableFrameSink::OnActionUserOffLine(WORD wChairID, IServerUserItem * pIServerUserItem)
{
	//更新房间用户信息
	UpdateRoomUserInfo(pIServerUserItem, USER_OFFLINE);

	return true;
}

//用户坐下
bool CTableFrameSink::OnActionUserSitDown(WORD wChairID, IServerUserItem * pIServerUserItem, bool bLookonUser)
{
	//历史积分
	if (bLookonUser==false) m_HistoryScore.OnEventUserEnter(pIServerUserItem->GetChairID());

	if(m_pITableFrame->GetGameStatus()!=GS_TK_FREE)
		m_cbDynamicJoin[pIServerUserItem->GetChairID()]=TRUE;
	
	//更新房间用户信息
	UpdateRoomUserInfo(pIServerUserItem, USER_SITDOWN);

	//更新同桌用户控制
	UpdateUserControl(pIServerUserItem);

	//霸王庄 首进玩家
	if (m_wFirstEnterUser == INVALID_CHAIR && m_bgtConfig == BGT_DESPOT_)
	{
		m_wFirstEnterUser = wChairID;
	}

	return true;
}

//用户起立
bool CTableFrameSink::OnActionUserStandUp(WORD wChairID, IServerUserItem * pIServerUserItem, bool bLookonUser)
{
	//历史积分
	if (bLookonUser==false) 
	{
		m_HistoryScore.OnEventUserLeave(pIServerUserItem->GetChairID());
		m_cbDynamicJoin[pIServerUserItem->GetChairID()]=FALSE;
	}
	
	//更新房间用户信息
	UpdateRoomUserInfo(pIServerUserItem, USER_STANDUP);

	//非房卡房间
	if ((m_pGameServiceOption->wServerType & GAME_GENRE_PERSONAL) == 0)
	{
		//霸王庄 首进玩家离开，逆时针下家为首进玩家
		if (m_wFirstEnterUser == wChairID && m_bgtConfig == BGT_DESPOT_)
		{	
			while (true)
			{
				m_wFirstEnterUser = (m_wFirstEnterUser + 1) % m_wPlayerCount;

				if (m_wFirstEnterUser == wChairID)
				{
					m_wFirstEnterUser = INVALID_CHAIR;
					break;
				}

				if (m_pITableFrame->GetTableUserItem(m_wFirstEnterUser) != NULL)
				{
					break;
				}
			}
		}
		else if (m_bgtConfig == BGT_NIUNIU_ || m_bgtConfig == BGT_NONIUNIU_)
		{
			if (wChairID == m_wBankerUser)
			{
				m_wBankerUser = INVALID_CHAIR;
			}
		}
	}

	//房卡模式
	if ((m_pGameServiceOption->wServerType & GAME_GENRE_PERSONAL) != 0)
	{
		//当前约战解散清理记录
		if (m_pITableFrame->IsPersonalRoomDisumme())
		{
			ZeroMemory(&m_stRecord, sizeof(m_stRecord));
			ZeroMemory(&m_RoomCardRecord, sizeof(m_RoomCardRecord));

			m_wFirstEnterUser = INVALID_CHAIR;
			m_wBankerUser = INVALID_CHAIR;
		}
	}

	return true;
}

//用户同意
bool CTableFrameSink::OnActionUserOnReady(WORD wChairID,IServerUserItem * pIServerUserItem, VOID * pData, WORD wDataSize)
{
	//私人房设置游戏模式
	if (((m_pGameServiceOption->wServerType) & GAME_GENRE_PERSONAL) != 0)
	{
		//cbGameRule[1] 为  2 、3 、4, 5, 6 0分别对应 2人 、 3人 、 4人 、5, 6 2-6人 这几种配置
		BYTE *pGameRule = m_pITableFrame->GetGameRule();
		switch(pGameRule[1])
		{
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			{
				if (m_pITableFrame->GetStartMode() != START_MODE_FULL_READY)
				{
					m_pITableFrame->SetStartMode(START_MODE_FULL_READY);
				}

				break;
			}
		case 0:
			{
				if (m_pITableFrame->GetStartMode() != START_MODE_ALL_READY)
				{
					m_pITableFrame->SetStartMode(START_MODE_ALL_READY);
				}
				break;
			}
		default:
			ASSERT(false);

		}
	}

	return true;
}

//游戏开始
bool CTableFrameSink::OnEventGameStart()
{
	//库存
	if(g_lRoomStorageCurrent>0 && NeedDeductStorage())
	{
		g_lRoomStorageCurrent=g_lRoomStorageCurrent-g_lRoomStorageCurrent*g_lStorageDeductRoom/1000;
	}

	CString strInfo;
	strInfo.Format(TEXT("当前库存：%I64d"), g_lRoomStorageCurrent);
	WriteInfo(strInfo);
	
	if (m_pGameVideo)
	{
		m_pGameVideo->StartVideo(m_pITableFrame, m_wPlayerCount);
	}

	//牌型最大倍数
	m_lMaxCardTimes = (m_ctConfig == CT_CLASSIC_ ? 4 : 8);

	//用户状态
	for (WORD i=0;i<m_wPlayerCount;i++)
	{
		//获取用户
		IServerUserItem *pIServerUserItem=m_pITableFrame->GetTableUserItem(i);
		if(pIServerUserItem==NULL)
		{
			m_cbPlayStatus[i]=FALSE;
		}
		else
		{
			m_cbPlayStatus[i]=TRUE;

			//更新房间用户信息
			UpdateRoomUserInfo(pIServerUserItem, USER_SITDOWN);
		}
	}

	CopyMemory(m_RoomCardRecord.cbPlayStatus, m_cbPlayStatus, sizeof(m_RoomCardRecord.cbPlayStatus));

	//随机扑克
	BYTE bTempArray[GAME_PLAYER*MAX_CARDCOUNT];
	m_GameLogic.RandCardList(bTempArray,sizeof(bTempArray), (m_gtConfig == GT_HAVEKING_ ? true : false));

	for (WORD i=0;i<m_wPlayerCount;i++)
	{
		IServerUserItem *pIServerUser=m_pITableFrame->GetTableUserItem(i);	
		if(pIServerUser==NULL)
		{
			continue;
		}

		//派发扑克
		CopyMemory(m_cbHandCardData[i],&bTempArray[i*MAX_CARDCOUNT],MAX_CARDCOUNT);
	}

#ifdef CARD_CONFIG
	if (m_cbconfigCard[0][0] != 0 && m_cbconfigCard[0][1] != 0 && m_cbconfigCard[0][2] != 0)
	{
		CopyMemory(m_cbHandCardData, m_cbconfigCard, sizeof(m_cbHandCardData));
	}

	//m_cbHandCardData[0][0] = 0x1A;
	//m_cbHandCardData[0][1] = 0x36;
	//m_cbHandCardData[0][2] = 0x21;
	//m_cbHandCardData[0][3] = 0x2C;
	//m_cbHandCardData[0][4] = 0x15;

	//m_cbHandCardData[1][0] = 0x39;
	//m_cbHandCardData[1][1] = 0x03;
	//m_cbHandCardData[1][2] = 0x0A;
	//m_cbHandCardData[1][3] = 0x08;
	//m_cbHandCardData[1][4] = 0x25;
#endif
	
	//临时扑克
	BYTE cbTempHandCardData[GAME_PLAYER][MAX_CARDCOUNT];
	ZeroMemory(cbTempHandCardData, sizeof(cbTempHandCardData));
	CopyMemory(cbTempHandCardData, m_cbHandCardData, sizeof(m_cbHandCardData));
	
	for (WORD i=0;i<m_wPlayerCount;i++)
	{
		IServerUserItem *pIServerUser=m_pITableFrame->GetTableUserItem(i);	
		if(pIServerUser==NULL)
		{
			continue;
		}
		
		m_bSpecialCard[i] = (m_GameLogic.GetCardType(cbTempHandCardData[i], MAX_CARDCOUNT, m_ctConfig) > CT_CLASSIC_OX_VALUENIUNIU ? true : false);
		
		//特殊牌型
		if (m_bSpecialCard[i])
		{
			m_cbOriginalCardType[i] = m_GameLogic.GetCardType(cbTempHandCardData[i], MAX_CARDCOUNT, m_ctConfig);
		}
		else
		{
			//获取牛牛牌型
			m_GameLogic.GetOxCard(cbTempHandCardData[i], MAX_CARDCOUNT);
			
			m_cbOriginalCardType[i] = m_GameLogic.GetCardType(cbTempHandCardData[i], MAX_CARDCOUNT, m_ctConfig);
		}
	}
	
	//获取坐庄模式
	ASSERT (m_bgtConfig != BGT_INVALID_);
	
	//庄家配置
	switch(m_bgtConfig)
	{
		//霸王庄
	case BGT_DESPOT_:
		{
			ASSERT (m_wFirstEnterUser !=INVALID_CHAIR);

			//房主强制为庄，若房主不参与游戏，则第一个进去此游戏的玩家强制为庄
			//获取房主
			WORD wRoomOwenrChairID = INVALID_CHAIR;
			DWORD dwRoomOwenrUserID = INVALID_DWORD;
			for (WORD i=0; i<m_wPlayerCount; i++)
			{
				//获取用户
				IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if (!pIServerUserItem)
				{
					continue;
				}

				m_cbCallBankerStatus[i] = TRUE;
				
				if (pIServerUserItem->GetUserID() == m_pITableFrame->GetTableOwner() && IsRoomCardType())
				{
					dwRoomOwenrUserID = pIServerUserItem->GetUserID();
					wRoomOwenrChairID = pIServerUserItem->GetChairID();
					break;
				}
			}
			
			//房主参与游戏
			if (dwRoomOwenrUserID != INVALID_DWORD && wRoomOwenrChairID != INVALID_CHAIR)
			{
				m_wBankerUser = wRoomOwenrChairID;
			}
			//房主不参与游戏和非房卡房间
			else      
			{
				m_wBankerUser = m_wFirstEnterUser;
			}

			m_bBuckleServiceCharge[m_wBankerUser]=true;

			//设置状态
			m_pITableFrame->SetGameStatus(GS_TK_SCORE);
			
			//更新房间用户信息
			for (WORD i=0; i<m_wPlayerCount; i++)
			{
				//获取用户
				IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if (pIServerUserItem != NULL)
				{
					UpdateRoomUserInfo(pIServerUserItem, USER_SITDOWN);
				}
			}

			//获取最大下注
			for (WORD i=0; i<m_wPlayerCount; i++)
			{
				if (m_cbPlayStatus[i] != TRUE || i == m_wBankerUser)
				{
					continue;
				}

				//下注变量
				m_lTurnMaxScore[i] = GetUserMaxTurnScore(i);
			}

			//设置变量
			CMD_S_GameStart GameStart;
			ZeroMemory(&GameStart, sizeof(GameStart));
			GameStart.wBankerUser=m_wBankerUser;
			CopyMemory(GameStart.cbPlayerStatus, m_cbPlayStatus, sizeof(m_cbPlayStatus));
			
			//发四等五
			if (m_stConfig == ST_SENDFOUR_)
			{
				for (WORD i=0;i<m_wPlayerCount;i++)
				{
					if (m_cbPlayStatus[i]==FALSE && m_cbDynamicJoin[i]==FALSE)
					{
						continue;
					}

					//派发扑克(开始只发四张牌)
					CopyMemory(GameStart.cbCardData[i], m_cbHandCardData[i], sizeof(BYTE) * 4);
				}
			}

			GameStart.stConfig = m_stConfig;
			GameStart.bgtConfig = m_bgtConfig;
			GameStart.btConfig = m_btConfig;
			CopyMemory(GameStart.lFreeConfig, m_lFreeConfig, sizeof(GameStart.lFreeConfig));
			CopyMemory(GameStart.lPercentConfig, m_lPercentConfig, sizeof(GameStart.lPercentConfig));
			
			bool bFirstRecord = true;

			WORD wRealPlayerCount = 0;
			for (WORD i = 0; i < m_wPlayerCount; i++)
			{
				IServerUserItem *pServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if (m_cbPlayStatus[i] == FALSE && m_cbDynamicJoin[i] == FALSE)
				{
					continue;
				}

				if (!pServerUserItem)
				{
					continue;
				}

				wRealPlayerCount++;
			}

			BYTE *pGameRule = m_pITableFrame->GetGameRule();

			//最大下注
			for (WORD i=0;i<m_wPlayerCount;i++)
			{
				IServerUserItem *pServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if (m_cbPlayStatus[i]==FALSE && m_cbDynamicJoin[i]==FALSE)
				{
					continue;
				}
				GameStart.lTurnMaxScore=m_lTurnMaxScore[i];
				m_pITableFrame->SendTableData(i,SUB_S_GAME_START,&GameStart,sizeof(GameStart));

				if (m_pGameVideo)
				{
					Video_GameStart video;
					ZeroMemory(&video, sizeof(video));

					video.wPlayerCount = wRealPlayerCount;
					video.wGamePlayerCountRule = pGameRule[1];
					video.wBankerUser = GameStart.wBankerUser;
					CopyMemory(video.cbPlayerStatus, GameStart.cbPlayerStatus, sizeof(video.cbPlayerStatus));
					video.lTurnMaxScore = GameStart.lTurnMaxScore;
					CopyMemory(video.cbCardData, GameStart.cbCardData, sizeof(video.cbCardData));
					video.ctConfig = m_ctConfig;
					video.stConfig = GameStart.stConfig;
					video.bgtConfig = GameStart.bgtConfig;
					video.btConfig = GameStart.btConfig;
					CopyMemory(video.lFreeConfig, GameStart.lFreeConfig, sizeof(video.lFreeConfig));
					CopyMemory(video.lPercentConfig, GameStart.lPercentConfig, sizeof(video.lPercentConfig));
					CopyMemory(video.szNickName, pServerUserItem->GetNickName(), sizeof(video.szNickName));
					video.wChairID = i;
					video.lScore = pServerUserItem->GetUserScore();

					m_pGameVideo->AddVideoData(SUB_S_GAME_START, &video, bFirstRecord);

					if (bFirstRecord == true)
					{
						bFirstRecord = false;
					}

					CString str;
					str.Format(TEXT("写入回放开始消息，大小 %d\n"), sizeof(video));
					WriteInfo(str);
				}
			}
			m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GAME_START,&GameStart,sizeof(GameStart));

			break;
		}
		//倍数抢庄
	case BGT_ROB_:
		{
			//设置状态
			m_pITableFrame->SetGameStatus(GS_TK_CALL);

			CMD_S_CallBanker CallBanker;
			ZeroMemory(&CallBanker, sizeof(CallBanker));
			CallBanker.ctConfig = m_ctConfig;
			CallBanker.stConfig = m_stConfig;
			CallBanker.bgtConfig = m_bgtConfig;

			BYTE *pGameRule = m_pITableFrame->GetGameRule();
			CallBanker.wGamePlayerCountRule = pGameRule[1];

			//更新房间用户信息
			for (WORD i=0; i<m_wPlayerCount; i++)
			{
				//获取用户
				IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if (pIServerUserItem != NULL)
				{
					UpdateRoomUserInfo(pIServerUserItem, USER_SITDOWN);
				}
			}

			//发送数据
			for (WORD i=0;i<m_wPlayerCount;i++)
			{
				if(m_cbPlayStatus[i]!=TRUE) 
				{
					continue;
				}
				m_pITableFrame->SendTableData(i, SUB_S_CALL_BANKER, &CallBanker, sizeof(CallBanker));
			}
			m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_CALL_BANKER, &CallBanker, sizeof(CallBanker));

			if (m_pGameVideo)
			{
				m_pGameVideo->AddVideoData(SUB_S_CALL_BANKER, &CallBanker);
			}

			break;
		}
		//牛牛上庄
		//无牛下庄
	case BGT_NIUNIU_:
	case BGT_NONIUNIU_:
		{
			if (m_wBankerUser == INVALID_CHAIR)
			{
				//随机坐庄
				BYTE cbRandomArray[GAME_PLAYER];
				memset(cbRandomArray, INVALID_BYTE, sizeof(cbRandomArray));
				BYTE cbRandomIndex = 0;
				for (WORD i=0; i<m_wPlayerCount; i++)
				{
					//获取用户
					IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
					if (!pIServerUserItem)
					{
						continue;
					}

					m_cbCallBankerStatus[i] = TRUE;
					cbRandomArray[cbRandomIndex++] = i;
				}

				m_wBankerUser = cbRandomArray[rand() % cbRandomIndex];
			}

			m_bBuckleServiceCharge[m_wBankerUser]=true;

			//设置状态
			m_pITableFrame->SetGameStatus(GS_TK_SCORE);

			//更新房间用户信息
			for (WORD i=0; i<m_wPlayerCount; i++)
			{
				//获取用户
				IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if (pIServerUserItem != NULL)
				{
					UpdateRoomUserInfo(pIServerUserItem, USER_SITDOWN);
				}
			}

			//获取最大下注
			for (WORD i=0; i<m_wPlayerCount; i++)
			{
				if (m_cbPlayStatus[i] != TRUE || i == m_wBankerUser)
				{
					continue;
				}

				//下注变量
				m_lTurnMaxScore[i] = GetUserMaxTurnScore(i);
			}

			//设置变量
			CMD_S_GameStart GameStart;
			ZeroMemory(&GameStart, sizeof(GameStart));
			GameStart.wBankerUser=m_wBankerUser;
			CopyMemory(GameStart.cbPlayerStatus, m_cbPlayStatus, sizeof(m_cbPlayStatus));

			//发四等五
			if (m_stConfig == ST_SENDFOUR_)
			{
				for (WORD i=0;i<m_wPlayerCount;i++)
				{
					if (m_cbPlayStatus[i]==FALSE && m_cbDynamicJoin[i]==FALSE)
					{
						continue;
					}

					//派发扑克(开始只发四张牌)
					CopyMemory(GameStart.cbCardData[i], m_cbHandCardData[i], sizeof(BYTE) * 4);
				}
			}

			GameStart.stConfig = m_stConfig;
			GameStart.bgtConfig = m_bgtConfig;
			GameStart.btConfig = m_btConfig;
			CopyMemory(GameStart.lFreeConfig, m_lFreeConfig, sizeof(GameStart.lFreeConfig));
			CopyMemory(GameStart.lPercentConfig, m_lPercentConfig, sizeof(GameStart.lPercentConfig));

			bool bFirstRecord = true;

			WORD wRealPlayerCount = 0;
			for (WORD i = 0; i < m_wPlayerCount; i++)
			{
				IServerUserItem *pServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if (m_cbPlayStatus[i] == FALSE && m_cbDynamicJoin[i] == FALSE)
				{
					continue;
				}

				if (!pServerUserItem)
				{
					continue;
				}

				wRealPlayerCount++;
			}

			BYTE *pGameRule = m_pITableFrame->GetGameRule();

			//最大下注
			for (WORD i=0;i<m_wPlayerCount;i++)
			{
				IServerUserItem *pServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if (m_cbPlayStatus[i]==FALSE && m_cbDynamicJoin[i]==FALSE)
				{
					continue;
				}
				GameStart.lTurnMaxScore=m_lTurnMaxScore[i];
				m_pITableFrame->SendTableData(i,SUB_S_GAME_START,&GameStart,sizeof(GameStart));

				if (m_pGameVideo)
				{
					Video_GameStart video;
					ZeroMemory(&video, sizeof(video));
					video.wPlayerCount = wRealPlayerCount;
					video.wGamePlayerCountRule = pGameRule[1];
					video.wBankerUser = GameStart.wBankerUser;
					CopyMemory(video.cbPlayerStatus, GameStart.cbPlayerStatus, sizeof(video.cbPlayerStatus));
					video.lTurnMaxScore = GameStart.lTurnMaxScore;
					CopyMemory(video.cbCardData, GameStart.cbCardData, sizeof(video.cbCardData));
					video.ctConfig = m_ctConfig;
					video.stConfig = GameStart.stConfig;
					video.bgtConfig = GameStart.bgtConfig;
					video.btConfig = GameStart.btConfig;
					CopyMemory(video.lFreeConfig, GameStart.lFreeConfig, sizeof(video.lFreeConfig));
					CopyMemory(video.lPercentConfig, GameStart.lPercentConfig, sizeof(video.lPercentConfig));
					CopyMemory(video.szNickName, pServerUserItem->GetNickName(), sizeof(video.szNickName));
					video.wChairID = i;
					video.lScore = pServerUserItem->GetUserScore();

					m_pGameVideo->AddVideoData(SUB_S_GAME_START, &video, bFirstRecord);

					if (bFirstRecord == true)
					{
						bFirstRecord = false;
					}
				}
			}
			m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GAME_START,&GameStart,sizeof(GameStart));

			break;
		}
	default:
		break;
	}

	//用户状态
	// for (WORD i = 0; i<m_wPlayerCount; i++)
	// {
	// 	//获取用户
	// 	IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
	// 	if (pIServerUserItem != NULL)
	// 	{
	// 		CString strOperationRecord;
	// 		strOperationRecord.Format(TEXT("用户昵称[%s], USERID = %d, userstatus = %d，游戏正式开始"),
	// 			pIServerUserItem->GetNickName(), pIServerUserItem->GetUserID(), pIServerUserItem->GetUserStatus());
	// 		WriteInfo(strOperationRecord);
	// 	}
	// }

	return true;
}

//游戏结束
bool CTableFrameSink::OnEventGameConclude(WORD wChairID, IServerUserItem * pIServerUserItem, BYTE cbReason)
{
	switch (cbReason)
	{
	case GER_DISMISS:		//游戏解散
		{
			//构造数据
			CMD_S_GameEnd GameEnd = {0};

			//发送信息
			m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));
			m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));

			//结束游戏
			m_pITableFrame->ConcludeGame(GS_TK_FREE);

			if(!IsRoomCardType())
			{
				//删除时间
				m_pITableFrame->KillGameTimer(IDI_SO_OPERATE);
			}

			m_wBankerUser = INVALID_CHAIR;
			m_wFirstEnterUser = INVALID_CHAIR;

			//房卡模式
			if ((m_pGameServiceOption->wServerType & GAME_GENRE_PERSONAL) !=0 )
			{
				//解散清理记录
				if (m_pITableFrame->IsPersonalRoomDisumme())
				{
					ZeroMemory(&m_stRecord, sizeof(m_stRecord));
				}
			}

			//更新房间用户信息
			for (WORD i=0; i<m_wPlayerCount; i++)
			{
				//获取用户
				IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);

				if (!pIServerUserItem)
				{
					continue;
				}

				UpdateRoomUserInfo(pIServerUserItem, USER_STANDUP);
			}

			return true;
		}
	case GER_NORMAL:		//常规结束
		{
			//定义变量
			CMD_S_GameEnd GameEnd;
			ZeroMemory(&GameEnd,sizeof(GameEnd));
			WORD wWinTimes[GAME_PLAYER];
			WORD wWinCount[GAME_PLAYER];
			ZeroMemory(wWinCount,sizeof(wWinCount));
			ZeroMemory(wWinTimes,sizeof(wWinTimes));
			
			//倍数抢庄 结算需要乘以cbMaxCallBankerTimes
			BYTE cbMaxCallBankerTimes = 1;
			if (m_bgtConfig == BGT_ROB_)
			{
				for (WORD i=0;i<m_wPlayerCount;i++)
				{
					if(m_cbPlayStatus[i]==TRUE && m_cbCallBankerStatus[i]==TRUE && m_cbCallBankerTimes[i] > cbMaxCallBankerTimes) 
					{
						cbMaxCallBankerTimes = m_cbCallBankerTimes[i];
					}
				}
			}

			//保存扑克
			BYTE cbUserCardData[GAME_PLAYER][MAX_CARDCOUNT];
			CopyMemory(cbUserCardData,m_cbHandCardData,sizeof(cbUserCardData));
		
			//庄家倍数
			wWinTimes[m_wBankerUser]=m_GameLogic.GetTimes(cbUserCardData[m_wBankerUser],MAX_CARDCOUNT, m_ctConfig, m_cbCombineCardType[m_wBankerUser]);

			//对比玩家
			for (WORD i=0;i<m_wPlayerCount;i++)
			{
				if(i==m_wBankerUser || m_cbPlayStatus[i]==FALSE)continue;

				//对比扑克
				if (m_GameLogic.CompareCard(cbUserCardData[i],cbUserCardData[m_wBankerUser],MAX_CARDCOUNT, m_ctConfig, m_cbCombineCardType[i], m_cbCombineCardType[m_wBankerUser])) 
				{
					wWinCount[i]++;
					//获取倍数
					wWinTimes[i]=m_GameLogic.GetTimes(cbUserCardData[i],MAX_CARDCOUNT, m_ctConfig, m_cbCombineCardType[i]);
				}
				else
				{
					wWinCount[m_wBankerUser]++;
				}
			}

			//统计得分
			for (WORD i=0;i<m_wPlayerCount;i++)
			{
				if(i==m_wBankerUser || m_cbPlayStatus[i]==FALSE)continue;

				if(wWinCount[i]>0)	//闲家胜利
				{
					GameEnd.lGameScore[i]=m_lTableScore[i]*wWinTimes[i] * m_pITableFrame->GetCellScore() * cbMaxCallBankerTimes;
					GameEnd.lGameScore[m_wBankerUser]-=GameEnd.lGameScore[i];
					m_lTableScore[i]=0;
				}
				else					//庄家胜利
				{
					GameEnd.lGameScore[i]=(-1)*m_lTableScore[i]*wWinTimes[m_wBankerUser] * m_pITableFrame->GetCellScore() * cbMaxCallBankerTimes;
					GameEnd.lGameScore[m_wBankerUser]+=(-1)*GameEnd.lGameScore[i];
					m_lTableScore[i]=0;
				}
			}

			//闲家强退分数	
			GameEnd.lGameScore[m_wBankerUser]+=m_lExitScore;

			//离开用户
			for (WORD i=0;i<m_wPlayerCount;i++)
			{
				if(m_lTableScore[i]>0)GameEnd.lGameScore[i]=-m_lTableScore[i];
			}

			//修改积分
			tagScoreInfo ScoreInfoArray[GAME_PLAYER];
			ZeroMemory(ScoreInfoArray,sizeof(ScoreInfoArray));

			//积分税收
			for(WORD i=0;i<m_wPlayerCount;i++)
			{
				if(m_cbPlayStatus[i]==FALSE)continue;

				if(GameEnd.lGameScore[i]>0L)
				{
					GameEnd.lGameTax[i] = m_pITableFrame->CalculateRevenue(i,GameEnd.lGameScore[i]);
					if(GameEnd.lGameTax[i]>0)
						GameEnd.lGameScore[i] -= GameEnd.lGameTax[i];
				}

				//历史积分
				m_HistoryScore.OnEventUserScore(i,GameEnd.lGameScore[i]);

				//保存积分
				ScoreInfoArray[i].lScore = GameEnd.lGameScore[i];
				ScoreInfoArray[i].lRevenue = GameEnd.lGameTax[i];
				ScoreInfoArray[i].cbType = (GameEnd.lGameScore[i]>0L)?SCORE_TYPE_WIN:SCORE_TYPE_LOSE;

				//约战记录
				if (m_stRecord.nCount < MAX_RECORD_COUNT)
				{
					if (GameEnd.lGameScore[i] > 0)
					{
						m_stRecord.lUserWinCount[i]++;
					}
					else
					{
						m_stRecord.lUserLostCount[i]++;
					}
				}

				//房卡模式
				if (((m_pGameServiceOption->wServerType & GAME_GENRE_PERSONAL) != 0) && (m_RoomCardRecord.nCount < MAX_RECORD_COUNT))
				{
					m_RoomCardRecord.lDetailScore[i][m_RoomCardRecord.nCount] = ScoreInfoArray[i].lScore;
				}
			}
			
			m_stRecord.nCount++;
			
			//房卡模式
			if ((m_pGameServiceOption->wServerType & GAME_GENRE_PERSONAL) != 0)
			{
				m_RoomCardRecord.nCount++;
			}

			//房卡模式
			if ((m_pGameServiceOption->wServerType & GAME_GENRE_PERSONAL) != 0 )
			{
				m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_RECORD, &m_stRecord, sizeof(m_stRecord));

				CMD_S_RoomCardRecord RoomCardRecord;
				ZeroMemory(&RoomCardRecord, sizeof(RoomCardRecord));

				CopyMemory(&RoomCardRecord, &m_RoomCardRecord, sizeof(m_RoomCardRecord));

				m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_ROOMCARD_RECORD, &RoomCardRecord, sizeof(RoomCardRecord));
				m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_ROOMCARD_RECORD, &RoomCardRecord, sizeof(RoomCardRecord));
			}
			
			CopyMemory(GameEnd.cbHandCardData, m_cbHandCardData, sizeof(m_cbHandCardData));

			//获取玩家牌型			
			for(WORD i=0;i<m_wPlayerCount;i++)
			{
				if(m_cbPlayStatus[i]==FALSE)
				{
					continue;
				}
				
				GameEnd.cbCardType[i] = m_cbCombineCardType[i];
			}

			//发送信息
			CMD_GC_GTrumpet sTrumpet;
			for (WORD i=0;i<m_wPlayerCount;i++)
			{
				if(m_cbPlayStatus[i]==FALSE&&m_cbDynamicJoin[i]==FALSE) continue;
				m_pITableFrame->SendTableData(i,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));

				// 检测发送喇叭
				IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if(!pIServerUserItem) {
					continue;
				}
				if (GameEnd.lGameScore[i] > 0) {
					ZeroMemory(&sTrumpet, sizeof(sTrumpet));
					sTrumpet.dwCardType = GameEnd.cbCardType[i];
					sTrumpet.lScore = GameEnd.lGameScore[i];
					m_pITableFrame->SendUserItemData(pIServerUserItem, SUB_S_WIN_TRUMPET, &sTrumpet, sizeof(sTrumpet));
				}
			}

			m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));
			
			if (m_pGameVideo)
			{
				m_pGameVideo->AddVideoData(SUB_S_GAME_END, &GameEnd);
			}

			if (m_pGameVideo)
			{
				m_pGameVideo->StopAndSaveVideo(m_pGameServiceOption->wServerID, m_pITableFrame->GetTableID());
			}

			TryWriteTableScore(ScoreInfoArray);

			//库存统计
			for (WORD i=0;i<m_wPlayerCount;i++)
			{
				//获取用户
				IServerUserItem * pIServerUserIte=m_pITableFrame->GetTableUserItem(i);
				if (pIServerUserIte==NULL) continue;

				//库存累计
				if(!pIServerUserIte->IsAndroidUser())
					g_lRoomStorageCurrent-=GameEnd.lGameScore[i];

			}
	
			//牛牛当庄 场上有牛牛以及牛牛以上的牌型，由最大的牌型玩家当庄, 如果没有还是原来的当庄
			if (m_bgtConfig == BGT_NIUNIU_) 
			{
				//牌型最大玩家
				WORD wMaxPlayerUser = INVALID_CHAIR;

				for(WORD i=0;i<m_wPlayerCount;i++)
				{
					//获取用户
					IServerUserItem * pIServerUserItem=m_pITableFrame->GetTableUserItem(i);
					if(pIServerUserItem==NULL) 
					{
						continue;
					}

					if(wMaxPlayerUser==INVALID_CHAIR) 
					{
						wMaxPlayerUser=i;
					}

					//获取较大者
					if(m_GameLogic.CompareCard(cbUserCardData[i],m_cbHandCardData[wMaxPlayerUser],MAX_CARDCOUNT, m_ctConfig, m_cbCombineCardType[i], m_cbCombineCardType[wMaxPlayerUser])==true)
					{
						wMaxPlayerUser=i;
					}
				}
				
				BYTE cbMaxCardType = ((m_cbCombineCardType[wMaxPlayerUser] == INVALID_BYTE) ? (m_GameLogic.GetCardType(m_cbHandCardData[wMaxPlayerUser], MAX_CARDCOUNT, m_ctConfig)) : m_cbCombineCardType[wMaxPlayerUser]);
				
				//如果牌型比牛牛少原来的玩家当庄
				if (cbMaxCardType >= CT_ADDTIMES_OX_VALUENIUNIU)
				{
					//牌型最大玩家当庄
					m_wBankerUser = wMaxPlayerUser;
				}
			}
			//无牛下庄
			else if (m_bgtConfig == BGT_NONIUNIU_)
			{
				//庄家无牛轮到下一位上庄
				ASSERT (m_wBankerUser != INVALID_CHAIR);
				BYTE cbBankerCardType = ((m_cbCombineCardType[m_wBankerUser] == INVALID_BYTE) ? (m_GameLogic.GetCardType(m_cbHandCardData[m_wBankerUser], MAX_CARDCOUNT, m_ctConfig)) : m_cbCombineCardType[m_wBankerUser]);
				if (cbBankerCardType == CT_CLASSIC_OX_VALUE0
					|| cbBankerCardType == CT_ADDTIMES_OX_VALUE0)
				{
					//始叫用户
					while(true)
					{
						m_wBankerUser = (m_wBankerUser+1) % m_wPlayerCount;
						if (m_pITableFrame->GetTableUserItem(m_wBankerUser) != NULL && m_cbPlayStatus[m_wBankerUser] == TRUE)
						{
							break;
						}
					}
				}
			}
				
			//发送库存
			CString strInfo;
			strInfo.Format(TEXT("当前库存：%I64d"), g_lRoomStorageCurrent);
			for (WORD i=0; i<m_wPlayerCount; i++)
			{
				IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if (!pIServerUserItem)
				{
					continue;
				}
				if (CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()) && !pIServerUserItem->IsAndroidUser())
				{
					m_pITableFrame->SendGameMessage(pIServerUserItem, strInfo, SMT_CHAT);

					CMD_S_ADMIN_STORAGE_INFO StorageInfo;
					ZeroMemory(&StorageInfo, sizeof(StorageInfo));
					StorageInfo.lRoomStorageStart = g_lRoomStorageStart;
					StorageInfo.lRoomStorageCurrent = g_lRoomStorageCurrent;
					StorageInfo.lRoomStorageDeduct = g_lStorageDeductRoom;
					StorageInfo.lMaxRoomStorage[0] = g_lStorageMax1Room;
					StorageInfo.lMaxRoomStorage[1] = g_lStorageMax2Room;
					StorageInfo.wRoomStorageMul[0] = (WORD)g_lStorageMul1Room;
					StorageInfo.wRoomStorageMul[1] = (WORD)g_lStorageMul2Room;
					m_pITableFrame->SendRoomData(NULL, SUB_S_ADMIN_STORAGE_INFO, &StorageInfo, sizeof(StorageInfo));
					//m_pITableFrame->SendLookonData(i, SUB_S_ADMIN_STORAGE_INFO, &StorageInfo, sizeof(StorageInfo));
				}
			}
			
			//房卡模式
			if ((m_pGameServiceOption->wServerType & GAME_GENRE_PERSONAL) !=0 )
			{
				//解散清理记录
				if (m_pITableFrame->IsPersonalRoomDisumme())
				{
					ZeroMemory(&m_stRecord, sizeof(m_stRecord));
					ZeroMemory(&m_RoomCardRecord, sizeof(m_RoomCardRecord));
				}
			}

			m_pITableFrame->ConcludeGame(GS_TK_FREE);

			if(!IsRoomCardType())
			{
				//删除时间
				m_pITableFrame->KillGameTimer(IDI_SO_OPERATE);
			}

			//更新房间用户信息
			for (WORD i=0; i<m_wPlayerCount; i++)
			{
				//获取用户
				IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);

				if (!pIServerUserItem)
				{
					continue;
				}

				UpdateRoomUserInfo(pIServerUserItem, USER_SITDOWN);
			}
			
			return true;
		}
	case GER_USER_LEAVE:		//用户强退
	case GER_NETWORK_ERROR:
		{
			//效验参数
			ASSERT(pIServerUserItem!=NULL);
			ASSERT(wChairID<m_wPlayerCount && (m_cbPlayStatus[wChairID]==TRUE||m_cbDynamicJoin[wChairID]==FALSE));

			if(m_cbPlayStatus[wChairID]==FALSE) return true;
			//设置状态
			m_cbPlayStatus[wChairID]=FALSE;
			m_cbDynamicJoin[wChairID]=FALSE;

			//定义变量
			CMD_S_PlayerExit PlayerExit;
			PlayerExit.wPlayerID=wChairID;

			//发送信息
			for (WORD i=0;i<m_wPlayerCount;i++)
			{
				if(i==wChairID || (m_cbPlayStatus[i]==FALSE&&m_cbDynamicJoin[i]==FALSE))continue;
				m_pITableFrame->SendTableData(i,SUB_S_PLAYER_EXIT,&PlayerExit,sizeof(PlayerExit));
			}
			m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_PLAYER_EXIT,&PlayerExit,sizeof(PlayerExit));


			WORD wWinTimes[GAME_PLAYER];
			ZeroMemory(wWinTimes,sizeof(wWinTimes));

			//倍数抢庄 结算需要乘以cbMaxCallBankerTimes
			BYTE cbMaxCallBankerTimes = 1;
			if (m_bgtConfig == BGT_ROB_)
			{
				for (WORD i=0;i<m_wPlayerCount;i++)
				{
					if(m_cbPlayStatus[i]==TRUE && m_cbCallBankerStatus[i]==TRUE && m_cbCallBankerTimes[i] > cbMaxCallBankerTimes) 
					{
						cbMaxCallBankerTimes = m_cbCallBankerTimes[i];
					}
				}
			}
			
			//游戏进行 （下注后）
			if (m_pITableFrame->GetGameStatus() == GS_TK_PLAYING)
			{
				if (wChairID==m_wBankerUser)	//庄家强退
				{
					//定义变量
					CMD_S_GameEnd GameEnd;
					ZeroMemory(&GameEnd,sizeof(GameEnd));
					ZeroMemory(wWinTimes,sizeof(wWinTimes));
					CopyMemory(GameEnd.cbHandCardData, m_cbHandCardData, sizeof(m_cbHandCardData));

					BYTE cbUserCardData[GAME_PLAYER][MAX_CARDCOUNT];
					CopyMemory(cbUserCardData,m_cbHandCardData,sizeof(cbUserCardData));

					//得分倍数
					for (WORD i=0;i<m_wPlayerCount;i++)
					{
						if(i==m_wBankerUser || m_cbPlayStatus[i]==FALSE)continue;
						wWinTimes[i]=(m_pITableFrame->GetGameStatus()!=GS_TK_PLAYING)?(1):(m_GameLogic.GetTimes(cbUserCardData[i],MAX_CARDCOUNT, m_ctConfig, m_cbCombineCardType[i]));
					}

					//统计得分 已下或没下
					for (WORD i=0;i<m_wPlayerCount;i++)
					{
						if(i==m_wBankerUser || m_cbPlayStatus[i]==FALSE)continue;
						GameEnd.lGameScore[i]=m_lTableScore[i]*wWinTimes[i] * m_pITableFrame->GetCellScore() * cbMaxCallBankerTimes;
						GameEnd.lGameScore[m_wBankerUser]-=GameEnd.lGameScore[i];
						m_lTableScore[i]=0;
					}

					//修改积分
					tagScoreInfo ScoreInfoArray[GAME_PLAYER];
					ZeroMemory(&ScoreInfoArray,sizeof(ScoreInfoArray));

					//积分税收
					for(WORD i=0;i<m_wPlayerCount;i++)
					{
						if(m_cbPlayStatus[i]==FALSE && i!=m_wBankerUser)continue;
						
						if(GameEnd.lGameScore[i]>0L)
						{
							GameEnd.lGameTax[i]=m_pITableFrame->CalculateRevenue(i,GameEnd.lGameScore[i]);
							if(GameEnd.lGameTax[i]>0)
								GameEnd.lGameScore[i]-=GameEnd.lGameTax[i];
						}

						//保存积分
						ScoreInfoArray[i].lRevenue = GameEnd.lGameTax[i];
						ScoreInfoArray[i].lScore = GameEnd.lGameScore[i];

						if(i==m_wBankerUser)
							ScoreInfoArray[i].cbType =SCORE_TYPE_FLEE;
						else if(m_cbPlayStatus[i]==TRUE)
							ScoreInfoArray[i].cbType =(GameEnd.lGameScore[i]>0L)?SCORE_TYPE_WIN:SCORE_TYPE_LOSE;


					}
						
					//获取玩家牌型			
					for(WORD i=0;i<m_wPlayerCount;i++)
					{
						if(m_cbPlayStatus[i]==FALSE)
						{
							continue;
						}

						GameEnd.cbCardType[i] = (m_cbCombineCardType[i] == 0 ? m_cbOriginalCardType[i] : m_cbCombineCardType[i]);
					}

					//发送信息
					for (WORD i=0;i<m_wPlayerCount;i++)
					{
						if(i==m_wBankerUser || (m_cbPlayStatus[i]==FALSE&&m_cbDynamicJoin[i]==FALSE))continue;
						m_pITableFrame->SendTableData(i,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));
					}
					m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));
						
					if (m_pGameVideo)
					{
						m_pGameVideo->AddVideoData(SUB_S_GAME_END, &GameEnd);
					}

					if (m_pGameVideo)
					{
						m_pGameVideo->StopAndSaveVideo(m_pGameServiceOption->wServerID, m_pITableFrame->GetTableID());
					}

					TryWriteTableScore(ScoreInfoArray);

					//写入库存
					for (WORD i=0;i<m_wPlayerCount;i++)
					{
						if(m_cbPlayStatus[i]==FALSE && i!=m_wBankerUser)continue;

						//获取用户
						IServerUserItem * pIServerUserIte=m_pITableFrame->GetTableUserItem(i);

						//库存累计
						if ((pIServerUserIte!=NULL)&&(!pIServerUserIte->IsAndroidUser())) 
							g_lRoomStorageCurrent-=GameEnd.lGameScore[i];

					}
					//结束游戏
					m_pITableFrame->ConcludeGame(GS_TK_FREE);

					if(!IsRoomCardType())
					{
						//删除时间
						m_pITableFrame->KillGameTimer(IDI_SO_OPERATE);
					}

					UpdateRoomUserInfo(pIServerUserItem, USER_STANDUP);

					//更新房间用户信息
					for (WORD i=0; i<m_wPlayerCount; i++)
					{
						if (i == wChairID)
						{
							continue;
						}

						//获取用户
						IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);

						if (!pIServerUserItem)
						{
							continue;
						}

						UpdateRoomUserInfo(pIServerUserItem, USER_SITDOWN);
					}

					return true;
				}
				else						//闲家强退
				{
					//已经下注
					if (m_lTableScore[wChairID]>0L)
					{
						ZeroMemory(wWinTimes,sizeof(wWinTimes));

						//用户扑克
						BYTE cbUserCardData[MAX_CARDCOUNT];
						CopyMemory(cbUserCardData,m_cbHandCardData[m_wBankerUser],MAX_CARDCOUNT);

						//用户倍数
						wWinTimes[m_wBankerUser]=(m_pITableFrame->GetGameStatus()==GS_TK_SCORE)?(1):(m_GameLogic.GetTimes(cbUserCardData,MAX_CARDCOUNT, m_ctConfig, m_cbCombineCardType[m_wBankerUser]));

						//修改积分
						LONGLONG lScore=-m_lTableScore[wChairID]*wWinTimes[m_wBankerUser] * m_pITableFrame->GetCellScore() * cbMaxCallBankerTimes;
						m_lExitScore+=(-1*lScore);
						m_lTableScore[wChairID]=(-1*lScore);
						
						tagScoreInfo ScoreInfo;
						ZeroMemory(&ScoreInfo,sizeof(ScoreInfo));
						ScoreInfo.lScore = lScore;
						ScoreInfo.cbType = SCORE_TYPE_FLEE;
						
						m_pITableFrame->WriteUserScore(wChairID, ScoreInfo);

						//获取用户
						IServerUserItem * pIServerUserIte=m_pITableFrame->GetTableUserItem(wChairID);
						
						//库存累计
						if ((pIServerUserIte!=NULL)&&(!pIServerUserIte->IsAndroidUser())) 
							g_lRoomStorageCurrent-=lScore;
					}

					//玩家人数
					WORD wUserCount=0;
					for (WORD i=0;i<m_wPlayerCount;i++)if(m_cbPlayStatus[i]==TRUE)wUserCount++;

					//结束游戏
					if(wUserCount==1)
					{
						//定义变量
						CMD_S_GameEnd GameEnd;
						ZeroMemory(&GameEnd,sizeof(GameEnd));
						CopyMemory(GameEnd.cbHandCardData, m_cbHandCardData, sizeof(m_cbHandCardData));
						ASSERT(m_lExitScore>=0L); 

						//统计得分
						GameEnd.lGameScore[m_wBankerUser]+=m_lExitScore;
						GameEnd.lGameTax[m_wBankerUser]=m_pITableFrame->CalculateRevenue(m_wBankerUser,GameEnd.lGameScore[m_wBankerUser]);
						GameEnd.lGameScore[m_wBankerUser]-=GameEnd.lGameTax[m_wBankerUser];

						//获取玩家牌型			
						for(WORD i=0;i<m_wPlayerCount;i++)
						{
							if(m_cbPlayStatus[i]==FALSE)
							{
								continue;
							}

							GameEnd.cbCardType[i] = (m_cbCombineCardType[i] == 0 ? m_cbOriginalCardType[i] : m_cbCombineCardType[i]);
						}

						//发送信息
						m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));
						m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));

						if (m_pGameVideo)
						{
							m_pGameVideo->AddVideoData(SUB_S_GAME_END, &GameEnd);
							m_pGameVideo->StopAndSaveVideo(m_pGameServiceOption->wServerID, m_pITableFrame->GetTableID());
						}

						WORD Zero = 0;
						for (; Zero<m_wPlayerCount; Zero++)if (m_lTableScore[Zero] != 0)break;
						if(Zero!=m_wPlayerCount)
						{
							//修改积分
							tagScoreInfo ScoreInfo;
							ZeroMemory(&ScoreInfo,sizeof(ScoreInfo));
							ScoreInfo.lScore=GameEnd.lGameScore[m_wBankerUser];
							ScoreInfo.lRevenue = GameEnd.lGameTax[m_wBankerUser];
							ScoreInfo.cbType = SCORE_TYPE_WIN;

							m_pITableFrame->WriteUserScore(m_wBankerUser, ScoreInfo);

							//获取用户
							IServerUserItem * pIServerUserIte=m_pITableFrame->GetTableUserItem(wChairID);
							
							//库存累计
							if ((pIServerUserIte!=NULL)&&(!pIServerUserIte->IsAndroidUser())) 
								g_lRoomStorageCurrent-=GameEnd.lGameScore[m_wBankerUser];

						}

						//结束游戏
						m_pITableFrame->ConcludeGame(GS_TK_FREE);

						if(!IsRoomCardType())
						{
							//删除时间
							m_pITableFrame->KillGameTimer(IDI_SO_OPERATE);
						}

						UpdateRoomUserInfo(pIServerUserItem, USER_STANDUP);

						//更新房间用户信息
						for (WORD i=0; i<m_wPlayerCount; i++)
						{
							if (i == wChairID)
							{
								continue;
							}

							//获取用户
							IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);

							if (!pIServerUserItem)
							{
								continue;
							}

							UpdateRoomUserInfo(pIServerUserItem, USER_SITDOWN);
						}

						return true;
					}
					
					OnUserOpenCard(wChairID, m_cbHandCardData[wChairID]);
				}
			}
			//下注逃跑
			else if (m_pITableFrame->GetGameStatus() == GS_TK_SCORE)
			{
				//剩余玩家人数
				WORD wUserCount=0;
				for (WORD i=0;i<m_wPlayerCount;i++)
				{
					if(m_cbPlayStatus[i]==TRUE)
					{
						wUserCount++;
					}
				}

				//发四等五 (已发4张牌) 按照最小筹码进行赔付
				if (m_stConfig == ST_SENDFOUR_)
				{
					//获取最小筹码
					LONGLONG lMinJetton = 0L;

					//自由配置额度
					if (m_btConfig == BT_FREE_)
					{
						lMinJetton = m_lFreeConfig[0];
					}
					//百分比配置额度
					else if (m_btConfig == BT_PENCENT_)
					{
						//最小下注玩家
						WORD wMinJettonChairID = INVALID_CHAIR;
						for(WORD i=0;i<m_wPlayerCount;i++)
						{
							//获取用户
							IServerUserItem * pIServerUserItem=m_pITableFrame->GetTableUserItem(i);
							if(pIServerUserItem==NULL || m_cbPlayStatus[i] == FALSE) 
							{
								continue;
							}

							if(wMinJettonChairID==INVALID_CHAIR) wMinJettonChairID=i;

							//获取较大者
							if(m_lTurnMaxScore[i] < m_lTurnMaxScore[wMinJettonChairID])
							{
								wMinJettonChairID=i;
							}
						}

						ASSERT (wMinJettonChairID != INVALID_CHAIR);

						lMinJetton = m_lTurnMaxScore[wMinJettonChairID] * m_lPercentConfig[0] / 100;
					}
					
					CMD_S_GameEnd GameEnd;
					ZeroMemory(&GameEnd,sizeof(GameEnd));
					CopyMemory(GameEnd.cbHandCardData, m_cbHandCardData, sizeof(m_cbHandCardData));

					tagScoreInfo ScoreInfoArray[GAME_PLAYER];
					ZeroMemory(&ScoreInfoArray,sizeof(ScoreInfoArray));

					//庄家逃跑结束游戏       
					if (wChairID == m_wBankerUser)
					{
						//发四等五	庄家逃跑，不显示牌型	
						for(WORD i=0;i<m_wPlayerCount;i++)
						{
							GameEnd.cbCardType[i] = INVALID_BYTE;
							//if(m_cbPlayStatus[i]==FALSE)
							//{
							//	continue;
							//}

							//GameEnd.cbCardType[i] = (m_cbCombineCardType[i] == 0 ? m_cbOriginalCardType[i] : m_cbCombineCardType[i]);
						}

						LONGLONG lBankerScore = 0L;
						for(WORD i=0;i<m_wPlayerCount;i++)
						{
							//获取用户
							IServerUserItem * pIServerUserItem=m_pITableFrame->GetTableUserItem(i);
							if(pIServerUserItem==NULL || m_cbPlayStatus[i] == FALSE || i == m_wBankerUser) 
							{
								continue;
							}

							//统计得分
							GameEnd.lGameScore[i] += lMinJetton;
							GameEnd.lGameTax[i] = m_pITableFrame->CalculateRevenue(i, GameEnd.lGameScore[i]);
							GameEnd.lGameScore[i] -= GameEnd.lGameTax[i];
							
							//写分
							ScoreInfoArray[i].lScore=GameEnd.lGameScore[i];
							ScoreInfoArray[i].lRevenue = GameEnd.lGameTax[i];
							ScoreInfoArray[i].cbType = SCORE_TYPE_WIN;

							lBankerScore -= lMinJetton;
						}

						GameEnd.lGameScore[m_wBankerUser] = lBankerScore;
						ScoreInfoArray[m_wBankerUser].lScore=GameEnd.lGameScore[m_wBankerUser];
						ScoreInfoArray[m_wBankerUser].cbType = SCORE_TYPE_FLEE;						
						
						//发送信息
						m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));
						m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));

						if (m_pGameVideo)
						{
							m_pGameVideo->AddVideoData(SUB_S_GAME_END, &GameEnd);
							m_pGameVideo->StopAndSaveVideo(m_pGameServiceOption->wServerID, m_pITableFrame->GetTableID());
						}

						//写分
						TryWriteTableScore(ScoreInfoArray);

						//结束游戏
						m_pITableFrame->ConcludeGame(GS_TK_FREE);

						if(!IsRoomCardType())
						{
							//删除时间
							m_pITableFrame->KillGameTimer(IDI_SO_OPERATE);
						}

						UpdateRoomUserInfo(pIServerUserItem, USER_STANDUP);

						//更新房间用户信息
						for (WORD i=0; i<m_wPlayerCount; i++)
						{
							if (i == wChairID)
							{
								continue;
							}

							//获取用户
							IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);

							if (!pIServerUserItem)
							{
								continue;
							}

							UpdateRoomUserInfo(pIServerUserItem, USER_SITDOWN);
						}

						return true;
					}
					//闲家逃跑，只剩下一个玩家
					else if (wChairID != m_wBankerUser && wUserCount == 1)
					{
						//统计得分
						GameEnd.lGameScore[wChairID] -= lMinJetton;
						GameEnd.lGameScore[m_wBankerUser] += lMinJetton;
						GameEnd.lGameTax[m_wBankerUser] = m_pITableFrame->CalculateRevenue(m_wBankerUser, GameEnd.lGameScore[m_wBankerUser]);
						GameEnd.lGameScore[m_wBankerUser] -= GameEnd.lGameTax[m_wBankerUser];

						//写分
						ScoreInfoArray[wChairID].lScore=GameEnd.lGameScore[wChairID];
						ScoreInfoArray[wChairID].cbType = SCORE_TYPE_FLEE;
						ScoreInfoArray[m_wBankerUser].lScore=GameEnd.lGameScore[m_wBankerUser];
						ScoreInfoArray[m_wBankerUser].cbType = SCORE_TYPE_WIN;
						ScoreInfoArray[m_wBankerUser].lRevenue = GameEnd.lGameTax[m_wBankerUser];

						//发送信息
						m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));
						m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));

						if (m_pGameVideo)
						{
							m_pGameVideo->AddVideoData(SUB_S_GAME_END, &GameEnd);
							m_pGameVideo->StopAndSaveVideo(m_pGameServiceOption->wServerID, m_pITableFrame->GetTableID());
						}

						//写分
						TryWriteTableScore(ScoreInfoArray);

						//结束游戏
						m_pITableFrame->ConcludeGame(GS_TK_FREE);

						if(!IsRoomCardType())
						{
							//删除时间
							m_pITableFrame->KillGameTimer(IDI_SO_OPERATE);
						}

						UpdateRoomUserInfo(pIServerUserItem, USER_STANDUP);

						//更新房间用户信息
						for (WORD i=0; i<m_wPlayerCount; i++)
						{
							if (i == wChairID)
							{
								continue;
							}

							//获取用户
							IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);

							if (!pIServerUserItem)
							{
								continue;
							}

							UpdateRoomUserInfo(pIServerUserItem, USER_SITDOWN);
						}

						return true;
					}
					//闲家逃跑，剩下2个以上玩家 继续游戏
					else
					{
						m_lExitScore += lMinJetton;
						m_lTableScore[wChairID] = 0;

						tagScoreInfo ScoreInfoArray[GAME_PLAYER];
						ZeroMemory(ScoreInfoArray,sizeof(ScoreInfoArray));
						ScoreInfoArray[wChairID].lScore -= lMinJetton;
						ScoreInfoArray[wChairID].cbType = SCORE_TYPE_FLEE;

						TryWriteTableScore(ScoreInfoArray);

						OnUserAddScore(wChairID,0);
					}
				}
				//下注发牌 (未发牌不需要赔付)
				else if (m_stConfig == ST_BETFIRST_)
				{
					//庄家逃跑结束游戏 或只剩下一个玩家
					if (wChairID == m_wBankerUser || wUserCount == 1)
					{
						CMD_S_GameEnd GameEnd;
						ZeroMemory(&GameEnd,sizeof(GameEnd));
					
						//发送信息
						m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));
						m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));

						if (m_pGameVideo)
						{
							m_pGameVideo->AddVideoData(SUB_S_GAME_END, &GameEnd);
							m_pGameVideo->StopAndSaveVideo(m_pGameServiceOption->wServerID, m_pITableFrame->GetTableID());
						}

						//结束游戏
						m_pITableFrame->ConcludeGame(GS_TK_FREE);	

						if(!IsRoomCardType())
						{
							//删除时间
							m_pITableFrame->KillGameTimer(IDI_SO_OPERATE);
						}

						UpdateRoomUserInfo(pIServerUserItem, USER_STANDUP);

						//更新房间用户信息
						for (WORD i=0; i<m_wPlayerCount; i++)
						{
							if (i == wChairID)
							{
								continue;
							}

							//获取用户
							IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);

							if (!pIServerUserItem)
							{
								continue;
							}

							UpdateRoomUserInfo(pIServerUserItem, USER_SITDOWN);
						}

						return true;
					}
					//闲家逃跑 且游戏人数大于1  继续游戏
					else
					{
						ASSERT (wUserCount >= 2);
						
						OnUserAddScore(wChairID,0);
						m_lTableScore[wChairID]=0L;
					}
				}
			}
			//叫庄状态逃跑
			else 
			{
				//玩家人数
				WORD wUserCount=0;
				for (WORD i=0;i<m_wPlayerCount;i++)if(m_cbPlayStatus[i]==TRUE)wUserCount++;

				//结束游戏
				if(wUserCount==1)
				{
					//定义变量
					CMD_S_GameEnd GameEnd;
					ZeroMemory(&GameEnd,sizeof(GameEnd));

					//发送信息
					for (WORD i=0;i<m_wPlayerCount;i++)
					{
						if(m_cbPlayStatus[i]==FALSE&&m_cbDynamicJoin[i]==FALSE)continue;
						m_pITableFrame->SendTableData(i,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));
					}
					m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));

					if (m_pGameVideo)
					{
						m_pGameVideo->AddVideoData(SUB_S_GAME_END, &GameEnd);
					}

					if (m_pGameVideo)
					{
						m_pGameVideo->StopAndSaveVideo(m_pGameServiceOption->wServerID, m_pITableFrame->GetTableID());
					}

					//结束游戏
					m_pITableFrame->ConcludeGame(GS_TK_FREE);	

					if(!IsRoomCardType())
					{
						//删除时间
						m_pITableFrame->KillGameTimer(IDI_SO_OPERATE);
					}

					UpdateRoomUserInfo(pIServerUserItem, USER_STANDUP);

					//更新房间用户信息
					for (WORD i=0; i<m_wPlayerCount; i++)
					{
						if (i == wChairID)
						{
							continue;
						}

						//获取用户
						IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);

						if (!pIServerUserItem)
						{
							continue;
						}

						UpdateRoomUserInfo(pIServerUserItem, USER_SITDOWN);
					}

					return true;
				}
				else
				{
					OnUserCallBanker(wChairID, false, 0);
				}
			}

			return true;
		}
	}

	return false;
}

//发送场景
bool CTableFrameSink::OnEventSendGameScene(WORD wChairID, IServerUserItem * pIServerUserItem, BYTE cbGameStatus, bool bSendSecret)
{
	switch (cbGameStatus)
	{
	case GAME_STATUS_FREE:		//空闲状态
		{
			//私人房间
			if ((m_pGameServiceOption->wServerType&GAME_GENRE_PERSONAL) !=0 )
			{
				//cbGameRule[1] 为  2 、3 、4, 5, 6 0分别对应 2人 、 3人 、 4人 、5, 6 2-6人 这几种配置
				BYTE *pGameRule = m_pITableFrame->GetGameRule();
				if (pGameRule[1] != 0)
				{
					m_wPlayerCount = pGameRule[1];

					//设置人数
					m_pITableFrame->SetTableChairCount(m_wPlayerCount);
				}
				else
				{
					m_wPlayerCount = GAME_PLAYER;

					//设置人数
					m_pITableFrame->SetTableChairCount(GAME_PLAYER);
				}

				ASSERT (pGameRule[3] == 22 || pGameRule[3] == 23);
				m_ctConfig = (CARDTYPE_CONFIG)(pGameRule[3]);

				ASSERT (pGameRule[4] == 32 || pGameRule[4] == 33);
				m_stConfig = (SENDCARDTYPE_CONFIG)(pGameRule[4]);

				ASSERT (pGameRule[5] == 42 || pGameRule[5] == 43);
				m_gtConfig = (KING_CONFIG)(pGameRule[5]);

				ASSERT (pGameRule[6] == 52 || pGameRule[6] == 53 || pGameRule[6] == 54 || pGameRule[6] == 55);
				m_bgtConfig = (BANERGAMETYPE_CONFIG)(pGameRule[6]);

				//下注配置只能在后台配置
			}

			//构造数据
			CMD_S_StatusFree StatusFree;
			ZeroMemory(&StatusFree,sizeof(StatusFree));

			//设置变量
			StatusFree.lCellScore=0L;
			StatusFree.lRoomStorageStart = g_lRoomStorageStart;
			StatusFree.lRoomStorageCurrent = g_lRoomStorageCurrent;
			
			StatusFree.ctConfig = m_ctConfig;
			StatusFree.stConfig = m_stConfig;
			StatusFree.bgtConfig = m_bgtConfig;
			StatusFree.btConfig = m_btConfig;
			StatusFree.lMinTableScore = m_pGameServiceOption->lMinTableScore;

			//历史积分
			for (WORD i=0;i<m_wPlayerCount;i++)
			{
				tagHistoryScore * pHistoryScore=m_HistoryScore.GetHistoryScore(i);
				StatusFree.lTurnScore[i]=pHistoryScore->lTurnScore;
				StatusFree.lCollectScore[i]=pHistoryScore->lCollectScore;
			}

			
			//获取自定义配置
			tagCustomRule *pCustomRule = (tagCustomRule *)m_pGameServiceOption->cbCustomRule;
			ASSERT(pCustomRule);
			tagCustomAndroid CustomAndroid;
			ZeroMemory(&CustomAndroid, sizeof(CustomAndroid));
			CustomAndroid.lRobotBankGet = pCustomRule->lRobotBankGet;
			CustomAndroid.lRobotBankGetBanker = pCustomRule->lRobotBankGetBanker;
			CustomAndroid.lRobotBankStoMul = pCustomRule->lRobotBankStoMul;
			CustomAndroid.lRobotScoreMax = pCustomRule->lRobotScoreMax;
			CustomAndroid.lRobotScoreMin = pCustomRule->lRobotScoreMin;
			CopyMemory(&StatusFree.CustomAndroid, &CustomAndroid, sizeof(CustomAndroid));
			BYTE *pGameRule = m_pITableFrame->GetGameRule();
			StatusFree.wGamePlayerCountRule = pGameRule[1];

			//防作弊
			if(CServerRule::IsAllowAvertCheatMode(m_pGameServiceOption->dwServerRule))
			{
				StatusFree.bIsAllowAvertCheat = true;
			}

			//权限判断
			if(CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()) && !pIServerUserItem->IsAndroidUser())
			{
				CMD_S_ADMIN_STORAGE_INFO StorageInfo;
				ZeroMemory(&StorageInfo, sizeof(StorageInfo));
				StorageInfo.lRoomStorageStart = g_lRoomStorageStart;
				StorageInfo.lRoomStorageCurrent = g_lRoomStorageCurrent;
				StorageInfo.lRoomStorageDeduct = g_lStorageDeductRoom;
				StorageInfo.lMaxRoomStorage[0] = g_lStorageMax1Room;
				StorageInfo.lMaxRoomStorage[1] = g_lStorageMax2Room;
				StorageInfo.wRoomStorageMul[0] = (WORD)g_lStorageMul1Room;
				StorageInfo.wRoomStorageMul[1] = (WORD)g_lStorageMul2Room;
				
				m_pITableFrame->SendRoomData(NULL, SUB_S_ADMIN_STORAGE_INFO, &StorageInfo, sizeof(StorageInfo));
				//m_pITableFrame->SendLookonData(wChairID, SUB_S_ADMIN_STORAGE_INFO, &StorageInfo, sizeof(StorageInfo));
			}

			//房卡模式
			if ((m_pGameServiceOption->wServerType & GAME_GENRE_PERSONAL) != 0)
			{
				m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_RECORD, &m_stRecord, sizeof(m_stRecord));

				CMD_S_RoomCardRecord RoomCardRecord;
				ZeroMemory(&RoomCardRecord, sizeof(RoomCardRecord));

				CopyMemory(&RoomCardRecord, &m_RoomCardRecord, sizeof(m_RoomCardRecord));

				m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_ROOMCARD_RECORD, &RoomCardRecord, sizeof(RoomCardRecord));
				m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_ROOMCARD_RECORD, &RoomCardRecord, sizeof(RoomCardRecord));
			}

			//发送场景
			return m_pITableFrame->SendGameScene(pIServerUserItem,&StatusFree,sizeof(StatusFree));
		}
	case GS_TK_CALL:	//叫庄状态
		{
			//构造数据
			CMD_S_StatusCall StatusCall;
			ZeroMemory(&StatusCall,sizeof(StatusCall));

			//历史积分
			for (WORD i=0;i<m_wPlayerCount;i++)
			{
				tagHistoryScore * pHistoryScore=m_HistoryScore.GetHistoryScore(i);
				StatusCall.lTurnScore[i]=pHistoryScore->lTurnScore;
				StatusCall.lCollectScore[i]=pHistoryScore->lCollectScore;
			}

			//设置变量
			StatusCall.cbDynamicJoin=m_cbDynamicJoin[wChairID];
			CopyMemory(StatusCall.cbPlayStatus,m_cbPlayStatus,sizeof(StatusCall.cbPlayStatus));
			StatusCall.lRoomStorageStart = g_lRoomStorageStart;
			StatusCall.lRoomStorageCurrent = g_lRoomStorageCurrent;
			
			StatusCall.ctConfig = m_ctConfig;
			StatusCall.stConfig = m_stConfig;
			StatusCall.bgtConfig = m_bgtConfig;
			StatusCall.btConfig = m_btConfig;
			StatusCall.lMinTableScore = m_pGameServiceOption->lMinTableScore;

			//获取自定义配置
			tagCustomRule *pCustomRule = (tagCustomRule *)m_pGameServiceOption->cbCustomRule;
			ASSERT(pCustomRule);
			tagCustomAndroid CustomAndroid;
			ZeroMemory(&CustomAndroid, sizeof(CustomAndroid));
			CustomAndroid.lRobotBankGet = pCustomRule->lRobotBankGet;
			CustomAndroid.lRobotBankGetBanker = pCustomRule->lRobotBankGetBanker;
			CustomAndroid.lRobotBankStoMul = pCustomRule->lRobotBankStoMul;
			CustomAndroid.lRobotScoreMax = pCustomRule->lRobotScoreMax;
			CustomAndroid.lRobotScoreMin = pCustomRule->lRobotScoreMin;
			CopyMemory(&StatusCall.CustomAndroid, &CustomAndroid, sizeof(CustomAndroid));
			BYTE *pGameRule = m_pITableFrame->GetGameRule();
			StatusCall.wGamePlayerCountRule = pGameRule[1];

			//防作弊
			if(CServerRule::IsAllowAvertCheatMode(m_pGameServiceOption->dwServerRule))
			{
				StatusCall.bIsAllowAvertCheat = true;
			}

			CopyMemory(StatusCall.cbCallBankerStatus, m_cbCallBankerStatus, sizeof(StatusCall.cbCallBankerStatus));
			CopyMemory(StatusCall.cbCallBankerTimes, m_cbCallBankerTimes, sizeof(StatusCall.cbCallBankerTimes));
			
			//更新房间用户信息
			UpdateRoomUserInfo(pIServerUserItem, USER_RECONNECT);

			//权限判断
			if(CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()) && !pIServerUserItem->IsAndroidUser())
			{
				CMD_S_ADMIN_STORAGE_INFO StorageInfo;
				ZeroMemory(&StorageInfo, sizeof(StorageInfo));
				StorageInfo.lRoomStorageStart = g_lRoomStorageStart;
				StorageInfo.lRoomStorageCurrent = g_lRoomStorageCurrent;
				StorageInfo.lRoomStorageDeduct = g_lStorageDeductRoom;
				StorageInfo.lMaxRoomStorage[0] = g_lStorageMax1Room;
				StorageInfo.lMaxRoomStorage[1] = g_lStorageMax2Room;
				StorageInfo.wRoomStorageMul[0] = (WORD)g_lStorageMul1Room;
				StorageInfo.wRoomStorageMul[1] = (WORD)g_lStorageMul2Room;
				m_pITableFrame->SendRoomData(NULL,  SUB_S_ADMIN_STORAGE_INFO, &StorageInfo, sizeof(StorageInfo));				
			}
			
			//房卡模式
			if ((m_pGameServiceOption->wServerType & GAME_GENRE_PERSONAL) != 0)
			{
				m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_RECORD, &m_stRecord, sizeof(m_stRecord));

				CMD_S_RoomCardRecord RoomCardRecord;
				ZeroMemory(&RoomCardRecord, sizeof(RoomCardRecord));

				CopyMemory(&RoomCardRecord, &m_RoomCardRecord, sizeof(m_RoomCardRecord));

				m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_ROOMCARD_RECORD, &RoomCardRecord, sizeof(RoomCardRecord));
				m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_ROOMCARD_RECORD, &RoomCardRecord, sizeof(RoomCardRecord));
			}

			//发送场景
			return m_pITableFrame->SendGameScene(pIServerUserItem,&StatusCall,sizeof(StatusCall));
		}
	case GS_TK_SCORE:	//下注状态
		{
			//构造数据
			CMD_S_StatusScore StatusScore;
			memset(&StatusScore,0,sizeof(StatusScore));

			//历史积分
			for (WORD i=0;i<m_wPlayerCount;i++)
			{
				tagHistoryScore * pHistoryScore=m_HistoryScore.GetHistoryScore(i);
				StatusScore.lTurnScore[i]=pHistoryScore->lTurnScore;
				StatusScore.lCollectScore[i]=pHistoryScore->lCollectScore;
			}

			//加注信息
			StatusScore.lTurnMaxScore=GetUserMaxTurnScore(wChairID);
			StatusScore.wBankerUser=m_wBankerUser;
			StatusScore.cbDynamicJoin=m_cbDynamicJoin[wChairID];
			CopyMemory(StatusScore.cbPlayStatus,m_cbPlayStatus,sizeof(StatusScore.cbPlayStatus));
			StatusScore.lRoomStorageStart = g_lRoomStorageStart;
			StatusScore.lRoomStorageCurrent = g_lRoomStorageCurrent;
			StatusScore.lMinTableScore = m_pGameServiceOption->lMinTableScore;

			//发四等五
			if (m_stConfig == ST_SENDFOUR_)
			{
				for (WORD i=0;i<m_wPlayerCount;i++)
				{
					if (m_cbPlayStatus[i]==FALSE && m_cbDynamicJoin[i]==FALSE)
					{
						continue;
					}

					//派发扑克(开始只发四张牌)
					CopyMemory(StatusScore.cbCardData[i], m_cbHandCardData[i], sizeof(BYTE) * 4);
				}
			}
			
			StatusScore.ctConfig = m_ctConfig;
			StatusScore.stConfig = m_stConfig;
			StatusScore.bgtConfig = m_bgtConfig;
			StatusScore.btConfig = m_btConfig;
			CopyMemory(StatusScore.lFreeConfig, m_lFreeConfig, sizeof(StatusScore.lFreeConfig));
			CopyMemory(StatusScore.lPercentConfig, m_lPercentConfig, sizeof(StatusScore.lPercentConfig));
			
			//设置积分
			for (WORD i=0;i<m_wPlayerCount;i++)
			{
				if(m_cbPlayStatus[i]==FALSE)continue;
				StatusScore.lTableScore[i]=m_lTableScore[i];
			}

			//获取自定义配置
			tagCustomRule *pCustomRule = (tagCustomRule *)m_pGameServiceOption->cbCustomRule;
			ASSERT(pCustomRule);
			tagCustomAndroid CustomAndroid;
			ZeroMemory(&CustomAndroid, sizeof(CustomAndroid));
			CustomAndroid.lRobotBankGet = pCustomRule->lRobotBankGet;
			CustomAndroid.lRobotBankGetBanker = pCustomRule->lRobotBankGetBanker;
			CustomAndroid.lRobotBankStoMul = pCustomRule->lRobotBankStoMul;
			CustomAndroid.lRobotScoreMax = pCustomRule->lRobotScoreMax;
			CustomAndroid.lRobotScoreMin = pCustomRule->lRobotScoreMin;
			CopyMemory(&StatusScore.CustomAndroid, &CustomAndroid, sizeof(CustomAndroid));
			BYTE *pGameRule = m_pITableFrame->GetGameRule();
			StatusScore.wGamePlayerCountRule = pGameRule[1];

			//防作弊
			if(CServerRule::IsAllowAvertCheatMode(m_pGameServiceOption->dwServerRule))
			{
				StatusScore.bIsAllowAvertCheat = true;
			}

			//更新房间用户信息
			UpdateRoomUserInfo(pIServerUserItem, USER_RECONNECT);

			//权限判断
			if(CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()) && !pIServerUserItem->IsAndroidUser())
			{
				CMD_S_ADMIN_STORAGE_INFO StorageInfo;
				ZeroMemory(&StorageInfo, sizeof(StorageInfo));
				StorageInfo.lRoomStorageStart = g_lRoomStorageStart;
				StorageInfo.lRoomStorageCurrent = g_lRoomStorageCurrent;
				StorageInfo.lRoomStorageDeduct = g_lStorageDeductRoom;
				StorageInfo.lMaxRoomStorage[0] = g_lStorageMax1Room;
				StorageInfo.lMaxRoomStorage[1] = g_lStorageMax2Room;
				StorageInfo.wRoomStorageMul[0] = (WORD)g_lStorageMul1Room;
				StorageInfo.wRoomStorageMul[1] = (WORD)g_lStorageMul2Room;
				m_pITableFrame->SendRoomData(NULL, SUB_S_ADMIN_STORAGE_INFO, &StorageInfo, sizeof(StorageInfo));
				//m_pITableFrame->SendLookonData(wChairID, SUB_S_ADMIN_STORAGE_INFO, &StorageInfo, sizeof(StorageInfo));
			}
			
			//房卡模式
			if ((m_pGameServiceOption->wServerType & GAME_GENRE_PERSONAL) != 0)
			{
				m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_RECORD, &m_stRecord, sizeof(m_stRecord));

				CMD_S_RoomCardRecord RoomCardRecord;
				ZeroMemory(&RoomCardRecord, sizeof(RoomCardRecord));

				CopyMemory(&RoomCardRecord, &m_RoomCardRecord, sizeof(m_RoomCardRecord));

				m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_ROOMCARD_RECORD, &RoomCardRecord, sizeof(RoomCardRecord));
				m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_ROOMCARD_RECORD, &RoomCardRecord, sizeof(RoomCardRecord));
			}

			//发送场景
			return m_pITableFrame->SendGameScene(pIServerUserItem,&StatusScore,sizeof(StatusScore));
		}
	case GS_TK_PLAYING:	//游戏状态
		{
			//构造数据
			CMD_S_StatusPlay StatusPlay;
			memset(&StatusPlay,0,sizeof(StatusPlay));

			//历史积分
			for (WORD i=0;i<m_wPlayerCount;i++)
			{
				tagHistoryScore * pHistoryScore=m_HistoryScore.GetHistoryScore(i);
				StatusPlay.lTurnScore[i]=pHistoryScore->lTurnScore;
				StatusPlay.lCollectScore[i]=pHistoryScore->lCollectScore;
			}

			//设置信息
			StatusPlay.lTurnMaxScore=GetUserMaxTurnScore(wChairID);
			StatusPlay.wBankerUser=m_wBankerUser;
			StatusPlay.cbDynamicJoin=m_cbDynamicJoin[wChairID];
			CopyMemory(StatusPlay.bOpenCard, m_bOpenCard,sizeof(StatusPlay.bOpenCard));
			CopyMemory(StatusPlay.bSpecialCard, m_bSpecialCard, sizeof(StatusPlay.bSpecialCard));
			CopyMemory(StatusPlay.cbOriginalCardType, m_cbOriginalCardType, sizeof(StatusPlay.cbOriginalCardType));
			CopyMemory(StatusPlay.cbCombineCardType, m_cbCombineCardType, sizeof(StatusPlay.cbCombineCardType));

			CopyMemory(StatusPlay.cbPlayStatus,m_cbPlayStatus,sizeof(StatusPlay.cbPlayStatus));
			StatusPlay.lRoomStorageStart = g_lRoomStorageStart;
			StatusPlay.lRoomStorageCurrent = g_lRoomStorageCurrent;
			
			StatusPlay.ctConfig = m_ctConfig;
			StatusPlay.stConfig = m_stConfig;
			StatusPlay.bgtConfig = m_bgtConfig;
			StatusPlay.btConfig = m_btConfig;
			CopyMemory(StatusPlay.lFreeConfig, m_lFreeConfig, sizeof(StatusPlay.lFreeConfig));
			CopyMemory(StatusPlay.lPercentConfig, m_lPercentConfig, sizeof(StatusPlay.lPercentConfig));
			StatusPlay.lMinTableScore = m_pGameServiceOption->lMinTableScore;

			//获取自定义配置
			tagCustomRule *pCustomRule = (tagCustomRule *)m_pGameServiceOption->cbCustomRule;
			ASSERT(pCustomRule);
			tagCustomAndroid CustomAndroid;
			ZeroMemory(&CustomAndroid, sizeof(CustomAndroid));
			CustomAndroid.lRobotBankGet = pCustomRule->lRobotBankGet;
			CustomAndroid.lRobotBankGetBanker = pCustomRule->lRobotBankGetBanker;
			CustomAndroid.lRobotBankStoMul = pCustomRule->lRobotBankStoMul;
			CustomAndroid.lRobotScoreMax = pCustomRule->lRobotScoreMax;
			CustomAndroid.lRobotScoreMin = pCustomRule->lRobotScoreMin;
			CopyMemory(&StatusPlay.CustomAndroid, &CustomAndroid, sizeof(CustomAndroid));
			BYTE *pGameRule = m_pITableFrame->GetGameRule();
			StatusPlay.wGamePlayerCountRule = pGameRule[1];

			//防作弊
			if(CServerRule::IsAllowAvertCheatMode(m_pGameServiceOption->dwServerRule))
			{
				StatusPlay.bIsAllowAvertCheat = true;
			}

			//设置扑克
			for (WORD i=0;i< m_wPlayerCount;i++)
			{
				if(m_cbPlayStatus[i]==FALSE)continue;
				WORD j= i;
				StatusPlay.lTableScore[j]=m_lTableScore[j];
				CopyMemory(StatusPlay.cbHandCardData[j],m_cbHandCardData[j],MAX_CARDCOUNT);
			}
			
			//更新房间用户信息
			UpdateRoomUserInfo(pIServerUserItem, USER_RECONNECT);

			//权限判断
			if(CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()) && !pIServerUserItem->IsAndroidUser())
			{
				CMD_S_ADMIN_STORAGE_INFO StorageInfo;
				ZeroMemory(&StorageInfo, sizeof(StorageInfo));
				StorageInfo.lRoomStorageStart = g_lRoomStorageStart;
				StorageInfo.lRoomStorageCurrent = g_lRoomStorageCurrent;
				StorageInfo.lRoomStorageDeduct = g_lStorageDeductRoom;
				StorageInfo.lMaxRoomStorage[0] = g_lStorageMax1Room;
				StorageInfo.lMaxRoomStorage[1] = g_lStorageMax2Room;
				StorageInfo.wRoomStorageMul[0] = (WORD)g_lStorageMul1Room;
				StorageInfo.wRoomStorageMul[1] = (WORD)g_lStorageMul2Room;
				m_pITableFrame->SendRoomData(NULL, SUB_S_ADMIN_STORAGE_INFO, &StorageInfo, sizeof(StorageInfo));
				//m_pITableFrame->SendLookonData(wChairID, SUB_S_ADMIN_STORAGE_INFO, &StorageInfo, sizeof(StorageInfo));
			}
			
			//房卡模式
			if ((m_pGameServiceOption->wServerType & GAME_GENRE_PERSONAL) != 0)
			{
				m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_RECORD, &m_stRecord, sizeof(m_stRecord));

				CMD_S_RoomCardRecord RoomCardRecord;
				ZeroMemory(&RoomCardRecord, sizeof(RoomCardRecord));

				CopyMemory(&RoomCardRecord, &m_RoomCardRecord, sizeof(m_RoomCardRecord));

				m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_ROOMCARD_RECORD, &RoomCardRecord, sizeof(RoomCardRecord));
				m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_ROOMCARD_RECORD, &RoomCardRecord, sizeof(RoomCardRecord));
			}

			//发送场景
			return m_pITableFrame->SendGameScene(pIServerUserItem,&StatusPlay,sizeof(StatusPlay));
		}
	}
	//效验错误
	ASSERT(FALSE);

	return false;
}

//定时器事件
bool CTableFrameSink::OnTimerMessage(DWORD dwTimerID, WPARAM wBindParam)
{

	switch(dwTimerID)
	{
	case IDI_SO_OPERATE:
		{
			//删除时间
			m_pITableFrame->KillGameTimer(IDI_SO_OPERATE);

			//游戏状态
			switch( m_pITableFrame->GetGameStatus() )
			{
			case GS_TK_CALL:			//用户叫庄
				{
					for (WORD i=0;i<m_wPlayerCount;i++)
					{
						if(m_cbPlayStatus[i]!=TRUE) 
						{
							continue;
						}
						if (m_cbCallBankerStatus[i] == TRUE)
						{
							continue;
						}
						OnUserCallBanker(i, false, 0);
					}

					break;
				}
			case GS_TK_SCORE:			//下注操作
				{
					for(WORD i=0;i<m_wPlayerCount;i++)
					{
						if(m_lTableScore[i]>0L || m_cbPlayStatus[i]==FALSE || i==m_wBankerUser)
							continue;

						if (m_lTurnMaxScore[i] > 0)
						{
							if (m_btConfig == BT_FREE_)
							{
								OnUserAddScore(i, m_lFreeConfig[0]);
							}
							else if (m_btConfig == BT_PENCENT_)
							{
								OnUserAddScore(i, m_lTurnMaxScore[i] * m_lPercentConfig[0] / 100);
							}
						}
						else
						{
							OnUserAddScore(i,1);
						}
					}

					break;
				}
			case GS_TK_PLAYING:			//用户开牌
				{
					for(WORD i=0;i<m_wPlayerCount;i++)
					{
						if(m_bOpenCard[i] == true || m_cbPlayStatus[i]==FALSE)
						{
							continue;
						}
						OnUserOpenCard(i, m_cbHandCardData[i]);
					}

					break;
				}
			default:
				{
					break;
				}
			}

			if(m_pITableFrame->GetGameStatus()!=GS_TK_FREE)
			{
				m_pITableFrame->SetGameTimer(IDI_SO_OPERATE,TIME_SO_OPERATE,1,0);
			}
			return true;
		}
	}
	return false;
}

//游戏消息处理 
bool CTableFrameSink::OnGameMessage(WORD wSubCmdID, void * pDataBuffer, WORD wDataSize, IServerUserItem * pIServerUserItem)
{
	bool bResult=false;
	switch (wSubCmdID)
	{
	case SUB_C_CALL_BANKER:			//用户叫庄
		{
			//效验数据
			ASSERT(wDataSize==sizeof(CMD_C_CallBanker));
			if (wDataSize!=sizeof(CMD_C_CallBanker)) return false;

			//变量定义
			CMD_C_CallBanker * pCallBanker=(CMD_C_CallBanker *)pDataBuffer;

			//用户效验
			tagUserInfo * pUserData=pIServerUserItem->GetUserInfo();
			//if (pUserData->cbUserStatus!=US_PLAYING) return true;

			//状态判断
			ASSERT (m_cbPlayStatus[pUserData->wChairID] == TRUE);
			if (m_cbPlayStatus[pUserData->wChairID] != TRUE) 
			{
				return false;
			}

			//消息处理
			bResult=OnUserCallBanker(pUserData->wChairID, pCallBanker->bBanker, pCallBanker->cbBankerTimes);
			break;
		}
	case SUB_C_ADD_SCORE:			//用户加注
		{
			//效验数据
			ASSERT(wDataSize==sizeof(CMD_C_AddScore));
			if (wDataSize!=sizeof(CMD_C_AddScore)) 
			{
				return false;
			}

			//变量定义
			CMD_C_AddScore * pAddScore=(CMD_C_AddScore *)pDataBuffer;

			//用户效验
			tagUserInfo * pUserData=pIServerUserItem->GetUserInfo();
			//if (pUserData->cbUserStatus!=US_PLAYING) return true;

			//状态判断
			ASSERT (m_cbPlayStatus[pUserData->wChairID] == TRUE);
			if (m_cbPlayStatus[pUserData->wChairID] != TRUE) 
			{
				WriteInfo(TEXT("m_cbPlayStatus = FALSE"));
				return false;
			}

			//消息处理
			bResult=OnUserAddScore(pUserData->wChairID,pAddScore->lScore);
			break;
		}
	case SUB_C_OPEN_CARD:			//用户摊牌
		{
			ASSERT(wDataSize==sizeof(CMD_C_OpenCard));
			if(wDataSize!=sizeof(CMD_C_OpenCard)) return false;

			CMD_C_OpenCard *pOpenCard=(CMD_C_OpenCard *)pDataBuffer;

			//用户效验
			tagUserInfo * pUserData=pIServerUserItem->GetUserInfo();
			//if (pUserData->cbUserStatus!=US_PLAYING) return true;

			//状态判断
			ASSERT(m_cbPlayStatus[pUserData->wChairID]!=FALSE);
			if(m_cbPlayStatus[pUserData->wChairID]==FALSE)
			{
				return true;
			}

			//消息处理
			bResult=OnUserOpenCard(pUserData->wChairID, pOpenCard->cbCombineCardData);
			break;
		}	
	case SUB_C_REQUEST_RCRecord:
		{
			ASSERT((m_pGameServiceOption->wServerType & GAME_GENRE_PERSONAL) != 0);
			if (!((m_pGameServiceOption->wServerType & GAME_GENRE_PERSONAL) != 0))
			{
				return false;
			}

			if (m_pITableFrame->IsPersonalRoomDisumme())
			{
				return true;
			}

			ASSERT(pIServerUserItem->IsMobileUser());
			if (!pIServerUserItem->IsMobileUser())
			{
				return false;
			}

			CMD_S_RoomCardRecord RoomCardRecord;
			ZeroMemory(&RoomCardRecord, sizeof(RoomCardRecord));

			CopyMemory(&RoomCardRecord, &m_RoomCardRecord, sizeof(m_RoomCardRecord));

			m_pITableFrame->SendTableData(pIServerUserItem->GetChairID(), SUB_S_ROOMCARD_RECORD, &RoomCardRecord, sizeof(RoomCardRecord));
			m_pITableFrame->SendLookonData(pIServerUserItem->GetChairID(), SUB_S_ROOMCARD_RECORD, &RoomCardRecord, sizeof(RoomCardRecord));

			return true;
		}
#ifdef CARD_CONFIG
	case SUB_C_CARD_CONFIG:
		{
			//效验数据
			ASSERT (wDataSize == sizeof(CMD_C_CardConfig));
			if (wDataSize != sizeof(CMD_C_CardConfig))
			{
				return true;
			}

			//消息处理
			CMD_C_CardConfig *pCardConfig = (CMD_C_CardConfig *)pDataBuffer;

			CopyMemory(m_cbconfigCard, pCardConfig->cbconfigCard, sizeof(m_cbconfigCard));

			return true;
		}
#endif
	}
	
	BYTE cbGameStatus = m_pITableFrame->GetGameStatus();

	//操作定时器
	if(bResult && !IsRoomCardType() && wSubCmdID != SUB_C_CARD_CONFIG)
	{
		m_pITableFrame->SetGameTimer(IDI_SO_OPERATE, TIME_SO_OPERATE, 1, 0);
	}

	if (cbGameStatus == GS_TK_FREE && !IsRoomCardType() && wSubCmdID != SUB_C_CARD_CONFIG)
	{
		m_pITableFrame->KillGameTimer(IDI_SO_OPERATE);
	}

	return true;
}

//框架消息处理
bool CTableFrameSink::OnFrameMessage(WORD wSubCmdID, void * pDataBuffer, WORD wDataSize, IServerUserItem * pIServerUserItem)
{
	// 消息处理
	if (wSubCmdID >= SUB_GF_FRAME_MESSAG_GAME_MIN && wSubCmdID <= SUB_GF_FRAME_MESSAG_GAME_MAX)
	{		
		switch (wSubCmdID - SUB_GF_FRAME_MESSAG_GAME_MIN)
		{
		case SUB_C_STORAGE:
		{
			ASSERT(wDataSize == sizeof(CMD_C_UpdateStorage));
			if (wDataSize != sizeof(CMD_C_UpdateStorage)) return false;

			//权限判断
			if (CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()) == false)
				return false;

			CMD_C_UpdateStorage *pUpdateStorage = (CMD_C_UpdateStorage *)pDataBuffer;
			g_lRoomStorageCurrent = pUpdateStorage->lRoomStorageCurrent;
			g_lStorageDeductRoom = pUpdateStorage->lRoomStorageDeduct;

			//20条操作记录
			if (g_ListOperationRecord.GetSize() == MAX_OPERATION_RECORD)
			{
				g_ListOperationRecord.RemoveHead();
			}

			CString strOperationRecord;
			CTime time = CTime::GetCurrentTime();
			strOperationRecord.Format(TEXT("操作时间: %d/%d/%d-%d:%d:%d, 控制账户[%s],修改当前库存为 %I64d,衰减值为 %I64d"),
				time.GetYear(), time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute(), time.GetSecond(), pIServerUserItem->GetNickName(),
				g_lRoomStorageCurrent, g_lStorageDeductRoom);

			g_ListOperationRecord.AddTail(strOperationRecord);

			//写入日志
			WriteInfo(strOperationRecord);

			//变量定义
			CMD_S_Operation_Record OperationRecord;
			ZeroMemory(&OperationRecord, sizeof(OperationRecord));
			POSITION posListRecord = g_ListOperationRecord.GetHeadPosition();
			WORD wIndex = 0;//数组下标
			while (posListRecord)
			{
				CString strRecord = g_ListOperationRecord.GetNext(posListRecord);

				CopyMemory(OperationRecord.szRecord[wIndex], strRecord, sizeof(OperationRecord.szRecord[wIndex]));
				wIndex++;
			}

			ASSERT(wIndex <= MAX_OPERATION_RECORD);

			//发送数据
			m_pITableFrame->SendRoomData(pIServerUserItem,SUB_S_OPERATION_RECORD, &OperationRecord, sizeof(OperationRecord));			

			return true;
		}
		case SUB_C_STORAGEMAXMUL:
		{
			ASSERT(wDataSize == sizeof(CMD_C_ModifyStorage));
			if (wDataSize != sizeof(CMD_C_ModifyStorage)) return false;

			//权限判断
			if (CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()) == false)
				return false;

			CMD_C_ModifyStorage *pModifyStorage = (CMD_C_ModifyStorage *)pDataBuffer;
			g_lStorageMax1Room = pModifyStorage->lMaxRoomStorage[0];
			g_lStorageMax2Room = pModifyStorage->lMaxRoomStorage[1];
			g_lStorageMul1Room = (SCORE)(pModifyStorage->wRoomStorageMul[0]);
			g_lStorageMul2Room = (SCORE)(pModifyStorage->wRoomStorageMul[1]);

			//20条操作记录
			if (g_ListOperationRecord.GetSize() == MAX_OPERATION_RECORD)
			{
				g_ListOperationRecord.RemoveHead();
			}

			CString strOperationRecord;
			CTime time = CTime::GetCurrentTime();
			strOperationRecord.Format(TEXT("操作时间: %d/%d/%d-%d:%d:%d,控制账户[%s], 修改库存上限值1为 %I64d,赢分概率1为 %I64d,上限值2为 %I64d,赢分概率2为 %I64d"),
				time.GetYear(), time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute(), time.GetSecond(), pIServerUserItem->GetNickName(), g_lStorageMax1Room, g_lStorageMul1Room, g_lStorageMax2Room, g_lStorageMul2Room);

			g_ListOperationRecord.AddTail(strOperationRecord);

			//写入日志
			WriteInfo(strOperationRecord);

			//变量定义
			CMD_S_Operation_Record OperationRecord;
			ZeroMemory(&OperationRecord, sizeof(OperationRecord));
			POSITION posListRecord = g_ListOperationRecord.GetHeadPosition();
			WORD wIndex = 0;//数组下标
			while (posListRecord)
			{
				CString strRecord = g_ListOperationRecord.GetNext(posListRecord);

				CopyMemory(OperationRecord.szRecord[wIndex], strRecord, sizeof(OperationRecord.szRecord[wIndex]));
				wIndex++;
			}

			ASSERT(wIndex <= MAX_OPERATION_RECORD);

			//发送数据
			m_pITableFrame->SendRoomData(pIServerUserItem, SUB_S_OPERATION_RECORD, &OperationRecord, sizeof(OperationRecord));
			
			return true;
		}
		case SUB_C_REQUEST_QUERY_USER:
		{
			ASSERT(wDataSize == sizeof(CMD_C_RequestQuery_User));
			if (wDataSize != sizeof(CMD_C_RequestQuery_User))
			{
				return false;
			}

			//权限判断
			if (CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()) == false || pIServerUserItem->IsAndroidUser())
			{
				return false;
			}

			CMD_C_RequestQuery_User *pQuery_User = (CMD_C_RequestQuery_User *)pDataBuffer;

			//遍历映射
			POSITION ptHead = g_MapRoomUserInfo.GetStartPosition();
			DWORD dwUserID = 0;
			ROOMUSERINFO userinfo;
			ZeroMemory(&userinfo, sizeof(userinfo));

			CMD_S_RequestQueryResult QueryResult;
			ZeroMemory(&QueryResult, sizeof(QueryResult));

			while (ptHead)
			{
				g_MapRoomUserInfo.GetNextAssoc(ptHead, dwUserID, userinfo);
				if (pQuery_User->dwGameID == userinfo.dwGameID || _tcscmp(pQuery_User->szNickName, userinfo.szNickName) == 0)
				{
					//拷贝用户信息数据
					QueryResult.bFind = true;
					CopyMemory(&(QueryResult.userinfo), &userinfo, sizeof(userinfo));

					ZeroMemory(&g_CurrentQueryUserInfo, sizeof(g_CurrentQueryUserInfo));
					CopyMemory(&(g_CurrentQueryUserInfo), &userinfo, sizeof(userinfo));
				}
			}

			//发送数据
			m_pITableFrame->SendRoomData(pIServerUserItem, SUB_S_REQUEST_QUERY_RESULT, &QueryResult, sizeof(QueryResult));

			return true;
		}
		case SUB_C_USER_CONTROL:
		{
			ASSERT(wDataSize == sizeof(CMD_C_UserControl));
			if (wDataSize != sizeof(CMD_C_UserControl))
			{
				return false;
			}

			//权限判断
			if (CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()) == false || pIServerUserItem->IsAndroidUser() == true)
			{
				return false;
			}

			CMD_C_UserControl *pUserControl = (CMD_C_UserControl *)pDataBuffer;

			//遍历映射
			POSITION ptMapHead = g_MapRoomUserInfo.GetStartPosition();
			DWORD dwUserID = 0;
			ROOMUSERINFO userinfo;
			ZeroMemory(&userinfo, sizeof(userinfo));

			//20条操作记录
			if (g_ListOperationRecord.GetSize() == MAX_OPERATION_RECORD)
			{
				g_ListOperationRecord.RemoveHead();
			}

			//变量定义
			CMD_S_UserControl serverUserControl;
			ZeroMemory(&serverUserControl, sizeof(serverUserControl));

			TCHAR szNickName[LEN_NICKNAME];
			ZeroMemory(szNickName, sizeof(szNickName));

			//激活控制
			if (pUserControl->userControlInfo.bCancelControl == false)
			{
				ASSERT(pUserControl->userControlInfo.control_type == CONTINUE_WIN || pUserControl->userControlInfo.control_type == CONTINUE_LOST);

				while (ptMapHead)
				{
					g_MapRoomUserInfo.GetNextAssoc(ptMapHead, dwUserID, userinfo);

					if (_tcscmp(pUserControl->szNickName, szNickName) == 0 && _tcscmp(userinfo.szNickName, szNickName) == 0)
					{
						continue;
					}

					if (pUserControl->dwGameID == userinfo.dwGameID || _tcscmp(pUserControl->szNickName, userinfo.szNickName) == 0)
					{
						//激活控制标识
						bool bEnableControl = false;
						IsSatisfyControl(userinfo, bEnableControl);

						//满足控制
						if (bEnableControl)
						{
							ROOMUSERCONTROL roomusercontrol;
							ZeroMemory(&roomusercontrol, sizeof(roomusercontrol));
							CopyMemory(&(roomusercontrol.roomUserInfo), &userinfo, sizeof(userinfo));
							CopyMemory(&(roomusercontrol.userControl), &(pUserControl->userControlInfo), sizeof(roomusercontrol.userControl));


							//遍历链表，除重
							TravelControlList(roomusercontrol);

							//压入链表（不压入同GAMEID和NICKNAME)
							g_ListRoomUserControl.AddHead(roomusercontrol);

							//复制数据
							serverUserControl.dwGameID = userinfo.dwGameID;
							CopyMemory(serverUserControl.szNickName, userinfo.szNickName, sizeof(userinfo.szNickName));
							serverUserControl.controlResult = CONTROL_SUCCEED;
							serverUserControl.controlType = pUserControl->userControlInfo.control_type;
							serverUserControl.cbControlCount = pUserControl->userControlInfo.cbControlCount;

							//操作记录
							CString strOperationRecord;
							CString strControlType;
							GetControlTypeString(serverUserControl.controlType, strControlType);
							CTime time = CTime::GetCurrentTime();
							strOperationRecord.Format(TEXT("操作时间: %d/%d/%d-%d:%d:%d, 控制账户[%s], 控制玩家%s,%s,控制局数%d "),
								time.GetYear(), time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute(), time.GetSecond(), pIServerUserItem->GetNickName(), serverUserControl.szNickName, strControlType, serverUserControl.cbControlCount);

							g_ListOperationRecord.AddTail(strOperationRecord);

							//写入日志
							WriteInfo(strOperationRecord);
						}
						else	//不满足
						{
							//复制数据
							serverUserControl.dwGameID = userinfo.dwGameID;
							CopyMemory(serverUserControl.szNickName, userinfo.szNickName, sizeof(userinfo.szNickName));
							serverUserControl.controlResult = CONTROL_FAIL;
							serverUserControl.controlType = pUserControl->userControlInfo.control_type;
							serverUserControl.cbControlCount = 0;

							//操作记录
							CString strOperationRecord;
							CString strControlType;
							GetControlTypeString(serverUserControl.controlType, strControlType);
							CTime time = CTime::GetCurrentTime();
							strOperationRecord.Format(TEXT("操作时间: %d/%d/%d-%d:%d:%d, 控制账户[%s], 控制玩家%s,%s,失败！"),
								time.GetYear(), time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute(), time.GetSecond(), pIServerUserItem->GetNickName(), serverUserControl.szNickName, strControlType);

							g_ListOperationRecord.AddTail(strOperationRecord);

							//写入日志
							WriteInfo(strOperationRecord);
						}

						//发送数据
						m_pITableFrame->SendRoomData(pIServerUserItem, SUB_S_USER_CONTROL, &serverUserControl, sizeof(serverUserControl));

						CMD_S_Operation_Record OperationRecord;
						ZeroMemory(&OperationRecord, sizeof(OperationRecord));
						POSITION posListRecord = g_ListOperationRecord.GetHeadPosition();
						WORD wIndex = 0;//数组下标
						while (posListRecord)
						{
							CString strRecord = g_ListOperationRecord.GetNext(posListRecord);

							CopyMemory(OperationRecord.szRecord[wIndex], strRecord, sizeof(OperationRecord.szRecord[wIndex]));
							wIndex++;
						}

						ASSERT(wIndex <= MAX_OPERATION_RECORD);

						//发送数据
						m_pITableFrame->SendRoomData(pIServerUserItem, SUB_S_OPERATION_RECORD, &OperationRecord, sizeof(OperationRecord));
						return true;
					}
				}

				ASSERT(FALSE);
				return false;
			}
			else	//取消控制
			{
				ASSERT(pUserControl->userControlInfo.control_type == CONTINUE_CANCEL);

				POSITION ptListHead = g_ListRoomUserControl.GetHeadPosition();
				POSITION ptTemp;
				ROOMUSERCONTROL roomusercontrol;
				ZeroMemory(&roomusercontrol, sizeof(roomusercontrol));

				//遍历链表
				while (ptListHead)
				{
					ptTemp = ptListHead;
					roomusercontrol = g_ListRoomUserControl.GetNext(ptListHead);
					if (pUserControl->dwGameID == roomusercontrol.roomUserInfo.dwGameID || _tcscmp(pUserControl->szNickName, roomusercontrol.roomUserInfo.szNickName) == 0)
					{
						//复制数据
						serverUserControl.dwGameID = roomusercontrol.roomUserInfo.dwGameID;
						CopyMemory(serverUserControl.szNickName, roomusercontrol.roomUserInfo.szNickName, sizeof(roomusercontrol.roomUserInfo.szNickName));
						serverUserControl.controlResult = CONTROL_CANCEL_SUCCEED;
						serverUserControl.controlType = pUserControl->userControlInfo.control_type;
						serverUserControl.cbControlCount = 0;

						//移除元素
						g_ListRoomUserControl.RemoveAt(ptTemp);

						//发送数据
						m_pITableFrame->SendRoomData(pIServerUserItem, SUB_S_USER_CONTROL, &serverUserControl, sizeof(serverUserControl));

						//操作记录
						CString strOperationRecord;
						CTime time = CTime::GetCurrentTime();
						strOperationRecord.Format(TEXT("操作时间: %d/%d/%d-%d:%d:%d, 控制账户[%s], 取消对玩家%s的控制！"),
							time.GetYear(), time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute(), time.GetSecond(), pIServerUserItem->GetNickName(), serverUserControl.szNickName);

						g_ListOperationRecord.AddTail(strOperationRecord);

						//写入日志
						WriteInfo(strOperationRecord);

						CMD_S_Operation_Record OperationRecord;
						ZeroMemory(&OperationRecord, sizeof(OperationRecord));
						POSITION posListRecord = g_ListOperationRecord.GetHeadPosition();
						WORD wIndex = 0;//数组下标
						while (posListRecord)
						{
							CString strRecord = g_ListOperationRecord.GetNext(posListRecord);

							CopyMemory(OperationRecord.szRecord[wIndex], strRecord, sizeof(OperationRecord.szRecord[wIndex]));
							wIndex++;
						}

						ASSERT(wIndex <= MAX_OPERATION_RECORD);

						//发送数据
						m_pITableFrame->SendRoomData(pIServerUserItem, SUB_S_OPERATION_RECORD, &OperationRecord, sizeof(OperationRecord));

						return true;
					}
				}

				//复制数据
				serverUserControl.dwGameID = pUserControl->dwGameID;
				CopyMemory(serverUserControl.szNickName, pUserControl->szNickName, sizeof(serverUserControl.szNickName));
				serverUserControl.controlResult = CONTROL_CANCEL_INVALID;
				serverUserControl.controlType = pUserControl->userControlInfo.control_type;
				serverUserControl.cbControlCount = 0;

				//发送数据
				m_pITableFrame->SendRoomData(pIServerUserItem, SUB_S_USER_CONTROL, &serverUserControl, sizeof(serverUserControl));

				//操作记录
				CString strOperationRecord;
				CTime time = CTime::GetCurrentTime();
				strOperationRecord.Format(TEXT("操作时间: %d/%d/%d-%d:%d:%d, 控制账户[%s], 取消对玩家%s的控制，操作无效！"),
					time.GetYear(), time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute(), time.GetSecond(), pIServerUserItem->GetNickName(), serverUserControl.szNickName);

				g_ListOperationRecord.AddTail(strOperationRecord);

				//写入日志
				WriteInfo(strOperationRecord);

				CMD_S_Operation_Record OperationRecord;
				ZeroMemory(&OperationRecord, sizeof(OperationRecord));
				POSITION posListRecord = g_ListOperationRecord.GetHeadPosition();
				WORD wIndex = 0;//数组下标
				while (posListRecord)
				{
					CString strRecord = g_ListOperationRecord.GetNext(posListRecord);

					CopyMemory(OperationRecord.szRecord[wIndex], strRecord, sizeof(OperationRecord.szRecord[wIndex]));
					wIndex++;
				}

				ASSERT(wIndex <= MAX_OPERATION_RECORD);

				//发送数据
				m_pITableFrame->SendRoomData(pIServerUserItem, SUB_S_OPERATION_RECORD, &OperationRecord, sizeof(OperationRecord));

			}

			return true;
		}
		case SUB_C_REQUEST_UDPATE_ROOMINFO:
		{
			//权限判断
			if (CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()) == false || pIServerUserItem->IsAndroidUser() == true)
			{
				return false;
			}

			CMD_S_RequestUpdateRoomInfo_Result RoomInfo_Result;
			ZeroMemory(&RoomInfo_Result, sizeof(RoomInfo_Result));

			RoomInfo_Result.lRoomStorageCurrent = g_lRoomStorageCurrent;


			DWORD dwKeyGameID = g_CurrentQueryUserInfo.dwGameID;
			TCHAR szKeyNickName[LEN_NICKNAME];
			ZeroMemory(szKeyNickName, sizeof(szKeyNickName));
			CopyMemory(szKeyNickName, g_CurrentQueryUserInfo.szNickName, sizeof(szKeyNickName));

			//遍历映射
			POSITION ptHead = g_MapRoomUserInfo.GetStartPosition();
			DWORD dwUserID = 0;
			ROOMUSERINFO userinfo;
			ZeroMemory(&userinfo, sizeof(userinfo));

			while (ptHead)
			{
				g_MapRoomUserInfo.GetNextAssoc(ptHead, dwUserID, userinfo);
				if (dwKeyGameID == userinfo.dwGameID && _tcscmp(szKeyNickName, userinfo.szNickName) == 0)
				{
					//拷贝用户信息数据
					CopyMemory(&(RoomInfo_Result.currentqueryuserinfo), &userinfo, sizeof(userinfo));

					ZeroMemory(&g_CurrentQueryUserInfo, sizeof(g_CurrentQueryUserInfo));
					CopyMemory(&(g_CurrentQueryUserInfo), &userinfo, sizeof(userinfo));
				}
			}


			//
			//变量定义
			POSITION ptListHead = g_ListRoomUserControl.GetHeadPosition();
			POSITION ptTemp;
			ROOMUSERCONTROL roomusercontrol;
			ZeroMemory(&roomusercontrol, sizeof(roomusercontrol));

			//遍历链表
			while (ptListHead)
			{
				ptTemp = ptListHead;
				roomusercontrol = g_ListRoomUserControl.GetNext(ptListHead);

				//寻找玩家
				if ((dwKeyGameID == roomusercontrol.roomUserInfo.dwGameID) &&
					_tcscmp(szKeyNickName, roomusercontrol.roomUserInfo.szNickName) == 0)
				{
					RoomInfo_Result.bExistControl = true;
					CopyMemory(&(RoomInfo_Result.currentusercontrol), &(roomusercontrol.userControl), sizeof(roomusercontrol.userControl));
					break;
				}
			}

			//发送数据
			m_pITableFrame->SendRoomData(pIServerUserItem, SUB_S_REQUEST_UDPATE_ROOMINFO_RESULT, &RoomInfo_Result, sizeof(RoomInfo_Result));

			CMD_S_ADMIN_STORAGE_INFO StorageInfo;
			ZeroMemory(&StorageInfo, sizeof(StorageInfo));
			StorageInfo.lRoomStorageStart = g_lRoomStorageStart;
			StorageInfo.lRoomStorageCurrent = g_lRoomStorageCurrent;
			StorageInfo.lRoomStorageDeduct = g_lStorageDeductRoom;
			StorageInfo.lMaxRoomStorage[0] = g_lStorageMax1Room;
			StorageInfo.lMaxRoomStorage[1] = g_lStorageMax2Room;
			StorageInfo.wRoomStorageMul[0] = (WORD)g_lStorageMul1Room;
			StorageInfo.wRoomStorageMul[1] = (WORD)g_lStorageMul2Room;
			m_pITableFrame->SendRoomData(NULL, SUB_S_ADMIN_STORAGE_INFO, &StorageInfo, sizeof(StorageInfo));

			return true;
		}
		case SUB_C_CLEAR_CURRENT_QUERYUSER:
		{
			//权限判断
			if (CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()) == false || pIServerUserItem->IsAndroidUser() == true)
			{
				return false;
			}

			ZeroMemory(&g_CurrentQueryUserInfo, sizeof(g_CurrentQueryUserInfo));

			return true;
		}
		}
	}
	return false;
}

//叫庄事件
bool CTableFrameSink::OnUserCallBanker(WORD wChairID, bool bBanker, BYTE cbBankerTimes)
{
	//状态效验
	BYTE cbGameStatus = m_pITableFrame->GetGameStatus();
	ASSERT(cbGameStatus==GS_TK_CALL);
	if (cbGameStatus!=GS_TK_CALL) return true;

	//设置变量
	m_cbCallBankerStatus[wChairID]=TRUE;
	m_cbCallBankerTimes[wChairID] = cbBankerTimes;

	//设置变量
	CMD_S_CallBankerInfo CallBanker;
	ZeroMemory(&CallBanker, sizeof(CallBanker));

	CopyMemory(CallBanker.cbCallBankerStatus, m_cbCallBankerStatus, sizeof(m_cbCallBankerStatus));
	CopyMemory(CallBanker.cbCallBankerTimes, m_cbCallBankerTimes, sizeof(m_cbCallBankerTimes));

	//发送数据
	for (WORD i=0;i<m_wPlayerCount;i++)
	{
		if(m_cbPlayStatus[i]==FALSE&&m_cbDynamicJoin[i]==FALSE)continue;
		m_pITableFrame->SendTableData(i,SUB_S_CALL_BANKERINFO,&CallBanker,sizeof(CallBanker));
	}
	m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_CALL_BANKERINFO,&CallBanker,sizeof(CallBanker));

	if (m_pGameVideo)
	{
		m_pGameVideo->AddVideoData(SUB_S_CALL_BANKERINFO, &CallBanker);
	}

	//叫庄人数
	WORD wCallUserCount=0;
	for (WORD i=0;i<m_wPlayerCount;i++)
	{
		if(m_cbPlayStatus[i]==TRUE && m_cbCallBankerStatus[i]==TRUE) wCallUserCount++;
		else if(m_cbPlayStatus[i]!=TRUE) wCallUserCount++;
	}

	//全部人叫完庄，下注开始
	if(wCallUserCount==m_wPlayerCount)
	{
		//叫庄最大倍数
		BYTE cbMaxBankerTimes = cbBankerTimes;
		for (WORD i=0;i<m_wPlayerCount;i++)
		{
			if(m_cbPlayStatus[i]==TRUE && m_cbCallBankerStatus[i]==TRUE && m_cbCallBankerTimes[i] > cbMaxBankerTimes) 
			{
				cbMaxBankerTimes = m_cbCallBankerTimes[i];
			}
		}

		//叫庄最大倍数的人数和CHAIRID
		BYTE cbMaxBankerCount = 0;
		WORD *pwMaxBankerTimesChairID = new WORD[m_wPlayerCount];
		for (WORD i=0;i<m_wPlayerCount;i++)
		{
			if(m_cbPlayStatus[i]==TRUE && m_cbCallBankerStatus[i]==TRUE && m_cbCallBankerTimes[i] == cbMaxBankerTimes) 
			{
				pwMaxBankerTimesChairID[cbMaxBankerCount++] = i;
			}
		}

		ASSERT (cbMaxBankerCount <= m_wPlayerCount);
		
		m_wBankerUser = pwMaxBankerTimesChairID[rand() % cbMaxBankerCount];
		m_bBuckleServiceCharge[m_wBankerUser]=true;

		delete[] pwMaxBankerTimesChairID;

		//设置状态
		m_pITableFrame->SetGameStatus(GS_TK_SCORE);

		//更新房间用户信息
		for (WORD i=0; i<m_wPlayerCount; i++)
		{
			//获取用户
			IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
			if (pIServerUserItem != NULL)
			{
				UpdateRoomUserInfo(pIServerUserItem, USER_SITDOWN);
			}
		}

		//获取最大下注
		for (WORD i=0; i<m_wPlayerCount; i++)
		{
			if (m_cbPlayStatus[i] != TRUE || i == m_wBankerUser)
			{
				continue;
			}

			//下注变量
			m_lTurnMaxScore[i] = GetUserMaxTurnScore(i);
		}

		//设置变量
		CMD_S_GameStart GameStart;
		ZeroMemory(&GameStart, sizeof(GameStart));
		GameStart.wBankerUser=m_wBankerUser;
		CopyMemory(GameStart.cbPlayerStatus, m_cbPlayStatus, sizeof(m_cbPlayStatus));

		//发四等五
		if (m_stConfig == ST_SENDFOUR_)
		{
			for (WORD i=0;i<m_wPlayerCount;i++)
			{
				if (m_cbPlayStatus[i]==FALSE && m_cbDynamicJoin[i]==FALSE)
				{
					continue;
				}

				//派发扑克(开始只发四张牌)
				CopyMemory(GameStart.cbCardData[i], m_cbHandCardData[i], sizeof(BYTE) * 4);
			}
		}

		GameStart.stConfig = m_stConfig;
		GameStart.bgtConfig = m_bgtConfig;
		GameStart.btConfig = m_btConfig;
		CopyMemory(GameStart.lFreeConfig, m_lFreeConfig, sizeof(GameStart.lFreeConfig));
		CopyMemory(GameStart.lPercentConfig, m_lPercentConfig, sizeof(GameStart.lPercentConfig));

		bool bFirstRecord = true;

		WORD wRealPlayerCount = 0;
		for (WORD i = 0; i < m_wPlayerCount; i++)
		{
			IServerUserItem *pServerUserItem = m_pITableFrame->GetTableUserItem(i);
			if (m_cbPlayStatus[i] == FALSE && m_cbDynamicJoin[i] == FALSE)
			{
				continue;
			}

			if (!pServerUserItem)
			{
				continue;
			}

			wRealPlayerCount++;
		}

		BYTE *pGameRule = m_pITableFrame->GetGameRule();

		//最大下注
		for (WORD i=0;i<m_wPlayerCount;i++)
		{
			IServerUserItem *pServerUserItem = m_pITableFrame->GetTableUserItem(i);
			if (m_cbPlayStatus[i]==FALSE && m_cbDynamicJoin[i]==FALSE)
			{
				continue;
			}
			GameStart.lTurnMaxScore=m_lTurnMaxScore[i];
			m_pITableFrame->SendTableData(i,SUB_S_GAME_START,&GameStart,sizeof(GameStart));

			if (m_pGameVideo)
			{
				Video_GameStart video;
				ZeroMemory(&video, sizeof(video));
				video.wPlayerCount = wRealPlayerCount;
				video.wGamePlayerCountRule = pGameRule[1];
				video.wBankerUser = GameStart.wBankerUser;
				CopyMemory(video.cbPlayerStatus, GameStart.cbPlayerStatus, sizeof(video.cbPlayerStatus));
				video.lTurnMaxScore = GameStart.lTurnMaxScore;
				CopyMemory(video.cbCardData, GameStart.cbCardData, sizeof(video.cbCardData));
				video.ctConfig = m_ctConfig;
				video.stConfig = GameStart.stConfig;
				video.bgtConfig = GameStart.bgtConfig;
				video.btConfig = GameStart.btConfig;
				CopyMemory(video.lFreeConfig, GameStart.lFreeConfig, sizeof(video.lFreeConfig));
				CopyMemory(video.lPercentConfig, GameStart.lPercentConfig, sizeof(video.lPercentConfig));
				CopyMemory(video.szNickName, pServerUserItem->GetNickName(), sizeof(video.szNickName));
				video.wChairID = i;
				video.lScore = pServerUserItem->GetUserScore();

				m_pGameVideo->AddVideoData(SUB_S_GAME_START, &video, bFirstRecord);

				if (bFirstRecord == true)
				{
					bFirstRecord = false;
				}
			}
		}
		m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GAME_START,&GameStart,sizeof(GameStart));
	}

	return true;
}

//加注事件
bool CTableFrameSink::OnUserAddScore(WORD wChairID, LONGLONG lScore)
{
	//状态效验
	BYTE cbGameStatus = m_pITableFrame->GetGameStatus();
	ASSERT(cbGameStatus==GS_TK_SCORE);
	if (cbGameStatus!=GS_TK_SCORE) 
	{
		WriteInfo(TEXT("cbGameStatus = FALSE"));
		return true;
	}
	
	//庄家校验
	if (wChairID == m_wBankerUser)
	{
		WriteInfo(TEXT("wChairID == m_wBankerUser = FALSE"));
		return false;
	}

	//金币效验
	if(m_cbPlayStatus[wChairID]==TRUE)
	{
		ASSERT (lScore>0);
		if (lScore<=0)
		{
			WriteInfo(TEXT("lScore < 0"));
			return false;
		}

		if (lScore > m_lTurnMaxScore[wChairID])
		{
			lScore = m_lTurnMaxScore[wChairID];
		}
	}
	else //没下注玩家强退
	{
		ASSERT(lScore==0);
		if (lScore!=0) 
		{
			WriteInfo(TEXT("lScore!=0"));
			return false;
		}
	}

	if(lScore>0L)
	{
		//下注金币
		m_lTableScore[wChairID]=lScore;
		m_bBuckleServiceCharge[wChairID]=true;
		//构造数据
		CMD_S_AddScore AddScore;
		AddScore.wAddScoreUser=wChairID;
		AddScore.lAddScoreCount=m_lTableScore[wChairID];

		//发送数据
		for (WORD i=0;i<m_wPlayerCount;i++)
		{
			if(m_cbPlayStatus[i]==FALSE&&m_cbDynamicJoin[i]==FALSE)continue;
			m_pITableFrame->SendTableData(i,SUB_S_ADD_SCORE,&AddScore,sizeof(AddScore));
		}
		m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_ADD_SCORE,&AddScore,sizeof(AddScore));

		if (m_pGameVideo)
		{
			m_pGameVideo->AddVideoData(SUB_S_ADD_SCORE, &AddScore);
		}
	}

	//下注人数
	BYTE bUserCount=0;
	for(WORD i=0;i<m_wPlayerCount;i++)
	{
		if(m_lTableScore[i]>0L && m_cbPlayStatus[i]==TRUE)
		{
			bUserCount++;
		}
		else if(m_cbPlayStatus[i]==FALSE || i==m_wBankerUser)
		{
			bUserCount++;
		}
	}
	
	CString strdebug;
	strdebug.Format(TEXT("bUserCount = %d, m_wPlayerCount = %d"), bUserCount, m_wPlayerCount);
	WriteInfo(strdebug);

	//闲家全到
	if(bUserCount==m_wPlayerCount)
	{
		//设置状态
		m_pITableFrame->SetGameStatus(GS_TK_PLAYING);

		//更新房间用户信息
		for (WORD i=0; i<m_wPlayerCount; i++)
		{
			//获取用户
			IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
			if (pIServerUserItem != NULL)
			{
				UpdateRoomUserInfo(pIServerUserItem, USER_SITDOWN);
			}
		}

		//构造数据
		CMD_S_SendCard SendCard;
		ZeroMemory(SendCard.cbCardData,sizeof(SendCard.cbCardData));

		//分析牌数据，下注发牌和发四等五区分处理
		bool AnalyseCardValid = true;
		for (WORD i = 0; i<m_wPlayerCount; i++)
		{
			if ((m_cbPlayStatus[i] == TRUE && m_cbHandCardData[i][0] == 0 && m_cbHandCardData[i][1] == 0 && m_cbHandCardData[i][2] == 0))
			{
				WriteInfo(L"", TEXT("AnalyseCardbeforewrong"));
				AnalyseCardValid = false;

				break;
			}
		}

		AnalyseCard(m_stConfig);

		for (WORD i = 0; i<m_wPlayerCount; i++)
		{
			if (m_cbPlayStatus[i] == TRUE && m_cbHandCardData[i][0] == 0 && m_cbHandCardData[i][1] == 0 && m_cbHandCardData[i][2] == 0)
			{
				WriteInfo(L"", TEXT("AnalyseCardwrong"));
				AnalyseCardValid = false;

				break;
			}
		}

		//当牌数据无效，可能都是【0】【0】【0】【0】【0】
		if (AnalyseCardValid == false)
		{
			//随机扑克
			BYTE bTempArray[GAME_PLAYER*MAX_CARDCOUNT];
			m_GameLogic.RandCardList(bTempArray, sizeof(bTempArray), (m_gtConfig == GT_HAVEKING_ ? true : false));

			for (WORD i = 0; i<m_wPlayerCount; i++)
			{
				if (m_cbPlayStatus[i] == TRUE)
				{
					//派发扑克
					CopyMemory(m_cbHandCardData[i], &bTempArray[i*MAX_CARDCOUNT], MAX_CARDCOUNT);
				}
			}
		}

		///////////////////////////////////

		//变量定义
		ROOMUSERCONTROL roomusercontrol;
		ZeroMemory(&roomusercontrol, sizeof(roomusercontrol));
		POSITION posKeyList;

		//控制 (下注发牌和发四等五区分控制)
		if (m_pServerControl != NULL && AnalyseRoomUserControl(roomusercontrol, posKeyList))
		{
			//校验数据
			ASSERT(roomusercontrol.roomUserInfo.wChairID != INVALID_CHAIR && roomusercontrol.userControl.cbControlCount != 0
				&& roomusercontrol.userControl.control_type != CONTINUE_CANCEL);

			if (m_pServerControl->ControlResult(m_cbHandCardData, roomusercontrol, m_stConfig, m_ctConfig, m_gtConfig))
			{
				//获取元素
				ROOMUSERCONTROL &tmproomusercontrol = g_ListRoomUserControl.GetAt(posKeyList);

				//校验数据
				ASSERT(roomusercontrol.userControl.cbControlCount == tmproomusercontrol.userControl.cbControlCount);

				//控制局数
				tmproomusercontrol.userControl.cbControlCount--;

				CMD_S_UserControlComplete UserControlComplete;
				ZeroMemory(&UserControlComplete, sizeof(UserControlComplete));
				UserControlComplete.dwGameID = roomusercontrol.roomUserInfo.dwGameID;
				CopyMemory(UserControlComplete.szNickName, roomusercontrol.roomUserInfo.szNickName, sizeof(UserControlComplete.szNickName));
				UserControlComplete.controlType = roomusercontrol.userControl.control_type;
				UserControlComplete.cbRemainControlCount = tmproomusercontrol.userControl.cbControlCount;

				for (WORD i = 0; i<m_wPlayerCount; i++)
				{
					IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
					if (!pIServerUserItem)
					{
						continue;
					}
					if (pIServerUserItem->IsAndroidUser() == true || CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()) == false)
					{
						continue;
					}

					//发送数据
					m_pITableFrame->SendTableData(i, SUB_S_USER_CONTROL_COMPLETE, &UserControlComplete, sizeof(UserControlComplete));
					m_pITableFrame->SendLookonData(i, SUB_S_USER_CONTROL_COMPLETE, &UserControlComplete, sizeof(UserControlComplete));

				}
			}
		}

		bool bControlCardValid = true;
		for (WORD i = 0; i<m_wPlayerCount; i++)
		{
			if ((m_cbPlayStatus[i] == TRUE && m_cbHandCardData[i][0] == 0 && m_cbHandCardData[i][1] == 0 && m_cbHandCardData[i][2] == 0))
			{
				WriteInfo(L"", TEXT("ControlCardwrong"));
				bControlCardValid = false;

				break;
			}
		}

		//当牌数据无效，可能都是【0】【0】【0】【0】【0】
		if (bControlCardValid == false)
		{
			//随机扑克
			BYTE bTempArray[GAME_PLAYER*MAX_CARDCOUNT];
			m_GameLogic.RandCardList(bTempArray, sizeof(bTempArray), (m_gtConfig == GT_HAVEKING_ ? true : false));

			for (WORD i = 0; i<m_wPlayerCount; i++)
			{
				if (m_cbPlayStatus[i] == TRUE)
				{
					//派发扑克
					CopyMemory(m_cbHandCardData[i], &bTempArray[i*MAX_CARDCOUNT], MAX_CARDCOUNT);
				}
			}
		}

		//临时扑克,因为分析和控制扑克，重算原始牌型
		BYTE cbTempHandCardData[GAME_PLAYER][MAX_CARDCOUNT];
		ZeroMemory(cbTempHandCardData, sizeof(cbTempHandCardData));
		CopyMemory(cbTempHandCardData, m_cbHandCardData, sizeof(m_cbHandCardData));

		for (WORD i=0;i<m_wPlayerCount;i++)
		{
			IServerUserItem *pIServerUser=m_pITableFrame->GetTableUserItem(i);	
			if(pIServerUser==NULL)
			{
				continue;
			}

			m_bSpecialCard[i] = (m_GameLogic.GetCardType(cbTempHandCardData[i], MAX_CARDCOUNT, m_ctConfig) > CT_CLASSIC_OX_VALUENIUNIU ? true : false);

			//特殊牌型
			if (m_bSpecialCard[i])
			{
				m_cbOriginalCardType[i] = m_GameLogic.GetCardType(cbTempHandCardData[i], MAX_CARDCOUNT, m_ctConfig);
			}
			else
			{
				//获取牛牛牌型
				m_GameLogic.GetOxCard(cbTempHandCardData[i], MAX_CARDCOUNT);

				m_cbOriginalCardType[i] = m_GameLogic.GetCardType(cbTempHandCardData[i], MAX_CARDCOUNT, m_ctConfig);
			}
		}

		CopyMemory(SendCard.cbCardData, m_cbHandCardData, sizeof(SendCard.cbCardData));
		CopyMemory(SendCard.bSpecialCard, m_bSpecialCard, sizeof(SendCard.bSpecialCard));
		CopyMemory(SendCard.cbOriginalCardType, m_cbOriginalCardType, sizeof(SendCard.cbOriginalCardType));

		//发送数据
		for (WORD i=0;i< m_wPlayerCount;i++)
		{
			if(m_cbPlayStatus[i]==FALSE&&m_cbDynamicJoin[i]==FALSE)
			{
				continue;
			}

			m_pITableFrame->SendTableData(i, SUB_S_SEND_CARD, &SendCard, sizeof(SendCard));
		}
		m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_SEND_CARD, &SendCard,sizeof(SendCard));

		if (m_pGameVideo)
		{
			m_pGameVideo->AddVideoData(SUB_S_SEND_CARD, &SendCard);
		}

		// CString strUserCard;
		// for (WORD i = 0; i<m_wPlayerCount; i++)
		// {
		// 	if (m_cbPlayStatus[i] == FALSE)
		// 	{
		// 		continue;
		// 	}

		// 	IServerUserItem *pIServer = m_pITableFrame->GetTableUserItem(i);
		// 	if (!pIServer)
		// 	{
		// 		continue;
		// 	}

		// 	strUserCard.Format(TEXT("USERID[%d],牌数据分别为[%d][%d][%d][%d][%d],牌型为[%d]"),
		// 		pIServer->GetUserID(), m_cbHandCardData[i][0], m_cbHandCardData[i][1], m_cbHandCardData[i][2],
		// 		m_cbHandCardData[i][3], m_cbHandCardData[i][4], m_cbOriginalCardType[i]);

		// 	WriteInfo(strUserCard);
		// }
	}

	return true;
}

//摊牌事件
bool CTableFrameSink::OnUserOpenCard(WORD wChairID, BYTE cbCombineCardData[MAX_CARDCOUNT])
{
	//状态效验
	BYTE cbGameStatus = m_pITableFrame->GetGameStatus();
	ASSERT (cbGameStatus==GS_TK_PLAYING);
	if (cbGameStatus!=GS_TK_PLAYING) return true;
	if (m_bOpenCard[wChairID] != false) return true;

	//校验客户端发过来的扑克
	typedef std::list<BYTE> LISTBYTE;
	LISTBYTE listClient;
	for (BYTE i = 0; i<MAX_CARDCOUNT; i++)
	{
		listClient.push_back(cbCombineCardData[i]);
		listClient.push_back(m_cbHandCardData[wChairID][i]);
	}

	//去重
	listClient.sort();
	listClient.unique();
	if (listClient.size() != MAX_CARDCOUNT)
	{
		return false;
	}

	//摊牌标志
	m_bOpenCard[wChairID] = true;
	
	//玩家重新组合牌标志
	//bool bUserCombine = false;
	//for (WORD i=0; i<MAX_CARDCOUNT; i++)
	//{
	//	if (m_cbHandCardData[wChairID][i] != cbCombineCardData[i])
	//	{
	//		bUserCombine = true;
	//		break;
	//	}
	//}
	//
	////没有重组过牌
	//if (!bUserCombine)
	//{
	//	m_cbCombineCardType[wChairID] = m_cbOriginalCardType[wChairID];
	//}
	//else
	{
		//特殊牌型 赋值初始牌型
		if (m_bSpecialCard[wChairID])
		{
			m_cbCombineCardType[wChairID] = m_cbOriginalCardType[wChairID];
		}
		else
		{
			//前面三张王牌张数
			BYTE cbFirstKingCount = m_GameLogic.GetKingCount(cbCombineCardData, 3);
			BYTE cbSecondKingCount = m_GameLogic.GetKingCount(&cbCombineCardData[3], 2);

			if (cbFirstKingCount == 0)
			{
				//前面三张逻辑值
				BYTE cbFirstNNLogicValue = 0;
				for (WORD i=0; i<3; i++)
				{
					cbFirstNNLogicValue += m_GameLogic.GetNNCardLogicValue(cbCombineCardData[i]);
				}

				if (cbFirstNNLogicValue % 10 != 0)
				{
					m_cbCombineCardType[wChairID] = CT_CLASSIC_OX_VALUE0;
				}
				else
				{
					if (cbSecondKingCount != 0)
					{
						m_cbCombineCardType[wChairID] = CT_CLASSIC_OX_VALUENIUNIU;
					}
					else
					{
						BYTE cbSecondNNLogicValue = 0;
						for (WORD i=3; i<5; i++)
						{
							cbSecondNNLogicValue += m_GameLogic.GetNNCardLogicValue(cbCombineCardData[i]);
						}	

						m_cbCombineCardType[wChairID] = ((cbSecondNNLogicValue % 10 == 0) ? CT_CLASSIC_OX_VALUENIUNIU : (cbSecondNNLogicValue % 10));
					}
				}
			}
			else
			{
				if (cbSecondKingCount != 0)
				{
					m_cbCombineCardType[wChairID] = CT_CLASSIC_OX_VALUENIUNIU;
				}
				else
				{
					BYTE cbSecondNNLogicValue = 0;
					for (WORD i=3; i<5; i++)
					{
						cbSecondNNLogicValue += m_GameLogic.GetNNCardLogicValue(cbCombineCardData[i]);
					}	

					m_cbCombineCardType[wChairID] = ((cbSecondNNLogicValue % 10 == 0) ? CT_CLASSIC_OX_VALUENIUNIU : (cbSecondNNLogicValue % 10));
				}
			}

			//重置组合过的扑克
			CopyMemory(m_cbHandCardData[wChairID], cbCombineCardData, sizeof(m_cbHandCardData[wChairID]));
		}		
	}

	//摊牌人数
	BYTE bUserCount=0;
	for(WORD i=0;i<m_wPlayerCount;i++)
	{
		if(m_bOpenCard[i] == true && m_cbPlayStatus[i]==TRUE)bUserCount++;
		else if(m_cbPlayStatus[i]==FALSE)bUserCount++;
	}

	 //构造变量
	CMD_S_Open_Card OpenCard;
	ZeroMemory(&OpenCard,sizeof(OpenCard));

	//设置变量
	OpenCard.bOpenCard = true;
	OpenCard.wOpenChairID=wChairID;

	//发送数据
	for (WORD i=0;i< m_wPlayerCount;i++)
	{
		if(m_cbPlayStatus[i]==FALSE&&m_cbDynamicJoin[i]==FALSE)continue;
		m_pITableFrame->SendTableData(i,SUB_S_OPEN_CARD,&OpenCard,sizeof(OpenCard));
	}
	m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_OPEN_CARD,&OpenCard,sizeof(OpenCard));	

	if (m_pGameVideo)
	{
		m_pGameVideo->AddVideoData(SUB_S_OPEN_CARD, &OpenCard);
	}

	//结束游戏
	if(bUserCount == m_wPlayerCount)
	{
		return OnEventGameConclude(INVALID_CHAIR,NULL,GER_NORMAL);
	}

	return true;
}

//扑克分析
void CTableFrameSink::AnalyseCard(SENDCARDTYPE_CONFIG stConfig)
{
	//机器人数
	bool bIsAiBanker = false;
	WORD wAiCount = 0;
	WORD wPlayerCount = 0;
	for (WORD i = 0; i<m_wPlayerCount; i++)
	{
		//获取用户
		IServerUserItem * pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
		if (pIServerUserItem != NULL)
		{
			if (m_cbPlayStatus[i] == FALSE&&m_cbDynamicJoin[i] == FALSE)continue;
			if (pIServerUserItem->IsAndroidUser())
			{
				wAiCount++;
				if (!bIsAiBanker && i == m_wBankerUser)bIsAiBanker = true;
			}
			wPlayerCount++;
		}
	}

	//全部机器
	if (wPlayerCount == wAiCount || wAiCount == 0)
	{
		return;
	}

	//扑克变量
	BYTE cbUserCardData[GAME_PLAYER][MAX_CARDCOUNT];
	CopyMemory(cbUserCardData, m_cbHandCardData, sizeof(m_cbHandCardData));

	//排列扑克
	//for (WORD i=0;i<m_wPlayerCount;i++)
	//{
	//	m_GameLogic.SortNNCardList(cbUserCardData[i],MAX_CARDCOUNT);
	//}

	//获取最大牌型
	for (WORD i = 0; i<m_wPlayerCount; i++)
	{
		if (m_cbPlayStatus[i] == TRUE)
		{
			m_GameLogic.GetOxCard(cbUserCardData[i], MAX_CARDCOUNT);
		}
	}

	//变量定义
	LONGLONG lAndroidScore = 0;

	//倍数变量
	BYTE cbCardTimes[GAME_PLAYER];
	ZeroMemory(cbCardTimes, sizeof(cbCardTimes));

	//查找倍数
	for (WORD i = 0; i<m_wPlayerCount; i++)
	{
		if (m_cbPlayStatus[i] == TRUE)
		{
			cbCardTimes[i] = m_GameLogic.GetTimes(cbUserCardData[i], MAX_CARDCOUNT, m_ctConfig);
		}
	}

	//机器庄家
	if (bIsAiBanker)
	{
		//对比扑克
		for (WORD i = 0; i<m_wPlayerCount; i++)
		{
			//用户过滤
			if ((i == m_wBankerUser) || (m_cbPlayStatus[i] == FALSE)) continue;

			//获取用户
			IServerUserItem * pIServerUserItem = m_pITableFrame->GetTableUserItem(i);

			//机器过滤
			if ((pIServerUserItem != NULL) && (pIServerUserItem->IsAndroidUser())) continue;

			//对比扑克
			if (m_GameLogic.CompareCard(cbUserCardData[i], cbUserCardData[m_wBankerUser], MAX_CARDCOUNT, m_ctConfig) == true)
			{
				lAndroidScore -= cbCardTimes[i] * m_lTableScore[i];
			}
			else
			{
				lAndroidScore += cbCardTimes[m_wBankerUser] * m_lTableScore[i];
			}
		}
	}
	else//用户庄家
	{
		//对比扑克
		for (WORD i = 0; i<m_wPlayerCount; i++)
		{
			//获取用户
			IServerUserItem * pIServerUserItem = m_pITableFrame->GetTableUserItem(i);

			//用户过滤
			if ((i == m_wBankerUser) || (pIServerUserItem == NULL) || !(pIServerUserItem->IsAndroidUser())) continue;

			//对比扑克
			if (m_GameLogic.CompareCard(cbUserCardData[i], cbUserCardData[m_wBankerUser], MAX_CARDCOUNT, m_ctConfig) == true)
			{
				lAndroidScore += cbCardTimes[i] * m_lTableScore[i];
			}
			else
			{
				lAndroidScore -= cbCardTimes[m_wBankerUser] * m_lTableScore[i];
			}
		}
	}

	LONGLONG lGameEndStorage = g_lRoomStorageCurrent + lAndroidScore;

	//下注发牌
	if (stConfig == ST_BETFIRST_)
	{
		//变量定义
		WORD wMaxUser = INVALID_CHAIR;
		WORD wMinAndroid = INVALID_CHAIR;
		WORD wMaxAndroid = INVALID_CHAIR;

		//查找特殊玩家
		for (WORD i = 0; i<m_wPlayerCount; i++)
		{
			//获取用户
			IServerUserItem * pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
			if (pIServerUserItem == NULL) continue;

			//真人玩家
			if (pIServerUserItem->IsAndroidUser() == false)
			{
				//初始设置
				if (wMaxUser == INVALID_CHAIR) wMaxUser = i;

				//获取较大者
				if (m_GameLogic.CompareCard(cbUserCardData[i], cbUserCardData[wMaxUser], MAX_CARDCOUNT, m_ctConfig) == true)
				{
					wMaxUser = i;
				}
			}

			//机器玩家
			if (pIServerUserItem->IsAndroidUser() == true)
			{
				//初始设置
				if (wMinAndroid == INVALID_CHAIR) wMinAndroid = i;
				if (wMaxAndroid == INVALID_CHAIR) wMaxAndroid = i;

				//获取较小者
				if (m_GameLogic.CompareCard(cbUserCardData[i], cbUserCardData[wMinAndroid], MAX_CARDCOUNT, m_ctConfig) == false)
				{
					wMinAndroid = i;
				}

				//获取较大者
				if (m_GameLogic.CompareCard(cbUserCardData[i], cbUserCardData[wMaxAndroid], MAX_CARDCOUNT, m_ctConfig) == true)
				{
					wMaxAndroid = i;
				}
			}
		}

		//库存判断
		if (g_lRoomStorageCurrent + lAndroidScore<0 || (lGameEndStorage < (g_lRoomStorageCurrent * (double)(1 - (double)5 / (double)100))))
		{
			//变量定义
			WORD wWinUser = wMaxUser;

			//机器坐庄
			if (bIsAiBanker)
			{
				//查找数据
				for (WORD i = 0; i<m_wPlayerCount; i++)
				{

					//用户过滤
					if (m_cbPlayStatus[i] == FALSE) continue;

					//获取较大者
					if (m_GameLogic.CompareCard(cbUserCardData[i], cbUserCardData[wWinUser], MAX_CARDCOUNT, m_ctConfig) == true)
					{
						wWinUser = i;
					}
				}
			}
			else
			{
				//获取较小者
				if (m_GameLogic.CompareCard(cbUserCardData[wMaxAndroid], cbUserCardData[wWinUser], MAX_CARDCOUNT, m_ctConfig) == false)
				{
					wWinUser = wMaxAndroid;
				}
			}

			if (bIsAiBanker)
			{
				//交换数据
				BYTE cbTempData[MAX_CARDCOUNT];
				CopyMemory(cbTempData, m_cbHandCardData[m_wBankerUser], MAX_CARDCOUNT);
				CopyMemory(m_cbHandCardData[m_wBankerUser], m_cbHandCardData[wWinUser], MAX_CARDCOUNT);
				CopyMemory(m_cbHandCardData[wWinUser], cbTempData, MAX_CARDCOUNT);
			}
			else
			{
				BYTE bUser = m_wBankerUser;

				//每一个玩家代替庄家，直到不会出现负库存为止
				do
				{
					bUser = (bUser + 1) % GAME_PLAYER;

					if (m_cbPlayStatus[bUser] == TRUE)
					{
						//交换数据
						BYTE cbTempData[MAX_CARDCOUNT];
						CopyMemory(cbTempData, m_cbHandCardData[m_wBankerUser], MAX_CARDCOUNT);
						CopyMemory(m_cbHandCardData[m_wBankerUser], m_cbHandCardData[bUser], MAX_CARDCOUNT);
						CopyMemory(m_cbHandCardData[bUser], cbTempData, MAX_CARDCOUNT);
					}

				} while (!JudgeStock() && (bUser != m_wBankerUser));

			}
		}
		else if (g_lRoomStorageCurrent>0 /*&& lAndroidScore>0*/ && g_lRoomStorageCurrent > g_lStorageMax2Room && g_lRoomStorageCurrent - lAndroidScore > 0 && rand() % 100 < g_lStorageMul2Room)
		{
			if (m_GameLogic.CompareCard(cbUserCardData[wMaxAndroid], cbUserCardData[wMaxUser], MAX_CARDCOUNT, m_ctConfig) == true)
			{
				//交换数据
				BYTE cbTempData[MAX_CARDCOUNT];
				CopyMemory(cbTempData, m_cbHandCardData[wMaxUser], MAX_CARDCOUNT);
				CopyMemory(m_cbHandCardData[wMaxUser], m_cbHandCardData[wMaxAndroid], MAX_CARDCOUNT);
				CopyMemory(m_cbHandCardData[wMaxAndroid], cbTempData, MAX_CARDCOUNT);

				//库存不够换回来
				if (JudgeStock() == false)
				{
					CopyMemory(cbTempData, m_cbHandCardData[wMaxUser], MAX_CARDCOUNT);
					CopyMemory(m_cbHandCardData[wMaxUser], m_cbHandCardData[wMaxAndroid], MAX_CARDCOUNT);
					CopyMemory(m_cbHandCardData[wMaxAndroid], cbTempData, MAX_CARDCOUNT);
				}
			}
		}
		else if (g_lRoomStorageCurrent>0 /*&& lAndroidScore>0*/ && g_lRoomStorageCurrent > g_lStorageMax1Room && g_lRoomStorageCurrent - lAndroidScore > 0 && rand() % 100 < g_lStorageMul1Room)
		{
			if (m_GameLogic.CompareCard(cbUserCardData[wMaxAndroid], cbUserCardData[wMaxUser], MAX_CARDCOUNT, m_ctConfig) == true)
			{
				//交换数据
				BYTE cbTempData[MAX_CARDCOUNT];
				CopyMemory(cbTempData, m_cbHandCardData[wMaxUser], MAX_CARDCOUNT);
				CopyMemory(m_cbHandCardData[wMaxUser], m_cbHandCardData[wMaxAndroid], MAX_CARDCOUNT);
				CopyMemory(m_cbHandCardData[wMaxAndroid], cbTempData, MAX_CARDCOUNT);

				//库存不够换回来
				if (JudgeStock() == false)
				{
					CopyMemory(cbTempData, m_cbHandCardData[wMaxUser], MAX_CARDCOUNT);
					CopyMemory(m_cbHandCardData[wMaxUser], m_cbHandCardData[wMaxAndroid], MAX_CARDCOUNT);
					CopyMemory(m_cbHandCardData[wMaxAndroid], cbTempData, MAX_CARDCOUNT);
				}
			}
		}
	}
	else if (stConfig == ST_SENDFOUR_)
	{
		//扑克链表
		CList<BYTE, BYTE&> cardlist;
		cardlist.RemoveAll();

		//含大小王
		if (m_gtConfig == GT_HAVEKING_)
		{
			for (WORD i = 0; i<54; i++)
			{
				cardlist.AddTail(m_GameLogic.m_cbCardListDataHaveKing[i]);
			}
		}
		else if (m_gtConfig == GT_NOKING_)
		{
			for (WORD i = 0; i<52; i++)
			{
				cardlist.AddTail(m_GameLogic.m_cbCardListDataNoKing[i]);
			}
		}

		//删除扑克 （删除前面4张，构造后面一张）
		for (WORD i = 0; i<GAME_PLAYER; i++)
		{
			for (WORD j = 0; j<MAX_CARDCOUNT - 1; j++)
			{
				if (m_cbHandCardData[i][j] != 0)
				{
					POSITION ptListHead = cardlist.GetHeadPosition();
					POSITION ptTemp;
					BYTE cbCardData = INVALID_BYTE;

					//遍历链表
					while (ptListHead)
					{
						ptTemp = ptListHead;
						if (cardlist.GetNext(ptListHead) == m_cbHandCardData[i][j])
						{
							cardlist.RemoveAt(ptTemp);
							break;
						}
					}
				}
			}
		}

		//库存判断
		if (g_lRoomStorageCurrent + lAndroidScore<0 || (lGameEndStorage < (g_lRoomStorageCurrent * (double)(1 - (double)5 / (double)100))))
		{
			//机器人从牛牛开始构造，普通玩家从无牛开始构造
			for (WORD i = 0; i<m_wPlayerCount; i++)
			{
				//获取用户
				IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if (pIServerUserItem != NULL && m_cbPlayStatus[i] == TRUE && m_cbDynamicJoin[i] == FALSE)
				{
					//机器人
					if (pIServerUserItem->IsAndroidUser() == true)
					{
						//从牛牛顺序开始构造，
						for (WORD wCardTypeIndex = CT_CLASSIC_OX_VALUENIUNIU; wCardTypeIndex>CT_CLASSIC_OX_VALUE0; wCardTypeIndex--)
						{
							BYTE cbKeyCardData = m_GameLogic.ConstructCardType(cardlist, m_cbHandCardData[i], wCardTypeIndex, m_gtConfig);
							if (cbKeyCardData == INVALID_BYTE)
							{
								continue;
							}
							else
							{
								m_cbHandCardData[i][4] = cbKeyCardData;
								break;
							}
						}
					}
					else if (pIServerUserItem->IsAndroidUser() == false)
					{
						//构造无牛到牛九
						for (WORD wCardTypeIndex = CT_CLASSIC_OX_VALUE0; wCardTypeIndex<CT_CLASSIC_OX_VALUENIUNIU; wCardTypeIndex++)
						{
							BYTE cbKeyCardData = m_GameLogic.ConstructCardType(cardlist, m_cbHandCardData[i], wCardTypeIndex, m_gtConfig);
							if (cbKeyCardData == INVALID_BYTE)
							{
								continue;
							}
							else
							{
								m_cbHandCardData[i][4] = cbKeyCardData;
								break;
							}
						}
					}
				}
			}
		}
		else if (g_lRoomStorageCurrent>0 /*&& lAndroidScore>0*/ && g_lRoomStorageCurrent > g_lStorageMax2Room && g_lRoomStorageCurrent - lAndroidScore > 0 && rand() % 100 < g_lStorageMul2Room)
		{
			//机器人从无牛开始构造，普通玩家从牛牛开始构造
			for (WORD i = 0; i<m_wPlayerCount; i++)
			{
				//获取用户
				IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if (pIServerUserItem != NULL && m_cbPlayStatus[i] == TRUE && m_cbDynamicJoin[i] == FALSE)
				{
					//机器人
					if (pIServerUserItem->IsAndroidUser() == true)
					{
						//构造无牛到牛九
						for (WORD wCardTypeIndex = CT_CLASSIC_OX_VALUE0; wCardTypeIndex<CT_CLASSIC_OX_VALUENIUNIU; wCardTypeIndex++)
						{
							BYTE cbKeyCardData = m_GameLogic.ConstructCardType(cardlist, m_cbHandCardData[i], wCardTypeIndex, m_gtConfig);
							if (cbKeyCardData == INVALID_BYTE)
							{
								continue;
							}
							else
							{
								m_cbHandCardData[i][4] = cbKeyCardData;
								break;
							}
						}
					}
					else if (pIServerUserItem->IsAndroidUser() == false)
					{
						//从牛牛顺序开始构造，
						for (WORD wCardTypeIndex = CT_CLASSIC_OX_VALUENIUNIU; wCardTypeIndex>CT_CLASSIC_OX_VALUE0; wCardTypeIndex--)
						{
							BYTE cbKeyCardData = m_GameLogic.ConstructCardType(cardlist, m_cbHandCardData[i], wCardTypeIndex, m_gtConfig);
							if (cbKeyCardData == INVALID_BYTE)
							{
								continue;
							}
							else
							{
								m_cbHandCardData[i][4] = cbKeyCardData;
								break;
							}
						}
					}
				}
			}
		}
		else if (g_lRoomStorageCurrent>0 /*&& lAndroidScore>0*/ && g_lRoomStorageCurrent > g_lStorageMax1Room && g_lRoomStorageCurrent - lAndroidScore > 0 && rand() % 100 < g_lStorageMul1Room)
		{
			//机器人从无牛开始构造，普通玩家从牛牛开始构造
			for (WORD i = 0; i<m_wPlayerCount; i++)
			{
				//获取用户
				IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if (pIServerUserItem != NULL && m_cbPlayStatus[i] == TRUE && m_cbDynamicJoin[i] == FALSE)
				{
					//机器人
					if (pIServerUserItem->IsAndroidUser() == true)
					{
						//构造无牛到牛九
						for (WORD wCardTypeIndex = CT_CLASSIC_OX_VALUE0; wCardTypeIndex<CT_CLASSIC_OX_VALUENIUNIU; wCardTypeIndex++)
						{
							BYTE cbKeyCardData = m_GameLogic.ConstructCardType(cardlist, m_cbHandCardData[i], wCardTypeIndex, m_gtConfig);
							if (cbKeyCardData == INVALID_BYTE)
							{
								continue;
							}
							else
							{
								m_cbHandCardData[i][4] = cbKeyCardData;
								break;
							}
						}
					}
					else if (pIServerUserItem->IsAndroidUser() == false)
					{
						//从牛牛顺序开始构造，
						for (WORD wCardTypeIndex = CT_CLASSIC_OX_VALUENIUNIU; wCardTypeIndex>CT_CLASSIC_OX_VALUE0; wCardTypeIndex--)
						{
							BYTE cbKeyCardData = m_GameLogic.ConstructCardType(cardlist, m_cbHandCardData[i], wCardTypeIndex, m_gtConfig);
							if (cbKeyCardData == INVALID_BYTE)
							{
								continue;
							}
							else
							{
								m_cbHandCardData[i][4] = cbKeyCardData;
								break;
							}
						}
					}
				}
			}
		}
	}

	return;
}

//判断库存
bool CTableFrameSink::JudgeStock()
{
	//机器人数
	bool bIsAiBanker = false;
	WORD wAiCount = 0;
	WORD wPlayerCount = 0;
	for (WORD i = 0; i<m_wPlayerCount; i++)
	{
		//获取用户
		IServerUserItem * pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
		if (pIServerUserItem != NULL)
		{
			if (m_cbPlayStatus[i] == FALSE&&m_cbDynamicJoin[i] == FALSE)continue;
			if (pIServerUserItem->IsAndroidUser())
			{
				wAiCount++;
				if (!bIsAiBanker && i == m_wBankerUser)bIsAiBanker = true;
			}
			wPlayerCount++;
		}
	}

	//扑克变量
	BYTE cbUserCardData[GAME_PLAYER][MAX_CARDCOUNT];
	CopyMemory(cbUserCardData, m_cbHandCardData, sizeof(m_cbHandCardData));

	//排列扑克
	//for (WORD i=0;i<m_wPlayerCount;i++)
	//{
	//	m_GameLogic.SortNNCardList(cbUserCardData[i],MAX_CARDCOUNT);
	//}

	//获取最大牌型
	for (WORD i = 0; i<m_wPlayerCount; i++)
	{
		if (m_cbPlayStatus[i] == TRUE)
		{
			m_GameLogic.GetOxCard(cbUserCardData[i], MAX_CARDCOUNT);
		}
	}

	//变量定义
	LONGLONG lAndroidScore = 0;

	//倍数变量
	BYTE cbCardTimes[GAME_PLAYER];
	ZeroMemory(cbCardTimes, sizeof(cbCardTimes));

	//查找倍数
	for (WORD i = 0; i<m_wPlayerCount; i++)
	{
		if (m_cbPlayStatus[i] == TRUE)
		{
			cbCardTimes[i] = m_GameLogic.GetTimes(cbUserCardData[i], MAX_CARDCOUNT, m_ctConfig);
		}
	}

	//机器庄家
	if (bIsAiBanker)
	{
		//对比扑克
		for (WORD i = 0; i<m_wPlayerCount; i++)
		{
			//用户过滤
			if ((i == m_wBankerUser) || (m_cbPlayStatus[i] == FALSE)) continue;

			//获取用户
			IServerUserItem * pIServerUserItem = m_pITableFrame->GetTableUserItem(i);

			//机器过滤
			if ((pIServerUserItem != NULL) && (pIServerUserItem->IsAndroidUser())) continue;

			//对比扑克
			if (m_GameLogic.CompareCard(cbUserCardData[i], cbUserCardData[m_wBankerUser], MAX_CARDCOUNT, m_ctConfig) == true)
			{
				lAndroidScore -= cbCardTimes[i] * m_lTableScore[i];
			}
			else
			{
				lAndroidScore += cbCardTimes[m_wBankerUser] * m_lTableScore[i];
			}
		}
	}
	else//用户庄家
	{
		//对比扑克
		for (WORD i = 0; i<m_wPlayerCount; i++)
		{
			//获取用户
			IServerUserItem * pIServerUserItem = m_pITableFrame->GetTableUserItem(i);

			//用户过滤
			if ((i == m_wBankerUser) || (pIServerUserItem == NULL) || !(pIServerUserItem->IsAndroidUser())) continue;

			//对比扑克
			if (m_GameLogic.CompareCard(cbUserCardData[i], cbUserCardData[m_wBankerUser], MAX_CARDCOUNT, m_ctConfig) == true)
			{
				lAndroidScore += cbCardTimes[i] * m_lTableScore[i];
			}
			else
			{
				lAndroidScore -= cbCardTimes[m_wBankerUser] * m_lTableScore[i];
			}
		}
	}

	//变量定义
	WORD wMaxUser = INVALID_CHAIR;
	WORD wMinAndroid = INVALID_CHAIR;
	WORD wMaxAndroid = INVALID_CHAIR;

	//查找特殊玩家
	for (WORD i = 0; i<m_wPlayerCount; i++)
	{
		//获取用户
		IServerUserItem * pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
		if (pIServerUserItem == NULL) continue;

		//真人玩家
		if (pIServerUserItem->IsAndroidUser() == false)
		{
			//初始设置
			if (wMaxUser == INVALID_CHAIR) wMaxUser = i;

			//获取较大者
			if (m_GameLogic.CompareCard(cbUserCardData[i], cbUserCardData[wMaxUser], MAX_CARDCOUNT, m_ctConfig) == true)
			{
				wMaxUser = i;
			}
		}

		//机器玩家
		if (pIServerUserItem->IsAndroidUser() == true)
		{
			//初始设置
			if (wMinAndroid == INVALID_CHAIR) wMinAndroid = i;
			if (wMaxAndroid == INVALID_CHAIR) wMaxAndroid = i;

			//获取较小者
			if (m_GameLogic.CompareCard(cbUserCardData[i], cbUserCardData[wMinAndroid], MAX_CARDCOUNT, m_ctConfig) == false)
			{
				wMinAndroid = i;
			}

			//获取较大者
			if (m_GameLogic.CompareCard(cbUserCardData[i], cbUserCardData[wMaxAndroid], MAX_CARDCOUNT, m_ctConfig) == true)
			{
				wMaxAndroid = i;
			}
		}
	}

	return g_lRoomStorageCurrent + lAndroidScore>0;

}

//查询是否扣服务费
bool CTableFrameSink::QueryBuckleServiceCharge(WORD wChairID)
{
	for (BYTE i=0;i<m_wPlayerCount;i++)
	{
		IServerUserItem *pUserItem=m_pITableFrame->GetTableUserItem(i);
		if(pUserItem==NULL) continue;
		
		if (m_bBuckleServiceCharge[i]&&i==wChairID)
		{
			return true;
		}
		
	}
	return false;
}


bool CTableFrameSink::TryWriteTableScore(tagScoreInfo ScoreInfoArray[])
{
	//修改积分
	tagScoreInfo ScoreInfo[GAME_PLAYER];
	ZeroMemory(&ScoreInfo,sizeof(ScoreInfo));
	memcpy(&ScoreInfo,ScoreInfoArray,sizeof(ScoreInfo));
	//记录异常
	LONGLONG beforeScore[GAME_PLAYER];
	ZeroMemory(beforeScore,sizeof(beforeScore));
	for (WORD i=0;i<m_wPlayerCount;i++)
	{
		IServerUserItem *pItem=m_pITableFrame->GetTableUserItem(i);
		if (pItem != NULL && ScoreInfo[i].cbType != SCORE_TYPE_NULL)
		{
			beforeScore[i]=pItem->GetUserScore();
			m_pITableFrame->WriteUserScore(i, ScoreInfo[i]);
		}
	}

	LONGLONG afterScore[GAME_PLAYER];
	ZeroMemory(afterScore,sizeof(afterScore));
	for (WORD i=0;i<m_wPlayerCount;i++)
	{
		IServerUserItem *pItem=m_pITableFrame->GetTableUserItem(i);
		if(pItem!=NULL)
		{
			afterScore[i]=pItem->GetUserScore();

			if(afterScore[i]<0)
			{
				//异常写入日志
				CString strInfo;
				strInfo.Format(TEXT("[%s] 出现负分, 写分前分数：%I64d, 写分信息：写入积分 %I64d，税收 %I64d, 写分后分数：%I64d"),
					pItem->GetNickName(), beforeScore[i], ScoreInfoArray[i].lScore, ScoreInfoArray[i].lRevenue, afterScore[i]);
				WriteInfo(strInfo);
			}

		}
	}
	return true;
}

//最大下分
SCORE CTableFrameSink::GetUserMaxTurnScore(WORD wChairID)
{

	SCORE lMaxTurnScore=0L;
	if(wChairID==m_wBankerUser)  return 0;
	//庄家积分
	IServerUserItem *pIBankerItem=m_pITableFrame->GetTableUserItem(m_wBankerUser);
	LONGLONG lBankerScore=0L;
	if(pIBankerItem!=NULL)
		lBankerScore=pIBankerItem->GetUserScore();

	//玩家人数
	WORD wUserCount=0;
	for (WORD i=0;i<m_wPlayerCount;i++)
		if(m_cbPlayStatus[i]==TRUE )wUserCount++;

	//获取用户
	IServerUserItem * pIServerUserItem=m_pITableFrame->GetTableUserItem(wChairID);

	BYTE cbMaxCallBankerTimes = 1;
	if (m_bgtConfig == BGT_ROB_)
	{
		for (WORD i=0;i<m_wPlayerCount;i++)
		{
			if(m_cbPlayStatus[i]==TRUE && m_cbCallBankerStatus[i]==TRUE && m_cbCallBankerTimes[i] > cbMaxCallBankerTimes) 
			{
				cbMaxCallBankerTimes = m_cbCallBankerTimes[i];
			}
		}
	}
	
	//计算百分比下注模式
	if(pIServerUserItem!=NULL)
	{
		//获取积分
		LONGLONG lScore=pIServerUserItem->GetUserScore();

		//判0校验
		LONGLONG lBankerVal = (wUserCount - 1) * m_lMaxCardTimes * cbMaxCallBankerTimes * m_pITableFrame->GetCellScore();
		LONGLONG lPlayerVal = m_lMaxCardTimes * cbMaxCallBankerTimes * m_pITableFrame->GetCellScore();
		if (lBankerVal == 0 || lPlayerVal == 0)
		{
			CString strdebug;
			strdebug.Format(TEXT("wUserCount = %d, cbMaxCallBankerTimes = %d, lCellScore = %d"), wUserCount, cbMaxCallBankerTimes, m_pITableFrame->GetCellScore());
			CTraceService::TraceString(strdebug, TraceLevel_Exception);

			return lMaxTurnScore;
		}

		lMaxTurnScore=__min(lBankerScore/(wUserCount-1)/m_lMaxCardTimes/cbMaxCallBankerTimes/m_pITableFrame->GetCellScore(), lScore/m_lMaxCardTimes/cbMaxCallBankerTimes/m_pITableFrame->GetCellScore());
	}

	//计算自由下注模式
	LONG lMaxBet = 0;
	if (m_btConfig == BT_FREE_)
	{
		for (WORD i = MAX_CONFIG - 1; i >= 0; i--)
		{
			if (m_lFreeConfig[i] == 0)
			{
				continue;
			}

			lMaxBet = m_lFreeConfig[i];
			break;
		}
	}

	return (m_btConfig == BT_FREE_ ? lMaxBet : lMaxTurnScore);
}

//查询限额
SCORE CTableFrameSink::QueryConsumeQuota(IServerUserItem * pIServerUserItem)
{
	return 0L;
}

//是否衰减
bool CTableFrameSink::NeedDeductStorage()
{
	for ( int i = 0; i < m_wPlayerCount; ++i )
	{
		IServerUserItem *pIServerUserItem=m_pITableFrame->GetTableUserItem(i);
		if (pIServerUserItem == NULL ) continue; 

		if(!pIServerUserItem->IsAndroidUser())
		{
			return true;
		}
	}

	return false;

}

//读取配置
void CTableFrameSink::ReadConfigInformation()
{	
	//获取自定义配置
	tagCustomRule *pCustomRule = (tagCustomRule *)m_pGameServiceOption->cbCustomRule;
	ASSERT(pCustomRule);
	
	g_lRoomStorageStart = pCustomRule->lRoomStorageStart;
	g_lRoomStorageCurrent = pCustomRule->lRoomStorageStart;
	g_lStorageDeductRoom = pCustomRule->lRoomStorageDeduct;
	g_lStorageMax1Room = pCustomRule->lRoomStorageMax1;
	g_lStorageMul1Room = pCustomRule->lRoomStorageMul1;
	g_lStorageMax2Room = pCustomRule->lRoomStorageMax2;
	g_lStorageMul2Room = pCustomRule->lRoomStorageMul2;

	if( g_lStorageDeductRoom < 0 || g_lStorageDeductRoom > 1000 )
		g_lStorageDeductRoom = 0;
	if ( g_lStorageDeductRoom > 1000 )
		g_lStorageDeductRoom = 1000;
	if (g_lStorageMul1Room < 0 || g_lStorageMul1Room > 100) 
		g_lStorageMul1Room = 50;
	if (g_lStorageMul2Room < 0 || g_lStorageMul2Room > 100) 
		g_lStorageMul2Room = 80;

	m_ctConfig = pCustomRule->ctConfig;
	m_stConfig = pCustomRule->stConfig;
	m_gtConfig = pCustomRule->gtConfig;
	m_bgtConfig = pCustomRule->bgtConfig;
	m_btConfig = pCustomRule->btConfig;

	CopyMemory(m_lFreeConfig, pCustomRule->lFreeConfig, sizeof(m_lFreeConfig));
	CopyMemory(m_lPercentConfig, pCustomRule->lPercentConfig, sizeof(m_lPercentConfig));
}

//更新房间用户信息
void CTableFrameSink::UpdateRoomUserInfo(IServerUserItem *pIServerUserItem, USERACTION userAction)
{
	//变量定义
	ROOMUSERINFO roomUserInfo;
	ZeroMemory(&roomUserInfo, sizeof(roomUserInfo));

	roomUserInfo.dwGameID = pIServerUserItem->GetGameID();
	CopyMemory(&(roomUserInfo.szNickName), pIServerUserItem->GetNickName(), sizeof(roomUserInfo.szNickName));
	roomUserInfo.cbUserStatus = pIServerUserItem->GetUserStatus();
	roomUserInfo.cbGameStatus = m_pITableFrame->GetGameStatus();

	roomUserInfo.bAndroid = pIServerUserItem->IsAndroidUser();

	//用户坐下和重连
	if (userAction == USER_SITDOWN || userAction == USER_RECONNECT)
	{
		roomUserInfo.wChairID = pIServerUserItem->GetChairID();
		roomUserInfo.wTableID = pIServerUserItem->GetTableID() + 1;
	}
	else if (userAction == USER_STANDUP || userAction == USER_OFFLINE)
	{
		roomUserInfo.wChairID = INVALID_CHAIR;
		roomUserInfo.wTableID = INVALID_TABLE;
	}

	g_MapRoomUserInfo.SetAt(pIServerUserItem->GetUserID(), roomUserInfo);

	//遍历映射，删除离开房间的玩家，
	POSITION ptHead = g_MapRoomUserInfo.GetStartPosition();
	DWORD dwUserID = 0;
	ROOMUSERINFO userinfo;
	ZeroMemory(&userinfo, sizeof(userinfo));
	TCHAR szNickName[LEN_NICKNAME];
	ZeroMemory(szNickName, sizeof(szNickName));
	DWORD *pdwRemoveKey	= new DWORD[g_MapRoomUserInfo.GetSize()];
	ZeroMemory(pdwRemoveKey, sizeof(DWORD) * g_MapRoomUserInfo.GetSize());
	WORD wRemoveKeyIndex = 0;

	while(ptHead)
	{
		g_MapRoomUserInfo.GetNextAssoc(ptHead, dwUserID, userinfo);

		if (userinfo.dwGameID == 0 && (_tcscmp(szNickName, userinfo.szNickName) == 0) && userinfo.cbUserStatus == 0 )
		{
			pdwRemoveKey[wRemoveKeyIndex++] = dwUserID;
		}

	}

	for (WORD i=0; i<wRemoveKeyIndex; i++)
	{
		g_MapRoomUserInfo.RemoveKey(pdwRemoveKey[i]);

		CString strtip;
		strtip.Format(TEXT("RemoveKey,wRemoveKeyIndex = %d, g_MapRoomUserInfosize = %d"), wRemoveKeyIndex, g_MapRoomUserInfo.GetSize());

		WriteInfo(strtip);
	}

	delete[] pdwRemoveKey;
}

//更新同桌用户控制
void CTableFrameSink::UpdateUserControl(IServerUserItem *pIServerUserItem)
{
	//变量定义
	POSITION ptListHead;
	POSITION ptTemp;
	ROOMUSERCONTROL roomusercontrol;

	//初始化
	ptListHead = g_ListRoomUserControl.GetHeadPosition();
	ZeroMemory(&roomusercontrol, sizeof(roomusercontrol));

	//遍历链表
	while(ptListHead)
	{
		ptTemp = ptListHead;
		roomusercontrol = g_ListRoomUserControl.GetNext(ptListHead);

		//寻找已存在控制玩家
		if ((pIServerUserItem->GetGameID() == roomusercontrol.roomUserInfo.dwGameID) &&
			_tcscmp(pIServerUserItem->GetNickName(), roomusercontrol.roomUserInfo.szNickName) == 0)
		{
			//获取元素
			ROOMUSERCONTROL &tmproomusercontrol = g_ListRoomUserControl.GetAt(ptTemp);

			//重设参数
			tmproomusercontrol.roomUserInfo.wChairID = pIServerUserItem->GetChairID();
			tmproomusercontrol.roomUserInfo.wTableID = m_pITableFrame->GetTableID() + 1;

			return;
		}
	}
}

//除重用户控制
void CTableFrameSink::TravelControlList(ROOMUSERCONTROL keyroomusercontrol)
{
	//变量定义
	POSITION ptListHead;
	POSITION ptTemp;
	ROOMUSERCONTROL roomusercontrol;

	//初始化
	ptListHead = g_ListRoomUserControl.GetHeadPosition();
	ZeroMemory(&roomusercontrol, sizeof(roomusercontrol));

	//遍历链表
	while(ptListHead)
	{
		ptTemp = ptListHead;
		roomusercontrol = g_ListRoomUserControl.GetNext(ptListHead);

		//寻找已存在控制玩家在用一张桌子切换椅子
		if ((keyroomusercontrol.roomUserInfo.dwGameID == roomusercontrol.roomUserInfo.dwGameID) &&
			_tcscmp(keyroomusercontrol.roomUserInfo.szNickName, roomusercontrol.roomUserInfo.szNickName) == 0)
		{
			g_ListRoomUserControl.RemoveAt(ptTemp);
		}
	}
}

//是否满足控制条件
void CTableFrameSink::IsSatisfyControl(ROOMUSERINFO &userInfo, bool &bEnableControl)
{
	if (userInfo.wChairID == INVALID_CHAIR || userInfo.wTableID == INVALID_TABLE)
	{
		bEnableControl = FALSE;
		return;
	}

	if (userInfo.cbUserStatus == US_SIT || userInfo.cbUserStatus == US_READY || userInfo.cbUserStatus == US_PLAYING)
	{
		bEnableControl = TRUE;
		return;	
	}
	else
	{
		bEnableControl = FALSE;
		return;
	}
}

//分析房间用户控制
bool CTableFrameSink::AnalyseRoomUserControl(ROOMUSERCONTROL &Keyroomusercontrol, POSITION &ptList)
{
	//变量定义
	POSITION ptListHead;
	POSITION ptTemp;
	ROOMUSERCONTROL roomusercontrol;

	//遍历链表
	for (WORD i=0; i<m_wPlayerCount; i++)
	{
		IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
		if (!pIServerUserItem)
		{
			continue;
		}

		//初始化
		ptListHead = g_ListRoomUserControl.GetHeadPosition();
		ZeroMemory(&roomusercontrol, sizeof(roomusercontrol));

		//遍历链表
		while(ptListHead)
		{
			ptTemp = ptListHead;
			roomusercontrol = g_ListRoomUserControl.GetNext(ptListHead);

			//寻找玩家
			if ((pIServerUserItem->GetGameID() == roomusercontrol.roomUserInfo.dwGameID) &&
				_tcscmp(pIServerUserItem->GetNickName(), roomusercontrol.roomUserInfo.szNickName) == 0)
			{
				//清空控制局数为0的元素
				if (roomusercontrol.userControl.cbControlCount == 0)
				{
					g_ListRoomUserControl.RemoveAt(ptTemp);
					break;
				}

				if (roomusercontrol.userControl.control_type == CONTINUE_CANCEL)
				{
					g_ListRoomUserControl.RemoveAt(ptTemp);
					break;
				}

				//拷贝数据
				CopyMemory(&Keyroomusercontrol, &roomusercontrol, sizeof(roomusercontrol));
				ptList = ptTemp;

				return true;
			}

		}

	}

	return false;
}

void CTableFrameSink::GetControlTypeString(CONTROL_TYPE &controlType, CString &controlTypestr)
{
	switch(controlType)
	{
	case CONTINUE_WIN:
		{
			controlTypestr = TEXT("控制类型为连赢");
			break;
		}
	case CONTINUE_LOST:
		{
			controlTypestr = TEXT("控制类型为连输");
			break;
		}
	case CONTINUE_CANCEL:
		{
			controlTypestr = TEXT("控制类型为取消控制");
			break;
		}
	}
}

//写日志文件
void CTableFrameSink::WriteInfo(LPCTSTR pszString)
{
	m_pITableFrame->SendGameMessage(pszString, SMT_GAMELOG);
	// //设置语言区域
	// char* old_locale = _strdup( setlocale(LC_CTYPE,NULL) );
	// setlocale( LC_CTYPE, "chs" );

	// CStdioFile myFile;
	// CString strFileName = TEXT("OxSixXDEBUG.txt");
	// BOOL bOpen = myFile.Open(strFileName, CFile::modeReadWrite|CFile::modeCreate|CFile::modeNoTruncate);
	// if ( bOpen )
	// {	
	// 	myFile.SeekToEnd();
	// 	myFile.WriteString( pszString );
	// 	myFile.Flush();
	// 	myFile.Close();
	// }

	// //还原区域设定
	// setlocale( LC_CTYPE, old_locale );
	// free( old_locale );
}

//测试写信息
void CTableFrameSink::WriteInfo(LPCTSTR pszFileName, LPCTSTR pszString)
{
	m_pITableFrame->SendGameMessage(pszString, SMT_GAMELOG);
	// //设置语言区域
	// char *old_locale = _strdup(setlocale(LC_CTYPE, NULL));
	// setlocale(LC_CTYPE, "chs");

	// CStdioFile myFile;
	// CString strFileName;
	// strFileName.Format(TEXT("%s"), pszFileName);
	// BOOL bOpen = myFile.Open(strFileName, CFile::modeReadWrite | CFile::modeCreate | CFile::modeNoTruncate);
	// if (bOpen)
	// {
	// 	myFile.SeekToEnd();
	// 	myFile.WriteString(pszString);
	// 	myFile.Flush();
	// 	myFile.Close();
	// }

	// //还原区域设定
	// setlocale(LC_CTYPE, old_locale);
	// free(old_locale);
}

//判断房卡房间
bool CTableFrameSink::IsRoomCardType()
{
	return ((m_pGameServiceOption->wServerType) & GAME_GENRE_PERSONAL) != 0;
}

//////////////////////////////////////////////////////////////////////////
