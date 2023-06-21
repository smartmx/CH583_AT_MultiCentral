#include "uart_interface.h"

/* ÿ���ļ�����debug��ӡ�Ŀ��أ���0���Խ�ֹ���ļ��ڲ���ӡ */
#define DEBUG_PRINT_IN_THIS_FILE    1

#if DEBUG_PRINT_IN_THIS_FILE
    #define PRINTF(...)   PRINT(__VA_ARGS__)
#else
    #define PRINTF(...)   do{} while(0)
#endif

/* ���ڳ�ʱ���� */
uint8_t uart_timeouts_value = UART_TIMEOUT_VALUE_CALC(115200);

#define AT_TX_BUFFER_SIZE      512
struct ringbuf os_at_uart_tx_ringbuf;
static uint8_t os_at_uart_txbuffer[AT_TX_BUFFER_SIZE];

#define AT_RX_BUFFER_SIZE      512
struct ringbuf os_at_uart_rx_ringbuf;
static uint8_t os_at_uart_rxbuffer[AT_RX_BUFFER_SIZE];

/* ���ڽ������ݻ��� */
at_uart_data_t at_uart_data;

/* ���ڽ�������״̬�� */
os_fsm_t os_at_uart_fsm;

/*
 * at uart���ڽ���
 */
OS_FSM_FUNC(at_uart, uint8_t c)
{
    OS_FSM_START();

at_uart_start:
    OS_FSM_SET_STATE();
    /* AT+ */
    if((c == 'A') || (c == 'a'))
    {
        OS_FSM_YIELD();
    }
    else
    {
        OS_FSM_RETURN();
    }

    if((c == 'T') || (c == 't'))
    {
        OS_FSM_YIELD();
    }
    else
    {
        goto at_uart_start;
    }

    if(c == '+')
    {
        OS_FSM_YIELD();
    }
    else
    {
        if(c == '\r')
        {
            OS_FSM_YIELD();
            if(c == '\n')
            {
                /* ����ͨѶָ�ֱ�ӷ��ͻظ� */
                UART_SEND_AT_STRING("OK\r\n");
                OS_FSM_RESTART();
            }
        }
        goto at_uart_start;
    }

    /* ��λ��Ϣ */
    at_uart_data.cmd_len = 0;
    at_uart_data.data_len = 0;

    /* ׼���洢���� */
    while(1)
    {
        if((at_uart_data.cmd_len < AT_UART_CMD_MAX_LEN))
        {
            if(((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')))
            {
                at_uart_data.cmd[at_uart_data.cmd_len] = c;
                at_uart_data.cmd_len++;
                OS_FSM_YIELD();
            }
            else
            {
                /* �������Ѿ����� */
                break;
            }
        }
        else
        {
            goto at_uart_start;   /* ���� */
        }
    }

    /* ׼���洢���� */
    while(1)
    {
        if((at_uart_data.data_len < AT_UART_DATA_MAX_LEN))
        {
            at_uart_data.data[at_uart_data.data_len] = c;
            at_uart_data.data_len++;
            OS_FSM_YIELD();
        }
        else
        {
            goto at_uart_start;   /* ���� */
        }
    }

    OS_FSM_END();
}

/*
 * ����������
 */
OS_TASK(os_at_uart, void)
{
    OS_TASK_START(os_at_uart);

    static uint8_t os_at_uart_timeouts;

    ringbuf_init(&os_at_uart_tx_ringbuf, os_at_uart_txbuffer, AT_TX_BUFFER_SIZE);       /* ��ʼ��tx ringbuf */
    ringbuf_init(&os_at_uart_rx_ringbuf, os_at_uart_rxbuffer, AT_RX_BUFFER_SIZE);       /* ��ʼ��rx ringbuf */

    HASH_MATCH_INIT(uart_at_cmd);

    GPIOB_ModeCfg(bRXD0, GPIO_ModeIN_Floating);
    GPIOB_SetBits(bTXD0);
    GPIOB_ModeCfg(bTXD0, GPIO_ModeOut_PP_5mA);

    UART0_BaudRateCfg(115200);

    R8_UART0_FCR = (UART_4BYTE_TRIG << 6) | RB_FCR_TX_FIFO_CLR | RB_FCR_RX_FIFO_CLR | RB_FCR_FIFO_EN; // FIFO�򿪣�������4�ֽ�
    R8_UART0_LCR = RB_LCR_WORD_SZ;
    R8_UART0_IER = RB_IER_TXD_EN;
    R8_UART0_DIV = 1;

    UART0_INTCfg(ENABLE, RB_IER_RECV_RDY | RB_IER_THR_EMPTY | RB_IER_LINE_STAT);
    PFIC_EnableIRQ(UART0_IRQn);

    UART_SEND_AT_STRING("+AT:READY\r\n");

    while (1)
    {
        OS_TASK_WAIT_UNTILX((ringbuf_elements(&os_at_uart_rx_ringbuf) != 0), 1, uart_timeouts_value, os_at_uart_timeouts); /* 625us * 2�ղ����µĴ������ݣ�625 * 2 / 83.5 Լ15���ֽ�ʱ�䣬��Ϊ��ʱ���ѽ��� */
        if (os_at_uart_timeouts == 0)
        {
            /* ��ʼ����� */
            if(os_at_uart_fsm != 0)
            {
                if((at_uart_data.data_len >= 2) && (at_uart_data.data[at_uart_data.data_len - 1] == '\n') && (at_uart_data.data[at_uart_data.data_len - 2] == '\r'))
                {
                    at_uart_data.data[at_uart_data.data_len] = 0;   /* ��ֹ��������Խ�� */
                    HASH_MATCH_NO_FOUND_ACTION(uart_at_cmd, at_uart_data.cmd, at_uart_data.cmd_len, &at_uart_data, UART_SEND_AT_STRING("\r\nERR:01\r\n"));
                }
                else
                {
                    at_uart_data.data[at_uart_data.data_len] = 0;   /* ��ֹ��������Խ�� */
                    PRINT("%d,%d\n", at_uart_data.cmd_len, at_uart_data.data_len);
                    UART_SEND_AT_STRING("\r\nERR:02\r\n");
                }
                OS_FSM_RESET_ANOTHER(os_at_uart_fsm);
            }
        }
        else
        {
            /* ���ڼ����������� */
            int c;
            while (1)
            {
                c = ringbuf_get(&os_at_uart_rx_ringbuf);
                if(c != -1)
                {
                    OS_RUN_FSM(at_uart, os_at_uart_fsm, (uint8_t)c);
                }
                else
                {
                    break;
                }
            }
        }
    }

    OS_TASK_END(os_at_uart);
}

__HIGH_CODE
void uart_at_send_data(uint8_t *sdata, uint16_t len)
{
    int      c;
    uint16_t plen = 0;  //put length
    if(len != 0)
    {
        while (plen < len)  //put all buf in ringbuf
        {
            while(ringbuf_put(&os_at_uart_tx_ringbuf, sdata[plen]) == 0){};
            plen++;
        }
    }
    else
    {
        while(sdata[plen] != 0)
        {
            while(ringbuf_put(&os_at_uart_tx_ringbuf, sdata[plen]) == 0){};
            plen++;
        }
    }
    PFIC_DisableIRQ(UART0_IRQn);
    while (R8_UART0_TFC < UART_FIFO_SIZE)
    {
        c = ringbuf_get(&os_at_uart_tx_ringbuf); //�жϺ����ﶼʹ����ringbuf_get�Ͳ���THR�Ĵ�����������Ҫ���жϴ���
        if (c != -1)
        {
            R8_UART0_THR = (uint8_t)(c); /* ��ringbuf�е����ݷżĴ����У����Ա�����ܳ��ֵķ����˵�BUG*/
        }
        else
        {
            break; //ringbuf��û��������
        }
    }
    PFIC_EnableIRQ(UART0_IRQn);
}

__INTERRUPT
__HIGH_CODE
void UART0_IRQHandler(void)
{
    int     c;
    uint8_t i, v;
    switch (UART0_GetITFlag())
    {
    case UART_II_LINE_STAT:
        i = UART0_GetLinSTA();
        break;
    case UART_II_RECV_RDY:
    case UART_II_RECV_TOUT:
    {
        while (R8_UART0_RFC)
        {
            v = R8_UART0_RBR;
            ringbuf_put(&os_at_uart_rx_ringbuf, v);
        }
        break;
    }
    case UART_II_THR_EMPTY:
    {
        v = UART_FIFO_SIZE - R8_UART0_TFC;
        while (--v)
        {
            c = ringbuf_get(&os_at_uart_tx_ringbuf);
            if (c != -1)
            {
                R8_UART0_THR = (uint8_t)(c);
            }
            else
            {
                //û��������
                break;
            }
        }
        break;
    }
    case UART_II_MODEM_CHG:
        break;
    default:
        break;
    }
}
