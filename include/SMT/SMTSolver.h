/**
 * Authors: Qingkai
 */

#ifndef SMT_SMTSOLVER_H
#define SMT_SMTSOLVER_H

#include <vector>
#include <llvm/Support/Debug.h>

#include "z3++.h"
#include "SMTObject.h"
#include "SMTLIBSolver.h"

class SMTFactory;
class SMTModel;
class SMTExpr;
class SMTExprVec;
class MessageQueue;

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

    // { Begin of SMTLIB solver related staff
	SmtlibSmtSolver *SmtlibSolver; 	// For communicating with SMTLIB solvers
	// NOTE:the following vectors are used for debugging
	// Since we will directly send all the commands to the binary SMTLIB solver
	std::vector<std::string> SMTLIBCnts = { ";\n" };
	std::vector<unsigned> SMTLIBBacktrackPoints = { };
	// } End

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
    class SMTDMessageQueues {
    public:
        /// User ID for communication
        long UserID = 0;

        /// This field is to pass command to smtd's master
        std::shared_ptr<MessageQueue> CommandMSQ;
        /// This field is for other communication with smtd's master
        std::shared_ptr<MessageQueue> CommunicateMSQ;
        /// This field is for communication with one of the smtd's slaves
        std::shared_ptr<MessageQueue> WorkerMSQ;

        ~SMTDMessageQueues();
    };

    std::shared_ptr<SMTDMessageQueues> Channels;

    /// reconnect to smtd
    void reconnect();
    /// @}
};

#endif
