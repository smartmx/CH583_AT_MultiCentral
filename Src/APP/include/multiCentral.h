/********************************** (C) COPYRIGHT *******************************
 * File Name          : multiCentral.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/11/12
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef MULTICENTRAL_H
#define MULTICENTRAL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */
#include "tiny-macro-os.h"
#include "tinyflashdb.h"

/*********************************************************************
 * CONSTANTS
 */
#define MASTER_SERVICE_CONST_NUM                (4)
#define MASTER_SERVICE_SUPPORT_NUM              (MASTER_SERVICE_CONST_NUM + 1)


// Simple BLE Observer Task Events
#define START_DEVICE_EVT                        0x0001
#define START_DISCOVERY_EVT                     0x0002
#define START_SCAN_EVT                          0x0004
#define START_SVC_DISCOVERY_EVT                 0x0008
#define START_PARAM_UPDATE_EVT                  0x0010
#define START_READ_OR_WRITE_EVT                 0x0020
#define START_READ_RSSI_EVT                     0x0040
#define ESTABLISH_LINK_TIMEOUT_EVT              0x0080
#define START_WRITE_CCCD_EVT                    0x0100

#define SPP_WRITE_EVT                           0x0200
#define CUSTOM_WRITE_EVT                        0x0400
#define CUSTOM_READ_EVT                         0x0800

/*********************************************************************
 * ENUMS
 */
typedef enum
{
    MULTICENTRAL_STATE_FREE = 0,
    MULTICENTRAL_STATE_CONNECTTING,
} MULTICENTRAL_STATE_T;


/*********************************************************************
 * MACROS
 */
typedef __attribute__((aligned(4))) struct _multiCentral_info_struct
{
    uint8_t     state;                          /* 状态state */
    uint8_t     scan_enabled;                   /* 主机是否扫描 */
} multiCentral_info_t;

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

typedef __attribute__((aligned(4))) struct
{
    gatt_service_info_t     *gsi_select;            /* 选择的需要枚举的gatt_service_info */
    void                    *pMsg;                  /* write消息缓存 */
    uint16_t                svcStartHdl;            /* Discovered service start handle */
    uint16_t                svcEndHdl;              /* Discovered service end handle */
    uint16_t                pMsg_len;               /* 已经写入的长度 */
    uint16_t                pMsg_wlen;              /* 需要写入的总长度 */
    uint16_t                connHandle;             /* Connection handle of current connection */
    uint16_t                notifyHdl;
    uint16_t                cccHdl;                 /* client characteristic configuration discovery handle */
    uint16_t                writeHdl;
    uint16_t                customHdl;
    uint16_t                mtu;

    uint8_t                 taskID;                 /* Task ID for internal task/event processing */
    uint8_t                 state;                  /* Application state */
    uint8_t                 peerAddr[B_ADDR_LEN];   /* 连接的mac地址 */
    uint8_t                 discState;              /* Discovery state */
    uint8_t                 procedureInProgress;    /* GATT read/write procedure state */
} centralConnItem_t;

typedef struct
{
    uint32_t    password;               /* 从机连接密码 */
    uint8_t     peerAddr[B_ADDR_LEN];   /* 从机mac地址 */
    uint8_t     gsi_select_num;         /* 选择需要枚举的服务 */
    uint8_t     enabled;                /* 是否启用 */
} peerAddrDefItem_t;

typedef enum
{
    procedureInProgress_free = 0,
    procedureInProgress_disc,
    procedureInProgress_write,
    procedureInProgress_read,
    procedureInProgress_cccd,
    procedureInProgress_mtu,
} procedureInProgress_STATE;
/*********************************************************************
 * GLOBAL VARIABLES
 */
extern centralConnItem_t centralConnList[CENTRAL_MAX_CONNECTION];

extern gatt_service_info_t vendor_def_service;

extern multiCentral_info_t multiCentral_info;

extern const gatt_service_info_t default_gatt_service_info[MASTER_SERVICE_CONST_NUM];

extern peerAddrDefItem_t PeerAddrDef[CENTRAL_MAX_CONNECTION];

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the BLE Application
 */
extern void Central_Init(void);

/*
 * Task Event Processor for the BLE Application
 */
extern uint16_t Central_ProcessEvent(uint8_t task_id, uint16_t events);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* MULTICENTRAL_H */
