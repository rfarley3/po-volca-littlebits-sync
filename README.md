# Pocket Operator and Volca Analogue Sync Divider plus littleBits Gate
An Arduino driven S-trig analogue sync divider.
This is the sync used in Teenage Engineering Pocket Operator or Korg Volca.
The Arduino receives a sync pulse, and outputs V and S-trig pulses at a lower 
BPM while remaining in time.

Based from 
https://raw.githubusercontent.com/EmergentProperly/Volca-Sync-Divider/main/Volca-Sync-Divider.ino

## More details
Can run on Arduino Uno (for testing), or an ATTiny (85 tested, 45 should work, 
this should also include Adafruit Trinket 5V, since they have an t85).

This script polls `sync_in` or `A0` for sync signals.
The signal doesn't have to be a logic high, see `AnalogReadSerial-PO.ino` or 
`debug_ain()` to find your threshold.
It uses a frequency divider (2^n, st n between 0 and 4) of 1:1, 2:1, 4:1, 8:1, 
16:1, defaulting to 2:1 or halving to convert 2 PPQN into littleBits 1 PPQN.
Press/click button_divider to change the division factor.
Special case divider of 0 means skip all inputs, so ensure divider > 0.

One output `sync_out` is the divided pulse as a V-trig, at a nice logic high and 
fixed period.
Sync outputs will match LED bright blink, skipped outputs are dim blink.
The other `gate_out` is nearly the inverse, an S-trig, almost always on while 
pulse output is off.
Gate out is useful as a gate for littleBits, particularly for the 4 step 
sequencer, envelope, keyboard, and anything else that needs a S-trig signal.

littleBits can use the inverted PO (so 2.5 msecs S-trigger) but the bits 
themselves keyboard/sequencer/etc short for 10-30 msecs.
PO may only be 2.5 msecs, and Volca is 15 msecs.
30 msecs will be ready to catch 30ish triggers per second, or 1800 pulses per 
minute.
At Volca/PO's 2 PPQN, this is 900 BPM.

## Circuit Simple, but Double BPM
Converting PO sync to littleBits could be easy.
My first circuit was simply a NPN, with the sync on the bias through 1k Ohm, 
emitter to ground, and collector lifted to 5V through 1k Ohm.
The collector is now the inverse of the bias, converting V-trig to S-trig.
The collector will have the signal load on it, which is why it is 1k, instead of 
something more like 10k that would be purely for lifting it.
A normally open momentary on the collector can manually S-trig.
A normally open momentary on the bias can be used to skipped triggers while 
held.
The collector can be 0-5V, allowing the sync to S-trig CV, not just 5V gate.
However, the PO sends 2 PPQN, which may make the littleBits seem 2x BPM.

## Circuit Without Microcontroller
To get the BPM equal, you need a frequency divider.
I tried conditioning the sync signal (transistor-resistor NPN to PNP) so it 
would be clearly 0 or 5V, and then AND'ing that with flip flop outputs (TTL 74).
I did two D flip flops. It worked on the breadboard, but not perf board.
TTLs suffer from no hysteresis and no buffering.

## Circuit Final
My final product was on an ATTiny85.
If you burn a bootloader, set internal clock to 16 MHz.

* TRS jack, Signal in (V-trig) - 10k R2 pull down - Pin 2
	* Use TRS for flexibility, PO have sync options where Right channel (Ring) is 
			audio, they also have an option where the Right channel (Ring) is another 
			sync. In either case, you don't want to short that to ground.
* Pin 3 - Normally open, momentary switch - Ground
* Pin 4 - Ground
* Pin 5 - LED - 220 R3 - Ground
* Pin 6 - Gate out (S-trig)
	* Add a normally open momentary switch to ground here for manual s-trig
* Pin 7 - Signal out (V-trig)
* Circuit from Datasheet for RST and VCC
	* 5V - Pin 8 (VCC) - +C1 10 uF - Ground
	* 5V - 10k R1 pull up - Pin 1 (RST) - Normally open, momentary switch, Ground

For convenience, I ran the sync output from this into the first NPN circuit 
(from the Circuit Simple section).
This allowed me to use the littleBits signal voltage (vs always 5V) as the 
output for the V-trig and S-trig.
For this, the ATTiny85 pin 7 (sync out) goes the bias of the NPN, and its 
collector is lifted by the littleBits signal.
The collector is now a gated CV (S-Trig per frequency divider).

## Parts
* ATTiny85
* LED
* 10k Ohm R1
* 10k Ohm R2
* 220 Ohm R3
* two normally open, momentary switch
* 10 uF electrolytic C1
* TRS jack
* 5V power and ground, such as littleBits rail or USB adapter
