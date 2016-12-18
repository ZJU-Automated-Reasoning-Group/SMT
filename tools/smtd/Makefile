LEVEL = ../..
TOOLNAME = smtd

# LLVM libraries that we used
LINK_COMPONENTS = option

# The LLVM options TOOLLINKOPTS and TOOLLINKOPTSB are the secret sauce for adding linking options before/after linked libraries 

# For linux we need the —start-group commands
# For OS X we don’t need them. 
UNAME_S := $(shell uname -s)
PPHOST_OS =
ifeq ($(UNAME_S),Linux)
    PPHOST_OS = LINUX
endif
ifeq ($(UNAME_S),Darwin)
    PPHOST_OS = OSX
endif

ifeq ($(PPHOST_OS),OSX)

else
	TOOLLINKOPTS = -Wl,--start-group
	TOOLLINKOPTSB = -Wl,--end-group
endif

# Now you can add new static libraries in arbirary order in the following list
USEDLIBS = SMT.a SMTSupport.a z3.a

# Include this here so we can get the configuration of the targets
# that have been configured for construction. We have to do this 
# early so we can set up USEDLIBS properly before includeing Makefile.rules
include $(LEVEL)/Makefile.common

LIBS += -fopenmp

${LibDir}/libz3.a : z3
