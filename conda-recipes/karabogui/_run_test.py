import pytest


def run(module, generate_coverage=False):
    opts = ["-v", "--disable-warnings", "--pyargs", f"--junitxml=junit.{module}.xml", module]
    if generate_coverage:
        opts.extend([f"--cov={module}", "--cov-report", "term", "--cov-report", "xml"])
    pytest.main(opts)


def main():
    run("karabo.common")
    run("karabo.native")
    run("karabogui", generate_coverage=True)

if __name__ == "__main__":
    main()