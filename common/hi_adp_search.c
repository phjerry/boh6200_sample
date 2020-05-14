#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "hi_type.h"
#include "hi_unf_avplay.h"
#include "hi_unf_video.h"
#include "hi_unf_audio.h"
#include "hi_unf_system.h"
#include "hi_unf_memory.h"
#include "hi_adp.h"

//#include "hi_unf_acodec_mp3dec.h"
//#include "hi_unf_acodec_mp2dec.h"
//#include "hi_unf_acodec_aacdec.h"
//#include "hi_codec_dolbyplusdec.h"
//#include "hi_unf_acodec_dtshddec.h"
//#include "HA.AUDIO.DRA.decode.h"
//#include "hi_unf_acodec_pcmdec.h"
//#include "HA.AUDIO.WMA9STD.decode.h"
//#include "hi_unf_acodec_amrnb.h"

#include "hi_adp_data.h"
#include "hi_adp_demux.h"
#include "hi_adp_search.h"
//#include "hi_adp_tuner.h"

#define PGPAT_TIMEOUT (5000)
#define PGPMT_TIMEOUT (2000)
#define PGSDT_TIMEOUT (10000)
#define HI_LANG_CODE_LEN (3)
#define HI_MAX_LANGUAGE_ELENUM  (32)

#define AUDIO_FORMAT_SWITCH(source, dest) \
do { \
    switch ((source)) { \
        case STREAM_TYPE_13818_7_AUDIO: \
            (dest) = FORMAT_AAC; \
            break; \
        case STREAM_TYPE_14496_3_AUDIO: \
            (dest) = FORMAT_AAC; \
            break; \
        case STREAM_TYPE_AC3_AUDIO: \
            (dest) = FORMAT_AC3; \
            break; \
        case STREAM_TYPE_DOLBY_TRUEHD_AUDIO: \
            (dest) = FORMAT_TRUEHD; \
            break; \
        case STREAM_TYPE_DTS_AUDIO: \
            (dest) = FORMAT_DTS; \
            break; \
        case STREAM_TYPE_13818_AUDIO: \
            (dest) = FORMAT_MP3; \
            break; \
        case STREAM_TYPE_11172_AUDIO: \
            (dest) = FORMAT_MP3; \
            break; \
        default: \
            (dest) = FORMAT_BUTT; \
            break; \
    } \
} while (0)

typedef struct tag_pmt_desc_iso639 {
    hi_u8 iso639_language_code[HI_LANG_CODE_LEN];
    hi_u8 audio_type;
} pmt_desc_iso639;

typedef struct tag_pmt_desc_iso639_language {
    hi_u32 language_ele_num;
    pmt_desc_iso639 ast_language_info[HI_MAX_LANGUAGE_ELENUM];
} pmt_desc_iso639_language;

static hi_u8 g_lang_ch_table[16][16] = {
    /*      0    1    2    3    4    5    6    7     8    9    A    B    C    D    E    F */
    /*0*/ {' ', ' ', ' ',  '0', '@', 'P',  '`', 'p', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    /*1*/ {' ', ' ', '!',  '1', 'A', 'Q',  'a', 'q', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    /*2*/ {' ', ' ', '"',  '2', 'B', 'R',  'b', 'r', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    /*3*/ {' ', ' ', '#',  '3', 'C', 'S',  'c', 's', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    /*4*/ {' ', ' ', '$',  '4', 'D', 'T',  'd', 't', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    /*5*/ {' ', ' ', '%',  '5', 'E', 'U',  'e', 'u', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    /*6*/ {' ', ' ', '&',  '6', 'F', 'V',  'f', 'v', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    /*7*/ {' ', ' ', '\'', '7', 'G', 'W',  'g', 'w', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    /*8*/ {' ', ' ', '(',  '8', 'H', 'X',  'h', 'x', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    /*9*/ {' ', ' ', ')',  '9', 'I', 'Y',  'i', 'y', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    /*A*/ {' ', ' ', '*',  ':', 'J', 'Z',  'j', 'z', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    /*B*/ {' ', ' ', '+',  ';', 'K', '[',  'k', '{', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    /*C*/ {' ', ' ', ',',  '<', 'L', '\\', 'l', '|', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    /*D*/ {' ', ' ', '-',  '=', 'M', ']',  'm', '}', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    /*E*/ {' ', ' ', '.',  '>', 'N', '^',  'n', '~', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    /*F*/ {' ', ' ', '/',  '?', 'O', '_',  'o', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}
};

static hi_void parse_sdt_desc(const hi_u8 *des_data, hi_s32 desc_len, sdt_info *pstru_prog)
{
    hi_u32 name_length = 0;
    hi_u8 tag = 0;
    hi_u8 length;

    const hi_u8 *data = HI_NULL;

    if ((des_data == HI_NULL) || (pstru_prog == HI_NULL)) {
        return;
    }

    while (desc_len > 0) {
        tag = *des_data++;
        length = *des_data++;

        if (length == 0) {
            return;
        }

        data = des_data;
        des_data += length;

        desc_len -= (length + 2);

        switch (tag) {
            case SERVICE_DESCRIPTOR: {
                pstru_prog->service_type = *data++;

                name_length = *data++;
                data += name_length;

                name_length = *data++;

                if (name_length == 0) {
                    break;
                }

                /* Parse service name */
                if (name_length > MAX_PROGNAME_LENGTH - 1) {
                    name_length = (hi_u32)(MAX_PROGNAME_LENGTH - 1);
                }

                strncpy((hi_char *)pstru_prog->prog_name, (hi_char *)data, name_length);

                break;
            }
            case NVOD_REFERENCE_DESCRIPTOR: {
                while (length > 0) {
                    pstru_prog->service_type = 0x04;

                    length = length - 6;

                    if (length <= 0) {
                        break;
                    }

                    data += 6;
                }

                break;
            }
            case COUNTRY_AVAILABILITY_DESCRIPTOR:
            case LINKAGE_DESCRIPTOR:

            case MOSAIC_DESCRIPTOR:

#ifdef SRCH_DEBUG
                printf("\n +++ SDT MOSAIC_DESCRIPTOR +++\n");
#endif
                break;
            case CA_IDENTIFIER_DESCRIPTOR:
            case TELEPHONE_DESCRIPTOR:
            case MULTILINGUAL_SERVICE_NAME_DESCRIPTOR:
            case PRIVATE_DATA_SPECIFIER_DESCRIPTOR:
            case DATA_BROADCAST_DESCRIPTOR:
            case STUFFING_DESCRIPTOR:
            case BOUQUET_NAME_DESCRIPTOR:
                break;
            default:
                break;
        }
    }
}

static long long get_bits48 (hi_u8 *buff, hi_s32 byte_offset, hi_s32 start_bit, hi_s32 bit_len)
{
    hi_u8 *p_tmp = HI_NULL;
    unsigned long long v = 0;
    unsigned long long mask = 0;
    unsigned long long tmp = 0;

    if (bit_len > 48) {
        return 0x7EFEFEFEFEFEFEFELL;
    }
    if (buff == (unsigned char *) 0) {
        return 0x7EFEFEFEFEFEFEFELL;
    }
    if (bit_len == 0) {
        return 0;
    }

    p_tmp = &buff[byte_offset + (start_bit >> 3)];
    start_bit %= 8;
    tmp = (unsigned long long)(*(p_tmp + 6) + (*(p_tmp + 5) << 8) + (*(p_tmp + 4) << 16) +
                               ((unsigned long long) * (p_tmp + 3) << 24) + ((unsigned long long) * (p_tmp + 2) << 32) +
                               ((unsigned long long) * (p_tmp + 1) << 40) + ((unsigned long long) * (p_tmp) << 48));
    start_bit = 56 - start_bit - bit_len;
    tmp  = tmp >> start_bit;
    mask = (1ULL << bit_len) - 1;
    v = tmp & mask;
    return v;
}

static unsigned long get_bits (hi_u8 *buff, hi_s32 byte_offset, hi_s32 start_bit, hi_s32 bit_len)
{
    hi_u8 *tmp = HI_NULL;
    unsigned long v = 0;
    unsigned long mask = 0;
    unsigned long tmp_long = 0;
    hi_s32 bit_high = 0;

    if (bit_len > 32) {
        return (hi_u32) 0xFEFEFEFE;
    }

    if (buff == (unsigned char *) 0) {
        return (hi_u32) 0xFEFEFEFE;
    }

    if (bit_len == 0) {
        return 0;
    }

    tmp = &buff[byte_offset + (start_bit >> 3)];
    start_bit %= 8;
    switch ((bit_len - 1) >> 3) {
        case 3:
            return (unsigned long) get_bits48(tmp, 0, start_bit, bit_len);
        case 2:
            tmp_long = (unsigned long)(*(tmp + 3) + (*(tmp + 2) << 8) + (*(tmp + 1) << 16) + (*(tmp) << 24));
            bit_high = 32;
            break;
        case 1:
            tmp_long = (unsigned long)(*(tmp + 2) + (*(tmp + 1) << 8) + (*(tmp) << 16));
            bit_high = 24;
            break;
        case 0:
            tmp_long = (unsigned long)((*(tmp) << 8) + * (tmp + 1));
            bit_high = 16;
            break;
        case - 1:
            return 0L;
        default:
            return (unsigned long) 0xFEFEFEFE;
    }
    start_bit = bit_high - start_bit - bit_len;
    tmp_long = tmp_long >> start_bit;
    mask = (1ULL << bit_len) - 1;
    v = tmp_long & mask;
    return v;
}

static hi_void parse_audio_language_code_trans(const hi_u8 ISO639_language_code[3], hi_u8 str_buf_len,
                                                         hi_u8 *str_buf)
{
    hi_u32 i = 0;
    hi_u8 first_nibble = 0;
    hi_u8 second_nibble = 0;

    if ((str_buf == HI_NULL) || (3 > str_buf_len)) {
        return;
    }

    for (i = 0; i < 3; i++) {
        first_nibble = (ISO639_language_code[i] & 0xF0) >> 4;
        second_nibble = ISO639_language_code[i] & 0x0F;
        if ((0xf < first_nibble) || (0xF < second_nibble)) {
            return;
        }
        str_buf[i] = g_lang_ch_table[second_nibble][first_nibble];
    }

    return;
}

static hi_void parse_audio_lunguage(const hi_u8 *des_data, hi_u32 buf_len, pmt_desc_iso639_language *data)
{
    hi_u8 *buff = (hi_u8 *)des_data;
    hi_u8 length = 0;
    hi_u8 str_code[3] = {0};

    if ((buff == HI_NULL) || (data == HI_NULL)) {
        return;
    }

    if (buff[0] != LANGUAGE_DESCRIPTOR) {
        return;
    }

    if (buf_len != (buff[1] + 2)) {
        return;
    }

    length = buff[1];
    buff += 2;
    data->language_ele_num = 0;
    while (length >= 4) {
        data->ast_language_info[data->language_ele_num].iso639_language_code[0] = (hi_u8)get_bits(buff, 0, 0, 8);
        data->ast_language_info[data->language_ele_num].iso639_language_code[1] = (hi_u8)get_bits(buff, 0, 8, 8);
        data->ast_language_info[data->language_ele_num].iso639_language_code[2] = (hi_u8)get_bits(buff, 0, 16, 8);
        data->ast_language_info[data->language_ele_num].audio_type = (hi_u8)get_bits(buff, 0, 24, 8);
        memset(str_code, 0x00, 3 * sizeof(hi_u8));
        parse_audio_language_code_trans(
            data->ast_language_info[data->language_ele_num].iso639_language_code, HI_LANG_CODE_LEN, str_code);
        data->ast_language_info[data->language_ele_num].iso639_language_code[0] = str_code[0];
        data->ast_language_info[data->language_ele_num].iso639_language_code[1] = str_code[1];
        data->ast_language_info[data->language_ele_num].iso639_language_code[2] = str_code[2];
        data->language_ele_num++;
        buff += 4;
        length -= 4;
    }
    return;
}

static hi_void parse_pmt_desc(const hi_u8 *des_data, hi_s32 desc_length, pmt_tb *pstru_prog, hi_bool parse_audio)
{
    hi_u8 tag = 0;
    hi_u8 length = 0;
    hi_u8 cnt_index = 0;
    const hi_u8 *data = HI_NULL;
    pmt_desc_iso639_language audio_lang;
    if ((des_data == HI_NULL) || (pstru_prog == HI_NULL)) {
        return;
    }

    while (desc_length > 0) {
        data = des_data;
        tag = *des_data++;
        length = *des_data++;
        if (length == 0) {
            return;
        }

        switch (tag) {
            case LANGUAGE_DESCRIPTOR: {
                if (parse_audio == HI_FALSE) {
                    break;
                }
                memset(&audio_lang, 0x00, sizeof(audio_lang));
                parse_audio_lunguage(data, length + 2, &audio_lang);
                pstru_prog->audio_info[pstru_prog->audo_num].aud_lang[0] =
                    audio_lang.ast_language_info[0].iso639_language_code[0];
                pstru_prog->audio_info[pstru_prog->audo_num].aud_lang[1] =
                    audio_lang.ast_language_info[0].iso639_language_code[1];
                pstru_prog->audio_info[pstru_prog->audo_num].aud_lang[2] =
                    audio_lang.ast_language_info[0].iso639_language_code[2];
                if (audio_lang.ast_language_info[0].audio_type == 3) {
                    pstru_prog->audio_info[pstru_prog->audo_num].ad_type = 1;
                }
                break;
            }

            case CA_DESCRIPTOR: {
                pstru_prog->ca_system[pstru_prog->ca_num].ca_system_id = des_data[0] << 8 | des_data[1];
                pstru_prog->ca_system[pstru_prog->ca_num].cap_id = des_data[2] << 8 | des_data[3];
                pstru_prog->ca_num++;
                break;
            }

            case TELETEXT_DESCRIPTOR: {
                hi_u16 ii = 0;
                hi_u16 n_count = length / 5;
                hi_u16 ttx_index = pstru_prog->ttx_num;

                for (ii = 0; ii < n_count; ii++) {
                    cnt_index = pstru_prog->ttx_info[ttx_index].des_info_cnt;
                    if ((desc_length >= 5) && cnt_index < TTX_DES_MAX) {
                        /* 8859-1 language code */
                        pstru_prog->ttx_info[ttx_index].ttx_des[cnt_index].iso639_language_code =
                            (des_data[ii * 5 + 0] << 16) | (des_data[ii * 5 + 1] << 8) | (des_data[ii * 5 + 2]);
                        /* teletext_type */
                        pstru_prog->ttx_info[ttx_index].ttx_des[cnt_index].ttx_type = (des_data[ii * 5 + 3] >> 3) & 0x1f;
                        /* teletext_magazine_number */
                        pstru_prog->ttx_info[ttx_index].ttx_des[cnt_index].ttx_magazine_number = des_data[ii * 5 + 3] & 0x7;
                        /* teletext_page_number */
                        pstru_prog->ttx_info[ttx_index].ttx_des[cnt_index].ttx_page_number = des_data[ii * 5 + 4];
                        pstru_prog->ttx_info[ttx_index].des_info_cnt++;
                    }
                }
                break;
            }

            case SUBTITLING_DESCRIPTOR: {
                hi_u16 ii = 0;
                hi_u16 n_count = length / 8; /* 8 =  3 + 1 + 2 + 2 */
                hi_u16 subt_index = pstru_prog->subtitling_num;

                for (ii = 0; ii < n_count; ii++) {
                    cnt_index = pstru_prog->subtiting_info[subt_index].des_info_cnt;
#ifdef SRCH_DEBUG
                    printf("Subtitle tag desc_length:%u.............pstru_prog->subtitling_num = %d, cnt_index = %d\n",
                           desc_length, pstru_prog->subtitling_num, cnt_index);
#endif
                    if ((desc_length >= 8) && cnt_index < SUBTDES_INFO_MAX) {
                        /* 8859-1 language code */
                        pstru_prog->subtiting_info[subt_index].des_info[cnt_index].lang_code =
                            (des_data[ii * 8 + 0] << 16) | (des_data[ii * 8 + 1] << 8) | (des_data[ii * 8 + 2]);

                        /* page id */
                        pstru_prog->subtiting_info[subt_index].des_info[cnt_index].page_id =
                            (des_data[ii * 8 + 4] << 8) | (des_data[ii * 8 + 5]);

                        /* ancilary page id */
                        pstru_prog->subtiting_info[subt_index].des_info[cnt_index].ancillary_page_id =
                            (des_data[ii * 8 + 6] << 8) | (des_data[ii * 8 + 7]);

                        pstru_prog->subtiting_info[subt_index].des_info_cnt++;
                    }
#ifdef SRCH_DEBUG
                    SAMPLE_COMMON_PRINTF("[%d]lang code:%#x\n", pstru_prog->subtitling_num,
                                         pstru_prog->subtiting_info[pstru_prog->subtitling_num].des_info[cnt_index].lang_code);
                    SAMPLE_COMMON_PRINTF("[%d]page id :%u\n", pstru_prog->subtitling_num,
                                         pstru_prog->subtiting_info[pstru_prog->subtitling_num].des_info[cnt_index].page_id);
                    SAMPLE_COMMON_PRINTF("[%d]ancilary page id : %u\n", pstru_prog->subtitling_num,
                                         pstru_prog->subtiting_info[pstru_prog->subtitling_num].des_info[cnt_index].ancillary_page_id);
                    SAMPLE_COMMON_PRINTF("[%d]count is %u\n\n", pstru_prog->subtitling_num,
                                         pstru_prog->subtiting_info[pstru_prog->subtitling_num].des_info_cnt);
                    SAMPLE_COMMON_PRINTF("[%d]pid is 0x%x\n\n", pstru_prog->subtitling_num,
                                         pstru_prog->subtiting_info[pstru_prog->subtitling_num].subtitling_pid);
#endif
                }

                break;
            }

            case AC3_DESCRIPTOR:
            case AC3_PLUS_DESCRIPTOR:
                break;

            case COPYRIGHT_DESCRIPTOR:
            case MOSAIC_DESCRIPTOR:
                break;

            case STREAM_IDENTIFIER_DESCRIPTOR:
            case PRIVATE_DATA_SPECIFIER_DESCRIPTOR:
            case SERVICE_MOVE_DESCRIPTOR:
            case CA_SYSTEM_DESCRIPTOR:
            case DATA_BROADCAST_ID_DESCRIPTOR:
                break;
#if defined(STREAM_TYPE_CAPTURE_86)
            case CAPTION_SERVICE_DESCRIPTOR: {
                hi_u8 service_num = 0;
                hi_u8 index = 0;
                hi_u32 lang_code = 0;
                hi_u8 j = 0;

                service_num = des_data[0] & 0x1f;
                pstru_prog->closed_caption_num = service_num;
                j ++;
                while (j < length) {
                    /* Ref : 6.9.2 Caption Service Descriptor in ATSC a/65 */
                    for (index = 0; index < service_num; index++) {
                        lang_code = (des_data[j] << 16 | des_data[j + 1] << 8 | des_data[j + 2]);
                        j += 3;

                        pstru_prog->closed_caption[index].is_digital_cc = des_data[j] & 0x80;
                        if (pstru_prog->closed_caption[index].is_digital_cc) {
                            pstru_prog->closed_caption[index].lang_code = lang_code;
                            pstru_prog->closed_caption[index].service_number = des_data[j] & 0x3f;
                            j += 1;
                            pstru_prog->closed_caption[index].is_easy_reader = des_data[j] & 0x80;
                            pstru_prog->closed_caption[index].is_wide_aspect_ratio = des_data[j] & 0x40;
                            j += 2;
                        } else {
                            j += 3;
                        }
                    }
                }
                break;
            }
#endif
            case EXTENSION_DESCRIPTOR: {
                hi_u8 extension_tag;
                extension_tag = des_data[0];
                if (extension_tag == SUPPLEMENTARY_AUDIO_DESCRIPTOR) {
                    if ((des_data[1] >> 7) == 0x0) {
                        pstru_prog->audio_info[pstru_prog->audo_num].ad_type = 1;
                    }
                }
                if (extension_tag == AC4_DESCRIPTOR) {
                    pstru_prog->audio_info[pstru_prog->audo_num].audio_enc_type = HI_UNF_ACODEC_ID_MS12_AC4;
                }
            }
            default:
                break;
        }

        des_data    += length;
        desc_length -= (length + 2);
    }
}

hi_s32 srh_parse_pat(const hi_u8  *section_data, hi_s32 length, hi_u8 *section_struct)
{
    hi_u16 programe_number = 0;
    hi_u16 pid = 0x1FFF;
    hi_u16 ts_id = 0;
    pat_tb *pat_tab = (pat_tb *)section_struct;
    pat_info *pat_info = HI_NULL;

    if ((section_data == HI_NULL) || (section_struct == HI_NULL)) {
        return HI_FAILURE;
    }

#ifdef SRCH_DEBUG
    hi_s32 index;
    SAMPLE_COMMON_PRINTF("\n");
    for (index = 0; index < length; index++) {
    }

    SAMPLE_COMMON_PRINTF("\n");
#endif

    if (section_data[0] != PAT_TABLE_ID) {
        return HI_FAILURE;
    }

    length = (hi_s32)((section_data[1] << 8) | section_data[2]) & 0x0fff;

    ts_id = (hi_u16)((section_data[3] << 8) | section_data[4]);

    pat_tab->ts_id = ts_id;

    section_data += 8;
    length -= 9;

    while (length >= 4) {
        pat_info = &(pat_tab->pat_info[pat_tab->prog_num]);

        programe_number = (hi_u16)((section_data[0] << 8) | section_data[1]);

        pid = (hi_u16)(((section_data[2] & 0x1F) << 8) | section_data[3]);

        if (programe_number != 0x0000) {
            pat_info->service_id = programe_number;
            pat_info->pmt_pid = pid;
            pat_tab->prog_num++;
#ifdef SRCH_DEBUG
            SAMPLE_COMMON_PRINTF(" parser PAT get PmtPid %d(0x%x) ServiceID %d(0x%x)  index %d\n",
                                 pat_info->pmt_pid, pat_info->pmt_pid, pat_info->service_id, pat_info->service_id, index);
#endif
        }

        section_data += 4;
        length -= 4;
    }

    return (HI_SUCCESS);
}

static hi_bool is_subtitle_stream(const hi_u8 *data, hi_s32 length)
{
    hi_bool ret = HI_FALSE;

    int n_len = length;
    hi_u8 *data_tmp = (hi_u8 *)data;
    hi_u8 tag = 0;
    hi_u8 length_tmp = 0;

    while (n_len > 0) {
        tag = *data_tmp++;
        length_tmp = *data_tmp++;

        if (tag == SUBTITLING_DESCRIPTOR) {
            ret = HI_TRUE;
            break;
        }

        data_tmp += length_tmp;

        n_len -= (length_tmp + 2);
    }

    return ret;
}

static hi_bool is_aribcc_stream(const hi_u8 *data, hi_s32 length)
{
    hi_bool ret = HI_FALSE;
    int n_len = length;
    hi_u8 *data_tmp = (hi_u8 *)data;
    hi_u8 tag = 0;
    hi_u8 length_tmp = 0;
    hi_u8 component_tag = 0;

    while (n_len > 0) {
        tag = *data_tmp++;
        length_tmp = *data_tmp++;
        /* Ref ARIB-TR-B14v2_8-vol3-p3-2-E2.pdf  P3-107 */
        if (tag == STREAM_IDENTIFIER_DESCRIPTOR) {
            component_tag = *data_tmp;

            if ((component_tag == 0x30) || (component_tag == 0x87)) {
                ret = HI_TRUE;
            }
            break;
        }

        data_tmp += length_tmp;

        n_len -= (length_tmp + 2);
    }

    return ret;
}

hi_bool is_ttx_stream(const hi_u8 *data, hi_s32 length)
{
    hi_bool ret = HI_FALSE;
    int n_len = length;
    hi_u8 *data_tmp = (hi_u8 *)data;
    hi_u8 tag = 0;
    hi_u8 length_tmp = 0;

    while (n_len > 0) {
        tag = *data_tmp++;
        length_tmp = *data_tmp++;

        if (tag == TELETEXT_DESCRIPTOR) {
            ret = HI_TRUE;
            break;
        }

        data_tmp += length_tmp;

        n_len -= (length_tmp + 2);
    }

    return ret;
}

static hi_bool is_dts_stream(const hi_u8 *buf, hi_u32 len)
{
    while (len >= 2) {
        hi_u32 tag      = buf[0];
        hi_u32 tag_len  = buf[1];

        buf += 2;
        len -= 2;

        switch (tag) {
            case REGISTRATION_DESCRIPTOR: {
                if ((htonl(*((hi_u32*)buf)) == STREAM_TYPE_DTS1_AUDIO_IDENTIFY) ||
                    (htonl(*((hi_u32*)buf)) == STREAM_TYPE_DTS2_AUDIO_IDENTIFY) ||
                    (htonl(*((hi_u32*)buf)) == STREAM_TYPE_DTS3_AUDIO_IDENTIFY)) {
                    return HI_TRUE;
                }
                break;
            }

            default: {
                buf += tag_len;
                if (tag_len >= len) {
                    len = 0;
                } else {
                    len -= tag_len;
                }
            }
        }
    }

    return HI_FALSE;
}

static hi_bool is_dra_stream(const hi_u8 *buf, hi_u32 len)
{
    while (len >= 2) {
        hi_u32 tag      = buf[0];
        hi_u32 tag_len  = buf[1];

        buf += 2;
        len -= 2;

        switch (tag) {
            case DRA_DESCRIPTOR:
                return HI_TRUE;

            default:
                buf += tag_len;
                if (tag_len >= len) {
                    len = 0;
                } else {
                    len -= tag_len;
                }
        }
    }

    return HI_FALSE;
}

static hi_bool is_ac3_stream(const hi_u8 *buf, hi_u32 len)
{
    while (len >= 2) {
        hi_u32 tag      = buf[0];
        hi_u32 tag_len  = buf[1];

        buf += 2;
        len -= 2;

        switch (tag) {
            case AC3_DESCRIPTOR:
            case AC3_PLUS_DESCRIPTOR:
            case AC3_EXT_DESCRIPTOR:
                return HI_TRUE;

            default:
                buf += tag_len;
                if (tag_len >= len) {
                    len = 0;
                } else {
                    len -= tag_len;
                }
        }
    }

    return HI_FALSE;
}

static hi_bool is_265_stream(const hi_u8 *data, hi_s32 length)
{
    hi_bool ret = HI_FALSE;

    int n_len = length;
    hi_u8 *data_tmp = (hi_u8 *)data;
    hi_u8 tag = 0;
    hi_u8 length_tmp = 0;

    while (n_len > 0) {
        tag = *data_tmp++;
        length_tmp = *data_tmp++;

        if (tag == REGISTRATION_DESCRIPTOR) {
            hi_u32 identify = htonl(*((hi_u32 *)data_tmp));

            if (identify == STREAM_TYPE_HEVC_VIDEO_IDENTIFY) {
                ret = HI_TRUE;
                break;
            }
        }

        data_tmp += length_tmp;

        n_len -= (length_tmp + 2);
    }

    return ret;
}

hi_s32 srh_parse_pmt (const hi_u8 *section_data, hi_s32 length, hi_u8 *section_struct)
{
    hi_u8 stream_type = 0;
    hi_u16 des_len = 0;
    hi_u16 pid  = 0x1FFF;
    pmt_tb *pmt_tab = (pmt_tb *)section_struct;

    if ((section_data == HI_NULL) || (section_struct == HI_NULL)) {
        return HI_FAILURE;
    }

    if (section_data[0] != PMT_TABLE_ID) {
        return HI_FAILURE;
    }

    memcpy(pmt_tab->pmt_data, section_data, length);
    pmt_tab->pmt_len = length;
#ifdef SRCH_DEBUG
    printf("enter length : %d\n", length);
#endif

    length = (hi_s32)((section_data[1] << 8) | section_data[2]) & 0x0fff;

#ifdef SRCH_DEBUG
    printf("length = %d, %#x:%#x\n", length, section_data[1], section_data[2]);
#endif

    pmt_tab->service_id = (hi_u16)((section_data[3] << 8) | section_data[4]);

    section_data += 8; /* skip 8-byte to PCR_PID */

    pid = (hi_u16)(((section_data[0] & 0x1F) << 8) | section_data[1]);
    des_len = (hi_u16)(((section_data[2] & 0x0F) << 8) | section_data[3]);

    pmt_tab->pcr_pid = pid;

    section_data += 4;

    length -= 9;

#ifdef SRCH_DEBUG
    printf("pcr pid = %#x, deslength:%d, s32Length = %d\n", pid, des_len, length);
#endif

    if (des_len > 0) {
        parse_pmt_desc(section_data, des_len, pmt_tab, HI_FALSE);
        section_data += des_len;
        length -= des_len;
    }

    while (length > 4) {
        stream_type = section_data[0];

        pid = (((section_data[1] & 0x1F) << 8) | section_data[2]);
        des_len = (((section_data[3] & 0x0F) << 8) | section_data[4]);

        section_data += 5;
        length -= 5;

#ifdef SRCH_DEBUG
        printf("stream type : %#x, pid:%#x, length:%d, u16DesLen = %d\n", stream_type, pid, length, des_len);
#endif

        switch (stream_type) {
            /* video stream */
            case STREAM_TYPE_14496_10_VIDEO:
            case STREAM_TYPE_14496_2_VIDEO:
            case STREAM_TYPE_11172_VIDEO:
            case STREAM_TYPE_13818_VIDEO:
            case STREAM_TYPE_AVS_VIDEO:
            case STREAM_TYPE_AVS2_VIDEO:
            case STREAM_TYPE_AVS3_VIDEO:
            case STREAM_TYPE_HEVC_VIDEO: {
#ifdef SRCH_DEBUG
                printf("video stream type is %#x, pid=%#x\n", stream_type, pid);
#endif
                if (pmt_tab->video_num < PROG_MAX_VIDEO) {
                    pmt_tab->video_info[pmt_tab->video_num].video_pid = pid;

                    if (stream_type == STREAM_TYPE_14496_10_VIDEO) {
                        pmt_tab->video_info[pmt_tab->video_num].video_enc_type = HI_UNF_VCODEC_TYPE_H264;
                    } else if (stream_type == STREAM_TYPE_14496_2_VIDEO) {
                        pmt_tab->video_info[pmt_tab->video_num].video_enc_type = HI_UNF_VCODEC_TYPE_MPEG4;
                    } else if (stream_type == STREAM_TYPE_AVS_VIDEO) {
                        pmt_tab->video_info[pmt_tab->video_num].video_enc_type = HI_UNF_VCODEC_TYPE_AVS;
                    } else if (stream_type == STREAM_TYPE_AVS2_VIDEO) {
                        pmt_tab->video_info[pmt_tab->video_num].video_enc_type = HI_UNF_VCODEC_TYPE_AVS2;
                    } else if (stream_type == STREAM_TYPE_AVS3_VIDEO) {
                        pmt_tab->video_info[pmt_tab->video_num].video_enc_type = HI_UNF_VCODEC_TYPE_AVS3;
                    } else if (stream_type == STREAM_TYPE_HEVC_VIDEO) {
                        pmt_tab->video_info[pmt_tab->video_num].video_enc_type = HI_UNF_VCODEC_TYPE_H265;
                    } else {
                        pmt_tab->video_info[pmt_tab->video_num].video_enc_type = HI_UNF_VCODEC_TYPE_MPEG2;
                    }

                    if (des_len > 0) {
                        parse_pmt_desc(section_data, des_len, pmt_tab, HI_FALSE);
                        section_data += des_len;
                        length -= des_len;
                    }

                    pmt_tab->video_num++;
                }

                break;
            }

            /* audio stream */
            case STREAM_TYPE_11172_AUDIO:
            case STREAM_TYPE_13818_AUDIO:
            case STREAM_TYPE_14496_3_AUDIO:
            case STREAM_TYPE_13818_7_AUDIO:
            case STREAM_TYPE_AC3_AUDIO:
#ifndef STREAM_TYPE_SCTE_82H
            case STREAM_TYPE_DTS_AUDIO:
#endif
            case STREAM_TYPE_DOLBY_TRUEHD_AUDIO:
#if ! defined (STREAM_TYPE_CAPTURE_86)
            case STREAM_TYPE_DTS_MA:
#endif
            {
#ifdef SRCH_DEBUG
                printf("audio stream type is %#x, pid = %#x\n", stream_type, pid);
#endif
                if (pmt_tab->audo_num < PROG_MAX_AUDIO) {
                    pmt_tab->audio_info[pmt_tab->audo_num].audio_pid = pid;
#ifdef SRCH_DEBUG
                    SAMPLE_COMMON_PRINTF(" audio type = 0x%x \n", stream_type);
#endif

                    if ((stream_type == STREAM_TYPE_13818_7_AUDIO) || (stream_type == STREAM_TYPE_14496_3_AUDIO)) {
                        pmt_tab->audio_info[pmt_tab->audo_num].audio_enc_type = HI_UNF_ACODEC_ID_AAC;
                    } else if (stream_type == STREAM_TYPE_AC3_AUDIO) {
                        pmt_tab->audio_info[pmt_tab->audo_num].audio_enc_type = HI_UNF_ACODEC_ID_DOLBY_PLUS;
                    } else if (stream_type == STREAM_TYPE_DOLBY_TRUEHD_AUDIO) {
                        pmt_tab->audio_info[pmt_tab->audo_num].audio_enc_type = HI_UNF_ACODEC_ID_DOLBY_TRUEHD;
                    } else if (stream_type == STREAM_TYPE_DTS_AUDIO
#if ! defined (STREAM_TYPE_CAPTURE_86)
                               || stream_type == STREAM_TYPE_DTS_MA
#endif
) {
                        pmt_tab->audio_info[pmt_tab->audo_num].audio_enc_type = HI_UNF_ACODEC_ID_DTSHD;
                    } else {
                        pmt_tab->audio_info[pmt_tab->audo_num].audio_enc_type = HI_UNF_ACODEC_ID_MP3;
                    }

                    if (des_len > 0) {
                        parse_pmt_desc(section_data, des_len, pmt_tab, HI_TRUE);
                        section_data += des_len;
                        length -= des_len;
                    }

                    pmt_tab->audo_num++;
                }

                break;
            }

            case 0x8b: { /* one special SCTE stream contains error subt info,skip it,not parse */
                if (des_len > 0) {
                    section_data += des_len;
                    length -= des_len;
                }
                break;
            }

            case STREAM_TYPE_PRIVATE: {
#ifdef SRCH_DEBUG
                printf("private stream...........pid=%#x\n", pid);
#endif
                if (des_len > 0) { /* subtitling stream info */
                    if  (is_subtitle_stream(section_data, des_len)) {
                        if (SUBTITLING_MAX > pmt_tab->subtitling_num) {
                            pmt_tab->subtiting_info[pmt_tab->subtitling_num].subtitling_pid = pid;

#ifdef SRCH_DEBUG
                            printf("using pmt_tb->subtitling_num = %d to parse description\n", pmt_tab->subtitling_num);
#endif
                            parse_pmt_desc(section_data, des_len, pmt_tab, HI_FALSE);
                            pmt_tab->subtitling_num++;
                        } else {
                            SAMPLE_COMMON_PRINTF("Subtitle language is over than the max number:%d\n", SUBTITLING_MAX);
                        }
                    } else if (is_aribcc_stream(section_data, des_len)) {
                        pmt_tab->aribcc_pid = pid;
                    } else if (is_ttx_stream(section_data, des_len)) {
                        if (TTX_MAX > pmt_tab->ttx_num) {
                            pmt_tab->ttx_info[pmt_tab->ttx_num].ttx_pid = pid;
                            parse_pmt_desc(section_data, des_len, pmt_tab, HI_FALSE);
                            pmt_tab->ttx_num++;
                        }
                    } else if (is_265_stream(section_data, des_len)) {
                        pmt_tab->video_info[pmt_tab->video_num].video_pid = pid;
                        pmt_tab->video_info[pmt_tab->video_num].video_enc_type = HI_UNF_VCODEC_TYPE_H265;

                        pmt_tab->video_num ++;
                    } else {
                        if (pmt_tab->audo_num < PROG_MAX_AUDIO) {
                            pmt_tab->audio_info[pmt_tab->audo_num].audio_pid = pid;

                            /* dolby parse */
                            if (is_ac3_stream(section_data, des_len)) {
                                pmt_tab->audio_info[pmt_tab->audo_num].audio_enc_type = HI_UNF_ACODEC_ID_DOLBY_PLUS;
                            } else if (is_dts_stream(section_data, des_len)) {
                                pmt_tab->audio_info[pmt_tab->audo_num].audio_enc_type = HI_UNF_ACODEC_ID_DTSHD;
                            } else if (is_dra_stream(section_data, des_len)) {
                                pmt_tab->audio_info[pmt_tab->audo_num].audio_enc_type = HI_UNF_ACODEC_ID_DRA;
                            } else {
                                pmt_tab->audio_info[pmt_tab->audo_num].audio_enc_type = HI_UNF_ACODEC_ID_MP3;
                            }
                            parse_pmt_desc(section_data, des_len, pmt_tab, HI_TRUE);
                            pmt_tab->audo_num++;
                        }
                    }

                    section_data += des_len;
                    length -= des_len;
                }

            }
            break;

#ifdef STREAM_TYPE_SCTE_82H
            case STREAM_TYPE_SCTE: {
                if (des_len > 0) {
                    parse_pmt_desc(section_data, des_len, pmt_tab, HI_FALSE);
                    section_data += des_len;
                    length -= des_len;
                }
                pmt_tab->scte_subt_info.scte_subt_pid = pid;

                break;
            }
#endif

            default: {
                if (des_len > 0) {
                    parse_pmt_desc(section_data, des_len, pmt_tab, HI_FALSE);
                    section_data += des_len;
                    length -= des_len;
                }
            }
            break;
        }
    }

    return HI_SUCCESS;
}

hi_s32 srh_parse_sdt(const hi_u8 *section_data, hi_s32 length, hi_u8 *section_struct)
{
    hi_u8 eit_flag = 0;
    hi_u8 eit_flag_pf = 0;
    hi_u8 run_status = 0;
    hi_u8 free_ca = 0;
    hi_u16 network_id = 0x1FFF;
    hi_u16 program_number = 0;
    hi_u16 ts_id = 0;
    hi_s32 des_len = 0;

    sdt_tb *sdt_tab = (sdt_tb *)section_struct;
    sdt_info *sdt_info = HI_NULL;

    if ((section_data == HI_NULL) || (section_struct == HI_NULL)) {
        return HI_FAILURE;
    }

    if (section_data[0] != SDT_TABLE_ID_ACTUAL) {
        return HI_FAILURE;
    }

    length = (hi_s32)((section_data[1] << 8) | section_data[2]) & 0x0fff;

    ts_id = (hi_u16)((section_data[3] << 8) | section_data[4]);
    network_id = (hi_u16)((section_data[8] << 8) | section_data[9]);

    sdt_tab->ts_id  = ts_id;
    sdt_tab->net_id = network_id;

    section_data += 11;
    length -= 11;

    while (length > 4) {
        sdt_info = &(sdt_tab->sdt_info[sdt_tab->prog_num]);

        program_number = (hi_u16)((section_data[0] << 8) | section_data[1]);

        eit_flag = (hi_u8)((section_data[2] & 0x02) >> 1);
        eit_flag_pf = (hi_u8)(section_data[2] & 0x01);

        run_status = (hi_u8)((section_data[3] & 0xE0) >> 5);

        free_ca = (hi_u8)((section_data[3] & 0x10) >> 4);

        sdt_info->service_id = program_number;
        sdt_info->eit_flag = eit_flag;
        sdt_info->eit_flag_pf = eit_flag_pf;
        sdt_info->run_state = run_status;
        sdt_info->ca_mode = free_ca;

        des_len = (hi_s32)(((section_data[3] & 0x0F) << 8) | section_data[4]);

        section_data += 5;
        length -= 5;

        parse_sdt_desc(section_data, des_len, sdt_info);

        section_data += des_len;
        length -= des_len;
        sdt_tab->prog_num++;
    }

    return HI_SUCCESS;
}

/* Description:get Pat table */
hi_s32 srh_pat_request_ext(hi_u32 dmx_id, pat_tb *pat_tab, hi_bool tee_enable)
{
    hi_s32 ret;
    dmx_data_filter data_filter;

#ifdef SRCH_DEBUG
    printf("\n ++++ PAT Request dmxid = %d \n", dmx_id);
#endif

    memset(data_filter.match, 0, DMX_FILTER_MAX_DEPTH * sizeof(hi_u8));
    memset(data_filter.mask, 0, DMX_FILTER_MAX_DEPTH * sizeof(hi_u8));
    memset(data_filter.negate, 0, DMX_FILTER_MAX_DEPTH * sizeof(hi_u8));

    data_filter.tspid   = PAT_TSPID;
    data_filter.time_out = PGPAT_TIMEOUT;
    data_filter.filter_depth = 1;
    data_filter.crcflag = 0;

    data_filter.match[0] = PAT_TABLE_ID;
    data_filter.mask[0] = 0x0;

    /* set call back func */
    data_filter.fun_section_fun_callback = &srh_parse_pat;
    data_filter.section_struct = (hi_u8 *)pat_tab;

    /* start data filter */
    ret = dmx_section_start_data_filter_ext(dmx_id, &data_filter, tee_enable);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("\n No PAT received \n");
    }

    return ret;
}
hi_s32 srh_pat_request(hi_u32 dmx_id, pat_tb *pat_tab)
{
    return srh_pat_request_ext(dmx_id, pat_tab, HI_FALSE);
}

hi_s32 srh_pmt_request_ext(hi_u32 dmx_id, pmt_tb *pmt_tab, hi_u16 pmt_pid, hi_u16 service_id, hi_bool tee_enable)
{
    hi_s32 ret;
    dmx_data_filter data_filter;

#ifdef SRCH_DEBUG
    SAMPLE_COMMON_PRINTF("+++++PMT PID %d(0x%x) ServiceID %d(0x%x)+++++\n", pmt_pid, pmt_pid, service_id, service_id);
#endif
    memset(data_filter.match, 0x00, DMX_FILTER_MAX_DEPTH * sizeof(hi_u8));
    memset(data_filter.mask, 0xff, DMX_FILTER_MAX_DEPTH * sizeof(hi_u8));
    memset(data_filter.negate, 0x00, DMX_FILTER_MAX_DEPTH * sizeof(hi_u8));

    data_filter.tspid   = pmt_pid;
    data_filter.time_out = PGPMT_TIMEOUT;
    data_filter.filter_depth = 3;
    data_filter.crcflag = 0;

    data_filter.match[0] = PMT_TABLE_ID;
    data_filter.mask[0] = 0x0;

    /* set service id */
    data_filter.match[1] = (hi_u8)((service_id >> 8) & 0x00ff);
    data_filter.mask[1] = 0;

    data_filter.match[2] = (hi_u8)(service_id & 0x00ff);
    data_filter.mask[2] = 0;

    /* set call back func */
    data_filter.fun_section_fun_callback = &srh_parse_pmt;
    data_filter.section_struct = (hi_u8 *)pmt_tab;

    /* start data filter */
    ret = dmx_section_start_data_filter_ext(dmx_id, &data_filter, tee_enable);
    if (ret == HI_FAILURE) {
        SAMPLE_COMMON_PRINTF("DMX_SectionStartDataFilter failed to receive PMT\n");
    }

#ifdef SRCH_DEBUG
    hi_s32 i;
    SAMPLE_COMMON_PRINTF("ServiceID %d  PcrPID %#x  VideoNum %d  AudioNum %d\n",
                         pmt_tab->service_id, pmt_tab->pcr_pid, pmt_tab->video_num, pmt_tab->audo_num);
    for (i = 0; i < pmt_tab->video_num; i++) {
        SAMPLE_COMMON_PRINTF("Video[%d] PID %#x Vtype %#x\n", i, pmt_tab->video_info[i].video_pid,
                             pmt_tab->video_info[i].video_enc_type);
    }

    for (i = 0; i < pmt_tab->audo_num; i++) {
        SAMPLE_COMMON_PRINTF("Audio[%d] PID %#x Atype %#x\n", i, pmt_tab->audio_info[i].audio_pid,
                             pmt_tab->audio_info[i].audio_enc_type);
    }

    SAMPLE_COMMON_PRINTF("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
#endif
    return ret;
}

hi_s32 srh_pmt_request(hi_u32 dmx_id, pmt_tb *pmt_tab, hi_u16 pmt_pid, hi_u16 service_id)
{
    return srh_pmt_request_ext(dmx_id, pmt_tab, pmt_pid, service_id, HI_FALSE);
}

hi_s32 srh_sdt_request(hi_u32 dmx_id, sdt_tb *sdt_tab)
{
    hi_s32 ret;
    dmx_data_filter data_filter;

#ifdef SRCH_DEBUG
    SAMPLE_COMMON_PRINTF("\n ++++ SDT Request");
#endif
    memset(data_filter.match, 0x00, DMX_FILTER_MAX_DEPTH * sizeof(hi_u8));
    memset(data_filter.mask, 0xff, DMX_FILTER_MAX_DEPTH * sizeof(hi_u8));
    memset(data_filter.negate, 0x00, DMX_FILTER_MAX_DEPTH * sizeof(hi_u8));

    data_filter.tspid = SDT_TSPID;                      /* set ts pid */
    data_filter.time_out = PGSDT_TIMEOUT;               /* timeout */
    data_filter.filter_depth = 4;
    data_filter.crcflag = 0;

    data_filter.match[0] = SDT_TABLE_ID_ACTUAL;
    data_filter.mask[0] = 0x0;

    /* set call back func */
    data_filter.fun_section_fun_callback = &srh_parse_sdt;
    data_filter.section_struct = (hi_u8 *)sdt_tab;

    /* start data filter */
    ret = dmx_section_start_data_filter(0, &data_filter);
    if (ret == HI_FAILURE) {
        SAMPLE_COMMON_PRINTF("\n No SDT received\n");
    }

    return (ret);
}

hi_s32 dvb_search_start(hi_u32 dmx_id)
{
    db_program prog_info = {0};
    pat_tb pat_tb  = {0};
    pmt_tb pmt_tab = {0};
    sdt_tb sdt_tab = {0};
    hi_u32 i, j;
    hi_s32 ret;

    ret = srh_pat_request(dmx_id, &pat_tb);
    if (ret == HI_FAILURE) {
        return HI_FAILURE;
    }

    for (i = 0; i < pat_tb.prog_num; i++) {
        memset(&prog_info, 0, sizeof(db_program));
        prog_info.frontend_id = SEARCHING_FRONTEND_ID;
        prog_info.ts_id   = pat_tb.ts_id;
        prog_info.pmt_pid = pat_tb.pat_info[i].pmt_pid;
        prog_info.service_id = pat_tb.pat_info[i].service_id;

        memset(&pmt_tab, 0, sizeof(pmt_tab));
        if (srh_pmt_request(dmx_id, &pmt_tab, prog_info.pmt_pid, prog_info.service_id) == HI_SUCCESS) {
            prog_info.pcr_pid = pmt_tab.pcr_pid;
            prog_info.audio_channel = pmt_tab.audo_num;
            for (j = 0; j <= pmt_tab.audo_num; j++) {
                prog_info.audio_ex[j].u16audiopid = pmt_tab.audio_info[j].audio_pid;
                prog_info.audio_ex[j].audio_enc_type = pmt_tab.audio_info[j].audio_enc_type;
            }

            prog_info.video_channel = pmt_tab.video_num;
            prog_info.video_ex.video_pid = pmt_tab.video_info[0].video_pid;
            prog_info.video_ex.video_enc_type = pmt_tab.video_info[0].video_enc_type;
        }

        db_add_dvbprog(&prog_info);
    }

    ret = srh_sdt_request(dmx_id, &sdt_tab);
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }

    for (i = 0; i < sdt_tab.prog_num; i++) {
        memset(&prog_info, 0, sizeof(db_program));

        ret = db_get_dvbprog_info_by_service_id(sdt_tab.sdt_info[i].service_id, &prog_info);
        if (ret == HI_FAILURE) {
            continue;
        }

        prog_info.network_id  = sdt_tab.net_id;
        prog_info.service_type = sdt_tab.sdt_info[i].service_type;
        memcpy(prog_info.program_name, sdt_tab.sdt_info[i].prog_name, MAX_PROGNAME_LENGTH);
        db_set_dvbprog_info(ret, &prog_info);
    }

    return HI_SUCCESS;
}

hi_void dvb_save_search(hi_u32 frontend_id)
{
    db_program prog_info;
    hi_u32 i;

    for (i = 0; i < db_get_prog_total_count(); i++) {
        if (db_get_dvbprog_info(i, &prog_info) == HI_FAILURE) {
            break;
        }

        if (prog_info.frontend_id == SEARCHING_FRONTEND_ID) {
            prog_info.frontend_id = frontend_id;
            db_set_dvbprog_info(i, &prog_info);
        }
    }
}

hi_void dvb_list_prog()
{
    hi_s32 i, j;
    db_program prog_info;

    SAMPLE_COMMON_PRINTF("\tList TV program\n");

    for (i = 0; i < db_get_prog_total_count(); i++) {
        if (db_get_dvbprog_info(i, &prog_info) != HI_FAILURE) {
            SAMPLE_COMMON_PRINTF("\n%-10d %-20s \n", i, prog_info.program_name);
            if (prog_info.video_channel > 0) {
                SAMPLE_COMMON_PRINTF("\tVideo Stream PID = %d(0x%04x) ", prog_info.video_ex.video_pid,
                                     prog_info.video_ex.video_pid);
                switch (prog_info.video_ex.video_enc_type) {
                    case HI_UNF_VCODEC_TYPE_H264:
                        SAMPLE_COMMON_PRINTF("\tStream Type H264\n");
                        break;
                    case HI_UNF_VCODEC_TYPE_MPEG4:
                        SAMPLE_COMMON_PRINTF("\tStream Type MPEG4\n");
                        break;
                    case HI_UNF_VCODEC_TYPE_MPEG2:
                        SAMPLE_COMMON_PRINTF("\tStream Type MPEG2\n");
                        break;
                    case HI_UNF_VCODEC_TYPE_H265:
                        SAMPLE_COMMON_PRINTF("\tStream Type HEVC\n");
                        break;
                    default:
                        SAMPLE_COMMON_PRINTF("\tStream Type error\n");
                }
            }

            for (j = 0; j < prog_info.audio_channel; j++) {
                SAMPLE_COMMON_PRINTF("\tAudio Stream PID = %d(0x%04x)", prog_info.audio_ex[j].u16audiopid,
                                     prog_info.audio_ex[j].u16audiopid);
                switch (prog_info.audio_ex[j].audio_enc_type) {
                    case HI_UNF_ACODEC_ID_AAC:
                        SAMPLE_COMMON_PRINTF("\tStream Type AAC\n");
                        break;
                    case HI_UNF_ACODEC_ID_MP3:
                        SAMPLE_COMMON_PRINTF("\tStream Type MP3\n");
                        break;
                    case HI_UNF_ACODEC_ID_DOLBY_PLUS:
                        SAMPLE_COMMON_PRINTF("\tStream Type AC3\n");
                        break;
                    case HI_UNF_ACODEC_ID_DTSHD:
                        SAMPLE_COMMON_PRINTF("\tStream Type DTSHD\n");
                        break;
                    case HI_UNF_ACODEC_ID_DRA:
                        SAMPLE_COMMON_PRINTF("\tStream Type DRA\n");
                        break;
                    case HI_UNF_ACODEC_ID_DOLBY_TRUEHD:
                        SAMPLE_COMMON_PRINTF("\tStream Type DOLBY TRUEHD\n");
                        break;
                    default:
                        SAMPLE_COMMON_PRINTF("\tStream Type error\n");
                }
            }
        }
    }

    SAMPLE_COMMON_PRINTF("\n\n");
}

static hi_bool g_srch_init = HI_FALSE;

hi_void hi_adp_search_init()
{
    if (g_srch_init == HI_FALSE) {
        g_srch_init = HI_TRUE;
    }

    return;
}

/* get PMT table */
hi_s32  hi_adp_search_get_all_pmt_ext(hi_u32 dmx_id, pmt_compact_tbl **pp_prog_table, hi_bool tee_enable)
{
    hi_s32 i, j, ret;
    pat_tb pat_tb = {0};
    pmt_tb pmt_tb = {0};
    pmt_compact_tbl *prog_table = HI_NULL;
    hi_u32 promnum = 0;
    hi_u32 cnt = 0;

    hi_u32 subtitle_index = 0;
    hi_u32 subt_desp_index = 0;

    if (pp_prog_table == HI_NULL) {
        SAMPLE_COMMON_PRINTF("para is null pointer!\n");
        return HI_FAILURE;
    }

    do {
        ret = srh_pat_request_ext(dmx_id, &pat_tb, tee_enable);
        if (ret != HI_SUCCESS) {
            SAMPLE_COMMON_PRINTF("failed to search PAT\n");
            return HI_FAILURE;
        }

        if (pat_tb.prog_num != 0) {
            break;
        }
        usleep(5000);
        cnt += 5;
    } while (cnt < 5000);

    prog_table = (pmt_compact_tbl *)malloc(sizeof(pmt_compact_tbl));
    if (prog_table == HI_NULL) {
        SAMPLE_COMMON_PRINTF("have no memory for pat\n");
        return HI_FAILURE;
    }

    prog_table->prog_num = pat_tb.prog_num;
    prog_table->proginfo = (pmt_compact_prog *)malloc(pat_tb.prog_num * sizeof(pmt_compact_prog));
    if (prog_table->proginfo == HI_NULL) {
        SAMPLE_COMMON_PRINTF("have no memory for pat\n");
        free(prog_table);
        prog_table = HI_NULL;
        return HI_FAILURE;
    }

    memset(prog_table->proginfo, 0, pat_tb.prog_num * sizeof(pmt_compact_prog));

    SAMPLE_COMMON_PRINTF("\n\nALL Program Infomation[%d]:\n", pat_tb.prog_num);


    for (i = 0; i < pat_tb.prog_num; i++) {
        if ((pat_tb.pat_info[i].service_id == 0) || (pat_tb.pat_info[i].pmt_pid == 0x1fff)) {
            continue;
        }

        memset(&pmt_tb, 0, sizeof(pmt_tb));
        ret = srh_pmt_request_ext(dmx_id, &pmt_tb, pat_tb.pat_info[i].pmt_pid,
                                  pat_tb.pat_info[i].service_id, tee_enable);
        if (ret != HI_SUCCESS) {
            SAMPLE_COMMON_PRINTF("failed to search PMT[%d]\n", i);
            continue;
        }
        prog_table->proginfo[promnum].prog_id = pat_tb.pat_info[i].service_id;
        prog_table->proginfo[promnum].pmt_pid = pat_tb.pat_info[i].pmt_pid;
        prog_table->proginfo[promnum].pcr_pid = pmt_tb.pcr_pid;
        prog_table->proginfo[promnum].video_type   = pmt_tb.video_info[0].video_enc_type;
        prog_table->proginfo[promnum].v_element_num = pmt_tb.video_num;
        prog_table->proginfo[promnum].v_element_pid = pmt_tb.video_info[0].video_pid;
        prog_table->proginfo[promnum].audio_type   = pmt_tb.audio_info[0].audio_enc_type;
        prog_table->proginfo[promnum].a_element_num = pmt_tb.audo_num;
        prog_table->proginfo[promnum].a_element_pid = pmt_tb.audio_info[0].audio_pid;

        prog_table->proginfo[promnum].subtitling_num = pmt_tb.subtitling_num;
        prog_table->proginfo[promnum].scte_subt_info.scte_subt_pid = pmt_tb.scte_subt_info.scte_subt_pid;
        prog_table->proginfo[promnum].closed_caption_num = pmt_tb.closed_caption_num;
        memcpy(prog_table->proginfo[promnum].closed_caption, pmt_tb.closed_caption,
               sizeof(pmt_tb.closed_caption));
        prog_table->proginfo[promnum].aribcc_pid = pmt_tb.aribcc_pid;
        prog_table->proginfo[promnum].ttx_num = pmt_tb.ttx_num;
        memcpy(prog_table->proginfo[promnum].ttx_info, pmt_tb.ttx_info, sizeof(pmt_ttx)*TTX_MAX);
        memcpy(prog_table->proginfo[promnum].pmt_data, pmt_tb.pmt_data, pmt_tb.pmt_len);
        prog_table->proginfo[promnum].pmt_len = pmt_tb.pmt_len;

        for (j = 0; j < pmt_tb.audo_num; j++) {
            /* added by gaoyanfeng 00182102 for multi-audio begin */
            if (j < PROG_MAX_AUDIO) {
                prog_table->proginfo[promnum].audio_info[j].audio_pid = pmt_tb.audio_info[j].audio_pid;
                prog_table->proginfo[promnum].audio_info[j].audio_enc_type = pmt_tb.audio_info[j].audio_enc_type;
                prog_table->proginfo[promnum].audio_info[j].ad_type = pmt_tb.audio_info[j].ad_type;
                prog_table->proginfo[promnum].audio_info[j].aud_lang[0] = pmt_tb.audio_info[j].aud_lang[0];
                prog_table->proginfo[promnum].audio_info[j].aud_lang[1] = pmt_tb.audio_info[j].aud_lang[1];
                prog_table->proginfo[promnum].audio_info[j].aud_lang[2] = pmt_tb.audio_info[j].aud_lang[2];
            }
        }

        prog_table->proginfo[promnum].ca_num = pmt_tb.ca_num;
        for (j = 0; j < pmt_tb.ca_num; j++) {
            if (j < PROG_MAX_CA) {
                prog_table->proginfo[promnum].ca_system[j].ca_system_id = pmt_tb.ca_system[j].ca_system_id;
                prog_table->proginfo[promnum].ca_system[j].cap_id = pmt_tb.ca_system[j].cap_id;
            }
        }
#ifdef SRCH_DEBUG
        printf("prog_table->proginfo[%d].subtitling_num = %d\n", promnum, prog_table->proginfo[promnum].subtitling_num);
#endif

        if (SUBTITLING_MAX < prog_table->proginfo[promnum].subtitling_num) {
            SAMPLE_COMMON_PRINTF("receive subtitling_num(%d) are over than the max number:%d\n",
                                 prog_table->proginfo[promnum].subtitling_num, SUBTITLING_MAX);
            prog_table->proginfo[promnum].subtitling_num = SUBTITLING_MAX;
        }

        /* parse and deal with subtitling info */
        for (subtitle_index = 0; subtitle_index < prog_table->proginfo[promnum].subtitling_num; subtitle_index++) {
            prog_table->proginfo[promnum].subtiting_info[subtitle_index].subtitling_pid =
                pmt_tb.subtiting_info[subtitle_index].subtitling_pid;

            prog_table->proginfo[promnum].subtiting_info[subtitle_index].des_info_cnt =
                pmt_tb.subtiting_info[subtitle_index].des_info_cnt;

            if (SUBTDES_INFO_MAX < prog_table->proginfo[promnum].subtiting_info[subtitle_index].des_info_cnt) {
                SAMPLE_COMMON_PRINTF("receive des_info_cnt(%d) are over than the max number:%d\n",
                                     prog_table->proginfo[promnum].subtiting_info[subtitle_index].des_info_cnt, SUBTDES_INFO_MAX);
                prog_table->proginfo[promnum].subtiting_info[subtitle_index].des_info_cnt = SUBTDES_INFO_MAX;
            }
#ifdef SRCH_DEBUG
            printf("prog_table->proginfo[%d].subtiting_info[%d].des_info_cnt = %d\n", promnum, subtitle_index,
                   prog_table->proginfo[promnum].subtiting_info[subtitle_index].des_info_cnt);
#endif

            for (subt_desp_index = 0;
                 subt_desp_index < prog_table->proginfo[promnum].subtiting_info[subtitle_index].des_info_cnt;
                 subt_desp_index++) {
                prog_table->proginfo[promnum].subtiting_info[subtitle_index].des_info[subt_desp_index].lang_code =
                    pmt_tb.subtiting_info[subtitle_index].des_info[subt_desp_index].lang_code;

                prog_table->proginfo[promnum].subtiting_info[subtitle_index].des_info[subt_desp_index].page_id =
                    pmt_tb.subtiting_info[subtitle_index].des_info[subt_desp_index].page_id;

                prog_table->proginfo[promnum].subtiting_info[subtitle_index].des_info[subt_desp_index].ancillary_page_id =
                    pmt_tb.subtiting_info[subtitle_index].des_info[subt_desp_index].ancillary_page_id;

#if SRCH_DEBUG
                SAMPLE_COMMON_PRINTF("pid:0x%02x,desp:%u, page id:%u, ancilary page id:%u, lang code: %#x\n",
                                     prog_table->proginfo[promnum].subtiting_info[subtitle_index].subtitling_pid, subt_desp_index,
                                     prog_table->proginfo[promnum].subtiting_info[subtitle_index].des_info[subt_desp_index].page_id,
                                     prog_table->proginfo[promnum].subtiting_info[subtitle_index].des_info[subt_desp_index].ancillary_page_id,
                                     prog_table->proginfo[promnum].subtiting_info[subtitle_index].des_info[subt_desp_index].lang_code);
#endif
            }
        }

        SAMPLE_COMMON_PRINTF("Channel Num = %d, Program ID = %d PMT PID = 0x%x,\n", promnum + 1,
                             pat_tb.pat_info[i].service_id,
                             pat_tb.pat_info[i].pmt_pid);
        for (j = 0; j < pmt_tb.audo_num; j++) {
            SAMPLE_COMMON_PRINTF("\tAudio Stream PID   = 0x%x\n", pmt_tb.audio_info[j].audio_pid);

            switch (pmt_tb.audio_info[j].audio_enc_type) {
                case HI_UNF_ACODEC_ID_MP3:
                    SAMPLE_COMMON_PRINTF("\tAudio Stream Type MP3\n");
                    break;
                case HI_UNF_ACODEC_ID_AAC:
                    SAMPLE_COMMON_PRINTF("\tAudio Stream Type AAC\n");
                    break;
                case HI_UNF_ACODEC_ID_DOLBY_PLUS:
                    SAMPLE_COMMON_PRINTF("\tAudio Stream Type AC3\n");
                    break;
                case HI_UNF_ACODEC_ID_DTSHD:
                    SAMPLE_COMMON_PRINTF("\tAudio Stream Type DTS\n");
                    break;
                case HI_UNF_ACODEC_ID_DRA:
                    SAMPLE_COMMON_PRINTF("\tAudio Stream Type DRA\n");
                    break;
                case HI_UNF_ACODEC_ID_DOLBY_TRUEHD:
                    SAMPLE_COMMON_PRINTF("\tAudio Stream Type DOLBY TRUEHD\n");
                    break;
                case HI_UNF_ACODEC_ID_MS12_AC4:
                    SAMPLE_COMMON_PRINTF("\tAudio Stream Type DOLBY AC4\n");
                    break;
                default:
                    SAMPLE_COMMON_PRINTF("\tAudio Stream Type error\n");
            }
        }

        for (j = 0; j < pmt_tb.video_num; j++) {
            SAMPLE_COMMON_PRINTF("\tVideo Stream PID   = 0x%x\n", pmt_tb.video_info[j].video_pid);
            switch (pmt_tb.video_info[j].video_enc_type) {
                case HI_UNF_VCODEC_TYPE_H264:
                    SAMPLE_COMMON_PRINTF("\tVideo Stream Type H264\n");
                    break;
                case HI_UNF_VCODEC_TYPE_MPEG2:
                    SAMPLE_COMMON_PRINTF("\tVideo Stream Type MPEG2\n");
                    break;
                case HI_UNF_VCODEC_TYPE_MPEG4:
                    SAMPLE_COMMON_PRINTF("\tVideo Stream Type MPEG4\n");
                    break;
                case HI_UNF_VCODEC_TYPE_H265:
                    SAMPLE_COMMON_PRINTF("\tVideo Stream Type HEVC\n");
                    break;
                case HI_UNF_VCODEC_TYPE_AVS:
                    SAMPLE_COMMON_PRINTF("\tVideo Stream Type AVS\n");
                    break;
                case HI_UNF_VCODEC_TYPE_AVS2:
                    SAMPLE_COMMON_PRINTF("\tVideo Stream Type AVS2\n");
                    break;
                default:
                    SAMPLE_COMMON_PRINTF("\tVideo Stream Type error\n");
            }
        }

        if (pmt_tb.subtitling_num > 0) {
            prog_table->proginfo[promnum].subt_type |= SUBT_TYPE_DVB;
            SAMPLE_COMMON_PRINTF("\tDVB subtitle NUM = 0x%x\n", pmt_tb.subtitling_num);
        }
        if (pmt_tb.scte_subt_info.scte_subt_pid) {
            prog_table->proginfo[promnum].subt_type |= SUBT_TYPE_SCTE;
            SAMPLE_COMMON_PRINTF("\tSCTE subtitle PID = 0x%x\n", pmt_tb.scte_subt_info.scte_subt_pid);
        }
        if (prog_table->proginfo[promnum].closed_caption_num > 0) {
            for (j = 0; j < prog_table->proginfo[promnum].closed_caption_num; j++) {
                if (prog_table->proginfo[promnum].closed_caption[j].is_digital_cc) {
                    SAMPLE_COMMON_PRINTF("\tClosed Captioning 708, language : %#x, service num : %d\n",
                                         prog_table->proginfo[promnum].closed_caption[j].lang_code,
                                         prog_table->proginfo[promnum].closed_caption[j].service_number);
                } else {
                    SAMPLE_COMMON_PRINTF("\tClosed Captioning 608\n");
                }
            }
        }

        if (prog_table->proginfo[promnum].aribcc_pid) {
            SAMPLE_COMMON_PRINTF("\tARIB CC PID = 0x%x\n", prog_table->proginfo[promnum].aribcc_pid);
        }

        if (prog_table->proginfo[promnum].ttx_num > 0) {
            SAMPLE_COMMON_PRINTF("\tTeletext NUM = 0x%x\n", pmt_tb.ttx_num);
            for (j = 0; j < prog_table->proginfo[promnum].ttx_num; j++) {
                SAMPLE_COMMON_PRINTF("\tTeletext%d PID = 0x%x\n", j, pmt_tb.ttx_info[j].ttx_pid);
            }
        }

        if ((pmt_tb.video_num > 0) || (pmt_tb.audo_num > 0)) {
            promnum++;
        }
    }

    prog_table->prog_num = promnum;

    SAMPLE_COMMON_PRINTF("\n");

    *pp_prog_table = prog_table;

    return promnum ? HI_SUCCESS : HI_FAILURE;
}
hi_s32  hi_adp_search_get_all_pmt(hi_u32 dmx_id, pmt_compact_tbl **pp_prog_table)
{
    return hi_adp_search_get_all_pmt_ext(dmx_id, pp_prog_table, HI_FALSE);
}

hi_s32  hi_adp_search_free_all_pmt(pmt_compact_tbl *prog_table)
{
    if (prog_table != HI_NULL) {
        if (prog_table->proginfo != HI_NULL) {
            free(prog_table->proginfo);
            prog_table->proginfo = HI_NULL;
        }

        free(prog_table);
        prog_table = HI_NULL;
    }

    return HI_SUCCESS;
}

hi_void hi_adp_search_de_init()
{
    if (g_srch_init == HI_TRUE) {
        g_srch_init = HI_FALSE;
    }

    return;
}
