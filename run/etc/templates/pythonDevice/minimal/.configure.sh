
echo
echo
echo "Creating an empty documentation project for __PACKAGE_NAME__"
echo

# Set up the documentation
mkdir doc
pushd doc

sphinx-quickstart -q --sep --no-batchfile -p "__PACKAGE_NAME__" -a "__EMAIL__" -v 0.1.0 .

# Replace the theme with one of our choosing
# In sphinx 1.4+, this can be done easily with the -D option...
sed -i "s%^html_theme\ \=\ '\([a-zA-Z]*\)'%html_theme\ \=\ 'default'%" source/conf.py

popd
