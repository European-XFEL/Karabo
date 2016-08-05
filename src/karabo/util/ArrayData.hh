/* 
 * Author: wigginsj
 *
 * Created on August 4, 2016, 2:33 PM
 */

#ifndef KARABO_UTIL_ARRAYDATA_HH
#define KARABO_UTIL_ARRAYDATA_HH

#include <stdlib.h>
#include <boost/shared_ptr.hpp>

namespace karabo {
    namespace util {

        template <typename T>
        class ArrayData {

            public:

            typedef boost::shared_ptr<T> PointerType;

            private:

            size_t m_numElems;
            PointerType m_data;

            public:

            // Copy & ownership of data
            ArrayData(const T* data, const size_t nelems) : m_numElems(nelems) {
                // Align to the pointer size of the platform
                const size_t nbytes = nelems * sizeof(T);
                T* copy = reinterpret_cast<T*>(aligned_alloc(sizeof(char*), nbytes));
                std::memcpy(copy, data, nbytes);
                m_data = PointerType(copy, &ArrayData::deallocator);
            }

            // Non-ownership of data
            template<class D>
            ArrayData(const T* data, const size_t nelems, const D& deleter) : m_numElems(nelems), m_data(const_cast<T*>(data), deleter) {}

            const size_t size() const { return m_numElems; }

            const T& operator[](const size_t idx) const { return m_data.get()[idx]; }
            T& operator[](const size_t idx) { return const_cast<T*>(m_data.get())[idx]; }

            const T* data() const { return m_data.get(); }
            T* data() { return const_cast<T*>(m_data.get()); }

            private:

            static void deallocator(const T* p) {
                free(const_cast<T*>(p));
            }

        };
    }
}

#endif
