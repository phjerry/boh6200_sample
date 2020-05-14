Procedure

Usage:
./sample_flash_read start_position length  withOOB device_name(can be partition name or blank) flash_type
For example: ./sample_flash_read 0x2c00000 0x1000 1 /dev/mtd3
or
./sample_flash_read 0x0 0x1000 1 /dev/mmcblk0p3 2
or
./sample_flash_erase start_position length (device_name)
For example: ./sample_flash_erase 0x0 0x80000 (rootfs)
During erase, the start_position and the length must be block aligned.
or
./flash_write start_position length fill value 
For example: ./flash_write 0x80000 0x1000 0x20 /dev/mtd3
or
./sample_flash_write 0x80000 0x1000 0x20 /dev/mmcblk0p3 2
or 
./sample_flash_write_yaffs new_rootfs.yaffs

Note:
1)
write_yaffs can use two ways, one way is: calling hi_unf_flash_write only once(For example: flash_write_yaffs.c)
the other is: writing yaffs block by block(For example: flash_write_yaffs2.c)
2)
/*
 * Brief: Open flash partition by type and name
 * @flash_type:
 *     Flash type
 * @partition_name:
 *     partition name for spi/nand, use '/dev/mtdx' as partition name; use
 *     '/dev/mmcblk0px' as partion name for EMMC
 *     CNcomment: 非EMMC器件(如SPI/NAND),只能用/dev/mtdx作为分区名。EMMC器件只
 *         能用/dev/mmcblk0px作为分区名。
 * @return:
 *     Fd or HI_INVALID_HANDLE
 */
hi_handle hi_unf_flash_open(hi_flash_type flash_type,
                            const hi_char *partition_name);

3)
flash type			corresponding_number
SPI flash type				0
NAND flash type 			1
eMMC flash type 			2

Parameter "withOOB" has no effect on sample_flash_read,it can be either 0 or 1 for eMMC

The device name and access mode are modified in the corresponding test case. For example, in the partition shown below, the device name can be data or /dev/mtd3(not /dev/mtdblock3).
 DevName        TotalSize    BlockSize  PartName
 /dev/mtdblock3,0x100000,    0x20000,   data

