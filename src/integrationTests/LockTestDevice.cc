/* 
 * File:   LockTestDevice.cc
 * Author: steffen
 * 
 * Created on October 2, 2016, 1:23 PM
 */

#include "LockTestDevice.hh"

using namespace std;

USING_KARABO_NAMESPACES

namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, LockTestDevice)

    void LockTestDevice::expectedParameters(Schema& expected) {
        
        STRING_ELEMENT(expected).key("controlledDevice")
                .assignmentOptional().defaultValue("")
                .commit();
        
         STRING_ELEMENT(expected).key("errorMessage")
                .readOnly()
                .commit();
       
        INT32_ELEMENT(expected).key("intProperty")
                .assignmentOptional().defaultValue(0)
                .commit();
        
        
        SLOT_ELEMENT(expected).key("lockAndWait")
            .commit();

        
    }


    LockTestDevice::LockTestDevice(const karabo::util::Hash& config) : Device<>(config) {
        KARABO_SLOT(lockAndWait);
        KARABO_SLOT(lockAndWaitLong);
        KARABO_SLOT(lockAndWaitTimeout);
        KARABO_SLOT(lockAndWaitRecursive);
        KARABO_SLOT(lockAndWaitRecursiveFail);
        KARABO_INITIAL_FUNCTION(initialize);
        
        
    }


    LockTestDevice::~LockTestDevice() {
    }

    void LockTestDevice::initialize() {
    }
    

  
    
    
    void LockTestDevice::lockAndWait(){
        try{
            Lock lk = remote().lock(this->get<std::string>("controlledDevice"), false, 0);
            for(int i = 0; i < 5; ++i){
                if(lk.valid()){
                    remote().set(this->get<std::string>("controlledDevice"), "intProperty", i);
                    boost::this_thread::sleep(boost::posix_time::milliseconds(250));
                } else {
                    set("errorMessage", "Lock was invalidated!");
                    reply("Lock was invalidated!");
                    return;
                }
            }
            reply("Acquired Lock");
        } catch (const karabo::util::LockException& e){
            set("errorMessage", e.userFriendlyMsg());
            reply(e.userFriendlyMsg());
        }

    }
    
    void LockTestDevice::lockAndWaitLong(){
        try{
            Lock lk = remote().lock(this->get<std::string>("controlledDevice"), false, 0);
            for(int i = 0; i < 5; ++i){
                if(lk.valid()){
                    remote().set(this->get<std::string>("controlledDevice"), "intProperty", i);
                    boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
                } else {
                    set("errorMessage", "Lock was invalidated!");
                    reply("Lock was invalidated!");
                    return;
                }
            }
            reply("Acquired Lock");
        } catch (const karabo::util::LockException& e){
            set("errorMessage", e.userFriendlyMsg());
            reply(e.userFriendlyMsg());
        }

    }
    
    void LockTestDevice::lockAndWaitTimeout(){
        try{
            Lock lk = remote().lock(this->get<std::string>("controlledDevice"), false, 1);
            reply("Acquired Lock");
        } catch (const karabo::util::LockException& e){
            set("errorMessage", e.userFriendlyMsg());
            reply(e.userFriendlyMsg());
        }

    }
    
    
    void LockTestDevice::lockAndWaitRecursive(){
        try{
            Lock lk = remote().lock(this->get<std::string>("controlledDevice"), true, 5);
            for(int i = 0; i < 5; ++i){
                Lock lk = remote().lock(this->get<std::string>("controlledDevice"), true, 0);
                remote().set(this->get<std::string>("controlledDevice"), "intProperty", i);
                boost::this_thread::sleep(boost::posix_time::milliseconds(500));
            }
            reply("Acquired Lock");
        } catch (const karabo::util::LockException& e){
            set("errorMessage", e.userFriendlyMsg());
            reply(e.userFriendlyMsg());
        }

    }
    
    void LockTestDevice::lockAndWaitRecursiveFail(){
        try{
            Lock lk = remote().lock(this->get<std::string>("controlledDevice"), false, 1);
            for(int i = 0; i < 5; ++i){
                Lock lk = remote().lock(this->get<std::string>("controlledDevice"), false, 0);
                remote().set(this->get<std::string>("controlledDevice"), "intProperty", i);
                boost::this_thread::sleep(boost::posix_time::milliseconds(500));
            }
            reply("Acquired Lock");
        } catch (const karabo::util::LockException& e){
            set("errorMessage", e.userFriendlyMsg());
            reply(e.userFriendlyMsg());
        }

    }
    
    
   
}