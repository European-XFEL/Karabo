# flake8: noqa
from .controller.build import (
    create_project_controller, destroy_project_controller
)
from .utils import (
    add_device_to_server, check_device_instance_exists, load_project,
    maybe_save_modified_project, save_object, show_save_project_message
)
from .view import ProjectView

# XXX: This is only until the Gui Server is in shape
TEST_DOMAIN = 'CAS_INTERNAL'
