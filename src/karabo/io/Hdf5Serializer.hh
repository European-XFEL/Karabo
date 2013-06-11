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

        class Hdf5Serializer {
        public:

            KARABO_CLASSINFO(Hdf5Serializer, "Hdf5Serializer", "1.0");
            KARABO_CONFIGURATION_BASE_CLASS

            static void expectedParameters(karabo::util::Schema& expected) {

            }

            Hdf5Serializer(const karabo::util::Hash& input) {
//                m_filename = boost::filesystem::path(input.get<std::string>("filename"));
            }

            virtual ~Hdf5Serializer() {
            }

            virtual void save(const karabo::util::Hash& object, hid_t h5file, const std::string& groupName) = 0;

            virtual void load(karabo::util::Hash& object, hid_t h5file, const std::string& groupName ) = 0;
            //
            //            void load(T& object, const std::vector<char>& archive) {
            //                load(object, &archive[0], archive.size());
            //            }
            
            //            T load(const char* archive, const size_t nBytes) {
            //                T object;
            //                this->load(object, archive, nBytes);
            //                return object;
            //            }
            //            
            //            T load(const std::vector<char>& archive) {
            //                T object;
            //                this->load(object, archive);
            //                return object;
            //            }
            //            
            //            

        private:
            boost::filesystem::path m_filename;
        };
    }
}

#endif
