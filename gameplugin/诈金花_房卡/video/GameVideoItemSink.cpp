#include "StdAfx.h"
#include "GameVideoItemSink.h"

#define  ADD_VIDEO_BUF  4096

CGameVideoItemSink::CGameVideoItemSink(void)
{
	//���ñ���
	m_iCurPos	= 0;	
	m_iBufferSize	= 0;
	m_pVideoDataBuffer = NULL;
	m_pITableFrame = NULL;

	m_cbPlayerCount = 0;
}

CGameVideoItemSink::~CGameVideoItemSink( void )
{

}

bool __cdecl CGameVideoItemSink::StartVideo(ITableFrame	*pTableFrame, BYTE cbPlayerCount)
{
	try
	{
		ResetVideoItem();

		m_pITableFrame = pTableFrame;
		m_cbPlayerCount = cbPlayerCount;

		m_iCurPos	= 0;
		m_iBufferSize  = ADD_VIDEO_BUF;

		//�����ڴ�
		m_pVideoDataBuffer =new BYTE [m_iBufferSize];
		if (m_pVideoDataBuffer==NULL) return false;	
		memset(m_pVideoDataBuffer,0,m_iBufferSize);
	}
	catch (...) 
	{
		ASSERT (FALSE);
		return false; 
	}
	
	return true;
}

bool __cdecl CGameVideoItemSink::StopAndSaveVideo(WORD wServerID,WORD wTableID)
{
	//���浽���ݿ�
	if(m_pITableFrame == NULL) return false;
	//�����ʽ���ַ���+Ψһ��ʶID (ID��������������ʱ����+����ID+����ID) 	
	CHAR   szVideoNumber[22];
	szVideoNumber[0] = 0;
	BuildVideoNumber(szVideoNumber,22,wServerID,wTableID);
	
	bool bAllAndroid = true;
	for (WORD i = 0; i<m_cbPlayerCount; i++)
	{
		IServerUserItem *pServerUserItem = m_pITableFrame->GetTableUserItem(i);
		if (pServerUserItem && pServerUserItem->IsAndroidUser() == false)
		{
			bAllAndroid = false;
		}
	}
	if(bAllAndroid) return TRUE;

	for (WORD i = 0; i<m_cbPlayerCount; i++)
	{
		IServerUserItem *pServerUserItem = m_pITableFrame->GetTableUserItem(i);
		if (pServerUserItem)
		{
			m_pITableFrame->WriteTableVideoPlayer(pServerUserItem->GetUserID(),szVideoNumber);
		}
	}
	m_pITableFrame->WriteTableVideoData(szVideoNumber,wServerID,wTableID,m_pVideoDataBuffer,(WORD)m_iCurPos);
	
	int iLength ;  
	TCHAR   szVideoID[50];
	iLength = MultiByteToWideChar (CP_ACP, 0, szVideoNumber, strlen (szVideoNumber) + 1, NULL, 0) ;  
	MultiByteToWideChar (CP_ACP, 0, szVideoNumber, strlen (szVideoNumber) + 1, szVideoID, iLength) ;  

	TCHAR szFilePath[MAX_PATH];
	_sntprintf(szFilePath,CountArray(szFilePath),TEXT("%s.Video"),szVideoID);

	ResetVideoItem();

	return TRUE;
}

bool __cdecl CGameVideoItemSink::AddVideoData(WORD wMsgKind,Video_GameStart *pVideoGameStart,bool bFirst)
{
	//һ��Ҫ��˳��д
	if(bFirst) WriteUint16(wMsgKind);

	WriteUint16(pVideoGameStart->wPlayerCount);
	WriteUint16(pVideoGameStart->wGamePlayerCountRule);
	Write(pVideoGameStart->cbHandCardData, sizeof(BYTE)* GAME_PLAYER * 3);
	WriteUint64(pVideoGameStart->lMaxScore);
	WriteUint64(pVideoGameStart->lCellScore);
	WriteUint64(pVideoGameStart->lCurrentTimes);
	WriteUint64(pVideoGameStart->lUserMaxScore);
	WriteUint16(pVideoGameStart->wBankerUser);
	WriteUint16(pVideoGameStart->wCurrentUser);
	Write(pVideoGameStart->cbPlayStatus, sizeof(BYTE)*GAME_PLAYER);
	Write(pVideoGameStart->szNickName,sizeof(TCHAR)*LEN_NICKNAME);
	WriteUint16(pVideoGameStart->wChairID);
	WriteUint64(pVideoGameStart->lScore);

	return TRUE;
}

bool __cdecl CGameVideoItemSink::AddVideoData(WORD wMsgKind,CMD_S_AddScore *pVideoAddScore)
{
	//һ��Ҫ��˳��д
	WriteUint16(wMsgKind);
	WriteUint8(pVideoAddScore->wCurrentUser);
	WriteUint8(pVideoAddScore->wAddScoreUser);
	WriteUint8(pVideoAddScore->wCompareState);
	WriteUint32(pVideoAddScore->lAddScoreCount);
	WriteUint8(pVideoAddScore->lCurrentTimes);

	return TRUE;
}

bool __cdecl CGameVideoItemSink::AddVideoData(WORD wMsgKind,CMD_S_GiveUp *pVideoGiveUp)
{
	//һ��Ҫ��˳��д
	WriteUint16(wMsgKind);
	WriteUint16(pVideoGiveUp->wGiveUpUser);

	return TRUE;
}

bool __cdecl CGameVideoItemSink::AddVideoData(WORD wMsgKind,CMD_S_CompareCard *pVideoCompareCard)
{
	//һ��Ҫ��˳��д
	WriteUint16(wMsgKind);
	WriteUint16(pVideoCompareCard->wCurrentUser);
	Write(pVideoCompareCard->wCompareUser, sizeof(WORD) * 2);
	WriteUint16(pVideoCompareCard->wLostUser);

	return true;
}

bool __cdecl CGameVideoItemSink::AddVideoData(WORD wMsgKind,CMD_S_LookCard *pVideoLookCard)
{
	WriteUint16(wMsgKind);

	WriteUint16(pVideoLookCard->wLookCardUser);
	Write(pVideoLookCard->cbCardData, sizeof(BYTE) * MAX_COUNT);

	return true;
}

bool __cdecl  CGameVideoItemSink::AddVideoData(WORD wMsgKind,CMD_S_GameEnd *pVideoGameEnd)
{
	//һ��Ҫ��˳��д
	WriteUint16(wMsgKind);
	WriteUint64(pVideoGameEnd->lGameTax);
	Write(pVideoGameEnd->lGameScore, sizeof(LONGLONG) * GAME_PLAYER);
	Write(pVideoGameEnd->cbCardData, sizeof(BYTE) * GAME_PLAYER * 3);
	Write(pVideoGameEnd->wCompareUser, sizeof(WORD) * GAME_PLAYER * 4);
	WriteUint16(pVideoGameEnd->wEndState);
	WriteUint8(pVideoGameEnd->bDelayOverGame);
	WriteUint16(pVideoGameEnd->wServerType);

	return true;
}

size_t	CGameVideoItemSink::Write(const void* data, size_t size)
{   
	if(size + m_iCurPos > m_iBufferSize)
	{
		if(RectifyBuffer(ADD_VIDEO_BUF/2)!=TRUE) return 0;
	}
	
	CopyMemory(m_pVideoDataBuffer+m_iCurPos,data,size);				
	m_iCurPos += size;

	return size;
}

//��������
bool CGameVideoItemSink::RectifyBuffer(size_t iSize)
{
	try
	{		
		size_t iNewbufSize =  iSize+m_iBufferSize;
		//�����ڴ�
		BYTE * pNewVideoBuffer=new BYTE [iNewbufSize];
		if (pNewVideoBuffer==NULL) return false;
		memset(pNewVideoBuffer,0,iNewbufSize);

		//��������
		if (m_pVideoDataBuffer!=NULL) 
		{
			CopyMemory(pNewVideoBuffer,m_pVideoDataBuffer,m_iCurPos);				
		}

		//���ñ���			
		m_iBufferSize=iNewbufSize;
		SafeDeleteArray(m_pVideoDataBuffer);
		m_pVideoDataBuffer=pNewVideoBuffer;
	}
	catch (...) 
	{
		ASSERT (FALSE);
		return false; 
	}

	return true;
}

//��������
void CGameVideoItemSink::ResetVideoItem()
{
	//���ñ���
	m_iCurPos	= 0;	
	m_iBufferSize  = 0;
	SafeDeleteArray(m_pVideoDataBuffer);
}

//¼����
VOID CGameVideoItemSink::BuildVideoNumber(CHAR szVideoNumber[], WORD wLen,WORD wServerID,WORD wTableID)
{
	//��ȡʱ��
	SYSTEMTIME SystemTime;
	GetLocalTime(&SystemTime); 

	//��ʽ�����
	_snprintf(szVideoNumber,wLen,"%04d%02d%02d%02d%02d%02d%03d%03d",SystemTime.wYear,SystemTime.wMonth,SystemTime.wDay,
		SystemTime.wHour,SystemTime.wMinute,SystemTime.wSecond,wServerID,wTableID);
}