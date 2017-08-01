from unittest import TestCase, main
from jupyter_client.kernelspec import get_kernel_spec


class Tests(TestCase):
    def test_kernel(self):
        get_kernel_spec("Karabo")


if __name__ == "__main__":
    main()
