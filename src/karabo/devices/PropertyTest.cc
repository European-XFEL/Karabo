#include "PropertyTest.hh"

#include "karabo/net/EventLoop.hh"
#include "karabo/util/Schema.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/State.hh"
#include "karabo/util/Dims.hh"
#include "karabo/util/NDArray.hh"
#include "karabo/xms/ImageData.hh"
#include "karabo/xms/InputChannel.hh"


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
                    .setNewOptions(State::INIT, State::NORMAL, State::STARTING, State::STARTED,
                                   State::STOPPING, State::ERROR)
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
                    .minInc(std::numeric_limits<signed char>::lowest())
                    .maxInc(std::numeric_limits<signed char>::max())
                    .reconfigurable()
                    .assignmentOptional().defaultValue(33)
                    .commit();

            INT8_ELEMENT(expected).key("int8PropertyReadOnly")
                    .displayedName("Int8 property")
                    .description("A int8 property")
                    .minInc(std::numeric_limits<signed char>::lowest())
                    .maxInc(std::numeric_limits<signed char>::max())
                    .readOnly()
                    .initialValue(33)
                    .alarmLow(std::numeric_limits<signed char>::lowest()).info("alarmLow").needsAcknowledging(true)
                    .warnLow(std::numeric_limits<signed char>::lowest()).info("warnLow").needsAcknowledging(false)
                    .warnHigh(std::numeric_limits<signed char>::max()).info("warnHigh").needsAcknowledging(false)
                    .alarmHigh(std::numeric_limits<signed char>::max()).info("alarmHigh").needsAcknowledging(true)
                    .commit();

            UINT8_ELEMENT(expected).key("uint8Property")
                    .displayedName("UInt8 property")
                    .description("A uint8 property")
                    .minInc(std::numeric_limits<unsigned char>::lowest())
                    .maxInc(std::numeric_limits<unsigned char>::max())
                    .reconfigurable()
                    .assignmentOptional().defaultValue(177)
                    .commit();

            UINT8_ELEMENT(expected).key("uint8PropertyReadOnly")
                    .displayedName("UInt8 property read-only")
                    .description("A uint8 property read-only")
                    .minInc(std::numeric_limits<unsigned char>::lowest())
                    .maxInc(std::numeric_limits<unsigned char>::max())
                    .readOnly()
                    .initialValue(177)
                    .alarmLow(std::numeric_limits<unsigned char>::lowest()).info("alarmLow").needsAcknowledging(true)
                    .warnLow(std::numeric_limits<unsigned char>::lowest()).info("warnLow").needsAcknowledging(false)
                    .warnHigh(std::numeric_limits<unsigned char>::max()).info("warnHigh").needsAcknowledging(false)
                    .alarmHigh(std::numeric_limits<unsigned char>::max()).info("alarmHigh").needsAcknowledging(true)
                    .commit();

            INT16_ELEMENT(expected).key("int16Property")
                    .displayedName("Int16 property")
                    .description("A int16 property")
                    .minInc(std::numeric_limits<short>::lowest())
                    .maxInc(std::numeric_limits<short>::max())
                    .reconfigurable()
                    .assignmentOptional().defaultValue(3200)
                    .commit();

            INT16_ELEMENT(expected).key("int16PropertyReadOnly")
                    .displayedName("Int16 property read-only")
                    .description("A int16 property read-only")
                    .minInc(std::numeric_limits<short>::lowest())
                    .maxInc(std::numeric_limits<short>::max())
                    .readOnly()
                    .initialValue(3200)
                    .alarmLow(std::numeric_limits<short>::lowest()).info("alarmLow").needsAcknowledging(true)
                    .warnLow(std::numeric_limits<short>::lowest()).info("warnLow").needsAcknowledging(false)
                    .warnHigh(std::numeric_limits<short>::max()).info("warnHigh").needsAcknowledging(false)
                    .alarmHigh(std::numeric_limits<short>::max()).info("alarmHigh").needsAcknowledging(true)
                    .commit();

            UINT16_ELEMENT(expected).key("uint16Property")
                    .displayedName("UInt16 property")
                    .description("A uint16 property")
                    .minInc(std::numeric_limits<unsigned short>::lowest())
                    .maxInc(std::numeric_limits<unsigned short>::max())
                    .reconfigurable()
                    .assignmentOptional().defaultValue(32000)
                    .commit();

            UINT16_ELEMENT(expected).key("uint16PropertyReadOnly")
                    .displayedName("UInt16 property read-only")
                    .description("A uint16 property read-only")
                    .minInc(std::numeric_limits<unsigned short>::lowest())
                    .maxInc(std::numeric_limits<unsigned short>::max())
                    .readOnly()
                    .initialValue(32000)
                    .alarmLow(std::numeric_limits<unsigned short>::lowest()).info("alarmLow").needsAcknowledging(true)
                    .warnLow(std::numeric_limits<unsigned short>::lowest()).info("warnLow").needsAcknowledging(false)
                    .warnHigh(std::numeric_limits<unsigned short>::max()).info("warnHigh").needsAcknowledging(false)
                    .alarmHigh(std::numeric_limits<unsigned short>::max()).info("alarmHigh").needsAcknowledging(true)
                    .commit();

            INT32_ELEMENT(expected).key("int32Property")
                    .displayedName("Int32 property")
                    .description("A int32 property")
                    .minInc(std::numeric_limits<int>::lowest())
                    .maxInc(std::numeric_limits<int>::max())
                    .reconfigurable()
                    .assignmentOptional().defaultValue(32000000)
                    .commit();

            INT32_ELEMENT(expected).key("int32PropertyReadOnly")
                    .displayedName("Int32 property read-only")
                    .description("A int32 property read-only")
                    .minInc(std::numeric_limits<int>::lowest())
                    .maxInc(std::numeric_limits<int>::max())
                    .readOnly()
                    .initialValue(32000000)
                    .alarmLow(-32000000).info("alarmLow").needsAcknowledging(true)
                    .warnLow(-10).info("warnLow").needsAcknowledging(false)
                    .warnHigh(std::numeric_limits<int>::max()).info("warnHigh").needsAcknowledging(false)
                    .alarmHigh(std::numeric_limits<int>::max()).info("alarmHigh").needsAcknowledging(true)
                    .commit();

            UINT32_ELEMENT(expected).key("uint32Property")
                    .displayedName("UInt32 property")
                    .description("A uint32 property")
                    .minInc(std::numeric_limits<unsigned int>::lowest())
                    .maxInc(std::numeric_limits<unsigned int>::max())
                    .reconfigurable()
                    .assignmentOptional().defaultValue(32000000)
                    .commit();

            UINT32_ELEMENT(expected).key("uint32PropertyReadOnly")
                    .displayedName("UInt32 property read-only")
                    .description("A uint32 property read-only")
                    .minInc(std::numeric_limits<unsigned int>::lowest())
                    .maxInc(std::numeric_limits<unsigned int>::max())
                    .readOnly()
                    .initialValue(32000000)
                    .alarmLow(std::numeric_limits<unsigned int>::lowest()).info("alarmLow").needsAcknowledging(true)
                    .warnLow(std::numeric_limits<unsigned int>::lowest()).info("warnLow").needsAcknowledging(false)
                    .warnHigh(std::numeric_limits<unsigned int>::max()).info("warnHigh").needsAcknowledging(false)
                    .alarmHigh(std::numeric_limits<unsigned int>::max()).info("alarmHigh").needsAcknowledging(true)
                    .commit();

            INT64_ELEMENT(expected).key("int64Property")
                    .displayedName("Int64 property")
                    .description("A int64 property")
                    .minInc(std::numeric_limits<long long>::lowest())
                    .maxInc(std::numeric_limits<long long>::max())
                    .reconfigurable()
                    .assignmentOptional().defaultValue(3200000000LL)
                    .commit();

            INT64_ELEMENT(expected).key("int64PropertyReadOnly")
                    .displayedName("Int64 property read-only")
                    .description("A int64 property read-only")
                    .minInc(std::numeric_limits<long long>::lowest())
                    .maxInc(std::numeric_limits<long long>::max())
                    .readOnly()
                    .initialValue(3200000000LL)
                    .alarmLow(-3200000000ll).info("Too low").needsAcknowledging(true)
                    .warnLow(-3200).info("warnLow").needsAcknowledging(false)
                    .warnHigh(std::numeric_limits<long long>::max()).info("warnHigh").needsAcknowledging(false)
                    .alarmHigh(std::numeric_limits<long long>::max()).info("alarmHigh").needsAcknowledging(true)
                    .commit();

            UINT64_ELEMENT(expected).key("uint64Property")
                    .displayedName("UInt64 property")
                    .description("A uint64 property")
                    .minInc(std::numeric_limits<unsigned long long>::lowest())
                    .maxInc(std::numeric_limits<unsigned long long>::max())
                    .reconfigurable()
                    .assignmentOptional().defaultValue(3200000000ULL)
                    .commit();

            UINT64_ELEMENT(expected).key("uint64PropertyReadOnly")
                    .displayedName("UInt64 property read-only")
                    .description("A UInt64 property read-only")
                    .minInc(std::numeric_limits<unsigned long long>::lowest())
                    .maxInc(std::numeric_limits<unsigned long long>::max())
                    .readOnly()
                    .initialValue(3200000000ULL)
                    .alarmLow(std::numeric_limits<unsigned long long>::lowest()).info("alarmLow").needsAcknowledging(true)
                    .warnLow(std::numeric_limits<unsigned long long>::lowest()).info("warnLow").needsAcknowledging(false)
                    .warnHigh(std::numeric_limits<unsigned long long>::max()).info("warnHigh").needsAcknowledging(false)
                    .alarmHigh(std::numeric_limits<unsigned long long>::max()).info("alarmHigh").needsAcknowledging(true)
                    .commit();

            FLOAT_ELEMENT(expected).key("floatProperty")
                    .displayedName("Float property")
                    .description("A float property")
                    .minInc(std::numeric_limits<float>::lowest())
                    .maxInc(std::numeric_limits<float>::max())
                    .reconfigurable()
                    .assignmentOptional().defaultValue(3.141596)
                    .commit();

            FLOAT_ELEMENT(expected).key("floatPropertyReadOnly")
                    .displayedName("Float property read-only")
                    .description("A Float property read-only")
                    .minInc(std::numeric_limits<float>::lowest())
                    .maxInc(std::numeric_limits<float>::max())
                    .readOnly()
                    .initialValue(3.141596)
                    .alarmLow(std::numeric_limits<float>::lowest()).info("alarmLow").needsAcknowledging(true)
                    .warnLow(std::numeric_limits<float>::lowest() / 2).info("warnLow").needsAcknowledging(false)
                    .warnHigh(std::numeric_limits<float>::max() / 2).info("warnHigh").needsAcknowledging(false)
                    .alarmHigh(std::numeric_limits<float>::max()).info("alarmHigh").needsAcknowledging(true)
                    .commit();

            DOUBLE_ELEMENT(expected).key("doubleProperty")
                    .displayedName("Double property")
                    .description("A double property")
                    .minInc(std::numeric_limits<double>::lowest()/2)
                    .maxInc(std::numeric_limits<double>::max()/2)
                    .reconfigurable()
                    .assignmentOptional().defaultValue(3.1415967773331)
                    .commit();

            DOUBLE_ELEMENT(expected).key("doublePropertyReadOnly")
                    .displayedName("Double property read-only")
                    .description("A double property read-only")
                    .minInc(std::numeric_limits<double>::lowest()/2)
                    .maxInc(std::numeric_limits<double>::max()/2)
                    .readOnly()
                    .initialValue(3.1415967773331)
                    .alarmLow(-100.).info("Too low").needsAcknowledging(false)
                    .warnLow(-10.).info("Rather low").needsAcknowledging(true)
                    .warnHigh(10).info("Rather high").needsAcknowledging(false)
                    .alarmHigh(100.).info("Too high").needsAcknowledging(true)
                    .commit();

            STRING_ELEMENT(expected).key("stringProperty")
                    .displayedName("String property")
                    .description("A string property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue("Some arbitrary text.")
                    .commit();

            SLOT_ELEMENT(expected).key("setAlarm")
                    .displayedName("Set Alarm")
                    .description("Set an acknowledgment requiring alarm to value of String property - if convertable")
                    .commit();

            SLOT_ELEMENT(expected).key("setAlarmNoNeedAck")
                    .displayedName("Set Alarm (no ackn.)")
                    .description("Set an alarm (that does not require acknowledgment) to value of String property - if convertable")
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

            TABLE_ELEMENT(expected).key("tableReadOnly")
                    .displayedName("Read-only table property")
                    .description("Read-only table containing one node.")
                    .addColumnsFromClass<NestedClass>()
                    .addColumns(columns)
                    .readOnly().initialValue({Hash("e1", "abc", "e2", true, "e3", 12, "e4", 0.9837F, "e5", 1.2345),
                                              Hash("e1", "xyz", "e2", false, "e3", 42, "e4", 2.33333F, "e5", 7.77777)})
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
                    .description("A string sent via the pipeline")
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
                    .allowedStates(State::NORMAL)
                    .commit();

            FLOAT_ELEMENT(expected).key("outputFrequency")
                    .displayedName("Output frequency")
                    .description("The target frequency for continously writing to 'Output'")
                    .unit(util::Unit::HERTZ)
                    .maxInc(1000)
                    .minExc(0.f)
                    .assignmentOptional().defaultValue(1.f)
                    .reconfigurable()
                    .commit();

            INT32_ELEMENT(expected).key("outputCounter")
                    .displayedName("Output Counter")
                    .description("Last value sent as 'int32' via output channel 'Output'")
                    .readOnly().initialValue(0)
                    .commit();

            SLOT_ELEMENT(expected).key("startWritingOutput")
                    .displayedName("Start Writing")
                    .description("Start writing continously to output channel 'Output'")
                    .allowedStates(State::NORMAL)
                    .commit();

            SLOT_ELEMENT(expected).key("stopWritingOutput")
                    .displayedName("Stop Writing")
                    .description("Stop writing continously to output channel 'Output'")
                    .allowedStates(State::STARTED)
                    .commit();

            SLOT_ELEMENT(expected).key("eosOutput")
                    .displayedName("EOS to Output")
                    .description("Write end-of-stream to output channel 'Output'")
                    .allowedStates(State::NORMAL)
                    .commit();

            INPUT_CHANNEL(expected).key("input")
                    .displayedName("Input")
                    .dataSchema(pipeData) // re-use what the output channel sends
                    .commit();

            UINT32_ELEMENT(expected).key("processingTime")
                    .displayedName("Processing Time")
                    .description("Processing time of input channel data handler")
                    .assignmentOptional()
                    .defaultValue(0u)
                    .reconfigurable()
                    .unit(util::Unit::SECOND)
                    .metricPrefix(util::MetricPrefix::MILLI)
                    .commit();

            INT32_ELEMENT(expected).key("currentInputId")
                    .displayedName("Current Input Id")
                    .description("Last value received as 'int32' on input channel (default: 0)")
                    .readOnly().initialValue(0)
                    .commit();

            UINT32_ELEMENT(expected).key("inputCounter")
                    .displayedName("Input Counter")
                    .description("Number of data items received on input channel")
                    .readOnly().initialValue(0)
                    .commit();

            SLOT_ELEMENT(expected).key("resetChannelCounters")
                    .displayedName("Reset Channels")
                    .description("Reset counters involved in input/output channel data flow")
                    .allowedStates(State::NORMAL)
                    .commit();

            SLOT_ELEMENT(expected).key("slotUpdateSchema")
                    .displayedName("Update Schema")
                    .description("Duplicate maxSize of vectors in schema")
                    .allowedStates(State::NORMAL)
                    .commit();

            PATH_ELEMENT(expected).key("inputPath")
                    .displayedName("Input File")
                    .description("An input file")
                    .isInputFile()
                    .assignmentOptional().defaultValue("./input_file")
                    .reconfigurable()
                    .commit();

            PATH_ELEMENT(expected).key("outputPath")
                    .displayedName("Output File")
                    .description("An output file")
                    .isOutputFile()
                    .assignmentOptional().defaultValue("./output_file")
                    .reconfigurable()
                    .commit();

            PATH_ELEMENT(expected).key("directoryPath")
                    .displayedName("Directory")
                    .description("A directory")
                    .isDirectory()
                    .assignmentOptional().defaultValue(".")
                    .reconfigurable()
                    .commit();

            NODE_ELEMENT(expected).key("node")
                    .displayedName("Node for Slots")
                    .commit();

            SLOT_ELEMENT(expected).key("node.increment")
                    .displayedName("Increment 'Counter read-only'")
                    .commit();

            SLOT_ELEMENT(expected).key("node.reset")
                    .displayedName("Reset Counter")
                    .commit();

            UINT32_ELEMENT(expected).key("node.counterReadOnly")
                    .displayedName("Counter read-only")
                    .readOnly()
                    .initialValue(0)
                    .warnHigh(1000000).info("Rather high").needsAcknowledging(true) // 1.e6
                    .alarmHigh(100000000).info("Too high").needsAcknowledging(false) // 1.e8 - false for test purposes
                    .commit();

            UINT32_ELEMENT(expected).key("node.counter")
                    .displayedName("Counter")
                    .description("Values will be transferred to 'Counter read-only' under same node")
                    .reconfigurable()
                    .assignmentOptional()
                    .defaultValue(0u)
                    .commit();
        }


        PropertyTest::PropertyTest(const Hash& input)
            : Device<>(input),
            m_writingOutput(false), m_writingOutputTimer(karabo::net::EventLoop::getIOService()) {

            KARABO_INITIAL_FUNCTION(initialize);
            KARABO_SLOT(setAlarm);
            KARABO_SLOT(setAlarmNoNeedAck);
            KARABO_SLOT(writeOutput);
            KARABO_SLOT(startWritingOutput);
            KARABO_SLOT(stopWritingOutput);
            KARABO_SLOT(resetChannelCounters);
            KARABO_SLOT(eosOutput);
            KARABO_SLOT(slotUpdateSchema);
            KARABO_SLOT(node_increment);
            KARABO_SLOT(node_reset);
        }


        PropertyTest::~PropertyTest() {
        }


        void PropertyTest::initialize() {
            // some initialization
            KARABO_ON_DATA("input", onData); // not yet possible in constructor, since uses bind_weak

            updateState(State::NORMAL);
        }

        void PropertyTest::preReconfigure(Hash& incomingReconfiguration) {
            const std::vector<std::string> keys = {"uint8Property", "int8Property",
                                                   "uint16Property", "int16Property", "uint32Property", "int32Property",
                                                   "uint64Property", "int64Property", "floatProperty", "doubleProperty",
                                                   "table", "node.counter"};
            Hash h;

            for (const std::string& key : keys) {
                if (incomingReconfiguration.has(key)) {
                    h.set(key + "ReadOnly", incomingReconfiguration.get<boost::any>(key));
                }
            }
            set(h);
        }


        void PropertyTest::setAlarm() {
            const karabo::util::AlarmCondition alarm = karabo::util::AlarmCondition::fromString(get<std::string>("stringProperty"));
            setAlarmCondition(alarm, true, "Acknowledgment requiring alarm");
        }


        void PropertyTest::setAlarmNoNeedAck() {
            const karabo::util::AlarmCondition alarm = karabo::util::AlarmCondition::fromString(get<std::string>("stringProperty"));
            setAlarmCondition(alarm, false, "No acknowledgment requiring alarm");
        }


        void PropertyTest::writeOutput() {

            const int outputCounter = get<int>("outputCounter") + 1;

            // Set all numbers inside to m_outputCounter:
            Hash data;
            Hash& node = data.bindReference<Hash>("node");
            node.set("int32", outputCounter);
            node.set("string", toString(outputCounter));
            node.set("vecInt64", std::vector<long long>(defVectorMaxSize, static_cast<long long> (outputCounter)));
            node.set("ndarray", NDArray(Dims(100ull, 200ull), static_cast<float> (outputCounter)));
            node.set("image", ImageData(NDArray(Dims(400ull, 500ull), static_cast<unsigned short> (outputCounter)),
                                        Dims(), // use Dims of NDArray
                                        Encoding::GRAY, // gray scale as is default
                                        16)); // unsigned short is 16 bits

            writeChannel("output", data);
            set("outputCounter", outputCounter);
        }


        void PropertyTest::writeOutputHandler(const boost::system::error_code& e) {
            if (e) {
                this->updateState(State::NORMAL);
                return; // most likely cancelled
            }

            writeOutput();

            if (m_writingOutput) {
                // get when fired the last time and add delay time
                auto next = m_writingOutputTimer.expires_at(); // at first time set in startWritingOutput())
                const int delayTime = 1000.f / this->get<float>("outputFrequency"); // minExc(0.f) guarantees non-zero value
                next += boost::posix_time::milliseconds(delayTime);
                // now fire again
                m_writingOutputTimer.expires_at(next);
                m_writingOutputTimer.async_wait(karabo::util::bind_weak(&PropertyTest::writeOutputHandler,
                                                                        this, boost::asio::placeholders::error));
            } else {
                this->updateState(State::NORMAL);
            }
        }


        void PropertyTest::startWritingOutput() {
            m_writingOutput = true;
            this->updateState(State::STARTED);

            // Start right away
            m_writingOutputTimer.expires_from_now(boost::posix_time::milliseconds(0)); // see writeOutputHandler
            karabo::net::EventLoop::getIOService().post(karabo::util::bind_weak(&PropertyTest::writeOutputHandler,
                                                                                this, boost::system::error_code()));
        }


        void PropertyTest::stopWritingOutput() {

            m_writingOutput = false;
            this->updateState(State::STOPPING);
            m_writingOutputTimer.cancel();
        }


        void PropertyTest::onData(const karabo::util::Hash& data, const karabo::xms::InputChannel::MetaData& meta) {

            // First sleep to simulate heavy work
            boost::this_thread::sleep(boost::posix_time::milliseconds(get<unsigned int>("processingTime")));

            const int currentInputId = data.get<int>("node.int32");
            const unsigned int inputCounter = get<unsigned int>("inputCounter");

            set(Hash("inputCounter", inputCounter + 1,
                     "currentInputId", currentInputId));

            // Writes data received to output channel to allow PropertyTest to build
            // pipelines of chained devices.
            writeOutput();
        }


        void PropertyTest::resetChannelCounters() {
            set(Hash("inputCounter", 0u,
                     "currentInputId", 0,
                     "outputCounter", 0));

        }

        void PropertyTest::eosOutput() {
            signalEndOfStream("output");
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


        void PropertyTest::node_increment() {
            // Use an AsyncReply to test and demonstrate its purpose.
            AsyncReply areply(this);
            const unsigned int counter = get<unsigned int>("node.counter");
            set("node.counter", counter + 1);
            karabo::net::EventLoop::getIOService().post(
                    karabo::util::bind_weak(&PropertyTest::replier, this, areply));
        }


        void PropertyTest::replier(const AsyncReply & areply) {
            areply(this->getState().name());
        }


        void PropertyTest::node_reset() {
            AsyncReply areply(this);
            set<unsigned int>("node.counter", 0);
            karabo::net::EventLoop::getIOService().post(
                    karabo::util::bind_weak(&PropertyTest::replier, this, areply));
        }

    }
}
