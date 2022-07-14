/**
 * Authors: rainoftime
 */

#ifndef SMT_SMTCONFIGURE_H
#define SMT_SMTCONFIGURE_H

#include <vector>
#include <string>

class SMTConfig {
public:

    // static int Timeout;
    static std::string Tactic;

    // For using SMTLIBSolver
    static bool UseSMTLIBSolver;
    static bool UseIncrementalSMTLIBSolver;
    static std::string SMTLIBSolverPath;
    static std::vector<std::string> SMTLIBSolverArgs;
    // End

public:
    static void init();
};



#endif
