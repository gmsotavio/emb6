/**
 * @file    cc112x_frontend.h
 * @date    04.01.2016
 * @author  PN
 */

#ifndef __CC112X_FRONTEND_H__
#define __CC112X_FRONTEND_H__

/*============================================================================*/
/*                                  DEFINES                                   */
/*============================================================================*/

/*============================================================================*/
/*                                TYPEDEF ENUMS                               */
/*============================================================================*/
/**
 * @brief   Front-end operating mode enumeration declaration
 */
typedef enum {
    E_RF_FRONTEND_OPMODE_TX,
    E_RF_FRONTEND_OPMODE_TX_BYPASS,
    E_RF_FRONTEND_OPMODE_RX,
    E_RF_FRONTEND_OPMODE_RX_BYPASS,
    E_RF_FRONTEND_OPMODE_SHUTDOWN,
    E_RF_FRONTEND_OPMODE_MAX,
}e_frontendOpMode_t;


/*============================================================================*/
/*                                  FUNCTIONS                                 */
/*============================================================================*/
/**
 * @brief   Initialize HAL RF-Frontend Sky65366
 */
void cc112x_frontend_init(void);

/**
 * @brief   Select operating mode of the TX/RX Front-End module
 *
 * @param   e_op_mode   Operating mode is one of following values
 *                      @ref e_hal_frontendOpMode_t
 * @retval  1 if successful, otherwise 0
 */
uint8_t cc112x_frontend_opModeSel(e_frontendOpMode_t e_op_mode);

#endif /* __CC112X_FRONTEND_H__ */
