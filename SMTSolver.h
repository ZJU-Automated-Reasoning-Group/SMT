/**
 * Authors: Qingkai & Andy
 */

#ifndef UTILS_SMT_SMTSOLVER_H
#define UTILS_SMT_SMTSOLVER_H

#include <z3++.h>
#include <llvm/Support/raw_ostream.h>

#include "SMTExpr.h"
#include "SMTModel.h"

enum SMTResult {
	UNSAT, SAT, UNKNOWN
};

class SMTSolver {
private:
	z3::solver Solver;

	SMTSolver(z3::solver s);

public:
	virtual ~SMTSolver();

	SMTSolver(const SMTSolver& Solver);

	SMTSolver& operator=(const SMTSolver& Solver);

	void add(SMTExpr e);

	void addAll(SMTExprVec es) {
		for (unsigned i = 0; i < es.size(); i++) {
			add(es[i]);
		}
	}

	SMTResult check(SMTExprVec* Assumptions = nullptr);

	SMTModel getSMTModel() {
		try {
			return SMTModel(Solver.get_model());
		} catch (z3::exception & e) {
			std::cerr << __FILE__ << " : " << __LINE__ << " : " << e << "\n";
			exit(1);
		}
	}

	SMTExprVec assertions();

	void reset();

	void printUnsatCore(std::ostream& O) {
		O << Solver.unsat_core() << "\n";
	}

	void printUnknownReason(std::ostream& O) {
		O << Solver.reason_unknown() << "\n";
	}

	virtual void push();

	virtual void pop(unsigned n = 1);

	bool operator<(const SMTSolver& Solver) const;

	friend std::ostream & operator<<(std::ostream & O, SMTSolver& Solver) {
		O << Solver.Solver.to_smt2() << "\n";
		return O;
	}

	friend llvm::raw_ostream & operator<<(llvm::raw_ostream & O, SMTSolver& Solver) {
		O << Solver.Solver.to_smt2() << "\n";
		return O;
	}

	friend class SMTModel;
	friend class SMTFactory;
};

#endif
