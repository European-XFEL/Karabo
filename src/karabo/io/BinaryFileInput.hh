/*
 * $Id: BinaryFileReader.hh 4644 2011-11-04 16:04:36Z heisenb@DESY.DE $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 16, 2011, 8:49 PM
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

#ifndef KARABO_IO_BINARYFILEINPUT_HH
#define KARABO_IO_BINARYFILEINPUT_HH

#include <filesystem>
#include <fstream>
#include <iosfwd>
#include <karabo/util/ChoiceElement.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/PathElement.hh>
#include <karabo/util/SimpleElement.hh>
#include <sstream>

#include "BinarySerializer.hh"
#include "Input.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package io
     */
    namespace io {

        /**
         * @class BinaryFileInput
         * @brief The binary file input specializes the Input class to read
         *        data from a binary file types T have been serialized to. The actual
         *        serialization format depends on the Serializer selected in this
         *        class' configuration.
         */
        template <class T>
        class BinaryFileInput : public Input<T> {
            std::filesystem::path m_filename;
            typename BinarySerializer<T>::Pointer m_serializer;
            std::vector<T> m_sequenceBuffer;

           public:
            KARABO_CLASSINFO(BinaryFileInput<T>, "BinaryFile", "1.0");

            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::util;

                PATH_ELEMENT(expected)
                      .key("filename")
                      .description("Name of the file to be read")
                      .displayedName("Filename")
                      .assignmentMandatory()
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

            BinaryFileInput(const karabo::util::Hash& config)
                : Input<T>(config), m_filename(config.get<std::string>("filename")) {
                if (config.has("format")) {
                    m_serializer = BinarySerializer<T>::createChoice("format", config);
                } else {
                    guessAndSetFormat();
                }
                // Read file already here
                std::vector<char> archive;
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

                vector<string> keys = BinarySerializer<Hash>::getRegisteredClasses();
                string extension = std::filesystem::path(m_filename).extension().string().substr(1);
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

            void readFile(std::vector<char>& buffer) {
                using namespace std;

                ifstream file(m_filename.string().c_str(), ios::in | ios::binary);
                if (file.is_open()) {
                    file.seekg(0, ios::end);
                    ifstream::pos_type fileSize = file.tellg();
                    file.seekg(0, ios::beg);
                    buffer.resize(fileSize);
                    if (!file.read(&buffer[0], fileSize)) {
                        KARABO_IO_EXCEPTION("Failed to read file: " + m_filename.string());
                    }
                    file.close();
                } else {
                    throw KARABO_IO_EXCEPTION("Cannot open file: " + m_filename.string());
                }
            }
        };
    } // namespace io
} // namespace karabo

#endif
