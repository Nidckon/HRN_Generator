#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdlib.h>
#include <stdio.h>
#include <wiringPi.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

/* Synopsis
 * gpio_to_file number_of_bytes read_frequency input_id [-d file_description] [-v] [-db] [-KB / -MB / -GB]*/

// pin id of serial transmission
#define RS_232 232
#define TEXT_SIZE 64
#define MAX_GB 2

const unsigned char INPUT_PINS[] = { 21, 20, RS_232, RS_232 };
const char *NAMES[] = { "op_amp", "sine", "lfsr", "a5/1"};

// initial number of bytes to read from GPIO and save to file
unsigned long bytes_number = 0;
// delay between reads from GPIO in microseconds
int read_delay_us = 0;
// flag of von Neumann Correction
bool v_correction = false;
// statement mode
bool debug_print = false;
// input identifier
int input_id = 0;
// filename text buffer
char filename[TEXT_SIZE] = {0};
// HH:mm:ss text buffer
char time_buffer[TEXT_SIZE] = {0};
// to measure time
time_t start_t, actual_t;

// print input parameters, config of program
void printParams() {
	if (bytes_number > 1024) {
		if (bytes_number > 1024 * 1024) {
			if (bytes_number > 1024*1024 * 1024) {
				printf("number of data to read: %.03f GiB, %lu bytes\n", bytes_number / 1024.0 / 1024 / 1024, bytes_number);
			} else {
				printf("number of data to read: %.03f MiB, %lu bytes\n", bytes_number / 1024.0 / 1024, bytes_number);
			}
		} else {
			printf("number of data to read: %.03f KiB, %lu bytes\n", bytes_number / 1024.0, bytes_number);
		}
	} else {
		printf("number of data to read: %lu B\n", bytes_number);
	}
	printf("one bit read frequency: %d Hz\n", 1000000/read_delay_us);
	printf("input name: %s\n", NAMES[input_id]);
	printf("filename: %s\n", filename);
	if (v_correction)
		printf("von Neumann correction: true\n");
	else
		printf("von Neumann correction: false\n");		
}

// to parse app arguments
int parseInput(int argc, char* argv[]) {
	if (argc < 4) {
		printf("Missing arguments, required number of bytes to save, freq of bit reads, input id and output filename parameters\n");
		return -1;
	}
	int temp = atoi(argv[1]);
	if (temp > 0) {
		bytes_number = temp;
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
		input_id = temp;
		
	else {
		printf("Uncorrect input id\n");
		return -4;
	}
	bool too_big = false;
	snprintf(filename, TEXT_SIZE, "%s.bin", NAMES[input_id]);
	for (int i = 4; i < argc; i++) {
		if (strcmp(argv[i], "-d") == 0) {
			if (i < (argc - 1)) {
				snprintf(filename, TEXT_SIZE, "%s_%s.bin", NAMES[input_id], argv[i+1]);
			} else {
				printf("Missing argument after -d parameter\n");	
			}
		}
		if (strcmp(argv[i], "-v") == 0) {
			v_correction = true;
		}
		if (strcmp(argv[i], "-db") == 0) {
			debug_print = true;
		}
		if (strcmp(argv[i], "-KB") == 0) {
			too_big = bytes_number > MAX_GB * 1024 * 1024 ? true : false;
			bytes_number *= 1024;
		} else {
			if (strcmp(argv[i], "-MB") == 0) {
				too_big = bytes_number > MAX_GB * 1024 ? true : false;
				bytes_number *= 1024 * 1024;
			} else {
				if (strcmp(argv[i], "-GB") == 0) {
					too_big = bytes_number > MAX_GB ? true : false;
					bytes_number *= 1024 * 1024 * 1024;
				}
			}
		}
	}
	if (too_big) {
		printf("To much data to save, limit is 2 GiB\n");
		exit(1);
	}
	if (debug_print) {
		printParams();
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

// insert to text buffer time period in seconds
void secondsToTime(char *buffer, int bufferLength, long seconds) {
	int sec = seconds % 60;
	int minutes = (seconds % 3600) / 60;
	int hours =  (seconds % (3600 * 60)) / 3600;
	if (hours > 0)
		snprintf(buffer, bufferLength, "%d:%02d:%02d", hours, minutes, sec);
	else
		snprintf(buffer, bufferLength, "%02d:%02d", minutes, sec);

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
	pinMode(INPUT_PINS[input_id], INPUT); 
	pullUpDnControl(INPUT_PINS[input_id], PUD_DOWN);
	
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
	unsigned long number_count = 0;
	// position in the v_table 
	unsigned char v_position = 0;
	// table of bits of von Neumann correction
	unsigned char v_table[2] = { 0, 0 };
	// skip write result
	bool v_skip = false;
	// to refresh progress
	long last_seconds = 0;
	// binary file to save numbers from GPIO
	FILE *f;
	
	f = fopen(filename, "wb");
	start_t = time(NULL);
	while (number_count < bytes_number) {
		// von Neumann correction
		if (v_correction) {
			v_skip = true;
			v_table[v_position++] = digitalRead(INPUT_PINS[input_id]);
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
			readed_data = digitalRead(INPUT_PINS[input_id]);
		}
		// add logic state at right position
		number_to_save |= readed_data << (NUMBER_BIT_SIZE - number_shl++ - 1);
		// when all bits in number are saved
		if (number_shl >= NUMBER_BIT_SIZE) {
			charByte(charBuffer, number_to_save);
			number_count++;
			if (debug_print) {
				printf("%lu of %lu: %s\n", number_count, bytes_number, charBuffer);
			} else {
				float percent = (float)number_count / bytes_number;
				actual_t = time(NULL);
				if (last_seconds != difftime(actual_t, start_t)) {
					last_seconds = difftime(actual_t, start_t);
					long seconds = (difftime(actual_t, start_t)) / percent - difftime(actual_t, start_t);
					secondsToTime(time_buffer, TEXT_SIZE, seconds);
					system("clear");
					
					printf("\033[1;32m"); // green color
					printf("gpio_to_file\n");
					printf("\033[0m");  // default color
					
					printParams();
					printf("%.2f%% remained %s\n", percent * 100, time_buffer);
				}
			
			}
			fwrite(&number_to_save, 1, NUMBER_SIZE, f);
			number_shl = 0;
			number_to_save = 0;
			
		}
		usleep(read_delay_us);
	}
	fclose(f);
	return 0 ;
}
