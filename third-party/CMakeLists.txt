ADD_CUSTOM_COMMAND(
        OUTPUT z3
        COMMAND "cd z3"
        COMMAND "python2.7 scripts/mk_make.py --staticlib"
        COMMAND "make -C ${PROJECT_BINARY_DIR}/third-party/z3/build/"
        COMMAND "cp ${PROJECT_BINARY_DIR}/third-party/z3/build/${z3} ${PROJECT_BINARY_DIR}/lib/${z3}"
)
