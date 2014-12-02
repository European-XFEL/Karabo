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

#include "Input.hh"
#include "Hdf5Serializer.hh"



namespace karabo {

    namespace io {

        template <class T>
        class Hdf5FileInput : public Input<T> {

            boost::filesystem::path m_filename;
            typename Hdf5Serializer<T>::Pointer m_serializer;
            hid_t m_h5file;
            std::string m_basePath;
            bool m_fileIsOpen;

        public:

            KARABO_CLASSINFO(Hdf5FileInput<T>, "Hdf5File", "1.0");

            static void expectedParameters(karabo::util::Schema& expected) {

                using namespace karabo::util;

                PATH_ELEMENT(expected).key("filename")
                        .description("Name of the file to be read")
                        .displayedName("Filename")
                        .assignmentMandatory()
                        .reconfigurable()
                        .commit();
                
                STRING_ELEMENT(expected).key("basePath")
                        .description("Set the base path of the data groups within the HDF5 file. It should not end with '/'")
                        .displayedName("H5 base path")
                        .assignmentOptional().defaultValue(std::string("/"))
                        .reconfigurable()
                        .commit();
                
                

            }

            Hdf5FileInput(const karabo::util::Hash& config) : Input<T>(config) {
                m_filename = config.get<std::string>("filename");
                m_basePath = config.get<std::string>("basePath")+"/";
                m_serializer = Hdf5Serializer<T>::create("h5");
                m_fileIsOpen = false;
                m_h5file = -1;
               
            }

            virtual ~Hdf5FileInput() {                
                if (m_h5file >= 0) {
                    KARABO_CHECK_HDF5_STATUS(H5Fclose(m_h5file));
                }
            }

            void read(T& data, size_t idx = 0) {
                if(!m_fileIsOpen){
                    openFile();
                }
                try {
                    
                    std::string groupName = m_basePath+boost::lexical_cast<std::string>(idx);
                    
                    m_serializer->load(data, m_h5file, groupName);
                } catch (...) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot serialize object from file " + m_filename.string()))
                }

            }

            size_t size() {
                if(!m_fileIsOpen){
                    openFile();
                }
                return m_serializer->size(m_h5file, m_basePath);
                /*H5G_info_t ginfo;
                KARABO_CHECK_HDF5_STATUS(H5Gget_info(m_h5file, &ginfo));
                //std::clog << "nobj=" << ginfo.nlinks << std::endl;
                return ginfo.nlinks;*/
            }

            virtual void update() {
                if(m_h5file > 0){
                    m_serializer->onCloseFile();
                    KARABO_CHECK_HDF5_STATUS(H5Fclose(m_h5file));
                }
                m_h5file = -1;
                m_fileIsOpen = false;
            }

        private:

            void openFile(){
                
                hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
                H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
                m_h5file = H5Fopen(m_filename.string().c_str(), H5F_ACC_RDONLY, fapl);
                KARABO_CHECK_HDF5_STATUS(m_h5file);
                KARABO_CHECK_HDF5_STATUS(H5Pclose(fapl));
                m_fileIsOpen = true;
                
            }
            
            void reconfigure(const karabo::util::Hash& config){
               
                if(config.has("Hdf5File.filename")){
                    if(config.get<std::string>("Hdf5File.filename") != m_filename){
                        update();
                        m_filename = config.get<std::string>("Hdf5File.filename");
                    }
                }
                if(config.has("Hdf5File.basePath")){
                    if(config.get<std::string>("Hdf5File.basePath") != m_basePath){
                       
                        m_basePath = config.get<std::string>("Hdf5File.basePath")+"/";
                    }
                }
            }

        };
    
    }
}

#endif	
