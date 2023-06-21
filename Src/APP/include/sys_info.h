#ifndef _SYS_INFO_H_
#define _SYS_INFO_H_

#include "CH58x_common.h"
#include "multiCentral.h"

extern uint8_t vendor_def_service_set(gatt_service_info_t* info);

extern void vendor_def_service_init(void);

extern uint8_t auto_connect_addr_set(uint8_t num, peerAddrDefItem_t* info);

extern void auto_connect_addr_init(void);

extern void ble_mac_init(uint8_t* addrto);

extern TFDB_Err_Code auto_connect_addr_get(uint8_t num, peerAddrDefItem_t *to);

extern uint8_t ble_mac_set(uint8_t* addr);

#endif /* _SYS_INFO_H_ */
