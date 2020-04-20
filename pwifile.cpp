#include "pwi2txt.hpp"

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

bool PWIFile::open(void)
{
  m_stream = new ifstream(m_filename.c_str(), ios::in | ios::binary);
  return m_stream->good();
}

bool PWIFile::read(void)
{
  return m_stream->good();
}

const string& PWIFile::text(void)
{
  m_text = "";
  for (auto const& line: m_lines) {
    m_text += line;
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
}

PWIResultFile::~PWIResultFile()
{
  if (m_stream) delete m_stream;
}

bool PWIResultFile::open(void)
{
  if (m_filename.length()!=0)
    m_stream = new ofstream(m_filename.c_str());
  else {
    m_stream = new ofstream();
    m_stream->basic_ios<char>::rdbuf(cout.rdbuf());
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
