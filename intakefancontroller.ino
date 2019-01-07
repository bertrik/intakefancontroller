// intake fan controller for revspace
// arduino nano board
// libraries used are standard versions

#include <Arduino.h>
#include <Time.h>
#include <TM1637Display.h>

//timer delay stuff
unsigned long previousmillis = 0;
const long millidelay = 1000;

//fan state
bool fanon = false;

//display stuff
#define CLK 6
#define DIO 7
TM1637Display display(CLK, DIO);
uint8_t data[] = { 0, 0, 0, 0 };

const uint8_t ON[] = {          //segment code for '-On-' state
    SEG_G,
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
    SEG_C | SEG_E | SEG_G,
    SEG_G
};

const uint8_t leet[] = {        //cyber enabled, maximum pander
    SEG_D | SEG_E | SEG_F,
    SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,
    SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,
    SEG_D | SEG_E | SEG_F | SEG_G
};

//time stuff
int totalminutes = 0;
int seconds = 0;
int minutes = 0;
int hours = 0;
time_t t;
tmElements_t tm;

void setup(void)
{
    unsigned long currentmilis = millis();

    display.setBrightness(7);   //full brightness

    pinMode(2, INPUT);
    pinMode(3, INPUT);
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(A0, OUTPUT);        //outputs fan state to esp, HIGH when fan is ON

    digitalWrite(A0, LOW);
    digitalWrite(4, LOW);
    digitalWrite(5, HIGH);      //pulse coil to turn fan ON (in case of reset/brownout while fan was disabled);
    delay(50);
    digitalWrite(5, LOW);
    display.setSegments(ON);

    tm.Second = 0;
    tm.Minute = 0;
    tm.Hour = 0;
    tm.Day = 0;
    tm.Month = 0;
    tm.Year = CalendarYrToTm(2018);
    t = makeTime(tm);
}

void displaytime()
{
    if (hours < 1) {            //when it fits, display seconds
        data[3] = display.encodeDigit(seconds / 1 % 10);
        data[2] = display.encodeDigit(seconds / 10 % 10);
        data[1] = 0x80 | display.encodeDigit(minutes / 1 % 10); //add semicolon separator
        data[0] = display.encodeDigit(minutes / 10 % 10);
        display.setSegments(data);
    }

    if (hours > 0) {            //omit seconds
        data[3] = display.encodeDigit(minutes / 1 % 10);
        data[2] = display.encodeDigit(minutes / 10 % 10);
        data[1] = 0x80 | SEG_C | SEG_E | SEG_G | SEG_F | SEG_B; //add semicolon and 'H' for hours as separator
        data[0] = display.encodeDigit(hours / 1 % 10);
        display.setSegments(data);
    }
    if (minutes == 13 && (seconds == 37)) {
        display.setSegments(leet);
    }
}

void redpress()
{                               //add 15 minutes to time, up to the maximum delay allowed (120 minutes)
    delay(40);                  //debounce overkill
    if (digitalRead(2) == 0) {
        totalminutes = hours * 60;
        totalminutes = totalminutes + minutes;

        if (totalminutes < 104) {       // limit max off time to 120 minutes
            tm.Minute = minutes + 15;
            tm.Hour = hours;
            tm.Second = seconds;
        } else {
            tm.Minute = 0;
            tm.Hour = 2;
            tm.Second = 0;
        }
        t = makeTime(tm);
    }
    while (digitalRead(2) == 0) {
    }
}

void greenpress()
{                               //reset all time variables to zero
    delay(40);                  //debounce overkill
    if (digitalRead(3) == 0) {
        tm.Hour = 0;
        tm.Minute = 0;
        tm.Second = 0;
        t = makeTime(tm);
        seconds = 0;
        minutes = 0;
        hours = 0;
    }
    while (digitalRead(3) == 0) {
    }
}

void countdown()
{                               //subtract count from time and display time/status
    if (hours + seconds + minutes > 0) {
        --t;
        seconds = second(t);
        minutes = minute(t);
        hours = hour(t);
        displaytime();
        turnoff();              //turn off when countdown initiates
    } else {
        display.setSegments(ON);
        turnon();               //turn fan back on when countdown completes
    }
}

void turnoff()
{                               //pulse coil to switch fan to OFF, set fanon status to prevent retrigger
    if (fanon == true) {
        digitalWrite(4, HIGH);
        delay(60);
        digitalWrite(4, LOW);
        digitalWrite(A0, LOW);

        fanon = false;
    }
}

void turnon()
{                               //pulse coil to switch fan back on, set fanon status to prevent retrigger
    if (fanon == false) {
        display.setSegments(ON);
        digitalWrite(5, HIGH);
        delay(60);
        digitalWrite(5, LOW);
        digitalWrite(A0, HIGH);

        fanon = true;
    }
}


void loop()
{
    unsigned long currentmillis = millis();     //reload millis at the start of loop
    if (digitalRead(2) == 0) {  //check for key press
        redpress();             //do stuff in response to key press
        seconds = second(t);
        minutes = minute(t);
        hours = hour(t);
        displaytime();          //display new time remaining immediately (don't wait for next countdown interval)
    }

    if (digitalRead(3) == 0) {  //check for key press
        greenpress();           //do stuff in response to key press
    }

    if (currentmillis - previousmillis >= millidelay) { //when set interval has elapsed, update time and display
        previousmillis = currentmillis;
        countdown();
    }
}

