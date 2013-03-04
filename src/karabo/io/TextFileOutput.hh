/*
 * $Id: TextFileOutput.hh 4951 2012-01-06 12:54:57Z heisenb@DESY.DE $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 16, 2010, 10:18 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_TEXTFILEWRITER_HH
#define	KARABO_IO_TEXTFILEWRITER_HH

#include <iosfwd>
#include <fstream>
#include <sstream>
#include <boost/filesystem.hpp>
#include <karabo/util/Factory.hh>
#include "Output.hh"
#include "Format.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

  /**
   * Namespace for package packageName
   */
  namespace io {

    /**
     * The TextFileOutput class.
     */
    template <class Tdata>
    class TextFileOutput : public Output<Tdata> {
    public:

      KARABO_CLASSINFO(TextFileOutput<Tdata>, "TextFile", "1.0")

      typedef boost::shared_ptr<Format<Tdata> > FormatPointer;

      TextFileOutput() {
      }
      
      TextFileOutput(const std::string& filename, const std::string& writeMode, const FormatPointer& format) :
      m_filename(filename), m_writeMode(writeMode), m_format(format) {
        if (m_format == 0) {
          guessAndSetFormat();
        }
      }

      static void expectedParameters(karabo::util::Schema& expected) {

        using namespace karabo::util;

        PATH_ELEMENT(expected).key("filename")
                .description("Name of the file to be written")
                .displayedName("Filename")
                .assignmentMandatory()
                .commit();

        STRING_ELEMENT(expected).key("writeMode")
                .description("Defines the behaviour in case of already existent file")
                .displayedName("Write Mode")
                .options("abort, truncate, append")
                .assignmentOptional().defaultValue(std::string("truncate"))
                .commit();

        CHOICE_ELEMENT<Format<Tdata> >(expected).key("format")
                .displayedName("Format")
                .description("Select the format which should be used to interprete the data")
                .assignmentOptional().noDefaultValue()
                .commit();
      }

      void configure(const karabo::util::Hash& input) {
        input.get("filename", m_filename);
        input.get("writeMode", m_writeMode);
        if (input.has("format")) {
          m_format = Format<Tdata>::createChoice("format", input);
        } else {
          guessAndSetFormat();
        }
      }

      void write(const Tdata& data) {
        std::stringstream content;
        m_format->convert(data, content);
        writeFile(content);
      }

    private:

      void guessAndSetFormat() {
        
        using namespace std;
        using namespace karabo::util;

        vector<string> keys = Factory<Format<Tdata> >::getRegisteredKeys();
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
        throw KARABO_NOT_SUPPORTED_EXCEPTION("FromFilenameSource::guessAndSetFormat -> Can not interprete extension: \"" + extension + "\"");
      }

      void writeFile(std::stringstream& sourceContent) {

        using namespace std;
        
        string filename = m_filename.string();
        if (m_writeMode == "abort") {
          if (exists(m_filename)) {
            throw KARABO_IO_EXCEPTION("TextFileOutput::write -> File " + filename + " does already exist");
          }
          ofstream outputStream(filename.c_str());
          outputStream << sourceContent.str();
        } else if (m_writeMode == "truncate") {
          ofstream outputStream(filename.c_str(), ios::trunc);
          outputStream << sourceContent.str();
        } else if (m_writeMode == "append") {
          ofstream outputStream(filename.c_str(), ios::app);
          outputStream << sourceContent.str();
        }
      }

      boost::filesystem::path m_filename;
      std::string m_writeMode;
      FormatPointer m_format;
    };

  } // namespace io
} // namespace karabo

#endif
