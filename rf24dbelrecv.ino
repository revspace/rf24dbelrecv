#include <avr/wdt.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// NB: This sketch assumes 32 bit addresses

static long int address = 0x66996699L;  // So that's 0xFF66996699
const int payload = 32;

RF24 rf(/*ce*/ 8, /*cs*/ 10);

int open = -1;


void beep(int n) {
    while (n--) {
        digitalWrite(4, HIGH);
        delay(50);
        digitalWrite(4, LOW);
        delay(100);
    }
}

boolean heartbeat_on = false;
void heartbeat() {
    if (heartbeat_on) {
        digitalWrite(2, HIGH);
    }
    else {
        digitalWrite(2, LOW);
    }
    heartbeat_on = !heartbeat_on;
}

void setup() {
    pinMode(4, OUTPUT);
    beep(3);
    delay(2000);  // lang, want condensator.
    
    pinMode(2, OUTPUT);  // for heartbeat
    digitalWrite(2, LOW);

    rf.begin();

    //  rf.setRetries(15, 15);
    rf.enableDynamicPayloads();
    rf.openReadingPipe(1, address);
    rf.setAutoAck(false);
    rf.startListening();

    Serial.begin(115200);
    Serial.println(rf.isPVariant());

    wdt_enable(WDTO_8S);
}

long extra = 600;
long ignore = 2000;
int progress = 0;

void loop() {
    static signed long stoptime = 0;
    static char buf[250];
    static int counter;

    static signed long starttime = 0;

    delay(50);
    progress++;
    if(progress >= 20) {
      progress = 0;
      heartbeat();
    }

    if (( (long) millis() - starttime) > 500) {
        starttime = 0;
    }

    if (stoptime && ((long) millis() - stoptime) > 0) {
        digitalWrite(4, LOW);
        stoptime = 0;
        starttime = 0;
    }

    if (rf.available()) {
        memset(&buf, 0, sizeof(buf));
        rf.read(&buf, sizeof(buf));

        wdt_reset();

        if (buf[0] > (payload - 1)) return;
        buf[ (int) buf[0] + 1 ] = '\0';

        Serial.print(millis());
        Serial.print(" ");
        Serial.println(buf + 1);

        if (strcmp("DBEL", buf + 1) == 0) {
            Serial.print("triiiiiing ");
            Serial.println(++counter);
            if (!starttime) {
                Serial.println("first packet ignored");  // workaround for stray packets
                starttime = millis();
                return;
            }

            digitalWrite(4, HIGH);
            stoptime = (long) millis() + extra;
        }
        else if (strncmp("OPEN", buf + 1, 4) == 0) {
            char arg = buf[5];
            int oldopen = open;
            if (arg == '0') open = 0;
            if (arg == '1') open = 1;
            if (oldopen >= 0 && open != oldopen) {
                beep(open ? 2 : 1);
            }
        }
    }
}

// vim: ft=cpp
