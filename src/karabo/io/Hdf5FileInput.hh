/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on May 5, 2013, 8:49 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_IO_HDF5FILEINPUT_HH
#define	KARABO_IO_HDF5FILEINPUT_HH

#include <boost/filesystem.hpp>

#include <karabo/util/Configurator.hh>
#include <karabo/util/PathElement.hh>
#include <karabo/util/ChoiceElement.hh>

#include <karabo/io/h5/File.hh>
#include <karabo/io/h5/Table.hh>
#include <karabo/io/h5/Format.hh>

#include "Input.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package io
     */
    namespace io {

        template <class T>
        class Hdf5FileInput : public Input<T> {

            boost::filesystem::path m_filename;


        public:

            KARABO_CLASSINFO(Hdf5FileInput<T>, "Hdf5File", "1.0");

            static void expectedParameters(karabo::util::Schema& expected) {

                using namespace karabo::util;

                PATH_ELEMENT(expected).key("filename")
                        .description("Name of the file to be read")
                        .displayedName("Filename")
                        .assignmentMandatory()
                        .commit();

            }

            Hdf5FileInput(const karabo::util::Hash& config) : Input<T>(config) {
                m_filename = config.get<std::string > ("filename");
            }

            void read(T& data, size_t idx = 0) {
                try {
                    karabo::io::h5::File file(m_filename.string());
                    file.open(karabo::io::h5::File::READONLY);
                    karabo::io::h5::Table::Pointer table = file.getTable("/root");
                    table->bind(data);
                    table->read(idx);
                    file.close();
                } catch (...) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot serialize object from file " + m_filename.string()))
                }

            }

            size_t size() const {
                return 1;
            }

        private:

            //            void guessAndSetFormat() {
            //
            //                using namespace std;
            //                using namespace karabo::util;
            //
            //                vector<string> keys = TextSerializer<T>::getRegisteredClasses();
            //                string extension = boost::filesystem::path(m_filename).extension().string().substr(1);
            //                boost::to_lower(extension);
            //
            //                BOOST_FOREACH(string key, keys) {
            //                    string lKey(key);
            //                    boost::to_lower(lKey);
            //                    if (lKey == extension) {
            //                        m_serializer = TextSerializer<T>::create(key);
            //                        return;
            //                    }
            //                }
            //                throw KARABO_NOT_SUPPORTED_EXCEPTION("Can not interprete extension: \"" + extension + "\"");
            //            }
            //
            //            void readFile(std::stringstream& buffer) {
            //
            //                using namespace std;
            //
            //                string line;
            //                ifstream inputStream(m_filename.string().c_str());
            //                if (inputStream.is_open()) {
            //                    while (!inputStream.eof()) {
            //                        getline(inputStream, line);
            //                        buffer << line << endl;
            //                    }
            //                    inputStream.close();
            //                } else {
            //                    throw KARABO_IO_EXCEPTION("Cannot open file: " + m_filename.string());
            //                }
            //            }
        };
    }
}

#endif	
