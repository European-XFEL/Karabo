/*
 * $Id: TextFileReader.hh 4644 2011-11-04 16:04:36Z heisenb@DESY.DE $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 16, 2011, 8:49 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_IO_TEXTFILEINPUT_HH
#define	KARABO_IO_TEXTFILEINPUT_HH

#include <iosfwd>
#include <fstream>
#include <sstream>
#include <boost/filesystem.hpp>

#include <karabo/util/util.hh>

#include "Input.hh"
#include "TextSerializer.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package io
     */
    namespace io {

        template <class T>
        class TextFileInput : public Input<T> {
            boost::filesystem::path m_filename;
            typename TextSerializer<T>::Pointer m_serializer;

        public:

            KARABO_CLASSINFO(TextFileInput<T>, "TextFile", "1.0");

            static void expectedParameters(karabo::util::Schema& expected) {

                using namespace karabo::util;

                PATH_ELEMENT(expected).key("filename")
                        .description("Name of the file to be read")
                        .displayedName("Filename")
                        .assignmentMandatory()
                        .commit();

                CHOICE_ELEMENT(expected).key("format")
                        .displayedName("Format")
                        .description("Select the format which should be used to interprete the data")
                        .appendNodesOfConfigurationBase<TextSerializer<T> >()
                        .assignmentOptional().noDefaultValue()
                        .commit();
            }

            TextFileInput(const karabo::util::Hash& config) : Input<T>(config) {
                m_filename = config.get<std::string > ("filename");
                if (config.has("format")) {
                    m_serializer = TextSerializer<T>::createChoice("format", config);
                } else {
                    guessAndSetFormat();
                }
            }

            void read(T& data, size_t idx = 0) {
                std::stringstream buffer;
                readFile(buffer);
                m_serializer->load(data, buffer);
            }

            size_t size() const {
                return 1;
            }

        private:

            void guessAndSetFormat() {

                using namespace std;
                using namespace karabo::util;

                vector<string> keys = TextSerializer<T>::getRegisteredClasses();
                string extension = boost::filesystem::path(m_filename).extension().string().substr(1);
                boost::to_lower(extension);

                BOOST_FOREACH(string key, keys) {
                    string lKey(key);
                    boost::to_lower(lKey);
                    if (lKey == extension) {
                        m_serializer = TextSerializer<T>::create(key);
                        return;
                    }
                }
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Can not interprete extension: \"" + extension + "\"");
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
                    throw KARABO_IO_EXCEPTION("Cannot open file: " + m_filename.string());
                }
            }
        };
    }
} 

#endif	
