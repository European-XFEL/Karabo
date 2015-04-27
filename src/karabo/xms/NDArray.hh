/* 
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 23, 2015, 10:17 AM
 */

#ifndef KARABO_XMS_NDARRAY_HH
#define	KARABO_XMS_NDARRAY_HH

#include "Data.hh"

namespace karabo {
    namespace xms {

        namespace Dimension {

            enum DimensionType {
                UNDEFINED = 0,
                STACK = -1,
                DATA = 1,
            };
        }

        typedef Dimension::DimensionType DimensionType;

        class NDArray : public Data {
        public:

            KARABO_CLASSINFO(NDArray, "NDArray", "1.3");

            static void expectedParameters(karabo::util::Schema& expected);

            NDArray();

            /**
             * Configuration constructor
             * This constructor should be used for filling in data that is going to be sent
             */
            NDArray(const karabo::util::Hash& config);
            
            
            NDArray(const karabo::util::Hash::Pointer& data);                           
            

            template <class T>
            NDArray(const T * const data,
                    const size_t size,
                    const bool copy = true,
                    const karabo::util::Dims& dimensions = karabo::util::Dims(),
                    const bool isBigEndian = karabo::util::isBigEndian());

            
            virtual ~NDArray();
            

            const char* getDataPointer() const;

            size_t getByteSize() const;

            const std::vector<char>& getData();                        

            template <class T>
            inline void setData(const std::vector<T>& data, const bool copy = true) {
                this->setData(&data[0], data.size(), copy);
            }

            template <class T>
            inline void setData(const T* data, const size_t size, const bool copy = true) {

                size_t byteSize = size * sizeof (T);

                if (copy) {
                    boost::optional<karabo::util::Hash::Node&> node = m_hash->find("data");
                    if (node) {
                        std::vector<char>& buffer = node->getValue<std::vector<char> >();
                        buffer.resize(byteSize);
                        std::memcpy(&buffer[0], reinterpret_cast<const char*> (data), byteSize);
                    } else {
                        std::vector<char>& buffer = m_hash->bindReference<std::vector<char> >("data");
                        buffer.resize(byteSize);
                        std::memcpy(&buffer[0], reinterpret_cast<const char*> (data), byteSize);
                    }
                } else {
                    // We have to be very careful with the exact type here. For the RTTI const T* and T* are different!
                    // The Type system currently knows only about pair<const T*, size_t> NOT pair<T*, size_t>
                    m_hash->set("data", std::make_pair(reinterpret_cast<const char*> (data), byteSize));
                }

                m_hash->set("dataType", karabo::util::Types::to<karabo::util::ToLiteral>(karabo::util::Types::from<T>()));
            }

            karabo::util::Dims getDimensions() const;

            void setDimensions(const karabo::util::Dims& dims);

            //void setDimensionTypes(const std::vector<int>& dimTypes);

            const std::string& getDataType() const;

            void setIsBigEndian(const bool isBigEndian);

            bool isBigEndian() const;

            //const std::vector<std::vector<std::string> >& getDimensionScales() const;
            
            //void setDimensionScales(const std::string& scales);

        protected:


            bool dataIsCopy() const;

            void ensureDataOwnership();


        };
        
        
        struct NDArrayElement : public DataElement<NDArrayElement, NDArray> {
            
            NDArrayElement(karabo::util::Schema& s) : DataElement<NDArrayElement, NDArray>(s) {
            }                        
            
            NDArrayElement& setDimensionScales(const std::string& scales) {
                return setDefaultValue("dimScales", scales);
            }
            
            NDArrayElement& setDimensions(const std::string& dimensions) {
                return setDefaultValue("dims", karabo::util::fromString<unsigned long long, std::vector>(dimensions));
            }                        
        };
        
        typedef NDArrayElement NDARRAY_ELEMENT;
    }
}



#endif

