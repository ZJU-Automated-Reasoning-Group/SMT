/**
 * Authors: Qingkai
 */

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>

#include "SMT/SMTSolver.h"
#include "SMT/SMTFactory.h"
#include "SMT/SMTExpr.h"
#include "SMT/SMTModel.h"

#include "SMT/SMTLIBSolver.h"
#include "SMT/SMTConfigure.h"
// #include "SMT/PushPopUtil.h"

#include "Support/MessageQueue.h"

#include <ctime>
#include <map>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <sstream>

#define DEBUG_TYPE "solver"

using namespace llvm;

static llvm::cl::opt<std::string> UsingSimplify("solver-simplify", llvm::cl::init(""),
        llvm::cl::desc("Using online simplification technique. Candidates are local and dillig."));

static llvm::cl::opt<std::string> DumpingConstraintsDst("dump-cnts-dst", llvm::cl::init(""),
        llvm::cl::desc("If solving time is larger than the time that -dump-cnts-timeout, the constraints will be output the destination."));

static llvm::cl::opt<int> DumpingConstraintsTimeout("dump-cnts-timeout",
        llvm::cl::desc("If solving time is too large (ms), the constraints will be output to the destination that -dump-cnts-dst set."));

static llvm::cl::opt<int> SolverTimeOut("solver-timeout", llvm::cl::init(10000), llvm::cl::desc("Set the timeout (ms) of the smt solver. The default value is 10000ms (i.e. 10s)."));

static llvm::cl::opt<int> EnableSMTD("solver-enable-smtd", llvm::cl::init(0), llvm::cl::ValueRequired,
        llvm::cl::desc("Using smtd service"));

static llvm::cl::opt<bool> EnableSMTDIncremental("solver-enable-smtd-incremental", llvm::cl::init(false),
        llvm::cl::desc("Using incremental when smtd is enabled"));

static llvm::cl::opt<bool> EnableLocalSimplify("enable-local-simplify", llvm::cl::init(true),
                                               llvm::cl::desc("Enable local simplifications while adding a vector of constraints"));



// for generating SMT string queries
template <typename T>
std::string join(const T& v) {
    std::ostringstream s;
    for (const auto& i : v) {
        s << i;
    }
    return s.str();
}

// only for debugging (single-thread)
bool SMTSolvingTimeOut = false;

SMTSolver::SMTSolver(SMTFactory* F, z3::solver& Z3Solver) : SMTObject(F),
        Solver(Z3Solver) {


    if (SolverTimeOut.getValue() > 0) {
        z3::params Z3Params(Z3Solver.ctx());
        Z3Params.set("timeout", (unsigned) SolverTimeOut.getValue());
        Z3Solver.set(Z3Params);
    }

    if (EnableSMTD.getNumOccurrences()) {
        Channels = std::make_shared<SMTDMessageQueues>();

        int MasterKey = EnableSMTD.getValue();
        Channels->CommandMSQ = std::make_shared<MessageQueue>(MasterKey);
        Channels->CommunicateMSQ = std::make_shared<MessageQueue>(++MasterKey);

        // Step 0: send id-request
        DEBUG_WITH_TYPE("solver-smtd", errs() << "[Client] Try to connect to master.\n");
        if (-1 == Channels->CommandMSQ->sendMessage("requestid")) {
            llvm_unreachable("Fail to send open command!");
        }
        std::string UserIDStr;
        if (-1 == Channels->CommunicateMSQ->recvMessage(UserIDStr, 12)) {
            llvm_unreachable("Fail to recv user id!");
        }
        Channels->UserID = std::stol(UserIDStr);
        DEBUG_WITH_TYPE("solver-smtd", errs() << "[Client] Connect to master and get user id: " << UserIDStr << ".\n");

        // Step 1: send open-request
        if (-1 == Channels->CommandMSQ->sendMessage(UserIDStr + ":open")) {
            llvm_unreachable("Fail to send open command!");
        }
        DEBUG_WITH_TYPE("solver-smtd", errs() << "[Client] Request sended: " << UserIDStr << ":open\n");

        // Step 2: wait for worker info
        std::string SlaveIDStr;
        if (-1 == Channels->CommunicateMSQ->recvMessage(SlaveIDStr, Channels->UserID)) {
            llvm_unreachable("Fail to recv worker id!");
        }
        int SlaveID = std::stoi(SlaveIDStr);
        DEBUG_WITH_TYPE("solver-smtd", errs() << "[Client] Receive worker Id: " << SlaveIDStr << "\n");

        // Step 3: confirmation
        if (-1 == Channels->CommunicateMSQ->sendMessage(std::to_string(Channels->UserID) + ":got", 11)) {
            llvm_unreachable("Fail to send got command!");
        }
        DEBUG_WITH_TYPE("solver-smtd", errs() << "[Client] Confirmation sended\n");

        // Step 4: connect to server
        Channels->WorkerMSQ = std::make_shared<MessageQueue>(SlaveID);
        DEBUG_WITH_TYPE("solver-smtd", errs() << "[Client] Connect to Slave\n");
    }

    // For communicating with SMTLIB solvers
    if (SMTConfig::UseIncrementalSMTLIBSolver) {
        SmtlibSolver = createSMTLIBSolver();
        if (SmtlibSolver == NULL) {
            std::cout << "Creating SMTLIB solver failure!!!\n";
        }
        // SmtlibSolver->setLogic("QF_BV");
    }
}

SMTSolver::SMTSolver(const SMTSolver& Solver) : SMTObject(Solver),
        Solver(Solver.Solver), Channels(Solver.Channels) {

    if (SMTConfig::UseIncrementalSMTLIBSolver) {
        SmtlibSolver = Solver.SmtlibSolver; // TODO: is this correct? 
        //SmtlibSolver = createSMTLIBSolver();
        //AssertionsCache = Solver.AssertionsCache; // correct?     
    }
}

SMTSolver& SMTSolver::operator=(const SMTSolver& Solver) {
    SMTObject::operator =(Solver);
    if (this != &Solver) {
        this->Solver = Solver.Solver;
        this->Channels = Solver.Channels;
    }

    if (SMTConfig::UseIncrementalSMTLIBSolver) {
        SmtlibSolver = Solver.SmtlibSolver; // TODO: is this correct?
        //SmtlibSolver = createSMTLIBSolver(); 
        //AssertionsCache = Solver.AssertionsCache; // correct?     
    }

    return *this;
}

SMTSolver::~SMTSolver() {
    //if (SMTConfig::UseIncrementalSMTLIBSolver) {
    //  if (SmtlibSolver && SmtlibSolver->processIdOfSolver == 0) { delete SmtlibSolver; }
    //}
}

SMTSolver::SMTDMessageQueues::~SMTDMessageQueues() {
    assert(CommandMSQ.get() && "CommandMSQ is not initialized in a channel!");
    CommandMSQ->sendMessage(std::to_string(UserID) + ":close");
}

void SMTSolver::reconnect() {
    assert(EnableSMTD.getNumOccurrences() && "reconnect can be used only if --solver-enable-smtd is opened!");

    if (-1 == Channels->CommandMSQ->sendMessage(std::to_string(Channels->UserID) + ":reopen")) {
        llvm_unreachable("Fail to send open command!");
    }
    DEBUG_WITH_TYPE("solver-smtd", errs() << "[Client] Request sended: " << Channels->UserID << ":open\n");
    std::string SlaveIdStr;
    if (-1 == Channels->CommunicateMSQ->recvMessage(SlaveIdStr, Channels->UserID)) {
        llvm_unreachable("Fail to recv worker id!");
    }
    DEBUG_WITH_TYPE("solver-smtd", errs() << "[Client] Receive Slave Id: " << SlaveIdStr << "\n");
    if (-1 == Channels->CommunicateMSQ->sendMessage(std::to_string(Channels->UserID) + ":got", 11)) {
        llvm_unreachable("Fail to send got command!");
    }
    DEBUG_WITH_TYPE("solver-smtd", errs() << "[Client] Confirmation sended\n");
    Channels->WorkerMSQ = std::make_shared<MessageQueue>(std::stoi(SlaveIdStr));
    DEBUG_WITH_TYPE("solver-smtd", errs() << "[Client] Connect to Slave\n");
}

SMTSolver::SMTResultType SMTSolver::check() {
    if (SMTConfig::UseSMTLIBSolver) {
        if (SMTConfig::UseIncrementalSMTLIBSolver) {
            // FIXME: enable "true incrmental"
            // NOTE: AssertionsCache is not used currently 
            // std::string Query = this->Solver.decls_to_string() + join(AssertionsCache.getCacheVector()) +  + "(check-sat)\n";
            std::string Query = "(set-logic QF_BV)\n" + this->Solver.to_smt2(); // slow
            this->SmtlibSolver->reset(); // clear last query?
            auto Result = SmtlibSolver->solveWholeFormula(Query);
            if (Result == SMTLIBSolverResult::SMTRT_Sat) {
                return SMTSolver::SMTResultType::SMTRT_Sat;
            } else if (Result == SMTLIBSolverResult::SMTRT_Unsat) {
                return SMTSolver::SMTResultType::SMTRT_Unsat;
            } else {
                return SMTSolver::SMTResultType::SMTRT_Unknown;
            }
        } else {
            std::string Query = "(set-logic QF_BV)\n" + this->Solver.to_smt2(); // slow
            auto dump_end = std::chrono::high_resolution_clock::now();

            SmtlibSmtSolver* BinSolver = new SmtlibSmtSolver(SMTConfig::SMTLIBSolverPath, SMTConfig::SMTLIBSolverArgs);
            // BinSolver->setLogic("QF_BV");
            auto Result = BinSolver->solveWholeFormula(Query);
            delete BinSolver;

            if (Result == SMTLIBSolverResult::SMTRT_Sat) {
                return SMTSolver::SMTResultType::SMTRT_Sat;
            } else if (Result == SMTLIBSolverResult::SMTRT_Unsat) {
                return SMTSolver::SMTResultType::SMTRT_Unsat;
            } else {
                return SMTSolver::SMTResultType::SMTRT_Unknown;
            }
        } 
    }

    if (EnableSMTD.getNumOccurrences()) {
        std::string Contraints;
        llvm::raw_string_ostream StringStream(Contraints);
        StringStream << *((SMTSolver*)this);

        // fault tolerance
        while (-1 == Channels->WorkerMSQ->sendMessage(StringStream.str(), 1)) {
            reconnect();
        }
        std::string ResultString;
        while (-1 == Channels->WorkerMSQ->recvMessage(ResultString, 2)) {
            reconnect();
            while (-1 == Channels->WorkerMSQ->sendMessage(StringStream.str(), 1)) {
                reconnect();
            }
        }

        SMTResultType Result = (SMTResultType) std::stoi(ResultString);
        return Result;
    }

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
        // std::cout << Solver.reason_unknown() << "\n";
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
        //if (SMTConfig::UseIncrementalSMTLIBSolver) {
            // explicitly maintain the assertion stacks
        //    AssertionsCache.pop(N);
        //}
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

    	//if (SMTConfig::UseIncrementalSMTLIBSolver) {
        //    std::string Cnt = "(assert " + E.Expr.to_string() + ")";
        //    AssertionsCache.add(Cnt + "\n");
    	//}
    } catch (z3::exception &Ex) {
        std::cerr << __FILE__ << " : " << __LINE__ << " : " << Ex << "\n";
        exit(1);
    }
}


void SMTSolver::addAll(SMTExprVec EVec) {
    // TODO: more tactics
    // 1. Add one by one(the default tactic)
    // 2. Call toAnd(EVec), and add the returned formula
    // 3. Call toAnd(EVec), add add a simplified version of the returned formula
    // 4. Take the size of EVec into considerations; choose the parameters of simplify()
    if (EnableLocalSimplify.getValue()) {
        add(EVec.toAndExpr());
        // Turn EVec to a single Expr, and call simplify()
        //Solver.add(EVec.toAndExpr().Expr.simplify());
    } else {
        for (unsigned I = 0; I < EVec.size(); I++) {
            add(EVec[I]);
        }
    }
}

void SMTSolver::addAll(const std::vector<SMTExpr>& EVec) {
    // TODO: add EnableLocalSimplify option for this.
    for (unsigned I = 0; I < EVec.size(); I++) {
        add(EVec[I]);
    }
}

SMTExprVec SMTSolver::assertions() {
    std::shared_ptr<z3::expr_vector> Vec = std::make_shared<z3::expr_vector>(Solver.assertions());
    return SMTExprVec(&getSMTFactory(), Vec);
}

void SMTSolver::reset() {
    // TODO: should we send "reset" or "reset-assertions" to the SMTLIB solver
    Solver.reset();
    //if (SMTConfig::UseIncrementalSMTLIBSolver) {
    //     AssertionsCache.reset();
    //}
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
