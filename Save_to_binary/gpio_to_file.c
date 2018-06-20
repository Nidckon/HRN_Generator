#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdlib.h>
#include <stdio.h>
#include <wiringPi.h>
#include <stdbool.h>

#define OUTPUT_PIN 4
#define INPUT_PIN 17

// number of bytes to read from GPIO and save to file
int bytes_count = 0;
// delay between reads from GPIO in microseconds
int read_delay_us = 0;

// to parse app arguments
int parseInput(int argc, char* argv[]) {
	if (argc < 4) {
		printf("Missing arguments, required number of bytes to save, freq of bit reads and output filename\n");
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
	return 0;
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
	pinMode(INPUT_PIN, INPUT); 
	pullUpDnControl(INPUT_PIN, PUD_DOWN);
	
	// logic state from GPIO input
	bool readed_data = false;
	// byte number to save in binary file
	unsigned short number_to_save = 0;
	// new bit position of bytes_to_save
	unsigned char number_shl = 0;
	// size of number (bytes)
	const int NUMBER_SIZE = sizeof(unsigned short);
	// size of number (bits)
	const int NUMBER_BIT_SIZE = NUMBER_SIZE * 8;
	// buffer to rename number to char table
	char charBuffer[18];
	int short_count = 0;
	
	/* We used two byte number to save, because fwrite writes two bytes 
	 * word when bytes are in little endian sequential order. First one 
	 * is less significant than second. Two byte in one fwrite resolve 
	 * the problem.
	 * Example:
	 * 		to save: 		FF F0
	 * 		one byte write:	F0 FF
	 * 		two byte write: FF F0
	 * */
	
	// binary file to save numbers from GPIO
	FILE *f;
	f = fopen(argv[3], "wb");
	
	while (bytes_count > 0) {
		readed_data = digitalRead(INPUT_PIN);
		// add logic state at right position
		number_to_save |= readed_data << (NUMBER_BIT_SIZE - number_shl++ - 1);
		// when all bits in number are saved
		if (number_shl >= NUMBER_BIT_SIZE) {
			charShort(charBuffer, number_to_save);
			printf("%d: %s\n", short_count++, charBuffer);
			fwrite(&number_to_save, 1, NUMBER_SIZE, f);
			number_shl = 0;
			number_to_save = 0;
			bytes_count-=2;
			
		}
		usleep(read_delay_us);
	}
	fclose(f);
	return 0 ;
}
