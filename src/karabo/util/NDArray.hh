/*
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 23, 2015, 10:17 AM
 */

#ifndef KARABO_UTIL_NDARRAY_HH
#define	KARABO_UTIL_NDARRAY_HH

#include "Exception.hh"
#include "Dims.hh"
#include "Hash.hh"
#include "Schema.hh"
#include "ByteSwap.hh"

namespace karabo {
    namespace util {

        class NDArray : protected Hash {

        public:

            KARABO_CLASSINFO(NDArray, "NDArray", "1.5");

            typedef boost::shared_ptr<char> DataPointer;

            struct NullDeleter {

                void operator()(void const *) const {
                    // Do nothing
                }
            };

            static void expectedParameters(karabo::util::Schema& s);

            NDArray(const Dims& shape,
                    const karabo::util::Types::ReferenceType& type = karabo::util::Types::DOUBLE,
                    const bool isBigEndian = karabo::util::isBigEndian());

            template <typename T>
            NDArray(const Dims& shape,
                    const T& fill,
                    const bool isBigEndian = karabo::util::isBigEndian());

            /**
             * This constructor copies data from the provided memory location.
             * Internally the data is kept as shared pointer.
             * @param dataPtr Typed address to a stretch of contiguous memory
             * @param numElems Number of elements (of type T)
             * @param shape Shape information
             * @param isBigEndian Endianess flag
             */
            template <typename T>
            NDArray(const T* dataPtr,
                    const size_t numElems,
                    const Dims& shape = Dims(),
                    const bool isBigEndian = karabo::util::isBigEndian());

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
            template<typename T, typename D>
            NDArray(const T* dataPtr,
                    const size_t numElems,
                    const D& deleter,
                    const Dims& shape = Dims(),
                    const bool isBigEndian = karabo::util::isBigEndian());


            /**
             * Non-templated no-copy construction.
             * @param ptr Shared_ptr to external memory location
             * @param type Type of provided data
             * @param itemSize Item size (element size)
             * @param numElems Number of elements of provided type
             * @param shape Shape information
             * @param isBigEndian Endianess flag
             */
            NDArray(const DataPointer& ptr,
                    const karabo::util::Types::ReferenceType& type,
                    const size_t& numElems,
                    const Dims& shape = Dims(),
                    const bool isBigEndian = karabo::util::isBigEndian());

            virtual ~NDArray() {
            }

            void setShape(const Dims& shape);

            karabo::util::Types::ReferenceType getType() const;

            /**
             * The number of items in the array.
             */
            const size_t size() const;

            /**
             * The total size of the array, in bytes.
             */
            inline size_t byteSize() const;

            /**
             * The size of each item, in bytes.
             */
            inline size_t itemSize() const;

            template <typename T>
            const T* getData() const {
                if (get<int>("type") == karabo::util::Types::from<T>()) {
                    return reinterpret_cast<T*> (get<ByteArray>("data").first.get());
                } else {
                    throw KARABO_CAST_EXCEPTION("Bad cast");
                }
            }

            template <typename T>
            T* getData() {
                if (get<int>("type") == karabo::util::Types::from<T>()) {
                    return reinterpret_cast<T*> (get<ByteArray>("data").first.get());
                } else {
                    throw KARABO_CAST_EXCEPTION("Bad cast");
                }
            }

            const DataPointer& getDataPtr() const;

            ByteArray getByteArray();

            const ByteArray getByteArray() const;

            Dims getShape() const;

            bool isBigEndian() const;

            void toLittleEndian();

            void toBigEndian();

        private:

            void setClassId();

            template <typename T>
            void setType() {
                set("type", static_cast<int> (karabo::util::Types::from<T>()));
            }

            template<typename T>
            void setData(const Dims& shape) {
                const size_t byteSize = shape.size() * sizeof (T);
                set("data", std::make_pair(DataPointer(new char[byteSize], &NDArray::deallocator), byteSize));
            }

            template <typename T>
            void setData(const T* data, const size_t nelems) {
                const size_t byteSize = nelems * sizeof (T);
                char* buffer = new char[byteSize];
                std::memcpy(buffer, reinterpret_cast<const char*> (data), byteSize);
                set("data", std::make_pair(DataPointer(buffer, &NDArray::deallocator), byteSize));
            }

            template <typename T, typename D>
            void setData(const T* data, const size_t nelems, const D& deleter) {
                const size_t byteSize = nelems * sizeof (T);
                char* tmp = const_cast<char*> (reinterpret_cast<const char*> (data));
                set("data", std::make_pair(DataPointer(tmp, deleter), byteSize));
            }

            void setBigEndian(const bool isBigEndian);

            void swapEndianess();

            static void deallocator(const char* p);

        };

        // Implementation of template functions

        template <typename T>
        NDArray::NDArray(const Dims& shape,
                         const T& fill,
                         const bool isBigEndian) {
            setClassId();
            const size_t itemSize = sizeof (T);
            const size_t byteSize = shape.size() * itemSize;
            T* buffer = new T[shape.size()];
            for (size_t i = 0; i < shape.size(); ++i) {
                buffer[i] = fill;
            }
            set("data", std::make_pair(DataPointer(reinterpret_cast<char*> (buffer), &NDArray::deallocator), byteSize));
            setType<T>();
            setShape(shape);
            setBigEndian(isBigEndian);
        }

        template <typename T>
        NDArray::NDArray(const T* dataPtr,
                         const size_t numElems,
                         const Dims& shape,
                         const bool isBigEndian) {
            setClassId();
            setData(dataPtr, numElems);
            setType<T>();
            setShape(shape);
            setBigEndian(isBigEndian);
        }

        template<typename T, typename D>
        NDArray::NDArray(const T* dataPtr,
                         const size_t numElems,
                         const D& deleter,
                         const Dims& shape,
                         const bool isBigEndian) {
            setClassId();
            setData(dataPtr, numElems, deleter);
            setType<T>();
            setShape(shape);
            setBigEndian(isBigEndian);
        }

    }
}

#endif
