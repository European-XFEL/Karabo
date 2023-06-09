# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
LIST_DOMAINS_QUERY = """
    xquery version "3.0";
    <collections>{{
    for $c in xmldb:get-child-collections("{}")
    return <item>{{$c}}</item>}}
    </collections>
    """
ROOT_COLLECTION = "krb_config"
ROOT_COLLECTION_TEST = "krb_test"
ROOT_COLLECTION_BACKUP = "krb_backup"

DATE_FORMAT = '%Y-%m-%d %H:%M:%S'
