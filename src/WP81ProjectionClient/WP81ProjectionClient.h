#pragma once

#include "WP81ProjectionCommon.h"

__forceinline BOOL InitWP81UsbPipePolicy(WINUSB_INTERFACE_HANDLE hUsbInterface,PWP_SCRREN_TO_PC_PIPE p)
{
	BOOL bResult = FALSE;
	USB_INTERFACE_DESCRIPTOR usbInterfaceDesc = {};
	WinUsb_QueryInterfaceSettings(hUsbInterface,0,&usbInterfaceDesc);
	if (usbInterfaceDesc.bNumEndpoints == 2) //һ�����ƣ�һ������Ļ
	{
		BOOLEAN bOpenState = TRUE;
		WinUsb_QueryPipe(hUsbInterface,0,0,&p->PipeOfData);
		WinUsb_QueryPipe(hUsbInterface,0,1,&p->PipeOfControl);
		if (p->PipeOfControl.PipeType == UsbdPipeTypeBulk && p->PipeOfData.PipeType == UsbdPipeTypeBulk)
		{
			if (WinUsb_SetPipePolicy(hUsbInterface,p->PipeOfControl.PipeId,7,1,&bOpenState) && WinUsb_SetPipePolicy(hUsbInterface,p->PipeOfData.PipeId,7,1,&bOpenState))
				bResult = TRUE;
		}
	}
	return bResult;
}

__forceinline BOOL SendWP81UsbCtlCode(WINUSB_INTERFACE_HANDLE hUsbInterface,UINT code)
{
	WINUSB_SETUP_PACKET usbPacket = {};
	usbPacket.RequestType = 0x21; //magic
	usbPacket.Request = code;
	DWORD dwTemp = 0;
	return WinUsb_ControlTransfer(hUsbInterface,usbPacket,NULL,0,&dwTemp,NULL);
}

typedef enum{
	WP_ProjectionScreenOrientation_Default = 0, //Ĭ�ϣ�����
	WP_ProjectionScreenOrientation_Normal = 1, //����
	WP_ProjectionScreenOrientation_Hori_KeyBack = 2, //�������������ź��˰�ť��
	WP_ProjectionScreenOrientation_Hori_KeySearch = 8, //��������������������ť��
}WP81ProjectionScreenOrientation,*PWP81ProjectionScreenOrientation; 

class CWP81ProjectionClient : public IUnknown
{
private:
	ULONG RefCount = 1;
	LPWSTR lpstrUsbVid = NULL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	HANDLE hEvent = NULL;
	WINUSB_INTERFACE_HANDLE hUsb = NULL;
	PVOID pBufOfData = NULL;
	DWORD dwBufferSize = 0;
	BOOL bIoPending = FALSE;
	WP_SCRREN_TO_PC_PIPE usbPipe;
	OVERLAPPED ioOverlapped;
	CRITICAL_SECTION cs;
	DWORD dwPhoneWidth = 0,dwPhoneHeight = 0,dwPhoneStride = 0;
public:
	CWP81ProjectionClient(LPCWSTR lpszUsbVid);
	~CWP81ProjectionClient();
public:  //IUnknown
	IFACEMETHODIMP QueryInterface(REFIID iid,void** ppvObject)
	{
		if (ppvObject == NULL)
			return E_POINTER;
		if (iid == IID_IUnknown)
		{
			*ppvObject = static_cast<IUnknown*>(this);
			AddRef();
			return S_OK;
		}else{
			*ppvObject = NULL;
			return E_NOINTERFACE;
		}
	}

	IFACEMETHODIMP_(ULONG) AddRef()
	{
		return (ULONG)InterlockedIncrement(&RefCount);
	}

	IFACEMETHODIMP_(ULONG) Release()
	{
		ULONG _RefCount = (ULONG)InterlockedDecrement(&RefCount);
		if (_RefCount == 0)
			delete this;
		return _RefCount;
	}
public: //My Methods (Error Query:GetLastError)
	/*
	��ʼ��WPͶӰ��(�ֻ�������Ƿ�����ͶӰ�Ի���)
	������
		dwMaxBufferSize �������ݻ�������С
	*/
	virtual BOOL STDMETHODCALLTYPE Initialize(DWORD dwMaxBufferSize = WP_SCREEN_TO_PC_ALIGN512_MAX_SIZE);
	/*
	��ȡ��ǰ����Ļͼ��ͼ��Ϊ16bit��BMP���ݣ�
	������
		dwBufferSize ��������С
		pBuffer ���ݽ���д�����������
		pWidth ͼ��߶�
		pHeight ͼ����
		pdwBits ͼ��ɫ��
		pOrientation ��Ļ����
	*/
	virtual BOOL STDMETHODCALLTYPE ReadImageAsync();
	/*
	�ȴ��첽IO��ȡ���
	������
		dwTimeout ��ʱʱ�䣬Ĭ�����޵ȴ�
	*/
	virtual BOOL STDMETHODCALLTYPE WaitReadImageComplete(DWORD dwSizeOfBuf,PBYTE pBuffer,PUINT32 pWidth,PUINT32 pHeight,PDWORD pdwBits,PUINT pOrientation,DWORD dwTimeout = INFINITE,BOOL bFastCall = FALSE);
	/*
	��ȡ�ֻ�ԭʼ��ʾ�ķֱ��ʴ�С
	������
		pdwWidth ԭʼ�߶�
		pdwHeight ԭʼ���
	*/
	virtual BOOL STDMETHODCALLTYPE GetPhoneScreenPixel(PDWORD pdwWidth,PDWORD pdwHeight);
	/*
	�ж�USB�ӿ��Ƿ���USB2.0���µ��ٶ�
	������
		�ޣ�����ֵΪTRUE���ǵ���
	*/
	virtual BOOL STDMETHODCALLTYPE IsLowSpeedUsbPort(PBOOL pbLowSpeed);
};