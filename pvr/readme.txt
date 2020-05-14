sample_pvr_play:
Purpose: sample_pvr_play to play the streams recorded by running the pvr_rec sample.
    The operations such as play, pause, fast forward, rewind, slow forward, see, step forward, etc are supported.

Command: sample_pvr_play file_name [vo_format]
      file_name: name of the file to be played, including the storage path
      The value vo_format is 1080P60, 1080P50, 1080i60, 1080i50, 720P60, or 720P50. The default value is 1080P60.

      When streams are being played, the command line interface is displayed. The following operations are supported:
         n: normal play
         p: pause
         f: fast forward(2x/4x/8x12x/16x/24x/32x/64x)
         s: slow forward(2x/4x/8x/16x/32x/64x)
         r: fast reward(2x/4x/8x/12x/16x/24x/32x/64x)
         t: step forward one frame
         k: seek to start
         e: seek to end
         l: seek forward 5 second
         j: seek reward 5 second
         x: destroy play channel and create again
         h: help
         q: quit

Note: The running of the commands depends on the font libraries in /sample/higo/res. To ensure that the font libraries are found, run the commands in the sample directory.

//////////////////////////////////////////////////////////////////////////////////
sample_pvr_rec:
Purpose: sample_pvr_rec to record streams by using the personal video recorder (PVR).
    The streams can be played during recording. Only one-channel program can be recorded, and the program list is searched automatically before recording.

Command: sample_pvr_rec file_path freq [SymbolRate] [qamtype or polarization] [vo_format]
      file_path: path of the recorded file. You do not need to enter the file name, because the file name is automatically generated based on the audio/video packet IDs (PIDs).
      Fields [SymbolRate], [qamtype or polarization] and [vo_format] are optional.
      SymbolRate:
          For cable, default 6875
          For satellite, default 27500
      qamtype or polarization:
          For cable, used as qamtype, value can be 16|32|[64]|128|256|512 defaut[64]
          For satellite, used as polarization, value can be [0] horizontal | 1 vertical defaut[0]
      vo_format:
          The value of vo_format can be 1080P60, 1080P50, 1080i60, 1080i50, 720P60, or 720P50. The default value is
          1080i50.

    After the preceding command is executed, programs are searched and the program list is displayed. Recording starts only after a played program is selected.

//////////////////////////////////////////////////////////////////////////////////
sample_pvr_rec_allts:
Purpose: sample_pvr_rec_allts To record all ts packets
    It is only used to record. Check whether the speed of storage devices meets requirements.

Usage: sample_pvr_rec_allts file_path freq [SymbolRate] [qamtype or polarization]
      File_path: Path of the recorded file. You do not need to enter the file name, because it is generated automatically based on the audio/video packet IDs (PIDs).
      Fields [SymbolRate], [qamtype or polarization] are optional.
      SymbolRate:
          For cable, default 6875
          For satellite, default 27500
      qamtype or polarization:
          For cable, used as qamtype, value can be 16|32|[64]|128|256|512 defaut[64]
          For satellite, used as polarization, value can be [0] horizontal | 1 vertical defaut[0]

//////////////////////////////////////////////////////////////////////////////////
sample_pvr_timeshift:
Purpose: time shift sample of the PVR.
    Switch between live broadcast and replay is supported. During replay, pausing, fast forwarding, rewinding, slow forwarding, seek, step forwarding, and resetting are supported.

Usage: sample_pvr_timeshift file_path freq [SymbolRate] [qamtype or polarization] [vo_format]
      file_path: Path of the replayed file. The file name is not required because it is generated automatically based on the audio and video pid.
      Fields [SymbolRate], [qamtype or polarization] and [vo_format] are optional.
      SymbolRate:
          For cable, default 6875
          For satellite, default 27500
      qamtype or polarization:
          For cable, used as qamtype, value can be 16|32|[64]|128|256|512 defaut[64]
          For satellite, used as polarization, value can be [0] horizontal | 1 vertical defaut[0]
      vo_format:
          The value of vo_format can be 1080P60, 1080P50, 1080i60, 1080i50, 720P60, or 720P50. The default value is
          1080i50.

      After time shift is started, the command line interface is displayed, on which the following operations are allowed.
         l: live play
         n: normal play
         p: pause/play
         f: fast forward (4x)
         s: slow forward (4x)
         r: fast rewind (4x)
         h: help
         q: quit

Note:
     To pause a live program, press p. To replay a program from the pausing place, also press p.
     To enter the live broadcast mode from the replay mode, press 1.
     To enter the normal play mode from the pause mode, press p or n. To enter the normal play mode from the trick play mode, press n. Do not press p, which only pauses the ongoing program.

//////////////////////////////////////////////////////////////////////////////////
sample_pvr_eth_timeshift:
Purpose: IPTV time shift sample of the PVR.
    Switch between live broadcast and replay is supported. During replay, pausing, fast forwarding, rewinding, slow forwarding, seek, step forwarding, and resetting are supported.

Usage: sample_pvr_timeshift file_path multiaddr udpport [vo_format]
      file_path: Path of the replayed file. The file name is not required because it is generated automatically based on the audio and video pid.
      multiaddr: Multicast address of stream sender.
      udpport: UDP port number
      vo_format:
          The value of vo_format can be 1080P60, 1080P50, 1080i60, 1080i50, 720P60, or 720P50. The default value is
          1080i50.

      After time shift is started, the command line interface is displayed, on which the following operations are allowed.
         l: live play
         n: normal play
         p: pause/play
         f: fast forward (4x)
         s: slow forward (4x)
         r: fast rewind (4x)
         h: help
         q: quit

Note:
     To pause a live program, press p. To replay a program from the pausing place, also press p.
     To enter the live broadcast mode from the replay mode, press 1.
     To enter the normal play mode from the pause mode, press p or n. To enter the normal play mode from the trick play mode, press n. Do not press p, which only pauses the ongoing program.

//////////////////////////////////////////////////////////////////////////////////
sample_pvr_file_ctrl:
Purpose: file linearization and file truncate of PVR.
    Support file linearization and file truncate;Support truncate head, tail, both head and tail, truncate asynchronous or synchronous, output to another file or on original file.

Usage: sample_pvr_file_ctrl in_file_name ctrl_type sync_flag head_offset, end_offset same_file_flag
      in_file_name: original file name
      ctrl_type: 0 - linearization; 1 - file truncate
      sync_flag(file truncate): 0 - asynchronous; 1 - synchronous
      head_offset(file truncate): head offset for file truncate
      end_offset(file truncate): end offset for file truncate
      same_file_flag(file truncate): 0 - output another file;1 - output the original file
