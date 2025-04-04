/*
 * $Id: TextFileReader.hh 4644 2011-11-04 16:04:36Z heisenb@DESY.DE $
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

#ifndef KARABO_IO_TEXTFILEINPUT_HH
#define KARABO_IO_TEXTFILEINPUT_HH

#include <filesystem>
#include <fstream>
#include <iosfwd>
#include <sstream>

#include "Input.hh"
#include "TextSerializer.hh"
#include "karabo/data/schema/ChoiceElement.hh"
#include "karabo/data/schema/Configurator.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package io
     */
    namespace io {

        /**
         * @class TextFileInput
         * @brief The text file input specializes the Input class to read
         *        data from a text file types T have been serialized to. The actual
         *        serialization format depends on the Serializer selected in this
         *        class' configuration.
         */
        template <class T>
        class TextFileInput : public Input<T> {
            typename TextSerializer<T>::Pointer m_serializer;

            std::filesystem::path m_filename;
            std::vector<T> m_sequenceBuffer;

           public:
            KARABO_CLASSINFO(TextFileInput<T>, "TextFile", "1.0");

            static void expectedParameters(karabo::data::Schema& expected) {
                using namespace karabo::data;

                STRING_ELEMENT(expected)
                      .key("filename")
                      .description("Name of the file to be read")
                      .displayedName("Filename")
                      .assignmentMandatory()
                      .commit();

                CHOICE_ELEMENT(expected)
                      .key("format")
                      .displayedName("Format")
                      .description("Select the format which should be used to interprete the data")
                      .appendNodesOfConfigurationBase<TextSerializer<T> >()
                      .assignmentOptional()
                      .noDefaultValue()
                      .commit();
            }

            TextFileInput(const karabo::data::Hash& config) : Input<T>(config) {
                m_filename = config.get<std::string>("filename");
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
                using namespace karabo::data;

                vector<string> keys = TextSerializer<T>::getRegisteredClasses();
                string extension = std::filesystem::path(m_filename).extension().string().substr(1);
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
    } // namespace io
} // namespace karabo

#endif
