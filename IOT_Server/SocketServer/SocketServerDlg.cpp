// SocketServerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SocketServer.h"
#include "SocketServerDlg.h"
#include "Convert.h"
#include <cstring>
#include <ctime>
#include <string>
#include <sstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CString res;	// ��ʾ�ܵĽ�����Ϣ

SOCKET ServerSock;  //��������Socket

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CSocketServerDlg dialog




CSocketServerDlg::CSocketServerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSocketServerDlg::IDD, pParent)
	, m_status(_T(""))
	, m_receiveData(_T(""))
	, m_serverPort(_T(""))
	, i(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	
	// ��ʼ�����Ӵ�����ʱ�����
	m_SSec = m_SMin = m_SHour = 0;
	CreateDirectory("./log",NULL);
	CreateDirectory("./log/RAW", NULL);
	CreateDirectory("./log/DATA", NULL);

}

void CSocketServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_STATIC_STATUS, m_status);
	DDX_Text(pDX, IDC_EDIT1, m_receiveData);
	DDX_Control(pDX, IDC_EDIT2, m_serverPortCtrl);
	DDX_Control(pDX, IDC_EDIT3, m_serverSendMsgCtrl);
	DDX_Control(pDX, IDC_BUTTON1, m_StartServer);
	DDX_Control(pDX, IDC_BUTTON2, m_StopServer);
}

BEGIN_MESSAGE_MAP(CSocketServerDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_MESSAGE(NETWORK_EVENT, OnNetEvent)
	ON_BN_CLICKED(IDC_BUTTON1, &CSocketServerDlg::OnBtnStartServer)
	ON_BN_CLICKED(IDC_BUTTON2, &CSocketServerDlg::OnBtnStopServer)
	ON_BN_CLICKED(IDC_BUTTON3, &CSocketServerDlg::OnBtnSendMsg)
	ON_WM_TIMER()
	ON_WM_VSCROLL()
	ON_EN_CHANGE(IDC_EDIT1, &CSocketServerDlg::OnEnChangeEdit1)
END_MESSAGE_MAP()


// CSocketServerDlg message handlers

BOOL CSocketServerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	for (int i = 0; i < CLNT_MAX_NUM; i++)
	{
		ClientSock[i] = INVALID_SOCKET;
	}

	// ����Ĭ�϶˿ں�
	SetDlgItemText(IDC_EDIT2, (CString)"27014");


	// �������Ӵ�������ʱ��
	//SetTimer(1, 1000, NULL);

	// �������Ӵ�������ʱ����ʼ״̬
	m_VStatus = 0;

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CSocketServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSocketServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CSocketServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CSocketServerDlg::InitNetwork()
{
	WSADATA wsaData;

	//��ʼ��TCPЭ��
	BOOL ret = WSAStartup(MAKEWORD(2,2), &wsaData);
	if(ret != 0)
	{
		ShowWindowText("��ʼ������Э��ʧ��!", CSocketServerDlg::STATUS);
		return FALSE;
	}

	//�������������׽���
    ServerSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(ServerSock == INVALID_SOCKET)
	{
		ShowWindowText("�����׽���ʧ��!", CSocketServerDlg::STATUS);
		closesocket(ServerSock);
		WSACleanup();
		return FALSE;
	}

	//�󶨵�����һ���˿���
	sockaddr_in localaddr;
	localaddr.sin_family = AF_INET;
	localaddr.sin_port = htons(atoi(m_serverPort));     //�˿ںŲ�Ҫ������Ӧ�ó����ͻ
	localaddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if(bind(ServerSock, (struct sockaddr*)&localaddr, sizeof(sockaddr)) == SOCKET_ERROR)
	{
		ShowWindowText("�󶨵�ַʧ��!", CSocketServerDlg::STATUS);
		closesocket(ServerSock);
		WSACleanup();
		return FALSE;
	}

	//��SeverSock����Ϊ�첽������ģʽ����Ϊ��ע����������첽�¼�������m_hWnd      
	//ΪӦ�ó�������Ի���������ڵľ��
	if(WSAAsyncSelect(ServerSock, m_hWnd, NETWORK_EVENT, FD_ACCEPT | FD_CLOSE | FD_READ | FD_WRITE) == SOCKET_ERROR)
	{
		ShowWindowText("ע�������첽�¼�ʧ��!", CSocketServerDlg::STATUS);
		WSACleanup();
		return FALSE;
	}
	listen(ServerSock, 5);  //��������ģʽ
	return TRUE;
}

//���涨�������첽�¼��Ļص�����
LRESULT CSocketServerDlg::OnNetEvent(WPARAM wParam, LPARAM lParam)
{
	//����Winsock API�������õ������¼�����
	int iEvent = WSAGETSELECTEVENT(lParam);
	//����Winsock API�������õ��������¼��Ŀͻ����׽���
	SOCKET CurSock = (SOCKET)wParam;

	switch(iEvent)
	{
	case FD_ACCEPT:         //�ͻ������������¼�
		OnAccept(CurSock);
		break;
	case FD_CLOSE:          //�ͻ��˶Ͽ��¼�:
		OnClose(wParam);
		break;
	case FD_READ:           //�������ݰ������¼�
		OnReceive(CurSock);
		break;
	case FD_WRITE:         //�������������¼�
		OnSend(CurSock);
		break;
	default: break;
	}

	return 0;
}

void CSocketServerDlg::OnAccept(SOCKET CurSock)
{
	//�����������󣬲������뷢����������Ŀͻ��˽���ͨ��Socket
	//Ϊ�µ�socketע���첽�¼���ע��û��Accept�¼�

	int nLength = sizeof(SOCKADDR);
	SOCKADDR_IN addrClient;
	
	for(i = 0; (i < CLNT_MAX_NUM) && (ClientSock[i] != INVALID_SOCKET); i++) ;
	if(i == CLNT_MAX_NUM)      
		return ;
	ClientSock[i] = accept(CurSock, (LPSOCKADDR)&addrClient, (LPINT)&nLength);
	ClientRaw[CurSock] = std::ofstream();

	uinfo[i].userip = inet_ntoa(addrClient.sin_addr);
	WSAAsyncSelect(ClientSock[i], m_hWnd, NETWORK_EVENT, FD_READ|FD_CLOSE);
	CString status = uinfo[i].userip + "���ӷ�����\n";
	ShowWindowText(status, CSocketServerDlg::STATUS);
}


void CSocketServerDlg::OnClose(WPARAM wParam)
{
	//��������Ӧ�Ŀͻ��˵�ͨ�ţ��ͷ���Ӧ��Դ
	for(i = 0; (i < CLNT_MAX_NUM) && (ClientSock[i] != wParam); i++) ;
	ClientRaw[ClientSock[i]].close();
	ClientData[ClientSock[i]].close();
	ClientRaw.erase(ClientSock[i]);
	ClientData.erase(ClientSock[i]);
	closesocket(ClientSock[i]);
	ClientSock[i] = INVALID_SOCKET;

	CString status = "�û�" + uinfo[i].username + "��ip" + uinfo[i].userip + "�˳�������";
	ShowWindowText(status, CSocketServerDlg::STATUS);
	
	uinfo[i].userip   = "";
	uinfo[i].username = "";	
}

void CSocketServerDlg::OnSend(SOCKET CurSock)
{
	//�ڸ��ͻ��˷�����ʱ�����Ԥ����
}

void CSocketServerDlg::OnReceive(SOCKET CurSock)
{
	//�������绺�����е����ݰ�
	char szText[1024] = {0};

	int Vol, cnt = 0;
	cnt = recv(CurSock, (char*)&szText, 1024, 0);
	if (cnt == SOCKET_ERROR)
		ShowWindowText("���յ�һ��������Ϣ!", CSocketServerDlg::RECV);
	
	int offSet = 0;
	unsigned short* tmp = (unsigned short*)szText;
	while (*tmp != 0xFFFF) {
		tmp++;
		offSet++;
	}
	unsigned short* device_id = (unsigned short*)(szText + 2);
	
	std::ofstream* RAW = &(ClientRaw[CurSock]);
	if (!RAW->is_open()) {
		time_t t_raw;
		time(&t_raw);
		struct tm* timeinfo;
		char buff[128];
		std::ostringstream fname;

		timeinfo = localtime(&t_raw);
		strftime(buff, 128, "%Y_%m_%d-", timeinfo);
		fname << buff << std::to_string(*device_id) << ".txt";
		RAW->open("./log/RAW/" + fname.str(), std::ofstream::app | std::ofstream::binary);

		ClientData[CurSock] = std::wofstream("./log/DATA/" + fname.str(), std::wostream::app);	
	}

	(*RAW) << szText;
	RAW->flush();
	offSet += 4;
	struct pack* haas100data; // ���հ�
	unsigned short* datalen; // ���ݳ���
	char rawlen[2] = { 0 };
	memcpy(rawlen, szText + offSet, 2);
	datalen = (unsigned short*)&rawlen;
	offSet += 2;
	char send_buf[128] = { 0 };
	for (int k = 0; k < *datalen; k += 8) {
		// ����ת��
		haas100data = (pack*)(szText + offSet + k);
		Vol = haas100data->ADC[0];
		CString receiveData;	// ����Buff
		CString str[4];

		switch (haas100data->State)
		{
		case 0:	 //M-520-2 //NK-7001
			break;
		case 1: //HD-1202E
			break;
		case 2:
			break;
		case 4:
			if (Vol > 1000) {

				m_SHour = m_SMin = m_SSec = 0;
				SetDlgItemText(IDC_STimer, (CString)"0:0:0");
				if (!m_VStatus) m_VStatus = 1;
			}
			if (m_VStatus == 1 && abs(Vol) <= 1000) {
				m_VStatus = 2;
				SetTimer(1, 1000, NULL);
			}
			else if (m_VStatus == 2 && abs(Vol) <= 100) {
				m_VStatus = 0;
				KillTimer(1);
				snprintf(send_buf, 128, "{\"dsp\":\"%dm, %ds\"}\0", m_SMin,m_SSec);
				if(send(CurSock, send_buf, strlen(send_buf), 0) == SOCKET_ERROR)
				{
					CString sSendError;
					sSendError.Format("Server Socket failed to send: %d", GetLastError());
					ShowWindowText(sSendError, CSocketServerDlg::STATUS);
				}
			}
			break;
		default:
			;
		}

		str[1].Format("%d", Vol);
		str[2].Format("%d", haas100data->ADC[1]);
		str[3].Format("%d", haas100data->ADC[2]);
		str[0].Format("%d", haas100data->State);
		receiveData += "ip: " + uinfo[i].userip + "����: " + CString(str[0]) + " " + CString(str[1]) + " " + CString(str[2]) \
			+ " " + CString(str[3]);
		ClientData[CurSock] << (LPCTSTR)receiveData<< std::endl;

		// У������
		int sumed = CheckSum((byte*)szText, 6 + *datalen + (4 + *datalen) / 4);
		if (!sumed) {
			res += receiveData + "\r\n";
			ReceiveCnt++;
			GetDlgItem(IDC_EDIT1)->SetWindowTextA(res + "\r\n");
			
		}
		else {
			str[1].Format("%d", sumed);
			GetDlgItem(IDC_EDIT1)->SetWindowTextA("error: "+ str[1] + " \r\n");
		}
		if ((ReceiveCnt & 1) == 0) res = "";
		
		// ������������/�ر� Timer
		// State specific action
		// Switch State parser
		

	}
	
}  

void CSocketServerDlg::ShowWindowText(const CString &sMsg, serverStatus type)
{
	CTime time = CTime::GetCurrentTime();
	CString strTime = "(%y-%m-%d %H:%M:%S)";
	strTime = time.Format(strTime);
	if (type == STATUS)
		m_status = strTime + sMsg;
	else if (type == RECV)
		m_receiveData = strTime + sMsg;
	
	UpdateData(FALSE);
}

int CSocketServerDlg::CheckSum(byte* sent, unsigned short recvd) {
	unsigned short* start = (unsigned short*)sent;
	while (recvd && *start != 0xFFFF) {
		recvd--;
	}
	if (recvd <= 16) {
		return -1;
	}

	byte* check = (byte*)(start + 1);
	unsigned short* data_len = start + 2;
	unsigned short check_len = (*data_len + 4) / 4;
	for (int i = 0; i < check_len; i++) {
		int sum = 0;
		for (int j = 0; j < 5; j++) {
			sum += *(check + j * check_len + i);
		}
		sum %= 0xFF;
		if (sum) {
			return 1;
		}
	}
	return 0;
}

void CSocketServerDlg::OnBtnStartServer()
{
	// TODO: Add your control notification handler code here
	CString status = "start server sucess!!\r\n�����еȴ���������.......\n";
	ShowWindowText(status, CSocketServerDlg::STATUS);

	m_serverPortCtrl.GetWindowText(m_serverPort);
	if (m_serverPort.IsEmpty())
	{
		ShowWindowText("Start Server Port Incomplete!!!\n", CSocketServerDlg::STATUS);
		return;
	}

	InitNetwork();

	m_StartServer.EnableWindow(FALSE);
	m_StopServer.EnableWindow(TRUE);
}

void CSocketServerDlg::OnBtnStopServer()
{
	// TODO: Add your control notification handler code here
	m_StartServer.EnableWindow(TRUE);
	m_StopServer.EnableWindow(FALSE);
	closesocket(ServerSock);
	/*	int i;*/
	for(i = 0; i < CLNT_MAX_NUM; i++)
		closesocket(ClientSock[i]);
	WSACleanup(); 
	ShowWindowText("Stop Server!", CSocketServerDlg::STATUS);
	//UpdateData(FALSE);
}

void CSocketServerDlg::OnBtnSendMsg()
{
	// TODO: Add your control notification handler code here
	m_serverSendMsgCtrl.GetWindowText(m_sendMsg);
	char strText[1024]={0};
	strcpy(strText,m_sendMsg);
	CString status = "���ڸ��û�" + uinfo[i].userip + "����ָ��\r\n";
	ShowWindowText(status, CSocketServerDlg::STATUS);
	if (send(ClientSock[i], strText,strlen(strText), 0) == SOCKET_ERROR)
	{
		CString sSendError;
		sSendError.Format("Server Socket failed to send: %d", GetLastError());
		ShowWindowText(sSendError, CSocketServerDlg::STATUS);
	}
	
}


void CSocketServerDlg::OnTimer(UINT_PTR nIDEvent) {
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	CString cSec, cMin, cHour;
	switch (nIDEvent) {
	case 1:
		if (m_SSec < 60) {
			m_SSec++;
			m_SMin += m_SSec / 60;
			m_SHour += m_SMin / 60;
			m_SSec %= 60;
		}
		cSec.Format(_T("%d"), m_SSec);
		cMin.Format(_T("%d"), m_SMin);
		cHour.Format(_T("%d"), m_SHour);
		SetDlgItemText(IDC_STimer, cHour + (CString)":" + cMin + (CString)":"  + cSec);
	default:
		break;
	}

	CDialog::OnTimer(nIDEvent);
}




void CSocketServerDlg::OnEnChangeEdit1()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}
