﻿#include "StdAfx.h"
#include "TableFrameSink.h"
#include <locale>

//////////////////////////////////////////////////////////////////////////

//限制变量
bool				CTableFrameSink::g_bControl=false;					//调控标志
LONGLONG			CTableFrameSink::g_lMaxWinScore=5000000L;			//最大赢分
LONGLONG			CTableFrameSink::g_lMaxVarietycore=2000000L;		//最大变化

//索引变量
CUserTracingMap		CTableFrameSink::g_UserTracingMap;					//用户记录
CUserAddressMap		CTableFrameSink::g_UserAddressMap;					//限制地址
CBlackListIDMap		CTableFrameSink::g_BlackListIDMap;					//限制用户

LONGLONG			CTableFrameSink::g_lStockScore = 0l;
LONGLONG			CTableFrameSink::g_lRobotScoreMin = 0l;
LONGLONG			CTableFrameSink::g_lRobotScoreMax = 0l;
LONGLONG			CTableFrameSink::g_lRobotBankTake[2] = {0, 0};
LONGLONG			CTableFrameSink::g_lRobotBankSave = 0l;

//动作标识
#define IDI_ADD_SCORE				1									//加注定时器
#define TIME_ADD_SCORE				1000								//结束定时器

//////////////////////////////////////////////////////////////////////////

//构造函数
CTableFrameSink::CTableFrameSink()
{
	//游戏变量
	m_bShowHand=false;
	m_bChaosCard=false;
	m_wCurrentUser=INVALID_CHAIR;
	m_lServiceCharge = 0L;

	//房卡信息
	m_bPersonalRoom=false;
	m_lPersonalCellScore=0;
	m_lPersonalInitScore=0;
	m_cbPersonalGameCount=0;
	m_cbPlayerCount=GAME_PLAYER;
	ZeroMemory(&m_stRecord, sizeof(m_stRecord));
	//下注信息
	m_lDrawMaxScore=0L;
	m_lTurnMaxScore=0L;
	m_lTurnLessScore=0L;
	m_lDrawCellScore=0L;

	//用户状态
	ZeroMemory(m_cbPlayStatus,sizeof(m_cbPlayStatus));
	ZeroMemory(m_cbOperaScore,sizeof(m_cbOperaScore));
	ZeroMemory(m_bGiveUp,sizeof(m_bGiveUp));

	//金币信息
	ZeroMemory(m_lUserScore,sizeof(m_lUserScore));
	ZeroMemory(m_lTableScore,sizeof(m_lTableScore));
	ZeroMemory(m_lUserMaxScore,sizeof(m_lUserMaxScore));

	//扑克变量
	m_cbSendCardCount=0;
	ZeroMemory(m_cbCardCount,sizeof(m_cbCardCount));
	ZeroMemory(m_cbHandCardData,sizeof(m_cbHandCardData));

	//组件变量
	m_pITableFrame=NULL;
	m_pGameServiceOption=NULL;

	//服务控制
	m_bAllAndroid = false;
	m_hControlInst = NULL;
	m_pServerContro = NULL;
	m_hControlInst = LoadLibrary(TEXT("HKFiveCardNewServerControl.dll"));
	if ( m_hControlInst )
	{
		typedef void * (*CREATE)(); 
		CREATE ServerControl = (CREATE)GetProcAddress(m_hControlInst,"CreateServerControl"); 
		if ( ServerControl )
		{
			m_pServerContro = static_cast<IServerControl*>(ServerControl());
		}
	}

	//游戏视频
	m_hVideoInst = NULL;
	m_pGameVideo = NULL;
	m_hVideoInst = LoadLibrary(TEXT("HKFiveCardNewGameVideo.dll"));
	if ( m_hVideoInst )
	{
		typedef void * (*CREATE)(); 
		CREATE GameVideo = (CREATE)GetProcAddress(m_hVideoInst,"CreateGameVideo"); 
		if ( GameVideo )
		{
			m_pGameVideo = static_cast<IGameVideo*>(GameVideo());
		}
	}	

	return;
}

//析构函数
CTableFrameSink::~CTableFrameSink()
{
	if( m_pServerContro )
	{
		delete m_pServerContro;
		m_pServerContro = NULL;
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

//释放对象
VOID CTableFrameSink::Release()
{
	if( m_pServerContro )
	{
		delete m_pServerContro;
		m_pServerContro = NULL;
	}

	if( m_hControlInst )
	{
		FreeLibrary(m_hControlInst);
		m_hControlInst = NULL;
	}

	delete this;
}

//接口查询
VOID * CTableFrameSink::QueryInterface(REFGUID Guid, DWORD dwQueryVer)
{
	QUERYINTERFACE(ITableFrameSink,Guid,dwQueryVer);
	QUERYINTERFACE(ITableUserAction,Guid,dwQueryVer);
	QUERYINTERFACE_IUNKNOWNEX(ITableFrameSink,Guid,dwQueryVer);
	return NULL;
}

//配置桌子
bool CTableFrameSink::Initialization(IUnknownEx * pIUnknownEx)
{
	//查询接口
	m_pITableFrame=QUERY_OBJECT_PTR_INTERFACE(pIUnknownEx,ITableFrame);
	if (m_pITableFrame==NULL) return false;

	//查询配置
	m_pGameServiceOption=m_pITableFrame->GetGameServiceOption();

	//开始模式
	m_pITableFrame->SetStartMode(START_MODE_ALL_READY);

	//设置文件名
	TCHAR szPath[MAX_PATH]=TEXT("");
	TCHAR szConfigFileName[MAX_PATH]=TEXT("");
	GetCurrentDirectory(sizeof(szPath),szPath);
	_sntprintf(szConfigFileName, sizeof(szConfigFileName), TEXT("%s\\HKFiveCardNewConfig.ini"), szPath);

	//变量定义
	TCHAR OutBuf[255];
	ZeroMemory(OutBuf, sizeof(OutBuf));
	GetPrivateProfileString(m_pGameServiceOption->szServerName, TEXT("StorageStart"), TEXT("0"), OutBuf, CountArray(OutBuf),szConfigFileName);
	_sntscanf(OutBuf, CountArray(OutBuf), TEXT("%I64d"), &g_lStockScore);
	
	ZeroMemory(OutBuf, sizeof(OutBuf));
	GetPrivateProfileString(m_pGameServiceOption->szServerName, TEXT("StorageDeduct"), TEXT("0"), OutBuf, 255, szConfigFileName);
	_sntscanf(OutBuf, lstrlen(OutBuf), TEXT("%I64d"), &m_lStorageDeduct);

	ZeroMemory(OutBuf, sizeof(OutBuf));
	GetPrivateProfileString(m_pGameServiceOption->szServerName, TEXT("StorageMax"), TEXT("50000000"), OutBuf, 255, szConfigFileName);
	_sntscanf(OutBuf, lstrlen(OutBuf), TEXT("%I64d"), &m_lStorageMax);
	
	ZeroMemory(OutBuf, sizeof(OutBuf));
	GetPrivateProfileString(m_pGameServiceOption->szServerName, TEXT("StorageMul"), TEXT("50"), OutBuf, 255, szConfigFileName);
	_sntscanf(OutBuf, lstrlen(OutBuf), TEXT("%I64d"), &m_nStorageMul);

	ZeroMemory(OutBuf, sizeof(OutBuf));
	GetPrivateProfileString(m_pGameServiceOption->szServerName, TEXT("RobotScoreMin"), TEXT("5000"), OutBuf, CountArray(OutBuf),szConfigFileName);
	_sntscanf(OutBuf, CountArray(OutBuf), TEXT("%I64d"), &g_lRobotScoreMin);

	ZeroMemory(OutBuf, sizeof(OutBuf));
	GetPrivateProfileString(m_pGameServiceOption->szServerName, TEXT("RobotScoreMax"), TEXT("10000000"), OutBuf, CountArray(OutBuf),szConfigFileName);
	_sntscanf(OutBuf, CountArray(OutBuf), TEXT("%I64d"), &g_lRobotScoreMax);

	ZeroMemory(OutBuf, sizeof(OutBuf));
	GetPrivateProfileString(m_pGameServiceOption->szServerName, TEXT("RobotBankTakeMin"), TEXT("10000"), OutBuf, CountArray(OutBuf),szConfigFileName);
	_sntscanf(OutBuf, CountArray(OutBuf), TEXT("%I64d"), &g_lRobotBankTake[0]);

	ZeroMemory(OutBuf, sizeof(OutBuf));
	GetPrivateProfileString(m_pGameServiceOption->szServerName, TEXT("RobotBankTakeMax"), TEXT("1000000"), OutBuf, CountArray(OutBuf),szConfigFileName);
	_sntscanf(OutBuf, CountArray(OutBuf), TEXT("%I64d"), &g_lRobotBankTake[1]);

	ZeroMemory(OutBuf, sizeof(OutBuf));
	GetPrivateProfileString(m_pGameServiceOption->szServerName, TEXT("RobotBankSave"), TEXT("10"), OutBuf, CountArray(OutBuf),szConfigFileName);
	_sntscanf(OutBuf, CountArray(OutBuf), TEXT("%I64d"), &g_lRobotBankSave);

	if( g_lRobotScoreMax < g_lRobotScoreMin )
		g_lRobotScoreMax = g_lRobotScoreMin;

	if( g_lRobotBankTake[1] <= g_lRobotBankTake[0] )
		g_lRobotBankTake[1] = g_lRobotBankTake[0] + 1;

	if( g_lRobotBankSave > 100 )
		g_lRobotBankSave = 100;

	return true;
}

//复位桌子
VOID CTableFrameSink::RepositionSink()
{
	//游戏变量
	m_bShowHand=false;
	m_bChaosCard=false;
	m_wCurrentUser=INVALID_CHAIR;

	//下注信息
	m_lDrawMaxScore=0L;
	m_lTurnMaxScore=0L;
	m_lTurnLessScore=0L;
	m_lDrawCellScore=0L;

	//用户状态
	ZeroMemory(m_cbPlayStatus,sizeof(m_cbPlayStatus));
	ZeroMemory(m_cbOperaScore,sizeof(m_cbOperaScore));
	ZeroMemory(m_bGiveUp,sizeof(m_bGiveUp));

	//金币信息
	ZeroMemory(m_lUserScore,sizeof(m_lUserScore));
	ZeroMemory(m_lTableScore,sizeof(m_lTableScore));
	ZeroMemory(m_lUserMaxScore,sizeof(m_lUserMaxScore));

	//扑克变量
	m_cbSendCardCount=0;
	ZeroMemory(m_cbCardCount,sizeof(m_cbCardCount));
	ZeroMemory(m_cbHandCardData,sizeof(m_cbHandCardData));

	return;
}

//游戏开始
bool CTableFrameSink::OnEventGameStart()
{
	//当前库存
	SendStorage();

	//删除定时器
	m_pITableFrame->KillGameTimer(IDI_ADD_SCORE);

	//设置状态
	m_pITableFrame->SetGameStatus(GAME_SCENE_PLAY);

	//私人房标志
	m_bPersonalRoom = (m_pGameServiceOption->wServerType&GAME_GENRE_PERSONAL);

	//派发扑克
	m_cbSendCardCount=2;
	m_GameLogic.RandCardList(m_cbHandCardData[0],sizeof(m_cbHandCardData)/sizeof(m_cbHandCardData[0][0]));

	int nPlayCount = 0;
	int nAndroidCount = 0;
	for (WORD i=0;i<m_cbPlayerCount;i++)
	{
		//获取用户
		IServerUserItem * pIServerUserItem=m_pITableFrame->GetTableUserItem(i);

		//设置变量
		if (pIServerUserItem!=NULL)
		{
			nPlayCount++;
			if (pIServerUserItem->IsAndroidUser())
			{
				nAndroidCount++;
			}
		}
	}

	//全部是机器人
	m_bAllAndroid = (nAndroidCount != 0 && nAndroidCount == nPlayCount);

	//底注积分
	if(m_bPersonalRoom)
	{
		m_lDrawCellScore = m_lPersonalCellScore;
	}
	else
	{
		m_lDrawCellScore=m_pGameServiceOption->lCellScore;
	}

	//服务费
	m_lServiceCharge = 0;

	//变量设置
	for (WORD i=0;i<m_cbPlayerCount;i++)
	{
		//获取用户
		IServerUserItem * pIServerUserItem=m_pITableFrame->GetTableUserItem(i);

		//设置变量
		if (pIServerUserItem!=NULL)
		{
			//状态设置
			m_cbPlayStatus[i]=TRUE;
			m_lTableScore[i]=m_lDrawCellScore;

			if(m_bPersonalRoom)
				m_lUserMaxScore[i] = m_lPersonalInitScore;
			else
				m_lUserMaxScore[i] = (pIServerUserItem->GetUserScore() - m_lServiceCharge);

			//扑克设置
			m_cbCardCount[i]=m_cbSendCardCount;
		}
	}

	//下注计算
	RectifyMaxScore();

	//限制处理
	if (g_bControl==true) ControlUserCard();

	//设置变量
	m_wCurrentUser=EstimateWinner(1,1);
	m_lTurnLessScore=m_lUserScore[m_wCurrentUser]+m_lTableScore[m_wCurrentUser];

	if(m_pGameVideo)
	{
		m_pGameVideo->StartVideo(m_pITableFrame);
	}


	//变量定义
	CMD_S_GameStart GameStart;
	ZeroMemory(&GameStart,sizeof(GameStart));

	//设置变量
	GameStart.lDrawMaxScore=m_lDrawMaxScore;
	GameStart.lTurnMaxScore=m_lTurnMaxScore;
	GameStart.lTurnLessScore=m_lTurnLessScore;
	GameStart.lCellScore=(LONG)m_lDrawCellScore;
	GameStart.lServiceCharge=(LONG)m_lServiceCharge;

	// 判断库存
	if( g_lStockScore < m_lDrawMaxScore )
	{
		// 获取最大牌位置
		WORD wWinerUser = EstimateWinner(0, MAX_COUNT-1);

		// 判断赢家不是机器人
		IServerUserItem * pIWinnerUserItem = m_pITableFrame->GetTableUserItem(wWinerUser);
		if( pIWinnerUserItem && !pIWinnerUserItem->IsAndroidUser() )
		{
			// 获取所有机器人位置
			CWHArray<WORD>	ArrayRobot;
			for (WORD i = 0; i < m_cbPlayerCount; i++)
			{
				IServerUserItem * pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if( pIServerUserItem && pIServerUserItem->IsAndroidUser() )
				{
					// 添加机器人
					ArrayRobot.Add(i);
				}
			}

			if(ArrayRobot.GetCount() > 0)
			{
				// 交换
				WORD wExchange = ArrayRobot[rand()%ArrayRobot.GetCount()];
				BYTE cbCardData[MAX_COUNT];
				CopyMemory(cbCardData, m_cbHandCardData[wExchange], sizeof(BYTE) * MAX_COUNT );
				CopyMemory(m_cbHandCardData[wExchange], m_cbHandCardData[wWinerUser], sizeof(BYTE) * MAX_COUNT );
				CopyMemory(m_cbHandCardData[wWinerUser], cbCardData, sizeof(BYTE) * MAX_COUNT );
			}
		}
	}
	else if(g_lStockScore > m_lStorageMax && rand()%100 < m_nStorageMul) //系统送分
	{
		// 获取最大牌位置
		WORD wWinerUser = EstimateWinner(0, MAX_COUNT-1);

		// 判断赢家是机器人
		IServerUserItem * pIWinnerUserItem = m_pITableFrame->GetTableUserItem(wWinerUser);

		if( pIWinnerUserItem && pIWinnerUserItem->IsAndroidUser())
		{
			// 获取所有用户位置
			CWHArray<WORD>	ArrayUser;
			for (WORD i = 0; i < m_cbPlayerCount; i++)
			{
				IServerUserItem * pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if(pIServerUserItem && !pIServerUserItem->IsAndroidUser())
				{
					// 添加用户
					ArrayUser.Add(i);
				}
			}

			if(ArrayUser.GetCount() > 0)
			{
				//交换
				WORD wExchange = ArrayUser[rand()%ArrayUser.GetCount()];
				BYTE cbCardData[MAX_COUNT];
				CopyMemory(cbCardData, m_cbHandCardData[wExchange], sizeof(BYTE) * MAX_COUNT );
				CopyMemory(m_cbHandCardData[wExchange], m_cbHandCardData[wWinerUser], sizeof(BYTE) * MAX_COUNT );
				CopyMemory(m_cbHandCardData[wWinerUser], cbCardData, sizeof(BYTE) * MAX_COUNT );
			}
		}
	}
	
	m_wCurrentUser=EstimateWinner(1,1);
	GameStart.wCurrentUser=m_wCurrentUser;

	//设置扑克
	for (WORD i=0;i<m_cbPlayerCount;i++)
	{
		if (m_cbPlayStatus[i]==FALSE) GameStart.cbCardData[i]=0;
		else GameStart.cbCardData[i]=m_cbHandCardData[i][1];
	}

	//发送数据
	for (WORD i=0;i<m_cbPlayerCount;i++)
	{
		// 机器人数据
		IServerUserItem * pIServerUserItem=m_pITableFrame->GetTableUserItem(i);
		if ( pIServerUserItem != NULL)
		{
			if(pIServerUserItem->IsAndroidUser())
			{
				CopyMemory(GameStart.cbHandCardData, m_cbHandCardData, sizeof(GameStart.cbHandCardData));
			}
			else
			{
				ZeroMemory(GameStart.cbHandCardData, sizeof(GameStart.cbHandCardData));
				CopyMemory(GameStart.cbHandCardData[i], m_cbHandCardData[i], sizeof(GameStart.cbHandCardData[i]));
			}
		}

		//游戏数据
		if (m_cbPlayStatus[i]==TRUE)
		{
			GameStart.cbObscureCard=m_cbHandCardData[i][0];
			m_pITableFrame->SendTableData(i,SUB_S_GAME_START,&GameStart,sizeof(GameStart));

			if (m_pGameVideo)
			{
				Video_GameStart video;
				lstrcpyn(video.szNickName,pIServerUserItem->GetNickName(),CountArray(video.szNickName));
				video.wChairID			= i;
				if(m_bPersonalRoom)
					video.lScore = m_lPersonalInitScore;
				else
					video.lScore = pIServerUserItem->GetUserScore();

				video.lPersonalInitScore=m_lPersonalInitScore;
				video.cbPersonalPlayerMode=m_cbPlayerCount;
				video.cbPersonalGameCount=m_cbPersonalGameCount;

				//以下变量与GameStart一样
				video.lCellScore=GameStart.lCellScore;
				video.lServiceCharge = GameStart.lServiceCharge;
				video.lDrawMaxScore = GameStart.lDrawMaxScore;
				video.lTurnMaxScore = GameStart.lTurnMaxScore;
				video.lTurnLessScore = GameStart.lTurnLessScore;

				video.wCurrentUser	= GameStart.wCurrentUser;
				video.cbObscureCard	= GameStart.cbObscureCard;
				CopyMemory(video.cbCardData,GameStart.cbCardData,sizeof(video.cbCardData));
				CopyMemory(video.cbHandCardData,GameStart.cbHandCardData,sizeof(video.cbHandCardData));

				m_pGameVideo->AddVideoData(SUB_S_GAME_START,&video,i==0);
			}
		}

		//旁观数据
		GameStart.cbObscureCard=(m_cbPlayStatus[i]==TRUE)?0xFF:0x00;
		m_pITableFrame->SendLookonData(i,SUB_S_GAME_START,&GameStart,sizeof(GameStart));


	}
	
	//保存名字
	for (WORD i=0;i<m_cbPlayerCount;i++)
	{
		//获取用户
		IServerUserItem * pIServerUserItem=m_pITableFrame->GetTableUserItem(i);

		//设置变量
		if (pIServerUserItem!=NULL)
		{
			m_strPalyName[i].Format(TEXT("%s"), pIServerUserItem->GetNickName() );
		}
	}

	if( m_pServerContro )
	{
		//服务信息
		BYTE cbHandCardData[GAME_PLAYER][5];
		CopyMemory(cbHandCardData, m_cbHandCardData, sizeof(cbHandCardData) );

		//排列扑克
		//for (WORD i= 0;i < GAME_PLAYER; i++)
		//	m_GameLogic.SortCardList(cbHandCardData[i], 5 );

		//信息结算
		m_pServerContro->ServerControl(cbHandCardData, m_pITableFrame);
	}

	//启动定时器
	m_pITableFrame->SetGameTimer(IDI_ADD_SCORE,TIME_ADD_SCORE*46,1,0);

	return true;
}

//游戏结束
bool CTableFrameSink::OnEventGameConclude(WORD wChairID, IServerUserItem * pIServerUserItem, BYTE cbReason)
{
	switch (cbReason)
	{
	case GER_NORMAL:		//常规结束
	case GER_NO_PLAYER:		//没有玩家
		{
			//定义变量
			CMD_S_GameEnd GameEnd;
			ZeroMemory(&GameEnd,sizeof(GameEnd));

			GameEnd.lMinTableScore = m_pGameServiceOption->lMinTableScore;

			//计算总注
			LONGLONG lDrawScore=0L;
			for (WORD i=0;i<CountArray(m_lTableScore);i++) lDrawScore+=m_lTableScore[i];

			//变量定义
			WORD wWinerUser=EstimateWinner(0,MAX_COUNT-1);

			//积分变量
			tagScoreInfo ScoreInfoArray[GAME_PLAYER];
			ZeroMemory(&ScoreInfoArray,sizeof(ScoreInfoArray));

			// 延时关闭
			bool bDelayOverGame=false;

			// 统计积分
			for (WORD i=0;i<m_cbPlayerCount;i++)
			{
				if ( m_cbPlayStatus[i] == TRUE ) 
				{
					//成绩计算
					LONGLONG lUserScore = (i == wWinerUser)?(lDrawScore - m_lTableScore[i]): -m_lTableScore[i];

					//设置积分
					ScoreInfoArray[i].lScore = lUserScore;
					ScoreInfoArray[i].cbType = (ScoreInfoArray[i].lScore >= 0L) ? SCORE_TYPE_WIN : SCORE_TYPE_LOSE;
					//计算税收
					ScoreInfoArray[i].lRevenue=m_pITableFrame->CalculateRevenue(i,ScoreInfoArray[i].lScore);
					if (ScoreInfoArray[i].lRevenue>0L) ScoreInfoArray[i].lScore-=ScoreInfoArray[i].lRevenue;
					GameEnd.lGameScore[i] = ScoreInfoArray[i].lScore;

					//扑克信息
					GameEnd.cbCardData[i]=(cbReason!=GER_NO_PLAYER)?m_cbHandCardData[i][0]:0;

					//游戏调控
					IServerUserItem * pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
					if ( g_bControl == true && pIServerUserItem) 
					{
						OnEventScoreAlter(pIServerUserItem->GetUserID(),ScoreInfoArray[i].lScore);
					}
					
					// 扣除服务费
					lUserScore=pIServerUserItem->GetUserScore();
					if(m_bPersonalRoom)
						lUserScore = m_lPersonalInitScore;
					if ( (ScoreInfoArray[i].lScore - m_lServiceCharge) < 0 
						 && -(ScoreInfoArray[i].lScore - m_lServiceCharge) > lUserScore )
					{
						ScoreInfoArray[i].lScore = -lUserScore;
					}
					else
					{
						ScoreInfoArray[i].lScore -= m_lServiceCharge;
					}

				}
				else
				{
					//设置扑克
					GameEnd.cbCardData[i] = 0;

					//设置成绩
					GameEnd.lGameScore[i] = -m_lTableScore[i];

					if ((m_pGameServiceOption->wServerType&GAME_GENRE_PERSONAL) != 0 && m_lTableScore[i] >0)//房卡模式
					{
						ScoreInfoArray[i].cbType = SCORE_TYPE_LOSE;
						ScoreInfoArray[i].lScore = -m_lTableScore[i];
					}
				}

				//历史积分
				m_HistoryScore.OnEventUserScore(i,GameEnd.lGameScore[i]);
			}

			// 结束消息
			for ( int j = 0;j<m_cbPlayerCount;j++)
			{
				IServerUserItem * pIServerUserItemSend = m_pITableFrame->GetTableUserItem(j);
				if (pIServerUserItemSend != NULL)
				{
					CString strOver(TEXT("本局结束，成绩统计："));
					for ( int i = 0;i<m_cbPlayerCount;i++)
					{						
						if ( GameEnd.lGameScore[i] != 0)
						{		
							CString str;	
							if (i == j)
							{
								int nStrLen = 0;
								for ( int j = 0 ; j < lstrlen(m_strPalyName[i]); ++j )
								{
									if( m_strPalyName[i].GetAt(j) >= 0xA0 )//全角 中文 
									{ 
										nStrLen += 12;
									} 
									else					//半角 英文 
									{ 
										nStrLen += 6;
									} 

									if ( nStrLen >= 60 )
									{
										for ( int k = j + 1; k < lstrlen(m_strPalyName[i]); ++k )
										{
											if ( k - j == 4 )
											{
												m_strPalyName[i].SetAt(k, '\0');
												break;
											}
											else
											{
												m_strPalyName[i].SetAt(k, '.');
											}
										}
										break;
									}
								}

								str.Format(TEXT("\n【%s】：%I64d"), m_strPalyName[i], GameEnd.lGameScore[i]);
								strOver += str;
							}
							else
							{
								str.Format(TEXT("\n【%s】：%I64d"), TEXT("游戏玩家"), GameEnd.lGameScore[i]);
								strOver += str;
							}
						}						
					}
					if ( !strOver.IsEmpty() )
					{
						m_pITableFrame->SendGameMessage(pIServerUserItemSend, strOver, SMT_CHAT );
					}
				}
			}

			if (m_pGameVideo)
			{
				m_pGameVideo->AddVideoData(SUB_S_GAME_END, &GameEnd);
				m_pGameVideo->StopAndSaveVideo(m_pGameServiceOption->wServerID, m_pITableFrame->GetTableID());
			}

			if (m_bPersonalRoom && m_stRecord.nCount < 32)
			{
				for (int i = 0; i < m_cbPlayerCount; i++)
				{
					m_stRecord.lDetailScore[i][m_stRecord.nCount] = GameEnd.lGameScore[i];
					m_stRecord.lAllScore[i] += GameEnd.lGameScore[i];
				}
				m_stRecord.nCount++;

				m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_RECORD, &m_stRecord, sizeof(m_stRecord));
			}

			//写入积分
			m_pITableFrame->WriteTableScore(ScoreInfoArray,m_cbPlayerCount);
			// 检测发送喇叭
			CMD_GC_GTrumpet sTrumpet;
			for (WORD i=0; i<GAME_PLAYER; i++) {
				IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if(!pIServerUserItem) {
					continue;
				}
				if (GameEnd.lGameScore[i] > 0) {
					ZeroMemory(&sTrumpet, sizeof(sTrumpet));
					sTrumpet.dwCardType = GameEnd.cbCardData[i];
					sTrumpet.lScore = GameEnd.lGameScore[i];
					m_pITableFrame->SendUserItemData(pIServerUserItem, SUB_S_WIN_TRUMPET, &sTrumpet, sizeof(sTrumpet));
				}
			}
			//发送数据
			m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));
			m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));
			


			//写入库存
			for (WORD i=0;i<m_cbPlayerCount;i++)
			{
				//状态判断
				if (m_cbPlayStatus[i]==FALSE) continue;

				//变量定义
				IServerUserItem * pIServerUserItem=m_pITableFrame->GetTableUserItem(i);
				if( pIServerUserItem->IsAndroidUser() && !m_bAllAndroid)
					g_lStockScore += ScoreInfoArray[i].lScore;
			}

			//库存衰减
			if(g_lStockScore>0 && !m_bAllAndroid)
			{
				g_lStockScore=g_lStockScore-g_lStockScore*m_lStorageDeduct/1000;
			}

			m_pITableFrame->ConcludeGame(GAME_SCENE_FREE);

			if ((m_pGameServiceOption->wServerType&GAME_GENRE_PERSONAL) != 0)//房卡模式
			{
				if (m_pITableFrame->IsPersonalRoomDisumme())//当前朋友局解散清理记录
				{
					ZeroMemory(&m_stRecord, sizeof(m_stRecord));
				}
			}

			// 给机器人发送真正结束消息
			for( int nIndex = 0; nIndex < m_cbPlayerCount; ++nIndex )
			{
				IServerUserItem* pAndroidUser = m_pITableFrame->GetTableUserItem(nIndex);
				if( pAndroidUser && pAndroidUser->IsAndroidUser() )
				{
					m_pITableFrame->SendUserItemData(pAndroidUser, SUB_S_TRUE_END);
				}
			}
		

			return true;
		}
	case GER_DISMISS:		//游戏解散
		{
			//变量定义
			CMD_S_GameEnd GameEnd;
			ZeroMemory(&GameEnd,sizeof(GameEnd));
			
			GameEnd.lMinTableScore = m_pGameServiceOption->lMinTableScore;

			//发送信息
			m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));
			m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GAME_END,&GameEnd,sizeof(GameEnd));

			if(m_pGameVideo)
			{
				m_pGameVideo->AddVideoData(SUB_S_GAME_END,&GameEnd);
				m_pGameVideo->StopAndSaveVideo(m_pGameServiceOption->wServerID,m_pITableFrame->GetTableID());
			}


			if ((m_pGameServiceOption->wServerType&GAME_GENRE_PERSONAL) != 0)//房卡模式
			{
				if (m_pITableFrame->IsPersonalRoomDisumme())//当前朋友局解散清理记录
				{
					ZeroMemory(&m_stRecord, sizeof(m_stRecord));
				}
			}

			//结束游戏
			m_pITableFrame->ConcludeGame(GAME_SCENE_FREE);

			// 给机器人发送真正结束消息
			for( int nIndex = 0; nIndex < m_cbPlayerCount; ++nIndex )
			{
				IServerUserItem* pAndroidUser = m_pITableFrame->GetTableUserItem(nIndex);
				if( pAndroidUser && pAndroidUser->IsAndroidUser() )
				{
					m_pITableFrame->SendUserItemData(pAndroidUser, SUB_S_TRUE_END);
				}
			}

			return true;
		}
	case GER_NETWORK_ERROR:		//用户强退
	case GER_USER_LEAVE:		
		{
			if ( wChairID < m_cbPlayerCount  && m_cbPlayStatus[wChairID] == TRUE)
			{
				//放弃处理
				OnUserGiveUp(wChairID);
			}
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
			StatusFree.lCellScore=(LONG)m_lDrawCellScore;

			//历史积分
			for (WORD i=0;i<m_cbPlayerCount;i++)
			{
				tagHistoryScore * pHistoryScore=m_HistoryScore.GetHistoryScore(i);
				StatusFree.lTurnScore[i]=pHistoryScore->lTurnScore;
				StatusFree.lCollectScore[i]=pHistoryScore->lCollectScore;
			}

			//机器人
			StatusFree.lRobotScoreMin = g_lRobotScoreMin;					// 积分低于取款
			StatusFree.lRobotScoreMax = g_lRobotScoreMax;					// 积分高于存款
			StatusFree.lRobotBankTake[0] = g_lRobotBankTake[0];				// 取款额度
			StatusFree.lRobotBankTake[1] = g_lRobotBankTake[1];				// 取款额度
			StatusFree.lRobotBankSave = g_lRobotBankSave;					// 存款额度

			//私人房信息
			if(m_bPersonalRoom)
			{
				StatusFree.lPersonalInitScore = m_lPersonalInitScore;
				StatusFree.cbPersonalGameCount = m_cbPersonalGameCount;
				StatusFree.cbPersonalPlayerMode = m_cbPlayerCount;

				m_pITableFrame->SendTableData(wChairID, SUB_S_RECORD, &m_stRecord, sizeof(m_stRecord));
			}


			//发送场景
			return m_pITableFrame->SendGameScene(pIServerUserItem,&StatusFree,sizeof(StatusFree));
		}
	case GAME_SCENE_PLAY:	//游戏状态
		{
			//构造数据
			CMD_S_StatusPlay StatusPlay;
			ZeroMemory(&StatusPlay,sizeof(StatusPlay));

			//设置变量
			StatusPlay.lCellScore=(LONG)m_lDrawCellScore;
			StatusPlay.lServiceCharge=(LONG)m_lServiceCharge;
			
			//加注信息
			StatusPlay.lDrawMaxScore=m_lDrawMaxScore;
			StatusPlay.lTurnMaxScore=m_lTurnMaxScore;
			StatusPlay.lTurnLessScore=m_lTurnLessScore;
			CopyMemory(StatusPlay.lUserScore,m_lUserScore,sizeof(StatusPlay.lUserScore));
			CopyMemory(StatusPlay.lTableScore,m_lTableScore,sizeof(StatusPlay.lTableScore));

			//状态信息
			StatusPlay.wCurrentUser=m_wCurrentUser;
			StatusPlay.cbShowHand=(m_bShowHand==true)?TRUE:FALSE;
			CopyMemory(StatusPlay.cbPlayStatus,m_cbPlayStatus,sizeof(StatusPlay.cbPlayStatus));

			//机器人
			StatusPlay.lRobotScoreMin = g_lRobotScoreMin;					// 积分低于取款
			StatusPlay.lRobotScoreMax = g_lRobotScoreMax;					// 积分高于存款
			StatusPlay.lRobotBankTake[0] = g_lRobotBankTake[0];				// 取款额度
			StatusPlay.lRobotBankTake[1] = g_lRobotBankTake[1];				// 取款额度
			StatusPlay.lRobotBankSave = g_lRobotBankSave;					// 存款额度

			//历史积分
			for (WORD i=0;i<m_cbPlayerCount;i++)
			{
				tagHistoryScore * pHistoryScore=m_HistoryScore.GetHistoryScore(i);
				StatusPlay.lTurnScore[i]=pHistoryScore->lTurnScore;
				StatusPlay.lCollectScore[i]=pHistoryScore->lCollectScore;
			}

			//设置扑克
			for (WORD i=0;i<m_cbPlayerCount;i++)
			{
				//设置数目
				StatusPlay.cbCardCount[i]=m_cbCardCount[i];

				//设置扑克
				if (m_cbPlayStatus[i]==TRUE)
				{
					if ((i==wChairID)&&(pIServerUserItem->GetUserStatus()==US_LOOKON)) 
					{
						StatusPlay.cbHandCardData[i][0]=0;
					}
					else if ((i==wChairID)&&(bSendSecret==true)&&(pIServerUserItem->GetUserStatus()!=US_LOOKON))
					{
						StatusPlay.cbHandCardData[i][0]=m_cbHandCardData[i][0];
					}
					CopyMemory(&StatusPlay.cbHandCardData[i][1],&m_cbHandCardData[i][1],(m_cbCardCount[i]-1)*sizeof(BYTE));
				}
			}

			//私人房信息
			if (m_bPersonalRoom)
			{
				StatusPlay.lPersonalInitScore = m_lPersonalInitScore;
				StatusPlay.cbPersonalGameCount = m_cbPersonalGameCount;
				StatusPlay.cbPersonalPlayerMode = m_cbPlayerCount;

				m_pITableFrame->SendTableData(wChairID, SUB_S_RECORD, &m_stRecord, sizeof(m_stRecord));
			}

			//发送场景
			return m_pITableFrame->SendGameScene(pIServerUserItem,&StatusPlay,sizeof(StatusPlay));
		}
	}

	//错误断言
	ASSERT(FALSE);

	return false;
}

//定时器事件
bool CTableFrameSink::OnTimerMessage(DWORD dwTimerID, WPARAM wBindParam)
{
	if(dwTimerID==IDI_ADD_SCORE)
	{
		m_pITableFrame->KillGameTimer(IDI_ADD_SCORE);
		if (m_bPersonalRoom)
			return true;
		if (m_wCurrentUser!=INVALID_CHAIR)
			return OnUserGiveUp(m_wCurrentUser);
	}

	return false;
}

//游戏消息
bool CTableFrameSink::OnGameMessage(WORD wSubCmdID, VOID * pData, WORD wDataSize, IServerUserItem * pIServerUserItem)
{
	switch (wSubCmdID)
	{
	case SUB_C_GIVE_UP:			//用户放弃
		{
			//状态效验
			ASSERT(m_pITableFrame->GetGameStatus()==GAME_SCENE_PLAY);
			if (m_pITableFrame->GetGameStatus()!=GAME_SCENE_PLAY) return true;

			//用户效验
			ASSERT(pIServerUserItem->GetUserStatus()==US_PLAYING);
			if (pIServerUserItem->GetUserStatus()!=US_PLAYING) return true;

			//消息处理
			return OnUserGiveUp(pIServerUserItem->GetChairID());
		}
	case SUB_C_ADD_SCORE:		//用户加注
		{
			//效验数据
			ASSERT(wDataSize==sizeof(CMD_C_AddScore));
			if (wDataSize!=sizeof(CMD_C_AddScore)) return false;

			//状态效验
			ASSERT(m_pITableFrame->GetGameStatus()==GAME_SCENE_PLAY);
			if (m_pITableFrame->GetGameStatus()!=GAME_SCENE_PLAY) return true;

			//用户效验
			ASSERT(pIServerUserItem->GetUserStatus()==US_PLAYING);
			if (pIServerUserItem->GetUserStatus()!=US_PLAYING) return true;

			//变量定义
			CMD_C_AddScore * pAddScore=(CMD_C_AddScore *)pData;

			//消息处理
			WORD wChairID=pIServerUserItem->GetChairID();
			return OnUserAddScore(wChairID,pAddScore->lScore);
		}
	case SUB_C_GET_WINNER:			//获取胜者
		{
			//状态效验
			ASSERT(m_pITableFrame->GetGameStatus()==GAME_SCENE_PLAY);
			if (m_pITableFrame->GetGameStatus()!=GAME_SCENE_PLAY) return true;

			return OnUserGetWinner(pIServerUserItem);
		}
	case SUB_C_ADD_SCORE_TIME:		//启动定时器
		{
// 			//用户效验
// 			tagUserInfo * pUserData=pIServerUserItem->GetUserInfo();
// 			if (pUserData->cbUserStatus!=US_PLAYING) return true;
// 
// 			m_pITableFrame->SetGameTimer(IDI_ADD_SCORE,TIME_ADD_SCORE*36,1,0);
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

//用户坐下
bool CTableFrameSink::OnActionUserSitDown(WORD wChairID, IServerUserItem * pIServerUserItem, bool bLookonUser)
{
	//历史积分
	if (bLookonUser==false) m_HistoryScore.OnEventUserEnter(pIServerUserItem->GetChairID());

	if( (m_pGameServiceOption->wServerType&GAME_GENRE_PERSONAL) !=0 )//房卡模式
	{
		//获取房卡规则
		BYTE* pSetInfo = m_pITableFrame->GetGameRule();
		if(pSetInfo[0] == 1)
		{
			m_bPersonalRoom=true;
			m_cbPlayerCount=pSetInfo[1];//玩家人数0表示2-5人，2345表示对应人数
			BYTE cbMaxChairCount=pSetInfo[2];//最大椅子数
			m_lPersonalInitScore = 100*(int)pSetInfo[3];//初始分
			m_lPersonalCellScore=pSetInfo[4];//配置底分
			m_lDrawCellScore = m_lPersonalCellScore;
			if(m_cbPlayerCount != 0)//固定人数
			{
				m_pITableFrame->SetTableChairCount(m_cbPlayerCount);
				m_pITableFrame->SetStartMode(START_MODE_FULL_READY);
			}
			else//2-5人
			{
				m_cbPlayerCount=GAME_PLAYER;
				m_pITableFrame->SetTableChairCount(m_cbPlayerCount);
				m_pITableFrame->SetStartMode(START_MODE_ALL_READY);
			}

		}
	}

	return true;
}

//用户起立
bool CTableFrameSink::OnActionUserStandUp(WORD wChairID, IServerUserItem * pIServerUserItem, bool bLookonUser)
{
	//历史积分
	if (bLookonUser==false) m_HistoryScore.OnEventUserLeave(pIServerUserItem->GetChairID());

	if ((m_pGameServiceOption->wServerType&GAME_GENRE_PERSONAL) != 0)//房卡模式
	{
		if (m_pITableFrame->IsPersonalRoomDisumme())//当前朋友局解散清理记录
		{
			ZeroMemory(&m_stRecord, sizeof(m_stRecord));
		}
	}
	return true;
}

//用户放弃
bool CTableFrameSink::OnUserGiveUp(WORD wChairID)
{
	ASSERT(m_cbPlayStatus[wChairID]==TRUE);
	if(m_cbPlayStatus[wChairID]==FALSE) return true;
	//设置变量
	m_cbPlayStatus[wChairID]=FALSE;

	if (m_bGiveUp[wChairID])
	{
		return true;
	}
	// 获取玩家
	IServerUserItem * pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);

	// 扣除分数
	LONGLONG lDeficiencyPoint = -(m_lUserScore[wChairID] + m_lTableScore[wChairID] + m_lServiceCharge);

	LONGLONG lUserScore=pIServerUserItem->GetUserScore();
	if(m_bPersonalRoom)
		lUserScore=m_lPersonalInitScore;

	// 扣除服务费
	if ( lDeficiencyPoint < 0 && -lDeficiencyPoint > lUserScore )
	{
		lDeficiencyPoint = -lUserScore;
	}

	//写入积分
	if ((m_pGameServiceOption->wServerType&GAME_GENRE_PERSONAL) == 0)//金币模式及时写分
	{
		tagScoreInfo ScoreInfoArray[GAME_PLAYER];
		ZeroMemory(ScoreInfoArray, sizeof(ScoreInfoArray));
		ScoreInfoArray[wChairID].cbType = SCORE_TYPE_LOSE;
		ScoreInfoArray[wChairID].lScore = lDeficiencyPoint;
		m_pITableFrame->WriteTableScore(ScoreInfoArray, m_cbPlayerCount);
	}


	//写入库存
	if( pIServerUserItem->IsAndroidUser() && !m_bAllAndroid)
		g_lStockScore += lDeficiencyPoint;

	m_bGiveUp[wChairID] = true;

	//游戏调控
	if (g_bControl==true)
	{
		OnEventScoreAlter(pIServerUserItem->GetUserID(),-(m_lUserScore[wChairID]+m_lTableScore[wChairID]));
	}

	//人数计算
	WORD wPlayerCount=0;
	for (WORD i=0;i<m_cbPlayerCount;i++)
	{
		if (m_cbPlayStatus[i]==TRUE) wPlayerCount++;
	}

	//继续判断
	if (wPlayerCount>=2)
	{
		//下注调整
		RectifyMaxScore();

		//切换用户
		if (m_wCurrentUser==wChairID)
		{
			//设置用户
			m_wCurrentUser=INVALID_CHAIR;

			m_pITableFrame->KillGameTimer(IDI_ADD_SCORE);

			//用户搜索
			for (WORD i=1;i<m_cbPlayerCount;i++)
			{
				//变量定义
				WORD wCurrentUser=(wChairID+i)%m_cbPlayerCount;
				LONGLONG lDrawAddScroe=m_lUserScore[wCurrentUser]+m_lTableScore[wCurrentUser];

				//状态判断
				if (m_cbPlayStatus[wCurrentUser]==FALSE) continue;

				//用户切换
				if ((m_cbOperaScore[wCurrentUser]==FALSE)||(lDrawAddScroe<m_lTurnLessScore))
				{
					m_wCurrentUser=wCurrentUser;
					m_pITableFrame->SetGameTimer(IDI_ADD_SCORE,TIME_ADD_SCORE*36,1,0);
					break;
				}
			}
		}
	}
	else 
	{
		//汇集金币
		for (WORD i=0;i<m_cbPlayerCount;i++) 
		{
			m_lTableScore[i]+=m_lUserScore[i];
			m_lUserScore[i]=0L;
		}

		//设置用户
		m_wCurrentUser=INVALID_CHAIR;
	}

	//变量定义
	CMD_S_GiveUp GiveUp;
	ZeroMemory(&GiveUp,sizeof(GiveUp));

	//设置变量
	GiveUp.wGiveUpUser=wChairID;
	GiveUp.wCurrentUser=m_wCurrentUser;
	GiveUp.lDrawMaxScore=m_lDrawMaxScore;
	GiveUp.lTrunMaxScore=m_lTurnMaxScore;
		
	//发送消息
	m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_GIVE_UP,&GiveUp,sizeof(GiveUp));
	m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_GIVE_UP,&GiveUp,sizeof(GiveUp));

	if (m_pGameVideo)
	{		
		m_pGameVideo->AddVideoData(SUB_S_GIVE_UP,&GiveUp);
	}
	//结束游戏
	if (wPlayerCount<=1)
	{
		OnEventGameConclude(INVALID_CHAIR,NULL,GER_NO_PLAYER);
		return true;
	}

	//发送扑克
	if (m_wCurrentUser==INVALID_CHAIR) DispatchUserCard();

	return true;
}

//用户加注
bool CTableFrameSink::OnUserAddScore(WORD wChairID, LONGLONG lScore)
{
	//状态效验
	ASSERT(m_wCurrentUser==wChairID);
	if (m_wCurrentUser!=wChairID) 
	{
		CString strdebug;
		strdebug.Format(TEXT("KEVINA 断线为%d"), m_pITableFrame->GetTableUserItem(wChairID)->IsAndroidUser() ? TRUE : FALSE);
		WriteInfo(L"", strdebug);

		if (m_pITableFrame->GetTableUserItem(wChairID)->IsAndroidUser())
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	//加注效验
	ASSERT((lScore+m_lUserScore[wChairID]+m_lTableScore[wChairID])<=m_lTurnMaxScore);
	ASSERT((lScore>=0L)&&((lScore+m_lUserScore[wChairID]+m_lTableScore[wChairID])>=m_lTurnLessScore));
	
	//校验身上金币
	LONGLONG lUserScore=m_pITableFrame->GetTableUserItem(wChairID)->GetUserScore();
	if(m_bPersonalRoom)
		lUserScore=m_lPersonalInitScore;

	ASSERT (lUserScore >= lScore);

	//加注效验
	if ((lScore+m_lUserScore[wChairID]+m_lTableScore[wChairID])>m_lTurnMaxScore) 
	{
		CString strdebug;
		strdebug.Format(TEXT("KEVINB 断线为%d, 本轮下注【%I64d】 用户下注【%I64d】 桌面下注【%I64d】 最大下注【%I64d】 携带金币【%I64d】"),
			m_pITableFrame->GetTableUserItem(wChairID)->IsAndroidUser() ? TRUE : FALSE,
			lScore, m_lUserScore[wChairID], m_lTableScore[wChairID], m_lTurnMaxScore, lUserScore);
		WriteInfo(L"", strdebug);

		return true;
	}

	if ((lScore<0L)||((lScore+m_lUserScore[wChairID]+m_lTableScore[wChairID])<m_lTurnLessScore)) 
	{
		CString strdebug;
		strdebug.Format(TEXT("KEVINC 断线为%d, 本轮下注【%I64d】 用户下注【%I64d】 桌面下注【%I64d】 最小下注【%I64d】"),
			m_pITableFrame->GetTableUserItem(wChairID)->IsAndroidUser() ? TRUE : FALSE,
			lScore, m_lUserScore[wChairID], m_lTableScore[wChairID], m_lTurnLessScore);
		WriteInfo(L"", strdebug);

		return true;
	}

	if (lUserScore < lScore)
	{
		return false;
	}

	//设置变量
	m_cbOperaScore[wChairID]=TRUE;
	m_lUserScore[wChairID]+=lScore;
	m_lTurnLessScore=m_lUserScore[wChairID]+m_lTableScore[wChairID];
	
	//状态变量
	m_wCurrentUser=INVALID_CHAIR;
	m_bShowHand=(m_lTurnLessScore==m_lDrawMaxScore);

	//用户搜索
	for (WORD i=1;i<m_cbPlayerCount;i++)
	{
		//变量定义
		WORD wCurrentUser=(wChairID+i)%m_cbPlayerCount;
		LONGLONG lDrawAddScroe=m_lUserScore[wCurrentUser]+m_lTableScore[wCurrentUser];

		//状态判断
		if (m_cbPlayStatus[wCurrentUser]==FALSE) continue;

		//用户切换
		if ((m_cbOperaScore[wCurrentUser]==FALSE)||(lDrawAddScroe<m_lTurnLessScore))
		{
			m_wCurrentUser=wCurrentUser;
			break;
		}
	}

	//变量定义
	CMD_S_AddScore AddScore;
	ZeroMemory(&AddScore,sizeof(AddScore));

	//设置变量
	AddScore.wAddScoreUser=wChairID;
	AddScore.wCurrentUser=m_wCurrentUser;
	AddScore.lTurnLessScore=m_lTurnLessScore;
	AddScore.lUserScoreCount=m_lUserScore[wChairID];

	//发送数据
	m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_ADD_SCORE,&AddScore,sizeof(AddScore));
	m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_ADD_SCORE,&AddScore,sizeof(AddScore));

	if (m_pGameVideo)
	{		
		m_pGameVideo->AddVideoData(SUB_S_ADD_SCORE,&AddScore);
	}

	//发送扑克
	if (m_wCurrentUser==INVALID_CHAIR) 
	{
		DispatchUserCard();
	}
	else
	{
		//启动定时器
		m_pITableFrame->SetGameTimer(IDI_ADD_SCORE,TIME_ADD_SCORE*36,1,0);
	}

	return true;
}

//调整下注
VOID CTableFrameSink::RectifyMaxScore()
{
	//设置变量
	m_lDrawMaxScore=0L;
	m_lTurnMaxScore=0L;

	//最大下注
	for (WORD i=0;i<m_cbPlayerCount;i++)
	{
		//用户判断
		if (m_cbPlayStatus[i]==FALSE) continue;

		//变量设置
		if ((m_lDrawMaxScore==0L)||(m_lUserMaxScore[i]<m_lDrawMaxScore)) 
			m_lDrawMaxScore=m_lUserMaxScore[i];
	}

	//当前下注
	if (m_cbSendCardCount<=2) 
		m_lTurnMaxScore=m_lDrawMaxScore/2L;
	else 
		m_lTurnMaxScore=m_lDrawMaxScore;

	return;
}

//发送扑克
VOID CTableFrameSink::DispatchUserCard()
{
	//汇集金币
	for (WORD i=0;i<m_cbPlayerCount;i++) 
	{
		m_lTableScore[i]+=m_lUserScore[i];
		m_lUserScore[i]=0L;
	}

	//结束判断
	if (m_cbSendCardCount==MAX_COUNT)
	{
		OnEventGameConclude(INVALID_CHAIR,NULL,GER_NORMAL);
		return;
	}

	//混乱扑克
	if (m_bChaosCard==true)
	{
		//交换扑克
		WORD wSwitchUser=rand()%m_cbPlayerCount;
		BYTE cbSwitchCard=m_cbHandCardData[wSwitchUser][m_cbSendCardCount];

		//交换扑克
		for (WORD i=0;i<m_cbPlayerCount-1;i++)
		{
			WORD wUserHead=(wSwitchUser+i)%m_cbPlayerCount;
			WORD wUserNext=(wSwitchUser+i+1)%m_cbPlayerCount;
			m_cbHandCardData[wUserHead][m_cbSendCardCount]=m_cbHandCardData[wUserNext][m_cbSendCardCount];
		}

		//交换扑克
		WORD wUserLeave=(wSwitchUser+m_cbPlayerCount-1)%m_cbPlayerCount;
		m_cbHandCardData[wUserLeave][m_cbSendCardCount]=cbSwitchCard;
	}

	//派发扑克
	BYTE cbSourceCount=m_cbSendCardCount;
	m_cbSendCardCount=(m_bShowHand==false)?(m_cbSendCardCount+1):MAX_COUNT;

	//当前用户
	if (m_bShowHand==false)
	{
		//状态变量
		ZeroMemory(m_cbOperaScore,sizeof(m_cbOperaScore));

		//设置用户
		m_wCurrentUser=EstimateWinner(1,m_cbSendCardCount-1);

		//下注设置
		m_lTurnMaxScore = (m_cbSendCardCount >= 3) ? m_lDrawMaxScore : m_lDrawMaxScore / 2L;
		m_lTurnLessScore=m_lUserScore[m_wCurrentUser]+m_lTableScore[m_wCurrentUser];
	}
	else
	{
		//设置变量
		m_wCurrentUser=INVALID_CHAIR;
		m_lTurnMaxScore=m_lDrawMaxScore;
		m_lTurnLessScore=m_lDrawMaxScore;
	}

	//构造数据
	CMD_S_SendCard SendCard;
	ZeroMemory(&SendCard,sizeof(SendCard));

	//设置变量
	SendCard.wCurrentUser=m_wCurrentUser;
	SendCard.lTurnMaxScore=m_lTurnMaxScore;
	SendCard.wStartChairID=EstimateWinner(1,cbSourceCount-1);
	SendCard.cbSendCardCount=m_cbSendCardCount-cbSourceCount;

	//发送扑克
	for (WORD i=0;i<m_cbPlayerCount;i++)
	{
		//状态判断
		if (m_cbPlayStatus[i]==FALSE) continue;

		//设置数目
		m_cbCardCount[i]=m_cbSendCardCount;

		//派发扑克
		for (BYTE j=0;j<(m_cbSendCardCount-cbSourceCount);j++)
		{
			ASSERT(j<CountArray(SendCard.cbCardData[i]));
			SendCard.cbCardData[j][i]=m_cbHandCardData[i][cbSourceCount+j];
		}
	}

	//发送数据
	m_pITableFrame->SendTableData(INVALID_CHAIR,SUB_S_SEND_CARD,&SendCard,sizeof(SendCard));
	m_pITableFrame->SendLookonData(INVALID_CHAIR,SUB_S_SEND_CARD,&SendCard,sizeof(SendCard));

	if (m_pGameVideo)
	{		
		m_pGameVideo->AddVideoData(SUB_S_SEND_CARD,&SendCard);
	}

	//结束处理
	if (m_wCurrentUser==INVALID_CHAIR) 
	{
		OnEventGameConclude(INVALID_CHAIR,NULL,GER_NORMAL);
	}
	else
	{
		//启动定时器
		m_pITableFrame->SetGameTimer(IDI_ADD_SCORE,TIME_ADD_SCORE*46,1,0);
	}

	return;
}

//调控扑克
VOID CTableFrameSink::ControlUserCard()
{
	//用户调控
	for (WORD i=0;i<m_cbPlayerCount;i++)
	{
		//状态判断
		if (m_cbPlayStatus[i]==FALSE) continue;

		//变量定义
		bool bSwitchCard=false;
		IServerUserItem * pIServerUserItem=m_pITableFrame->GetTableUserItem(i);

		//查询记录
		tagUserTracing * pUserTracing=NULL;
		g_UserTracingMap.Lookup(pIServerUserItem->GetUserID(),pUserTracing);

		//积分限制
		if (pUserTracing!=NULL)
		{
			if ((g_lMaxWinScore!=0L)&&(pUserTracing->lWinScore>g_lMaxWinScore)) bSwitchCard=true;
			if ((g_lMaxVarietycore!=0L)&&(pUserTracing->lVarietyScore>g_lMaxVarietycore)) bSwitchCard=true;
		}

		//用户限制
		if (bSwitchCard==false)
		{
			DWORD dwUserID=0L;
			if ((g_BlackListIDMap.Lookup(pIServerUserItem->GetUserID(),dwUserID)==TRUE)&&(dwUserID!=0L)) bSwitchCard=true;
		}

		//地址限制
		if (bSwitchCard==false)
		{
			DWORD dwUserAddress=0L;
			if ((g_UserAddressMap.Lookup(pIServerUserItem->GetClientAddr(),dwUserAddress)==TRUE)&&(dwUserAddress!=0)) bSwitchCard=true;
		}

		//交换扑克
		if (bSwitchCard==true)
		{
			//获取输牌
			WORD wLoseIndex=EstimateLoser(0,MAX_COUNT-1);

			//交换扑克
			if (wLoseIndex!=i)
			{
				BYTE cbTempCardData[MAX_COUNT];
				CopyMemory(cbTempCardData,m_cbHandCardData[i],sizeof(cbTempCardData));
				CopyMemory(m_cbHandCardData[i],m_cbHandCardData[wLoseIndex],sizeof(cbTempCardData));
				CopyMemory(m_cbHandCardData[wLoseIndex],cbTempCardData,sizeof(cbTempCardData));
			}

			//禁止混乱
			m_bChaosCard=false;

			break;
		}
	}

	return;
}

//积分事件
VOID CTableFrameSink::OnEventScoreAlter(DWORD dwUserID, LONGLONG lVarietyScore)
{
	//获取时间
	SYSTEMTIME SystemTime;
	GetLocalTime(&SystemTime);

	//搜索用户
	tagUserTracing * pUserTracing=NULL;
	g_UserTracingMap.Lookup(dwUserID,pUserTracing);

	//增加用户
	if (pUserTracing==NULL)
	{
		//创建对象
		pUserTracing=new tagUserTracing;
		g_UserTracingMap[dwUserID]=pUserTracing;

		//设置变量
		pUserTracing->lWinScore=0L;
		pUserTracing->lVarietyScore=0L;
		pUserTracing->SystemTimeTrace=SystemTime;
	}

	//还原判断
	if (pUserTracing->SystemTimeTrace.wDay!=SystemTime.wDay)
	{
		pUserTracing->lWinScore=0L;
		pUserTracing->lVarietyScore=0L;
		pUserTracing->SystemTimeTrace=SystemTime;
	}

	//设置变量
	pUserTracing->lVarietyScore+=lVarietyScore;
	if (lVarietyScore>0L) pUserTracing->lWinScore+=lVarietyScore;

	return;
}

//推断输者
WORD CTableFrameSink::EstimateLoser(BYTE cbStartPos, BYTE cbConcludePos)
{
	//保存扑克
	BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
	CopyMemory(cbUserCardData,m_cbHandCardData,sizeof(cbUserCardData));

	//寻找玩家
	WORD wLoser=0;
	for (;wLoser<m_cbPlayerCount;wLoser++)
	{
		if (m_cbPlayStatus[wLoser]==TRUE) 
		{
			m_GameLogic.SortCardList(cbUserCardData[wLoser]+cbStartPos,cbConcludePos-cbStartPos+1);
			break;
		}
	}

	//对比玩家
	for (WORD i=(wLoser+1);i<m_cbPlayerCount;i++)
	{
		//用户过滤
		if (m_cbPlayStatus[i]==FALSE) continue;

		//排列扑克
		m_GameLogic.SortCardList(cbUserCardData[i]+cbStartPos,cbConcludePos-cbStartPos+1);

		//对比扑克
		if (m_GameLogic.CompareCard(cbUserCardData[i]+cbStartPos,cbUserCardData[wLoser]+cbStartPos,cbConcludePos-cbStartPos+1)==false) 
		{
			wLoser=i;
		}
	}

	return wLoser;
}

//推断胜者
WORD CTableFrameSink::EstimateWinner(BYTE cbStartPos, BYTE cbConcludePos)
{
	//保存扑克
	BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
	CopyMemory(cbUserCardData,m_cbHandCardData,sizeof(cbUserCardData));

	//寻找玩家
	WORD wWinner=0;
	for (;wWinner<m_cbPlayerCount;wWinner++)
	{
		if (m_cbPlayStatus[wWinner]==TRUE) 
		{
			m_GameLogic.SortCardList(cbUserCardData[wWinner]+cbStartPos,cbConcludePos-cbStartPos+1);
			break;
		}
	}

	//对比玩家
	for (WORD i=(wWinner+1);i<m_cbPlayerCount;i++)
	{
		//用户过滤
		if (m_cbPlayStatus[i]==FALSE) continue;

		//排列扑克
		m_GameLogic.SortCardList(cbUserCardData[i]+cbStartPos,cbConcludePos-cbStartPos+1);

		//对比扑克
		if (m_GameLogic.CompareCard(cbUserCardData[i]+cbStartPos,cbUserCardData[wWinner]+cbStartPos,cbConcludePos-cbStartPos+1)==true) 
		{
			wWinner=i;
		}
	}

	return wWinner;
}

//获取信息
bool CTableFrameSink::OnUserGetWinner( IServerUserItem * pIServerUserItem )
{
	return true;
	////效验权限
	//ASSERT( CUserRight::CanCheat(pIServerUserItem->GetUserRight()) );
	//if( !CUserRight::CanCheat(pIServerUserItem->GetUserRight()) ) return false;
	//if( pIServerUserItem->GetUserStatus() == US_LOOKON ) return true;

	////保存扑克
	//BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
	//CopyMemory(cbUserCardData,m_cbHandCardData,sizeof(cbUserCardData));

	////变量定义
	//BYTE cbPlayStatus[GAME_PLAYER] = {0};
	//CopyMemory( cbPlayStatus,m_cbPlayStatus,sizeof(cbPlayStatus) );

	//CMD_S_GetWinner GetWinner = {0};
	//
	////排列扑克
	//for( WORD i = 0; i < GAME_PLAYER; i++ )
	//{
	//	if( cbPlayStatus[i] )
	//	{
	//		m_GameLogic.SortCardList(cbUserCardData[i],MAX_COUNT);
	//	}
	//}

	////寻找玩家
	//do
	//{
	//	for( WORD i = 0; i < GAME_PLAYER; i++ )
	//	{
	//		if( cbPlayStatus[i] ) break;
	//	}
	//	if( i == GAME_PLAYER ) break;

	//	WORD wWinner = i;

	//	//对比玩家
	//	for (WORD j=0;j<GAME_PLAYER;j++)
	//	{
	//		//用户过滤
	//		if (cbPlayStatus[j]==FALSE||j==wWinner) continue;

	//		//对比扑克
	//		if (m_GameLogic.CompareCard(cbUserCardData[j],cbUserCardData[wWinner],MAX_COUNT)==true) 
	//		{
	//			wWinner=j;
	//		}
	//	}

	//	GetWinner.wChairOrder[GetWinner.wOrderCount++] = wWinner;

	//	cbPlayStatus[wWinner] = FALSE;
	//}while(true);

	////发送信息
	//WORD wSendSize = sizeof(GetWinner)-sizeof(GetWinner.wChairOrder)+sizeof(WORD)*GetWinner.wOrderCount;
	//m_pITableFrame->SendUserData(pIServerUserItem,SUB_S_GET_WINNER,&GetWinner,wSendSize);

	//return true;
}

//发送库存
void CTableFrameSink::SendStorage()
{
	//通知消息
	TCHAR szMessage[128]=TEXT("");
	_sntprintf(szMessage,CountArray(szMessage),TEXT("当前库存：%I64d"), g_lStockScore);

	//用户状态
	for (WORD i=0;i<m_cbPlayerCount;i++)
	{
		//获取用户
		IServerUserItem *pIServerUser=m_pITableFrame->GetTableUserItem(i);
		if(pIServerUser != NULL)
		{
			if (CUserRight::IsGameCheatUser(pIServerUser->GetUserRight()))
			{				
				m_pITableFrame->SendGameMessage(pIServerUser,szMessage,SMT_CHAT);
			}
		}
	}

	int nLookonCount = 0;
	IServerUserItem* pLookonUserItem = m_pITableFrame->EnumLookonUserItem(nLookonCount);
	while( pLookonUserItem )
	{
		if (CUserRight::IsGameCheatUser(pLookonUserItem->GetUserRight()))
		{				
			m_pITableFrame->SendGameMessage(pLookonUserItem,szMessage,SMT_CHAT);
		}

		nLookonCount++;
		pLookonUserItem = m_pITableFrame->EnumLookonUserItem(nLookonCount);
	}
}

void CTableFrameSink::WriteInfo(LPCTSTR pszFileName, LPCTSTR pszString)
{
	m_pITableFrame->SendGameMessage(pszString, SMT_GAMELOG);
// 	//设置语言区域
// 	char* old_locale = _strdup(setlocale(LC_CTYPE,NULL));
// 	setlocale(LC_CTYPE, "chs");

// 	CStdioFile myFile;
// 	CString strFileName;
// 	strFileName.Format(TEXT("%s"), pszFileName);
// 	BOOL bOpen = myFile.Open(strFileName, CFile::modeReadWrite|CFile::modeCreate|CFile::modeNoTruncate);
// 	if (bOpen)
// 	{	
// 		myFile.SeekToEnd();
// 		myFile.WriteString(pszString);
// 		myFile.Flush();
// 		myFile.Close();
// 	}

// 	//还原区域设定
// 	setlocale(LC_CTYPE, old_locale);
// 	free(old_locale);
}
//////////////////////////////////////////////////////////////////////////
