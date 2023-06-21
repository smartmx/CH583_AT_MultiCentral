#ifndef _UART_INTERFACE_H_
#define _UART_INTERFACE_H_

#include "tiny-macro-os.h"
#include "at_cmd.h"
#include "ringbuf.h"

#define AT_UART_CMD_MAX_LEN         16
#define AT_UART_DATA_MAX_LEN        512

extern struct ringbuf os_at_uart_tx_ringbuf;

extern struct ringbuf os_at_uart_rx_ringbuf;

typedef enum
{
    AT_UART_STATE_FREE = 0,
    AT_UART_STATE_HANDLING,
    AT_UART_STATE_ERR_AT,
    AT_UART_STATE_ERR_CMD,
    AT_UART_STATE_ERR_DATA,
    AT_UART_STATE_OK,
}AT_UART_STATE_t;

typedef __attribute__((aligned(4))) struct _at_uart_data_struct
{
    uint8_t cmd_len;
    char cmd[AT_UART_CMD_MAX_LEN + 1];   /* AT命令最大支持15个字符+1个'\0' */
    uint16_t data_len;
    uint8_t data[AT_UART_DATA_MAX_LEN];      /* AT后数据最长支持510字符 */
} at_uart_data_t;

extern OS_TASK(os_at_uart, void);

extern void uart_at_send_data(uint8_t *sdata, uint16_t len);

/* 发送固定长度数据 */
#define UART_SEND_AT_STRING(STRING)         uart_at_send_data(STRING, (sizeof(STRING) - 1))

/* 发送未知长度字符串 */
#define UART_SEND_AT_VSTRING(VSTRING)       uart_at_send_data(VSTRING, 0)

extern uint8_t uart_timeouts_value;

#define UART_TIMEOUT_VALUE_CALC(BAUD)       (((400000000 / BAUD) + 6249) / 6250 + 3)

#endif /* _UART_INTERFACE_H_ */
