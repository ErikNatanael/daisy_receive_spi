#include <WiFi.h>
#include <SPI.h>

// SPI
//static const int spiClk = 100000; // 0.1 MHz
//static const int spiClk = 1000000; // 1 MHz
//static const int spiClk = 25000000; // 25 MHz
//static const int spiClk = 12500000; // 12.5 MHz
//static const int spiClk = 3125000; // 3.125 MHz
static const int spiClk = 6250000;  // 6.25 MHz
//static const int spiClk = 9375000; // 9.375 MHz
#define HSPI_MISO 12
#define HSPI_MOSI 13
#define HSPI_CLK 14
#define HSPI_SS 15

SPIClass* hspi = NULL;

// WIFI credentials
const char* ssid = "NETGEAR35";
const char* password = "perfectrosebud521";

const uint16_t port = 8081;
const char* host = "192.168.1.6";
char* audio_buffer;
const size_t audio_buffer_max_size = 1000000;
size_t audio_buffer_size = 0;


void logMemory() {
  log_d("Used PSRAM: %d", ESP.getPsramSize() - ESP.getFreePsram());
}

void setup() {
  log_d("Total heap: %d", ESP.getHeapSize());
  log_d("Free heap: %d", ESP.getFreeHeap());
  log_d("Total PSRAM: %d", ESP.getPsramSize());
  log_d("Free PSRAM: %d", ESP.getFreePsram());

  Serial.begin(115200);

  logMemory();
  audio_buffer = (char*)ps_malloc(audio_buffer_max_size * sizeof(char));
  if (audio_buffer == NULL) {
    Serial.println("Failed to allocate memory");
  }
  logMemory();

  // Set up SPI

  pinMode(HSPI_SS, OUTPUT);

  hspi = new SPIClass(HSPI);

  hspi->begin(HSPI_CLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);
}

void loop() {
  // send_test_spi_u8();
  // send_test_spi_u8_return_plus_two();
  send_test_spi_u32_return_plus_two();
  delay(500);
}

inline uint32_t DecodeFixed32(const char* ptr) {
  const uint8_t* const buffer = reinterpret_cast<const uint8_t*>(ptr);

  // Recent clang and gcc optimize this to a single mov / ldr instruction.
  return (static_cast<uint32_t>(buffer[0])) | (static_cast<uint32_t>(buffer[1]) << 8) | (static_cast<uint32_t>(buffer[2]) << 16) | (static_cast<uint32_t>(buffer[3]) << 24);
}

union mix_t {
  std::uint32_t integer;
  std::uint8_t theBytes[4];
};

void send_test_spi_u8() {
  uint8_t recv[255];
  for (char i = 0; i < 255; i++) {
    char send = i;
    hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
    digitalWrite(HSPI_SS, LOW);
    recv[i] = hspi->transfer(send);
    digitalWrite(HSPI_SS, HIGH);
    hspi->endTransaction();
    // delay(1);
  }
  for (char i = 0; i < 255; i++) {
    if (recv[i] != 255 - i) {
      Serial.printf("i: %u, expected %u received %u\n", i, 255 - i, (uint8_t)recv[i]);
    } else {
      //Serial.printf("i: %u, recv %u\n", i, (uint8_t)recv[i]);
    }
  }
  Serial.printf("u8 SPI test complete!\n");
}

void send_test_spi_u8_return_plus_two() {
  char recv;
  digitalWrite(HSPI_SS, LOW);
  hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  recv = hspi->transfer(0);
  hspi->endTransaction();
  digitalWrite(HSPI_SS, HIGH);
  for (char i = 3; i < 250; i += 3) {
    // delay(10);
    digitalWrite(HSPI_SS, LOW);
    hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
    recv = hspi->transfer(i);
    hspi->endTransaction();
    digitalWrite(HSPI_SS, HIGH);
    //Serial.printf("i: %u, received %u\n", i, (uint8_t)recv);
    if (recv != i - 3 + 2) {
      Serial.printf("Wrong i: %u, expected %u received %u\n", i, i - 3 + 2, (uint8_t)recv);
      return;
    }
  }
  Serial.printf("u8 SPI test complete!\n");
}

void send_test_spi_u32_return_plus_two() {
  char recv[4];
  mix_t send;
  uint32_t received_number = spi_transfer_u32(0);
  // digitalWrite(HSPI_SS, LOW);
  //   hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  //     recv[0] = hspi->transfer(0);
  //     recv[1] = hspi->transfer(0);
  //     recv[2] = hspi->transfer(0);
  //     recv[3] = hspi->transfer(0);
  //   hspi->endTransaction();
  //   digitalWrite(HSPI_SS, HIGH);
  size_t stride = 17;
  for (size_t i = stride; i < 2000000; i += stride) {
    uint32_t rint = spi_transfer_u32(i);
    if (rint != i - stride + 2) {
      Serial.printf("Wrong i: %u, expected %u %04x received %u %04x (u8) %u %04x\n", i, i - stride + 2, i - stride + 2, rint, rint, (uint8_t)rint, (uint8_t)rint);
      return;
    }
  }
  Serial.printf("u32 SPI test complete!\n");
}

void send_audio_buffer() {
  spi_send_u32(0);  // Try to reset the connection
  mix_t buffer_size;
  buffer_size.integer = audio_buffer_size;
  // mix_t buffer_size_ack;

  // Send buffer size
  Serial.println("Sending buffer size");

  spi_send_u32(audio_buffer_size);

  uint32_t buffer_size_ack = spi_receive_u32();
  if (buffer_size.integer != buffer_size_ack) {
    Serial.printf("Received buffer size does not match: %u, %04x\n", buffer_size_ack, buffer_size_ack);
    char abort[4] = { 255, 255, 255, 255 };
    spi_send_buffer(abort, 4);
    return;
  }
  Serial.println("Sending GTG");
  // Send good to go
  // hspi->transfer(0); hspi->transfer(1); hspi->transfer(2); hspi->transfer(3);
  char gtg[4] = { 0, 1, 2, 3 };
  spi_send_buffer(gtg, 4);

  Serial.println("Sending buffer");
  // Send buffer
  spi_send_buffer(audio_buffer, audio_buffer_size);

  // digitalWrite(HSPI_SS, HIGH);
  // hspi->endTransaction();
}
int delay_time = 1;
int delay_us = 3;
void spi_send_buffer(char* output, size_t num_bytes) {
  hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(HSPI_SS, LOW);
  for (size_t i = 0; i < num_bytes; i++) {
    hspi->transfer(output[i]);
  }
  digitalWrite(HSPI_SS, HIGH);
  hspi->endTransaction();
  delay(delay_time);
}
void spi_send_u32(uint32_t output) {
  mix_t b;
  uint32_t val;
  b.integer = output;
  digitalWrite(HSPI_SS, LOW);
  delay(10);
  hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  val = hspi->transfer(b.theBytes[0]);
  val |= (uint32_t)hspi->transfer(b.theBytes[1]) << 8;
  val |= (uint32_t)hspi->transfer(b.theBytes[2]) << 16;
  val |= (uint32_t)hspi->transfer(b.theBytes[3]) << 24;
  hspi->endTransaction();
  digitalWrite(HSPI_SS, HIGH);
  //Serial.printf("sent: %u, %04x | rws: %u, %04x\n", output, output, val, val);
  delay(delay_time);
}
uint32_t spi_transfer_u32(uint32_t output) {
  mix_t b;
  uint32_t val;
  b.integer = output;
  digitalWrite(HSPI_SS, LOW);
  hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  val = hspi->transfer(b.theBytes[0]);
  val |= (uint32_t)hspi->transfer(b.theBytes[1]) << 8;
  val |= (uint32_t)hspi->transfer(b.theBytes[2]) << 16;
  val |= (uint32_t)hspi->transfer(b.theBytes[3]) << 24;
  hspi->endTransaction();
  digitalWrite(HSPI_SS, HIGH);
  //Serial.printf("sent: %u, %04x | rws: %u, %04x\n", output, output, val, val);
  ets_delay_us(delay_us);
  return val;
}
void spi_receive_buffer(char* input, size_t num_bytes) {
  digitalWrite(HSPI_SS, LOW);
  delay(10);
  hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  for (size_t i = 0; i < num_bytes; i++) {
    input[i] = hspi->transfer(1);
  }
  hspi->endTransaction();
  digitalWrite(HSPI_SS, HIGH);
  delay(delay_time);
}

uint32_t spi_receive_u32() {
  hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  uint32_t val;
  val = hspi->transfer(0);  //0xff is a dummy
  val |= (uint32_t)hspi->transfer(0) << 8;
  val |= (uint32_t)hspi->transfer(0) << 16;
  val |= (uint32_t)hspi->transfer(0xf) << 24;
  hspi->endTransaction();
  delay(delay_time);
  return val;
}
