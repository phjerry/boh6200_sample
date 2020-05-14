Description
    sample_pmoc is used to demonstrate how to use the interface related to standby.

Command format
    > usage: sample_pmoc [-t trigger] [-s set] [-g get] [[command] arg].
    > command as follows:
    > sample_pmoc -t                  -- enter standby.
    > sample_pmoc -s wakeup_attr      -- configure wakeup params.
    > sample_pmoc -s display          -- configure keyled display params.
    > sample_pmoc -s power_off_gpio   -- configure power off gpio params.
    > sample_pmoc -g wakeup_attr      -- get wakeup params.

Notice: 1.In ethernet fowrward scene, make sure power up two ethernet ports
          before enter standby status.