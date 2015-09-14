/**
 * Authors: Qingkai & Andy
 */

#ifndef UTILS_SMT_SMTSOLVER_H
#define UTILS_SMT_SMTSOLVER_H

#include <z3++.h>

#include "Utils/SMT/SMTExpr.h"
#include "Utils/SMT/SMTModel.h"
#include "Debug/PPDebug.hh"

enum SMTResult {
	UNSAT, SAT, UNKNOWN
};

class SMTSolver {
private:
	z3::solver z3_solver;

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
			return SMTModel(z3_solver.get_model());
		} catch (z3::exception & e) {
			std::cerr << SFLINFO << e << "\n";
			exit(1);
		}
	}

	SMTExprVec assertions();

	void reset();

	void printUnsatCore(std::ostream& O) {
		O << z3_solver.unsat_core() << "\n";
	}

	void printUnknownReason(std::ostream& O) {
		O << z3_solver.reason_unknown() << "\n";
	}

	virtual void push();

	virtual void pop(unsigned n = 1);

	bool operator<(const SMTSolver& Solver) const;

	friend std::ostream & operator<<(std::ostream & O, SMTSolver& Solver) {
		O << Solver.z3_solver.to_smt2() << "\n";
		return O;
	}

	friend class SMTModel;
	friend class SMTFactory;
};

#endif
