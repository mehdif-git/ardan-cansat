// rf95_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messageing client
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example rf95_server
// Tested with Anarduino MiniWirelessLoRa

#include <RH_RF95.h>
RH_RF95<HardwareSerial> rf95(Serial);

void setup() {
    rf95.init();
    // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

    // The default transmitter power is 13dBm, using PA_BOOST.
    // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
    // you can set transmitter powers from 5 to 23 dBm:
    //rf95.setTxPower(13, false);
    rf95.setFrequency(868.125);
}

void loop() {
    // Send a message to rf95_server
    uint8_t data[] = "Hello World!";
    rf95.send(data, sizeof(data));
    rf95.waitPacketSent();
    delay(4000);
}


