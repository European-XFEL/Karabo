#
#  There exist several targets which are by default empty and which can be 
#  used for execution of your targets. These targets are usually executed 
#  before and after some main targets. They are: 
#
#     .build-pre:              called before 'build' target
#     .build-post:             called after 'build' target
#     .clean-pre:              called before 'clean' target
#     .clean-post:             called after 'clean' target
#     .clobber-pre:            called before 'clobber' target
#     .clobber-post:           called after 'clobber' target
#     .all-pre:                called before 'all' target
#     .all-post:               called after 'all' target
#     .help-pre:               called before 'help' target
#     .help-post:              called after 'help' target
#
#  Targets beginning with '.' are not intended to be called on their own.
#
#  Main targets can be executed directly, and they are:
#  
#     build                    build a specific configuration
#     clean                    remove built files from a configuration
#     clobber                  remove all built files
#     all                      build all configurations
#     help                     print help mesage
#  
#  Targets .build-impl, .clean-impl, .clobber-impl, .all-impl, and
#  .help-impl are implemented in nbproject/makefile-impl.mk.
#
#  Available make variables:
#
#     CND_BASEDIR                base directory for relative paths
#     CND_DISTDIR                default top distribution directory (build artifacts)
#     CND_BUILDDIR               default top build directory (object files, ...)
#     CONF                       name of current configuration
#     CND_PLATFORM_${CONF}       platform name (current configuration)
#     CND_ARTIFACT_DIR_${CONF}   directory of build artifact (current configuration)
#     CND_ARTIFACT_NAME_${CONF}  name of build artifact (current configuration)
#     CND_ARTIFACT_PATH_${CONF}  path to build artifact (current configuration)
#     CND_PACKAGE_DIR_${CONF}    directory of package (current configuration)
#     CND_PACKAGE_NAME_${CONF}   name of package (current configuration)
#     CND_PACKAGE_PATH_${CONF}   path to package (current configuration)
#
# NOCDDL


# Environment 
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin

# Fallback solution if KARABO_EXTERN is not set
KARABO_EXTERN ?= $(CND_BASEDIR)/../../../extern
EXTERN_RESOURCE_DIR = $(KARABO_EXTERN)/resources
DIST_DIR = $(CND_BASEDIR)/$(CND_DISTDIR)/$(CONF)/$(CND_PLATFORM_$(CONF))
EXTERN_DIST_DIR = $(DIST_DIR)/extern

# build
build: .build-post

.build-pre:
# Add your pre 'build' code here...
	@if test -d $(EXTERN_DIST_DIR);\
	then echo "### External dependencies already installed ###";\
	else $(MKDIR) -p $(EXTERN_DIST_DIR)/include;\
	$(MKDIR) -p $(EXTERN_DIST_DIR)/lib;\
	$(MKDIR) -p $(EXTERN_DIST_DIR)/bin;\
	$(EXTERN_RESOURCE_DIR)/boost/install.sh $(EXTERN_DIST_DIR);\
	$(EXTERN_RESOURCE_DIR)/openmqc/install.sh $(EXTERN_DIST_DIR);\
	$(EXTERN_RESOURCE_DIR)/hdf5/install.sh $(EXTERN_DIST_DIR);\
	$(EXTERN_RESOURCE_DIR)/snmp/install.sh $(EXTERN_DIST_DIR);\
	$(EXTERN_RESOURCE_DIR)/log4cpp/install.sh $(EXTERN_DIST_DIR);\
	fi

.build-post: .build-impl
# Add your post 'build' code here...
	@$(MKDIR) -p  $(DIST_DIR)/include/karabo/core
	@$(MKDIR) -p  $(DIST_DIR)/include/karabo/io
	@$(MKDIR) -p  $(DIST_DIR)/include/karabo/log
	@$(MKDIR) -p  $(DIST_DIR)/include/karabo/net
	@$(MKDIR) -p  $(DIST_DIR)/include/karabo/python
	@$(MKDIR) -p  $(DIST_DIR)/include/karabo/util
	@$(MKDIR) -p  $(DIST_DIR)/include/karabo/xml
	@$(MKDIR) -p  $(DIST_DIR)/include/karabo/xms
	@$(CP) $(CND_BASEDIR)/../../../src/karabo/core/*.hh $(DIST_DIR)/include/karabo/core
	@$(CP) $(CND_BASEDIR)/../../../src/karabo/io/*.hh $(DIST_DIR)/include/karabo/io
	@$(CP) $(CND_BASEDIR)/../../../src/karabo/log/*.hh $(DIST_DIR)/include/karabo/log
	@$(CP) $(CND_BASEDIR)/../../../src/karabo/net/*.hh $(DIST_DIR)/include/karabo/net
	@$(CP) $(CND_BASEDIR)/../../../src/karabo/python/*.hh $(DIST_DIR)/include/karabo/python
	@$(CP) $(CND_BASEDIR)/../../../src/karabo/util/*.hh $(DIST_DIR)/include/karabo/util
	@$(CP) $(CND_BASEDIR)/../../../src/karabo/xml/*.hh $(DIST_DIR)/include/karabo/xml
	@$(CP) $(CND_BASEDIR)/../../../src/karabo/xms/*.hh $(DIST_DIR)/include/karabo/xms

# Force extern re-build
.PHONY: extern
extern:
	$(MKDIR) -p $(EXTERN_DIST_DIR)
	$(EXTERN_RESOURCE_DIR)/boost/install.sh $(EXTERN_DIST_DIR)
	$(EXTERN_RESOURCE_DIR)/openmqc/install.sh $(EXTERN_DIST_DIR)
	$(EXTERN_RESOURCE_DIR)/hdf5/install.sh $(EXTERN_DIST_DIR)
	$(EXTERN_RESOURCE_DIR)/snmp/install.sh $(EXTERN_DIST_DIR)
	$(EXTERN_RESOURCE_DIR)/log4cpp/install.sh $(EXTERN_DIST_DIR)

# clean
clean: .clean-post

.clean-pre:
# Add your pre 'clean' code here...

.clean-post: .clean-impl
# Add your post 'clean' code here...


# clobber
clobber: .clobber-post

.clobber-pre:
# Add your pre 'clobber' code here...

.clobber-post: .clobber-impl
# Add your post 'clobber' code here...


# all
all: .all-post

.all-pre:
# Add your pre 'all' code here...

.all-post: .all-impl
# Add your post 'all' code here...


# build tests
build-tests: .build-tests-post

.build-tests-pre:
# Add your pre 'build-tests' code here...

.build-tests-post: .build-tests-impl
# Add your post 'build-tests' code here...


# run tests
test: .test-post

.test-pre:
# Add your pre 'test' code here...

.test-post: .test-impl
# Add your post 'test' code here...


# help
help: .help-post

.help-pre:
# Add your pre 'help' code here...

.help-post: .help-impl
# Add your post 'help' code here...



# include project implementation makefile
include nbproject/Makefile-impl.mk

# include project make variables
include nbproject/Makefile-variables.mk
