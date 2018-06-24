# Saving to binary
Load random data from GPIO and probably RS_232 to binary files.

# Commands to install wiringPi required to compile program

```
$ cd /tmp
$ wget https://unicorn.drogon.net/wiringpi-2.46-1.deb
$ sudo dpkg -i wiringpi-2.46-1.deb
```
# Compile the program

```
$ gcc -Wall gpio_to_file.c -o gpio_to_file -lwiringPi
```
# Input Id's
| id | Name | BCM pin |
| -- | ---- | ------- |
| 0 | "op_amp" | 21 |
| 1| "sine" | 20 |
| 2 | "lfsr" | RS_232 | 
| 3 | "a5/1" | RS_232 |
# Synopsis
**gpio_to_file** number_of_bytes read_frequency input_id [-d file_description] [-v] [-db] [-KB / -MB / -GB]
# Options
- **number_of_bytes** - number of bytes to read
- **read_frequency** - frequency of one bit read
- **input id**
- **-d description** - add description to filename
- **-v** - enable von Neumann Correction
- **-db** - printing all bytes (debug mode), without percent progress
- **KB** - KiB multiplier of **number_of_bytes**
- **MB** - MiB multiplier of **number_of_bytes**
- **GB** - GiB multiplier of **number_of_bytes**
Max value of multiplied number of bytes is 2 GiB. Algorithm always load the smallest multiplier.

# Binary preview of output file
```
$ xxd -b op_amp_desc.bin
```