/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 07, 2013, 10:18 AM
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


#ifndef KARABO_IO_BINARYFILEOUTPUT_HH
#define KARABO_IO_BINARYFILEOUTPUT_HH

#include <filesystem>
#include <fstream>
#include <iosfwd>
#include <karabo/util/ChoiceElement.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/SimpleElement.hh>
#include <sstream>

#include "BinarySerializer.hh"
#include "Output.hh"

namespace karabo {

    namespace io {

        /**
         * @class BinaryFileOutput
         * @brief The binary file output specializes the Output class to write
         *        data of type T to a binary file. The actual
         *        serialization format depends on the Serializer selected in this
         *        class' configuration.
         */
        template <class T>
        class BinaryFileOutput : public Output<T> {
            std::filesystem::path m_filename;
            std::string m_writeMode;
            typename BinarySerializer<T>::Pointer m_serializer;
            std::vector<T> m_sequenceBuffer;

           public:
            KARABO_CLASSINFO(BinaryFileOutput<T>, "BinaryFile", "1.0")

            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::util;

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

                CHOICE_ELEMENT(expected)
                      .key("format")
                      .displayedName("Format")
                      .description("Select the format which should be used to interprete the data")
                      .appendNodesOfConfigurationBase<BinarySerializer<T> >()
                      .assignmentOptional()
                      .noDefaultValue()
                      .commit();
            }

            BinaryFileOutput(const karabo::util::Hash& config)
                : Output<T>(config), m_filename(config.get<std::string>("filename")) {
                config.get("writeMode", m_writeMode);
                if (config.has("format")) {
                    m_serializer = BinarySerializer<T>::createChoice("format", config);
                } else {
                    guessAndSetFormat();
                }
            }

            void write(const T& data) {
                if (this->m_appendModeEnabled) {
                    m_sequenceBuffer.push_back(data);
                } else {
                    std::vector<char> archive;
                    m_serializer->save(data, archive);
                    writeFile(archive);
                }
            }

           private:
            void update() {
                if (this->m_appendModeEnabled) {
                    std::vector<char> archive;
                    m_serializer->save(m_sequenceBuffer, archive);
                    writeFile(archive);
                    m_sequenceBuffer.clear();
                }
            }

            void guessAndSetFormat() {
                using namespace std;
                using namespace karabo::util;

                vector<string> keys = BinarySerializer<T>::getRegisteredClasses();
                string extension = m_filename.extension().string().substr(1);
                boost::to_lower(extension);

                for (const string& key : keys) {
                    string lKey(key);
                    boost::to_lower(lKey);
                    if (lKey == extension) {
                        m_serializer = BinarySerializer<T>::create(key);
                        return;
                    }
                }
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Can not interprete extension: \"" + extension + "\"");
            }

            void writeFile(std::vector<char>& buffer) {
                using namespace std;

                string filename = m_filename.string();
                if (m_writeMode == "exclusive") {
                    if (std::filesystem::exists(m_filename)) {
                        throw KARABO_IO_EXCEPTION("File " + filename + " does already exist");
                    }
                    ofstream file(filename.c_str(), ios::out | ios::binary);
                    file.write(&buffer[0], buffer.size());
                    file.close();
                } else if (m_writeMode == "truncate") {
                    ofstream file(filename.c_str(), ios::out | ios::trunc | ios::binary);
                    file.write(&buffer[0], buffer.size());
                    file.close();
                }
            }
        };

    } // namespace io
} // namespace karabo

#endif
