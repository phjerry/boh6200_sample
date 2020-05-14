#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "hi_type.h"
#include "hi_unf_system.h"
#include "hi_unf_flash.h"
#include "sample_flash.h"

#define FLASH_NAME  "/dev/mtd3"

hi_s32 main(int argc, char *argv[])
{
    hi_flash_info info;
    char *image_file = NULL;
    hi_u8 *p_wbuf = NULL;
    hi_handle fd;
    hi_s32 img_fd = -1;
    hi_s32 ret;
    hi_u32 img_len2;
    hi_u64 start_addr = 0;
    hi_s32 ret_len = 0;
    char *partion_name = NULL;

    if ((argc != ARG_NUM2) && (argc != ARG_NUM3)) {
        printf("Usage:\n"
               "\t%s file_path [partion_name]\n"
               "Example:\n"
               "\t%s test.yaffs /dev/mtd3\n", argv[0], argv[0]);
        return 0;
    }

    ret = hi_unf_sys_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_sys_init failed.\n");
        return ret;
    }

    image_file = argv[ARG1];
    if (argv[ARG2]) {
        partion_name = argv[ARG2];
    }

    fd = hi_unf_flash_open(HI_UNF_FLASH_TYPE_NAND_0, partion_name);
    if (fd == INVALID_FD)
        return -1;

    img_fd = open(image_file, O_RDONLY);
    if (img_fd < 0) {
        printf("img_fd=%d: open yaffs fail!\n", img_fd);
        goto img_error2;
    }

    ret = hi_unf_flash_getinfo(fd, &info);
    if (ret != 0) {
        printf("ret=%d: get flash info fail!\n", ret);
        goto img_error1;
    }

    img_len2 = (info.page_size + info.oob_size) *
               (info.block_size / info.page_size);

    p_wbuf = (hi_u8 *) malloc(img_len2);
    if (p_wbuf == NULL) {
        printf("p_wbuf malloc fail!\n");
        goto img_error1;
    }

    lseek(img_fd, 0, SEEK_SET);

    while (1) {
        if ((ret_len = read(img_fd, p_wbuf, img_len2)) <= 0) {
            printf("ret=%d, img_len2=0x%x: read yaffs %s!\n",
                   ret_len, img_len2, ret_len ? "fail" : "end");
            goto img_error0;
        }

        ret = hi_unf_flash_write(fd, start_addr, p_wbuf,
                                 ret_len, HI_FLASH_RW_FLAG_WITH_OOB);
        if (ret <= 0) {
            printf("ret=%d: write length=0x%x, start_addr=0x%llx: "
                   "write yaffs fail!\n", ret, ret_len, start_addr);
            goto img_error0;
        }

        if (ret_len % (info.page_size + info.oob_size)) {
            printf("wirte size(0x%x) should be aligned with "
                   "pagesize+oobsize(0x%x))!\n",
                   ret_len,
                   info.page_size + info.oob_size);
            break;
        }

        start_addr += info.block_size;
    }

img_error0:
    free(p_wbuf);
    printf("> %s: %d\n", __FUNCTION__, __LINE__);

img_error1:
    ret = close(img_fd);
    if (ret != 0)
        printf("ret=%d: close yaffs failed!\n", ret);

img_error2:
    ret = hi_unf_flash_close(fd);
    if (ret != 0) {
        printf("ret=%d: Flash Close failed!\n", ret);
        return -1;
    }

    return 0;
}
