/*
 * $Id$
 *
 *
 * Created on May 10, 2012, 5:05 PM
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifndef KARABO_IO_HDF5SERIALIZER_HH
#define KARABO_IO_HDF5SERIALIZER_HH

#include <hdf5/hdf5.h>

#include <boost/filesystem.hpp>
#include <karabo/util/Configurator.hh>
#include <karabo/util/PathElement.hh>

#include "h5/ErrorHandler.hh"


namespace karabo {
    namespace io {

        /*
         * @class Hdf5Serializer
         * @brief The Hdf5Serializer implements a HDF5 format serialization and
         *        de-serialization interface for type T. The actual serialization
         *        is logic is implemented in derived class.
         */
        template <typename T>
        class Hdf5Serializer {
           public:
            KARABO_CLASSINFO(Hdf5Serializer, "Hdf5Serializer", "1.0");

            KARABO_CONFIGURATION_BASE_CLASS

            static void expectedParameters(karabo::util::Schema& expected) {}

            Hdf5Serializer(const karabo::util::Hash& input) {}

            virtual ~Hdf5Serializer() {}

            /**
             * Save an object to a group in a HDF5 file
             * @param object to save
             * @param h5file HDF5 access handle to the file
             * @param groupName to save the object to
             */
            virtual void save(const T& object, hid_t h5file, const std::string& groupName) = 0;

            /**
             * Load an object from a group in an HDF5 file
             * @param object to load data into
             * @param h5file HDF5 access handle to the file
             * @param groupName to load the object from
             */
            virtual void load(T& object, hid_t h5file, const std::string& groupName) = 0;

            /**
             * Return the number of elements in group
             * @param h5file HDF5 access handle to the file
             * @param groupName to return size of
             * @return
             */
            virtual unsigned long long size(hid_t h5file, const std::string& groupName) = 0;

            /**
             * Hook to call before the file is closed.
             */
            virtual void onCloseFile() {}

           private:
            boost::filesystem::path m_filename;
        };
    } // namespace io
} // namespace karabo

#endif
