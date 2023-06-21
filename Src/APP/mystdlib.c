#include "mystdlib.h"

/*********************************************************************
 * @fn      int_pow
 *
 * @brief   计算a^n 求幂，整形数据计算
 *
 * @param   a,n
 *
 * @return  uint32 - 求出的数值，不判断溢出
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
 * @brief   asc转成hex 默认是偶数个，不够补0，不区分大小写，
 *          自动隔开非hex要的字符，直到为0。
 *
 * @param   p_des - hex存放地址
 * @param   p_sour - 字符串地址
 * @param   max_len - 最大存储的hex长度
 *
 * @return  uint16_t
 */
uint16_t AscToHex(uint8_t *p_des, uint8_t *p_sour, uint8_t max_len, uint8_t exit_char, uint8_t **p_sour_end)
{
    uint8_t *p1;
    uint8_t *p2;
    uint16_t i = 0; //计数
    // uint8_t tmp = 0;
    uint8_t tog = 0;

    p1 = p_des;
    p2 = p_sour;
    tog = ~tog; //先是高
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
 * @brief   hex转成asc，按照每半个字节变换成一个字符
 *
 * @param   p_des - 字符串存放指针
 * @param   p_sour - hex存放指针
 * @param   len - 需要变换的hex长度
 *
 * @return  uint8 * - 下一个p_des指针地址
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
 * @brief   字符串转换成数据，直到字符串结束或不是数字字符
 *
 * @param   p_dat - 字符串指针
 *
 * @return  变换后的数值（最大32位）
 */
uint32 AscToInt(uint8 *p_dat, uint8_t max_len, uint8_t **p_sour_end)
{
    uint32 val = 0;
    uint8 i;
    uint8 *p_asc;

    p_asc = p_dat;
    i = 0;
    while (1) //最大位数
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
 * @brief   将数据转换成ASC保存到指定的地址，返回的下一个地址max:4294967295
 *
 * @param   p_dat - 存放转换后的指针
 * @param   val - 转换的数值
 *
 * @return  下一个指针位置
 */
uint8 *IntToAsc(uint8 *p_dat, uint32 val)
{
    uint32 i;
    uint8 j, k;        //位
    uint8 val_buf[11]; //最大是10位
    uint8 *p_mem;

    i = val;
    p_mem = p_dat;
    j = 9;    //查找最大位
    while (j) //( ((i/int_pow(10,j--))%10) == 0 ); //万一这个数就是0
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
 * @brief   字符串拼接，strcat((char *)p_data,(char *)"-");
 *
 * @param   p1 - 待拼接字符串1，字符串2将添加到该字符串1末尾
 * @param   p2 - 待拼接字符串2
 *
 * @return  拼接后的字符串的末尾地址
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
