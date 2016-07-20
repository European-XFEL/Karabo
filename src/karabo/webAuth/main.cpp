/* 
 * File:   main.cpp
 * Author: maia
 *
 * Created on April 10, 2013, 3:55 PM
 */

#include <cstdlib>
#include <string>
//#include <../../Libraries/boost_1_53_0/boost/lambda/lambda.hpp>

#include "authentication/soapAuthenticationPortBindingProxy.h"
#include "authentication/AuthenticationPortBinding.nsmap"

using namespace std;

//Declarations
AuthenticationPortBindingProxy service;
bool OPERATION_SUCESS_TRUE = true;
bool OPERATION_SUCESS_FALSE = false;


/*
 *
 */
ns1__getUserNonceResponse getUserNonce(string username, string provider, string ipAddress) {

    ns1__getUserNonce nsUserNonce;
    ns1__getUserNonceResponse nsUserNonceResp;

    nsUserNonce.username = &username;
    nsUserNonce.provider = &provider;
    nsUserNonce.ipAddress = &ipAddress;

    // If obtain successfully answer from Web Service it print message returned!
    if (service.getUserNonce(&nsUserNonce, &nsUserNonceResp) == SOAP_OK) {
        //std::cout << &(nsUserNonceResp.return_) << std::endl;
        if (*nsUserNonceResp.return_->operationSuccess == 1) {
            printf("The nonce was: %s\n", nsUserNonceResp.return_->sessionToken->c_str());
        } else {
            printf("Error message: %s\n", nsUserNonceResp.return_->errorMsg->c_str());
        }
    } else {
        soap_print_fault(service.soap, stderr);
    }

    return nsUserNonceResp;
}


/*
 *
 */
ns1__loginResponse login(string username, string password, string provider,
                         string ipAddress, string hostname, string portNumber,
                         string nonce, string software, string time) {

    ns1__login nsLogin;
    ns1__loginResponse nsLoginResp;

    nsLogin.username = &username;
    nsLogin.password = &password;
    nsLogin.provider = &provider;
    nsLogin.ipAddress = &ipAddress;
    nsLogin.hostname = &hostname;
    nsLogin.portNumber = &portNumber;
    nsLogin.nonce = &nonce;
    nsLogin.software = &software;
    nsLogin.time = &time;




    // If obtain successfully answer from Web Service it print message returned!
    if (service.login(&nsLogin, &nsLoginResp) == SOAP_OK) {
        //std::cout << &(nsLoginResp.return_) << std::endl;
        if (*nsLoginResp.return_->operationSuccess == 1) {
            printf("The SessionToken is: %s\n", nsLoginResp.return_->sessionToken->c_str());
        } else {
            printf("Error message: %s\n", nsLoginResp.return_->errorMsg->c_str());
        }
    } else {
        soap_print_fault(service.soap, stderr);
    }

    return nsLoginResp;
}


int authenticate(string username, string password, string provider, string ipAddress,
                 string hostname, string portNumber, string software, string time) {

    ns1__getUserNonceResponse nsUserNonceResp;
    ns1__loginResponse nsLoginResp;

    nsUserNonceResp = getUserNonce(username, provider, ipAddress);
    if (!nsUserNonceResp.return_->operationSuccess) {
        return 0;
    }

    string nonce = nsUserNonceResp.return_->sessionToken->c_str();

    nsLoginResp = login(username, password, provider, ipAddress, hostname, portNumber, nonce, software, time);
    if (!nsLoginResp.return_->operationSuccess) {
        return 0;
    }
    printf("Information received: \n");
    printf("firstName: %s\n", nsLoginResp.return_->firstName->c_str());
    printf("familyName: %s\n", nsLoginResp.return_->familyName->c_str());
    printf("username: %s\n", nsLoginResp.return_->username->c_str());
    printf("provider: %s\n", nsLoginResp.return_->provider->c_str());
    printf("roleDesc: %s\n", nsLoginResp.return_->roleDesc->c_str());
    printf("softwareDesc: %s\n", nsLoginResp.return_->softwareDesc->c_str());
    printf("sessionToken: %s\n", nsLoginResp.return_->sessionToken->c_str());
    printf("welcomeMessage: %s\n", nsLoginResp.return_->welcomeMessage->c_str());



    return 1; //If arrives here returns success
}


/*
 * 
 */
int main(int argc, char** argv) {

    string username = "maial";
    string password = "pass";
    string provider = "LOCAL";
    string ipAddress = "127.0.0.1";
    string hostname = "127.0.0.1";
    string portNumber = "4444";
    string software = "Karabo";
    string time = "20130410145159257";

    // Call Authentication function
    return authenticate(username, password, provider, ipAddress, hostname, portNumber, software, time);

}

