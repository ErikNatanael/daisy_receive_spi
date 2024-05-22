#include <WiFi.h>
#include <SPI.h>

// SPI
//static const int spiClk = 1000000; // 1 MHz
//static const int spiClk = 25000000; // 25 MHz
//static const int spiClk = 12500000; // 12.5 MHz
static const int spiClk = 3125000; // 3.125 MHz
#define HSPI_MISO   12
#define HSPI_MOSI   13
#define HSPI_CLK    14
#define HSPI_SS     15

SPIClass * hspi = NULL;

// WIFI credentials
const char* ssid = "NETGEAR35";
const char* password =  "perfectrosebud521";
 
const uint16_t port = 8081;
const char * host = "192.168.1.6";
char* audio_buffer;
const size_t audio_buffer_max_size = 1000000;
size_t audio_buffer_size = 0;


void logMemory() {
  log_d("Used PSRAM: %d", ESP.getPsramSize() - ESP.getFreePsram());
}

void setup()
{
  log_d("Total heap: %d", ESP.getHeapSize());
  log_d("Free heap: %d", ESP.getFreeHeap());
  log_d("Total PSRAM: %d", ESP.getPsramSize());
  log_d("Free PSRAM: %d", ESP.getFreePsram());
  
  Serial.begin(115200);
 
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("...");
  }
 
  Serial.print("WiFi connected with IP: ");
  Serial.println(WiFi.localIP());
  logMemory();
  audio_buffer = (char*)ps_malloc(audio_buffer_max_size * sizeof(char));
  if(audio_buffer == NULL) {
    Serial.println("Failed to allocate memory");
  }
    logMemory();

  // Set up SPI

  pinMode(HSPI_SS, OUTPUT);

  hspi = new SPIClass(HSPI);

  hspi->begin(HSPI_CLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);
 
}
 
void loop()
{
    WiFiClient client;
 
    if (!client.connect(host, port)) {
 
        Serial.println("Connection to host failed");
 
        delay(1000);
        return;
    }
 
    Serial.println("Connected to server successful!");
 
    client.print("Hello from ESP32!");
    while(true) {
      String reply = client.readStringUntil('\n');
      Serial.println("Received: ");
      Serial.println(reply);
      if(reply == "audio") {
        // Receive length as u32 (4 bytes)
        uint32_t bytes_to_read = 0;
        char temp_buf[4];
        client.readBytes(temp_buf, 4);
        bytes_to_read = DecodeFixed32(temp_buf);
        Serial.printf("bytes_to_read: %d 0x%04x\n", bytes_to_read, bytes_to_read);
        audio_buffer_size = client.readBytes(audio_buffer, bytes_to_read);
        send_audio_buffer();
      }
    }
 
    Serial.println("Disconnecting...");
    client.stop();
 
    delay(10000);
}

inline uint32_t DecodeFixed32(const char* ptr) {
  const uint8_t* const buffer = reinterpret_cast<const uint8_t*>(ptr);

  // Recent clang and gcc optimize this to a single mov / ldr instruction.
  return (static_cast<uint32_t>(buffer[0])) |
         (static_cast<uint32_t>(buffer[1]) << 8) |
         (static_cast<uint32_t>(buffer[2]) << 16) |
         (static_cast<uint32_t>(buffer[3]) << 24);
}

union mix_t
{
    std::uint32_t integer;
    std::uint8_t theBytes[4];
};


void send_audio_buffer() {
  spi_send_u32(0); // Try to reset the connection
  mix_t buffer_size;
  buffer_size.integer = audio_buffer_size;
  // mix_t buffer_size_ack;

  // Send buffer size
  Serial.println("Sending buffer size");

  spi_send_u32(audio_buffer_size);

  uint32_t buffer_size_ack = spi_receive_u32();
  if (buffer_size.integer != buffer_size_ack) {
    Serial.printf("Received buffer size does not match: %u, %04x\n", buffer_size_ack, buffer_size_ack);
    char abort[4] = {255, 255, 255, 255};
    spi_send_buffer(abort, 4);
    return;
  }
  Serial.println("Sending GTG");
  // Send good to go
  // hspi->transfer(0); hspi->transfer(1); hspi->transfer(2); hspi->transfer(3);
  char gtg[4] = {0, 1, 2, 3};
  spi_send_buffer(gtg, 4);

  Serial.println("Sending buffer");
  // Send buffer
  spi_send_buffer(audio_buffer, audio_buffer_size);

  // digitalWrite(HSPI_SS, HIGH);
  // hspi->endTransaction();
}
int delay_time = 50;
void spi_send_buffer(char* output, size_t num_bytes) {
  hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(HSPI_SS, LOW);
  for(size_t i = 0; i < num_bytes; i++) {
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
  Serial.printf("sent: %u, %04x | rws: %u, %04x\n", output, output, val, val);
  delay(delay_time);
}
void spi_receive_buffer(char* input, size_t num_bytes) {
  digitalWrite(HSPI_SS, LOW);
  delay(10);
  hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  for(size_t i = 0; i < num_bytes; i++) {
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
