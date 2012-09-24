#include <iostream>
#include <string>
#include <ostream>
#include <istream>
#include <boost/assert.hpp>
#include <boost/assign.hpp>
#include <boost/filesystem.hpp>
#include <karabo/util/Types.hh>
#include <karabo/util/Factory.hh>
#include <karabo/util/Hash.hh>
#include "PlcFormat.hh"

using namespace std;
using namespace boost::assign;
using namespace exfel::util;

// TODO Currently taken out
//EXFEL_REGISTER_FACTORY_CC(exfel::io::Format<exfel::util::Hash>, exfel::io::PlcFormat) 

namespace exfel {
    namespace io {

        struct PlcTypeMap {

            enum Type {
                BOOL, // bool
                INT8, // signed char
                INT16, // signed short
                INT32, // signed int
                INT64, // signed long long
                UINT8, // unsigned char
                UINT16, // unsigned short
                UINT32, // unsigned int
                UINT64, // unsigned long long
                FLOAT, // float
                COMPLEX_FLOAT, // complex<float>
                DOUBLE, // double
                COMPLEX_DOUBLE, // complex<double>
                STRING, // std::string
                CONST_CHAR_PTR, // const char*
                VECTOR_STRING, // std::vector<std::string>
                VECTOR_PATH, // std::vector<boost::filesystem::path>
                VECTOR_INT8, // std::vector<std::signed char>
                VECTOR_INT16, // std::vector<std::signed short>
                VECTOR_INT32, // std::vector<std::int>
                VECTOR_INT64, // std::vector<std::signed long long>
                VECTOR_UINT8, // std::vector<std::unsigned char>
                VECTOR_UINT16, // std::vector<std::unsigned short>
                VECTOR_UINT32, // std::vector<std::unsigned int>
                VECTOR_UINT64, // std::vector<std::unsigned long long>
                VECTOR_BOOL, // std::deque<std::bool>
                VECTOR_DOUBLE, // std::vector<std::double>
                VECTOR_FLOAT // std::vector<std::float>
            };

            static std::map<exfel::util::Types::Type, Type> toProType;
            static std::map<Type, exfel::util::Types::Type> fromProType;
        };

        void PlcFormat::expectedParameters(Schema& expected) {

            STRING_ELEMENT(expected)
                    .key("separator")
                    .displayedName("Separator")
                    .description("Separator symbol used to represent Hash hierarchy")
                    .assignmentOptional().defaultValue("\n")
                    .reconfigurable()
                    .commit();

            INTERNAL_ANY_ELEMENT(expected).key("dictionary")
                    .description("The configuration containing alias2key and key2alias tables")
                    .commit();

        }

        void PlcFormat::configure(const Hash& input) {
            input.get("dictionary", m_config);
            input.get("separator", m_sep);
        }

        int PlcFormat::readKey(istream& is, string& key) {
            int length = 0;
            int alias;
            int rc = this->readFrom(is, &alias, sizeof (alias));
            if (rc < 0)
                return length;
            length += rc;
            try {
                key = m_config.alias2key<int>(alias);
            } catch (...) {
                key = String::toString(alias);
            }
            return length;
        }

        int PlcFormat::readType(istream& is, Types::Type& id) {
            int length = 0;
            PlcTypeMap::Type type;
            int rc = this->readFrom(is, &type, sizeof (type));
            if (rc < 0)
                return length;
            length += rc;
            id = PlcTypeMap::fromProType[type];
            return length;
        }

        void PlcFormat::writeKey(std::ostream& os, const std::string& key) {
            try {
                int alias = m_config.key2alias<int>(key);
                this->writeTo(os, &alias, sizeof (int));
            } catch (const ParameterException&) {
                throw PARAMETER_EXCEPTION("PlcTypeMap::nameEncode -> Key \"" + key + "\" does not exist");
            } catch (...) {
                RETHROW
            }
        }

        void PlcFormat::writeType(ostream& os, Types::Type id) {
            try {
                PlcTypeMap::Type type = PlcTypeMap::toProType[id];
                this->writeTo(os, &type, sizeof (PlcTypeMap::Type));
            } catch (...) {
                RETHROW
            }
        }

        // 'toProType' table
        map<Types::Type, PlcTypeMap::Type> PlcTypeMap::toProType = map_list_of
                (Types::BOOL, PlcTypeMap::BOOL)
        (Types::INT8, PlcTypeMap::INT8)
        (Types::INT16, PlcTypeMap::INT16)
        (Types::INT32, PlcTypeMap::INT32)
        (Types::INT64, PlcTypeMap::INT64)
        (Types::UINT8, PlcTypeMap::UINT8)
        (Types::UINT16, PlcTypeMap::UINT16)
        (Types::UINT32, PlcTypeMap::UINT32)
        (Types::UINT64, PlcTypeMap::UINT64)
        (Types::FLOAT, PlcTypeMap::FLOAT)
        (Types::DOUBLE, PlcTypeMap::DOUBLE)
        (Types::STRING, PlcTypeMap::STRING)
        (Types::CONST_CHAR_PTR, PlcTypeMap::CONST_CHAR_PTR)
        (Types::VECTOR_STRING, PlcTypeMap::VECTOR_STRING)
        (Types::VECTOR_INT8, PlcTypeMap::VECTOR_INT8)
        (Types::VECTOR_INT16, PlcTypeMap::VECTOR_INT16)
        (Types::VECTOR_INT32, PlcTypeMap::VECTOR_INT32)
        (Types::VECTOR_INT64, PlcTypeMap::VECTOR_INT64)
        (Types::VECTOR_UINT8, PlcTypeMap::VECTOR_UINT8)
        (Types::VECTOR_UINT16, PlcTypeMap::VECTOR_UINT16)
        (Types::VECTOR_UINT32, PlcTypeMap::VECTOR_UINT32)
        (Types::VECTOR_UINT64, PlcTypeMap::VECTOR_UINT64)
        (Types::VECTOR_BOOL, PlcTypeMap::VECTOR_BOOL)
        (Types::VECTOR_DOUBLE, PlcTypeMap::VECTOR_DOUBLE)
        (Types::VECTOR_FLOAT, PlcTypeMap::VECTOR_FLOAT)
        ;
        // 'fromProType' table
        map<PlcTypeMap::Type, Types::Type> PlcTypeMap::fromProType = map_list_of
                (PlcTypeMap::BOOL, Types::BOOL)
        (PlcTypeMap::INT8, Types::INT8)
        (PlcTypeMap::INT16, Types::INT16)
        (PlcTypeMap::INT32, Types::INT32)
        (PlcTypeMap::INT64, Types::INT64)
        (PlcTypeMap::UINT8, Types::UINT8)
        (PlcTypeMap::UINT16, Types::UINT16)
        (PlcTypeMap::UINT32, Types::UINT32)
        (PlcTypeMap::UINT64, Types::UINT64)
        (PlcTypeMap::FLOAT, Types::FLOAT)
        (PlcTypeMap::DOUBLE, Types::DOUBLE)
        (PlcTypeMap::STRING, Types::STRING)
        (PlcTypeMap::CONST_CHAR_PTR, Types::CONST_CHAR_PTR)
        (PlcTypeMap::VECTOR_STRING, Types::VECTOR_STRING)
        (PlcTypeMap::VECTOR_INT8, Types::VECTOR_INT8)
        (PlcTypeMap::VECTOR_INT16, Types::VECTOR_INT16)
        (PlcTypeMap::VECTOR_INT32, Types::VECTOR_INT32)
        (PlcTypeMap::VECTOR_INT64, Types::VECTOR_INT64)
        (PlcTypeMap::VECTOR_UINT8, Types::VECTOR_UINT8)
        (PlcTypeMap::VECTOR_UINT16, Types::VECTOR_UINT16)
        (PlcTypeMap::VECTOR_UINT32, Types::VECTOR_UINT32)
        (PlcTypeMap::VECTOR_UINT64, Types::VECTOR_UINT64)
        (PlcTypeMap::VECTOR_BOOL, Types::VECTOR_BOOL)
        (PlcTypeMap::VECTOR_DOUBLE, Types::VECTOR_DOUBLE)
        (PlcTypeMap::VECTOR_FLOAT, Types::VECTOR_FLOAT)
        ;
    }
}
