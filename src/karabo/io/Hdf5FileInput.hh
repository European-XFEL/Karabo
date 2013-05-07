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

            Hdf5FileInput(const karabo::util::Hash& config) : Input<T>(config), m_file(config) {
                m_file.open(karabo::io::h5::File::READONLY);
                m_table = m_file.getTable("/root");
            }

            virtual ~Hdf5FileInput() {
                m_file.close();
            }

            void read(T& data, size_t idx = 0) {
                try {

                    m_table->bind(data);
                    m_table->read(idx);

                } catch (...) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot serialize object from file " + m_file.getName()))
                }

            }

            size_t size() const {
                return m_table->size();
            }

        private:

            karabo::io::h5::File m_file;
            karabo::io::h5::Table::Pointer m_table;
        };
    }
}

#endif	
