﻿#include "StdAfx.h"
#include "TableFrameSink.h"

//////////////////////////////////////////////////////////////////////////////////
#define IDI_CHECK_TABLE				1					//检查桌子
#define IDI_USER_OUT_TIME			6					//玩家超时

//构造函数
CTableFrameSink::CTableFrameSink()
{
	//组件变量
	m_pITableFrame=NULL;
	m_pGameCustomRule=NULL;
	m_pGameServiceOption=NULL;
	m_pGameServiceAttrib=NULL;
	m_bOffLineTrustee = false;

	//炸弹变量
	m_wFirstUser=INVALID_CHAIR;
	m_wBankerUser=INVALID_CHAIR;
	m_wCurrentUser=INVALID_CHAIR;
	ZeroMemory(m_cbOutCardCount,sizeof(m_cbOutCardCount));
	ZeroMemory(m_cbUserTrustee,sizeof(m_cbUserTrustee));
	m_lScoreTimes = 0L;

	//游戏变量
	m_wTimerControl=0;
	m_cbBombCount=0;
	ZeroMemory(m_cbEachBombCount,sizeof(m_cbEachBombCount));

	//叫分信息
	m_cbBankerScore=0;
	ZeroMemory(m_cbScoreInfo,sizeof(m_cbScoreInfo));
	m_cbCallScoreCount=0;

	//出牌信息
	m_cbTurnCardCount=0;
	m_wTurnWiner=INVALID_CHAIR;
	ZeroMemory(m_cbTurnCardData,sizeof(m_cbTurnCardData));

	//扑克信息
	ZeroMemory(m_cbBankerCard,sizeof(m_cbBankerCard));
	ZeroMemory(m_cbHandCardData,sizeof(m_cbHandCardData));
	ZeroMemory(m_cbHandCardCount,sizeof(m_cbHandCardCount));

	//服务控制
	m_hControlInst = NULL;
	m_pServerControl = NULL;
	m_hControlInst = LoadLibrary(TEXT("LandServerControl.dll"));
	if ( m_hControlInst )
	{
		typedef void * (*CREATE)(); 
		CREATE ServerControl = (CREATE)GetProcAddress(m_hControlInst,"CreateServerControl"); 
		if ( ServerControl )
		{
			m_pServerControl = static_cast<IServerControl*>(ServerControl());
		}
	}
	//游戏视频
	m_hVideoInst = NULL;
	m_pGameVideo = NULL;
	m_hVideoInst = LoadLibrary(TEXT("LandGameVideo.dll"));
	if ( m_hVideoInst )
	{
		typedef void * (*CREATE)(); 
		CREATE GameVideo = (CREATE)GetProcAddress(m_hVideoInst,"CreateGameVideo"); 
		if ( GameVideo )
		{
			m_pGameVideo = static_cast<IGameVideo*>(GameVideo());
		}
	}	
	
	ZeroMemory(&m_RoomCardRecord, sizeof(m_RoomCardRecord));

	return;
}

//析构函数
CTableFrameSink::~CTableFrameSink()
{

	if( m_pServerControl )
	{
		delete m_pServerControl;
		m_pServerControl = NULL;
	}

	if( m_hControlInst )
	{
		FreeLibrary(m_hControlInst);
		m_hControlInst = NULL;
	}
	
	if(m_pGameVideo)
	{
		delete m_pGameVideo;
		m_pGameVideo = NULL;
	}
	
	if(m_hVideoInst)
	{
		FreeLibrary(m_hVideoInst);
		m_hVideoInst = NULL;
	}
}

//接口查询
VOID * CTableFrameSink::QueryInterface(REFGUID Guid, DWORD dwQueryVer)
{
	QUERYINTERFACE(ITableFrameSink,Guid,dwQueryVer);
	QUERYINTERFACE(ITableUserAction,Guid,dwQueryVer);
	QUERYINTERFACE_IUNKNOWNEX(ITableFrameSink,Guid,dwQueryVer);
	return NULL;
}

//复位桌子
VOID CTableFrameSink::RepositionSink()
{
	//游戏变量
	m_cbBombCount=0;
	m_wBankerUser=INVALID_CHAIR;
	m_wCurrentUser=INVALID_CHAIR;
	ZeroMemory(m_cbOutCardCount,sizeof(m_cbOutCardCount));
	ZeroMemory(m_cbEachBombCount,sizeof(m_cbEachBombCount));
	ZeroMemory(m_cbUserTrustee,sizeof(m_cbUserTrustee));
	m_lScoreTimes = 0L;

	//叫分信息
	m_cbBankerScore=0;
	ZeroMemory(m_cbScoreInfo,sizeof(m_cbScoreInfo));
	m_cbCallScoreCount=0;

	//出牌信息
	m_cbTurnCardCount=0;
	m_wTurnWiner=INVALID_CHAIR;
	ZeroMemory(m_cbTurnCardData,sizeof(m_cbTurnCardData));

	//扑克信息
	ZeroMemory(m_cbBankerCard,sizeof(m_cbBankerCard));
	ZeroMemory(m_cbHandCardData,sizeof(m_cbHandCardData));
	ZeroMemory(m_cbHandCardCount,sizeof(m_cbHandCardCount));

	return;
}

//配置桌子
bool CTableFrameSink::Initialization(IUnknownEx * pIUnknownEx)
{
	//查询接口
	m_pITableFrame=QUERY_OBJECT_PTR_INTERFACE(pIUnknownEx,ITableFrame);

	//错误判断
	if (m_pITableFrame==NULL)
	{
		CTraceService::TraceString(TEXT("游戏桌子 CTableFrameSink 查询 ITableFrame 接口失败"),TraceLevel_Exception);
		return false;
	}

	//开始模式
	m_pITableFrame->SetStartMode(START_MODE_FULL_READY);

	//游戏配置
	m_pGameServiceAttrib=m_pITableFrame->GetGameServiceAttrib();
	m_pGameServiceOption=m_pITableFrame->GetGameServiceOption();

	//自定规则
	ASSERT(m_pITableFrame->GetCustomRule()!=NULL);
	m_pGameCustomRule=(tagCustomRule *)m_pITableFrame->GetCustomRule();

	m_bOffLineTrustee = CServerRule::IsAllowOffLineTrustee(m_pGameServiceOption->dwServerRule);

	return true;
}

//消费能力
SCORE CTableFrameSink::QueryConsumeQuota(IServerUserItem * pIServerUserItem)
{
	return 0L;
}

//最少积分
SCORE CTableFrameSink::QueryLessEnterScore(WORD wChairID, IServerUserItem * pIServerUserItem)
{
	return 0L;
}

//游戏开始
bool CTableFrameSink::OnEventGameStart()
{
	//出牌信息
	m_cbTurnCardCount=0;
	m_wTurnWiner=INVALID_CHAIR;
	ZeroMemory(m_cbTurnCardData,sizeof(m_cbTurnCardData));

	//设置状态
	m_pITableFrame->SetGameStatus(GAME_SCENE_CALL);

	//混乱扑克
	BYTE cbRandCard[FULL_COUNT];
	m_GameLogic.RandCardList(cbRandCard,CountArray(cbRandCard));

// 
 #ifdef _DEBUG

	//0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D, 方块
	//0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D, 梅花
	//0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D, 红桃
	//0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D, 黑桃

 	//BYTE cbTempRandCard[FULL_COUNT]=
 	//{
 	//	0x11,0x11,0x1D,0x0C,0x0B,0x0A,0x0A,0x09,0x07,0x07,0x07,0x07,0x06,0x06,0x06,0x05,0x05,
		//0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,
		//0x29,0x2A,0x2B,0x2C,0x2D,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,
		//0x05,0x05,0x04,
 	//};
 
 	//CopyMemory(cbRandCard, cbTempRandCard, sizeof(cbRandCard));
 #endif
	
	//比赛房间随机第一个随机叫分
	if ((m_pGameServiceOption->wServerType&GAME_GENRE_MATCH)!=0 )
	{
		m_wFirstUser = INVALID_CHAIR;
	}

	//抽取明牌
	BYTE cbValidCardData=0;
	BYTE cbValidCardIndex=0;
	WORD wStartUser=m_wFirstUser;
	WORD wCurrentUser=m_wFirstUser;

	//抽取玩家
	if (wStartUser==INVALID_CHAIR)
	{
		//抽取扑克
		cbValidCardIndex=rand()%DISPATCH_COUNT;
		cbValidCardData=cbRandCard[cbValidCardIndex];

		//设置用户
		wStartUser=m_GameLogic.GetCardValue(cbValidCardData)%GAME_PLAYER;
		wCurrentUser=(wStartUser+cbValidCardIndex/NORMAL_COUNT)%GAME_PLAYER;
	}

	
	//用户扑克
	for (WORD i=0;i<GAME_PLAYER;i++)
	{
		WORD wUserIndex=(wStartUser+i)%GAME_PLAYER;
		m_cbHandCardCount[wUserIndex]=NORMAL_COUNT;
		CopyMemory(&m_cbHandCardData[wUserIndex],&cbRandCard[i*m_cbHandCardCount[wUserIndex]],sizeof(BYTE)*m_cbHandCardCount[wUserIndex]);
	}

	//设置底牌
	CopyMemory(m_cbBankerCard,&cbRandCard[DISPATCH_COUNT],sizeof(m_cbBankerCard));

	//机器人数据
	CMD_S_AndroidCard AndroidCard ;
	ZeroMemory(&AndroidCard, sizeof(AndroidCard)) ;

	//用户扑克
	for (WORD i=0;i<GAME_PLAYER;i++)
	{
		WORD wUserIndex = (m_wFirstUser+i)%GAME_PLAYER;
		m_cbHandCardCount[wUserIndex]=NORMAL_COUNT;
		CopyMemory(&m_cbHandCardData[wUserIndex],&cbRandCard[i*m_cbHandCardCount[wUserIndex]],sizeof(BYTE)*m_cbHandCardCount[wUserIndex]);
		CopyMemory(&AndroidCard.cbHandCard[wUserIndex], &cbRandCard[i*m_cbHandCardCount[wUserIndex]], sizeof(BYTE)*m_cbHandCardCount[wUserIndex]) ;
	}

	//设置用户
	m_wFirstUser=wCurrentUser;
	m_wCurrentUser=wCurrentUser;

	//构造变量
	CMD_S_GameStart GameStart;
	GameStart.wStartUser=wStartUser;
	GameStart.wCurrentUser=wCurrentUser;
	GameStart.cbValidCardData=cbValidCardData;
	GameStart.cbValidCardIndex=cbValidCardIndex;

	AndroidCard.wCurrentUser = m_wCurrentUser;
	CopyMemory(AndroidCard.cbBankerCard,m_cbBankerCard,3*sizeof(BYTE));

	if(m_pGameVideo)
	{
		m_pGameVideo->StartVideo(m_pITableFrame);
	}
	//发送数据
	for (WORD i=0;i<GAME_PLAYER;i++)
	{
		//构造扑克
		ASSERT(CountArray(GameStart.cbCardData)>=m_cbHandCardCount[i]);
		CopyMemory(GameStart.cbCardData,m_cbHandCardData[i],sizeof(BYTE)*m_cbHandCardCount[i]);

		//发送数据
		IServerUserItem *pServerUserItem = m_pITableFrame->GetTableUserItem(i) ;
		if(!pServerUserItem->IsAndroidUser())
		{
			m_pITableFrame->SendTableData(i,SUB_S_GAME_START,&GameStart,sizeof(GameStart));
			m_pITableFrame->SendLookonData(i,SUB_S_GAME_START,&GameStart,sizeof(GameStart));
		}
		else 
		{
			m_pITableFrame->SendTableData(i,SUB_S_GAME_START,&AndroidCard,sizeof(AndroidCard));
			m_pITableFrame->SendLookonData(i,SUB_S_GAME_START,&GameStart,sizeof(GameStart));
		}
	
		if (m_pGameVideo)
		{
			Video_GameStart video;
			lstrcpyn(video.szNickName,pServerUserItem->GetNickName(),CountArray(video.szNickName));
			video.wTableID			= i;
			video.wStartUser		= wStartUser;
			video.wCurrentUser		= wCurrentUser;
			video.cbValidCardData	= cbValidCardData;
			video.cbValidCardIndex	= cbValidCardIndex;
			CopyMemory(video.cbCardData,m_cbHandCardData[i],sizeof(BYTE)*m_cbHandCardCount[i]);
			video.lScore			= pServerUserItem->GetUserScore();
			video.lCellScore		= m_pITableFrame->GetCellScore();;
			m_pGameVideo->AddVideoData(SUB_S_GAME_START,&video,i==0?true:false);
		}
	}

	//WB改
	SendCheatCard();

	//排列扑克
	for (WORD i=0;i<GAME_PLAYER;i++)
	{
		m_GameLogic.SortCardList(m_cbHandCardData[i],m_cbHandCardCount[i],ST_ORDER);
	}

	if(m_pITableFrame->GetTableUserItem(m_wCurrentUser)->IsTrusteeUser())
		IsOfflineTrustee();
	return true;
}

//游戏结束
bool CTableFrameSink::OnEventGameConclude(WORD wChairID, IServerUserItem * pIServerUserItem, BYTE cbReason)
{
	switch (cbReason)
	{
	case GER_NORMAL:		//常规结束
		{
			//变量定义
			CMD_S_GameConclude GameConclude;
			ZeroMemory(&GameConclude,sizeof(GameConclude));

			//设置变量
			GameConclude.cbBankerScore=m_cbBankerScore;
			GameConclude.lCellScore=m_pITableFrame->GetCellScore();

			//炸弹信息
			GameConclude.cbBombCount=m_cbBombCount;
			CopyMemory(GameConclude.cbEachBombCount,m_cbEachBombCount,sizeof(GameConclude.cbEachBombCount));

			//用户扑克
			BYTE cbCardIndex=0;
			for (WORD i=0;i<GAME_PLAYER;i++)
			{
				//拷贝扑克
				GameConclude.cbCardCount[i]=m_cbHandCardCount[i];
				CopyMemory(&GameConclude.cbHandCardData[cbCardIndex],m_cbHandCardData[i],m_cbHandCardCount[i]*sizeof(BYTE));

				//设置索引
				cbCardIndex+=m_cbHandCardCount[i];
			}

			//炸弹统计
			LONG lScoreTimes=1L * m_cbBankerScore;
			for (BYTE i=0;i<m_cbBombCount;i++) lScoreTimes*=2L;

			//春天判断
			if (wChairID==m_wBankerUser)
			{
				//用户定义
				WORD wUser1=(m_wBankerUser+1)%GAME_PLAYER;
				WORD wUser2=(m_wBankerUser+2)%GAME_PLAYER;

				//用户判断
				if ((m_cbOutCardCount[wUser1]==0)&&(m_cbOutCardCount[wUser2]==0)) 
				{
					lScoreTimes*=2L;
					GameConclude.bChunTian=TRUE;
				}
			}

			//春天判断
			if (wChairID!=m_wBankerUser)
			{
				if (m_cbOutCardCount[m_wBankerUser]==1)
				{
					lScoreTimes*=2L;
					GameConclude.bFanChunTian=TRUE;
				}
			}

			//调整倍数
			lScoreTimes=__min(m_pGameCustomRule->wMaxScoreTimes,lScoreTimes);

			m_lScoreTimes = lScoreTimes;

			//农民托管数量
			WORD wTrusteeCount=0;
			if ((m_pGameServiceOption->wServerType&GAME_GENRE_MATCH)!=0)
			{
				for(WORD i = 0; i < GAME_PLAYER; i++)
				{
					if (i!=m_wBankerUser)
					{
						if (m_pITableFrame->GetTableUserItem(i)->IsTrusteeUser()==true) wTrusteeCount++;
					}
				}
			}

			//积分变量
			tagScoreInfo ScoreInfoArray[GAME_PLAYER];
			ZeroMemory(&ScoreInfoArray,sizeof(ScoreInfoArray));

			//统计积分
			for (WORD i=0;i<GAME_PLAYER;i++)
			{
				//变量定义
				SCORE lUserScore=0L;
				SCORE lCellScore=m_pITableFrame->GetCellScore();
				bool bTrusteeUser=m_pITableFrame->GetTableUserItem(i)->IsTrusteeUser();
				
				//积分基数
				if (i==m_wBankerUser)
				{
					lUserScore=(m_cbHandCardCount[m_wBankerUser]==0)?2L:-2L;
				}
				else
				{
					if (m_cbHandCardCount[m_wBankerUser]==0)
					{
						if (wTrusteeCount>0) lUserScore=(bTrusteeUser==true)?(-2L/wTrusteeCount):0L;
						else lUserScore=-1L;
					}
					else
					{
						if (wTrusteeCount>0) lUserScore=(bTrusteeUser==true)?0L:(2L/wTrusteeCount);
						else lUserScore=1L;
					}
				}

				//计算积分
				ScoreInfoArray[i].lScore=lUserScore*lCellScore*lScoreTimes;
				ScoreInfoArray[i].cbType=(ScoreInfoArray[i].lScore>=0L)?SCORE_TYPE_WIN:SCORE_TYPE_LOSE;

				//计算税收
				ScoreInfoArray[i].lRevenue=m_pITableFrame->CalculateRevenue(i,ScoreInfoArray[i].lScore);
				if (ScoreInfoArray[i].lRevenue>0L) ScoreInfoArray[i].lScore-=ScoreInfoArray[i].lRevenue;

				//设置积分
				GameConclude.lGameScore[i]=ScoreInfoArray[i].lScore;

				//历史积分
				m_HistoryScore.OnEventUserScore(i,GameConclude.lGameScore[i]);

				//房卡模式
				if (((m_pGameServiceOption->wServerType & GAME_GENRE_PERSONAL) != 0) && (m_RoomCardRecord.nCount < MAX_RECORD_COUNT))
				{
					m_RoomCardRecord.lDetailScore[i][m_RoomCardRecord.nCount] = ScoreInfoArray[i].lScore;
				}
			}

			//房卡模式
			if ((m_pGameServiceOption->wServerType & GAME_GENRE_PERSONAL) != 0)
			{
				m_RoomCardRecord.nCount++;
			}

			GameConclude.lScoreTimes = m_lScoreTimes;

			//发送数据
			m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_GAME_CONCLUDE,&GameConclude,sizeof(GameConclude));
			m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GAME_CONCLUDE,&GameConclude,sizeof(GameConclude));

			if(m_pGameVideo)
			{
				m_pGameVideo->AddVideoData(SUB_S_GAME_CONCLUDE,&GameConclude);
				m_pGameVideo->StopAndSaveVideo(m_pGameServiceOption->wServerID,m_pITableFrame->GetTableID());
			}
			//写入积分
			m_pITableFrame->WriteTableScore(ScoreInfoArray,CountArray(ScoreInfoArray));

			// 检测发送喇叭
			CMD_GC_GTrumpet sTrumpet;
			for (WORD i=0; i<GAME_PLAYER; i++) {
				IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if(!pIServerUserItem) {
					continue;
				}
				if (GameConclude.lGameScore[i] > 0) {
					ZeroMemory(&sTrumpet, sizeof(sTrumpet));
					sTrumpet.wMultiple = GameConclude.lScoreTimes;
					sTrumpet.lScore = GameConclude.lGameScore[i];
					m_pITableFrame->SendUserItemData(pIServerUserItem, SUB_S_WIN_TRUMPET, &sTrumpet, sizeof(sTrumpet));
				}
			}

			//切换用户
			m_wFirstUser=wChairID;

			//结束游戏
			m_pITableFrame->ConcludeGame(GAME_SCENE_FREE);
				
			//房卡模式
			if (((m_pGameServiceOption->wServerType & GAME_GENRE_PERSONAL) != 0) && m_pITableFrame->IsPersonalRoomDisumme())
			{
				//约战解散
				ZeroMemory(&m_RoomCardRecord, sizeof(m_RoomCardRecord));
			}
			return true;
		}
	case GER_DISMISS:		//游戏解散
		{
			//变量定义
			CMD_S_GameConclude GameConclude;
			ZeroMemory(&GameConclude,sizeof(GameConclude));

			//用户扑克
			BYTE cbCardIndex=0;
			for (WORD i=0;i<GAME_PLAYER;i++)
			{
				//拷贝扑克
				GameConclude.cbCardCount[i]=m_cbHandCardCount[i];
				CopyMemory(&GameConclude.cbHandCardData[cbCardIndex],m_cbHandCardData[i],m_cbHandCardCount[i]*sizeof(BYTE));

				//设置索引
				cbCardIndex+=m_cbHandCardCount[i];
			}

			//发送信息
			m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_GAME_CONCLUDE,&GameConclude,sizeof(GameConclude));
			m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GAME_CONCLUDE,&GameConclude,sizeof(GameConclude));
			
			/*if(m_pGameVideo)
			{
				m_pGameVideo->AddVideoData(SUB_S_GAME_CONCLUDE,&GameConclude);
				m_pGameVideo->StopAndSaveVideo(m_pGameServiceOption->wServerID,m_pITableFrame->GetTableID());
			}*/
			//结束游戏
			m_pITableFrame->ConcludeGame(GAME_SCENE_FREE);

			return true;
		}
	case GER_USER_LEAVE:	//用户强退
	case GER_NETWORK_ERROR:	//网络中断
		{
			if ((m_pGameServiceOption->wServerType&GAME_GENRE_MATCH)!=0)
			{
				if(m_bOffLineTrustee)
				{
					IsOfflineTrustee();

					return true;
				}
			}

			//变量定义
			CMD_S_GameConclude GameConclude;
			ZeroMemory(&GameConclude,sizeof(GameConclude));

			//设置变量
			GameConclude.cbBankerScore=m_cbBankerScore;
			GameConclude.lCellScore=m_pITableFrame->GetCellScore();

			//炸弹信息
			GameConclude.cbBombCount=m_cbBombCount;
			CopyMemory(GameConclude.cbEachBombCount,m_cbEachBombCount,sizeof(GameConclude.cbEachBombCount));

			//用户扑克
			BYTE cbCardIndex=0;
			for (WORD i=0;i<GAME_PLAYER;i++)
			{
				//拷贝扑克
				GameConclude.cbCardCount[i]=m_cbHandCardCount[i];
				CopyMemory(&GameConclude.cbHandCardData[cbCardIndex],m_cbHandCardData[i],m_cbHandCardCount[i]*sizeof(BYTE));

				//设置索引
				cbCardIndex+=m_cbHandCardCount[i];
			}

			//炸弹统计
			WORD lScoreTimes=1;
			for (WORD i=0;i<m_cbBombCount;i++) lScoreTimes*=2L;

			//调整倍数
			lScoreTimes=__min(m_pGameCustomRule->wMaxScoreTimes,lScoreTimes);

			//积分变量
			tagScoreInfo ScoreInfoArray[GAME_PLAYER];
			ZeroMemory(&ScoreInfoArray,sizeof(ScoreInfoArray));

			//变量定义
			SCORE lCellScore=m_pITableFrame->GetCellScore();
			SCORE lUserScore=lCellScore*__max(lScoreTimes,m_pGameCustomRule->wFleeScoreTimes);

			//金币平衡
			if ((m_pGameServiceOption->wServerType&SCORE_GENRE_POSITIVE)!=0 || (m_pGameServiceOption->wServerType&(GAME_GENRE_GOLD|GAME_GENRE_EDUCATE))!=0)
			{
				lUserScore=__min(pIServerUserItem->GetUserScore(),lUserScore);
			}

			//逃跑罚分
			ScoreInfoArray[wChairID].lScore=-lUserScore;
			ScoreInfoArray[wChairID].cbType=SCORE_TYPE_FLEE;

			//分享罚分
			if (m_pGameCustomRule->cbFleeScorePatch==TRUE)
			{
				for (WORD i=0;i<GAME_PLAYER;i++)
				{
					//过滤
					if ( i == wChairID )
						continue;

					//设置积分
					ScoreInfoArray[i].lScore=lUserScore/2L;
					ScoreInfoArray[i].cbType=SCORE_TYPE_WIN;

					//计算税收
					ScoreInfoArray[i].lRevenue=m_pITableFrame->CalculateRevenue(i,ScoreInfoArray[i].lScore);
					if (ScoreInfoArray[i].lRevenue>0L) ScoreInfoArray[i].lScore-=ScoreInfoArray[i].lRevenue;
				}
			}

			//历史积分
			for (WORD i=0;i<GAME_PLAYER;i++)
			{
				//设置成绩
				GameConclude.lGameScore[i]=ScoreInfoArray[i].lScore;

				//历史成绩
				m_HistoryScore.OnEventUserScore(i,GameConclude.lGameScore[i]);
			}

			//发送信息
			m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_GAME_CONCLUDE,&GameConclude,sizeof(GameConclude));
			m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GAME_CONCLUDE,&GameConclude,sizeof(GameConclude));
			/*
			if(m_pGameVideo)
			{
				m_pGameVideo->AddVideoData(SUB_S_GAME_CONCLUDE,&GameConclude);
				m_pGameVideo->StopAndSaveVideo(m_pGameServiceOption->wServerID,m_pITableFrame->GetTableID());
			}*/
			//写入积分
			m_pITableFrame->WriteTableScore(ScoreInfoArray,CountArray(ScoreInfoArray));

			//结束游戏
			m_pITableFrame->ConcludeGame(GAME_SCENE_FREE);

			return true;
		}
	}

	//错误断言
	ASSERT(FALSE);

	return false;
}

//发送场景
bool CTableFrameSink::OnEventSendGameScene(WORD wChairID, IServerUserItem * pIServerUserItem, BYTE cbGameStatus, bool bSendSecret)
{
	switch (cbGameStatus)
	{
	case GAME_SCENE_FREE:	//空闲状态
		{
			//构造数据
			CMD_S_StatusFree StatusFree;
			ZeroMemory(&StatusFree,sizeof(StatusFree));

			//设置变量
			StatusFree.lCellScore=m_pITableFrame->GetCellScore();

			//自定规则
			StatusFree.cbTimeOutCard=m_pGameCustomRule->cbTimeOutCard;		
			StatusFree.cbTimeCallScore=m_pGameCustomRule->cbTimeCallScore;
			StatusFree.cbTimeStartGame=m_pGameCustomRule->cbTimeStartGame;
			StatusFree.cbTimeHeadOutCard=m_pGameCustomRule->cbTimeHeadOutCard;

			//历史积分
			for (WORD i=0;i<GAME_PLAYER;i++)
			{
				//变量定义
				tagHistoryScore * pHistoryScore=m_HistoryScore.GetHistoryScore(i);

				//设置变量
				StatusFree.lTurnScore[i]=pHistoryScore->lTurnScore;
				StatusFree.lCollectScore[i]=pHistoryScore->lCollectScore;
			}

			//发送场景
			return m_pITableFrame->SendGameScene(pIServerUserItem,&StatusFree,sizeof(StatusFree));
		}
	case GAME_SCENE_CALL:	//叫分状态
		{
			//构造数据
			CMD_S_StatusCall StatusCall;
			ZeroMemory(&StatusCall,sizeof(StatusCall));

			//单元积分
			StatusCall.lCellScore=m_pITableFrame->GetCellScore();

			//自定规则
			StatusCall.cbTimeOutCard=m_pGameCustomRule->cbTimeOutCard;		
			StatusCall.cbTimeCallScore=m_pGameCustomRule->cbTimeCallScore;
			StatusCall.cbTimeStartGame=m_pGameCustomRule->cbTimeStartGame;
			StatusCall.cbTimeHeadOutCard=m_pGameCustomRule->cbTimeHeadOutCard;

			//游戏信息
			StatusCall.wCurrentUser=m_wCurrentUser;
			StatusCall.cbBankerScore=m_cbBankerScore;
			CopyMemory(StatusCall.cbScoreInfo,m_cbScoreInfo,sizeof(m_cbScoreInfo));
			CopyMemory(StatusCall.cbHandCardData,m_cbHandCardData[wChairID],m_cbHandCardCount[wChairID]*sizeof(BYTE));
			CopyMemory(StatusCall.cbUserTrustee,m_cbUserTrustee,sizeof(m_cbUserTrustee));

			StatusCall.lScoreTimes = m_lScoreTimes;

			//历史积分
			for (WORD i=0;i<GAME_PLAYER;i++)
			{
				//变量定义
				tagHistoryScore * pHistoryScore=m_HistoryScore.GetHistoryScore(i);

				//设置变量
				StatusCall.lTurnScore[i]=pHistoryScore->lTurnScore;
				StatusCall.lCollectScore[i]=pHistoryScore->lCollectScore;
			}

			//发送场景
			bool bSendResult = m_pITableFrame->SendGameScene(pIServerUserItem,&StatusCall,sizeof(StatusCall));

			if(pIServerUserItem)
			{
				//作弊用户
				if(CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()))
				{
					OnCheatCard(pIServerUserItem);
				}
			}
			return bSendResult;
		}
	case GAME_SCENE_PLAY:	//游戏状态
		{
			//构造数据
			CMD_S_StatusPlay StatusPlay;
			ZeroMemory(&StatusPlay,sizeof(StatusPlay));

			//单元积分
			StatusPlay.lCellScore=m_pITableFrame->GetCellScore();

			//游戏变量
			StatusPlay.cbBombCount=m_cbBombCount;
			StatusPlay.wBankerUser=m_wBankerUser;
			StatusPlay.wCurrentUser=m_wCurrentUser;
			StatusPlay.cbBankerScore=m_cbBankerScore;

			//自定规则
			StatusPlay.cbTimeOutCard=m_pGameCustomRule->cbTimeOutCard;		
			StatusPlay.cbTimeCallScore=m_pGameCustomRule->cbTimeCallScore;
			StatusPlay.cbTimeStartGame=m_pGameCustomRule->cbTimeStartGame;
			StatusPlay.cbTimeHeadOutCard=m_pGameCustomRule->cbTimeHeadOutCard;

			//出牌信息
			StatusPlay.wTurnWiner=m_wTurnWiner;
			StatusPlay.cbTurnCardCount=m_cbTurnCardCount;
			CopyMemory(StatusPlay.cbTurnCardData,m_cbTurnCardData,m_cbTurnCardCount*sizeof(BYTE));

			//扑克信息
			CopyMemory(StatusPlay.cbBankerCard,m_cbBankerCard,sizeof(m_cbBankerCard));
			CopyMemory(StatusPlay.cbHandCardCount,m_cbHandCardCount,sizeof(m_cbHandCardCount));
			CopyMemory(StatusPlay.cbHandCardData,m_cbHandCardData[wChairID],sizeof(BYTE)*m_cbHandCardCount[wChairID]);
			CopyMemory(StatusPlay.cbUserTrustee,m_cbUserTrustee,sizeof(m_cbUserTrustee));
			StatusPlay.lScoreTimes = m_lScoreTimes;

			//历史积分
			for (WORD i=0;i<GAME_PLAYER;i++)
			{
				//变量定义
				tagHistoryScore * pHistoryScore=m_HistoryScore.GetHistoryScore(i);

				//设置变量
				StatusPlay.lTurnScore[i]=pHistoryScore->lTurnScore;
				StatusPlay.lCollectScore[i]=pHistoryScore->lCollectScore;
			}

			//发送场景
			bool bSendResult = m_pITableFrame->SendGameScene(pIServerUserItem,&StatusPlay,sizeof(StatusPlay));

			if(pIServerUserItem)
			{
				//作弊用户
				if(CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()))
				{
					OnCheatCard(pIServerUserItem);
				}
			}
			return bSendResult;
		}
	}

	//错误断言
	ASSERT(FALSE);

	return false;
}

//时间事件
bool CTableFrameSink::OnTimerMessage(DWORD wTimerID, WPARAM wBindParam)
{
	if((m_pGameServiceOption->wServerType&GAME_GENRE_MATCH)!=0)
	{
		if(wTimerID==IDI_USER_OUT_TIME)
		{
			m_pITableFrame->KillGameTimer(IDI_USER_OUT_TIME);

			if (m_wCurrentUser==INVALID_CHAIR) return true;

			IServerUserItem *pServerUserItem=m_pITableFrame->GetTableUserItem(m_wCurrentUser);
			ASSERT(pServerUserItem!=NULL);
			if (pServerUserItem==NULL) return false;

			bool bTrusteeUser=pServerUserItem->IsTrusteeUser();
			BYTE cbGameStatus = m_pITableFrame->GetGameStatus();
			switch(cbGameStatus)
			{
			case GAME_SCENE_CALL:
				{
					if (m_cbBankerScore==0)
						OnUserCallScore(m_wCurrentUser,0x01);
					else
						OnUserCallScore(m_wCurrentUser,0xFF);
					break;
				}
			case GAME_SCENE_PLAY:
				{
					if(m_cbTurnCardCount == 0)
					{
						tagSearchCardResult SearchCardResult;
						m_GameLogic.SearchOutCard( m_cbHandCardData[m_wCurrentUser],m_cbHandCardCount[m_wCurrentUser],NULL,0,&SearchCardResult);
						if(SearchCardResult.cbCardCount > 0)
						{
							OnUserOutCard(m_wCurrentUser, SearchCardResult.cbResultCard[0], SearchCardResult.cbCardCount[0]);
						}
						else
						{
							ASSERT(FALSE);
						}

					}
					else
					{
						OnUserPassCard(m_wCurrentUser);
					}					
					break;
				}
			}
			return true;
		}
	}	

	return false;
}

//数据事件
bool CTableFrameSink::OnDataBaseMessage(WORD wRequestID, VOID * pData, WORD wDataSize)
{
	return false;
}

//积分事件
bool CTableFrameSink::OnUserScroeNotify(WORD wChairID, IServerUserItem * pIServerUserItem, BYTE cbReason)
{
	return false;
}

//游戏消息
bool CTableFrameSink::OnGameMessage(WORD wSubCmdID, VOID * pData, WORD wDataSize, IServerUserItem * pIServerUserItem)
{
	m_cbWaitTime=0;
	switch (wSubCmdID)
	{
	case SUB_C_CALL_SCORE:	//用户叫分
		{
			//效验数据
			ASSERT(wDataSize==sizeof(CMD_C_CallScore));
			if (wDataSize!=sizeof(CMD_C_CallScore)) return false;

			//状态效验
			//ASSERT(m_pITableFrame->GetGameStatus()==GAME_SCENE_CALL);
			if (m_pITableFrame->GetGameStatus()!=GAME_SCENE_CALL) return true;

			//用户效验
			//ASSERT(pIServerUserItem->GetUserStatus()==US_PLAYING);
			if (pIServerUserItem->GetUserStatus()!=US_PLAYING) return true;

			//变量定义
			CMD_C_CallScore * pCallScore=(CMD_C_CallScore *)pData;

			//消息处理
			WORD wChairID=pIServerUserItem->GetChairID();
			return OnUserCallScore(wChairID,pCallScore->cbCallScore);
		}
	case SUB_C_OUT_CARD:	//用户出牌
		{
			//变量定义
			CMD_C_OutCard * pOutCard=(CMD_C_OutCard *)pData;
			WORD wHeadSize=sizeof(CMD_C_OutCard)-sizeof(pOutCard->cbCardData);

			//效验数据
			ASSERT((wDataSize>=wHeadSize)&&(wDataSize==(wHeadSize+pOutCard->cbCardCount*sizeof(BYTE))));
			if ((wDataSize<wHeadSize)||(wDataSize!=(wHeadSize+pOutCard->cbCardCount*sizeof(BYTE)))) return false;

			//状态效验
			//ASSERT(m_pITableFrame->GetGameStatus()==GAME_SCENE_PLAY);
			if (m_pITableFrame->GetGameStatus()!=GAME_SCENE_PLAY) return true;

			//用户效验
			//ASSERT(pIServerUserItem->GetUserStatus()==US_PLAYING);
			if (pIServerUserItem->GetUserStatus()!=US_PLAYING) return true;

			//消息处理
			WORD wChairID=pIServerUserItem->GetChairID();
			return OnUserOutCard(wChairID,pOutCard->cbCardData,pOutCard->cbCardCount);
		}
	case SUB_C_PASS_CARD:	//用户放弃
		{
			//状态效验
			//ASSERT(m_pITableFrame->GetGameStatus()==GAME_SCENE_PLAY);
			if (m_pITableFrame->GetGameStatus()!=GAME_SCENE_PLAY) return true;

			//用户效验
			//ASSERT(pIServerUserItem->GetUserStatus()==US_PLAYING);
			if (pIServerUserItem->GetUserStatus()!=US_PLAYING) return true;

			//消息处理
			return OnUserPassCard(pIServerUserItem->GetChairID());
		}
	case SUB_C_TRUSTEE:
		{
			//效验数据
			ASSERT(wDataSize==sizeof(CMD_C_TRUSTEE));
			if (wDataSize!=sizeof(CMD_C_TRUSTEE)) return false;

			CMD_C_TRUSTEE* pCTrustee = (CMD_C_TRUSTEE*)pData;
			CMD_S_TRUSTEE pSTrustee;
			pSTrustee.wTrusteeUser = pIServerUserItem->GetChairID();
			pSTrustee.bTrustee = pCTrustee->bTrustee;

			m_cbUserTrustee[pIServerUserItem->GetChairID()] = pCTrustee->bTrustee;

			m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_TRUSTEE,&pSTrustee,sizeof(CMD_S_TRUSTEE));
			m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_TRUSTEE,&pSTrustee,sizeof(CMD_S_TRUSTEE));
			return true;
		}
	case SUB_C_REQUEST_RCRecord:
		{
			ASSERT (((m_pGameServiceOption->wServerType & GAME_GENRE_PERSONAL) !=0)/* && (m_pITableFrame->IsPersonalRoomDisumme())*/);
			if (!(((m_pGameServiceOption->wServerType & GAME_GENRE_PERSONAL) !=0)/* && (m_pITableFrame->IsPersonalRoomDisumme())*/))
			{
				return false;
			}

			ASSERT (pIServerUserItem->IsMobileUser());
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
	}

	return false;
}

//框架消息
bool CTableFrameSink::OnFrameMessage(WORD wSubCmdID, VOID * pData, WORD wDataSize, IServerUserItem * pIServerUserItem)
{
	return false;
}

//用户断线
bool CTableFrameSink::OnActionUserOffLine(WORD wChairID, IServerUserItem * pIServerUserItem)
{
	if(m_bOffLineTrustee && wChairID == m_wCurrentUser)
	{
		IsOfflineTrustee();
	}
	return true;
}

//用户重入
bool CTableFrameSink::OnActionUserConnect(WORD wChairID, IServerUserItem * pIServerUserItem)
{
	if(m_bOffLineTrustee)
	{
		if (((m_pGameServiceOption->wServerType&GAME_GENRE_MATCH)!=0)&&(pIServerUserItem->GetChairID()==m_wCurrentUser))
		{
			m_pITableFrame->KillGameTimer(IDI_USER_OUT_TIME);
		}
	}
	return true; 
}

//用户坐下
bool CTableFrameSink::OnActionUserSitDown(WORD wChairID, IServerUserItem * pIServerUserItem, bool bLookonUser)
{
	//历史积分
	if (bLookonUser==false)
	{
		ASSERT(wChairID!=INVALID_CHAIR);
		m_HistoryScore.OnEventUserEnter(wChairID);
	}

	return true;
}

//用户起立
bool CTableFrameSink::OnActionUserStandUp(WORD wChairID, IServerUserItem * pIServerUserItem, bool bLookonUser)
{
	//历史积分
	if (bLookonUser==false)
	{
		ASSERT(wChairID!=INVALID_CHAIR);
		m_HistoryScore.OnEventUserLeave(wChairID);
	}
	m_cbUserTrustee[wChairID] = 0;

	//约战解散
	if (((m_pGameServiceOption->wServerType & GAME_GENRE_PERSONAL) != 0) && m_pITableFrame->IsPersonalRoomDisumme())
	{
		ZeroMemory(&m_RoomCardRecord, sizeof(m_RoomCardRecord));
	}

	return true;
}

//用户放弃
bool CTableFrameSink::OnUserPassCard(WORD wChairID)
{
	//效验状态
	ASSERT((wChairID==m_wCurrentUser)&&(m_cbTurnCardCount!=0));
	if ((wChairID!=m_wCurrentUser)||(m_cbTurnCardCount==0)) return true;

	//设置变量
	m_wCurrentUser=(m_wCurrentUser+1)%GAME_PLAYER;
	if (m_wCurrentUser==m_wTurnWiner) m_cbTurnCardCount=0;

	//构造消息
	CMD_S_PassCard PassCard;
	PassCard.wPassCardUser=wChairID;
	PassCard.wCurrentUser=m_wCurrentUser;
	PassCard.cbTurnOver=(m_cbTurnCardCount==0)?TRUE:FALSE;

	//发送数据
	m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_PASS_CARD,&PassCard,sizeof(PassCard));
	m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_PASS_CARD,&PassCard,sizeof(PassCard));

	if (m_pGameVideo)
	{		
		m_pGameVideo->AddVideoData(SUB_S_PASS_CARD,&PassCard);
	}

	if(m_pITableFrame->GetTableUserItem(m_wCurrentUser)->IsTrusteeUser())
		IsOfflineTrustee();

	return true;
}

//用户叫分
bool CTableFrameSink::OnUserCallScore(WORD wChairID, BYTE cbCallScore)
{
	//效验状态
	ASSERT(wChairID==m_wCurrentUser);
	if (wChairID!=m_wCurrentUser) return true;

	//效验参数
	//ASSERT(((cbCallScore>=1)&&(cbCallScore<=3)&&(cbCallScore>m_cbBankerScore))||(cbCallScore==255));
	if (((cbCallScore<1)||(cbCallScore>3)||(cbCallScore<=m_cbBankerScore))&&(cbCallScore!=255))
		cbCallScore = 255;

	//设置状态
	if (cbCallScore!=0xFF)
	{
		m_cbBankerScore=cbCallScore;
		m_wBankerUser=m_wCurrentUser;
	}

	//设置叫分
	m_cbScoreInfo[wChairID]=cbCallScore;

	//设置用户
	if ((m_cbBankerScore==3)||(m_wFirstUser==(wChairID+1)%GAME_PLAYER))
	{
		m_wCurrentUser=INVALID_CHAIR;
	}
	else
	{
		m_wCurrentUser=(wChairID+1)%GAME_PLAYER;
	}

	//构造变量
	CMD_S_CallScore CallScore;
	CallScore.wCallScoreUser=wChairID;
	CallScore.wCurrentUser=m_wCurrentUser;
	CallScore.cbUserCallScore=cbCallScore;
	CallScore.cbCurrentScore=m_cbBankerScore;

	//发送消息
	m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_CALL_SCORE,&CallScore,sizeof(CallScore));
	m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_CALL_SCORE,&CallScore,sizeof(CallScore));

	if (m_pGameVideo)
	{		
		m_pGameVideo->AddVideoData(SUB_S_CALL_SCORE,&CallScore);
	}

	//开始判断
	if ((m_cbBankerScore==3)||(m_wFirstUser==(wChairID+1)%GAME_PLAYER))
	{
		//无人叫分
		if ( m_cbBankerScore == 0 )
		{
			if ((m_pGameServiceOption->wServerType&GAME_GENRE_MATCH)==0)
			{
				if(m_cbCallScoreCount<2)
				{
					m_cbCallScoreCount++;
					return OnEventGameStart();
				}
			}
			m_wBankerUser=m_wFirstUser;
			m_cbBankerScore=1;
		}

		//设置状态
		m_pITableFrame->SetGameStatus(GAME_SCENE_PLAY);

		//设置变量
		if (m_cbBankerScore==0) m_cbBankerScore=1;
		if (m_wBankerUser==INVALID_CHAIR) m_wBankerUser=m_wFirstUser;
		m_cbCallScoreCount=0;

		//发送底牌
		m_cbHandCardCount[m_wBankerUser]+=CountArray(m_cbBankerCard);
		CopyMemory(&m_cbHandCardData[m_wBankerUser][NORMAL_COUNT],m_cbBankerCard,sizeof(m_cbBankerCard));

		//排列扑克
		m_GameLogic.SortCardList(m_cbHandCardData[m_wBankerUser],m_cbHandCardCount[m_wBankerUser],ST_ORDER);


		//设置用户
		m_wTurnWiner=m_wBankerUser;
		m_wCurrentUser=m_wBankerUser;

		//发送消息
		CMD_S_BankerInfo BankerInfo;
		BankerInfo.wBankerUser=m_wBankerUser;
		BankerInfo.wCurrentUser=m_wCurrentUser;
		BankerInfo.cbBankerScore=m_cbBankerScore;
		CopyMemory(BankerInfo.cbBankerCard,m_cbBankerCard,sizeof(m_cbBankerCard));
		
		//游戏倍数
		m_lScoreTimes = m_cbBankerScore;
		BankerInfo.lScoreTimes = m_lScoreTimes;

		//发送消息
		m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_BANKER_INFO,&BankerInfo,sizeof(BankerInfo));
		m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_BANKER_INFO,&BankerInfo,sizeof(BankerInfo));

		if (m_pGameVideo)
		{		
			m_pGameVideo->AddVideoData(SUB_S_BANKER_INFO,&BankerInfo);
		}

		if(m_pITableFrame->GetTableUserItem(m_wCurrentUser)->IsTrusteeUser())
			IsOfflineTrustee();

		return true;

	}
	if(m_pITableFrame->GetTableUserItem(m_wCurrentUser)->IsTrusteeUser())
		IsOfflineTrustee();

	return true;
}

//用户出牌
bool CTableFrameSink::OnUserOutCard(WORD wChairID, BYTE cbCardData[], BYTE cbCardCount)
{
	//效验状态
	ASSERT(wChairID==m_wCurrentUser);
	if (wChairID!=m_wCurrentUser) return true;

	//获取类型
	BYTE cbCardType=m_GameLogic.GetCardType(cbCardData,cbCardCount);

	//类型判断
	if (cbCardType==CT_ERROR) 
	{
		ASSERT(FALSE);
		return false;
	}

	//出牌判断
	if (m_cbTurnCardCount!=0)
	{
		//对比扑克
		if (m_GameLogic.CompareCard(m_cbTurnCardData,cbCardData,m_cbTurnCardCount,cbCardCount)==false)
		{
			ASSERT(FALSE);
			return false;
		}
	}

	//删除扑克
	if (m_GameLogic.RemoveCardList(cbCardData,cbCardCount,m_cbHandCardData[wChairID],m_cbHandCardCount[wChairID])==false)
	{
		ASSERT(FALSE);
		return false;
	}

	//出牌变量
	m_cbOutCardCount[wChairID]++;

	//设置变量
	m_cbTurnCardCount=cbCardCount;
	m_cbHandCardCount[wChairID]-=cbCardCount;
	CopyMemory(m_cbTurnCardData,cbCardData,sizeof(BYTE)*cbCardCount);

	//炸弹判断
	if ((cbCardType==CT_BOMB_CARD)||(cbCardType==CT_MISSILE_CARD)) 
	{
		m_cbBombCount++;
		m_cbEachBombCount[wChairID]++;

		m_lScoreTimes *= 2;
	}
	
	//切换用户
	m_wTurnWiner=wChairID;
	if (m_cbHandCardCount[wChairID]!=0)
	{
		if (cbCardType!=CT_MISSILE_CARD)
		{
			m_wCurrentUser=(m_wCurrentUser+1)%GAME_PLAYER;
		}
	}
	else m_wCurrentUser=INVALID_CHAIR;

	//构造数据
	CMD_S_OutCard OutCard;
	OutCard.wOutCardUser=wChairID;
	OutCard.cbCardCount=cbCardCount;
	OutCard.wCurrentUser=m_wCurrentUser;
	CopyMemory(OutCard.cbCardData,m_cbTurnCardData,m_cbTurnCardCount*sizeof(BYTE));
	OutCard.lScoreTimes = m_lScoreTimes;

	//发送数据
	WORD wHeadSize=sizeof(OutCard)-sizeof(OutCard.cbCardData);
	WORD wSendSize=wHeadSize+OutCard.cbCardCount*sizeof(BYTE);
	m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_OUT_CARD,&OutCard,wSendSize);
	m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_OUT_CARD,&OutCard,wSendSize);
	if (m_pGameVideo)
	{
		m_pGameVideo->AddVideoData(SUB_S_OUT_CARD,&OutCard);
	}
	//出牌最大
	if (cbCardType==CT_MISSILE_CARD) m_cbTurnCardCount=0;

	//结束判断
	if (m_wCurrentUser==INVALID_CHAIR) OnEventGameConclude(wChairID,NULL,GER_NORMAL);
	else
	{
		if(m_pITableFrame->GetTableUserItem(m_wCurrentUser)->IsTrusteeUser())
			IsOfflineTrustee();
	}

	return true;
}

//作弊用户
bool CTableFrameSink::OnCheatCard(IServerUserItem * pIServerUserItem)
{
	if(m_pServerControl)
	{
		CMD_S_CheatCard CheatCard ;
		ZeroMemory(&CheatCard, sizeof(CheatCard));

		for(WORD i = 0; i < GAME_PLAYER; i++)
		{
			CheatCard.wCardUser[CheatCard.cbUserCount] = i;
			CheatCard.cbCardCount[CheatCard.cbUserCount] = m_cbHandCardCount[i];
			CopyMemory(CheatCard.cbCardData[CheatCard.cbUserCount++],m_cbHandCardData[i],sizeof(BYTE)*m_cbHandCardCount[i]);
		}
		//发送作弊
		//WB改 这里这样发包存在BUG
		//m_pITableFrame->SendTableData(wChairID,SUB_S_CHEAT_CARD,&CheatCard,sizeof(CheatCard));
		//m_pITableFrame->SendLookonData(wChairID,SUB_S_CHEAT_CARD,&CheatCard,sizeof(CheatCard));
		m_pServerControl->ServerControl(&CheatCard, m_pITableFrame, pIServerUserItem);
		
	}

	return true;
}

//发送作弊信息
void CTableFrameSink::SendCheatCard()
{
	if(m_pServerControl)
	{
		CMD_S_CheatCard CheatCard ;
		ZeroMemory(&CheatCard, sizeof(CheatCard));

		for(WORD i = 0; i < GAME_PLAYER; i++)
		{
			CheatCard.wCardUser[CheatCard.cbUserCount] = i;
			CheatCard.cbCardCount[CheatCard.cbUserCount] = m_cbHandCardCount[i];
			CopyMemory(CheatCard.cbCardData[CheatCard.cbUserCount++],m_cbHandCardData[i],sizeof(BYTE)*m_cbHandCardCount[i]);
		}
		m_pServerControl->ServerControl(&CheatCard, m_pITableFrame);
	}

	return;
}
//设置基数
void CTableFrameSink::SetGameBaseScore(LONG lBaseScore)
{
	//发送数据
	m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_SET_BASESCORE,&lBaseScore,sizeof(lBaseScore));
	m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_SET_BASESCORE,&lBaseScore,sizeof(lBaseScore));
}

//是否托管
bool CTableFrameSink::IsOfflineTrustee()
{
	//非比赛模式
	if((m_pGameServiceOption->wServerType&GAME_GENRE_MATCH)==0) return false;
	//非常规座位
	if(m_wCurrentUser==INVALID_CHAIR) return false;

	//允许代打
	if (m_bOffLineTrustee)
	{
		IServerUserItem *pServerUserItem=m_pITableFrame->GetTableUserItem(m_wCurrentUser);
		ASSERT(pServerUserItem!=NULL);
		if (pServerUserItem==NULL) return false;

		//删除超时定时器
		m_pITableFrame->KillGameTimer(IDI_USER_OUT_TIME);

		//变量定义
		bool bTrusteeUser=pServerUserItem->IsTrusteeUser();
		DWORD dwTimerTime=3;
		
		if (bTrusteeUser==true)
		{
			dwTimerTime+=(rand()%3);
		}
		else
		{
			BYTE cbGameStatus=m_pITableFrame->GetGameStatus();
			if (cbGameStatus==GAME_SCENE_CALL) 
			{
				dwTimerTime=m_pGameCustomRule->cbTimeCallScore;
			}
			if (cbGameStatus==GAME_SCENE_PLAY)
			{
				if (m_cbTurnCardCount==0)
					dwTimerTime=m_pGameCustomRule->cbTimeHeadOutCard;
				else
					dwTimerTime=m_pGameCustomRule->cbTimeOutCard;
			}

			dwTimerTime+=(rand()%3+5);
		}

		m_pITableFrame->SetGameTimer(IDI_USER_OUT_TIME,dwTimerTime*1000,-1,NULL);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////
