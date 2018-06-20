#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdlib.h>
#include <stdio.h>
#include <wiringPi.h>
#include <stdbool.h>
#include <string.h>


const unsigned char INPUT_PINS[] = { 21, 20, 16, 16 };
const char *NAMES[] = { "op_amp", "sine", "lfsr", "a5/1"};

// number of bytes to read from GPIO and save to file
int bytes_count = 0;
// delay between reads from GPIO in microseconds
int read_delay_us = 0;
// flag of von Neumann Correction
bool v_correction = false;

int input_number = 0;
char filename[64] = {0};

// to parse app arguments
int parseInput(int argc, char* argv[]) {
	if (argc < 4) {
		printf("Missing arguments, required number of bytes to save, freq of bit reads, input id and output filename parameters\n");
		return -1;
	}
	int temp = atoi(argv[1]);
	if (temp > 0) {
		if (temp % 2) {
			printf("Bytes number not even, added one\n");
		}
		bytes_count = temp;
	}
	else {
		printf("Number of bytes to save must be greater than 0\n");
		return -2;
	}
	temp = atoi(argv[2]);
	
	if (temp > 0)
		read_delay_us = 1000000 / temp;
	else {
		printf("Freq of read must be greater than 0\n");
		return -3;
	}
	temp = atoi(argv[3]);
	if (temp >= 0 && temp <= 3)
		input_number = temp;
		
	else {
		printf("Uncorrect input id\n");
		return -4;
	}
	
	snprintf(filename, 64, "%s.bin", NAMES[input_number]);
	if (argc > 4) {
		if (strcmp(argv[4], "-v") == 0) {
			v_correction = true;
			
		} else {
			snprintf(filename, 64, "%s_%s.bin", NAMES[input_number], argv[4]);
			if (argc > 5) {
				if (strcmp(argv[5], "-v") == 0) {
					v_correction = true;
				}
			}
		}
	} 
	return 0;
}

// parse two byte number to char number, buffer pointer size must be 
// greater than 8
void charByte(char *buffer, unsigned char number) {
	buffer[8] = 0;
	for (int i = 7; i >= 0; i--) {
		buffer[i] = number % 2 ? '1' : '0';
		number >>= 1;
	}
}
// parse two byte number to char number, buffer pointer size must be 
// greater than 17
void charShort(char *buffer, unsigned short number) {
	buffer[17] = 0;
	for (int i = 16; i >= 9; i--) {
		buffer[i] = number % 2 ? '1' : '0';
		number >>= 1;
	}
	buffer[8] = ' ';
	for (int i = 7; i >= 0; i--) {
		buffer[i] = number % 2 ? '1' : '0';
		number >>= 1;
	}
}


int main(int argc, char* argv[]) {
	// when error in parsing
	if (parseInput(argc, argv) < 0) {
		return 1;
	}
	
	// when error in GPIO init
	if (wiringPiSetupGpio () == -1) {
		printf("Error on GPIO init\n");
		return 1;
	}
	 
	// setting pinmodes and pull down resistor to input
	//pinMode(OUTPUT_PIN, OUTPUT); // not used 
	pinMode(INPUT_PINS[input_number], INPUT); 
	pullUpDnControl(INPUT_PINS[input_number], PUD_DOWN);
	
	// logic state from GPIO input
	bool readed_data = false;
	// byte number to save in binary file
	unsigned char number_to_save = 0;
	// new bit position of bytes_to_save
	unsigned char number_shl = 0;
	// size of number (bytes)
	const int NUMBER_SIZE = sizeof(unsigned char);
	// size of number (bits)
	const int NUMBER_BIT_SIZE = NUMBER_SIZE * 8;
	// buffer to rename number to char table
	char charBuffer[NUMBER_BIT_SIZE + 1];
	// count of printing numbers
	int short_count = 0;
	// position in the v_table 
	unsigned char v_position = 0;
	// table of bits of von Neumann correction
	unsigned char v_table[2] = { 0, 0 };
	// skip write result
	bool v_skip = false;
	
	// binary file to save numbers from GPIO
	FILE *f;
	f = fopen(filename, "wb");
	
	while (bytes_count > 0) {
		// von Neumann correction
		if (v_correction) {
			v_skip = true;
			v_table[v_position++] = digitalRead(INPUT_PINS[input_number]);
			if (v_position >= 2) {
				if (v_table[0] != v_table[1]) {
					readed_data = v_table[0];
					v_skip = false;
				} 
				v_position = 0;
			} 
			if (v_skip) {
				usleep(read_delay_us);
				continue;
			}
			
		} else {
			readed_data = digitalRead(INPUT_PINS[input_number]);
		}
		// add logic state at right position
		number_to_save |= readed_data << (NUMBER_BIT_SIZE - number_shl++ - 1);
		// when all bits in number are saved
		if (number_shl >= NUMBER_BIT_SIZE) {
			charByte(charBuffer, number_to_save);
			printf("%d: %s\n", short_count++, charBuffer);
			fwrite(&number_to_save, 1, NUMBER_SIZE, f);
			number_shl = 0;
			number_to_save = 0;
			bytes_count--;
			
		}
		usleep(read_delay_us);
	}
	fclose(f);
	return 0 ;
}
