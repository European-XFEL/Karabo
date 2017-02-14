from nose.tools import assert_raises

from karabo.common.api import ShellNamespaceWrapper
from karabo.testing.utils import temp_xml_file

SHELL_SCRIPT = """
#! /usr/bin/sh
NAME=value
OTHER_NAME=42
# Interpreted variable values are ignored
INTERPRETED=$NAME

for name in $(ls .) do
    # Indented variable declarations are also ignored
    INDENTED_DECL="anyhow ignored"
    VAR=$name
    echo $name
done
"""


def test_basics():
    with temp_xml_file(SHELL_SCRIPT) as path:
        shfile = ShellNamespaceWrapper(path)

        names = sorted(shfile)
        assert names == ['NAME', 'OTHER_NAME']
        assert shfile['NAME'] == 'value'
        assert shfile['OTHER_NAME'] == '42'

        # Writing after no mutation preserves the contents!
        shfile.write()
        with open(path, 'r') as fp:
            text = fp.read()
        assert text == SHELL_SCRIPT


def test_disallow_variable_addition():
    with temp_xml_file(SHELL_SCRIPT) as path:
        shfile = ShellNamespaceWrapper(path)

        with assert_raises(RuntimeError):
            shfile['NOPE'] = 'not allowed'


def test_dangerous_variables():
    DANGEROUS_VALUE0 = '"evil"; rm -rf /'
    DANGEROUS_VALUE1 = '$(rm -rf /)'
    with temp_xml_file(SHELL_SCRIPT) as path:
        shfile = ShellNamespaceWrapper(path)

        shfile['NAME'] = DANGEROUS_VALUE0
        shfile['OTHER_NAME'] = DANGEROUS_VALUE1
        shfile.write()

        reread_shfile = ShellNamespaceWrapper(path)
        # The fact that the dangerous values come back as regular variables
        # mean that they were safely quoted.
        assert reread_shfile['NAME'] == DANGEROUS_VALUE0
        assert reread_shfile['OTHER_NAME'] == DANGEROUS_VALUE1


def test_variable_addition():
    with temp_xml_file(SHELL_SCRIPT) as path:
        shfile = ShellNamespaceWrapper(path, growable=True)

        shfile['NEW_NAME'] = 'Leonce'
        shfile.write()

        reread_shfile = ShellNamespaceWrapper(path)
        assert 'NEW_NAME' in reread_shfile
        assert reread_shfile['NEW_NAME'] == 'Leonce'


def test_variable_replacement():
    with temp_xml_file(SHELL_SCRIPT) as path:
        shfile = ShellNamespaceWrapper(path)

        shfile['NAME'] = 'not'
        shfile.write()

        reread_shfile = ShellNamespaceWrapper(path)
        assert reread_shfile['NAME'] == 'not'
