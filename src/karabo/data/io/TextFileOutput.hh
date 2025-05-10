/*
 * $Id: TextFileOutput.hh 4951 2012-01-06 12:54:57Z heisenb@DESY.DE $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 16, 2010, 10:18 PM
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */


#ifndef KARABO_DATA_IO_TEXTFILEOUTPUT_HH
#define KARABO_DATA_IO_TEXTFILEOUTPUT_HH

#include <filesystem>
#include <fstream>
#include <iosfwd>
#include <sstream>

#include "Output.hh"
#include "TextSerializer.hh"
#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/schema/NodeElement.hh"
#include "karabo/data/schema/SimpleElement.hh"


namespace karabo {

    namespace data {

        /**
         * @class TextFileOutput
         * @brief The text file output specializes the Output class to write
         *        data of type T to a text file. The actual
         *        serialization format depends on the Serializer selected in this
         *        class' configuration.
         */
        template <class T>
        class TextFileOutput : public Output<T> {
            std::filesystem::path m_filename;
            std::string m_writeMode;
            typename TextSerializer<T>::Pointer m_serializer;
            std::vector<T> m_sequenceBuffer;

           public:
            KARABO_CLASSINFO(TextFileOutput<T>, "TextFile", "1.0")

            static void expectedParameters(karabo::data::Schema& expected) {
                using namespace karabo::data;

                STRING_ELEMENT(expected)
                      .key("filename")
                      .description("Name of the file to be written")
                      .displayedName("Filename")
                      .assignmentMandatory()
                      .commit();

                STRING_ELEMENT(expected)
                      .key("writeMode")
                      .description("Defines the behaviour in case of already existent file")
                      .displayedName("Write Mode")
                      .options("exclusive, truncate")
                      .assignmentOptional()
                      .defaultValue(std::string("truncate"))
                      .commit();

                STRING_ELEMENT(expected)
                      .key("format")
                      .displayedName("Format")
                      .description("Select the format which should be used to interprete the data")
                      .options("Xml")
                      .assignmentOptional()
                      .noDefaultValue()
                      .commit();

                NODE_ELEMENT(expected)
                      .key("Xml")
                      .appendParametersOfConfigurableClass<TextSerializer<T>>("Xml")
                      .commit();
            }

            TextFileOutput(const karabo::data::Hash& config) : Output<T>(config) {
                m_filename = config.get<std::string>("filename");
                config.get("writeMode", m_writeMode);
                if (config.has("format")) {
                    const std::string& selected = config.get<std::string>("format");
                    m_serializer = TextSerializer<T>::create(selected, config.get<Hash>(selected));
                } else {
                    guessAndSetFormat();
                }
            }

            void write(const T& data) {
                if (this->m_appendModeEnabled) {
                    m_sequenceBuffer.push_back(data);
                } else {
                    std::string archive;
                    m_serializer->save(data, archive);
                    writeFile(archive);
                }
            }

           private:
            void update() {
                if (this->m_appendModeEnabled) {
                    std::string archive;
                    m_serializer->save(m_sequenceBuffer, archive);
                    writeFile(archive);
                    m_sequenceBuffer.clear();
                }
            }

            void guessAndSetFormat() {
                using namespace std;
                using namespace karabo::data;

                vector<string> keys = TextSerializer<T>::getRegisteredClasses();
                string extension = m_filename.extension().string().substr(1);
                boost::to_lower(extension);

                for (const string& key : keys) {
                    string lKey(key);
                    boost::to_lower(lKey);
                    if (lKey == extension) {
                        m_serializer = TextSerializer<T>::create(key);
                        return;
                    }
                }
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Can not interprete extension: \"" + extension + "\"");
            }

            void writeFile(std::string& sourceContent) {
                using namespace std;

                string filename = m_filename.string();
                if (m_writeMode == "exclusive") {
                    if (std::filesystem::exists(m_filename)) {
                        throw KARABO_IO_EXCEPTION("TextFileOutput::write -> File " + filename + " does already exist");
                    }
                    ofstream outputStream(filename.c_str());
                    outputStream << sourceContent;
                } else if (m_writeMode == "truncate") {
                    ofstream outputStream(filename.c_str(), ios::trunc);
                    outputStream << sourceContent;
                }
            }
        };

    } // namespace data
} // namespace karabo

#endif
