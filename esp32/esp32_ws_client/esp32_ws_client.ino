#include <SocketIOclient.h>
#include <WebSockets.h>
#include <WebSocketsClient.h>
#include <WebSocketsVersion.h>

#include <SPI.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <WebSockets.h>
#include <WebSocketsClient.h>

const char* ssid = "NETGEAR35";   
const char* password = "perfectrosebud521"; 

// Websocket host
char path[] = "";
char host[] = "192.168.1.6";

WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
#define USE_SERIAL Serial



void hexdump(const void *mem, uint32_t len, uint8_t cols = 16) {
	const uint8_t* src = (const uint8_t*) mem;
	USE_SERIAL.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);
	for(uint32_t i = 0; i < len; i++) {
		if(i % cols == 0) {
			USE_SERIAL.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
		}
		USE_SERIAL.printf("%02X ", *src);
		src++;
	}
	USE_SERIAL.printf("\n");
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

	switch(type) {
		case WStype_DISCONNECTED:
			USE_SERIAL.printf("[WSc] Disconnected!\n");
			break;
		case WStype_CONNECTED:
			USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);

			// send message to server when Connected
			webSocket.sendTXT("Connected");
			break;
		case WStype_TEXT:
			USE_SERIAL.printf("[WSc] get text: %s\n", payload);

			// send message to server
			// webSocket.sendTXT("message here");
			break;
		case WStype_BIN:
			USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
			hexdump(payload, length);

			// send data to server
			// webSocket.sendBIN(payload, length);
			break;
		case WStype_ERROR:			
		case WStype_FRAGMENT_TEXT_START:
		case WStype_FRAGMENT_BIN_START:
		case WStype_FRAGMENT:
		case WStype_FRAGMENT_FIN:
			break;
	}

}

static const int spiClk = 1000000; // 1 MHz
#define HSPI_MISO   12
#define HSPI_MOSI   13
#define HSPI_CLK    14
#define HSPI_SS     15

SPIClass * hspi = NULL;

byte number = 0;
int ostinato_index =  0;
uint8_t ostinato[16] = {0, 1, 2, 4, 3, 5, 4, 3, 2, 3, 1, 6, 4, 3, 2, 1};

void setup() {
  // USE_SERIAL.begin(921600);
	USE_SERIAL.begin(115200);

	//Serial.setDebugOutput(true);
	USE_SERIAL.setDebugOutput(false);

	USE_SERIAL.println();
	USE_SERIAL.println();
	USE_SERIAL.println();

	for(uint8_t t = 4; t > 0; t--) {
		USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
		USE_SERIAL.flush();
		delay(1000);
	}

	WiFiMulti.addAP(ssid, password);

	//WiFi.disconnect();
	while(WiFiMulti.run() != WL_CONNECTED) {
		delay(100);
	}
  USE_SERIAL.println("WiFi connected");
  USE_SERIAL.print("IP: ");
  USE_SERIAL.print(WiFi.localIP());
  USE_SERIAL.println();



	// server address, port and URL
	webSocket.begin(host, 8080, path);

	// event handler
	webSocket.onEvent(webSocketEvent);

	// use HTTP Basic Authorization this is optional remove if not needed
	// webSocket.setAuthorization("user", "Password");

	// try ever 5000 again if connection has failed
	webSocket.setReconnectInterval(5000);

  // SPI
  pinMode(HSPI_SS, OUTPUT);

  hspi = new SPIClass(HSPI);

  hspi->begin(HSPI_CLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);
}

void loop() {
  // hspiCommand();
  // delay(100);
  webSocket.loop();
}

void hspiCommand() {
  byte stuff = 0b11001100;
  
  hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(HSPI_SS, LOW);
  hspi->transfer(ostinato[ostinato_index]);
  digitalWrite(HSPI_SS, HIGH);
  hspi->endTransaction();

  ostinato_index = (ostinato_index+1)%16;
  size_t rand_index = esp_random() % 16;
  ostinato[rand_index] = esp_random() % 16;
}