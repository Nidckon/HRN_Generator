# Saving to binary
Load random data from GPIO and SPI/UART to binary files.

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
| 2 | "lfsr" | 16 | 
| 3 | "a5/1" | 16 |
# Example Execute
```
$ ./gpio_to_file 5120 10000 0 desc -v 
```
Generated filename: "op_amp_desc.bin" with 5 KiB of data.

Parameters:
1. number of bytes to read
2. frequency of one bit read
3. input id
4. (optional) description of filename
5. (optional) with von Neumann correction


# Binary preview of output file
```
$ xxd -b op_amp_desc.bin
```