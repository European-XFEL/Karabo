/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on May 5, 2013, 10:18 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_HDF5FILEOUTPUT_HH
#define	KARABO_IO_HDF5FILEOUTPUT_HH


#include <boost/filesystem.hpp>

#include <karabo/util/Configurator.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/PathElement.hh>
#include <karabo/util/ChoiceElement.hh>

#include "Output.hh"
#include "h5/File.hh"
#include "h5/Table.hh"
#include "h5/Format.hh"










/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package packageName
     */
    namespace io {

        /**
         * The Hdf5FileOutput class.
         */

        template <class T>
        class Hdf5FileOutput : public Output<T> {

            boost::filesystem::path m_filename;
            karabo::io::h5::File::AccessMode m_writeMode;


        public:

            KARABO_CLASSINFO(Hdf5FileOutput<T>, "Hdf5File", "1.0")

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
                        .options("exclusive, truncate")
                        .assignmentOptional().defaultValue(std::string("truncate"))
                        .commit();
            }

            Hdf5FileOutput(const karabo::util::Hash& config) : Output<karabo::util::Hash>(config)  {
                m_filename = config.get<std::string>("filename");
                configureWriteMode(config);                
            }

            
            void configureWriteMode(const karabo::util::Hash& config) {
                std::string writeModeString;
                config.get("writeMode", writeModeString);
                if (writeModeString == "truncate") {
                    m_writeMode = karabo::io::h5::File::TRUNCATE;
                } else if (writeModeString == "exclusive"){
                    m_writeMode = karabo::io::h5::File::EXCLUSIVE;
                }
            }

            void write(const T& data) {
                try {
                    karabo::util::Hash config;
                    karabo::io::h5::Format::discoverFromHash(data, config);
                    karabo::io::h5::Format::Pointer dataFormat = karabo::io::h5::Format::createFormat(config);
                    karabo::io::h5::File file(m_filename.string());
                    file.open(m_writeMode);
                    karabo::io::h5::Table::Pointer t = file.createTable("/root", dataFormat, 1);
                    t->write(data, 0);
                    file.close();

                } catch (...) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot serialize object to file " + m_filename.string()))
                }

            }

        private:
            

        };

    }
}

#endif
