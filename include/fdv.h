

#ifndef _FDV_H_
#define _FDV_H_

// disable macros like "read"
#ifndef LWIP_COMPAT_SOCKETS
#define LWIP_COMPAT_SOCKETS 0
#endif

extern "C" {
#include "esp_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

// used for data
#define FLASHMEM                                                               \
  __attribute__((aligned(4))) __attribute__((section(".irom.text")))
#define FLASHMEM2                                                              \
  __attribute__((aligned(4))) __attribute__((section(".irom2.text")))

// used for parameters
#define FSTR(s)                                                                \
  (__extension__({                                                             \
    static const char __c[] FLASHMEM2 = (s);                                   \
    &__c[0];                                                                   \
  }))

// used for functions
#define FUNC_FLASHMEM __attribute__((section(".irom0.text")))

// used for methods
#define MTD_FLASHMEM __attribute__((section(".irom1.text")))

// used for template methods
#define TMTD_FLASHMEM __attribute__((section(".irom4.text")))

// used for static methods
#define STC_FLASHMEM __attribute__((section(".irom3.text")))

#include "fdvconfig.h"
#include "fdvcommonstr.h"
#include "fdvprintf.h"
#include "fdvdebug.h"
#include "fdvsync.h"
#include "fdvtask.h"
#include "fdvflash.h"
#include "fdvutils.h"
#include "fdvstrings.h"
#include "fdvcollections.h"
#include "fdvgpio.h"
#include "fdvserial.h"
#include "fdvnetwork.h"
#include "fdvdatetime.h"
#include "fdvserialserv.h"
#include "fdvconfmanager.h"

#endif	