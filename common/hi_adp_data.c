#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "hi_type.h"
#include "hi_adp.h"
#include "hi_adp_data.h"

static db_frontend g_dvbfe_chan[MAX_FRONTEND_COUNT];
static db_program g_prog_list[MAX_PROG_COUNT];

static hi_s32 g_prog_total_count = 0;
static hi_s32 g_prog_cur_num = 0;
static hi_s32 g_dvbfe_chan_total_count = 0;

hi_s32  db_get_dvbprog_info_by_service_id(hi_u16 service_id, db_program *proginfo)
{
    hi_s32 i = 0;

    if (proginfo == NULL) {
        return HI_FAILURE;
    }

    for (i = 0; i < g_prog_total_count; i++) {
        if (g_prog_list[i].service_id == service_id) {
            break;
        }
    }
    if (i >= g_prog_total_count) {
        return HI_FAILURE;
    }

    memcpy((void *)proginfo, (void *)&g_prog_list[i], sizeof(db_program));

    return i;
}

hi_s32  db_get_dvbprog_info(hi_u32 prognum, db_program *proginfo)
{
    if ((prognum >= MAX_PROG_COUNT) || (proginfo == NULL)) {
        return HI_FAILURE;
    }

    memcpy((void *)proginfo, (void *)&g_prog_list[prognum], sizeof(db_program));

    return HI_SUCCESS;
}

hi_s32  db_set_dvbprog_info(hi_u32 prognum, const db_program *proginfo)
{
    if ((prognum >= MAX_PROG_COUNT) || (proginfo == NULL)) {
        return HI_FAILURE;
    }

    memcpy((void *)&g_prog_list[prognum], (void *)proginfo, sizeof(db_program));

    return HI_SUCCESS;
}

hi_s32  db_add_dvbprog(const db_program *proginfo)
{
    hi_s32 i = 0;

    if ((proginfo == NULL) || (g_prog_total_count >= (MAX_PROG_COUNT - 1))) {
        return HI_FAILURE;
    }

    for (i = 0; i < g_prog_total_count; i++) {
        if (g_prog_list[i].service_id == proginfo->service_id) {
            return i;
        }
    }

    memcpy((void *)&g_prog_list[g_prog_total_count], (void *)proginfo, sizeof(db_program));
    g_prog_total_count++;

    return g_prog_total_count;
}

hi_s32  db_get_prog_total_count(void)
{
    return g_prog_total_count;
}

hi_s32  db_get_fechan_info(hi_u32 channum, db_frontend *chaninfo)
{
    if ((channum >= MAX_FRONTEND_COUNT) || (chaninfo == NULL)) {
        return HI_FAILURE;
    }

    memcpy((void *)chaninfo, (void *)&g_dvbfe_chan[channum], sizeof(db_frontend));

    return HI_SUCCESS;
}

hi_s32  db_set_fechan_info(hi_u32 channum, const db_frontend *chaninfo)
{
    if ((channum >= MAX_FRONTEND_COUNT) || (chaninfo == NULL)) {
        return HI_FAILURE;
    }

    memcpy((void *)&g_dvbfe_chan[channum], (void *)chaninfo, sizeof(db_frontend));

    return HI_SUCCESS;
}

hi_s32  db_add_fechan(const db_frontend *chaninfo)
{
    hi_u32 i;

    if ((g_dvbfe_chan_total_count >= (MAX_FRONTEND_COUNT)) || (chaninfo == NULL)) {
        return HI_FAILURE;
    }

    if (chaninfo->fe_type == FE_TYPE_RF) {
        for (i = 0; i < g_dvbfe_chan_total_count; i++) {
            if ((g_dvbfe_chan[i].f_etype.fe_para_rf.frequency == chaninfo->f_etype.fe_para_rf.frequency)
                    && (g_dvbfe_chan[i].f_etype.fe_para_rf.modulation == chaninfo->f_etype.fe_para_rf.modulation)
                    && (g_dvbfe_chan[i].f_etype.fe_para_rf.symbol_rate ==
                        chaninfo->f_etype.fe_para_rf.symbol_rate)) {
                return i;
            }
        }
    }

    i = g_dvbfe_chan_total_count;
    memcpy((void *)&g_dvbfe_chan[i], (void *)chaninfo, sizeof(db_frontend));
    g_dvbfe_chan_total_count++;

    return i;
}

hi_s32  db_get_fechan_total_count(void)
{
    return g_dvbfe_chan_total_count;
}

hi_s32  db_reset(void)
{
    g_prog_cur_num = g_prog_total_count = 0;
    g_dvbfe_chan_total_count = 0;

    memset((void *)g_prog_list, 0, sizeof(g_prog_list[MAX_PROG_COUNT]));
    memset((void *)g_dvbfe_chan, 0, sizeof(g_dvbfe_chan[MAX_FRONTEND_COUNT]));

    return HI_SUCCESS;
}

hi_s32  db_save_to_file(FILE *filestream)
{
    hi_u32 prog_list_size, fe_chan_list_size;
    hi_s32 ret;

    prog_list_size   = g_prog_total_count * sizeof(db_program);
    fe_chan_list_size = g_dvbfe_chan_total_count * sizeof(db_frontend);

    ret = fwrite((void *)&prog_list_size, 1, sizeof(prog_list_size), filestream);

#ifdef DB_DEBUG
    printf("\n  ret DB_Save 0x%x prog %d", ret, prog_list_size);
#endif

    ret += fwrite((void *)&fe_chan_list_size, 1, sizeof(fe_chan_list_size), filestream);

#ifdef DB_DEBUG
    printf("\n  ret DB_Save 0x%x chan %d", ret, fe_chan_list_size);
#endif

    ret += fwrite((void *)&g_prog_list, 1, prog_list_size, filestream);
    ret += fwrite((void *)&g_dvbfe_chan, 1, fe_chan_list_size, filestream);

    SAMPLE_COMMON_PRINTF("\n db_save_to_file  0x%x \n", ret);
    return HI_SUCCESS;
}

hi_s32  db_restore_from_file(FILE *filestream)
{
    hi_u32 prog_list_size, fe_chan_list_size;
    hi_s32 ret;

    ret = fread((void *)&prog_list_size, 1, sizeof(prog_list_size), filestream);

#ifdef DB_DEBUG
    printf("\n  ret DB_RestoreFromFile0x%x prog %d", s32ret, u32ProgListSize);
#endif

    ret += fread((void *)&fe_chan_list_size, 1, sizeof(fe_chan_list_size), filestream);

#ifdef DB_DEBUG
    printf("\n  ret DB_RestoreFromFile0x%x chan %d", s32ret, u32FEChanListSize);
#endif

    ret += fread((void *)&g_prog_list, 1, prog_list_size, filestream);

    ret += fread((void *)&g_dvbfe_chan, 1, fe_chan_list_size, filestream);

    g_prog_total_count = prog_list_size / sizeof(db_program);
    g_dvbfe_chan_total_count = fe_chan_list_size / sizeof(db_frontend);

    SAMPLE_COMMON_PRINTF("\n  db_restore_from_file 0x%x \n", ret);

    return HI_SUCCESS;
}
