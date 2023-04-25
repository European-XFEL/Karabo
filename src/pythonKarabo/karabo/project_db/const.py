# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
