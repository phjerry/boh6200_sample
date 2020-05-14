#MinGW adding .c and .h files
#run by using mingw32-make
CC		=	gcc

CFLAGS = -D ES1_FPGA
C_FILES = 	.\..\inv_host_drivers\inv478x\inv478x_hal\inv478x_hal.c -Werror \
			.\inv478x_main.c -Werror \
			.\..\inv_host_drivers\inv478x\inv478x.c -Werror \
			.\..\inv_host_drivers\common\cra\inv_drv_cra.c -Werror \
			.\..\inv_host_drivers\common\ipc\inv_drv_ipc.c -Werror \
			.\..\inv_host_drivers\common\isp\inv_drv_isp.c -Werror \
			.\..\inv_host_drivers\common\log\inv_sys_log.c -Werror \
			.\..\inv_host_drivers\common\malloc\inv_sys_malloc.c -Werror \
			.\..\inv_host_drivers\common\obj\inv_sys_obj.c -Werror \
			.\..\inv_host_drivers\common\time\inv_sys_time.c -Werror \
			.\..\inv_host_drivers\common\ipc_master\inv_drv_ipc_master.c -Werror \
			.\..\inv_host_drivers\platforms\pc\inv_platform_wrapper.c -Werror \
			.\..\inv_host_drivers\platforms\pc\inv_adapter.c -Werror


H_FILES =  	-I.\
			-I..\inv_host_drivers\inv478x\inv478x_hal   \
			-I..\inv_host_drivers\inv478x \
			-I..\inv_host_drivers\common\cra\
			-I..\inv_host_drivers\common\ipc  \
			-I..\inv_host_drivers\common\isp  \
			-I..\inv_host_drivers\common\log  \
			-I..\inv_host_drivers\common\malloc  \
			-I..\inv_host_drivers\common\obj  \
			-I..\inv_host_drivers\common\seq  \
			-I..\inv_host_drivers\common\time  \
			-I..\inv_host_drivers\internal \
			-I..\inv_host_drivers\platforms\pc\
			-I..\inv_host_drivers\platforms \
			-I..\inv_host_drivers\common\
			-I..\inv_host_drivers\common\ipc_master\
			-I..\inv_host_drivers\inv478x\comlib

all:
		$(CC) 	-o .\inv478x_app_fw_update.exe  $(CFLAGS) $(H_FILES) $(C_FILES)
		cmd /C if not exist bin mkdir bin
		cmd /C move .\inv478x_app_fw_update.exe bin
		cmd /C copy  .\..\inv_host_drivers\platforms\pc\inv_adapter.dll bin
		cmd /K copy	 .\..\inv_host_drivers\platforms\pc\aardvark.dll bin
clean:
		rd /s /q bin