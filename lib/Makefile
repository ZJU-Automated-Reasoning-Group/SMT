#
# Relative path to the top of the source tree.
#
LEVEL=..

#
# List all of the subdirectories that we will compile.
#
#DIRS=DSA AssistDS DSAToCallgraph Model
DIRS = $(patsubst $(PROJ_SRC_DIR)/%/Makefile,%,$(wildcard $(PROJ_SRC_DIR)/*/Makefile))

include $(LEVEL)/Makefile.common
