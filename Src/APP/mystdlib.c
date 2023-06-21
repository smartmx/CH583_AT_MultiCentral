#include "mystdlib.h"

/*********************************************************************
 * @fn      int_pow
 *
 * @brief   ����a^n ���ݣ��������ݼ���
 *
 * @param   a,n
 *
 * @return  uint32 - �������ֵ�����ж����
 */
uint32 int_pow(uint32 a, uint32 n)
{
    uint32 i;
    uint32 val;

    val = 1;
    for (i = 0; i < n; i++)
        val = val * a;

    return val;
}

/*********************************************************************
 * @fn      AscToHex
 *
 * @brief   ascת��hex Ĭ����ż������������0�������ִ�Сд��
 *          �Զ�������hexҪ���ַ���ֱ��Ϊ0��
 *
 * @param   p_des - hex��ŵ�ַ
 * @param   p_sour - �ַ�����ַ
 * @param   max_len - ���洢��hex����
 *
 * @return  uint16_t
 */
uint16_t AscToHex(uint8_t *p_des, uint8_t *p_sour, uint8_t max_len, uint8_t exit_char, uint8_t **p_sour_end)
{
    uint8_t *p1;
    uint8_t *p2;
    uint16_t i = 0; //����
    // uint8_t tmp = 0;
    uint8_t tog = 0;

    p1 = p_des;
    p2 = p_sour;
    tog = ~tog; //���Ǹ�
    while (1)
    {
        if ((*p2 == 0) || (*p2 == exit_char))
            break;
        if (((*p2) >= '0') && ((*p2) <= '9')) // tmp = (*p_sour)
        {
            if (tog)
            {
                *p1 = ((*p2++) - '0') * 16;
                tog = ~tog;
            }
            else
            {
                *p1++ += ((*p2++) - '0');
                i++;
                tog = ~tog;
            }
        }
        else if (((*p2) >= 'A') && ((*p2) <= 'F'))
        {
            if (tog)
            {
                *p1 = ((*p2++) - 'A' + 0x0a) * 16;
                tog = ~tog;
            }
            else
            {
                *p1++ += ((*p2++) - 'A' + 0x0a);
                i++;
                tog = ~tog;
            }
        }
        else if (((*p2) >= 'a') && ((*p2) <= 'f'))
        {
            if (tog)
            {
                *p1 = ((*p2++) - 'a' + 0x0a) * 16;
                tog = ~tog;
            }
            else
            {
                *p1++ += ((*p2++) - 'a' + 0x0a);
                i++;
                tog = ~tog;
            }
        }
        else
        {
            p2++;
        }
        if(i >= max_len)
        {
            break;
        }
    }

    *p_sour_end = p2;
    return i;
}

/*********************************************************************
 * @fn      HexToAsc
 *
 * @brief   hexת��asc������ÿ����ֽڱ任��һ���ַ�
 *
 * @param   p_des - �ַ������ָ��
 * @param   p_sour - hex���ָ��
 * @param   len - ��Ҫ�任��hex����
 *
 * @return  uint8 * - ��һ��p_desָ���ַ
 */
uint8 *HexToAsc(uint8 *p_des, uint8 *p_sour, uint16 len)
{
    uint16 num, i;
    uint8 *p1, *p2;

    p1 = p_des;
    p2 = p_sour;
    num = len;
    for (i = 0; i < num; i++)
    {
        *p1 = (*p2) / 16;
        if (*p1 < 10)
            *p1 += '0';
        else
            *p1 = *p1 - 10 + 'A';
        p1++;
        *p1 = (*p2) % 16;
        if (*p1 < 10)
            *p1 += '0';
        else
            *p1 = *p1 - 10 + 'A';
        p1++;
        p2++;
    }

    return p1;
}

/*********************************************************************
 * @fn      AscToInt
 *
 * @brief   �ַ���ת�������ݣ�ֱ���ַ����������������ַ�
 *
 * @param   p_dat - �ַ���ָ��
 *
 * @return  �任�����ֵ�����32λ��
 */
uint32 AscToInt(uint8 *p_dat, uint8_t max_len, uint8_t **p_sour_end)
{
    uint32 val = 0;
    uint8 i;
    uint8 *p_asc;

    p_asc = p_dat;
    i = 0;
    while (1) //���λ��
    {
        if ((*(p_asc + i) < '0') || (*(p_asc + i) > '9'))
            break;
        i++;
        if (i > max_len)
            break;
    }
    while (i)
    {
        val += (*p_asc++ - '0') * (int_pow(10, i-- - 1));
    }
    *p_sour_end = p_asc + i;
    return val;
}

/*********************************************************************
 * @fn      IntToAsc
 *
 * @brief   ������ת����ASC���浽ָ���ĵ�ַ�����ص���һ����ַmax:4294967295
 *
 * @param   p_dat - ���ת�����ָ��
 * @param   val - ת������ֵ
 *
 * @return  ��һ��ָ��λ��
 */
uint8 *IntToAsc(uint8 *p_dat, uint32 val)
{
    uint32 i;
    uint8 j, k;        //λ
    uint8 val_buf[11]; //�����10λ
    uint8 *p_mem;

    i = val;
    p_mem = p_dat;
    j = 9;    //�������λ
    while (j) //( ((i/int_pow(10,j--))%10) == 0 ); //��һ���������0
    {
        if (((i / int_pow(10, j)) % 10))
            break;
        j--;
        if (j == 0)
            break;
    }
    j++;
    tmos_memset(val_buf, 0, 11);
    for (k = 0; k < j; k++)
        val_buf[k] = (i / int_pow(10, (j - k - 1))) % 10 + '0';
    k = 0;
    while (1)
    {
        if (val_buf[k] == 0)
            break;
        *p_mem++ = val_buf[k++];
    }

    return p_mem;
}

/*********************************************************************
 * @fn      my_strcat
 *
 * @brief   �ַ���ƴ�ӣ�strcat((char *)p_data,(char *)"-");
 *
 * @param   p1 - ��ƴ���ַ���1���ַ���2����ӵ����ַ���1ĩβ
 * @param   p2 - ��ƴ���ַ���2
 *
 * @return  ƴ�Ӻ���ַ�����ĩβ��ַ
 */
char *my_strcat(char *p1, char *p2)
{
    char *p_dest;
    char *p_sour;

    p_dest = p1;
    p_sour = p2;

    while (*p_dest != 0)
        p_dest++;
    while (*p_sour != 0)
        *p_dest++ = *p_sour++;

    return p_dest;
}
