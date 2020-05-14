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
This host application contains external repeater functionality of INV478x.

Procedure to build the host application:
Follow the below mentioned procedure to build the executable file "inv478x_app_extr_rptr.exe" of external repeater host application.
1. Extract the inv478x_Host_App_1.14_XXXXX.zip folder provided in the release package.
2. Open inv478x_app_extr_rptr folder present inside inv478x_Host_App_1.14_XXXXX folder.
3. Open command prompt in the same inv478x_app_extr_rptr folder.
4. Type the command "mingw32-make" and press enter.
5. MinGw now builds the inv478x_app_extr_rptr.exe file. 
6. The inv478x_app_extr_rptr.exe file can be obtained from the bin folder generated in same inv478x_app_extr_rptr folder.

Prerequisite to be done in APIGUI tool before running the host application:
1. Set the av_link between the two INV478x Starter Kits as INV_HDMI_AV_LINK__TMDS2.
2. Set the hdcp_mode between the two INV478x Starter Kits as INV_HDCP_MODE_NONE.
3. Establish the video path from Rx of INV478x Starter Kit1 to Tx of INV478x Starter Kit2 as required for the HDCP repeater path.


Procedure to execute the host application:
1. Connect two INV478x Starter Kits with PC by using Aardvark.
2. Run inv478x_host_app.exe.
3. Select the combination of External-Repeater from the list by pressing the corresponding number/alphabet from PC Keyboard.
4. Select another repeater combination(optional) from the list by pressing the corresponding number/Alphabet, if secong combination is
not needed press any other key which is not in the list from PC Keyboard.
5. Select the Aardvark of INV478x Starter Kit1 and INV478x Starter Kit2 from the list by pressing the corresponding number from PC Keyboard.
6. The window displays chip_id query of both INV478x Starter Kits if Aardvarks are connected successfully.
7. From this point Host application will handle all the HDCP transactions in between INV478x Starter Kit1 and INV478x Starter Kit2