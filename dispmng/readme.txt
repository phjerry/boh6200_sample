sample_mono_display

Purpose:
    demonstrate the basic process of display manager.
Command:
    sample_mono_display
Parameter:
    none.
Example:
    ./sample_mono_display


sample_dual_display
Purpose:
    demonstrate dual screen display via display manager (dispmng).
Command:
    sample_dural_display
Parameters:
    none.
Example:
    ./sample_dual_dispaly
Attention:
    Before the demonstration, two TV sets need to be connected to the two HDMI ports of the STB.

sample_display_mode
Purpose:
    Demonstrate how to get/set display via display manager (dispmng).
    The sample supports folloing commands:
           help       to display this message
           info       to display info of sink device
           capa       to display capability of sink device
           status     to display status of interface
           modes      to display available display modes
           mode       to display current display mode
           enable     to enable attached interface
           disable    to disable attached interface
           offset [left] [top] [right] [bottom]  to set screen offset
           brightness [0~1023]    to set display brightness
           contrast   [0~1023]    to set display contrast
           hue        [0~1023]    to set display hue
           saturation [0~1023]    to set display saturation
           auto       to set display mode to auto mode
           format [format]   to set display fomat, such as 1080P60
           force  [format]   to force set display fomat
           rgb     to set pixel format to RGB
           444     to set pixel format to YUV444
           420     to set pixel format to YUV420
           422     to set pixel format to YUV422
           8       to set pixel width  to 8 bit
           10      to set pixel width  to 10 bit
           12      to set pixel width  to 12 bit
Command:
    sample_display_mode display_id
Parameters:
    display_id    0----Master display, 1----Slave display
Example:
    ./sample_dual_dispaly 0
Attention:
    Before the demonstration, the TV set must be connected to the corresponding HDMI port.

sample_tsplay_multiscreen
Purpose:
    demonstrate how to play ts media file on multi-screen via display manager (dispmng).
Command:
    sample_display_mode file|dir display_id [format]
Parameters:
    file|dir     the file or dir which locate the media file or media file list
    display_id   0----Master display, 1----Slave display
    format       4320P120|4320P60|2160P60|2160P50|1080P60|1080P50
Example:
    ./sample_dual_dispaly /mnt/sdr.ts 0 1080P60
Attention:
    Before the demonstration, the TV set must be connected to the corresponding HDMI port.