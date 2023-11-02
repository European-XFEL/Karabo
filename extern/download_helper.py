from argparse import ArgumentParser
import os
import os.path as op
from subprocess import check_call
from tempfile import TemporaryDirectory
import yaml


def check_exists(resource_dir, package, target):
    return op.exists(f"{resource_dir}/{package}/{target}")


def download_item(tmp, package, url, format):
    check_call(["curl", url, "-L", "-o", f"{package}.{format}"], cwd=tmp)


def repackage_item(tmp, package, format, target):
    print(f"Converting {package}: {format} => {target}")
    if target.endswith(format):
        print("Formats match...")
        if f"{package}.{format}" != target:
            print("Renaming...")
            check_call(["mv", f"{package}.{format}", target], cwd=tmp)
        return
    if format == "zip" and ".tar" in target:
        print("Converting zip to tar...")

        target_dir, _ = target.rsplit(".", 1)
        if target_dir.endswith(".tar"):
            target_dir, _ = target_dir.rsplit(".", 1)

        check_call(["unzip", "-qq", f"{package}.{format}", "-d", "tmp"],
                   cwd=tmp)
        check_call(["rm", f"{package}.{format}"], cwd=tmp)
        check_call(f"mv tmp/* {target_dir}", cwd=tmp, shell=True)
        check_call(["rm", "-rf", "tmp"], cwd=tmp)
        check_call(["tar", "cf", target, "./"], cwd=tmp)

        if target.endswith(".gz"):
            print("Gzipping result...")
            check_call(f'mv {target} {target.replace(".gz", "")}',
                       cwd=tmp, shell=True)
            check_call(["gzip", target.replace(".gz", "")], cwd=tmp)
        return True

    if format.endswith(".gz"):
        print("Gunzipping result...")
        check_call(["gunzip", f"{package}.{format}"], cwd=tmp)
        if target.endswith(".zip"):
            if "tar" in format:
                print("Extracting tar...")
                check_call(["tar", "-x", f"{package}.tar"], cwd=tmp)
                check_call(["mv", package, target], cwd=tmp)
            print("Zipping result...")
            check_call(["zip", target, "./"], cwd=tmp)
        else:
            iform, _ = format.rsplit(".", 1)
            check_call(["mv", f"{package}.{iform}", target], cwd=tmp)
        return True

    print(
        f"Don't know how to convert from {format} to {target} " +
        f"for package {package}!")
    return False


def move_to_resources(tmp, package, target, resources_path):
    check_call(["mv", target, f"{resources_path}/{package}/{target}"], cwd=tmp)


def download_list(definition_file, overwrite_tmp):
    with open(definition_file, "r") as f:
        defs = yaml.safe_load(f.read())

    # if downloads.yml is empty, then quit gracefully
    if defs is None:
        return

    resources_path = f"{op.abspath(op.dirname(__file__))}/resources"
    for package, item in defs.items():
        if check_exists(resources_path, package, item["target"]):
            print(f"{package} exists, skipping...")
            continue
        print(f"Downloading {package}...")
        with TemporaryDirectory() as tmp:
            if overwrite_tmp:
                tmp = overwrite_tmp
            download_item(tmp, package, item["source"], item["source_format"])
            repack_ok = repackage_item(
                tmp, package, item["source_format"],
                item["target"])
            if repack_ok:
                os.makedirs(f"{resources_path}/{package}", exist_ok=True)
                move_to_resources(tmp, package, item["target"], resources_path)
            else:
                print("...skipped because of error!")


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("definition_file")
    parser.add_argument("--tmp", default="")
    args = parser.parse_args()
    download_list(args.definition_file, args.tmp)
