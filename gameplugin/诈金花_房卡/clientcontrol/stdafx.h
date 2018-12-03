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
#define _WIN32_WINDOWS 0x0410
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
#include <AtlBase.h>
#include <AtlDbcli.h>
#include <AfxDtctl.h>

//////////////////////////////////////////////////////////////////////////////////
//#pragma comment (lib,"../../../../系统模块/链接库/Unicode/WHImage.lib")
//#pragma comment (lib,"../../../../系统模块/链接库/Unicode/GameFrame.lib")
//#pragma comment (lib,"../../../../系统模块/链接库/Unicode/GameEngine.lib")
//#pragma comment (lib,"../../../../系统模块/链接库/Unicode/SkinControl.lib")
//#pragma comment (lib,"../../../../系统模块/链接库/Unicode/ServiceCore.lib")
//#pragma comment (lib,"../../../../系统模块/链接库/Unicode/ShareControl.lib")

//组件头文件
#include "resource.h"
#include "..\..\support\用户服务\UserServiceHead.h"
#include "..\..\support\界面控件\SkinControlHead.h"
#include "..\..\support\游戏控件\ShareControlHead.h"
#include "..\..\support\misc\ClientControl.h"
#include "..\msgdef\cmd_zajinhua.h"