/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description : adp frontend
 * Author        : sdk
 * Create        : 2019-11-01
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "hi_adp.h"
#include "hi_adp_frontend.h"
#include "hi_adp_boardcfg.h"
#include "hi_unf_frontend.h"
#include "hi_unf_i2c.h"

static hi_unf_frontend_sig_type g_sig_type[HI_FRONTEND_NUMBER];
static hi_u32  g_cfg_frontend_num = 1;  /* read from ini-file in function: hi_adp_frontend_init. */

/* ******************************** FRONTEND public function****************************************** */
static hi_s32 hi_adp_fronend_get_config_i2c_type(hi_u32 port, hi_unf_frontend_attr *fe_attr)
{
    hi_s32 ret;
    hi_u32 i2c_type;
    hi_u32 gpio_sda, gpio_scl;
    hi_u32 gpio_i2c_num = 0;
    ini_data_section ini_data = { FRONTEND_CONFIG_FILE, "" };

    if (port >= g_cfg_frontend_num) {
        SAMPLE_COMMON_PRINTF("port[%d] is out of config frontend number[%d].\n", port, g_cfg_frontend_num);
        return HI_FAILURE;
    }

    snprintf(ini_data.section, SECTION_MAX_LENGTH, "frontend%dinfo", port);

    ret = hi_adp_ini_get_u32(&ini_data, "I2cType", &i2c_type);
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    if (i2c_type == USE_I2C) {
        ret = hi_adp_ini_get_u16(&ini_data, "DemodI2cChNum", &(fe_attr->demod_i2c_channel));
        if (ret != HI_SUCCESS) {
            return HI_FAILURE;
        }
        ret = hi_adp_ini_get_u16(&ini_data, "TunerI2cChNum", &(fe_attr->tuner_i2c_channel));
        if (ret != HI_SUCCESS) {
            return HI_FAILURE;
        }
    } else {
        ret = hi_adp_ini_get_u32(&ini_data, "GpioI2cScl", &gpio_scl);
        if (ret != HI_SUCCESS) {
            return HI_FAILURE;
        }
        ret = hi_adp_ini_get_u32(&ini_data, "GpioI2cSda", &gpio_sda);
        if (ret != HI_SUCCESS) {
            return HI_FAILURE;
        }
        if (gpio_scl && gpio_sda) {
            /* 8 bit a group */
            ret = hi_unf_i2c_create_gpio_i2c(gpio_scl / 8, gpio_scl % 8, gpio_sda / 8, gpio_sda % 8, &gpio_i2c_num);
            if (ret != HI_SUCCESS) {
                SAMPLE_COMMON_PRINTF("hi_unf_i2c_create_gpio_i2c failed 0x%x\n", ret);
                return HI_FAILURE;
            }

            fe_attr->demod_i2c_channel = gpio_i2c_num;
        } else {
            SAMPLE_COMMON_PRINTF("Invalid data: GpioSCL[%d] or GpioSDA[%d]\n", gpio_scl, gpio_sda);
            return HI_FAILURE;
        }
    }
    return HI_SUCCESS;
}

static hi_s32 hi_adp_fronend_get_config_rst_type(hi_u32 port, hi_unf_frontend_attr *fe_attr)
{
    hi_s32 ret;
    hi_u32 value;
    hi_u16 gpio_num;
    ini_data_section ini_data = { FRONTEND_CONFIG_FILE, "" };
    snprintf(ini_data.section, SECTION_MAX_LENGTH, "frontend%dinfo", port);
    ret = hi_adp_ini_get_u32(&ini_data, "DemodRstType", &value);
    if (value == NEED_RESET) {
        ret = hi_adp_ini_get_u16(&ini_data, "DemodRstGpio", &gpio_num);
        if (ret != HI_SUCCESS) {
            return HI_FAILURE;
        }
        fe_attr->ext_dem_reset_gpio_group = gpio_num / 8; /* 8 bit per byte */
        fe_attr->ext_dem_reset_gpio_bit = gpio_num % 8;   /* 8 bit per byte */
    }
    return HI_SUCCESS;
}

static hi_s32 hi_adp_fronend_get_config(hi_u32 port, hi_unf_frontend_attr *fe_attr)
{
    hi_s32 ret;
    ini_data_section ini_data = { FRONTEND_CONFIG_FILE, "" };
    if (port >= g_cfg_frontend_num) {
        SAMPLE_COMMON_PRINTF("port[%d] is out of config frontend number[%d].\n", port, g_cfg_frontend_num);
        return HI_FAILURE;
    }

    snprintf(ini_data.section, SECTION_MAX_LENGTH, "frontend%dinfo", port);

    ret = hi_adp_ini_get_u32(&ini_data, "SigType", &(fe_attr->sig_type));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "TunerType", &(fe_attr->tuner_dev_type));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "TunerI2cAddr", &(fe_attr->tuner_addr));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "DemodType", &(fe_attr->demod_dev_type));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "DemodI2cAddr", &(fe_attr->demod_addr));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u16(&ini_data, "DemodClk", &(fe_attr->demod_clk));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    fe_attr->memory_mode = DEFAULT_MEMORY_MODE;
    ret = hi_adp_fronend_get_config_rst_type(port, fe_attr);
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_fronend_get_config_i2c_type(port, fe_attr);
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

hi_s32 hi_adp_fronend_get_sat_attr(hi_u32 port, hi_unf_frontend_sat_attr *fe_sat_attr)
{
    hi_s32 ret;
    ini_data_section ini_data = { FRONTEND_CONFIG_FILE, "" };

    if (port >= g_cfg_frontend_num) {
        SAMPLE_COMMON_PRINTF("port[%d] is out of config frontend number[%d].\n", port, g_cfg_frontend_num);
        return HI_FAILURE;
    }

    snprintf(ini_data.section, SECTION_MAX_LENGTH, "frontend%dinfo", port);

    ret = hi_adp_ini_get_u16(&ini_data, "SatTunerMaxLPF", &(fe_sat_attr->tuner_max_lpf));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "SatRfAgc", &(fe_sat_attr->rf_agc));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "SatIQSpectrum", &(fe_sat_attr->iq_spectrum));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "SatDiseqcWave", &(fe_sat_attr->diseqc_wave));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "SatLnbCtrlDev", &(fe_sat_attr->lnb_ctrl_dev));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u16(&ini_data, "SatLnbCtrlDevAddr", &(fe_sat_attr->lnb_dev_address));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u16(&ini_data, "LNBI2cChNum", &(fe_sat_attr->lnb_i2c_channel));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

static hi_s32 hi_adp_frontend_get_ts_pin(ini_data_section ini_data, hi_unf_demod_ts_out *ts_out)
{
    hi_s32 ret;
    hi_unf_demod_ts_pin *ts_pin = ts_out->ts_pin;
    ret = hi_adp_ini_get_u32(&ini_data, "DemodOutputTSDAT0", &(ts_pin[HI_UNF_DEMOD_TS_PIN_DAT0]));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "DemodOutputTSDAT1", &(ts_pin[HI_UNF_DEMOD_TS_PIN_DAT1]));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "DemodOutputTSDAT2", &(ts_pin[HI_UNF_DEMOD_TS_PIN_DAT2]));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "DemodOutputTSDAT3", &(ts_pin[HI_UNF_DEMOD_TS_PIN_DAT3]));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "DemodOutputTSDAT4", &(ts_pin[HI_UNF_DEMOD_TS_PIN_DAT4]));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "DemodOutputTSDAT5", &(ts_pin[HI_UNF_DEMOD_TS_PIN_DAT5]));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "DemodOutputTSDAT6", &(ts_pin[HI_UNF_DEMOD_TS_PIN_DAT6]));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "DemodOutputTSDAT7", &(ts_pin[HI_UNF_DEMOD_TS_PIN_DAT7]));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "DemodOutputTSSYNC", &(ts_pin[HI_UNF_DEMOD_TS_PIN_SYNC]));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "DemodOutputTSVLD", &(ts_pin[HI_UNF_DEMOD_TS_PIN_VLD]));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "DemodOutputTSERR", &(ts_pin[HI_UNF_DEMOD_TS_PIN_ERR]));
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

hi_s32 hi_adp_frontend_set_ts_out(hi_u32 port)
{
    hi_s32 ret;
    hi_u32 cfg_ts_out;
    hi_unf_demod_ts_out ts_out;
    ini_data_section ini_data = { FRONTEND_CONFIG_FILE, "" };

    if (port >= g_cfg_frontend_num) {
        SAMPLE_COMMON_PRINTF("port[%d] is out of config frontend number[%d].\n", port, g_cfg_frontend_num);
        return HI_FAILURE;
    }

    ret = hi_unf_frontend_get_ts_out(port, &ts_out);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("[%s] hi_unf_frontend_get_ts_out failed 0x%x\n", __FUNCTION__, ret);
        return HI_FAILURE;
    }

    snprintf(ini_data.section, SECTION_MAX_LENGTH, "frontend%dinfo", port);
    ret = hi_adp_ini_get_u32(&ini_data, "TsOutPutMode", &(ts_out.ts_mode));
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("[%s] get DemodTsOutMode failed 0x%x\n", __FUNCTION__, ret);
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "TsClkPolar", &(ts_out.ts_clk_polar));
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("[%s] get DemodTsOutMode failed 0x%x\n", __FUNCTION__, ret);
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "TsFormat", &(ts_out.ts_format));
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("[%s] get DemodTsOutMode failed 0x%x\n", __FUNCTION__, ret);
        return HI_FAILURE;
    }
    ret = hi_adp_ini_get_u32(&ini_data, "DemodTsOutMode", &cfg_ts_out);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("[%s] get DemodTsOutMode failed 0x%x\n", __FUNCTION__, ret);
        return HI_FAILURE;
    }
    if (cfg_ts_out == USER_DEFINED) {
        hi_adp_frontend_get_ts_pin(ini_data, &ts_out);
    }
    ret = hi_unf_frontend_set_ts_out(port, &ts_out);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("[%s] hi_unf_frontend_set_ts_out failed 0x%x\n", __FUNCTION__, ret);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

static hi_s32 hi_adp_frontend_set_attr(hi_u32 port)
{
    hi_s32                          ret;
    hi_unf_frontend_attr            fe_attr;
    /* get default attribute in order to set attribute next */
    ret = hi_unf_frontend_get_def_attr(port, &fe_attr);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("[%s] hi_unf_frontend_get_def_attr failed 0x%x\n", __FUNCTION__, ret);
        return ret;
    }

    ret = hi_adp_fronend_get_config(port, &fe_attr);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("[%s] hi_adp_fronend_get_config failed 0x%x\n", __FUNCTION__, ret);
        return ret;
    }

    printf("****************************************************************************\n");
    printf("port=%u, TunerType=%u, TunerAddr=0x%x, I2cChannel=%u, DemodAddr=0x%x, \n"
           "DemodType=%u,SigType:%u,ResetGpioNo:GPIO%u_%u\n",
           port,
           fe_attr.tuner_dev_type,
           fe_attr.tuner_addr,
           fe_attr.demod_i2c_channel,
           fe_attr.demod_addr,
           fe_attr.demod_dev_type,
           fe_attr.sig_type,
           fe_attr.ext_dem_reset_gpio_group, fe_attr.ext_dem_reset_gpio_bit);
    printf("****************************************************************************\n");

    ret = hi_unf_frontend_set_attr(port, &fe_attr);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("[%s] hi_unf_frontend_set_attr failed 0x%x\n", __FUNCTION__, ret);
        return ret;
    }

    g_sig_type[port] = fe_attr.sig_type;
    return ret;
}

static hi_s32 hi_adp_frontend_set_sat_attr(hi_u32 port)
{
    hi_s32  ret;
    hi_unf_frontend_lnb_config    lnb_config;
    hi_unf_frontend_sat_attr         fe_sat_attr;
    if (g_sig_type[port] >= HI_UNF_FRONTEND_SIG_TYPE_DVB_S &&
        g_sig_type[port] <= HI_UNF_FRONTEND_SIG_TYPE_ISDB_S3) {
        /* Default use dual freqency, C band, 5150/5750 */
        ret = hi_adp_fronend_get_sat_attr(port, &fe_sat_attr);
        if (ret != HI_SUCCESS) {
            SAMPLE_COMMON_PRINTF("[%s] hi_adp_fronend_get_sat_attr failed 0x%x\n", __FUNCTION__, ret);
            return ret;
        }

        ret = hi_unf_frontend_set_sat_attr(port, &fe_sat_attr);
        if (ret != HI_SUCCESS) {
            SAMPLE_COMMON_PRINTF("[%s] hi_unf_frontend_set_sat_attr failed 0x%x\n", __FUNCTION__, ret);
            return ret;
        }
        lnb_config.lnb_type = HI_UNF_FRONTEND_LNB_TYPE_DUAL_FREQUENCY;
        lnb_config.lnb_band = HI_UNF_FRONTEND_LNB_BAND_C;
        lnb_config.low_lo = 5150; /* 5150:default low local oscillator */
        lnb_config.high_lo = 5750; /* 5750:default high local oscillator */
        hi_unf_frontend_set_lnb_config(port, &lnb_config);
        if (ret != HI_SUCCESS) {
            SAMPLE_COMMON_PRINTF("[%s] hi_unf_frontend_set_lnb_config failed 0x%x\n", __FUNCTION__, ret);
            return ret;
        }

        hi_unf_frontend_set_lnb_power(port, HI_UNF_FRONTEND_LNB_POWER_ON);
        if (ret != HI_SUCCESS) {
            SAMPLE_COMMON_PRINTF("[%s] hi_unf_frontend_set_lnb_power failed 0x%x\n", __FUNCTION__, ret);
            return ret;
        }
    } else if (g_sig_type[port] == HI_UNF_FRONTEND_SIG_TYPE_ABSS) {
        lnb_config.lnb_type = HI_UNF_FRONTEND_LNB_TYPE_SINGLE_FREQUENCY;
        lnb_config.lnb_band = HI_UNF_FRONTEND_LNB_BAND_KU;
        lnb_config.low_lo = 10750; /* 10750:default local oscillator */
        lnb_config.high_lo = 10750; /* 10750:default local oscillator */
        ret = hi_unf_frontend_set_lnb_config(port, &lnb_config);
        if (ret != HI_SUCCESS) {
            SAMPLE_COMMON_PRINTF("[%s] hi_unf_frontend_set_lnb_config failed 0x%x\n", __FUNCTION__, ret);
            return ret;
        }
    }
    return HI_SUCCESS;
}

static hi_s32 hi_adp_frontend_init_config(hi_void)
{
    hi_s32  ret;
    ini_data_section ini_data = { FRONTEND_CONFIG_FILE, "frontendnum" };
    ret = hi_adp_ini_get_u32(&ini_data, "FrontendNum", &g_cfg_frontend_num);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("[%s] %s fail! %s %s.\n",
            __FUNCTION__, "hi_adp_ini_get_u32", FRONTEND_CONFIG_FILE, "frontendnum");
        return ret;
    }
    if (g_cfg_frontend_num > HI_FRONTEND_NUMBER) {
        SAMPLE_COMMON_PRINTF("[%s] g_cfg_frontend_num[%d] is out of max number[%d].\n",
            __FUNCTION__, g_cfg_frontend_num, HI_FRONTEND_NUMBER);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

hi_s32 hi_adp_frontend_init(hi_void)
{
    hi_s32  ret;
    hi_u32  i;

    ret = hi_adp_frontend_init_config();
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("[%s] hi_adp_frontend_init_config failed 0x%x\n", __FUNCTION__, ret);
        return ret;
    }

    ret = hi_unf_i2c_init();
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("[%s] hi_unf_i2c_init failed 0x%x\n", __FUNCTION__, ret);
        return ret;
    }

    ret = hi_unf_frontend_init();
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("[%s] hi_unf_frontend_init failed 0x%x\n", __FUNCTION__, ret);
        return ret;
    }

    for (i = 0; i < g_cfg_frontend_num; i++) {
        ret = hi_unf_frontend_open(i);
        if (ret != HI_SUCCESS) {
            SAMPLE_COMMON_PRINTF("[%s] hi_unf_frontend_open failed 0x%x\n", __FUNCTION__, ret);
            break;
        }

        ret = hi_adp_frontend_set_attr(i);
        if (ret != HI_SUCCESS) {
            break;
        }

        if (g_sig_type[i] >= HI_UNF_FRONTEND_SIG_TYPE_DVB_S && g_sig_type[i] <= HI_UNF_FRONTEND_SIG_TYPE_ABSS) {
            ret = hi_adp_frontend_set_sat_attr(i);
            if (ret != HI_SUCCESS) {
                break;
            }
        }

        ret = hi_adp_frontend_set_ts_out(i);
        if (ret != HI_SUCCESS) {
            break;
        }
    }

    if (ret != HI_SUCCESS) {
        for (i = 0; i < g_cfg_frontend_num; i++) {
            hi_unf_frontend_close(i);
        }
        hi_unf_frontend_deinit();
    }
    return ret;
}

static hi_unf_modulation_type hi_adp_frontend_convert_mod_type(hi_u32 mod_type)
{
    switch (mod_type) {
        case 16: /* 16:16QAM */
            return HI_UNF_MOD_TYPE_QAM_16;
        case 32: /* 32:32QAM */
            return HI_UNF_MOD_TYPE_QAM_32;
        case 64: /* 64:64QAM */
            return HI_UNF_MOD_TYPE_QAM_64;
        case 128: /* 128:128QAM */
            return HI_UNF_MOD_TYPE_QAM_128;
        case 256: /* 256:256QAM */
            return HI_UNF_MOD_TYPE_QAM_256;
        case 512: /* 512:512QAM */
            return HI_UNF_MOD_TYPE_QAM_512;
        default:
            return HI_UNF_MOD_TYPE_QAM_64;
    }
    return HI_UNF_MOD_TYPE_QAM_64;
}

/* Freq:MHZ  SymbolRate:KHZ  */
hi_s32 hi_adp_frontend_connect(hi_u32 port, hi_u32 freq, hi_u32 symbol_rate, hi_u32 third_param)
{
    hi_unf_frontend_connect_para  connect_para = {0};
    hi_u32 default_time_out = 0;
    hi_s32 ret;
    connect_para.sig_type = g_sig_type[port];
    /* DVB-S/S2 demod: AVL6211 / 3136 */
    if (g_sig_type[port] >= HI_UNF_FRONTEND_SIG_TYPE_DVB_S &&
        g_sig_type[port] <= HI_UNF_FRONTEND_SIG_TYPE_DVB_S2X) {
        if (third_param >= HI_UNF_FRONTEND_POLARIZATION_MAX) {
            third_param = HI_UNF_FRONTEND_POLARIZATION_H;
        }
        connect_para.connect_para.sat.polar = third_param;
        connect_para.connect_para.sat.freq = freq * 1000; /* 1000:MHz to KHz */
        connect_para.connect_para.sat.symbol_rate = symbol_rate * 1000; /* 1000:KHz to Hz */
        connect_para.connect_para.sat.scramble_value = 0;
    } else if (g_sig_type[port] == HI_UNF_FRONTEND_SIG_TYPE_ABSS) {
        if (third_param >= HI_UNF_FRONTEND_POLARIZATION_MAX) {
            third_param = HI_UNF_FRONTEND_POLARIZATION_L;
        }
        connect_para.connect_para.sat.polar = third_param;
        connect_para.connect_para.sat.freq = freq * 1000; /* 1000:MHz to KHz */
        connect_para.connect_para.sat.symbol_rate = symbol_rate * 1000; /* 1000:KHz to Hz */
        connect_para.connect_para.sat.scramble_value = 0;
    } else if ((g_sig_type[port] == HI_UNF_FRONTEND_SIG_TYPE_ISDB_S) ||
        (g_sig_type[port] == HI_UNF_FRONTEND_SIG_TYPE_ISDB_S3)) {
        connect_para.connect_para.sat.freq = freq * 1000; /* 1000:MHz to KHz */
        connect_para.connect_para.sat.stream_id = symbol_rate;
        connect_para.connect_para.sat.stream_id_type = third_param;
    } else if ((g_sig_type[port] >= HI_UNF_FRONTEND_SIG_TYPE_DVB_C) &&
        (g_sig_type[port] <= HI_UNF_FRONTEND_SIG_TYPE_J83B)) {
        /* DVB-C demod */
        connect_para.connect_para.cab.reverse = 0;
        connect_para.connect_para.cab.mod_type = hi_adp_frontend_convert_mod_type(third_param);
        connect_para.connect_para.cab.freq = freq * 1000; /* 1000:MHz to KHz */
        connect_para.connect_para.cab.symbol_rate = symbol_rate * 1000; /* 1000:KHz to Hz */
    } else if ((g_sig_type[port] >= HI_UNF_FRONTEND_SIG_TYPE_DVB_T) &&
        (g_sig_type[port] <= HI_UNF_FRONTEND_SIG_TYPE_DTMB)) {
        /* DVB_T/T2... demod */
        connect_para.connect_para.ter.reverse = 0;
        connect_para.connect_para.ter.freq = freq * 1000; /* 1000:MHz to KHz */
        connect_para.connect_para.ter.band_width = symbol_rate;
        connect_para.connect_para.ter.channel_mode = third_param;
        connect_para.connect_para.ter.dvbt_prio = third_param + 1;
    }

    ret = hi_unf_frontend_get_default_time_out(port, &connect_para, &default_time_out);
    if (ret !=  HI_SUCCESS) {
        return ret;
    }

    /* connect Tuner */
    return hi_unf_frontend_connect(port, &connect_para, default_time_out);
}

hi_s32 hi_adp_frontend_get_connect_para(hi_u32 port, hi_unf_frontend_connect_para *connect_para)
{
    if (port > g_cfg_frontend_num) {
        SAMPLE_COMMON_PRINTF("[%s] port invalid\n", __FUNCTION__);
        return HI_FAILURE;
    }
    connect_para->sig_type = g_sig_type[port];
    switch (g_sig_type[port]) {
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_C:
            connect_para->connect_para.cab.reverse = 0;
            connect_para->connect_para.cab.freq = 666000;  /* 666000:default frequency */
            connect_para->connect_para.cab.symbol_rate = 6875000;  /* 6875000:default symbol rate */
            connect_para->connect_para.cab.mod_type = HI_UNF_MOD_TYPE_QAM_64;
            break;
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_S:
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_S2:
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_S2X:
            connect_para->connect_para.sat.freq = 3840000;  /* 3840000:default frequency */
            connect_para->connect_para.sat.symbol_rate = 27500000; /* 27500000:default symbol rate */
            connect_para->connect_para.sat.polar = HI_UNF_FRONTEND_POLARIZATION_H;
            break;
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_T:
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_T2:
            connect_para->connect_para.ter.reverse = 0;
            connect_para->connect_para.ter.freq = 666000;  /* 666000:default frequency */
            connect_para->connect_para.ter.band_width = 8000; /* 8000:default band width */
            break;
        case HI_UNF_FRONTEND_SIG_TYPE_ISDB_T:
        case HI_UNF_FRONTEND_SIG_TYPE_ATSC_T:
        case HI_UNF_FRONTEND_SIG_TYPE_DTMB:
            connect_para->connect_para.ter.reverse = 0;
            connect_para->connect_para.ter.freq = 666000;  /* 666000:default frequency */
            connect_para->connect_para.ter.band_width = 6000; /* 6000:default band width */
            break;
        case HI_UNF_FRONTEND_SIG_TYPE_J83B:
            connect_para->connect_para.cab.reverse = 0;
            connect_para->connect_para.cab.freq = 666000;  /* 666000:default frequency */
            connect_para->connect_para.cab.symbol_rate = 5056941; /* 5056941:default symbol rate */
            connect_para->connect_para.cab.mod_type = HI_UNF_MOD_TYPE_QAM_64;
            break;
        case HI_UNF_FRONTEND_SIG_TYPE_ABSS:
            connect_para->connect_para.sat.freq = 1184000;  /* 1184000:default frequency */
            connect_para->connect_para.sat.symbol_rate = 28800000;  /* 28800000:default symbol rate */
            connect_para->connect_para.sat.polar = HI_UNF_FRONTEND_POLARIZATION_L;
            break;
        default:
            break;
    }
    return HI_SUCCESS;
}

hi_void hi_adp_frontend_deinit(hi_void)
{
    hi_u32 i;
    hi_s32 ret;
    hi_u32 max_i2c_num;
    hi_unf_frontend_attr fe_attr;

    ret = hi_unf_i2c_get_total_num(&max_i2c_num);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("[%s] hi_unf_i2c_get_total_num failed 0x%x\n", __FUNCTION__, ret);
        return;
    }

    for (i = 0; i < g_cfg_frontend_num; i++) {
        ret = hi_unf_frontend_get_attr(i, &fe_attr);
        if (ret != HI_SUCCESS) {
            SAMPLE_COMMON_PRINTF("[%s] hi_unf_frontend_get_attr failed 0x%x\n", __FUNCTION__, ret);
        } else {
            if (fe_attr.demod_i2c_channel >= max_i2c_num) {
                hi_unf_i2c_destroy_gpio_i2c(fe_attr.demod_i2c_channel);
            }
        }

        hi_unf_frontend_close(i);
    }

    hi_unf_i2c_deinit();

    hi_unf_frontend_deinit();
}

