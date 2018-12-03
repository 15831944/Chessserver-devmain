#ifndef CMD_LK_FISH_HEAD_FILE
#define CMD_LK_FISH_HEAD_FILE

#pragma pack(1)

#define SUB_S_SYNCHRONOUS			101	// 同步信息101
#define SUB_S_FISH_CREATE			102	// 鱼创建102
#define SUB_S_FISH_CATCH			103	// 捕获鱼103
#define SUB_S_FIRE						104	// 开火104
#define SUB_S_EXCHANGE_SCENE	105	// 转换场景105
#define SUB_S_OVER						106	// 结算106
#define SUB_S_DELAY_BEGIN			107	// 延迟107
#define SUB_S_DELAY						108	// 延迟108
#define SUB_S_BEGIN_LASER			109	// 准备激光109
#define SUB_S_LASER						110	// 激光110
#define SUB_S_BANK_TAKE				111	// 银行取款111
#define SUB_S_SPEECH					112	// 语音消息112
#define SUB_S_SYSTEM					113	// 系统消息113
#define SUB_S_MULTIPLE				114	// 倍数消息114
#define SUB_S_SUPPLY_TIP			115	// 补给提示115
#define SUB_S_SUPPLY					116	// 补给消息116
#define SUB_S_AWARD_TIP				117	// 分数提示117
#define SUB_S_CONTROL					118	// 控制消息118
#define SUB_S_UPDATE_GAME			119	// 更新游戏119
#define SUB_S_STAY_FISH				120	// 停留鱼120

struct CMD_S_AwardTip {
	WORD wTableID;
	WORD wChairID;
	TCHAR szPlayName[32];
	BYTE nFishType;
	int nFishMultiple;
	LONGLONG lFishScore;
	int nScoreType;
};

#endif
