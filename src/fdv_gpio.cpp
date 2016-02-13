

#include "fdv.h"

extern "C" {
#include "esp8266/pin_mux_register.h"
#include "esp8266/eagle_soc.h"
}

namespace fdv {

////////////////////
////////////////////
// GPIO

// gpioNum : 0..15
MTD_FLASHMEM GPIO::GPIO(uint32_t gpioNum) : m_gpioNum(gpioNum) { init(); }

void MTD_FLASHMEM GPIO::init() {
  static uint32_t pinReg[16] = {
      PERIPHS_IO_MUX_GPIO0_U,    PERIPHS_IO_MUX_U0TXD_U,
      PERIPHS_IO_MUX_GPIO2_U,    PERIPHS_IO_MUX_U0RXD_U,
      PERIPHS_IO_MUX_GPIO4_U,    PERIPHS_IO_MUX_GPIO5_U,
      PERIPHS_IO_MUX_SD_CLK_U,   PERIPHS_IO_MUX_SD_DATA0_U,
      PERIPHS_IO_MUX_SD_DATA1_U, PERIPHS_IO_MUX_SD_DATA2_U,
      PERIPHS_IO_MUX_SD_DATA3_U, PERIPHS_IO_MUX_SD_CMD_U,
      PERIPHS_IO_MUX_MTDI_U,     PERIPHS_IO_MUX_MTCK_U,
      PERIPHS_IO_MUX_MTMS_U,     PERIPHS_IO_MUX_MTDO_U};
  static uint8_t pinFunc[16] = {
      FUNC_GPIO0,  FUNC_GPIO1,  FUNC_GPIO2,  FUNC_GPIO3,
      FUNC_GPIO4,  FUNC_GPIO5,  FUNC_GPIO6,  FUNC_GPIO7,
      FUNC_GPIO8,  FUNC_GPIO9,  FUNC_GPIO10, FUNC_GPIO11,
      FUNC_GPIO12, FUNC_GPIO13, FUNC_GPIO14, FUNC_GPIO15};
  m_pinReg = pinReg[m_gpioNum];
  m_pinFunc = pinFunc[m_gpioNum];
}

void MTD_FLASHMEM GPIO::setupAsGPIO() {
  PIN_FUNC_SELECT(m_pinReg, m_pinFunc);
  enablePullUp(false);
}

void MTD_FLASHMEM GPIO::modeInput() {
  setupAsGPIO();
  GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 0);
  GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 0);
  GPIO_REG_WRITE(GPIO_ENABLE_W1TS_ADDRESS, 0);
  GPIO_REG_WRITE(GPIO_ENABLE_W1TC_ADDRESS, 1 << m_gpioNum);
}

void MTD_FLASHMEM GPIO::modeOutput() {
  setupAsGPIO();
  GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 0);
  GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 0);
  GPIO_REG_WRITE(GPIO_ENABLE_W1TS_ADDRESS, 1 << m_gpioNum);
  GPIO_REG_WRITE(GPIO_ENABLE_W1TC_ADDRESS, 0);
}

void MTD_FLASHMEM GPIO::enablePullUp(bool value) {
  if (value)
    PIN_PULLUP_EN(m_pinReg);
  else
    PIN_PULLUP_DIS(m_pinReg);
}
}
