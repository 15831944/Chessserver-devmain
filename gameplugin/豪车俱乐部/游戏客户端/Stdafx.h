﻿//#pragma once
//
////////////////////////////////////////////////////////////////////////////
//
//#ifndef VC_EXTRALEAN
//#define VC_EXTRALEAN
//#endif
//
//#ifndef WINVER
//#define WINVER 0x0500
//#endif
//
//#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x0400
//#endif
//
//#ifndef _WIN32_WINDOWS
//#define _WIN32_WINDOWS 0x0410
//#endif
//
//#ifndef _WIN32_IE
//#define _WIN32_IE 0x0400
//#endif
//
//#define _ATL_ATTRIBUTES
//#define _AFX_ALL_WARNINGS
//#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
//
////////////////////////////////////////////////////////////////////////////
//
////MFC 文件
//#include <AfxWin.h>
//#include <AfxExt.h>
//#include <AfxCmn.h>
//#include <AfxDisp.h>
//#include <AtlBase.h>
//#include <AtlDbcli.h>
//#include <AfxDtctl.h>
//#include <strsafe.h>
//#pragma warning( disable : 4995 )
//
////////////////////////////////////////////////////////////////////////////
//
////配置文件
//#include "GameOption.h"
//
////////////////////////////////////////////////////////////////////////////
////链接代码
//
//#define SDK_CONDITION
//#ifdef SDK_CONDITION
//
////////////////////////////////////////////////////////////////////////////////////
////开发环境
//#include "..\消息定义\CMD_ZodiacBattle.h"
//#include "..\..\..\开发库\Include\GameFrameHead.h"
//
//#ifndef _DEBUG
//#ifndef _UNICODE
//#pragma comment (lib,"../../../开发库/Lib/Ansi/WHImage.lib")
////#pragma comment (lib,"../../../开发库/Lib/Ansi/D3DEngine.lib")
//#pragma comment (lib,"../../../开发库/Lib/Ansi/GameFrame.lib")
//#pragma comment (lib,"../../../开发库/Lib/Ansi/SkinControl.lib")
//#else
//#pragma comment (lib,"../../../开发库/Lib/Unicode/WHImage.lib")
////#pragma comment (lib,"../../../开发库/Lib/Unicode/D3DEngine.lib")
//#pragma comment (lib,"../../../开发库/Lib/Unicode/GameFrame.lib")
//#pragma comment (lib,"../../../开发库/Lib/Unicode/SkinControl.lib")
//#endif
//#else
//#ifndef _UNICODE
//#pragma comment (lib,"../../../开发库/Lib/Ansi/WHImageD.lib")
////#pragma comment (lib,"../../../开发库/Lib/Ansi/D3DEngineD.lib")
//#pragma comment (lib,"../../../开发库/Lib/Ansi/GameFrameD.lib")
//#pragma comment (lib,"../../../开发库/Lib/Ansi/SkinControlD.lib")
//#else
//#pragma comment (lib,"../../../开发库/Lib/Unicode/WHImageD.lib")
////#pragma comment (lib,"../../../开发库/Lib/Unicode/D3DEngineD.lib")
//#pragma comment (lib,"../../../开发库/Lib/Unicode/GameFrameD.lib")
//#pragma comment (lib,"../../../开发库/Lib/Unicode/SkinControlD.lib")
//#endif
//#endif
//
////////////////////////////////////////////////////////////////////////////////////
//
//#else
//
////////////////////////////////////////////////////////////////////////////////////
////系统环境
//
//#include "..\消息定义\CMD_ZodiacBattle.h"
//#include "..\..\..\开发库\Include\GameFrameHead.h"
//
//#ifndef _DEBUG
//#ifndef _UNICODE
//#pragma comment (lib,"../../../开发库/Lib/Ansi/WHImage.lib")
////#pragma comment (lib,"../../../../开发库/Lib/Ansi/D3DEngine.lib")
//#pragma comment (lib,"../../../开发库/Lib/Ansi/GameFrame.lib")
//#pragma comment (lib,"../../../开发库/Lib/Ansi/SkinControl.lib")
//#else
//#pragma comment (lib,"../../../开发库/Lib/Unicode/WHImage.lib")
////#pragma comment (lib,"../../../开发库/Lib/Unicode/D3DEngine.lib")
//#pragma comment (lib,"../../../开发库/Lib/Unicode/GameFrame.lib")
//#pragma comment (lib,"../../../开发库/Lib/Unicode/SkinControl.lib")
//#endif
//#else
//#ifndef _UNICODE
//#pragma comment (lib,"../../../开发库/Lib/Ansi/WHImageD.lib")
////#pragma comment (lib,"../../../开发库/Lib/Ansi/D3DEngineD.lib")
//#pragma comment (lib,"../../../开发库/Lib/Ansi/GameFrameD.lib")
//#pragma comment (lib,"../../../开发库/Lib/Ansi/SkinControlD.lib")
//#else
//#pragma comment (lib,"../../../开发库/Lib/Unicode/WHImageD.lib")
////#pragma comment (lib,"../../../开发库/Lib/Unicode/D3DEngineD.lib")
//#pragma comment (lib,"../../../开发库/Lib/Unicode/GameFrameD.lib")
//#pragma comment (lib,"../../../开发库/Lib/Unicode/SkinControlD.lib")
//#endif
//#endif
//
////////////////////////////////////////////////////////////////////////////////////
//
//#endif



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
#define _WIN32_WINDOWS 0x0501
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
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
#include <AtlBase.h>
#include <AtlDbcli.h>
#include <AfxDtctl.h>

//////////////////////////////////////////////////////////////////////////////////


//其他文件
#include "Other\SkinSB.h"
#include "Other\Detours.h"

//图片控件
#import "ImageOle.dll" named_guids

//////////////////////////////////////////////////////////////////////////////////
//链接代码

#pragma comment (lib,"Other/SkinSB.lib")
#pragma comment (lib,"Other/Detours.lib")

//编译环境
#include "..\..\..\SDKCondition.h"

//////////////////////////////////////////////////////////////////////////////////
#ifdef SDK_CONDITION

//////////////////////////////////////////////////////////////////////////////////
//开发环境

#include "..\..\..\开发库\Include\GameFrameHead.h"

#ifndef _DEBUG
#ifndef _UNICODE
#pragma comment (lib,"../../../开发库/Lib/Ansi/WHImage.lib")
#pragma comment (lib,"../../../开发库/Lib/Ansi/GameFrame.lib")
#pragma comment (lib,"../../../开发库/Lib/Ansi/GameEngine.lib")
#pragma comment (lib,"../../../开发库/Lib/Ansi/SkinControl.lib")
#pragma comment (lib,"../../../开发库/Lib/Ansi/ShareControl.lib")
#pragma comment (lib,"../../../开发库/Lib/Ansi/ServiceCore.lib")
#pragma comment (lib,"../../../开发库/Lib/Ansi/GameProperty.lib")

#else
#pragma comment (lib,"../../../开发库/Lib/Unicode/WHImage.lib")
#pragma comment (lib,"../../../开发库/Lib/Unicode/GameFrame.lib")
#pragma comment (lib,"../../../开发库/Lib/Unicode/GameEngine.lib")
#pragma comment (lib,"../../../开发库/Lib/Unicode/SkinControl.lib")
#pragma comment (lib,"../../../开发库/Lib/Unicode/ShareControl.lib")
#pragma comment (lib,"../../../开发库/Lib/Unicode/ServiceCore.lib")
#pragma comment (lib,"../../../开发库/Lib/Unicode/GameProperty.lib")
#endif
#else
#ifndef _UNICODE
#pragma comment (lib,"../../../开发库/Lib/Ansi/WHImageD.lib")
#pragma comment (lib,"../../../开发库/Lib/Ansi/GameFrameD.lib")
#pragma comment (lib,"../../../开发库/Lib/Ansi/GameEngineD.lib")
#pragma comment (lib,"../../../开发库/Lib/Ansi/SkinControlD.lib")
#pragma comment (lib,"../../../开发库/Lib/Ansi/ShareControlD.lib")
#pragma comment (lib,"../../../开发库/Lib/Ansi/ServiceCoreD.lib")
#pragma comment (lib,"../../../开发库/Lib/Ansi/GamePropertyD.lib")

#else
#pragma comment (lib,"../../../开发库/Lib/Unicode/WHImageD.lib")
#pragma comment (lib,"../../../开发库/Lib/Unicode/GameFrameD.lib")
#pragma comment (lib,"../../../开发库/Lib/Unicode/GameEngineD.lib")
#pragma comment (lib,"../../../开发库/Lib/Unicode/SkinControlD.lib")
#pragma comment (lib,"../../../开发库/Lib/Unicode/ShareControlD.lib")
#pragma comment (lib,"../../../开发库/Lib/Unicode/ServiceCoreD.lib")
#pragma comment (lib,"../../../开发库/Lib/Unicode/GamePropertyD.lib")
#endif
#endif

//////////////////////////////////////////////////////////////////////////////////

#else

//////////////////////////////////////////////////////////////////////////////////
//系统环境

#include "..\..\..\..\系统模块\客户端组件\游戏框架\GameFrameHead.h"

#ifndef _DEBUG
#ifndef _UNICODE
#pragma comment (lib,"../../../../系统模块/链接库/Ansi/WHImage.lib")
#pragma comment (lib,"../../../../系统模块/链接库/Ansi/GameFrame.lib")
#pragma comment (lib,"../../../../系统模块/链接库/Ansi/GameEngine.lib")
#pragma comment (lib,"../../../../系统模块/链接库/Ansi/SkinControl.lib")
#pragma comment (lib,"../../../../系统模块/链接库/Ansi/ServiceCore.lib")
#else
#pragma comment (lib,"../../../../系统模块/链接库/Unicode/WHImage.lib")
#pragma comment (lib,"../../../../系统模块/链接库/Unicode/GameFrame.lib")
#pragma comment (lib,"../../../../系统模块/链接库/Unicode/GameEngine.lib")
#pragma comment (lib,"../../../../系统模块/链接库/Unicode/SkinControl.lib")
#pragma comment (lib,"../../../../系统模块/链接库/Unicode/ServiceCore.lib")
#endif
#else
#ifndef _UNICODE
#pragma comment (lib,"../../../../系统模块/链接库/Ansi/WHImageD.lib")
#pragma comment (lib,"../../../../系统模块/链接库/Ansi/GameFrameD.lib")
#pragma comment (lib,"../../../../系统模块/链接库/Ansi/GameEngineD.lib")
#pragma comment (lib,"../../../../系统模块/链接库/Ansi/SkinControlD.lib")
#pragma comment (lib,"../../../../系统模块/链接库/Ansi/ServiceCoreD.lib")
#else
#pragma comment (lib,"../../../../系统模块/链接库/Unicode/WHImageD.lib")
#pragma comment (lib,"../../../../系统模块/链接库/Unicode/GameFrameD.lib")
#pragma comment (lib,"../../../../系统模块/链接库/Unicode/GameEngineD.lib")
#pragma comment (lib,"../../../../系统模块/链接库/Unicode/SkinControlD.lib")
#pragma comment (lib,"../../../../系统模块/链接库/Unicode/ServiceCoreD.lib")
#endif
#endif

//////////////////////////////////////////////////////////////////////////////////

#endif

#define  __TEST__  1
#define  __BANKER___
#define  __SPECIAL___

//////////////////////////////////////////////////////////////////////////
#include "..\消息定义\CMD_ZodiacBattle.h"


//////////////////////////////////////////////////////////////////////////
