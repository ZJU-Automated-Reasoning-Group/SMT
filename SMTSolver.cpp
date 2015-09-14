/**
 * Authors: Qingkai
 */

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>

#include "SMTSolver.h"

#include <time.h>
#include <map>
#include <stack>

#define DEBUG_TYPE "smt-solver"

static llvm::cl::opt<std::string> UsingSimplify("simplify", llvm::cl::init(""),
		llvm::cl::desc("Using online simplification technique. Candidates are local and dillig."));

SMTSolver::SMTSolver(z3::solver s) :
		z3_solver(s) {
}

SMTSolver::~SMTSolver() {
}

SMTSolver::SMTSolver(const SMTSolver& Solver) :
		z3_solver(Solver.z3_solver) {
}

SMTSolver& SMTSolver::operator=(const SMTSolver& Solver) {
	if (this != &Solver) {
		this->z3_solver = Solver.z3_solver;
	}
	return *this;
}

SMTResult SMTSolver::check(SMTExprVec* Assumptions) {
	std::clock_t start;

	z3::check_result res;
	try {
		DEBUG(std::cerr << "\nStart solving!\n"; start = std::clock());

		if (UsingSimplify.getNumOccurrences()) {
			std::clock_t start;
			DEBUG(std::cerr << "Start simplifying!\n"; start = std::clock());

			z3::solver solver4sim(z3_solver.ctx());

			// 1. merge as one
			SMTExpr Whole = this->assertions().toAndExpr();

			// 2. simplify
			if (UsingSimplify.getValue() == "local") {
				// NOTE: do not call add(Whole.localSimplify())
				solver4sim.add(Whole.localSimplify().z3_expr);
			} else if (UsingSimplify.getValue() == "dillig") {
				SMTExpr SimplifiedForm = Whole.dilligSimplify();
				// if (SimplifiedForm.equals(z3_solver.ctx().bool_val(false))) {
				//		return SMTResult::UNSAT;
				// } else {
				//		if (debug_bug_report) {
				//			// check so that get_model is valid
				//			z3_solver.check();
				//		}
				//		return SMTResult::SAT;
				// }
				solver4sim.add(SimplifiedForm.z3_expr.simplify());
			} else {
				solver4sim.add(Whole.z3_expr);
			}

			DEBUG(std::cerr << "Simplifying Done: (" << (double)(std::clock() - start) * 1000 / CLOCKS_PER_SEC << ")\n");

			res = solver4sim.check();
		} else if (Assumptions == nullptr || Assumptions->size() == 0) {
			res = z3_solver.check();
		} else {
			res = z3_solver.check(Assumptions->z3_expr_vec);
		}
		DEBUG(
				std::cerr << "Solving done: (" << (double)(std::clock() - start) * 1000 / CLOCKS_PER_SEC << ", " << res << ")\n");
	} catch (z3::exception & e) {
		std::cerr << __FILE__ << " : " << __LINE__ << " : " << e << "\n";
		// std::cerr << z3_solver.to_smt2() << "\n";
		return SMTResult::UNKNOWN;
	}

	// Use a return value to suppress gcc warning
	SMTResult ret_val = SMTResult::UNKNOWN;

	switch (res) {
	case z3::check_result::sat:
		ret_val = SMTResult::SAT;
		break;
	case z3::check_result::unsat:
		ret_val = SMTResult::UNSAT;
		break;
	case z3::check_result::unknown:
		ret_val = SMTResult::UNKNOWN;
		break;
	}

	return ret_val;
}

void SMTSolver::push() {
	try {
		z3_solver.push();
	} catch (z3::exception & e) {
                std::cerr << __FILE__ << " : " << __LINE__ << " : " << e << "\n";
		exit(1);
	}
}

void SMTSolver::pop(unsigned N) {
	try {
		z3_solver.pop(N);
	} catch (z3::exception & e) {
		std::cerr << __FILE__ << " : " << __LINE__ << " : " << e << "\n";
		exit(1);
	}
}

void SMTSolver::add(SMTExpr e) {
	if (e.equals(z3_solver.ctx().bool_val(true))) {
		return;
	}

	try {
		// FIXME In some cases (ar._bfd_elf_parse_eh_frame.bc),
		// simplify() will seriously affect the performance.
		z3_solver.add(e.z3_expr/*.simplify()*/);
	} catch (z3::exception & e) {
                std::cerr << __FILE__ << " : " << __LINE__ << " : " << e << "\n";
		exit(1);
	}
}

SMTExprVec SMTSolver::assertions() {
	z3::expr_vector vec = z3_solver.assertions();
	return SMTExprVec(vec);
}

void SMTSolver::reset() {
	z3_solver.reset();
}

bool SMTSolver::operator<(const SMTSolver& Solver) const {
	return ((Z3_solver) this->z3_solver) < ((Z3_solver) Solver.z3_solver);
}
