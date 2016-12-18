/**
 * Authors: Qingkai & Andy
 */

#ifndef SMT_SMTSOLVER_H
#define SMT_SMTSOLVER_H

#include <vector>
#include <llvm/Support/Debug.h>

#include "z3++.h"
#include "SMTObject.h"
#include "Support/MessageQueue.h"

class SMTFactory;
class SMTModel;
class SMTExpr;
class SMTExprVec;

class SMTSolver : public SMTObject {
public:
	enum SMTResultType {
		SMTRT_Unsat,
		SMTRT_Sat,
		SMTRT_Unknown,
		SMTRT_Uncheck
	};
	
private:
	z3::solver Solver;

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

private:
	/// The followings are used when smtd is enabled
	/// @{
	/// This field is to pass command to smtd's master
	MessageQueue* CommandMSQ = nullptr;
	/// This field is for other communication with smtd's master
	MessageQueue* CommunicateMSQ = nullptr;
	/// This field is for communication with one of the smtd's slaves
	MessageQueue* WorkerMSQ = nullptr;

	/// reconnect to smtd
	void reconnect();
	/// @}
};

#endif
