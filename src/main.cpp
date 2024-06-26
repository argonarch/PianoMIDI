#include "MIDIUSB.h"
#include <Arduino.h>

#define NUM_ROWS 8
#define NUM_COLS 8 // 11

#define NOTE_ON_CMD 0x90
#define NOTE_OFF_CMD 0x80
#define NOTE_VELOCITY 127
#define CHANNEL 0
// MIDI baud rate
#define SERIAL_RATE 31250

// Pin Definitions

// Row input pins
const int row8Pin = 7;
const int row7Pin = 6;
const int row6Pin = 5;
const int row5Pin = 4;
const int row4Pin = 3;
const int row3Pin = 2;
const int row2Pin = 1;
const int row1Pin = 0;

// 74HC595 pins
const int dataPin = 8;
const int latchPin = 9;
const int clockPin = 10;

boolean keyPressed[NUM_ROWS][NUM_COLS];
uint8_t keyToMidiMap[NUM_ROWS][NUM_COLS];

// bitmasks for scanning columns
int bits[] = {B00000001, B10000000, B01000000, B00100000,
							B00010000, B00001000, B00000100, B00000010};

void scanColumn(int colNum);
void noteOn(int row, int col);
void noteOff(int row, int col);
void noteOnMIDI(byte pitch, byte velocity);
void noteOffMIDI(byte pitch, byte velocity);
void controlChange(byte channel, byte control, byte value);

void setup() {
	int note = 31 - 7;

	for (int colCtr = 0; colCtr < NUM_COLS; ++colCtr) {
		for (int rowCtr = 0; rowCtr < NUM_ROWS; ++rowCtr) {
			keyPressed[rowCtr][colCtr] = false;
			keyToMidiMap[rowCtr][colCtr] = note;
			note++;
		}
	}

	// setup pins output/input mode
	pinMode(dataPin, OUTPUT);
	pinMode(clockPin, OUTPUT);
	pinMode(latchPin, OUTPUT);

	pinMode(row1Pin, INPUT);
	pinMode(row2Pin, INPUT);
	pinMode(row3Pin, INPUT);
	pinMode(row4Pin, INPUT);
	pinMode(row5Pin, INPUT);
	pinMode(row6Pin, INPUT);
	pinMode(row7Pin, INPUT);
	pinMode(row8Pin, INPUT);

	Serial.begin(SERIAL_RATE);
}

void loop() {
	for (int colCtr = 0; colCtr < NUM_COLS; ++colCtr) {
		// scan next column
		scanColumn(colCtr);

		// get row values at this column
		int rowValue[NUM_ROWS];
		rowValue[0] = digitalRead(row1Pin);
		rowValue[1] = digitalRead(row2Pin);
		rowValue[2] = digitalRead(row3Pin);
		rowValue[3] = digitalRead(row4Pin);
		rowValue[4] = digitalRead(row5Pin);
		rowValue[5] = digitalRead(row6Pin);
		rowValue[6] = digitalRead(row7Pin);
		rowValue[7] = digitalRead(row8Pin);

		// process keys pressed
		for (int rowCtr = 0; rowCtr < NUM_ROWS; ++rowCtr) {
			if (rowValue[rowCtr] != 0 && !keyPressed[rowCtr][colCtr]) {
				keyPressed[rowCtr][colCtr] = true;
				noteOn(rowCtr, colCtr);
			}
		}

		// process keys released
		for (int rowCtr = 0; rowCtr < NUM_ROWS; ++rowCtr) {
			if (rowValue[rowCtr] == 0 && keyPressed[rowCtr][colCtr]) {
				keyPressed[rowCtr][colCtr] = false;
				noteOff(rowCtr, colCtr);
			}
		}
	}
}

void scanColumn(int colNum) {
	digitalWrite(latchPin, LOW);

	if (0 <= colNum && colNum <= 7) {
		shiftOut(dataPin, clockPin, MSBFIRST, B00000000);		 // right sr
		shiftOut(dataPin, clockPin, MSBFIRST, bits[colNum]); // left sr
	} else {
		shiftOut(dataPin, clockPin, MSBFIRST, bits[colNum - 8]); // right sr
		shiftOut(dataPin, clockPin, MSBFIRST, B00000000);				 // left sr
	}
	digitalWrite(latchPin, HIGH);
}

void noteOn(int row, int col) {
	Serial.write(NOTE_ON_CMD);
	Serial.write(keyToMidiMap[row][col]);
	Serial.write(NOTE_VELOCITY);

	noteOnMIDI(keyToMidiMap[row][col], NOTE_VELOCITY); // Nueva Linea Leonardo
	MidiUSB.flush();																	 // Nueva Linea Leonardo
}

void noteOff(int row, int col) {
	Serial.write(NOTE_OFF_CMD);
	Serial.write(keyToMidiMap[row][col]);
	Serial.write(NOTE_VELOCITY);

	noteOffMIDI(keyToMidiMap[row][col],
							NOTE_VELOCITY); // Channel 0, middle C, normal velocity
	MidiUSB.flush();
}

//-------------------------
void noteOnMIDI(byte pitch, byte velocity) {
	midiEventPacket_t noteOn = {0x09, 0x90 | CHANNEL, pitch, velocity};
	MidiUSB.sendMIDI(noteOn);
}

void noteOffMIDI(byte pitch, byte velocity) {
	midiEventPacket_t noteOff = {0x08, 0x80 | CHANNEL, pitch, velocity};
	MidiUSB.sendMIDI(noteOff);
}

void controlChange(byte control, byte value) {
	midiEventPacket_t event = {0x0B, 0xB0 | CHANNEL, control, value};
	MidiUSB.sendMIDI(event);
}
