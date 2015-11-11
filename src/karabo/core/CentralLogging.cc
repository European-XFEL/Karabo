/* 
 * File:   CentralLogging.cc
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on November 9, 2015, 12:28 PM
 */

#include <karabo/karabo.hpp>
#include "CentralLogging.hh"
 
namespace karabo {
    namespace core {
 
        USING_KARABO_NAMESPACES
                using namespace std;
 
 
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
 
 
        CentralLogging::CentralLogging(const karabo::util::Hash& input) : Device<>(input), m_svc(), m_timer(m_svc) {
 
            registerInitialFunction(boost::bind(&karabo::core::CentralLogging::initialize, this));
 
            // Inherit from device connection settings
            Hash loggerInput = input;
            string hostname = getConnection()->getBrokerHostname();
            unsigned int port = getConnection()->getBrokerPort();
            const vector<string>& brokers = getConnection()->getBrokerHosts();
            string host = hostname + ":" + toString(port);
 
            loggerInput.set("loggerConnection.Jms.hostname", host);
            loggerInput.set("loggerConnection.Jms.port", port);
            loggerInput.set("loggerConnection.Jms.brokerHosts", brokers);
 
            m_loggerConnection = BrokerConnection::createChoice("loggerConnection", loggerInput);
            m_loggerIoService = m_loggerConnection->getIOService();
 
        }
 
 
        CentralLogging::~CentralLogging() {
            m_loggerIoService->stop();
            m_svc.stop();
 
            if (m_logThread.get_id() != boost::this_thread::get_id())
                m_logThread.join();
 
            if (m_svcThread.get_id() != boost::this_thread::get_id())
                m_svcThread.join();
        }
 
 
        void CentralLogging::initialize() {
            try {
                if (!boost::filesystem::exists(get<string>("directory"))) {
                    boost::filesystem::create_directory(get<string>("directory"));
                }
 
                m_lastIndex = determineLastIndex();
 
                // Start the logging thread
                m_loggerConnection->start();
                m_loggerChannel = m_loggerConnection->createChannel();
                m_loggerChannel->setFilter("target = 'log'");
                m_loggerChannel->readAsyncHashHash(boost::bind(&karabo::core::CentralLogging::logHandler, this, _1, _2, _3));
                m_logThread = boost::thread(boost::bind(&karabo::net::BrokerIOService::work, m_loggerIoService));
 
                m_timer.expires_from_now(boost::posix_time::seconds(get<int>("flushInterval")));
                m_timer.async_wait(boost::bind(&CentralLogging::flushHandler, this, boost::asio::placeholders::error));
                m_svcThread = boost::thread(boost::bind(&boost::asio::io_service::run, &m_svc));
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


        void CentralLogging::logHandler(karabo::net::BrokerChannel::Pointer channel,
                                        const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& data) {
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
