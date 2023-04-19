/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_H5_GROUP_HH
#define KARABO_IO_H5_GROUP_HH

#include <karabo/util/Configurator.hh>
#include <string>

#include "Element.hh"
#include "ErrorHandler.hh"
#include "TypeTraits.hh"

namespace karabo {

    namespace io {

        namespace h5 {

            /**
             * @class Group
             * @brief An interface for grouping Hash data in HDF5
             * @deprecated This class seams to not be used. Deprecate it.S
             */
            class Group : public karabo::io::h5::Element {
               public:
                KARABO_CLASSINFO(Group, "Group", "1.0")

                static void expectedParameters(karabo::util::Schema& expected);

                Group(const karabo::util::Hash& input) : Element(input), m_isVectorHash(false), m_vectorSize(0) {
                    if (input.has("type")) {
                        if (input.get<std::string>("type") == "VECTOR_HASH") {
                            m_isVectorHash = true;
                            if (input.has("size")) {
                                m_vectorSize = input.getAs<unsigned long long>("size");
                            } else {
                                m_vectorSize = 0;
                            }
                        }
                    }
                }

                virtual ~Group() {}

                bool isGroup() const {
                    return true;
                }

                bool isDataset() const {
                    return false;
                }

                karabo::util::Types::ReferenceType getMemoryType() const {
                    if (m_isVectorHash) return karabo::util::Types::VECTOR_HASH;
                    return karabo::util::Types::HASH;
                }

                void create(hid_t tableGroup);

                hid_t open(hid_t group);

                void write(const karabo::util::Hash& data, hsize_t recordId) {}

                void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) {}


                void close();

                void bind(karabo::util::Hash& data);

                void bind(karabo::util::Hash& data, hsize_t len);

                void read(karabo::util::Hash& data, hsize_t recordId) {}

                inline void read(hsize_t recordId) {}

                void read(hsize_t recordId, hsize_t len) {}

                karabo::util::Dims getDims() const {
                    return karabo::util::Dims(m_vectorSize);
                }

               private:
                void openH5(hid_t group) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "group m_h5obj = " << m_h5obj;
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "group m_h5PathName = " << m_h5PathName;
                    if (m_h5obj < 0) {
                        m_h5obj = H5Gopen2(group, m_h5PathName.c_str(), H5P_DEFAULT);
                        KARABO_CHECK_HDF5_STATUS(m_h5obj);
                    }
                }

                void closeH5() {
                    if (m_h5obj > -1) {
                        KARABO_CHECK_HDF5_STATUS(H5Gclose(m_h5obj));
                        m_h5obj = -1;
                    }
                }


                bool m_isVectorHash;
                unsigned long long m_vectorSize;
            };


        } // namespace h5
    }     // namespace io
} // namespace karabo

#endif
