/*
 * $Id$
 *
 * Author: <gero.flucke@xfel.eu>
 *
 * Created on November 6, 2015
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/algorithm/string/split.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <cstdlib>
#include <iomanip>
#include <iosfwd>
#include <iostream>
#include <karabo/core/DeviceClient.hh>
#include <karabo/log/Logger.hh>
#include <karabo/net/AmqpClient.hh>
#include <karabo/net/Broker.hh>
#include <karabo/net/EventLoop.hh>
#include <karabo/net/JmsConnection.hh>
#include <karabo/net/JmsConsumer.hh>
#include <karabo/net/MqttClient.hh>
#include <karabo/net/RedisClient.hh>
#include <karabo/util/Epochstamp.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/MetaTools.hh>
#include <karabo/util/Schema.hh>
#include <map>
#include <unordered_set>

using namespace karabo;
using namespace boost::placeholders;

class BrokerStatistics : public boost::enable_shared_from_this<BrokerStatistics> {
   public:
    /// Stats as pair of number of calls and accumulated size in bytes.
    typedef std::pair<unsigned int, size_t> Stats;

    /// SignalId as pair of instanceId and signalId of sender.
    typedef std::pair<const std::string, const std::string> SignalId;

    typedef std::map<SignalId, Stats> SignalStatsMap;

    /// SlotId is single string: a ':' separates slotInstanceId and slotFunction.
    typedef std::string SlotId;

    typedef std::map<SlotId, Stats> SlotStatsMap;

    /// Constructor with length of interval to use for averaging.


    BrokerStatistics(util::TimeValue intervalSec, const std::vector<std::string>& receiverIds,
                     const std::vector<std::string>& senderIds)
        : m_interval(intervalSec, 0ll),
          m_receivers(receiverIds.begin(), receiverIds.end()),
          m_senders(senderIds.begin(), senderIds.end()),
          m_start(0ull, 0ull) {}

    // virtual ~BrokerStatistics() {} // no need for virtual...

    /// Register a message, i.e. increase statistics and possibly print.
    void registerMessage(const util::Hash::Pointer& header, const util::Hash::Pointer& body);

   private:
    void registerPerSignal(const util::Hash::Pointer& header, size_t bodySize);

    void registerPerSlot(const util::Hash::Pointer& header, size_t bodySize);

    /**
     * Helper checking whether slot is currently among receivers - empty receivers means everything is received
     *
     * @param slot slot id from message header
     * @param receivers set of recever instance ids
     * @return whether received
     */
    bool slotReceivedBy(const SlotId& slot, const std::unordered_set<std::string>& receivers) const;

    void registerLogMessage(size_t bodySize);

    void printStatistics(const util::Epochstamp& timeStamp, float elapsedSeconds) const;

    /// The keys of StatsMap must either support
    /// ostream& operator<<(ostream&, const KeyType&),
    /// preferably with a width of 59 characters or there must be a specialisation
    /// of BrokerStatistics::printId for IdType=KeyType.
    template <typename StatsMap>
    Stats printStatistics(const StatsMap& statsMap, const util::Epochstamp& timeStamp, float elapsedSeconds) const;

    /// Helper of printStatistics.
    template <class IdType>
    void printLine(const IdType& id, const Stats& stats, float elapsedSeconds) const;

    /// Helper of printLine (see also printStatistics).
    template <class IdType>
    void printId(std::ostream& out, const IdType& id) const;

    const util::TimeDuration m_interval;
    const std::unordered_set<std::string> m_receivers;
    const std::unordered_set<std::string> m_senders;
    util::Epochstamp m_start;

    /// mapping SignalId to Stats
    SignalStatsMap m_signalStats;

    /// mapping SlotId to Stats
    SlotStatsMap m_slotStats;

    static const std::string m_delimLine;
};

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

static bool debug = false;
const std::string BrokerStatistics::m_delimLine(
      "===============================================================================\n");

////////////////////////////////////////////////////////////////////////////


template <class IdType>
void BrokerStatistics::printId(std::ostream& out, const IdType& id) const {
    out << std::left << std::setw(59) << id;
}


template <>
void BrokerStatistics::printId(std::ostream& out, const SignalId& id) const {
    out << std::left << std::setw(39) << id.first << std::setw(20) << id.second;
}


template <>
void BrokerStatistics::printId(std::ostream& out, const SlotId& id) const {
    // SlotId is a string with a colon
    std::vector<std::string> deviceSlot;
    boost::split(deviceSlot, id, boost::is_any_of(":"));

    const std::string& slotName = (deviceSlot.size() >= 2 ? deviceSlot[1] : "");
    out << std::left << std::setw(39) << deviceSlot[0] << std::setw(20) << slotName;

    // Create a warning if deviceSlot.size() > 2, at least once?
}

////////////////////////////////////////////////////////////////////////////


void BrokerStatistics::registerMessage(const util::Hash::Pointer& header, const util::Hash::Pointer& body) {
    try {
        // In the very first call we reset the start time.
        // Otherwise (if the constructor initialises m_start with 'now') starting this
        // tool and then starting the first device in the topic leads to wrongly low
        // rates.
        if (!m_start.getSeconds()) m_start.now();

        // Since we told the consumer to skip serialisation, there is just the raw data:
        const size_t bodySize = body->get<std::vector<char>>("raw").size();
        boost::optional<util::Hash::Node&> targetNode = header->find("target");
        if (targetNode && targetNode->is<std::string>() && targetNode->getValue<std::string>() == "log") {
            this->registerLogMessage(bodySize);
        } else {
            // Register for per sender and per (intended) receiver:
            this->registerPerSignal(header, bodySize);
            this->registerPerSlot(header, bodySize);
        }

        // Now it might be time to print and reset.
        // Since it is done inside registerMessage, one does not get any printout if
        // the watched topic is silent :-(. But then we do not need monitoring anyway.
        const util::Epochstamp now;
        const util::TimeDuration diff = now.elapsed(m_start);
        if (diff >= m_interval) {
            // Calculating in single float precision should be enough...
            const float elapsedSeconds =
                  static_cast<float>(diff.getTotalSeconds()) + diff.getFractions(util::MICROSEC) / 1.e6f;
            this->printStatistics(now, elapsedSeconds);

            // Reset:
            m_start = now;
            m_signalStats.clear();
            m_slotStats.clear();
        }
    } catch (const std::exception& e) {
        std::cerr << "Problem registering message: " << e.what() << "\nheader:\n" << *header << std::endl;
    }
}

////////////////////////////////////////////////////////////////////////////


void BrokerStatistics::registerPerSignal(const util::Hash::Pointer& header, size_t bodySize) {
    // Get who sent the message:
    const std::string& signalId = header->get<std::string>("signalInstanceId");
    // If special senders requested (i.e. m_senders non-empty), go on only for those
    if (!m_senders.empty() && m_senders.find(signalId) == m_senders.end()) {
        return;
    }
    const std::string& signalFunc = header->get<std::string>("signalFunction");

    // Find signal ID in map and increase statistics.
    const SignalId key(signalId, signalFunc);
    Stats& stats = m_signalStats[key];
    ++stats.first;
    stats.second += bodySize;
}

////////////////////////////////////////////////////////////////////////////


void BrokerStatistics::registerPerSlot(const util::Hash::Pointer& header, size_t bodySize) {
    // Get who receives the message, e.g.:
    // "slotFunctions": |DataLogger-Cam7_Proc:slotChanged||Karabo_GuiServer_0:_slotChanged|
    // Asynchronous replies do not have that key, so we use instead:
    // "slotInstanceIds": |DataLogger-Cam7_Proc||Karabo_GuiServer_0|
    boost::optional<util::Hash::Node&> funcNode = header->find("slotFunctions");
    const std::string& slots =
          (funcNode ? funcNode->getValue<std::string>() : header->get<std::string>("slotInstanceIds"));

    std::vector<std::string> slotsVec;
    // token_compress_on: treat "||" as "|"
    boost::split(slotsVec, slots, boost::is_any_of("|"), boost::token_compress_on);

    for (std::vector<std::string>::const_iterator iSlot = slotsVec.begin(), iEnd = slotsVec.end(); iSlot != iEnd;
         ++iSlot) {
        if (iSlot->empty()) continue; // before first or after last '|'

        // Find slot ID in map and increase statistics.
        const SlotId& slot = *iSlot;
        if (this->slotReceivedBy(slot, m_receivers)) {
            Stats& stats = m_slotStats[slot];
            ++stats.first;
            stats.second += bodySize;
        }
    }
}


bool BrokerStatistics::slotReceivedBy(const SlotId& slot, const std::unordered_set<std::string>& receivers) const {
    if (receivers.empty()) {
        return true;
    }

    const std::string slotInstanceId(slot.substr(0, slot.find(':'))); // i.e. copy of slot if no ':'
    return (receivers.find(slotInstanceId) != receivers.end());
}


////////////////////////////////////////////////////////////////////////////


void BrokerStatistics::registerLogMessage(size_t bodySize) {
    // We have no clue who sends or receives log messages.
    // Treat them as sent and received once:
    const SignalId signalKey("?", "log");
    Stats& senderStats = m_signalStats[signalKey];
    ++senderStats.first;
    senderStats.second += bodySize;

    const SlotId senderKey("?:log");
    Stats& receiverStats = m_slotStats[senderKey];
    ++receiverStats.first;
    receiverStats.second += bodySize;
}

////////////////////////////////////////////////////////////////////////////


void BrokerStatistics::printStatistics(const util::Epochstamp& timeStamp, float elapsedSeconds) const {
    std::string when;
    try {
        when = timeStamp.toFormattedString() += " (UTC)";
    } catch (const std::runtime_error& e) {
        // With a mis-configured language system (e.g. 'export LANG=some_garbage'), we get std::runtime_error
        // with message "locale::facet::_S_create_c_locale name not valid".
        when = timeStamp.toIso8601Ext(); // This works - but human readability is reduced...
    }
    // Print kind of header
    std::cout << "\n"
              << m_delimLine << m_delimLine << std::setprecision(2) << std::fixed // 2 digits, keep 0s
              << when << " - average over " << elapsedSeconds << " s:\n";


    std::cout << "Senders:\n" << m_delimLine.substr(0, 8) << "\n";
    Stats total = this->printStatistics(m_signalStats, timeStamp, elapsedSeconds);
    this->printLine(SignalId("Total sent", ""), total, elapsedSeconds);
    std::cout << m_delimLine;

    std::cout << "Receivers:\n" << m_delimLine.substr(0, 10) << "\n";
    total = this->printStatistics(m_slotStats, timeStamp, elapsedSeconds);
    this->printLine(SlotId("Total to be received"), total, elapsedSeconds);
    std::cout /*<< m_delimLine*/ << std::flush;
}

////////////////////////////////////////////////////////////////////////////


template <typename StatsMap>
BrokerStatistics::Stats BrokerStatistics::printStatistics(const StatsMap& statsMap, const util::Epochstamp& timeStamp,
                                                          float elapsedSeconds) const {
    // Iterators for looping through map of stats:
    typename StatsMap::const_iterator iter = statsMap.begin();
    const typename StatsMap::const_iterator end = statsMap.end();

    // Keep track of the one with highest rate:
    typename StatsMap::const_iterator iterQuickest = iter;

    // Sum messages and kBytes:
    unsigned int numTotal = 0;
    size_t kBytesTotal = 0;

    // Now loop and print for each signal:
    for (; iter != end; ++iter) {
        const Stats& stats = iter->second; // i.e. count & sum_of_bytes
        if (stats.first) {                 // i.e. if some counts
            this->printLine(iter->first, stats, elapsedSeconds);
        }
        numTotal += stats.first;
        kBytesTotal += stats.second;
        // Check whether iter is the 'Plappermaul' so far:
        if (stats.first > iterQuickest->second.first) {
            iterQuickest = iter;
        }
    }

    // Finally print the one with the highest rate again...
    if (iterQuickest != end) { // even if 0 Hz... (else '&& maxCount > 0')
        std::cout << "\nHighest rate was:\n";
        this->printLine(iterQuickest->first, iterQuickest->second, elapsedSeconds);
    }

    return Stats(numTotal, kBytesTotal);
}

////////////////////////////////////////////////////////////////////////////


template <class IdType>
void BrokerStatistics::printLine(const IdType& id, const Stats& stats, float elapsedSeconds) const {
    const float kBytes = stats.first ? stats.second / (1.e3f * stats.first) : 0.f;

    this->printId(std::cout, id);
    std::cout << ":" << std::right << std::setw(6) << stats.first / elapsedSeconds << " Hz," << std::right
              << std::setw(6) << kBytes << " kB\n";
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


std::pair<std::vector<std::string>, std::vector<std::string>> instancesOfServers(const std::string& serverIn,
                                                                                 const std::string& serverOut,
                                                                                 unsigned int sleepSeconds,
                                                                                 bool debug) {
    std::pair<std::vector<std::string>, std::vector<std::string>> receiversSenders;

    if (!serverIn.empty() || !serverOut.empty()) {
        if (sleepSeconds > 10) { // If waiting is long, give a hint when it started
            std::cout << "\n" << karabo::util::Timestamp().toFormattedString() << " (UTC):";
        }
        std::cout << "\nGathering topology to identify devices of servers. . " << std::flush;
        // Instead of the gymnastics below, we could add a slot to the server to query it for all their devices...

        // Need an event loop for
        boost::thread thread(boost::bind(&net::EventLoop::work));
        std::cout << ". " << std::flush;

        auto client =
              boost::make_shared<core::DeviceClient>("", false); // default unique id, explicitly call initialize()
        client->initialize();
        std::cout << ". " << std::flush;  // output some progress markers...
        client->enableInstanceTracking(); // blocking a while to gather topology
        std::cout << ". " << std::flush;

        // If servers busy, discovery might take longer than the above blocking
        while (sleepSeconds-- > 0u) { // postfix decrement!
            boost::this_thread::sleep(boost::posix_time::seconds(1));
            std::cout << ". " << std::flush;
        }
        std::cout << "\n";
        if (!serverIn.empty()) {
            std::vector<std::string>& instances = receiversSenders.first;
            instances.push_back(serverIn);
            const std::vector<std::string> devices(client->getDevices(serverIn));
            std::cout << "\nFound " << devices.size() << " devices of receiving server " << serverIn;
            if (debug) {
                std::cout << ": " << util::toString(devices);
            } else {
                std::cout << ".";
            }
            std::cout << std::endl;
            instances.insert(instances.end(), devices.begin(), devices.end());
        }
        if (!serverOut.empty()) {
            std::vector<std::string>& instances = receiversSenders.second;
            instances.push_back(serverOut);
            const std::vector<std::string> devices(client->getDevices(serverOut));
            std::cout << "\nFound " << devices.size() << " devices of sending server " << serverOut;
            if (debug) {
                std::cout << ": " << util::toString(devices) << std::endl;
            } else {
                std::cout << ".";
            }
            instances.insert(instances.end(), devices.begin(), devices.end());
        }
        net::EventLoop::stop();
        thread.join();
    }
    return receiversSenders;
}

void printHelp(const char* name) {
    // Get name without leading directories
    std::string nameStr(name ? name : "'command'");
    const std::string::size_type lastSlashPos = nameStr.find_last_of('/');
    if (lastSlashPos != std::string::npos) {
        nameStr.replace(0, lastSlashPos + 1, "");
    }
    std::cout << "\n  " << nameStr << " [-h|--help] [other options with values] [interval]\n\n"
              << "Prints the rate and average size of all signals sent to the "
              << "broker and of\n"
              << "the intended calls of the slots that receive the signals.\n"
              << "Broker host and topic are read from the usual environment "
              << "variables\nKARABO_BROKER and KARABO_BROKER_TOPIC or, if "
              << "these are not defined, use the\n"
              << "usual defaults. Optional 'interval' argument specifies the time in seconds\n"
              << "for averaging (default: 5).\n"
              << "Available options:\n"
              << "   --receivers a[,b[,c[,...]]]  Consider only messages FOR given ids\n"
              << "   --senders a[,b[,c[,...]]]    Consider only messages FROM given ids\n"
              << "   --receiversServer serverId   Consider only messages FOR given serverId,\n"
              << "                                   including its devices\n"
              << "   --sendersServer serverId     Consider only messages FROM given serverId,\n"
              << "                                   including its devices\n"
              << "   --discoveryWait seconds      Extra seconds for topology discovery\n"
              << "   --debug y|n                  If yes, adds some debug output\n\n"
              << "The options '--receiversServer' and '--sendersServer' require to discover the\n"
              << "topology of the Karabo installation. If a server of interest is slowly\n"
              << "responding, the normal discovery time might be too short to identify all its\n"
              << "devices and some extra delay should be added using '--discoveryWait'.\n"
              << std::endl;
}

std::string assembleJmsSelector(const std::vector<std::string>& receivers, std::vector<std::string>& senders) {
    std::ostringstream str;
    bool first = true;
    for (const auto& receiverId : receivers) {
        if (first) first = false;
        else str << " OR ";
        str << "slotInstanceIds LIKE '%|" << receiverId << "|%'";
    }
    for (const auto& senderId : senders) {
        if (first) first = false;
        else str << " OR ";
        str << "signalInstanceId = '" << senderId << "'";
    }
    if (!first) {
        // Take care that broadcasts are also listed if senders/receivers are specified
        str << " OR slotInstanceIds LIKE '%|*|%'"; // if logs are of interest: add " OR  target = 'log'"
    }
    return str.str();
}

void startJmsMonitor(const std::vector<std::string>& brokers, const std::string& topic,
                     const std::vector<std::string>& receivers, std::vector<std::string>& senders,
                     const util::TimeValue& interval) {
    // Create connection object
    net::JmsConnection::Pointer connection = boost::make_shared<net::JmsConnection>(brokers);
    connection->connect();

    // Assemble selector
    const std::string selector = assembleJmsSelector(receivers, senders);

    if (debug) std::cout << "\nSelector:\n" << selector << std::endl;

    // 3rd argument true: skip serialisation (but get access to raw message size)!
    net::JmsConsumer::Pointer consumer = connection->createConsumer(topic, selector, true);

    std::cout << "\nStart monitoring signal and slot rates of \n   topic         '" << topic << "'\n   on broker     '"
              << connection->getBrokerUrl() << "',\n   ";
    if (!receivers.empty()) {
        std::cout << "messages to   '" << util::toString(receivers) << "',\n   ";
    }
    if (!senders.empty()) {
        std::cout << "messages from '" << util::toString(senders) << "',\n   ";
    }
    std::cout << "interval is   " << interval << " s." << std::endl;

    // Register our registration message as async reader:
    boost::shared_ptr<BrokerStatistics> stats(boost::make_shared<BrokerStatistics>(interval, receivers, senders));
    consumer->startReading(boost::bind(&BrokerStatistics::registerMessage, stats, _1, _2));

    if (debug) std::cout << "\n------------------ wait on work" << std::endl;
    // Block forever
    net::EventLoop::work();
}

void startMqttMonitor(const std::vector<std::string>& brokers, const std::string& domain,
                      const std::vector<std::string>& receivers, std::vector<std::string>& senders,
                      const util::TimeValue& interval) {
    // Create client object
    const std::string clientName = util::Configurator<net::MqttClient>::getRegisteredClasses().at(0);
    // Skip message body deserialization: "skipFlag"
    const util::Hash config("brokers", brokers, "domain", domain, "skipFlag", true);
    net::MqttClient::Pointer client = net::MqttClient::create(clientName, config);

    // Connect and subscribe
    auto ec = client->connect();
    if (ec) {
        throw KARABO_NETWORK_EXCEPTION("Failed to connect to MQTT broker at " + util::toString(brokers));
    }

    boost::shared_ptr<BrokerStatistics> stats(boost::make_shared<BrokerStatistics>(interval, receivers, senders));
    // `BrokerStatistics' expects the message formatted as a Hash: with 'header' as Hash and 'raw' as vector of char
    // which serialized 'body' value.  If the incoming message does not contain 'raw' key, serialize the body part.
    auto binSerializer = io::BinarySerializer<util::Hash>::create("Bin");
    util::Hash::Pointer body(boost::make_shared<util::Hash>());

    auto readHandler = [stats, binSerializer, body](const boost::system::error_code ec, const std::string& topic,
                                                    const util::Hash::Pointer& message) {
        if (!ec) {
            std::vector<char>& raw = body->bindReference<std::vector<char>>("raw");
            if (message->has("raw")) {
                raw.swap(message->get<std::vector<char>>("raw"));
            } else {
                binSerializer->save(message->get<util::Hash>("body"), raw); // body -> raw
            }
            stats->registerMessage(boost::make_shared<util::Hash>(message->get<util::Hash>("header")), body);
        }
    };

    net::TopicSubOptions subscriptions;
    if (receivers.empty() && senders.empty()) {
        // No input on commandline ...
        subscriptions.emplace_back(domain + "/slots/+", net::SubQos::AtMostOnce, readHandler);
        subscriptions.emplace_back(domain + "/signals/+/+", net::SubQos::AtMostOnce, readHandler);
    } else {
        for (const auto& rid : receivers) {
            std::string topic = domain + "/slots/" + boost::replace_all_copy(rid, "/", "|");
            subscriptions.emplace_back(topic, net::SubQos::AtMostOnce, readHandler);
        }
        for (const auto& sid : senders) {
            std::string topic = domain + "/signals/" + boost::replace_all_copy(sid, "/", "|") + "/+";
            subscriptions.emplace_back(topic, net::SubQos::AtMostOnce, readHandler);
        }
    }

    std::cout << "\nStart monitoring signal and slot rates of \n   domain         '" << domain
              << "'\n   on broker     '" << client->getBrokerUrl() << "',\n   ";
    if (!receivers.empty()) {
        std::cout << "messages to   '" << util::toString(receivers) << "',\n   ";
    }
    if (!senders.empty()) {
        std::cout << "messages from '" << util::toString(senders) << "',\n   ";
    }
    std::cout << "interval is   " << interval << " s." << std::endl;

    // subscribing means 'start reading'...
    ec = client->subscribe(subscriptions);
    if (ec) {
        throw KARABO_NETWORK_EXCEPTION("Failed to subscribe to MQTT broker");
    }

    if (debug) std::cout << "\n------------------ wait on work" << std::endl;
    // Block forever
    net::EventLoop::work();
}

void startRedisMonitor(const std::vector<std::string>& brokers, const std::string& domain,
                       const std::vector<std::string>& receivers, std::vector<std::string>& senders,
                       const util::TimeValue& interval) {
    // Skip message body deserialization: "skipFlag"
    const util::Hash config("brokers", brokers, "domain", domain, "skipFlag", true);
    net::RedisClient::Pointer client = util::Configurator<net::RedisClient>::create("RedisClient", config);

    // Connect and subscribe
    auto ec = client->connect();
    if (ec) {
        throw KARABO_NETWORK_EXCEPTION("Failed to connect to Redis broker at " + util::toString(brokers));
    }

    boost::shared_ptr<BrokerStatistics> stats(boost::make_shared<BrokerStatistics>(interval, receivers, senders));
    // `BrokerStatistics' expects the message formatted as a Hash: with 'header' as Hash and 'raw' as vector of char
    // which serialized 'body' value.  If the incoming message does not contain 'raw' key, serialize the body part.
    auto binSerializer = io::BinarySerializer<util::Hash>::create("Bin");
    util::Hash::Pointer body(boost::make_shared<util::Hash>());

    auto readHandler = [stats, binSerializer, body](const boost::system::error_code ec, const std::string& topic,
                                                    const util::Hash::Pointer& message) {
        if (!ec) {
            std::vector<char>& raw = body->bindReference<std::vector<char>>("raw");
            if (message->has("raw")) {
                raw.swap(message->get<std::vector<char>>("raw"));
            } else {
                binSerializer->save(message->get<util::Hash>("body"), raw); // body -> raw
            }
            stats->registerMessage(boost::make_shared<util::Hash>(message->get<util::Hash>("header")), body);
        }
    };

    net::RedisTopicSubOptions subscriptions;
    if (receivers.empty() && senders.empty()) {
        // No input on commandline
        subscriptions.emplace_back(domain + "/slots/*", readHandler);
        subscriptions.emplace_back(domain + "/signals/*/*", readHandler);
    } else {
        for (const auto& rid : receivers) {
            std::string topic = domain + "/slots/" + boost::replace_all_copy(rid, "/", "|");
            subscriptions.emplace_back(topic, readHandler);
        }
        for (const auto& sid : senders) {
            std::string topic = domain + "/signals/" + boost::replace_all_copy(sid, "/", "|") + "/*";
            subscriptions.emplace_back(topic, readHandler);
        }
    }

    std::cout << "\nStart monitoring signal and slot rates of \n   domain         '" << domain
              << "'\n   on broker     '" << client->getBrokerUrl() << "',\n   ";
    if (!receivers.empty()) {
        std::cout << "messages to   '" << util::toString(receivers) << "',\n   ";
    }
    if (!senders.empty()) {
        std::cout << "messages from '" << util::toString(senders) << "',\n   ";
    }
    std::cout << "interval is   " << interval << " s." << std::endl;

    // subscribing means 'start reading'...
    ec = client->subscribe(subscriptions);
    if (ec) {
        throw KARABO_NETWORK_EXCEPTION("Failed to subscribe to REDIS broker");
    }

    if (debug) std::cout << "\n------------------ wait on work" << std::endl;
    // Block forever
    net::EventLoop::work();
}

void startAmqpMonitor(const std::vector<std::string>& brokers, const std::string& domain,
                      const std::vector<std::string>& receivers, std::vector<std::string>& senders,
                      const util::TimeValue& interval) {
    // Skip message body deserialization: "skipFlag"
    const util::Hash config("brokers", brokers, "domain", domain, "skipFlag", true);
    net::AmqpClient::Pointer client = util::Configurator<net::AmqpClient>::create("AmqpClient", config);

    // Connect and subscribe
    auto ec = client->connect();
    if (ec) {
        throw KARABO_NETWORK_EXCEPTION("Failed to connect to AMQP (RabbitMQ) broker at " + util::toString(brokers));
    }

    boost::shared_ptr<BrokerStatistics> stats(boost::make_shared<BrokerStatistics>(interval, receivers, senders));
    // `BrokerStatistics' expects the message formatted as a Hash: with 'header' as Hash and 'raw' as vector of char
    // which serialized 'body' value.  If the incoming message does not contain 'raw' key, serialize the body part.
    auto binSerializer = io::BinarySerializer<util::Hash>::create("Bin");
    util::Hash::Pointer body(boost::make_shared<util::Hash>());

    auto readHandler = [stats, binSerializer, body](const boost::system::error_code& ec,
                                                    const util::Hash::Pointer& msg) {
        if (!ec) {
            std::vector<char>& raw = body->bindReference<std::vector<char>>("raw");
            if (msg->has("raw")) {
                raw.swap(msg->get<std::vector<char>>("raw"));
            } else {
                binSerializer->save(msg->get<util::Hash>("body"), raw); // body -> raw
            }
            stats->registerMessage(boost::make_shared<util::Hash>(msg->get<util::Hash>("header")), body);
        }
    };

    client->registerConsumerHandler(readHandler);

    std::cout << "\nStart monitoring signal and slot rates of \n   domain         '" << domain
              << "'\n   on broker     '" << client->getBrokerUrl() << "',\n   ";
    if (!receivers.empty()) {
        std::cout << "messages to   '" << util::toString(receivers) << "',\n   ";
    }
    if (!senders.empty()) {
        std::cout << "messages from '" << util::toString(senders) << "',\n   ";
    }
    std::cout << "interval is   " << interval << " s." << std::endl;

    const std::string slotsExchange = domain + ".slots";
    const std::string signalsExchange = domain + ".signals";

    if (receivers.empty() && senders.empty()) {
        // No input on commandline
        ec = client->subscribe(slotsExchange, "#");
        if (ec) {
            throw KARABO_NETWORK_EXCEPTION("Failed to subscribe to AMQP broker");
        }
        ec = client->subscribe(signalsExchange, "*.*");
        if (ec) {
            throw KARABO_NETWORK_EXCEPTION("Failed to subscribe to AMQP broker");
        }
    } else {
        for (const auto& rid : receivers) {
            ec = client->subscribe(slotsExchange, rid);
            if (ec) {
                throw KARABO_NETWORK_EXCEPTION("Failed to subscribe to AMQP broker");
            }
        }
        for (const auto& sid : senders) {
            ec = client->subscribe(signalsExchange, sid + ".*");
            if (ec) {
                throw KARABO_NETWORK_EXCEPTION("Failed to subscribe to AMQP broker");
            }
        }
    }

    // Block forever
    net::EventLoop::work();
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


int main(int argc, const char** argv) {
    net::EventLoop::addThread(2); // e.g. for JmsConsumer's serializer and handler strands

    // Setup option defaults
    karabo::util::Hash options("period", static_cast<util::TimeValue>(5ull), "--receivers", "", "--senders", "",
                               "--receiversServer", "", "--sendersServer", "", "--discoveryWait", "0");
    for (int i = 1; i < argc; i += 2) {
        const std::string argv_i(argv[i]);
        if (argv_i == "-h" || argv_i == "--help") { // both for backward compatibility
            printHelp(argv[0]);
            return EXIT_SUCCESS;
        } else if (argc == i + 1) {
            // The last of an odd number of arguments maybe the averaging period
            options.set("period", strtoull(argv[i], 0, 0));
        } else if (argv_i != "--receivers" && argv_i != "--senders" && argv_i != "--receiversServer" &&
                   argv_i != "--sendersServer" && argv_i != "--discoveryWait" && argv_i != "--debug") {
            printHelp(argv[0]);
            return EXIT_FAILURE;
        } else {
            options.set(argv[i], argv[i + 1]);
        }
    }
    // fromString<bool>(..) understands y, yes, Yes, true, True, 1, n, no, No, false, False, 0 and maybe more...
    debug = (options.has("--debug") ? util::fromString<bool>(options.get<std::string>("--debug")) : false);

    const std::string topic(karabo::net::Broker::brokerDomainFromEnv());
    const util::TimeValue interval = options.get<util::TimeValue>("period");

    // Unpack configured senders and receivers.
    std::vector<std::string> receivers(
          karabo::util::fromString<std::string, std::vector>(options.get<std::string>("--receivers")));
    std::vector<std::string> senders(
          karabo::util::fromString<std::string, std::vector>(options.get<std::string>("--senders")));
    // If full servers are requested, unpack and insert to senders and receivers as well.
    const auto recAndSendFromServers =
          instancesOfServers(options.get<std::string>("--receiversServer"), options.get<std::string>("--sendersServer"),
                             util::fromString<unsigned int>(options.get<std::string>("--discoveryWait")), debug);
    receivers.insert(receivers.end(), recAndSendFromServers.first.begin(), recAndSendFromServers.first.end());
    senders.insert(senders.end(), recAndSendFromServers.second.begin(), recAndSendFromServers.second.end());

    // Start Logger, but suppress INFO and DEBUG
    log::Logger::configure(util::Hash("priority", "WARN"));
    log::Logger::useOstream();

    const std::vector<std::string> brokers(net::Broker::brokersFromEnv());
    const std::string brkType = net::Broker::brokerTypeFrom(brokers);

    try {
        if (brkType == "jms") {
            startJmsMonitor(brokers, topic, receivers, senders, interval);
        } else if (brkType == "mqtt") {
            startMqttMonitor(brokers, topic, receivers, senders, interval);
        } else if (brkType == "amqp") {
            startAmqpMonitor(brokers, topic, receivers, senders, interval);
        } else if (brkType == "redis") {
            startRedisMonitor(brokers, topic, receivers, senders, interval);
        } else {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION(brkType + " not supported!");
        }

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
