/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "LowLevelControlDevice.hh"


using namespace std;
using namespace exfel::util;

namespace exfel {
  namespace core {

    EXFEL_REGISTER_FACTORY_CC(Device, LowLevelControlDevice)

   
    LowLevelControlDevice::~LowLevelControlDevice() {
    }

    void LowLevelControlDevice::expectedParameters(Config& expected) {
    }

    void LowLevelControlDevice::configure(const Config& input) {
    }

    void LowLevelControlDevice::run() {
     
      while (true) {
        cout << "Setup signal (only body): ";
        string signal;
        cin >> signal;
        if (cin.fail()) {
          cout << "Requires a string" << endl;
          std::cin.clear();
        }
        if (signal == "q") break;
        registerSignal<string>(signal);
        cout << "Command: \"" << signal << "(string)\" registered " << endl;
      }

      while (true) {
        cout << "Enter connection as two strings, signal and slot (q for quit): ";
        string signal, slot;
        cin >> signal >> slot;
        if (cin.fail()) {
          if (signal == "q") break;
          cout << "Requires two strings" << endl;
          std::cin.clear();
        } else {
          if (signal == "q") break;
          connect(signal, slot);
          cout << "Connected: " << signal << " <-> " << slot << endl;
        }
      }
      while (true) {
        string function, a1;
        cout << "Emit signal: " << endl;
        cin >> function >> a1;
        if (cin.fail()) {
          if (function == "q") break;
          cout << "Requires two strings" << endl;
          std::cin.clear();
        } else {
          emit(function, a1);
        }
      }
    }
  }
}
