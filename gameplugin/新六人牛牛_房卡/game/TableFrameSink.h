#ifndef TABLE_FRAME_SINK_HEAD_FILE
#define TABLE_FRAME_SINK_HEAD_FILE

#pragma once

#include "Stdafx.h"
#include "GameLogic.h"
#include "HistoryScore.h"
#include "DlgCustomRule.h"
#include "ServerControl.h"
#include "GameVideo.h"

//////////////////////////////////////////////////////////////////////////


static CString GetFileDialogPath()
{
	CString strFileDlgPath;
	TCHAR szModuleDirectory[MAX_PATH];	//模块目录
	GetModuleFileName(AfxGetInstanceHandle(),szModuleDirectory,sizeof(szModuleDirectory));
	int nModuleLen=lstrlen(szModuleDirectory);
	int nProcessLen=lstrlen(AfxGetApp()->m_pszExeName)+lstrlen(TEXT(".EXE")) + 1;
	if (nModuleLen<=nProcessLen) 
		return TEXT("");
	szModuleDirectory[nModuleLen-nProcessLen]=0;
	strFileDlgPath = szModuleDirectory;
	return strFileDlgPath;
}

static void NcaTextOut(CString strInfo, CString strRoomName)
{
	CString strName;
	strName.Format(TEXT("%s\\OxSixX %s.log"), GetFileDialogPath(), strRoomName);

	CTime time = CTime::GetCurrentTime() ;
	CString strTime ;
	strTime.Format( TEXT( "%d-%d-%d %d:%d:%d" ) ,
		time.GetYear() ,
		time.GetMonth() ,
		time.GetDay() ,
		time.GetHour() ,
		time.GetMinute() ,
		time.GetSecond()
		);
	CString strMsg;
	strMsg.Format(TEXT("%s,%s\r\n"),strTime,strInfo);

	CFile fLog;
	if(fLog.Open( strName, CFile::modeCreate|CFile::modeNoTruncate|CFile::modeReadWrite ))
	{

		fLog.SeekToEnd(); 	
		int strLength=strMsg.GetLength();
#ifdef _UNICODE
		BYTE bom[2] = {0xff, 0xfe};
		fLog.Write(bom,sizeof(BYTE)*2);
		strLength*=2;
#endif
		fLog.Write((LPCTSTR)strMsg,strLength);
		fLog.Close();
	}

}

//游戏桌子类
class CTableFrameSink : public ITableFrameSink, public ITableUserAction
{
	//游戏变量
protected:
	WORD							m_wBankerUser;							//庄家用户
	WORD							m_wFirstEnterUser;						//首进玩家CHAIRID (对霸王庄有效)
	LONGLONG						m_lExitScore;							//强退分数

	//用户数据
protected:
	BYTE                            m_cbDynamicJoin[GAME_PLAYER];           //动态加入
	BYTE							m_cbPlayStatus[GAME_PLAYER];			//游戏状态
	BYTE							m_cbCallBankerStatus[GAME_PLAYER];		//叫庄状态
	BYTE							m_cbCallBankerTimes[GAME_PLAYER];		//叫庄倍数

	bool							m_bOpenCard[GAME_PLAYER];				//开牌标识
	LONGLONG						m_lTableScore[GAME_PLAYER];				//下注数目
	bool							m_bBuckleServiceCharge[GAME_PLAYER];	//收服务费

	//扑克变量
protected:
	BYTE							m_cbHandCardData[GAME_PLAYER][MAX_CARDCOUNT];//桌面扑克
	bool							m_bSpecialCard[GAME_PLAYER];			//特殊牌型标志 （四花牛，五花牛，顺子，同花，葫芦，炸弹，同花顺，五小牛）
	BYTE							m_cbOriginalCardType[GAME_PLAYER];		//玩家原始牌型（没有经过玩家组合的牌型）
	BYTE							m_cbCombineCardType[GAME_PLAYER];       //玩家组合牌型（经过玩家组合的牌型）

	//下注信息
protected:
	LONGLONG						m_lTurnMaxScore[GAME_PLAYER];			//最大下注

	//组件变量
protected:
	CGameLogic						m_GameLogic;							//游戏逻辑
	ITableFrame						* m_pITableFrame;						//框架接口
	CHistoryScore					m_HistoryScore;							//历史成绩
	tagGameServiceOption		    *m_pGameServiceOption;					//配置参数
	tagGameServiceAttrib			*m_pGameServiceAttrib;					//游戏属性

	//属性变量
protected:
	WORD							m_wPlayerCount;							//游戏人数
	
	//游戏规则
protected:
	CARDTYPE_CONFIG					m_ctConfig;
	SENDCARDTYPE_CONFIG				m_stConfig;
	KING_CONFIG						m_gtConfig;
	BANERGAMETYPE_CONFIG			m_bgtConfig;
	BETTYPE_CONFIG					m_btConfig;

	//自由配置额度(无效值0)
	LONG							m_lFreeConfig[MAX_CONFIG];

	//百分比配置额度(无效值0)
	LONG							m_lPercentConfig[MAX_CONFIG];

	LONG							m_lMaxCardTimes;						//牌型最大倍数
	
	CMD_S_RECORD					m_stRecord;								//游戏记录

	CMD_S_RoomCardRecord			m_RoomCardRecord;						//房卡游戏记录

#ifdef CARD_CONFIG
	BYTE							m_cbconfigCard[GAME_PLAYER][MAX_CARDCOUNT];	//桌面扑克
#endif

	//服务控制
protected:
	HINSTANCE						m_hInst;								//控制句柄
	IServerControl*					m_pServerControl;						//控制组件

	//游戏视频
protected:
	HINSTANCE						m_hVideoInst;
	IGameVideo*						m_pGameVideo;

	//函数定义
public:
	//构造函数
	CTableFrameSink();
	//析构函数
	virtual ~CTableFrameSink();

	//基础接口
public:
	//释放对象
	virtual VOID Release() {}
	//接口查询
	virtual void * QueryInterface(const IID & Guid, DWORD dwQueryVer);

	//管理接口
public:
	//初始化
	virtual bool Initialization(IUnknownEx * pIUnknownEx);
	//复位桌子
	virtual void RepositionSink();
	//游戏事件
public:
	//游戏开始
	virtual bool OnEventGameStart();
	//游戏结束
	virtual bool OnEventGameConclude(WORD wChairID, IServerUserItem * pIServerUserItem, BYTE cbReason);
	//发送场景
	virtual bool OnEventSendGameScene(WORD wChairID, IServerUserItem * pIServerUserItem, BYTE bGameStatus, bool bSendSecret);


	//事件接口
public:
	//定时器事件
	virtual bool OnTimerMessage(DWORD dwTimerID, WPARAM wBindParam);
	//游戏消息处理
	virtual bool OnGameMessage(WORD wSubCmdID, void * pDataBuffer, WORD wDataSize, IServerUserItem * pIServerUserItem);
	//框架消息处理
	virtual bool OnFrameMessage(WORD wSubCmdID, void * pDataBuffer, WORD wDataSize, IServerUserItem * pIServerUserItem);
	//数据事件
	virtual bool OnGameDataBase(WORD wRequestID, VOID * pData, WORD wDataSize){return true;}

	//查询接口
public:
	//查询限额
	virtual SCORE QueryConsumeQuota(IServerUserItem * pIServerUserItem);
	//最少积分
	virtual SCORE QueryLessEnterScore(WORD wChairID, IServerUserItem * pIServerUserItem){return 0;}
	//查询服务费
	virtual bool QueryBuckleServiceCharge(WORD wChairID);
	//数据事件
	virtual bool OnDataBaseMessage(WORD wRequestID, VOID * pData, WORD wDataSize){return false;}
	//积分事件
	virtual bool OnUserScroeNotify(WORD wChairID, IServerUserItem * pIServerUserItem, BYTE cbReason){return false;}
	//设置基数
	virtual void SetGameBaseScore(LONG lBaseScore){return;}


	//用户事件
public:
	//用户断线
	virtual bool OnActionUserOffLine(WORD wChairID, IServerUserItem * pIServerUserItem);
	//用户重入
	virtual bool OnActionUserConnect(WORD wChairID, IServerUserItem * pIServerUserItem) { return true; };
	//用户坐下
	virtual bool OnActionUserSitDown(WORD wChairID, IServerUserItem * pIServerUserItem, bool bLookonUser);
	//用户起立
	virtual bool OnActionUserStandUp(WORD wChairID, IServerUserItem * pIServerUserItem, bool bLookonUser);
	//用户同意
	virtual bool OnActionUserOnReady(WORD wChairID, IServerUserItem * pIServerUserItem, VOID * pData, WORD wDataSize);

	//游戏事件
protected:
	//叫庄事件
	bool OnUserCallBanker(WORD wChairID, bool bBanker, BYTE cbBankerTimes);
	//加注事件
	bool OnUserAddScore(WORD wChairID, LONGLONG lScore);
	//摊牌事件
	bool OnUserOpenCard(WORD wChairID, BYTE cbCombineCardData[MAX_CARDCOUNT]);
	//写分函数
	bool TryWriteTableScore(tagScoreInfo ScoreInfoArray[]);

	//功能函数
protected:
	//扑克分析
	void AnalyseCard(SENDCARDTYPE_CONFIG stConfig);
	//最大下分
	SCORE GetUserMaxTurnScore(WORD wChairID);
	//判断库存
	bool JudgeStock();
	//是否衰减
	bool NeedDeductStorage();
	//读取配置
	void ReadConfigInformation();
	//更新房间用户信息
	void UpdateRoomUserInfo(IServerUserItem *pIServerUserItem, USERACTION userAction);
	//更新同桌用户控制
	void UpdateUserControl(IServerUserItem *pIServerUserItem);
	//除重用户控制
	void TravelControlList(ROOMUSERCONTROL keyroomusercontrol);
	//是否满足控制条件
	void IsSatisfyControl(ROOMUSERINFO &userInfo, bool &bEnableControl);
	//分析房间用户控制
	bool AnalyseRoomUserControl(ROOMUSERCONTROL &Keyroomusercontrol, POSITION &ptList);
	//获取控制类型
	void GetControlTypeString(CONTROL_TYPE &controlType, CString &controlTypestr);
	//写日志文件
	void WriteInfo(LPCTSTR pszString);
	// 记录函数
	void WriteInfo(LPCTSTR pszFileName, LPCTSTR pszString);
	//判断房卡房间
	bool IsRoomCardType();
};

//////////////////////////////////////////////////////////////////////////

#endif
