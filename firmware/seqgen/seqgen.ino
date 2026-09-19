/*
 * Sequence Generator
 * 
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SEQ_LEN_MAX      512
#define DISP_BUF_LEN     SEQ_LEN_MAX+1+1 // +1 for "_" character and another +1 for \0 termination

LiquidCrystal_I2C lcd(0x27, 16, 2);

char firmware_version[] = "seqgen v20260919_0815";

int OUTPUT_PIN = 8;     // PB0
int INPUT_0_PIN = 3;    // PD3
int INPUT_1_PIN = 4;    // PD4
int DEL_PIN = 5;        // PD5
int INPUT_PW_PIN = A0;  // PC0
int SETUP_PIN = 2;      // PD2
int OUT_IND_LED = 7;    // PD7

unsigned char sequence[SEQ_LEN_MAX] = {};
int seq_cnt = 0;
unsigned long int pulse_width_us;
unsigned long int pw_adder = 1;

unsigned char display_buffer[DISP_BUF_LEN] = {};

// wrapper function that determines if a button has been pressed and released
bool buttonPressed(int PIN_NUMBER){
    if(digitalRead(PIN_NUMBER) == HIGH){
        while(digitalRead(PIN_NUMBER) != LOW){}  // wait for button to be released
        return true;
    }else{
        return false;
    }
}

// function for displaying text on a 16x2 display
void display16x2(unsigned char msg[]){
    
    int msg_len = strlen(msg);
    int printed_cnt = 0;

    lcd.clear();
    lcd.setCursor(0,0);
    
    if(msg_len > 16*2){
        lcd.print("^");
        printed_cnt+=1;

        int to_print = msg_len - 16*((msg_len-33)/16 + 1) - printed_cnt;
        for(int i = to_print; i > 0; i--){
            if(printed_cnt == 16) lcd.setCursor(0,1);
            lcd.write(msg[msg_len-i]);
            printed_cnt++;
        }
    } else {
        for(int i = 0; i < msg_len; i ++){
            if(printed_cnt == 16) lcd.setCursor(0,1);
            lcd.write(msg[i]);
            printed_cnt++;
        }
    }
}

void setup() {
    pinMode(OUTPUT_PIN, OUTPUT);
    pinMode(INPUT_0_PIN, INPUT);
    pinMode(INPUT_1_PIN, INPUT);
    pinMode(DEL_PIN, INPUT);
    pinMode(INPUT_PW_PIN, INPUT);
    pinMode(SETUP_PIN, INPUT);
    pinMode(OUT_IND_LED, OUTPUT);

    Serial.begin(9600);

    lcd.init();
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("sequence");
    lcd.setCursor(0,1);
    lcd.print("generator");

    // output constant 0v signal (no output) initially
    sequence[0] = LOW;
    seq_cnt = 1;
}

void loop() {

    seq_output_start:
    for(int i = 0; i < seq_cnt; i++){

        // The below read operation will take about 100-120 us (code delay), which adds to actual pulse width.
        pulse_width_us = analogRead(INPUT_PW_PIN) + pw_adder;

        digitalWrite(OUTPUT_PIN, sequence[i]);
        unsigned long pulse_begin_t = micros();
        
        // wait pulse_width_us time while also checking if setup button is pressed
        while(micros()-pulse_begin_t < pulse_width_us){

            if(buttonPressed(INPUT_1_PIN)){
                pw_adder+=1000;
            }
            if(pw_adder >= 1000 && buttonPressed(INPUT_0_PIN)){
                pw_adder-=1000;
            }
            if(buttonPressed(SETUP_PIN)){

                // clear display buffer
                for(int i = 0; i < DISP_BUF_LEN; i++) display_buffer[i] = 0;
                
                display_buffer[0] = '_';
                display16x2(display_buffer);
                
                digitalWrite(OUTPUT_PIN, LOW);
                digitalWrite(OUT_IND_LED, LOW);
                
                bool seq_set = false;
                seq_cnt = 0;
                while(1){
                    // TODO: implement functionality for seq_cnt overflow (e.g. blink LED and set seq_cnt to 0)
                    if(buttonPressed(INPUT_0_PIN)){

                        if(seq_cnt == SEQ_LEN_MAX){
                            // if sequence has overflowed, reset sequence
                            seq_cnt = 0;
                            for(int i = 0; i < DISP_BUF_LEN; i++) display_buffer[i] = 0;    // clear display buffer
                        }

                        display_buffer[seq_cnt] = '0';
                        display_buffer[seq_cnt+1] = (seq_cnt < SEQ_LEN_MAX-1) ? '_' : '!';  // in normal condition, display '_' asking for next character, but in near overflow condition display '!'
                        display16x2(display_buffer);
                        
                        seq_set = true;
                        sequence[seq_cnt++] = LOW;
                    }
                    if(buttonPressed(INPUT_1_PIN)){

                        if(seq_cnt == SEQ_LEN_MAX){
                            // if sequence has overflowed, reset sequence
                            seq_cnt = 0;
                            for(int i = 0; i < DISP_BUF_LEN; i++) display_buffer[i] = 0;    // clear display buffer
                        }

                        display_buffer[seq_cnt] = '1';
                        display_buffer[seq_cnt+1] = (seq_cnt < SEQ_LEN_MAX-1) ? '_' : '!';  // in normal condition, display '_' asking for next character, but in near overflow condition display '!'
                        display16x2(display_buffer);
                        
                        seq_set = true;
                        sequence[seq_cnt++] = HIGH;
                    }
                    if(seq_set && buttonPressed(DEL_PIN)){
                        display_buffer[seq_cnt] = 0;
                        display_buffer[seq_cnt-1] = '_';
                        display16x2(display_buffer);

                        seq_cnt--;
                        if(seq_cnt == 0) seq_set = false;
                    }
                    if(seq_set && buttonPressed(SETUP_PIN)){
                        display_buffer[seq_cnt] = 0;
                        display16x2(display_buffer);
                        
                        digitalWrite(OUT_IND_LED, HIGH);
                        goto seq_output_start;
                    }
                }
            }
        }
    }
}
