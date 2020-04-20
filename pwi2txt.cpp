#include "pwi2txt.hpp"

#define X__DEBUGLOG__

bool file_exists(char *fn)
{
  struct stat buffer;
  return (stat(fn, &buffer) == 0);
}

void process_filenames(const char *ifn, const char *ofn)
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
    process_filenames(ifn, ofn);
  }
  return 0;
}
