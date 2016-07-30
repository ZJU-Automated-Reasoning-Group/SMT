/**
 * Authors: Qingkai & Andy
 */

#ifndef SMT_SMTSOLVER_H
#define SMT_SMTSOLVER_H

#include <z3++.h>
#include <llvm/Support/raw_ostream.h>

class SMTFactory;
class SMTModel;
class SMTExpr;
class SMTExprVec;

class SMTSolver {
public:
	enum SMTResultType {
		SMTRT_Unsat,
		SMTRT_Sat,
		SMTRT_Unknown,
		SMTRT_Uncheck
	};
	
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

	virtual SMTResultType check();

	SMTModel getSMTModel();

	SMTExprVec assertions();

	virtual void reset();

	virtual void push();

	virtual void pop(unsigned N = 1);

	unsigned getNumScopes();

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
