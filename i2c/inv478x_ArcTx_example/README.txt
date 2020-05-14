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
This host application is used to test e-ARC-Tx fallback to legacy ARC with support of CEC using onbaord SiI9612.

Procedure to build the host application:
Follow the below mentioned procedure to build the executable file "inv478x_ArcTx_example_app.exe".
1. Extract the inv478x_Host_App_1.14_XXXXX.zip folder provided in the release package.
2. Open inv478x_ArcTx_example folder present inside inv478x_Host_App_1.14_XXXXX folder.
3. Open command prompt in the same inv478x_ArcTx_example folder.
4. Type the command "mingw32-make" and press enter.
5. MinGw now builds the inv478x_ArcTx_example file. 
6. The inv478x_ArcTx_example_app.exe file can be obtained from the bin folder generated in same inv478x_ArcTx_example folder.

Procedure to execute the host application:
1. Connect INV478x Starter Kit with PC by using Aardvark/VCOM.
2. Connect JP27.1 to JP1.1
3. Change the switch setting SW8 as ON and keep all the jumpers/switches settings to default.
4. Run inv478x_ArcTx_example_app.exe
5. Select the port Aardvark/VCOM from the list by pressing the corresponding number from PC Keyboard.
6. The window displays some initial query result.
7. Using PC keyboard, press letter 's'(in lower case) to start CEC.
8. Press 'B' for CEC Rx.  
9. The example application returns a message "CEC Rx Create done". 
10.Execute the test case HFR5-1-52 for eARC fallback in compliance equipment.Observe that the eARC-Tx fallback test case is successful.  

