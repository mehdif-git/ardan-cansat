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

    rf95.setFrequency(868.125);
}

void loop() {
    if (rf95.available()) {
        uint8_t buf[64];
        uint8_t len = sizeof(buf);
        if(rf95.recv(buf, &len)){
            digitalWrite(led, HIGH);
            char message[64];
            char temperature[16];
            char pressure[16];
            char altitude[16];
            if(sscanf((char*)buf,"%s %s %s ",temperature,pressure,altitude)){
                ShowSerial.println(temperature);
                ShowSerial.println(pressure);
                ShowSerial.println(altitude);
            } else{
                sscanf((char*)buf, "%s\n", message);
                ShowSerial.println(message);
            }
            digitalWrite(led, LOW);
        }
    }
}


