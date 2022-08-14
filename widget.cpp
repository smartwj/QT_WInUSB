#include "widget.h"
#include "ui_widget.h"
#include "tchar.h"
#include "strsafe.h"
#include "string.h"


//宏定义
WINUSB_INTERFACE_HANDLE hWinusb;
HDEVINFO                deviceInfo;
HRESULT                 hr;

//设置设备的VID、PID 、GUID
#define MY_VID  0x1234
#define MY_PID  0xABCD
//#define MY_GUID 12345678-ABCD-1234-ABCD-FEDCBA987654

DEFINE_GUID(GUID_DEVINTERFACE_USBApplication,
      0x12345678,0xABCD,0x1234,0xAB,0xCD,0xFE,0xDC,0xBA,0x98,0x76,0x54);

// 全局变量
GUID guidDeviceInterface = GUID_DEVINTERFACE_USBApplication;
BOOL bResult = false;
PIPE_ID PipeID;//定义结构体
HANDLE hDeviceHandle = INVALID_HANDLE_VALUE;
WINUSB_INTERFACE_HANDLE hWinUSBHandle = INVALID_HANDLE_VALUE;
ULONG cbSize=0;
UCHAR DeviceSpeed = 0x01;//定义设备的速度


//构造函数
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    InitInterface();//交互函数
}

Widget::~Widget()
{

    CloseHandle( hDeviceHandle );
    WinUsb_Free( hWinUSBHandle );
    delete ui;
}

//初始化界面
void Widget::InitInterface()
{
    this->setWindowTitle("WinUSB");
    ui->UIDlineEdit->setText(QString::number(MY_VID,16).toUpper());//设置初始化默认参数，默认是10进制，然后再转换为Qstring类型赋给控件
    ui->PIDlineEdit->setText(QString::number(MY_PID,16).toUpper());
    ui->labelShowMsg->setText("未找到设备");
    ui->SendButton->setEnabled(false);
    ui->SendlineEdit->setEnabled(false);

    bResult = GetDeviceHandle( guidDeviceInterface, &hDeviceHandle );
    if ( !bResult )
     {
        qDebug()<<"获取设备句柄错误";
        CloseHandle( hDeviceHandle );
        WinUsb_Free( hWinUSBHandle );
     }

    bResult = GetWinUSBHandle( hDeviceHandle, &hWinUSBHandle );
    if ( !bResult )
    {
        qDebug()<<"获取WinUsb句柄错误";
        CloseHandle( hDeviceHandle );
        WinUsb_Free( hWinUSBHandle );
    }

    bResult = GetUSBDeviceSpeed( hWinUSBHandle, &DeviceSpeed );
    if ( !bResult )
    {
        qDebug()<<"获取GetUSBDeviceSpeed句柄错误";
        CloseHandle( hDeviceHandle );
        WinUsb_Free( hWinUSBHandle );

    }

    bResult = QueryDeviceEndpoints( hWinUSBHandle, &PipeID );
     if ( !bResult )
     {
       qDebug()<<"获取QueryDeviceEndpoints句柄错误";
     }

}

//对设备进行同步读写访问的文件句柄。
WINBOOL Widget::GetDeviceHandle(GUID guidDeviceInterface, PHANDLE hDeviceHandle)
{

      BOOL bResult = TRUE;
      HDEVINFO hDeviceInfo;
      SP_DEVINFO_DATA DeviceInfoData;

      SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
      PSP_DEVICE_INTERFACE_DETAIL_DATA pInterfaceDetailData = NULL;

      ULONG requiredLength = 0;
      LPTSTR lpDevicePath = NULL;
      DWORD index = 0;

      // Get information about all the installed devices for the specified
      // device interface class.
      hDeviceInfo = SetupDiGetClassDevs( &guidDeviceInterface, NULL, NULL,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE );

      if ( hDeviceInfo == INVALID_HANDLE_VALUE )
      {
        // ERROR
//        printf( "Error SetupDiGetClassDevs: %d.\n", GetLastError( ) );
        qDebug()<<QString("Error SetupDiGetClassDevs: %1.\n").arg(GetLastError());
        goto done;
      }

      //Enumerate all the device interfaces in the device information set.
      DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

      for ( index = 0; SetupDiEnumDeviceInfo( hDeviceInfo, index, &DeviceInfoData );
          index++ )
      {
        //Reset for this iteration
        if ( lpDevicePath )
        {
          LocalFree( lpDevicePath );
        }
        if ( pInterfaceDetailData )
        {
          LocalFree( pInterfaceDetailData );
        }

        deviceInterfaceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);

        //Get information about the device interface.
        bResult = SetupDiEnumDeviceInterfaces( hDeviceInfo, &DeviceInfoData,
          &guidDeviceInterface, 0, &deviceInterfaceData );

        // Check if last item
        if ( GetLastError( ) == ERROR_NO_MORE_ITEMS )
        {
          break;
        }

        //Check for some other error
        if ( !bResult )
        {
//          printf( "Error SetupDiEnumDeviceInterfaces: %d.\n", GetLastError( ) );
          qDebug()<<QString("Error SetupDiEnumDeviceInterfaces: %1.\n").arg(GetLastError());
          goto done;
        }

        //Interface data is returned in SP_DEVICE_INTERFACE_DETAIL_DATA
        //which we need to allocate, so we have to call this function twice.
        //First to get the size so that we know how much to allocate
        //Second, the actual call with the allocated buffer

        // 获取设备接口的详细信息
        bResult = SetupDiGetDeviceInterfaceDetail( hDeviceInfo,
          &deviceInterfaceData, NULL, 0, &requiredLength, NULL );

        //Check for some other error
        if ( !bResult )
        {
          if ( ( ERROR_INSUFFICIENT_BUFFER == GetLastError( ) )
            && ( requiredLength > 0 ) )
          {
            //we got the size, allocate buffer
            pInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) LocalAlloc(
              LPTR, requiredLength );

            if ( !pInterfaceDetailData )
            {
              // ERROR
//              printf( "Error allocating memory for the device detail buffer.\n" );
                qDebug()<<QString("Error allocating memory for the device detail buffer.\n");
              goto done;
            }
          }
          else
          {
 //           printf( "Error SetupDiEnumDeviceInterfaces: %d.\n", GetLastError( ) );
            qDebug()<<QString("Error SetupDiEnumDeviceInterfaces: %1.\n").arg(GetLastError());
            goto done;
          }
        }

        //get the interface detailed data
        pInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        //Now call it with the correct size and allocated buffer
        bResult = SetupDiGetDeviceInterfaceDetail( hDeviceInfo,
          &deviceInterfaceData, pInterfaceDetailData, requiredLength, NULL,
          &DeviceInfoData );

        //Check for some other error
        if ( !bResult )
        {
          printf( "Error SetupDiGetDeviceInterfaceDetail: %d.\n", GetLastError( ) );
          qDebug()<<QString("Error SetupDiGetDeviceInterfaceDetail: %1.\n").arg(GetLastError());
          goto done;
        }

        //copy device path

        size_t nLength = wcslen( pInterfaceDetailData->DevicePath ) + 1;
        lpDevicePath = (TCHAR *) LocalAlloc( LPTR, nLength * sizeof(TCHAR) );
        StringCchCopy( lpDevicePath, nLength, pInterfaceDetailData->DevicePath );
        lpDevicePath[ nLength - 1 ] = 0;

//        printf( "Device path:  %s\n", lpDevicePath );
        qDebug()<<QString("Device path: %1").arg(lpDevicePath);
      }

      if ( !lpDevicePath )
      {
        //Error.
//        printf( "Error %d.", GetLastError( ) );
        qDebug()<<QString("Error %1\n").arg(GetLastError());
        goto done;
      }

      //Open the device 创建文件句柄
      *hDeviceHandle = CreateFile( lpDevicePath, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED|FILE_ATTRIBUTE_NORMAL, NULL );

      if ( *hDeviceHandle == INVALID_HANDLE_VALUE )
      {
        //Error.
//        printf( "Error %d.", GetLastError( ) );
        qDebug()<<QString("Error %1\n").arg(GetLastError());
        goto done;
      }

      done: LocalFree( lpDevicePath );
      LocalFree( pInterfaceDetailData );
      bResult = SetupDiDestroyDeviceInfoList( hDeviceInfo );

      return bResult;
}

//获取USB句柄函数
BOOL Widget::GetWinUSBHandle( HANDLE hDeviceHandle,PWINUSB_INTERFACE_HANDLE phWinUSBHandle )
{
  if ( hDeviceHandle == INVALID_HANDLE_VALUE )
  {
    return FALSE;
  }
  BOOL bResult = WinUsb_Initialize( hDeviceHandle, phWinUSBHandle );
  if ( !bResult )
  {
    qDebug()<<QString("WinUsb_Initialize Error %1").arg(GetLastError());
    return FALSE;
  }
  return bResult;
}


//获取USB设备的传输速率
BOOL Widget::GetUSBDeviceSpeed( WINUSB_INTERFACE_HANDLE hDeviceHandle,
  UCHAR* pDeviceSpeed )
{
  if ( !pDeviceSpeed || hDeviceHandle == INVALID_HANDLE_VALUE )
  {
    return FALSE;
  }

  BOOL bResult = TRUE;
  ULONG length = sizeof(UCHAR);
  qDebug()<<*pDeviceSpeed;

  bResult = WinUsb_QueryDeviceInformation( hDeviceHandle, DEVICE_SPEED, &length,pDeviceSpeed );
  if ( !bResult )
  {
    printf( "Error getting device speed: %d.\n", GetLastError( ) );
    goto done;
  }

  qDebug()<<*pDeviceSpeed;

  if ( *pDeviceSpeed == LowSpeed )
  {
//    printf( "Device speed: %d (Low speed).\n", *pDeviceSpeed );
    qDebug()<<QString("Device speed: %1 (Low speed).\n").arg(*pDeviceSpeed);
    goto done;
  }
  if ( *pDeviceSpeed == FullSpeed )
  {
//    printf( "Device speed: %d (Full speed).\n", *pDeviceSpeed );
    qDebug()<<QString("Device speed: %1 (Full speed).\n").arg(*pDeviceSpeed);
    goto done;
  }
  if ( *pDeviceSpeed == HighSpeed )
  {
//    printf( "Device speed: %d (High speed).\n", *pDeviceSpeed );
    qDebug()<<QString("Device speed: %1 (High speed).\n").arg(*pDeviceSpeed);
    goto done;
  }

  done: return bResult;
}


//检索支持的终结点类型及其管道标识符
BOOL Widget::QueryDeviceEndpoints( WINUSB_INTERFACE_HANDLE hDeviceHandle,PIPE_ID* pipeid )
{
  if ( hDeviceHandle == INVALID_HANDLE_VALUE )
  {
    return FALSE;
  }

  BOOL bResult = TRUE;

  USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
  ZeroMemory( &InterfaceDescriptor, sizeof(USB_INTERFACE_DESCRIPTOR) );

  WINUSB_PIPE_INFORMATION Pipe;
  ZeroMemory( &Pipe, sizeof(WINUSB_PIPE_INFORMATION) );

  bResult = WinUsb_QueryInterfaceSettings( hDeviceHandle, 0,
    &InterfaceDescriptor );

  if ( bResult )
  {
    for ( int index = 0; index < InterfaceDescriptor.bNumEndpoints; index++ )
    {
      //获取每个接口上每个终结点的信息
      bResult = WinUsb_QueryPipe( hDeviceHandle, 0, index, &Pipe );

      if ( bResult )
      {
        if ( Pipe.PipeType == UsbdPipeTypeControl )
        {
          printf( "Endpoint index: %d Pipe type: %d Control Pipe ID: %d.\n", index,
            Pipe.PipeType, Pipe.PipeId );
        }
        if ( Pipe.PipeType == UsbdPipeTypeIsochronous )
        {
          printf( "Endpoint index: %d Pipe type:%d Isochronous Pipe ID: %d .\n",
            index, Pipe.PipeType, Pipe.PipeId );
        }
        if ( Pipe.PipeType == UsbdPipeTypeBulk )
        {
          if ( USB_ENDPOINT_DIRECTION_IN( Pipe.PipeId ) )//函数确定指定的 USB 终结点是否为输入终结点。
          {
            printf( "Endpoint index: %d Pipe type:%d Bulk Pipe ID: %d.\n", index,
              Pipe.PipeType, Pipe.PipeId );
            pipeid->PipeInId = Pipe.PipeId;
          }
          if ( USB_ENDPOINT_DIRECTION_OUT( Pipe.PipeId ) )//确定指定的 USB 终结点是否是输出终结点。
          {
            printf( "Endpoint index: %d Pipe type:%d Bulk Pipe ID: %d.\n", index,
              Pipe.PipeType, Pipe.PipeId );
            pipeid->PipeOutId = Pipe.PipeId;
          }

        }
        if ( Pipe.PipeType == UsbdPipeTypeInterrupt )
        {
          printf( "Endpoint index: %d Pipe type: Interrupt Pipe ID: %d.\n",
            index, Pipe.PipeType, Pipe.PipeId );
        }
      }
      else
      {
        continue;
      }
    }
  }

  done: return bResult;
}


BOOL Widget::WriteToBulkEndpoint( WINUSB_INTERFACE_HANDLE hDeviceHandle, UCHAR* pID,
  ULONG* pcbWritten )
{
  if ( hDeviceHandle == INVALID_HANDLE_VALUE || !pID || !pcbWritten )
  {
    return FALSE;
  }

  BOOL bResult = TRUE;
  ULONG cbSent=0;
  UCHAR sendtext[1024];//发送数据缓存区大小
  ULONG cbSize = ui->SendlineEdit->text().length();//发送数据的长度

  for(ulong i=0;i<cbSize;i++)
  {
    sendtext[i]= (UCHAR)ui->SendlineEdit->text().at(i).toLatin1();
  }

  bResult = WinUsb_WritePipe( hDeviceHandle, *pID, sendtext, cbSize, &cbSent,0);
  if ( !bResult )
  {
    goto done;
  }
  printf( "Write to pipe %d: %s Actual data transferred: %d\n", *pID,sendtext,cbSent);
  *pcbWritten = cbSent;

  done: return bResult;
}

BOOL Widget::ReadFromBulkEndpoint( WINUSB_INTERFACE_HANDLE hDeviceHandle, UCHAR* pID, ULONG cbSize)
{
  if ( hDeviceHandle == INVALID_HANDLE_VALUE)
  {
    return FALSE;
  }
  BOOL bResult = TRUE;
//  UCHAR* readBuffer = (UCHAR*) LocalAlloc( LHND, sizeof(UCHAR) * cbSize );//在堆区开辟空间
  UCHAR readBuffer[8192]={0};//发送数据缓存区大小 大小可以自己更改
  ULONG cbRead = 0;
  bResult = WinUsb_ReadPipe( hDeviceHandle, *pID, readBuffer, cbSize, &cbRead, 0);
  if ( !bResult )
  {
    goto done;
  }

  for(ulong i=0;i<cbSize;i++)
  {
     ui->RecivetextEdit->insertPlainText(QString(readBuffer[i]).toUtf8());//在 输出显示框显示接收数据
  }
  ui->RecivetextEdit->insertPlainText("\n");//换行显示

  printf( "Read from pipe %d: %s Actual data read: %d.\n", *pID, readBuffer,cbRead );
  done: /*LocalFree( readBuffer );*///释放堆区空间
  return bResult;
}

//打开USB按钮信号槽函数
void Widget::on_OpenUsbButton_clicked()
{

    if(ui->OpenUsbButton->text()== "打开USB")
    {
        qDebug()<<"打开USB成功";
        ui->OpenUsbButton->setText("关闭USB");

        //创建设备的文件句柄
        deviceInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USBApplication,
                                         NULL,
                                         NULL,
                                         DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
//        qDebug()<<deviceInfo;
        if(deviceInfo != NULL)
         {
            qDebug()<<"找到设备";
            ui->labelShowMsg->setText("找到设备");
            ui->SendButton->setEnabled(true);
            ui->SendlineEdit->setEnabled(true);
         }
        if (deviceInfo == INVALID_HANDLE_VALUE)
         {
                qDebug()<<"未找到设备";
                ui->labelShowMsg->setText("未找到设备");
                ui->SendButton->setEnabled(false);
                ui->SendlineEdit->setEnabled(false);
         }
    }
    else
    {
        qDebug()<<"未打开USB";
        ui->OpenUsbButton->setText("打开USB");
        ui->labelShowMsg->setText("未找到设备");
    }
}

//发送数据槽函数
void Widget::on_SendButton_clicked()
{

    bResult = WriteToBulkEndpoint( hWinUSBHandle, &PipeID.PipeOutId, &cbSize );
    if ( !bResult )
    {
        qDebug()<<"获取WriteToBulkEndpoint句柄错误";
        CloseHandle( hDeviceHandle );
        WinUsb_Free( hWinUSBHandle );
    }

    bResult = ReadFromBulkEndpoint( hWinUSBHandle, &PipeID.PipeInId, cbSize );
    if ( !bResult )
    {
        qDebug()<<"获取ReadFromBulkEndpoint句柄错误";
        CloseHandle( hDeviceHandle );
        WinUsb_Free( hWinUSBHandle );
    }
//  ui->RecivetextEdit->append(ui->SendlineEdit->text());

}

//清除显示槽函数
void Widget::on_ReciveButton_clicked()
{
    ui->SendlineEdit->clear();
    ui->RecivetextEdit->clear();
}
