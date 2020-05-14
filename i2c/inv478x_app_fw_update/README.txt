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

About:
This host application is used to erase and program the INV478x firmware binary to SPI flash.
It is mandatory to erase the chip first before programming.

Procedure to build the host application:
Follow the below mentioned procedure to build the executable file "inv478x_app_fw_update.exe" of  firmware update host application.
1. Extract the inv478x_Host_App_1.14_XXXXX.zip folder provided in the release package.
2. Open inv478x_app_fw_update folder present inside inv478x_Host_App_1.14_XXXXX folder.
3. Open command prompt in the same inv478x_app_fw_update folder.
4. Type the command "mingw32-make" and press enter.
5. MinGw now builds the inv478x_app_fw_update.exe file. 
6. The inv478x_app_fw_update.exe file can be obtained from the bin folder generated in same inv478x_app_fw_update folder.

Procedure to execute the host application:
1. Connect INV478x Starter Kit with PC by using Aardvark/Mini-USB cable.
2. Run inv478x_host_app.exe.
3. Select the port Aardvark/VCOM from the list by pressing the corresponding number from PC Keyboard.
4. The window displays some initial query result.
5. Using PC keyboard, press letter 'f'(in lower case) to select APIs for flash program.
6. Press the letter 'a'(in lower case) to select the API "Flash Erase".
7. Wait for fraction of seconds while the application erase INV478x chip, observe that the host application returns a message "Erase Done" and heartbeet LED of INV478x goes off.
8. Using PC keyboard, again press letter 'f'(in lower case) to select APIs for flash program.
9. Now press the letter 'b'(in lower case) to select the API "Flash Program".
10. Enter the complete path of "inv478x_swxxx_xxxxx.bin" file, for example- "C:\INV478x_091_package\inv478x_sw091_45232.bin" and press 'Enter' key.
11. Wait for fraction of seconds while the application program INV478x chip, observe that the host application returns a message "checksum pass" and "Programming Done". The heartbeet LED of INV478x starts blinking again.
12. Press the letter 'q'(in lower case) to terminate the host application.
