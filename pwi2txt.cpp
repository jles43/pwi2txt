#include "pwi2txt.hpp"

#include <string.h>

#define X__DEBUGLOG__

#ifdef __DEBUGLOG__
char *hex(CHAR *buf, int len) {
  static char out[128];
  static char h[]="0123456789abcdef";
  if (len>41) len=41;
  int j=0;
  for (int i=0; i<len; i++) {
    if (i) out[j++]=' ';
    out[j++]=h[(buf[i]>>4)&0xF];
    out[j++]=h[buf[i]&0xF];
  }
  out[j]='\0';
  return out;
};
#endif

bool file_exists(char *fn)
{
  struct stat buffer;
  return (stat(fn, &buffer) == 0);
}

/* n=1..3 */
unsigned int get_mark(CHAR *p, int n)
{
#ifdef __DEBUGLOG__
  cout << "> get_mark(p="<<(void*)p<<", n="<<n<<"), *p=["<<hex(p, 3)<<"]"<<endl;
#endif
  unsigned int rv = *p++;
  if (--n>0) rv = (rv << 8) | *p++;
  if (--n>0) rv = (rv << 8) | *p++;
#ifdef __DEBUGLOG__
  cout << "< get_mark returns "<<(void*)(unsigned long)rv<<endl;
#endif
  return rv;
}

bool begin_of_line(CHAR *p)
{
#ifdef __DEBUGLOG__
  cout << "> begin_of_line(p="<<(void*)p<<")"<<endl;
#endif
  unsigned int mark = get_mark(p, 3);
  bool rv = mark==FORMAT1_ON || mark==FORMAT2_ON;
#ifdef __DEBUGLOG__
  cout << "< begin_of_line returns "<<rv<<endl;
#endif
  return rv;
}

bool valid_line(CHAR *p)
{
  bool rv;
  unsigned int mark;
  // пропускаем все метки формата
  do {
    mark = get_mark(p, 3);
    p += 3;
  } while (mark==FORMAT1_ON || mark==FORMAT1_OFF || mark==FORMAT2_ON || mark==FORMAT2_OFF);
  rv = *p!='\0';  // если после всех меток формата у нас ненулевой байт,
                  // то строка годная
  return rv;
}

bool end_of_line(CHAR *p)
{
#ifdef __DEBUGLOG__
  cout << "> end_of_line(p="<<(void*)p<<")"<<endl;
#endif
  bool rv = get_mark(p, 2)==LINE_END;
#ifdef __DEBUGLOG__
  cout << "< end_of_line returns "<<rv<<endl;
#endif
  return rv;
}

bool find_line(CHAR *buf, off_t &pos, off_t len)
{
#ifdef __DEBUGLOG__
  cout << "> find_line(buf="<< (void *)buf << ", pos=" << pos << ", len="<<len<<")"<<endl;
#endif
  bool rv = false;
  while (pos<len) {
    if (begin_of_line(buf+pos)) {
      if (valid_line(buf+pos)) {
        rv = true;
        break;
      }
    }
    pos += 4;
  }
#ifdef __DEBUGLOG__
  cout << "< find_line returns "<<rv<<", pos="<<pos<<endl;
#endif
  return rv;
}

CHAR *get_line(CHAR *buf, off_t &pos, off_t len)
{
#ifdef __DEBUGLOG__
  cout << "> get_line(buf="<<(void*)buf<<", pos="<<pos<<", len="<<len<<")"<<endl;
  cout << "+ get_line: buf=["<<hex(buf+pos, 40)<<"]"<<endl;
#endif
  CHAR *rv;
  unsigned long slen=0;
  while (pos+slen<len) {
    if (end_of_line(buf+pos+slen)) break;
    slen++;
  }
#ifdef __DEBUGLOG__
  cout << "  get_line: slen="<<slen<<endl;
#endif
  rv = new CHAR[slen+1];
  if (slen>0)
    memcpy(rv, buf+pos, slen);
  rv[slen]='\0';
  // сигнатура конца строки = c4 00, потом 00 или (очень редко) другой байт
  pos += slen+3;  // поэтому можно прибавить сразу 3
  // теперь выравниваем на 4 байта
  pos = (pos+3)&((off_t)-1^3);
#ifdef __DEBUGLOG__
  cout << "< get_line returns \""<<rv<<"\" ["<<hex(rv,slen+1)<<"]"<<endl;
#endif
  return rv;
}

/* мы можем строку декодировать на месте, потому что кириллица переводится
 * 2 байта в 2 байта
 */
void decode_line(CHAR *line)
{
#ifdef __DEBUGLOG__
  cout << "> decode_line(line=["<<hex(line, strlen((char*)line))<<"]"<<endl;
#endif
  int i = 0, j = 0;
  while (line[i]) {
#ifdef __DEBUGLOG__
    cout << "  decode_line: line["<<i<<"]="<<hex(line+i, 1);
    cout << ", line["<<i+1<<"]="<<hex(line+i+1, 1)<<endl;
#endif
    if (line[i]==0xE5 || line[i]==0xE6) { // метка формата
      i+=3; // игнорируем её
    }
    else if (line[i]==0xC1 && line[i+1]==0xB0) {  // знак градуса
#ifdef __DEBUGLOG__
      cout << "  decode_line: DEGREE"<<endl;
#endif
      line[j++]=0xC2;
      line[j++]=0xB0;
      i+=2;
    }
    else if (line[i]==0xAC && line[i+1]==0x82) {  // знак Евро
#ifdef __DEBUGLOG__
      cout << "  decode_line: EURO"<<endl;
#endif
      // поскольку в начале строки всегда есть метка формата, то
      // у нас есть место, куда копировать. Если, конечно, в строке не 20
      // знаков Евро. Хотя, конечно, ситуация заставляет переписать
      // функцию.
      line[j++]=0xE2;
      line[j++]=0x82;
      line[j++]=0xAC;
      i+=2;
    }
    else if (line[i]==0xC4 && line[i+1]==0x04) {  // tab?
#ifdef __DEBUGLOG__
      cout << "  decode_line: TAB"<<endl;
#endif
      line[j++]='\t';
      i+=2;
    }
    else if (line[i]==0xC1) {  // умляуты
#ifdef __DEBUGLOG__
      cout << "  decode_line: Umlaute"<<endl;
#endif
      CHAR b1=line[i++]|0x02;   // ставим 1 в бит 1:   00000010 = 0x02
      CHAR b2=line[i++]&0xBF;   // обнуляем бит 6: 10111111 = 0xBF
#ifdef __DEBUGLOG__
      cout << "  decode_line: b1="<<hex(&b1, 1);
      cout << ", b2="<<hex(&b2, 1)<<endl;
#endif
      line[j++]=b1;
      line[j++]=b2; 
    }
    else if (line[i]>=128) {
#ifdef __DEBUGLOG__
      cout << "  decode_line: Cyrillic?"<<endl;
#endif
      // первый байт не изменяем
      CHAR b1 = line[i++];
      // из 0x10 делаем 0xD0, из 0x11 делаем 0xD1, как в UTF-8
      CHAR b2 = line[i++]|0xC0;
      // для UTF-8 меняем порядок 
      line[j++]=b2;    // сначала идёт 0xD0 или 0xD1
      line[j++]=b1;  // а потом специфический байт буквы
    }
    else
      line[j++]=line[i++];
  }
  line[j]='\0';
#ifdef __DEBUGLOG__
  cout << "< decode_line, line=["<<hex(line, strlen((char*)line))<<"]"<<endl;
#endif
}

void process_buffer(ofstream &os, CHAR *buffer, off_t len)
{
  off_t pos = 0;
  // ищем начало строки.
  // оно выравнено на 4 байта и начинается с LINE_START
  while (find_line(buffer, pos, len)) {
    CHAR *line = get_line(buffer, pos, len);
    decode_line(line);
    //cout << "line: " << line << endl;
    os.write((char*)line, strlen((char*)line));
    os.write("\n", 1);
    delete[] line;
  }
}

void process_file(ifstream &is, ofstream &os, off_t flen)
{
#define BUFSIZE 128*1024
#ifdef __DEBUGLOG__
  cout << "> process_file(is="<<&is<<", os="<<&os<<", flen="<<flen<<")"<<endl;
#endif
  char header[0x12c];
  is.read(header, sizeof(header)); flen -= 0x12c;
  CHAR *buffer = new CHAR[flen];
  is.read((char*)buffer, flen);
  process_buffer(os, buffer, flen);
  delete[] buffer;
#ifdef __DEBUGLOG__
  cout << "< process_file" << endl;
#endif
}

void process_filenames(char *ifn, char *ofn)
{
  ifstream *is = new ifstream(ifn, ios::in | ios::binary);
  ofstream *os;
  bool co = ofn==nullptr; // "console output"
  if (co) {
    cout << "File \""<<ifn<<"\""<<endl;
    os = new ofstream();
    os->basic_ios<char>::rdbuf(cout.rdbuf());
  }
  else
    os = new ofstream(ofn);
  struct stat buffer;
  stat(ifn, &buffer);
  process_file(*is, *os, buffer.st_size);
  if (co) cout << endl;
  delete os;
  delete is;
}

void process_filenames2(const char *ifn, const char *ofn)
{
  PWIFile f(ifn);
  if (f.open() && f.read()) {
    PWIResultFile rf(ofn);
    if (rf.open()) {
      rf.write(f);
    } else {
      cerr << "Output file: " << rf.error_message() << endl;
    }
  } else {
    cerr << "Input file:" << f.error_message() << endl;
  }
}

int main(int argc, char *argv[])
{
  int i;
  for (i=1; i<argc; ) {
    char *ifn = argv[i++], *ofn;
    if (i<argc) {
      ofn=argv[i];
      if (!file_exists(ofn))
        i++;
      else
        ofn = nullptr;
    }
    else ofn = nullptr;
#ifdef __DEBUGLOG__
    cout << "input file: \"" << ifn << "\", output file: ";
    if (ofn == nullptr) cout << "stdout";
    else cout << "\"" << ofn << "\"";
    cout << endl;
#endif
    process_filenames2(ifn, ofn);
  }
  return 0;
}
