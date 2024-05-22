#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

static DaisySeed hw;
Oscillator osc;
uint8_t harmonic = 0;
float root_freq = 110.0;

static void AudioCallback(AudioHandle::InterleavingInputBuffer in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t size) {
  float sig, freq, amp;
  // size_t wave = 1;

  for (size_t i = 0; i < size; i++) {
    // Read Knobs
    freq = root_freq * (harmonic + 1);
    amp = 0.1;
    // Set osc params
    osc.SetFreq(freq);
    // osc.SetWaveform(wave);
    osc.SetAmp(amp);
    //.Process
    sig = osc.Process() * 0.1;
    // Assign Synthesized Waveform to all four outputs.
    for (size_t chn = 0; chn < 2; chn++) {
      out[chn + i * 2] = sig;
    }
  }
}

int main(void) {
  // Init the seed and so on...
  // initialize seed hardware and daisysp modules
  float sample_rate;
  hw.Configure();
  hw.Init();
  hw.SetAudioBlockSize(4);
  sample_rate = hw.AudioSampleRate();

  // set parameters for sine oscillator object
  osc.Init(sample_rate);
  osc.SetWaveform(Oscillator::WAVE_SIN);
  osc.SetFreq(100);
  osc.SetAmp(0.25);

  // SpiHandle object and Spi Configuration object
  SpiHandle spi_handle;
  SpiHandle::Config spi_conf;

  // Set some configurations
  spi_conf.periph = SpiHandle::Config::Peripheral::SPI_1;
  spi_conf.mode = SpiHandle::Config::Mode::SLAVE;
  spi_conf.direction = SpiHandle::Config::Direction::TWO_LINES_RX_ONLY;
  spi_conf.nss = SpiHandle::Config::NSS::HARD_INPUT;

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

  // infinite loop
  while (1) {
    // Blocking read of 4 bytes from SPI
    uint8_t buffer[4];
    spi_handle.BlockingReceive(buffer, 1, 10000);
    harmonic = buffer[0];
  }
}