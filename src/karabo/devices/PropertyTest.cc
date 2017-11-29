#include "PropertyTest.hh"

#include "karabo/util/Schema.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/State.hh"
#include "karabo/util/Dims.hh"
#include "karabo/util/NDArray.hh"
#include "karabo/xms/ImageData.hh"


namespace karabo {
    namespace devices {

        using namespace std;
        using namespace karabo::core;
        using namespace karabo::util;
        using namespace karabo::net;
        using namespace karabo::xms;

        const unsigned int defVectorMaxSize = 100;

        KARABO_REGISTER_FOR_CONFIGURATION(NestedClass)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, PropertyTest)


        NestedClass::NestedClass(const karabo::util::Hash& input) {
        }


        NestedClass::~NestedClass() {
        }


        void NestedClass::expectedParameters(Schema& expected) {

            STRING_ELEMENT(expected).key("e1")
                    .displayedName("E1")
                    .description("E1 property")
                    .assignmentOptional().defaultValue("E1")
                    .reconfigurable()
                    .commit();

            BOOL_ELEMENT(expected).key("e2")
                    .displayedName("E2")
                    .description("E2 property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(false)
                    .commit();

            INT32_ELEMENT(expected).key("e3")
                    .displayedName("E3")
                    .description("E3 property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(77)
                    .commit();
        }


        void PropertyTest::expectedParameters(Schema& expected) {

            OVERWRITE_ELEMENT(expected).key("state")
                    .setNewOptions(State::INIT, State::NORMAL, State::ERROR)
                    .setNewDefaultValue(State::INIT)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue<int>(Schema::AccessLevel::ADMIN)
                    .commit();

            BOOL_ELEMENT(expected).key("boolProperty")
                    .displayedName("Bool property")
                    .description("A bool property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(false)
                    .commit();

            CHAR_ELEMENT(expected).key("charProperty")
                    .displayedName("Char property")
                    .description("A char property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue('A')
                    .commit();

            INT8_ELEMENT(expected).key("int8Property")
                    .displayedName("Int8 property")
                    .description("A int8 property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(77)
                    .commit();

            UINT8_ELEMENT(expected).key("uint8Property")
                    .displayedName("UInt8 property")
                    .description("A uint8 property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(177)
                    .commit();

            INT16_ELEMENT(expected).key("int16Property")
                    .displayedName("Int16 property")
                    .description("A int16 property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(3200)
                    .commit();

            UINT16_ELEMENT(expected).key("uint16Property")
                    .displayedName("UInt16 property")
                    .description("A uint16 property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(32000)
                    .commit();

            INT32_ELEMENT(expected).key("int32Property")
                    .displayedName("Int32 property")
                    .description("A int32 property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(32000000)
                    .commit();

            UINT32_ELEMENT(expected).key("uint32Property")
                    .displayedName("UInt32 property")
                    .description("A uint32 property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(32000000)
                    .commit();

            INT64_ELEMENT(expected).key("int64Property")
                    .displayedName("Int64 property")
                    .description("A int64 property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(3200000000LL)
                    .commit();

            UINT64_ELEMENT(expected).key("uint64Property")
                    .displayedName("UInt64 property")
                    .description("A uint64 property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(3200000000ULL)
                    .commit();

            FLOAT_ELEMENT(expected).key("floatProperty")
                    .displayedName("Float property")
                    .description("A float property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(3.141596)
                    .commit();

            DOUBLE_ELEMENT(expected).key("doubleProperty")
                    .displayedName("Double property")
                    .description("A double property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(3.1415967773331)
                    .commit();

            STRING_ELEMENT(expected).key("stringProperty")
                    .displayedName("String property")
                    .description("A string property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue("Some arbitrary text.")
                    .commit();

            NODE_ELEMENT(expected).key("vectors")
                    .displayedName("Vectors")
                    .description("A node containing vector properties")
                    .commit();

            VECTOR_BOOL_ELEMENT(expected).key("vectors.boolProperty")
                    .displayedName("Bool property")
                    .description("A vector boolean property")
                    .reconfigurable()
                    .minSize(1)
                    .maxSize(10)
                    .assignmentOptional().defaultValue({true, false, true,
                                                       false, true, false})
                    .commit();

            VECTOR_CHAR_ELEMENT(expected).key("vectors.charProperty")
                    .displayedName("Char property")
                    .description("A vector character property")
                    .reconfigurable()
                    .minSize(1)
                    .maxSize(10)
                    .assignmentOptional().defaultValue({'A', 'B', 'C', 'D', 'E', 'F'})
                    .commit();

            VECTOR_INT8_ELEMENT(expected).key("vectors.int8Property")
                    .displayedName("Int8 property")
                    .description("A vector int8 property")
                    .reconfigurable()
                    .minSize(1)
                    .maxSize(10)
                    .assignmentOptional().defaultValue({41, 42, 43,
                                                       44, 45, 46})
                    .commit();

            VECTOR_UINT8_ELEMENT(expected).key("vectors.uint8Property")
                    .displayedName("UInt8 property")
                    .description("A vector uint8 property")
                    .reconfigurable()
                    .minSize(1)
                    .maxSize(10)
                    .assignmentOptional().defaultValue({41, 42, 43,
                                                       44, 45, 46})
                    .commit();

            VECTOR_INT16_ELEMENT(expected).key("vectors.int16Property")
                    .displayedName("Int16 property")
                    .description("A vector int16 property")
                    .reconfigurable()
                    .minSize(1)
                    .maxSize(10)
                    .assignmentOptional().defaultValue({20041, 20042, 20043, 20044, 20045, 20046})
                    .commit();

            VECTOR_UINT16_ELEMENT(expected).key("vectors.uint16Property")
                    .displayedName("UInt16 property")
                    .description("A vector uint16 property")
                    .reconfigurable()
                    .minSize(1)
                    .maxSize(10)
                    .assignmentOptional().defaultValue({10041, 10042, 10043,
                                                       10044, 10045, 10046})
                    .commit();

            VECTOR_INT32_ELEMENT(expected).key("vectors.int32Property")
                    .displayedName("Int32 property")
                    .description("A vector int32 property")
                    .reconfigurable()
                    .minSize(1)
                    .maxSize(10)
                    .assignmentOptional().defaultValue({20000041, 20000042, 20000043,
                                                       20000044, 20000045, 20000046})
                    .commit();

            VECTOR_UINT32_ELEMENT(expected).key("vectors.uint32Property")
                    .displayedName("UInt32 property")
                    .description("A vector uint32 property")
                    .reconfigurable()
                    .minSize(1)
                    .maxSize(10)
                    .assignmentOptional().defaultValue({90000041, 90000042, 90000043,
                                                       90000044, 90000045, 90000046})
                    .commit();

            VECTOR_INT64_ELEMENT(expected).key("vectors.int64Property")
                    .displayedName("Int64 property")
                    .description("A vector int64 property")
                    .reconfigurable()
                    .minSize(1)
                    .maxSize(10)
                    .assignmentOptional().defaultValue({20000000041LL, 20000000042LL, 20000000043LL,
                                                       20000000044LL, 20000000045LL, 20000000046LL})
                    .commit();

            VECTOR_UINT64_ELEMENT(expected).key("vectors.uint64Property")
                    .displayedName("UInt64 property")
                    .description("A vector uint64 property")
                    .reconfigurable()
                    .minSize(1)
                    .maxSize(10)
                    .assignmentOptional().defaultValue({90000000041ULL, 90000000042ULL, 90000000043ULL,
                                                       90000000044ULL, 90000000045ULL, 90000000046ULL})
                    .commit();

            VECTOR_FLOAT_ELEMENT(expected).key("vectors.floatProperty")
                    .displayedName("Float property")
                    .description("A vector float property")
                    .reconfigurable()
                    .minSize(1)
                    .maxSize(10)
                    .assignmentOptional().defaultValue({1.23456, 2.34567, 3.45678, 4.56789, 5.67891, 6.78912})
                    .commit();

            VECTOR_DOUBLE_ELEMENT(expected).key("vectors.doubleProperty")
                    .displayedName("Double property")
                    .description("A vector double property")
                    .reconfigurable()
                    .minSize(1).maxSize(10)
                    .assignmentOptional().defaultValue({1.234567891, 2.345678912, 3.456789123,
                                                       4.567891234, 5.678901234, 6.123456789})
                    .commit();

            VECTOR_STRING_ELEMENT(expected).key("vectors.stringProperty")
                    .displayedName("String property")
                    .description("A vector string property")
                    .reconfigurable()
                    .minSize(1).maxSize(10)
                    .assignmentOptional().defaultValue({"1111111", "2222222", "3333333",
                                                       "4444444", "5555555", "6666666"})
                    .commit();

            Schema columns;

            FLOAT_ELEMENT(columns).key("e4")
                    .displayedName("E4")
                    .description("E4 property")
                    .assignmentOptional().defaultValue(3.1415F)
                    .reconfigurable()
                    .commit();

            DOUBLE_ELEMENT(columns).key("e5")
                    .displayedName("E5")
                    .description("E5 property")
                    .assignmentOptional().defaultValue(2.78)
                    .reconfigurable()
                    .commit();

            TABLE_ELEMENT(expected).key("table")
                    .displayedName("Table property")
                    .description("Table containing one node.")
                    .addColumnsFromClass<NestedClass>()
                    .addColumns(columns)
                    .assignmentOptional().defaultValue({Hash("e1", "abc", "e2", true, "e3", 12, "e4", 0.9837F, "e5", 1.2345),
                                                       Hash("e1", "xyz", "e2", false, "e3", 42, "e4", 2.33333F, "e5", 7.77777)})
                    .reconfigurable()
                    .commit();


            Schema pipeData;
            NODE_ELEMENT(pipeData).key("node")
                    .displayedName("Node for DAQ")
                    .description("An intermediate node needed by DAQ")
                    .setDaqDataType(karabo::util::TRAIN)
                    .commit();

            INT32_ELEMENT(pipeData).key("node.int32")
                    .description("A signed 32-bit integer sent via the pipeline")
                    .readOnly()
                    .commit();

            STRING_ELEMENT(pipeData).key("node.string")
                    .description("A string send via the pipeline")
                    .readOnly()
                    .commit();

            VECTOR_INT64_ELEMENT(pipeData).key("node.vecInt64")
                    .description("A vector of signed 64-bit integers sent via the pipeline")
                    .maxSize(defVectorMaxSize) // DAQ needs that
                    .readOnly()
                    .commit();

            NDARRAY_ELEMENT(pipeData).key("node.ndarray")
                    .description("A multi dimensional array of floats sent via the pipeline")
                    .dtype(Types::FLOAT)
                    .shape("100,200")
                    .commit();

            IMAGEDATA_ELEMENT(pipeData).key("node.image")
                    .description("An image with pixels as 16-bit unsigned integers sent via the pipeline")
                    .setDimensions("400,500")
                    .setEncoding(Encoding::GRAY)
                    // guess that DAQ needs more...
                    .commit();

            OUTPUT_CHANNEL(expected).key("output")
                    .displayedName("Output")
                    .dataSchema(pipeData)
                    .commit();

            SLOT_ELEMENT(expected).key("writeOutput")
                    .displayedName("Write to Output")
                    .description("Write once to output channel 'Output'")
                    .commit();

            SLOT_ELEMENT(expected).key("slotUpdateSchema")
                    .displayedName("Update Schema")
                    .description("Duplicate maxSize of vectors in schema")
                    .commit();
        }


        PropertyTest::PropertyTest(const Hash& input) : Device<>(input), m_outputCounter(0ll) {
            KARABO_INITIAL_FUNCTION(initialize);

            KARABO_SLOT(writeOutput);
            KARABO_SLOT(slotUpdateSchema);
        }


        PropertyTest::~PropertyTest() {
        }


        void PropertyTest::initialize() {
            // some initialization
            updateState(State::NORMAL);
        }


        void PropertyTest::writeOutput() {

            ++m_outputCounter;

            // Set all numbers inside to m_outputCounter:
            Hash data;
            Hash& node = data.bindReference<Hash>("node");
            node.set("int32", static_cast<int> (m_outputCounter));
            node.set("string", toString(m_outputCounter));
            node.set("vecInt64", std::vector<long long>(defVectorMaxSize, m_outputCounter));
            node.set("ndarray", NDArray(Dims(100ull, 200ull), static_cast<float> (m_outputCounter)));
            node.set("image", ImageData(NDArray(Dims(400ull, 500ull), static_cast<unsigned short> (m_outputCounter)),
                                        Dims(), // use Dims of NDArray
                                        Encoding::GRAY, // gray scale as is default
                                        16)); // unsigned short is 16 bits

            writeChannel("output", data);
        }


        void PropertyTest::slotUpdateSchema() {
            const Schema schema(getFullSchema());
            const Hash vectors(get<Hash>("vectors"));
            std::set<std::string> keys;
            vectors.getKeys(keys);
            size_t counter = 0;
            for (const std::string& key : keys) {
                const std::string path("vectors." + key);
                // Only for last key send update:
                const bool sendUpdate = (++counter >= keys.size());
                appendSchemaMaxSize(path, schema.getMaxSize(path) * 2, sendUpdate);
            }
        }
    }
}
