/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: mpi layer interface declaration for hdmi tx
 * Author: hdmi sw team
 * Create: 2019-08-22
 */

#ifndef __HDMI_TEST_CMD_H__
#define __HDMI_TEST_CMD_H__

int hdmi_test_cmd(const char *cmd, unsigned char len);

#define HDMI_INPUT_CMD_MAX 128

#endif
