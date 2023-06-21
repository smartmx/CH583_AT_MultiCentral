#include "sys_info.h"
#include "config.h"
#include "tinyflashdb.h"
#include "uart_interface.h"
#include "multiCentral.h"

/* 每个文件单独debug打印的开关，置0可以禁止本文件内部打印 */
#define DEBUG_PRINT_IN_THIS_FILE 1
#if DEBUG_PRINT_IN_THIS_FILE
#define PRINTF(...) PRINT(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

const tfdb_index_t ble_mac_index =
{
    .end_byte = 0x00,
    .flash_addr = 0,
    .flash_size = 256,
    .value_length = 6,
};
tfdb_addr_t ble_mac_index_cache = 0;   /* 地址缓存 */

/*********************************************************************
 * @fn      ble_mac_set
 *
 * @brief   设置的自定义枚举service
 *
 * @param   info     -   gatt_service_info_t
 *
 * @return  绑定成功与否.
 */
uint8_t ble_mac_set(uint8_t* addr)
{
    TFDB_Err_Code err;
    uint32_t rw_buf[((6 + 2 + 3) / 4)];

    err = tfdb_set(&ble_mac_index,(uint8_t *)rw_buf, &ble_mac_index_cache, addr);
    if( err == TFDB_NO_ERR )
    {
        PRINTF( "ble MacAddr set ok\n" );
        return 1;
    }

    PRINTF( "ble MacAddr set err\n" );
    return 0;
}

/*********************************************************************
 * @fn      ble_mac_init
 *
 * @brief   初始化自定义枚举service的gatt_service_info_t的参数
 *
 * @param   None.
 *
 * @return  None.
 */
void ble_mac_init(uint8_t* addrto)
{
    TFDB_Err_Code err;
    size_t len;
    uint8_t i,count = 0;
    uint32_t rw_buf[((6 + 2 + 3) / 4)];

    err = tfdb_get(&ble_mac_index,(uint8_t *)rw_buf, &ble_mac_index_cache, addrto);
    if( err == TFDB_NO_ERR )
    {
        if(tmos_isbufset(addrto, 0xff, 6))
        {
            goto error;
        }
        PRINTF( "ble MacAddr get success\n" );
    }
    else
    {
error:
        PRINTF( "ble MacAddr get failed\n" );
        tmos_memcpy(addrto, MacAddr, 6);
    }
}

const tfdb_index_t vendor_def_service_index =
{
    .end_byte = 0x00,
    .flash_addr = 256,
    .flash_size = 256,
    .value_length = sizeof(gatt_service_info_t),
};
tfdb_addr_t vendor_def_service_index_cache = 0;   /* 地址缓存 */

/*********************************************************************
 * @fn      vendor_def_service_set
 *
 * @brief   设置的自定义枚举service
 *
 * @param   info     -   gatt_service_info_t
 *
 * @return  绑定成功与否.
 */
uint8_t vendor_def_service_set(gatt_service_info_t* info)
{
    TFDB_Err_Code err;
    uint32_t rw_buf[((sizeof(gatt_service_info_t) + 2 + 3) / 4)];

    err = tfdb_set(&vendor_def_service_index,(uint8_t *)rw_buf, &vendor_def_service_index_cache, info);
    if( err == TFDB_NO_ERR )
    {
        PRINTF( "vendor_def_service set ok\n" );
        return 1;
    }

    PRINTF( "vendor_def_service set err\n" );
    return 0;
}

/*********************************************************************
 * @fn      vendor_def_service_init
 *
 * @brief   初始化自定义枚举service的gatt_service_info_t的参数
 *
 * @param   None.
 *
 * @return  None.
 */
void vendor_def_service_init(void)
{
    TFDB_Err_Code err;
    size_t len;
    uint8_t i,count = 0;
    uint32_t rw_buf[((sizeof(gatt_service_info_t) + 2 + 3) / 4)];

    err = tfdb_get(&vendor_def_service_index,(uint8_t *)rw_buf, &vendor_def_service_index_cache, &vendor_def_service);
    if( err == TFDB_NO_ERR )
    {
        PRINTF( "vendor def load success\n" );
    }
    else
    {
        PRINTF( "vendor def load failed\n" );
        tmos_memset(&vendor_def_service, 0, sizeof(gatt_service_info_t));
    }
}

const tfdb_index_t auto_connect_addr_index[CENTRAL_MAX_CONNECTION] =
{
#undef TFDB_CENTRAL_CONNECTION_ADDR_CNT
#define TFDB_CENTRAL_CONNECTION_ADDR_CNT    1
#if CENTRAL_MAX_CONNECTION >= TFDB_CENTRAL_CONNECTION_ADDR_CNT
    {
        .end_byte = 0x00,
        .flash_addr = 256 + (256 * TFDB_CENTRAL_CONNECTION_ADDR_CNT),
        .flash_size = 256,
        .value_length = sizeof(peerAddrDefItem_t),
    },
#endif

#undef TFDB_CENTRAL_CONNECTION_ADDR_CNT
#define TFDB_CENTRAL_CONNECTION_ADDR_CNT    2
#if CENTRAL_MAX_CONNECTION >= TFDB_CENTRAL_CONNECTION_ADDR_CNT
    {
        .end_byte = 0x00,
        .flash_addr = 256 + (256 * TFDB_CENTRAL_CONNECTION_ADDR_CNT),
        .flash_size = 256,
        .value_length = sizeof(peerAddrDefItem_t),
    },
#endif

#undef TFDB_CENTRAL_CONNECTION_ADDR_CNT
#define TFDB_CENTRAL_CONNECTION_ADDR_CNT    3
#if CENTRAL_MAX_CONNECTION >= TFDB_CENTRAL_CONNECTION_ADDR_CNT
    {
        .end_byte = 0x00,
        .flash_addr = 256 + (256 * TFDB_CENTRAL_CONNECTION_ADDR_CNT),
        .flash_size = 256,
        .value_length = sizeof(peerAddrDefItem_t),
    },
#endif

#undef TFDB_CENTRAL_CONNECTION_ADDR_CNT
#define TFDB_CENTRAL_CONNECTION_ADDR_CNT    4
#if CENTRAL_MAX_CONNECTION >= TFDB_CENTRAL_CONNECTION_ADDR_CNT
    {
        .end_byte = 0x00,
        .flash_addr = 256 + (256 * TFDB_CENTRAL_CONNECTION_ADDR_CNT),
        .flash_size = 256,
        .value_length = sizeof(peerAddrDefItem_t),
    },
#endif

#undef TFDB_CENTRAL_CONNECTION_ADDR_CNT
#define TFDB_CENTRAL_CONNECTION_ADDR_CNT    5
#if CENTRAL_MAX_CONNECTION >= TFDB_CENTRAL_CONNECTION_ADDR_CNT
    {
        .end_byte = 0x00,
        .flash_addr = 256 + (256 * TFDB_CENTRAL_CONNECTION_ADDR_CNT),
        .flash_size = 256,
        .value_length = sizeof(peerAddrDefItem_t),
    },
#endif

#undef TFDB_CENTRAL_CONNECTION_ADDR_CNT
#define TFDB_CENTRAL_CONNECTION_ADDR_CNT    6
#if CENTRAL_MAX_CONNECTION >= TFDB_CENTRAL_CONNECTION_ADDR_CNT
    {
        .end_byte = 0x00,
        .flash_addr = 256 + (256 * TFDB_CENTRAL_CONNECTION_ADDR_CNT),
        .flash_size = 256,
        .value_length = sizeof(peerAddrDefItem_t),
    },
#endif

#undef TFDB_CENTRAL_CONNECTION_ADDR_CNT
#define TFDB_CENTRAL_CONNECTION_ADDR_CNT    7
#if CENTRAL_MAX_CONNECTION >= TFDB_CENTRAL_CONNECTION_ADDR_CNT
    {
        .end_byte = 0x00,
        .flash_addr = 256 + (256 * TFDB_CENTRAL_CONNECTION_ADDR_CNT),
        .flash_size = 256,
        .value_length = sizeof(peerAddrDefItem_t),
    },
#endif

#undef TFDB_CENTRAL_CONNECTION_ADDR_CNT
#define TFDB_CENTRAL_CONNECTION_ADDR_CNT    8
#if CENTRAL_MAX_CONNECTION >= TFDB_CENTRAL_CONNECTION_ADDR_CNT
    {
        .end_byte = 0x00,
        .flash_addr = 256 + (256 * TFDB_CENTRAL_CONNECTION_ADDR_CNT),
        .flash_size = 256,
        .value_length = sizeof(peerAddrDefItem_t),
    },
#endif
};
tfdb_addr_t auto_connect_addr_index_cache[CENTRAL_MAX_CONNECTION] = {0};   /* 地址缓存 */

/*********************************************************************
 * @fn      auto_connect_addr_set
 *
 * @brief   设置的自定义枚举service
 *
 * @param   info     -   peerAddrDefItem_t
 *
 * @return  绑定成功与否.
 */
uint8_t auto_connect_addr_set(uint8_t num, peerAddrDefItem_t* info)
{
    TFDB_Err_Code err;
    uint32_t rw_buf[((sizeof(peerAddrDefItem_t) + 2 + 3) / 4)];

    err = tfdb_set(&auto_connect_addr_index[num],(uint8_t *)rw_buf, &auto_connect_addr_index_cache[num], info);
    if( err == TFDB_NO_ERR )
    {
        PRINTF( "auto connect addr set ok\n" );
        return 1;
    }

    PRINTF( "auto connect addr set err\n" );
    return 0;
}

/*********************************************************************
 * @fn      auto_connect_addr_init
 *
 * @brief   初始化自定义枚举service的peerAddrDefItem_t的参数
 *
 * @param   None.
 *
 * @return  None.
 */
TFDB_Err_Code auto_connect_addr_get(uint8_t num, peerAddrDefItem_t *to)
{
    TFDB_Err_Code err;
    size_t len;
    uint32_t rw_buf[((sizeof(peerAddrDefItem_t) + 2 + 3) / 4)];

    err = tfdb_get(&auto_connect_addr_index[num],(uint8_t *)rw_buf, &auto_connect_addr_index_cache[num], to);
    if( err == TFDB_NO_ERR )
    {
        PRINTF( "auto connect get success\n" );
    }
    else
    {
        PRINTF( "auto connect get failed\n" );
    }
    return err;
}

/*********************************************************************
 * @fn      auto_connect_addr_init
 *
 * @brief   初始化自定义枚举service的peerAddrDefItem_t的参数
 *
 * @param   None.
 *
 * @return  None.
 */
void auto_connect_addr_init(void)
{
    TFDB_Err_Code err;
    size_t len;
    uint8_t i,count = 0;
    uint32_t rw_buf[((sizeof(peerAddrDefItem_t) + 2 + 3) / 4)];

    for(uint8_t i = 0; i < CENTRAL_MAX_CONNECTION; i++)
    {
        err = tfdb_get(&auto_connect_addr_index[i],(uint8_t *)rw_buf, &auto_connect_addr_index_cache[i], &PeerAddrDef[i]);
        if( err == TFDB_NO_ERR )
        {
            PRINTF( "auto connect load success\n" );
        }
        else
        {
            PRINTF( "auto connect load failed\n" );
            tmos_memset(&PeerAddrDef[i], 0, sizeof(peerAddrDefItem_t));
        }
    }
}
