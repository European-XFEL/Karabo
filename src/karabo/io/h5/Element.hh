/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_H5_ELEMENT_HH
#define KARABO_IO_H5_ELEMENT_HH


#include <hdf5/hdf5.h>

#include <boost/enable_shared_from_this.hpp>
#include <iostream>
#include <karabo/util/Configurator.hh>
#include <karabo/util/Factory.hh>
#include <karabo/util/FromTypeInfo.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/ToLiteral.hh>
#include <karabo/util/Types.hh>
#include <map>
#include <string>
#include <vector>

#include "Attribute.hh"


namespace karabo {
    namespace io {
        namespace h5 {

            /**
             * @class Element
             * @brief This class maps Karabo Hash values to HDF5 elements
             */
            class Element {
               public:
                KARABO_CLASSINFO(Element, "Element", "1.0");
                KARABO_CONFIGURATION_BASE_CLASS

                static void expectedParameters(karabo::util::Schema& expected);

                /**
                 * Expected parameters used for factorized configuration:
                 *
                 * - h5name: the name of the dataset in the HDF5 file
                 * - h5path: path to the dataset in the HDF5 file
                 * - key: the name of the attribute in the Karabo Hash
                 * - attributes: attributes associated with this value
                 *
                 * @param expected
                 */
                Element(const karabo::util::Hash& input);

                virtual ~Element() {}

                /**
                 * Get element name. Element can represent hdf5 group or dataset
                 * @return name element name
                 */
                const std::string& getFullName() const {
                    return m_h5PathName;
                }

                /**
                 * Get HDF5 name, i.e. name of dataset
                 * @return
                 */
                const std::string& getH5name() const {
                    return m_h5name;
                }

                /**
                 * Get HDF5 path to dataset
                 * @return
                 */
                const std::string& getH5path() const {
                    return m_h5path;
                }

                /**
                 * Get key/path to element in a Karabo Hash this element is bound to
                 * @param sep Karabo path separator
                 * @return
                 */
                const std::string getKey(const char sep = '.') const {
                    std::ostringstream oss;
                    oss << sep;
                    return boost::replace_all_copy(m_key, "/", oss.str());
                }

                /**
                 * Get the memory/data type that defines the element
                 * @return
                 */
                virtual karabo::util::Types::ReferenceType getMemoryType() const = 0;

                /**
                 * Get the classId of the element
                 * @return
                 */
                std::string getElementType() const {
                    return getClassInfo().getClassId();
                }

                /**
                 * Evaluate if this HDF5 element is a dataset
                 * @return
                 */
                virtual bool isDataset() const = 0;

                /**
                 * Evaluate if this HDF5 element is a group
                 * @return
                 */
                virtual bool isGroup() const = 0;

                /**
                 * Evaluate if this Element if of type classId
                 * @param classId
                 * @return
                 */
                bool isType(const std::string& classId) const {
                    if (getClassInfo().getClassId() == classId) return true;
                    return false;
                }

                /**
                 * Get dimensions of this element
                 * @return
                 */
                virtual karabo::util::Dims getDims() const = 0;

                /**
                 * Set compression level to use for HDf5 dataset representing this element
                 * @param level
                 */
                virtual void setCompressionLevel(int level) {}

                /**
                 * Return the configuration of this element as defined by evaluation of its input configuration
                 * @param config
                 */
                void getConfig(karabo::util::Hash& config) {
                    config = m_config;
                }


                /**
                 * Create a HDF5 dataset or group representing this element
                 * @param tableGroup
                 */
                virtual void create(hid_t tableGroup) = 0;

                /**
                 * Open datasets referring to this element in an HDF5 group
                 * @param group
                 */
                virtual hid_t open(hid_t group) = 0;

                /**
                 * Create the attributes pertinent to this element
                 */
                void createAttributes();

                /**
                 * Save attributes contained in data to the tableGroup representing this element in HDF5
                 * @param tableGroup
                 * @param data
                 */
                void saveAttributes(hid_t tableGroup, const karabo::util::Hash& data);

                /**
                 * Open HDF5 datasets representing the attributes pertinent to this element.
                 */
                void openAttributes();


               protected:
                virtual void openH5(hid_t group) = 0;
                virtual void closeH5() = 0;


               public:
                virtual void close() = 0;

                /**
                 * Write data to dataset. Hash structure must contain key and value pair.
                 *
                 * @param data Hash with data to be written.
                 * @param recordId Record number (numbering starts from 0)
                 */
                virtual void write(const karabo::util::Hash& data, hsize_t recordId) = 0;

                /**
                 * Write many records of data to a dataset (buffered writing).
                 * The value of the Hash must be a vector(?) of values of type as defined at the dataset creation time.
                 * The length of the vector must be at least len.
                 * The key is the name of the dataset, value must correspond to the type as defined at the dataset
                 * creation time
                 *
                 *
                 * @param data Hash with data to be written.
                 * @param recordId Record number (numbering starts from 0)
                 * @param len Number of values to be written
                 */
                virtual void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) = 0;


                virtual void writeAttributes(const karabo::util::Hash& data);

                virtual void readAttributes(karabo::util::Hash& data);

                void closeAttributes();

                /**
                 * Allocate memory for single record
                 * If the entry in the Hash does not exist, this function must allocate memory to hold the complete
                 * dataset If the entry exist assume the memory is allocated. This can be used when client delivers own
                 * buffers.
                 * @param data Hash where the data will be stored when using read function
                 */
                virtual void bind(karabo::util::Hash& data) = 0;

                /**
                 * allocate memory for len number of records
                 * @param data Hash where the data will be stored when using read function
                 * @param len number of records to be allocated
                 */
                virtual void bind(karabo::util::Hash& buffer, hsize_t len) = 0;


                virtual void read(hsize_t recordId) = 0;


                virtual void read(hsize_t recordId, hsize_t len) = 0;


               protected:
                std::string m_h5name; // name of this element in hdf5 file

                std::string m_h5path; // path to the parent of this element from the root of the table (/ as separator)

                std::string m_h5PathName;

                std::string m_key; // key  (including path) to the data element in hash

                hid_t m_h5obj; // this dataset or group

                hid_t m_parentGroup; // parent group of this element

                hid_t m_tableGroup; //  hdf5 group of the table where this element belongs to

                karabo::util::Hash m_config;

                std::vector<Attribute::Pointer> m_attributes;
            };


        } // namespace h5
    }     // namespace io
} // namespace karabo


#endif
