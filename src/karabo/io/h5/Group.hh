/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_H5_GROUP_HH
#define	KARABO_IO_H5_GROUP_HH

#include <string>

#include "Element.hh"
#include "TypeTraits.hh"
#include <karabo/util/util.hh>
#include "ErrorHandler.hh"

namespace karabo {

    namespace io {

        namespace h5 {

            class Group : public karabo::io::h5::Element {
            public:

                KARABO_CLASSINFO(Group, "Group", "1.0")

                static void expectedParameters(karabo::util::Schema& expected);

                Group(const karabo::util::Hash& input) : Element(input) {

                }

                virtual ~Group() {
                }

                void create(hsize_t chunkSize);

                void write(const karabo::util::Hash& data, hsize_t recordId) {
                }

                void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) {
                }
                
                void open(hid_t group);
                
                void close();

                inline void bind(karabo::util::Hash & data) {
                }

                inline void allocate(karabo::util::Hash& buffer, size_t len) {
                }

                inline void read(karabo::util::Hash& data, hsize_t recordId) {
                }
                
                inline void read( hsize_t recordId) {
                }
                

                hid_t m_group; // this group


            };



        }
    }
}

#endif	
