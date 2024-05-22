#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

static DaisySeed hw;
Oscillator osc;

// Calculate buffer length
static const uint32_t kBufferLengthSec = 2;
static const uint32_t kSampleRate = 44100;
static const size_t kBufferLenghtSamples = kBufferLengthSec * kSampleRate;
static float DSY_SDRAM_BSS buf0[kBufferLenghtSamples];
static uint8_t DSY_SDRAM_BSS receive_buffer[kBufferLenghtSamples];
size_t buf_reader_pointer = 0;
uint32_t current_audio_buffer_size = 10000;

float i16_reciprocal;

inline uint32_t DecodeFixed32(const char *ptr) {
  const uint8_t *const buffer = reinterpret_cast<const uint8_t *>(ptr);

  // Recent clang and gcc optimize this to a single mov / ldr instruction.
  return (static_cast<uint32_t>(buffer[0])) |
         (static_cast<uint32_t>(buffer[1]) << 8) |
         (static_cast<uint32_t>(buffer[2]) << 16) |
         (static_cast<uint32_t>(buffer[3]) << 24);
}

static void AudioCallback(AudioHandle::InterleavingInputBuffer in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t size) {
  float sig, freq, amp;
  // size_t wave = 1;

  for (size_t i = 0; i < size; i++) {
    sig = buf0[buf_reader_pointer];
    buf_reader_pointer++;
    if (buf_reader_pointer >= current_audio_buffer_size) {
      buf_reader_pointer = 0;
    }
    for (size_t chn = 0; chn < 2; chn++) {
      out[chn + i * 2] = sig;
    }
  }
}

union u8_to_i16 {
  std::int16_t integer;
  std::uint8_t theBytes[2];
};

union u8_to_u32 {
  std::uint32_t integer;
  std::uint8_t theBytes[4];
};

bool send_countdown_bytes(SpiHandle *spi_handle) {
  uint8_t rx_buf[1];
  uint8_t tx_buf[1];
  for (uint8_t i = 255; i > 0; i--) {
    tx_buf[0] = i;
    if (spi_handle->BlockingTransmitAndReceive(tx_buf, rx_buf, 1, 200) ==
        SpiHandle::Result::ERR) {
      return false;
    }
  }
  return true;
}

bool send_plus_two(SpiHandle *spi_handle, DaisySeed *hw) {
  uint8_t rx_buf[1];
  uint8_t tx_buf[1];
  tx_buf[0] = 2;
  while (1) {
    if (spi_handle->BlockingTransmitAndReceive(tx_buf, rx_buf, 1, 2000) ==
        SpiHandle::Result::ERR) {
      return false;
    }
    tx_buf[0] = rx_buf[0] + 2;
    // hw->PrintLine("Received %u returning %u", rx_buf[0], tx_buf[0]);
  }
  return true;
}

bool send_plus_two_u32(SpiHandle *spi_handle, DaisySeed *hw) {
  uint8_t rx_buf[4];
  u8_to_u32 tx_buf;
  tx_buf.integer = 2;
  while (1) {
    if (spi_handle->BlockingTransmitAndReceive(
            tx_buf.theBytes, rx_buf, 4, 2000) == SpiHandle::Result::ERR) {
      return false;
    }
    uint32_t received_number = DecodeFixed32((char *)rx_buf);
    tx_buf.integer = received_number + 2;
    // hw->PrintLine("Received %u returning %u", received_number,
    // tx_buf.integer);
  }
  return true;
}

int main(void) {
  i16_reciprocal = 1.0 / 32767.0;
  // Init the seed and so on...
  // initialize seed hardware and daisysp modules
  float sample_rate;
  hw.Configure();
  hw.Init();
  hw.SetAudioBlockSize(4);
  sample_rate = hw.AudioSampleRate();

  // Enable Logging, and set up the USB connection.
  // Setting true here means that the program will wait until
  // a connection has been made to a USB Host
  hw.StartLog(true);

  // SpiHandle object and Spi Configuration object
  SpiHandle spi_handle;
  SpiHandle::Config spi_conf;

  // Set some configurations
  spi_conf.periph = SpiHandle::Config::Peripheral::SPI_1;
  spi_conf.mode = SpiHandle::Config::Mode::SLAVE;
  spi_conf.direction = SpiHandle::Config::Direction::TWO_LINES;
  spi_conf.nss = SpiHandle::Config::NSS::HARD_INPUT;
  spi_conf.baud_prescaler = SpiHandle::Config::BaudPrescaler::PS_8;
  spi_conf.clock_polarity = SpiHandle::Config::ClockPolarity::LOW;
  spi_conf.clock_phase = SpiHandle::Config::ClockPhase::ONE_EDGE;

  // spi_conf.pin_config.sclk = Pin(PORTG, 11);
  // spi_conf.pin_config.miso =
  //     Pin(PORTB, 4); // won't use this, but here it is anyways
  // spi_conf.pin_config.mosi = Pin(PORTB, 5);
  // spi_conf.pin_config.nss = Pin(PORTG, 10);

  using namespace daisy::seed;
  spi_conf.pin_config.sclk = D8;
  spi_conf.pin_config.miso = D9;
  spi_conf.pin_config.mosi = D10;
  spi_conf.pin_config.nss = D7;
  // Initialize the handle using our configuration
  spi_handle.Init(spi_conf);

  // any final initialization...

  // start callback
  hw.StartAudio(AudioCallback);

  // // infinite loop
  // while (1) {
  //   // Blocking read of 4 bytes from SPI
  //   uint8_t buffer[4];
  //   uint8_t buffer2[4];
  //   // Receive size of next message
  //   hw.PrintLine("Waiting to receive");
  //   buffer2[0] = 0;
  //   buffer2[1] = 0;
  //   buffer2[2] = 0;
  //   buffer2[3] = 255;
  //   if (spi_handle.BlockingTransmitAndReceive(buffer2, buffer, 4, 200) ==
  //       SpiHandle::Result::ERR) {
  //     continue;
  //   }
  //   uint32_t new_buffer_size = DecodeFixed32((char *)buffer);
  //   // hw.PrintLine("new_buffer_size: %d, %04x", new_buffer_size,
  //   // new_buffer_size);
  //   if (new_buffer_size == 0 || new_buffer_size == 0xffffffff) {
  //     // What we received was not a buffer size, it was an abort
  //     hw.PrintLine("0");
  //     continue;
  //   }
  //   // spi_handle.BlockingTransmit(buffer, 4, 10000);
  //   if (spi_handle.BlockingTransmitAndReceive(buffer, buffer2, 4, 200) ==
  //       SpiHandle::Result::ERR) {
  //     continue;
  //   }
  //   uint32_t received_while_transmitting = DecodeFixed32((char *)buffer2);
  //   hw.PrintLine(
  //       "Sent buffer size back: %u, %04x | Received while sending: %u, %04x",
  //       new_buffer_size, new_buffer_size, received_while_transmitting,
  //       received_while_transmitting);
  //   if (spi_handle.BlockingReceive(buffer, 4, 200) == SpiHandle::Result::ERR)
  //   {
  //     continue;
  //   }
  //   uint32_t debug_rec = DecodeFixed32((char *)buffer);
  //   hw.PrintLine("received response: %u %u %u %u ie %04x", buffer[0],
  //   buffer[1],
  //                buffer[2], buffer[3], debug_rec);
  //   if (!(buffer[0] == 0 && buffer[1] == 1 && buffer[2] == 2 &&
  //         buffer[3] == 3)) {
  //     hw.PrintLine("Wrong response, starting over");
  //     // spi_handle.BlockingReceive(buffer, 4, 10000);
  //     // uint32_t debug_rec = DecodeFixed32((char *)buffer);
  //     // hw.PrintLine("extra received response: %d %d %d %d ie %04x",
  //     buffer[0],
  //     //              buffer[1], buffer[2], buffer[3], debug_rec);
  //     continue;
  //   }
  //   if (spi_handle.BlockingReceive(receive_buffer, new_buffer_size, 10000) ==
  //       SpiHandle::Result::ERR) {
  //     continue;
  //   }
  //   hw.PrintLine("Received buffer");
  //   u8_to_i16 convert;
  //   for (size_t i = 0; i < new_buffer_size; i += 2) {
  //     convert.theBytes[0] = receive_buffer[i];
  //     convert.theBytes[1] = receive_buffer[i + 1];
  //     buf0[i / 2] = float(convert.integer) * i16_reciprocal;
  //   }
  //   hw.PrintLine("Converted buffer");
  //   current_audio_buffer_size = new_buffer_size / 2;
  //   buf_reader_pointer = 0;
  // }

  while (1) {

    // if (!(send_countdown_bytes(&spi_handle))) {
    //   hw.PrintLine("Timeout");
    // } else {
    //   hw.PrintLine("Send test succeeded");
    // }
    // if (!(send_plus_two(&spi_handle, &hw))) {
    //   hw.PrintLine("SPI timeout");
    // } else {
    //   hw.PrintLine("Success");
    // }
    if (!(send_plus_two_u32(&spi_handle, &hw))) {
      hw.PrintLine("SPI timeout");
    } else {
      hw.PrintLine("Success");
    }
  }
}
