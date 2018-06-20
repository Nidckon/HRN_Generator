# Saving to binary
Load random data from GPIO and SPI/UART to binary files.

# Commands to install wiringPi required to compile program

```
$ wget https://unicorn.drogon.net/wiringpi-2.46-1.deb
$ sudo dpkg -i wiringpi-2.46-1.deb
```
# To compile the program

```
$ gcc -Wall gpio_to_file.c -o gpio_to_file -lwiringPi
```
