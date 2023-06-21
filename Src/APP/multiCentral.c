/********************************** (C) COPYRIGHT *******************************
* File Name          : multiCentral.c
* Author             : WCH
* Version            : V1.0
* Date               : 2018/11/12
* Description        : 主机多连接例程
*******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"
#include "gattprofile.h"
#include "multiCentral.h"
#include "uart_interface.h"
#include "at_cmd.h"

/*********************************************************************
 * MACROS
 */

// Length of bd addr as a string
#define B_ADDR_STR_LEN                      15

/*********************************************************************
 * CONSTANTS
 */
// Maximum number of scan responses
#define DEFAULT_MAX_SCAN_RES                10

// Scan duration in 0.625ms
#define DEFAULT_SCAN_DURATION               2400

// Connection min interval in 1.25ms
#define DEFAULT_MIN_CONNECTION_INTERVAL     24

// Connection max interval in 1.25ms
#define DEFAULT_MAX_CONNECTION_INTERVAL     48

// Connection supervision timeout in 10ms
#define DEFAULT_CONNECTION_TIMEOUT          100

// Discovey mode (limited, general, all)
#define DEFAULT_DISCOVERY_MODE              DEVDISC_MODE_ALL

// TRUE to use active scan
#define DEFAULT_DISCOVERY_ACTIVE_SCAN       TRUE

// TRUE to use white list during discovery
#define DEFAULT_DISCOVERY_WHITE_LIST        FALSE

// TRUE to use high scan duty cycle when creating link
#define DEFAULT_LINK_HIGH_DUTY_CYCLE        FALSE

// TRUE to use white list when creating link
#define DEFAULT_LINK_WHITE_LIST             FALSE

// Default read RSSI period in 0.625ms
#define DEFAULT_RSSI_PERIOD                 2400

// Minimum connection interval (units of 1.25ms)
#define DEFAULT_UPDATE_MIN_CONN_INTERVAL    24

// Maximum connection interval (units of 1.25ms)
#define DEFAULT_UPDATE_MAX_CONN_INTERVAL    48

// Slave latency to use parameter update
#define DEFAULT_UPDATE_SLAVE_LATENCY        0

// Supervision timeout value (units of 10ms)
#define DEFAULT_UPDATE_CONN_TIMEOUT         600

// Default passcode
#define DEFAULT_PASSCODE                    123456

// Default GAP pairing mode
#define DEFAULT_PAIRING_MODE                GAPBOND_PAIRING_MODE_WAIT_FOR_REQ

// Default MITM mode (TRUE to require passcode or OOB when pairing)
#define DEFAULT_MITM_MODE                   TRUE

// Default bonding mode, TRUE to bond, max bonding 6 devices
#define DEFAULT_BONDING_MODE                FALSE

// Default GAP bonding I/O capabilities
#define DEFAULT_IO_CAPABILITIES             GAPBOND_IO_CAP_KEYBOARD_DISPLAY

// Default service discovery timer delay in 0.625ms
#define DEFAULT_SVC_DISCOVERY_DELAY         1600

// Default parameter update delay in 0.625ms
#define DEFAULT_PARAM_UPDATE_DELAY          3200

// Default read or write timer delay in 0.625ms
#define DEFAULT_READ_OR_WRITE_DELAY         1600

// Default write CCCD delay in 0.625ms
#define DEFAULT_WRITE_CCCD_DELAY            1600

// Establish link timeout in 0.625ms
#define ESTABLISH_LINK_TIMEOUT              3200

// Application states
enum
{
    BLE_STATE_IDLE,
    BLE_STATE_CONNECTING,
    BLE_STATE_CONNECTED,
    BLE_STATE_DISCONNECTING
};

// Discovery states
enum
{
    BLE_DISC_STATE_IDLE,            // Idle
    BLE_DISC_STATE_SVC,             // Service discovery
    BLE_DISC_STATE_NOTIFY_CHAR,     // Characteristic discovery
    BLE_DISC_STATE_NOTIFY_CCCD,     // client characteristic configuration discovery
    BLE_DISC_STATE_WRITE_CHAR,
    BLE_DISC_STATE_CUSTOM_CHAR,
};
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
const gatt_service_info_t default_gatt_service_info[MASTER_SERVICE_CONST_NUM] =
{

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

};

gatt_service_info_t vendor_def_service = {0};    /* 自定uuid，如果不在模板中，客户自定的service服务 */

multiCentral_info_t multiCentral_info = {0};

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

// Task ID for internal task/event processing
static uint8_t centralTaskId;

// Number of scan results
static uint8_t centralScanRes;

// Scan result list
static gapDevRec_t centralDevList[DEFAULT_MAX_SCAN_RES];

// Peer device address
peerAddrDefItem_t PeerAddrDef[CENTRAL_MAX_CONNECTION];

// Connection item list
centralConnItem_t centralConnList[CENTRAL_MAX_CONNECTION];

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void     centralProcessGATTMsg(gattMsgEvent_t *pMsg);
static void     centralRssiCB(uint16_t connHandle, int8_t rssi);
static void     centralEventCB(gapRoleEvent_t *pEvent);
static void     centralHciMTUChangeCB(uint16_t connHandle, uint16_t maxTxOctets, uint16_t maxRxOctets);
static void     centralPasscodeCB(uint8_t *deviceAddr, uint16_t connectionHandle,
                                  uint8_t uiInputs, uint8_t uiOutputs);
static void     centralPairStateCB(uint16_t connHandle, uint8_t state, uint8_t status);
static uint16_t connect_ProcessEvent(uint8_t task_id, uint16_t events);
static void     central_ProcessTMOSMsg(tmos_event_hdr_t *pMsg);
static void     centralGATTDiscoveryEvent(uint8_t connItem, gattMsgEvent_t *pMsg);
static void     centralConnIistStartDiscovery(uint8_t connNum);
static void     centralAddDeviceInfo(uint8_t *pAddr, uint8_t addrType);
static void     centralInitConnItem(uint8_t task_id, centralConnItem_t *centralConnList);
static uint8_t  centralAddrCmp(peerAddrDefItem_t *PeerAddrDef, uint8_t *addr);

/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapCentralRoleCB_t centralRoleCB = {
    centralRssiCB,        // RSSI callback
    centralEventCB,       // Event callback
    centralHciMTUChangeCB // MTU change callback
};

// Bond Manager Callbacks
static gapBondCBs_t centralBondCB = {
    centralPasscodeCB,
    centralPairStateCB
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      Central_Init
 *
 * @brief   Initialization function for the Central App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notification).
 *
 * @param   task_id - the ID assigned by TMOS.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void Central_Init()
{
    centralTaskId = TMOS_ProcessEventRegister(Central_ProcessEvent);

    // Setup GAP
    GAP_SetParamValue(TGAP_DISC_SCAN, DEFAULT_SCAN_DURATION);
    GAP_SetParamValue(TGAP_CONN_EST_INT_MIN, DEFAULT_MIN_CONNECTION_INTERVAL);
    GAP_SetParamValue(TGAP_CONN_EST_INT_MAX, DEFAULT_MAX_CONNECTION_INTERVAL);
    GAP_SetParamValue(TGAP_CONN_EST_SUPERV_TIMEOUT, DEFAULT_CONNECTION_TIMEOUT);

    // Setup the GAP Bond Manager
    {
        uint32_t passkey = DEFAULT_PASSCODE;
        uint8_t  pairMode = DEFAULT_PAIRING_MODE;
        uint8_t  mitm = DEFAULT_MITM_MODE;
        uint8_t  ioCap = DEFAULT_IO_CAPABILITIES;
        uint8_t  bonding = DEFAULT_BONDING_MODE;

        GAPBondMgr_SetParameter(GAPBOND_CENT_DEFAULT_PASSCODE, sizeof(uint32_t), &passkey);
        GAPBondMgr_SetParameter(GAPBOND_CENT_PAIRING_MODE, sizeof(uint8_t), &pairMode);
        GAPBondMgr_SetParameter(GAPBOND_CENT_MITM_PROTECTION, sizeof(uint8_t), &mitm);
        GAPBondMgr_SetParameter(GAPBOND_CENT_IO_CAPABILITIES, sizeof(uint8_t), &ioCap);
        GAPBondMgr_SetParameter(GAPBOND_CENT_BONDING_ENABLED, sizeof(uint8_t), &bonding);
    }

    // Init Connection Item
    centralInitConnItem(centralTaskId, centralConnList);
    // Initialize GATT Client
    GATT_InitClient();
    // Register to receive incoming ATT Indications/Notifications
    GATT_RegisterForInd(centralTaskId);
    // Setup a delayed profile startup
    tmos_set_event(centralTaskId, START_DEVICE_EVT);
}

/*********************************************************************
 * @fn      centralInitConnItem
 *
 * @brief   Init Connection Item
 *
 * @param   task_id -
 *          centralConnList -
 *
 * @return  NULL
 */
static void centralInitConnItem(uint8_t task_id, centralConnItem_t *centralConnList)
{
    uint8_t connItem;
    for(connItem = 0; connItem < CENTRAL_MAX_CONNECTION; connItem++)
    {
        /* 每个连接的任务通过taskID区分 */
        centralConnList[connItem].taskID = TMOS_ProcessEventRegister(Central_ProcessEvent);
        centralConnList[connItem].connHandle = GAP_CONNHANDLE_INIT;
        centralConnList[connItem].state = BLE_STATE_IDLE;
        centralConnList[connItem].discState = BLE_DISC_STATE_IDLE;
        centralConnList[connItem].procedureInProgress = procedureInProgress_free;

        centralConnList[connItem].svcStartHdl = 0;
        centralConnList[connItem].svcEndHdl = 0;
        centralConnList[connItem].cccHdl = 0;
    }
}

/*********************************************************************
 * @fn      Central_ProcessEvent
 *
 * @brief   Central Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16_t Central_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if(events & SYS_EVENT_MSG)
    {
        uint8_t *pMsg;

        if((pMsg = tmos_msg_receive(centralTaskId)) != NULL)
        {
            central_ProcessTMOSMsg((tmos_event_hdr_t *)pMsg);
            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    if(events & START_DEVICE_EVT)
    {
        // Start the Device
        GAPRole_CentralStartDevice(centralTaskId, &centralBondCB, &centralRoleCB);
        return (events ^ START_DEVICE_EVT);
    }

    if(events & ESTABLISH_LINK_TIMEOUT_EVT)
    {
        GAPRole_TerminateLink(INVALID_CONNHANDLE);
        return (events ^ ESTABLISH_LINK_TIMEOUT_EVT);
    }

    return connect_ProcessEvent(task_id, events);

    // Discard unknown events
    return 0;
}

/*********************************************************************
 * @fn      connect_ProcessEvent
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static uint16_t connect_ProcessEvent(uint8_t task_id, uint16_t events)
{
    uint8_t conn_num;

    for(conn_num = 0; conn_num < CENTRAL_MAX_CONNECTION; conn_num++)
    {
        if(task_id == centralConnList[conn_num].taskID)
        {
            break;
        }
    }
    if(conn_num == CENTRAL_MAX_CONNECTION)
    {
        return 0;
    }

    if(events & START_SVC_DISCOVERY_EVT)
    {
        // start service discovery
        centralConnIistStartDiscovery(conn_num);
        return (events ^ START_SVC_DISCOVERY_EVT);
    }

    if(events & START_PARAM_UPDATE_EVT)
    {
        // start connect parameter update
        GAPRole_UpdateLink(centralConnList[conn_num].connHandle,
                           DEFAULT_UPDATE_MIN_CONN_INTERVAL,
                           DEFAULT_UPDATE_MAX_CONN_INTERVAL,
                           DEFAULT_UPDATE_SLAVE_LATENCY,
                           DEFAULT_UPDATE_CONN_TIMEOUT);

        return (events ^ START_PARAM_UPDATE_EVT);
    }

    if(events & SPP_WRITE_EVT)
    {
        attWriteReq_t req;
        if(centralConnList[conn_num].procedureInProgress == procedureInProgress_free)
        {
            if(centralConnList[conn_num].pMsg != NULL)
            {
                if(centralConnList[conn_num].pMsg_wlen != 0)
                {
                    req.len = centralConnList[conn_num].pMsg_wlen - centralConnList[conn_num].pMsg_len;     /* 计算还需写入的长度 */
                    if(req.len > (centralConnList[conn_num].mtu - 3))   /* 如果本次写入长度大于MTU-3 */
                    {
                        req.len = (centralConnList[conn_num].mtu - 3);  /* 本次写入长度改为MTU-3 */
                    }
                    req.pValue = GATT_bm_alloc(centralConnList[conn_num].connHandle, ATT_WRITE_CMD, req.len, NULL, 0);
                    if(req.pValue != NULL)
                    {
                        uint8_t ret;

                        req.cmd = FALSE;
                        req.sig = FALSE;
                        req.handle = centralConnList[conn_num].writeHdl;
                        tmos_memcpy(req.pValue, centralConnList[conn_num].pMsg + centralConnList[conn_num].pMsg_len, req.len);

                        ret = GATT_WriteCharValue(centralConnList[conn_num].connHandle, &req, centralConnList[conn_num].taskID);

                        if(ret == SUCCESS)
                        {
                            PRINT("write OK\r\n");
                            centralConnList[conn_num].pMsg_len = centralConnList[conn_num].pMsg_len + req.len;
                            if(centralConnList[conn_num].pMsg_len >= centralConnList[conn_num].pMsg_wlen)
                            {
                                tmos_memory_free(centralConnList[conn_num].pMsg);
                                centralConnList[conn_num].pMsg = NULL;
                                PRINT("all buf write down\r\n");
                                UART_SEND_AT_STRING("+SPPWRITE:\r\nADDR:");
                                at_uart_send_hex(centralConnList[conn_num].peerAddr, 6);
                                UART_SEND_AT_STRING("\r\nDONE\r\n");
                            }
                            else
                            {
                                goto spp_restart_later;
                            }
                        }
                        else
                        {
                            PRINT("write error_______%d_____\n",ret);
                            GATT_bm_free((gattMsg_t *)&req, ATT_WRITE_REQ);
                            goto spp_restart_later;
                        }

                    }
                    else
                    {
                        goto spp_restart_later;
                    }
                }
                else
                {
                    /* 已经写入完成 */
                    tmos_memory_free(centralConnList[conn_num].pMsg);
                    centralConnList[conn_num].pMsg = NULL;
                }
            }
            else
            {
                /* 没有消息需要发送，结束 */
            }
        }
        else
        {
spp_restart_later:
            tmos_start_task(task_id, SPP_WRITE_EVT, MS1_TO_SYSTEM_TIME(30));    /* 30ms后重试 */
        }
        return (events ^ SPP_WRITE_EVT);
    }

    if(events & CUSTOM_WRITE_EVT)
    {
        attWriteReq_t req;
        if(centralConnList[conn_num].procedureInProgress == procedureInProgress_free)
        {
            if(centralConnList[conn_num].pMsg != NULL)
             {
                 if(centralConnList[conn_num].pMsg_wlen != 0)
                 {
                     req.len = centralConnList[conn_num].pMsg_wlen - centralConnList[conn_num].pMsg_len;     /* 计算还需写入的长度 */
                     if(req.len > (centralConnList[conn_num].mtu - 3))   /* 如果本次写入长度大于MTU-3 */
                     {
                         req.len = (centralConnList[conn_num].mtu - 3);  /* 本次写入长度改为MTU-3 */
                     }
                     req.pValue = GATT_bm_alloc(centralConnList[conn_num].connHandle, ATT_WRITE_CMD, req.len, NULL, 0);
                     if(req.pValue != NULL)
                     {
                         uint8_t ret;

                         req.cmd = FALSE;
                         req.sig = FALSE;
                         req.handle = centralConnList[conn_num].customHdl;
                         tmos_memcpy(req.pValue, centralConnList[conn_num].pMsg + centralConnList[conn_num].pMsg_len, req.len);

                         ret = GATT_WriteCharValue(centralConnList[conn_num].connHandle, &req, centralConnList[conn_num].taskID);

                         if(ret == SUCCESS)
                         {
                             PRINT("write OK\r\n");
                             centralConnList[conn_num].pMsg_len = centralConnList[conn_num].pMsg_len + req.len;
                             if(centralConnList[conn_num].pMsg_len >= centralConnList[conn_num].pMsg_wlen)
                             {
                                 tmos_memory_free(centralConnList[conn_num].pMsg);
                                 centralConnList[conn_num].pMsg = NULL;
                                 PRINT("all buf write down\r\n");
                                 UART_SEND_AT_STRING("+CUSTOMWRITE:\r\nADDR:");
                                 at_uart_send_hex(centralConnList[conn_num].peerAddr, 6);
                                 UART_SEND_AT_STRING("\r\nDONE\r\n");
                             }
                             else
                             {
                                 goto cw_restart_later;
                             }
                         }
                         else
                         {
                             PRINT("write error_______%d_____\n",ret);
                             GATT_bm_free((gattMsg_t *)&req, ATT_WRITE_REQ);
                             goto cw_restart_later;
                         }

                     }
                     else
                     {
                         goto cw_restart_later;
                     }
                 }
                 else
                 {
                     /* 已经写入完成 */
                     tmos_memory_free(centralConnList[conn_num].pMsg);
                     centralConnList[conn_num].pMsg = NULL;
                 }
             }
             else
             {
                 /* 没有消息需要发送，结束 */
             }
        }
        else
        {
cw_restart_later:
            tmos_start_task(task_id, CUSTOM_WRITE_EVT, MS1_TO_SYSTEM_TIME(30));    /* 30ms后重试 */
        }
        return (events ^ CUSTOM_WRITE_EVT);
    }

    if(events & CUSTOM_READ_EVT)
    {
        if(centralConnList[conn_num].procedureInProgress == procedureInProgress_free)
        {
            // Do a read
            attReadReq_t req;

            req.handle = centralConnList[conn_num].customHdl;
            if(GATT_ReadCharValue(centralConnList[conn_num].connHandle, &req, centralTaskId) == SUCCESS)
            {
                centralConnList[conn_num].procedureInProgress = procedureInProgress_read;
            }
            else
            {
                goto cr_restart_later;
            }
        }
        else
        {
cr_restart_later:
            tmos_start_task(task_id, CUSTOM_READ_EVT, MS1_TO_SYSTEM_TIME(30));    /* 30ms后重试 */
        }
        return (events ^ CUSTOM_READ_EVT);
    }

    if(events & START_WRITE_CCCD_EVT)
    {
        if(centralConnList[conn_num].procedureInProgress == procedureInProgress_free)
        {
            // Do a write
            attWriteReq_t req;

            req.cmd = FALSE;
            req.sig = FALSE;
            req.handle = centralConnList[conn_num].cccHdl;
            req.len = 2;
            req.pValue = GATT_bm_alloc(centralConnList[conn_num].connHandle, ATT_WRITE_REQ, req.len, NULL, 0);
            if(req.pValue != NULL)
            {
                req.pValue[0] = 1;
                req.pValue[1] = 0;
                if(GATT_WriteCharValue(centralConnList[conn_num].connHandle, &req, centralTaskId) == SUCCESS)
                {
                    centralConnList[conn_num].procedureInProgress = procedureInProgress_cccd;  /* 写入cccd状态 */
                    PRINT("write cccd ok\n");
                }
                else
                {
                    GATT_bm_free((gattMsg_t *)&req, ATT_WRITE_REQ);
                    PRINT("write cccd failed\n");
                    goto cccd_restart_later;
                }
            }
            else
            {
                goto cccd_restart_later;
            }
        }
        else
        {
cccd_restart_later:
            tmos_start_task(task_id, START_WRITE_CCCD_EVT, MS1_TO_SYSTEM_TIME(30));    /* 30ms后重试 */
        }
        return (events ^ START_WRITE_CCCD_EVT);
    }

    // Discard unknown events
    return 0;
}
/*********************************************************************
 * @fn      central_ProcessTMOSMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void central_ProcessTMOSMsg(tmos_event_hdr_t *pMsg)
{
    switch(pMsg->event)
    {
        case GATT_MSG_EVENT:
            centralProcessGATTMsg((gattMsgEvent_t *)pMsg);
            break;
    }
}

/*********************************************************************
 * @fn      centralProcessGATTMsg
 *
 * @brief   Process GATT messages
 *
 * @return  none
 */
static void centralProcessGATTMsg(gattMsgEvent_t *pMsg)
{
    uint8_t connItem;
    for(connItem = 0; connItem < CENTRAL_MAX_CONNECTION; connItem++)
    {
        if(centralConnList[connItem].connHandle == pMsg->connHandle)
            break;
    }
    if(connItem == CENTRAL_MAX_CONNECTION)
    {
        return;
        // Should not go there
    }

    if(centralConnList[connItem].state != BLE_STATE_CONNECTED)
    {
        // In case a GATT message came after a connection has dropped,
        // ignore the message
        GATT_bm_free(&pMsg->msg, pMsg->method);
        return;
    }

    if((pMsg->method == ATT_EXCHANGE_MTU_RSP) ||
       ((pMsg->method == ATT_ERROR_RSP) &&
        (pMsg->msg.errorRsp.reqOpcode == ATT_EXCHANGE_MTU_REQ)))
    {
        if(pMsg->method == ATT_ERROR_RSP)
        {
            uint8_t status = pMsg->msg.errorRsp.errCode;

            PRINT("Exchange MTU Error: %x\n", status);
        }
        centralConnList[connItem].procedureInProgress = procedureInProgress_free;
    }

    if(pMsg->method == ATT_MTU_UPDATED_EVENT)
    {
        char ascii_at[16];
        PRINT("MTU: %x\n", pMsg->msg.mtuEvt.MTU);
        UART_SEND_AT_STRING("+MTUUPDATE:");
        at_uart_send_hex(centralConnList[connItem].peerAddr, 6);
        sprintf_(ascii_at, ",VALUE:%d\r\n", pMsg->msg.mtuEvt.MTU);
        UART_SEND_AT_VSTRING(ascii_at);
        centralConnList[connItem].mtu = pMsg->msg.mtuEvt.MTU;

    }

    if((pMsg->method == ATT_READ_RSP) ||
       ((pMsg->method == ATT_ERROR_RSP) &&
        (pMsg->msg.errorRsp.reqOpcode == ATT_READ_REQ)))
    {
        char rec_len[16];
        if(pMsg->method == ATT_ERROR_RSP)
        {
            uint8_t status = pMsg->msg.errorRsp.errCode;
            PRINT("Read Error: %x\n", status);
            if(centralConnList[connItem].procedureInProgress == procedureInProgress_read)
            {
                UART_SEND_AT_STRING("+CUSTOMREAD:\r\nADDR:");
                at_uart_send_hex(centralConnList[connItem].peerAddr, 6);
                sprintf_(rec_len, ",0x%02x", pMsg->msg.errorRsp.errCode);
                UART_SEND_AT_VSTRING(rec_len);
                UART_SEND_AT_STRING("\r\nERR\r\n");
                centralConnList[connItem].procedureInProgress = procedureInProgress_free;
            }
        }
        else
        {
            // After a successful read, display the read value
            if(centralConnList[connItem].procedureInProgress == procedureInProgress_read)
            {
                UART_SEND_AT_STRING("+CUSTOMREAD:\r\nADDR:");
                at_uart_send_hex(centralConnList[connItem].peerAddr, 6);
                sprintf_(rec_len, ",LEN:%d,DATA:\r\n", pMsg->msg.readRsp.len);
                UART_SEND_AT_VSTRING(rec_len);
                if(pMsg->msg.readRsp.len != 0)
                {
                    uart_at_send_data(pMsg->msg.readRsp.pValue, pMsg->msg.readRsp.len);
                }
                UART_SEND_AT_STRING("\r\nOK\r\n");
                centralConnList[connItem].procedureInProgress = procedureInProgress_free;
            }
        }
    }
    else if((pMsg->method == ATT_WRITE_RSP) ||
            ((pMsg->method == ATT_ERROR_RSP) &&
             (pMsg->msg.errorRsp.reqOpcode == ATT_WRITE_REQ)))
    {
        if(pMsg->method == ATT_ERROR_RSP == ATT_ERROR_RSP)
        {
            uint8_t status = pMsg->msg.errorRsp.errCode;
            if(centralConnList[connItem].procedureInProgress == procedureInProgress_cccd)
            {
                UART_SEND_AT_STRING("+NOTIFY:");
                at_uart_send_hex(centralConnList[connItem].peerAddr, 6);
                UART_SEND_AT_STRING(",ERR,RETRY\r\n");
                tmos_start_task(centralConnList[connItem].taskID, START_WRITE_CCCD_EVT, MS1_TO_SYSTEM_TIME(30));    /* 30ms后重试 */
            }
            PRINT("Write Error: %x\n", status);
        }
        else
        {
            // After a succesful write, display the value that was written and increment value
//            PRINT("Write sent: %x\n", centralCharVal);
            if(centralConnList[connItem].procedureInProgress == procedureInProgress_cccd)
            {
                UART_SEND_AT_STRING("+NOTIFY:");
                at_uart_send_hex(centralConnList[connItem].peerAddr, 6);
                UART_SEND_AT_STRING(",READY\r\n");
            }
        }
        centralConnList[connItem].procedureInProgress = procedureInProgress_free;
    }
    else if(pMsg->method == ATT_HANDLE_VALUE_NOTI)
    {
        char rec_len[16];
        UART_SEND_AT_STRING("+NOTIFY:\r\nADDR:");
        at_uart_send_hex(centralConnList[connItem].peerAddr, 6);
        sprintf_(rec_len, ",LEN:%d,DATA:\r\n", pMsg->msg.handleValueNoti.len);
        UART_SEND_AT_VSTRING(rec_len);

        uart_at_send_data(pMsg->msg.handleValueNoti.pValue, pMsg->msg.handleValueNoti.len);
        UART_SEND_AT_STRING("\r\nOK\r\n");
    }
    else if(centralConnList[connItem].discState != BLE_DISC_STATE_IDLE)
    {
        centralGATTDiscoveryEvent(connItem, pMsg);
    }
    GATT_bm_free(&pMsg->msg, pMsg->method);
}

/*********************************************************************
 * @fn      centralRssiCB
 *
 * @brief   RSSI callback.
 *
 * @param   connHandle - connection handle
 * @param   rssi - RSSI
 *
 * @return  none
 */
static void centralRssiCB(uint16_t connHandle, int8_t rssi)
{
    PRINT("RSSI -%d dB Conn - %x \n", -rssi, connHandle);
}

/*********************************************************************
 * @fn      centralHciMTUChangeCB
 *
 * @brief   MTU changed callback.
 *
 * @param   maxTxOctets - Max tx octets
 * @param   maxRxOctets - Max rx octets
 *
 * @return  none
 */
static void centralHciMTUChangeCB(uint16_t connHandle, uint16_t maxTxOctets, uint16_t maxRxOctets)
{
    attExchangeMTUReq_t req;

    uint8_t connItem;
    for(connItem = 0; connItem < CENTRAL_MAX_CONNECTION; connItem++)
    {
        if(centralConnList[connItem].connHandle == connHandle)
            break;
    }
    if(connItem == CENTRAL_MAX_CONNECTION)
    {
        return;
        // Should not go there
    }

    req.clientRxMTU = maxRxOctets;
    GATT_ExchangeMTU(connHandle, &req, centralTaskId);
    PRINT("exchange mtu:%d\n", maxRxOctets);
    centralConnList[connItem].procedureInProgress = procedureInProgress_mtu;
}

/*********************************************************************
 * @fn      centralEventCB
 *
 * @brief   Central event callback function.
 *
 * @param   pEvent - pointer to event structure
 *
 * @return  none
 */
static void centralEventCB(gapRoleEvent_t *pEvent)
{
    switch(pEvent->gap.opcode)
    {
        case GAP_DEVICE_INIT_DONE_EVENT:
        {
            if(multiCentral_info.scan_enabled == TRUE)
            {
                PRINT("Discovering...\n");
                GAPRole_CentralStartDiscovery(DEFAULT_DISCOVERY_MODE,
                                              DEFAULT_DISCOVERY_ACTIVE_SCAN,
                                              DEFAULT_DISCOVERY_WHITE_LIST);
            }
        }
        break;

        case GAP_DEVICE_INFO_EVENT:
        {
            // Add device to list
            centralAddDeviceInfo(pEvent->deviceInfo.addr, pEvent->deviceInfo.addrType);
        }
        break;

        case GAP_DEVICE_DISCOVERY_EVENT:
        {
            uint8_t i;

            // See if peer device has been discovered
            for(i = 0; i < centralScanRes; i++)
            {
                if(centralAddrCmp(PeerAddrDef, centralDevList[i].addr) != CENTRAL_MAX_CONNECTION)
                    break;
            }

            // Peer device not found
            if(i == centralScanRes)
            {
                PRINT("Device not found...\n");
                centralScanRes = 0;
                if(multiCentral_info.scan_enabled == TRUE)
                {
                    GAPRole_CentralStartDiscovery(DEFAULT_DISCOVERY_MODE,
                                                  DEFAULT_DISCOVERY_ACTIVE_SCAN,
                                                  DEFAULT_DISCOVERY_WHITE_LIST);
                    PRINT("Discovering...\n");
                }
            }

            // Peer device found
            else
            {
                PRINT("Device found...\n");

                GAPRole_CentralEstablishLink(DEFAULT_LINK_HIGH_DUTY_CYCLE,
                                             DEFAULT_LINK_WHITE_LIST,
                                             centralDevList[i].addrType,
                                             centralDevList[i].addr);

//                centralDevList[i]
                // Start establish link timeout event
                tmos_start_task(centralTaskId, ESTABLISH_LINK_TIMEOUT_EVT, ESTABLISH_LINK_TIMEOUT);
                PRINT("Connecting...\n");
            }
        }
        break;

        case GAP_LINK_ESTABLISHED_EVENT:
        {
            tmos_stop_task(centralTaskId, ESTABLISH_LINK_TIMEOUT_EVT);
            if(pEvent->gap.hdr.status == SUCCESS)
            {
                uint8_t connItem, peerAddrItem;
                // 查询是否有空余连接条目
                for(connItem = 0; connItem < CENTRAL_MAX_CONNECTION; connItem++)
                {
                    if(centralConnList[connItem].connHandle == GAP_CONNHANDLE_INIT)
                        break;
                }
                peerAddrItem = centralAddrCmp(PeerAddrDef, pEvent->linkCmpl.devAddr);

                if((connItem == CENTRAL_MAX_CONNECTION) || (peerAddrItem == CENTRAL_MAX_CONNECTION))
                {
                    GAPRole_TerminateLink(pEvent->linkCmpl.connectionHandle);
                    PRINT("Connection max...\n");
                }
                else
                {
                    centralConnList[connItem].state = BLE_STATE_CONNECTED;
                    centralConnList[connItem].connHandle = pEvent->linkCmpl.connectionHandle;

                    tmos_memcpy(centralConnList[connItem].peerAddr, pEvent->linkCmpl.devAddr, 6);

                    PRINT("Conn %x - Int %x \n", pEvent->linkCmpl.connectionHandle, pEvent->linkCmpl.connInterval);

                    if(PeerAddrDef[peerAddrItem].gsi_select_num < MASTER_SERVICE_CONST_NUM)
                    {
                        centralConnList[connItem].gsi_select = (void *)&default_gatt_service_info[PeerAddrDef[peerAddrItem].gsi_select_num];
                    }
                    else
                    {
                        centralConnList[connItem].gsi_select = (void *)&vendor_def_service;
                    }

                    centralConnList[connItem].procedureInProgress = procedureInProgress_disc;

                    // Initiate service discovery
                    tmos_start_task(centralConnList[connItem].taskID, START_SVC_DISCOVERY_EVT, DEFAULT_SVC_DISCOVERY_DELAY);

                    // Initiate connect parameter update
                    tmos_start_task(centralConnList[connItem].taskID, START_PARAM_UPDATE_EVT, DEFAULT_PARAM_UPDATE_DELAY);

                    PRINT("Connected...\n");

                    UART_SEND_AT_STRING("+CONNECTED:");
                    at_uart_send_hex(centralConnList[connItem].peerAddr, 6);
                    UART_SEND_AT_STRING("\r\nOK\r\n");

                    centralConnList[connItem].mtu = 23; /* 默认MTU */
                    centralConnList[connItem].pMsg = NULL;
                    // See if need discover again
                    for(connItem = 0; connItem < CENTRAL_MAX_CONNECTION; connItem++)
                    {
                        if(centralConnList[connItem].connHandle == GAP_CONNHANDLE_INIT)
                            break;
                    }
                    if(connItem < CENTRAL_MAX_CONNECTION)
                    {
                        PRINT("Discovering...\n");
                        centralScanRes = 0;
                        if(multiCentral_info.scan_enabled == TRUE)
                        {
                            GAPRole_CentralStartDiscovery(DEFAULT_DISCOVERY_MODE,
                                                          DEFAULT_DISCOVERY_ACTIVE_SCAN,
                                                          DEFAULT_DISCOVERY_WHITE_LIST);
                        }
                    }
                }
            }
            else
            {
                PRINT("Connect Failed...Reason:%X\n", pEvent->gap.hdr.status);
                PRINT("Discovering...\n");
                centralScanRes = 0;
                if(multiCentral_info.scan_enabled == TRUE)
                {
                    GAPRole_CentralStartDiscovery(DEFAULT_DISCOVERY_MODE,
                                                  DEFAULT_DISCOVERY_ACTIVE_SCAN,
                                                  DEFAULT_DISCOVERY_WHITE_LIST);
                }
            }
        }
        break;

        case GAP_LINK_TERMINATED_EVENT:
        {
            uint8_t connItem;
            char print_ascii[16];

            for(connItem = 0; connItem < CENTRAL_MAX_CONNECTION; connItem++)
            {
                if(centralConnList[connItem].connHandle == pEvent->linkTerminate.connectionHandle)
                    break;
            }
            if(connItem == CENTRAL_MAX_CONNECTION)
            {
                // Should not go there
            }

            UART_SEND_AT_STRING("DISCONNECTED:");
            at_uart_send_hex(centralConnList[connItem].peerAddr, 6);
            sprintf_(print_ascii, ", REASON: 0x%02x\r\n");
            UART_SEND_AT_VSTRING(print_ascii);

            PRINT("  %x  Disconnected...Reason:%x\n", centralConnList[connItem].connHandle, pEvent->linkTerminate.reason);
            centralConnList[connItem].state = BLE_STATE_IDLE;
            centralConnList[connItem].connHandle = GAP_CONNHANDLE_INIT;
            centralConnList[connItem].discState = BLE_DISC_STATE_IDLE;
            centralConnList[connItem].procedureInProgress = procedureInProgress_free;
            centralScanRes = 0;
            tmos_memset(centralConnList[connItem].peerAddr, 0, 6);

            if(centralConnList[connItem].pMsg != NULL)
            {
                tmos_memory_free(centralConnList[connItem].pMsg);
                centralConnList[connItem].pMsg = NULL;
            }
            tmos_stop_task(centralConnList[connItem].taskID, START_WRITE_CCCD_EVT);
            tmos_stop_task(centralConnList[connItem].taskID, SPP_WRITE_EVT);
            tmos_stop_task(centralConnList[connItem].taskID, CUSTOM_WRITE_EVT);
            tmos_stop_task(centralConnList[connItem].taskID, CUSTOM_READ_EVT);

            PRINT("Discovering...\n");
            if(multiCentral_info.scan_enabled == TRUE)
            {
                GAPRole_CentralStartDiscovery(DEFAULT_DISCOVERY_MODE,
                                              DEFAULT_DISCOVERY_ACTIVE_SCAN,
                                              DEFAULT_DISCOVERY_WHITE_LIST);
            }
        }
        break;

        case GAP_LINK_PARAM_UPDATE_EVENT:
        {
            PRINT("Update %x - Int %x \n", pEvent->linkUpdate.connectionHandle, pEvent->linkUpdate.connInterval);
        }
        break;

        default:
            break;
    }
}

/*********************************************************************
 * @fn      pairStateCB
 *
 * @brief   Pairing state callback.
 *
 * @return  none
 */
static void centralPairStateCB(uint16_t connHandle, uint8_t state, uint8_t status)
{
    if(state == GAPBOND_PAIRING_STATE_STARTED)
    {
        PRINT("Connection %04x - Pairing started:%d\n", connHandle, status);
    }
    else if(state == GAPBOND_PAIRING_STATE_COMPLETE)
    {
        if(status == SUCCESS)
        {
            PRINT("Connection %04x - Pairing success\n", connHandle);
        }
        else
        {
            PRINT("Connection %04x - Pairing fail\n", connHandle);
        }
    }
    else if(state == GAPBOND_PAIRING_STATE_BONDED)
    {
        if(status == SUCCESS)
        {
            PRINT("Connection %04x - Bonding success\n", connHandle);
        }
    }
    else if(state == GAPBOND_PAIRING_STATE_BOND_SAVED)
    {
        if(status == SUCCESS)
        {
            PRINT("Connection %04x - Bond save success\n", connHandle);
        }
        else
        {
            PRINT("Connection %04x - Bond save failed: %d\n", connHandle, status);
        }
    }
}

/*********************************************************************
 * @fn      centralPasscodeCB
 *
 * @brief   Passcode callback.
 *
 * @return  none
 */
static void centralPasscodeCB(uint8_t *deviceAddr, uint16_t connectionHandle,
                              uint8_t uiInputs, uint8_t uiOutputs)
{
    uint32_t passcode;
    uint8_t dev;

    dev = centralAddrCmp(PeerAddrDef, deviceAddr);

    if(dev < CENTRAL_MAX_CONNECTION)
    {
        passcode = PeerAddrDef[dev].password;
    }
    else
    {
        passcode = 123456;
    }

    // Display passcode to user
    PRINT("Passcode:%06d\n", (int)passcode);

    // Send passcode response
    GAPBondMgr_PasscodeRsp(connectionHandle, SUCCESS, 123456);
}

/*********************************************************************
 * @fn      centralConnIistStartDiscovery
 *
 * @brief   Start connection service discovery.
 *
 * @return  none
 */
static void centralConnIistStartDiscovery(uint8_t connNum)
{
    uint8_t uuid[ATT_UUID_SIZE];

    if((centralConnList[connNum].gsi_select->service_uuid_len == ATT_UUID_SIZE) || (centralConnList[connNum].gsi_select->service_uuid_len == ATT_BT_UUID_SIZE))
    {
        PRINT("centralConnIistStartDiscovery\n");
        // Initialize cached handles
        centralConnList[connNum].svcStartHdl =
            centralConnList[connNum].svcEndHdl =
                centralConnList[connNum].writeHdl =
                    centralConnList[connNum].cccHdl =
                        centralConnList[connNum].notifyHdl =
                            centralConnList[connNum].customHdl = 0;

        centralConnList[connNum].discState = BLE_DISC_STATE_SVC;

        tmos_memcpy(uuid, centralConnList[connNum].gsi_select->service_uuid, centralConnList[connNum].gsi_select->service_uuid_len);

        // Discovery simple BLE service
        GATT_DiscPrimaryServiceByUUID(centralConnList[connNum].connHandle,
                                      uuid,
                                      ATT_UUID_SIZE,
                                      centralTaskId);
    }
}

/*********************************************************************
 * @fn      centralGATTDiscoveryEvent
 *
 * @brief   Process GATT discovery event
 *
 * @return  none
 */
static void centralGATTDiscoveryEvent(uint8_t connItem, gattMsgEvent_t *pMsg)
{
    attReadByTypeReq_t req;

    if(centralConnList[connItem].discState == BLE_DISC_STATE_SVC)
    {
        // Service found, store handles
        if(pMsg->method == ATT_FIND_BY_TYPE_VALUE_RSP &&
           pMsg->msg.findByTypeValueRsp.numInfo > 0)
        {
            centralConnList[connItem].svcStartHdl = ATT_ATTR_HANDLE(pMsg->msg.findByTypeValueRsp.pHandlesInfo, 0);
            centralConnList[connItem].svcEndHdl = ATT_GRP_END_HANDLE(pMsg->msg.findByTypeValueRsp.pHandlesInfo, 0);

            // Display Profile Service handle range
            PRINT("Found Service1 handle : %x ~ %x \n", centralConnList[connItem].svcStartHdl, centralConnList[connItem].svcEndHdl);
        }
        // If procedure complete
        if((pMsg->method == ATT_FIND_BY_TYPE_VALUE_RSP &&
            pMsg->hdr.status == bleProcedureComplete) ||
           (pMsg->method == ATT_ERROR_RSP))
        {
            if(centralConnList[connItem].svcStartHdl != 0)
            {
                // Discover characteristic
                req.startHandle = centralConnList[connItem].svcStartHdl;
                req.endHandle = centralConnList[connItem].svcEndHdl;

                if((centralConnList[connItem].gsi_select->notify_uuid_len == ATT_BT_UUID_SIZE)
                    || (centralConnList[connItem].gsi_select->notify_uuid_len == ATT_UUID_SIZE))
                {
                    centralConnList[connItem].discState = BLE_DISC_STATE_NOTIFY_CHAR;
                    req.type.len = centralConnList[connItem].gsi_select->notify_uuid_len;
                    tmos_memcpy(req.type.uuid, centralConnList[connItem].gsi_select->notify_uuid, centralConnList[connItem].gsi_select->notify_uuid_len);
                }
                else if((centralConnList[connItem].gsi_select->write_uuid_len == ATT_BT_UUID_SIZE)
                    || (centralConnList[connItem].gsi_select->write_uuid_len == ATT_UUID_SIZE))
                {
                    centralConnList[connItem].discState = BLE_DISC_STATE_WRITE_CHAR;
                    req.type.len = centralConnList[connItem].gsi_select->write_uuid_len;
                    tmos_memcpy(req.type.uuid, centralConnList[connItem].gsi_select->write_uuid, centralConnList[connItem].gsi_select->write_uuid_len);
                }
                else if((centralConnList[connItem].gsi_select->custom_uuid_len == ATT_BT_UUID_SIZE)
                    || (centralConnList[connItem].gsi_select->custom_uuid_len == ATT_UUID_SIZE))
                {
                    centralConnList[connItem].discState = BLE_DISC_STATE_CUSTOM_CHAR;
                    req.type.len = centralConnList[connItem].gsi_select->custom_uuid_len;
                    tmos_memcpy(req.type.uuid, centralConnList[connItem].gsi_select->custom_uuid, centralConnList[connItem].gsi_select->custom_uuid_len);
                }
                else
                {
                    /* 没有合法的UUID需要枚举使用，退出枚举流程 */
                    centralConnList[connItem].procedureInProgress = procedureInProgress_free;
                    centralConnList[connItem].discState = BLE_DISC_STATE_IDLE;
                    return;
                }
                GATT_DiscCharsByUUID(centralConnList[connItem].connHandle, &req, centralTaskId);
            }
        }
    }
    else if(centralConnList[connItem].discState == BLE_DISC_STATE_NOTIFY_CHAR)
    {
        // Characteristic found, store handle
        if(pMsg->method == ATT_READ_BY_TYPE_RSP &&
           pMsg->msg.readByTypeRsp.numPairs > 0)
        {
            centralConnList[connItem].notifyHdl = BUILD_UINT16(pMsg->msg.readByTypeRsp.pDataList[0],
                                                             pMsg->msg.readByTypeRsp.pDataList[1]) + 1;

            // Display notify handle
            PRINT("Found notify handle : %x \n", centralConnList[connItem].notifyHdl);
        }

        if((pMsg->method == ATT_READ_BY_TYPE_RSP &&
            pMsg->hdr.status == bleProcedureComplete) ||
           (pMsg->method == ATT_ERROR_RSP))
        {
            // Discover cccd
            centralConnList[connItem].discState = BLE_DISC_STATE_NOTIFY_CCCD;
            req.startHandle = centralConnList[connItem].svcStartHdl;
            req.endHandle = centralConnList[connItem].svcEndHdl;
            req.type.len = ATT_BT_UUID_SIZE;
            req.type.uuid[0] = LO_UINT16(GATT_CLIENT_CHAR_CFG_UUID);
            req.type.uuid[1] = HI_UINT16(GATT_CLIENT_CHAR_CFG_UUID);

            GATT_ReadUsingCharUUID(centralConnList[connItem].connHandle, &req, centralTaskId);
        }
    }
    else if(centralConnList[connItem].discState == BLE_DISC_STATE_NOTIFY_CCCD)
    {
        // Characteristic found, store handle
        if(pMsg->method == ATT_READ_BY_TYPE_RSP &&
           pMsg->msg.readByTypeRsp.numPairs > 0)
        {
            centralConnList[connItem].cccHdl = BUILD_UINT16(pMsg->msg.readByTypeRsp.pDataList[0],
                                                             pMsg->msg.readByTypeRsp.pDataList[1]);

            // Display Characteristic 1 handle
            PRINT("Found cccd : %x \n", centralConnList[connItem].cccHdl); //2d
        }

        if((pMsg->method == ATT_READ_BY_TYPE_RSP &&
            pMsg->hdr.status == bleProcedureComplete) ||
           (pMsg->method == ATT_ERROR_RSP))
        {
            // Discover characteristic
            req.startHandle = centralConnList[connItem].notifyHdl;
            req.endHandle = centralConnList[connItem].svcEndHdl;

            if((centralConnList[connItem].gsi_select->write_uuid_len == ATT_BT_UUID_SIZE)
                || (centralConnList[connItem].gsi_select->write_uuid_len == ATT_UUID_SIZE))
            {
                centralConnList[connItem].discState = BLE_DISC_STATE_WRITE_CHAR;
                req.type.len = centralConnList[connItem].gsi_select->write_uuid_len;
                tmos_memcpy(req.type.uuid, centralConnList[connItem].gsi_select->write_uuid, centralConnList[connItem].gsi_select->write_uuid_len);
            }
            else
            {
                if(centralConnList[connItem].gsi_select->write_uuid_len == 0xff)
                {
                    /* 表明write和notify占用同一个通道 */
                    centralConnList[connItem].writeHdl = centralConnList[connItem].notifyHdl;
                }
                if((centralConnList[connItem].gsi_select->custom_uuid_len == ATT_BT_UUID_SIZE)
                    || (centralConnList[connItem].gsi_select->custom_uuid_len == ATT_UUID_SIZE))
                {
                    centralConnList[connItem].discState = BLE_DISC_STATE_CUSTOM_CHAR;
                    req.type.len = centralConnList[connItem].gsi_select->custom_uuid_len;
                    tmos_memcpy(req.type.uuid, centralConnList[connItem].gsi_select->custom_uuid, centralConnList[connItem].gsi_select->custom_uuid_len);
                }
                else
                {

                    // Start do write CCCD
                    tmos_start_task(centralConnList[connItem].taskID, START_WRITE_CCCD_EVT, DEFAULT_WRITE_CCCD_DELAY);
                    centralConnList[connItem].procedureInProgress = procedureInProgress_free;
                    /* 没有合法的UUID需要枚举使用，退出枚举流程 */
                    centralConnList[connItem].discState = BLE_DISC_STATE_IDLE;
                    return;
                }
            }
            GATT_DiscCharsByUUID(centralConnList[connItem].connHandle, &req, centralTaskId);
        }
    }
    else if(centralConnList[connItem].discState == BLE_DISC_STATE_WRITE_CHAR)
    {
        // Characteristic found, store handle
        if(pMsg->method == ATT_READ_BY_TYPE_RSP &&
           pMsg->msg.readByTypeRsp.numPairs > 0)
        {
            centralConnList[connItem].writeHdl = BUILD_UINT16(pMsg->msg.readByTypeRsp.pDataList[0],
                                                             pMsg->msg.readByTypeRsp.pDataList[1]) + 1;

            // Display write handle
            PRINT("Found write handle : %x \n", centralConnList[connItem].writeHdl);
        }

        if((pMsg->method == ATT_READ_BY_TYPE_RSP &&
            pMsg->hdr.status == bleProcedureComplete) ||
           (pMsg->method == ATT_ERROR_RSP))
        {
            // Discover characteristic
            req.startHandle = centralConnList[connItem].svcStartHdl;
            req.endHandle = centralConnList[connItem].svcEndHdl;

            if((centralConnList[connItem].gsi_select->custom_uuid_len == ATT_BT_UUID_SIZE)
                || (centralConnList[connItem].gsi_select->custom_uuid_len == ATT_UUID_SIZE))
            {
                centralConnList[connItem].discState = BLE_DISC_STATE_CUSTOM_CHAR;
                req.type.len = centralConnList[connItem].gsi_select->custom_uuid_len;
                tmos_memcpy(req.type.uuid, centralConnList[connItem].gsi_select->custom_uuid, centralConnList[connItem].gsi_select->custom_uuid_len);
            }
            else
            {

                // Start do write CCCD
                tmos_start_task(centralConnList[connItem].taskID, START_WRITE_CCCD_EVT, DEFAULT_WRITE_CCCD_DELAY);
                centralConnList[connItem].procedureInProgress = procedureInProgress_free;
                /* 没有合法的UUID需要枚举使用，退出枚举流程 */
                centralConnList[connItem].discState = BLE_DISC_STATE_IDLE;
                return;
            }
            GATT_DiscCharsByUUID(centralConnList[connItem].connHandle, &req, centralTaskId);
        }
    }
    else if(centralConnList[connItem].discState == BLE_DISC_STATE_CUSTOM_CHAR)
    {
        // Characteristic found, store handle
        if(pMsg->method == ATT_READ_BY_TYPE_RSP && pMsg->msg.readByTypeRsp.numPairs > 0)
        {
            centralConnList[connItem].customHdl = BUILD_UINT16(pMsg->msg.readByTypeRsp.pDataList[0],
                                                             pMsg->msg.readByTypeRsp.pDataList[1]);

            // Start do write CCCD
            tmos_start_task(centralConnList[connItem].taskID, START_WRITE_CCCD_EVT, DEFAULT_WRITE_CCCD_DELAY);

            // Display custom handle
            PRINT("Found custom handle : %x \n", centralConnList[connItem].customHdl);
        }
        centralConnList[connItem].procedureInProgress = procedureInProgress_free;
        centralConnList[connItem].discState = BLE_DISC_STATE_IDLE;
    }
}

/*********************************************************************
 * @fn      centralAddDeviceInfo
 *
 * @brief   Add a device to the device discovery result list
 *
 * @return  none
 */
static void centralAddDeviceInfo(uint8_t *pAddr, uint8_t addrType)
{
    uint8_t i;

    // If result count not at max
    if(centralScanRes < DEFAULT_MAX_SCAN_RES)
    {
        // Check if device is already in scan results
        for(i = 0; i < centralScanRes; i++)
        {
            if(tmos_memcmp(pAddr, centralDevList[i].addr, B_ADDR_LEN))
            {
                return;
            }
        }
        // Add addr to scan result list
        tmos_memcpy(centralDevList[centralScanRes].addr, pAddr, B_ADDR_LEN);
        centralDevList[centralScanRes].addrType = addrType;
        // Increment scan result count
        centralScanRes++;

        UART_SEND_AT_STRING("+SCAN:");
        at_uart_send_hex(centralDevList[centralScanRes - 1].addr, 6);
        UART_SEND_AT_STRING("\r\n");
    }
}

/*********************************************************************
 * @fn      centralAddrCmp
 *
 * @brief   none
 *
 * @return  none
 */
static uint8_t centralAddrCmp(peerAddrDefItem_t *PeerAddrDef, uint8_t *addr)
{
    uint8_t i;
    for(i = 0; i < CENTRAL_MAX_CONNECTION; i++)
    {
        if(PeerAddrDef[i].enabled != FALSE)
        {
            if(tmos_memcmp(PeerAddrDef[i].peerAddr, addr, B_ADDR_LEN))
                break;
        }
    }
    return i;
}

/************************ endfile @ central **************************/
