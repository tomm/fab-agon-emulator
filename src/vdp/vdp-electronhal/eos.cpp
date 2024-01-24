#include "hal.h"
#include "eos.h"

bool eos_running = false;

bool eos_wakeup ()
{
    // for 5 seconds
    // send ESC until we get CTRL-Z back
    for (int i=0;i<50;i++)
    {
        ez80_serial.write (ESC);
        if (ez80_serial.available() > 0)
        {
            // read character
            byte c = ez80_serial.read();
            if (c==CTRL_Z)
            {
                eos_running = true;
                return true;
            }
        }
        delay (100);
    }
    return false;
}