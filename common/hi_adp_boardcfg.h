#ifndef __BOARDCFG_H__
#define __BOARDCFG_H__

#include "hi_unf_demux.h"
#include "hi_unf_frontend.h"
#include "hi_adp_ini.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifndef HI_FRONTEND_NUMBER
#define HI_FRONTEND_NUMBER         8
#endif

#ifdef ANDROID
#define FRONTEND_CONFIG_PATH  "/system/lib/"
#else
#define FRONTEND_CONFIG_PATH  "/usr/local/cfg/"
#endif
#define FRONTEND_CONFIG_FILE  FRONTEND_CONFIG_PATH "frontend_config.ini"

#define USE_I2C       0
#define NOT_MODIFY    0
#define NEED_RESET    1
#define USER_DEFINED  1
#define DEFAULT_MEMORY_MODE       0

/* TSO */
#define DEFAULT_DEMUX_TSO_NUM       (1)
/* TSO Attr */
#define DEFAULT_DEMUX_TSO_CLK       (HI_UNF_DMX_TSO_CLK_1200M)
#define DEFAULT_DEMUX_TSO_CLK_MODE  (HI_UNF_DMX_TSO_CLK_MODE_NORMAL)
#define DEFAULT_DEMUX_TSO_VLD_MODE  (HI_UNF_DMX_TSO_VALID_ACTIVE_HIGH)
#define DEFAULT_DEMUX_TSO_PORT_TYPE (HI_UNF_DMX_PORT_TYPE_SERIAL)
#define DEFAULT_DEMUX_TSO_BITSEL    (HI_UNF_DMX_TSO_SERIAL_BIT_0)


#define TUNER_USE        (0)


#ifdef __cplusplus
}
#endif
#endif

