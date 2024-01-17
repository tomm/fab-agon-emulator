#ifndef __UPDATER_H
#define __UPDATER_H

#include <stdint.h>

static constexpr uint8_t unlockCode[] = "unlock";
static constexpr uint8_t unlock_response[] = "unlocked! ";

class vdu_updater 
{
    void unlock ();
    void receiveFirmware();
    void switchFirmware();
    void test ();

    public:
        bool locked;
    
        vdu_updater();
        void command (uint8_t mode);
        uint8_t get_unlock_response (uint8_t index);
};

#endif //__UPDATER_H