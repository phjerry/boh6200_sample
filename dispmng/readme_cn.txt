sample_mono_display
功能描述：单显示器模式的Display Manager(DISOMNG)功能演示
命令格式：sample_mono_display
参数说明：无
命令参考：sample_mono_display
注意事项：在演示之前，需要将电视机连接到机顶盒的HDMI1端口上。

sample_dual_display
功能描述：双屏显示器展示机顶盒的双屏显示功能
命令格式：sample_dual_display
参数说明：无
命令参考：sample_dual_display
注意事项：在演示之前，需要将两台电视机连接到机顶盒的两个HDMI端口上。

sample_display_mode
功能描述:演示显示模式设置相关功能，支持如下命令行：
           help       帮助信息
           info       展示电视制造商信息
           capa       展示电视能力信息
           status     展示电视机状态信息
           modes      展示电视机支持显示模式列表
           mode       展示当前的显示模式
           enable     使能当前的显示接口
           disable    去使能当前的显示接口
           offset [left] [top] [right] [bottom]  设置显示的偏移
           brightness [0~1023]    设置亮度
           contrast   [0~1023]    设置对比度
           hue        [0~1023]    设置色调
           saturation [0~1023]    设置饱和度
           auto       将显示模式设置为自动模式
           format [format]   设置显示模式（正常模式，会检查电视机是否支持）
           force  [format]   设置显示模式（强制模式，不检查电视机是否支持）
           rgb     设置输出pixel format为RGB
           444     设置输出pixel format为YUV444
           420     设置输出pixel format为YUV420
           422     设置输出pixel format为YUV422
           8       设置输出bit width为8比特
           10      设置输出bit width为10比特
           12      设置输出bit width为12比特
命令格式:
    sample_display_mode display_id
参数说明:
    display_id    显示通道ID，0----主屏显示通道, 1----辅屏显示通道
命令参考:
    ./sample_dual_dispaly 0
注意事项:
    在演示之前，需要将电视机连接到相应的HDMI接口。

sample_tsplay_multiscreen
功能描述:
    演示在不同屏之上的视频的播放
命令格式:
    sample_display_mode file|dir display_id [format]
参数说明:
    file|dir     待播放ts媒体文件或目录
    display_id   显示通道ID，0----主屏显示通道, 1----辅屏显示通道
    format       显示制式，如4320P120|4320P60|2160P60|2160P50|1080P60|1080P50
命令参考:
    ./sample_dual_dispaly /mnt/sdr.ts 0 1080P60
注意事项:
    在演示之前，需要将电视机连接到相应的HDMI接口。