
// WaterRulerMfcDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WaterRulerMfc.h"
#include "WaterRulerMfcDlg.h"
#include "afxdialogex.h"
#include "SettingDlg.h"
#include "HttpClient.h"
#include "base64.h"
#include "ModeSettingDlg.h"
#include "Log4cppHelper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CPlayAPI g_PlayAPI;

int nWidth;
int nHeight;
bool bReadParaFile;
bool bOpenCam;
bool bSaveImage;
bool bRealDemo;
CDC* pDCCam; CRect rectCam;

//������
CRITICAL_SECTION g_crit;
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
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

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CWaterRulerMfcDlg dialog




CWaterRulerMfcDlg::CWaterRulerMfcDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CWaterRulerMfcDlg::IDD, pParent)
	, m_SuperDog(NULL)
	, m_bUploadData(FALSE)
	, m_bUploadImage(FALSE)
	, m_iDataInterval(10)
	, m_iImageInterval(30)
	, m_iDataCount(0)
	, m_iImageCount(0)
	, m_strUploadLevel(_T("0.0"))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_strResult = _T("");
	m_lPlayID = 0;
	g_PlayAPI.LoadPlayDll();
	CIvsModule::Init();
	//nWidth = 540;
	//nHeight = 960;
	nWidth = 1280;
	nHeight = 720;
}

void CWaterRulerMfcDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_STR_RESULT, m_strResult);
}

BEGIN_MESSAGE_MAP(CWaterRulerMfcDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_OPEN_CAMERA, &CWaterRulerMfcDlg::OnBnClickedOpenCamera)
	ON_BN_CLICKED(IDC_SAVE_IMG, &CWaterRulerMfcDlg::OnBnClickedSvaeImg)
	ON_BN_CLICKED(IDC_BUILD_MODEL, &CWaterRulerMfcDlg::OnBnClickedBuildModel)
	ON_BN_CLICKED(IDC_REAL_DETECTION, &CWaterRulerMfcDlg::OnBnClickedRealDetection)
	ON_MESSAGE(WM_UPDATE_STATIC, &CWaterRulerMfcDlg::OnUpdateStatic)
	ON_WM_DESTROY()
	ON_WM_TIMER()
END_MESSAGE_MAP()

void CWaterRulerMfcDlg::OnDisConnect(LLONG lLoginID, char *pchDVRIP, LONG nDVRPort, LDWORD dwUser)
{
	return;
}

void CALLBACK RealDataCallBackEx(LLONG lRealHandle, DWORD dwDataType, BYTE *pBuffer,DWORD dwBufSize, LONG lParam, LDWORD dwUser)
{
	if(dwUser == 0)
	{
		return;
	}

	//cv::Mat yuv(720, 1280, CV_8UC1, pBuffer);
	//cv::Mat bgr;
	//cv::cvtColor(yuv, bgr, CV_YUV2RGB_YV12);
	
	CWaterRulerMfcDlg *dlg = (CWaterRulerMfcDlg *)dwUser;
	dlg->ReceiveRealData(lRealHandle,dwDataType, pBuffer, dwBufSize, dwUser);
	//::PostMessage(dlg->m_hWnd, WM_UPDATE_STATIC, 1, 0);

}

//Process after receiving real-time data 
void CWaterRulerMfcDlg::ReceiveRealData(LLONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, LDWORD dwUser)
{
	//Stream port number according to the real-time handle.
	CWaterRulerMfcDlg *dlg = (CWaterRulerMfcDlg *)dwUser;

	long lRealPort= 450;
	//Input the stream data getting from the card
	BOOL bInput=FALSE;

	
	cv::Mat dst(nHeight, nWidth, CV_8UC3);//����nHeightΪ720,nWidthΪ1280,8UC3��ʾ8bit uchar �޷�������,3ͨ��ֵ
	cv::Mat src(nHeight + nHeight / 2, nWidth, CV_8UC1, (uchar*)pBuffer);

	//Mat gray;
	double level =0 ;
	Mat tmat;

	if(0 != lRealPort)
	{
		switch(dwDataType) {
		case 0:
			//Original data 
			bInput = g_PlayAPI.PLAY_InputData(lRealPort,pBuffer,dwBufSize);

			break;
		case 1:
			//Standard video data 	
			break;
		case 2:
			//yuv data 
			if(bSaveImage)
			{
				cv::cvtColor(src, dst, CV_YUV2RGB_YV12);
				imwrite("Image.jpg",dst);
				MessageBox(_T("����ͼ���Ѿ����棡"));
				bSaveImage = false;
			}

			if(bRealDemo)
			{
				cv::cvtColor(src, dst, CV_YUV2RGB_YV12);
				level = DetectValueFromImage(dst);
				
				//��������
				EnterCriticalSection(&g_crit);
				m_frameImg = dst;
				LeaveCriticalSection(&g_crit);
				//vector<uchar> img_data;
				//imencode(".jpg", dst, img_data);
				//string str_Encode(img_data.begin(), img_data.end());
				//const char *p = str_Encode.c_str();
				//level = 5;
				//CString strTemp;
				//strTemp.Format(_T("%s"),level);
				float lev = (float)level;
				UINT u = *(int*)&lev;
				::PostMessage(dlg->m_hWnd, WM_UPDATE_STATIC, u, 0);

				//::PostMessage(dlg->m_hWnd, WM_UPDATE_STATIC, level, 0); 
				//m_strResult.Format(_T("ˮλԤ��ֵΪ:%0.1lf"),level);   //�������׼���
				//GetDlgItem(IDC_STR_RESULT)->SetWindowText(m_strResult);
			}		
			break;
		case 3:
			//pcm audio data 
			break;
		case 4:
			//Original audio data 
			break;
		default:
			break;
		}	
	}
}

LRESULT CWaterRulerMfcDlg::OnUpdateStatic(WPARAM wParam, LPARAM lParam)  
{  
	//double level = wParam;
	
	float level = *(float *)&wParam;
	m_strUploadLevel.Format(_T("%0.1lf"), level);
	m_strResult.Format(_T("ˮλԤ��ֵΪ:%0.1lf"),level); 
	GetDlgItem(IDC_STR_RESULT)->SetWindowText(m_strResult);
	//while(true)
	//{
		/*if (wParam == 0)
		{  
			GetDlgItem(IDC_STR_RESULT)->SetWindowText(L"Hello Linux");  
		}
		else 
		{  
			GetDlgItem(IDC_STR_RESULT)->SetWindowText(L"Hello Windows");  
		} 
	//}
	//UpdateData(false);*/

	return 0;  
}  


double CWaterRulerMfcDlg::DetectValueFromImage(Mat realMat)
{
	bool bIsImg = (realMat.rows == nHeight) && (realMat.cols == nWidth);
	if(bIsImg && bRealDemo)
	{
		Mat imgTP = realMat(realRect);
		//ʵ�ʲ���
		int delta = sz - step;
		int tH = imgTP.rows;
		int tW = imgTP.cols;
		int tRow = floor((float)(tH-step)/delta);
		int tCol = floor((float)(tW-step)/delta);
		int tCount = 0;
		int nDim = sz*sz*3;
		Mat tmpfeature = Mat::zeros(tRow*tCol, nDim, CV_8UC1);   //������
		for(int i=0; i<tRow; i++)
		{
			for(int j=0; j<tCol; j++)
			{
				Rect rect(delta*j, delta*i, sz, sz);
				//std::cout<<rect.x<<","<<rect.y<<","<<rect.width<<","<<rect.height<<";"<<std::endl;
				Mat block = imgTP(rect);
				Mat f = block.clone().reshape(1,1);  //ת����1��
				f.copyTo(tmpfeature.row(tCount));    //���һ������
				tCount = tCount+1;
			}
		}
		Mat t_label = Mat::zeros(tRow*tCol,1,CV_32FC1);
		Mat tfeature;
		tmpfeature.convertTo(tfeature,CV_32FC1);
		tfeature = tfeature/255;     //��һ��
		CvMat cvtFeature = tfeature;

		//������ݲ���
		int nsvm = realSVM.get_var_count();
		if(nsvm==0)
		{
			MessageBox(_T("û�м���ģ���ļ���"));
		}

		CvMat *results=cvCreateMat(tRow*tCol,1,CV_32FC1);
		float response = realSVM.predict(&cvtFeature, results);

		//��resultתΪmat;
		Mat res = results;
		Mat resM = res.reshape(1,tRow)-1;
		//std::cout<<resM<<std::endl;

		//ֱ�����
		Scalar sSum = sum(resM);
		double pos_w = sSum.val[0]/tCol;      //������а�ɫ����ĸ߶�,�Ϻڣ��°�
		double pos_b = tRow-pos_w;            //%������а�ɫ����ĸ߶�

		cvReleaseMat(&results);   //�����ڴ�й©

		//���ؾ��뻻��
		double LinePos = pos_b*delta + realRect.y;
		double pixelDistance = LinePos-BASE_pixel;  // �������¼���ֽ�ĸ߶�
		//std::cout<<"pixelDistance is:"<<pixelDistance<<std::endl;

		//����ӳ��
		double x = PCoeff[0];
		double value = PCoeff[0]* pow(pixelDistance,3) + PCoeff[1]* pow(pixelDistance,2) + PCoeff[2]* pixelDistance + PCoeff[3];
		double level = BASE_height - value;
		//std::cout<<"level is:"<<level<<std::endl;
		//static float level =3;
		//level = level+1;
		return level;
	}
	return 0;
}

void CALLBACK SnapPicRet(LLONG ILoginID, BYTE *pBuf, UINT RevLen, UINT EncodeType, DWORD CmdSerial, LDWORD dwUser)
{
    CWaterRulerMfcDlg *pThis = (CWaterRulerMfcDlg*)dwUser;
	//pThis->ProcessPicture(ILoginID,pBuf,RevLen,EncodeType,CmdSerial);
}

void CWaterRulerMfcDlg::ProcessPicture(LONG ILoginID, BYTE *pBuf, UINT RevLen, UINT EncodeType, UINT CmdSerial)
{
	TCHAR m_ShortName[100];
	wsprintf( m_ShortName, TEXT("Image.bmp\0"));   //�ļ��� 
	HANDLE hf = CreateFile(m_ShortName, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                           CREATE_ALWAYS, NULL, NULL );
	if( hf == INVALID_HANDLE_VALUE )
		return ;
	DWORD dwWritten = 0;
	WriteFile( hf, pBuf, sizeof( char )*RevLen, &dwWritten, NULL );
	CloseHandle( hf );
	
	
	//ˮλ������Ĳ���-----------------------------------------------------------------
	char* strPath = "Image.bmp"; //"Image.bmp";
	realMat = cv::imread(strPath); 
	double level = DetectValueFromImage(realMat);

	m_strUploadLevel.Format(_T("%0.1lf"), level);
	//��ʾ���
	m_strResult.Format(_T("ˮλԤ��ֵΪ:%0.1lf"),level);
	GetDlgItem(IDC_STR_RESULT)->SetWindowText(m_strResult);
	//UpdateData(false);  //debugģʽ�����
	//ˮλ������Ĳ���-----------------------------------------------------------------
}

// CWaterRulerMfcDlg message handlers


BOOL CWaterRulerMfcDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
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
#ifdef SUPERDOG
	//���������ܹ���ʱ��,5���Ӽ��һ��
	SetTimer(1, 60000 * 5, NULL);
#endif
	//�붨ʱ���������ϴ�������
	SetTimer(2, 1000, NULL);
	//���ñ༭����ɫ
	CEdit *pEdit = (CEdit*)GetDlgItem(IDC_STR_RESULT);
	CButton *pButton1 = (CButton*)GetDlgItem(IDC_OPEN_CAMERA);
	CButton *pButton2 = (CButton*)GetDlgItem(IDC_SAVE_IMG);
	CButton *pButton3 = (CButton*)GetDlgItem(IDC_BUILD_MODEL);
	CButton *pButton4 = (CButton*)GetDlgItem(IDC_REAL_DETECTION);


	m_FontBig.CreatePointFont(340,_T("����"),NULL);
	m_FontSmall.CreatePointFont(110,_T("����"),NULL);

	pEdit->SetFont(&m_FontBig,TRUE);
	pButton1->SetFont(&m_FontSmall,TRUE);
	pButton2->SetFont(&m_FontSmall,TRUE);
	pButton3->SetFont(&m_FontSmall,TRUE);
	pButton4->SetFont(&m_FontSmall,TRUE);
	//������ʾ����
	pDCCam = GetDlgItem(IDC_CAM)->GetDC();
	GetDlgItem(IDC_CAM)->GetClientRect(&rectCam);

	for(int i=0; i<14;i++)  //IP��ַ
	{
		charIP[i] = 0;
	}

	sz = 3;
	step = 1;
	bReadParaFile = false;
	bOpenCam = false;
	bSaveImage = false;
	bRealDemo = false;
	bReadParaFile = ReadParaFile();
	realSVM.load( "Options\\Model.xml" );
	int nsvm = realSVM.get_var_count();
	if(nsvm==0)
	{
		m_strResult.Format(_T("û�м���ģ���ļ���"));
	}
	else
	{
		m_strResult.Format(_T("�Ѿ�����ģ���ļ���"));
	}

	UpdateData(false);
	//������ͷ
	BOOL ret = CLIENT_Init(CWaterRulerMfcDlg::OnDisConnect, (LDWORD)this);
	if(ret)
	{
		CLIENT_SetSnapRevCallBack(SnapPicRet,(DWORD)this);
	}
	else
	{
		MessageBox(_T("initialize SDK failed!"), _T("prompt"));
	}

	//OnLoginCam();
	InitializeCriticalSection(&g_crit);
	Log4cppHelper::init();
	
	Log4cppHelper::Log("Application initialize!");
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CWaterRulerMfcDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CWaterRulerMfcDlg::OnPaint()
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
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CWaterRulerMfcDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



HBRUSH CWaterRulerMfcDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  Change any attributes of the DC here
	/*if(pWnd->GetDlgCtrlID() == IDC_STR_RESULT)
	{
		pDC->SetTextColor(RGB(255,255,0));
		pDC->SetBkMode(OPAQUE);
		pDC->SetBkColor(RGB(0,0,0));       //�����ı��ı�����ɫ
		hbr=CreateSolidBrush(RGB(0,0,0));  //�ؼ��ı���ɫΪ��ɫ
	}*/
	// TODO:  Return a different brush if the default is not desired
	return hbr;
}


bool CWaterRulerMfcDlg::OnLoginCam()
{
	//��½����ͷ
	//char* szIp = "192.168.1.7";    //charIP
	int m_nPort = 37777;
	//char* m_szUserAnsi = "admin";
	//char* m_szPassAnsi = "admin";
	NET_DEVICEINFO netDevInfo = {0};
	int error = 0;

 	LONG lLoginID = CLIENT_Login(charIP, (WORD)m_nPort, m_szUserAnsi, m_szPassAnsi, &netDevInfo, &error);
	if (lLoginID == 0)
	{
		MessageBox(_T("�����¼ʧ��"),_T("Prompt"));
		return 0;
	}
	m_netDevInfo = netDevInfo;
	m_lLoginID = lLoginID;

	//��������ͷԤ��
	CIvsModule::ClearData();
	HWND hWnd = GetDlgItem(IDC_CAM)->GetSafeHwnd();
	int nChannelID = 0;
	int nPort = 450;
	
	BOOL bOpenRet = g_PlayAPI.PLAY_OpenStream(nPort,0,0,1024*900);	//Enable stream
	if(bOpenRet)
	{
		BOOL bRet = g_PlayAPI.PLAY_SetIVSCallBack(nPort,CIvsModule::GetIvsInfoProc,(DWORD)this);
		if (!bRet)
		{
			MessageBox(_T("PLAY_SetIVSCallBack failed!"),_T("Prompt"));
		}
		bRet = g_PlayAPI.PLAY_RigisterDrawFun(nPort,CIvsModule::DrawIvsInfoProc,(DWORD)this);
		if (!bRet)
		{
			MessageBox(_T("PLAY_RigisterDrawFun failed!"),_T("Prompt"));
		}
		//Begin play 
		BOOL bPlayRet = g_PlayAPI.PLAY_Play(nPort,hWnd);   //��ָ�����ڲ�����Ƶ
		if(bPlayRet)
		{
			//Real-time play 
			//m_lPlayID = CLIENT_RealPlay(m_lLoginID,nChannelID,0);
			m_lPlayID = CLIENT_RealPlay(m_lLoginID,nChannelID,GetDlgItem(IDC_SNAP)->m_hWnd);
			if(0 != m_lPlayID)
			{
				//Callback monitor data and then save 
				CLIENT_SetRealDataCallBackEx(m_lPlayID, RealDataCallBackEx, (LDWORD)this, 0x1f);
				CIvsModule::OnOpenDeviceRealplay(m_lLoginID,nChannelID,nPort,hWnd,m_netDevInfo.byChanNum);
			}
			else
			{
				MessageBox(_T("Fail to play!"), _T("Prompt"));
			}
		}
	}

	//CLIENT_SetupDeviceTime(m_lLoginID,


	//����ϵͳʱ��
	CTime time;
	time = CTime::GetCurrentTime();
	NET_TIME	m_stuTime;
	m_stuTime.dwYear = time.GetYear();
	m_stuTime.dwMonth = time.GetMonth();
	m_stuTime.dwDay = time.GetDay();
	m_stuTime.dwHour = time.GetHour();
	m_stuTime.dwMinute = time.GetMinute();
	m_stuTime.dwSecond = time.GetSecond();
	BOOL bRet = CLIENT_SetupDeviceTime(m_lLoginID, &m_stuTime);

	//��������ͷ��׽
	/*if (0 != m_lLoginID)
	{
		//Fill in request structure 
		SNAP_PARAMS snapparams = {0};
		snapparams.Channel =0;// m_ctlChannel.GetCurSel();
		snapparams.mode = 1;   //    m_snapmode=1 Ϊ�Զ�������ͼ,Ϊ�����Զ���ͼģʽ
		snapparams.CmdSerial = 1;   //GetDlgItemInt(IDC_EDIT_SERIAL);
		
		BOOL b = CLIENT_SnapPicture(m_lLoginID, snapparams);
		if (!b)
		{
			MessageBox(_T("begin to snap failed!"), _T("prompt"));
		}
	}*/

	
	return true;

}

void CWaterRulerMfcDlg::OnBnClickedOpenCamera()
{
#ifdef SUPERDOG
	TRACE("OnBnClickedOpenCamera Check SuperDog");
	if (!m_SuperDog->CheckDogSession())
	{
		AfxMessageBox(_T("�Ҳ�����Ӧ�ļ�������\n���򼴽��˳���"), MB_OK | MB_ICONSTOP);
		PostQuitMessage(0);
		return;
	}
#endif // SUPERDOG

	
	// TODO: Add your control notification handler code here
	bOpenCam = OnLoginCam();
}


void CWaterRulerMfcDlg::OnBnClickedSvaeImg()
{
#ifdef SUPERDOG
	TRACE("OnBnClickedSvaeImg Check SuperDog");
	if (!m_SuperDog->CheckDogSession())
	{
		AfxMessageBox(_T("�Ҳ�����Ӧ�ļ�������\n���򼴽��˳���"), MB_OK | MB_ICONSTOP);
		PostQuitMessage(0);
		return;
	}
#endif
	// TODO: Add your control notification handler code here
	bSaveImage = true;
	//CDialogEx::OnOK();  //�رնԻ���ͼ���Զ�����
}


bool CWaterRulerMfcDlg::ReadParaFile()
{
	//��ȡ����
	FILE* fp1;
	CString strConfigPath = _T("Options");
	CString strBasicFile= strConfigPath + _T("\\Parameter.txt");
	if((fp1=_tfopen(strBasicFile,_T("r"))) == NULL)
	{
		MessageBox(_T("������Parameter.txt�ļ���"));
		return false;
	}
	fscanf(fp1,"%s",charIP);
	fscanf(fp1, "%s", m_szUserAnsi);
	fscanf(fp1, "%s", m_szPassAnsi);
	fscanf(fp1, "%d %d", &nWidth, &nHeight);
	fscanf(fp1,"%d %d",&BASE_pixel, &BASE_height);
	fscanf(fp1,"%lf %lf %lf %lf",&PCoeff[0],&PCoeff[1],&PCoeff[2],&PCoeff[3]);  
	fscanf(fp1,"%d %d %d %d", &rectNeg.x, &rectNeg.y, &rectNeg.width, &rectNeg.height);
	fscanf(fp1,"%d %d %d %d", &rectPos.x, &rectPos.y, &rectPos.width, &rectPos.height);
	fscanf(fp1,"%d %d %d %d", &realRect.x, &realRect.y, &realRect.width, &realRect.height);
	fscanf(fp1, "%s", &m_szIMEI);

	fclose(fp1);

	return true;
}


void CWaterRulerMfcDlg::OnBnClickedBuildModel()
{
#ifdef SUPERDOG
	TRACE("OnBnClickedBuildModel Check SuperDog");
	if (!m_SuperDog->CheckDogSession())
	{
		AfxMessageBox(_T("�Ҳ�����Ӧ�ļ�������\n���򼴽��˳���"), MB_OK | MB_ICONSTOP);
		PostQuitMessage(0);
		return;
	}
#endif
	// TODO: Add your control notification handler code her
	//��ȡͼ������
	bRealDemo = false;
	//�����öԻ����������

	int nModel = 1;
	int nNegImg = 1;
	int nPosImg = 1;

	CModeSettingDlg m_dlgSetting;
	if (m_dlgSetting.DoModal() == IDOK)
	{
		nModel = m_dlgSetting.model;
		nNegImg = m_dlgSetting.m_nNumNeg;
		nPosImg = m_dlgSetting.m_nNumPos;
	}
	else
		return;

	int nDim = sz*sz * 3;
	int delta = sz - step;
	Mat img = imread("Image.jpg");
	Mat imgNeg = img(rectNeg);
	Mat imgPos = img(rectPos);
	Mat imgTP = img(realRect);


	int n_neg = 0;  //�������ĸ���
	int n_pos = 0;  //�������ĸ���
	Mat fea_neg;    //��Ÿ�����
	Mat fea_pos;    //���������


	if (nModel == 1)
	{
		//ģʽ1�� ֱ�Ӵӱ�׼ͼ���ȡ����
		int pH = imgNeg.rows;
		int pW = imgNeg.cols;
		int nRow = (int)floor((float)(pH - step) / delta);
		int nCol = (int)floor((float)(pW - step) / delta);

		n_neg = nRow*nCol;
		n_pos = nRow*nCol;

		fea_neg = Mat::zeros(n_neg, nDim, CV_8UC1);   //������
		fea_pos = Mat::zeros(n_pos, nDim, CV_8UC1);   //������

		Mat f_neg;  //ת����1��
		Mat f_pos;  //ת����1��
		int count = 0;
		for (int i = 0; i < nRow; i++)
		{
			for (int j = 0; j < nCol; j++)
			{
				Rect rect(delta*j, delta*i, sz, sz);
				//std::cout<<rect.x<<","<<rect.y<<","<<rect.width<<","<<rect.height<<";"<<std::endl;
				Mat block_neg = imgNeg(rect);
				Mat block_pos = imgPos(rect);
				f_neg = block_neg.clone().reshape(1, 1);  //ת����1��
				f_pos = block_pos.clone().reshape(1, 1);  //ת����1��
				f_neg.copyTo(fea_neg.row(count));    //���һ������
				f_pos.copyTo(fea_pos.row(count));    //���һ������
				count = count + 1;
			}
		}

	}
	else if (nModel == 2)
	{
		//������
		for (int k = 0; k < nNegImg; k++)
		{
			USES_CONVERSION;
			CString strPath;
			strPath.Format(_T("Samples\\bkgd\\%d.jpg"), k + 1);
			cv::String str_path = W2A(strPath);
			Mat tNeg = imread(str_path);
			int pH = tNeg.rows;
			int pW = tNeg.cols;
			int nRow = floor((float)(pH - step) / delta);
			int nCol = floor((float)(pW - step) / delta);
			int n_nPatch = nRow * nCol;
			n_neg = n_neg + n_nPatch;  //�õ�����������
		}

		fea_neg = Mat::zeros(n_neg, nDim, CV_8UC1);   //Ϊ����������ռ�

		int count = 0;
		for (int k = 0; k < nNegImg; k++)
		{
			USES_CONVERSION;
			CString strPath;
			strPath.Format(_T("Samples\\bkgd\\%d.jpg"), k + 1);
			cv::String str_path = W2A(strPath);
			Mat tNeg = imread(str_path);
			int pH = tNeg.rows;
			int pW = tNeg.cols;
			int nRow = floor((float)(pH - step) / delta);
			int nCol = floor((float)(pW - step) / delta);
			int n_nPatch = nRow * nCol;

			Mat f_neg;  //��ʾһ������
			for (int i = 0; i < nRow; i++)
			{
				for (int j = 0; j < nCol; j++)
				{
					Rect rect(delta*j, delta*i, sz, sz);
					Mat block_neg = tNeg(rect);
					f_neg = block_neg.clone().reshape(1, 1);  //ת����1��
					f_neg.copyTo(fea_neg.row(count));    //���һ������
					count = count + 1;
				}
			}
		}


		//������
		for (int k = 0; k < nPosImg; k++)
		{
			USES_CONVERSION;
			CString strPath;
			strPath.Format(_T("Samples\\water\\%d.jpg"), k + 1);
			cv::String str_path = W2A(strPath);
			Mat tPos = imread(str_path);
			int pH = tPos.rows;
			int pW = tPos.cols;
			int nRow = floor((float)(pH - step) / delta);
			int nCol = floor((float)(pW - step) / delta);
			int n_nPatch = nRow * nCol;
			n_pos = n_pos + n_nPatch;  //�õ�����������
		}

		fea_pos = Mat::zeros(n_pos, nDim, CV_8UC1);   //Ϊ����������ռ�

		count = 0;  //����������
		for (int k = 0; k < nPosImg; k++)
		{
			USES_CONVERSION;
			CString strPath;
			strPath.Format(_T("Samples\\water\\%d.jpg"), k + 1);
			cv::String str_path = W2A(strPath);
			Mat tPos = imread(str_path);
			int pH = tPos.rows;
			int pW = tPos.cols;
			int nRow = floor((float)(pH - step) / delta);
			int nCol = floor((float)(pW - step) / delta);
			int n_nPatch = nRow * nCol;

			Mat f_pos;  //��ʾһ������
			for (int i = 0; i < nRow; i++)
			{
				for (int j = 0; j < nCol; j++)
				{
					Rect rect(delta*j, delta*i, sz, sz);
					Mat block_pos = tPos(rect);
					f_pos = block_pos.clone().reshape(1, 1);  //ת����1��
					f_pos.copyTo(fea_pos.row(count));    //���һ������
					count = count + 1;
				}
			}
		}
	}


	//����Ԥ����
	Mat tr_fea_tmp;
	vconcat(fea_neg, fea_pos, tr_fea_tmp);
	Mat tr_fea;
	tr_fea_tmp.convertTo(tr_fea, CV_32FC1);
	tr_fea = tr_fea / 255;     //��һ��

	Mat nLabel = Mat::ones(n_neg, 1, CV_32FC1);
	Mat pLabel = Mat::ones(n_pos, 1, CV_32FC1);
	pLabel = pLabel * 2;
	Mat tr_label;
	vconcat(nLabel, pLabel, tr_label);

	CvMat cvFeature = tr_fea;	//����ת��
	CvMat cvLabels = tr_label;

	//SVMѵ������
	CvSVMParams params;
	params.svm_type = CvSVM::C_SVC;                 //SVM����
	params.kernel_type = CvSVM::RBF;//CvSVM::LINEAR;             //�˺���������
	params.term_crit = cvTermCriteria(CV_TERMCRIT_ITER, 100, FLT_EPSILON); //SVMѵ�����̵���ֹ����, max_iter:����������  epsilon:����ľ�ȷ��

	//SVM����ѵ��
	CvSVM SVM;
	SVM.train(&cvFeature, &cvLabels, NULL, NULL, params);

	//SVMģ�ͱ���
	SVM.save("Options\\Model.xml");
	MessageBox(_T("ģ���ļ��Ѿ����棡"));

	//SVM.load("Model.xml");

	//ʵ�ʲ���
	int tH = imgTP.rows;
	int tW = imgTP.cols;
	int tRow = (int)floor((float)(tH - step) / delta);
	int tCol = (int)floor((float)(tW - step) / delta);
	int tCount = 0;

	Mat tmpfeature = Mat::zeros(tRow*tCol, nDim, CV_8UC1);   //������
	for (int i = 0; i < tRow; i++)
	{
		for (int j = 0; j < tCol; j++)
		{
			Rect rect(delta*j, delta*i, sz, sz);
			//std::cout<<rect.x<<","<<rect.y<<","<<rect.width<<","<<rect.height<<";"<<std::endl;
			Mat block = imgTP(rect);
			Mat f = block.clone().reshape(1, 1);  //ת����1��
			f.copyTo(tmpfeature.row(tCount));    //���һ������
			tCount = tCount + 1;
		}
	}
	Mat t_label = Mat::zeros(tRow*tCol, 1, CV_32FC1);
	Mat tfeature;
	tmpfeature.convertTo(tfeature, CV_32FC1);
	tfeature = tfeature / 255;     //��һ��
	CvMat cvtFeature = tfeature;

	//������ݲ���
	CvMat *results = cvCreateMat(tRow*tCol, 1, CV_32FC1);
	float response = SVM.predict(&cvtFeature, results);

	//��resultתΪmat;
	Mat res = results;
	Mat resM = res.reshape(1, tRow) - 1;
	std::cout << resM << std::endl;

	//ֱ�����
	Scalar sSum = sum(resM);
	double pos_w = sSum.val[0] / tCol;      //������а�ɫ����ĸ߶�,�Ϻڣ��°�
	double pos_b = tRow - pos_w;            //%������а�ɫ����ĸ߶�
	//std::cout<<"sum is:"<<sSum<<std::endl;
	//std::cout<<"nCol is:"<<tCol<<std::endl;
	//std::cout<<"pos_w is:"<<pos_w<<std::endl;

	//���ؾ��뻻��
	double LinePos = pos_b*delta + realRect.y;
	double pixelDistance = LinePos - BASE_pixel;  // �������¼���ֽ�ĸ߶�
	std::cout << "pixelDistance is:" << pixelDistance << std::endl;

	//����ӳ��
	double x = PCoeff[0];
	double value = PCoeff[0] * pow(pixelDistance, 3) + PCoeff[1] * pow(pixelDistance, 2) + PCoeff[2] * pixelDistance + PCoeff[3];
	double level = BASE_height - value;
	std::cout << "level is:" << level << std::endl;

	//��ʾ���
	m_strResult.Format(_T("��׼Ԥ��ֵΪ:%0.1lf"), level);
	UpdateData(false);

}

void CWaterRulerMfcDlg::OnBnClickedRealDetection()
{
#ifdef SUPERDOG
	TRACE("OnBnClickedRealDetection Check SuperDog");
	if (!m_SuperDog->CheckDogSession())
	{
		AfxMessageBox(_T("�Ҳ�����Ӧ�ļ�������\n���򼴽��˳���"), MB_OK | MB_ICONSTOP);
		PostQuitMessage(0);
		return;
	}
#endif
	// TODO: Add your control notification handler code here
	if(!bOpenCam)
	{
		bOpenCam = OnLoginCam();
	}
	bRealDemo = true;
}

void CWaterRulerMfcDlg::OnDestroy()
{
	KillTimer(1);
	KillTimer(2);
	CDialogEx::OnDestroy();
	CLIENT_Cleanup();

	// TODO: Add your message handler code here
}


void CWaterRulerMfcDlg::OnTimer(UINT_PTR nIDEvent)
{
#ifdef SUPERDOG
	// ��ʱ�����ܹ��Ƿ����
	TRACE("Check superdog if is exist");
	if(nIDEvent == 1)
	{
		if (!m_SuperDog->CheckDogSession())
		{
			KillTimer(1);
			AfxMessageBox(_T("�Ҳ�����Ӧ�ļ�������\n���򼴽��˳���"), MB_OK | MB_ICONSTOP);
			PostQuitMessage(0);
		}
		log4cpp::Category& log = log4cpp::Category::getInstance(std::string("LOG"));
		log.addAppender(appender);
		log.info("Check Superdog passed!");
	}
#endif
	if (nIDEvent == 2)
	{
		if (m_bUploadData && bRealDemo)
		{
			if (m_iDataCount++ >= m_iDataInterval)
			{
				m_iDataCount = 0;
				CTime tm = CTime::GetCurrentTime();
				CString strResponse;
				CString strPost;
				CString strIMEI = CString(m_szIMEI);
				strPost.Format(_T("{\"MonitorImgData\":\"\",\"MonitorTime\": %u,\"IMEI\":" + strIMEI + ",\"WaterLevelValue\": \"" + m_strUploadLevel + "\"} "), tm.GetTime());
				
			
				Log4cppHelper::Log("Start to send level data:");
				Log4cppHelper::Log(strPost);

				CHttpClient httpClient;
				httpClient.HttpPost(_T("http://47.96.97.214/WaterLevelApi/Index?Method=api.waterlevelinfo.receive&Format=json"), strPost, strResponse);

				Log4cppHelper::Log("Server return infomation:");
				Log4cppHelper::Log(strResponse);
			}

		}
		if (m_bUploadImage && bRealDemo)
		{
			if (m_iImageCount++ >= m_iImageInterval)
			{
				m_iImageCount = 0;
				EnterCriticalSection(&g_crit);
				CString base64 = Base64::Mat2Base64(m_frameImg, "jpg");
				LeaveCriticalSection(&g_crit);
				CTime tm = CTime::GetCurrentTime();
				CString strResponse;
				CString strPost;
				CString strIMEI = CString(m_szIMEI);
				strPost.Format(_T("{\"MonitorImgData\":\"" + base64 + "\",\"MonitorTime\": %u,\"IMEI\":" + strIMEI + ",\"WaterLevelValue\": \"" + m_strUploadLevel + "\"} "), tm.GetTime());
				
			
				string infostr = std::to_string(strPost.GetLength());
			
				Log4cppHelper::Log("Start to send image length:"+ infostr);
			
				
				CHttpClient httpClient;
				httpClient.HttpPost(_T("http://47.96.97.214/WaterLevelApi/Index?Method=api.waterlevelinfo.receive&Format=json"), strPost, strResponse);

				Log4cppHelper::Log("Server return infomation:");
				Log4cppHelper::Log(strResponse);
			}
		}
	}
	CDialogEx::OnTimer(nIDEvent);
}


BOOL CWaterRulerMfcDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_F9)
		{
			OpenSettingDlg();
			return TRUE;
		}
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}


void CWaterRulerMfcDlg::OpenSettingDlg()
{
	CSettingDlg setDlg;
	setDlg.m_bUploadData = m_bUploadData;
	setDlg.m_bUploadImage = m_bUploadImage;
	setDlg.m_iDataInterval = m_iDataInterval;
	setDlg.m_iImageInterval = m_iImageInterval;
	if (setDlg.DoModal() == IDOK)
	{
		m_bUploadData = setDlg.m_bUploadData;
		m_bUploadImage = setDlg.m_bUploadImage;
		m_iDataInterval = setDlg.m_iDataInterval;
		m_iImageInterval = setDlg.m_iImageInterval;
	}

}
