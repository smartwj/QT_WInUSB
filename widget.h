#ifndef WIDGET_H
#define WIDGET_H
#include <QWidget>
#include <windows.h>
#include "winusb.h"
#include "usbuser.h"
#include "winusbio.h"
#include "setupapi.h"
#include "usbspec.h "
#include <QDebug>

struct PIPE_ID
{
    UCHAR  PipeInId;
    UCHAR  PipeOutId;
};

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    bool    SearchUSB;              //是否搜索到了usb
    bool    UsbOpen;                //usb是否打开

    void InitInterface();


    BOOL GetDeviceHandle( GUID guidDeviceInterface, PHANDLE hDeviceHandle );
    BOOL GetWinUSBHandle( HANDLE hDeviceHandle,PWINUSB_INTERFACE_HANDLE phWinUSBHandle );//得到USB设备的句柄
    BOOL GetUSBDeviceSpeed(WINUSB_INTERFACE_HANDLE hDeviceHandle, UCHAR* pDeviceSpeed);//获得USB设备的速度
    BOOL QueryDeviceEndpoints (WINUSB_INTERFACE_HANDLE hDeviceHandle, PIPE_ID* pipeid);//获取受支持终结点的类型及其管道标识符
    BOOL SendDatatoDefaultEndpoint(WINUSB_INTERFACE_HANDLE hDeviceHandle);//将控制传输发送到默认终结点
    BOOL WriteToBulkEndpoint(WINUSB_INTERFACE_HANDLE hDeviceHandle, UCHAR* pID, ULONG* pcbWritten);
    BOOL ReadFromBulkEndpoint(WINUSB_INTERFACE_HANDLE hDeviceHandle, UCHAR* pID,ULONG cbSize);

private slots:
    void on_OpenUsbButton_clicked();

    void on_SendButton_clicked();

    void on_ReciveButton_clicked();

private:
    Ui::Widget *ui;

};
#endif // WIDGET_H
