/*****************************************************************************
 *                                                                           *
 * INVECAS CONFIDENTIAL                                                      *
 * ____________________                                                      *
 *                                                                           *
 * [2018] - [2023] INVECAS, Incorporated                                     *
 * All Rights Reserved.                                                      *
 *                                                                           *
 * NOTICE:  All information contained herein is, and remains                 *
 * the property of INVECAS, Incorporated, its licensors and suppliers,       *
 * if any.  The intellectual and technical concepts contained                *
 * herein are proprietary to INVECAS, Incorporated, its licensors            *
 * and suppliers and may be covered by U.S. and Foreign Patents,             *
 * patents in process, and are protected by trade secret or copyright law.   *
 * Dissemination of this information or reproduction of this material        *
 * is strictly forbidden unless prior written permission is obtained         *
 * from INVECAS, Incorporated.                                               *
 *                                                                           *
 *****************************************************************************/
/**
 * @file inv478x_user_def.h
 *
 * @brief logging library
 *
 *****************************************************************************/

#ifndef INV478X_USER_DEF_H
#define INV478X_USER_DEF_H

/*****************************************************************************/
/** All definitions below can be modified by user.
 ******************************************************************************/

/*****************************************************************************/
/**
 * When defined as '1' host driver relies on the following platform functions to be provided
 * user application:
 *     invPlatformSemaphoreCreate()
 *     invPlatformSemaphoreDelete()
 *     invPlatformSemaphoreGive()
 *     invPlatformSemaphoreTake()
 * In Multi-Thread mode each API function is allowed to be called preemptively.
 * When defined zero the host driver will not call any of these platform functions.
 * In non multi-thread mode user must make sure that non of the host driver API functions
 * are called pre-emptively.
 *
 ******************************************************************************/
#define INV478X_USER_DEF__MULTI_THREAD     1

/*****************************************************************************/
/**
 * When defined as '1' user must call Inv478xHandle() from a independent thread to retrieve
 * IPC response message from Inv478x device and allow API function to finish IPC request.
 * When defined as '0' API function applies local polling for returning IPC response message.
 *
 * note: this definition is ignored if 'INV478X_USER_DEF__MULTI_THREAD' is defined as '0'.
 *
 ******************************************************************************/
#define INV478X_USER_DEF__IPC_SEMAPHORE    0

/*****************************************************************************/
/**
 * If defined with zero then host driver writes Flash data provided through Inv478xFlashUpdate()
 * directly to flash without cashing the data. The size indicated in Inv478xFlashUpdate() must be
 * 256.
 * For any value other than zero the host driver will allocate a memory buffer to store data before it is
 * programmed in flash. In this mode the user can dynamcally change size of flash packet to any value
 * between 0 and INV478X_USER_DEF__ISP_CASH_SIZE.
 *
 ******************************************************************************/
#define INV478X_USER_DEF__ISP_CASH_SIZE    256

/* These APIs are for internal use for API GUI tool and host app pc */

#endif /* INV478X_USER_DEF_H */

/***** end of file ***********************************************************/
