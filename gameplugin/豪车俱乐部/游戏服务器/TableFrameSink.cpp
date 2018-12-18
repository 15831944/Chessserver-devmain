﻿#include "StdAfx.h"
#include "TableFrameSink.h"
//#include <locale>
#include "DlgCustomRule.h"
//////////////////////////////////////////////////////////////////////////

//常量定义
#define SEND_COUNT					300									//发送次数

//索引定义
#define INDEX_PLAYER				0									//闲家索引
#define INDEX_BANKER				1									//庄家索引

//下注时间
#define IDI_FREE					1									//空闲时间
#define TIME_FREE					5									//空闲时间

//下注时间
#define IDI_PLACE_JETTON			2									//下注时间

#define TIME_PLACE_JETTON			15									//下注时间

//结束时间
#define IDI_GAME_END				3									//结束时间
#define TIME_GAME_END				20									//结束时间
#define TIME_GAME_ADD               10                                  //附加时间
//////////////////////////////////////////////////////////////////////////

//静态变量
const WORD			CTableFrameSink::m_wPlayerCount=GAME_PLAYER;				//游戏人数
//ConsoleWindow	    CTableFrameSink::m_DebugWindow;
const LONGLONG BANKER_DEFSCORE = 1000000000;
//////////////////////////////////////////////////////////////////////////

//构造函数
CTableFrameSink::CTableFrameSink()
{
	//总下注数
	ZeroMemory(m_lAllJettonScore,sizeof(m_lAllJettonScore));

	//个人下注
	ZeroMemory(m_lUserJettonScore,sizeof(m_lUserJettonScore));


	m_pServerControl=NULL;
	m_cbControlArea = 0xff;
	m_cbControlTimes = 0;

	m_lStorageMax1 = 0;
	m_lStorageMul1 = 0;
	m_lStorageMax2 = 0;
	m_lStorageMul2 = 0;
	//玩家成绩	
	ZeroMemory(m_lUserWinScore,sizeof(m_lUserWinScore));
	ZeroMemory(m_lUserReturnScore,sizeof(m_lUserReturnScore));
	ZeroMemory(m_lUserRevenue,sizeof(m_lUserRevenue));
	//起始分数
	ZeroMemory(m_lUserStartScore,sizeof(m_lUserStartScore));
	//扑克信息
	m_cbTableCard = 0;
	//状态变量
	m_dwJettonTime=0L;
      m_nRobotListMaxCount =0;
	//庄家信息
	m_ApplyUserArray.RemoveAll();
	m_wCurrentBanker=INVALID_CHAIR;
	m_wBankerTime=0;
	m_lBankerWinScore=0L;		
	m_lBankerCurGameScore=0L;
	m_bEnableSysBanker = false;
		
	//记录变量
	ZeroMemory(m_GameRecordArrary,sizeof(m_GameRecordArrary));
	m_nRecordFirst=0;
	m_nRecordLast=0;

	//控制变量
	m_lStorageCurrent = 0l;
	m_StorageDeduct = 0l;

	//机器人控制					
	m_lRobotAreaLimit = 0l;
	m_lRobotBetCount = 0l;										

	//庄家设置
	m_lBankerMAX = 0l;
	m_lBankerAdd = 0l;							
	m_lBankerScoreMAX = 0l;
	m_lBankerScoreAdd = 0l;
	m_lPlayerBankerMAX = 0l;
	m_bExchangeBanker = true;

	//时间控制
	m_cbFreeTime = TIME_FREE;
	m_cbBetTime = TIME_PLACE_JETTON;
	m_cbEndTime = TIME_GAME_END;

	m_cbAddTime = TIME_GAME_ADD;
	//机器人控制
	m_nChipRobotCount = 0;
	ZeroMemory(m_lRobotAreaScore, sizeof(m_lRobotAreaScore));
	ZeroMemory(m_nAnimalTimes, sizeof(m_nAnimalTimes));

	srand(GetTickCount());

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
}

//接口查询
void *  CTableFrameSink::QueryInterface(const IID & Guid, DWORD dwQueryVer)
{

	QUERYINTERFACE(ITableFrameSink,Guid,dwQueryVer);
	QUERYINTERFACE(ITableUserAction,Guid,dwQueryVer);	
#ifdef __BANKER___
	QUERYINTERFACE(ITableUserActionEX,Guid,dwQueryVer);	
#endif
	QUERYINTERFACE_IUNKNOWNEX(ITableFrameSink,Guid,dwQueryVer);
	return NULL;

}

//初始化
bool  CTableFrameSink::Initialization(IUnknownEx * pIUnknownEx)
{

	//查询接口
	ASSERT(pIUnknownEx!=NULL);
	m_pITableFrame=QUERY_OBJECT_PTR_INTERFACE(pIUnknownEx,ITableFrame);
	if (m_pITableFrame==NULL) return false;

	//查询配置
	m_pGameServiceOption=m_pITableFrame->GetGameServiceOption();
	m_pGameServiceAttrib=m_pITableFrame->GetGameServiceAttrib();

	//开始模式
	//m_pITableFrame->SetStartMode(START_MODE_ALL_READY);

	m_pITableFrame->SetStartMode(START_MODE_TIME_CONTROL);
      ReadConfigInformation();
	//服务控制
	m_hInst = NULL;
	m_pServerControl = NULL;
	m_hInst = LoadLibrary(TEXT("LuxuryCarServerControl.dll"));
	if ( m_hInst )
	{
		typedef void * (*CREATE)(); 
		CREATE ServerControl = (CREATE)GetProcAddress(m_hInst,"CreateServerControl"); 
		if ( ServerControl )
		{
			m_pServerControl = static_cast<IServerControl*>(ServerControl());
		}
	}
	

	return true;
}

//复位桌子
VOID  CTableFrameSink::RepositionSink()
{
	//总下注数
	ZeroMemory(m_lAllJettonScore,sizeof(m_lAllJettonScore));
	
	//个人下注
	ZeroMemory(m_lUserJettonScore,sizeof(m_lUserJettonScore));

	//玩家成绩	
	ZeroMemory(m_lUserWinScore,sizeof(m_lUserWinScore));
	ZeroMemory(m_lUserReturnScore,sizeof(m_lUserReturnScore));
	ZeroMemory(m_lUserRevenue,sizeof(m_lUserRevenue));

	//机器人控制
	m_nChipRobotCount = 0;
	ZeroMemory(m_lRobotAreaScore, sizeof(m_lRobotAreaScore));
	//m_cbTableCard = 0;
	return;
}

//游戏状态
bool  CTableFrameSink::IsUserPlaying(WORD wChairID)
{
	return true;
}

//查询限额
SCORE CTableFrameSink::QueryConsumeQuota(IServerUserItem * pIServerUserItem)
{
	if(pIServerUserItem->GetUserStatus() == US_PLAYING)
	{
		return 0L;
	}
	else
	{
		return __max(pIServerUserItem->GetUserScore()-m_pGameServiceOption->lMinTableScore, 0L);
	}
}

//查询服务费
bool CTableFrameSink::QueryBuckleServiceCharge(WORD wChairID)
{
	for (int nAreaIndex=1; nAreaIndex<=AREA_COUNT; ++nAreaIndex) 
	{
		if(m_lUserJettonScore[nAreaIndex][wChairID]>0)
			return true;
	}
	return false;
}

//游戏开始
bool  CTableFrameSink::OnEventGameStart()
{

	//变量定义
	CMD_S_GameStart GameStart;
	ZeroMemory(&GameStart,sizeof(GameStart));

	//获取庄家
	IServerUserItem *pIBankerServerUserItem=NULL;
	if (INVALID_CHAIR!=m_wCurrentBanker) pIBankerServerUserItem=m_pITableFrame->GetTableUserItem(m_wCurrentBanker);

	//设置变量
	GameStart.cbTimeLeave=m_cbBetTime;
	GameStart.wBankerUser=m_wCurrentBanker;
	GameStart.lBankerScore = BANKER_DEFSCORE;
	if (pIBankerServerUserItem!=NULL) 
		GameStart.lBankerScore=pIBankerServerUserItem->GetUserScore()*m_nJettonMultiple;

	//下注机器人数量
	int nChipRobotCount = 0;
	for (int i = 0; i < GAME_PLAYER; i++) 
	{
		IServerUserItem *pIServerUserItem=m_pITableFrame->GetTableUserItem(i);
		if (pIServerUserItem != NULL && pIServerUserItem->IsAndroidUser())
			nChipRobotCount++;
	}
	GameStart.nChipRobotCount = min(nChipRobotCount, m_nMaxChipRobot);

	nChipRobotCount = 0;
	for (int i = 0; i < m_ApplyUserArray.GetCount(); i++) 
	{
		IServerUserItem *pIServerUserItem=m_pITableFrame->GetTableUserItem(m_ApplyUserArray.GetAt(i));
		if (pIServerUserItem != NULL && pIServerUserItem->IsAndroidUser())
			nChipRobotCount++;
	}

	if(nChipRobotCount > 0)
		GameStart.nAndriodCount=nChipRobotCount-1;		
	
	//	记录库存消息
	CString strStorage;
	strStorage.Format(TEXT("房间:%s 库存: %I64d"), m_pGameServiceOption->szServerName, m_lStorageCurrent);
	WriteInfo(strStorage);

	//机器人控制
	m_nChipRobotCount = 0;
	
    //旁观玩家
	m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GAME_START,&GameStart,sizeof(GameStart));	

	//游戏玩家
	for (WORD wChairID=0; wChairID<GAME_PLAYER; ++wChairID)
	{
		IServerUserItem *pIServerUserItem=m_pITableFrame->GetTableUserItem(wChairID);
		if (pIServerUserItem==NULL) continue;

		if (m_bEnableSysBanker==false && m_wCurrentBanker==INVALID_CHAIR)
		{
			GameStart.lBankerScore = 1;
		}

		//设置积分
		GameStart.lUserMaxScore=min(pIServerUserItem->GetUserScore()*m_nJettonMultiple,m_lUserLimitScore);

		m_pITableFrame->SendTableData(wChairID,SUB_S_GAME_START,&GameStart,sizeof(GameStart));	
	}

	return true;
}

//游戏结束
bool  CTableFrameSink::OnEventGameConclude(WORD wChairID, IServerUserItem * pIServerUserItem, BYTE cbReason)
{
	switch (cbReason)
	{
	case GER_NORMAL:		//常规结束	
		{
			if ( m_pServerControl != NULL && m_pServerControl->NeedControl() )
			{
				tagControlInfo ControlInfo;
				m_pServerControl->ReturnControlArea(ControlInfo,m_lAllJettonScore);
				m_cbTableCard = ControlInfo.cbControlArea;
				m_pServerControl->CompleteControl();
				//系统输赢
				LONGLONG lSystemScore = 0l;

				JudgeSystemScore(lSystemScore);
				m_lStorageCurrent += lSystemScore;
				if(NeedDeductStorage())
					m_lStorageCurrent = m_lStorageCurrent - (m_lStorageCurrent * m_StorageDeduct / 1000);
			}
			else
			{
				//设置区域
				static BYTE cbCategoryAreaIndex[8]={8,2,1,5,13,9,4,3};						//车型在转盘第一个的索引
				static BYTE	cbCarAreaIndex[4][3]={{8,11,14},{2,12,16},{4,7,10},{3,6,15}};	//五倍车所有的索引
				//变量定义
				CWHArray<BYTE> DrawItemList,BigDrawItemList;
				CWHArray<BYTE> DrawItemWinList,DrawItemLostList;

				LONGLONG lSystemScore = 0l;		

				//计算可开奖项
				for (INT i=0; i<AREA_ALL; i++)
				{
					lSystemScore = 0l;	
					m_cbTableCard = cbCategoryAreaIndex[i];
					JudgeSystemScore(lSystemScore);
					//lSystemScore>0 防止没有开奖区域
					if(lSystemScore+m_lStorageCurrent>0 || lSystemScore>=0) 
					{
						if(i<2||i>6)DrawItemList.Add(m_cbTableCard);
						else BigDrawItemList.Add(m_cbTableCard);

						if(lSystemScore>0) DrawItemWinList.Add(m_cbTableCard);
						else DrawItemLostList.Add(m_cbTableCard);
					}
				}
				ASSERT(DrawItemList.GetCount()+BigDrawItemList.GetCount()>0);
				ASSERT(DrawItemWinList.GetCount()+DrawItemLostList.GetCount()>0);

				if( (DrawItemList.GetCount()+BigDrawItemList.GetCount())==0
					||(DrawItemWinList.GetCount()+DrawItemLostList.GetCount())==0)
				{
					
					CTraceService::TraceString(TEXT("没有可开奖区域!"),TraceLevel_Warning);
					return true;
				}

				//设置开奖区域
				m_cbTableCard=0;
				bool bSystemLost = false;
				if ( m_lStorageCurrent > m_lStorageMax2)	bSystemLost= (rand()%100 < m_lStorageMul2);
				else if(m_lStorageCurrent>m_lStorageMax1)	bSystemLost= (rand()%100 < m_lStorageMul1);
				if(bSystemLost && DrawItemLostList.GetCount()>0)
				{
					m_cbTableCard = DrawItemLostList.GetAt(rand()%DrawItemLostList.GetCount());
				}
				else
				{
					if(DrawItemList.GetCount()==0)
					{
						ASSERT(BigDrawItemList.GetCount()!=0);

						m_cbTableCard = BigDrawItemList.GetAt(rand()%BigDrawItemList.GetCount());
					}
					else if(BigDrawItemList.GetCount()==0)
					{

						ASSERT(DrawItemList.GetCount()==0);
						BYTE cbIndex = DrawItemList.GetAt(rand()%DrawItemList.GetCount());
						for(BYTE cb=0;cb<4;cb++)
						{
							if(cbCarAreaIndex[cb][0]==cbIndex)
							{
								m_cbTableCard=cbCarAreaIndex[cb][rand()%3];
							}
						}
					}
					else
					{
						if(rand()%100<80)
						{
							BYTE cbIndex = DrawItemList.GetAt(rand()%DrawItemList.GetCount());
							for(BYTE cb=0;cb<4;cb++)
							{
								if(cbCarAreaIndex[cb][0]==cbIndex)
								{
									m_cbTableCard=cbCarAreaIndex[cb][rand()%3];
								}
							}
						}
						else
						{
							m_cbTableCard = BigDrawItemList.GetAt(rand()%BigDrawItemList.GetCount());
						}
					}
				}


				ASSERT(m_cbTableCard!=0);
				lSystemScore = 0l;
				JudgeSystemScore(lSystemScore);
				m_lStorageCurrent += lSystemScore;
				if(NeedDeductStorage())
					m_lStorageCurrent = m_lStorageCurrent - (m_lStorageCurrent * m_StorageDeduct / 1000);

				
// 				bool bSystemLost = false;
// 
// 				while(true)
// 				{
// 					if ( m_lStorageCurrent > m_lStorageMax2)
// 					{
// 						bSystemLost= (rand()%100 < m_lStorageMul2);
// 					}
// 					else if(m_lStorageCurrent>m_lStorageMax1)
// 					{
// 						bSystemLost= (rand()%100 < m_lStorageMul1);
// 					}
// 					//派发扑克
// 					m_cbTableCard = 0;
// 					DispatchTableCard(bSystemLost);
// 					//试探性判断
// 					if (ProbeJudge(bSystemLost))
// 					{
// 						break;
// 					}
// 				}
			}
			
			//计算分数
			LONGLONG lBankerWinScore=CalculateScore();
					
			//递增次数
			m_wBankerTime++;

			//结束消息
			CMD_S_GameEnd GameEnd;
			ZeroMemory(&GameEnd,sizeof(GameEnd));

			//庄家信息
			GameEnd.nBankerTime = m_wBankerTime;
			GameEnd.lBankerTotallScore=m_lBankerWinScore;
			GameEnd.lBankerScore=lBankerWinScore;
							
			//扑克信息
			GameEnd.cbTableCard = m_cbTableCard;
			
			//发送积分
			GameEnd.cbTimeLeave=m_cbEndTime;	

			for ( WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex )
			{
				IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(wUserIndex);
				if ( pIServerUserItem == NULL ) continue;

				//设置成绩
				GameEnd.lUserScore=m_lUserWinScore[wUserIndex];

				//返还积分
				GameEnd.lUserReturnScore=m_lUserReturnScore[wUserIndex];

				//设置税收
				if (m_lUserRevenue[wUserIndex]>0) GameEnd.lRevenue=m_lUserRevenue[wUserIndex];
				else if (m_wCurrentBanker!=INVALID_CHAIR) GameEnd.lRevenue=m_lUserRevenue[m_wCurrentBanker];
				else GameEnd.lRevenue=0;

				//发送消息					
				m_pITableFrame->SendTableData(wUserIndex,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));
				m_pITableFrame->SendLookonData(wUserIndex,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));
			}

			return true;
		}
	case GER_USER_LEAVE:		//用户离开
	case GER_NETWORK_ERROR:
		{
			//闲家判断
			if (m_wCurrentBanker!=wChairID)
			{
				//变量定义
				LONGLONG lScore=0;
				LONGLONG lRevenue=0;
				
				//统计成绩
				for (int nAreaIndex=1; nAreaIndex<=AREA_COUNT; ++nAreaIndex) lScore -= m_lUserJettonScore[nAreaIndex][wChairID];

				//写入积分
				if (m_pITableFrame->GetGameStatus() != GS_GAME_END)
				{
					for (int nAreaIndex=1; nAreaIndex<=AREA_COUNT; ++nAreaIndex)
					{
						if (m_lUserJettonScore[nAreaIndex][wChairID] != 0)
						{
							CMD_S_PlaceJettonFail PlaceJettonFail;
							ZeroMemory(&PlaceJettonFail,sizeof(PlaceJettonFail));
							//PlaceJettonFail.lJettonArea=nAreaIndex;
							//PlaceJettonFail.lPlaceScore=m_lUserJettonScore[nAreaIndex][wChairID];
							//PlaceJettonFail.wPlaceUser=wChairID;

							//游戏玩家
							for (WORD i=0; i<GAME_PLAYER; ++i)
							{
								IServerUserItem *pIServerUserItem=m_pITableFrame->GetTableUserItem(i);
								if (pIServerUserItem==NULL) 
									continue;

								//m_pITableFrame->SendTableData(i,SUB_S_PLACE_JETTON_FAIL,&PlaceJettonFail,sizeof(PlaceJettonFail));
							}

							m_lAllJettonScore[nAreaIndex] -= m_lUserJettonScore[nAreaIndex][wChairID];
							m_lUserJettonScore[nAreaIndex][wChairID] = 0;
						}
					}
				}
				else
				{

					for (int nAreaIndex=1; nAreaIndex<=AREA_COUNT; ++nAreaIndex)
					{
						if (m_lUserJettonScore[nAreaIndex][wChairID] != 0)
						{
							m_lUserJettonScore[nAreaIndex][wChairID] = 0;
						}
					}

					
					//写入积分
					if (m_lUserWinScore[wChairID]!=0L) 
					{

						tagScoreInfo ScoreInfo[GAME_PLAYER];
						ZeroMemory(ScoreInfo,sizeof(ScoreInfo));
						ScoreInfo[wChairID].cbType=(m_lUserWinScore[wChairID]>0L)?SCORE_TYPE_WIN:SCORE_TYPE_LOSE;
						ScoreInfo[wChairID].lRevenue=m_lUserRevenue[wChairID];
						ScoreInfo[wChairID].lScore=m_lUserWinScore[wChairID];
						m_pITableFrame->WriteTableScore(ScoreInfo,CountArray(ScoreInfo));

						m_lUserWinScore[wChairID] = 0;
					}
				}
				return true;
			}

			//状态判断
			if (m_pITableFrame->GetGameStatus()!=GS_GAME_END)
			{
				//提示消息
				TCHAR szTipMsg[128];

				tagUserInfo * pUserInfo = pIServerUserItem->GetUserInfo();
				myprintf(szTipMsg,CountArray(szTipMsg),TEXT("由于庄家[ %s ]强退，游戏提前结束！"),pUserInfo->szNickName);

				//发送消息
				SendGameMessage(INVALID_CHAIR,szTipMsg);	

				//设置状态
				m_pITableFrame->SetGameStatus(GS_GAME_END);

				//设置时间
				m_pITableFrame->KillGameTimer(IDI_PLACE_JETTON);
				m_dwJettonTime=(DWORD)time(NULL);
				m_pITableFrame->SetGameTimer(IDI_GAME_END,m_cbEndTime*1000,1,0L);

				//控制
				if ( m_cbControlArea != 0xff && m_cbControlTimes > 0)
				{
					BYTE cbControlArea[8][4] = { 1, 9, 17, 25, 
												3, 11, 19, 27,
												5, 13, 21, 30,
												7, 15, 23, 32,
												2, 10, 18, 26,
												4, 12, 20, 28,
												6, 14, 22, 31,
												8, 16, 24, 0 };
					m_cbTableCard = cbControlArea[m_cbControlArea][rand()%4];
					m_cbControlTimes--;
					if(m_cbControlTimes == 0)
					{
						m_cbControlArea = 0xff;
					}
					//系统输赢
					LONGLONG lSystemScore = 0l;

					JudgeSystemScore(lSystemScore);
					m_lStorageCurrent += lSystemScore;
					if(NeedDeductStorage())
						m_lStorageCurrent = m_lStorageCurrent - (m_lStorageCurrent * m_StorageDeduct / 1000);
				}
				else
				{
					bool bSystemLost = false;
					if ( m_lStorageCurrent > m_lStorageMax2)
					{
						bSystemLost= (rand()%100 < m_lStorageMul2);
					}
					else
					{
						bSystemLost= (rand()%100 < m_lStorageMul1);
					}

					while(true)
					{
						//派发扑克
						m_cbTableCard = 0;
						DispatchTableCard(bSystemLost);
						//试探性判断
						if (ProbeJudge(bSystemLost))
						{
							break;
						}
					}
				}

				//计算分数
				CalculateScore();

				//结束消息
				CMD_S_GameEnd GameEnd;
				ZeroMemory(&GameEnd,sizeof(GameEnd));

				//庄家信息
				GameEnd.nBankerTime = m_wBankerTime;
				GameEnd.lBankerTotallScore=m_lBankerWinScore;
				
				if (m_lBankerWinScore>0) GameEnd.lBankerScore=0;
			
				//扑克信息
				GameEnd.cbTableCard = m_cbTableCard;
				
				//发送积分
				GameEnd.cbTimeLeave=m_cbEndTime;	
				for ( WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex )
				{
					IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(wUserIndex);
					if ( pIServerUserItem == NULL ) continue;

					//设置成绩
					GameEnd.lUserScore=m_lUserWinScore[wUserIndex];

					//返还积分
					GameEnd.lUserReturnScore=m_lUserReturnScore[wUserIndex];

					//设置税收
					if (m_lUserRevenue[wUserIndex]>0) GameEnd.lRevenue=m_lUserRevenue[wUserIndex];
					else if (m_wCurrentBanker!=INVALID_CHAIR) GameEnd.lRevenue=m_lUserRevenue[m_wCurrentBanker];
					else GameEnd.lRevenue=0;

					//发送消息					
					m_pITableFrame->SendTableData(wUserIndex,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));
					m_pITableFrame->SendLookonData(wUserIndex,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));
				}
			}

			//扣除分数
			if (m_lUserWinScore[m_wCurrentBanker] != 0l)
			{
				tagScoreInfo ScoreInfo[GAME_PLAYER];
				ZeroMemory(ScoreInfo,sizeof(ScoreInfo));
				ScoreInfo[m_wCurrentBanker].cbType=(m_lUserWinScore[m_wCurrentBanker]>0L)?SCORE_TYPE_WIN:SCORE_TYPE_LOSE;
				ScoreInfo[m_wCurrentBanker].lRevenue=m_lUserRevenue[m_wCurrentBanker];
				ScoreInfo[m_wCurrentBanker].lScore=m_lUserWinScore[m_wCurrentBanker];
				m_pITableFrame->WriteTableScore(ScoreInfo,CountArray(ScoreInfo));
				m_lUserWinScore[m_wCurrentBanker] = 0;
			}

			//切换庄家
			ChangeBanker(true);

			return true;
		}
	}

	return false;
}

//发送场景
bool  CTableFrameSink::OnEventSendGameScene(WORD wChiarID, IServerUserItem * pIServerUserItem, BYTE cbGameStatus, bool bSendSecret)
{
	switch (cbGameStatus)
	{
	case GAME_STATUS_FREE:			//空闲状态
		{
			//发送记录
			SendGameRecord(pIServerUserItem);

			//构造数据
			CMD_S_StatusFree StatusFree;
			ZeroMemory(&StatusFree,sizeof(StatusFree));			

			//控制信息
			StatusFree.lApplyBankerCondition = m_lApplyBankerCondition;
			StatusFree.lAreaLimitScore = m_lAreaLimitScore;
			//StatusFree.CheckImage = m_CheckImage;
			StatusFree.bGenreEducate =  m_pGameServiceOption->wServerType&GAME_GENRE_EDUCATE;
			//庄家信息
			StatusFree.bEnableSysBanker=m_bEnableSysBanker;
			StatusFree.wBankerUser=m_wCurrentBanker;	
			StatusFree.cbBankerTime=m_wBankerTime;
			StatusFree.lBankerWinScore=m_lBankerWinScore;
			StatusFree.lBankerScore = BANKER_DEFSCORE;
			StatusFree.nMultiple = m_nJettonMultiple;
			if (m_wCurrentBanker!=INVALID_CHAIR)
			{
				IServerUserItem *pIServerUserItem=m_pITableFrame->GetTableUserItem(m_wCurrentBanker);
				StatusFree.lBankerScore=pIServerUserItem->GetUserScore()*m_nJettonMultiple;
			}

			//玩家信息
			if (pIServerUserItem->GetUserStatus()!=US_LOOKON)
			{
				StatusFree.lUserMaxScore=min(pIServerUserItem->GetUserScore()*m_nJettonMultiple,m_lUserLimitScore*4); 
			}

			//全局信息
			DWORD dwPassTime=(DWORD)time(NULL)-m_dwJettonTime;
			StatusFree.cbTimeLeave=(BYTE)(m_cbFreeTime-__min(dwPassTime,m_cbFreeTime));

			//房间名称
			CopyMemory(StatusFree.szGameRoomName, m_pGameServiceOption->szServerName, sizeof(StatusFree.szGameRoomName));
			
			//房间全名
			{
				TCHAR szAllName[256]=TEXT("");
				myprintf(szAllName,CountArray(szAllName),TEXT("%s > %s > 第%d桌"),m_pGameServiceAttrib->szGameName,m_pGameServiceOption->szServerName,m_pITableFrame->GetTableID()+1);
				memcpy(StatusFree.szRoomTotalName, szAllName, sizeof(szAllName));
			}

			//机器人配置
			if(pIServerUserItem->IsAndroidUser())
			{
				tagCustomConfig *pCustomConfig = (tagCustomConfig *)m_pITableFrame->GetCustomRule();				
				ASSERT(pCustomConfig);

				CopyMemory(&StatusFree.CustomAndroid, &pCustomConfig->CustomAndroid, sizeof(tagCustomAndroid));
			}
			CopyMemory(&StatusFree.lUserStartScore, m_lUserStartScore, sizeof(m_lUserStartScore));

			//发送场景
			bool bSuccess = m_pITableFrame->SendGameScene(pIServerUserItem,&StatusFree,sizeof(StatusFree));
			
			//限制提示
			TCHAR szTipMsg[128];
			_sntprintf(szTipMsg,CountArray(szTipMsg),_T("本房间上庄条件为:%I64d,区域限制为:%I64d,玩家限制为:%I64d"),m_lApplyBankerCondition,m_lAreaLimitScore,m_lUserLimitScore);
			m_pITableFrame->SendGameMessage(pIServerUserItem,szTipMsg,SMT_CHAT);	

			//发送申请者
			SendApplyUser(pIServerUserItem);
			//更新库存信息
			if(CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()))
			{
				CMD_S_UpdateStorage updateStorage;
				ZeroMemory(&updateStorage, sizeof(updateStorage));

				updateStorage.cbReqType = RQ_REFRESH_STORAGE;
				updateStorage.lStorageStart = m_StorageStart;
				updateStorage.lStorageDeduct = m_StorageDeduct;
				updateStorage.lStorageCurrent = m_lStorageCurrent;
				updateStorage.lStorageMax1 = m_lStorageMax1;
				updateStorage.lStorageMul1 = m_lStorageMul1;
				updateStorage.lStorageMax2 = m_lStorageMax2;
				updateStorage.lStorageMul2 = m_lStorageMul2;

				m_pITableFrame->SendUserItemData(pIServerUserItem,SUB_S_UPDATE_STORAGE,&updateStorage,sizeof(updateStorage));
			}
			return bSuccess;
		}
	case GS_PLACE_JETTON:		//游戏状态
	case GS_GAME_END:			//结束状态
		{
			//发送记录
			SendGameRecord(pIServerUserItem);		

			//构造数据
			CMD_S_StatusPlay StatusPlay={0};

			//全局下注
			CopyMemory(StatusPlay.lAllJettonScore,m_lAllJettonScore,sizeof(StatusPlay.lAllJettonScore));

			//玩家下注
			if (pIServerUserItem->GetUserStatus()!=US_LOOKON)
			{
				for (int nAreaIndex=1; nAreaIndex<=AREA_COUNT; ++nAreaIndex)
					StatusPlay.lUserJettonScore[nAreaIndex] = m_lUserJettonScore[nAreaIndex][wChiarID];

				//最大下注
				StatusPlay.lUserMaxScore=min(pIServerUserItem->GetUserScore()*m_nJettonMultiple,m_lUserLimitScore);
			}

			//控制信息
			StatusPlay.lApplyBankerCondition=m_lApplyBankerCondition;		
			StatusPlay.lAreaLimitScore=m_lAreaLimitScore;		
			StatusPlay.bGenreEducate =  m_pGameServiceOption->wServerType&GAME_GENRE_EDUCATE;
			//庄家信息
			StatusPlay.bEnableSysBanker=m_bEnableSysBanker;
			StatusPlay.wBankerUser=m_wCurrentBanker;			
			StatusPlay.cbBankerTime=m_wBankerTime;
			StatusPlay.lBankerWinScore=m_lBankerWinScore;	
			StatusPlay.lBankerScore = BANKER_DEFSCORE;
			StatusPlay.nMultiple = m_nJettonMultiple;

			
			if (m_wCurrentBanker!=INVALID_CHAIR)
			{
				IServerUserItem *pIServerUserItem=m_pITableFrame->GetTableUserItem(m_wCurrentBanker);
				StatusPlay.lBankerScore=pIServerUserItem->GetUserScore()*m_nJettonMultiple;
			}	

			//全局信息
			DWORD dwPassTime=(DWORD)time(NULL)-m_dwJettonTime;
			StatusPlay.cbTimeLeave=(BYTE)(m_cbBetTime-__min(dwPassTime,m_cbBetTime));
			StatusPlay.cbGameStatus=m_pITableFrame->GetGameStatus();			

			//结束判断
			if (cbGameStatus==GS_GAME_END)
			{
				
				BYTE cbEndTime=0;
				
				cbEndTime=m_cbEndTime;
				
				StatusPlay.cbTimeLeave=(BYTE)(cbEndTime-__min(dwPassTime,cbEndTime));

				//设置成绩
				StatusPlay.lEndUserScore=m_lUserWinScore[wChiarID];

				//返还积分
				StatusPlay.lEndUserReturnScore=m_lUserReturnScore[wChiarID];

				//设置税收
				if (m_lUserRevenue[wChiarID]>0) StatusPlay.lEndRevenue=m_lUserRevenue[wChiarID];
				else if (m_wCurrentBanker!=INVALID_CHAIR) StatusPlay.lEndRevenue=m_lUserRevenue[m_wCurrentBanker];
				else StatusPlay.lEndRevenue=0;

				//庄家成绩
				StatusPlay.lEndBankerScore=m_lBankerCurGameScore;

				//扑克信息
				StatusPlay.cbTableCard = m_cbTableCard;
			}

			//房间名称
			CopyMemory(StatusPlay.szGameRoomName, m_pGameServiceOption->szServerName, sizeof(StatusPlay.szGameRoomName));

			//房间全名
			{
				TCHAR szAllName[256]=TEXT("");
				myprintf(szAllName,CountArray(szAllName),TEXT("%s > %s > 第%d桌"),m_pGameServiceAttrib->szGameName,m_pGameServiceOption->szServerName,m_pITableFrame->GetTableID()+1);
				memcpy(StatusPlay.szRoomTotalName, szAllName, sizeof(szAllName));
			}

			//机器人配置
			if(pIServerUserItem->IsAndroidUser())
			{
				tagCustomConfig *pCustomConfig = (tagCustomConfig *)m_pITableFrame->GetCustomRule();				
				ASSERT(pCustomConfig);

				CopyMemory(&StatusPlay.CustomAndroid, &pCustomConfig->CustomAndroid, sizeof(tagCustomAndroid));
			}

			CopyMemory(&StatusPlay.lUserStartScore, m_lUserStartScore, sizeof(m_lUserStartScore));

			//发送场景
			bool bSuccess = m_pITableFrame->SendGameScene(pIServerUserItem,&StatusPlay,sizeof(StatusPlay));

			////限制提示
			//TCHAR szTipMsg[128];
			//myprintf(szTipMsg,CountArray(szTipMsg),TEXT("本房间上庄条件为：%I64d,区域限制为：%I64d,玩家限制为：%I64d"),m_lApplyBankerCondition,
			//	m_lAreaLimitScore,m_lUserLimitScore);
			//
			//m_pITableFrame->SendGameMessage(pIServerUserItem,szTipMsg,SMT_CHAT);
			
			//限制提示
			TCHAR szTipMsg[128];
			_sntprintf(szTipMsg,CountArray(szTipMsg),_T("本房间上庄条件为:%I64d,区域限制为:%I64d,玩家限制为:%I64d"),m_lApplyBankerCondition,m_lAreaLimitScore,m_lUserLimitScore);
			m_pITableFrame->SendGameMessage(pIServerUserItem,szTipMsg,SMT_CHAT);	

			//发送申请者
			SendApplyUser( pIServerUserItem );
			//更新库存信息
			if(CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()))
			{
				CMD_S_UpdateStorage updateStorage;
				ZeroMemory(&updateStorage, sizeof(updateStorage));

				updateStorage.cbReqType = RQ_REFRESH_STORAGE;
				updateStorage.lStorageStart = m_StorageStart;
				updateStorage.lStorageDeduct = m_StorageDeduct;
				updateStorage.lStorageCurrent = m_lStorageCurrent;
				updateStorage.lStorageMax1 = m_lStorageMax1;
				updateStorage.lStorageMul1 = m_lStorageMul1;
				updateStorage.lStorageMax2 = m_lStorageMax2;
				updateStorage.lStorageMul2 = m_lStorageMul2;

				m_pITableFrame->SendUserItemData(pIServerUserItem,SUB_S_UPDATE_STORAGE,&updateStorage,sizeof(updateStorage));
			}
			return bSuccess;
		}
	}

	return false;
}

//定时器事件
bool  CTableFrameSink::OnTimerMessage(DWORD wTimerID, WPARAM wBindParam)
{
	switch (wTimerID)
	{
	case IDI_FREE:		//空闲时间
		{
			//无人坐庄
			if (m_bEnableSysBanker==false && m_wCurrentBanker==INVALID_CHAIR)
			{
				//设置状态
				//m_pITableFrame->SetGameStatus(GAME_STATUS_FREE);
				//m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_WAIT_BANKER);

				//设置时间
				//m_dwJettonTime=(DWORD)time(NULL);
				//m_pITableFrame->SetGameTimer(IDI_FREE,m_cbFreeTime*1000,1,0L);

				//发送消息
				//CMD_S_GameFree GameFree;
				//ZeroMemory(&GameFree,sizeof(GameFree));

				//GameFree.cbTimeLeave = m_cbFreeTime;
				//m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_GAME_FREE,&GameFree,sizeof(GameFree));
				//m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GAME_FREE,&GameFree,sizeof(GameFree));

				//return true;
			}
			
			//开始游戏
			m_pITableFrame->StartGame();

			//设置时间
			m_dwJettonTime=(DWORD)time(NULL);
			m_pITableFrame->SetGameTimer(IDI_PLACE_JETTON,m_cbBetTime*1000,1,0L);

			//设置状态
			m_pITableFrame->SetGameStatus(GS_PLACE_JETTON);

			return true;
		}
	case IDI_PLACE_JETTON:		//下注时间
		{
			//状态判断(防止强退重复设置)
			if (m_pITableFrame->GetGameStatus()!=GS_GAME_END)
			{
				//设置状态
				m_pITableFrame->SetGameStatus(GS_GAME_END);			

				//结束游戏
				OnEventGameConclude(INVALID_CHAIR,NULL,GER_NORMAL);

				//设置时间
				m_dwJettonTime=(DWORD)time(NULL);
				m_pITableFrame->SetGameTimer(IDI_GAME_END,m_cbEndTime*1000,1,0L);			
			}

			return true;
		}
	case IDI_GAME_END:			//结束游戏
		{
			tagScoreInfo ScoreInfo;

			//写入积分
			for ( WORD wUserChairID = 0; wUserChairID < GAME_PLAYER; ++wUserChairID )
			{
				IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(wUserChairID);
				if ( pIServerUserItem == NULL ) continue;
				ZeroMemory(&ScoreInfo,sizeof(ScoreInfo));

				if(m_lUserWinScore[wUserChairID] != 0)
				{
					ScoreInfo.cbType=(m_lUserWinScore[wUserChairID]>0L)?SCORE_TYPE_WIN:SCORE_TYPE_LOSE;
					ScoreInfo.lRevenue=m_lUserRevenue[wUserChairID];
					ScoreInfo.lScore=m_lUserWinScore[wUserChairID];
					
					m_pITableFrame->WriteUserScore(wUserChairID,ScoreInfo);
				}

			}
			//结束游戏
			m_pITableFrame->ConcludeGame(GAME_STATUS_FREE);

			//切换庄家
			ChangeBanker(false);

			RobotBankerControl();

			//设置时间
			m_dwJettonTime=(DWORD)time(NULL);
			m_pITableFrame->SetGameTimer(IDI_FREE,m_cbFreeTime*1000,1,0L);

			//发送消息
			CMD_S_GameFree GameFree;
			ZeroMemory(&GameFree,sizeof(GameFree));

			GameFree.cbTimeLeave = m_cbFreeTime;
			GameFree.lStorageCurrent=m_lStorageCurrent;
			m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_GAME_FREE,&GameFree,sizeof(GameFree));
			m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GAME_FREE,&GameFree,sizeof(GameFree));

			return true;
		}
	}

	return false;
}

//游戏消息处理
bool  CTableFrameSink::OnGameMessage(WORD wSubCmdID, VOID * pDataBuffer, WORD wDataSize, IServerUserItem * pIServerUserItem)
{
	
	switch (wSubCmdID)
	{
	case SUB_C_PLACE_JETTON:		//用户加注
		{
			//效验数据
			ASSERT(wDataSize==sizeof(CMD_C_PlaceJetton));
			if (wDataSize!=sizeof(CMD_C_PlaceJetton)) return false;

			//用户效验
			
			tagUserInfo * pUserData=pIServerUserItem->GetUserInfo();

			if (pUserData->cbUserStatus!=US_PLAYING) return true;

			//消息处理
			CMD_C_PlaceJetton * pPlaceJetton=(CMD_C_PlaceJetton *)pDataBuffer;
			//玩家下注的实际值要除以兑换倍数
			return OnUserPlaceJetton(pUserData->wChairID,pPlaceJetton->cbJettonArea,pPlaceJetton->lJettonScore/m_nJettonMultiple);
		}
	case SUB_C_APPLY_BANKER:		//申请做庄
		{
			//用户效验
			tagUserInfo * pUserData=pIServerUserItem->GetUserInfo();
			if (pUserData->cbUserStatus==US_LOOKON) return true;

			return OnUserApplyBanker(pIServerUserItem);	
		}
	case SUB_C_CANCEL_BANKER:		//取消做庄
		{
			//用户效验
			tagUserInfo * pUserData=pIServerUserItem->GetUserInfo();
			if (pUserData->cbUserStatus==US_LOOKON) return true;

			return OnUserCancelBanker(pIServerUserItem);	
		}
	case SUB_C_UPDATE_STORAGE:		//更新库存
		{
			ASSERT(wDataSize==sizeof(CMD_C_UpdateStorage));
			if(wDataSize!=sizeof(CMD_C_UpdateStorage)) return false;

			//权限判断
			if(CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight())==false)
				return false;

			//消息处理
			CMD_C_UpdateStorage * pUpdateStorage=(CMD_C_UpdateStorage *)pDataBuffer;
			if (pUpdateStorage->cbReqType==RQ_SET_STORAGE)
			{
				m_StorageDeduct = pUpdateStorage->lStorageDeduct;
				m_lStorageCurrent = pUpdateStorage->lStorageCurrent;
				m_lStorageMax1 = pUpdateStorage->lStorageMax1;
				m_lStorageMul1 = pUpdateStorage->lStorageMul1;
				m_lStorageMax2 = pUpdateStorage->lStorageMax2;
				m_lStorageMul2 = pUpdateStorage->lStorageMul2;

				//记录信息
				CString strControlInfo;
				strControlInfo.Format(TEXT("修改库存 房间: %s | 桌号: %u | 控制人账号: %s | 控制人ID: %u"),
					m_pGameServiceOption->szServerName, m_pITableFrame->GetTableID()+1,
					pIServerUserItem->GetNickName(), pIServerUserItem->GetGameID());
				WriteInfo(strControlInfo);
			}
			return TRUE;
		}		
	case SUB_C_ADMIN_COMMDN:
		{
			ASSERT(wDataSize==sizeof(CMD_C_ControlApplication));
			if(wDataSize!=sizeof(CMD_C_ControlApplication))return false;
			if ( m_pServerControl == NULL) return true;

			if( !CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()) ) return false;

			//设置库存
			const CMD_C_ControlApplication* AdminReq=static_cast<const CMD_C_ControlApplication*>(pDataBuffer);
			if(AdminReq->cbControlAppType==C_CA_SET_STORAGE)
			{
				m_lStorageCurrent=AdminReq->lSetStorage;
				m_pServerControl->SetStorage(m_lStorageCurrent);

				return true;
			}

			m_pServerControl->SetStorage(m_lStorageCurrent);

			return m_pServerControl->ServerControl(wSubCmdID, pDataBuffer, wDataSize, pIServerUserItem, m_pITableFrame);
		}
	case SUB_C_CLEAR_JETTON:	//清除下注
		{
			ASSERT(wDataSize==0);
			if(wDataSize!=0) return false;
			//用户效验
// 			tagUserInfo * pUserData=pIServerUserItem->GetUserInfo();
// 
// 			if (pUserData->cbUserStatus!=US_PLAYING) return true;
// 			//状态校验
// 			if(m_pITableFrame->GetGameStatus() != GS_PLACE_JETTON) return false;
// 
// 			for(int i=0;i<AREA_COUNT+1;i++)
// 			{
// 				m_lAllJettonScore[i]-=m_lUserJettonScore[i][pIServerUserItem->GetUserID()];
// 				m_lUserJettonScore[i][pIServerUserItem->GetUserID()]=0;
// 			}
			return true;
		}
	}

	return false;
}

//框架消息处理
bool  CTableFrameSink::OnFrameMessage(WORD wSubCmdID, VOID * pDataBuffer, WORD wDataSize, IServerUserItem * pIServerUserItem)
{
	return false;
}

//数据事件
bool CTableFrameSink::OnGameDataBase(WORD wRequestID, VOID * pData, WORD wDataSize)
{
	return false;
}

//用户坐下
bool  CTableFrameSink::OnActionUserSitDown(WORD wChairID, IServerUserItem * pIServerUserItem, bool bLookonUser)
{
	//起始分数
	m_lUserStartScore[wChairID] = pIServerUserItem->GetUserScore();
	//设置时间
	if ((bLookonUser==false)&&(m_dwJettonTime==0L))
	{
		m_dwJettonTime=(DWORD)time(NULL);
		m_pITableFrame->SetGameTimer(IDI_FREE,m_cbFreeTime*1000,1,NULL);
		m_pITableFrame->SetGameStatus(GAME_STATUS_FREE);
	}

	//限制提示
	/*TCHAR szTipMsg[128];
	myprintf(szTipMsg,CountArray(szTipMsg),TEXT("本房间上庄条件为：%I64d,区域限制为：%I64d,玩家限制为：%I64d"),m_lApplyBankerCondition,
		m_lAreaLimitScore,m_lUserLimitScore);
	m_pITableFrame->SendGameMessage(pIServerUserItem,szTipMsg,SMT_CHAT);*/

	return true;
}

//用户起来
bool  CTableFrameSink::OnActionUserStandUp(WORD wChairID, IServerUserItem * pIServerUserItem, bool bLookonUser)
{
	//起始分数
	m_lUserStartScore[wChairID] = 0;
	//记录成绩
	if (bLookonUser==false)
	{
		//切换庄家
		if (wChairID==m_wCurrentBanker)
		{
			ChangeBanker(true);
		}

		//取消申请
		for (WORD i=0; i<m_ApplyUserArray.GetCount(); ++i)
		{
			if (wChairID!=m_ApplyUserArray[i]) continue;

			//删除玩家
			m_ApplyUserArray.RemoveAt(i);

			//构造变量
			CMD_S_CancelBanker CancelBanker;
			ZeroMemory(&CancelBanker,sizeof(CancelBanker));
			CancelBanker.wCancelUser = wChairID;

			//发送消息
			m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_CANCEL_BANKER, &CancelBanker, sizeof(CancelBanker));
			m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_CANCEL_BANKER, &CancelBanker, sizeof(CancelBanker));

			break;
		}

		return true;
	}

	return true;
}

//加注事件
bool CTableFrameSink::OnUserPlaceJetton(WORD wChairID, BYTE cbJettonArea, LONGLONG lJettonScore)
{
	//效验参数
	ASSERT((cbJettonArea<=AREA_COUNT && cbJettonArea>=1)&&(lJettonScore>0L));
	if ((cbJettonArea>AREA_COUNT)||(lJettonScore<=0L) || cbJettonArea<1)
	{
		return false;
	}

	if (m_pITableFrame->GetGameStatus()!=GS_PLACE_JETTON || m_wCurrentBanker == wChairID)
	{
		CMD_S_PlaceJettonFail PlaceJettonFail;
		ZeroMemory(&PlaceJettonFail,sizeof(PlaceJettonFail));
		PlaceJettonFail.lJettonArea=cbJettonArea;
		PlaceJettonFail.lPlaceScore=lJettonScore;
		PlaceJettonFail.wPlaceUser=wChairID;

		//发送消息
		m_pITableFrame->SendTableData(wChairID,SUB_S_PLACE_JETTON_FAIL,&PlaceJettonFail,sizeof(PlaceJettonFail));
		return true;
	}

	//庄家判断
	if (m_wCurrentBanker==wChairID) return true;
	if (m_bEnableSysBanker==false && m_wCurrentBanker==INVALID_CHAIR) return true;

	//变量定义
	IServerUserItem * pIServerUserItem=m_pITableFrame->GetTableUserItem(wChairID);
	LONGLONG lJettonCount=0L;
	for (int nAreaIndex=1; nAreaIndex<=AREA_COUNT; ++nAreaIndex) lJettonCount += m_lUserJettonScore[nAreaIndex][wChairID];

	//玩家积分
	LONGLONG lUserScore = pIServerUserItem->GetUserScore();

	//合法校验
	if (lUserScore < lJettonCount + lJettonScore) return true;
	if (m_lUserLimitScore/m_nJettonMultiple < lJettonCount + lJettonScore) return true;

	//成功标识
	bool bPlaceJettonSuccess=true;

	//合法验证
	if (GetUserMaxJetton(wChairID,cbJettonArea) >= lJettonScore)
	{
		//机器人验证
		if(pIServerUserItem->IsAndroidUser())
		{
			//区域限制
			if (m_lRobotAreaScore[cbJettonArea] + lJettonScore > m_lRobotAreaLimit)
				return true;

			//数目限制
			bool bHaveChip = false;
			for (int i = 0; i < AREA_COUNT; i++)
			{
				if (m_lUserJettonScore[i+1][wChairID] != 0)
					bHaveChip = true;
			}

			if (!bHaveChip)
			{
				if (m_nChipRobotCount+1 > m_nMaxChipRobot)
				{
					bPlaceJettonSuccess = false;
				}
				else
					m_nChipRobotCount++;
			}

			//统计分数
			if (bPlaceJettonSuccess)
				m_lRobotAreaScore[cbJettonArea] += lJettonScore;
		}

		if (bPlaceJettonSuccess)
		{
			//保存下注
			m_lAllJettonScore[cbJettonArea] += lJettonScore;
			m_lUserJettonScore[cbJettonArea][wChairID] += lJettonScore;	
			
		}	
	}
	else
	{
		bPlaceJettonSuccess=false;

		if(pIServerUserItem->IsAndroidUser())
		{
			CString strInfo;
			strInfo.Format(TEXT("机器人下注失败，额度：%I64d，区域：%d"),lJettonScore,cbJettonArea);

			NcaTextOut(strInfo);

		}
	}

	if (bPlaceJettonSuccess)
	{
		//变量定义
		CMD_S_PlaceJetton PlaceJetton;
		ZeroMemory(&PlaceJetton,sizeof(PlaceJetton));
		//构造变量
		PlaceJetton.wChairID=wChairID;
		PlaceJetton.cbJettonArea=cbJettonArea;
		PlaceJetton.lJettonScore=lJettonScore;

		//获取用户
		IServerUserItem * pIServerUserItem=m_pITableFrame->GetTableUserItem(wChairID);
		if (pIServerUserItem != NULL) 
			PlaceJetton.cbAndroid = pIServerUserItem->IsAndroidUser()? TRUE : FALSE;

		//发送消息
		m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_PLACE_JETTON,&PlaceJetton,sizeof(PlaceJetton));
		m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_PLACE_JETTON,&PlaceJetton,sizeof(PlaceJetton));
	}
	else
	{
		CMD_S_PlaceJettonFail PlaceJettonFail;
		ZeroMemory(&PlaceJettonFail,sizeof(PlaceJettonFail));
		PlaceJettonFail.lJettonArea=cbJettonArea;
		PlaceJettonFail.lPlaceScore=lJettonScore;
		PlaceJettonFail.wPlaceUser=wChairID;

		//发送消息
		m_pITableFrame->SendTableData(wChairID,SUB_S_PLACE_JETTON_FAIL,&PlaceJettonFail,sizeof(PlaceJettonFail));
	}

	return true;
}
void CTableFrameSink::RandList(BYTE cbCardBuffer[], BYTE cbBufferCount)
{

	//混乱准备
	BYTE *cbCardData = new BYTE[cbBufferCount];
	CopyMemory(cbCardData,cbCardBuffer,cbBufferCount);

	//混乱扑克
	BYTE cbRandCount=0,cbPosition=0;
	do
	{
		cbPosition=rand()%(cbBufferCount-cbRandCount);
		cbCardBuffer[cbRandCount++]=cbCardData[cbPosition];
		cbCardData[cbPosition]=cbCardData[cbBufferCount-cbRandCount];
	} while (cbRandCount<cbBufferCount);

	delete []cbCardData;
	cbCardData = NULL;

	return;
}


//发送扑克
bool CTableFrameSink::DispatchTableCard(bool bCheat)
{
	//有没有真人下注
	if(bCheat==true)
	{
		bool bHaveRealUser = false;
		for (WORD wChairID=0; wChairID<GAME_PLAYER; wChairID++)
		{
			IServerUserItem * pIServerUserItem=m_pITableFrame->GetTableUserItem(wChairID);
			if (pIServerUserItem==NULL ) continue;
			if(pIServerUserItem->IsAndroidUser()==true) continue;

			for (WORD wAreaIndex = 1; wAreaIndex <= AREA_COUNT; ++wAreaIndex)
			{
				if(m_lUserJettonScore[wAreaIndex][wChairID]>0)
				{
					bHaveRealUser = true;
					break;
				}
			}
		}
		if(bHaveRealUser==false) bCheat = false;
	}


	m_cbTableCard = GetRandCardValue(bCheat);

	return true;
}

//获取数值
BYTE CTableFrameSink::GetRandCardValue(bool bCheat)
{
	

	if(bCheat == false)
	{
		int n1=m_nMultiple[ID_LANDROVER-1] ;		
		int n2=m_nMultiple[ID_BMW-1] ;	
		int n3=m_nMultiple[ID_MASERATI-1] ;			
		int n4=m_nMultiple[ID_FERRARI-1] ;		
		int n5=m_nMultiple[ID_LAMBORGHINI-1] ;			
		int n6=m_nMultiple[ID_PORSCHE-1] ;			
		int n7=m_nMultiple[ID_JAGUAR-1] ;	
		int n8=m_nMultiple[ID_BENZ-1] ;		

		INT cbControlArea[16] = { 1, 2,  3,  4,   5,  6,  7,  8,   9, 10, 11, 12,   13, 14, 15, 16 };
		INT cbnChance[16]	  = { n3,n2, n8, n7, n4,  n8, n7,  n1, n6, n7, n1, n2, n5, n1, n8, n2 };

		m_GameLogic.RandArray(cbControlArea, CountArray(cbControlArea), cbnChance, CountArray(cbnChance));

		//随机倍数
		DWORD wTick = GetTickCount();

		//几率和值
		INT nChanceAndValue = 0;
		for ( int n = 0; n < CountArray(cbnChance); ++n )
			nChanceAndValue += cbnChance[n];

		INT nMuIndex = 0;
		int nRandNum = 0;					//随机辅助
		static int nStFluc = 1;				
		nRandNum = (rand() + wTick + nStFluc*3 ) % nChanceAndValue;
		for (int j = 0; j < CountArray(cbnChance); j++)
		{
			nRandNum -= cbnChance[j];
			if (nRandNum < 0)
			{
				nMuIndex = j;
				break;
			}
		}
		nStFluc = nStFluc%3 + 1;

		return cbControlArea[nMuIndex];
	}
	else
	{	
		INT cbControlArea[16] = { 1, 2,  3,  4,   5,  6,  7,  8,   9, 10, 11, 12,   13, 14, 15, 16 };
		//混乱数组
		DWORD wTick = GetTickCount();
		for (int nRandIndex=0; nRandIndex<15; nRandIndex++)
		{
			int nTempIndex = rand()%(15-nRandIndex);

			INT nTemp = cbControlArea[15-nRandIndex];
			cbControlArea[15-nRandIndex]=cbControlArea[nTempIndex];
			cbControlArea[nTempIndex]=nTemp;
		}

		for (INT i=0; i<16; i++)
		{

			LONGLONG lSystemScore = 0l;		
			LONGLONG lAllScore = 0l;
			m_cbTableCard = cbControlArea[i];
			JudgeSystemScore(lSystemScore);
			if (lSystemScore < 0)
			{
				return cbControlArea[i];
			}
		}
	}
	return 0;
}

//申请庄家
bool CTableFrameSink::OnUserApplyBanker(IServerUserItem *pIApplyServerUserItem)
{
	//合法判断
	LONGLONG lUserScore=pIApplyServerUserItem->GetUserScore()*m_nJettonMultiple;
	if (lUserScore<m_lApplyBankerCondition)
	{
		m_pITableFrame->SendGameMessage(pIApplyServerUserItem,TEXT("你的金币不足以申请庄家，申请失败！"),SMT_CHAT|SMT_EJECT);
		return true;
	}

	//存在判断
	WORD wApplyUserChairID=pIApplyServerUserItem->GetChairID();
	for (INT_PTR nUserIdx=0; nUserIdx<m_ApplyUserArray.GetCount(); ++nUserIdx)
	{
		WORD wChairID=m_ApplyUserArray[nUserIdx];
		if (wChairID==wApplyUserChairID)
		{
			m_pITableFrame->SendGameMessage(pIApplyServerUserItem,TEXT("你已经申请了庄家，不需要再次申请！"),SMT_CHAT|SMT_EJECT);
			return true;
		}
	}
	if (pIApplyServerUserItem->IsAndroidUser()&&(m_ApplyUserArray.GetCount())>m_nRobotListMaxCount)
	{
		return true;
	}
	//保存信息 
	m_ApplyUserArray.Add(wApplyUserChairID);

	//构造变量
	CMD_S_ApplyBanker ApplyBanker;
	ZeroMemory(&ApplyBanker,sizeof(ApplyBanker));

	//设置变量
	ApplyBanker.wApplyUser=wApplyUserChairID;

	//发送消息
	m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_APPLY_BANKER, &ApplyBanker, sizeof(ApplyBanker));
	m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_APPLY_BANKER, &ApplyBanker, sizeof(ApplyBanker));

	//切换判断
	if (m_pITableFrame->GetGameStatus()==GAME_STATUS_FREE && m_ApplyUserArray.GetCount()==1)
	{
		if (m_wCurrentBanker==INVALID_CHAIR)
		{
			ChangeBanker(false);

// 			m_dwJettonTime=(DWORD)time(NULL);
// 			m_pITableFrame->SetGameTimer(IDI_FREE,m_cbFreeTime*1000,1,NULL);
// 			m_pITableFrame->SetGameStatus(GAME_STATUS_FREE);
		}
	}

	return true;
}

//取消申请
bool CTableFrameSink::OnUserCancelBanker(IServerUserItem *pICancelServerUserItem)
{
	//当前庄家
	if (pICancelServerUserItem->GetChairID()==m_wCurrentBanker && m_pITableFrame->GetGameStatus()!=GAME_STATUS_FREE)
	{
		//发送消息
		m_pITableFrame->SendGameMessage(pICancelServerUserItem,TEXT("游戏已经开始，不可以取消当庄！"),SMT_CHAT|SMT_EJECT);
		return true;
	}

	//存在判断
	for (WORD i=0; i<m_ApplyUserArray.GetCount(); ++i)
	{
		//获取玩家
		WORD wChairID=m_ApplyUserArray[i];
		IServerUserItem *pIServerUserItem=m_pITableFrame->GetTableUserItem(wChairID);

		//条件过滤
		if (pIServerUserItem==NULL) continue;
		if (pIServerUserItem->GetUserID()!=pICancelServerUserItem->GetUserID()) continue;

		//删除玩家
		m_ApplyUserArray.RemoveAt(i);

		if (m_wCurrentBanker!=wChairID)
		{
			//构造变量
			CMD_S_CancelBanker CancelBanker;
			ZeroMemory(&CancelBanker,sizeof(CancelBanker));
			CancelBanker.wCancelUser = pICancelServerUserItem->GetChairID();

			//发送消息
			m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_CANCEL_BANKER, &CancelBanker, sizeof(CancelBanker));
			m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_CANCEL_BANKER, &CancelBanker, sizeof(CancelBanker));
		}
		else if (m_wCurrentBanker==wChairID)
		{
			//切换庄家 
			m_wCurrentBanker=INVALID_CHAIR;
			ChangeBanker(true);
		}

		return true;
	}

	return true;
}

//更换庄家
bool CTableFrameSink::ChangeBanker(bool bCancelCurrentBanker)
{
	//切换标识
	bool bChangeBanker=false;

	//取消当前
	if (bCancelCurrentBanker)
	{
		for (WORD i=0; i<m_ApplyUserArray.GetCount(); ++i)
		{
			//获取玩家
			WORD wChairID=m_ApplyUserArray[i];

			//条件过滤
			if (wChairID!=m_wCurrentBanker) continue;

			//删除玩家
			m_ApplyUserArray.RemoveAt(i);

			break;
		}

		//设置庄家
		m_wCurrentBanker=INVALID_CHAIR;

		//轮换判断
		if (m_pITableFrame->GetGameStatus()==GAME_STATUS_FREE && FindSuitBanker())
		{
			m_wCurrentBanker=m_ApplyUserArray[0];
		}

		//设置变量
		bChangeBanker=true;
		m_bExchangeBanker = true;
	}
	//轮庄判断
	else if (m_wCurrentBanker!=INVALID_CHAIR)
	{
		//获取庄家
		IServerUserItem *pIServerUserItem=m_pITableFrame->GetTableUserItem(m_wCurrentBanker);

		if(pIServerUserItem!= NULL)
		{
			LONGLONG lBankerScore=pIServerUserItem->GetUserScore()*m_nJettonMultiple;

			//次数判断
			if (m_lPlayerBankerMAX<=m_wBankerTime || lBankerScore<m_lApplyBankerCondition)
			{
				//庄家增加判断 同一个庄家情况下只判断一次
				if(m_lPlayerBankerMAX <= m_wBankerTime && m_bExchangeBanker)
				{
					bool bScoreMAX = true;
					m_bExchangeBanker = false;
			
					for (WORD i=0; i<m_ApplyUserArray.GetCount(); ++i)
					{
						//获取玩家
						WORD wChairID = m_ApplyUserArray[i];
						IServerUserItem *pIUserItem = m_pITableFrame->GetTableUserItem(wChairID);
						LONGLONG lScore = pIUserItem->GetUserScore()*m_nJettonMultiple;

						if ( wChairID != m_wCurrentBanker && lBankerScore <= lScore )
						{
							bScoreMAX = false;
							break;
						}
					}

					LONGLONG wAddTimes=0;
					if ( bScoreMAX || (lBankerScore > m_lBankerScoreMAX && m_lBankerScoreMAX != 0l) )
					{
						if ( bScoreMAX )
							wAddTimes=m_lBankerAdd;
						if ( lBankerScore > m_lBankerScoreMAX && m_lBankerScoreMAX != 0l )
							wAddTimes+=m_lBankerScoreAdd;

						m_lPlayerBankerMAX+=wAddTimes;
						tagUserInfo * pUserInfo = pIServerUserItem->GetUserInfo();
						//提示消息
						TCHAR szTipMsg[128] = {};
						if (wAddTimes==m_lBankerAdd)
							myprintf(szTipMsg,CountArray(szTipMsg),TEXT("[ %s ]的分数超过其他申请上庄玩家，获得%d次额外坐庄次数!"),pUserInfo->szNickName,wAddTimes);
						else if (wAddTimes==m_lBankerScoreAdd)
							myprintf(szTipMsg,CountArray(szTipMsg),TEXT("[ %s ]的分数超过[%I64d]，获得%d次额外坐庄次数!"),pUserInfo->szNickName,m_lBankerScoreMAX,wAddTimes);
						else if(wAddTimes==(m_lBankerScoreAdd+m_lBankerAdd))
							myprintf(szTipMsg,CountArray(szTipMsg),TEXT("[ %s ]的分数超过[%I64d]且超过其他申请上庄玩家，获得%d次额外坐庄次数!"),pUserInfo->szNickName,m_lBankerScoreMAX,wAddTimes);
				
						if (wAddTimes != 0)
						{
							//发送消息
							SendGameMessage(INVALID_CHAIR,szTipMsg);
							return true;
						}
						return true;
					}
				}

				//撤销玩家
				for (WORD i=0; i<m_ApplyUserArray.GetCount(); ++i)
				{
					//获取玩家
					WORD wChairID=m_ApplyUserArray[i];

					//条件过滤
					if (wChairID!=m_wCurrentBanker) continue;

					//删除玩家
					m_ApplyUserArray.RemoveAt(i);

					break;
				}

				//设置庄家
				m_wCurrentBanker=INVALID_CHAIR;

				//轮换判断
				if (FindSuitBanker())
				{
					m_wCurrentBanker=m_ApplyUserArray[0];
				}

				bChangeBanker=true;
				m_bExchangeBanker = true;
				tagUserInfo * pUserInfo = pIServerUserItem->GetUserInfo();
				//提示消息
				TCHAR szTipMsg[128];
				if (lBankerScore<m_lApplyBankerCondition)
					myprintf(szTipMsg,CountArray(szTipMsg),TEXT("[ %s ]分数少于(%I64d)，强行换庄!"),pUserInfo->szNickName,m_lApplyBankerCondition);
				else
					myprintf(szTipMsg,CountArray(szTipMsg),TEXT("[ %s ]做庄次数达到(%d)，强行换庄!"),pUserInfo->szNickName,m_lPlayerBankerMAX);

				//发送消息
				SendGameMessage(INVALID_CHAIR,szTipMsg);	
			}
		}
		else
		{
			for (WORD i=0; i<m_ApplyUserArray.GetCount(); ++i)
			{
				//获取玩家
				WORD wChairID=m_ApplyUserArray[i];

				//条件过滤
				if (wChairID!=m_wCurrentBanker) continue;

				//删除玩家
				m_ApplyUserArray.RemoveAt(i);

				break;
			}
			//设置庄家
			m_wCurrentBanker=INVALID_CHAIR;
		}

	}
	//系统做庄
	else if (m_wCurrentBanker==INVALID_CHAIR &&FindSuitBanker())
	{
		m_wCurrentBanker=m_ApplyUserArray[0];
		bChangeBanker=true;
		m_bExchangeBanker = true;
	}

	//切换判断
	if (bChangeBanker)
	{
		//最大坐庄数
		m_lPlayerBankerMAX = m_lBankerMAX;

		//设置变量
		m_wBankerTime = 0;
		m_lBankerWinScore=0;

		//发送消息
		CMD_S_ChangeBanker sChangeBanker;
		ZeroMemory(&sChangeBanker,sizeof(sChangeBanker));
		sChangeBanker.wBankerUser=m_wCurrentBanker;
		sChangeBanker.lBankerScore = BANKER_DEFSCORE;
		if (m_wCurrentBanker!=INVALID_CHAIR)
		{
			IServerUserItem *pIServerUserItem=m_pITableFrame->GetTableUserItem(m_wCurrentBanker);
			sChangeBanker.lBankerScore=pIServerUserItem->GetUserScore()*m_nJettonMultiple;
		}
		m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_CHANGE_BANKER,&sChangeBanker,sizeof(sChangeBanker));
		m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_CHANGE_BANKER,&sChangeBanker,sizeof(sChangeBanker));

		if (m_wCurrentBanker!=INVALID_CHAIR)
		{
			//读取消息
//  			LONGLONG lMessageCount=GetPrivateProfileInt(m_szGameRoomName,TEXT("MessageCount"),0,m_szConfigFileName);
//  			if (lMessageCount!=0)
// 			{
// 				//读取配置
//  				LONGLONG lIndex=rand()%lMessageCount;
// 				TCHAR szKeyName[32],szMessage1[256],szMessage2[256];				
// 				myprintf(szKeyName,CountArray(szKeyName),TEXT("Item%I64d"),lIndex);
// 				GetPrivateProfileString(m_szGameRoomName,szKeyName,TEXT("恭喜[ %s ]上庄"),szMessage1,CountArray(szMessage1),m_szConfigFileName);

				//获取玩家
				TCHAR	szMessage2[256];
				IServerUserItem *pIServerUserItem=m_pITableFrame->GetTableUserItem(m_wCurrentBanker);
	//			tagUserInfo * pUserInfo = pIServerUserItem->GetUserInfo();
				//发送消息
				
				myprintf(szMessage2,CountArray(szMessage2),TEXT("恭喜[ %s ]上庄,让我们给他点颜色看看!"),pIServerUserItem->GetNickName());
				SendGameMessage(INVALID_CHAIR,szMessage2);
			//}
		}
	}

	return bChangeBanker;
}



//发送庄家
void CTableFrameSink::SendApplyUser( IServerUserItem *pRcvServerUserItem )
{
	for (INT_PTR nUserIdx=0; nUserIdx<m_ApplyUserArray.GetCount(); ++nUserIdx)
	{
		WORD wChairID=m_ApplyUserArray[nUserIdx];

		//获取玩家
		IServerUserItem *pServerUserItem=m_pITableFrame->GetTableUserItem(wChairID);
		if (!pServerUserItem) continue;

		//庄家判断
		if (pServerUserItem->GetChairID()==m_wCurrentBanker) continue;

		//构造变量
		CMD_S_ApplyBanker ApplyBanker;
		ApplyBanker.wApplyUser=wChairID;

		//发送消息
		m_pITableFrame->SendUserItemData(pRcvServerUserItem, SUB_S_APPLY_BANKER, &ApplyBanker, sizeof(ApplyBanker));
	}
}

//用户断线
bool  CTableFrameSink::OnActionUserOffLine(WORD wChairID, IServerUserItem * pIServerUserItem) 
{
	//切换庄家
	if (wChairID==m_wCurrentBanker) ChangeBanker(true);

	//取消申请
	for (WORD i=0; i<m_ApplyUserArray.GetCount(); ++i)
	{
		if (wChairID!=m_ApplyUserArray[i]) continue;

		//删除玩家
		m_ApplyUserArray.RemoveAt(i);

		//构造变量
		CMD_S_CancelBanker CancelBanker;
		ZeroMemory(&CancelBanker,sizeof(CancelBanker));
		CancelBanker.wCancelUser = pIServerUserItem->GetUserID();

		//发送消息
		m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_CANCEL_BANKER, &CancelBanker, sizeof(CancelBanker));
		m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_CANCEL_BANKER, &CancelBanker, sizeof(CancelBanker));

		break;
	}

	return true;
}
void   CTableFrameSink::GetAllWinArea(BYTE bcWinArea[],BYTE bcAreaCount,BYTE InArea)
{
	if (InArea==0xFF)
	{
		return ;
	}
	ZeroMemory(bcWinArea,bcAreaCount);


	LONGLONG lMaxSocre = 0;

	for (int i = 0;i<ANIMAL_COUNT;i++)
	{
		BYTE bcOutCadDataWin[AREA_COUNT];
		BYTE bcData[1];
		bcData[0]=i+1;
		m_GameLogic.GetCardType(bcData[0],1,bcOutCadDataWin);
		for (int j= 0;j<AREA_COUNT;j++)
		{

			if(bcOutCadDataWin[j]>1&&j==InArea-1)
			{
				LONGLONG Score = 0; 
				for (int nAreaIndex=1; nAreaIndex<=AREA_COUNT; ++nAreaIndex) 
				{
					if(bcOutCadDataWin[nAreaIndex-1]>1)
					{
						Score += m_lAllJettonScore[nAreaIndex]*(bcOutCadDataWin[nAreaIndex-1]);
					}
				}
				if(Score>=lMaxSocre)
				{
					lMaxSocre = Score;
					CopyMemory(bcWinArea,bcOutCadDataWin,bcAreaCount);

				}
				break;
			}
		}
	}
}
//最大下注
LONGLONG CTableFrameSink::GetUserMaxJetton(WORD wChairID,BYTE Area)
{
	IServerUserItem *pIMeServerUserItem=m_pITableFrame->GetTableUserItem(wChairID);
	if (NULL==pIMeServerUserItem) return 0L;

	
	//已下注额
	LONGLONG lNowJetton = 0;
	ASSERT(AREA_COUNT<=CountArray(m_lUserJettonScore));
	for (int nAreaIndex=1; nAreaIndex<=AREA_COUNT; ++nAreaIndex) lNowJetton += m_lUserJettonScore[nAreaIndex][wChairID];

	//庄家金币
	LONGLONG lBankerScore=BANKER_DEFSCORE;
	if (m_wCurrentBanker!=INVALID_CHAIR)
	{
		IServerUserItem *pIUserItemBanker=m_pITableFrame->GetTableUserItem(m_wCurrentBanker);
		if (NULL!=pIUserItemBanker) lBankerScore=pIUserItemBanker->GetUserScore();
	}

	BYTE bcWinArea[AREA_COUNT];
	LONGLONG LosScore = 0;
	LONGLONG WinScore = 0;

	GetAllWinArea(bcWinArea,AREA_COUNT,Area);

	for (int nAreaIndex=1; nAreaIndex<=AREA_COUNT; ++nAreaIndex) 
	{
		if(bcWinArea[nAreaIndex-1]>1)
		{
			LosScore+=m_lAllJettonScore[nAreaIndex]*(bcWinArea[nAreaIndex-1]);
		}else
		{
			if(bcWinArea[nAreaIndex-1]==0)
			{
				WinScore+=m_lAllJettonScore[nAreaIndex];

			}
		}
	}
	lBankerScore = lBankerScore + WinScore - LosScore;

	if ( lBankerScore < 0 )
	{
		if (m_wCurrentBanker!=INVALID_CHAIR)
		{
			IServerUserItem *pIUserItemBanker=m_pITableFrame->GetTableUserItem(m_wCurrentBanker);
			if (NULL!=pIUserItemBanker) 
				lBankerScore=pIUserItemBanker->GetUserScore();
		}
		else
		{
			lBankerScore = BANKER_DEFSCORE;
		}
	}

	//个人限制
	LONGLONG lMeMaxScore = min((pIMeServerUserItem->GetUserScore()-lNowJetton), m_lUserLimitScore/m_nJettonMultiple);

	//区域限制
	lMeMaxScore=min(lMeMaxScore,m_lAreaLimitScore/m_nJettonMultiple);

	BYTE diMultiple[AREA_COUNT];

	for (int i = 0;i<ANIMAL_COUNT;i++)
	{
		   BYTE bcData[1];
		   bcData[0]= i+1;
		   BYTE  bcOutCadDataWin[AREA_COUNT];
		   m_GameLogic.GetCardType(bcData[0],1,bcOutCadDataWin);
		   for (int j = 0;j<AREA_COUNT;j++)
		   {
			   if(bcOutCadDataWin[j]>1)
			   {
				   diMultiple[j] = bcOutCadDataWin[j];

			   }
		   }
	}
	//庄家限制
	lMeMaxScore=(min(lMeMaxScore,lBankerScore/(diMultiple[Area-1])));

	//非零限制
	ASSERT(lMeMaxScore >= 0);
	lMeMaxScore = max(lMeMaxScore, 0);

	return (LONGLONG)(lMeMaxScore);
}
//计算得分
LONGLONG CTableFrameSink::CalculateScore()
{
	//变量定义
	float static cbRevenue=m_pGameServiceOption->wRevenueRatio;

	BYTE  bcResulteOut[AREA_COUNT];
	memset(bcResulteOut,0,AREA_COUNT);
	m_GameLogic.GetCardType(m_cbTableCard,1,bcResulteOut);
	
	//游戏记录
	tagServerGameRecord &GameRecord = m_GameRecordArrary[m_nRecordLast];

	GameRecord.cbAnimal=m_cbTableCard;
	BYTE  cbMultiple[AREA_COUNT]={1};

	//移动下标
	m_nRecordLast = (m_nRecordLast+1) % MAX_SCORE_HISTORY;
	if ( m_nRecordLast == m_nRecordFirst ) m_nRecordFirst = (m_nRecordFirst+1) % MAX_SCORE_HISTORY;

	//庄家总量
	LONGLONG lBankerWinScore = 0;

	//玩家成绩
	ZeroMemory(m_lUserWinScore, sizeof(m_lUserWinScore));
	ZeroMemory(m_lUserReturnScore, sizeof(m_lUserReturnScore));
	ZeroMemory(m_lUserRevenue, sizeof(m_lUserRevenue));
	LONGLONG lUserLostScore[GAME_PLAYER];
	ZeroMemory(lUserLostScore, sizeof(lUserLostScore));

	//玩家下注
	LONGLONG *pUserScore[AREA_COUNT+1];
	pUserScore[0]=NULL;
	for (int i = 1;i<AREA_COUNT+1;i++)
	{
		pUserScore[i]=m_lUserJettonScore[i];
	}

	//税收比例
	float fRevenuePer=float(cbRevenue/1000);

	//计算积分 税收
	for (WORD wChairID=0; wChairID<GAME_PLAYER; wChairID++)
	{
		//庄家判断
		if (m_wCurrentBanker==wChairID) continue;

		//获取用户
		IServerUserItem * pIServerUserItem=m_pITableFrame->GetTableUserItem(wChairID);
		if (pIServerUserItem==NULL) continue;

		for (WORD wAreaIndex = 1; wAreaIndex <= AREA_COUNT; ++wAreaIndex)
		{
			if (bcResulteOut[wAreaIndex-1]>1) 
			{
				m_lUserWinScore[wChairID] += ( pUserScore[wAreaIndex][wChairID] *(bcResulteOut[wAreaIndex-1]-1)) ;
				m_lUserReturnScore[wChairID] += pUserScore[wAreaIndex][wChairID] ;
				lBankerWinScore -= ( pUserScore[wAreaIndex][wChairID] * (bcResulteOut[wAreaIndex-1]-1) ) ;
			}
			else
			{
				if (bcResulteOut[wAreaIndex-1]==0)
				{
					lUserLostScore[wChairID] -= pUserScore[wAreaIndex][wChairID];
					lBankerWinScore += pUserScore[wAreaIndex][wChairID];
				}
				
			}
		}
		//税收
		//m_lUserRevenue[wChairID]  = LONGLONG(m_lUserWinScore[wChairID]*fRevenuePer+0.5);
		if (m_lUserWinScore[wChairID] > 0) {
			m_lUserRevenue[wChairID] = m_pITableFrame->CalculateRevenue(wChairID, m_lUserWinScore[wChairID]);
		} else {
			m_lUserRevenue[wChairID] = 0;
		}
		CString str;
		str.Format(TEXT("[%d]=result=[%d,%d,%d]"), wChairID,
			m_lUserWinScore[wChairID],
			m_lUserRevenue[wChairID],
			lUserLostScore[wChairID]);
		WriteInfo(str);
		//总的分数
		m_lUserWinScore[wChairID] += lUserLostScore[wChairID];
		m_lUserWinScore[wChairID] -= m_lUserRevenue[wChairID];
	}

	//庄家成绩
	if (m_wCurrentBanker!=INVALID_CHAIR)
	{
		m_lUserWinScore[m_wCurrentBanker] = lBankerWinScore;
	}
	
	//计算庄家税收
	for(WORD wChairID = 0;wChairID < GAME_PLAYER;wChairID++)
	{
		IServerUserItem * pIServerUserItem=m_pITableFrame->GetTableUserItem(wChairID);
		if (pIServerUserItem==NULL)
			continue;

		if(m_wCurrentBanker!=INVALID_CHAIR && wChairID == m_wCurrentBanker && lBankerWinScore > 0)
		{
			//m_lUserRevenue[m_wCurrentBanker]  = LONGLONG(m_lUserWinScore[m_wCurrentBanker]*fRevenuePer+0.5);
			if (m_lUserWinScore[m_wCurrentBanker] > 0) {
				m_lUserRevenue[m_wCurrentBanker] = m_pITableFrame->CalculateRevenue(wChairID, m_lUserWinScore[m_wCurrentBanker]);
			} else {
				m_lUserRevenue[m_wCurrentBanker] = 0;
			}

			m_lUserWinScore[m_wCurrentBanker] -= m_lUserRevenue[m_wCurrentBanker];	
			lBankerWinScore = m_lUserWinScore[m_wCurrentBanker];
		}
	}


	//累计积分
	m_lBankerWinScore += lBankerWinScore;

	//当前积分
	m_lBankerCurGameScore=lBankerWinScore;


	return lBankerWinScore;
}

//试探性判断
bool CTableFrameSink::ProbeJudge(bool& bSystemLost)
{	
	//系统输赢
	LONGLONG lSystemScore = 0l;
  
	JudgeSystemScore(lSystemScore);
	if (m_lStorageCurrent+lSystemScore>=0 || lSystemScore>=0)
	{
		m_lStorageCurrent += lSystemScore;
		if(NeedDeductStorage())
		{
			m_lStorageCurrent = m_lStorageCurrent - (m_lStorageCurrent * m_StorageDeduct / 1000);
		}
		return true;
	}
	//库存不够赔重开
	else if (m_lStorageCurrent+lSystemScore<0)
	{
// 		if(bSystemLost==true)
// 		{
// 			if(m_lStorageCurrent + lSystemScore < 0)
// 			{
// 				bSystemLost = false;
// 				return false;
// 			}
// 			m_lStorageCurrent += lSystemScore;
// 			if(NeedDeductStorage())
// 			{
// 				m_lStorageCurrent = m_lStorageCurrent - (m_lStorageCurrent * m_StorageDeduct / 1000);
// 			}
// 			return true;
// 		}else
		return false;
	}
	return true;
}




//发送记录
void CTableFrameSink::SendGameRecord(IServerUserItem *pIServerUserItem)
{
	WORD wBufferSize=0;
	BYTE cbBuffer[SOCKET_TCP_BUFFER];
	int nIndex = m_nRecordFirst;
	while ( nIndex != m_nRecordLast )
	{
		if ((wBufferSize+sizeof(tagServerGameRecord))>sizeof(cbBuffer))
		{
			m_pITableFrame->SendUserItemData(pIServerUserItem,SUB_S_SEND_RECORD,cbBuffer,wBufferSize);
			wBufferSize=0;
		}
		CopyMemory(cbBuffer+wBufferSize,&m_GameRecordArrary[nIndex],sizeof(tagServerGameRecord));
		wBufferSize+=sizeof(tagServerGameRecord);

		nIndex = (nIndex+1) % MAX_SCORE_HISTORY;
	}
	if (wBufferSize>0) m_pITableFrame->SendUserItemData(pIServerUserItem,SUB_S_SEND_RECORD,cbBuffer,wBufferSize);
}

//发送消息
void CTableFrameSink::SendGameMessage(WORD wChairID, LPCTSTR pszTipMsg)
{
	if (wChairID==INVALID_CHAIR)
	{
		//游戏玩家
		for (WORD i=0; i<GAME_PLAYER; ++i)
		{
			IServerUserItem *pIServerUserItem=m_pITableFrame->GetTableUserItem(i);
			if (pIServerUserItem==NULL) continue;
			m_pITableFrame->SendGameMessage(pIServerUserItem,pszTipMsg,SMT_CHAT);
		}

		//旁观玩家
		WORD wIndex=0;
		do {
			IServerUserItem *pILookonServerUserItem=m_pITableFrame->EnumLookonUserItem(wIndex++);
			if (pILookonServerUserItem==NULL) break;

			m_pITableFrame->SendGameMessage(pILookonServerUserItem,pszTipMsg,SMT_CHAT);

		}while(true);
	}
	else
	{
		IServerUserItem *pIServerUserItem=m_pITableFrame->GetTableUserItem(wChairID);
		if (pIServerUserItem!=NULL) m_pITableFrame->SendGameMessage(pIServerUserItem,pszTipMsg,SMT_CHAT|SMT_EJECT);
	}
}

//////////////////////////////////////////////////////////////////////////
//银行操作
#ifdef __SPECIAL___
bool  CTableFrameSink::OnActionUserBank(WORD wChairID, IServerUserItem * pIServerUserItem)
{
	return true;
}
#endif

//控制
bool CTableFrameSink::OnAdminControl( CMD_C_ControlApplication* pData , IServerUserItem * pIServerUserItem)
{
	switch(pData->cbControlAppType)
	{
	case C_CA_UPDATE:	//更新
		{
			CMD_S_ControlReturns ControlReturns;
			ZeroMemory(&ControlReturns,sizeof(ControlReturns));
			ControlReturns.cbReturnsType = S_CR_UPDATE_SUCCES;
			ControlReturns.cbControlArea = m_cbControlArea;
			ControlReturns.cbControlTimes = m_cbControlTimes;
			m_pITableFrame->SendUserItemData(pIServerUserItem,SUB_S_ADMIN_COMMDN,&ControlReturns,sizeof(ControlReturns));
		}
		return true;
	case C_CA_SET:		//设置
		{
			if ( pData->cbControlArea == 0xff || pData->cbControlArea  > 7 )
			{
				//设置失败
				CMD_S_ControlReturns ControlReturns;
				ZeroMemory(&ControlReturns,sizeof(ControlReturns));
				ControlReturns.cbReturnsType = S_CR_FAILURE;
				ControlReturns.cbControlArea = 0xff;
				m_pITableFrame->SendUserItemData(pIServerUserItem,SUB_S_ADMIN_COMMDN,&ControlReturns,sizeof(ControlReturns));
				return true;
			}
			m_cbControlArea = pData->cbControlArea;
			m_cbControlTimes = pData->cbControlTimes;
			CMD_S_ControlReturns ControlReturns;
			ZeroMemory(&ControlReturns,sizeof(ControlReturns));
			ControlReturns.cbReturnsType = S_CR_SET_SUCCESS;
			ControlReturns.cbControlArea = m_cbControlArea;
			ControlReturns.cbControlTimes = m_cbControlTimes;
			m_pITableFrame->SendUserItemData(pIServerUserItem,SUB_S_ADMIN_COMMDN,&ControlReturns,sizeof(ControlReturns));
		}
		return true;
	case C_CA_CANCELS:	//取消
		{
			m_cbControlArea = 0xff;
			m_cbControlTimes = 0;
			CMD_S_ControlReturns ControlReturns;
			ZeroMemory(&ControlReturns,sizeof(ControlReturns));
			ControlReturns.cbReturnsType = S_CR_CANCEL_SUCCESS;
			ControlReturns.cbControlArea = 0xff;
			m_pITableFrame->SendUserItemData(pIServerUserItem,SUB_S_ADMIN_COMMDN,&ControlReturns,sizeof(ControlReturns));
		}
		return true;
	}
	//设置失败
	CMD_S_ControlReturns ControlReturns;
	ZeroMemory(&ControlReturns,sizeof(ControlReturns));
	ControlReturns.cbReturnsType = S_CR_FAILURE;
	ControlReturns.cbControlArea = 0xff;
	m_pITableFrame->SendUserItemData(pIServerUserItem,SUB_S_ADMIN_COMMDN,&ControlReturns,sizeof(ControlReturns));
	return true;
}
//////////////////////////////////////////////////////////////////////////
//查找庄家
bool CTableFrameSink::FindSuitBanker()
{

	bool bReturn=false;
	while(m_ApplyUserArray.GetCount()>0)
	{
		IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(m_ApplyUserArray[0]);
		if ( pIServerUserItem == NULL )
		{
			m_ApplyUserArray.RemoveAt(0);
		}
		else
		{
			//坐庄判断
			LONGLONG lUserScore=pIServerUserItem->GetUserScore()*m_nJettonMultiple;
			if(lUserScore<m_lApplyBankerCondition)
			{
				//发送消息
				CMD_S_CancelBanker CancelBanker;
				ZeroMemory(&CancelBanker,sizeof(CancelBanker));
				CancelBanker.wCancelUser = pIServerUserItem->GetUserID();

				//发送消息
				m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_CANCEL_BANKER, &CancelBanker, sizeof(CancelBanker));
				m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_CANCEL_BANKER, &CancelBanker, sizeof(CancelBanker));

				//提示消息
				TCHAR szTipMsg[128];
				myprintf(szTipMsg,CountArray(szTipMsg),TEXT("由于你的金币数少于坐庄必须金币数（%I64d）,你自动下庄！"),m_lApplyBankerCondition);
				SendGameMessage(m_ApplyUserArray[0],szTipMsg);

				//删除玩家
				m_ApplyUserArray.RemoveAt(0);
			}
			else
			{
				bReturn=true;
				break;
			}
		}
	}

	return bReturn;
}


//上庄控制 (机器人)
void CTableFrameSink::RobotBankerControl()
{
	//是否添加上庄机器人
	if (m_ApplyUserArray.GetCount()-1 > 10)
		return;

	if (m_ApplyUserArray.GetCount()-1 > 3)
		return;

	//变量定义
	int nRobotBankerCount = rand() % (10-3) + (3-(m_ApplyUserArray.GetCount()-1));
	int nSendBankerMsgCount = 0;
	int nRandNum = rand() % GAME_PLAYER;

	//可上庄机器人数量
	for (int i = 0; i < GAME_PLAYER; i++)
	{
		WORD wChairID = (nRandNum+i) % GAME_PLAYER;
		IServerUserItem *pIServerUserItem=m_pITableFrame->GetTableUserItem(wChairID);
		if (pIServerUserItem != NULL && pIServerUserItem->IsAndroidUser())
		{
			if ( m_wCurrentBanker != wChairID && pIServerUserItem->GetUserScore()*m_nJettonMultiple >= m_lApplyBankerCondition)
			{
				//检测上庄列表
				bool bOnApplayArray = false;
				for (int j = 0; j < m_ApplyUserArray.GetCount(); j++)
				{
					if (wChairID == m_ApplyUserArray[j]) 
					{
						bOnApplayArray = true;
						break;
					}
				}

				if (!bOnApplayArray)
				{
					//发送消息
					m_pITableFrame->SendTableData(wChairID, SUB_S_ROBOT_BANKER);

					//人数统计
					nSendBankerMsgCount++;
					if (nSendBankerMsgCount >= nRobotBankerCount)
						break;
				}
			}
		}
	}
}

//是否衰减
bool CTableFrameSink::NeedDeductStorage()
{

	for ( int i = 0; i < GAME_PLAYER; ++i )
	{
		IServerUserItem *pIServerUserItem=m_pITableFrame->GetTableUserItem(i);
		if (pIServerUserItem == NULL ) continue; 

		if(!pIServerUserItem->IsAndroidUser())
		{
			for (int nAreaIndex=0; nAreaIndex<=AREA_COUNT; ++nAreaIndex) 
			{
				if (m_lUserJettonScore[nAreaIndex][i]!=0)
				{
					return true;
				}				
			}	
		}
	}

	return false;

}

//测试写信息
void CTableFrameSink::WriteInfo( LPCTSTR pszString )
{
	m_pITableFrame->SendGameMessage(pszString, SMT_GAMELOG);
	// //设置语言区域
	// char* old_locale = _strdup( setlocale(LC_CTYPE,NULL) );
	// setlocale( LC_CTYPE, "chs" );

	// CStdioFile myFile;
	// CString strFileName;
	// strFileName.Format(TEXT("豪车俱乐部控制日记.txt"));
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

//读取配置
void CTableFrameSink::ReadConfigInformation()
{
	//获取自定义配置
	tagCustomConfig *pCustomConfig = (tagCustomConfig *)m_pGameServiceOption->cbCustomRule;
	ASSERT(pCustomConfig);

	//上庄
	m_lApplyBankerCondition = pCustomConfig->CustomGeneral.lApplyBankerCondition;
	m_lBankerMAX = pCustomConfig->CustomGeneral.lBankerTime;
	m_lBankerAdd = pCustomConfig->CustomGeneral.lBankerTimeAdd;
	m_lBankerScoreMAX = pCustomConfig->CustomGeneral.lBankerScoreMAX;
	m_lBankerScoreAdd = pCustomConfig->CustomGeneral.lBankerTimeExtra;
	m_bEnableSysBanker = (pCustomConfig->CustomGeneral.nEnableSysBanker == TRUE)?true:false;

	//时间
	m_cbFreeTime = pCustomConfig->CustomGeneral.cbFreeTime;
	m_cbBetTime = pCustomConfig->CustomGeneral.cbBetTime;
	m_cbEndTime = pCustomConfig->CustomGeneral.cbEndTime;
	if(m_cbFreeTime < TIME_FREE	|| m_cbFreeTime > 99) m_cbFreeTime = TIME_FREE;
	if(m_cbBetTime < TIME_PLACE_JETTON || m_cbBetTime > 99) m_cbBetTime = TIME_PLACE_JETTON;
	if(m_cbEndTime < TIME_GAME_END || m_cbEndTime > 99) m_cbEndTime = TIME_GAME_END;

	//下注
	m_lAreaLimitScore = pCustomConfig->CustomGeneral.lAreaLimitScore;
	m_lUserLimitScore = pCustomConfig->CustomGeneral.lUserLimitScore;
	m_nJettonMultiple = pCustomConfig->CustomGeneral.nMultiple;
	if(m_nJettonMultiple<=0) m_nJettonMultiple=1;
	//库存
	m_StorageStart = pCustomConfig->CustomGeneral.StorageStart;
	m_lStorageCurrent = m_StorageStart;
	m_StorageDeduct = pCustomConfig->CustomGeneral.StorageDeduct;
	m_lStorageMax1 = pCustomConfig->CustomGeneral.StorageMax1;
	m_lStorageMul1 = pCustomConfig->CustomGeneral.StorageMul1;
	m_lStorageMax2 = pCustomConfig->CustomGeneral.StorageMax2;
	m_lStorageMul2 = pCustomConfig->CustomGeneral.StorageMul2;
	if(m_lStorageMul1 < 0 || m_lStorageMul1 > 100 ) m_lStorageMul1 = 50;
	if(m_lStorageMul2 < 0 || m_lStorageMul2 > 100 ) m_lStorageMul2 = 80;

	//机器人
	m_nRobotListMaxCount = pCustomConfig->CustomAndroid.lRobotListMaxCount;

	LONGLONG lRobotBetMinCount = pCustomConfig->CustomAndroid.lRobotBetMinCount;
	LONGLONG lRobotBetMaxCount = pCustomConfig->CustomAndroid.lRobotBetMaxCount;
	m_nMaxChipRobot = rand()%(lRobotBetMaxCount-lRobotBetMinCount+1) + lRobotBetMinCount;
	if (m_nMaxChipRobot < 0)	m_nMaxChipRobot = 8;
	m_lRobotAreaLimit = pCustomConfig->CustomAndroid.lRobotAreaLimit;


	if ( m_lBankerScoreMAX <= m_lApplyBankerCondition)
		m_lBankerScoreMAX = 0l;

	m_lPlayerBankerMAX = m_lBankerMAX;

	//区域几率
	m_nMultiple[ID_LANDROVER-1] = 20;
	m_nMultiple[ID_BMW-1] = 20;
	m_nMultiple[ID_MASERATI-1] = 5;
	m_nMultiple[ID_FERRARI-1] =5;
	m_nMultiple[ID_LAMBORGHINI-1] = 5;
	m_nMultiple[ID_PORSCHE-1] = 5;
	m_nMultiple[ID_JAGUAR-1] = 20;
	m_nMultiple[ID_BENZ-1] = 20;

}
void CTableFrameSink::JudgeSystemScore( LONGLONG& lSystemScore)
{
	BYTE  bcResulteOut[AREA_COUNT];
	memset(bcResulteOut,0,AREA_COUNT);
	m_GameLogic.GetCardType(m_cbTableCard,1,bcResulteOut);
	//玩家下注
	LONGLONG *pUserScore[AREA_COUNT+1];
	pUserScore[0] = NULL;
	for (int i = 1;i<AREA_COUNT+1;i++)
	{
		pUserScore[i] = m_lUserJettonScore[i];
	}

	//庄家是不是机器人
	bool bIsBankerAndroidUser = false;
	if ( m_wCurrentBanker != INVALID_CHAIR )
	{
		IServerUserItem * pIBankerUserItem = m_pITableFrame->GetTableUserItem(m_wCurrentBanker);
		if (pIBankerUserItem != NULL) 
		{
			bIsBankerAndroidUser = pIBankerUserItem->IsAndroidUser();
		}
	}

	//计算积分
	for (WORD wChairID=0; wChairID<GAME_PLAYER; wChairID++)
	{
		//庄家判断
		if (m_wCurrentBanker == wChairID) continue;

		//获取用户
		IServerUserItem * pIServerUserItem=m_pITableFrame->GetTableUserItem(wChairID);
		if (pIServerUserItem==NULL) 
			continue;

		bool bIsAndroidUser = pIServerUserItem->IsAndroidUser();

		for (WORD wAreaIndex = 1; wAreaIndex <= AREA_COUNT; ++wAreaIndex)
		{

			if (bcResulteOut[wAreaIndex-1]>1) 
			{
				if ( bIsAndroidUser )
					lSystemScore += (pUserScore[wAreaIndex][wChairID] *(bcResulteOut[wAreaIndex-1]-1));

				if (m_wCurrentBanker == INVALID_CHAIR || bIsBankerAndroidUser)
					lSystemScore -= (pUserScore[wAreaIndex][wChairID] *(bcResulteOut[wAreaIndex-1]-1));
			}
			else
			{
				if (bcResulteOut[wAreaIndex-1]==0)
				{
					if ( bIsAndroidUser )
						lSystemScore -= pUserScore[wAreaIndex][wChairID];

					if (m_wCurrentBanker == INVALID_CHAIR || bIsBankerAndroidUser)
						lSystemScore += pUserScore[wAreaIndex][wChairID];
				}
			}
		}
	}

}

//积分事件
bool CTableFrameSink::OnUserScroeNotify(WORD wChairID, IServerUserItem * pIServerUserItem, BYTE cbReason)
{
	//当庄家分数在空闲时间变动时(即庄家进行了存取款)校验庄家的上庄条件
	if(wChairID == m_wCurrentBanker && m_pITableFrame->GetGameStatus() == GAME_STATUS_FREE)
	{
		ChangeBanker(false);
	}

	return true;
}

