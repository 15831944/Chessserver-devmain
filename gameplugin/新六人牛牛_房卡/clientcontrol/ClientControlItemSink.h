#pragma once
#include "ClientControl.h"
#include "../../support/misc/ClientControl.h"

typedef enum
{
	QUERY_USER_GAMEID,
	QUERY_USER_NICKNAME,
	QUERY_INVALID
}QUERY_TYPE;

class AFX_EXT_CLASS CClientControlItemSinkDlg : public IClientControlDlg, public IClientControl
{
	DECLARE_DYNAMIC(CClientControlItemSinkDlg)

protected:
	CWnd *										m_pParentWnd;					// 父窗口
	IClientControlCallback *					m_pIClientControlCallback;		// 回调接口

	LONGLONG				m_lMaxRoomStorage[2];						//库存上限
	WORD					m_wRoomStorageMul[2];						//赢分概率
	LONGLONG				m_lRoomStorageCurrent;						//库存数值
	LONGLONG				m_lRoomStorageDeduct;						//库存衰减

    DWORD					m_dwQueryUserGameID;						//查询GAMEID
	TCHAR					m_szQueryUserNickName[LEN_NICKNAME];	//查询用户昵称
	QUERY_TYPE				m_QueryType;							//查询用户的类型

public:
	//控件变量
	CSkinEdit				m_editCurrentStorage;
	CSkinEdit				m_editStorageDeduct;
	CSkinEdit				m_editStorageMax1;
	CSkinEdit				m_editStorageMul1;
	CSkinEdit				m_editStorageMax2;
	CSkinEdit				m_editStorageMul2;
	CSkinEdit				m_editUserID;
	CSkinEdit				m_editControlCount;
	CSkinButton				m_btUpdateStorage;
	CSkinButton				m_btModifyStorage;
	CSkinButton				m_btContinueWin;
	CSkinButton				m_btContinueLost;
	CSkinButton				m_btContinueCancel;
	CSkinButton				m_btQueryUser;
	CSkinRichEdit			m_richEditUserDescription;
	CSkinRichEdit			m_richEditUserControl;
	CSkinRichEdit			m_richEditOperationRecord;

public:
	CClientControlItemSinkDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CClientControlItemSinkDlg();

// 对话框数据
	enum { IDD = IDD_CLIENT_CONTROL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
	// 继承函数
public:
	// 释放接口
	virtual void Release();
	// 创建函数
	virtual bool Create(CWnd * pParentWnd, IClientControlCallback * pIClientControlCallback);
	// 显示窗口
	virtual bool ShowWindow(bool bShow);
	// 消息函数
	virtual bool OnControlMessage(WORD nMessageID,WORD wTableID, void * pData, WORD nSize);

public:
	//设置房间库存
	virtual void SetRoomStorage(LONGLONG lRoomStartStorage, LONGLONG lRoomCurrentStorage);
	virtual void SetStorageInfo(CMD_S_ADMIN_STORAGE_INFO *pStorageInfo);
	//查询用户结果
	virtual void RequestQueryResult(CMD_S_RequestQueryResult *pQueryResult);
	//房间用户控制结果
	virtual void RoomUserControlResult(CMD_S_UserControl *pUserControl);
	//用户控制完成
	virtual void UserControlComplete(CMD_S_UserControlComplete *pUserControlComplete);
	//操作记录
	virtual void UpdateOperationRecord(CMD_S_Operation_Record *pOperation_Record);
	//设置更新定时器
	virtual void SetUpdateIDI(); 
	//更新房间信息
	virtual void UpdateRoomInfoResult(CMD_S_RequestUpdateRoomInfo_Result *RoomInfo_Result);
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	afx_msg void  OnCancel();

	//修改库存上限
	afx_msg void OnModifyStorage();
	//更新库存
	afx_msg void OnUpdateStorage();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnEnSetfocusEditUserId();
	afx_msg void OnContinueControlWin();
	afx_msg void OnContinueControlLost();
	afx_msg void OnContinueControlCancel();
	afx_msg void OnBnClickedBtnUserBetQuery();
	afx_msg void OnQueryUserGameID();
	afx_msg void OnQueryUserNickName();
	afx_msg void OnTimer(UINT_PTR nIDEvent);

//功能函数
public:
	//获取用户状态
	void GetUserStatusString(CMD_S_RequestQueryResult *pQueryResult, CString &userStatus);
	//获取游戏状态
	void GetGameStatusString(CMD_S_RequestQueryResult *pQueryResult, CString &gameStatus);
	//获取是否满足控制
	void GetSatisfyControlString(CMD_S_RequestQueryResult *pQueryResult, CString &satisfyControl, bool &bEnableControl);
	//获取控制类型
	void GetControlTypeString(CONTROL_TYPE &controlType, CString &controlTypestr);
	// 控制信息
	bool SendControlMessage(UINT nMessageID, void * pData = NULL, UINT nSize = 0);
};
