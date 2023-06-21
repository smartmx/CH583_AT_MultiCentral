#include "at_cmd.h"
#include "uart_interface.h"
#include "stdlib.h"
#include "mystdlib.h"
#include "multiCentral.h"
#include "sys_info.h"

/* 系统版本号 */
#define SYS_VER "20230315_rc1"

void at_uart_send_hex(uint8_t *src, uint8_t len)
{
    char send_ascii[4];
    for(uint8_t i = 0; i < len; i++)
    {
        sprintf_(send_ascii, " %02x", src[i]);
        UART_SEND_AT_VSTRING(send_ascii);
    }
}

/*
 * 获取系统版本号
 */
AT_HASH_MATCH_EXPORT(ver)
{
    UART_SEND_AT_STRING("+VER:" SYS_VER "\r\nOK\r\n");
}

/*
 * 复位芯片
 */
AT_HASH_MATCH_EXPORT(reset)
{
    UART_SEND_AT_STRING("+RESET\r\nOK\r\n");
    while(ringbuf_elements(&os_at_uart_tx_ringbuf) != 0);
    while(R8_UART0_TFC != 0);
    PFIC_DisableAllIRQ();
    DelayMs(1);
    PFIC_SystemReset();
}

/*
 * 设置蓝牙MAC地址，获取蓝牙mac地址
 */
AT_HASH_MATCH_EXPORT(blemac)
{
    uint8_t rMacAddr[6];
    uint8_t *s;
    uint32_t num;
    char stp[2];

    if(p->data[0] == '=')
    {
        UART_SEND_AT_STRING("+BLEMAC");
        num = AscToHex(rMacAddr, p->data + 1, 6, ',', &s);
        if(num == 6)
        {
            if(tmos_isbufset(rMacAddr, 0xff, 6))
            {
                UART_SEND_AT_STRING("\r\nINVALID MAC\r\n");
                goto error;
            }
            if(ble_mac_set(rMacAddr) == 0)
            {
                goto error;
            }
            UART_SEND_AT_STRING("\r\nOK\r\n");
        }
        else
        {
error:
            UART_SEND_AT_STRING("ERR\r\n");
        }
    }
    else if((p->data[0] == '?') && (p->data_len == 3))
    {
        /* 发送当前的blemac地址 */
        UART_SEND_AT_STRING("+BLEMAC:");
        ble_mac_init(rMacAddr);
        at_uart_send_hex(rMacAddr, 6);
        UART_SEND_AT_STRING("\r\nOK\r\n");
    }
    else if((p->data_len == 4) && (p->data[0] == '&') && (p->data[1] == '0'))
    {
        /* 清除信息，使用芯片本身默认的MAC地址 */
        UART_SEND_AT_STRING("+BLEMAC&0:");
        tmos_memset(rMacAddr, 0xff, 6);
        if(ble_mac_set(rMacAddr) == 0)
        {
            goto error;
        }
        UART_SEND_AT_STRING("\r\nOK\r\n");
    }
    else
    {
        UART_SEND_AT_STRING("\r\nERR:02\r\n");
    }
}

/*
 * 获取芯片内部MAC地址
 */
AT_HASH_MATCH_EXPORT(rawmac)
{
    uint8_t rMacAddr[6];
    GetMACAddress(rMacAddr);
    UART_SEND_AT_STRING("+RAWMAC:");
    at_uart_send_hex(rMacAddr, 6);
    UART_SEND_AT_STRING("\r\nOK\r\n");
}

/*
 * AT+STARTSCAN命令，启用主机扫描
 */
AT_HASH_MATCH_EXPORT(startscan)
{
    bStatus_t res;
    char stp[2];
    stp[1] = 0;
    UART_SEND_AT_STRING("+STARTSCAN:");

    /* 开始主机扫描服务 */
    res = GAPRole_CentralStartDiscovery(DEVDISC_MODE_ALL, TRUE, FALSE);
    if(res == 0)
    {
        multiCentral_info.scan_enabled = TRUE;
        stp[0] = 0x30;
        UART_SEND_AT_VSTRING(stp);
        UART_SEND_AT_STRING("\r\nOK\r\n");
    }
    else
    {
        stp[0] = 0x31 - bleAlreadyInRequestedMode + res;
        UART_SEND_AT_VSTRING(stp);
        UART_SEND_AT_STRING("\r\nERR\r\n");
    }
}
/*
 * AT+STOPSCAN命令，启用主机扫描
 */
AT_HASH_MATCH_EXPORT(stopscan)
{
    bStatus_t res;
    char stp[2];
    stp[1] = 0;
    UART_SEND_AT_STRING("+STOPSCAN:");

    /* 开始主机扫描服务 */
    res = GAPRole_CentralCancelDiscovery();
    multiCentral_info.scan_enabled = FALSE;
    if(res == 0)
    {
        stp[0] = 0x30;
        UART_SEND_AT_VSTRING(stp);
        UART_SEND_AT_STRING("\r\nOK\r\n");
    }
    else
    {
        stp[0] = 0x31;
        UART_SEND_AT_VSTRING(stp);
        UART_SEND_AT_STRING("\r\nERR\r\n");
    }
}

/*
 * AT+CUSTOMREAD命令，从自定义通道读取数据
 */
AT_HASH_MATCH_EXPORT(customread)
{
    uint8_t *s, i, *mem;
    uint8_t rMacAddr[6];
    uint32_t num;
    if(p->data[0] == '=')
    {
        UART_SEND_AT_STRING("+CUSTOMREAD:");
        num = AscToHex(rMacAddr, p->data + 1, 6, '\r', &s);
        if(num == 6)
        {
            for(i = 0; i < CENTRAL_MAX_CONNECTION; i++)
            {
                if(tmos_memcmp(centralConnList[i].peerAddr, rMacAddr, 6))
                {
                    break;
                }
            }
            if(i != CENTRAL_MAX_CONNECTION)
            {
                /* 准备写 */
                if(centralConnList[i].customHdl != 0)
                {
                    if((tmos_get_task_timer(centralConnList[i].taskID, CUSTOM_READ_EVT) != 0)
                        || (centralConnList[i].procedureInProgress == procedureInProgress_read))
                    {
                        UART_SEND_AT_STRING("ALREADY IN READ PROCESS");
                        goto error;
                    }
                    else
                    {
                        tmos_set_event(centralConnList[i].taskID, CUSTOM_READ_EVT);
                        UART_SEND_AT_STRING("PREPARE TO READ");
                    }
                }
                else
                {
                    UART_SEND_AT_STRING("NO CUSTOM HANDLER");
                    goto error;
                }
            }
            else
            {
                UART_SEND_AT_STRING("DEVICE NOT CONNECTED");
                goto error;
            }

            UART_SEND_AT_STRING("\r\nOK\r\n");
        }
        else
        {
error:
            UART_SEND_AT_STRING("\r\nERR\r\n");
        }
    }
    else
    {
        UART_SEND_AT_STRING("\r\nERR:02\r\n");
    }
}

/*
 * AT+CUSTOMWRITE命令，向自定义通道发送数据
 */
AT_HASH_MATCH_EXPORT(customwrite)
{
    uint8_t *s, i, *mem;
    uint8_t rMacAddr[6];
    uint32_t num;
    if(p->data[0] == '=')
    {
        UART_SEND_AT_STRING("+CUSTOMWRITE:");
        num = AscToHex(rMacAddr, p->data + 1, 6, ',', &s);
        if(num == 6)
        {
            for(i = 0; i < CENTRAL_MAX_CONNECTION; i++)
            {
                if(tmos_memcmp(centralConnList[i].peerAddr, rMacAddr, 6))
                {
                    break;
                }
            }
            if(i != CENTRAL_MAX_CONNECTION)
            {
                /* 准备写 */
                if(centralConnList[i].pMsg == NULL)
                {
                    if(centralConnList[i].customHdl != 0)
                    {
                        s++;
                        centralConnList[i].pMsg_wlen = p->data_len - (s - p->data) - 2;
                        PRINT("len:%d\r\n", centralConnList[i].pMsg_wlen);
                        mem = tmos_memory_allocate(centralConnList[i].pMsg_wlen, 15);
                        if(mem != NULL)
                        {
                            centralConnList[i].pMsg = mem;
                            tmos_memcpy(mem, s, centralConnList[i].pMsg_wlen);
                            centralConnList[i].pMsg_len = 0;
                            tmos_set_event(centralConnList[i].taskID, CUSTOM_WRITE_EVT);
                            UART_SEND_AT_STRING("PREPARE TO WRITE");
                        }
                        else
                        {
                            UART_SEND_AT_STRING("MEMORY NOT ENOUGH");
                            goto error;
                        }
                    }
                    else
                    {
                        UART_SEND_AT_STRING("NO CUSTOM HANDLER");
                        goto error;
                    }
                }
                else
                {
                    UART_SEND_AT_STRING("DEVICE IS BUSY");
                    goto error;
                }
            }
            else
            {
                UART_SEND_AT_STRING("DEVICE NOT CONNECTED");
                goto error;
            }

            UART_SEND_AT_STRING("\r\nOK\r\n");
        }
        else
        {
error:
            UART_SEND_AT_STRING("\r\nERR\r\n");
        }
    }
    else
    {
        UART_SEND_AT_STRING("\r\nERR:02\r\n");
    }
}

/*
 * AT+SPPWRITE命令，向默认写入通道发送数据
 */
AT_HASH_MATCH_EXPORT(sppwrite)
{
    uint8_t *s, i, *mem;
    uint8_t rMacAddr[6];
    uint32_t num;
    if(p->data[0] == '=')
    {
        UART_SEND_AT_STRING("+SPPWRITE:");
        num = AscToHex(rMacAddr, p->data + 1, 6, ',', &s);
        if(num == 6)
        {
            for(i = 0; i < CENTRAL_MAX_CONNECTION; i++)
            {
                if(tmos_memcmp(centralConnList[i].peerAddr, rMacAddr, 6))
                {
                    break;
                }
            }
            if(i != CENTRAL_MAX_CONNECTION)
            {
                /* 准备写 */
                if(centralConnList[i].pMsg == NULL)
                {
                    if(centralConnList[i].writeHdl != 0)
                    {
                        s++;
                        centralConnList[i].pMsg_wlen = p->data_len - (s - p->data) - 2;
                        PRINT("len:%d\r\n", centralConnList[i].pMsg_wlen);
                        mem = tmos_memory_allocate(centralConnList[i].pMsg_wlen, 15);
                        if(mem != NULL)
                        {
                            centralConnList[i].pMsg = mem;
                            tmos_memcpy(mem, s, centralConnList[i].pMsg_wlen);
                            centralConnList[i].pMsg_len = 0;
                            tmos_set_event(centralConnList[i].taskID, SPP_WRITE_EVT);
                            UART_SEND_AT_STRING("PREPARE TO WRITE");
                        }
                        else
                        {
                            UART_SEND_AT_STRING("MEMORY NOT ENOUGH");
                            goto error;
                        }
                    }
                    else
                    {
                        UART_SEND_AT_STRING("NO WRITE HANDLER");
                        goto error;
                    }
                }
                else
                {
                    UART_SEND_AT_STRING("DEVICE IS BUSY");
                    goto error;
                }
            }
            else
            {
                UART_SEND_AT_STRING("DEVICE NOT CONNECTED");
                goto error;
            }

            UART_SEND_AT_STRING("\r\nOK\r\n");
        }
        else
        {
error:
            UART_SEND_AT_STRING("\r\nERR\r\n");
        }
    }
    else
    {
        UART_SEND_AT_STRING("\r\nERR:02\r\n");
    }
}

/*
 * AT+DISABLECONNECT命令，设置当前连接参数，不存放到flash，下次复位还是之前的
 */
AT_HASH_MATCH_EXPORT(disableconnect)
{
    uint8_t *s;
    uint32_t num, len;
    peerAddrDefItem_t info;
    char stp[16] = {0,0};
    if(p->data[0] == '=')
    {
        num = atoi(p->data + 1);
        UART_SEND_AT_STRING("+DISABLECONNECT\r\n");
        if(num >= CENTRAL_MAX_CONNECTION)
        {
            UART_SEND_AT_STRING("UNSUPPORT NUMBER\r\n");
            UART_SEND_AT_STRING("ERR\r\n");
        }
        else
        {
            PeerAddrDef[num].enabled = FALSE;
            UART_SEND_AT_STRING("OK\r\n");
        }
    }
    else
    {
        UART_SEND_AT_STRING("\r\nERR:02\r\n");
    }
}

/*
 * AT+ENABLECONNECT命令，设置当前连接参数，不存放到flash，下次复位还是之前的
 */
AT_HASH_MATCH_EXPORT(enableconnect)
{
    uint8_t *s;
    uint32_t num, len;
    peerAddrDefItem_t info;
    char stp[16] = {0,0};
    if(p->data[0] == '=')
    {
        num = atoi(p->data + 1);
        UART_SEND_AT_STRING("+ENABLECONNECT\r\n");
        if(num >= CENTRAL_MAX_CONNECTION)
        {
            UART_SEND_AT_STRING("UNSUPPORT NUMBER\r\n");
            UART_SEND_AT_STRING("ERR\r\n");
        }
        else
        {
            PeerAddrDef[num].enabled = TRUE;
            UART_SEND_AT_STRING("OK\r\n");
        }
    }
    else
    {
        UART_SEND_AT_STRING("\r\nERR:02\r\n");
    }
}

/*
 * AT+SETCONNECT命令，设置当前连接参数，不存放到flash，下次复位还是之前的
 */
AT_HASH_MATCH_EXPORT(setconnect)
{
    uint8_t *s;
    uint32_t num, len;
    peerAddrDefItem_t info;
    char stp[16] = {0,0};
    if(p->data[0] == '=')
    {
        num = atoi(p->data + 1);
        UART_SEND_AT_STRING("+SETCONNECT\r\n");
        if(num >= CENTRAL_MAX_CONNECTION)
        {
            UART_SEND_AT_STRING("UNSUPPORT NUMBER\r\n");
            goto error;
        }
        else
        {
            len = AscToHex(info.peerAddr, p->data + 3, 6, ',', &s);
            if(len != 6)
            {
                goto error;
            }
            info.password = AscToInt(s + 1, 6, &s);
            info.gsi_select_num = AscToInt(s + 1, 1, &s);
            info.enabled = AscToInt(s + 1, 1, &s);
            tmos_memcpy(&PeerAddrDef[num], &info, sizeof(peerAddrDefItem_t));
            UART_SEND_AT_STRING("OK\r\n");
            return;
        }
error:
        UART_SEND_AT_STRING("ERR\r\n");
    }
    else
    {
        UART_SEND_AT_STRING("\r\nERR:02\r\n");
    }
}

/*
 * AT+GETCONNECT命令，获取当前的连接设置
 */
AT_HASH_MATCH_EXPORT(getconnect)
{
    uint32_t num;
    char stp[16] = {0,0};
    if(p->data[0] == '=')
    {
        UART_SEND_AT_STRING("+GETCONNECT:\r\n");
        num = atoi(p->data + 1);
        if(num >= CENTRAL_MAX_CONNECTION)
        {
            UART_SEND_AT_STRING("UNSUPPORT NUMBER\r\n");
            UART_SEND_AT_STRING("ERR\r\n");
            return;
        }

        if(PeerAddrDef[num].enabled == 0)
        {
            UART_SEND_AT_STRING("NONE");
        }
        else
        {
            UART_SEND_AT_STRING("MAC:");
            at_uart_send_hex(PeerAddrDef[num].peerAddr, 6);
            UART_SEND_AT_STRING("\r\nPASSWORD:");
            snprintf_(stp, 15, "%d", PeerAddrDef[num].password);
            UART_SEND_AT_VSTRING(stp);
            UART_SEND_AT_STRING("\r\nSERVICE:");
            stp[0] = PeerAddrDef[num].gsi_select_num + 0x30;
            stp[1] = 0;
            UART_SEND_AT_VSTRING(stp);
        }
        UART_SEND_AT_STRING("\r\nOK\r\n");
    }
    else
    {
        UART_SEND_AT_STRING("\r\nERR:02\r\n");
    }
}

/*
 * AT+SETAUTOCONNECT命令，存储自动连接设置
 */
AT_HASH_MATCH_EXPORT(setautoconnect)
{
    uint8_t *s;
    uint32_t num, len;
    peerAddrDefItem_t info;
    char stp[16] = {0,0};
    if(p->data[0] == '=')
    {
        num = atoi(p->data + 1);
        UART_SEND_AT_STRING("+SETAUTOCONNECT\r\n");
        if(num >= CENTRAL_MAX_CONNECTION)
        {
            UART_SEND_AT_STRING("UNSUPPORT NUMBER\r\n");
            goto error;
        }
        else
        {
            len = AscToHex(info.peerAddr, p->data + 3, 6, ',', &s);
            if(len != 6)
            {
                goto error;
            }
            info.password = AscToInt(s + 1, 6, &s);
            info.gsi_select_num = AscToInt(s + 1, 1, &s);
            info.enabled = AscToInt(s + 1, 1, &s);
            if(auto_connect_addr_set(num, &info) != 0)
            {
                UART_SEND_AT_STRING("OK\r\n");
                return;
            }
        }
error:
        UART_SEND_AT_STRING("ERR\r\n");
    }
    else
    {
        UART_SEND_AT_STRING("\r\nERR:02\r\n");
    }
}

/*
 * AT+GETAUTOCONNECT命令，获取自动连接设置
 */
AT_HASH_MATCH_EXPORT(getautoconnect)
{
    uint32_t num;
    peerAddrDefItem_t info;
    char stp[16] = {0,0};
    if(p->data[0] == '=')
    {
        UART_SEND_AT_STRING("+GETAUTOCONNECT:\r\n");
        num = atoi(p->data + 1);
        if(num >= CENTRAL_MAX_CONNECTION)
        {
            UART_SEND_AT_STRING("UNSUPPORT NUMBER\r\n");
            UART_SEND_AT_STRING("ERR\r\n");
            return;
        }
        else
        {
            auto_connect_addr_get(num, &info);
        }
        if(info.enabled == 0)
        {
            UART_SEND_AT_STRING("NONE");
        }
        else
        {
            UART_SEND_AT_STRING("MAC:");
            at_uart_send_hex(info.peerAddr, 6);
            UART_SEND_AT_STRING("\r\nPASSWORD:");
            snprintf_(stp, 15, "%d", info.password);
            UART_SEND_AT_VSTRING(stp);
            UART_SEND_AT_STRING("\r\nSERVICE:");
            stp[0] = info.gsi_select_num + 0x30;
            stp[1] = 0;
            UART_SEND_AT_VSTRING(stp);
        }
        UART_SEND_AT_STRING("\r\nOK\r\n");
    }
    else
    {
        UART_SEND_AT_STRING("\r\nERR:02\r\n");
    }
}

/*
 * AT+DISCONNECT命令，断开连接
 */
AT_HASH_MATCH_EXPORT(disconnect)
{
    uint8_t rMacAddr[6];
    uint8_t *s;
    uint32_t num;
    uint8_t i;
    char stp[2];

    if(p->data[0] == '=')
    {
        UART_SEND_AT_STRING("+DISCONNECT\r\n");
        num = AscToHex(rMacAddr, p->data + 1, 6, ',', &s);
        if(num == 6)
        {
            for(i = 0; i < CENTRAL_MAX_CONNECTION; i++)
            {
                if(tmos_memcmp(centralConnList[i].peerAddr, rMacAddr, 6))
                {
                    break;
                }
            }
            if(i != CENTRAL_MAX_CONNECTION)
            {
                GAPRole_TerminateLink(centralConnList[i].connHandle);
            }
            else
            {
                goto error;
            }

            UART_SEND_AT_STRING("OK\r\n");
        }
        else
        {
error:
            UART_SEND_AT_STRING("ERR\r\n");
        }
    }
    else
    {
        UART_SEND_AT_STRING("\r\nERR:02\r\n");
    }
}

/*
 * AT+SETSERVICE命令，设置需要枚举的UUID模板
 */
AT_HASH_MATCH_EXPORT(setservice)
{
    uint8_t *s;
    uint32_t num;
    gatt_service_info_t info;
    if(p->data[0] == '=')
    {
        UART_SEND_AT_STRING("+SETSERVICE\r\n");
        info.service_uuid_len = AscToHex(info.service_uuid, p->data + 1, 16, ',', &s);
        if((info.service_uuid_len != ATT_UUID_SIZE) && (info.service_uuid_len != ATT_BT_UUID_SIZE))
        {
            PRINT("service_uuid_len:%d\n", info.service_uuid_len);
            goto error;
        }

        info.notify_uuid_len = AscToHex(info.notify_uuid, s + 1, 16, ',', &s);
        if((info.notify_uuid_len != ATT_UUID_SIZE) && (info.notify_uuid_len != ATT_BT_UUID_SIZE) && (info.notify_uuid_len != 0))
        {
            PRINT("notify_uuid_len:%d\n", info.notify_uuid_len);
            goto error;
        }

        info.write_uuid_len = AscToHex(info.write_uuid, s + 1, 16, ',', &s);
        if((info.write_uuid_len != ATT_UUID_SIZE) && (info.write_uuid_len != ATT_BT_UUID_SIZE))
        {
            PRINT("write_uuid_len:%d\n", info.write_uuid_len);
            goto error;
        }
        else if((info.write_uuid_len != 0))
        {
            if((info.write_uuid_len == info.notify_uuid_len) && (tmos_memcmp(info.write_uuid, info.notify_uuid, info.write_uuid_len)))
            {
                info.write_uuid_len = 0xff;
            }
        }
        info.custom_uuid_len = AscToHex(info.custom_uuid, s + 1, 16, ',', &s);
        if((info.custom_uuid_len != ATT_UUID_SIZE) && (info.custom_uuid_len != ATT_BT_UUID_SIZE) && (info.custom_uuid_len != 0))
        {
            PRINT("custom_uuid_len:%d\n", info.custom_uuid_len);
            goto error;
        }

        if(vendor_def_service_set(&info) != 0)
        {
            tmos_memcpy(&vendor_def_service, &info, sizeof(gatt_service_info_t));
            UART_SEND_AT_STRING("OK\r\n");
            return;
        }

error:
        UART_SEND_AT_STRING("ERR\r\n");
    }
    else
    {
        UART_SEND_AT_STRING("\r\nERR:02\r\n");
    }
}

/*
 * AT+GETSERVICE命令，获取存放的需要枚举的UUID模板
 */
AT_HASH_MATCH_EXPORT(getservice)
{
    uint32_t num;
    gatt_service_info_t *info;
    char stp[2];
    if(p->data[0] == '=')
    {
        num = atoi(p->data + 1);
        if(num > MASTER_SERVICE_CONST_NUM)
        {
            UART_SEND_AT_STRING("UNSUPPORT NUMBER\r\n");
            UART_SEND_AT_STRING("ERR\r\n");
            return;
        }
        else if(num == MASTER_SERVICE_CONST_NUM)
        {
            info = (gatt_service_info_t *)&vendor_def_service;
        }
        else
        {
            info = (gatt_service_info_t *)&default_gatt_service_info[num];
        }
        UART_SEND_AT_STRING("+GETSERVICE:\r\n");
        if(info->service_uuid_len > 0)
        {
            UART_SEND_AT_STRING("Service UUID:");
            at_uart_send_hex(info->service_uuid, info->service_uuid_len);
            UART_SEND_AT_STRING("\r\n");

            if((info->notify_uuid_len == ATT_UUID_SIZE) || (info->notify_uuid_len == ATT_BT_UUID_SIZE))
            {
                UART_SEND_AT_STRING("Notify UUID:");
                at_uart_send_hex(info->notify_uuid, info->notify_uuid_len);
                UART_SEND_AT_STRING("\r\n");
            }

            if((info->write_uuid_len == ATT_UUID_SIZE) || (info->write_uuid_len == ATT_BT_UUID_SIZE))
            {
                UART_SEND_AT_STRING("Write UUID:");
                at_uart_send_hex(info->write_uuid, info->write_uuid_len);
                UART_SEND_AT_STRING("\r\n");
            }

            if((info->custom_uuid_len == ATT_UUID_SIZE) || (info->custom_uuid_len == ATT_BT_UUID_SIZE))
            {
                UART_SEND_AT_STRING("Custom UUID:");
                at_uart_send_hex(info->custom_uuid, info->custom_uuid_len);
                UART_SEND_AT_STRING("\r\n");
            }
        }
        else
        {
            UART_SEND_AT_STRING("NO SERVICE\r\n");
        }
        UART_SEND_AT_STRING("OK\r\n");
    }
    else
    {
        UART_SEND_AT_STRING("\r\nERR:02\r\n");
    }
}

/*
 * AT+GETSERVICE命令，启用主机扫描
 */
AT_HASH_MATCH_EXPORT(setbaudrate)
{
    uint32_t baudrate;
    if(p->data[0] == '=')
    {
        UART_SEND_AT_STRING("+SETBAUDRATE");
        baudrate = atoi(p->data + 1);
        if((baudrate >= 300) && (baudrate <= 1500000))
        {
            UART_SEND_AT_STRING("\r\nOK\r\n");
            while(ringbuf_elements(&os_at_uart_tx_ringbuf) != 0);
            while(R8_UART0_TFC != 0);
            PFIC_DisableIRQ(UART0_IRQn);
            UART0_BaudRateCfg(baudrate);
            uart_timeouts_value = UART_TIMEOUT_VALUE_CALC(baudrate);
            PFIC_EnableIRQ(UART0_IRQn);
            UART_SEND_AT_STRING("+AT:READY\r\n");
            return;
        }
        else
        {
            UART_SEND_AT_STRING("\r\nERR\r\n");
        }
    }
    else
    {
        UART_SEND_AT_STRING("\r\nERR:02\r\n");
    }
}

/*
 * AT+GETMTU命令，启用主机扫描
 */
AT_HASH_MATCH_EXPORT(getmtu)
{
    uint8_t *s, i;
    uint8_t rMacAddr[6];
    char mtuAscii[16];
    uint32_t num;
    if(p->data[0] == '=')
    {
        UART_SEND_AT_STRING("+GETMTU:");
        num = AscToHex(rMacAddr, p->data + 1, 6, '\r', &s);
        if(num == 6)
        {
            for(i = 0; i < CENTRAL_MAX_CONNECTION; i++)
            {
                if(tmos_memcmp(centralConnList[i].peerAddr, rMacAddr, 6))
                {
                    break;
                }
            }
            if(i != CENTRAL_MAX_CONNECTION)
            {
                sprintf_(mtuAscii, "%d", centralConnList[i].mtu);
            }
            else
            {
                UART_SEND_AT_STRING("DEVICE NOT CONNECTED");
                goto error;
            }

            UART_SEND_AT_VSTRING(mtuAscii);
            UART_SEND_AT_STRING("\r\nOK\r\n");
        }
        else
        {
error:
            UART_SEND_AT_STRING("\r\nERR\r\n");
        }
    }
    else
    {
        UART_SEND_AT_STRING("\r\nERR:02\r\n");
    }
}
