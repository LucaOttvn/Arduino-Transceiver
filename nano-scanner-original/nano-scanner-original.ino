#include <ELECHOUSE_CC1101_SRC_DRV.h>

/**
  This is a simple RF detector based on arduino + CC1101 module.
  The program does this: 
  - Starts the serial so you can read output on the PC.
  - Initializes the CC1101.
  - Configures it in a very permissive receiver mode.
  - Scans a list of common remote-control frequencies.
  - Measures signal strength at each frequency.
  - Prints where the strongest activity was seen.
*/

struct Band {
  float center;
  float start;
  float stop;
  float step;
  const char* name;
};

/**
  {315.00, 314.50, 315.50, 0.01, "315 MHz"} means:
  - Expected band center is 315.00.
  - Start scanning at 314.50.
  - Stop scanning at 315.50.
  - Move in 0.01 MHz steps, which is 10 KHz each step.
  This means the radio will test 314.50, 314.51, 314.52, and so on until 315.50.
*/
Band bands[] = {
  {315.00, 314.50, 315.50, 0.01, "315 MHz"},
  {390.00, 389.50, 390.50, 0.01, "390 MHz"},
  {418.00, 417.50, 418.50, 0.01, "418 MHz"},
  {433.92, 433.40, 434.40, 0.01, "433.92 MHz"},
  {868.35, 867.80, 868.80, 0.02, "868 MHz"},
  {915.00, 914.40, 915.60, 0.02, "915 MHz"}
};

/**
  This calculates how many entries are inside bands[].
  sizeof(bands) = total bytes used by the whole array.
  sizeof(bands[0]) = bytes used by one single entry.
  Dividing them gives number of entries.
  So instead of manually writing 6, the code counts automatically. If you later add more bands, this still works.
*/
const int bandCount = sizeof(bands) / sizeof(bands[0]);

// “Only call it a hit if the signal is stronger than -75 dBm.”
const int rssiThreshold = -75;

/**
  When you retune the CC1101 to a new frequency, it needs a tiny moment to settle.  
  This line says to wait 4 milliseconds after changing frequency before reading RSSI.  
  Without a small delay, you can read unstable values.
*/
const int settleDelayMs = 4;

/**
  After one whole band is scanned, the code waits 250 ms before jumping to the next band.  
  This just slows things slightly so the serial output is easier to follow and the radio is not hammered continuously.
*/
const int pauseBetweenBands = 250;

void setup() {
  Serial.begin(115200);

  // This delay gives the serial connection time to come up after reset so you don’t miss the first lines.
  delay(1000);

  if (ELECHOUSE_cc1101.getCC1101()) {
    Serial.println("CC1101 connection OK");
  } else {
    Serial.println("CC1101 connection FAILED");
    while (1);
  }

  ELECHOUSE_cc1101.Init();

  /**
    Put the CC1101 in its standard packet/radio mode.
    In this mode the chip uses its own built-in RF logic instead of being treated as a more raw signal device.
  */
  ELECHOUSE_cc1101.setCCMode(1);

  /**
    This sets modulation to ASK/OOK. In this library:
    - 0 = 2-FSK
    - 1 = GFSK
    - 2 = ASK/OOK
    Because many cheap remote controls use OOK/ASK, especially at 433.92 MHz. It is not perfect for every remote, but it is a good first try.
  */
  ELECHOUSE_cc1101.setModulation(2);

  /**
    This sets the receive bandwidth very wide. A wide bandwidth makes the receiver less selective.
  */
  ELECHOUSE_cc1101.setRxBW(812);  
   
  /**
    This disables sync-word checking. 
    Sync words are special patterns used to mark the start of a valid packet. 
    By turning sync off, you make the receiver more permissive, which is useful for scanning unknown remotes.
  */
  ELECHOUSE_cc1101.setSyncMode(0);  

  /**
    This disables CRC checking. 
    CRC is an error-detection check used when receiving known packet formats.
    This isn't needed for a general purpose RF scanner like this one.
  */
  ELECHOUSE_cc1101.setCrc(0);       
  
  // This puts the CC1101 in receive mode. It basically says "start listening now". 
  ELECHOUSE_cc1101.SetRx();

  Serial.println("CC1101 scanner ready");
  Serial.println("Press the remote button repeatedly while scanning...");
  Serial.println();
}

void scanBand(Band band) {

  /**
    These are tracking variables:
    - bestFreq = strongest frequency found so far.
    - bestRssi = strongest RSSI found so far.
    - hits = how many times RSSI went above the threshold
    - -120 is used as a starting value because it is lower than the signal you expect, so almost any real signal will beat it.
  */
  float bestFreq = 0;
  int bestRssi = -120;
  int hits = 0;

  Serial.print("Scanning ");
  Serial.println(band.name);

  for (float f = band.start; f <= band.stop; f += band.step) {
    ELECHOUSE_cc1101.setMHZ(f);
    ELECHOUSE_cc1101.SetRx();
    delay(settleDelayMs);

    int rssi = ELECHOUSE_cc1101.getRssi();

    if (rssi > rssiThreshold) {
      hits++;
      Serial.print("  Hit at ");
      Serial.print(f, 2);
      Serial.print(" MHz  RSSI: ");
      Serial.println(rssi);
    }

    if (rssi > bestRssi) {
      bestRssi = rssi;
      bestFreq = f;
    }
  }

  Serial.print("Best in ");
  Serial.print(band.name);
  Serial.print(": ");
  Serial.print(bestFreq, 2);
  Serial.print(" MHz  RSSI: ");
  Serial.print(bestRssi);
  Serial.print("  Hits: ");
  Serial.println(hits);
  Serial.println();
}

void loop() {
  for (int i = 0; i < bandCount; i++) {
    scanBand(bands[i]);
    delay(pauseBetweenBands);
  }

  Serial.println("Full scan complete. Press the remote again to compare results.");
  Serial.println("------------------------------------------------------------");
  delay(1000);
}