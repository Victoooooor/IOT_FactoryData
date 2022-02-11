// SocketServerDlg.h : header file
//

#pragma once
#pragma pack(1)

#include <iostream>
#include <fstream>
#include <unordered_map>
#include "afxwin.h"


#define     NETWORK_EVENT     WM_USER+166     //���������¼�

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

	SOCKET ClientSock[CLNT_MAX_NUM]; //�洢��ͻ���ͨ�ŵ�Socket������
	std::unordered_map<SOCKET, std::ofstream> ClientRaw;
	std::unordered_map<SOCKET, std::wofstream> ClientData;
	userinfo uinfo[CLNT_MAX_NUM];

	/*���������첽�¼��Ĵ�����*/
	void OnClose(WPARAM wParam);      //�Զ�Socket�Ͽ�
	void OnSend(SOCKET CurSock);      //�����������ݰ�
	void OnReceive(SOCKET CurSock);   //�������ݰ�����
	void OnAccept(SOCKET CurSock);    //�ͻ�����������
	BOOL InitNetwork();               //��ʼ�����纯��
	afx_msg LRESULT OnNetEvent(WPARAM wParam, LPARAM lParam);  //�첽�¼��ص�����
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

	// ���뺯��
	int CheckSum(byte* sent, unsigned short recvd);
	int ReceiveCnt;
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	// ���Ӵ�������ʱ��
	int m_SSec, m_SMin, m_SHour;
	int m_VStatus;	// ��ǰ״̬ 0����ʼ״̬ 1������1000 2��С��1000


	afx_msg void OnEnChangeEdit1();
	afx_msg void OnLbnSelchangeList1();
};
