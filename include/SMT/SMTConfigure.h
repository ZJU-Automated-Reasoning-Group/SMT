/**
 * Authors: rainoftime
 */

#ifndef SMT_SMTCONFIGURE_H
#define SMT_SMTCONFIGURE_H

#include <vector>
#include <string>

class SMTConfig {
public:
    static SMTConfig& get() {
        static SMTConfig Instance;
        return Instance;
    }

    std::string& getIncTactic() {
    	return Tactic;
    }

    // For using SMTLIBSolver
    bool UseSMTLIBSolver;
    std::string SMTLIBSolverPath;
    std::vector<std::string> SMTLIBSolverArgs;
    // End


private:
    SMTConfig(); // initialize the members once

    std::string Tactic;

    // static int Timeout;



};

#endif
