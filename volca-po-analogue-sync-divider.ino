  //o  O O  VOLCA SYNC DIVIDER  (O) 000 o|
///|   (O)  x   o   o   o     o o o o    |
///|        o   o   x   x     = = = =    |
///|   (O)  o   o   x   o                |
///|o___________________________________o|
///||   [][][]  [][]  [][][]  [][]  []  ||
///||  [][][][][][][][][][][][][][][][] ||
///||-----------------------------------||
///|/   *  *   *   *     *  *  *   *    \|
/////////////////////////////////////////
////////////////////////////////////////

/*
  An Arduino driven Korg Volca sync divider. The arduino recieves a sync pulse from a volca, and outputs 2 or more
  pulses at a lower BPM while remaining in time.

  Based from https://raw.githubusercontent.com/EmergentProperly/Volca-Sync-Divider/main/Volca-Sync-Divider.ino
*/


#if defined(ARDUINO_AVR_UNO)
  const int led_in = LED_BUILTIN;
  const int sync_in = A0;
  const int sync_out = 2;
  const int gate_out = 5;
  const int led_sync = 3;
  const int button_divider = A1;
#else  // ATTINY85
  const int sync_in = 3;  // analog read, 3 is pin 2
  const int sync_out = 2;  // digital write, 2 is pin 7
  const int gate_out = 1;  // analog write for potential cv, 1 is pin 6
  const int led_sync = 0;  // analog write for dim on all inputs, bright on output, 0 is pin 5
  const int button_divider = 4;  // digital read to cycle through dividers (1, 2, 4, 8, 16); 4 is pin 3.
  // consider analog read on button divider with resistor ladder and multiple buttons for instant/non-cycling select and reset pulse_cnt
  // or consider a long hold on button for reset instead of using the reset button (bc when you hit the reset button, you loose the divider index)
#endif


int pulse_cnt = 0;  // counter for the number of sync pulses
int prev_input = 0;  // previous state of the sync input

int prev_button = 0;  // previous state of the button input
int buttonState = 0;  // the current reading from the input pin
// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

// initialize to output 1 sync for every divider input pulse, these are the options
int dividers[] = {0, 1, 2, 4, 8, 16};
int dividers_len = 6;
int dividers_idx = 2;  // PO and Volca are 2 PPQN, this defaults the division to 1 PPQN
// AVR 10b ADC means 1024 points, max 1023
// With 5v, 204.8 is 1v. PO can be .6-1v, Volca 3.3-5v.
// Using the Examples > 01. Basic > AnalogReadSerial, observe the min/max values
// Without a pull-down across tip & sleeve/ground you'll get lots of high values when nothing is plugged in
// With no pull-down 191-233
// With 1k 157-190
// With 10k 172-238; there was an occasional 53. Set at 100 and see if any are skipped
int adc_threshold = 100;


void setup() {
  pinMode(sync_in, INPUT);  // active-high
  pinMode(sync_out, OUTPUT);
  digitalWrite(sync_out, LOW);
  pinMode(gate_out, OUTPUT);
  digitalWrite(gate_out, LOW);  // start in non-playing
  pinMode(led_sync, OUTPUT);
  digitalWrite(led_sync, LOW);
  pinMode(button_divider, INPUT_PULLUP); // active-low
  do_alive();
}


void do_alive() {
  // signal that it's alive/on
  #if defined(ARDUINO_AVR_UNO)
    Serial.begin(9600);
    pinMode(led_in, OUTPUT);
    digitalWrite(led_in, LOW);
    Serial.println(String("V-trigger divider for Volca/PO analog sync. Supports 1-5V. Default 1/2. Active-low to change division on pin ") + button_divider);
    digitalWrite(led_in, HIGH);
    delay(100);
    digitalWrite(led_in, LOW);
    delay(50);
    digitalWrite(led_in, HIGH);
    delay(100);
    digitalWrite(led_in, LOW);
    delay(50);
    digitalWrite(led_in, HIGH);
    delay(100);
    digitalWrite(led_in, LOW);
  #else  // attiny
    analogWrite(led_sync, 255);
    delay(5);
    digitalWrite(led_sync, LOW);
    delay(5);
    analogWrite(led_sync, 80);
    delay(5);
    digitalWrite(led_sync, LOW);
  #endif
}  // end do_alive


void debug_ain(int sync_volts) {
  // temporary to learn what values the PO sync shows up as on analog input
  #if defined(ARDUINO_AVR_UNO)
    // use builtin on/off led, to show on if above threshold
    // 0-1023, so 204.8 is 1v
    if (sync_volts > adc_threshold) {
      Serial.println(String("sync ") + sync_volts  + String(" /1023"));
      digitalWrite(led_in, HIGH);
      delay(29);
      digitalWrite(led_in, LOW);
    }
  #else  // attiny
    analogWrite(led_sync, sync_volts/4);
    delay(20);
    digitalWrite(led_sync, LOW);
    delay(9);
  #endif
  return;
} // end debug_ain


void loop() {
  int sync_volts = analogRead(sync_in);
  // debug_ain(sync_volts);
  do_divider(sync_volts);
  adjust_divider();
  // do_divider uses 29 msecs, with 15 in the active state. With this 1, total is 30 msecs
  // When there is not a trigger this loop repeats with only delay(1)
  // When there is a trigger this loop repeats with total delay(30)
  // littleBits can use the inverted PO (so 2.5 msecs S-trigger) but the bits themselves keyboard/sequencer/etc short for 10-30 msecs
  // PO may only be 2.5 msecs, and Volca is 15 msecs
  // 30 msecs will be ready to catch 30ish triggers per second, or 1800 pulses per minute. At Volca/PO's 2 PPQN, this is 900 BPM
  // At MIDI 24 PPQN, 1800 pulses/minute is only 75 BPM
  // If adapted to read MIDI serial rx, then you'd need to shorten the pulse divider makes or the artificial post pulse delay
  delay(1);
}  // end loop


void do_divider(int sync_volts) {
  // is input high right now?
  if (sync_volts <= adc_threshold) {
    // save curr input high/low into prev_input for next loop
    prev_input = 0;
    return;
  }
  // can now assume curr_input is high
  // if prev_input high (no change happend), then ignore
  if (prev_input) {
    return;
  }
  // can now assume curr_input is high and prev_input was low (change happened, this is a new pulse)
  #if defined(ARDUINO_AVR_UNO)
    // tick led_builtin, leverage the below unified 15 msec delay for timing
    digitalWrite(led_in, HIGH);
  #endif
  // if this pulse is on the division (pulse_cnt % division), then output to sync_out and short gate_out and bright tick led_sync, all for 15 msec
  // special case divider of 0 means skip all inputs, so ensure divider > 0
  if (dividers[dividers_idx] && (pulse_cnt % dividers[dividers_idx]) == 0) {
    #if defined(ARDUINO_AVR_UNO)
      Serial.println(String("Y ") + pulse_cnt % dividers[dividers_idx] + String("/") + dividers[dividers_idx]);
    #endif
    analogWrite(led_sync, 255);
    digitalWrite(sync_out, HIGH);
    digitalWrite(gate_out, LOW);
  }
  // else dim tick led_sync for 15 msec to indicate that it got a pulse but didn't output it
  else {
    #if defined(ARDUINO_AVR_UNO)
      Serial.println(String("N ") + pulse_cnt % dividers[dividers_idx] + String("/") + dividers[dividers_idx]);
    #endif
    analogWrite(led_sync, 10);
  }
  // unified timing for all outputs
  delay(15);
  // now turn everything off that was on
  #if defined(ARDUINO_AVR_UNO)
    digitalWrite(led_in, LOW);
  #endif
  analogWrite(led_sync, 0);
  digitalWrite(sync_out, LOW);
  digitalWrite(gate_out, HIGH);
  // now sleep until 29th msec, there is a 1 msec in the loop() for a total of 30 msec
  delay(14);
  prev_input = 1;
  pulse_cnt = (pulse_cnt + 1) % 256;  // some arbitraty large value to prevent overflow
}  // end do_divider


void adjust_divider() {
    // read the state of the switch into a local variable:
  int curr_button = digitalRead(button_divider);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (curr_button != prev_button) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the curr_button is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (curr_button != buttonState) {
      buttonState = curr_button;

      // only cycle to the next divider if the new button state is HIGH
      if (buttonState == LOW) {
        dividers_idx = (dividers_idx + 1) % dividers_len;
        #if defined(ARDUINO_AVR_UNO)
          Serial.println(String("New divider ") + dividers[dividers_idx] + String(" index ") + dividers_idx);
        #endif
      }
    }
  }
  // save the curr_button. Next time through the loop, it'll be the prev_button:
  prev_button = curr_button;
}  // end adjust_divider
