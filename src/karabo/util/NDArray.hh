/*
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 23, 2015, 10:17 AM
 */

#ifndef KARABO_UTIL_NDARRAY_HH
#define	KARABO_UTIL_NDARRAY_HH

#include <boost/shared_ptr.hpp>
#include <vector>
#include "Dims.hh"

namespace karabo {
    namespace util {

        template <typename T>
        class NDArray {

            public:

            typedef std::vector<T> VectorType;
            typedef boost::shared_ptr<VectorType> VectorTypePtr;

            private:

            VectorTypePtr m_dataPtr;
            Dims m_shape;
            bool m_isBigEndian;

            public:

            // Empty construct needed for Hash operations...
            NDArray() {}

            NDArray(const VectorType& data,
                    const Dims& shape = Dims(),
                    const bool isBigEndian = karabo::util::isBigEndian()) {
                        // Explicitly copy the data which is passed!
                        const VectorTypePtr dataPtr(new VectorType(data));
                        setData(dataPtr);
                        setShape(shape);
                        setBigEndian(isBigEndian);
                    }

            // Copy-free constructor
            NDArray(const VectorTypePtr& dataPtr,
                    const Dims& shape = Dims(),
                    const bool isBigEndian = karabo::util::isBigEndian()) {
                        setData(dataPtr);
                        setShape(shape);
                        setBigEndian(isBigEndian);
                    }

            virtual ~NDArray() {}

            VectorTypePtr getData() const { return m_dataPtr; }

            void setData(const VectorTypePtr& dataPtr) {
                m_dataPtr = dataPtr;
            }

            const Dims& getShape() const { return m_shape; }

            void setShape(const Dims& shape) {
                if (shape.size() == 0) {
                    // shape needs to be derived from the data
                    m_shape = Dims(m_dataPtr->size());
                }
                else {
                    m_shape = shape;
                }
            }

            bool isBigEndian() const { return m_isBigEndian; }

            void setBigEndian(const bool isBigEndian) {
                m_isBigEndian = isBigEndian;
            }
        };
    }
}

#endif
