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
#include "Hdf5Serializer.hh"


namespace karabo {

    namespace io {

        template <class T>
        class Hdf5FileOutput : public Output<T> {

            Hdf5Serializer::Pointer m_serializer;
            boost::filesystem::path m_filename;
            hid_t m_h5file;
            karabo::io::h5::File::AccessMode m_writeMode;
            bool m_enableAppendMode;
            size_t m_idx;

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

                BOOL_ELEMENT(expected).key("enableAppendMode")
                        .description("If set to true a different internal structure is used, which buffers consecutive "
                                     "calls to write(). The update() function must be called to trigger final outputting "
                                     "of the accumulated sequence of data.")
                        .displayedName("Enable append mode")
                        .assignmentOptional().defaultValue(false)
                        .init()
                        .commit();
            }

            Hdf5FileOutput(const karabo::util::Hash& config) : Output<karabo::util::Hash>(config), m_idx(0) {
                config.get("enableAppendMode", m_enableAppendMode);
                m_filename = config.get<std::string>("filename");
                configureWriteMode(config);
                m_serializer = Hdf5Serializer::create("h5");
                if (m_enableAppendMode) {
                    openFile();
                }

            }

            ~Hdf5FileOutput() {
                closeFile();
            }

            void configureWriteMode(const karabo::util::Hash& config) {
                std::string writeModeString;
                config.get("writeMode", writeModeString);
                if (writeModeString == "truncate") {
                    m_writeMode = karabo::io::h5::File::TRUNCATE;
                } else if (writeModeString == "exclusive") {
                    m_writeMode = karabo::io::h5::File::EXCLUSIVE;
                }
            }

            void write(const T& data) {
                using namespace std;
                karabo::util::Profiler p("Output");
                try {
                    if (m_enableAppendMode) {
                        m_serializer->save(data, m_h5file, karabo::util::toString(m_idx));
                        m_idx++;
                    } else {
                        p.start("write");
                        openFile();
                        m_serializer->save(data, m_h5file, "0");
                        closeFile();
                        p.stop("write");
                        double writeTime = karabo::util::HighResolutionTimer::time2double(p.getTime("write"));
                        if (false) {
                            std::clog << std::endl;
                            std::clog << "write data                       : " << writeTime << " [s]" << std::endl;
                        }
                    }


                } catch (...) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot serialize object to file " + m_filename.string()))
                }

            }

            void update() {
                if (m_enableAppendMode) {
                    closeFile();
                }
            }


        private:

            void openFile() {
                hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
                H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
                hid_t fcpl = H5Pcreate(H5P_FILE_CREATE);
                H5Pset_link_creation_order(fcpl, H5P_CRT_ORDER_TRACKED | H5P_CRT_ORDER_INDEXED);
                m_h5file = H5Fcreate(m_filename.c_str(), H5F_ACC_TRUNC, fcpl, fapl);
                KARABO_CHECK_HDF5_STATUS(m_h5file);
                KARABO_CHECK_HDF5_STATUS(H5Pclose(fcpl));
                KARABO_CHECK_HDF5_STATUS(H5Pclose(fapl));
            }

            void closeFile() {
                if (m_h5file >= 0) {
                    KARABO_CHECK_HDF5_STATUS(H5Fclose(m_h5file));
                    m_h5file = -1;
                }
            }

        };











        /**
         * The Hdf5FileOutput class.
         */

        //        template <class T>
        //        class Hdf5FileOutput : public Output<T> {
        //
        //            boost::filesystem::path m_filename;
        //            karabo::io::h5::File::AccessMode m_writeMode;
        //
        //
        //        public:
        //
        //            KARABO_CLASSINFO(Hdf5FileOutput<T>, "Hdf5File", "1.0")
        //
        //            static void expectedParameters(karabo::util::Schema& expected) {
        //
        //                using namespace karabo::util;
        //
        //                PATH_ELEMENT(expected).key("filename")
        //                        .description("Name of the file to be written")
        //                        .displayedName("Filename")
        //                        .assignmentMandatory()
        //                        .commit();
        //
        //                STRING_ELEMENT(expected).key("writeMode")
        //                        .description("Defines the behaviour in case of already existent file")
        //                        .displayedName("Write Mode")
        //                        .options("exclusive, truncate")
        //                        .assignmentOptional().defaultValue(std::string("truncate"))
        //                        .commit();
        //            }
        //
        //            Hdf5FileOutput(const karabo::util::Hash& config) : Output<karabo::util::Hash>(config) {
        //                m_filename = config.get<std::string>("filename");
        //                configureWriteMode(config);
        //            }
        //
        //            void configureWriteMode(const karabo::util::Hash& config) {
        //                std::string writeModeString;
        //                config.get("writeMode", writeModeString);
        //                if (writeModeString == "truncate") {
        //                    m_writeMode = karabo::io::h5::File::TRUNCATE;
        //                } else if (writeModeString == "exclusive") {
        //                    m_writeMode = karabo::io::h5::File::EXCLUSIVE;
        //                }
        //            }
        //
        //            void write(const T& data) {
        //                karabo::util::Profiler p("Output");
        //                try {
        //                    p.start("format");
        //                    karabo::io::h5::Format::Pointer dataFormat = karabo::io::h5::Format::discover(data);
        //                    p.stop("format");
        //                    karabo::io::h5::File file(m_filename.string());
        //                    file.open(m_writeMode);
        //                    p.start("create");
        //                    karabo::io::h5::Table::Pointer t = file.createTable("/root", dataFormat);
        //                    p.stop("create");
        //                    p.start("attribute");
        //                    t->writeAttributes(data);
        //                    p.stop("attribute");
        //                    p.start("write");
        //                   t->write(data, 0);
        //                    p.stop("write");
        //                    p.start("close");
        //                    file.close();
        //                    p.stop("close");
        //                    double formatTime = karabo::util::HighResolutionTimer::time2double(p.getTime("format"));
        //                    double createTime = karabo::util::HighResolutionTimer::time2double(p.getTime("create"));
        //                    double attributeTime = karabo::util::HighResolutionTimer::time2double(p.getTime("attribute"));
        //                    double writeTime = karabo::util::HighResolutionTimer::time2double(p.getTime("write"));
        //                    double closeTime = karabo::util::HighResolutionTimer::time2double(p.getTime("close"));
        //
        //                    if (true) {
        //                        std::clog << std::endl;
        //                        std::clog << "format                           : " << formatTime << " [s]" << std::endl;
        //                        std::clog << "open/prepare file                : " << createTime << " [s]" << std::endl;
        //                        std::clog << "write attributes                 : " << attributeTime << " [s]" << std::endl;
        //                        std::clog << "write data                       : " << writeTime << " [s]" << std::endl;
        //                        std::clog << "close                            : " << closeTime << " [s]" << std::endl;
        //                        std::clog << "Total write time                 : " << formatTime + createTime + attributeTime + writeTime + closeTime << " [s]" << std::endl;
        //                    }
        //
        //
        //
        //                } catch (...) {
        //                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot serialize object to file " + m_filename.string()))
        //                }
        //
        //            }
        //
        //        private:
        //
        //
        //        };

    }
}

#endif
