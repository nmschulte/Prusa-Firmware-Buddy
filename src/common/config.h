//config.h - main configuration file
#pragma once

#include "printers.h"
#include "config_buddy_2209_02.h"

//--------------------------------------
//BUDDY_ENABLE_ETHERNET configuration
#ifdef BUDDY_ENABLE_WUI
    #define BUDDY_ENABLE_ETHERNET
#endif //BUDDY_ENABLE_WUI

//marlin api config
enum {
    MARLIN_MAX_CLIENTS = 5,    // maximum number of clients registered in same time
    MARLIN_MAX_REQUEST = 100,  // maximum request length in chars
    MARLIN_SERVER_QUEUE = 128, // size of marlin server input character queue (number of characters)
    MARLIN_CLIENT_QUEUE = 16,  // size of marlin client input message queue (number of messages)
};

// default string used as LAN hostname
#if ((PRINTER_TYPE == PRINTER_PRUSA_MK4))
    #define LAN_HOSTNAME_DEF "PrusaMK4"
#elif (PRINTER_TYPE == PRINTER_PRUSA_XL)
    #define LAN_HOSTNAME_DEF "PrusaXL"
#elif (PRINTER_TYPE == PRINTER_PRUSA_IXL)
    #define LAN_HOSTNAME_DEF "Prusa_iX"
#else
    #define LAN_HOSTNAME_DEF "PrusaMINI"
#endif

#if defined(_DEBUG)
    #define BUDDY_ENABLE_DFU_ENTRY
#endif

//display PSOD instead of BSOD
//#define PSOD_BSOD

//Enabled Z calibration (MK3, MK4, XL)
#if ((PRINTER_TYPE == PRINTER_PRUSA_MK4) || (PRINTER_TYPE == PRINTER_PRUSA_XL))
    #define WIZARD_Z_CALIBRATION
#endif

//CRC32 config - use hardware CRC32 with RTOS
#define CRC32_USE_HW
#define CRC32_USE_RTOS

//guiconfig.h included with config
#include <option/has_gui.h>
#if HAS_GUI()
    #include "../guiconfig/guiconfig.h"
#endif
