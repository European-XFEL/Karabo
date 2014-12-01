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

#include <karabo/util/Configurator.hh>
#include <karabo/util/PathElement.hh>
#include <karabo/util/ChoiceElement.hh>

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

            typename TextSerializer<T>::Pointer m_serializer;

            boost::filesystem::path m_filename;
            std::vector<T> m_sequenceBuffer;

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
                // Read file already here
                std::stringstream archive;
                readFile(archive);
                m_serializer->load(m_sequenceBuffer, archive);
            }

            void read(T& data, size_t idx = 0) {
                data = m_sequenceBuffer[idx];
            }

            size_t size() {
                return m_sequenceBuffer.size();
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

                ifstream file(m_filename.string().c_str());
                if (file) {
                    buffer << file.rdbuf();
                    file.close();
                } else {
                    throw KARABO_IO_EXCEPTION("Cannot open file: " + m_filename.string());
                }
            }
        };
    }
}

#endif	
