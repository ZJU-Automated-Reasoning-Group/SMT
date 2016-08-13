/**
 * Authors: Qingkai
 */

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>

#include "SMTSolver.h"
#include "SMTFactory.h"
#include "SMTExpr.h"
#include "SMTModel.h"

#include <time.h>
#include <map>
#include <stack>
#include <iostream>
#include <fstream>

#define DEBUG_TYPE "smt-solver"

static llvm::cl::opt<std::string> UsingSimplify("simplify", llvm::cl::init(""),
		llvm::cl::desc("Using online simplification technique. Candidates are local and dillig."));

static llvm::cl::opt<std::string> DumpingConstraintsDst("dump-cnts-dst", llvm::cl::init(""),
		llvm::cl::desc("If solving time is larger than the time that -dump-cnts-timeout, the constraints will be output the destination."));

static llvm::cl::opt<int> DumpingConstraintsTimeout("dump-cnts-timeout",
		llvm::cl::desc("If solving time is too large (ms), the constraints will be output to the destination that -dump-cnts-dst set."));

static llvm::cl::opt<int> SolverTimeOut("solver-time-out", llvm::cl::init(-1), llvm::cl::desc("Set the timeout (ms) of the smt solver."));

// only for debugging (single-thread)
bool SMTSolvingTimeOut = false;

SMTSolver::SMTSolver(SMTFactory* F, z3::solver& Z3Solver) : SMTObject(F),
		Solver(Z3Solver) {
	z3::params Z3Params(Z3Solver.ctx());
	if (SolverTimeOut.getValue() > 0) {
		// the unit is ms
		Z3Params.set("timeout", (unsigned) SolverTimeOut.getValue());
	}
	Z3Solver.set(Z3Params);
}

SMTSolver::~SMTSolver() {
}

SMTSolver::SMTSolver(const SMTSolver& Solver) : SMTObject(Solver),
		Solver(Solver.Solver) {
}

SMTSolver& SMTSolver::operator=(const SMTSolver& Solver) {
	SMTObject::operator =(Solver);
	if (this != &Solver) {
		this->Solver = Solver.Solver;
	}
	return *this;
}

SMTSolver::SMTResultType SMTSolver::check() {
	z3::check_result Result;
	try {
		clock_t Start;
		DEBUG(
				std::cerr << "\nStart solving! Constraint Size: " << assertions().constraintSize() << "/" << assertions().size() << "\n"; Start = clock());
		if (DumpingConstraintsTimeout.getNumOccurrences()) {
			Start = clock();
		}

		if (UsingSimplify.getNumOccurrences()) {
			z3::solver Z3Solver4Sim(Solver.ctx());

			// 1. merge as one
			SMTExpr Whole = this->assertions().toAndExpr();

			// 2. simplify
			if (UsingSimplify.getValue() == "local") {
				Z3Solver4Sim.add(Whole.localSimplify().Expr);
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
				Z3Solver4Sim.add(SimplifiedForm.Expr);
			} else {
				Z3Solver4Sim.add(Whole.Expr);
			}

			DEBUG(std::cerr << "Simplifying Done: (" << (double)(clock() - Start) * 1000 / CLOCKS_PER_SEC << ")\n");

			Result = Z3Solver4Sim.check();
		} else {
			Result = Solver.check();
		}

		if (DumpingConstraintsTimeout.getNumOccurrences()) {
			double TimeCost = (double) (clock() - Start) * 1000 / CLOCKS_PER_SEC;
			if (TimeCost > DumpingConstraintsTimeout.getValue() && DumpingConstraintsDst.getNumOccurrences()) {
				// output the constraints to a temp file in the dst
				std::string DstFileName = DumpingConstraintsDst.getValue();
				DstFileName.append("/case");
				DstFileName.append(std::to_string(clock()));
				DstFileName.append(".smt2");

				std::ofstream DstFile;
				DstFile.open(DstFileName);

				if (DstFile.is_open()) {
					DstFile << *this << "\n";
					DstFile.close();
				} else {
					std::cerr << "File cannot be opened: " << DstFileName << "\n";
				}

				SMTSolvingTimeOut = true;
			} else {
				SMTSolvingTimeOut = false;
			}
			DEBUG(std::cerr << "Solving done: (" << TimeCost << ", " << Result << ")\n");
		} else {
			DEBUG(std::cerr << "Solving done: (" << (double)(clock() - Start) * 1000 / CLOCKS_PER_SEC << ", " << Result << ")\n");
		}
	} catch (z3::exception &Ex) {
		std::cerr << __FILE__ << " : " << __LINE__ << " : " << Ex << "\n";
		return SMTResultType::SMTRT_Unknown;
	}

	// Use a return value to suppress gcc warning
	SMTResultType RetVal = SMTResultType::SMTRT_Unknown;

	switch (Result) {
	case z3::check_result::sat:
		RetVal = SMTResultType::SMTRT_Sat;
		break;
	case z3::check_result::unsat:
		RetVal = SMTResultType::SMTRT_Unsat;
		break;
	case z3::check_result::unknown:
		RetVal = SMTResultType::SMTRT_Unknown;
		break;
	}

	return RetVal;
}

void SMTSolver::push() {
	try {
		Solver.push();
	} catch (z3::exception &Ex) {
		std::cerr << __FILE__ << " : " << __LINE__ << " : " << Ex << "\n";
		exit(1);
	}
}

void SMTSolver::pop(unsigned N) {
	try {
		Solver.pop(N);
	} catch (z3::exception &Ex) {
		std::cerr << __FILE__ << " : " << __LINE__ << " : " << Ex << "\n";
		exit(1);
	}
}

unsigned SMTSolver::getNumScopes() {
	return Z3_solver_get_num_scopes(Solver.ctx(), Solver);
}

void SMTSolver::add(SMTExpr E) {
	if (E.isTrue()) {
		return;
	}

	try {
		// FIXME In some cases (ar._bfd_elf_parse_eh_frame.bc),
		// simplify() will seriously affect the performance.
		Solver.add(E.Expr/*.simplify()*/);
	} catch (z3::exception &Ex) {
		std::cerr << __FILE__ << " : " << __LINE__ << " : " << Ex << "\n";
		exit(1);
	}
}

void SMTSolver::addAll(SMTExprVec EVec) {
	for (unsigned I = 0; I < EVec.size(); I++) {
		add(EVec[I]);
	}
}

void SMTSolver::addAll(const std::vector<SMTExpr>& EVec) {
	for (unsigned I = 0; I < EVec.size(); I++) {
		add(EVec[I]);
	}
}

SMTExprVec SMTSolver::assertions() {
	std::shared_ptr<z3::expr_vector> Vec = std::make_shared<z3::expr_vector>(Solver.assertions());
	return SMTExprVec(&getSMTFactory(), Vec);
}

void SMTSolver::reset() {
	Solver.reset();
}

bool SMTSolver::operator<(const SMTSolver& Solver) const {
	return ((Z3_solver) this->Solver) < ((Z3_solver) Solver.Solver);
}

SMTModel SMTSolver::getSMTModel() {
	try {
		return SMTModel(&getSMTFactory(), Solver.get_model());
	} catch (z3::exception & e) {
		std::cerr << __FILE__ << " : " << __LINE__ << " : " << e << "\n";
		exit(1);
	}
}
