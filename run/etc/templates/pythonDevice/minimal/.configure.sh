
echo
echo
echo "Creating an empty documentation project for __PACKAGE_NAME__"
echo

# Set up the documentation
mkdir doc
pushd doc

sphinx-quickstart -q --sep -p "__PACKAGE_NAME__" -a "__EMAIL__" -v 0.1.0 .

popd
