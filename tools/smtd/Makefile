LEVEL = ../..
TOOLNAME = smtd

# LLVM libraries that we used
LINK_COMPONENTS = option

# The LLVM options TOOLLINKOPTS and TOOLLINKOPTSB are the secret sauce for adding linking options before/after linked libraries 

# For linux we need the —start-group commands
# For OS X we don’t need them. 
ifeq ($(PPHOST_OS),OSX)

else
	TOOLLINKOPTS = -Wl,--start-group
	TOOLLINKOPTSB = -Wl,--end-group
endif

# Now you can add new static libraries in arbirary order in the following list
USEDLIBS = SMT.a SMTSupport.a

# Include this here so we can get the configuration of the targets
# that have been configured for construction. We have to do this 
# early so we can set up USEDLIBS properly before includeing Makefile.rules
include $(LEVEL)/Makefile.common

LIBS += $(Z3LinkOpt)

