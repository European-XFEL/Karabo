/*
 *
 * Created on August 22, 2016, 17:17 PM
 *
 * Copyright (c) 2016-2018 European XFEL GmbH Hamburg. All rights reserved.
 */

#include "NDArray.hh"
#include "NDArrayElement.hh"
#include "ToSize.hh"

namespace karabo {
    namespace util {


        NDArray::NDArray(const Dims& shape,
                         const karabo::util::Types::ReferenceType& type,
                         const bool isBigEndian) {
            setClassId();
            const size_t byteSize = shape.size() * Types::to<ToSize>(type);
            char* buffer = new char[byteSize];
            set("data", std::make_pair(DataPointer(buffer, &NDArray::deallocator), byteSize));
            set("type", static_cast<int> (type));
            setShape(shape);
            setBigEndian(isBigEndian);
        }


        NDArray::NDArray(const DataPointer& ptr,
                         const karabo::util::Types::ReferenceType& type,
                         const size_t& numElems, const Dims& shape,
                         const bool isBigEndian) {
            setClassId();
            const size_t itemSize = Types::to<ToSize>(type);
            const size_t byteSize = numElems * itemSize;          
            set("data", std::make_pair(ptr, byteSize));
            set("type", static_cast<int> (type));
            setShape(shape);
            setBigEndian(isBigEndian);
        }


        void NDArray::setShape(const Dims& shape) {
            if (shape.size() == 0) {
                // shape needs to be derived from the data
                set("shape", Dims(size()).toVector());
            } else {
                const unsigned long long dataSize = size();
                const unsigned long long shapeSize = shape.size();
                if (dataSize != shapeSize) {
                    // NOTE: I'm avoiding StringTools here to avoid complicated header interdependency...
                    std::ostringstream msg;
                    msg << "NDArray::setShape: Size of shape: " << std::fixed << shapeSize;
                    msg << " does not match size of data: " << std::fixed << dataSize;
                    throw KARABO_PARAMETER_EXCEPTION(msg.str());
                }
                set("shape", shape.toVector());
            }
        }


        karabo::util::Types::ReferenceType NDArray::getType() const {
            return static_cast<karabo::util::Types::ReferenceType> (get<int>("type"));
        }


        const size_t NDArray::size() const {
            return byteSize() / itemSize();
        }


        size_t NDArray::byteSize() const {
            return get<ByteArray>("data").second;
        }


        size_t NDArray::itemSize() const {
            return Types::to<ToSize>(getType());
        }


        const NDArray::DataPointer& NDArray::getDataPtr() const {
            return get<ByteArray>("data").first;
        }


        ByteArray NDArray::getByteArray() {
            return get<ByteArray>("data");
        }


        ByteArray NDArray::getByteArray() const {
            return get<ByteArray>("data");
        }


        Dims NDArray::getShape() const {
            return Dims(get<std::vector<unsigned long long> >("shape"));
        }


        bool NDArray::isBigEndian() const {
            return get<bool>("isBigEndian");
        }


        void NDArray::toLittleEndian() {
            if (isBigEndian()) {
                swapEndianess();
                setBigEndian(false);
            }
        }


        void NDArray::toBigEndian() {
            if (!isBigEndian()) {
                swapEndianess();
                setBigEndian(true);
            }
        }


        void NDArray::setClassId() {
            set("__classId", getClassInfo().getClassId());
        }


        void NDArray::setBigEndian(const bool isBigEndian) {
            set("isBigEndian", isBigEndian);
        }


        void NDArray::swapEndianess() {
            const int wordSize = byteSize() / size();
            switch (wordSize) {
                case 1:
                    // No swap needed.
                    break;
                case 2:
                {
                    unsigned short* data = reinterpret_cast<unsigned short*> (get<ByteArray>("data").first.get());
                    for (size_t i = 0; i < size(); ++i) {
                        data[i] = karabo::util::byteSwap16(data[i]);
                    }
                    break;
                }
                case 4:
                {
                    unsigned int* data = reinterpret_cast<unsigned int*> (get<ByteArray>("data").first.get());
                    for (size_t i = 0; i < size(); ++i) {
                        data[i] = karabo::util::byteSwap32(data[i]);
                    }
                    break;
                }
                case 8:
                {
                    unsigned long long* data = reinterpret_cast<unsigned long long*> (get<ByteArray>("data").first.get());
                    for (size_t i = 0; i < size(); ++i) {

                        data[i] = karabo::util::byteSwap64(data[i]);
                    }
                    break;
                }
                default:
                    throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Endianess conversion not implemented for this data type");
            }
        }


        void NDArray::deallocator(const char* p) {
            delete [] p;
        }

    }
}