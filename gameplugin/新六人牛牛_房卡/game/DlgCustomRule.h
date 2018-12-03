#ifndef GAME_DLG_CUSTOM_RULE_HEAD_FILE
#define GAME_DLG_CUSTOM_RULE_HEAD_FILE

#pragma once

#include "Stdafx.h"

//////////////////////////////////////////////////////////////////////////////////

//配置结构
struct tagCustomRule
{
	SCORE									lRoomStorageStart;			//库存起始值
	SCORE									lRoomStorageDeduct;			//衰减值
	SCORE									lRoomStorageMax1;			//库存封顶值1
	SCORE									lRoomStorageMul1;			//赢分百分比1
	SCORE									lRoomStorageMax2;			//库存封顶值1
	SCORE									lRoomStorageMul2;			//赢分百分比1

	//机器人存款取款
	SCORE									lRobotScoreMin;	
	SCORE									lRobotScoreMax;
	SCORE	                                lRobotBankGet; 
	SCORE									lRobotBankGetBanker; 
	SCORE									lRobotBankStoMul; 

	//游戏规则
	CARDTYPE_CONFIG							ctConfig;
	SENDCARDTYPE_CONFIG						stConfig;
	KING_CONFIG								gtConfig;
	BANERGAMETYPE_CONFIG					bgtConfig;
	BETTYPE_CONFIG							btConfig;
	
	//自由配置额度(无效值0)
	LONG									lFreeConfig[MAX_CONFIG];

	//百分比配置额度(无效值0)
	LONG									lPercentConfig[MAX_CONFIG];
};

//////////////////////////////////////////////////////////////////////////////////

//配置窗口
class CDlgCustomRule : public CDialog
{
	//配置变量
protected:
	tagCustomRule					m_CustomRule;						//配置结构

	//函数定义
public:
	//构造函数
	CDlgCustomRule();
	//析构函数
	virtual ~CDlgCustomRule();

	//重载函数
protected:
	//配置函数
	virtual BOOL OnInitDialog();
	//确定函数
	virtual VOID OnOK();
	//取消消息
	virtual VOID OnCancel();

	//功能函数
public:
	//更新控件
	bool FillDataToControl();
	//更新数据
	bool FillControlToData();

	//配置函数
public:
	//读取配置
	bool GetCustomRule(tagCustomRule & CustomRule);
	//设置配置
	bool SetCustomRule(tagCustomRule & CustomRule);
	
	afx_msg void OnClickBTFree();

	afx_msg void OnClickBTPercent();

	DECLARE_MESSAGE_MAP()
};

//////////////////////////////////////////////////////////////////////////////////

#endif