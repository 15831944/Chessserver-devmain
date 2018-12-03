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

//MFC �ļ�
#include <AfxWin.h>
#include <AfxExt.h>
#include <AfxCmn.h>
#include <AfxDisp.h>
#include <AtlBase.h>
#include <AtlDbcli.h>
#include <AfxDtctl.h>

//////////////////////////////////////////////////////////////////////////////////
//#pragma comment (lib,"../../../../ϵͳģ��/���ӿ�/Unicode/WHImage.lib")
//#pragma comment (lib,"../../../../ϵͳģ��/���ӿ�/Unicode/GameFrame.lib")
//#pragma comment (lib,"../../../../ϵͳģ��/���ӿ�/Unicode/GameEngine.lib")
//#pragma comment (lib,"../../../../ϵͳģ��/���ӿ�/Unicode/SkinControl.lib")
//#pragma comment (lib,"../../../../ϵͳģ��/���ӿ�/Unicode/ServiceCore.lib")
//#pragma comment (lib,"../../../../ϵͳģ��/���ӿ�/Unicode/ShareControl.lib")

//���ͷ�ļ�
#include "resource.h"
#include "..\..\support\�û�����\UserServiceHead.h"
#include "..\..\support\����ؼ�\SkinControlHead.h"
#include "..\..\support\��Ϸ�ؼ�\ShareControlHead.h"
#include "..\..\support\misc\ClientControl.h"
#include "..\msgdef\cmd_zajinhua.h"