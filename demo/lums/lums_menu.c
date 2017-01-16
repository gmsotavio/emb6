#include "emb6.h"
#include "bsp.h"
#include "evproc.h"
#include "clp.h"
#include "lums_menu.h"


/*
 * --- Local macros ---------------------------------------------------------*
 */
#define LUMS_STKCFG_MEM                     EN_HAL_INFOFLASH_SEG_A
#define LUMS_MENU_UART_RX_IRQ               EN_HAL_PERIPHIRQ_USERUART0_RX
/* configuration timeout in milliseconds */
#define LUMS_CFG_TIMEOUT                    ( 10000u )

/*
 * --- Local variables ------------------------------------------------------*
 */
static void *p_lumsConfMemHandle;
static s_lums_cmd_t s_lumsCmdObj;
static uint8_t lumsMenuCfgDone;

/*
 * --- Local functions ------------------------------------------------------*
 */

/*---------------------------------------------------------------------------*/
/*
* lums_menuWriteAddrs()
*/
static void lums_menuWriteStkCfg( s_mac_phy_conf_t *p_cfg )
{
    /* indicates the configuration is to be saved */
    p_cfg->is_saved = TRUE;
    /* write to flash */
    bsp_infoFlashWrite(p_lumsConfMemHandle, (uint8_t *)p_cfg, sizeof(*p_cfg));
}

/*---------------------------------------------------------------------------*/
/*
* lums_readAddrs()
*/
static void lums_menuReadStkCfg( s_mac_phy_conf_t *p_cfg )
{
    /* read configuration stored in the flash */
    bsp_infoFlashRead(p_lumsConfMemHandle, (uint8_t *)p_cfg, sizeof(*p_cfg));
}

/*---------------------------------------------------------------------------*/
/*
* lums_menuGetStkCfg()
*/
static void lums_menuGetStkCfg (int i_argNum, const char* p_args[])
{
    if (i_argNum && (strcmp(p_args[0], "help") == 0)) {
        clp_output("Usage: get");
    }
    else {
        s_mac_phy_conf_t *p_cfg = &mac_phy_config;
        uint8_t *p_mac = p_cfg->mac_address;

        clp_output("MAC:   %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x \r\n", p_mac[0], p_mac[1], p_mac[2], p_mac[3], p_mac[4], p_mac[5], p_mac[6], p_mac[7]);
        clp_output("PANID: %04x \r\n", p_cfg->pan_id);
        clp_output("SAVED: %d \r\n", p_cfg->is_saved);
    }
}

/*---------------------------------------------------------------------------*/
/*
* lums_menuSetStkCfg()
*/
static void lums_menuSaveStkCfg (int i_argNum, const char* p_args[])
{
    if (i_argNum && (strcmp(p_args[0], "help") == 0)) {
        clp_output("Usage: save");
    }
    else {
        lums_menuWriteStkCfg(&mac_phy_config);
        lums_menuGetStkCfg(0, NULL);
    }
}

/*---------------------------------------------------------------------------*/
/*
* lums_menuGetDevAddr()
*/
static void lums_menuGetDevAddr (int i_argNum, const char* p_args[])
{
    if (i_argNum && (strcmp(p_args[0], "help") == 0)) {
        clp_output("Usage: get");
    }
    else {
        uint8_t *addr = mac_phy_config.mac_address;
        clp_output("device address is %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
                addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
    }
}

/*---------------------------------------------------------------------------*/
/*
* lums_menuSetDevAddr()
*/
static void lums_menuSetDevAddr (int i_argNum, const char* p_args[])
{
    if (i_argNum == 0) {
        clp_output("Too less input parameters");
    }
    else if (strcmp(p_args[0], "help") == 0) {
        clp_output("Usage: set <addr> (e.g. 0012)");
    }
    else {
        uint32_t addrLsb;
        uint8_t *p_mac = mac_phy_config.mac_address;

        /* modify */
        clp_output("before: device address is %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X \r\n",
                p_mac[0], p_mac[1], p_mac[2], p_mac[3], p_mac[4], p_mac[5], p_mac[6], p_mac[7]);

        /* set the last 2 bytes of mac address */
        sscanf(*p_args, "%04X", &addrLsb);
        p_mac[7] = (uint8_t)(addrLsb);
        p_mac[6] = (uint8_t)(addrLsb >> 8);

        clp_output("after:  device address is %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X \r\n",
                p_mac[0], p_mac[1], p_mac[2], p_mac[3], p_mac[4], p_mac[5], p_mac[6], p_mac[7]);
    }
}

/*---------------------------------------------------------------------------*/
/*
* lums_menuGetTime()
*/
static void lums_menuGetTime (int i_argNum, const char* p_args[])
{
    if (i_argNum && (strcmp(p_args[0], "help") == 0)) {
        clp_output("Usage: get");
    }
    else {
        en_hal_rtc_t rtc;

        bsp_rtcGetTime(&rtc);
        clp_output("RTC: %02d:%02d:%02d %02d:%02d:%02d",
                rtc.uc_day, rtc.uc_mon, rtc.ui_year,
                rtc.uc_hour, rtc.uc_min, rtc.uc_sec);
    }
}

/*---------------------------------------------------------------------------*/
/*
* lums_menuSetTime()
*/
static void lums_menuSetTime (int i_argNum, const char* p_args[])
{
    if (i_argNum == 0) {
        clp_output("Too less input parameters");
    }
    else if (strcmp(p_args[0], "help") == 0) {
        clp_output("Usage: set <time> (e.g. 0012)");
    }
    else {
        /* TODO set current time */
    }
}

/*---------------------------------------------------------------------------*/
/*
* lums_cmd()
*/
static void lums_menuCmdParser (int i_argNum, const char* p_args[])
{
    if (i_argNum && (strcmp(p_args[0], "help") == 0)) {
        clp_output("Usage: get");
    }
    else {
        /* set command attributes properly */
        sscanf(p_args[0], "%s", s_lumsCmdObj.type);
        sscanf(p_args[1], "%d", &s_lumsCmdObj.param1);
        sscanf(p_args[2], "%d", &s_lumsCmdObj.param2);

        if (strncmp(s_lumsCmdObj.type, "exit", 4) == 0) {
            lumsMenuCfgDone = TRUE;
        }
        else {
            evproc_putEvent(E_EVPROC_HEAD, NETSTK_APP_EVENT_TX, &s_lumsCmdObj);
        }
    }
}


/*---------------------------------------------------------------------------*/
/*
* lums_menuRxChar()
*/
static void lums_menuRxChar(void *p_char)
{
    unsigned char c = *((unsigned char *)p_char);
    clp_rxChar(c);
}


/*
 * --- Global functions -----------------------------------------------------*
 */

/*---------------------------------------------------------------------------*/
/*
* lums_menuInit()
*/
int8_t lums_menuInit(void *p_ctx)
{
    void *p_uart;
    s_mac_phy_conf_t stkCfg;

    /* initialize info-flash */
    p_lumsConfMemHandle = bsp_infoFlashInit(LUMS_STKCFG_MEM);
    EMB6_ASSERT_RET(p_lumsConfMemHandle != NULL, -1);

    /* overwrite stack configuration with the one previously saved in flash */
    lums_menuReadStkCfg(&stkCfg);
    if (stkCfg.is_saved == TRUE) {
        memcpy(&mac_phy_config, &stkCfg, sizeof(stkCfg));
    }

    /* initialize UART */
    p_uart = bsp_uartInit(EN_HAL_UART_USER_0);
    EMB6_ASSERT_RET(p_uart != NULL, -1);

    /* register UART RX interrupt event for menu handling */
    bsp_periphIRQRegister(LUMS_MENU_UART_RX_IRQ, lums_menuRxChar, NULL);

    /*
     * command line program
     */
    clp_init(p_uart);
    clp_setTitle("\r\n"
            "**********************************************************************\r\n"
            "                     __   __  ____  _______\r\n"
            "                    / /  / / / /  |/  / __/\r\n"
            "                   / /__/ /_/ / /|_/ /\\ \\  \r\n"
            "                  /____/\\____/_/  /_/___/  \r\n"
            "                      / __/___             \r\n"
            "                      > _/_ _/             \r\n"
            "                     |_____/______         \r\n"
            "                    / // / __/ __ \\        \r\n"
            "                   / _  /\\ \\/ /_/ /        \r\n"
            "                  /_//_/___/\\____/         \r\n"
            "\r\n"
            "**********************************************************************\r\n"
            "\r\n");

    /*
     * Get default settings
     */
    {
        static s_clp_menuEntry_t s_stkCfg;
        static s_clp_menuEntry_t s_stkCfgGet;
        static s_clp_menuEntry_t s_stkCfgSave;

        clp_menuAddEntry(&s_stkCfg, NULL, "stkcfg", "", NULL);
        clp_menuAddEntry(&s_stkCfgGet, &s_stkCfg, "get", "Get stack configuration", lums_menuGetStkCfg);
        clp_menuAddEntry(&s_stkCfgSave, &s_stkCfg, "save", "Save stack configuration", lums_menuSaveStkCfg);
    }

    /*
     * Add menu entry for device address handling
     */
    {
        static s_clp_menuEntry_t s_macAddr;
        static s_clp_menuEntry_t s_macAddrGet;
        static s_clp_menuEntry_t s_macAddrSet;

        clp_menuAddEntry(&s_macAddr, NULL, "mac", "", NULL);
        clp_menuAddEntry(&s_macAddrGet, &s_macAddr, "get", "Get device MAC address", lums_menuGetDevAddr);
        clp_menuAddEntry(&s_macAddrSet, &s_macAddr, "set", "Set device MAC address", lums_menuSetDevAddr);
    }

    /*
     * Add menu entry for RTC handling
     */
    {
        static s_clp_menuEntry_t s_rtc;
        static s_clp_menuEntry_t s_rtcGet;
        static s_clp_menuEntry_t s_rtcSet;

        clp_menuAddEntry(&s_rtc, NULL, "rtc", "", NULL);
        clp_menuAddEntry(&s_rtcGet, &s_rtc, "get", "Get time", lums_menuGetTime);
        clp_menuAddEntry(&s_rtcSet, &s_rtc, "set", "Set time", lums_menuSetTime);
    }

    /*
     * Testing commands
     */
    {
        static s_clp_menuEntry_t s_cmd;

        clp_menuAddEntry(&s_cmd, NULL, "c",
                "s  [node_id]               request a node to sync time\r\n"
                "si [node_id] [interval]    request a node to update tx interval\r\n"
                "gi [node_id]               query a node's tx interval\r\n"
                "exit                       exit configuration session\r\n",
                lums_menuCmdParser);
    }

    clp_echoEnable(1);
    clp_outputTitle();
    clp_outputPrompt();
    return 0;
}

/*---------------------------------------------------------------------------*/
/*
* lums_menuRun()
*/
void lums_menuRun(void)
{
    lumsMenuCfgDone = FALSE;
    clock_time_t tickStart;

    /* wait until configuration timeout */
    tickStart = bsp_getTick();
    while (lumsMenuCfgDone == FALSE) {
        if ((bsp_getTick() - tickStart) > LUMS_CFG_TIMEOUT)
            break;
    }
    clp_output("configuration timeout...\r\n");
}
