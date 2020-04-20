#include "pwi2txt.hpp"

void get_markers(const CHAR *buf, unsigned int &m2, unsigned int &m3)
{
  m2 = (buf[0]<<8)|buf[1];
  m3 = (m2<<8)|buf[2];
}

unsigned int align4(unsigned int x)
{
  return (x+3)&((unsigned int)-1^3);
}

// class PWILine ///////////////////////////////////////////////////////////////

PWILine::PWILine(const CHAR *buf, unsigned int length)
{
  m_data = buf;
  m_len = length;
  m_line = "";
}

PWILine::~PWILine()
{
}

const char *PWILine::decode(void)
{
  int i;
  for (i=0; i<m_len; ) {
    unsigned int m2, m3;
    get_markers(m_data+i, m2, m3);
    if (m3==FORMAT1_ON || m3==FORMAT2_ON || m3==FORMAT1_OFF || m3==FORMAT2_OFF) {
      // маркер формата игнорируем
      i+=3;
    }
    else if (m2==0xC1B0) {
      // знак градуса
      m_line += "\xC2\xB0";
      i+=2;
    }
    else if (m2==0xAC82) {
      // знак евро
      m_line += "\xE2\x82\xAC";
      i+=2;
    }
    else if (m2==0xC404) {
      // TAB?
      m_line += "\t";
      i+=2;
    }
    else if (m_data[i]==0xC1) {
      // умляуты
      m_line += m_data[i++]|0x02; // ставим 1 в бит 1:   00000010 = 0x02
      m_line += m_data[i++]&0xBF; // обнуляем бит 6:     10111111 = 0xBF
    }
    else if (m_data[i]>=128) {
      CHAR b1 = m_data[i++];
      // из 0x10 делаем 0xD0, из 0x11 делаем 0xD1, как в UTF-8
      CHAR b2 = m_data[i++]|0xC0;
      // и меняем порядок байтов
      m_line += b2;
      m_line += b1;
    }
    else
      m_line += m_data[i++];
  }
  return m_line.c_str();
}

// class PWIBuffer /////////////////////////////////////////////////////////////

PWIBuffer::PWIBuffer(off_t length)
{
  m_len = length;
  m_data = new CHAR[length];
  m_pos = m_next = 0;
}

PWIBuffer::~PWIBuffer()
{
  delete[] m_data;
}

/* Ищет конец строки: либо маркер конца строки, либо нулевой байт вне маркера формата
 */
unsigned int PWIBuffer::find_eol(void)
{
  unsigned int i = m_pos;
  while (i<m_len-1) {
    if (m_data[i]==0xC4 && m_data[i+1]==0x00) return i;
    if (m_data[i]==0x00) return i;
    if (m_data[i]==0xE5 || m_data[i]==0xE6) i+=3; // пропускаем маркер формата
    else i++;
  }
  return i;
}

bool PWIBuffer::find_line(void)
{
  bool rv = false;
  for (m_pos = m_next; m_pos<m_len; m_pos+=4) {
    unsigned int m2, m3;
    get_markers(m_data+m_pos, m2, m3);
    if (m3==FORMAT1_ON || m3==FORMAT2_ON || m3==FORMAT1_OFF || m3==FORMAT2_OFF) {
      unsigned int epos = find_eol();
      // m_pos по-прежнему указывает на маркер формата
      // epos указывает либо на C4 00, либо на 00
      if (m_data[epos]) { // в первом случае это правильная строка
        m_next = align4(epos+3);
        rv = true;
        break;
      } else {  // а если указывает на 00, то продолжаем искать следующую
                // строку с позиции epos
        m_pos = align4(epos+3)-4;
      }
    }
  }
  return rv;
}

const CHAR *PWIBuffer::line(void)
{
  return m_data+m_pos;
}

/* -1, если не найден конец строки */
unsigned int PWIBuffer::linesize(void)
{
  unsigned int i;
  for (i=m_pos; i<m_len-1; i++) {
    if (m_data[i]==0xC4 && m_data[i+1]==0x00) break;
  }
  return i-m_pos;
}

// class PWIFile ///////////////////////////////////////////////////////////////

PWIFile::PWIFile(const char *filename)
{
  m_errmsg = "";
  m_filename = filename;
  m_stream = nullptr;
  m_text = "";
};

PWIFile::~PWIFile()
{
  if (m_stream) delete m_stream;
}

off_t PWIFile::filesize(void)
{
  struct stat buffer;
  stat(m_filename.c_str(), &buffer);
  return buffer.st_size;
}

bool PWIFile::open(void)
{
  m_stream = new ifstream(m_filename.c_str(), ios::in | ios::binary);
  return m_stream->good();
}

bool PWIFile::read(void)
{
  off_t flen = filesize();
  char header[PWI_HEADER_SIZE];
  m_stream->read(header, sizeof(header)); flen -= PWI_HEADER_SIZE;
  PWIBuffer buf(flen);
  if (read_buffer(buf)) {
    while (buf.find_line()) {
      PWILine line(buf.line(), buf.linesize());
      m_lines.push_back(line.decode());
    }
  } else {
    m_errmsg = "reading buffer failed: ";
    m_errmsg += buf.error_message();
  }
  return m_stream->good();
}

bool PWIFile::read_buffer(PWIBuffer &buf)
{
  m_stream->read((char*)buf.m_data, buf.m_len);
  return m_stream->good();
}

const string& PWIFile::text(void)
{
  m_text = "";
  for (vector<string>::iterator i=m_lines.begin(); i!=m_lines.end(); i++) {
    m_text += *i;
    m_text += "\n";
  }
  return m_text;
}

// class PWIResultFile /////////////////////////////////////////////////////////

PWIResultFile::PWIResultFile(const char *filename)
{
  m_errmsg = "";
  m_filename = filename? filename : "";
  m_stream = nullptr;
#ifdef __WATCOMC__
  m_iobuf = nullptr;
#endif
}

PWIResultFile::~PWIResultFile()
{
  if (m_stream) delete m_stream;
#ifdef __WATCOMC__
  if (m_iobuf) delete[] m_iobuf;
#endif
}

bool PWIResultFile::open(void)
{
  if (m_filename.length()!=0)
    m_stream = new ofstream(m_filename.c_str());
  else {
#ifdef __WATCOMC__
    m_iobuf = new char[1024];
    m_stream = new ofstream(stdout->_handle, m_iobuf, 1024);
#else
    m_stream = new ofstream();
    m_stream->basic_ios<char>::rdbuf(cout.rdbuf());
#endif
  }
  return m_stream->good();
}

bool PWIResultFile::write(PWIFile &f)
{
  string header, footer;
  if (m_filename.length()==0) { // cout
    // печатаем имя входного файла
    header = "File "+f.m_filename+"\n";
    footer = "\n";
  } else {  // обычный файл
    header = "";
    footer = "";
  }
  string text(header);
  text += f.text();
  text += footer;
  m_stream->write(text.c_str(), text.length());
  return true;
}
