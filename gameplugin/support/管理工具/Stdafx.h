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

//MFC 文件
#include <AfxWin.h>
#include <AfxExt.h>
#include <AfxCmn.h>
#include <AfxDisp.h>

//MFC 文件
#include <Nb30.h>
#include <AfxInet.h>
#include <afxdtctl.h>
//////////////////////////////////////////////////////////////////////////////////
//平台文件

//平台文件
#include "..\..\..\server\global\Platform.h"

//网络命令
#include "..\..\..\server\msgdef\CMD_Commom.h"
#include "..\..\..\server\msgdef\CMD_GameServer.h"
#include "..\..\..\server\msgdef\CMD_LogonServer.h"

//进程命令
#include "..\..\..\server\msgdef\IPC_GameFrame.h"

//组件文件
#include "..\..\..\server\servicecore\ServiceCoreHead.h"
#include "..\..\..\server\process\ProcessControlHead.h"

//组件文件
#include "..\图像控件\WHImageHead.h"
#include "..\下载组件\DownLoadHead.h"
#include "..\界面控件\SkinControlHead.h"
#include "..\用户服务\UserServiceHead.h"
#include "..\平台数据\PlatformDataHead.h"
#include "..\游戏控件\ShareControlHead.h"
#include "..\游戏道具\GamePropertyHead.h"
#include "..\形象组件\AvatarControlHead.h"
#include "..\游戏引擎\GameEngineHead.h"

//////////////////////////////////////////////////////////////////////////////////
//链接代码

////多媒体库
#pragma comment (lib,"Winmm.lib")
//
//	#pragma comment (lib,"../../链接库/Unicode/WHImage.lib")
//	#pragma comment (lib,"../../链接库/Unicode/FGuilib.lib")
//	#pragma comment (lib,"../../链接库/Unicode/DownLoad.lib")
//	#pragma comment (lib,"../../链接库/Unicode/ServiceCore.lib")
//	#pragma comment (lib,"../../链接库/Unicode/SkinControl.lib")
//	#pragma comment (lib,"../../链接库/Unicode/GameProperty.lib")
//	#pragma comment (lib,"../../链接库/Unicode/ShareControl.lib")
//	#pragma comment (lib,"../../链接库/Unicode/PlatformData.lib")
//	#pragma comment (lib,"../../链接库/Unicode/AvatarControl.lib")
//	#pragma comment (lib,"../../链接库/Unicode/GameEngine.lib")
//	#pragma comment (lib,"../../链接库/Unicode/FGuimfcFactory.lib")	

//////////////////////////////////////////////////////////////////////////////////
