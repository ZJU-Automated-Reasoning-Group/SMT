/**
 * Authors: Qingkai & Andy
 */

#ifndef UTILS_SMT_SMTSOLVER_H
#define UTILS_SMT_SMTSOLVER_H

#include <z3++.h>
#include <llvm/Support/raw_ostream.h>

enum SMTResult {
	UNSAT, SAT, UNKNOWN, UNCHECK
};

class SMTFactory;
class SMTModel;
class SMTExpr;
class SMTExprVec;

class SMTSolver {
private:
	z3::solver Solver;
	SMTFactory* Factory;

	SMTSolver(SMTFactory* F, z3::solver& Z3Solver);

public:
	virtual ~SMTSolver();

	SMTSolver(const SMTSolver& Solver);

	SMTSolver& operator=(const SMTSolver& Solver);

	void add(SMTExpr);

	void addAll(SMTExprVec);

	void addAll(const std::vector<SMTExpr>& EVec);

	virtual SMTResult check();

	SMTModel getSMTModel();

	SMTExprVec assertions();

	virtual void reset();

	virtual void push();

	virtual void pop(unsigned N = 1);

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
