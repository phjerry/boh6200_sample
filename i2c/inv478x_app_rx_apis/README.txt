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
This host application contains all the APIs related to HDMI Rx ports of INV478x. Please refer the API document for all supported APIs of INV478x.

Procedure to build the host application:
Follow the below mentioned procedure to build the executable file "inv478x_app_rx_apis.exe" of Rx APIs host application.
1. Extract the inv478x_Host_App_1.14_XXXXX.zip folder provided in the release package.
2. Open inv478x_app_rx_apis folder present inside inv478x_Host_App_1.14_XXXXX folder.
3. Open command prompt in the same inv478x_app_rx_apis folder.
4. Type the command "mingw32-make" and press enter.
5. MinGw now builds the inv478x_app_rx_apis.exe file. 
6. The inv478x_app_rx_apis.exe file can be obtained from the bin folder generated in same inv478x_app_rx_apis folder.

Procedure to execute the host application:
1. Connect INV478x Starter Kit with PC by using Aardvark/VCOM.
2. Run inv478x_host_app.exe.
3. Select the port Aardvark/VCOM from the list by pressing the corresponding number from PC Keyboard.
4. The window displays some initial query result.
5. Using PC keyboard, press letter 's'(in lower case) to select APIs for HDMI-Rx.
6. It gives the following options-
   ***********************************
   1. User Notification
   2. Receiver Common
   3. Receiver
   ***********************************
7. Enter the corresponding number for selecting different sub category of HDMI-Rx APIs.
8. Let us consider an example of the API "Rx Plus5V Query" for demonstration.
9. Enter number '3' (after step 6) to check Receiver APIs.
10. It again gives list of all the APIs under Receiver category.
11. Press the letter 'a'(in lower case) to select the API "Rx Plus5V Query".
12. It asks to choose the Rx port for which this query is to be raised.
13. Pres number '0' or '1' for Rx0 or Rx1 port respectively and press 'Enter' key.
14. The API "Rx Plus5V Query" returs '1' if cable is connected and returns '0' if no cable is connected on respective
Rx port.
15. Similarly follow the menu options for testing other HDMI-Rx APIs.
15. Press the letter 'q'(in lower case) to exit the host application.






