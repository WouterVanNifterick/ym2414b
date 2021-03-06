// some code grabbed from YM2151 Library, see http://www.ooishoo.org/?page_id=15
// see http://sr4.sakura.ne.jp/fmsound/opz.html for YM2414B register analysis
#define __PROG_TYPES_COMPAT__
#include <avr/pgmspace.h>
#include "opz.h"
#include <SdFat.h>

const uint8_t spiSpeed = SPI_HALF_SPEED;
const uint8_t SD_CHIP_SELECT = SS;

SdFat sd;
SdBaseFile file;

const uint8_t opz_channel_count = 8;

uint8_t midi_channel_volume = 127;
uint8_t midi_channel_number = 1;
uint8_t microtuning = 0;
uint8_t midi_bank = 0;
uint8_t midi_program = 0;

opz_rt_note_t opz_channel[opz_channel_count];

MIDI_CREATE_DEFAULT_INSTANCE();

void handleCC(byte channel, byte cc, byte value)
{
	char dirname[3];

	switch (cc)
	{
	case midi::ChannelVolume:
		midi_channel_volume = value;
		for (uint8_t i = 0; i < opz_channel_count; i++)
		{
			if (opz_channel[i].midi_note > 0)
			{
				modify_running_note(i, opz_channel[i].midi_note, opz_channel[i].midi_velocity, value);
				opz_channel[i].midi_volume = value;
			}
		}
		break;
	case midi::ModulationWheel:
		break;
	case midi::BankSelect:
		sd.chdir();
		sd.chdir("0");
		sd.chdir(itoa(value, dirname, 10));
		break;
	default:
		break;
	}
}

void handleProgramChange(byte channel, byte program)
{
	char filename[3];

	if (file.open(itoa(program, filename, 10)))
	{
		switch (file.fileSize())
		{
		case 84:
			file.read(&amem, 84);
			break;
		default:
			break;
		}
		file.close();
		init_voice();
	}
}

void handlePitchWheel(byte channel, int value)
{

}

void handleNoteOn(byte channel, byte note, byte velocity)
{
	for (uint8_t i = 0; i < opz_channel_count; i++)
	{
		if (opz_channel[i].midi_note == 0)
		{
			set_note(i, note, velocity, midi_channel_volume, microtuning);
			opz_channel[i].midi_note = note;
			opz_channel[i].midi_velocity = velocity;
			opz_channel[i].midi_volume = midi_channel_volume;
			opz_channel[i].microtuning = microtuning;
			break;
		}
	}
}

void handleNoteOff(byte channel, byte note, byte velocity)
{
	for (uint8_t i = 0; i < opz_channel_count; i++)
	{
		if (opz_channel[i].midi_note == note)
		{
			unset_note(i);
			opz_channel[i].midi_note = 0;
			break;
		}
	}
}

void setup()
{
	char dirname[3];

	YM_CTRL_DDR |= _BV(YM_CS) | _BV(YM_RD) | _BV(YM_WR) | _BV(YM_A0);	// output mode for control pins
	//YM_DATA_DDR = 0xff;													// output mode for data bus pins
	DDRD |= B11111100;													// pins 2-7 output
	DDRB |= B00000011;													// pins 8-9 output
	YM_CTRL_PORT |= _BV(YM_WR) | _BV(YM_RD); 							// WR and RD high by default

	// reset YM
	/*
	digitalWrite(pinIC, LOW);
	delay(100);
	digitalWrite(pinIC, HIGH);
	*/

	//Serial.begin(9600);
	//Serial.println("YM2414B demo");

	delay(1000);

	// setup as TZ81Z does during poweron

	setreg(0x09, 0x00);
	setreg(0x0f, 0x00);
	setreg(0x1c, 0x00);
	setreg(0x1e, 0x00);
	setreg(0x0a, 0x04);
	setreg(0x14, 0x70);
	setreg(0x15, 0x01);

	//load_patch(0);

	for (uint8_t j = 0; j < 8; j++)
	{
		setreg(0x08, 0x00 + j);	// Key OFF channel j
	}

	MIDI.setHandleNoteOn(handleNoteOn);
	MIDI.setHandleNoteOff(handleNoteOff);
	MIDI.setHandleControlChange(handleCC);
	MIDI.setHandlePitchBend(handlePitchWheel);
	MIDI.setHandleProgramChange(handleProgramChange);
	MIDI.begin(midi_channel_number);

	if (!sd.begin(SD_CHIP_SELECT, SPI_HALF_SPEED))
	{
		sd.initErrorHalt();
	}

	if (sd.chdir("0"))
	{
		sd.chdir(itoa(midi_bank, dirname, 10));
		handleProgramChange(midi_channel_number, midi_program);
		init_voice();
	}
}

void loop()
{
	MIDI.read();
	//setreg(0x1b, 0x00);
}