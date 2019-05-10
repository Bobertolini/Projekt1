#include <iostream>
#include <chrono>
#include <thread>
#include <math.h>
#include <float.h>

#include <wiringPi.h>

using namespace std;

enum MODE : uint8_t { //0-255

    MODEDAY,
    MODENIGHT,
    MODETRAFFIC,
    MODEUNKNOWN
};

//helper method to compare floats https://floating-point-gui.de/errors/comparison/
#define FLT_EQL(a, b) (fabsf((float)((a) - (b))) <= 0.01)

bool daymode(float &delta)
{

    if (FLT_EQL(delta, 1.2f)) {

        digitalWrite(1,1); // piesi czerwone
        digitalWrite(7,0);
        digitalWrite(4,1); // samochody pionowo czerwone
        digitalWrite(5,0);
        digitalWrite(6,0);
        digitalWrite(3,1); // samochody poziomo zielone
        digitalWrite(2,0);
        digitalWrite(0,0);
    }

    // zmiana kierunku ruchu na pionowy
    if (FLT_EQL(delta, 12.0f)) {

        digitalWrite(3,0);
        digitalWrite(2,1);
    }

    if (FLT_EQL(delta, 13.2f)) {

        digitalWrite(2,0);
        digitalWrite(0,1);
        digitalWrite(5,1);
    }

    // margines bezpieczenstwa
    if (FLT_EQL(delta, 14.4f)) {

        digitalWrite(4,0);
        digitalWrite(5,0);
        digitalWrite(6,1); // samochody pionowo zielone
    }

    if (FLT_EQL(delta, 29.4f)) { // zmiana kierunku ruchu na poziomy

        digitalWrite(6,0);
        digitalWrite(5,1);
    }

    if (FLT_EQL(delta, 30.6f)) {

        digitalWrite(5,0);
        digitalWrite(4,1);
        digitalWrite(2,1);
    }

    if (delta > 31.0f) {

        delta = 0.0f;
        return true;
    }

    return false;
}

bool nightmode(float &delta)
{
    // mruganie lampek pomaranczowych

    if (FLT_EQL(delta, 0.5f)) {

        digitalWrite(2,1);
        digitalWrite(5,1);
    }

    if (FLT_EQL(delta, 1.0f)) {

        digitalWrite(2,0);
        digitalWrite(5,0);
    }

    if (delta > 1.0f) {

        delta = 0.0f;
        return true;
    }

    return false;
}

bool modetraffic(float &delta) //pedestrians
{
    if (FLT_EQL(delta, 1.0f)) {

        digitalWrite(1,1);
        digitalWrite(7,0);
        digitalWrite(2,1);
        digitalWrite(0,0);
        digitalWrite(3,0);
        digitalWrite(5,1);
        digitalWrite(4,0);
        digitalWrite(6,0);
    }

    if (FLT_EQL(delta, 2.8f)) {

        digitalWrite(2,0);
        digitalWrite(5,0);
        digitalWrite(0,1);
        digitalWrite(4,1);
        digitalWrite(7,1);  // piesi zielone
    }

    if (FLT_EQL(delta, 9.8f)) {

        for (int i=0; i<20; i++) {   // mruganie diody pieszych

            digitalWrite(7,0);
            this_thread::sleep_for(chrono::milliseconds(100));
            digitalWrite(7,1);
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }

    if (delta > 9.9f) {

        delta = 0.0f;
        return true;
    }

    return false;
}

void reset() {

    digitalWrite(1,0);
    digitalWrite(7,0);
    digitalWrite(2,0);
    digitalWrite(0,0);
    digitalWrite(3,0);
    digitalWrite(5,0);
    digitalWrite(4,0);
    digitalWrite(6,0);
}

int main()
{
    if (wiringPiSetup() == -1) {
        return 1;
    }

    cout << "Start" << endl;

    pinMode(0,OUTPUT); //czerwone-samochody, droga pozioma
    pinMode(2,OUTPUT); //pomaranczowe-samochody, droga pozioma
    pinMode(3,OUTPUT); //zielone-samochody, droga pozioma

    pinMode(1,OUTPUT); //czerwone-piesi
    pinMode(7,OUTPUT); //zielone-piesi

    pinMode(4,OUTPUT); //czerwone-samochody, droga pionowa
    pinMode(5,OUTPUT); //pomaranczowe-samochody, droga pozioma
    pinMode(6,OUTPUT); //zielone-samochody, droga pozioma

    reset();

    pinMode(21,INPUT); //aktywowanie tryb_dzienny
    pinMode(22,INPUT); //aktywowanie tryb_nocny/awaria
    pinMode(23,INPUT); //aktywowanie tryb_piesi

    pullUpDnControl(21, PUD_DOWN);
    pullUpDnControl(22, PUD_DOWN);
    pullUpDnControl(23, PUD_DOWN);

    const float DT = 0.1f; //100ms
    float accum = 0.0f;

    MODE mode = MODEUNKNOWN;

    bool finished = false;
    bool shouldEnableTraffic = false;

    while (true)
    {
        accum += DT;

        if (accum >= FLT_MAX) accum = 0;

        const int bt1state = digitalRead(21);
        const int bt2state = digitalRead(22);
        const int bt3state = digitalRead(23);

        cout << "DEBUG (" << bt1state << "," << bt2state << "," << bt3state << ")" << endl;

        if (bt1state > 0 && mode != MODEDAY) {

            cout << "DAY" << endl;
            mode = MODEDAY;
            accum = 0;
        }

        if (bt2state > 0) {

            cout << "NIGHT" << endl;
            mode = MODENIGHT;
            accum = 0;
            reset();
            shouldEnableTraffic = false;
        }

        if (bt3state > 0 && mode != MODETRAFFIC) {
            cout << "TRAFFIC" << endl;

            if (finished) {

                mode = MODETRAFFIC;
                accum = 0;

            }
            else {
                shouldEnableTraffic = true;
            }
        }

        switch(mode) {
            case MODEDAY: {
                finished = daymode(accum);

                if (finished && shouldEnableTraffic == true) {

                    mode = MODETRAFFIC;
                    accum = 0;
                    shouldEnableTraffic = false;
                }

                break;
            }
            case MODENIGHT: {
                finished = nightmode(accum);
                break;
            }
            case MODETRAFFIC: {
                finished = modetraffic(accum);

                if (finished) {

                    mode = MODEDAY;
                    accum = 0;
                }

                break;
            }
            case MODEUNKNOWN:
            default:
                break;
        }

        this_thread::sleep_for(chrono::milliseconds(100));
    }

    return 0;
}
