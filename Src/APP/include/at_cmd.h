#ifndef _AT_CMD_H_
#define _AT_CMD_H_

#include "hash-match.h"
#include "uart_interface.h"

/* AT命令中不支持带空格 */
#define _AT_HASH_MATCH_EXPORT(A, B)         static void A##_handler(at_uart_data_t *p);                             \
                                            HASH_MATCH_EXPORT(.uart_at_cmd, A, B, (sizeof(B) - 1), A##_handler);     \
                                            static void A##_handler(at_uart_data_t *p)

#define AT_HASH_MATCH_EXPORT(X)             _AT_HASH_MATCH_EXPORT(at_##X, #X)


extern void at_uart_send_hex(uint8_t *src, uint8_t len);

#endif /* _AT_CMD_H_ */
