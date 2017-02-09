from collections.abc import Mapping
import re
import shlex

_LEADING_WHITESPACE_REGEX = re.compile(r'^\s+.*')


def _format_shell_variable(name, value):
    return '{0}={1}\n'.format(name, shlex.quote(value))


def _parse_shell_line(line):
    """Parse a line with shlex to see if it is a variable declaration
    """
    # Ignore lines beginning with whitespace (like indentation)
    if _LEADING_WHITESPACE_REGEX.match(line) is not None:
        return []

    # We are looking for lines which are exactly `VARNAME=value`
    tokens = list(shlex.shlex(line, posix=True))
    if len(tokens) == 3 and tokens[1] == '=':
        return tokens
    return []


class ShellNamespaceWrapper(Mapping):
    """An object which gives access to variables in Unix shell script files.

    Allows variables to be read and written and gives the ability to write
    changes back to a file without harming its other contents.
    """

    def __init__(self, path, growable=False):
        self._path = path
        self._growable = growable
        self._vars = self._read_file()

    def __getitem__(self, key):
        return self._vars[key]

    def __iter__(self):
        return iter(self._vars)

    def __len__(self):
        return len(self._vars)

    def __setitem__(self, key, value):
        if key not in self._vars and not self._growable:
            msg = 'Attempted variable addition to a fixed namespace!'
            raise RuntimeError(msg)
        self._vars[key] = value

    def write(self):
        """Write the file, replacing any variables with those found in the
        vars dictionary.
        """
        with open(self._path, 'r') as fp:
            lines = fp.readlines()

        vars_copy = self._vars.copy()
        for lineno in range(len(lines)):
            line = lines[lineno]
            tokens = _parse_shell_line(line)
            if tokens:
                name = tokens[0]
                if name in vars_copy:
                    value = vars_copy.pop(name)
                    line = _format_shell_variable(name, value)
            lines[lineno] = line

        # Account for added variables
        if self._growable and vars_copy:
            lines.append('\n')
            for name, value in vars_copy.items():
                lines.append(_format_shell_variable(name, value))
            lines.append('\n')

        with open(self._path, 'w') as fp:
            fp.writelines(lines)

    def _read_file(self):
        """Read the file and return its variables as a vars dictionary.
        """
        vars = {}
        with open(self._path, 'r') as fp:
            for line in fp:
                tokens = _parse_shell_line(line)
                if tokens:
                    name, _, value = tokens
                    vars[name] = value

        return vars
