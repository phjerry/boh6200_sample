Introduction: Demonstration of switch between playing local ES files and TS files.
Purpose: sample_es_ts_switch esvidfile vidtype tsfile [vo_format]
        esvidfile: video ES file path
        vidtype:   the video encoding format.
        tsfile:    TS file path
        vo_format: video output mode in high definition. Value range: 1080P60|1080P50|1080i60|[1080i50]|720P60|720P50. Default value: 1080p50
                The standard definition mode is set automatically inside based on the high definition mode.
       After the program is started, input e to play esvidfile, t to play tsfile.
       While the program is playing tsfile, programs can be changed by pressing number keys.
