/* 
 * File:   CentralLogging.cc
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on November 9, 2015, 12:28 PM
 */

#include "karabo/core.hpp"
#include "karabo/util.hpp"

#include "CentralLogging.hh"

namespace karabo {
    namespace devices {


        using namespace std;
        using namespace karabo::core;
        using namespace karabo::util;
        using namespace karabo::net;


        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, CentralLogging)


        void CentralLogging::expectedParameters(Schema& expected) {

            OVERWRITE_ELEMENT(expected).key("deviceId")
                    .setNewDefaultValue("clog_0")
                    .commit();

            PATH_ELEMENT(expected).key("directory")
                    .displayedName("Directory")
                    .description("The directory where the log files should be placed")
                    .assignmentOptional().defaultValue("logs")
                    .commit();

            INT32_ELEMENT(expected).key("maximumFileSize")
                    .displayedName("Maximum file size")
                    .description("After any log file has reached this size it will be time-stamped and not appended anymore")
                    .unit(Unit::BYTE)
                    .metricPrefix(MetricPrefix::MEGA)
                    .assignmentOptional().defaultValue(5)
                    .commit();

            INT32_ELEMENT(expected).key("flushInterval")
                    .displayedName("Flush interval")
                    .description("The interval after which the memory accumulated data is made persistent")
                    .unit(Unit::SECOND)
                    .assignmentOptional().defaultValue(10)
                    .reconfigurable()
                    .commit();

            INT64_ELEMENT(expected).key("counter")
                    .displayedName("Message counter")
                    .description("The number of messages logged in current session")
                    .readOnly().initialValue(0)
                    .commit();
        }


        CentralLogging::CentralLogging(const karabo::util::Hash& input) : Device<>(input), m_svc(new boost::asio::io_service()), m_timer(*m_svc) {

            KARABO_INITIAL_FUNCTION(initialize)
            m_loggerInput = input;
        }


        CentralLogging::~CentralLogging() {
            m_svc->stop();
            if (m_svcThread.get_id() != boost::this_thread::get_id())
                m_svcThread.join();
        }


        void CentralLogging::initialize() {

            m_loggerConsumer = getConnection()->createConsumer();
            m_loggerConsumer->readAsync(bind_weak(&karabo::devices::CentralLogging::logHandler, this, _1, _2),
                                        m_topic, "target = 'log'");

            try {
                if (!boost::filesystem::exists(get<string>("directory"))) {
                    boost::filesystem::create_directory(get<string>("directory"));
                }

                m_lastIndex = determineLastIndex();

                m_timer.expires_from_now(boost::posix_time::seconds(get<int>("flushInterval")));
                m_timer.async_wait(boost::bind(&CentralLogging::flushHandler, this, boost::asio::placeholders::error));
                m_svcThread = boost::thread(boost::bind(&karabo::net::runProtected, m_svc, this->getInstanceId(),
                                                        "for flushing to file", 100));
                // Produce some information
                KARABO_LOG_INFO << "Central Logging service started listening all log messages ...";

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in initialize(): " << e.userFriendlyMsg();
            }


        }


        void CentralLogging::flushHandler(const boost::system::error_code& ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "flushHandler called ...";
            if (!ec) {
                try {
                    boost::mutex::scoped_lock lock(m_streamMutex);
                    if (m_logstream.is_open()) {
                        m_logstream.flush();
                    }
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Problem in flushHandler(): " << e.what();
                }
            }
            m_timer.expires_from_now(boost::posix_time::seconds(get<int>("flushInterval")));
            m_timer.async_wait(boost::bind(&CentralLogging::flushHandler, this, boost::asio::placeholders::error));
        }


        void CentralLogging::logHandler(const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& data) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "logHandler called ...";
                boost::mutex::scoped_lock lock(m_streamMutex);

                if (!m_logstream.is_open()) {
                    string logname = get<string>("directory") + "/log_" + toString(m_lastIndex) + ".txt";
                    m_logstream.open(logname.c_str(), ios::out | ios::app);
                    if (!m_logstream.is_open()) {
                        KARABO_LOG_FRAMEWORK_ERROR << "Failed to open \"" << logname << "\". Check permissions.";
                        return;
                    }
                    if (m_logstream.tellp() > 0) m_logstream << "\n";
                }

                if (data->has("messages")) {
                    const vector<Hash>& vechash = data->get<std::vector<util::Hash> >("messages");
                    KARABO_LOG_FRAMEWORK_DEBUG << "Log " << vechash.size();
                    for (vector<Hash>::const_iterator it = vechash.begin(); it != vechash.end(); ++it) {
                        m_logstream << it->get<string>("timestamp") << "\t" << it->get<string>("type") << "\t"
                                << it->get<string>("category") << "\t" << it->get<string>("message") << "\n";
                    }
                    set("counter", get<long long>("counter") + vechash.size());
                }

                if (m_logstream.tellp() >= get<int>("maximumFileSize")*1000000) {
                    m_logstream.close();
                    m_lastIndex = incrementLastIndex();
                }

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in logHandler(): " << e.userFriendlyMsg();
            }
            // Re-register the log message reading
            m_loggerConsumer->readAsync(bind_weak(&karabo::devices::CentralLogging::logHandler, this, _1, _2),
                                        m_topic, "target = 'log'");

        }


        int CentralLogging::determineLastIndex() {
            string lastIndexFilename = get<string>("directory") + "/LastIndex.txt";
            int idx;
            fstream fs;
            if (!boost::filesystem::exists(lastIndexFilename)) {
                for (size_t i = 0;; i++) {
                    string filename = get<string>("directory") + "/log_" + toString(i) + ".txt";
                    if (!boost::filesystem::exists(filename)) {
                        idx = i;
                        break;
                    }
                }
                fs.open(lastIndexFilename.c_str(), ios::out | ios::app);
                fs << idx << "\n";
            } else {
                fs.open(lastIndexFilename.c_str(), ios::in);
                fs >> idx;
            }
            fs.close();
            return idx;
        }


        int CentralLogging::incrementLastIndex() {
            string lastIndexFilename = get<string>("directory") + "/LastIndex.txt";
            int idx;
            if (!boost::filesystem::exists(lastIndexFilename)) {
                idx = determineLastIndex();
            }
            fstream file(lastIndexFilename.c_str(), ios::in | ios::out);
            file >> idx;
            if (file.fail()) file.clear();
            ++idx;
            file.seekg(0);
            file << idx << "\n";
            file.close();
            return idx;
        }
    }
}

