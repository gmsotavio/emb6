/**
 * @file    cc112x_frontend.c
 * @date    04.01.2016
 * @author  PN
 */

#include "bsp.h"
#include "board_conf.h"

#if (NETSTK_SUPPORT_RF_FRONTEND == TRUE)
#include "cc112x_frontend.h"

/*============================================================================*/
/*                              LOCAL MACROS                                  */
/*============================================================================*/
#define CC112X_FRONTEND_TR_PIN                EN_HAL_PIN_RFCTRL3
#define CC112X_FRONTEND_EN_PIN                EN_HAL_PIN_RFCTRL4
#define CC112X_FRONTEND_BYP_PIN               EN_HAL_PIN_RFCTRL5


/*============================================================================*/
/*                              LOCAL DATA TYPES                              */
/*============================================================================*/
struct s_frontendSetting {
  uint8_t tr;
  uint8_t en;
  uint8_t byp;
};


/*============================================================================*/
/*                              LOCAL VARIABLES                               */
/*============================================================================*/
static void* cc112x_trHandle;
static void* cc112x_enHandle;
static void* cc112x_bypHandle;


/*============================================================================*/
/*                                LOCAL MACROS                                */
/*============================================================================*/


/*=============================================================================
 *  cc112x_sky65366_init()
 *============================================================================*/
void cc112x_frontend_init(void)
{
  cc112x_trHandle = bsp_pinInit(CC112X_FRONTEND_TR_PIN);
  cc112x_enHandle = bsp_pinInit(CC112X_FRONTEND_EN_PIN);
  cc112x_bypHandle = bsp_pinInit(CC112X_FRONTEND_BYP_PIN);
}


/*=============================================================================
 *  cc112x_frontend_opModeSel()
 *============================================================================*/
uint8_t cc112x_frontend_opModeSel(e_frontendOpMode_t opMode)
{
  /*
   *              Operating mode truth table
   * /------------------------------------------------------\
   * |    Operation    |    P6.6    |   P6.5    |   P6.4    |
   * |      Modes      |     TR     |    EN     |    BYP    |
   * |-----------------|------------|-----------|-----------|
   * | Transmit        |      1     |     1     |     0     |
   * | Transmit bypass |      1     |     1     |     1     |
   * | Receive         |      0     |     1     |     0     |
   * | Receive bypass  |      0     |     1     |     1     |
   * | Shutdown        |      X     |     0     |     X     |
   * \------------------------------------------------------/
   */
  struct s_frontendSetting lookupTable[E_RF_FRONTEND_OPMODE_MAX] =
  {
      {1, 1, 0},  /* TX */
      {1, 1, 1},  /* TX_BYPASS */
      {0, 1, 0},  /* RX */
      {0, 1, 1},  /* RX_BYPASS */
      {0, 0, 0},  /* SHUTDOWN */
  };

  bsp_pinSet(cc112x_trHandle, lookupTable[opMode].tr);
  bsp_pinSet(cc112x_enHandle, lookupTable[opMode].en);
  bsp_pinSet(cc112x_bypHandle, lookupTable[opMode].byp);
  bsp_delayUs(2);
  return 1;
}
#endif /* #if (NETSTK_SUPPORT_RF_FRONTEND == TRUE) */
