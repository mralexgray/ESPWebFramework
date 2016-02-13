
#include "fdv.h"

using namespace fdv;

extern "C" void FUNC_FLASHMEM user_init(void) {
  selectFlashBankSafe(0);

  // setup HTTP server with 2 threads and 512 bytes of stack each one
  ConfigurationManager::applyAll<TCPServer<DefaultHTTPHandler, 2, 512> >();
}
