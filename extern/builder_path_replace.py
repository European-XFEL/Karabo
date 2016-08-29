
import argparse as ap
import os
import os.path as op
import re
import subprocess
import tokenize


def fix_paths(build_dir, search_pattern, replacement_text):
    abs_build_dir = op.abspath(build_dir)
    regex = re.compile(re.escape(search_pattern))

    for dirpath, dirname, filenames in os.walk(abs_build_dir):
        for fn in filenames:
            path = op.join(dirpath, fn)
            file_type = read_file_type(path)
            if file_type.startswith('text'):
                substitute_pattern(path, regex, replacement_text)


def run_cmd(cmd):
    output = subprocess.check_output(cmd, universal_newlines=True,
                                     stderr=subprocess.DEVNULL)
    return output.split()


def read_file_type(path):
    cmd = ['file', '-b', '--mime-type', path]
    try:
        return run_cmd(cmd)[0]
    except (subprocess.CalledProcessError, IndexError):
        pass
    return ''


def substitute_pattern(path, regex, replace_with):
    try:
        with open(path, mode='rb') as fp:
            encoding = tokenize.detect_encoding(fp.readline)[0]
    except SyntaxError:
        print('Unreadable file "{}" ignored'.format(path))
        return

    try:
        with open(path, mode='r', encoding=encoding) as fin:
            text = fin.readlines()
    except UnicodeDecodeError:
        print('Failed to read file "{}"'.format(path))
        return

    rewritten_text = [regex.sub(replace_with, line) for line in text]
    with open(path, mode='w', encoding=encoding) as fout:
        fout.writelines(rewritten_text)


def main():
    parser = ap.ArgumentParser(
        formatter_class=ap.ArgumentDefaultsHelpFormatter)
    parser.add_argument('build_directory',
                        help='The $INSTALL_PREFIX of an external deps build.')
    parser.add_argument('search_pattern',
                        help='The text to search $INSTALL_PREFIX for')
    parser.add_argument('replace_text',
                        help='The text to replace "search_pattern" with')

    args = parser.parse_args()
    fix_paths(args.build_directory, args.search_pattern, args.replace_text)

if __name__ == '__main__':
    main()
