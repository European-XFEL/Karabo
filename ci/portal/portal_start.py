import os
import requests


def main():
    """Start the test suite of the karabo test portal"""
    headers = {"Content-type": "application/json"}

    tag = os.environ.get("CI_COMMIT_TAG")
    project = os.environ.get("CI_PROJECT_PATH")
    email = os.environ.get("TEST_NOTIFICATION_EMAIL")
    if not email:
        print("No notification email provided, aborting ...")
        return
    destination = os.environ.get("TEST_PORTAL")
    if not destination:
        print("No test portal destination provided, aborting ...")
        return

    d = {"tests": ["Distributed/200/01", "*/01/01", "Laptop/100/01"],
         "user": email,
         "test_variables": {"karabo_version": tag},
         "project": project}

    request = requests.post(destination, json=d, headers=headers, verify=False)
    print("Portal Request", request)


if __name__ == "__main__":
    main()
