# The default password of the docker image:
# https://git.xfel.eu/gitlab/ITDM/docker_existdb
TESTDB_ADMIN_PASSWORD = "change_me_please"

LIST_DOMAINS_QUERY = """
    xquery version "3.0";
    <collections>{{
    for $c in xmldb:get-child-collections("{}")
    return <item>{{$c}}</item>}}
    </collections>
    """
