 // PixelmanProducerMFCDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PixelmanProducerMFC.h"
#include "PixelmanProducerMFCDlg.h"
 
#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"
#include "TimepixProducer.h"
#include "TimePixDAQStatus.h"
#include <limits>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


short _stdcall Inp32(short PortAddress);
void _stdcall Out32(short PortAddress, short data);
//TimepixProducer* producer;



//char* sThlMaskFilePath;

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


// CPixelmanProducerMFCDlg dialog


IMPLEMENT_DYNAMIC(CPixelmanProducerMFCDlg, CDialog)

CPixelmanProducerMFCDlg::CPixelmanProducerMFCDlg(CWnd* pParent/*=NULL*/)
	: CDialog(CPixelmanProducerMFCDlg::IDD, pParent), timePixDaqStatus(0x378),
		m_AcquisitionActive(false), m_producer(0), m_StartAcquisitionFailed(false)
{
	m_csThlFilePath.Empty();
	m_csMaskFilePath.Empty();
	m_csTestBitMaskFilePath.Empty();
	m_csThhorModeMaskFilePath.Empty();
	//m_lptPort.m_bHex = true;
	mpxCurrSel = 0;
	infDouble  = std::numeric_limits<double>::infinity();

	// Inititalise the mutexes
    pthread_mutex_init( &m_AcquisitionActiveMutex, 0 );
    pthread_mutex_init( &m_StartAcquisitionFailedMutex, 0 );	
	pthread_mutex_init( &m_producer_mutex, 0);
	pthread_mutex_init( &m_frameAcquisitionThreadMutex, 0);
}



CPixelmanProducerMFCDlg::~CPixelmanProducerMFCDlg()	
{
	//pixelmanProducerCreated = false;
	//if(producerStarted==TRUE)
	//	producer->SetDone(true);
	delete m_producer;
	
	//this->DialogBoxDelete(this);

    pthread_mutex_destroy( &m_AcquisitionActiveMutex );	
    pthread_mutex_destroy( &m_StartAcquisitionFailedMutex );	
    pthread_mutex_destroy( &m_producer_mutex );	
	pthread_mutex_destroy( &m_frameAcquisitionThreadMutex );
}


void CPixelmanProducerMFCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_hostname);
	DDX_Control(pDX, IDC_SPINMODULEID, m_SpinModuleID);
//	DDX_Control(pDX, IDC_SPINACQCOUNT, m_SpinAcqCount);
	DDX_Control(pDX, IDC_MODULEID, m_ModuleID);
//	DDX_Control(pDX, IDC_ACQCOUNT, m_AcqCount);
//	DDX_Control(pDX, IDC_EDIT3, m_AcqTime);
	DDX_Control(pDX, IDC_THLMSKLABEL, m_ThlMaskLabel);
	DDX_Control(pDX, IDC_THLASCIIMASK, m_AsciiThlAdjFile);
	DDX_Control(pDX, IDC_CHIPSELECT, m_chipSelect);
	DDX_Control(pDX, IDC_WRITEASCIIMASKS, m_writeMask);
	DDX_Control(pDX, IDC_CONNECT, m_connect);
	DDX_Control(pDX, IDC_EDIT2, m_parPortAddress);
	DDX_Control(pDX, IDC_COMMANDS, m_commHistRunCtrl);

	DDX_Control(pDX, IDC_TOS, m_timeToEndOfShutter);
}


BEGIN_MESSAGE_MAP(CPixelmanProducerMFCDlg, CDialog)
	/*ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP*/
	ON_BN_CLICKED(IDCANCEL, &CPixelmanProducerMFCDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_QUIT, &CPixelmanProducerMFCDlg::OnBnClickedQuit)

	ON_BN_CLICKED(IDC_THLASCIIMASK, &CPixelmanProducerMFCDlg::OnBnClickedThlAsciiMask)
	ON_BN_CLICKED(IDC_WRITEASCIIMASKS, &CPixelmanProducerMFCDlg::OnBnClickedWriteMasks)

	ON_CBN_SELCHANGE(IDC_CHIPSELECT, &CPixelmanProducerMFCDlg::OnCbnSelchangeChipselect)
	//ON_WM_TIMER()

	ON_BN_CLICKED(IDC_BUTTON1, &CPixelmanProducerMFCDlg::OnBnClickedButton1)
	//ON_EN_CHANGE(IDC_EDIT2, &CPixelmanProducerMFCDlg::OnEnChangeParPortAddr)
	ON_BN_CLICKED(ID_CONNECT, &CPixelmanProducerMFCDlg::OnBnClickedConnect)
END_MESSAGE_MAP()


// CPixelmanProducerMFCDlg message handlers


BOOL CPixelmanProducerMFCDlg::OnInitDialog()
{	//char ** argv = 0;
	SetTimer(1,1000,NULL);
	CDialog::OnInitDialog();
	//OnPaint;

	m_hostname.SetWindowText("192.168.0.3");	
	
	m_ModuleID.SetWindowText("1");
//	m_AcqCount.SetWindowText("0");
//	m_AcqTime.SetWindowText("0");
	m_SpinModuleID.SetRange(0, 10000);
	m_SpinModuleID.SetBuddy(&m_ModuleID);
//	m_SpinAcqCount.SetRange(0, 10000);
//	m_SpinAcqCount.SetBuddy(&m_AcqCount);
	m_commHistRunCtrl.AddString("Producer Started");
	
	producerStarted = false;

	
	for (int i = 0; i<mpxCount; i++)
	{
		static CString chipBuffer;
		chipBuffer.Format("Chip %i",(mpxDevId+i)->chipNo);
		m_chipSelect.AddString(chipBuffer);
		m_chipSelect.SetCurSel(0);
	}

	m_parPortAddress.SetWindowText("0x378");
	m_timeToEndOfShutter.SetWindowText("0");

	m_ThlMaskLabel.SetWindowText("No ASCII Mask selected");
	


	
	//sprintf(buffer, "mpxCount %i deviceId %d chipNo. %d", this->mpxCount, this->mpxDevId[mpxCount-1].deviceId, this->mpxDevId[mpxCount-1].chipNo);
	//MessageBox(buffer,"Test der deviceId", 0);

	return true;
	
}




// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CPixelmanProducerMFCDlg::OnPaint()
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
HCURSOR CPixelmanProducerMFCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

UINT mpxCtrlPerformAcqLoopThread(LPVOID pParam)
{
	CPixelmanProducerMFCDlg* pMainWnd = (CPixelmanProducerMFCDlg*) pParam;

	bool executeMainLoop = true;
	do
	{
		switch (TimepixProducer::timepix_producer_command_t c = pMainWnd->getProducer()->PopCommand())
		{

			case TimepixProducer::NONE :
				break; // don't do anything
			
			case TimepixProducer::START_RUN :
				pMainWnd->getProducer()->SetRunStatusFlag(TimepixProducer::RUN_ACTIVE);
				break;

			case TimepixProducer::STOP_RUN :
				//EUDAQ_DEBUG("Main loop: Stopping run");
				pMainWnd->finishRun();
				break;
			
			case TimepixProducer::TERMINATE:
				executeMainLoop = false;
				break;
			
			case TimepixProducer::CONFIGURE:
				// nothing to do here, is done in the communication thread
				break;
			
			case TimepixProducer::RESET:	
				// currently ignored
				break;

			default: 
				AfxMessageBox("Invalid Command", MB_ICONERROR, 0);  // invalid command, this should not be possible

		}

		// if run is active read the next event
		if (pMainWnd->getProducer()->GetRunStatusFlag() == TimepixProducer::RUN_ACTIVE )
		{
			if (!pMainWnd->getAcquisitionActive())
			{
				// no acquisition active, start it
				pMainWnd->mpxCtrlStartTriggeredFrameAcqTimePixProd();
			}
			else // acquisition is active, check for trigger and perform possible readout
			{
				int triggerStatus = pMainWnd->mpxCheckForTrigger();

				if ( triggerStatus == 1)
				{
					// no trigger yet, continue the main loop
					continue;
				}

				if ( triggerStatus<0)
				{		
					// there was an error, give a warning
					CString errorStr;
					errorStr.Format("MpxMgrError: %i", triggerStatus);
					AfxMessageBox(errorStr, MB_ICONERROR, 0);
					// and do nothing ???
				}

				// now perform the actual readout of the chip and send the data to eudaq
				pMainWnd->mpxCtrlFinishTriggeredFrameAcqTimePixProd();
			}
		}
		// This slows down the trigger detection, but should improve overall performance
		Sleep(1);
	} while (executeMainLoop);
	pMainWnd->enablePixelManProdAcqControls();
	
	return 0;
}


void CPixelmanProducerMFCDlg::OnBnClickedConnect()
{
	disablePixelManProdAcqControls();
		
	eudaq::OptionParser op("TimePix Producer", "0.0", "The TimePix Producer");
	eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://"+m_hostname.getStdStr()+":44000", "address",
                                   "The address of the RunControl application");
	eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
                                   "The minimum level for displaying log messages locally");
	eudaq::Option<std::string> name (op, "n", "name", "TimePix", "string",
                                   "The name of this Producer");
	//op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
	
	pthread_mutex_lock( & m_producer_mutex );
		m_producer = new TimepixProducer(name.Value(), rctrl.Value(), this);
	pthread_mutex_unlock( & m_producer_mutex );

	CWinThread* pThread = AfxBeginThread(mpxCtrlPerformAcqLoopThread, this);
	
	producerStarted = true;
	m_commHistRunCtrl.AddString(_T("Connected"));
	
	//MessageBox("Goodbye", "HaveFun", NULL);


}




void CPixelmanProducerMFCDlg::OnBnClickedCancel()
{
	//delete timePixDaqStatus;
	
	//producer->SetDone(true);
	//delete getProducer();	
}

void CPixelmanProducerMFCDlg::OnBnClickedQuit()
{
	//producer->SetDone(true);
	DialogBoxDelete(this);
}



void CPixelmanProducerMFCDlg::OnBnClickedThlAsciiMask()
{ 
	if (m_AsciiThlAdjFile.GetCheck() == BST_CHECKED)
	{
		//static CString m_strFilePath;
		
		
		//static CString errorStr;

		CFileDialog m_ThlMask(TRUE,NULL, NULL, OFN_DONTADDTORECENT | OFN_ENABLESIZING, NULL, this,0);
		
		if(m_ThlMask.DoModal()==IDOK)
		{
			m_csThlFilePath.Empty();
			m_csThlFilePath = m_ThlMask.GetPathName();
			m_ThlMaskLabel.SetWindowText(m_ThlMask.GetFileName());
		}
	}
	else
	{
		m_csThlFilePath.Empty();
		m_ThlMaskLabel.SetWindowText(m_csThlFilePath);
	}
		
		
}



void CPixelmanProducerMFCDlg::OnBnClickedWriteMasks()
{
	CT2CA sThlMaskFilePath(m_csThlFilePath);
	static CString errorStr;
	static DEVID devId;
	static int retval;
	devId = (mpxDevId+mpxCurrSel)->deviceId;
	
	
	
	//strcpy_s(sThlMaskFilePath,strlen(sThlMaskFilePath),m_csThlFilePath);
//	m_ThlMaskLabel.SetWindowText(m_csThlFilePath.GetFileName());
	
//	sThlMaskFilePath = new char[m_strFilePath.GetLength()+1];
//	strcpy_s(sThlMaskFilePath,strlen(sThlMaskFilePath),m_strFilePath);
//	m_ThlMaskLabel.SetWindowText(m_ThlMask.GetFileName());
	

	retval = mpxCtrlLoadPixelsCfgAscii(devId, NULL , NULL, sThlMaskFilePath , NULL, 0);
		if (retval != 0)
		{
			errorStr.Format("MpxMgrError: %i", retval);
			AfxMessageBox(errorStr, MB_ICONERROR, 0);
		}
}



void CPixelmanProducerMFCDlg::OnCbnSelchangeChipselect()
{
	mpxCurrSel = m_chipSelect.GetCurSel();
}


//starting mpxCtrlPerformFrameAcq in own thread (DEVID devId, int numberOfFrames, 
//						   double timeOfEachAcq, u32 fileFlags,
//						   const char *fileName);

UINT mpxCtrlPerformFrameAcqThread(LPVOID pParam)//thread der zur normalen Acq. gehört
{
	CPixelmanProducerMFCDlg* pMainWnd = (CPixelmanProducerMFCDlg*) pParam;
	
	int threadRetVal = 0;
	int retval, retval2;
	
	pMainWnd->disablePixelManProdAcqControls();


	DEVID devId = pMainWnd->mpxDevId[pMainWnd->mpxCurrSel].deviceId;
	
//	int numberOfFrames = pMainWnd->m_AcqCount.getInt();
	int numberOfFrames = 1;
//	int timeOfEachAcq = pMainWnd->m_AcqTime.getInt();
	int timeOfEachAcq = 1; // = 1 second ???
	
	i16 *  databuffer = pMainWnd->mpxDevId[pMainWnd->mpxCurrSel].databuffer;            
	u32 sizeOfDataBuffer = pMainWnd->mpxDevId[pMainWnd->mpxCurrSel].sizeOfDataBuffer;
	retval = pMainWnd->mpxCtrlPerformFrameAcq(devId, numberOfFrames, timeOfEachAcq, NULL, NULL);
	retval2 = pMainWnd->mpxCtrlGetFrame16(devId, databuffer, sizeOfDataBuffer, 0);
	for(u32 i = 256*256; i<sizeOfDataBuffer; i++)
		if (databuffer[i]>0)
			databuffer[i];

			
	
	//pMainWnd->enablePixelManProdAcqControls();
	
	if (retval <= retval2)
		threadRetVal = retval;
	else
		threadRetVal = retval2;
	
	return 0;
	
}

	
int CPixelmanProducerMFCDlg::mpxCtrlPerformFrameAcqTimePixProd()
{
	static int retval;
	static CString errorStr;
	static int trigger;


	
	CWinThread* pThread = AfxBeginThread(mpxCtrlPerformFrameAcqThread,this);
	
	/*if (_threadRetVal != 0)
		{
			errorStr.Format("MpxMgrError: %i", retval);
			AfxMessageBox(errorStr, MB_ICONERROR, 0);
		}
		
	return _threadRetVal;*/
	//return value of mpxCtrlPerformFrameAcqThread is per Defualt
	return 0;
}





UINT mpxCtrlPerformTriggeredFrameAcqThread(LPVOID pParam)
{	
	
	CPixelmanProducerMFCDlg* pMainWnd = (CPixelmanProducerMFCDlg*) pParam;
	
	int threadRetVal = 0;
	int retval = 0;
	int retval2 = 0;
	static CString errorStr;

	//pMainWnd->disablePixelManProdAcqControls();

	DEVID devId = pMainWnd->mpxDevId[pMainWnd->mpxCurrSel].deviceId;
	i16 *  databuffer = pMainWnd->mpxDevId[pMainWnd->mpxCurrSel].databuffer;            
	u32 sizeOfDataBuffer = pMainWnd->mpxDevId[pMainWnd->mpxCurrSel].sizeOfDataBuffer;

	// this thing triggers the callback which sets AcquisitionActive.
	// If it fails 3 times set StartAcquisitionFailed to notify the main loop
	int my_very_private_dont_touch_i;
	for (my_very_private_dont_touch_i = 0; my_very_private_dont_touch_i < 3; my_very_private_dont_touch_i++)
	{
		retval = pMainWnd->mpxCtrlPerformFrameAcq(devId, 1, pMainWnd->infDouble, NULL, NULL);
		// break if frame acquisition was sucessful
		if (retval == 0)
		{
			break;
		}
		else //try to revive the chip
		{
			int abortOK = pMainWnd->mpxCtrlAbortOperation(devId);

			if (my_very_private_dont_touch_i > 0) // try to reconnect on second and third attempt
			{
				int con = pMainWnd->mpxCtrlReconnectMpx(devId);
				EUDAQ_INFO("Device " + eudaq::to_string(pMainWnd->m_ModuleID.getInt()) + ": reconnecting Chip");
			}

			int ini = pMainWnd->mpxCtrlInitMpxDevice(devId);
			EUDAQ_INFO("Device " + eudaq::to_string(pMainWnd->m_ModuleID.getInt()) + ": reinitialising Chip");						
		}
	}

	// if start of acquisition was not successful notify the main loop
	if (my_very_private_dont_touch_i == 3)
	{
		pMainWnd->setStartAcquisitionFailed(true);
	}

	// only perform readout if acquisition was successful
	if (retval == 0)
	{
		retval2 = pMainWnd->mpxCtrlGetFrame16(devId, databuffer, sizeOfDataBuffer, 0);
	}
	
	/////////////

	//more negative Mpx-Error will be returned, zero is okay
	if (retval <= retval2)
		threadRetVal = retval;
	else
		threadRetVal = retval2;
	
	if (threadRetVal<0)
		{
			//errorStr.Format("MpxMgrError: %i", threadRetVal);
			//AfxMessageBox(errorStr, MB_ICONERROR, 0);
			// copy the error values to the frame
			for (unsigned int i=0; i < sizeOfDataBuffer ; i ++)
				databuffer[i] = (i16)0xFFFF;
			EUDAQ_ERROR("Device "+pMainWnd->m_ModuleID.getStdStr()+
				": Could not read frame "+eudaq::to_string(pMainWnd->getProducer()->GetEventNumber()));
			// 
			DEVID devId = pMainWnd->mpxDevId[pMainWnd->mpxCurrSel].deviceId;
			int abortOK = pMainWnd->mpxCtrlAbortOperation(devId);
			EUDAQ_DEBUG("Operation aborted");
			int initOK = pMainWnd->mpxCtrlInitMpxDevice(devId);
			EUDAQ_DEBUG("Device reinitialised");
		}

//	pMainWnd->enablePixelManProdAcqControls();
	
	return 0;
}

void CPixelmanProducerMFCDlg::mpxCtrlStartTriggeredFrameAcqTimePixProd()
{	
	setStartAcquisitionFailed(false);

	setFrameAcquisitionThread( AfxBeginThread(mpxCtrlPerformTriggeredFrameAcqThread,this) );

	// wait until the acquisition realy is active before lowering the busy flag.
    // (The flag is set via callback function and can be accessed with getAcquisitionActive())
	while ((getAcquisitionActive() == false) && (getStartAcquisitionFailed()==false))
	{
		Sleep(1);
	}

	if (getStartAcquisitionFailed())
	{ // acquisition failed to start after several retries, quit the producer
		getProducer()->PushCommand( TimepixProducer::TERMINATE );
		// the busy line remains high
	}
	else // acquisition has started successfully, lower the busy to accept a trigger
	{
		timePixDaqStatus.parPortSetBusyLineLow();
	}

	// ok, acquisition is running. Return to the main loop
}

void CPixelmanProducerMFCDlg::mpxCtrlFinishTriggeredFrameAcqTimePixProd()
{
	// wait for the data acquisition thread (readout of the chip) to finish
	WaitForSingleObject(getFrameAcquisitionThread()->m_hThread, INFINITE);

	// now the chip is read out
	// send the data to eudaq
	Sleep(50);

	getProducer()->Event(mpxDevId[mpxCurrSel].databuffer, mpxDevId[mpxCurrSel].sizeOfDataBuffer);

	// data readout is finished, we can clear the AcquisitonActive flag and return
	clearAcquisitionActive();
}

void CPixelmanProducerMFCDlg::finishRun()
{
	// flag whether there still is an event to read out and send
	bool performReadout = false;

	// check if there was a trigger
	if(timePixDaqStatus.parPortCheckTriggerLine() == HIGH)
	{
		performReadout=true; // we need this information later
	}

	// raise the busy, this will lower the trigger line if it had been high
	timePixDaqStatus.parPortSetBusyLineHigh();

	//EUDAQ_DEBUG("Busy raised");

	if (performReadout) // in case there is still an event to read
	{
		//EUDAQ_DEBUG("Busy raised");
		// wait until the shutter has closed again
		// (unfortunately 1 ms is the shortest we can get :-( )
		Sleep(1);
	}
	else
	{
		// check for a race condition: The producer has raised busy without the trigger line being high
		// If there was a trigger just this moment both levels could be high and the readout could be blocked

		//EUDAQ_DEBUG("checking for race condition");
		if(timePixDaqStatus.parPortCheckTriggerLine() == HIGH)
		{ // the race condition occured. lower the busy and raise it again
			EUDAQ_WARN("Resolvong race condition: Busy and Trigger were high at the same time.");
			timePixDaqStatus.parPortSetBusyLineLow();
			Sleep(1); // wait to give the TLU time to notice the change
			timePixDaqStatus.parPortSetBusyLineHigh();
		}
	}

	// now it's time to stop the acquisition 
	DEVID devId = this->mpxDevId[this->mpxCurrSel].deviceId;
	int retval = mpxCtrlTriggerType(devId, TRIGGER_ACQSTOP); // ignore return value? I have no idea what to do...
	// wait for acquisition thread to finish
	WaitForSingleObject(getFrameAcquisitionThread()->m_hThread, INFINITE);

	// if there is still an event to send
	if(performReadout)
	{
		//EUDAQ_DEBUG("sending last event in run");
		// send the data to eudaq
		Sleep(50); // we  need this sleep, don't ask me why 
		getProducer()->Event(mpxDevId[mpxCurrSel].databuffer, mpxDevId[mpxCurrSel].sizeOfDataBuffer);
	}

	// finally the AcquisitonActive and the RUN_ACTIVE flag
	clearAcquisitionActive();
	//EUDAQ_DEBUG("Setting status to run stopped");
	getProducer()->SetRunStatusFlag(TimepixProducer::RUN_STOPPED);
}

int CPixelmanProducerMFCDlg::mpxCheckForTrigger()
{
	DEVID devId = this->mpxDevId[this->mpxCurrSel].deviceId;
	
	//static i16 *  databuffer = mpxDevId[mpxCurrSel].databuffer;            
	//u32 sizeOfDataBuffer = mpxDevId[mpxCurrSel].sizeOfDataBuffer;

	if(timePixDaqStatus.parPortCheckTriggerLine() == HIGH)
	{
		timePixDaqStatus.parPortSetBusyLineHigh();
		// wait until the shutter has closed again
		// (unfortunately 1 ms is the shortest we can get :-( )
		Sleep(1);
		return mpxCtrlTriggerType(devId, TRIGGER_ACQSTOP);
	}
	else
		return  1;
}

		
	
void CPixelmanProducerMFCDlg::OnTimer(UINT_PTR nIDEvent)
{
	int trigger = this->timePixDaqStatus.parPortCheckTriggerLine();
	if(trigger==1)
		MessageBox("Hallo", "Boss", 0);
	CDialog::OnTimer(nIDEvent);

}
	



/*int CPixelmanProducerMFCDlg::mpxCtrlGetFrame32TimePixProd()
{
	static int retval;
	static CString errorStr;
	
	
	CWinThread* pThread = AfxBeginThread(mpxCtrlPerformFrameAcqThread,this);
	
	if (retval != 0)
		{
			errorStr.Format("MpxMgrError: %i", retval);
			AfxMessageBox(errorStr, MB_ICONERROR, 0);
		}
	return retval;
	
	//return value of mpxCtrlPerformFrameAcqThread is per Defualt
	}*/
void CPixelmanProducerMFCDlg::OnBnClickedButton1()
{	
	/*static CString hexBuffer;
	
	int testHexBuffer = m_parPortAddress.GetValue();;
	timePixDaqStatus.parPortUpdateAddress(testHexBuffer);
	

	int busyLineStatus = this->timePixDaqStatus.parPortCheckBusyLine();
	if (busyLineStatus == HIGH)
		timePixDaqStatus.parPortSetBusyLineLow();
	else
		timePixDaqStatus.parPortSetBusyLineHigh();*/

	// start acquisition
	mpxCtrlStartTriggeredFrameAcqTimePixProd();
	// wait for trigger
	while (mpxCheckForTrigger() == 1) Sleep(1) ;

	// perform the readout
	mpxCtrlFinishTriggeredFrameAcqTimePixProd();
}



void CPixelmanProducerMFCDlg::OnEnChangeParPortAddr()
{
	static int hexBuffer;
	hexBuffer = m_parPortAddress.GetValue();
	timePixDaqStatus.parPortUpdateAddress(hexBuffer);
}

void  CPixelmanProducerMFCDlg::setAcquisitionActive()
{
	pthread_mutex_lock( &m_AcquisitionActiveMutex);
		m_AcquisitionActive = true;
	pthread_mutex_unlock( &m_AcquisitionActiveMutex);
}

void  CPixelmanProducerMFCDlg::setStartAcquisitionFailed(bool failedStatus)
{
	pthread_mutex_lock( &m_StartAcquisitionFailedMutex);
		m_StartAcquisitionFailed = failedStatus;
	pthread_mutex_unlock( &m_StartAcquisitionFailedMutex);
}

void  CPixelmanProducerMFCDlg::clearAcquisitionActive()
{
	pthread_mutex_lock( &m_AcquisitionActiveMutex);
		m_AcquisitionActive = false;
	pthread_mutex_unlock( &m_AcquisitionActiveMutex);
}

bool  CPixelmanProducerMFCDlg::getAcquisitionActive()
{	
	bool retval;
	pthread_mutex_lock( &m_AcquisitionActiveMutex);
		retval = m_AcquisitionActive;
	pthread_mutex_unlock( &m_AcquisitionActiveMutex);
	return retval;
}

bool  CPixelmanProducerMFCDlg::getStartAcquisitionFailed()
{	
	bool retval;
	pthread_mutex_lock( &m_StartAcquisitionFailedMutex);
		retval = m_StartAcquisitionFailed;
	pthread_mutex_unlock( &m_StartAcquisitionFailedMutex);
	return retval;
}

void CPixelmanProducerMFCDlg::disablePixelManProdAcqControls()
{
	m_connect.EnableWindow(false);
	m_AsciiThlAdjFile.EnableWindow(false);
	m_writeMask.EnableWindow(false);
	m_chipSelect.EnableWindow(false);
	m_ModuleID.EnableWindow(false);
//	m_AcqCount.EnableWindow(false);
//	m_AcqTime.EnableWindow(false);
	m_SpinModuleID.EnableWindow(false);
	m_timeToEndOfShutter.EnableWindow(false);
//	m_SpinAcqCount.EnableWindow(false);
}

void CPixelmanProducerMFCDlg::enablePixelManProdAcqControls()
{	
	m_connect.EnableWindow(true);
	m_AsciiThlAdjFile.EnableWindow(true);
	m_writeMask.EnableWindow(true);
	m_chipSelect.EnableWindow(true);
	m_ModuleID.EnableWindow(true);
//	m_AcqCount.EnableWindow(true);
//	m_AcqTime.EnableWindow(true);
	m_SpinModuleID.EnableWindow(true);
	m_timeToEndOfShutter.EnableWindow(true);
//	m_SpinAcqCount.EnableWindow(true);
}

TimepixProducer * CPixelmanProducerMFCDlg::getProducer()
{
	TimepixProducer * retval;
	pthread_mutex_lock( & m_producer_mutex );
		retval = m_producer;
	pthread_mutex_unlock( & m_producer_mutex );
	return retval;

}

void CPixelmanProducerMFCDlg::deleteProducer()
{
	pthread_mutex_lock( & m_producer_mutex );
		delete m_producer;
	pthread_mutex_unlock( & m_producer_mutex );
}

void CPixelmanProducerMFCDlg::setFrameAcquisitionThread( CWinThread* frameAcquThread )
{
	pthread_mutex_lock( & m_frameAcquisitionThreadMutex );
		m_frameAcquisitionThread = frameAcquThread;
	pthread_mutex_unlock( & m_frameAcquisitionThreadMutex );
}

CWinThread* CPixelmanProducerMFCDlg::getFrameAcquisitionThread( )
{
	CWinThread* retval;
	pthread_mutex_lock( & m_frameAcquisitionThreadMutex );
		retval = m_frameAcquisitionThread;
	pthread_mutex_unlock( & m_frameAcquisitionThreadMutex );

	return retval;
}
