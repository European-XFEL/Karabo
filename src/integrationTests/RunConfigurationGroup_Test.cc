#include "DeviceServerRunner_Test.hh"


USING_KARABO_NAMESPACES;

#define KRB_TEST_MAX_TIMEOUT 10



void DeviceServerRunner_Test::testRunConfigurationGroup() {
    Hash config("deviceId", "testRunConfigurationGroup_0",
                "group", Hash("id", "Sample Environment",
                              "description",  "A group summarizing data sources of SPB SAMPLE domain.",
                              "expert", vector<Hash>(),
                              "user", vector<Hash>()));
    
    vector<Hash>& expert = config.get<vector<Hash> >("group.expert");
    {
        Hash s1("source", "SASE1/SPB/SAMP/INJ_FLOW",
                "type", "control",
                "behavior", "read-only",
                "monitored", false);
        s1.setAttribute("source", "pipeline", false);
        expert.push_back(s1);

        Hash s2("source", "SASE1/SPB/SAMP/INJ_CAM_1",
                "type", "control",
                "behavior", "read-only",
                "monitored", false);
        s2.setAttribute("source", "pipeline", false);
        expert.push_back(s2);

        Hash s3("source", "SASE1/SPB/SAMP/INJ_CAM_1:ch1",
                "type", "control",
                "behavior", "init",
                "monitored", true);
        s3.setAttribute("source", "pipeline", true);
        expert.push_back(s3);
    }
    
    vector<Hash>& user = config.get<vector<Hash> >("group.user");
    {
        Hash s1("source", "SASE1/SPB/SAMP/INJ_TEMP_1",
                "type", "control",
                "behavior", "read-only",
                "monitored", false);
        s1.setAttribute("source", "pipeline", false);
        user.push_back(s1);

        Hash s2("source", "SASE1/SPB/SAMP/INJ_TEMP_2",
                "type", "control",
                "behavior", "read-only",
                "monitored", false);
        s2.setAttribute("source", "pipeline", false);
        user.push_back(s2);
    }
    
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testDeviceServer_0", "RunConfigurationGroup",
                                                                       config, KRB_TEST_MAX_TIMEOUT);
    //clog << "Result of instantiate is '" << success.second << "'" << endl;
    CPPUNIT_ASSERT(success.first);
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    
    //===========  Start tests for RunConfigurationGroup
    testGetGroup();
    
    //----------- Stop tests for RunConfigurationGroup
    
    std::pair<bool, std::string> rc = m_deviceClient->killDevice("testRunConfigurationGroup_0", KRB_TEST_MAX_TIMEOUT);
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    CPPUNIT_ASSERT(rc.first);
}


void DeviceServerRunner_Test::testGetGroup() {
    clog << "Test getting group structure and check the validity ..." << endl;
    
    Hash group = m_deviceClient->get<Hash>("testRunConfigurationGroup_0", "group");

    CPPUNIT_ASSERT(group.get<string>("id") == "Sample Environment");
    CPPUNIT_ASSERT(group.get<string>("description") == "A group summarizing data sources of SPB SAMPLE domain.");

    CPPUNIT_ASSERT(group.get<string>("expert[0].source") == "SASE1/SPB/SAMP/INJ_FLOW");
    CPPUNIT_ASSERT(group.get<string>("expert[0].type") == "control");
    CPPUNIT_ASSERT(group.get<string>("expert[0].behavior") == "read-only");
    CPPUNIT_ASSERT(group.get<bool>("expert[0].monitored") == false);
    CPPUNIT_ASSERT(group.getAttribute<bool>("expert[0].source", "pipeline") == false);
    
    CPPUNIT_ASSERT(group.get<string>("expert[1].source") == "SASE1/SPB/SAMP/INJ_CAM_1");
    CPPUNIT_ASSERT(group.get<string>("expert[1].type") == "control");
    CPPUNIT_ASSERT(group.get<string>("expert[1].behavior") == "read-only");
    CPPUNIT_ASSERT(group.get<bool>("expert[1].monitored") == false);
    CPPUNIT_ASSERT(group.getAttribute<bool>("expert[1].source", "pipeline") == false);
    
    CPPUNIT_ASSERT(group.get<string>("expert[2].source") == "SASE1/SPB/SAMP/INJ_CAM_1:ch1");
    CPPUNIT_ASSERT(group.get<string>("expert[2].type") == "control");
    CPPUNIT_ASSERT(group.get<string>("expert[2].behavior") == "init");
    CPPUNIT_ASSERT(group.get<bool>("expert[2].monitored") == true);
    CPPUNIT_ASSERT(group.getAttribute<bool>("expert[2].source", "pipeline") == true);
    
    CPPUNIT_ASSERT(group.get<string>("user[0].source") == "SASE1/SPB/SAMP/INJ_TEMP_1");
    CPPUNIT_ASSERT(group.get<string>("user[0].type") == "control");
    CPPUNIT_ASSERT(group.get<string>("user[0].behavior") == "read-only");
    CPPUNIT_ASSERT(group.get<bool>("user[0].monitored") == false);
    CPPUNIT_ASSERT(group.getAttribute<bool>("user[0].source", "pipeline") == false);
    
    CPPUNIT_ASSERT(group.get<string>("user[1].source") == "SASE1/SPB/SAMP/INJ_TEMP_2");
    CPPUNIT_ASSERT(group.get<string>("user[1].type") == "control");
    CPPUNIT_ASSERT(group.get<string>("user[1].behavior") == "read-only");
    CPPUNIT_ASSERT(group.get<bool>("user[1].monitored") == false);
    CPPUNIT_ASSERT(group.getAttribute<bool>("user[1].source", "pipeline") == false);
    
    clog << "Test getting group structure and check the validity ... OK" << endl;
}
