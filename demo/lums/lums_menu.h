/*
 * lums_menu.h
 *
 *  Created on: 09.01.2017
 *      Author: verwalter
 */

#ifndef DEMO_LUMS_LUMS_MENU_H_
#define DEMO_LUMS_LUMS_MENU_H_


int8_t lums_menuInit(void *p_ctx);
void lums_menuRun(void);

/*
 * Interval functions
 */
typedef struct s_lums_cmd
{
    char type[3];
    int param1;
    int param2;
} s_lums_cmd_t;

#endif /* DEMO_LUMS_LUMS_MENU_H_ */
