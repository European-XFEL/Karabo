/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */





#ifndef KARABO_IO_H5_ATTRIBUTE_HH
#define	KARABO_IO_H5_ATTRIBUTE_HH


#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <karabo/util/Factory.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/ToLiteral.hh>

#include <karabo/util/Types.hh>
#include <karabo/util/FromTypeInfo.hh>

#include <hdf5/hdf5.h>

#include <boost/enable_shared_from_this.hpp>


namespace karabo {
    namespace io {
        namespace h5 {

            class Table;

            class Attribute : public boost::enable_shared_from_this<Attribute> {
            public:
                KARABO_CLASSINFO(Attribute, "Attribute", "1.0");
                KARABO_CONFIGURATION_BASE_CLASS

                static void expectedParameters(karabo::util::Schema& expected);

                void configure(const karabo::util::Hash& input);

                Attribute(const karabo::util::Hash& input);

                virtual ~Attribute() {
                }

                /**
                 * Get element name. Attribute can represent hdf5 group or dataset
                 * @return name element name
                 */
                const std::string& getName();

                /** 
                 * Get Hash representation of this Attribute.
                 * 
                 * @param Hash this object will be filled by the function. 
                 * Key is equal to element name, value is this object.
                 * In case nested groups are defined, for each group an instance of Hash is created.
                 * i.e. if dataset is "a.b.c" -> hash looks like:
                 * a => Hash
                 *   b => Hash
                 *     c => RecordAttribute
                 */
                virtual void getAttribute(karabo::util::Hash& element);

                /**
                 * Create UNLIMITED CHUNKED HDF5 dataset.
                 * @param group Hdf5 group where the dataset belongs to.
                 * @param chunkSize Chunk size as defined by hdf5
                 */
                virtual void create(hsize_t chunkSize) = 0;


                virtual void write(const karabo::util::Hash& data, hsize_t recordId) = 0;
                //
                //

                /**
                 * Write many records of data to a dataset (buffered writing).
                 * The value of the Hash must be a vector(?) of values of type as defined at the dataset creation time.
                 * The length of the vector must be at least len.
                 * The key is the name of the dataset, value must correspond to the type as defined at the dataset creation time
                 * 
                 *
                 * @param data Hash with data to be written.
                 * @param recordId Record number (numbering starts from 0)
                 * @param len Number of values to be written
                 */
                virtual void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) = 0;



            protected:

                std::string m_h5name; // name of this element
                std::string m_h5path; // path to the parent of this element from the root of the table (/ as separator)
                std::string m_h5PathName;
                std::string m_key; // attribute key
                hid_t m_group; // parent group of this element
                
                karabo::util::Hash m_config;

            };


        }
    }
}


#endif

