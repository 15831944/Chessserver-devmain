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
#include "..\..\..\server\gameservice\GameServiceHead.h"
//���ͷ�ļ�
#include "..\msgdef\CMD_Baccarat.h"

//#pragma comment (lib,"WHImage.lib")
//#pragma comment (lib,"GameFrame.lib")
//#pragma comment (lib,"GameEngine.lib")
//#pragma comment (lib,"SkinControl.lib")
//#pragma comment (lib,"ServiceCore.lib")
//////////////////////////////////////////////////////////////////////////