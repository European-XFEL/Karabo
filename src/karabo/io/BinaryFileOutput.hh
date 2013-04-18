/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 07, 2013, 10:18 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_BINARYFILEOUTPUT_HH
#define	KARABO_IO_BINARYFILEOUTPUT_HH

#include <iosfwd>
#include <fstream>
#include <sstream>
#include <boost/filesystem.hpp>

#include <karabo/util/Configurator.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/PathElement.hh>
#include <karabo/util/ChoiceElement.hh>
#include "Output.hh"
#include "BinarySerializer.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package packageName
     */
    namespace io {

        /**
         * The BinaryFileOutput class.
         */
        template <class T>
        class BinaryFileOutput : public Output<T> {

            boost::filesystem::path m_filename;
            std::string m_writeMode;
            typename BinarySerializer<T>::Pointer m_serializer;

        public:

            KARABO_CLASSINFO(BinaryFileOutput<T>, "BinaryFile", "1.0")

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

                CHOICE_ELEMENT(expected).key("format")
                        .displayedName("Format")
                        .description("Select the format which should be used to interprete the data")
                        .appendNodesOfConfigurationBase<BinarySerializer<T> >()
                        .assignmentOptional().noDefaultValue()
                        .commit();
            }

            BinaryFileOutput(const karabo::util::Hash& config) : Output<T>(config) {
                m_filename = config.get<std::string > ("filename");
                config.get("writeMode", m_writeMode);
                if (config.has("format")) {
                    m_serializer = BinarySerializer<T>::createChoice("format", config);
                } else {
                    guessAndSetFormat();
                }
            }

            void write(const T& data) {
                std::vector<char> buffer;
                m_serializer->save(data, buffer);
                writeFile(buffer);
            }

        private:

            void guessAndSetFormat() {

                using namespace std;
                using namespace karabo::util;

                vector<string> keys = BinarySerializer<T>::getRegisteredClasses();
                string extension = m_filename.extension().string().substr(1);
                boost::to_lower(extension);

                BOOST_FOREACH(string key, keys) {
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
                if (m_writeMode == "abort") {
                    if (boost::filesystem::exists(m_filename)) {
                        throw KARABO_IO_EXCEPTION("File " + filename + " does already exist");
                    }
                    ofstream file(filename.c_str(), ios::out | ios::binary);
                    file.write(&buffer[0], buffer.size());
                    file.close();
                } else if (m_writeMode == "truncate") {
                    ofstream file(filename.c_str(), ios::out | ios::trunc | ios::binary);
                    file.write(&buffer[0], buffer.size());
                    file.close();
                } else if (m_writeMode == "append") {
                    ofstream file(filename.c_str(), ios::out | ios::app | ios::binary);
                    file.write(&buffer[0], buffer.size());
                    file.close();
                }
            }


        };

    }
}

#endif
