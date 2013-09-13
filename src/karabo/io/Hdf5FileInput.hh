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
            Hdf5Serializer::Pointer m_serializer;
            hid_t m_h5file;

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
                m_filename = config.get<std::string>("filename");
                m_serializer = Hdf5Serializer::create("h5");
                hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
                H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
                m_h5file = H5Fopen(m_filename.string().c_str(), H5F_ACC_RDONLY, fapl);
                KARABO_CHECK_HDF5_STATUS(m_h5file);
                KARABO_CHECK_HDF5_STATUS(H5Pclose(fapl));
            }

            virtual ~Hdf5FileInput() {                
                if (m_h5file >= 0) {
                    KARABO_CHECK_HDF5_STATUS(H5Fclose(m_h5file));
                }
            }

            void read(T& data, size_t idx = 0) {
                try {
                    m_serializer->load(data, m_h5file, karabo::util::toString(idx));
                } catch (...) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot serialize object from file " + m_filename.string()))
                }

            }

            size_t size() const {
                H5G_info_t ginfo;
                KARABO_CHECK_HDF5_STATUS(H5Gget_info(m_h5file, &ginfo));
                //std::clog << "nobj=" << ginfo.nlinks << std::endl;
                return ginfo.nlinks;
            }

            virtual void update() {
                KARABO_CHECK_HDF5_STATUS(H5Fclose(m_h5file));
                m_h5file = -1;
            }

        private:


        };
    
    }
}

#endif	
