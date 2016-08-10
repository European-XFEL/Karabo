/*
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 23, 2015, 10:17 AM
 */

#ifndef KARABO_UTIL_NDARRAY_HH
#define	KARABO_UTIL_NDARRAY_HH

#include "Exception.hh"
#include "ArrayData.hh"
#include "Dims.hh"

namespace karabo {
    namespace util {

        template <typename T>
        class NDArray {

            public:

            typedef boost::shared_ptr<ArrayData<T> > ArrayDataTypePtr;

            private:

            ArrayDataTypePtr m_data;
            Dims m_shape;
            bool m_isBigEndian;

            public:

            // Empty construct needed for Hash operations...
            NDArray() {}

            NDArray(const T* dataPtr,
                    const size_t numElems,
                    const Dims& shape = Dims(),
                    const bool isBigEndian = karabo::util::isBigEndian()) {
                        // Explicitly copy the data which is passed!
                        const ArrayDataTypePtr arrayPtr(new ArrayData<T>(dataPtr, numElems));
                        setData(arrayPtr);
                        setShape(shape);
                        setBigEndian(isBigEndian);
                    }

            // Copy-free constructor
            NDArray(const ArrayDataTypePtr& data,
                    const Dims& shape = Dims(),
                    const bool isBigEndian = karabo::util::isBigEndian()) {
                        setData(data);
                        setShape(shape);
                        setBigEndian(isBigEndian);
                    }

            virtual ~NDArray() {}

            ArrayDataTypePtr getData() const { return m_data; }

            void setData(const ArrayDataTypePtr& data) {
                m_data = data;
            }

            const Dims& getShape() const { return m_shape; }

            void setShape(const Dims& shape) {
                if (shape.size() == 0) {
                    // shape needs to be derived from the data
                    m_shape = Dims(m_data->size());
                }
                else {
                    const unsigned long long dataSize = m_data->size();
                    const unsigned long long shapeSize = shape.size();
                    if (dataSize != shapeSize) {
                        // NOTE: I'm avoiding StringTools here to avoid complicated header interdependency...
                        std::ostringstream msg;
                        msg << "NDArray::setShape: Size of shape: " << std::fixed << shapeSize;
                        msg << " does not match size of data: " << std::fixed << dataSize;
                        throw KARABO_PARAMETER_EXCEPTION(msg.str());
                    }
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
