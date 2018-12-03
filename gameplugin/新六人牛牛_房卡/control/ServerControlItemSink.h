#pragma once
#include "../game/ServerControl.h"
#include "../game/GameLogic.h"


class CServerControlItemSink : public IServerControl
{

	CGameLogic						m_GameLogic;							//游戏逻辑

public:
	CServerControlItemSink(void);
	virtual ~CServerControlItemSink(void);

public:
	//返回控制区域
	virtual bool __cdecl ControlResult(BYTE cbControlCardData[GAME_PLAYER][MAX_CARDCOUNT], ROOMUSERCONTROL Keyroomusercontrol, SENDCARDTYPE_CONFIG stConfig, CARDTYPE_CONFIG ctConfig, KING_CONFIG gtConfig);

};
