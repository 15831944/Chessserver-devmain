﻿#include "StdAfx.h"
#include "GameLogic.h"

//////////////////////////////////////////////////////////////////////////

//扑克数据
BYTE CGameLogic::m_cbCardListDataNoKing[52]=
{
	0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,		//方块 A - K
	0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,		//梅花 A - K
	0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,		//红桃 A - K
	0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D		//黑桃 A - K
};

//扑克数据
BYTE CGameLogic::m_cbCardListDataHaveKing[54]=
{
	0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,		//方块 A - K
	0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,		//梅花 A - K
	0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,		//红桃 A - K
	0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,		//黑桃 A - K
	0x4E,0x4F
};

//////////////////////////////////////////////////////////////////////////

//构造函数
CGameLogic::CGameLogic()
{
}

//析构函数
CGameLogic::~CGameLogic()
{
}

//获取类型
BYTE CGameLogic::GetCardType(BYTE cbCardData[], BYTE cbCardCount, CARDTYPE_CONFIG ctConfig)
{
	//数据效验
	ASSERT (cbCardCount == MAX_CARDCOUNT);

	//分析扑克
	tagAnalyseResult AnalyseResult;
	ZeroMemory(&AnalyseResult, sizeof(AnalyseResult));
	AnalysebCardData(cbCardData, cbCardCount, AnalyseResult);
	
	//特殊牌型
	//经典模式
	if (ctConfig == CT_CLASSIC_)
	{
		if (IsFiveSNiuNiu(cbCardData, cbCardCount))
		{
			return CT_CLASSIC_OX_VALUE_FIVESNIUNIU;
		}
		else if (IsTongHuaShun(cbCardData, cbCardCount))
		{
			return CT_CLASSIC_OX_VALUE_TONGHUASHUN;
		}
		else if (IsBomb(cbCardData, cbCardCount))
		{
			return CT_CLASSIC_OX_VALUE_BOMB;
		}
		else if (IsHuLu(cbCardData, cbCardCount))
		{
			return CT_CLASSIC_OX_VALUE_HULU;
		}
		else if (IsTongHua(cbCardData, cbCardCount))
		{
			return CT_CLASSIC_OX_VALUE_SAMEFLOWER;
		}
		else if (IsShunZi(cbCardData, cbCardCount))
		{
			return CT_CLASSIC_OX_VALUE_SHUNZI;
		}
		else if (IsFiveFlowerNN(cbCardData, cbCardCount))
		{
			return CT_CLASSIC_OX_VALUE_FIVEFLOWER;
		}
		else if (IsFourFlowerNN(cbCardData, cbCardCount))
		{
			return CT_CLASSIC_OX_VALUE_FOURFLOWER;
		}
	}
	else if (ctConfig == CT_ADDTIMES_)
	{
		if (IsTongHuaShun(cbCardData, cbCardCount))
		{
			return CT_ADDTIMES_OX_VALUE_TONGHUASHUN;
		}
		else if (IsBomb(cbCardData, cbCardCount))
		{
			return CT_ADDTIMES_OX_VALUE_BOMB;
		}
		else if (IsFiveSNiuNiu(cbCardData, cbCardCount))
		{
			return CT_ADDTIMES_OX_VALUE_FIVESNIUNIU;
		}
		else if (IsFiveFlowerNN(cbCardData, cbCardCount))
		{
			return CT_ADDTIMES_OX_VALUE_FIVEFLOWER;
		}
		else if (IsFourFlowerNN(cbCardData, cbCardCount))
		{
			return CT_ADDTIMES_OX_VALUE_FOURFLOWER;
		}
		else if (IsHuLu(cbCardData, cbCardCount))
		{
			return CT_ADDTIMES_OX_VALUE_HULU;
		}
		else if (IsTongHua(cbCardData, cbCardCount))
		{
			return CT_ADDTIMES_OX_VALUE_SAMEFLOWER;
		}
		else if (IsShunZi(cbCardData, cbCardCount))
		{
			return CT_ADDTIMES_OX_VALUE_SHUNZI;
		}
	}

	//一般牌型
	//两张王
	if (AnalyseResult.cbKingCount == 2)
	{	
		//经典模式
		if (ctConfig == CT_CLASSIC_)
		{
			//设置变量
			BYTE cbTempData[MAX_CARDCOUNT];
			CopyMemory(cbTempData, cbCardData, sizeof(cbTempData));

			if (cbTempData[0] != 0x4E && cbTempData[0] != 0x4F &&
				cbTempData[1] != 0x4E && cbTempData[1] != 0x4F &&
				cbTempData[2] != 0x4E && cbTempData[2] != 0x4F)
			{
				BYTE cbValue0 = GetNNCardLogicValue(cbTempData[0]);
				BYTE cbValue1 = GetNNCardLogicValue(cbTempData[1]);
				BYTE cbValue2 = GetNNCardLogicValue(cbTempData[2]);

				if ((cbValue0 + cbValue1 + cbValue2) % 10 == 0)
				{
					return CT_CLASSIC_OX_VALUENIUNIU;
				}
			}
			else if (cbTempData[3] != 0x4E && cbTempData[3] != 0x4F &&
				cbTempData[4] != 0x4E && cbTempData[4] != 0x4F)
			{
				BYTE cbCount = (GetNNCardLogicValue(cbTempData[3]) + GetNNCardLogicValue(cbTempData[4])) % 10;
				switch (cbCount)
				{
				case 1: return CT_CLASSIC_OX_VALUE1;
				case 2: return CT_CLASSIC_OX_VALUE2;
				case 3: return CT_CLASSIC_OX_VALUE3;
				case 4: return CT_CLASSIC_OX_VALUE4;
				case 5: return CT_CLASSIC_OX_VALUE5;
				case 6: return CT_CLASSIC_OX_VALUE6;
				case 7: return CT_CLASSIC_OX_VALUE7;
				case 8: return CT_CLASSIC_OX_VALUE8;
				case 9: return CT_CLASSIC_OX_VALUE9;
				case 0: return CT_CLASSIC_OX_VALUENIUNIU;
				default: ASSERT(FALSE);
				}
			}
			else 
			{
				return CT_CLASSIC_OX_VALUENIUNIU;
			}
		}
		else if (ctConfig == CT_ADDTIMES_)
		{
			//设置变量
			BYTE cbTempData[MAX_CARDCOUNT];
			CopyMemory(cbTempData, cbCardData, sizeof(cbTempData));

			if (cbTempData[0] != 0x4E && cbTempData[0] != 0x4F &&
				cbTempData[1] != 0x4E && cbTempData[1] != 0x4F &&
				cbTempData[2] != 0x4E && cbTempData[2] != 0x4F)
			{
				BYTE cbValue0 = GetNNCardLogicValue(cbTempData[0]);
				BYTE cbValue1 = GetNNCardLogicValue(cbTempData[1]);
				BYTE cbValue2 = GetNNCardLogicValue(cbTempData[2]);

				if ((cbValue0 + cbValue1 + cbValue2) % 10 == 0)
				{
					return CT_ADDTIMES_OX_VALUENIUNIU;
				}
			}
			else if (cbTempData[3] != 0x4E && cbTempData[3] != 0x4F &&
				cbTempData[4] != 0x4E && cbTempData[4] != 0x4F)
			{
				BYTE cbCount = (GetNNCardLogicValue(cbTempData[3]) + GetNNCardLogicValue(cbTempData[4])) % 10;
				switch (cbCount)
				{
				case 1: return CT_ADDTIMES_OX_VALUE1;
				case 2: return CT_ADDTIMES_OX_VALUE2;
				case 3: return CT_ADDTIMES_OX_VALUE3;
				case 4: return CT_ADDTIMES_OX_VALUE4;
				case 5: return CT_ADDTIMES_OX_VALUE5;
				case 6: return CT_ADDTIMES_OX_VALUE6;
				case 7: return CT_ADDTIMES_OX_VALUE7;
				case 8: return CT_ADDTIMES_OX_VALUE8;
				case 9: return CT_ADDTIMES_OX_VALUE9;
				case 0: return CT_ADDTIMES_OX_VALUENIUNIU;
				default: ASSERT(FALSE);
				}
			}
			else 
			{
				return CT_ADDTIMES_OX_VALUENIUNIU;
			}
		}
	}
	else if (AnalyseResult.cbKingCount == 1)
	{
		//经典模式
		if (ctConfig == CT_CLASSIC_)
		{
			//设置变量
			BYTE cbTempData[MAX_CARDCOUNT];
			CopyMemory(cbTempData, cbCardData, sizeof(cbTempData));

			if (cbTempData[0] != 0x4E && cbTempData[0] != 0x4F &&
				cbTempData[1] != 0x4E && cbTempData[1] != 0x4F &&
				cbTempData[2] != 0x4E && cbTempData[2] != 0x4F)
			{
				BYTE cbValue0 = GetNNCardLogicValue(cbTempData[0]);
				BYTE cbValue1 = GetNNCardLogicValue(cbTempData[1]);
				BYTE cbValue2 = GetNNCardLogicValue(cbTempData[2]);

				if ((cbValue0 + cbValue1 + cbValue2) % 10 == 0)
				{
					return CT_CLASSIC_OX_VALUENIUNIU;
				}
			}
			else
			{
				BYTE cbCount = (GetNNCardLogicValue(cbTempData[3]) + GetNNCardLogicValue(cbTempData[4])) % 10;
				switch (cbCount)
				{
				case 1: return CT_CLASSIC_OX_VALUE1;
				case 2: return CT_CLASSIC_OX_VALUE2;
				case 3: return CT_CLASSIC_OX_VALUE3;
				case 4: return CT_CLASSIC_OX_VALUE4;
				case 5: return CT_CLASSIC_OX_VALUE5;
				case 6: return CT_CLASSIC_OX_VALUE6;
				case 7: return CT_CLASSIC_OX_VALUE7;
				case 8: return CT_CLASSIC_OX_VALUE8;
				case 9: return CT_CLASSIC_OX_VALUE9;
				case 0: return CT_CLASSIC_OX_VALUENIUNIU;
				default: ASSERT(FALSE);
				}
			}
		}
		else if (ctConfig == CT_ADDTIMES_)
		{
			//设置变量
			BYTE cbTempData[MAX_CARDCOUNT];
			CopyMemory(cbTempData, cbCardData, sizeof(cbTempData));

			if (cbTempData[0] != 0x4E && cbTempData[0] != 0x4F &&
				cbTempData[1] != 0x4E && cbTempData[1] != 0x4F &&
				cbTempData[2] != 0x4E && cbTempData[2] != 0x4F)
			{
				BYTE cbValue0 = GetNNCardLogicValue(cbTempData[0]);
				BYTE cbValue1 = GetNNCardLogicValue(cbTempData[1]);
				BYTE cbValue2 = GetNNCardLogicValue(cbTempData[2]);

				if ((cbValue0 + cbValue1 + cbValue2) % 10 == 0)
				{
					return CT_ADDTIMES_OX_VALUENIUNIU;
				}
			}
			else
			{
				BYTE cbCount = (GetNNCardLogicValue(cbTempData[3]) + GetNNCardLogicValue(cbTempData[4])) % 10;
				switch (cbCount)
				{
				case 1: return CT_ADDTIMES_OX_VALUE1;
				case 2: return CT_ADDTIMES_OX_VALUE2;
				case 3: return CT_ADDTIMES_OX_VALUE3;
				case 4: return CT_ADDTIMES_OX_VALUE4;
				case 5: return CT_ADDTIMES_OX_VALUE5;
				case 6: return CT_ADDTIMES_OX_VALUE6;
				case 7: return CT_ADDTIMES_OX_VALUE7;
				case 8: return CT_ADDTIMES_OX_VALUE8;
				case 9: return CT_ADDTIMES_OX_VALUE9;
				case 0: return CT_ADDTIMES_OX_VALUENIUNIU;
				default: ASSERT(FALSE);
				}
			}
		}
	}
	//不含大小王
	else if (AnalyseResult.cbKingCount == 0)
	{
		BYTE bTemp[MAX_CARDCOUNT];
		BYTE bSum=0;
		for (BYTE i=0;i<cbCardCount;i++)
		{
			bTemp[i]=GetNNCardLogicValue(cbCardData[i]);
			bSum+=bTemp[i];
		}

		for (BYTE i=0;i<cbCardCount-1;i++)
		{
			for (BYTE j=i+1;j<cbCardCount;j++)
			{
				if((bSum-bTemp[i]-bTemp[j])%10==0)
				{
					return ((bTemp[i]+bTemp[j])>10)?(bTemp[i]+bTemp[j]-10):(bTemp[i]+bTemp[j]);
				}
			}
		}
	}

	return CT_CLASSIC_OX_VALUE0;
}

//获取倍数
BYTE CGameLogic::GetTimes(BYTE cbCardData[], BYTE cbCardCount, CARDTYPE_CONFIG ctConfig, BYTE cbCombineCardType)
{
	if (cbCardCount != MAX_CARDCOUNT)
	{
		return 0;
	}
	
	//获取牌型
	BYTE cbCardType = ((cbCombineCardType == INVALID_BYTE) ? GetCardType(cbCardData, MAX_CARDCOUNT, ctConfig) : cbCombineCardType);

	//经典模式
	if (ctConfig == CT_CLASSIC_)
	{
		switch(cbCardType)
		{
		case CT_CLASSIC_OX_VALUE_FIVESNIUNIU:
			{
				return 4;
			}
		case CT_CLASSIC_OX_VALUE_TONGHUASHUN:
		case CT_CLASSIC_OX_VALUE_BOMB:
		case CT_CLASSIC_OX_VALUE_HULU:
		case CT_CLASSIC_OX_VALUE_SAMEFLOWER:
		case CT_CLASSIC_OX_VALUE_SHUNZI:
		case CT_CLASSIC_OX_VALUE_FIVEFLOWER:
			{
				return 4;
			}
		case CT_CLASSIC_OX_VALUE_FOURFLOWER:
			{
				return 4;
			}
		case CT_CLASSIC_OX_VALUENIUNIU:
			{
				return 3;
			}
		case CT_CLASSIC_OX_VALUE9:
		case CT_CLASSIC_OX_VALUE8:
		case CT_CLASSIC_OX_VALUE7:
			{
				return 2;
			}
		case CT_CLASSIC_OX_VALUE6:
		case CT_CLASSIC_OX_VALUE5:
		case CT_CLASSIC_OX_VALUE4:
		case CT_CLASSIC_OX_VALUE3:
		case CT_CLASSIC_OX_VALUE2:
		case CT_CLASSIC_OX_VALUE1:
		case CT_CLASSIC_OX_VALUE0:
			{
				return 1;
			}
		default:
			ASSERT(false);
		}
	}	
	//疯狂加倍
	else if (ctConfig == CT_ADDTIMES_) {
		switch(cbCardType) {
			case CT_ADDTIMES_OX_VALUE_TONGHUASHUN:
				return 8;
			case CT_ADDTIMES_OX_VALUE_BOMB:
				return 7;
			case CT_ADDTIMES_OX_VALUE_FIVESNIUNIU:
				return 6;
			case CT_ADDTIMES_OX_VALUE_FIVEFLOWER:
				return 5;
			case CT_ADDTIMES_OX_VALUE_FOURFLOWER:
			case CT_ADDTIMES_OX_VALUE_HULU:
			case CT_ADDTIMES_OX_VALUE_SAMEFLOWER:
			case CT_ADDTIMES_OX_VALUE_SHUNZI:
				return 4;
			case CT_ADDTIMES_OX_VALUENIUNIU:
				return 3;
			case CT_ADDTIMES_OX_VALUE9:
			case CT_ADDTIMES_OX_VALUE8:
			case CT_ADDTIMES_OX_VALUE7:
				return 2;
			case CT_ADDTIMES_OX_VALUE6:
			case CT_ADDTIMES_OX_VALUE5:
			case CT_ADDTIMES_OX_VALUE4:
			case CT_ADDTIMES_OX_VALUE3:
			case CT_ADDTIMES_OX_VALUE2:
			case CT_ADDTIMES_OX_VALUE1:
			case CT_ADDTIMES_OX_VALUE0:
				return 1;
			default:
				return 1;
		}
		
	}
	return INVALID_BYTE;
}

//排列扑克(通用牛牛牌型使用)
void CGameLogic::SortNNCardList(BYTE cbCardData[], BYTE cbCardCount)
{
	//转换数值
	BYTE cbLogicValue[MAX_CARDCOUNT];
	for (BYTE i=0;i<cbCardCount;i++) cbLogicValue[i]=GetCardValue(cbCardData[i]);	

	//排序操作
	bool bSorted=true;
	BYTE cbTempData,bLast=cbCardCount-1;
	do
	{
		bSorted=true;
		for (BYTE i=0;i<bLast;i++)
		{
			if ((cbLogicValue[i]<cbLogicValue[i+1])||
				((cbLogicValue[i]==cbLogicValue[i+1])&&(cbCardData[i]<cbCardData[i+1])))
			{
				//交换位置
				cbTempData=cbCardData[i];
				cbCardData[i]=cbCardData[i+1];
				cbCardData[i+1]=cbTempData;
				cbTempData=cbLogicValue[i];
				cbLogicValue[i]=cbLogicValue[i+1];
				cbLogicValue[i+1]=cbTempData;
				bSorted=false;
			}	
		}
		bLast--;
	} while(bSorted==false);

	return;
}

void CGameLogic::SortCardList(BYTE cbCardData[], BYTE cbCardCount, bool bDescend)
{
	//转换数值
	BYTE cbSortValue[MAX_CARDCOUNT];
	for (BYTE i=0;i<cbCardCount;i++) cbSortValue[i]=GetCardLogicValue(cbCardData[i]);	

	//排序操作
	bool bSorted=true;
	BYTE cbSwitchData=0,cbLast=cbCardCount-1;
	
	//降序
	if (bDescend)
	{
		do
		{
			bSorted=true;
			for (BYTE i=0;i<cbLast;i++)
			{
				if ((cbSortValue[i]<cbSortValue[i+1])||
					((cbSortValue[i]==cbSortValue[i+1])&&(cbCardData[i]<cbCardData[i+1])))
				{
					//设置标志
					bSorted=false;

					//扑克数据
					cbSwitchData=cbCardData[i];
					cbCardData[i]=cbCardData[i+1];
					cbCardData[i+1]=cbSwitchData;

					//排序权位
					cbSwitchData=cbSortValue[i];
					cbSortValue[i]=cbSortValue[i+1];
					cbSortValue[i+1]=cbSwitchData;
				}	
			}
			cbLast--;
		} while(bSorted==false);
	}
	else
	{
		do
		{
			bSorted=true;
			for (BYTE i=0;i<cbLast;i++)
			{
				if ((cbSortValue[i]>cbSortValue[i+1])||
					((cbSortValue[i]==cbSortValue[i+1])&&(cbCardData[i]>cbCardData[i+1])))
				{
					//设置标志
					bSorted=false;

					//扑克数据
					cbSwitchData=cbCardData[i];
					cbCardData[i]=cbCardData[i+1];
					cbCardData[i+1]=cbSwitchData;

					//排序权位
					cbSwitchData=cbSortValue[i];
					cbSortValue[i]=cbSortValue[i+1];
					cbSortValue[i+1]=cbSwitchData;
				}	
			}
			cbLast--;
		} while(bSorted==false);
	}
}

//混乱扑克
void CGameLogic::RandCardList(BYTE cbCardBuffer[], BYTE cbBufferCount, bool bHaveKing)
{
	//不含大王
	if (!bHaveKing)
	{
		//混乱准备
		BYTE cbCardData[CountArray(m_cbCardListDataNoKing)];
		CopyMemory(cbCardData,m_cbCardListDataNoKing,sizeof(m_cbCardListDataNoKing));

		//混乱扑克
		BYTE bRandCount=0,bPosition=0;
		do
		{
			bPosition=rand()%(CountArray(m_cbCardListDataNoKing)-bRandCount);
			cbCardBuffer[bRandCount++]=cbCardData[bPosition];
			cbCardData[bPosition]=cbCardData[CountArray(m_cbCardListDataNoKing)-bRandCount];
		} while (bRandCount<cbBufferCount);
	}
	else
	{
		//混乱准备
		BYTE cbCardData[CountArray(m_cbCardListDataHaveKing)];
		CopyMemory(cbCardData,m_cbCardListDataHaveKing,sizeof(m_cbCardListDataHaveKing));

		//混乱扑克
		BYTE bRandCount=0,bPosition=0;
		do
		{
			bPosition=rand()%(CountArray(m_cbCardListDataHaveKing)-bRandCount);
			cbCardBuffer[bRandCount++]=cbCardData[bPosition];
			cbCardData[bPosition]=cbCardData[CountArray(m_cbCardListDataHaveKing)-bRandCount];
		} while (bRandCount<cbBufferCount);
	}

	return;
}

//逻辑数值(通用牛牛牌型使用)
BYTE CGameLogic::GetNNCardLogicValue(BYTE cbCardData)
{
	//扑克属性
	BYTE bCardColor=GetCardColor(cbCardData);
	BYTE bCardValue=GetCardValue(cbCardData);

	//转换数值
	return (bCardValue>10)?(10):bCardValue;
}

//逻辑数值(新加牌型使用)
BYTE CGameLogic::GetCardLogicValue(BYTE cbCardData)
{
	//扑克属性
	BYTE cbCardColor=GetCardColor(cbCardData);
	BYTE cbCardValue=GetCardValue(cbCardData);

	//转换数值
	if (cbCardColor==0x40) return cbCardValue+2;
	return (cbCardValue<=2)?(cbCardValue+13):cbCardValue;
}

//对比扑克
bool CGameLogic::CompareCard(BYTE cbFirstData[], BYTE cbNextData[], BYTE cbCardCount, CARDTYPE_CONFIG ctConfig, BYTE cbFirstTypeEX, BYTE cbNextTypeEX)
{
	//比较牛大小(FirstOX 只是判断是牛一及以上)
	//获取点数
	BYTE cbNextType = ((cbNextTypeEX == INVALID_BYTE) ? GetCardType(cbNextData, cbCardCount, ctConfig) : cbNextTypeEX);
	BYTE cbFirstType = ((cbFirstTypeEX == INVALID_BYTE) ? GetCardType(cbFirstData, cbCardCount, ctConfig) : cbFirstTypeEX);

	//类型判断
	if (cbFirstType != cbNextType) 
	{
		return (cbFirstType > cbNextType);
	}
	
	//比较是否有王牌
	bool bNextKingFlag = IsContainKingCard(cbNextData, cbCardCount);
	bool bFirstKingFlag = IsContainKingCard(cbFirstData, cbCardCount);

	if (bNextKingFlag != bFirstKingFlag)
	{
		if ((bNextKingFlag == true) && (bFirstKingFlag == false))
		{
			return true;
		}
		else if ((bNextKingFlag == false) && (bFirstKingFlag == true))
		{
			return false;
		}
	}

	//分析扑克
	tagAnalyseResult FirstAnalyseResult;
	tagAnalyseResult NextAnalyseResult;
	ZeroMemory(&FirstAnalyseResult, sizeof(FirstAnalyseResult));
	ZeroMemory(&NextAnalyseResult, sizeof(NextAnalyseResult));
	AnalysebCardData(cbFirstData, cbCardCount, FirstAnalyseResult);
	AnalysebCardData(cbNextData, cbCardCount, NextAnalyseResult);
	
	//同牌型，  顺子 同花  葫芦  炸弹 同花顺   这些牌型的逻辑值还是从 2 A ・・・・・3 
	//同牌型牛类 牛1 到牛牛，四花牛，五花牛  五小牛 这些牌型的逻辑值还是从  K Q ・・・・・・A 
	//比较同牌型的(经典模式和加倍模式的宏定义值一样,这里只有经典模式，适用于加倍模式下的比牌)
	switch(cbNextType)
	{
	case CT_CLASSIC_OX_VALUE0:
	case CT_CLASSIC_OX_VALUE1:
	case CT_CLASSIC_OX_VALUE2:
	case CT_CLASSIC_OX_VALUE3:
	case CT_CLASSIC_OX_VALUE4:
	case CT_CLASSIC_OX_VALUE5:
	case CT_CLASSIC_OX_VALUE6:
	case CT_CLASSIC_OX_VALUE7:
	case CT_CLASSIC_OX_VALUE8:
	case CT_CLASSIC_OX_VALUE9:
	case CT_CLASSIC_OX_VALUENIUNIU:
	case CT_CLASSIC_OX_VALUE_FOURFLOWER:
	case CT_CLASSIC_OX_VALUE_FIVEFLOWER:
	case CT_CLASSIC_OX_VALUE_FIVESNIUNIU:
		{
			//排序大小
			BYTE bFirstTemp[MAX_CARDCOUNT];
			BYTE bNextTemp[MAX_CARDCOUNT];
			CopyMemory(bFirstTemp,cbFirstData,cbCardCount);
			CopyMemory(bNextTemp,cbNextData,cbCardCount);
			SortNNCardList(bFirstTemp,cbCardCount);
			SortNNCardList(bNextTemp,cbCardCount);

			//比较数值
			BYTE cbNextMaxValue=GetCardValue(bNextTemp[0]);
			BYTE cbFirstMaxValue=GetCardValue(bFirstTemp[0]);
			if (cbNextMaxValue!=cbFirstMaxValue)
			{
				return cbFirstMaxValue>cbNextMaxValue;
			}

			//比较颜色
			return GetCardColor(bFirstTemp[0]) > GetCardColor(bNextTemp[0]);
		}
		//同花顺
	case CT_CLASSIC_OX_VALUE_TONGHUASHUN:
		{
			//排序大小
			BYTE bFirstTemp[MAX_CARDCOUNT];
			BYTE bNextTemp[MAX_CARDCOUNT];
			CopyMemory(bFirstTemp,cbFirstData,cbCardCount);
			CopyMemory(bNextTemp,cbNextData,cbCardCount);
			SortCardList(bFirstTemp,cbCardCount);
			SortCardList(bNextTemp,cbCardCount);

			//比较花色
			BYTE cbNextMaxColor=GetCardColor(bNextTemp[MAX_CARDCOUNT-1]);
			BYTE cbFirstMaxColor=GetCardColor(bFirstTemp[MAX_CARDCOUNT-1]);
			if (cbNextMaxColor != cbFirstMaxColor)
			{
				return cbFirstMaxColor>cbNextMaxColor;
			}
			
			ASSERT (FirstAnalyseResult.cbKingCount == NextAnalyseResult.cbKingCount);
			
			//无王牌
			if (FirstAnalyseResult.cbKingCount == 0)
			{
				//比较数值
				BYTE cbNextMaxValue=GetCardLogicValue(bNextTemp[MAX_CARDCOUNT-1]);
				BYTE cbFirstMaxValue=GetCardLogicValue(bFirstTemp[MAX_CARDCOUNT-1]);
				if (cbNextMaxValue!=cbFirstMaxValue)
				{
					return cbFirstMaxValue>cbNextMaxValue;
				}
			}
			//各含一张王牌（需要构造同花顺来比较）
			else if (FirstAnalyseResult.cbKingCount == 1)
			{
				//升序排列
				SortCardList(bFirstTemp, cbCardCount, false);
				SortCardList(bNextTemp, cbCardCount, false);

				BYTE cbFirstValue = GetCardLogicValue(bFirstTemp[0]);
				BYTE cbFirstColor = GetCardColor(bFirstTemp[0]);
				for (int i=0; i<cbCardCount; i++)
				{
					if (bFirstTemp[i] == 0x4E || bFirstTemp[i] == 0x4F)
					{
						continue;
					}

					//检验连续
					if (GetCardLogicValue(bFirstTemp[i]) != cbFirstValue + i)
					{
						bFirstTemp[GetKingCardIndex(bFirstTemp, cbCardCount)] = ConstructCard(cbFirstValue + i, cbFirstColor);
						break;
					}
				}

				BYTE cbNextValue = GetCardLogicValue(bNextTemp[0]);
				BYTE cbNextColor = GetCardColor(bNextTemp[0]);
				for (int i=0; i<cbCardCount; i++)
				{
					if (bNextTemp[i] == 0x4E || bNextTemp[i] == 0x4F)
					{
						continue;
					}

					//检验连续
					if (GetCardLogicValue(bNextTemp[i]) != cbNextValue + i)
					{
						bNextTemp[GetKingCardIndex(bNextTemp, cbCardCount)] = ConstructCard(cbNextValue + i, cbNextColor);
						break;
					}
				}
				
				//降序
				SortCardList(bFirstTemp, cbCardCount);
				SortCardList(bNextTemp, cbCardCount);
				
				//比较数值
				BYTE cbNextMaxValue = GetCardLogicValue(bNextTemp[0]);
				BYTE cbFirstMaxValue = GetCardLogicValue(bFirstTemp[0]);
				if (cbNextMaxValue != cbFirstMaxValue)
				{
					return cbFirstMaxValue > cbNextMaxValue;
				}				
			}

			ASSERT (false);
		}
		//炸弹
	case CT_CLASSIC_OX_VALUE_BOMB:
		{
			ASSERT (FirstAnalyseResult.cbKingCount == NextAnalyseResult.cbKingCount);

			//无王牌
			if (FirstAnalyseResult.cbKingCount == 0)
			{
				//比较数值
				BYTE cbNextMaxValue = GetCardLogicValue(NextAnalyseResult.cbCardData[3][0]);
				BYTE cbFirstMaxValue = GetCardLogicValue(FirstAnalyseResult.cbCardData[3][0]);
				if (cbNextMaxValue != cbFirstMaxValue)
				{
					return cbFirstMaxValue > cbNextMaxValue;
				}
			}
			else if (FirstAnalyseResult.cbKingCount == 1)
			{
				BYTE cbNextMaxValue = (NextAnalyseResult.cbBlockCount[3] != 0 ? GetCardLogicValue(NextAnalyseResult.cbCardData[3][0]) : GetCardLogicValue(NextAnalyseResult.cbCardData[2][0]));
				BYTE cbFirstMaxValue = (FirstAnalyseResult.cbBlockCount[3] != 0 ? GetCardLogicValue(FirstAnalyseResult.cbCardData[3][0]) : GetCardLogicValue(FirstAnalyseResult.cbCardData[2][0]));		
				
				if (cbNextMaxValue != cbFirstMaxValue)
				{
					return cbFirstMaxValue > cbNextMaxValue;
				}
			}

			ASSERT (false);
		}
		//葫芦
	case CT_CLASSIC_OX_VALUE_HULU:
		{
			ASSERT (FirstAnalyseResult.cbKingCount == NextAnalyseResult.cbKingCount);

			//无王牌
			if (FirstAnalyseResult.cbKingCount == 0)
			{
				//比较数值
				BYTE cbNextMaxValue = GetCardLogicValue(NextAnalyseResult.cbCardData[2][0]);
				BYTE cbFirstMaxValue = GetCardLogicValue(FirstAnalyseResult.cbCardData[2][0]);
				if (cbNextMaxValue != cbFirstMaxValue)
				{
					return cbFirstMaxValue > cbNextMaxValue;
				}
			}
			else if (FirstAnalyseResult.cbKingCount == 1)
			{
				BYTE cbNextMaxValue = GetCardLogicValue(NextAnalyseResult.cbCardData[1][0]);
				BYTE cbFirstMaxValue = GetCardLogicValue(FirstAnalyseResult.cbCardData[1][0]);

				if (cbNextMaxValue != cbFirstMaxValue)
				{
					return cbFirstMaxValue > cbNextMaxValue;
				}
			}

			ASSERT (false);
		}
		//同花
	case CT_CLASSIC_OX_VALUE_SAMEFLOWER:
		{
			//排序大小
			BYTE bFirstTemp[MAX_CARDCOUNT];
			BYTE bNextTemp[MAX_CARDCOUNT];
			CopyMemory(bFirstTemp,cbFirstData,cbCardCount);
			CopyMemory(bNextTemp,cbNextData,cbCardCount);
			SortCardList(bFirstTemp,cbCardCount);
			SortCardList(bNextTemp,cbCardCount);

			//比较花色
			BYTE cbNextMaxColor=GetCardColor(bNextTemp[MAX_CARDCOUNT-1]);
			BYTE cbFirstMaxColor=GetCardColor(bFirstTemp[MAX_CARDCOUNT-1]);
			if (cbNextMaxColor != cbFirstMaxColor)
			{
				return cbFirstMaxColor>cbNextMaxColor;
			}

			//比较数值
			BYTE cbNextMaxValue = GetCardLogicValue(bNextTemp[0]);
			BYTE cbFirstMaxValue = GetCardLogicValue(bFirstTemp[0]);
			if (cbNextMaxValue != cbFirstMaxValue)
			{
				return cbFirstMaxValue > cbNextMaxValue;
			}

			ASSERT (false);
		}
		//顺子
	case CT_CLASSIC_OX_VALUE_SHUNZI:
		{
			//排序大小
			BYTE bFirstTemp[MAX_CARDCOUNT];
			BYTE bNextTemp[MAX_CARDCOUNT];
			CopyMemory(bFirstTemp,cbFirstData,cbCardCount);
			CopyMemory(bNextTemp,cbNextData,cbCardCount);
			SortCardList(bFirstTemp,cbCardCount);
			SortCardList(bNextTemp,cbCardCount);

			ASSERT (FirstAnalyseResult.cbKingCount == NextAnalyseResult.cbKingCount);

			//无王牌
			if (FirstAnalyseResult.cbKingCount == 0)
			{
				//比较数值
				BYTE cbNextMaxValue=GetCardLogicValue(bNextTemp[0]);
				BYTE cbFirstMaxValue=GetCardLogicValue(bFirstTemp[0]);
				if (cbNextMaxValue!=cbFirstMaxValue)
				{
					return cbFirstMaxValue>cbNextMaxValue;
				}

				//比较花色
				BYTE cbNextMaxColor=GetCardColor(bNextTemp[0]);
				BYTE cbFirstMaxColor=GetCardColor(bFirstTemp[0]);
				if (cbNextMaxColor != cbFirstMaxColor)
				{
					return cbFirstMaxColor>cbNextMaxColor;
				}
			}
			//各含一张王牌（需要构造顺子来比较）
			else if (FirstAnalyseResult.cbKingCount == 1)
			{
				//升序排列
				SortCardList(bFirstTemp, cbCardCount, false);
				SortCardList(bNextTemp, cbCardCount, false);

				BYTE cbFirstValue = GetCardLogicValue(bFirstTemp[0]);
				BYTE cbFirstColor = 0x30;
				for (int i=0; i<cbCardCount; i++)
				{
					if (bFirstTemp[i] == 0x4E || bFirstTemp[i] == 0x4F)
					{
						continue;
					}

					//检验连续
					if (GetCardLogicValue(bFirstTemp[i]) != cbFirstValue + i)
					{
						bFirstTemp[GetKingCardIndex(bFirstTemp, cbCardCount)] = ConstructCard(cbFirstValue + i, cbFirstColor);
						break;
					}
				}

				BYTE cbNextValue = GetCardLogicValue(bNextTemp[0]);
				BYTE cbNextColor = 0x30;
				for (int i=0; i<cbCardCount; i++)
				{
					if (bNextTemp[i] == 0x4E || bNextTemp[i] == 0x4F)
					{
						continue;
					}

					//检验连续
					if (GetCardLogicValue(bNextTemp[i]) != cbNextValue + i)
					{
						bNextTemp[GetKingCardIndex(bNextTemp, cbCardCount)] = ConstructCard(cbNextValue + i, cbNextColor);
						break;
					}
				}

				//降序
				SortCardList(bFirstTemp, cbCardCount);
				SortCardList(bNextTemp, cbCardCount);

				//比较数值
				BYTE cbNextMaxValue = GetCardLogicValue(bNextTemp[0]);
				BYTE cbFirstMaxValue = GetCardLogicValue(bFirstTemp[0]);
				if (cbNextMaxValue != cbFirstMaxValue)
				{
					return cbFirstMaxValue > cbNextMaxValue;
				}
				
				//比较花色
				BYTE cbNextMaxColor=GetCardColor(bNextTemp[0]);
				BYTE cbFirstMaxColor=GetCardColor(bFirstTemp[0]);
				if (cbNextMaxColor != cbFirstMaxColor)
				{
					return cbFirstMaxColor>cbNextMaxColor;
				}
			}

			ASSERT (false);
		}

	}
	
	return false;
}

//获取牛牛
bool CGameLogic::GetOxCard(BYTE cbCardData[], BYTE cbCardCount)
{
	//数据效验
	ASSERT (cbCardCount == MAX_CARDCOUNT);

	//王牌统计
	BYTE cbKingCount = 0;
	for (BYTE i = 0; i <MAX_CARDCOUNT; i++)
	{
		if (cbCardData[i] == 0x4E || cbCardData[i] == 0x4F)
		{
			cbKingCount++;
		}
	}
	ASSERT (cbKingCount < 3);

	//分类组合
	if (cbKingCount == 2)
	{
		//拷贝扑克
		BYTE cbTempData[MAX_CARDCOUNT];
		CopyMemory(cbTempData, cbCardData, sizeof(cbTempData));
		SortNNCardList(cbTempData, MAX_CARDCOUNT);

		cbCardData[0] = cbTempData[2];
		cbCardData[1] = cbTempData[3];
		cbCardData[2] = cbTempData[0];
		cbCardData[3] = cbTempData[1];
		cbCardData[4] = cbTempData[4];

		return true;
	}
	else if (cbKingCount == 1)
	{
		//设置变量
		BYTE cbTempData[MAX_CARDCOUNT];
		CopyMemory(cbTempData, cbCardData, sizeof(cbTempData));
		SortNNCardList(cbTempData, MAX_CARDCOUNT);

		//牛牛牌型
		BYTE cbSum = 0;
		BYTE cbTemp[MAX_CARDCOUNT];
		ZeroMemory(cbTemp, sizeof(cbTemp));
		for (BYTE i = 1; i < MAX_CARDCOUNT; i++)
		{
			cbTemp[i] = GetNNCardLogicValue(cbTempData[i]);
			cbSum += cbTemp[i];
		}

		//三张成十
		for (BYTE i = 1; i < MAX_CARDCOUNT; i++)
		{
			if ((cbSum - cbTemp[i])%10 == 0)
			{
				BYTE cbCount = 0;
				for (BYTE j = 1; j < MAX_CARDCOUNT; j++ )
				{
					if (i != j)
					{
						cbCardData[cbCount++] = cbTempData[j];
					}
				}
				ASSERT(cbCount == 3);

				cbCardData[cbCount++] = cbTempData[i];
				cbCardData[cbCount++] = cbTempData[0];

				return true;
			}
		}

		//两张成十
		for (BYTE i = 1; i < MAX_CARDCOUNT - 1; i++)
		{
			for (BYTE j = i + 1; j < MAX_CARDCOUNT; j++)
			{
				if ((cbSum - cbTemp[i] - cbTemp[j]) % 10 == 0)
				{
					BYTE cbCount = 0;
					cbCardData[cbCount++] = cbTempData[0];
					cbCardData[cbCount++] = cbTempData[i];
					cbCardData[cbCount++] = cbTempData[j];

					for (BYTE k = 1; k < MAX_CARDCOUNT; k++ )
					{
						if (i != k && j != k)
						{
							cbCardData[cbCount++] = cbTempData[k];
						}
					}
					ASSERT(cbCount == 5);

					return true;
				}
			}
		}

		//四中取大
		BYTE cbBigCount = 0;
		BYTE cbBigIndex1 = 0;
		BYTE cbBigIndex2 = 0;
		for (BYTE i = 1; i < MAX_CARDCOUNT; i++)
		{
			for (BYTE j = i + 1;j < MAX_CARDCOUNT; j++)
			{
				BYTE cbSumCount = (GetNNCardLogicValue(cbTempData[i]) + GetNNCardLogicValue(cbTempData[j])) % 10;
				if (cbSumCount > cbBigCount)
				{
					cbBigIndex1 = i;
					cbBigIndex2 = j;
					cbBigCount = cbSumCount;
				}
			}
		}

		BYTE cbCount = 0;
		for (BYTE i = 1; i < MAX_CARDCOUNT; i++)
		{
			if (i != cbBigIndex1 && i != cbBigIndex2)
			{
				cbCardData[cbCount++] = cbTempData[i];
			}
		}
		ASSERT(cbCount == 2);

		cbCardData[cbCount++] = cbTempData[0];
		cbCardData[cbCount++] = cbTempData[cbBigIndex1];
		cbCardData[cbCount++] = cbTempData[cbBigIndex2];

		return true;
	}
	else if (cbKingCount == 0)
	{
		//普通牌型
		BYTE cbSum = 0;
		BYTE cbTemp[MAX_CARDCOUNT];
		BYTE cbTempData[MAX_CARDCOUNT];
		ZeroMemory(cbTemp, sizeof(cbTemp));
		CopyMemory(cbTempData, cbCardData, sizeof(cbTempData));
		for (BYTE i = 0; i < cbCardCount; i++)
		{
			cbTemp[i] = GetNNCardLogicValue(cbCardData[i]);
			cbSum += cbTemp[i];
		}

		//查找牛牛
		for (BYTE i = 0;i < cbCardCount - 1; i++)
		{
			for (BYTE j = i + 1; j < cbCardCount; j++)
			{
				if((cbSum - cbTemp[i] - cbTemp[j]) % 10 == 0)
				{
					BYTE cbCount = 0;
					for (BYTE k = 0; k < cbCardCount; k++)
					{
						if(k != i && k != j)
						{
							cbCardData[cbCount++] = cbTempData[k];
						}
					}
					ASSERT(cbCount == 3);

					cbCardData[cbCount++] = cbTempData[i];
					cbCardData[cbCount++] = cbTempData[j];

					return true;
				}
			}
		}
	}

	return false;
}

//分析扑克
bool CGameLogic::AnalysebCardData(const BYTE cbCardData[], BYTE cbCardCount, tagAnalyseResult & AnalyseResult)
{
	//设置结果
	ZeroMemory(&AnalyseResult,sizeof(AnalyseResult));

	//扑克分析
	for (BYTE i=0;i<cbCardCount;i++)
	{
		//变量定义
		BYTE cbSameCount=1,cbCardValueTemp=0;
		BYTE cbLogicValue=GetCardLogicValue(cbCardData[i]);

		//大小王计数
		if (GetCardColor(cbCardData[i]) == GetCardColor(0x4E))
		{
			AnalyseResult.cbKingCount++;
		}

		//搜索同牌
		for (BYTE j=i+1;j<cbCardCount;j++)
		{
			//获取扑克
			if (GetCardLogicValue(cbCardData[j])!=cbLogicValue) break;

			//设置变量
			cbSameCount++;
		}
		if(cbSameCount > 4)
		{
			return false;
		}
		//设置结果
		BYTE cbIndex = AnalyseResult.cbBlockCount[cbSameCount-1]++;
		for (BYTE j=0;j<cbSameCount;j++)
		{
			AnalyseResult.cbCardData[cbSameCount-1][cbIndex*cbSameCount+j]=cbCardData[i+j];
		}

		//设置索引
		i+=cbSameCount-1;
	}

	return true;
}

//判断五小牛
bool CGameLogic::IsFiveSNiuNiu(BYTE cbCardData[], BYTE cbCardCount)
{
	//数据效验
	ASSERT (cbCardCount <= MAX_CARDCOUNT);

	BYTE *pcbTempCardData = new BYTE[cbCardCount];
	ZeroMemory(pcbTempCardData, sizeof(BYTE) * cbCardCount);
	CopyMemory(pcbTempCardData, cbCardData, sizeof(BYTE) * cbCardCount);
	SortCardList(pcbTempCardData, cbCardCount);

	//分析扑克
	tagAnalyseResult AnalyseResult;
	ZeroMemory(&AnalyseResult, sizeof(AnalyseResult));
	AnalysebCardData(pcbTempCardData, cbCardCount, AnalyseResult);

	BYTE cbSum = 0;
	for (WORD i=0; i<cbCardCount; i++)
	{
		if (pcbTempCardData[i] == 0x4E || pcbTempCardData[i] == 0x4F)
		{
			continue;
		}

		if (GetCardValue(pcbTempCardData[i]) >= 5)
		{
			return false;
		}

		cbSum += GetCardValue(pcbTempCardData[i]);
	}
	
	delete[] pcbTempCardData;

	//王牌当最小点 1点
	if (cbSum + AnalyseResult.cbKingCount <= 10)
	{
		return true;
	}

	return false;
}

//判断同花顺
bool CGameLogic::IsTongHuaShun(BYTE cbCardData[], BYTE cbCardCount)
{
	//数据效验
	ASSERT (cbCardCount <= MAX_CARDCOUNT);
	
	return IsTongHua(cbCardData, cbCardCount) && IsShunZi(cbCardData, cbCardCount);
}

//判断同花
bool CGameLogic::IsTongHua(BYTE cbCardData[], BYTE cbCardCount)
{
	//数据效验
	ASSERT (cbCardCount <= MAX_CARDCOUNT);
	
	BYTE *pcbTempCardData = new BYTE[cbCardCount];
	ZeroMemory(pcbTempCardData, sizeof(BYTE) * cbCardCount);
	CopyMemory(pcbTempCardData, cbCardData, sizeof(BYTE) * cbCardCount);
	
	//从大到小逻辑排序
	SortCardList(pcbTempCardData, cbCardCount);

	BYTE cbColor = GetCardColor(pcbTempCardData[cbCardCount - 1]);

	for (BYTE i = 0; i < cbCardCount; i++)
	{
		if (pcbTempCardData[i] == 0x4E || pcbTempCardData[i] == 0x4F)
		{
			continue;
		}
		
		if (cbColor != GetCardColor(pcbTempCardData[i]))
		{
			delete[] pcbTempCardData;
			return false;
		}
	}

	delete[] pcbTempCardData;

	return true;
}

//判断顺子(待测试算法)
bool CGameLogic::IsShunZi(BYTE cbCardData[], BYTE cbCardCount)
{
	//数据效验
	ASSERT (cbCardCount <= MAX_CARDCOUNT);
	
	//检测 10 J Q K A
	if (IsContainAShunZi(cbCardData, cbCardCount) == true)
	{
		return true;
	}
	
	BYTE *pcbTempCardData = new BYTE[cbCardCount];
	ZeroMemory(pcbTempCardData, sizeof(BYTE) * cbCardCount);
	CopyMemory(pcbTempCardData, cbCardData, sizeof(BYTE) * cbCardCount);
	SortNNCardList(pcbTempCardData, cbCardCount);

	//分析扑克
	tagAnalyseResult AnalyseResult;
	ZeroMemory(&AnalyseResult, sizeof(AnalyseResult));
	AnalysebCardData(pcbTempCardData, cbCardCount, AnalyseResult);
	
	//如果不是5个单张
	if (AnalyseResult.cbBlockCount[0] != cbCardCount)
	{
		delete[] pcbTempCardData;
		return false;
	}

	BYTE cbFirstValue = GetCardValue(pcbTempCardData[cbCardCount - 1]);
	BYTE cbReplaceCount = 0;
	for (int i=cbCardCount-1; i>=0; i--)
	{
		if (pcbTempCardData[i] == 0x4E || pcbTempCardData[i] == 0x4F)
		{
			continue;
		}
		
		//检验连续
		if (GetCardValue(pcbTempCardData[i]) - (cbCardCount -1 - i) != cbFirstValue)
		{
			if (AnalyseResult.cbKingCount == 0)
			{
				delete[] pcbTempCardData;
				return false;
			}

			cbReplaceCount += (GetCardValue(pcbTempCardData[i]) - (cbCardCount -1 - i) - cbFirstValue);
			cbFirstValue += (GetCardValue(pcbTempCardData[i]) - (cbCardCount -1 - i) - cbFirstValue);
		}
	}
	
	delete[] pcbTempCardData;

	if (cbReplaceCount > AnalyseResult.cbKingCount)
	{
		return false;
	}

	//10 J Q K A顺子单数判断
	if ((cbFirstValue >= 1 && cbFirstValue <= 10 && AnalyseResult.cbKingCount == 0)
	|| (cbFirstValue >= 1 && cbFirstValue <= 11 && AnalyseResult.cbKingCount == 1)
	|| (cbFirstValue >= 1 && cbFirstValue <= 12 && AnalyseResult.cbKingCount == 2))
	{
		return true;
	}

	return false;
}

//判断炸弹
bool CGameLogic::IsBomb(BYTE cbCardData[], BYTE cbCardCount)
{
	//数据效验
	ASSERT (cbCardCount <= MAX_CARDCOUNT);

	BYTE *pcbTempCardData = new BYTE[cbCardCount];
	ZeroMemory(pcbTempCardData, sizeof(BYTE) * cbCardCount);
	CopyMemory(pcbTempCardData, cbCardData, sizeof(BYTE) * cbCardCount);
	SortCardList(pcbTempCardData, cbCardCount);

	//分析扑克
	tagAnalyseResult AnalyseResult;
	ZeroMemory(&AnalyseResult, sizeof(AnalyseResult));
	AnalysebCardData(pcbTempCardData, cbCardCount, AnalyseResult);
	
	delete[] pcbTempCardData;

	if ((AnalyseResult.cbBlockCount[3] == 1)
	 || (AnalyseResult.cbBlockCount[2] == 1 && AnalyseResult.cbKingCount >= 1)
	 || (AnalyseResult.cbBlockCount[1] >= 1 && AnalyseResult.cbKingCount >= 2))
	{
		return true;
	}

	return false;
}

//判断葫芦
bool CGameLogic::IsHuLu(BYTE cbCardData[], BYTE cbCardCount)
{
	//数据效验
	ASSERT (cbCardCount <= MAX_CARDCOUNT);

	BYTE *pcbTempCardData = new BYTE[cbCardCount];
	ZeroMemory(pcbTempCardData, sizeof(BYTE) * cbCardCount);
	CopyMemory(pcbTempCardData, cbCardData, sizeof(BYTE) * cbCardCount);
	SortCardList(pcbTempCardData, cbCardCount);

	//分析扑克
	tagAnalyseResult AnalyseResult;
	ZeroMemory(&AnalyseResult, sizeof(AnalyseResult));
	AnalysebCardData(pcbTempCardData, cbCardCount, AnalyseResult);

	delete[] pcbTempCardData;
	
	//无王牌
	if (AnalyseResult.cbKingCount == 0)
	{
		if (AnalyseResult.cbBlockCount[2] == 1 && AnalyseResult.cbBlockCount[1] == 1)
		{
			return true;
		}
	}
	else if (AnalyseResult.cbKingCount == 1)
	{
		if ((AnalyseResult.cbBlockCount[2] == 1)
		 || (AnalyseResult.cbBlockCount[1] == 2 && AnalyseResult.cbBlockCount[0] == 1))
		{
			return true;
		}
	}
	else if (AnalyseResult.cbKingCount == 2)
	{
		if ((AnalyseResult.cbBlockCount[2] == 1)
		 || (AnalyseResult.cbBlockCount[1] == 1))
		{
			return true;
		}
	}

	return false;
}

//判断五花牛
bool CGameLogic::IsFiveFlowerNN(BYTE cbCardData[], BYTE cbCardCount)
{
	//数据效验
	ASSERT (cbCardCount <= MAX_CARDCOUNT);

	BYTE *pcbTempCardData = new BYTE[cbCardCount];
	ZeroMemory(pcbTempCardData, sizeof(BYTE) * cbCardCount);
	CopyMemory(pcbTempCardData, cbCardData, sizeof(BYTE) * cbCardCount);
	SortCardList(pcbTempCardData, cbCardCount);

	//分析扑克
	tagAnalyseResult AnalyseResult;
	ZeroMemory(&AnalyseResult, sizeof(AnalyseResult));
	AnalysebCardData(pcbTempCardData, cbCardCount, AnalyseResult);

	BYTE cbJQK = 0;
	for (WORD i=0; i<cbCardCount; i++)
	{
		if (GetCardValue(pcbTempCardData[i]) >= 11 && GetCardValue(pcbTempCardData[i]) <= 13)
		{
			cbJQK++;
		}
	}
	
	delete[] pcbTempCardData;

	if (cbJQK + AnalyseResult.cbKingCount == cbCardCount)
	{
		return true;
	}

	return false;
}


//判断四花牛
bool CGameLogic::IsFourFlowerNN(BYTE cbCardData[], BYTE cbCardCount)
{
	//数据效验
	ASSERT (cbCardCount <= MAX_CARDCOUNT);

	BYTE *pcbTempCardData = new BYTE[cbCardCount];
	ZeroMemory(pcbTempCardData, sizeof(BYTE) * cbCardCount);
	CopyMemory(pcbTempCardData, cbCardData, sizeof(BYTE) * cbCardCount);
	SortCardList(pcbTempCardData, cbCardCount);

	//分析扑克
	tagAnalyseResult AnalyseResult;
	ZeroMemory(&AnalyseResult, sizeof(AnalyseResult));
	AnalysebCardData(pcbTempCardData, cbCardCount, AnalyseResult);

	BYTE cbJQK = 0;
	BYTE cbTen = 0;
	for (WORD i=0; i<cbCardCount; i++)
	{
		if (GetCardValue(pcbTempCardData[i]) >= 11 && GetCardValue(pcbTempCardData[i]) <= 13)
		{
			cbJQK++;
		}
		else if (GetCardValue(pcbTempCardData[i]) == 10)
		{
			cbTen++;
		}
	}

	delete[] pcbTempCardData;
	
	//先判断五花牛再判断四花牛
	if ((cbJQK + cbTen + AnalyseResult.cbKingCount == cbCardCount) && ((cbTen == 1) || (cbTen == 0 && AnalyseResult.cbKingCount != 0)))
	{
		return true;
	}

	return false;
}

//是否含有王牌
bool CGameLogic::IsContainKingCard(BYTE cbCardData[], BYTE cbCardCount)
{
	//数据效验
	ASSERT (cbCardCount <= MAX_CARDCOUNT);
	for (WORD i=0; i<cbCardCount; i++)
	{
		if (cbCardData[i] == 0x4E || cbCardData[i] == 0x4F)
		{
			return true;
		}
	}

	return false;
}

//获取第一张王牌的索引
BYTE CGameLogic::GetKingCardIndex(BYTE cbCardData[], BYTE cbCardCount)
{
	//数据效验
	ASSERT (cbCardCount <= MAX_CARDCOUNT);

	for (WORD i=0; i<cbCardCount; i++)
	{
		if (cbCardData[i] == 0x4E || cbCardData[i] == 0x4F)
		{
			return i;
		}
	}

	return INVALID_BYTE;
}

//构造扑克
BYTE CGameLogic::ConstructCard(BYTE cbCardLogicValue, BYTE cbCardColor)
{
	ASSERT (cbCardLogicValue >= 3 && cbCardLogicValue <= 17);
	cbCardColor = (cbCardColor >> 4);
	ASSERT (cbCardColor >= 0 && cbCardColor <=4);

	if (cbCardColor == 0x40)
	{
		return (cbCardLogicValue == 16 ? 0x4E : 0x4F);
	}

	if (cbCardLogicValue >= 14 && cbCardLogicValue <= 15)
	{
		cbCardLogicValue -= 13;
	}

	return 16 * cbCardColor + cbCardLogicValue;
}

//获取王牌张数
BYTE CGameLogic::GetKingCount(BYTE cbCardData[], BYTE cbCardCount)
{
	BYTE cbKingCount = 0;
	for (WORD i=0; i<cbCardCount; i++)
	{
		if (cbCardData[i] == 0x4E || cbCardData[i] == 0x4F)
		{
			cbKingCount++;
		}
	}

	return cbKingCount;
}

//是否含有A的顺子
bool CGameLogic::IsContainAShunZi(BYTE cbCardData[], BYTE cbCardCount)
{
	//数据效验
	ASSERT (cbCardCount <= MAX_CARDCOUNT);
	
	bool bHaveA = false;
	for (WORD i=0; i<cbCardCount; i++)
	{
		if (GetCardValue(cbCardData[i]) == 1)
		{
			bHaveA = true;
			break;
		}
	}

	if (!bHaveA)
	{
		return false;
	}

	BYTE *pcbTempCardData = new BYTE[cbCardCount];
	ZeroMemory(pcbTempCardData, sizeof(BYTE) * cbCardCount);
	CopyMemory(pcbTempCardData, cbCardData, sizeof(BYTE) * cbCardCount);
	SortCardList(pcbTempCardData, cbCardCount);

	//分析扑克
	tagAnalyseResult AnalyseResult;
	ZeroMemory(&AnalyseResult, sizeof(AnalyseResult));
	AnalysebCardData(pcbTempCardData, cbCardCount, AnalyseResult);

	//如果不是5个单张
	if (AnalyseResult.cbBlockCount[0] != cbCardCount)
	{
		delete[] pcbTempCardData;
		return false;
	}

	BYTE cbFirstValue = GetCardLogicValue(pcbTempCardData[cbCardCount - 1]);
	BYTE cbReplaceCount = 0;
	for (int i=cbCardCount-1; i>=0; i--)
	{
		if (pcbTempCardData[i] == 0x4E || pcbTempCardData[i] == 0x4F)
		{
			continue;
		}

		//检验连续
		if (GetCardLogicValue(pcbTempCardData[i]) - (cbCardCount -1 - i) != cbFirstValue)
		{
			if (AnalyseResult.cbKingCount == 0)
			{
				delete[] pcbTempCardData;
				return false;
			}

			cbReplaceCount += (GetCardLogicValue(pcbTempCardData[i]) - (cbCardCount -1 - i) - cbFirstValue);
			cbFirstValue += (GetCardLogicValue(pcbTempCardData[i]) - (cbCardCount -1 - i) - cbFirstValue);
		}
	}

	delete[] pcbTempCardData;

	if (cbReplaceCount > AnalyseResult.cbKingCount)
	{
		return false;
	}

	//
	if ((cbFirstValue >= 3 && cbFirstValue <= 10 && AnalyseResult.cbKingCount == 0)
		|| (cbFirstValue >= 3 && cbFirstValue <= 11 && AnalyseResult.cbKingCount == 1)
		|| (cbFirstValue >= 3 && cbFirstValue <= 12 && AnalyseResult.cbKingCount == 2))
	{
		return true;
	}

	return false;
}

//构造牌型，从确定前面4张牌，最后一张牌可变才构造牌型,返回最后一张牌， INVALID_BYTE为无法构造
BYTE CGameLogic::ConstructCardType(CList<BYTE, BYTE&> &cardlist, BYTE cbConstructCardData[MAX_CARDCOUNT], BYTE cbCardType, KING_CONFIG gtConfig)
{
	BYTE cbConstructCard = INVALID_BYTE;
	
	//构造牌型
	POSITION ptListHead = cardlist.GetHeadPosition();
	BYTE cbCardData = INVALID_BYTE;

	BYTE cbTempCardData[MAX_CARDCOUNT];
	ZeroMemory(cbTempCardData, sizeof(cbTempCardData));
	
	//遍历链表
	while(ptListHead)
	{
		CopyMemory(cbTempCardData, cbConstructCardData, sizeof(cbTempCardData));

		//替换最后一张牌
		BYTE cbLastCard = cardlist.GetNext(ptListHead);

		cbTempCardData[4] = cbLastCard;
		
		//获取牛牛牌型
		GetOxCard(cbTempCardData, MAX_CARDCOUNT);

		if (GetCardType(cbTempCardData, MAX_CARDCOUNT) == cbCardType)
		{
			cbConstructCard = cbLastCard;
			break;
		}
	}

	if (cbConstructCard != INVALID_BYTE)
	{
		RemoveKeyCard(cardlist, cbConstructCard);
	}

	return cbConstructCard;
}

//删除目标牌型
bool CGameLogic::RemoveKeyCard(CList<BYTE, BYTE&> &cardlist, BYTE cbKeyCard)
{
	POSITION ptListHead = cardlist.GetHeadPosition();
	POSITION ptTemp;
	BYTE cbCardData = INVALID_BYTE;

	//遍历链表
	while(ptListHead)
	{
		ptTemp = ptListHead;
		if (cardlist.GetNext(ptListHead) == cbKeyCard)
		{
			cardlist.RemoveAt(ptTemp);
			return true;
		}
	}
	
	return false;
}
//////////////////////////////////////////////////////////////////////////
