# AT多主机例程

本多主机例程基于CH583系列芯片，使用UART0作为AT指令交互串口，默认波特率为115200，UART1为printf默认重定向串口。

AT指令不区分大小写，即```at+ver\r\n```和```AT+VER\r\n```和```aT+vEr\r\n```都可以触发获取版本号，返回都是大写。

AT指令必须以AT开头，如果有命令则以`+`作为衔接，结尾必须为`\r\n`。

## 使用流程

1. 启动设备，收到`+AT:READY\r\n`后，发送`AT\r\n`进行通讯测试。
2. 如果需要的服务不在默认支持内，通过`at+setservice`设置服务信息。
3. 通过`at+setautoconnect`设置从机设备信息。
4. 使用`AT+reset\r\n`复位芯片，重新载入从机设备信息。
5. 使用`AT+startscan\r\n`开启扫描，扫描到设备后会进行连接。
6. 等到接收到`+CONNECTED: 01 02 03 04 05 06\r\nOK\r\n`。
7. 如果无需继续连接设备，使用`AT+stopscan\r\n`停止扫描。
8. 如果有Notify服务，等待接收到`+NOTIFY: 01 02 03 04 05 06,READY\r\n`。
9. 现在则可以通过AT指令和从机开始交互了。

## 主机枚举参数配置

本例程通过本意是给蓝牙串口模块搭配使用的，提供了一路NOTIFY通道，一路WRITE通道，和一路自定义可读可写通道。用户可以通过自定义接口，实现不同的功能。

工程通过`gatt_service_info_t`结构体设置服务，默认支持4个不可更改服务，一个自定义服务。

不可更改的服务见后续小节**默认支持的gatt_service_info_t**。

```c
typedef __attribute__((aligned(4))) struct _gatt_service_info_struct
{
    uint8_t     service_uuid_len;               /* 0 - not used, ATT_UUID_SIZE and ATT_BT_UUID_SIZE is legal. */
    uint8_t     notify_uuid_len;                /* 0 - not used, ATT_UUID_SIZE and ATT_BT_UUID_SIZE is legal. */
    uint8_t     write_uuid_len;                 /* 0 - not used, ATT_UUID_SIZE and ATT_BT_UUID_SIZE is legal, 0xff means it's uuid  same as notify's uuid. */
    uint8_t     custom_uuid_len;

    uint8_t     service_uuid[ATT_UUID_SIZE];    /* Service UUID */
    uint8_t     notify_uuid[ATT_UUID_SIZE];     /* NOTIFY UUID */
    uint8_t     write_uuid[ATT_UUID_SIZE];      /* WRITE UUID */
    uint8_t     custom_uuid[ATT_UUID_SIZE];     /* CUSTOM UUID */
} gatt_service_info_t;
```

设备连接从机后，会根据从机设置的service信息去进行蓝牙枚举，获取指定几个服务的handle值。

自定义的那一个gatt_service_info_t可以通过AT指令设置：

```text
at+setservice=e0ff,e1ff,e2ff,0\r\n
```

```text
at+setservice=7941dc240ee5a9e093f3a3b50100406e,7941dc240ee5a9e093f3a3b50200406e,7941dc240ee5a9e093f3a3b50300406e,0\r\n
```

## 从机信息设置

主机只会根据设定的从机参数连接，不会连接其他从机，连接判断条件为MAC地址。
从机信息分为存储的从机信息，和运行中的从机信息。
存储的从机信息会在上电时载入至运行中的从机信息中，修改运行中从机信息不会影响存储的从机信息。

从机信息结构体如下：

```c
typedef struct
{
    uint32_t    password;               /* 从机连接密码 */
    uint8_t     peerAddr[B_ADDR_LEN];   /* 从机mac地址 */
    uint8_t     gsi_select_num;         /* 选择需要枚举的服务 */
    uint8_t     enabled;                /* 是否启用 */
} peerAddrDefItem_t;
```

其中gsi_select_num为大于等于4时，就是用户自定的服务信息。

```text
at+setautoconnect=0,6d03001414c4,123456,3,1\r\n
```

```text
at+setconnect=0,6d03001414c4,123456,3,1\r\n
```

## AT默认响应

在某些情况下，在特定事件发生后，会通过AT串口上报状态，无需AT指令单独触发。

| AT默认响应 | 说明 |
| :---- | :---- |
| `+AT:READY\r\n` | 表明AT串口初始化就绪，可以开始交互，在改变波特率、复位后都会上报 |
| `+SCAN: 01 02 03 04 05 06\r\n` | 开启扫描后，扫描到的设备MAC地址都会上报 |
| `+CONNECTED: 01 02 03 04 05 06\r\nOK\r\n` | 连接成功状态上报 |
| `+DISCONNECTED: 01 02 03 04 05 06, REASON: 0x%02x\r\n` | 连接断开状态上报 |
| `+MTUUPDATE:\r\nADDR: 01 02 03 04 05 06,VALUE:%d\r\n` | 指定地址设备MTU更新上报 |
| `+NOTIFY: 01 02 03 04 05 06,READY\r\n` | 表示连接设备的NOTIFY通道CCCD使能成功 |
| `+NOTIFY: 01 02 03 04 05 06,ERR,RETRY\r\n` | 表示连接设备的NOTIFY通道CCCD使能失败，继续重试 |
| `+NOTIFY:\r\nADDR: 01 02 03 04 05 06,LEN:%d,DATA:\r\n` + NOTIFY收到的数据（原始数据，无需转换）+ `\r\nOK\r\n` | 上报收到的指定地址设备的NOTIFY数据 |
| `+CUSTOMREAD:\r\nADDR: 01 02 03 04 05 06,LEN:%d,DATA:\r\n` + CUSTOMREAD收到的数据（原始数据，无需转换）+ `\r\nOK\r\n` | 上报读取到的指定地址设备的数据 |
| `+CUSTOMREAD:\r\nADDR: 01 02 03 04 05 06,0x%02x\r\nERR\r\n` | 读取失败状态上报 |
| `+CUSTOMWRITE:\r\nADDR: 01 02 03 04 05 06\r\nDONE\r\n` | 自定义服务写入完成状态上报，则可以开始下一次写入，否则请等待 |
| `+SPPWRITE:\r\nADDR: 01 02 03 04 05 06\r\nDONE\r\n` | 默认串口服务写入完成状态上报，则可以开始下一次写入，否则请等待 |

## AT指令集

| AT指令 | 说明 | 返回 |
| :---- | :---- | :---- |
| `AT\r\n` | 通信测试 | `OK\r\n` |
| `AT+ver\r\n` | 获取版本号 | `+VER:20230314_rc1\r\nOK\r\n` |
| `AT+reset\r\n` | 复位芯片 | `+RESET\r\nOK\r\n` |
| `AT+blemac&0\r\n` | 清除设置的蓝牙MAC地址 | 正确：`+BLEMAC&0:\r\nOK\r\n` |
| | 错误原因：设置信息保存到flash失败 | 错误：`+BLEMAC&0:\r\nERR\r\n` |
| `AT+blemac=1234567890ab\r\n` | 设置的蓝牙MAC地址，等号后面为hex格式，例子中即设置蓝牙mac地址为 0x12 0x34 0x56 0x78 0x90 0xab。| 正确：`+BLEMAC\r\nOK\r\n` |
| | 错误原因：设置信息保存到flash失败 | 错误：`+BLEMAC\r\nERR\r\n` |
| | 错误原因：非法MAC地址 | 错误：`+BLEMAC\r\nINVALID MAC\r\nERR\r\n` |
| | 错误原因：MAC地址数据错误 | 错误：`+BLEMAC\r\nERR\r\n` |
| `AT+blemac?\r\n` | 获取设置的蓝牙MAC地址 | `+BLEMAC: c4 c2 c4 03 02 02\r\nOK\r\n` |
| `AT+rawmac\r\n` | 获取芯片内部MAC地址 |`+RAWMAC: ff ff ff ff ff ff\r\nOK\r\n` |
| `AT+startscan\r\n` | 开始扫描，扫描到设定的MAC地址后开始连接，直到无法再连接 | 正确：`+STARTSCAN:0\r\nOK\r\n` |
| | 错误原因：`bleAlreadyInRequestedMode` | 错误：`+STARTSCAN:1\r\nERR\r\n` |
| | 错误原因：`bleIncorrectMode` | 错误：`+STARTSCAN:2\r\nERR\r\n` |
| `AT+stopscan\r\n` | 停止扫描 | 正确：`+STOPSCAN:0\r\nOK\r\n` |
| | 错误原因：`bleIncorrectMode`/`bleInvalidTaskID` | 错误：`+STOPSCAN:1\r\nERR\r\n` |
| `AT+setbaudrate=baud\r\n` | 改变AT串口的波特率为10进制参数`baud` | `+SETBAUDRATE\r\nOK\r\n` |
| | 错误原因：`baud`参数不在`300`-`1500000`之间 | 错误：`+SETBAUDRATE\r\nERR\r\n` |
| | 错误原因：`setbaudrate`后不是`=`号 | 错误：`\r\nERR:02\r\n` |
| `AT+setservice=su,nu,wu,cu\r\n` | 设置需要枚举的UUID模板 | `+SETSERVICE\r\nOK\r\n` |
| | 错误原因：uuid不正确，su/nu/wu长度不为2/16，cu长度不为0/2/16 | 错误：`+SETSERVICE\r\nERR\r\n` |
| | 错误原因：`setservice`后不是`=`号 | 错误：`\r\nERR:02\r\n` |
| `AT+getservice=num\r\n` | 获取存放的需要枚举的UUID模板 | `+GETSERVICE:\r\nService UUID: f0 ff\r\nNotify UUID: f1 ff\r\nWrite UUID: f2 ff\r\nCustom UUID: f3 ff\r\nOK\r\n` |
| | 没有服务 | `+GETSERVICE:\r\nNO SERVICE\r\nOK\r\n` |
| | 错误原因：`getservice`后不是`=`号 | 错误：`\r\nERR:02\r\n` |
| `AT+setautoconnect=num,1234567890ab,password,gsi_select,enable\r\n` | 设置存储的自动连接从机信息 | `+SETAUTOCONNECT\r\nOK\r\n` |
| | 错误原因：`num`过大 | 错误：`+SETAUTOCONNECT\r\nUNSUPPORT NUMBER\r\nERR\r\n` |
| | 错误原因：mac地址错误 | 错误：`+SETAUTOCONNECT\r\nERR\r\n` |
| | 错误原因：`setautoconnect`后不是`=`号 | 错误：`\r\nERR:02\r\n` |
| `AT+getautoconnect\r\n` | 获取存储的自动连接从机信息 | `+GETAUTOCONNECT:\r\nMAC: 9c 03 00 14 14 c4\r\nPASSWORD:123456\r\nSERVICE:3\r\nOK\r\n` |
| | 连接未使能 | `+GETAUTOCONNECT:\r\nNONE\r\nOK\r\n` |
| | 错误原因：`num`过大 | 错误：`+GETAUTOCONNECT\r\nUNSUPPORT NUMBER\r\nERR\r\n` |
| | 错误原因：`getautoconnect`后不是`=`号 | 错误：`\r\nERR:02\r\n` |
| `AT+setconnect=num,1234567890ab,password,gsi_select,enable\r\n` | 设置当前连接参数，不存放到flash，下次复位还是之前的 | `+SETCONNECT\r\nOK\r\n` |
| | 错误原因：`num`过大 | 错误：`+SETCONNECT\r\nUNSUPPORT NUMBER\r\nERR\r\n` |
| | 错误原因：mac地址错误 | 错误：`+SETCONNECT\r\nERR\r\n` |
| | 错误原因：`setconnect`后不是`=`号 | 错误：`\r\nERR:02\r\n` |
| `AT+getconnect=num\r\n` | 获取当前的连接设置 | `+GETCONNECT:\r\nMAC: 9c 03 00 14 14 c4\r\nPASSWORD:123456\r\nSERVICE:3\r\nOK\r\n` |
| | 连接未使能 | `+GETCONNECT:\r\nNONE\r\nOK\r\n` |
| | 错误原因：`num`过大 | 错误：`+GETCONNECT:\r\nUNSUPPORT NUMBER\r\nERR\r\n` |
| | 错误原因：`getconnect`后不是`=`号 | 错误：`\r\nERR:02\r\n` |
| `AT+enableconnect=num\r\n` | 允许连接指定运行中的从机信息 | `+ENABLECONNECT\r\nOK\r\n` |
| | 错误原因：`num`过大 | 错误：`+ENABLECONNECT\r\nUNSUPPORT NUMBER\r\nERR\r\n` |
| | 错误原因：`enableconnect`后不是`=`号 | 错误：`\r\nERR:02\r\n` |
| `AT+disableconnect=num\r\n` | 禁止连接指定运行中的从机信息 | `+DISABLECONNECT\r\nOK\r\n` |
| | 错误原因：`num`过大 | 错误：`+DISABLECONNECT\r\nUNSUPPORT NUMBER\r\nERR\r\n` |
| | 错误原因：`disableconnect`后不是`=`号 | 错误：`\r\nERR:02\r\n` |
| `AT+sppwrite=1234567890ab,data\r\n` | 向mac地址1234567890ab的从机的默认写入通道发送数据data | `+SPPWRITE:PREPARE TO WRITE\r\nOK\r\n` |
| | 错误原因：上次写入服务还在进行中 | 错误：`+SPPWRITE:DEVICE IS BUSY\r\nERR\r\n` |
| | 错误原因：内存不足 | 错误：`+SPPWRITE:MEMORY NOT ENOUGH\r\nERR\r\n` |
| | 错误原因：指定MAC地址设备未连接 | 错误：`+SPPWRITE:DEVICE NOT CONNECTED\r\nERR\r\n` |
| | 错误原因：指定MAC地址设备没有write服务 | 错误：`+SPPWRITE:NO WRITE HANDLER\r\nERR\r\n` |
| | 错误原因：MAC地址格式错误 | 错误：`+SPPWRITE:\r\nERR\r\n` |
| | 错误原因：`sppwrite`后不是`=`号 | 错误：`\r\nERR:02\r\n` |
| `AT+customwrite=1234567890ab,data\r\n` | 向mac地址1234567890ab的从机的自定义通道发送数据data | `+CUSTOMWRITE:PREPARE TO WRITE\r\nOK\r\n` |
| | 错误原因：上次写入服务还在进行中 | 错误：`+CUSTOMWRITE:DEVICE IS BUSY\r\nERR\r\n` |
| | 错误原因：内存不足 | 错误：`+CUSTOMWRITE:MEMORY NOT ENOUGH\r\nERR\r\n` |
| | 错误原因：指定MAC地址设备未连接 | 错误：`+CUSTOMWRITE:DEVICE NOT CONNECTED\r\nERR\r\n` |
| | 错误原因：指定MAC地址设备没有CUSTOM服务 | 错误：`+CUSTOMWRITE:NO CUSTOM HANDLER\r\nERR\r\n` |
| | 错误原因：MAC地址格式错误 | 错误：`+CUSTOMWRITE:\r\nERR\r\n` |
| | 错误原因：`customwrite`后不是`=`号 | 错误：`\r\nERR:02\r\n` |
| `AT+customread=1234567890ab\r\n` | 从mac地址1234567890ab的从机的自定义通道读取数据 | `+CUSTOMREAD:PREPARE TO READ\r\nOK\r\n` |
| | 错误原因：上次读取服务还在进行中 | 错误：`+CUSTOMREAD:ALREADY IN READ PROCESS\r\nERR\r\n` |
| | 错误原因：指定MAC地址设备未连接 | 错误：`+CUSTOMREAD:DEVICE NOT CONNECTED\r\nERR\r\n` |
| | 错误原因：指定MAC地址设备没有CUSTOM服务 | 错误：`+CUSTOMREAD:NO CUSTOM HANDLER\r\nERR\r\n` |
| | 错误原因：MAC地址格式错误 | 错误：`+CUSTOMREAD:\r\nERR\r\n` |
| | 错误原因：`customread`后不是`=`号 | 错误：`\r\nERR:02\r\n` |
| `AT+disconnect=1234567890ab\r\n` | 断开指定MAC地址设备 | `+DISCONNECT\r\nOK\r\n` |
| | 错误原因：MAC地址格式错误 | 错误：`+DISCONNECT\r\nERR\r\n` |
| | 错误原因：`disconnect`后不是`=`号 | 错误：`\r\nERR:02\r\n` |
| `AT+getmtu=1234567890ab\r\n` | 获取指定MAC地址设备的MTU | `+GETMTU:23\r\nOK\r\n` |
| | 错误原因：MAC地址格式错误 | 错误：`+GETMTU:\r\nERR\r\n` |
| | 错误原因：指定MAC地址设备未连接 | 错误：`+GETMTU:DEVICE NOT CONNECTED\r\nERR\r\n` |
| | 错误原因：`getmtu`后不是`=`号 | 错误：`\r\nERR:02\r\n` |
| 其他未知命令 | | `\r\nERR:01\r\n` |
| 格式错误命令 | | `\r\nERR:02\r\n` |

## 默认支持的gatt_service_info_t

该4个默认支持的、不可更改的服务如下，当然可以通过更改源代码进行更换：

```c
    /* CH9141 Uart Service */
    {
        /* service_uuid */
        .service_uuid =
        {
            0xf0,0xff,
        },
        /* service_uuid_len */
        .service_uuid_len = ATT_BT_UUID_SIZE,
        /* notify_uuid */
        .notify_uuid =
        {
            0xf1,0xff,
        },
        /* notify_uuid_len */
        .notify_uuid_len = ATT_BT_UUID_SIZE,
        /* write_uuid */
        .write_uuid =
        {
            0xf2,0xff,
        },
        /* write_uuid_len */
        .write_uuid_len = ATT_BT_UUID_SIZE,
        /* custom_uuid */
        .custom_uuid =
        {
            0xf3,0xff,
        },
        /* custom_uuid_len */
        .custom_uuid_len = ATT_BT_UUID_SIZE,
    },

    /* Nordic Uart Service */
    {
        /* service_uuid */
        .service_uuid =
        {
            0x9e,0xca,0xdc,0x24,0x0e,0xe5,0xa9,0xe0,0x93,0xf3,0xa3,0xb5,0x01,0x00,0x40,0x6e
        },
        /* service_uuid_len */
        .service_uuid_len = ATT_UUID_SIZE,
        /*  notify_uuid*/
        .notify_uuid =
        {
            0x9e,0xca,0xdc,0x24,0x0e,0xe5,0xa9,0xe0,0x93,0xf3,0xa3,0xb5,0x03,0x00,0x40,0x6e,
        },
        /* notify_uuid_len */
        .notify_uuid_len = ATT_UUID_SIZE,
        /* write_uuid */
        .write_uuid =
        {
            0x9e,0xca,0xdc,0x24,0x0e,0xe5,0xa9,0xe0,0x93,0xf3,0xa3,0xb5,0x02,0x00,0x40,0x6e
        },
        /* write_uuid_len */
        .write_uuid_len = ATT_UUID_SIZE,
        /* custom_uuid */
        .custom_uuid =
        {
            0
        },
        /* custom_uuid_len */
        .custom_uuid_len = 0,
    },

    /* BT05 Service */
    {
        /* service_uuid */
        .service_uuid =
        {
            0xe0,0xff,
        },
        /* service_uuid_len */
        .service_uuid_len = ATT_BT_UUID_SIZE,
        /* notify_uuid */
        .notify_uuid =
        {
            0xe1,0xff,
        },
        /* notify_uuid_len */
        .notify_uuid_len = ATT_BT_UUID_SIZE,
        /* write_uuid */
        .write_uuid =
        {
            0,
        },
        /* write_uuid_len */
        .write_uuid_len = 0xff,
        /* custom_uuid */
        .custom_uuid =
        {
            0,
        },
        /* custom_uuid_len */
        .custom_uuid_len = 0,
    },

    /* MW741 */
    {
        /* service_uuid */
        .service_uuid =
        {
            0x79,0x41,0xdc,0x24,0x0e,0xe5,0xa9,0xe0,0x93,0xf3,0xa3,0xb5,0x01,0x00,0x40,0x6e,
        },
        /* service_uuid_len */
        .service_uuid_len = ATT_UUID_SIZE,
        /*  notify_uuid*/
        .notify_uuid =
        {
            0x79,0x41,0xdc,0x24,0x0e,0xe5,0xa9,0xe0,0x93,0xf3,0xa3,0xb5,0x03,0x00,0x40,0x6e,
        },
        /* notify_uuid_len */
        .notify_uuid_len = ATT_UUID_SIZE,
        /* write_uuid */
        .write_uuid =
        {
            0x79,0x41,0xdc,0x24,0x0e,0xe5,0xa9,0xe0,0x93,0xf3,0xa3,0xb5,0x02,0x00,0x40,0x6e,
        },
        /* write_uuid_len */
        .write_uuid_len = ATT_UUID_SIZE,
        /* custom_uuid */
        .custom_uuid =
        {
            0
        },
        /* custom_uuid_len */
        .custom_uuid_len = 0,
    },
```

## AT串口使用示例

```text
[16:37:11.371]收←◆+AT:READY

[16:37:15.961]发→◇AT+blemac?
□
[16:37:15.973]收←◆+BLEMAC: c4 c2 c4 03 02 02
OK

[16:37:23.870]发→◇AT+blemac=010203040506
□
[16:37:23.879]收←◆+BLEMAC
[16:37:23.901]收←◆
OK

[16:37:25.692]发→◇AT+blemac?
□
[16:37:25.703]收←◆+BLEMAC: 01 02 03 04 05 06
OK

[16:37:34.412]发→◇at+setautoconnect=0,6d03001414c4,123456,3,1
□
[16:37:34.423]收←◆+SETAUTOCONNECT
OK

[16:37:36.707]发→◇at+setautoconnect=1,9c03001414c4,123456,3,1
□
[16:37:36.717]收←◆+SETAUTOCONNECT
OK

[16:37:43.793]发→◇AT+RESET
□
[16:37:43.802]收←◆+RESET
OK

[16:37:43.895]收←◆+AT:READY

[16:38:16.599]发→◇at+startscan
□
[16:38:16.609]收←◆+STARTSCAN:0
OK
+SCAN: 9c 03 00 14 14 c4
+SCAN: 79 6e 06 e1 60 48

[16:38:16.648]收←◆+SCAN: 7d f6 7c d6 86 c9
+SCAN: f0 d5 2e 31 92 3c
+SCAN: aa b2 29 ce 65 34
+SCAN: 86 36 fc 86 17 10
+SCAN: 95 8a a7 f9 d7 1a
+SCAN: 91 6d 5b 00 8a 37
+SCAN: 5d 5e ab bc 5b 37
+SCAN: 24 00 e2 da f7 11

[16:38:18.133]收←◆+CONNECTED: 9c 03 00 14 14 c4
OK
+SCAN: d1 72 75 fe 37 1b
+SCAN: 63 48 c7 69 85 19
+SCAN: 26 af e3 12 cc e9
+SCAN: f6 f6 5d 88 33 12
+SCAN: 75 68 ae 39 80 6e
+SCAN: 23 4d bb 13 e4 57
+SCAN: 01 d7 12 3e d5 36
+SCAN: 00 80 45 0c 73 db
+SCAN: 86 36 fc 86 17 10
+SCAN: 7e 9d 92 e9 ba e4

[16:38:19.650]收←◆+SCAN: 7e 9d 92 e9 ba e4
+SCAN: bd b5 9c 98 e9 2f
+SCAN: cd 03 a0 90 7c 40
+SCAN: 62 44 b2 9d 79 6c
+SCAN: 27 fc f5 85 6a 64
+SCAN: 80 9b ff b7 ca 7f
+SCAN: 83 29 34 26 3e f8
+SCAN: 1b b4 0e 55 40 48
+SCAN: 91 6d 5b 00 8a 37
+SCAN: 45 f6 41 3c e7 2e

[16:38:21.134]收←◆+NOTIFY: 9c 03 00 14 14 c4,READY

[16:38:21.168]收←◆+SCAN: 27 fc f5 85 6a 64
+SCAN: ec 2f b6 2c f8 2e
+SCAN: dd 82 34 ad da 32
+SCAN: b5 09 7a 5a a7 56
+SCAN: 5d 5e ab bc 5b 37
+SCAN: 6d ab 12 f7 9a 0f
+SCAN: aa b2 29 ce 65 34
+SCAN: 5a ef bd f0 ff 3f
+SCAN: 75 68 ae 39 80 6e
+SCAN: 82 41 8f 93 bf 28

[16:38:22.651]收←◆+SCAN: 7e 9d 92 e9 ba e4
+SCAN: 35 e9 c6 b0 ae 0c
+SCAN: d7 d8 54 0a 12 74
+SCAN: 6d 03 00 14 14 c4
+SCAN: bd b5 9c 98 e9 2f
+SCAN: a7 f5 22 b0 03 5d
+SCAN: 79 6e 06 e1 60 48
+SCAN: f6 f6 5d 88 33 12
+SCAN: 6d 57 46 ae 8f 47
+SCAN: 63 48 c7 69 85 19

[16:38:24.191]收←◆+CONNECTED: 6d 03 00 14 14 c4
OK

[16:38:24.247]收←◆+SCAN: 18 05 49 e4 c2 84
+SCAN: 1b eb bf 96 b0 33
+SCAN: 63 48 c7 69 85 19

[16:38:24.274]收←◆+SCAN: cd 03 a0 90 7c 40
+SCAN: 91 6d 5b 00 8a 37
+SCAN: 83 29 34 26 3e f8
+SCAN: 80 9b ff b7 ca 7f
+SCAN: 50 7c 1b c9 d7 31
+SCAN: 5a ef bd f0 ff 3f
+SCAN: 67 f6 e2 09 9b 37

[16:38:25.702]收←◆+SCAN: 63 48 c7 69 85 19
+SCAN: d7 d8 54 0a 12 74
+SCAN: 23 4d bb 13 e4 57
+SCAN: bd b5 9c 98 e9 2f
+SCAN: 91 6d 5b 00 8a 37
+SCAN: 1b b4 0e 55 40 48
+SCAN: aa b2 29 ce 65 34
+SCAN: 24 00 e2 da f7 11
+SCAN: 5e 10 a9 77 3d 5e
+SCAN: 27 fc f5 85 6a 64

[16:38:27.111]收←◆+NOTIFY: 6d 03 00 14 14 c4,READY

[16:38:27.228]收←◆+SCAN: 30 33 fb b7 d6 0a
+SCAN: 30 0f 79 bc ab 68
+SCAN: 5a ef bd f0 ff 3f
+SCAN: d1 72 75 fe 37 1b
+SCAN: 3c 45 62 6e cf 7a
+SCAN: 24 52 75 51 60 03
+SCAN: 27 fc f5 85 6a 64
+SCAN: 9d f5 fd 48 7c 1d
+SCAN: c6 7f 95 7c cc 25
+SCAN: f6 f6 5d 88 33 12

[16:38:28.717]收←◆+SCAN: 84 8f 72 04 6f de
+SCAN: f6 f6 5d 88 33 12
+SCAN: 05 1d 97 d5 c0 08
+SCAN: bd b5 9c 98 e9 2f
+SCAN: 01 d7 12 3e d5 36
+SCAN: a7 f5 22 b0 03 5d
+SCAN: 94 a3 3e f3 42 60
+SCAN: 35 e9 c6 b0 ae 0c
+SCAN: b4 3e f5 01 9b 6b
+SCAN: 1d b1 03 c7 f8 d9

[16:38:29.245]发→◇at+stopscan
□
[16:38:29.253]收←◆+STOPSCAN:0
OK

[16:38:33.001]发→◇at+sppwrite=6d03001414c4,h!\0C\0\0\0\0\0瀽@1\03\0?
□
[16:38:33.016]收←◆+SPPWRITE:PREPARE TO WRITE
OK

[16:38:33.082]收←◆+SPPWRITE:
ADDR: 6d 03 00 14 14 c4
DONE

[16:38:33.201]收←◆+NOTIFY:
ADDR: 6d 03 00 14 14 c4,LEN:20,DATA:
hT\0?\0\0\0\0\0G?@1
OK
+NOTIFY:
ADDR: 6d 03 00 14 14 c4,LEN:20,DATA:
\0
 type=Relay\0\0
OK
+NOTIFY:
ADDR: 6d 03 00 14 14 c4,LEN:20,DATA:
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0
OK
+NOTIFY:
ADDR: 6d 03 00 14 14 c4,LEN:20,DATA:
 \0	\0\0?\0m\0\0\0
OK
+NOTIFY:
ADDR: 6d 03 00 14 14 c4,LEN:6,DATA:
\0\0\0L?
OK

[16:38:35.199]发→◇at+sppwrite=9c03001414c4,h!\0C\0\0\0\0\0瀽@1\03\0?
□
[16:38:35.213]收←◆+SPPWRITE:PREPARE TO WRITE
OK

[16:38:35.279]收←◆+SPPWRITE:
ADDR: 9c 03 00 14 14 c4
DONE

[16:38:35.392]收←◆+NOTIFY:
ADDR: 9c 03 00 14 14 c4,LEN:20,DATA:
hT\0?\0\0\0\0\0G?@1
OK
+NOTIFY:
ADDR: 9c 03 00 14 14 c4,LEN:20,DATA:
\0
 type=Relay\0\0
OK
+NOTIFY:
ADDR: 9c 03 00 14 14 c4,LEN:20,DATA:
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0
OK
+NOTIFY:
ADDR: 9c 03 00 14 14 c4,LEN:20,DATA:
 \0	\0\0?\0?\0\0\0
OK
+NOTIFY:
ADDR: 9c 03 00 14 14 c4,LEN:6,DATA:
\0\0\0h(
OK
```
