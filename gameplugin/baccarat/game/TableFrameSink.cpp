﻿#include "StdAfx.h"
#include "DlgCustomRule.h"
#include "TableFrameSink.h"
#include <locale>
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
#define TIME_PLACE_JETTON			10									//下注时间

//结束时间
#define IDI_GAME_END				3									//结束时间
#define TIME_GAME_END				20									//结束时间


//信息查询
#define KEY_STOCK					0
#define KEY_IMMORTAL_COUNT			1
#define KEY_ROBOT_COUNT				2
#define KEY_IMMORTAL_BET			3
#define KEY_ROBOT_BET				4
#define KEY_MAX						5

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


//构造函数
CTableFrameSink::CTableFrameSink()
{
	//起始分数
	ZeroMemory(m_lUserStartScore, sizeof(m_lUserStartScore));

	//下注数
	ZeroMemory(m_lAllBet, sizeof(m_lAllBet));
	ZeroMemory(m_lPlayBet, sizeof(m_lPlayBet));

	//分数
	m_lBankerScore = 0l;
	ZeroMemory(m_lPlayScore, sizeof(m_lPlayScore));
	ZeroMemory(m_lUserWinScore, sizeof(m_lUserWinScore));
	ZeroMemory(m_lUserRevenue, sizeof(m_lUserRevenue));

	//扑克信息
	ZeroMemory(m_cbCardCount, sizeof(m_cbCardCount));
	ZeroMemory(m_cbTableCardArray, sizeof(m_cbTableCardArray));

	//状态变量
	m_dwBetTime = 0L;
	m_bControl = false;
	m_lControlStorage = 0;
	CopyMemory(m_szControlName, TEXT("无人"), sizeof(m_szControlName));
	m_nServiceCharge = 0;

	//庄家信息
	m_ApplyUserArray.RemoveAll();
	m_wCurrentBanker = INVALID_CHAIR;
	m_wOfflineBanker = INVALID_CHAIR;
	m_wBankerTime = 0;
	m_lBankerWinScore = 0L;
	m_lBankerCurGameScore = 0L;
	m_bEnableSysBanker = true;
	ZeroMemory(&m_superbankerConfig, sizeof(m_superbankerConfig));
	m_typeCurrentBanker = INVALID_SYSBANKER;
	m_wCurSuperRobBankerUser = INVALID_CHAIR;

	ZeroMemory(&m_occupyseatConfig, sizeof(m_occupyseatConfig));

	//占位
	for(WORD i = 0; i < MAX_OCCUPY_SEAT_COUNT; i++)
	{
		m_wOccupySeatChairIDArray[i] = INVALID_CHAIR;
	}

	//记录变量
	ZeroMemory(m_GameRecordArrary, sizeof(m_GameRecordArrary));
	m_nRecordFirst = 0;
	m_nRecordLast = 0;
	m_dwRecordCount = 0;

	//庄家设置
	m_nBankerTimeLimit = 0l;
	m_nBankerTimeAdd = 0l;
	m_lExtraBankerScore = 0l;
	m_nExtraBankerTime = 0l;
	m_lPlayerBankerMAX = 0l;
	m_bExchangeBanker = true;
	m_nStorageCount = 0;
	//m_lStorageMax=50000000;

	//时间控制
	m_cbFreeTime = TIME_FREE;
	m_cbBetTime = TIME_PLACE_JETTON;
	m_cbEndTime = TIME_GAME_END;

	//机器人控制
	m_nChipRobotCount = 0;
	m_nRobotListMaxCount = 0;
	ZeroMemory(m_lRobotAreaScore, sizeof(m_lRobotAreaScore));

	//服务控制
	m_hControlInst = NULL;
	m_pServerContro = NULL;
	m_hControlInst = LoadLibrary(TEXT("BaccaratNewServerControl.dll"));
	if(m_hControlInst)
	{
		typedef void *(*CREATE)();
		CREATE ServerControl = (CREATE)GetProcAddress(m_hControlInst, "CreateServerControl");
		if(ServerControl)
		{
			m_pServerContro = static_cast<IServerControl *>(ServerControl());
		}
	}
	return;
}

//析构函数
CTableFrameSink::~CTableFrameSink(void)
{
	if(m_pServerContro)
	{
		delete m_pServerContro;
		m_pServerContro = NULL;
	}

	if(m_hControlInst)
	{
		FreeLibrary(m_hControlInst);
		m_hControlInst = NULL;
	}
}

//释放对象
VOID CTableFrameSink::Release()
{
	if(m_pServerContro)
	{
		delete m_pServerContro;
		m_pServerContro = NULL;
	}

	if(m_hControlInst)
	{
		FreeLibrary(m_hControlInst);
		m_hControlInst = NULL;
	}
}

//接口查询
void *CTableFrameSink::QueryInterface(const IID &Guid, DWORD dwQueryVer)
{
	QUERYINTERFACE(ITableFrameSink, Guid, dwQueryVer);
	QUERYINTERFACE(ITableUserAction, Guid, dwQueryVer);
	QUERYINTERFACE_IUNKNOWNEX(ITableFrameSink, Guid, dwQueryVer);
	return NULL;
}

//初始化
bool CTableFrameSink::Initialization(IUnknownEx *pIUnknownEx)
{
	//查询接口
	ASSERT(pIUnknownEx != NULL);
	m_pITableFrame = QUERY_OBJECT_PTR_INTERFACE(pIUnknownEx, ITableFrame);
	if(m_pITableFrame == NULL)
	{
		return false;
	}

	//获取参数
	m_pGameServiceOption = m_pITableFrame->GetGameServiceOption();
	ASSERT(m_pGameServiceOption != NULL);

	//开始模式
	m_pITableFrame->SetStartMode(START_MODE_TIME_CONTROL);

	//读取配置
	memcpy(m_szGameRoomName, m_pGameServiceOption->szServerName, sizeof(m_szGameRoomName));

	//设置文件名
	TCHAR szPath[MAX_PATH] = TEXT("");
	GetCurrentDirectory(sizeof(szPath), szPath);
	_sntprintf_s(m_szConfigFileName, sizeof(m_szConfigFileName), TEXT("%s\\BaccaratNewConfig.ini"), szPath);

	ReadConfigInformation();

	return true;
}

//复位桌子
void CTableFrameSink::RepositionSink()
{
	m_wOfflineBanker = INVALID_CHAIR;

	//下注数
	ZeroMemory(m_lAllBet, sizeof(m_lAllBet));
	ZeroMemory(m_lPlayBet, sizeof(m_lPlayBet));

	//分数
	m_lBankerScore = 0l;
	ZeroMemory(m_lPlayScore, sizeof(m_lPlayScore));
	ZeroMemory(m_lUserWinScore, sizeof(m_lUserWinScore));
	ZeroMemory(m_lUserRevenue, sizeof(m_lUserRevenue));

	//机器人控制
	m_nChipRobotCount = 0;
	m_bControl = false;
	m_lControlStorage = 0;
	m_nStorageCount = 0;
	ZeroMemory(m_lRobotAreaScore, sizeof(m_lRobotAreaScore));

	return;
}


//游戏状态
bool CTableFrameSink::IsUserPlaying(WORD wChairID)
{
	return true;
}
//查询限额
SCORE CTableFrameSink::QueryConsumeQuota(IServerUserItem *pIServerUserItem)
{
	if(pIServerUserItem->GetUserStatus() == US_PLAYING)
	{
		return 0L;
	}
	else
	{
		return __max(pIServerUserItem->GetUserScore() - m_pGameServiceOption->lMinTableScore, 0L);
	}
}

//游戏开始
bool CTableFrameSink::OnEventGameStart()
{
	//设置随机种子
	srand(GetTickCount());

	//发送库存消息
	for(WORD i = 0; i < GAME_PLAYER; i++)
	{
		//获取用户
		IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
		if(pIServerUserItem == NULL)
		{
			continue;
		}

		if(CUserRight::IsGameCheatUser(m_pITableFrame->GetTableUserItem(i)->GetUserRight()))
		{
			CString strInfo;
			strInfo.Format(TEXT("当前库存：%I64d"), m_lStorageCurrent);

			m_pITableFrame->SendGameMessage(pIServerUserItem, strInfo, SMT_CHAT);
		}
	}

	//变量定义
	CMD_S_GameStart GameStart;
	ZeroMemory(&GameStart, sizeof(GameStart));

	//获取庄家
	IServerUserItem *pIBankerServerUserItem = NULL;
	if(m_wCurrentBanker == INVALID_CHAIR)
	{
		m_lBankerScore = 1000000000;
	}
	else
	{
		IServerUserItem *pIBankerServerUserItem = m_pITableFrame->GetTableUserItem(m_wCurrentBanker);
		if(pIBankerServerUserItem != NULL)
		{
			m_lBankerScore = pIBankerServerUserItem->GetUserScore();
		}
	}

	//设置变量
	GameStart.cbTimeLeave = m_cbBetTime;
	GameStart.wBankerUser = m_wCurrentBanker;
	GameStart.lBankerScore = m_lBankerScore;
	GameStart.nListUserCount = (int)(m_ApplyUserArray.GetCount());

	//下注机器人数量
	int nChipRobotCount = 0;
	for(int i = 0; i < GAME_PLAYER; i++)
	{
		IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
		if(pIServerUserItem != NULL && pIServerUserItem->IsAndroidUser())
		{
			nChipRobotCount++;
		}
	}
	GameStart.nChipRobotCount = min(nChipRobotCount, m_nMaxChipRobot);

	nChipRobotCount = 0;
	for(int i = 0; i < m_ApplyUserArray.GetCount(); i++)
	{
		IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(m_ApplyUserArray.GetAt(i));
		if(pIServerUserItem != NULL && pIServerUserItem->IsAndroidUser())
		{
			nChipRobotCount++;
		}
	}

	if(nChipRobotCount > 0)
	{
		GameStart.nAndriodCount = nChipRobotCount - 1;
	}

	//机器人控制
	m_nChipRobotCount = 0;
	ZeroMemory(m_lRobotAreaScore, sizeof(m_lRobotAreaScore));

	//旁观玩家
	m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_GAME_START, &GameStart, sizeof(GameStart));

	//游戏玩家
	for(WORD wChairID = 0; wChairID < GAME_PLAYER; ++wChairID)
	{
		IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
		if(pIServerUserItem == NULL)
		{
			continue;
		}

		//设置积分
		GameStart.lPlayBetScore = min(pIServerUserItem->GetUserScore() - m_nServiceCharge, m_lUserLimitScore);
		GameStart.lPlayFreeSocre = pIServerUserItem->GetUserScore();

		m_pITableFrame->SendTableData(wChairID, SUB_S_GAME_START, &GameStart, sizeof(GameStart));
	}

	//给超端发送下注信息
	SendUserBetInfo();

	return true;
}

//游戏结束
bool CTableFrameSink::OnEventGameConclude(WORD wChairID, IServerUserItem *pIServerUserItem, BYTE cbReason)
{
	switch(cbReason)
	{
	case GER_NORMAL:		//常规结束
	{
		//计算分数
		LONGLONG lBankerWinScore = GameOver();

		//递增次数
		m_wBankerTime++;

		//记录信息
		CString strControlInfo;
		strControlInfo.Format(TEXT("房间: %s | 桌号: %u | 当前库存为 [%I64d]"),
			m_pGameServiceOption->szServerName, m_pITableFrame->GetTableID() + 1, m_lStorageCurrent);
		WriteInfo(TEXT("欢乐30秒控制信息.log"), strControlInfo);

		//结束消息
		CMD_S_GameEnd GameEnd;
		ZeroMemory(&GameEnd, sizeof(GameEnd));

		//占位玩家成绩
		for(WORD i = 0; i < MAX_OCCUPY_SEAT_COUNT; i++)
		{
			if(m_wOccupySeatChairIDArray[i] == INVALID_CHAIR)
			{
				continue;
			}
			IServerUserItem *pIOccupySeatServerUserItem = m_pITableFrame->GetTableUserItem(m_wOccupySeatChairIDArray[i]);
			if(!pIOccupySeatServerUserItem)
			{
				continue;
			}

			GameEnd.lOccupySeatUserWinScore[i] = m_lUserWinScore[m_wOccupySeatChairIDArray[i]];
		}

		//庄家信息
		GameEnd.nBankerTime = m_wBankerTime;
		GameEnd.lBankerTotallScore = m_lBankerWinScore;
		GameEnd.lBankerScore = lBankerWinScore;

		//扑克信息
		CopyMemory(GameEnd.cbTableCardArray, m_cbTableCardArray, sizeof(m_cbTableCardArray));
		CopyMemory(GameEnd.cbCardCount, m_cbCardCount, sizeof(m_cbCardCount));

		//发送积分
		GameEnd.cbTimeLeave = m_cbEndTime;
		CMD_GC_GTrumpet sTrumpet;
		for(WORD i = 0; i < GAME_PLAYER; ++i)
		{
			IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
			if(pIServerUserItem == NULL)
			{
				continue;
			}

			//设置成绩
			GameEnd.lPlayAllScore = m_lUserWinScore[i];
			memcpy(GameEnd.lPlayScore, m_lPlayScore[i], sizeof(GameEnd.lPlayScore));
			//设置税收
			GameEnd.lRevenue = m_lUserRevenue[i];

			//发送消息
			m_pITableFrame->SendTableData(i, SUB_S_GAME_END, &GameEnd, sizeof(GameEnd));
			m_pITableFrame->SendLookonData(i, SUB_S_GAME_END, &GameEnd, sizeof(GameEnd));

			//喇叭发送
			for (WORD ti=0; ti<AREA_MAX; ++ti) {
				if (m_lPlayScore[i][ti] > 0) {
					ZeroMemory(&sTrumpet, sizeof(sTrumpet));
					sTrumpet.dwCardType = ti;
					sTrumpet.lScore = m_lPlayScore[i][ti];
					m_pITableFrame->SendUserItemData(pIServerUserItem, SUB_S_WIN_TRUMPET, &sTrumpet, sizeof(sTrumpet));
				}
			}
		}

		return true;
	}
	case GER_NETWORK_ERROR:		//网络中断
	case GER_USER_LEAVE:		//用户离开
	{
		if(wChairID == m_wCurSuperRobBankerUser)
		{
			m_wCurSuperRobBankerUser = INVALID_CHAIR;

			CMD_S_CurSuperRobLeave CurSuperRobLeave;
			ZeroMemory(&CurSuperRobLeave, sizeof(CurSuperRobLeave));

			//设置变量
			CurSuperRobLeave.wCurSuperRobBankerUser = m_wCurSuperRobBankerUser;

			//发送消息
			m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_CURSUPERROB_LEAVE, &CurSuperRobLeave, sizeof(CurSuperRobLeave));
			m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_CURSUPERROB_LEAVE, &CurSuperRobLeave, sizeof(CurSuperRobLeave));
		}

		//闲家判断
		if(m_wCurrentBanker != wChairID)
		{
			//变量定义
			LONGLONG lRevenue = 0;

			//写入积分
			if(m_pITableFrame->GetGameStatus() != GAME_SCENE_END)
			{
				for(WORD wAreaIndex = AREA_XIAN; wAreaIndex <= AREA_ZHUANG_DUI; ++wAreaIndex)
				{
					if(m_lPlayBet[wChairID][wAreaIndex] != 0)
					{
						CMD_S_PlaceBetFail PlaceBetFail;
						ZeroMemory(&PlaceBetFail, sizeof(PlaceBetFail));
						PlaceBetFail.lBetArea = (BYTE)wAreaIndex;
						PlaceBetFail.lPlaceScore = m_lPlayBet[wChairID][wAreaIndex];
						PlaceBetFail.wPlaceUser = wChairID;

						//游戏玩家
						for(WORD i = 0; i < GAME_PLAYER; ++i)
						{
							IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
							if(pIServerUserItem == NULL)
							{
								continue;
							}

							m_pITableFrame->SendTableData(i, SUB_S_PLACE_JETTON_FAIL, &PlaceBetFail, sizeof(PlaceBetFail));
						}

						m_lAllBet[wAreaIndex] -= m_lPlayBet[wChairID][wAreaIndex];
						m_lPlayBet[wChairID][wAreaIndex] = 0;
					}
				}
			}
			else
			{

				//是否泄愤
				bool bWritePoints = false;
				for(WORD wAreaIndex = AREA_XIAN; wAreaIndex <= AREA_ZHUANG_DUI; ++wAreaIndex)
				{
					if(m_lPlayBet[wChairID][wAreaIndex] != 0)
					{
						bWritePoints = true;
						break;
					}
				}

				//不写分
				if(!bWritePoints)
				{
					return true;
				}

				//写入积分
				if((pIServerUserItem->GetUserScore() - m_nServiceCharge) + m_lUserWinScore[wChairID] < 0L)
				{
					m_lUserWinScore[wChairID] = -(pIServerUserItem->GetUserScore() - m_nServiceCharge);
				}

				tagScoreInfo ScoreInfoArray;
				ZeroMemory(&ScoreInfoArray, sizeof(ScoreInfoArray));
				ScoreInfoArray.lScore = m_lUserWinScore[wChairID];
				ScoreInfoArray.lRevenue = m_lUserRevenue[wChairID];

				if(ScoreInfoArray.lScore > 0)
				{
					ScoreInfoArray.cbType = SCORE_TYPE_WIN;
				}
				else if(ScoreInfoArray.lScore < 0)
				{
					ScoreInfoArray.cbType = SCORE_TYPE_LOSE;
				}
				else
				{
					ScoreInfoArray.cbType = SCORE_TYPE_DRAW;
				}

				m_pITableFrame->WriteUserScore(wChairID, ScoreInfoArray);
				m_lUserWinScore[wChairID] = 0;
				m_lUserRevenue[wChairID] = 0;
				ZeroMemory(m_lPlayScore[wChairID], sizeof(m_lPlayScore[wChairID]));


				//清除下注
				for(WORD wAreaIndex = AREA_XIAN; wAreaIndex <= AREA_ZHUANG_DUI; ++wAreaIndex)
				{
					m_lPlayBet[wChairID][wAreaIndex] = 0;
				}
			}

			return true;
		}

		//状态判断
		if(m_pITableFrame->GetGameStatus() != GAME_SCENE_END)
		{
			//提示消息
			TCHAR szTipMsg[128];
			_sntprintf_s(szTipMsg, CountArray(szTipMsg), TEXT("由于庄家[ %s ]强退，游戏提前结束！"), pIServerUserItem->GetNickName());

			//发送消息
			SendGameMessage(INVALID_CHAIR, szTipMsg);

			//设置状态
			m_pITableFrame->SetGameStatus(GAME_SCENE_END);

			//设置时间
			m_dwBetTime = (DWORD)time(NULL);
			m_pITableFrame->KillGameTimer(IDI_PLACE_JETTON);
			m_pITableFrame->SetGameTimer(IDI_GAME_END, m_cbEndTime * 1000, 1, 0L);

			//计算分数
			GameOver();

			//结束消息
			CMD_S_GameEnd GameEnd;
			ZeroMemory(&GameEnd, sizeof(GameEnd));

			//占位玩家成绩
			for(WORD i = 0; i < MAX_OCCUPY_SEAT_COUNT; i++)
			{
				if(m_wOccupySeatChairIDArray[i] == INVALID_CHAIR)
				{
					continue;
				}
				IServerUserItem *pIOccupySeatServerUserItem = m_pITableFrame->GetTableUserItem(m_wOccupySeatChairIDArray[i]);
				if(!pIOccupySeatServerUserItem)
				{
					continue;
				}

				GameEnd.lOccupySeatUserWinScore[i] = m_lUserWinScore[m_wOccupySeatChairIDArray[i]];
			}

			//庄家信息
			GameEnd.nBankerTime = m_wBankerTime;
			GameEnd.lBankerTotallScore = m_lBankerWinScore;
			if(m_lBankerWinScore > 0L)
			{
				GameEnd.lBankerScore = 0;
			}

			//扑克信息
			CopyMemory(GameEnd.cbTableCardArray, m_cbTableCardArray, sizeof(m_cbTableCardArray));
			CopyMemory(GameEnd.cbCardCount, m_cbCardCount, sizeof(m_cbCardCount));

			//发送积分
			GameEnd.cbTimeLeave = m_cbEndTime;
			for(WORD i = 0; i < GAME_PLAYER; ++i)
			{
				IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if(pIServerUserItem == NULL)
				{
					continue;
				}

				//设置成绩
				GameEnd.lPlayAllScore = m_lUserWinScore[i];
				memcpy(GameEnd.lPlayScore, m_lPlayScore[i], sizeof(GameEnd.lPlayScore));

				//设置税收
				GameEnd.lRevenue = m_lUserRevenue[i];

				//发送消息
				m_pITableFrame->SendTableData(i, SUB_S_GAME_END, &GameEnd, sizeof(GameEnd));
				m_pITableFrame->SendLookonData(i, SUB_S_GAME_END, &GameEnd, sizeof(GameEnd));
			}
		}

		//扣除分数
		if(pIServerUserItem->GetUserScore() - m_nServiceCharge + m_lUserWinScore[m_wCurrentBanker] < 0L)
		{
			m_lUserWinScore[m_wCurrentBanker] = -(pIServerUserItem->GetUserScore() - m_nServiceCharge);
		}

		tagScoreInfo ScoreInfoArray;
		ZeroMemory(&ScoreInfoArray, sizeof(ScoreInfoArray));
		ScoreInfoArray.lScore = m_lUserWinScore[m_wCurrentBanker];
		ScoreInfoArray.lRevenue = m_lUserRevenue[m_wCurrentBanker];

		if(ScoreInfoArray.lScore > 0)
		{
			ScoreInfoArray.cbType = SCORE_TYPE_WIN;
		}
		else if(ScoreInfoArray.lScore < 0)
		{
			ScoreInfoArray.cbType = SCORE_TYPE_LOSE;
		}
		else
		{
			ScoreInfoArray.cbType = SCORE_TYPE_DRAW;
		}

		//m_pITableFrame->WriteTableScore(ScoreInfoArray,CountArray(ScoreInfoArray));

		m_pITableFrame->WriteUserScore(m_wCurrentBanker, ScoreInfoArray);

		m_lUserWinScore[m_wCurrentBanker] = 0L;

		//切换庄家
		ChangeBanker(true);

		return true;
	}
	}

	return false;
}

//发送场景
bool CTableFrameSink::OnEventSendGameScene(WORD wChiarID, IServerUserItem *pIServerUserItem, BYTE cbGameStatus, bool bSendSecret)
{
	switch(cbGameStatus)
	{
	case GAME_SCENE_FREE:			//空闲状态
	{
		//发送记录
		SendGameRecord(pIServerUserItem);

		//构造数据
		CMD_S_StatusFree StatusFree;
		ZeroMemory(&StatusFree, sizeof(StatusFree));

		//全局信息
		DWORD dwPassTime = (DWORD)time(NULL) - m_dwBetTime;
		StatusFree.cbTimeLeave = (BYTE)(m_cbFreeTime - __min(dwPassTime, m_cbFreeTime));
		StatusFree.bGenreEducate =  m_pGameServiceOption->wServerType & GAME_GENRE_EDUCATE;
		//玩家信息
		StatusFree.lPlayFreeSocre = pIServerUserItem->GetUserScore();

		//庄家信息
		StatusFree.wBankerUser = m_wCurrentBanker;
		StatusFree.wBankerTime = m_wBankerTime;
		StatusFree.lBankerWinScore = m_lBankerWinScore;

		//获取庄家
		IServerUserItem *pIBankerServerUserItem = NULL;
		if(m_wCurrentBanker == INVALID_CHAIR)
		{
			m_lBankerScore = 1000000000;
		}
		else
		{
			IServerUserItem *pIBankerServerUserItem = m_pITableFrame->GetTableUserItem(m_wCurrentBanker);
			if(pIBankerServerUserItem != NULL)
			{
				m_lBankerScore = pIBankerServerUserItem->GetUserScore();
			}
		}

		StatusFree.lBankerScore = m_lBankerScore;

		//是否系统坐庄
		StatusFree.bEnableSysBanker = m_bEnableSysBanker;

		//控制信息
		StatusFree.lApplyBankerCondition = m_lApplyBankerCondition;
		StatusFree.lAreaLimitScore = m_lAreaLimitScore;

		//房间名称
		CopyMemory(StatusFree.szGameRoomName, m_szGameRoomName, sizeof(StatusFree.szGameRoomName));

		//超级抢庄
		CopyMemory(&(StatusFree.superbankerConfig), &m_superbankerConfig, sizeof(m_superbankerConfig));
		StatusFree.wCurSuperRobBankerUser = m_wCurSuperRobBankerUser;
		StatusFree.typeCurrentBanker = m_typeCurrentBanker;

		//占位
		CopyMemory(&(StatusFree.occupyseatConfig), &m_occupyseatConfig, sizeof(m_occupyseatConfig));
		CopyMemory(StatusFree.wOccupySeatChairIDArray, m_wOccupySeatChairIDArray, sizeof(m_wOccupySeatChairIDArray));

		//机器人配置
		if(pIServerUserItem->IsAndroidUser())
		{
			tagCustomConfig *pCustomConfig = (tagCustomConfig *)m_pGameServiceOption->cbCustomRule;
			ASSERT(pCustomConfig);

			CopyMemory(&StatusFree.CustomAndroid, &pCustomConfig->CustomAndroid, sizeof(tagCustomAndroid));
		}

		//发送场景
		bool bSuccess = m_pITableFrame->SendGameScene(pIServerUserItem, &StatusFree, sizeof(StatusFree));

		//限制提示
		TCHAR szTipMsg[128];
		_sntprintf_s(szTipMsg, CountArray(szTipMsg), TEXT("\n本房间上庄条件为：%s\n区域限制为：%s\n玩家限制为：%s"), AddComma(m_lApplyBankerCondition), AddComma(m_lAreaLimitScore), AddComma(m_lUserLimitScore));
		m_pITableFrame->SendGameMessage(pIServerUserItem, szTipMsg, SMT_CHAT);

		//发送申请者
		SendApplyUser(pIServerUserItem);

		return bSuccess;
	}
	case GAME_SCENE_BET:		//游戏状态
	case GAME_SCENE_END:		//结束状态
	{
		//发送记录
		SendGameRecord(pIServerUserItem);

		//构造数据
		CMD_S_StatusPlay StatusPlay;
		ZeroMemory(&StatusPlay, sizeof(StatusPlay));

		//全局信息
		DWORD dwPassTime = (DWORD)time(NULL) - m_dwBetTime;
		int	nTotalTime = ((cbGameStatus == GAME_SCENE_BET) ? m_cbBetTime : m_cbEndTime);
		StatusPlay.cbTimeLeave = (BYTE)(nTotalTime - __min(dwPassTime, (DWORD)nTotalTime));
		StatusPlay.cbGameStatus = m_pITableFrame->GetGameStatus();

		//全局下注
		memcpy(StatusPlay.lAllBet, m_lAllBet, sizeof(StatusPlay.lAllBet));
		StatusPlay.lPlayFreeSocre = pIServerUserItem->GetUserScore();
		StatusPlay.bGenreEducate =  m_pGameServiceOption->wServerType & GAME_GENRE_EDUCATE;

		//玩家下注
		if(pIServerUserItem->GetUserStatus() != US_LOOKON && bSendSecret)
		{
			memcpy(StatusPlay.lPlayBet, m_lPlayBet[wChiarID], sizeof(StatusPlay.lPlayBet));
			memcpy(StatusPlay.lPlayScore, m_lPlayScore[wChiarID], sizeof(StatusPlay.lPlayScore));

			//最大下注
			StatusPlay.lPlayBetScore = min(pIServerUserItem->GetUserScore() - m_nServiceCharge, m_lUserLimitScore);
		}

		//庄家信息
		StatusPlay.wBankerUser = m_wCurrentBanker;
		StatusPlay.wBankerTime = m_wBankerTime;
		StatusPlay.lBankerWinScore = m_lBankerWinScore;

		//获取庄家
		IServerUserItem *pIBankerServerUserItem = NULL;
		if(m_wCurrentBanker == INVALID_CHAIR)
		{
			m_lBankerScore = 1000000000;
		}
		else
		{
			IServerUserItem *pIBankerServerUserItem = m_pITableFrame->GetTableUserItem(m_wCurrentBanker);
			if(pIBankerServerUserItem != NULL)
			{
				m_lBankerScore = pIBankerServerUserItem->GetUserScore();
			}
		}

		StatusPlay.lBankerScore = m_lBankerScore;

		//是否系统坐庄
		StatusPlay.bEnableSysBanker = m_bEnableSysBanker;

		//控制信息
		StatusPlay.lApplyBankerCondition = m_lApplyBankerCondition;
		StatusPlay.lAreaLimitScore = m_lAreaLimitScore;

		//结束判断
		if(cbGameStatus == GAME_SCENE_END)
		{
			//扑克信息
			CopyMemory(StatusPlay.cbCardCount, m_cbCardCount, sizeof(m_cbCardCount));
			CopyMemory(StatusPlay.cbTableCardArray, m_cbTableCardArray, sizeof(m_cbTableCardArray));

			//结束分数
			StatusPlay.lPlayAllScore = m_lUserWinScore[wChiarID];
			StatusPlay.lRevenue = m_lUserRevenue[wChiarID];
		}

		//房间名称
		CopyMemory(StatusPlay.szGameRoomName, m_szGameRoomName, sizeof(StatusPlay.szGameRoomName));

		//超级抢庄
		CopyMemory(&(StatusPlay.superbankerConfig), &m_superbankerConfig, sizeof(m_superbankerConfig));
		StatusPlay.wCurSuperRobBankerUser = m_wCurSuperRobBankerUser;
		StatusPlay.typeCurrentBanker = m_typeCurrentBanker;

		//占位
		CopyMemory(&(StatusPlay.occupyseatConfig), &m_occupyseatConfig, sizeof(m_occupyseatConfig));
		CopyMemory(StatusPlay.wOccupySeatChairIDArray, m_wOccupySeatChairIDArray, sizeof(m_wOccupySeatChairIDArray));

		//占位玩家成绩
		for(WORD i = 0; i < MAX_OCCUPY_SEAT_COUNT; i++)
		{
			if(m_wOccupySeatChairIDArray[i] == INVALID_CHAIR)
			{
				continue;
			}
			IServerUserItem *pIOccupySeatServerUserItem = m_pITableFrame->GetTableUserItem(m_wOccupySeatChairIDArray[i]);
			if(!pIOccupySeatServerUserItem)
			{
				continue;
			}

			StatusPlay.lOccupySeatUserWinScore[i] = m_lUserWinScore[m_wOccupySeatChairIDArray[i]];
		}

		//机器人配置
		if(pIServerUserItem->IsAndroidUser())
		{
			tagCustomConfig *pCustomConfig = (tagCustomConfig *)m_pGameServiceOption->cbCustomRule;
			ASSERT(pCustomConfig);

			CopyMemory(&StatusPlay.CustomAndroid, &pCustomConfig->CustomAndroid, sizeof(tagCustomAndroid));
		}

		//发送场景
		bool bSuccess = m_pITableFrame->SendGameScene(pIServerUserItem, &StatusPlay, sizeof(StatusPlay));

		//限制提示
		TCHAR szTipMsg[128];
		_sntprintf_s(szTipMsg, CountArray(szTipMsg), TEXT("\n本房间上庄条件为：%s\n区域限制为：%s\n玩家限制为：%s"), AddComma(m_lApplyBankerCondition), AddComma(m_lAreaLimitScore), AddComma(m_lUserLimitScore));
		m_pITableFrame->SendGameMessage(pIServerUserItem, szTipMsg, SMT_CHAT);

		//发送申请者
		SendApplyUser(pIServerUserItem);

		return bSuccess;
	}
	}

	return false;
}

//定时器事件
bool  CTableFrameSink::OnTimerMessage(DWORD dwTimerID, WPARAM dwBindParameter)
{
	switch(dwTimerID)
	{
	case IDI_FREE:		//空闲时间
	{
		//开始游戏
		m_pITableFrame->StartGame();

		//设置时间
		m_dwBetTime = (DWORD)time(NULL);
		m_pITableFrame->SetGameTimer(IDI_PLACE_JETTON, m_cbBetTime * 1000, 1, 0L);

		//设置状态
		m_pITableFrame->SetGameStatus(GAME_SCENE_BET);

		return true;
	}
	case IDI_PLACE_JETTON:		//下注时间
	{
		//状态判断(防止强退重复设置)
		if(m_pITableFrame->GetGameStatus() != GAME_SCENE_END)
		{
			//设置状态
			m_pITableFrame->SetGameStatus(GAME_SCENE_END);

			//结束游戏
			OnEventGameConclude(INVALID_CHAIR, NULL, GER_NORMAL);

			//设置时间
			m_dwBetTime = (DWORD)time(NULL);
			m_pITableFrame->SetGameTimer(IDI_GAME_END, m_cbEndTime * 1000, 1, 0L);
		}

		return true;
	}
	case IDI_GAME_END:			//结束游戏
	{
		tagScoreInfo ScoreInfoArray[GAME_PLAYER];
		ZeroMemory(&ScoreInfoArray, sizeof(ScoreInfoArray));
		LONGLONG TempStartScore = 0;
		//写入积分
		for(WORD wUserChairID = 0; wUserChairID < GAME_PLAYER; ++wUserChairID)
		{
			IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(wUserChairID);
			if(pIServerUserItem == NULL)
			{
				continue;
			}

			//判断是否写分
			bool bXie = false;
			for(WORD iA = 0; iA < AREA_MAX; ++iA)
			{
				if(m_lPlayBet[wUserChairID][iA] != 0)
				{
					bXie = true;
					break;
				}
			}

			//当前庄家
			if(wUserChairID == m_wCurrentBanker)
			{
				bXie = true;
			}

			//离线庄家
			if(wUserChairID == m_wOfflineBanker)
			{
				bXie = true;
			}

			if(!bXie)
			{
				continue;
			}

			//防止负分
			if(((pIServerUserItem->GetUserScore() - m_nServiceCharge) + m_lUserWinScore[wUserChairID]) < 0L)
			{
				m_lUserWinScore[wUserChairID] = -(pIServerUserItem->GetUserScore() - m_nServiceCharge);
			}

			//写入积分
			ScoreInfoArray[wUserChairID].lScore = m_lUserWinScore[wUserChairID];
			ScoreInfoArray[wUserChairID].lRevenue = m_lUserRevenue[wUserChairID];

			if(ScoreInfoArray[wUserChairID].lScore > 0)
			{
				ScoreInfoArray[wUserChairID].cbType = SCORE_TYPE_WIN;
			}
			else if(ScoreInfoArray[wUserChairID].lScore < 0)
			{
				ScoreInfoArray[wUserChairID].cbType = SCORE_TYPE_LOSE;
			}
			else
			{
				ScoreInfoArray[wUserChairID].cbType = SCORE_TYPE_DRAW;
			}

			////库存金币
			//if (!pIServerUserItem->IsAndroidUser())
			//	//m_lStorageCurrent -= m_lUserWinScore[wUserChairID];
			//	m_lStorageCurrent -= (m_lUserWinScore[wUserChairID]+m_lUserRevenue[wUserChairID]);
			//	//m_lStorageCurrent -= m_lUserRevenue[wUserChairID];
		}

		m_pITableFrame->WriteTableScore(ScoreInfoArray, CountArray(ScoreInfoArray));

		//库存衰减
		//INT lTempDeduct=0;
		//lTempDeduct=m_lStorageDeduct;
		//bool bDeduct=NeedDeductStorage();
		//lTempDeduct=bDeduct?lTempDeduct:0;
		//if (m_lStorageCurrent > 0)
		//	m_lStorageCurrent = m_lStorageCurrent - m_lStorageCurrent*lTempDeduct/1000;

		//设置时间
		m_dwBetTime = (DWORD)time(NULL);
		m_pITableFrame->SetGameTimer(IDI_FREE, m_cbFreeTime * 1000, 1, 0L);

		//发送消息
		CMD_S_GameFree GameFree;
		ZeroMemory(&GameFree, sizeof(GameFree));
		GameFree.cbTimeLeave = m_cbFreeTime;
		m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_GAME_FREE, &GameFree, sizeof(GameFree));
		m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_GAME_FREE, &GameFree, sizeof(GameFree));

		//结束游戏
		m_pITableFrame->ConcludeGame(GAME_SCENE_FREE);

		//切换庄家
		ChangeBanker(false);

		return true;
	}
	}

	return false;
}

//游戏消息处理
bool  CTableFrameSink::OnGameMessage(WORD wSubCmdID, VOID *pData, WORD wDataSize, IServerUserItem *pIServerUserItem)
{
	switch(wSubCmdID)
	{
	case SUB_C_PLACE_JETTON:		//用户加注
	{
		//效验数据
		ASSERT(wDataSize == sizeof(CMD_C_PlaceBet));
		if(wDataSize != sizeof(CMD_C_PlaceBet))
		{
			return false;
		}

		//用户效验
		//if ( pIServerUserItem->GetUserStatus() != US_PLAYING ) return true;

		//消息处理
		CMD_C_PlaceBet *pPlaceBet = (CMD_C_PlaceBet *)pData;
		return OnUserPlayBet(pIServerUserItem->GetChairID(), pPlaceBet->cbBetArea, pPlaceBet->lBetScore);
	}
	case SUB_C_APPLY_BANKER:		//申请做庄
	{
		//用户效验
		if(pIServerUserItem->GetUserStatus() == US_LOOKON)
		{
			return true;
		}

		return OnUserApplyBanker(pIServerUserItem);
	}
	case SUB_C_CANCEL_BANKER:		//取消做庄
	{
		//用户效验
		if(pIServerUserItem->GetUserStatus() == US_LOOKON)
		{
			return true;
		}

		return OnUserCancelBanker(pIServerUserItem);
	}
	case SUB_C_SUPERROB_BANKER:
	{
		//用户效验
		if(pIServerUserItem->GetUserStatus() == US_LOOKON)
		{
			return true;
		}

		//校验条件
		if(m_superbankerConfig.superbankerType == SUPERBANKER_VIPTYPE)
		{
			ASSERT(pIServerUserItem->GetMemberOrder() >= GetMemberOrderIndex(m_superbankerConfig.enVipIndex));

			if(!(pIServerUserItem->GetMemberOrder() >= GetMemberOrderIndex(m_superbankerConfig.enVipIndex)))
			{
				return true;
			}
		}
		else if(m_superbankerConfig.superbankerType == SUPERBANKER_CONSUMETYPE)
		{
			ASSERT(pIServerUserItem->GetUserScore() > m_superbankerConfig.lSuperBankerConsume);

			if(!(pIServerUserItem->GetUserScore() > m_superbankerConfig.lSuperBankerConsume))
			{
				return true;
			}
		}

		//已有超级抢庄玩家
		if(m_wCurSuperRobBankerUser != INVALID_CHAIR)
		{
			CMD_S_SuperRobBanker SuperRobBanker;
			ZeroMemory(&SuperRobBanker, sizeof(SuperRobBanker));
			SuperRobBanker.bSucceed = false;
			SuperRobBanker.wCurSuperRobBankerUser = m_wCurSuperRobBankerUser;
			SuperRobBanker.wApplySuperRobUser = pIServerUserItem->GetChairID();

			//发送消息
			m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_SUPERROB_BANKER, &SuperRobBanker, sizeof(SuperRobBanker));
			m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_SUPERROB_BANKER, &SuperRobBanker, sizeof(SuperRobBanker));
		}
		else
		{
			CMD_S_SuperRobBanker SuperRobBanker;
			ZeroMemory(&SuperRobBanker, sizeof(SuperRobBanker));
			SuperRobBanker.bSucceed = true;
			SuperRobBanker.wCurSuperRobBankerUser = pIServerUserItem->GetChairID();
			SuperRobBanker.wApplySuperRobUser = pIServerUserItem->GetChairID();
			m_wCurSuperRobBankerUser = pIServerUserItem->GetChairID();

			//超级抢庄消耗类型
			if(m_superbankerConfig.superbankerType == SUPERBANKER_CONSUMETYPE)
			{
				tagScoreInfo ScoreInfo;
				ZeroMemory(&ScoreInfo, sizeof(ScoreInfo));
				ScoreInfo.cbType = SCORE_TYPE_SERVICE;
				ScoreInfo.lRevenue = 0;
				ScoreInfo.lScore = -m_superbankerConfig.lSuperBankerConsume;
				m_pITableFrame->WriteUserScore(pIServerUserItem->GetChairID(), ScoreInfo);
			}

			m_ApplyUserArray.Add(INVALID_CHAIR);
			//改变上庄列表顺序
			for(WORD i = m_ApplyUserArray.GetCount() - 1; i > 0; i--)
			{
				m_ApplyUserArray[i] = m_ApplyUserArray[i - 1];
			}

			m_ApplyUserArray[0] = pIServerUserItem->GetChairID();


			//发送消息
			m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_SUPERROB_BANKER, &SuperRobBanker, sizeof(SuperRobBanker));
			m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_SUPERROB_BANKER, &SuperRobBanker, sizeof(SuperRobBanker));
		}

		return true;
	}
	case SUB_C_OCCUPYSEAT:
	{
		//效验数据
		ASSERT(wDataSize == sizeof(CMD_C_OccupySeat));
		if(wDataSize != sizeof(CMD_C_OccupySeat))
		{
			return true;
		}

		//消息处理
		CMD_C_OccupySeat *pOccupySeat = (CMD_C_OccupySeat *)pData;
		return OnUserOccupySeat(pOccupySeat->wOccupySeatChairID, pOccupySeat->cbOccupySeatIndex);
	}
	case SUB_C_QUIT_OCCUPYSEAT:
	{
		bool binvalid = false;
		//校验数据
		for(WORD i = 0; i < MAX_OCCUPY_SEAT_COUNT; i++)
		{
			if(m_wOccupySeatChairIDArray[i] == pIServerUserItem->GetChairID())
			{
				binvalid = true;

				//重置无效
				m_wOccupySeatChairIDArray[i] = INVALID_CHAIR;

				break;
			}
		}

		ASSERT(binvalid == true);

		CMD_S_UpdateOccupySeat UpdateOccupySeat;
		ZeroMemory(&UpdateOccupySeat, sizeof(UpdateOccupySeat));
		CopyMemory(UpdateOccupySeat.wOccupySeatChairIDArray, m_wOccupySeatChairIDArray, sizeof(m_wOccupySeatChairIDArray));
		UpdateOccupySeat.wQuitOccupySeatChairID = pIServerUserItem->GetChairID();

		//发送数据
		m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_UPDATE_OCCUPYSEAT, &UpdateOccupySeat, sizeof(UpdateOccupySeat));
		m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_UPDATE_OCCUPYSEAT, &UpdateOccupySeat, sizeof(UpdateOccupySeat));

		return true;
	}
	}

	return false;
}

//框架消息处理
bool CTableFrameSink::OnFrameMessage(WORD wSubCmdID, VOID *pData, WORD wDataSize, IServerUserItem *pIServerUserItem)
{
	// 消息处理
	if(wSubCmdID >= SUB_GF_FRAME_MESSAG_GAME_MIN && wSubCmdID <= SUB_GF_FRAME_MESSAG_GAME_MAX)
	{
		switch(wSubCmdID - SUB_GF_FRAME_MESSAG_GAME_MIN)
		{
		case SUB_C_AMDIN_COMMAND:
		{
			ASSERT(wDataSize == sizeof(CMD_C_AdminReq));
			if(wDataSize != sizeof(CMD_C_AdminReq))
			{
				return false;
			}

			if(m_pServerContro == NULL)
			{
				return false;
			}
			CopyMemory(m_szControlName, pIServerUserItem->GetNickName(), sizeof(m_szControlName));

			return m_pServerContro->ServerControl(wSubCmdID, pData, wDataSize, pIServerUserItem, m_pITableFrame, m_pGameServiceOption);
		}
		case SUB_C_UPDATE_STORAGE:		//更新库存
		{
			ASSERT(wDataSize == sizeof(CMD_C_UpdateStorage));
			if(wDataSize != sizeof(CMD_C_UpdateStorage))
			{
				return false;
			}

			//权限判断
			if(CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()) == false)
			{
				return false;
			}

			//消息处理
			CMD_C_UpdateStorage *pUpdateStorage = (CMD_C_UpdateStorage *)pData;
			if(pUpdateStorage->cbReqType == RQ_SET_STORAGE)
			{
				LONGLONG lbeforeStorage = m_lStorageCurrent;
				m_lStorageDeduct = pUpdateStorage->lStorageDeduct;
				m_lStorageCurrent = pUpdateStorage->lStorageCurrent;
				m_lStorageMax1 = pUpdateStorage->lStorageMax1;
				m_lStorageMul1 = pUpdateStorage->lStorageMul1;
				m_lStorageMax2 = pUpdateStorage->lStorageMax2;
				m_lStorageMul2 = pUpdateStorage->lStorageMul2;

				//记录信息
				CString strControlInfo;
				strControlInfo.Format(TEXT("房间: %s | 桌号: %u | 控制人账号: %s | 控制人ID: %u 修改库存设置, 库存由[%I64d]修改为[%I64d]"),
					m_pGameServiceOption->szServerName, m_pITableFrame->GetTableID() + 1, pIServerUserItem->GetNickName(),
					pIServerUserItem->GetGameID(), lbeforeStorage, m_lStorageCurrent);

				WriteInfo(TEXT("欢乐30秒控制信息.log"), strControlInfo);
			}

			CMD_S_UpdateStorage updateStorage;
			ZeroMemory(&updateStorage, sizeof(updateStorage));

			if(RQ_SET_STORAGE == pUpdateStorage->cbReqType)
			{
				updateStorage.cbReqType = RQ_SET_STORAGE;
			}
			else
			{
				updateStorage.cbReqType = RQ_REFRESH_STORAGE;
			}

			updateStorage.lStorageStart = m_lStorageStart;
			updateStorage.lStorageDeduct = m_lStorageDeduct;
			updateStorage.lStorageCurrent = m_lStorageCurrent;
			updateStorage.lStorageMax1 = m_lStorageMax1;
			updateStorage.lStorageMul1 = m_lStorageMul1;
			updateStorage.lStorageMax2 = m_lStorageMax2;
			updateStorage.lStorageMul2 = m_lStorageMul2;

			m_pITableFrame->SendRoomData(pIServerUserItem, SUB_S_UPDATE_STORAGE, &updateStorage, sizeof(updateStorage));


			return true;
		}
		}
	}
	// 聊天消息
	if(wSubCmdID == SUB_GF_USER_CHAT && CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()))
	{
		//变量定义
		CMD_GF_C_UserChat *pUserChat = (CMD_GF_C_UserChat *)pData;

		//效验参数
		ASSERT(wDataSize <= sizeof(CMD_GF_C_UserChat));
		ASSERT(wDataSize >= (sizeof(CMD_GF_C_UserChat) - sizeof(pUserChat->szChatString)));
		ASSERT(wDataSize == (sizeof(CMD_GF_C_UserChat) - sizeof(pUserChat->szChatString) + pUserChat->wChatLength * sizeof(pUserChat->szChatString[0])));

		//效验参数
		if(wDataSize > sizeof(CMD_GF_C_UserChat))
		{
			return false;
		}
		if(wDataSize < (sizeof(CMD_GF_C_UserChat) - sizeof(pUserChat->szChatString)))
		{
			return false;
		}
		if(wDataSize != (sizeof(CMD_GF_C_UserChat) - sizeof(pUserChat->szChatString) + pUserChat->wChatLength * sizeof(pUserChat->szChatString[0])))
		{
			return false;
		}

		bool bKeyProcess = false;
		CString strChatString(pUserChat->szChatString);
		CString strKey[KEY_MAX] = { TEXT("/stock"), TEXT("/immortal count"), TEXT("/robot count"), TEXT("/immortal bet"), TEXT("/robot bet") };
		CString strName[KEY_MAX] = { TEXT("库存"), TEXT("真人数量"), TEXT("机器人数量"), TEXT("真人下注"), TEXT("机器人下注") };
		if(strChatString == TEXT("/help"))
		{
			bKeyProcess = true;
			CString strMsg;
			for(int i = 0 ; i < KEY_MAX; ++i)
			{
				strMsg += TEXT("\n");
				strMsg += strKey[i];
				strMsg += TEXT(" 查看");
				strMsg += strName[i];
			}
			m_pITableFrame->SendGameMessage(pIServerUserItem, strMsg, SMT_CHAT);
		}
		else
		{
			CString strMsg;
			for(int i = 0 ; i < KEY_MAX; ++i)
			{
				if(strChatString == strKey[i])
				{
					bKeyProcess = true;
					switch(i)
					{
					case KEY_STOCK:
					{
						strMsg.Format(TEXT("库存剩余量：%I64d"), m_lStorageCurrent);
					}
					break;
					case KEY_IMMORTAL_COUNT:
					{
						int nImmortal = 0;
						for(WORD wChairID = 0; wChairID < GAME_PLAYER; ++wChairID)
						{
							//获取用户
							IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
							if(pIServerUserItem == NULL)
							{
								continue;
							}

							if(!pIServerUserItem->IsAndroidUser())
							{
								nImmortal += 1;
							}
						}
						strMsg.Format(TEXT("真人数量：%d"), nImmortal);
					}
					break;
					case KEY_ROBOT_COUNT:
					{
						int nRobot = 0;
						for(WORD wChairID = 0; wChairID < GAME_PLAYER; ++wChairID)
						{
							//获取用户
							IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
							if(pIServerUserItem == NULL)
							{
								continue;
							}

							if(pIServerUserItem->IsAndroidUser())
							{
								nRobot += 1;
							}
						}
						strMsg.Format(TEXT("机器人数量：%d"), nRobot);
					}
					break;
					case KEY_IMMORTAL_BET:
					{
						LONGLONG lBet[AREA_MAX] = {0};
						for(WORD wChairID = 0; wChairID < GAME_PLAYER; ++wChairID)
						{
							//获取用户
							IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
							if(pIServerUserItem == NULL)
							{
								continue;
							}

							if(!pIServerUserItem->IsAndroidUser())
							{
								for(int nArea = 0; nArea < AREA_MAX; ++nArea)
								{
									lBet[nArea] += m_lPlayBet[wChairID][nArea];
								}
							}
						}

						strMsg.Format(TEXT("玩家下注：\n 闲：%I64d \n 平：%I64d \n 庄：%I64d \n 闲天王：%I64d \n 庄天王：%I64d \n 同点平：%I64d \n 闲对子：%I64d \n 庄对子：%I64d"),
									  lBet[AREA_XIAN], lBet[AREA_PING], lBet[AREA_ZHUANG], lBet[AREA_XIAN_TIAN], lBet[AREA_ZHUANG_TIAN], lBet[AREA_TONG_DUI], lBet[AREA_XIAN_DUI], lBet[AREA_ZHUANG_DUI]);
					}
					break;
					case KEY_ROBOT_BET:
					{
						LONGLONG lBet[AREA_MAX] = {0};
						for(WORD wChairID = 0; wChairID < GAME_PLAYER; ++wChairID)
						{
							//获取用户
							IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
							if(pIServerUserItem == NULL)
							{
								continue;
							}

							if(pIServerUserItem->IsAndroidUser())
							{
								for(int nArea = 0; nArea < AREA_MAX; ++nArea)
								{
									lBet[nArea] += m_lPlayBet[wChairID][nArea];
								}
							}
						}

						strMsg.Format(TEXT("机器人下注：\n 闲：%I64d \n 平：%I64d \n 庄：%I64d \n 闲天王：%I64d \n 庄天王：%I64d \n 同点平：%I64d \n 闲对子：%I64d \n 庄对子：%I64d"),
									  lBet[AREA_XIAN], lBet[AREA_PING], lBet[AREA_ZHUANG], lBet[AREA_XIAN_TIAN], lBet[AREA_ZHUANG_TIAN], lBet[AREA_TONG_DUI], lBet[AREA_XIAN_DUI], lBet[AREA_ZHUANG_DUI]);
					}
					break;
					}
					m_pITableFrame->SendGameMessage(pIServerUserItem, strMsg, SMT_CHAT);
					break;
				}
			}
		}
		return bKeyProcess;
	}
	return false;
}

//用户坐下
bool CTableFrameSink::OnActionUserSitDown(WORD wChairID, IServerUserItem *pIServerUserItem, bool bLookonUser)
{
	//起始分数
	m_lUserStartScore[wChairID] = pIServerUserItem->GetUserScore();

	//设置时间
	if((bLookonUser == false) && (m_dwBetTime == 0L))
	{
		m_dwBetTime = (DWORD)time(NULL);
		m_pITableFrame->SetGameTimer(IDI_FREE, m_cbFreeTime * 1000, 1, NULL);
		m_pITableFrame->SetGameStatus(GAME_SCENE_FREE);
	}

	//限制提示
	TCHAR szTipMsg[128];
	_sntprintf_s(szTipMsg, CountArray(szTipMsg), TEXT("\n本房间上庄条件为：%I64d\n区域限制为：%I64d\n玩家限制为：%I64d"),
				 AddComma(m_lApplyBankerCondition), AddComma(m_lAreaLimitScore), AddComma(m_lUserLimitScore));
	m_pITableFrame->SendGameMessage(pIServerUserItem, szTipMsg, SMT_CHAT);

	return true;
}

//用户起来
bool CTableFrameSink::OnActionUserStandUp(WORD wChairID, IServerUserItem *pIServerUserItem, bool bLookonUser)
{
	//起始分数
	m_lUserStartScore[wChairID] = 0;

	if(wChairID == m_wCurSuperRobBankerUser)
	{
		m_wCurSuperRobBankerUser = INVALID_CHAIR;

		CMD_S_CurSuperRobLeave CurSuperRobLeave;
		ZeroMemory(&CurSuperRobLeave, sizeof(CurSuperRobLeave));

		//设置变量
		CurSuperRobLeave.wCurSuperRobBankerUser = m_wCurSuperRobBankerUser;

		//发送消息
		m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_CURSUPERROB_LEAVE, &CurSuperRobLeave, sizeof(CurSuperRobLeave));
		m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_CURSUPERROB_LEAVE, &CurSuperRobLeave, sizeof(CurSuperRobLeave));
	}

	bool bvalid = false;
	//校验数据
	for(WORD i = 0; i < MAX_OCCUPY_SEAT_COUNT; i++)
	{
		if(m_wOccupySeatChairIDArray[i] == wChairID)
		{
			bvalid = true;

			//重置无效
			m_wOccupySeatChairIDArray[i] = INVALID_CHAIR;

			break;
		}
	}

	if(bvalid == true)
	{
		CMD_S_UpdateOccupySeat UpdateOccupySeat;
		ZeroMemory(&UpdateOccupySeat, sizeof(UpdateOccupySeat));
		CopyMemory(UpdateOccupySeat.wOccupySeatChairIDArray, m_wOccupySeatChairIDArray, sizeof(m_wOccupySeatChairIDArray));
		UpdateOccupySeat.wQuitOccupySeatChairID = wChairID;

		//发送数据
		m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_UPDATE_OCCUPYSEAT, &UpdateOccupySeat, sizeof(UpdateOccupySeat));
		m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_UPDATE_OCCUPYSEAT, &UpdateOccupySeat, sizeof(UpdateOccupySeat));
	}

	//记录成绩
	if(bLookonUser == false)
	{
		//切换庄家
		if(wChairID == m_wCurrentBanker)
		{
			ChangeBanker(true);
		}

		//取消申请
		for(WORD i = 0; i < m_ApplyUserArray.GetCount(); ++i)
		{
			if(wChairID != m_ApplyUserArray[i])
			{
				continue;
			}

			//删除玩家
			m_ApplyUserArray.RemoveAt(i);

			//构造变量
			CMD_S_CancelBanker CancelBanker;
			ZeroMemory(&CancelBanker, sizeof(CancelBanker));

			//设置变量
			CancelBanker.wCancelUser = pIServerUserItem->GetChairID();

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
bool CTableFrameSink::OnUserPlayBet(WORD wChairID, BYTE cbBetArea, LONGLONG lBetScore)
{
	//效验参数
	ASSERT((cbBetArea <= AREA_ZHUANG_DUI) && (lBetScore > 0L));
	if((cbBetArea > AREA_ZHUANG_DUI) || (lBetScore <= 0L))
	{
		return false;
	}

	//效验状态
	if(m_pITableFrame->GetGameStatus() != GAME_SCENE_BET)
	{
		SendPlaceBetFail(wChairID, cbBetArea, lBetScore);
		return true;
	}

	//庄家判断
	if(m_wCurrentBanker == wChairID)
	{
		SendPlaceBetFail(wChairID, cbBetArea, lBetScore);
		return true;
	}
	if(m_bEnableSysBanker == false && m_wCurrentBanker == INVALID_CHAIR)
	{
		SendPlaceBetFail(wChairID, cbBetArea, lBetScore);
		return true;
	}

	//庄家积分
	LONGLONG lBankerScore = 0;
	if(INVALID_CHAIR != m_wCurrentBanker)
	{
		IServerUserItem *pIServerUserItemBanker = m_pITableFrame->GetTableUserItem(m_wCurrentBanker);
		lBankerScore = pIServerUserItemBanker->GetUserScore();
	}

	//变量定义
	IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
	LONGLONG lUserScore = pIServerUserItem->GetUserScore() - m_nServiceCharge;
	LONGLONG lBetCount = 0;
	for(int i = 0; i < AREA_MAX; ++i)
	{
		lBetCount += m_lPlayBet[wChairID][i];
	}

	LONGLONG lAllBetCount = 0L;
	for(int nAreaIndex = 0; nAreaIndex < AREA_MAX; ++nAreaIndex)
	{
		lAllBetCount += m_lAllBet[nAreaIndex];
	}

	//成功标识
	bool bPlaceBetSuccess = true;

	//合法校验
	if(lUserScore < lBetCount + lBetScore)
	{
		SendPlaceBetFail(wChairID, cbBetArea, lBetScore);
		return true;
	}
	else
	{
		//机器人验证
		if(pIServerUserItem->IsAndroidUser())
		{
			//区域限制
			if(m_lRobotAreaScore[cbBetArea] + lBetScore > m_lRobotAreaLimit)
			{
				return true;
			}

			//数目限制
			bool bHaveChip = false;
			bHaveChip = (lBetCount > 0);

			if(!bHaveChip)
			{
				if(m_nChipRobotCount + 1 > m_nMaxChipRobot)
				{
					SendPlaceBetFail(wChairID, cbBetArea, lBetScore);
					return true;
				}
				else
				{
					m_nChipRobotCount++;
				}
			}

			//统计分数
			if(bPlaceBetSuccess)
			{
				m_lRobotAreaScore[cbBetArea] += lBetScore;
			}
		}
	}

	if(m_lUserLimitScore < lBetCount + lBetScore)
	{
		SendPlaceBetFail(wChairID, cbBetArea, lBetScore);
		return true;
	}


	//合法验证
	if(GetMaxPlayerScore(cbBetArea, wChairID) >= lBetScore)
	{
		m_lAllBet[cbBetArea] += lBetScore;
		m_lPlayBet[wChairID][cbBetArea]  += lBetScore;
	}
	else
	{
		bPlaceBetSuccess = false;
	}

	if(bPlaceBetSuccess)
	{
		//变量定义
		CMD_S_PlaceBet PlaceBet;
		ZeroMemory(&PlaceBet, sizeof(PlaceBet));

		//构造变量
		PlaceBet.wChairID = wChairID;
		PlaceBet.cbBetArea = cbBetArea;
		PlaceBet.lBetScore = lBetScore;
		PlaceBet.cbAndroidUser = pIServerUserItem->IsAndroidUser() ? TRUE : FALSE;
		PlaceBet.cbAndroidUserT = pIServerUserItem->IsAndroidUser() ? TRUE : FALSE;

		//发送消息
		m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_PLACE_JETTON, &PlaceBet, sizeof(PlaceBet));
		m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_PLACE_JETTON, &PlaceBet, sizeof(PlaceBet));

		//发送玩家下注信息
		if(!pIServerUserItem->IsAndroidUser())
		{
			//给超端发送下注信息
			SendUserBetInfo();
		}
	}
	else
	{
		//发送消息
		SendPlaceBetFail(wChairID, cbBetArea, lBetScore);
	}

	return true;
}

//发送消息
void CTableFrameSink::SendPlaceBetFail(WORD wChairID, BYTE cbBetArea, LONGLONG lBetScore)
{
	CMD_S_PlaceBetFail PlaceBetFail;
	ZeroMemory(&PlaceBetFail, sizeof(PlaceBetFail));
	PlaceBetFail.lBetArea = cbBetArea;
	PlaceBetFail.lPlaceScore = lBetScore;
	PlaceBetFail.wPlaceUser = wChairID;

	//发送消息
	m_pITableFrame->SendTableData(wChairID, SUB_S_PLACE_JETTON_FAIL, &PlaceBetFail, sizeof(PlaceBetFail));
}

//发送扑克
bool CTableFrameSink::DispatchTableCard()
{
	//随机扑克
	m_GameLogic.RandCardList(m_cbTableCardArray[0], sizeof(m_cbTableCardArray) / sizeof(m_cbTableCardArray[0][0]));

	//首次发牌
	m_cbCardCount[INDEX_PLAYER] = 2;
	m_cbCardCount[INDEX_BANKER] = 2;

	//计算点数
	BYTE cbBankerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);
	BYTE cbPlayerTwoCardCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);

	//闲家补牌
	BYTE cbPlayerThirdCardValue = 0 ;	//第三张牌点数
	if(cbPlayerTwoCardCount <= 5 && cbBankerCount < 8)
	{
		//计算点数
		m_cbCardCount[INDEX_PLAYER]++;
		cbPlayerThirdCardValue = m_GameLogic.GetCardPip(m_cbTableCardArray[INDEX_PLAYER][2]);
	}

	//庄家补牌
	if(cbPlayerTwoCardCount < 8 && cbBankerCount < 8)
	{
		switch(cbBankerCount)
		{
		case 0:
		case 1:
		case 2:
			m_cbCardCount[INDEX_BANKER]++ ;
			break;

		case 3:
			if((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 8) || m_cbCardCount[INDEX_PLAYER] == 2)
			{
				m_cbCardCount[INDEX_BANKER]++ ;
			}
			break;

		case 4:
			if((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 8 && cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) || m_cbCardCount[INDEX_PLAYER] == 2)
			{
				m_cbCardCount[INDEX_BANKER]++ ;
			}
			break;

		case 5:
			if((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 2 && cbPlayerThirdCardValue != 3  && cbPlayerThirdCardValue != 8 && cbPlayerThirdCardValue != 9 &&  cbPlayerThirdCardValue != 0) || m_cbCardCount[INDEX_PLAYER] == 2)
			{
				m_cbCardCount[INDEX_BANKER]++ ;
			}
			break;

		case 6:
			if(m_cbCardCount[INDEX_PLAYER] == 3 && (cbPlayerThirdCardValue == 6 || cbPlayerThirdCardValue == 7))
			{
				m_cbCardCount[INDEX_BANKER]++ ;
			}
			break;

		//不须补牌
		case 7:
		case 8:
		case 9:
			break;
		default:
			break;
		}
	}

	return true;
}

//申请庄家
bool CTableFrameSink::OnUserApplyBanker(IServerUserItem *pIApplyServerUserItem)
{
	//合法判断
	LONGLONG lUserScore = pIApplyServerUserItem->GetUserScore();
	if(lUserScore < m_lApplyBankerCondition)
	{
		m_pITableFrame->SendGameMessage(pIApplyServerUserItem, TEXT("你的金币不足以申请庄家，申请失败！"), SMT_CHAT | SMT_EJECT);
		return true;
	}

	//存在判断
	WORD wApplyUserChairID = pIApplyServerUserItem->GetChairID();
	for(INT_PTR nUserIdx = 0; nUserIdx < m_ApplyUserArray.GetCount(); ++nUserIdx)
	{
		WORD wChairID = m_ApplyUserArray[nUserIdx];
		if(wChairID == wApplyUserChairID)
		{
			m_pITableFrame->SendGameMessage(pIApplyServerUserItem, TEXT("你已经申请了庄家，不需要再次申请！"), SMT_CHAT | SMT_EJECT);
			return true;
		}
	}

	if(pIApplyServerUserItem->IsAndroidUser() && (m_ApplyUserArray.GetCount()) > m_nRobotListMaxCount)
	{
		return true;
	}

	//保存信息
	m_ApplyUserArray.Add(wApplyUserChairID);

	//构造变量
	CMD_S_ApplyBanker ApplyBanker;
	ZeroMemory(&ApplyBanker, sizeof(ApplyBanker));

	//设置变量
	ApplyBanker.wApplyUser = wApplyUserChairID;

	//发送消息
	m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_APPLY_BANKER, &ApplyBanker, sizeof(ApplyBanker));
	m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_APPLY_BANKER, &ApplyBanker, sizeof(ApplyBanker));

	//切换判断
	if(m_pITableFrame->GetGameStatus() == GAME_SCENE_FREE && m_ApplyUserArray.GetCount() == 1)
	{
		ChangeBanker(false);
	}

	return true;
}

//取消申请
bool CTableFrameSink::OnUserCancelBanker(IServerUserItem *pICancelServerUserItem)
{
	//当前庄家
	if(pICancelServerUserItem->GetChairID() == m_wCurrentBanker && m_pITableFrame->GetGameStatus() != GAME_SCENE_FREE)
	{
		//发送消息
		m_pITableFrame->SendGameMessage(pICancelServerUserItem, TEXT("游戏已经开始，不可以取消当庄！"), SMT_CHAT | SMT_EJECT);
		return true;
	}

	bool bValidQuitBanker = true;
	//取消上庄列表上的（取消申请）
	for(WORD i = 0; i < m_ApplyUserArray.GetCount(); ++i)
	{
		//获取玩家
		WORD wChairID = m_ApplyUserArray[i];
		IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);

		//条件过滤
		if(pIServerUserItem == NULL)
		{
			continue;
		}
		if(pIServerUserItem->GetUserID() != pICancelServerUserItem->GetUserID())
		{
			continue;
		}

		//删除玩家
		m_ApplyUserArray.RemoveAt(i);

		bValidQuitBanker = false;

		//超级抢庄玩家
		if(wChairID == m_wCurSuperRobBankerUser)
		{
			m_wCurSuperRobBankerUser = INVALID_CHAIR;
		}

		if(m_wCurrentBanker != wChairID)
		{
			//构造变量
			CMD_S_CancelBanker CancelBanker;
			ZeroMemory(&CancelBanker, sizeof(CancelBanker));

			//设置变量
			CancelBanker.wCancelUser = pIServerUserItem->GetChairID();

			//发送消息
			m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_CANCEL_BANKER, &CancelBanker, sizeof(CancelBanker));
			m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_CANCEL_BANKER, &CancelBanker, sizeof(CancelBanker));
		}
		else if(m_wCurrentBanker == wChairID)
		{
			//切换庄家
			m_wCurrentBanker = INVALID_CHAIR;
			m_typeCurrentBanker = INVALID_SYSBANKER;
			ChangeBanker(true);
		}

		return true;
	}

	if(bValidQuitBanker == true)
	{
		m_wCurrentBanker = INVALID_CHAIR;
		m_typeCurrentBanker = INVALID_SYSBANKER;
		ChangeBanker(true);
		return true;
	}

	return false;
}

//用户占位
bool CTableFrameSink::OnUserOccupySeat(WORD wOccupyChairID, BYTE cbOccupySeatIndex)
{
	//校验用户
	ASSERT(wOccupyChairID != INVALID_CHAIR);
	if(wOccupyChairID == INVALID_CHAIR)
	{
		return true;
	}

	ASSERT(cbOccupySeatIndex != SEAT_INVALID_INDEX);

	ASSERT(cbOccupySeatIndex >= SEAT_LEFT1_INDEX && cbOccupySeatIndex <= SEAT_RIGHT4_INDEX);
	if(!(cbOccupySeatIndex >= SEAT_LEFT1_INDEX && cbOccupySeatIndex <= SEAT_RIGHT4_INDEX))
	{
		return true;
	}

	if(m_wOccupySeatChairIDArray[cbOccupySeatIndex] != INVALID_CHAIR)
	{
		return true;
	}
	ASSERT(m_wOccupySeatChairIDArray[cbOccupySeatIndex] == INVALID_CHAIR);

	//校验是否已经占位
	for(WORD i = 0; i < MAX_OCCUPY_SEAT_COUNT; i++)
	{
		//占位失败
		if(m_wOccupySeatChairIDArray[i] == wOccupyChairID)
		{
			CMD_S_OccupySeat_Fail OccupySeat_Fail;
			ZeroMemory(&OccupySeat_Fail, sizeof(OccupySeat_Fail));
			CopyMemory(OccupySeat_Fail.wOccupySeatChairIDArray, m_wOccupySeatChairIDArray, sizeof(m_wOccupySeatChairIDArray));
			OccupySeat_Fail.wAlreadyOccupySeatChairID = wOccupyChairID;
			OccupySeat_Fail.cbAlreadyOccupySeatIndex = i;

			//发送数据
			m_pITableFrame->SendTableData(wOccupyChairID, SUB_S_OCCUPYSEAT_FAIL, &OccupySeat_Fail, sizeof(OccupySeat_Fail));
			m_pITableFrame->SendLookonData(wOccupyChairID, SUB_S_OCCUPYSEAT_FAIL, &OccupySeat_Fail, sizeof(OccupySeat_Fail));

			return true;
		}
	}

	//占位VIP类型
	if(m_occupyseatConfig.occupyseatType == OCCUPYSEAT_VIPTYPE)
	{
		ASSERT(m_pITableFrame->GetTableUserItem(wOccupyChairID)->GetMemberOrder() >= m_occupyseatConfig.enVipIndex);
		if(m_pITableFrame->GetTableUserItem(wOccupyChairID)->GetMemberOrder() < m_occupyseatConfig.enVipIndex)
		{
			return true;
		}

		//校验是否满足占位最低金币条件
		if(m_pITableFrame->GetTableUserItem(wOccupyChairID)->GetUserScore() < m_occupyseatConfig.lForceStandUpCondition)
		{
			return true;
		}
	}
	else if(m_occupyseatConfig.occupyseatType == OCCUPYSEAT_CONSUMETYPE)
	{
		ASSERT(m_pITableFrame->GetTableUserItem(wOccupyChairID)->GetUserScore() >= m_occupyseatConfig.lOccupySeatConsume);
		if(m_pITableFrame->GetTableUserItem(wOccupyChairID)->GetUserScore() < m_occupyseatConfig.lOccupySeatConsume)
		{
			return true;
		}

		//校验是否满足占位最低金币条件
		if(m_pITableFrame->GetTableUserItem(wOccupyChairID)->GetUserScore() < m_occupyseatConfig.lForceStandUpCondition)
		{
			return true;
		}

		//占位消耗
		tagScoreInfo ScoreInfoArray[GAME_PLAYER];
		ZeroMemory(ScoreInfoArray, sizeof(ScoreInfoArray));
		ScoreInfoArray[wOccupyChairID].cbType = SCORE_TYPE_DRAW;
		ScoreInfoArray[wOccupyChairID].lRevenue = 0;
		ScoreInfoArray[wOccupyChairID].lScore = -m_occupyseatConfig.lOccupySeatConsume;

		//占位写分
		m_pITableFrame->WriteTableScore(ScoreInfoArray, CountArray(ScoreInfoArray));
	}
	else if(m_occupyseatConfig.occupyseatType == OCCUPYSEAT_FREETYPE)
	{
		ASSERT(m_pITableFrame->GetTableUserItem(wOccupyChairID)->GetUserScore() >= m_occupyseatConfig.lOccupySeatFree);
		if(m_pITableFrame->GetTableUserItem(wOccupyChairID)->GetUserScore() < m_occupyseatConfig.lOccupySeatFree)
		{
			return true;
		}

		//校验是否满足占位最低金币条件
		if(m_pITableFrame->GetTableUserItem(wOccupyChairID)->GetUserScore() < m_occupyseatConfig.lForceStandUpCondition)
		{
			return true;
		}
	}

	//占位成功
	m_wOccupySeatChairIDArray[cbOccupySeatIndex] = wOccupyChairID;

	CMD_S_OccupySeat OccupySeat;
	ZeroMemory(&OccupySeat, sizeof(OccupySeat));
	CopyMemory(OccupySeat.wOccupySeatChairIDArray, m_wOccupySeatChairIDArray, sizeof(m_wOccupySeatChairIDArray));
	OccupySeat.wOccupySeatChairID = wOccupyChairID;
	OccupySeat.cbOccupySeatIndex = cbOccupySeatIndex;

	//发送数据
	m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_OCCUPYSEAT, &OccupySeat, sizeof(OccupySeat));
	m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_OCCUPYSEAT, &OccupySeat, sizeof(OccupySeat));

	return true;
}

//更换庄家
bool CTableFrameSink::ChangeBanker(bool bCancelCurrentBanker)
{
	//切换标识
	bool bChangeBanker = false;

	//取消当前
	if(bCancelCurrentBanker)
	{
		for(WORD i = 0; i < m_ApplyUserArray.GetCount(); ++i)
		{
			//获取玩家
			WORD wChairID = m_ApplyUserArray[i];

			//条件过滤
			if(wChairID != m_wCurrentBanker)
			{
				continue;
			}

			//删除玩家
			m_ApplyUserArray.RemoveAt(i);

			break;
		}

		//设置庄家
		m_wCurrentBanker = INVALID_CHAIR;

		m_typeCurrentBanker = INVALID_SYSBANKER;

		//轮换判断
		TakeTurns();

		if(m_wCurSuperRobBankerUser != INVALID_CHAIR && m_wCurrentBanker != INVALID_CHAIR)
		{
			ASSERT(m_wCurSuperRobBankerUser == m_wCurrentBanker);
			m_wCurSuperRobBankerUser = INVALID_CHAIR;
			m_typeCurrentBanker = SUPERROB_BANKER;
		}
		else if(m_wCurSuperRobBankerUser == INVALID_CHAIR && m_wCurrentBanker != INVALID_CHAIR)
		{
			m_typeCurrentBanker = ORDINARY_BANKER;
		}
		else if(m_wCurSuperRobBankerUser == INVALID_CHAIR && m_wCurrentBanker == INVALID_CHAIR)
		{
			m_typeCurrentBanker = INVALID_SYSBANKER;
		}

		//设置变量
		bChangeBanker = true;
		m_bExchangeBanker = true;
	}
	//轮庄判断
	else if(m_wCurrentBanker != INVALID_CHAIR)
	{
		if(m_wCurSuperRobBankerUser != INVALID_CHAIR && m_typeCurrentBanker == ORDINARY_BANKER)
		{
			//轮换判断
			TakeTurns();

			if(m_wCurSuperRobBankerUser != INVALID_CHAIR && m_wCurrentBanker != INVALID_CHAIR)
			{
				m_wCurSuperRobBankerUser = INVALID_CHAIR;
				m_typeCurrentBanker = SUPERROB_BANKER;
			}
			else if(m_wCurSuperRobBankerUser == INVALID_CHAIR && m_wCurrentBanker != INVALID_CHAIR)
			{
				m_typeCurrentBanker = ORDINARY_BANKER;
			}
			else if(m_wCurSuperRobBankerUser == INVALID_CHAIR && m_wCurrentBanker == INVALID_CHAIR)
			{
				m_typeCurrentBanker = INVALID_SYSBANKER;
			}

			//撤销玩家
			for(WORD i = 0; i < m_ApplyUserArray.GetCount(); ++i)
			{
				//获取玩家
				WORD wChairID = m_ApplyUserArray[i];

				//条件过滤
				if(wChairID != m_wCurrentBanker)
				{
					continue;
				}

				//删除玩家
				m_ApplyUserArray.RemoveAt(i);

				break;
			}

			bChangeBanker = true;
			m_bExchangeBanker = true;
		}
		else
		{
			//获取庄家
			IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(m_wCurrentBanker);
			if(pIServerUserItem)
			{
				LONGLONG lBankerScore = pIServerUserItem->GetUserScore();

				//次数判断
				if(m_lPlayerBankerMAX <= m_wBankerTime || lBankerScore < m_lApplyBankerCondition)
				{
					//庄家增加判断 同一个庄家情况下只判断一次
					if(m_lPlayerBankerMAX <= m_wBankerTime && m_bExchangeBanker && lBankerScore >= m_lApplyBankerCondition)
					{
						//加庄局数设置：当庄家坐满设定的局数之后(m_nBankerTimeLimit)，
						//所带金币值还超过下面申请庄家列表里面所有玩家金币时，
						//可以再加坐庄m_nBankerTimeAdd局，加庄局数可设置。

						//金币超过m_lExtraBankerScore之后，
						//就算是下面玩家的金币值大于他的金币值，他也可以再加庄m_nExtraBankerTime局。
						bool bScoreMAX = true;
						m_bExchangeBanker = false;

						for(WORD i = 0; i < m_ApplyUserArray.GetCount(); ++i)
						{
							//获取玩家
							WORD wChairID = m_ApplyUserArray[i];
							IServerUserItem *pIUserItem = m_pITableFrame->GetTableUserItem(wChairID);
							LONGLONG lScore = pIServerUserItem->GetUserScore();

							if(wChairID != m_wCurrentBanker && lBankerScore <= lScore)
							{
								bScoreMAX = false;
								break;
							}
						}

						if(bScoreMAX || (lBankerScore > m_lExtraBankerScore && m_lExtraBankerScore != 0l))
						{
							if(bScoreMAX)
							{
								m_lPlayerBankerMAX += m_nBankerTimeAdd;
							}
							if(lBankerScore > m_lExtraBankerScore && m_lExtraBankerScore != 0l)
							{
								m_lPlayerBankerMAX += m_nExtraBankerTime;
							}
							return true;
						}
					}

					//设置庄家
					m_wCurrentBanker = INVALID_CHAIR;

					m_typeCurrentBanker = INVALID_SYSBANKER;

					//轮换判断
					TakeTurns();

					if(m_wCurSuperRobBankerUser != INVALID_CHAIR && m_wCurrentBanker != INVALID_CHAIR)
					{
						m_wCurSuperRobBankerUser = INVALID_CHAIR;
						m_typeCurrentBanker = SUPERROB_BANKER;
					}
					else if(m_wCurSuperRobBankerUser == INVALID_CHAIR && m_wCurrentBanker != INVALID_CHAIR)
					{
						m_typeCurrentBanker = ORDINARY_BANKER;
					}
					else if(m_wCurSuperRobBankerUser == INVALID_CHAIR && m_wCurrentBanker == INVALID_CHAIR)
					{
						m_typeCurrentBanker = INVALID_SYSBANKER;
					}

					//撤销玩家
					for(WORD i = 0; i < m_ApplyUserArray.GetCount(); ++i)
					{
						//获取玩家
						WORD wChairID = m_ApplyUserArray[i];

						//条件过滤
						if(wChairID != m_wCurrentBanker)
						{
							continue;
						}

						//删除玩家
						m_ApplyUserArray.RemoveAt(i);

						break;
					}

					bChangeBanker = true;
					m_bExchangeBanker = true;

					//提示消息
					TCHAR szTipMsg[128];
					if(lBankerScore < m_lApplyBankerCondition)
					{
						_sntprintf_s(szTipMsg, CountArray(szTipMsg), TEXT("[ %s ]分数少于(%I64d)，强行换庄!"), pIServerUserItem->GetNickName(), m_lApplyBankerCondition);
					}
					else
					{
						_sntprintf_s(szTipMsg, CountArray(szTipMsg), TEXT("[ %s ]做庄次数达到(%d)，强行换庄!"), pIServerUserItem->GetNickName(), m_lPlayerBankerMAX);
					}

					//发送消息
					SendGameMessage(INVALID_CHAIR, szTipMsg);
				}
			}
		}
	}
	//系统做庄
	else if(m_wCurrentBanker == INVALID_CHAIR && m_ApplyUserArray.GetCount() != 0)
	{
		//轮换判断
		TakeTurns();

		if(m_wCurSuperRobBankerUser != INVALID_CHAIR && m_wCurrentBanker != INVALID_CHAIR)
		{
			ASSERT(m_wCurSuperRobBankerUser == m_wCurrentBanker);
			m_wCurSuperRobBankerUser = INVALID_CHAIR;
			m_typeCurrentBanker = SUPERROB_BANKER;
		}
		else if(m_wCurSuperRobBankerUser == INVALID_CHAIR && m_wCurrentBanker != INVALID_CHAIR)
		{
			m_typeCurrentBanker = ORDINARY_BANKER;
		}

		//撤销玩家
		for(WORD i = 0; i < m_ApplyUserArray.GetCount(); ++i)
		{
			//获取玩家
			WORD wChairID = m_ApplyUserArray[i];

			//条件过滤
			if(wChairID != m_wCurrentBanker)
			{
				continue;
			}

			//删除玩家
			m_ApplyUserArray.RemoveAt(i);

			break;
		}

		bChangeBanker = true;
		m_bExchangeBanker = true;
	}

	//切换判断
	if(bChangeBanker)
	{
		//最大坐庄数
		m_lPlayerBankerMAX = m_nBankerTimeLimit;

		//设置变量
		m_wBankerTime = 0;
		m_lBankerWinScore = 0;

		//发送消息
		CMD_S_ChangeBanker stChangeBanker;
		ZeroMemory(&stChangeBanker, sizeof(stChangeBanker));
		stChangeBanker.wBankerUser = m_wCurrentBanker;
		if(m_wCurrentBanker != INVALID_CHAIR)
		{
			IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(m_wCurrentBanker);
			stChangeBanker.lBankerScore = pIServerUserItem->GetUserScore();
		}
		else
		{
			stChangeBanker.lBankerScore = 100000000;
		}

		stChangeBanker.typeCurrentBanker = m_typeCurrentBanker;
		m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_CHANGE_BANKER, &stChangeBanker, sizeof(CMD_S_ChangeBanker));
		m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_CHANGE_BANKER, &stChangeBanker, sizeof(CMD_S_ChangeBanker));

		if(m_wCurrentBanker != INVALID_CHAIR)
		{
			//获取参数
			m_pGameServiceOption = m_pITableFrame->GetGameServiceOption();
			ASSERT(m_pGameServiceOption != NULL);

			//读取消息
			int nMessageCount = 3;

			//读取配置
			INT nIndex = rand() % nMessageCount;
			TCHAR szMessage1[256], szMessage2[256];
			tagCustomConfig *pCustomConfig = (tagCustomConfig *)m_pGameServiceOption->cbCustomRule;
			ASSERT(pCustomConfig);
			if(0 == nIndex)
			{
				CopyMemory(szMessage1, pCustomConfig->CustomGeneral.szMessageItem1, sizeof(pCustomConfig->CustomGeneral.szMessageItem1));
			}
			else if(1 == nIndex)
			{
				CopyMemory(szMessage1, pCustomConfig->CustomGeneral.szMessageItem2, sizeof(pCustomConfig->CustomGeneral.szMessageItem2));
			}
			else if(2 == nIndex)
			{
				CopyMemory(szMessage1, pCustomConfig->CustomGeneral.szMessageItem3, sizeof(pCustomConfig->CustomGeneral.szMessageItem3));
			}

			//获取玩家
			IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(m_wCurrentBanker);

			//发送消息
			_sntprintf_s(szMessage2, CountArray(szMessage2), TEXT("【 %s 】上庄了，%s"), pIServerUserItem->GetNickName(), szMessage1);
			SendGameMessage(INVALID_CHAIR, szMessage2);
		}
	}

	return bChangeBanker;
}

//轮换判断
void CTableFrameSink::TakeTurns()
{
	//变量定义
	int nInvalidApply = 0;

	for(int i = 0; i < m_ApplyUserArray.GetCount(); i++)
	{
		if(m_pITableFrame->GetGameStatus() == GAME_SCENE_FREE)
		{
			//获取分数
			IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(m_ApplyUserArray[i]);
			if(pIServerUserItem != NULL)
			{
				if(pIServerUserItem->GetUserScore() >= m_lApplyBankerCondition)
				{
					m_wCurrentBanker = m_ApplyUserArray[i];
					break;
				}
				else
				{
					nInvalidApply = i + 1;

					//发送消息
					CMD_S_CancelBanker CancelBanker = {};

					//设置变量
					CancelBanker.wCancelUser = pIServerUserItem->GetChairID();

					//发送消息
					m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_CANCEL_BANKER, &CancelBanker, sizeof(CancelBanker));
					m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_CANCEL_BANKER, &CancelBanker, sizeof(CancelBanker));

					//提示消息
					TCHAR szTipMsg[128] = {};
					_sntprintf_s(szTipMsg, CountArray(szTipMsg), TEXT("由于你的金币数（%I64d）少于坐庄必须金币数（%I64d）,你无法上庄！"),
								 pIServerUserItem->GetUserScore(), m_lApplyBankerCondition);
					SendGameMessage(m_ApplyUserArray[i], szTipMsg);
				}
			}
		}
	}

	//删除玩家
	if(nInvalidApply != 0)
	{
		m_ApplyUserArray.RemoveAt(0, nInvalidApply);
	}
}

//发送庄家
void CTableFrameSink::SendApplyUser(IServerUserItem *pRcvServerUserItem)
{
	for(INT_PTR nUserIdx = 0; nUserIdx < m_ApplyUserArray.GetCount(); ++nUserIdx)
	{
		WORD wChairID = m_ApplyUserArray[nUserIdx];

		//获取玩家
		IServerUserItem *pServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
		if(!pServerUserItem)
		{
			continue;
		}

		//庄家判断
		if(pServerUserItem->GetChairID() == m_wCurrentBanker)
		{
			continue;
		}

		//构造变量
		CMD_S_ApplyBanker ApplyBanker;
		ApplyBanker.wApplyUser = wChairID;

		//发送消息
		m_pITableFrame->SendUserItemData(pRcvServerUserItem, SUB_S_APPLY_BANKER, &ApplyBanker, sizeof(ApplyBanker));
	}
}

//用户断线
bool CTableFrameSink::OnActionUserOffLine(WORD wChairID, IServerUserItem *pIServerUserItem)
{
	//离线庄家
	if(wChairID == m_wCurrentBanker && pIServerUserItem->IsAndroidUser() == false)
	{
		m_wOfflineBanker = wChairID;
	}

	if(wChairID == m_wCurSuperRobBankerUser)
	{
		m_wCurSuperRobBankerUser = INVALID_CHAIR;

		CMD_S_CurSuperRobLeave CurSuperRobLeave;
		ZeroMemory(&CurSuperRobLeave, sizeof(CurSuperRobLeave));

		//设置变量
		CurSuperRobLeave.wCurSuperRobBankerUser = m_wCurSuperRobBankerUser;

		//发送消息
		m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_CURSUPERROB_LEAVE, &CurSuperRobLeave, sizeof(CurSuperRobLeave));
		m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_CURSUPERROB_LEAVE, &CurSuperRobLeave, sizeof(CurSuperRobLeave));
	}

	//切换庄家
	if(wChairID == m_wCurrentBanker)
	{
		ChangeBanker(true);
	}

	//取消申请
	for(WORD i = 0; i < m_ApplyUserArray.GetCount(); ++i)
	{
		if(wChairID != m_ApplyUserArray[i])
		{
			continue;
		}

		//删除玩家
		m_ApplyUserArray.RemoveAt(i);

		//构造变量
		CMD_S_CancelBanker CancelBanker;
		ZeroMemory(&CancelBanker, sizeof(CancelBanker));

		//设置变量
		CancelBanker.wCancelUser = pIServerUserItem->GetChairID();

		//发送消息
		m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_CANCEL_BANKER, &CancelBanker, sizeof(CancelBanker));
		m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_CANCEL_BANKER, &CancelBanker, sizeof(CancelBanker));

		break;
	}

	return true;
}

//最大下注
LONGLONG CTableFrameSink::GetMaxPlayerScore(BYTE cbBetArea, WORD wChairID)
{
	//获取玩家
	IServerUserItem *pIMeServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
	if(NULL == pIMeServerUserItem)
	{
		return 0L;
	}

	if(cbBetArea >= AREA_MAX)
	{
		return 0L;
	}

	//已下注额
	LONGLONG lNowBet = 0l;
	for(int nAreaIndex = 0; nAreaIndex < AREA_MAX; ++nAreaIndex)
	{
		lNowBet += m_lPlayBet[wChairID][nAreaIndex];
	}

	////庄家金币
	//LONGLONG lBankerScore = 2147483647;
	//if (m_wCurrentBanker!=INVALID_CHAIR)
	//{
	//	lBankerScore=m_lBankerScore;
	//}
	LONGLONG lBankerScore = m_lBankerScore;

	//区域倍率
	BYTE cbMultiple[AREA_MAX] = {MULTIPLE_XIAN, MULTIPLE_PING, MULTIPLE_ZHUANG,
								 MULTIPLE_XIAN_TIAN, MULTIPLE_ZHUANG_TIAN, MULTIPLE_TONG_DIAN,
								 MULTIPLE_XIAN_PING, MULTIPLE_ZHUANG_PING
								};

	//区域输赢
	BYTE cbArae[4][4] = {	{ AREA_XIAN_DUI,	255,			AREA_MAX,			AREA_MAX },
		{ AREA_ZHUANG_DUI,	255,			AREA_MAX,			AREA_MAX },
		{ AREA_XIAN,		AREA_PING,		AREA_ZHUANG,		AREA_MAX },
		{ AREA_XIAN_TIAN,	AREA_TONG_DUI,	AREA_ZHUANG_TIAN,	255 }
	};

	//筹码设定
	for(int nTopL = 0; nTopL < 4; ++nTopL)
	{
		if(cbArae[0][nTopL] == AREA_MAX)
		{
			continue;
		}

		for(int nTopR = 0; nTopR < 4; ++nTopR)
		{
			if(cbArae[1][nTopR] == AREA_MAX)
			{
				continue;
			}

			for(int nCentral = 0; nCentral < 4; ++nCentral)
			{
				if(cbArae[2][nCentral] == AREA_MAX)
				{
					continue;
				}

				for(int nBottom = 0; nBottom < 4; ++nBottom)
				{
					if(cbArae[3][nBottom] == AREA_MAX)
					{
						continue;
					}

					BYTE cbWinArea[AREA_MAX] = {FALSE};

					//指定获胜区域
					if(cbArae[0][nTopL] != 255)
					{
						cbWinArea[cbArae[0][nTopL]] = TRUE;
					}

					if(cbArae[1][nTopR] != 255)
					{
						cbWinArea[cbArae[1][nTopR]] = TRUE;
					}

					if(cbArae[2][nCentral] != 255)
					{
						cbWinArea[cbArae[2][nCentral]] = TRUE;
					}

					if(cbArae[3][nBottom] != 255)
					{
						cbWinArea[cbArae[3][nBottom]] = TRUE;
					}

					//选择区域为玩家胜利，同等级的其他的区域为玩家输。以得出最大下注值
					for(int i = 0; i < 4; i++)
					{
						for(int j = 0; j < 4; j++)
						{
							if(cbArae[i][j] == cbBetArea)
							{
								for(int n = 0; n < 4; ++n)
								{
									if(cbArae[i][n] != 255 && cbArae[i][n] != AREA_MAX)
									{
										cbWinArea[cbArae[i][n]] = FALSE;
									}
								}
								cbWinArea[cbArae[i][j]] = TRUE;
							}
						}
					}

					LONGLONG lScore = m_lBankerScore;
					for(int nAreaIndex = 0; nAreaIndex < AREA_MAX; ++nAreaIndex)
					{
						if(cbWinArea[nAreaIndex] == TRUE)
						{
							lScore -= m_lAllBet[nAreaIndex] * (cbMultiple[nAreaIndex] - 1);
						}
						else if(cbWinArea[AREA_PING] == TRUE && (nAreaIndex == AREA_XIAN || nAreaIndex == AREA_ZHUANG))
						{

						}
						else
						{
							lScore += m_lAllBet[nAreaIndex];
						}
					}
					if(lBankerScore == -1)
					{
						lBankerScore = lScore;
					}
					else
					{
						lBankerScore = min(lBankerScore, lScore);
					}
				}
			}
		}
	}

	//最大下注
	LONGLONG lMaxBet = 0L;

	//最大下注
	lMaxBet = min(pIMeServerUserItem->GetUserScore() - lNowBet, m_lUserLimitScore - m_lPlayBet[wChairID][cbBetArea]);

	lMaxBet = min(lMaxBet, m_lAreaLimitScore - m_lAllBet[cbBetArea]);

	lMaxBet = min(lMaxBet, lBankerScore / (cbMultiple[cbBetArea] - 1));

	//非零限制
	ASSERT(lMaxBet >= 0);
	lMaxBet = max(lMaxBet, 0);

	//庄闲对等
	if(cbBetArea == AREA_XIAN && (m_lAllBet[AREA_ZHUANG] - m_lAllBet[AREA_XIAN]) && lMaxBet < (m_lAllBet[AREA_ZHUANG] - m_lAllBet[AREA_XIAN]))
	{
		lMaxBet = m_lAllBet[AREA_ZHUANG] - m_lAllBet[AREA_XIAN];
	}
	else if(cbBetArea == AREA_ZHUANG && (m_lAllBet[AREA_XIAN] - m_lAllBet[AREA_ZHUANG]) && lMaxBet < (m_lAllBet[AREA_XIAN] - m_lAllBet[AREA_ZHUANG]))
	{
		lMaxBet = m_lAllBet[AREA_XIAN] - m_lAllBet[AREA_ZHUANG];
	}

	return lMaxBet;
}


//计算得分
bool CTableFrameSink::CalculateScore(OUT LONGLONG &lBankerWinScore, OUT tagServerGameRecord &GameRecord, BYTE cbDispatchCount)
{
	m_bControl = false;
	if(m_pServerContro && m_pServerContro->NeedControl() && m_pServerContro->ControlResult(m_cbTableCardArray[0], m_cbCardCount))
	{
		m_bControl = true;
	}

	//计算牌点
	BYTE cbPlayerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);
	BYTE cbBankerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);

	//系统输赢
	LONGLONG lSystemScore = 0l;

	//推断玩家
	BYTE cbWinArea[AREA_MAX] = {FALSE};
	DeduceWinner(cbWinArea);

	//游戏记录
	GameRecord.cbBankerCount = cbBankerCount;
	GameRecord.cbPlayerCount = cbPlayerCount;
	GameRecord.bPlayerTwoPair = cbWinArea[AREA_XIAN_DUI] == TRUE;
	GameRecord.bBankerTwoPair = cbWinArea[AREA_ZHUANG_DUI] == TRUE;
	if(cbWinArea[AREA_TONG_DUI] == TRUE)
	{
		GameRecord.cbKingWinner = AREA_TONG_DUI;
	}
	else if(cbWinArea[AREA_XIAN_TIAN] == TRUE)
	{
		GameRecord.cbKingWinner = AREA_XIAN_TIAN;
	}
	else if(cbWinArea[AREA_ZHUANG_TIAN] == TRUE)
	{
		GameRecord.cbKingWinner = AREA_ZHUANG_TIAN;
	}


	//玩家成绩
	LONGLONG lUserLostScore[GAME_PLAYER];
	ZeroMemory(m_lPlayScore, sizeof(m_lPlayScore));
	ZeroMemory(m_lUserWinScore, sizeof(m_lUserWinScore));
	ZeroMemory(m_lUserRevenue, sizeof(m_lUserRevenue));
	ZeroMemory(lUserLostScore, sizeof(lUserLostScore));

	//区域倍率
	BYTE cbMultiple[AREA_MAX] = {MULTIPLE_XIAN, MULTIPLE_PING, MULTIPLE_ZHUANG,
								 MULTIPLE_XIAN_TIAN, MULTIPLE_ZHUANG_TIAN, MULTIPLE_TONG_DIAN,
								 MULTIPLE_XIAN_PING, MULTIPLE_ZHUANG_PING
								};

	//庄家是不是机器人
	bool bIsBankerAndroidUser = false;
	if(m_wCurrentBanker != INVALID_CHAIR)
	{
		IServerUserItem *pIBankerUserItem = m_pITableFrame->GetTableUserItem(m_wCurrentBanker);
		if(pIBankerUserItem != NULL)
		{
			bIsBankerAndroidUser = pIBankerUserItem->IsAndroidUser();
		}
	}

	//计算积分
	for(WORD wChairID = 0; wChairID < GAME_PLAYER; wChairID++)
	{
		//庄家判断
		if(m_wCurrentBanker == wChairID)
		{
			continue;
		}

		//获取用户
		IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
		if(pIServerUserItem == NULL)
		{
			continue;
		}

		bool bIsAndroidUser = pIServerUserItem->IsAndroidUser();

		for(WORD wAreaIndex = 0; wAreaIndex < AREA_MAX; ++wAreaIndex)
		{
			if(cbWinArea[wAreaIndex] == TRUE)
			{
				m_lUserWinScore[wChairID] += (m_lPlayBet[wChairID][wAreaIndex] * (cbMultiple[wAreaIndex] - 1));
				m_lPlayScore[wChairID][wAreaIndex] += (m_lPlayBet[wChairID][wAreaIndex] * (cbMultiple[wAreaIndex] - 1));
				lBankerWinScore -= (m_lPlayBet[wChairID][wAreaIndex] * (cbMultiple[wAreaIndex] - 1));

				//系统得分
				if(bIsAndroidUser)
				{
					lSystemScore += (m_lPlayBet[wChairID][wAreaIndex] * (cbMultiple[wAreaIndex] - 1));
				}
				if(m_wCurrentBanker == INVALID_CHAIR || bIsBankerAndroidUser)
				{
					lSystemScore -= (m_lPlayBet[wChairID][wAreaIndex] * (cbMultiple[wAreaIndex] - 1));
				}

				//庄家得分
				if(m_wCurrentBanker != INVALID_CHAIR && m_wCurrentBanker != wChairID)
				{
					m_lPlayScore[m_wCurrentBanker][wAreaIndex] -= (m_lPlayBet[wChairID][wAreaIndex] * (cbMultiple[wAreaIndex] - 1));
				}
			}
			else if(cbWinArea[AREA_PING] == TRUE && (wAreaIndex == AREA_XIAN || wAreaIndex == AREA_ZHUANG))
			{

			}
			else
			{
				lUserLostScore[wChairID] -= m_lPlayBet[wChairID][wAreaIndex];
				m_lPlayScore[wChairID][wAreaIndex] -= m_lPlayBet[wChairID][wAreaIndex];
				lBankerWinScore += m_lPlayBet[wChairID][wAreaIndex];

				//系统得分
				if(bIsAndroidUser)
				{
					lSystemScore -= m_lPlayBet[wChairID][wAreaIndex];
				}
				if(m_wCurrentBanker == INVALID_CHAIR || bIsBankerAndroidUser)
				{
					lSystemScore += m_lPlayBet[wChairID][wAreaIndex];
				}

				//庄家得分
				if(m_wCurrentBanker != INVALID_CHAIR && m_wCurrentBanker != wChairID)
				{
					m_lPlayScore[m_wCurrentBanker][wAreaIndex] += m_lPlayBet[wChairID][wAreaIndex];
				}
			}

		}

		//总的分数
		m_lUserWinScore[wChairID] += lUserLostScore[wChairID];

		//计算税收
		if(m_lUserWinScore[wChairID] > 0)
		{
			float fRevenuePer = float(m_pGameServiceOption->wRevenueRatio / 1000.0);
			m_lUserRevenue[wChairID]  = LONGLONG(m_lUserWinScore[wChairID] * fRevenuePer);
			m_lUserWinScore[wChairID] -= m_lUserRevenue[wChairID];
		}
	}

	//庄家成绩
	if(m_wCurrentBanker != INVALID_CHAIR)
	{
		m_lUserWinScore[m_wCurrentBanker] = lBankerWinScore;

		//计算税收
		if(0 < m_lUserWinScore[m_wCurrentBanker])
		{
			float fRevenuePer = float(m_pGameServiceOption->wRevenueRatio / 1000.0);
			m_lUserRevenue[m_wCurrentBanker]  = LONGLONG(m_lUserWinScore[m_wCurrentBanker] * fRevenuePer);
			m_lUserWinScore[m_wCurrentBanker] -= m_lUserRevenue[m_wCurrentBanker];
			lBankerWinScore = m_lUserWinScore[m_wCurrentBanker];
		}
	}

	LONGLONG lTempDeduct = 0;
	lTempDeduct = m_lStorageDeduct;

	//特殊控制
	if(m_bControl)
	{
		m_lStorageCurrent += lSystemScore;

		//扣取服务费
		bool bDeduct = NeedDeductStorage();
		lTempDeduct = bDeduct ? lTempDeduct : 0;
		m_lStorageCurrent = m_lStorageCurrent - (m_lStorageCurrent * lTempDeduct / 1000);

		return true;
	}

	//系统分值计算
	if((lSystemScore + m_lStorageCurrent) < 0l && (cbDispatchCount < 5 || cbDispatchCount >= 5 && lSystemScore < 0))
	{
		return false;
	}
	else
	{
		//大于封顶值，系统输分
		if((m_lStorageCurrent > m_lStorageMax1 && m_lStorageCurrent <= m_lStorageMax2 && rand() % 100 <= m_lStorageMul1) ||
				(m_lStorageCurrent > m_lStorageMax2 && rand() % 100 <= m_lStorageMul2))
		{
			//系统输钱
			if(lSystemScore >= 0)
			{
				if(m_nStorageCount >= 50)
				{
					m_lStorageCurrent += lSystemScore;
					//扣取服务费
					bool bDeduct = NeedDeductStorage();
					lTempDeduct = bDeduct ? lTempDeduct : 0;
					m_lStorageCurrent = m_lStorageCurrent - (m_lStorageCurrent * lTempDeduct / 1000);
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				if(m_lStorageCurrent + lSystemScore < 0)
				{
					return false;
				}
				else
				{
					m_lStorageCurrent += lSystemScore;
					//扣取服务费
					bool bDeduct = NeedDeductStorage();
					lTempDeduct = bDeduct ? lTempDeduct : 0;
					m_lStorageCurrent = m_lStorageCurrent - (m_lStorageCurrent * lTempDeduct / 1000);
					return true;
				}
			}
		}
		m_lStorageCurrent += lSystemScore;
		//扣取服务费
		bool bDeduct = NeedDeductStorage();
		lTempDeduct = bDeduct ? lTempDeduct : 0;
		m_lStorageCurrent = m_lStorageCurrent - (m_lStorageCurrent * lTempDeduct / 1000);
		return true;
	}
}

//游戏结束计算
LONGLONG CTableFrameSink::GameOver()
{
	//定义变量
	LONGLONG lBankerWinScore = 0l;
	bool bSuccess = false;

	//游戏记录
	tagServerGameRecord &GameRecord = m_GameRecordArrary[m_nRecordLast];

	//计算分数
	BYTE cbDispatchCount = 0;
	do
	{
		m_nStorageCount++;
		//派发扑克
		DispatchTableCard();

		//试探性判断
		lBankerWinScore = 0l;
		bSuccess = CalculateScore(lBankerWinScore, GameRecord, cbDispatchCount);
		cbDispatchCount ++;

	}
	while(!bSuccess);

	//累计积分
	m_lBankerWinScore += lBankerWinScore;

	//当前积分
	m_lBankerCurGameScore = lBankerWinScore;

	//移动下标
	m_nRecordLast = (m_nRecordLast + 1) % MAX_SCORE_HISTORY;
	if(m_nRecordLast == m_nRecordFirst)
	{
		m_nRecordFirst = (m_nRecordFirst + 1) % MAX_SCORE_HISTORY;
	}

	return lBankerWinScore;
}

//推断赢家
void CTableFrameSink::DeduceWinner(BYTE *pWinArea)
{
	//计算牌点
	BYTE cbPlayerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);
	BYTE cbBankerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);

	//胜利区域--------------------------
	//平
	if(cbPlayerCount == cbBankerCount)
	{
		pWinArea[AREA_PING] = TRUE;

		// 同平点
		if(m_cbCardCount[INDEX_PLAYER] == m_cbCardCount[INDEX_BANKER])
		{
			WORD wCardIndex = 0;
			for(; wCardIndex < m_cbCardCount[INDEX_PLAYER]; ++wCardIndex)
			{
				BYTE cbBankerValue = m_GameLogic.GetCardValue(m_cbTableCardArray[INDEX_BANKER][wCardIndex]);
				BYTE cbPlayerValue = m_GameLogic.GetCardValue(m_cbTableCardArray[INDEX_PLAYER][wCardIndex]);
				if(cbBankerValue != cbPlayerValue)
				{
					break;
				}
			}

			if(wCardIndex == m_cbCardCount[INDEX_PLAYER])
			{
				pWinArea[AREA_TONG_DUI] = TRUE;
			}
		}
	}
	// 庄
	else if(cbPlayerCount < cbBankerCount)
	{
		pWinArea[AREA_ZHUANG] = TRUE;

		//天王判断
		if(cbBankerCount == 8 || cbBankerCount == 9)
		{
			pWinArea[AREA_ZHUANG_TIAN] = TRUE;
		}
	}
	// 闲
	else
	{
		pWinArea[AREA_XIAN] = TRUE;

		//天王判断
		if(cbPlayerCount == 8 || cbPlayerCount == 9)
		{
			pWinArea[AREA_XIAN_TIAN] = TRUE;
		}
	}


	//对子判断
	if(m_GameLogic.GetCardValue(m_cbTableCardArray[INDEX_PLAYER][0]) == m_GameLogic.GetCardValue(m_cbTableCardArray[INDEX_PLAYER][1]))
	{
		pWinArea[AREA_XIAN_DUI] = TRUE;
	}
	if(m_GameLogic.GetCardValue(m_cbTableCardArray[INDEX_BANKER][0]) == m_GameLogic.GetCardValue(m_cbTableCardArray[INDEX_BANKER][1]))
	{
		pWinArea[AREA_ZHUANG_DUI] = TRUE;
	}
}


//发送记录
void CTableFrameSink::SendGameRecord(IServerUserItem *pIServerUserItem)
{
	WORD wBufferSize = 0;
	BYTE cbBuffer[SOCKET_TCP_BUFFER];
	int nIndex = m_nRecordFirst;
	while(nIndex != m_nRecordLast)
	{
		if((wBufferSize + sizeof(tagServerGameRecord)) > sizeof(cbBuffer))
		{
			m_pITableFrame->SendUserItemData(pIServerUserItem, SUB_S_SEND_RECORD, cbBuffer, wBufferSize);
			wBufferSize = 0;
		}
		CopyMemory(cbBuffer + wBufferSize, &m_GameRecordArrary[nIndex], sizeof(tagServerGameRecord));
		wBufferSize += sizeof(tagServerGameRecord);

		nIndex = (nIndex + 1) % MAX_SCORE_HISTORY;
	}
	if(wBufferSize > 0)
	{
		m_pITableFrame->SendUserItemData(pIServerUserItem, SUB_S_SEND_RECORD, cbBuffer, wBufferSize);
	}
}

//发送消息
void CTableFrameSink::SendGameMessage(WORD wChairID, LPCTSTR pszTipMsg)
{
	if(wChairID == INVALID_CHAIR)
	{
		//游戏玩家
		for(WORD i = 0; i < GAME_PLAYER; ++i)
		{
			IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
			if(pIServerUserItem == NULL)
			{
				continue;
			}
			m_pITableFrame->SendGameMessage(pIServerUserItem, pszTipMsg, SMT_CHAT);
		}

		//旁观玩家
		WORD wIndex = 0;
		do
		{
			IServerUserItem *pILookonServerUserItem = m_pITableFrame->EnumLookonUserItem(wIndex++);
			if(pILookonServerUserItem == NULL)
			{
				break;
			}

			m_pITableFrame->SendGameMessage(pILookonServerUserItem, pszTipMsg, SMT_CHAT);

		}
		while(true);
	}
	else
	{
		IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
		if(pIServerUserItem != NULL)
		{
			m_pITableFrame->SendGameMessage(pIServerUserItem, pszTipMsg, SMT_CHAT | SMT_EJECT);
		}
	}
}

//读取配置
void CTableFrameSink::ReadConfigInformation()
{
	//获取自定义配置
	tagCustomConfig *pCustomConfig = (tagCustomConfig *)m_pGameServiceOption->cbCustomRule;
	ASSERT(pCustomConfig);

	//上庄
	m_lApplyBankerCondition = pCustomConfig->CustomGeneral.lApplyBankerCondition;
	m_nBankerTimeLimit = pCustomConfig->CustomGeneral.lBankerTime;
	m_nBankerTimeAdd = pCustomConfig->CustomGeneral.lBankerTimeAdd;
	m_lExtraBankerScore = pCustomConfig->CustomGeneral.lBankerScoreMAX;
	m_nExtraBankerTime = pCustomConfig->CustomGeneral.lBankerTimeExtra;
	m_bEnableSysBanker = (pCustomConfig->CustomGeneral.nEnableSysBanker == TRUE) ? true : false;

	//超级抢庄
	CopyMemory(&m_superbankerConfig, &(pCustomConfig->CustomGeneral.superbankerConfig), sizeof(m_superbankerConfig));

	//占位
	CopyMemory(&m_occupyseatConfig, &(pCustomConfig->CustomGeneral.occupyseatConfig), sizeof(m_occupyseatConfig));

	//时间
	m_cbFreeTime = pCustomConfig->CustomGeneral.lFreeTime;
	m_cbBetTime = pCustomConfig->CustomGeneral.lBetTime;
	m_cbEndTime = pCustomConfig->CustomGeneral.lEndTime;
	if(m_cbFreeTime < TIME_FREE	|| m_cbFreeTime > 99)
	{
		m_cbFreeTime = TIME_FREE;
	}
	if(m_cbBetTime < TIME_PLACE_JETTON || m_cbBetTime > 99)
	{
		m_cbBetTime = TIME_PLACE_JETTON;
	}
	if(m_cbEndTime < TIME_GAME_END || m_cbEndTime > 99)
	{
		m_cbEndTime = TIME_GAME_END;
	}

	//下注
	m_lAreaLimitScore = pCustomConfig->CustomGeneral.lAreaLimitScore;
	m_lUserLimitScore = pCustomConfig->CustomGeneral.lUserLimitScore;

	//库存
	m_lStorageStart = pCustomConfig->CustomGeneral.StorageStart;
	m_lStorageCurrent = m_lStorageStart;
	m_lStorageDeduct = pCustomConfig->CustomGeneral.StorageDeduct;
	m_lStorageMax1 = pCustomConfig->CustomGeneral.StorageMax1;
	m_lStorageMul1 = pCustomConfig->CustomGeneral.StorageMul1;
	m_lStorageMax2 = pCustomConfig->CustomGeneral.StorageMax2;
	m_lStorageMul2 = pCustomConfig->CustomGeneral.StorageMul2;
	if(m_lStorageMul1 < 0 || m_lStorageMul1 > 100)
	{
		m_lStorageMul1 = 50;
	}
	if(m_lStorageMul2 < 0 || m_lStorageMul2 > 100)
	{
		m_lStorageMul2 = 80;
	}

	//机器人
	m_nRobotListMaxCount = pCustomConfig->CustomAndroid.lRobotListMaxCount;

	LONGLONG lRobotBetMinCount = pCustomConfig->CustomAndroid.lRobotBetMinCount;
	LONGLONG lRobotBetMaxCount = pCustomConfig->CustomAndroid.lRobotBetMaxCount;
	m_nMaxChipRobot = rand() % (lRobotBetMaxCount - lRobotBetMinCount + 1) + lRobotBetMinCount;
	if(m_nMaxChipRobot < 0)
	{
		m_nMaxChipRobot = 8;
	}
	m_lRobotAreaLimit = pCustomConfig->CustomAndroid.lRobotAreaLimit;

	return;
}

// 添加逗号
CString CTableFrameSink::AddComma(LONGLONG lScore)
{
	CString strScore;
	CString strReturn;
	LONGLONG lNumber = lScore;
	if(lScore < 0)
	{
		lNumber = -lNumber;
	}

	strScore.Format(TEXT("%I64d"), lNumber);

	int nStrCount = 0;
	for(int i = strScore.GetLength() - 1; i >= 0;)
	{
		if((nStrCount % 3) == 0 && nStrCount != 0)
		{
			strReturn.Insert(0, ',');
			nStrCount = 0;
		}
		else
		{
			strReturn.Insert(0, strScore.GetAt(i));
			nStrCount++;
			i--;
		}
	}

	if(lScore < 0)
	{
		strReturn.Insert(0, '-');
	}

	return strReturn;
}
//////////////////////////////////////////////////////////////////////////

bool CTableFrameSink::OnSubAmdinCommand(IServerUserItem *pIServerUserItem, const void *pDataBuffer)
{
	//如果不具有管理员权限 则返回错误
	if(!CUserRight::IsGameCheatUser(pIServerUserItem->GetUserRight()))
	{
		return false;
	}

	return true;
}

//咨询玩家
bool CTableFrameSink::OnEventQueryChargeable(IServerUserItem *pIServerUserItem, bool bLookonUser)
{
	if(bLookonUser)
	{
		return false;
	}

	return true;
}

//咨询服务费
LONGLONG CTableFrameSink::OnEventQueryCharge(IServerUserItem *pIServerUserItem)
{
	if(m_lUserWinScore[pIServerUserItem->GetChairID()] != 0)
	{
		return m_nServiceCharge;
	}

	return 0;
}

//查询是否扣服务费
bool CTableFrameSink::QueryBuckleServiceCharge(WORD wChairID)
{
	// 庄家判断
	if(wChairID == m_wCurrentBanker)
	{
		for(int i = 0; i < GAME_PLAYER; ++i)
		{
			for(int j = 0; j < AREA_MAX; ++j)
			{
				if(m_lPlayBet[i][j] != 0)
				{
					return true;
				}
			}
		}
		return false;
	}

	// 一般玩家
	for(int i = 0; i < AREA_MAX; ++i)
	{
		if(m_lPlayBet[wChairID][i] != 0)
		{
			return true;
		}
	}

	return false;
}

//积分事件
bool CTableFrameSink::OnUserScroeNotify(WORD wChairID, IServerUserItem *pIServerUserItem, BYTE cbReason)
{
	//当庄家分数在空闲时间变动时(即庄家进行了存取款)校验庄家的上庄条件
	if(wChairID == m_wCurrentBanker && m_pITableFrame->GetGameStatus() == GAME_SCENE_FREE)
	{
		ChangeBanker(false);
	}

	if(m_pITableFrame->GetGameStatus() == GAME_SCENE_BET)
	{
		CMD_S_UserScoreNotify UserScoreNotify;
		ZeroMemory(&UserScoreNotify, sizeof(UserScoreNotify));

		UserScoreNotify.wChairID = wChairID;
		UserScoreNotify.lPlayBetScore = min(pIServerUserItem->GetUserScore() - m_nServiceCharge, m_lUserLimitScore);

		m_pITableFrame->SendTableData(wChairID, SUB_S_USER_SCORE_NOTIFY, &UserScoreNotify, sizeof(UserScoreNotify));
	}

	//校验是否满足占位最低条件
	if(pIServerUserItem->GetUserScore() < m_occupyseatConfig.lForceStandUpCondition)
	{
		bool bvalid = false;
		//校验数据
		for(WORD i = 0; i < MAX_OCCUPY_SEAT_COUNT; i++)
		{
			if(m_wOccupySeatChairIDArray[i] == wChairID)
			{
				bvalid = true;

				//重置无效
				m_wOccupySeatChairIDArray[i] = INVALID_CHAIR;

				break;
			}
		}

		if(bvalid == true)
		{
			CMD_S_UpdateOccupySeat UpdateOccupySeat;
			ZeroMemory(&UpdateOccupySeat, sizeof(UpdateOccupySeat));
			CopyMemory(UpdateOccupySeat.wOccupySeatChairIDArray, m_wOccupySeatChairIDArray, sizeof(m_wOccupySeatChairIDArray));
			UpdateOccupySeat.wQuitOccupySeatChairID = wChairID;

			//发送数据
			m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_UPDATE_OCCUPYSEAT, &UpdateOccupySeat, sizeof(UpdateOccupySeat));
			m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_UPDATE_OCCUPYSEAT, &UpdateOccupySeat, sizeof(UpdateOccupySeat));
		}

	}

	return true;
}

//是否衰减
bool CTableFrameSink::NeedDeductStorage()
{

	if(m_lStorageCurrent <= 0)
	{
		return false;
	}

	for(int i = 0; i < GAME_PLAYER; ++i)
	{
		IServerUserItem *pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
		if(pIServerUserItem == NULL)
		{
			continue;
		}

		if(!pIServerUserItem->IsAndroidUser())
		{
			for(int nAreaIndex = 0; nAreaIndex <= AREA_MAX; ++nAreaIndex)
			{
				if(m_lPlayBet[i][nAreaIndex] != 0)
				{
					return true;
				}
			}
		}
	}

	return false;

}

//发送下注信息
void CTableFrameSink::SendUserBetInfo()
{
	//构造数据
	CMD_S_SendUserBetInfo SendUserBetInfo;
	ZeroMemory(&SendUserBetInfo, sizeof(SendUserBetInfo));

	CopyMemory(&SendUserBetInfo.lUserStartScore, m_lUserStartScore, sizeof(m_lUserStartScore));

	for(WORD i = 0; i < GAME_PLAYER; i++)
	{
		for(WORD wAreaIndex = 0; wAreaIndex < AREA_MAX; wAreaIndex++)
		{
			SendUserBetInfo.lUserJettonScore[i][wAreaIndex] = (LONG)m_lPlayBet[i][wAreaIndex];
		}
	}

	//发送消息
	m_pITableFrame->SendRoomData(NULL, SUB_S_SEND_USER_BET_INFO, &SendUserBetInfo, sizeof(SendUserBetInfo));

	return;
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
	// if(bOpen)
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

//获取会员等级
int CTableFrameSink::GetMemberOrderIndex(VIPINDEX vipIndex)
{
	ASSERT(vipIndex != VIP_INVALID);

	switch(vipIndex)
	{
	case VIP1_INDEX:
	{
		return 1;
	}
	case VIP2_INDEX:
	{
		return 2;
	}
	case VIP3_INDEX:
	{
		return 3;
	}
	case VIP4_INDEX:
	{
		return 4;
	}
	case VIP5_INDEX:
	{
		return 5;
	}
	default:
		return -1;
	}

	return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////