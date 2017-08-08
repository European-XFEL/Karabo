/* 
 * File:   p2pMessageLogger.cc
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on August 7, 2017, 12:02 PM
 */

#include <iostream>
#include <karabo/core/DeviceClient.hh>
#include <karabo/net/PointToPoint.hh>

using namespace std;
using namespace karabo::util;
using namespace karabo::log;
using namespace karabo::net;
using namespace karabo::core;


void readHandler(const Hash::Pointer& header, const Hash::Pointer& message) {

    cout << *header << endl;
    cout << *message << endl;
    cout << "-----------------------------------------------------------------------" << endl << endl;
}


int main(int argc, char** argv) {

    try {
        boost::thread t(EventLoop::work);

        // Start Logger
        Logger::configure(Hash("priority", "ERROR"));
        Logger::useOstream();

        // Create pointToPoint object and register handler
        PointToPoint::Pointer pointToPoint = boost::make_shared<PointToPoint>();
        pointToPoint->start(boost::bind(readHandler, _1, _2));

        // Create device client
        DeviceClient client;

        // Get system information
        vector<string> devices;
        Hash sysinfo = client.getSystemInformation();
        if (!sysinfo.has("device")) {
            cout << "\nWARNING:  No devices found around!\n" << endl;
            return 1;
        }
        const Hash& infos = sysinfo.get<Hash>("device");
        devices.reserve(infos.size());

        // populate the pointToPoint object with info
        for (Hash::const_map_iterator it = infos.mbegin(); it != infos.mend(); ++it) {
            const string& remoteInstanceId = it->second.getKey();
            if (!it->second.hasAttribute("type") || !it->second.hasAttribute("serverId")) continue;
            if (!it->second.hasAttribute("p2p_connection")) continue;
            const string& remoteType = it->second.getAttribute<string>("type");
            const string& remoteUrl = it->second.getAttribute<string>("p2p_connection");
            if (remoteType != "device") continue;
            pointToPoint->updateUrl(remoteInstanceId, remoteUrl);
            devices.push_back(remoteInstanceId);
        }

        cout << "#################################################################################################\n";
        cout << "\n# Starting to consume messages from the following devices ..." << endl;
        cout << toString(devices) << endl << endl;

        for (const auto& remoteInstanceId : devices) pointToPoint->connectAsync(remoteInstanceId);

        cout << pointToPoint->allMapsToString() << endl;
        cout << "#################################################################################################\n\n";

        t.join();

    } catch (const Exception& e) {
        cerr << e << endl;
        return EXIT_FAILURE;
    }
    return 0;
}
