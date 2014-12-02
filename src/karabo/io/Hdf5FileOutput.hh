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

            typename Hdf5Serializer<T>::Pointer m_serializer;
            boost::filesystem::path m_filename;
            hid_t m_h5file;
            karabo::io::h5::File::AccessMode m_writeMode;
            karabo::io::h5::File::AccessMode m_writeModeNew;
            karabo::io::h5::File::AccessMode m_writeModeUpdate;
            bool m_fileIsOpen;
            bool m_enableAppendMode;
            size_t m_idx;
            std::string m_basePath;

        public:

            KARABO_CLASSINFO(Hdf5FileOutput<T>, "Hdf5File", "1.0")

            static void expectedParameters(karabo::util::Schema& expected) {

                using namespace karabo::util;

                PATH_ELEMENT(expected).key("filename")
                        .description("Name of the file to be written")
                        .displayedName("Filename")
                        .assignmentMandatory()
                        .reconfigurable()
                        .commit();

                STRING_ELEMENT(expected).key("writeMode")
                        .description("Defines the behaviour in case of already existent files when first initializing")
                        .displayedName("Write Mode")
                        .options("exclusive, truncate, append")
                        .assignmentOptional().defaultValue(std::string("truncate"))
                        .commit();
                
                STRING_ELEMENT(expected).key("writeModeUpdate")
                        .description("Defines the behaviour in case of already existent files after calling update")
                        .displayedName("Write Mode after update")
                        .options("exclusive, truncate, append")
                        .assignmentOptional().defaultValue(std::string("append"))
                        .commit();
                
                STRING_ELEMENT(expected).key("basePath")
                        .description("Set the base path of the data groups within the HDF5 file. It should not end with '/'")
                        .displayedName("H5 base path")
                        .assignmentOptional().defaultValue(std::string("/0"))
                        .reconfigurable()
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

            Hdf5FileOutput(const karabo::util::Hash& config) : Output<T>(config), m_idx(0) {
                config.get("enableAppendMode", m_enableAppendMode);
                m_filename = config.get<std::string>("filename");
                configureWriteMode(config);
                m_serializer = Hdf5Serializer<T>::create("h5");
                /*if (m_enableAppendMode) {
                    openFile();
                }*/
                m_basePath = config.get<std::string>("basePath")+"/";
                m_fileIsOpen = false;

            }

            ~Hdf5FileOutput() {
                closeFile();
            }
            
            

            void configureWriteMode(const karabo::util::Hash& config) {
                std::string writeModeString;
                config.get("writeMode", writeModeString);
                
                if (writeModeString == "truncate") {
                    m_writeModeNew = karabo::io::h5::File::TRUNCATE;
                } else if (writeModeString == "exclusive") {
                    m_writeModeNew = karabo::io::h5::File::EXCLUSIVE;
                } else if (writeModeString == "append"){
                    m_writeModeNew = karabo::io::h5::File::APPEND;
                }
                
                std::string writeModeUpdateString;
                config.get("writeModeUpdate", writeModeUpdateString);
                
                if (writeModeUpdateString == "truncate") {
                    m_writeModeUpdate = karabo::io::h5::File::TRUNCATE;
                } else if (writeModeUpdateString == "exclusive") {
                    m_writeModeUpdate = karabo::io::h5::File::EXCLUSIVE;
                } else if (writeModeUpdateString == "append"){
                    m_writeModeUpdate = karabo::io::h5::File::APPEND;
                }
                
                m_writeMode = m_writeModeNew;
            }

            void write(const T& data) {
                using namespace std;
                
                karabo::util::TimeProfiler p("Output");
                p.open();
                
                
                try {
                    if (m_enableAppendMode) {
                        if(!m_fileIsOpen){
                            openFile();
                            m_fileIsOpen = true;
                        }
                        std::string groupName = m_basePath+boost::lexical_cast<std::string>(m_idx);
                        m_serializer->save(data, m_h5file, groupName); //karabo::util::toString(m_idx));
                        
                        m_idx++;
                    } else {
                        p.startPeriod("write");
                        openFile();
                        m_serializer->save(data, m_h5file, "0");
                        closeFile();
                        p.stopPeriod("write");
                        p.close();
                        if (false) {
                            std::clog << std::endl;
                            std::clog << "write data                       : " << p.getPeriod("write").getDuration() << " [s]" << std::endl;
                        }
                    }


                } catch (...) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot serialize object to file " + m_filename.string()))
                }

            }

            void update() {
                if (m_fileIsOpen) {
                    closeFile();
                    m_fileIsOpen = false;
                }
                m_writeMode = m_writeModeUpdate;
            }
            
            


        private:

            void openFile() {
                
                if(m_writeMode == karabo::io::h5::File::EXCLUSIVE || m_writeMode == karabo::io::h5::File::TRUNCATE){
                    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
                    H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
                    hid_t fcpl = H5Pcreate(H5P_FILE_CREATE);
                    H5Pset_link_creation_order(fcpl, H5P_CRT_ORDER_TRACKED | H5P_CRT_ORDER_INDEXED);
                    if(m_writeMode == karabo::io::h5::File::TRUNCATE){
                        m_h5file = H5Fcreate(m_filename.c_str(), H5F_ACC_TRUNC, fcpl, fapl);
                
                    } else if (m_writeMode == karabo::io::h5::File::EXCLUSIVE){
                        m_h5file = H5Fcreate(m_filename.c_str(), H5F_ACC_EXCL, fcpl, fapl);
                        
                    }
                    KARABO_CHECK_HDF5_STATUS(m_h5file);
                    
                    KARABO_CHECK_HDF5_STATUS(H5Pclose(fcpl));
                    
                    KARABO_CHECK_HDF5_STATUS(H5Pclose(fapl));
                    
                } else if (m_writeMode == karabo::io::h5::File::APPEND){
                    
                    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
                    H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
                    m_h5file = H5Fopen(m_filename.c_str(), H5F_ACC_RDWR, fapl);
                    
                    if(m_h5file < 0){
                        //assume the file does not exist, try creating it new instead
                    
                        m_writeMode == karabo::io::h5::File::TRUNCATE;
                        openFile();
                    }
                    KARABO_CHECK_HDF5_STATUS(H5Pclose(fapl));
                    
                    
                }
            }

            void closeFile() {
                if (m_h5file >= 0) {
                    m_serializer->onCloseFile();
                    KARABO_CHECK_HDF5_STATUS(H5Fclose(m_h5file));
                    m_h5file = -1;
                }
            }
            
            void reconfigure(const karabo::util::Hash& config) {
                
                if(config.has("Hdf5File.filename")){
                    if(config.get<std::string>("Hdf5File.filename") != m_filename){
                        update();
                        m_filename = config.get<std::string>("Hdf5File.filename");
                        m_writeMode = m_writeModeNew;
                    }
                }
                if(config.has("Hdf5File.basePath")){
                    if(config.get<std::string>("Hdf5File.basePath") != m_basePath){
                        m_idx = 0;
                        m_basePath = config.get<std::string>("Hdf5File.basePath")+"/";
                    }
                }
            }

        };

    }
}

#endif
