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
* file inv_sys_obj.c
*
* brief :- Object creater
*
*****************************************************************************/

/***** #include statements ***************************************************/
#define INV_DEBUG 0

#include "inv_common.h"
#include "inv_sys_log_api.h"
#include "inv_sys_obj_api.h"
#include "inv_sys_time_api.h"
#include "inv478x_platform_api.h"

/***** Register Module name **************************************************/

INV_MODULE_NAME_SET (lib_log, 8);

/***** local macro definitions ***********************************************/

#define LOG_LINE_WRAP              (80)
#define LOG_LENGTH_LHS_MAX         (34)
#define LOG_LENGTH_CLASS_NAME      ((LOG_LENGTH_LHS_MAX-2)/3)
#define LOG_LENGTH_OBJECT_NAME     ((LOG_LENGTH_LHS_MAX-2)/3)

/***** local type definitions ************************************************/

/***** local prototypes ******************************************************/

static void s_put_char(char character);
static uint8_t s_put_string(const char *p_str);
static uint8_t s_log_limited(uint8_t size, const char *p_str);
static void s_log_put_string(char *p_str);

/***** local data objects ****************************************************/

static uint16_t s_line_pos;

/***** public functions ******************************************************/

void inv_sys_log_time_stamp(const char *p_class_str, const char *p_func_str, void *p_obj)
{
    if (p_func_str) {
        inv_sys_time_milli msec = (inv_sys_time_milli) inv_platform_time_msec_query();
        uint16_t tot = 0;
        uint16_t ts_len = 0;

        /*
         * Print time stamp
         */
        {
            char str[40];

            s_put_char('\n');
            INV_SPRINTF (str, "%d", msec / 1000);
            ts_len += s_put_string(str);
            s_put_char('.');
            INV_SPRINTF (str, "%03d", msec % 1000);
            ts_len += s_put_string(str);
            s_put_char('-');
            ts_len += 3;
        }
        /*
         * Print module name
         */
        tot += s_log_limited(LOG_LENGTH_CLASS_NAME, p_class_str);

        /*
         * If instance print instance name
         */
        if (p_obj) {
            /*
             * Separation character
             */
            s_put_char('.');
            tot++;
            tot += s_log_limited(LOG_LENGTH_OBJECT_NAME, inv_sys_obj_name_get(p_obj));
        }
        if (*p_func_str) {
            /*
             * Separation character
             */
            s_put_char('.');
            tot++;
            tot += s_log_limited((uint8_t) (LOG_LENGTH_LHS_MAX - tot), p_func_str);
        }
        /*
         * Print alignment space characters
         */
        tot = (LOG_LENGTH_LHS_MAX < tot) ? (0) : (LOG_LENGTH_LHS_MAX - tot);
        while (tot--)
            s_put_char(' ');
        /*
         * Print end of preamble
         */
        s_line_pos = ts_len + LOG_LENGTH_LHS_MAX - 1;   /* - 1 to compensate for the newline counted with the timestamp */
        s_line_pos += s_put_string(": ");
    }
}

void inv_sys_log_printf(const char *p_frm, ...)
{
#define STR_LEN   160
    va_list arg;
    char str[STR_LEN];          /* CEC_LOGGER requires 160 */
    va_start(arg, p_frm);
    INV_VSPRINTF (str, p_frm, arg);
    va_end(arg);
#ifdef UART_ENABLE
    uart_write("\n\r", 2);
    uart_write((uint8_t *) str, strlen(str));
#endif
    /*
     * INV_ASSERT(STR_LEN > ((int)sizeof(str)));
     */
    s_log_put_string(str);
}

void inv_printf(const char *p_frm, ...)
{
#define STR_LEN   160
    va_list arg;
    char str[STR_LEN];          /* CEC_LOGGER requires 160 */
    va_start(arg, p_frm);
    INV_VSPRINTF (str, p_frm, arg);
    va_end(arg);
    inv_platform_log_print(str);
}

void inv_sys_dump_data_array(char *ptitle, const uint8_t *parray, int length)
{
    INV_ASSERT (parray);
    INV_LOG_STRING ((ptitle));
    INV_LOG_STRING (("|\n"));
    INV_LOG_ARRAY (parray, (uint16_t) length);
    /*
     * for (i = 0, row = 0; i < length; row++)
     * {
     * INV_LOG1B(("\n%04X| ", i));
     * for (col = 0; col < 16; col++)
     * {
     * INV_LOG1B(("%02X ", (int)parray[i]));
     * i++;
     * if (i >= length)
     * break;
     * }
     * }
     */
    INV_LOG_STRING (("|\n"));
}

/***** local functions *******************************************************/

static uint8_t s_put_string(const char *p_str)
{
    uint8_t len = (uint8_t) INV_STRLEN (p_str);
    inv_platform_log_print(p_str);
    return len;
}

static void s_put_char(char character)
{
    char str[2];
    str[0] = character;
    str[1] = 0;
    inv_platform_log_print(str);
}

static uint8_t s_log_limited(uint8_t size, const char *p_str)
{
    uint8_t i = 0;
    uint8_t len = (uint8_t) INV_STRLEN (p_str);
    /*
     * does string length exceed size limitation?
     */
    if (len > size) {
        /*
         * truncate left of string
         */
        s_put_char('*');
        p_str += (len - size + 1);
        i++;
    }
    i += s_put_string(p_str);
    return i;
}

static void s_log_put_string(char *p_str)
{
    uint8_t last_space = 0;
    uint8_t index = 0;
    while (p_str[index]) {
        if (s_line_pos == 0) {
            s_line_pos = s_put_string("\n    ");
        }
        if (p_str[index] == ' ')
            last_space = index;

        if (p_str[index] == '\n') {
            p_str[index] = 0x00;
            s_put_string(p_str);
            s_line_pos = 0;
            p_str += (index + 1);
            last_space = index = 0;
        } else {
            index++;
        }

        if ((s_line_pos + index) >= LOG_LINE_WRAP) {
            if (last_space != 0) {
                p_str[last_space] = 0x00;
                s_put_string(p_str);
                p_str += (last_space + 1);
            }
            s_line_pos = s_put_string("\n    ");
            last_space = index = 0;
        }
    }
    if (index != 0)
        s_line_pos += s_put_string(p_str);
}

/***** end of file ***********************************************************/
