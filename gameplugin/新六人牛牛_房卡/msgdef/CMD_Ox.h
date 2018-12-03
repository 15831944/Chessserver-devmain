#ifndef CMD_OXSIXX_HEAD_FILE
#define CMD_OXSIXX_HEAD_FILE

#pragma pack(push)  
#pragma pack(1)

#define		CARD_CONFIG

//////////////////////////////////////////////////////////////////////////
//公共宏定义
#define KIND_ID							50									//游戏 I D
#define GAME_PLAYER						6									//游戏人数
#define GAME_NAME						TEXT("新六人牛牛")					//游戏名字
#define MAX_CARDCOUNT					5									//最大数目
#define MAX_CONFIG						5									//最大配置个数
#define MAX_RECORD_COUNT				32									//房卡结算统计最大局数
#define MAX_CARD_TYPE					19									//最大牌型

#define VERSION_SERVER					PROCESS_VERSION(7,0,1)				//程序版本
#define VERSION_CLIENT					PROCESS_VERSION(7,0,1)				//程序版本

//游戏状态
#define GS_TK_FREE						GAME_STATUS_FREE					//等待开始
#define GS_TK_CALL						GAME_STATUS_PLAY					//叫庄状态
#define GS_TK_SCORE						GAME_STATUS_PLAY+1					//下注状态
#define GS_TK_PLAYING					GAME_STATUS_PLAY+2					//游戏进行

#define MAX_RECORD_COUNT				32									//房卡结算统计最大局数

//命令消息
#define IDM_ADMIN_UPDATE_STORAGE		WM_USER+1001
#define IDM_ADMIN_MODIFY_STORAGE		WM_USER+1011
#define IDM_REQUEST_QUERY_USER			WM_USER+1012
#define IDM_USER_CONTROL				WM_USER+1013
#define IDM_REQUEST_UPDATE_ROOMINFO		WM_USER+1014
#define IDM_CLEAR_CURRENT_QUERYUSER		WM_USER+1015

//操作记录
#define MAX_OPERATION_RECORD			20									//操作记录条数
#define RECORD_LENGTH					128									//每条记录字长

#define INVALID_LONGLONG				((LONGLONG)(0xFFFFFFFF))			//无效数值

//游戏记录
struct CMD_S_RECORD
{
	int									nCount;
	LONGLONG							lUserWinCount[GAME_PLAYER];						//玩家胜利次数
	LONGLONG							lUserLostCount[GAME_PLAYER];					//玩家失败次数
};

//-------------------------------------------
//游戏牌型
typedef enum
{
	CT_CLASSIC_ = 22,						//经典模式
	CT_ADDTIMES_ = 23,						//疯狂加倍
	CT_INVALID_ = 255,						//无效
}CARDTYPE_CONFIG;

//发牌模式
typedef enum
{
	ST_SENDFOUR_ = 32,						//发四等五
	ST_BETFIRST_ = 33,						//下注发牌
	ST_INVALID_ = 255,						//无效
}SENDCARDTYPE_CONFIG;

//扑克玩法
typedef enum
{
	GT_HAVEKING_ = 42,						//有大小王
	GT_NOKING_ = 43,						//无大小王
	GT_INVALID_ = 255,						//无效
}KING_CONFIG;

//庄家玩法
typedef enum
{
	BGT_DESPOT_ = 52,						//霸王庄
	BGT_ROB_ = 53,							//倍数抢庄
	BGT_NIUNIU_ = 54,						//牛牛上庄
	BGT_NONIUNIU_ = 55,						//无牛下庄
	BGT_INVALID_ = 255,						//无效
}BANERGAMETYPE_CONFIG;

//下注配置
typedef enum
{
	BT_FREE_ = 62,							//自由配置额度
	BT_PENCENT_ = 63,						//百分比配置额度
	BT_INVALID_ = 255,						//无效
}BETTYPE_CONFIG;
//-------------------------------------------

//////////////////////////////////////////////////////////////////////////
//服务器命令结构
#define SUB_S_GAME_START				100									//游戏开始
#define SUB_S_ADD_SCORE					101									//加注结果
#define SUB_S_PLAYER_EXIT				102									//用户强退
#define SUB_S_SEND_CARD					103									//发牌消息
#define SUB_S_GAME_END					104									//游戏结束
#define SUB_S_OPEN_CARD					105									//用户摊牌
#define SUB_S_CALL_BANKER				106									//用户叫庄
#define SUB_S_CALL_BANKERINFO			107									//用户叫庄信息
#define SUB_S_ADMIN_STORAGE_INFO		112									//刷新控制服务端
#define SUB_S_RECORD					113									//游戏记录
#define SUB_S_ROOMCARD_RECORD			114									//房卡记录

#define SUB_S_REQUEST_QUERY_RESULT		115									//查询用户结果
#define SUB_S_USER_CONTROL				116									//用户控制
#define SUB_S_USER_CONTROL_COMPLETE		117									//用户控制完成
#define SUB_S_OPERATION_RECORD		    118									//操作记录
#define SUB_S_REQUEST_UDPATE_ROOMINFO_RESULT 119

//////////////////////////////////////////////////////////////////////////////////////

//预留机器人存款取款
struct tagCustomAndroid
{
	SCORE								lRobotScoreMin;	
	SCORE								lRobotScoreMax;
	SCORE	                            lRobotBankGet; 
	SCORE								lRobotBankGetBanker; 
	SCORE								lRobotBankStoMul; 
};

//控制类型
typedef enum{CONTINUE_WIN, CONTINUE_LOST, CONTINUE_CANCEL}CONTROL_TYPE;

//控制结果      控制成功 、控制失败 、控制取消成功 、控制取消无效
typedef enum{CONTROL_SUCCEED, CONTROL_FAIL, CONTROL_CANCEL_SUCCEED, CONTROL_CANCEL_INVALID}CONTROL_RESULT;

//用户行为
typedef enum{USER_SITDOWN = 11, USER_STANDUP, USER_OFFLINE, USER_RECONNECT}USERACTION;

//控制信息
typedef struct
{
	CONTROL_TYPE						control_type;					  //控制类型
	BYTE								cbControlCount;					  //控制局数
	bool							    bCancelControl;					  //取消标识
}USERCONTROL;

//房间用户信息
typedef struct
{
	WORD								wChairID;							//椅子ID
	WORD								wTableID;							//桌子ID
	DWORD								dwGameID;							//GAMEID
	bool								bAndroid;							//机器人标识
	TCHAR								szNickName[LEN_NICKNAME];			//用户昵称
	BYTE								cbUserStatus;						//用户状态
	BYTE								cbGameStatus;						//游戏状态
}ROOMUSERINFO;

//房间用户控制
typedef struct
{
	ROOMUSERINFO						roomUserInfo;						//房间用户信息
	USERCONTROL							userControl;						//用户控制
}ROOMUSERCONTROL;

//////////////////////////////////////////////////////////////////////////////////////

//游戏状态
struct CMD_S_StatusFree
{
	LONGLONG							lCellScore;							//基础积分
	LONGLONG							lRoomStorageStart;					//房间起始库存
	LONGLONG							lRoomStorageCurrent;				//房间当前库存

	//历史积分
	LONGLONG							lTurnScore[GAME_PLAYER];			//积分信息
	LONGLONG							lCollectScore[GAME_PLAYER];			//积分信息
	tagCustomAndroid					CustomAndroid;						//机器人配置

	bool								bIsAllowAvertCheat;					//反作弊标志
	
	CARDTYPE_CONFIG						ctConfig;							//游戏牌型
	SENDCARDTYPE_CONFIG					stConfig;							//发牌模式
	BANERGAMETYPE_CONFIG				bgtConfig;							//庄家玩法
	BETTYPE_CONFIG						btConfig;							//下注配置

	WORD								wGamePlayerCountRule;				//2-6人为0，其他的人数多少值为多少
	SCORE								lMinTableScore;						//最低积分
};

//叫庄状态
struct CMD_S_StatusCall
{
	BYTE                                cbDynamicJoin;                      //动态加入 
	BYTE                                cbPlayStatus[GAME_PLAYER];          //用户状态

	LONGLONG							lRoomStorageStart;					//房间起始库存
	LONGLONG							lRoomStorageCurrent;				//房间当前库存

	//历史积分
	LONGLONG							lTurnScore[GAME_PLAYER];			//积分信息
	LONGLONG							lCollectScore[GAME_PLAYER];			//积分信息
	tagCustomAndroid					CustomAndroid;						//机器人配置

	bool								bIsAllowAvertCheat;					//反作弊标志
	
	BYTE								cbCallBankerStatus[GAME_PLAYER];	//叫庄状态
	BYTE								cbCallBankerTimes[GAME_PLAYER];		//叫庄倍数
	CARDTYPE_CONFIG						ctConfig;							//游戏牌型
	SENDCARDTYPE_CONFIG					stConfig;							//发牌模式
	BANERGAMETYPE_CONFIG				bgtConfig;							//庄家玩法
	BETTYPE_CONFIG						btConfig;							//下注配置

	WORD								wGamePlayerCountRule;				//2-6人为0，其他的人数多少值为多少
	SCORE								lMinTableScore;						//最低积分
};

//下注状态
struct CMD_S_StatusScore
{
	//下注信息
	BYTE                                cbPlayStatus[GAME_PLAYER];          //用户状态
	BYTE                                cbDynamicJoin;                      //动态加入
	LONGLONG							lTurnMaxScore;						//最大下注
	LONGLONG							lTableScore[GAME_PLAYER];			//下注数目
	WORD								wBankerUser;						//庄家用户

	LONGLONG							lRoomStorageStart;					//房间起始库存
	LONGLONG							lRoomStorageCurrent;				//房间当前库存

	//历史积分
	LONGLONG							lTurnScore[GAME_PLAYER];			//积分信息
	LONGLONG							lCollectScore[GAME_PLAYER];			//积分信息
	tagCustomAndroid					CustomAndroid;						//机器人配置

	bool								bIsAllowAvertCheat;					//反作弊标志

	BYTE								cbCardData[GAME_PLAYER][MAX_CARDCOUNT];	//用户扑克
	CARDTYPE_CONFIG						ctConfig;							//游戏牌型
	SENDCARDTYPE_CONFIG					stConfig;							//发牌模式
	BANERGAMETYPE_CONFIG				bgtConfig;							//庄家玩法
	BETTYPE_CONFIG						btConfig;							//下注配置
	LONG								lFreeConfig[MAX_CONFIG];			//自由配置额度(无效值0)
	LONG								lPercentConfig[MAX_CONFIG];			//百分比配置额度(无效值0)

	WORD								wGamePlayerCountRule;				//2-6人为0，其他的人数多少值为多少
	SCORE								lMinTableScore;						//最低积分
};

//游戏状态
struct CMD_S_StatusPlay
{
	//状态信息
	BYTE                                cbPlayStatus[GAME_PLAYER];          //用户状态
	BYTE                                cbDynamicJoin;                      //动态加入
	LONGLONG							lTurnMaxScore;						//最大下注
	LONGLONG							lTableScore[GAME_PLAYER];			//下注数目
	WORD								wBankerUser;						//庄家用户

	LONGLONG							lRoomStorageStart;					//房间起始库存
	LONGLONG							lRoomStorageCurrent;				//房间当前库存

	//扑克信息
	BYTE								cbHandCardData[GAME_PLAYER][MAX_CARDCOUNT];//桌面扑克
	bool								bOpenCard[GAME_PLAYER];				//开牌标识
	bool								bSpecialCard[GAME_PLAYER];			//特殊牌型标志 （四花牛，五花牛，顺子，同花，葫芦，炸弹，同花顺，五小牛）
	BYTE								cbOriginalCardType[GAME_PLAYER];	//玩家原始牌型（没有经过玩家组合的牌型）
	BYTE								cbCombineCardType[GAME_PLAYER];     //玩家组合牌型（经过玩家组合的牌型）

	//历史积分
	LONGLONG							lTurnScore[GAME_PLAYER];			//积分信息
	LONGLONG							lCollectScore[GAME_PLAYER];			//积分信息
	tagCustomAndroid					CustomAndroid;						//机器人配置
	bool								bIsAllowAvertCheat;					//反作弊标志
	
	CARDTYPE_CONFIG						ctConfig;							//游戏牌型
	SENDCARDTYPE_CONFIG					stConfig;							//发牌模式
	BANERGAMETYPE_CONFIG				bgtConfig;							//庄家玩法
	BETTYPE_CONFIG						btConfig;							//下注配置
	LONG								lFreeConfig[MAX_CONFIG];			//自由配置额度(无效值0)
	LONG								lPercentConfig[MAX_CONFIG];			//百分比配置额度(无效值0)

	WORD								wGamePlayerCountRule;				//2-6人为0，其他的人数多少值为多少
	SCORE								lMinTableScore;						//最低积分
};

//用户叫庄信息
struct CMD_S_CallBankerInfo
{
	BYTE								cbCallBankerStatus[GAME_PLAYER];	//叫庄状态
	BYTE								cbCallBankerTimes[GAME_PLAYER];		//叫庄倍数(若用户不叫庄，赋值0)
};

//游戏开始
struct CMD_S_GameStart
{
	WORD								wBankerUser;						//庄家用户
	BYTE								cbPlayerStatus[GAME_PLAYER];		//玩家状态
	LONGLONG							lTurnMaxScore;						//最大下注

	//(发牌模式如果为发四等五，则发四张牌， 否则全为0)
	BYTE								cbCardData[GAME_PLAYER][MAX_CARDCOUNT];	//用户扑克
	SENDCARDTYPE_CONFIG					stConfig;							//发牌模式
	BANERGAMETYPE_CONFIG				bgtConfig;							//庄家玩法
	BETTYPE_CONFIG						btConfig;							//下注配置
	LONG								lFreeConfig[MAX_CONFIG];			//自由配置额度(无效值0)
	LONG								lPercentConfig[MAX_CONFIG];			//百分比配置额度(无效值0)
};

//用户下注
struct CMD_S_AddScore
{
	WORD								wAddScoreUser;						//加注用户
	LONGLONG							lAddScoreCount;						//加注数目
};

//游戏结束
struct CMD_S_GameEnd
{
	LONGLONG							lGameTax[GAME_PLAYER];				//游戏税收
	LONGLONG							lGameScore[GAME_PLAYER];			//游戏得分
	BYTE								cbHandCardData[GAME_PLAYER][MAX_CARDCOUNT];//桌面扑克
	BYTE								cbCardType[GAME_PLAYER];			//玩家牌型
	BYTE								cbDelayOverGame;
};

//发牌数据包
struct CMD_S_SendCard
{
	//(发全部5张牌，如果发牌模式是发四等五，则前面四张和CMD_S_GameStart消息一样)
	BYTE								cbCardData[GAME_PLAYER][MAX_CARDCOUNT];	//用户扑克
	bool								bSpecialCard[GAME_PLAYER];				//特殊牌型标志 （四花牛，五花牛，顺子，同花，葫芦，炸弹，同花顺，五小牛）
	BYTE								cbOriginalCardType[GAME_PLAYER];		//玩家原始牌型（没有经过玩家组合的牌型）
};

//用户退出
struct CMD_S_PlayerExit
{
	WORD								wPlayerID;							//退出用户
};

//用户摊牌
struct CMD_S_Open_Card
{
	WORD								wOpenChairID;						//摊牌用户
	BYTE								bOpenCard;							//摊牌标志
};

struct CMD_S_RequestQueryResult
{
	ROOMUSERINFO						userinfo;							//用户信息
	bool								bFind;								//找到标识
};

//用户控制
struct CMD_S_UserControl
{
	DWORD								dwGameID;							//GAMEID
	TCHAR								szNickName[LEN_NICKNAME];			//用户昵称
	CONTROL_RESULT						controlResult;						//控制结果
	CONTROL_TYPE						controlType;						//控制类型
	BYTE								cbControlCount;						//控制局数
};

//用户控制
struct CMD_S_UserControlComplete
{
	DWORD								dwGameID;							//GAMEID
	TCHAR								szNickName[LEN_NICKNAME];			//用户昵称
	CONTROL_TYPE						controlType;						//控制类型
	BYTE								cbRemainControlCount;				//剩余控制局数
};

//控制服务端库存信息
struct CMD_S_ADMIN_STORAGE_INFO
{
	LONGLONG							lRoomStorageStart;						//房间起始库存
	LONGLONG							lRoomStorageCurrent;
	LONGLONG							lRoomStorageDeduct;
	LONGLONG							lMaxRoomStorage[2];
	WORD								wRoomStorageMul[2];
};

//操作记录
struct CMD_S_Operation_Record
{
	TCHAR								szRecord[MAX_OPERATION_RECORD][RECORD_LENGTH];					//记录最新操作的20条记录
};

//请求更新结果
struct CMD_S_RequestUpdateRoomInfo_Result
{
	LONGLONG							lRoomStorageCurrent;				//房间当前库存
	ROOMUSERINFO						currentqueryuserinfo;				//当前查询用户信息
	bool								bExistControl;						//查询用户存在控制标识
	USERCONTROL							currentusercontrol;
};

//录像数据
struct Video_GameStart
{
	WORD								wPlayerCount;						//真实在玩人数
	WORD								wGamePlayerCountRule;				//2-6人为0，其他的人数多少值为多少
	WORD								wBankerUser;						//庄家用户
	BYTE								cbPlayerStatus[GAME_PLAYER];		//玩家状态
	LONGLONG							lTurnMaxScore;						//最大下注

	//(发牌模式如果为发四等五，则发四张牌， 否则全为0)
	BYTE								cbCardData[GAME_PLAYER][MAX_CARDCOUNT];	//用户扑克
	CARDTYPE_CONFIG						ctConfig;							//游戏牌型
	SENDCARDTYPE_CONFIG					stConfig;							//发牌模式
	BANERGAMETYPE_CONFIG				bgtConfig;							//庄家玩法
	BETTYPE_CONFIG						btConfig;							//下注配置
	DWORD								lFreeConfig[MAX_CONFIG];			//自由配置额度(无效值0)
	DWORD								lPercentConfig[MAX_CONFIG];			//百分比配置额度(无效值0)
	TCHAR								szNickName[LEN_NICKNAME];			//用户昵称		
	WORD								wChairID;							//椅子ID
	LONGLONG							lScore;								//积分
};

//房卡记录
struct CMD_S_RoomCardRecord
{
	WORD							nCount;											//局数
	LONGLONG						lDetailScore[GAME_PLAYER][MAX_RECORD_COUNT];	//单局结算分
	BYTE                            cbPlayStatus[GAME_PLAYER];						//用户状态
};

////////用于回放
//用户叫庄
struct CMD_S_CallBanker
{
	WORD								wGamePlayerCountRule;				//2-6人为0，其他的人数多少值为多少
	CARDTYPE_CONFIG						ctConfig;							//游戏牌型
	SENDCARDTYPE_CONFIG					stConfig;							//发牌模式
	BANERGAMETYPE_CONFIG				bgtConfig;							//庄家玩法
};

//////////////////////////////////////////////////////////////////////////
//客户端命令结构
#define SUB_C_CALL_BANKER				1									//用户叫庄
#define SUB_C_ADD_SCORE					2									//用户加注
#define SUB_C_OPEN_CARD					3									//用户摊牌
#define SUB_C_STORAGE					6									//更新库存
#define	SUB_C_STORAGEMAXMUL				7									//设置上限

#ifdef CARD_CONFIG
#define SUB_C_CARD_CONFIG				10									//配牌
#endif

#define SUB_C_REQUEST_RCRecord			12									//查询房卡记录

#define SUB_C_REQUEST_QUERY_USER		13									//请求查询用户
#define SUB_C_USER_CONTROL				14									//用户控制

//请求更新命令
#define SUB_C_REQUEST_UDPATE_ROOMINFO	15									//请求更新房间信息
#define SUB_C_CLEAR_CURRENT_QUERYUSER	16


//用户叫庄
struct CMD_C_CallBanker
{
	bool								bBanker;							//叫庄标志
	BYTE								cbBankerTimes;						//叫庄倍数(若用户不叫庄，赋值0)
};

//用户加注
struct CMD_C_AddScore
{
	LONGLONG							lScore;								//加注数目
};

//用户摊牌
struct CMD_C_OpenCard
{	
	//按照组合的牌，看前三张能否凑成10相关
	BYTE								cbCombineCardData[MAX_CARDCOUNT];	//玩家组合扑克
};

struct CMD_C_UpdateStorage
{
	LONGLONG							lRoomStorageCurrent;				//库存数值
	LONGLONG							lRoomStorageDeduct;					//库存数值
};

struct CMD_C_ModifyStorage
{
	LONGLONG							lMaxRoomStorage[2];					//库存上限
	WORD								wRoomStorageMul[2];					//赢分概率
};

#ifdef CARD_CONFIG
struct CMD_C_CardConfig
{
	BYTE								cbconfigCard[GAME_PLAYER][MAX_CARDCOUNT];	//配牌扑克
};
#endif

struct CMD_C_RequestQuery_User
{
	DWORD								dwGameID;								//查询用户GAMEID
	TCHAR								szNickName[LEN_NICKNAME];			    //查询用户昵称
};

//用户控制
struct CMD_C_UserControl
{
	DWORD								dwGameID;							//GAMEID
	TCHAR								szNickName[LEN_NICKNAME];			//用户昵称
	USERCONTROL							userControlInfo;					//
};

#pragma pack(pop)

#endif
