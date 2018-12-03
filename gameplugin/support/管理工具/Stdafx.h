#pragma once

//////////////////////////////////////////////////////////////////////////////////

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#ifndef WINVER
#define WINVER 0x0501
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0400
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0400
#endif

#define _ATL_ATTRIBUTES
#define _AFX_ALL_WARNINGS
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS

//////////////////////////////////////////////////////////////////////////////////

//MFC �ļ�
#include <AfxWin.h>
#include <AfxExt.h>
#include <AfxCmn.h>
#include <AfxDisp.h>

//MFC �ļ�
#include <Nb30.h>
#include <AfxInet.h>
#include <afxdtctl.h>
//////////////////////////////////////////////////////////////////////////////////
//ƽ̨�ļ�

//ƽ̨�ļ�
#include "..\..\..\server\global\Platform.h"

//��������
#include "..\..\..\server\msgdef\CMD_Commom.h"
#include "..\..\..\server\msgdef\CMD_GameServer.h"
#include "..\..\..\server\msgdef\CMD_LogonServer.h"

//��������
#include "..\..\..\server\msgdef\IPC_GameFrame.h"

//����ļ�
#include "..\..\..\server\servicecore\ServiceCoreHead.h"
#include "..\..\..\server\process\ProcessControlHead.h"

//����ļ�
#include "..\ͼ��ؼ�\WHImageHead.h"
#include "..\�������\DownLoadHead.h"
#include "..\����ؼ�\SkinControlHead.h"
#include "..\�û�����\UserServiceHead.h"
#include "..\ƽ̨����\PlatformDataHead.h"
#include "..\��Ϸ�ؼ�\ShareControlHead.h"
#include "..\��Ϸ����\GamePropertyHead.h"
#include "..\�������\AvatarControlHead.h"
#include "..\��Ϸ����\GameEngineHead.h"

//////////////////////////////////////////////////////////////////////////////////
//���Ӵ���

////��ý���
#pragma comment (lib,"Winmm.lib")
//
//	#pragma comment (lib,"../../���ӿ�/Unicode/WHImage.lib")
//	#pragma comment (lib,"../../���ӿ�/Unicode/FGuilib.lib")
//	#pragma comment (lib,"../../���ӿ�/Unicode/DownLoad.lib")
//	#pragma comment (lib,"../../���ӿ�/Unicode/ServiceCore.lib")
//	#pragma comment (lib,"../../���ӿ�/Unicode/SkinControl.lib")
//	#pragma comment (lib,"../../���ӿ�/Unicode/GameProperty.lib")
//	#pragma comment (lib,"../../���ӿ�/Unicode/ShareControl.lib")
//	#pragma comment (lib,"../../���ӿ�/Unicode/PlatformData.lib")
//	#pragma comment (lib,"../../���ӿ�/Unicode/AvatarControl.lib")
//	#pragma comment (lib,"../../���ӿ�/Unicode/GameEngine.lib")
//	#pragma comment (lib,"../../���ӿ�/Unicode/FGuimfcFactory.lib")	

//////////////////////////////////////////////////////////////////////////////////
