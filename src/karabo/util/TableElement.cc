/*
 * Author: <gero.flucke@xfel.eu>
 *
 * Created on March 23, 2016, 12:10 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "TableElement.hh"

const karabo::util::Validator::ValidationRules
karabo::util::tableValidationRules(
        /* injectDefaults */ true,
        /* allowUnrootedConfiguration */ true,
        /* allowAdditionalKeys */ false,
        /* allowMissingKeys */ false,
        /* injectTimestamps */ false
        );
