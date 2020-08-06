import argparse
import os
import re
import subprocess

# these constants match the src/ subfolder where
# the tests are located
GUI = "pythonGui"
PYKARABO = "pythonKarabo"
INTEGRATION = "integrationTests"
CPP_LONG = "cppLongTests"
KARABO = "karabo"
TESTS= (
    GUI,
    PYKARABO,
    INTEGRATION,
    CPP_LONG,
    KARABO
)
UNIT_TESTS = (PYKARABO, KARABO, GUI)


def check_diff():
    """returns an iterable of tests that shall be performed
    
    it uses the gitlab CI predefined variables
    (https://docs.gitlab.com/ee/ci/variables/predefined_variables.html)
    to establish which part of the code will be merged to the
    target branch and returns an iterable of strings matching the 
    src/ subfolder where the tests are located.
    """
    if any(["CI_MERGE_REQUEST_TARGET_BRANCH_NAME" not in os.environ,
            "CI_COMMIT_REF_NAME" not in os.environ]):
        print("Cannot establish diff, assuming all unit tests should be executed")
        return UNIT_TESTS
    # we get the root of the git repository here
    target_branch = 'origin/{}'.format(os.environ["CI_MERGE_REQUEST_TARGET_BRANCH_NAME"])
    this_branch = 'origin/{}'.format(os.environ["CI_COMMIT_REF_NAME"])
    # find the most recent common ancestor
    lca = subprocess.check_output(
            ['git', 'merge-base', target_branch, this_branch]).split(b'\n')[0].decode('utf-8')

    test_dependencies = {
        "conda-recipes/karabo-native": GUI,
        "conda-recipes/karabogui": GUI,
        "docs": None,
        "src/brokerMessageLogger": None,
        "src/cppLongTests": CPP_LONG,
        "src/deviceServer": KARABO,
        "src/environment.in": None,
        "src/integrationTests": INTEGRATION,
        "src/jkarabo": None,
        "src/karabo": KARABO,
        "src/karabo/devices": INTEGRATION,
        "src/pythonGui" : GUI,
        "src/pythonKarabo": PYKARABO,
        "src/pythonKarabo/karabo/": PYKARABO,
        "src/pythonKarabo/karabo/common": GUI,
        "src/pythonKarabo/karabo/integration_tests": INTEGRATION,
        "src/pythonKarabo/karabo/native": GUI,
        "src/service.in": None,
        "src/templates": None,
        "src/tools": None,
    }

    diff = (line.decode('utf-8')
            for line in subprocess.check_output(
                ['git', 'diff', '--name-only', lca, this_branch]).split(b'\n'))

    tests = set()

    for file_name in diff:
        unknown_path = True
        for path, test in test_dependencies.items():
            if test in tests:
                # minor optimization, this test will be performed no need to match
                continue
            if re.match('^{}.*$'.format(path), file_name):
                unknown_path = False
                tests.add(test)
        if unknown_path:
            # unknown path: all unit tests should be executed
            return UNIT_TESTS
    return tests

def main(args):
    if args.test_target[0] not in TESTS:
        exit(0)
    try:
        tests = check_diff()
        if args.test_target[0] not in tests:
            exit(1)
    except Exception as e:
        print(e, e.msg)
        print('Exception when inspecting diffs, returning 0')
    exit(0)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Will examine the diff and return 0 if the TARGET tests are needed, or the target is unexpected, will return 1 otherwise")
    
    parser.add_argument('test_target', metavar='TARGET', type=str, nargs=1,
                    help='one of these options: {}'.format(", ".join(TESTS)))

    args = parser.parse_args()
    main(args)
