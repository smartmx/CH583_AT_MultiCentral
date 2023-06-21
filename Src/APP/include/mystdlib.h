#ifndef _MYSTDLIB_H_
#define _MYSTDLIB_H_

#include "config.h"

extern uint32 int_pow(uint32 a, uint32 n);

extern uint16_t AscToHex(uint8_t *p_des, uint8_t *p_sour, uint8_t max_len, uint8_t exit_char, uint8_t **p_sour_end);

extern uint8 *HexToAsc(uint8 *p_des, uint8 *p_sour, uint16 len);

extern uint32 AscToInt(uint8 *p_dat, uint8_t max_len, uint8_t **p_sour_end);

extern uint8 *IntToAsc(uint8 *p_dat, uint32 val);

extern char *my_strcat(char *p1, char *p2);

#endif /* _MYSTDLIB_H_ */
