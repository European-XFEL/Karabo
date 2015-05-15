/* 
 * Author: 
 * 
 * Created on March 23, 2015, 10:17 AM
 */

#include "NDArray.hh"
#include "karabo/util/LeafElement.hh"

using namespace karabo::util;

namespace karabo {
    namespace xms {


        KARABO_REGISTER_FOR_CONFIGURATION(Data, NDArray)

        // Output Schema (i.e. describes how to fill for sending)
        void NDArray::expectedParameters(karabo::util::Schema& s) {

            VECTOR_CHAR_ELEMENT(s).key("data")
                    .displayedName("Data")
                    .description("Pixel array")
                    .readOnly()
                    .commit();
            STRING_ELEMENT(s).key("dataType")
                    .displayedName("Type")
                    .description("Describes the underlying data type")
                    .readOnly()
                    .commit();
            VECTOR_UINT32_ELEMENT(s).key("dims")
                    .displayedName("Dimensions")
                    .description("The length of the array reflects total dimensionality and each element the extension in this dimension")
                    .readOnly()
                    .commit();
            VECTOR_INT32_ELEMENT(s).key("dimTypes")
                    .displayedName("Dimension Types")
                    .description("Any dimension should have an enumerated type")
                    .readOnly()
                    .commit();
            STRING_ELEMENT(s).key("dimScales")
                    .displayedName("Dimension Scales")
                    .description("")
                    .readOnly()
                    .commit();
            BOOL_ELEMENT(s).key("isBigEndian")
                    .displayedName("Is big endian")
                    .description("Flags whether the raw data are in big or little endian")
                    .readOnly()
                    .commit();
        }


        NDArray::NDArray() : Data() {
        }


        NDArray::NDArray(const karabo::util::Hash& config) : Data(config) {
        }


        NDArray::NDArray(const karabo::util::Hash::Pointer& data) : Data(data) {
        }


        NDArray::~NDArray() {
        }


        const char* NDArray::getDataPointer() const {
            boost::optional<karabo::util::Hash::Node&> node = m_hash->find("data");
            if (node) {
                if (node->getType() == Types::VECTOR_CHAR) {
                    return &(m_hash->get<std::vector<char> >("data"))[0];
                }
                return m_hash->get<std::pair<const char*, size_t> >("data").first;
            }
            return 0;
        }


        const std::vector<char>& NDArray::getData() {
            ensureDataOwnership();
            return m_hash->get<std::vector<char> >("data");
        }


        void NDArray::ensureDataOwnership() {
            if (!dataIsCopy()) {
                setData(getDataPointer(), getByteSize());
            }
        }


        bool NDArray::dataIsCopy() const {
            boost::optional<karabo::util::Hash::Node&> node = m_hash->find("data");
            if (node) return node->getType() == Types::VECTOR_CHAR;
            else return true;
        }


        size_t NDArray::getByteSize() const {
            boost::optional<karabo::util::Hash::Node&> node = m_hash->find("data");
            if (node) {
                if (node->getType() == Types::VECTOR_CHAR) {
                    return m_hash->get<std::vector<char> >("data").size();
                }
                return m_hash->get<std::pair<const char*, size_t> >("data").second;
            }
            return 0;
        }


        karabo::util::Dims NDArray::getDimensions() const {
            return karabo::util::Dims(m_hash->get<std::vector<unsigned long long> >("dims"));
        }


        void NDArray::setDimensions(const karabo::util::Dims& dimensions) {
            m_hash->set<vector<unsigned long long> >("dims", dimensions.toVector());
            // In case the dimensionTypes were not yet set, inject a default here
            if (!m_hash->has("dimTypes")) {
                m_hash->set("dimTypes", vector<int>(dimensions.rank(), Dimension::UNDEFINED));
            }
        }


        void NDArray::setDimensionTypes(const std::vector<int>& dimTypes) {
            m_hash->set("dimTypes", dimTypes);
        }
        
        
        const std::vector<int>& NDArray::getDimensionTypes() {
            return m_hash->get<vector<int > >("dimTypes");
        }
        
        
        const std::string& NDArray::getDataType() const {
            return m_hash->get<string>("dataType");
        }


        void NDArray::setIsBigEndian(const bool isBigEndian) {
            m_hash->set<bool>("isBigEndian", isBigEndian);
        }


        bool NDArray::isBigEndian() const {
            return m_hash->get<bool>("isBigEndian");
        }


        const std::string& NDArray::getDimensionScales() const {
            return m_hash->get<string>("dimScales");
        }

        
        void NDArray::setDimensionScales(const std::string& scales) {
            m_hash->set("dimScales", scales);
        }
             
    }
}
