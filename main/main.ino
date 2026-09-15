#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <bitset>

#define CC1101_GDO0 0
#define CC1101_CSN  1
#define CC1101_SCK  2
#define CC1101_MOSI 3
#define CC1101_MISO 4
// #define CC1101_GD02 5


// microseconds (us)
#define MIN_PULSE_US 150
#define FRAME_GAP_US 4000   // silence after 4000 us is a gap
#define MIN_PULSES 24       // ignore frarmes shorter than 24 MIN_PULSES
#define MAX_PULSES 256       


volatile uint32_t rxFrame[MAX_PULSES];    // populate with pulse timings
volatile uint32_t lastRssiChangeTime = 0;
volatile bool rxFrameReady = false;
// change might not define a frame, but a frame must have changes
volatile uint32_t numPulsesThisFrame = 0;
volatile uint32_t numPulsesThisChange =  0;


void IRAM_ATTR onRssiChange() {
  uint32_t now = micros();
  uint32_t delta = now - lastRssiChangeTime;
  lastRssiChangeTime = now;

  if (rxFrameReady) return;

  if (delta > FRAME_GAP_US) {
    if (numPulsesThisChange > MIN_PULSES) {
      numPulsesThisFrame = numPulsesThisChange;
      rxFrameReady = true;
    }
    numPulsesThisChange = 0;
    return;
  }

  if (delta < MIN_PULSE_US) {
    // drop pulse, is noise
    numPulsesThisChange = 0;
    return;
  }

  rxFrame[numPulsesThisChange++] = delta;

  if (numPulsesThisChange >= MAX_PULSES) {
    // Don't call print in IRAM_ATTR interrupt!!
    // Serial.println("Critical error: Frame will overflow MAX_PULSES value, terminating early to prevent crash");
      numPulsesThisFrame = numPulsesThisChange;
      rxFrameReady = true;
      numPulsesThisChange = 0;
      return;
  }
}

void pulseToBinary(const volatile uint32_t* frame, uint32_t sz, std::bitset<MAX_PULSES>* out, bool* out_drop, uint32_t* out_long_us, uint32_t* out_short_us) {
  auto isShort = [](uint32_t t) { return t >= 250 && t <= 550; };
  auto isLong  = [](uint32_t t) { return t >= 850 && t <= 1400; };

  out->reset();

  uint32_t sum_long_dur_us = 0;
  int num_long = 0;
  uint32_t sum_short_dur_us = 0;
  int num_short = 0;

  for (uint32_t i=0; i<sz; ++i) {
    if (isLong(frame[i])) {
      out->set(i);
      sum_long_dur_us += frame[i];
      ++num_long;
    }
    else if (isShort(frame[i])) {
      sum_short_dur_us += frame[i];
      ++num_short;
    }
    else {
      // malformed packet
      *out_drop = true;
      return;
    }
  }

  // calculate average pulse dur
  *out_long_us = sum_long_dur_us / num_long;
  *out_short_us = sum_short_dur_us / num_short;

  // verify packet is valid (no long+long or short+short combos)
  for (uint32_t i=0; i+1<sz; i+=2) {
    // first and second bit
    const bool fbit = (*out)[i];
    const bool sbit = (*out)[i+1];
    if (fbit == sbit) {
      *out_drop = true;
      return;
    }
  }

  *out_drop = false;
  return;
}



void setup() {
  Serial.begin(115200);

  ELECHOUSE_cc1101.setSpiPin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, CC1101_CSN);
  ELECHOUSE_cc1101.setGDO0(CC1101_GDO0);
  ELECHOUSE_cc1101.Init();

  if (ELECHOUSE_cc1101.getCC1101()){         // Check the CC1101 Spi connection.
    Serial.println("CC1101 Connection OK");
  } else{
    Serial.println("CC1101 Connection Error");
  }

  ELECHOUSE_cc1101.setMHZ(433.92);
  ELECHOUSE_cc1101.setCCMode(0);    // 0 for raw data (eg. garage door controller), 1 for comms with other CC1101

  /*
  0 = 2-FSK (2-level Frequency Shift Keying) — the carrier frequency itself shifts slightly up or down to represent a 1 or a 0. Simple, robust, and what your code uses.
  1 = GFSK (Gaussian FSK) — same idea as 2-FSK, but the frequency shifts are smoothed out (Gaussian-filtered) instead of sudden jumps. This keeps the signal's bandwidth narrower and reduces interference with neighboring channels. Very commonly used in practice.
  2 = ASK/OOK (Amplitude Shift Keying / On-Off Keying) — instead of shifting frequency, the amplitude (strength) of the signal is toggled: full power = 1, no power = 0. This is what most cheap remotes, garage doors, and simple sensors use, since it's easy and cheap to generate on the transmitting end.
  3 = 4-FSK — like 2-FSK, but uses four distinct frequency levels instead of two, letting it encode 2 bits per symbol instead of 1 (higher data rate, but more sensitive to noise).
  4 = MSK (Minimum Shift Keying) — a special, more bandwidth-efficient variant of FSK where the frequency shifts are timed precisely to avoid abrupt phase changes. Efficient but more complex to work with.
  */ 
  ELECHOUSE_cc1101.setModulation(2);

  /*
  The general concept: in a normal CC1101-to-CC1101 link, every packet starts with a fixed bit pattern called a sync word (16 or 32 bits, configurable). The receiver is constantly listening to raw noise, and it uses the sync word like a "wake up, this is real data" signal. Once it sees that exact bit pattern, it knows a packet is starting and switches into "capture the following bytes" mode. Without this, the receiver has no way to distinguish real transmissions from random RF noise.

  The setting controls how strictly that sync word has to match:

  0 = No preamble/sync — the chip does no sync-word filtering at all. It just passes through whatever it receives continuously. This is what you want in raw/remote mode, since a garage door remote or generic sensor doesn't transmit any CC1101-style sync word — there's nothing for the chip to "match" against.
  1 = 15/16 bits — a looser match (1 bit of the 16-bit sync word is allowed to be wrong).
  2 = 16/16 bits — an exact 16-bit match required. Fewer false triggers, but more likely to miss a packet if there's a bit error.
  3 = 30/32 bits — like mode 1 but with a doubled (32-bit) sync word for extra reliability.
  4-7 — same patterns as above, but additionally require the signal strength to be above a carrier-sense threshold before triggering.
  */
  ELECHOUSE_cc1101.setSyncMode(0);

  /*
  The general concept: a CRC is a small checksum value calculated from the data in a packet and appended to the end of it.
  */
  ELECHOUSE_cc1101.setCrc(0);

  ELECHOUSE_cc1101.SetRx();
  pinMode(CC1101_GDO0, INPUT);
  attachInterrupt(digitalPinToInterrupt(CC1101_GDO0), onRssiChange, CHANGE);

  Serial.println("CC1101 setup complete");
}

#define PRINT_NOISE_DUR_MS 10000
volatile uint32_t elapsed_ms = 0;

void loop() {
  static uint32_t prev = (unsigned)millis();
  uint32_t now = (unsigned)millis();
  uint32_t dt = now - prev;
  prev = now;
  elapsed_ms += dt;

  if (elapsed_ms < PRINT_NOISE_DUR_MS) {
    Serial.print(ELECHOUSE_cc1101.getRssi());   // received signal strength indicator -30dBm(decibel watts) is strong, -90dBm is weak
    Serial.println(" dBm");
  }

  /*
  From testing, background RSSI fluctuates around 93-96dBm
  */

  if (rxFrameReady) {
    static const bool printRawPulseTimings = true;
    if (printRawPulseTimings) {
      Serial.printf("Pulses(%d): ", numPulsesThisFrame);
      for (uint16_t i=0; i<numPulsesThisFrame; ++i) {
        Serial.printf("%d%s", rxFrame[i], i+1>=numPulsesThisFrame ? "" : ", ");
      }
      Serial.printf(")\n");
    }

    bool dropPacket = false;
    std::bitset<MAX_PULSES> pulseBinary;
    uint32_t long_pulse_us = 0;
    uint32_t short_pulse_us = 0;
    pulseToBinary(rxFrame, numPulsesThisFrame, &pulseBinary, &dropPacket, &long_pulse_us, &short_pulse_us);

    if (!dropPacket) {
      Serial.printf("\n--\nPulse binary(%d) - Read left to right:\nLong pulse (us): %d\nShort pulse (us): %d\n", numPulsesThisFrame, long_pulse_us, short_pulse_us);
      for (uint32_t i=0; i<numPulsesThisFrame; ++i) {
        Serial.printf("%d%s", pulseBinary[i] ? 1 : 0, i != 0 && i%4 == 0 ? " " : "");
      }
      Serial.printf("\n--\n");
    }

    rxFrameReady = false;
  }
}
