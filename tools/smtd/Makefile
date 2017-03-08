LEVEL = ../..
TOOLNAME = smtd

# LLVM libraries that we used
LINK_COMPONENTS = option

# Now you can add new static libraries in arbirary order in the following list
USEDLIBS = SMT.a SMTSupport.a

include $(LEVEL)/Makefile.common

# This is a hack. 
# It's actually smtd depends on z3, but I don't have a way to tell Makefile compile z3 first.
${LibDir}/libSMT.a :: z3

LIBS += $(Z3LinkOpt)
