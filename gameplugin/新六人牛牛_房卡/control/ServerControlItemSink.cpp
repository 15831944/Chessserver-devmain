#include "StdAfx.h"
#include "servercontrolitemsink.h"

//////////////////////////////////////////////////////////////////////////

//
CServerControlItemSink::CServerControlItemSink(void)
{
}

CServerControlItemSink::~CServerControlItemSink( void )
{

}

//返回控制区域
bool __cdecl CServerControlItemSink::ControlResult(BYTE cbControlCardData[GAME_PLAYER][MAX_CARDCOUNT], ROOMUSERCONTROL Keyroomusercontrol, SENDCARDTYPE_CONFIG stConfig, CARDTYPE_CONFIG ctConfig, KING_CONFIG gtConfig)
{
	ASSERT(Keyroomusercontrol.roomUserInfo.wChairID < GAME_PLAYER);

	//变量定义
	BYTE bCardData[MAX_CARDCOUNT];
	BYTE bHandCardData[GAME_PLAYER][MAX_CARDCOUNT];
	ZeroMemory(bCardData, sizeof(bCardData));
	ZeroMemory(bHandCardData, sizeof(bHandCardData));

	if (stConfig == ST_BETFIRST_)
	{
		//最大玩家
		BYTE bMaxUser = INVALID_BYTE;
		//最小玩家
		BYTE bMinUser = INVALID_BYTE;

		//控制胜利
		if (Keyroomusercontrol.userControl.control_type == CONTINUE_WIN)
		{
			for(BYTE i=0;i<GAME_PLAYER;i++)
			{
				CopyMemory(bHandCardData[i], cbControlCardData[i], sizeof(BYTE)*MAX_CARDCOUNT);

				if(INVALID_BYTE == bMaxUser)
				{
					if(cbControlCardData[i][0] != 0)
					{
						bMaxUser = i;
						CopyMemory(bCardData,cbControlCardData[i],sizeof(bCardData));
					}
				}
			}

			//最大牌型
			BYTE i = bMaxUser + 1;
			for(;i<GAME_PLAYER;i++)
			{
				if(cbControlCardData[i][0] == 0)continue;

				bool bFistOx=m_GameLogic.GetOxCard(bCardData,MAX_CARDCOUNT);
				bool bNextOx=m_GameLogic.GetOxCard(cbControlCardData[i],MAX_CARDCOUNT);

				if(m_GameLogic.CompareCard(bCardData,cbControlCardData[i],MAX_CARDCOUNT,ctConfig) == false)
				{
					CopyMemory(bCardData,cbControlCardData[i],sizeof(bCardData));
					bMaxUser=i;
				}
			}

			CopyMemory(cbControlCardData[Keyroomusercontrol.roomUserInfo.wChairID], bCardData, sizeof(bCardData));

			if(Keyroomusercontrol.roomUserInfo.wChairID != bMaxUser)
			{
				CopyMemory(cbControlCardData[bMaxUser], bHandCardData[Keyroomusercontrol.roomUserInfo.wChairID], sizeof(bHandCardData[i]));
			}

			return true;
		}
		else if (Keyroomusercontrol.userControl.control_type == CONTINUE_LOST)
		{	
			for(BYTE i=0;i<GAME_PLAYER;i++)
			{
				CopyMemory(bHandCardData[i], cbControlCardData[i], sizeof(BYTE)*MAX_CARDCOUNT);

				if(INVALID_BYTE == bMinUser)
				{
					if(cbControlCardData[i][0] != 0)
					{
						bMinUser = i;
						CopyMemory(bCardData,cbControlCardData[i],sizeof(bCardData));
					}
				}
			}

			//最小牌型
			BYTE i = bMinUser + 1;
			for(;i<GAME_PLAYER;i++)
			{
				if(cbControlCardData[i][0] == 0)continue;

				bool bFistOx=m_GameLogic.GetOxCard(cbControlCardData[i],MAX_CARDCOUNT);
				bool bNextOx=m_GameLogic.GetOxCard(bCardData,MAX_CARDCOUNT);

				if(m_GameLogic.CompareCard(cbControlCardData[i],bCardData,MAX_CARDCOUNT,ctConfig) == false)
				{
					CopyMemory(bCardData,cbControlCardData[i],sizeof(bCardData));
					bMinUser=i;
				}
			}

			CopyMemory(cbControlCardData[Keyroomusercontrol.roomUserInfo.wChairID], bCardData, sizeof(bCardData));

			if(Keyroomusercontrol.roomUserInfo.wChairID != bMinUser)
			{
				CopyMemory(cbControlCardData[bMinUser], bHandCardData[Keyroomusercontrol.roomUserInfo.wChairID], sizeof(bHandCardData[i]));
			}

			return true;
		}
	}
	else if (stConfig == ST_SENDFOUR_)
	{
		//扑克链表
		CList<BYTE, BYTE&> cardlist;
		cardlist.RemoveAll();

		//含大小王
		if (gtConfig == GT_HAVEKING_)
		{
			for (WORD i=0; i<54; i++)
			{
				cardlist.AddTail(m_GameLogic.m_cbCardListDataHaveKing[i]);
			}
		}
		else if (gtConfig == GT_NOKING_)
		{
			for (WORD i=0; i<52; i++)
			{
				cardlist.AddTail(m_GameLogic. m_cbCardListDataNoKing[i]);
			}
		}

		//删除扑克 （删除前面4张，构造后面一张）
		for (WORD i=0; i<GAME_PLAYER; i++)
		{
			for (WORD j=0; j<MAX_CARDCOUNT-1; j++)
			{
				if (cbControlCardData[i][j] != 0)
				{
					POSITION ptListHead = cardlist.GetHeadPosition();
					POSITION ptTemp;
					BYTE cbCardData = INVALID_BYTE;

					//遍历链表
					while(ptListHead)
					{
						ptTemp = ptListHead;
						if (cardlist.GetNext(ptListHead) == cbControlCardData[i][j])
						{
							cardlist.RemoveAt(ptTemp);
							break;
						}
					}
				}
			}
		}

		//控制胜利
		if (Keyroomusercontrol.userControl.control_type == CONTINUE_WIN)
		{
			//胜利玩家 从牛牛顺序开始构造，
			for (WORD wCardTypeIndex=CT_CLASSIC_OX_VALUENIUNIU; wCardTypeIndex>CT_CLASSIC_OX_VALUE0; wCardTypeIndex--)
			{
				BYTE cbKeyCardData = m_GameLogic.ConstructCardType(cardlist, cbControlCardData[Keyroomusercontrol.roomUserInfo.wChairID], wCardTypeIndex, gtConfig);
				if (cbKeyCardData == INVALID_BYTE)
				{
					continue;
				}
				else
				{
					cbControlCardData[Keyroomusercontrol.roomUserInfo.wChairID][4] = cbKeyCardData;
					break;
				}
			}

			//其他玩家构造 从无牛开始构造
			for (WORD i=0; i<GAME_PLAYER; i++)
			{
				if (cbControlCardData[i][0] != 0 && i != Keyroomusercontrol.roomUserInfo.wChairID)
				{
					//构造无牛到牛九
					for (WORD wCardTypeIndex=CT_CLASSIC_OX_VALUE0; wCardTypeIndex<CT_CLASSIC_OX_VALUENIUNIU; wCardTypeIndex++)
					{
						BYTE cbKeyCardData = m_GameLogic.ConstructCardType(cardlist, cbControlCardData[i], wCardTypeIndex, gtConfig);
						if (cbKeyCardData == INVALID_BYTE)
						{
							continue;
						}
						else
						{
							cbControlCardData[i][4] = cbKeyCardData;
							break;
						}
					}
				}
			}		
		}	
		else if (Keyroomusercontrol.userControl.control_type == CONTINUE_LOST)
		{
			//失败玩家 从无牛开始构造 
			for (WORD wCardTypeIndex=CT_CLASSIC_OX_VALUE0; wCardTypeIndex<CT_CLASSIC_OX_VALUENIUNIU; wCardTypeIndex++)
			{
				BYTE cbKeyCardData = m_GameLogic.ConstructCardType(cardlist, cbControlCardData[Keyroomusercontrol.roomUserInfo.wChairID], wCardTypeIndex, gtConfig);
				if (cbKeyCardData == INVALID_BYTE)
				{
					continue;
				}
				else
				{
					cbControlCardData[Keyroomusercontrol.roomUserInfo.wChairID][4] = cbKeyCardData;
					break;
				}
			}

			//其他玩家构造 从牛牛顺序开始构造
			for (WORD i=0; i<GAME_PLAYER; i++)
			{
				if (cbControlCardData[i][0] != 0 && i != Keyroomusercontrol.roomUserInfo.wChairID)
				{
					//构造无牛到牛九
					for (WORD wCardTypeIndex=CT_CLASSIC_OX_VALUENIUNIU; wCardTypeIndex>CT_CLASSIC_OX_VALUE0; wCardTypeIndex--)
					{
						BYTE cbKeyCardData = m_GameLogic.ConstructCardType(cardlist, cbControlCardData[i], wCardTypeIndex, gtConfig);
						if (cbKeyCardData == INVALID_BYTE)
						{
							continue;
						}
						else
						{
							cbControlCardData[i][4] = cbKeyCardData;
							break;
						}
					}
				}
			}
		}

		return true;
	}
	
	ASSERT(FALSE);

	return false;
}