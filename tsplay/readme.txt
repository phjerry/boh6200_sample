Introduction: Sample of playing the of the local ts file.
Purpose: sample_tsplay file [vo_format]
      file: TS file path
      vo_format: video output mode in high definition. Value range: 1080P60|1080P50|1080i60|[1080i50]|720P60|720P50. Default value: 1080P60
                The standard definition mode is set automatically inside based on the high definition mode.
      After the program is started, the first program in the ts file is played automatically. Programs can be changed by pressing number keys.
 
Introduction: Sample of playing the of the local multiaud ts file.
Purpose: sample_tsplay_multiaud file [vo_format]
      file: TS file path
      vo_format: video output mode in high definition. Value range: 1080P60|1080P50|1080i60|[1080i50]|720P60|720P50. Default value: 1080P60
                The standard definition mode is set automatically inside based on the high definition mode.
      After the program is started, the first program in the ts file is played automatically. Audio track can be changed by pressing 'c' keys.

Introduction: Sample of playing the of the local specify PIDs ts file.
Purpose: sample_tsplay_multiaud file [vo_format]
      file: TS file path
      vo_format: video output mode in high definition. Value range: 1080P60|1080P50|1080i60|[1080i50]|720P60|720P50. Default value: 1080P60
                The standard definition mode is set automatically inside based on the high definition mode.
      vpid: video pid
      vtype: video encoding type(h264/mpeg2/mpeg4/avs/real8/real9/vc1ap/vc1smp5(WMV3)/vc1smp8/vp6/vp6f/vp6a/vp8/divx3/h263/sors)
      apid: audio pid
      atype: audio encoding type(aac/mp3/dts/dra/mlp/pcm/ddp)
Attention:	if only play audio£¬vfile and vtype should be null£¬if only play video, afile and atype should be null
