from conan.tools.files import copy
import os
import shutil


def deploy(graph, output_folder, **kwargs):

    conanfile = graph.root.conanfile
    conanfile.output.info(f"Conan built-in pkg deployer to {output_folder}")

    # example of using filter to only iterate direct dependencies
    # for dep in conanfile.dependencies.filter({"direct": True}).values():

    for dep in conanfile.dependencies.values():
        if dep.folders.package_folder is not None:
            copy(graph.root.conanfile, "*", dep.folders.package_folder, output_folder)
            # shutil.copytree(dep.package_folder, output_folder, symlinks=True)
            dep.set_deploy_folder(output_folder)
