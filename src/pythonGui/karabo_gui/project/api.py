# flake8: noqa
from .db_connection import ProjectDatabaseConnection, get_db_conn
from .view import ProjectView

# XXX: This is only until the Gui Server is in shape
TEST_DOMAIN = 'LOCAL'
