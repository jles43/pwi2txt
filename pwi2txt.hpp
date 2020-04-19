#ifndef __PWI2TXT_HPP_INCLUDED
#define __PWI2TXT_HPP_INCLUDED

#include <sys/stat.h>
#include <string.h>
#include <iostream>
#include <fstream>

// метки формата записаны в обратном порядке байтов
// так удобнее считывать из буфера, и последовательность байтов в unsigned long
// совпадает с последовательностью байтов в буфере
#define FORMAT1_ON  0xe50100
#define FORMAT1_OFF 0xe50000
#define FORMAT2_ON  0xe60a00
#define FORMAT2_OFF 0xe60000
#define LINE_END    0xc400
#define LINE_FORMAT 0x4200

typedef unsigned char CHAR;

#endif
