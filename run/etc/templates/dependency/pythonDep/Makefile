# If source files aren't under src/ folder, update the following variable:
source_folder = src

# Keep default target as make all
all:
	make clean
	make install

clean:
	rm -rf ./$(source_folder)/build
	rm -rf ./$(source_folder)/dist
	rm -rf ./$(source_folder)/*.egg-info
	rm -rf ./package

install:
	@./.install.sh

