#ifndef UART_H
#define UART_H

#include <unistd.h> // POSIX standard constants
#include <stdio.h>
#include <string.h>
#include <stdint.h> // for fixed-width integer types
#include <time.h> // for nanosleep()

#include <fcntl.h> // for file control operstions like open()
#include <termios.h>
#include <errno.h>

int configure_uart(const char* device);

#endif /* UART_H */