/*
 * $Id$
 *
 * Author: <your.email@xfel.eu>
 *
 * Created on September 28, 2011, 3:28 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <assert.h>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "../Test.hh"
#include "../Exception.hh"

using namespace std;
using namespace exfel::util;
using namespace boost;


class A; // forward

class B {
public:

    B(){}
    
    B(A* a) {
        m_a = a;
    }

    void bar() {
        try {
            cout << "bar" << endl;
            crash();
            cout << "bar" << endl;
        } catch (const Exception& e) {
            cout << "Exception thrown in thread" << endl;
            Exception::memorize();
        }
    }
        
private:

    void crash() {
        throw LOGIC_EXCEPTION("Crashing because its part of the test");
    }

private: // members
    A* m_a;
    B* m_self;

};

class A {
public:

    A() {
        m_b = B(this);
        m_thread = boost::thread(boost::bind(&A::foo, this));
        m_thread.join();
        if (Exception::hasUnhandled()) {
            cout << "Rethrowing in main thread" << endl;
            throw PROPAGATED_EXCEPTION("Rethrown within main thread");
        }
       
    }

    void foo() {
        cout << "foo" << endl;
        m_b.bar();
    }
    
private:

    B m_b;
    boost::thread m_thread;
};

int testException(int argc, char** argv) {

    Test t;
    TEST_INIT(t, argc, argv);
    cout << t << endl;

    try {
        try {
            A a;
        } catch (...) {
            RETHROW_AS(LOGIC_EXCEPTION("Construction of A did not succeed"))
        }
    } catch (const Exception& e) {
        cout << e.userFriendlyMsg();
        cout << e;
    }

    return 0;
}
