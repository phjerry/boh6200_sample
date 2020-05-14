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
    char *partion_name = FLASH_NAME;
    hi_u8 *p_wbuf = NULL;
    hi_handle fd;
    hi_s32 img_fd = -1;
    hi_s32 img_len;
    hi_s32 ret;
    hi_u64 part_size;

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
    if (fd == INVALID_FD) {
        printf("ret=%d: open %s failed!\n", fd, partion_name);
        return -1;
    }

    img_fd = -1;
    img_fd = open(image_file, O_RDONLY);
    if (img_fd < 0) {
        printf("ret=%d: open %s failed!\n", img_fd, image_file);
        goto img_error2;
    }

    img_len = lseek(img_fd, 0, SEEK_END);
    lseek(img_fd, 0, SEEK_SET);

    ret = hi_unf_flash_getinfo(fd, &info);
    if (ret != 0) {
        printf("ret=%d: get flash info failed!\n", ret);
        goto img_error1;
    }

    part_size = info.open_len;

    if (img_len > part_size) {
        printf("wirte size(0x%x) beyound flash partion size(0x%llx)!\n",
               img_len, part_size);
    }

    if (img_len % (info.page_size + info.oob_size)) {
        printf("wirte size(0x%x) should be aligned with "
               "pagesize+oobsize(0x%x))!\n",
               img_len, info.page_size + info.oob_size);
    }
    p_wbuf = (hi_u8 *) malloc(part_size);
    if (p_wbuf == NULL) {
        printf("p_wbuf malloc failed!\n");
        goto img_error1;
    }

    memset(p_wbuf, '\0', part_size);

    if ((ret = read(img_fd, p_wbuf, img_len)) != img_len) {
        printf("read yaffs fail!\n");
        goto img_error0;
    }

    ret = hi_unf_flash_write(fd, 0x0, p_wbuf, img_len,
                             HI_FLASH_RW_FLAG_WITH_OOB
                             | HI_FLASH_RW_FLAG_ERASE_FIRST);
    if (ret <= 0)
        printf("ret=%d: write yaffs failed!\n", ret);
    else
        printf("flash write yaffs OK!\n");

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
