/*
 * $Id: Column.hh 5491 2012-03-09 17:27:25Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_COLUMN_HH
#define	KARABO_IO_COLUMN_HH

#include <string>
#include <vector>

#include <karabo/util/Hash.hh>
#include "Table.hh"

namespace karabo {
    namespace io {
        namespace hdf5 {

            
            template<class T>
            class Column { 
            public:

                Column(const std::string& key, boost::shared_ptr<karabo::io::hdf5::Table> table) :
                m_arrayView(table->getCache<T>(key)),
                m_table(table) {
                }

                T& operator[](unsigned long long recordNumber) {
                    size_t position = m_table->updateCache(recordNumber);
                    return m_arrayView[position];
                }

                const T& operator[](unsigned long long recordNumber) const {
                    size_t position = m_table->updateCache(recordNumber);
                    return m_arrayView[position];
                }

                
            private:

                karabo::io::ArrayView<T>& m_arrayView;             
                boost::shared_ptr<karabo::io::hdf5::Table> m_table;
            };
        }
    }
}


#endif	/* KARABO_IO_COLUMN_HH */

