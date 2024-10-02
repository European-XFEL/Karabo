/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   AmqpUtils.cc
 * Author: gero.flucke@xfel.eu, based on code from serguei.essenov@xfel.eu
 *
 * Created on Apr 16, 2024
 */

#include "AmqpUtils.hh"

#include <amqpcpp.h>

#include <boost/system/error_category.hpp>
#include <string>

namespace karabo::net {

    //----- https://akrzemi1.wordpress.com/2017/07/12/your-own-error-code/ -----v

    namespace { // anonymous namespace

        struct AmqpCppErrCategory : boost::system::error_category {
            const char* name() const noexcept override;
            std::string message(int ev) const override;
        };


        const char* AmqpCppErrCategory::name() const noexcept {
            return "amqpcpp";
        }


        std::string AmqpCppErrCategory::message(int ev) const {
            switch (static_cast<AmqpCppErrc>(ev)) {
                case AmqpCppErrc::eCreateChannelError:
                    return "error creating channel";

                case AmqpCppErrc::eCreateExchangeError:
                    return "error creating exchange";

                case AmqpCppErrc::eCreateQueueError:
                    return "error creating queue";

                case AmqpCppErrc::eBindQueueError:
                    return "error binding queue";

                case AmqpCppErrc::eCreateConsumerError:
                    return "error creating consumer";

                case AmqpCppErrc::eUnbindQueueError:
                    return "error unbinding queue";

                case AmqpCppErrc::eDrop:
                    return "channel dropped error";

                case AmqpCppErrc::eMessageDrop:
                    return "message dropped error";

                default:
                    return "(unrecognized error)";
            }
        }

        const AmqpCppErrCategory theAmqpCppErrCategory{};

    } // anonymous namespace


    boost::system::error_code make_error_code(AmqpCppErrc e) {
        return {static_cast<int>(e), theAmqpCppErrCategory};
    }

    //----- https://akrzemi1.wordpress.com/2017/07/12/your-own-error-code/ -----^

} // namespace karabo::net
