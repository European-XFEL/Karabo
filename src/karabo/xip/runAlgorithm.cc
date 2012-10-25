/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *   
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <exfel/core/Runner.hh>

#include "Algorithm.hh"

using namespace exfel::core;
using namespace exfel::xip;
        
int main(int argc, char** argv) {
    try {    
        
        Algorithm::Pointer algorithm = Runner<Algorithm>::instantiate(argc, argv);
        if (algorithm) algorithm->compute();
                
    } catch (const exfel::util::Exception& e) {
        std::cout << e;
    } catch (...) {
        RETHROW
    }
}
