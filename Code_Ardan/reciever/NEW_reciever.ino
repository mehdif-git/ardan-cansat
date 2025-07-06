// rf95_server.pde
// -*- mode: C++ -*-

#include <RH_RF95.h>
#include <SoftwareSerial.h>
SoftwareSerial SSerial(10, 11); // RX, TX
#define COMSerial SSerial
#define ShowSerial Serial

RH_RF95<SoftwareSerial> rf95(COMSerial);

int led = 13;

void setup() {
    ShowSerial.begin(115200);
    ShowSerial.println("Reciever STARTED.");

    pinMode(led, OUTPUT);

    if (!rf95.init()) {
        ShowSerial.println("init failed");
        while (1);
    }
    // Defaults after init are 868.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

    // The default transmitter power is 13dBm, using PA_BOOST.
    // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
    // you can set transmitter powers from 5 to 23 dBm:
    //rf95.setTxPower(13, false);

    rf95.setFrequency(866.125);
}

void loop() {
if (rf95.available()) {
        // Should be a message for us now
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        if (rf95.recv(buf, &len)) {
            digitalWrite(led, HIGH);

            ShowSerial.print("got request: ");
            ShowSerial.println((char*)buf);

            // Send a reply
            uint8_t data[] = "And hello back to you";
            rf95.send(data, sizeof(data));
            rf95.waitPacketSent();
            ShowSerial.println("Sent a reply");

            digitalWrite(led, LOW);
        } else {
            ShowSerial.println("recv failed");
        }
    }}


