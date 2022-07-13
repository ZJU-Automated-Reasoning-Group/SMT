#include <llvm/Support/CommandLine.h>
#include "z3++.h"
#include "SMT/SMTConfigure.h"


static llvm::cl::opt<std::string> IncTactic("set-inc-tactic", llvm::cl::init("pp_qfbv_light_tactic"),
     llvm::cl::desc("Set the tactic for creating the incremental solver. Candidates are smt_tactic, qfbv_tactic, pp_qfbv_tactic, pp_inc_bv_solver and pp_qfbv_light_tactic. Default: pp_qfbv_tactic"));


static llvm::cl::opt<std::string> UsingSMTLIBSolver("use-smtlib-solver",
		llvm::cl::init(""),
		llvm::cl::desc("Use SMTLIB2 solver to solve the query."));



/*
 * TODO: 1. If we pre-build (or pre-download) the third-party solvers in SMT/third-party, then the paths can be hard-coded.
 *          (similar to ESBMC?)
 *       2. Or, should be allow the user to specify the bin paths of the solver explicitly?
 */

const std::string z3_path = "";
const std::vector<std::string> z3_args = { "-in", "-t:5000" };

const std::string cvc5_path = "";
const std::vector<std::string> cvc5_args =
		{ "--lang=smt2", "--tlimit-per=5000", "--incremental"};

const std::string yices2_path = "";
const std::vector<std::string> yices2_args = { "--timeout=5", "--incremental"};

const std::string btor_path = "";
const std::vector<std::string> btor_args = { "--time=5", "--incremental" };


// this function should only be called once
// as in Singeleton pattern
SMTConfig::SMTConfig() {
	 Tactic = IncTactic.getValue();

	if (Tactic == "pp_qfbv_light_tactic")
		z3::set_param("inc_qfbv", 4);
	else if (Tactic == "pp_qfbv_tactic")
		z3::set_param("inc_qfbv", 2);
	else if (Tactic == "smt_tactic")
		z3::set_param("inc_qfbv", 0);
	else if (Tactic == "pp_inc_bv_solver")
		z3::set_param("inc_qfbv", 3);
	else if (Tactic == "qfbv_tactic")
		z3::set_param("inc_qfbv", 1);
	else
		z3::set_param("inc_qfbv", 4); // Default changes to pp_qfbv_light_tactic

	if (UsingSMTLIBSolver.getNumOccurrences()) {
		UseSMTLIBSolver = true;
		std::string &SolverName = UsingSMTLIBSolver.getValue();
		if (SolverName == "z3") {
			SMTLIBSolverPath = z3_path;
			SMTLIBSolverArgs = z3_args;
		} else if (SolverName == "cvc5") {
			SMTLIBSolverPath = cvc5_path;
			SMTLIBSolverArgs = cvc5_args;
		} else if (SolverName == "btor") {
			SMTLIBSolverPath = btor_path;
			SMTLIBSolverArgs = btor_args;
		} else if (SolverName == "yices2") {
			SMTLIBSolverPath = yices2_path;
			SMTLIBSolverArgs = yices2_args;
		} else {
			SMTLIBSolverPath = z3_path;
			SMTLIBSolverArgs = z3_args;
		}
	} else {
		UseSMTLIBSolver = false;
	}
}

