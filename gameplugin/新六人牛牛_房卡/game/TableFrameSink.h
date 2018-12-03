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
	TCHAR szModuleDirectory[MAX_PATH];	//ģ��Ŀ¼
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

//��Ϸ������
class CTableFrameSink : public ITableFrameSink, public ITableUserAction
{
	//��Ϸ����
protected:
	WORD							m_wBankerUser;							//ׯ���û�
	WORD							m_wFirstEnterUser;						//�׽����CHAIRID (�԰���ׯ��Ч)
	LONGLONG						m_lExitScore;							//ǿ�˷���

	//�û�����
protected:
	BYTE                            m_cbDynamicJoin[GAME_PLAYER];           //��̬����
	BYTE							m_cbPlayStatus[GAME_PLAYER];			//��Ϸ״̬
	BYTE							m_cbCallBankerStatus[GAME_PLAYER];		//��ׯ״̬
	BYTE							m_cbCallBankerTimes[GAME_PLAYER];		//��ׯ����

	bool							m_bOpenCard[GAME_PLAYER];				//���Ʊ�ʶ
	LONGLONG						m_lTableScore[GAME_PLAYER];				//��ע��Ŀ
	bool							m_bBuckleServiceCharge[GAME_PLAYER];	//�շ����

	//�˿˱���
protected:
	BYTE							m_cbHandCardData[GAME_PLAYER][MAX_CARDCOUNT];//�����˿�
	bool							m_bSpecialCard[GAME_PLAYER];			//�������ͱ�־ ���Ļ�ţ���廨ţ��˳�ӣ�ͬ������«��ը����ͬ��˳����Сţ��
	BYTE							m_cbOriginalCardType[GAME_PLAYER];		//���ԭʼ���ͣ�û�о��������ϵ����ͣ�
	BYTE							m_cbCombineCardType[GAME_PLAYER];       //���������ͣ����������ϵ����ͣ�

	//��ע��Ϣ
protected:
	LONGLONG						m_lTurnMaxScore[GAME_PLAYER];			//�����ע

	//�������
protected:
	CGameLogic						m_GameLogic;							//��Ϸ�߼�
	ITableFrame						* m_pITableFrame;						//��ܽӿ�
	CHistoryScore					m_HistoryScore;							//��ʷ�ɼ�
	tagGameServiceOption		    *m_pGameServiceOption;					//���ò���
	tagGameServiceAttrib			*m_pGameServiceAttrib;					//��Ϸ����

	//���Ա���
protected:
	WORD							m_wPlayerCount;							//��Ϸ����
	
	//��Ϸ����
protected:
	CARDTYPE_CONFIG					m_ctConfig;
	SENDCARDTYPE_CONFIG				m_stConfig;
	KING_CONFIG						m_gtConfig;
	BANERGAMETYPE_CONFIG			m_bgtConfig;
	BETTYPE_CONFIG					m_btConfig;

	//�������ö��(��Чֵ0)
	LONG							m_lFreeConfig[MAX_CONFIG];

	//�ٷֱ����ö��(��Чֵ0)
	LONG							m_lPercentConfig[MAX_CONFIG];

	LONG							m_lMaxCardTimes;						//���������
	
	CMD_S_RECORD					m_stRecord;								//��Ϸ��¼

	CMD_S_RoomCardRecord			m_RoomCardRecord;						//������Ϸ��¼

#ifdef CARD_CONFIG
	BYTE							m_cbconfigCard[GAME_PLAYER][MAX_CARDCOUNT];	//�����˿�
#endif

	//�������
protected:
	HINSTANCE						m_hInst;								//���ƾ��
	IServerControl*					m_pServerControl;						//�������

	//��Ϸ��Ƶ
protected:
	HINSTANCE						m_hVideoInst;
	IGameVideo*						m_pGameVideo;

	//��������
public:
	//���캯��
	CTableFrameSink();
	//��������
	virtual ~CTableFrameSink();

	//�����ӿ�
public:
	//�ͷŶ���
	virtual VOID Release() {}
	//�ӿڲ�ѯ
	virtual void * QueryInterface(const IID & Guid, DWORD dwQueryVer);

	//����ӿ�
public:
	//��ʼ��
	virtual bool Initialization(IUnknownEx * pIUnknownEx);
	//��λ����
	virtual void RepositionSink();
	//��Ϸ�¼�
public:
	//��Ϸ��ʼ
	virtual bool OnEventGameStart();
	//��Ϸ����
	virtual bool OnEventGameConclude(WORD wChairID, IServerUserItem * pIServerUserItem, BYTE cbReason);
	//���ͳ���
	virtual bool OnEventSendGameScene(WORD wChairID, IServerUserItem * pIServerUserItem, BYTE bGameStatus, bool bSendSecret);


	//�¼��ӿ�
public:
	//��ʱ���¼�
	virtual bool OnTimerMessage(DWORD dwTimerID, WPARAM wBindParam);
	//��Ϸ��Ϣ����
	virtual bool OnGameMessage(WORD wSubCmdID, void * pDataBuffer, WORD wDataSize, IServerUserItem * pIServerUserItem);
	//�����Ϣ����
	virtual bool OnFrameMessage(WORD wSubCmdID, void * pDataBuffer, WORD wDataSize, IServerUserItem * pIServerUserItem);
	//�����¼�
	virtual bool OnGameDataBase(WORD wRequestID, VOID * pData, WORD wDataSize){return true;}

	//��ѯ�ӿ�
public:
	//��ѯ�޶�
	virtual SCORE QueryConsumeQuota(IServerUserItem * pIServerUserItem);
	//���ٻ���
	virtual SCORE QueryLessEnterScore(WORD wChairID, IServerUserItem * pIServerUserItem){return 0;}
	//��ѯ�����
	virtual bool QueryBuckleServiceCharge(WORD wChairID);
	//�����¼�
	virtual bool OnDataBaseMessage(WORD wRequestID, VOID * pData, WORD wDataSize){return false;}
	//�����¼�
	virtual bool OnUserScroeNotify(WORD wChairID, IServerUserItem * pIServerUserItem, BYTE cbReason){return false;}
	//���û���
	virtual void SetGameBaseScore(LONG lBaseScore){return;}


	//�û��¼�
public:
	//�û�����
	virtual bool OnActionUserOffLine(WORD wChairID, IServerUserItem * pIServerUserItem);
	//�û�����
	virtual bool OnActionUserConnect(WORD wChairID, IServerUserItem * pIServerUserItem) { return true; };
	//�û�����
	virtual bool OnActionUserSitDown(WORD wChairID, IServerUserItem * pIServerUserItem, bool bLookonUser);
	//�û�����
	virtual bool OnActionUserStandUp(WORD wChairID, IServerUserItem * pIServerUserItem, bool bLookonUser);
	//�û�ͬ��
	virtual bool OnActionUserOnReady(WORD wChairID, IServerUserItem * pIServerUserItem, VOID * pData, WORD wDataSize);

	//��Ϸ�¼�
protected:
	//��ׯ�¼�
	bool OnUserCallBanker(WORD wChairID, bool bBanker, BYTE cbBankerTimes);
	//��ע�¼�
	bool OnUserAddScore(WORD wChairID, LONGLONG lScore);
	//̯���¼�
	bool OnUserOpenCard(WORD wChairID, BYTE cbCombineCardData[MAX_CARDCOUNT]);
	//д�ֺ���
	bool TryWriteTableScore(tagScoreInfo ScoreInfoArray[]);

	//���ܺ���
protected:
	//�˿˷���
	void AnalyseCard(SENDCARDTYPE_CONFIG stConfig);
	//����·�
	SCORE GetUserMaxTurnScore(WORD wChairID);
	//�жϿ��
	bool JudgeStock();
	//�Ƿ�˥��
	bool NeedDeductStorage();
	//��ȡ����
	void ReadConfigInformation();
	//���·����û���Ϣ
	void UpdateRoomUserInfo(IServerUserItem *pIServerUserItem, USERACTION userAction);
	//����ͬ���û�����
	void UpdateUserControl(IServerUserItem *pIServerUserItem);
	//�����û�����
	void TravelControlList(ROOMUSERCONTROL keyroomusercontrol);
	//�Ƿ������������
	void IsSatisfyControl(ROOMUSERINFO &userInfo, bool &bEnableControl);
	//���������û�����
	bool AnalyseRoomUserControl(ROOMUSERCONTROL &Keyroomusercontrol, POSITION &ptList);
	//��ȡ��������
	void GetControlTypeString(CONTROL_TYPE &controlType, CString &controlTypestr);
	//д��־�ļ�
	void WriteInfo(LPCTSTR pszString);
	// ��¼����
	void WriteInfo(LPCTSTR pszFileName, LPCTSTR pszString);
	//�жϷ�������
	bool IsRoomCardType();
};

//////////////////////////////////////////////////////////////////////////

#endif
