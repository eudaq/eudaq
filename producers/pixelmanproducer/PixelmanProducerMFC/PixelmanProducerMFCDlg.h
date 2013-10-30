// PixelmanProducerMFCDlg.h : header file
//

#pragma once
#include "resource.h"
#include "eudaq/DummyProducer.hh"
#include "ceditextended.h"
#include "afxcmn.h"
#include "afxwin.h"
#include "mpxpluginmgrapi.h"
#include "medipixChipId.h"
#include "hexedit.h"
//#include "TimepixProducer.h"
#include "TimePixDAQStatus.h"
#include <pthread.h>

class TimepixProducer;

// CPixelmanProducerMFCDlg dialog
class CPixelmanProducerMFCDlg : public CDialog
{
	DECLARE_DYNAMIC(CPixelmanProducerMFCDlg)//Adds the ability to access run-time
											//information about an object's class
											//when deriving a class from CObject.
// Construction
public:
	CPixelmanProducerMFCDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CPixelmanProducerMFCDlg();
	void (*DialogBoxDelete)(CWnd*);
	//BOOL *pixelmanProducerCreated;
	
	//exported MpxpluginMgrFunctions
	int (*mpxCtrlLoadPixelsCfgAscii) (DEVID devID, const char *maskBitFile,
						   const char *testBitFile, const char *thlFile,
						   const char *thhOrModeFile, BOOL loadDacs);
	int (*mpxCtrlPerformFrameAcq)(DEVID devId, int numberOfFrames, 
						   double timeOfEachAcq, u32 fileFlags,
						   const char *fileName);
	int (*mpxCtrlGetFrame16)(DEVID devId, i16 *buffer, u32 size,
							 u32 frameNumber);
	int (*mpxCtrlTriggerType)(DEVID devId, int trigger);
	int (*mpxCtrlReconnectMpx)(DEVID devId);
	int (*mpxCtrlInitMpxDevice)(DEVID devId);
	int (*mpxCtrlReviveMpxDevice)(DEVID devId);
	int (*mpxCtrlAbortOperation)(DEVID devId);

	//functions to be remotely called from runcontrol
	int mpxCtrlPerformFrameAcqTimePixProd();
	void mpxCtrlStartTriggeredFrameAcqTimePixProd();
	void mpxCtrlFinishTriggeredFrameAcqTimePixProd();
	int mpxCtrlGetFrame32TimePixProd();
	
	void setAcquisitionActive();
	bool getAcquisitionActive();

	void setStartAcquisitionFailed(bool failedStatus);
	bool getStartAcquisitionFailed();

	void finishRun();

	void disablePixelManProdAcqControls();
	void enablePixelManProdAcqControls();
	
	void setFrameAcquisitionThread( CWinThread* frameAcquThread );
	CWinThread* getFrameAcquisitionThread();

	/** Check if trigger line is raised and stop the acquisiton on the chip if so.
	 *	Note: This function is thread safe because it only uses thread safe functions
	 * (I hope that mpxCtrlTriggerType() is tread safe...)
	 */
	int mpxCheckForTrigger();

	// There might be several plugins available, so we have to define which module/chip is readout
	CEditExtended m_ModuleID;
	CEditExtended m_timeToEndOfShutter;
//	CEditExtended m_AcqCount;
//	CEditExtended m_AcqTime;
	
	CEditExtended m_hostname;
	CHexEdit m_parPortAddress;
	
	
	
	CListBox m_commHistRunCtrl;
	BOOL producerStarted;
	
	

	int mpxCount;//needed to now how long medipixChipId is
	int mpxCurrSel;//needed to have information on the currently selected chip
	medipixChipId* mpxDevId;
	double infDouble;
	
// Dialog Data
	enum { IDD = IDD_PIXELMANPRODUCERMFC_DIALOG };

	/// Returns a pointer to the producer
	TimepixProducer * getProducer();
	void deleteProducer();//Threadsace delete

	//DECLARE_MESSAGE_MAP()
	
	

	

protected://veerbte Klassen koennen drauf zugreifen
// Implementation
	DECLARE_MESSAGE_MAP()//Declares that the class defines a message map
	/*afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	*/
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	HICON m_hIcon;
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	
	//protected member variables functions

	//a PixelmanProducer has a TimePixDAQStatus
	TimePixDAQStatus timePixDaqStatus;
	CStatic m_ThlMaskLabel;
	CString m_csThlFilePath;
	CString m_csMaskFilePath;
	CString m_csTestBitMaskFilePath;
	CString m_csThhorModeMaskFilePath;


	void clearAcquisitionActive();

	TimepixProducer* m_producer;
	pthread_mutex_t m_producer_mutex;


public:
	afx_msg void OnBnClickedQuit();
	afx_msg void OnBnClickedThlAsciiMask();
	afx_msg void OnBnClickedWriteMasks();
	afx_msg void OnCbnSelchangeChipselect();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	
	afx_msg void OnBnClickedButton1();
	afx_msg void OnEnChangeParPortAddr();
	afx_msg void OnBnClickedConnect();
	afx_msg void OnBnClickedCancel();

	
private://keiner kann drauf zu greifen
	bool m_AcquisitionActive;
	pthread_mutex_t m_AcquisitionActiveMutex;

	bool m_StartAcquisitionFailed;
	pthread_mutex_t m_StartAcquisitionFailedMutex;

	//Acq. Related controls	
	CSpinButtonCtrl m_SpinModuleID;
//	CSpinButtonCtrl m_SpinAcqCount;
		CComboBox m_chipSelect;
	CButton m_AsciiThlAdjFile;
	CButton m_writeMask;
	CButton m_connect;
	
	// variable to hold the frame acquisition thread
	CWinThread* m_frameAcquisitionThread;
	// mutex to protext the frameAcquisitonThread variable, since it is accessed
	// by the thread with the main loop
	pthread_mutex_t m_frameAcquisitionThreadMutex;	
};
