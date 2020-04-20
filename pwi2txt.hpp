#ifndef __PWI2TXT_HPP_INCLUDED
#define __PWI2TXT_HPP_INCLUDED

#include <sys/stat.h>
#include <vector> 
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

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

class PWIFile {
  string m_errmsg;
  string m_filename;
  ifstream *m_stream;
  vector<string> m_lines;
  string m_text;
  friend class PWIResultFile;
public:
  PWIFile(const char *filename);
  ~PWIFile();
  bool open(void);
  bool read(void);
  const char *error_message(void) {return m_filename.c_str();}
  const string& text(void);
};

class PWIResultFile {
  string m_errmsg;
  string m_filename;
  ofstream *m_stream;
public:
  PWIResultFile(const char *filename);
  ~PWIResultFile();
  bool open(void);
  bool write(PWIFile &f);
  const char *error_message(void) {return m_errmsg.c_str();}
};

#endif
