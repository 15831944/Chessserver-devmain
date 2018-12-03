#pragma once

//��Ϸ��Ƶ����
class IGameVideo
{
public:
	IGameVideo(void){};
	virtual ~IGameVideo(void){};

public:
	//��ʼ¼��
	virtual bool __cdecl	StartVideo(ITableFrame	*pTableFrame)		= NULL;
	//ֹͣ�ͱ���
	virtual bool __cdecl	StopAndSaveVideo(WORD wServerID,WORD wTableID)	= NULL;
	//����¼������
	virtual bool __cdecl    AddVideoData(WORD wMsgKind,Video_GameStart *pVideoGameStart,bool first)		= NULL;
	virtual bool __cdecl    AddVideoData(WORD wMsgKind,CMD_S_GiveUp *pVideoGiveUp)			= NULL;
	virtual bool __cdecl    AddVideoData(WORD wMsgKind,CMD_S_AddScore *pVideoAddScore)			= NULL;
	virtual bool __cdecl    AddVideoData(WORD wMsgKind,CMD_S_SendCard *pVideoSendCard)			= NULL;
	virtual bool __cdecl    AddVideoData(WORD wMsgKind,CMD_S_GameEnd *pVideoGameEnd)			= NULL;
};
