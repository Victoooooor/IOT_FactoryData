// SocketServerDlg.h : header file
//

#pragma once
#pragma pack(1)

#include <iostream>
#include <fstream>
#include <unordered_map>
#include "afxwin.h"


#define     NETWORK_EVENT     WM_USER+166     //定义网络事件

#define     CLNT_MAX_NUM      255



// CSocketServerDlg dialog
class CSocketServerDlg : public CDialog
{
// Construction
public:
	CSocketServerDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_SOCKETSERVER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	enum serverStatus
	{
		STATUS,
		RECV
	};

	struct userinfo
	{
		CString username;
		CString userip;

	};

	struct pack {
		unsigned short State;
		short ADC[3];
	};

	CString m_serverPort;
	CString m_sendMsg;

	SOCKET ClientSock[CLNT_MAX_NUM]; //存储与客户端通信的Socket的数组
	std::unordered_map<SOCKET, std::ofstream> ClientRaw;
	std::unordered_map<SOCKET, std::wofstream> ClientData;
	userinfo uinfo[CLNT_MAX_NUM];

	/*各种网络异步事件的处理函数*/
	void OnClose(WPARAM wParam);      //对端Socket断开
	void OnSend(SOCKET CurSock);      //发送网络数据包
	void OnReceive(SOCKET CurSock);   //网络数据包到达
	void OnAccept(SOCKET CurSock);    //客户端连接请求
	BOOL InitNetwork();               //初始化网络函数
	afx_msg LRESULT OnNetEvent(WPARAM wParam, LPARAM lParam);  //异步事件回调函数
	CString m_status;
	CString	m_receiveData;
	int i;
	CEdit m_serverPortCtrl;
	CEdit m_serverSendMsgCtrl;
	afx_msg void OnBtnStartServer();
	afx_msg void OnBtnStopServer();
	afx_msg void OnBtnSendMsg();
	//void ShowWindowText(const CString &sMsg);
	void ShowWindowText(const CString &sMsg, serverStatus type);
	CButton m_StartServer;
	CButton m_StopServer;

	// 解码函数
	int CheckSum(byte* sent, unsigned short recvd);
	int ReceiveCnt;
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	// 离子传感器计时器
	int m_SSec, m_SMin, m_SHour;
	int m_VStatus;	// 当前状态 0：初始状态 1：大于1000 2：小于1000


	afx_msg void OnEnChangeEdit1();
	afx_msg void OnLbnSelchangeList1();
};
