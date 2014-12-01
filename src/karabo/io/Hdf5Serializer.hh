/*
 * $Id$
 * 
 *
 * Created on May 10, 2012, 5:05 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_IO_HDF5SERIALIZER_HH
#define	KARABO_IO_HDF5SERIALIZER_HH

#include <hdf5/hdf5.h>
#include "h5/ErrorHandler.hh"

#include <karabo/util/Configurator.hh>
#include <karabo/util/PathElement.hh>
#include <boost/filesystem.hpp>


namespace karabo {
    namespace io {

        template<typename T>
        class Hdf5Serializer {
        public:

            KARABO_CLASSINFO(Hdf5Serializer, "Hdf5Serializer", "1.0");
            KARABO_CONFIGURATION_BASE_CLASS

            static void expectedParameters(karabo::util::Schema& expected) {

            }

            Hdf5Serializer(const karabo::util::Hash& input) {
            }

            virtual ~Hdf5Serializer() {
            }

            virtual void save(const T& object, hid_t h5file, const std::string& groupName) = 0;

            virtual void load(T& object, hid_t h5file, const std::string& groupName) = 0;   
            
            virtual unsigned long long size(hid_t h5file, const std::string & groupName) = 0;
            
            
            virtual void onCloseFile() {
                
            }

        private:
            boost::filesystem::path m_filename;
        };
    }
}

#endif
