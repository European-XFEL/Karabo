/*
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 23, 2015, 10:17 AM
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef KARABO_DATA_TYPES_NDARRAY_HH
#define KARABO_DATA_TYPES_NDARRAY_HH

#include "ByteSwap.hh"
#include "Dims.hh"
#include "Exception.hh"
#include "FromInt.hh"
#include "Hash.hh"
#include "ToSize.hh"


namespace karabo {
    namespace data {

        /**
         * @class NDArray
         * @brief A class representing multi-dimensional data in Karabo that seaminglessy converts to numpy.ndarray
         *
         * The NDArray class is intended to store any multidimensional data occurring in Karabo. Internally it
         * holds the data in a ByteArray. It is a Hash-derived structure, which means it serializes into a
         * karabo::data::Hash. Its meta-data is chosen such that it can be seamlessly converted into a numpy.ndarray
         *
         * Internally (i.e. relevant only for serialisation), it holds the following keys:
         * - "data": a ByteArray
         * - "shape": a vector<unsigned long long>
         * - "type": an int matching a Types::ReferenceType value
         * - "isBigEndian": a bool
         * (If this changes, it needs to be followed up in the NDArrayDescription helper class of the NDArrayElement.)
         */
        class NDArray : protected Hash {
           public:
            KARABO_CLASSINFO(NDArray, "NDArray", "1.5");

            typedef std::shared_ptr<char> DataPointer;

            struct NullDeleter {
                void operator()(void const*) const {
                    // Do nothing
                }
            };

            /**
             * This constructor creates an empty NDArray
             * @param shape
             * @param type
             * @param isBigEndian
             */
            explicit NDArray(const Dims& shape = Dims(),
                             const karabo::data::Types::ReferenceType& type = karabo::data::Types::DOUBLE,
                             const bool isBigEndian = karabo::data::isBigEndian());

            /**
             * This constructor creates an NDArray where all values are initialized with a fill value
             * @param shape
             * @param fill
             * @param isBigEndian
             */
            template <typename T>
            NDArray(const Dims& shape, const T& fill, const bool isBigEndian = karabo::data::isBigEndian());

            /**
             * This constructor copies data from the provided memory location.
             * Internally the data is kept as shared pointer.
             * @param dataPtr Typed address to a stretch of contiguous memory
             * @param numElems Number of elements (of type T)
             * @param shape Shape information
             * @param isBigEndian Endianess flag
             */
            template <typename T>
            NDArray(const T* dataPtr, const size_t numElems, const Dims& shape = Dims(),
                    const bool isBigEndian = karabo::data::isBigEndian());

            /**
             * This constructor copies data from the provided iterator range.
             * Data type is deduced from the value_type of the InputIterator.
             * The range from 'first' (included) to 'last' (excluded) has to be valid range.
             * @param first Begin of range
             * @param last End of range (i.e. points one behind as vector.end())
             * @param shape Shape information
             * @param isBigEndian Endianess flag
             */
            template <typename InputIterator>
            NDArray(InputIterator first, InputIterator last, const Dims& shape = Dims(),
                    const bool isBigEndian = karabo::data::isBigEndian());

            /**
             * This constructor does NOT copy data.
             * Only a view on the external memory is established. A functor for dealing
             * with the viewed on memory must be provided in case this object gets destructed.
             * @param dataPtr Typed address to a stretch of contiguous memory
             * @param numElems Number of elements (of type T)
             * @param deleter A functor defining the deletion behavior
             * @param shape Shape information
             * @param isBigEndian Endianess flag
             */
            template <typename T, typename D>
            NDArray(const T* dataPtr, const size_t numElems, const D& deleter, const Dims& shape = Dims(),
                    const bool isBigEndian = karabo::data::isBigEndian());


            /**
             * Non-templated copy/no-copy construction depending on 'copy' flag (default is false).
             * @param ptr Shared_ptr to external memory location
             * @param type Type of provided data
             * @param numElems Number of elements of provided type
             * @param shape Shape information
             * @param isBigEndian Endianess flag
             */
            NDArray(const DataPointer& ptr, const karabo::data::Types::ReferenceType& type, const size_t& numElems,
                    const Dims& shape = Dims(), const bool isBigEndian = karabo::data::isBigEndian(),
                    bool copy = false);

            virtual ~NDArray() {}

            /**
             * Set the schape of the array
             * @param shape
             */
            void setShape(const Dims& shape);

            karabo::data::Types::ReferenceType getType() const;

            /**
             * The number of items in the array.
             */
            inline size_t size() const {
                return byteSize() / itemSize();
            }

            /**
             * The total size of the array, in bytes.
             */
            inline size_t byteSize() const {
                return get<ByteArray>("data").second;
            }

            /**
             * The size of each item, in bytes.
             */
            inline size_t itemSize() const {
                return Types::to<ToSize>(getType());
            }

            /**
             * Get the data contained in the array as a pointer
             * @return
             */
            template <typename T>
            const T* getData() const {
                if (get<int>("type") == Types::from<T>()) {
                    return reinterpret_cast<T*>(get<ByteArray>("data").first.get());
                } else {
                    const int fromType = get<int>("type");
                    // If fromType is invalid (e.g. since corrupted NDArray), we cannot get a string literal...
                    std::string fromTypeStr("_invalid_");
                    try {
                        fromTypeStr = Types::convert<FromInt, ToLiteral>(fromType);
                    } catch (const karabo::data::Exception& e) {
                        karabo::data::Exception::clearTrace();
                    }
                    // For unsupported types T, Types::from<T>() does not even compile and we always get a valid
                    // toTypeStr
                    const Types::ReferenceType toType = Types::from<T>();
                    const std::string toTypeStr = Types::convert<FromInt, ToLiteral>(toType);
                    throw KARABO_CAST_EXCEPTION(
                          "NDArray::getData(): Failed to cast "
                          "from " +
                                fromTypeStr + " (" + toString(fromType) +=
                          ") "
                          "to " +
                          toTypeStr + " (" + toString(toType) += ")");
                }
            }

            template <typename T>
            T* getData() {
                // Call the const version of getData:
                const T* data = const_cast<const NDArray*>(this)->getData<T>();
                // Convert back result to non-const:
                return const_cast<T*>(data);
            }

            /**
             * Get a shared pointer to the underlying ByteArray data
             * @return
             */
            const DataPointer& getDataPtr() const;

            /**
             * Return the underlying ByteArray
             * @return
             */
            ByteArray getByteArray();

            const ByteArray getByteArray() const;

            /**
             * Return the shape of the array as a karabo::data::Dims object
             * @return
             */
            Dims getShape() const;

            /**
             * Evaluate if the data contained in the array is big endian
             * @return true if big endian, false if little ednian
             */
            bool isBigEndian() const;

            /**
             * Convert data to little endian
             */
            void toLittleEndian();

            /**
             * Convert data to big endian
             */
            void toBigEndian();

            NDArray copy() const;

           private:
            template <typename T>
            void setType() {
                set("type", static_cast<int>(karabo::data::Types::from<T>()));
            }

            template <typename T>
            void setData(const Dims& shape) {
                const size_t byteSize = shape.size() * sizeof(T);
                set("data", std::make_pair(DataPointer(new char[byteSize], &NDArray::deallocator), byteSize));
            }

            template <typename T>
            void setData(const T* data, const size_t nelems) {
                const size_t byteSize = nelems * sizeof(T);
                char* buffer = new char[byteSize];
                std::memcpy(buffer, reinterpret_cast<const char*>(data), byteSize);
                set("data", std::make_pair(DataPointer(buffer, &NDArray::deallocator), byteSize));
            }

            template <typename T, typename D>
            void setData(const T* data, const size_t nelems, const D& deleter) {
                const size_t byteSize = nelems * sizeof(T);
                char* tmp = const_cast<char*>(reinterpret_cast<const char*>(data));
                set("data", std::make_pair(DataPointer(tmp, deleter), byteSize));
            }

            void setBigEndian(const bool isBigEndian);

            void swapEndianess();

            static void deallocator(const char* p);
        };

        // Implementation of template functions

        template <typename T>
        NDArray::NDArray(const Dims& shape, const T& fill, const bool isBigEndian) {
            const size_t itemSize = sizeof(T);
            const size_t byteSize = shape.size() * itemSize;
            T* buffer = new T[shape.size()];
            for (size_t i = 0; i < shape.size(); ++i) {
                buffer[i] = fill;
            }
            set("data", std::make_pair(DataPointer(reinterpret_cast<char*>(buffer), &NDArray::deallocator), byteSize));
            setType<T>();
            setShape(shape);
            setBigEndian(isBigEndian);
        }

        template <typename T>
        NDArray::NDArray(const T* dataPtr, const size_t numElems, const Dims& shape, const bool isBigEndian) {
            setData(dataPtr, numElems);
            setType<T>();
            setShape(shape);
            setBigEndian(isBigEndian);
        }

        template <typename InputIterator>
        NDArray::NDArray(InputIterator first, InputIterator last, const Dims& shape, const bool isBigEndian) {
            using DataType = typename std::iterator_traits<InputIterator>::value_type;

            // Create data structure and specify type
            const size_t byteSize = (last - first) * sizeof(DataType);
            set("data", std::make_pair(DataPointer(new char[byteSize], &NDArray::deallocator), byteSize));
            setType<DataType>();

            // Set further input
            setShape(shape); // after set up of data structure!
            setBigEndian(isBigEndian);

            // In the end, copy from the iterator:
            DataType* data = getData<DataType>();
            for (; first != last; ++first) {
                *(data++) = *first; // postfix increment!
            }
        }

        template <typename T, typename D>
        NDArray::NDArray(const T* dataPtr, const size_t numElems, const D& deleter, const Dims& shape,
                         const bool isBigEndian) {
            setData(dataPtr, numElems, deleter);
            setType<T>();
            setShape(shape);
            setBigEndian(isBigEndian);
        }

    } // namespace data
} // namespace karabo

#endif
