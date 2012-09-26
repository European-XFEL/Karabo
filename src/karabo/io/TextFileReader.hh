/*
 * $Id: TextFileReader.hh 4644 2011-11-04 16:04:36Z heisenb@DESY.DE $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 16, 2011, 8:49 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_IO_TEXTFILEREADER_HH
#define	KARABO_IO_TEXTFILEREADER_HH

#include <iosfwd>
#include <fstream>
#include <sstream>
#include <boost/filesystem.hpp>

#include <karabo/util/Factory.hh>

#include "Reader.hh"
#include "Format.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

  /**
   * Namespace for package io
   */
  namespace io {

    template <class Tdata>
    class TextFileReader : public Reader<Tdata> {
    public:

      KARABO_CLASSINFO(TextFileReader<Tdata>, "TextFile", "1.0")

      typedef boost::shared_ptr<Format<Tdata> > FormatPointer;
      typedef karabo::util::Factory<Format<Tdata> > FormatFactory;

      TextFileReader() {
      }

      TextFileReader(const std::string& filename, const FormatPointer& format) :
      m_filename(filename), m_format(format) {
        if (m_format == 0) {
          guessAndSetFormat();
        }
      }

     static void expectedParameters(karabo::util::Schema& expected) {

        using namespace karabo::util;

        PATH_ELEMENT(expected).key("filename")
                .description("Name of the file to be read")
                .displayedName("Filename")
                .assignmentMandatory()
                .commit();

        CHOICE_ELEMENT<Format<Tdata> >(expected).key("format")
                .displayedName("Format")
                .description("Select the format which should be used to interprete the data")
                .assignmentOptional().noDefaultValue()
                .commit();
      }

   
      void configure(const karabo::util::Hash& input) {
        input.get("filename", m_filename);
        if (input.has("format")) {
          m_format = Format<Tdata>::createChoice("format", input);
        } else {
          guessAndSetFormat();
        }
      }

      void guessAndSetFormat() {

        using namespace std;
        using namespace karabo::util;

        vector<string> keys = FormatFactory::getRegisteredKeys();
        string extension = m_filename.extension().string().substr(1);
        boost::to_lower(extension);

        BOOST_FOREACH(string key, keys) {
          string lKey(key);
          boost::to_lower(lKey);
          if (lKey == extension) {
            Hash c;
            c.set(key, Hash());
            m_format = Format<Tdata>::create(c);
            return;
          }
        }
        throw NOT_SUPPORTED_EXCEPTION("Can not interprete extension: \"" + extension + "\"");
      }

      void readFile(std::stringstream& buffer) {

        using namespace std;

        string line;
        ifstream inputStream(m_filename.string().c_str());
        if (inputStream.is_open()) {
          while (!inputStream.eof()) {
            getline(inputStream, line);
          buffer << line << endl;
          }
          inputStream.close();
        } else {
          throw IO_EXCEPTION("Cannot open file: " + m_filename.string());
        }
      }
   
      void read(Tdata& data) {
        std::stringstream buffer;
        readFile(buffer);
        m_format->convert(buffer, data);
      }

    private:

      boost::filesystem::path m_filename;
      FormatPointer m_format;
    };
  } // namespace io
} // namespace karabo

#endif	/* KARABO_IO_TEXTFILEREADER_HH */
