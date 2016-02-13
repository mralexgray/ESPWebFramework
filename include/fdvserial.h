

#ifndef _FDVSERIAL_H_
#define _FDVSERIAL_H_

#include "fdv.h"

extern "C" {
#include <stdarg.h>
}

namespace fdv {

void DisableStdOut();

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// Serial

class Serial {

public:
  virtual void put(uint8_t value) = 0;
  virtual void write(uint8_t b) = 0;
  virtual int16_t peek() = 0;
  virtual int16_t read(uint32_t timeOutMs = 0) = 0;
  virtual uint16_t available() = 0;
  virtual void flush() = 0;
  virtual bool waitForData(uint32_t timeOutMs = portMAX_DELAY) = 0;

  uint16_t read(void *buffer, uint16_t bufferLen, uint32_t timeOutMs = 0);
  bool readLine(bool echo, LinkedCharChunks *receivedLine,
                uint32_t timeOutMs = portMAX_DELAY);

  void writeNewLine();
  void write(uint8_t const *buffer, uint16_t bufferLen);
  void write(char const *str);
  void writeln(char const *str);
  uint16_t printf(char const *fmt, ...);
};

// Serial
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
// HardwareSerial

// only UART0 is supported
class HardwareSerial : public Serial {
public:
  explicit HardwareSerial(uint32_t baud_rate = 115200,
                          uint32_t rxBufferLength = 128)
      : m_queue(rxBufferLength) {
    reconfig(baud_rate);
  }

  void reconfig(uint32_t baud_rate);

  using Serial::write;
  void write(uint8_t b);

  static HardwareSerial *getSerial(uint32_t uart);

  // call only from ISR
  void put(uint8_t value);

  int16_t peek();
  int16_t read(uint32_t timeOutMs = 0);
  uint16_t available();
  void MTD_FLASHMEM flush();
  bool MTD_FLASHMEM waitForData(uint32_t timeOutMs = portMAX_DELAY);

private:
  Queue<uint8_t> m_queue;

  static HardwareSerial *s_serials[1]; // only one serial is supported
};

// HardwareSerial
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
}

#endif