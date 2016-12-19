/*
 * This tool provide SMT solving service.
 *
 * Author: Qingkai
 */


#include <llvm/Support/Signals.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/FileSystem.h>

#include <sys/wait.h> // waitpid
#include <signal.h> // kill
#include <error.h>
#include <unistd.h> // fork

#include <map>
#include <set>

#include "Support/SignalHandler.h"
#include "Support/MessageQueue.h"
#include "SMT/SMTFactory.h"

#define DEBUG_TYPE "smtd"

using namespace llvm;

static cl::opt<int> MSQKey("smtd-key", cl::desc("Indicate the key of the master's message queue."), cl::init(1234));

static cl::opt<bool> Incremental("smtd-incremental", cl::desc("Enable incremental SMT solving."), cl::init(false));

static cl::opt<bool> TestClient("smtd-test", cl::desc("Run a testing client."), cl::init(false), cl::ReallyHidden);

static void testReconnect(int UserID, MessageQueue* CommandMSQ, MessageQueue* CommunicateMSQ, MessageQueue*& WorkerMSQ) {
    if (-1 == CommandMSQ->sendMessage(std::to_string(UserID) + ":reopen")) {
        llvm_unreachable("Fail to send open command!");
    }
    DEBUG(errs() << "[Client] Request sended: " << UserID << ":open\n");
    std::string SlaveIdStr;
    if (-1 == CommunicateMSQ->recvMessage(SlaveIdStr, UserID)) {
        llvm_unreachable("Fail to recv worker id!");
    }
    DEBUG(errs() << "[Client] Receive Slave Id: " << SlaveIdStr << "\n");
    if (-1 == CommunicateMSQ->sendMessage(std::to_string(UserID) + ":got", 11)) {
        llvm_unreachable("Fail to send got command!");
    }
    DEBUG(errs() << "[Client] Confirmation sended\n");
    delete WorkerMSQ; // avoid memory leak
    WorkerMSQ = new MessageQueue(std::stoi(SlaveIdStr));
    DEBUG(errs() << "[Client] Connect to Slave\n");
}

static void test() {
    MessageQueue* MasterCommandMSQ = new MessageQueue(MSQKey.getValue());
    MessageQueue* MasterCommunicateMSQ = new MessageQueue(1 + MSQKey.getValue());

    while (true) {
        // Step 1: send command: userid:request
        int UserId;
        char Request[100];
        printf("%s", "Enter message to master: ");
        if (scanf("%d:%s", &UserId, Request) == EOF) {
            perror("Scanf error: ");
            llvm_unreachable("Scanf error!");
        }
        DEBUG(errs() << "SEND TO MASTER: " << UserId << ":" << Request << "\n");
        MasterCommandMSQ->sendMessage(std::to_string(UserId) + ":" + Request);
        DEBUG(errs() << "SEND DONE\n");

        // Step 2: get worker key
        std::string Reply;
        MasterCommunicateMSQ->recvMessage(Reply, UserId);
        DEBUG(errs() << "RECV FROM MASTER: " << Reply << "\n");

        // Step 3: confirmation
        DEBUG(errs() << "SEND TO MASTER: " << std::to_string(UserId) + ":got" << "\n");
        MasterCommunicateMSQ->sendMessage(std::to_string(UserId) + ":got", 11);
        DEBUG(errs() << "SEND DONE\n");

        // Step 4: connect to worker
        int SlaveKey = std::stoi(Reply);
        MessageQueue* SlaveMSQ = new MessageQueue(SlaveKey);

        SMTFactory Factory;
        SMTSolver Solver = Factory.createSMTSolver();
        SMTExpr A = Factory.createBitVecConst("A", 32);
        Solver.add(A > 10);

        std::string Constraints;
        raw_string_ostream Stream(Constraints);
        Stream << Solver;

        // Step 5: send constraints to worker
        DEBUG(errs() << "SEND TO SLAVE: \n" << Stream.str() << "\n");
        SlaveMSQ->sendMessage(Stream.str(), 1);
        DEBUG(errs() << "SEND DONE\n");

        // Step 6: get result
        std::string ReplyFromSlave;
        while (-1 == SlaveMSQ->recvMessage(ReplyFromSlave, 2)) {
            testReconnect(UserId, MasterCommandMSQ, MasterCommunicateMSQ, SlaveMSQ);
            SlaveMSQ->sendMessage(Stream.str(), 1);
        }
        DEBUG(errs() << "RECV FROM SLAVE: " << ReplyFromSlave << "\n");

        delete SlaveMSQ;
    }

    delete MasterCommandMSQ;
    delete MasterCommunicateMSQ;
}

static MessageQueue* SlaveMSQ = nullptr;

static MessageQueue* CommandMSQ = nullptr;

static MessageQueue* CommunicateMSQ = nullptr;

static MessageQueue* UserIDMSQ = nullptr;

static pid_t MainProcessId;

static inline long allocateUserID(std::set<long>& ExistingUsers, std::vector<long>& FreeUserIDs) {
    static long ID = 101;

    if (FreeUserIDs.empty()) {
        bool Overflow = false;
        for (; ; ++ID) {
            if (ID > 0 && !ExistingUsers.count(ID)) {
                ExistingUsers.insert(ID);
                return ID;
            } else if (ID <= 0) { // overflow
                if (!Overflow) {
                    Overflow = true;
                    ID = 100;
                } else {
                    llvm_unreachable("Too many users and no available ID can be assigned.");
                }
            }
        }
        llvm_unreachable("Too many users and no available ID can be assigned.");
        return ID;
    } else {
        long Ret = FreeUserIDs.back();
        FreeUserIDs.pop_back();

        ExistingUsers.insert(Ret);
        return Ret;
    }
}

int main(int argc, char **argv) {
    PrettyStackTraceProgram X(argc, argv);

    // Enable debug stream buffering.
    llvm::EnableDebugBuffering = true;
    llvm_shutdown_obj Y;  // Call llvm_shutdown() on exit.

    cl::ParseCommandLineOptions(argc, argv, "SMT solving service for Pinpoint.\n");

    if (TestClient.getValue()) {
        test();
        return 0;
    }

    outs() << "*******************************\n"
            << "Please run your applications with -solver-enable-smtd=" << MSQKey.getValue()
            << " -solver-enable-smtd-incremental=" << (Incremental.getValue() ? "true" : "false") << "\n"
            << "*******************************\n";

    if (Incremental.getValue()) {
        llvm_unreachable("Incremental mode has not supported yet!");
    }

    // Signal handlers
    MainProcessId = getpid();
    RegisterSignalHandler();
    std::function<void()> ExitHandler = []() {
        // close all child processes
        if (MainProcessId == getpid()) {
            CommandMSQ->destroy();
            CommunicateMSQ->destroy();
            UserIDMSQ->destroy();
            delete CommunicateMSQ;
            delete CommandMSQ;
            delete UserIDMSQ;

            while (waitpid(-1, nullptr, 0)) {
                if (errno == ECHILD) {
                    // until no children exist, avoid dead processes
                    break;
                }
            }

        } else {
            SlaveMSQ->destroy();
            delete SlaveMSQ;
            DEBUG(errs() << "\nSignal handler deletes slave " << SlaveMSQ << "\n");
        }
        CommunicateMSQ = nullptr;
        CommandMSQ = nullptr;
        UserIDMSQ = nullptr;
        SlaveMSQ = nullptr;
    };
    AddInterruptSigHandler(ExitHandler);
    AddErrorSigHandler(ExitHandler);

    std::set<long> ExistingUsers;
    std::vector<long> FreeUserIDs;
    std::vector<std::pair<pid_t, int>> FreeMSQs;
    std::map<long, std::pair<pid_t, int>> UserWorkerMap;

    int Counter = 0;

    CommandMSQ = new MessageQueue(MSQKey.getValue(), true);
    CommunicateMSQ = new MessageQueue(MSQKey.getValue() + ++Counter, true);
    UserIDMSQ = new MessageQueue(MSQKey.getValue() + ++Counter, true);

    std::string Command, CtrlMsg;
    while (true) {
        if (-1 == CommandMSQ->recvMessage(Command)) {
            llvm_unreachable("[Master] fail to recv command");
        }
        DEBUG(errs() << "[Master] get msg (0): " << Command << "\n";);

        long UserId = -1;
        try {
            if (Command == "requestid") {
                UserId = allocateUserID(ExistingUsers, FreeUserIDs);
                UserIDMSQ->sendMessage(std::to_string(UserId));
                continue;
            }

            size_t M = Command.find_first_of(':');
            if (M == 0 || M == std::string::npos || M + 1 == Command.length()) {
                throw std::runtime_error(": is not found or the last character!\n");
            }

            // request message: "user_id:request_content";
            UserId = std::stol(Command.substr(0, M));
            if (UserId < 100) {
                throw std::runtime_error("[Master] UserId cannot less than 100!");
            }

            std::string UserRequest = Command.substr(M + 1);
            DEBUG(errs() << "[Master] get request: " << UserRequest << "\n";);
            if (UserRequest == "open") {
                auto It = UserWorkerMap.find(UserId);
                if (It == UserWorkerMap.end()) {
                    if (!FreeMSQs.empty()) {
                        // try to reuse
                        auto& Worker = FreeMSQs.back();

                        DEBUG(errs() << "[Master] (open) Reusing and sending to " << UserId << ": " << Worker.second << "...");
                        if (CommunicateMSQ->sendMessage(std::to_string(Worker.second), UserId) == -1) {
                            throw std::runtime_error("[Master] Fail to send slave id to user after open with reuse!");
                        }
                        DEBUG(errs() << "Done!\n");

                        UserWorkerMap[UserId] = Worker;
                        FreeMSQs.pop_back();

                        // Here we need one guarantee
                        // 1. client has got the sent msg
                        if (-1 == CommunicateMSQ->recvMessage(CtrlMsg, 11)) {
                            throw std::runtime_error("[Master] Fail to receive confirmation!");
                        }
                        DEBUG(assert(CtrlMsg == Command.substr(0, M) + ":got"));
                        continue;
                    }
                } else {
                    throw std::runtime_error("[Master] Existent user requests open!");
                }
            } else if (UserRequest == "close") {
                auto It = UserWorkerMap.find(UserId);
                if (It != UserWorkerMap.end()) {
                    UserWorkerMap.erase(It);
                    FreeMSQs.push_back(It->second);
                    FreeUserIDs.push_back(UserId);
                    continue;
                } else {
                    throw std::runtime_error("[Master] Non-existent user requests close!");
                }
            } else if (UserRequest == "reopen") {
                // reopen means the client detects the exception of
                // the slave, and request a new one.
                auto It = UserWorkerMap.find(UserId);
                if (It != UserWorkerMap.end()) {
                    auto ChildPID = It->second.first;
                    UserWorkerMap.erase(It);
                    kill(ChildPID, SIGINT);
                    waitpid(ChildPID, 0, 0);

                    // try reuse
                    if (!FreeMSQs.empty()) {
                        auto& Worker = FreeMSQs.back();
                        DEBUG(errs() << "[Master] (reopen) Reusing and sending to " << UserId << ": " << Worker.second << "...");
                        if (CommunicateMSQ->sendMessage(std::to_string(Worker.second), UserId) == -1) {
                            throw std::runtime_error("[Master] Fail to send slave id to user after reopen!");
                        }
                        DEBUG(errs() << "Done!\n");

                        UserWorkerMap[UserId] = Worker;
                        FreeMSQs.pop_back();

                        // Here we need one guarantee
                        // 1. client has got the sent msg
                        if (-1 == CommunicateMSQ->recvMessage(CtrlMsg, 11)) {
                            throw std::runtime_error("[Master] Fail to receive confirmation!");
                        }
                        DEBUG(assert(CtrlMsg == Command.substr(0, M) + ":got"));
                        continue;
                    }
                } else {
                    throw std::runtime_error("[Master] Non-existent user requests reopen!");
                }
            } else {
                throw std::runtime_error("[Master] Unknown request: " + UserRequest);
            }
        } catch (std::exception& Ex) {
            errs() << "[Master] Invalid request message: " << Command << "\n";
            errs() << Ex.what() << "\n";
            abort();
        }

        int ChildMSQKey = MSQKey.getValue() + ++Counter;
        SlaveMSQ = new MessageQueue(ChildMSQKey, true);
        // NOTE: must be sent before fork
        DEBUG(errs() << "[Master] Sending to " << UserId << ": " << ChildMSQKey << "...");
        if (CommunicateMSQ->sendMessage(std::to_string(ChildMSQKey), UserId) == -1) {
            throw std::runtime_error("[Master] Fail to send slave id to user after (re)open without reuse!");
        }
        DEBUG(errs() << "Done!\n");

        pid_t ChildPid = fork();
        switch(ChildPid) {
        case -1: {
            kill(0, SIGINT); // sent to every process in the group
            DEBUG(errs() << "[Master] Fail to fork: " << strerror(errno) << "\n");
            llvm_unreachable("Fail to fork subprocess for smt solving!");
        }
        break;
        case 0: { // Child process
            // The parent process' MSQ is not needed here.
            delete CommandMSQ;
            delete CommunicateMSQ;
            delete UserIDMSQ;
            CommandMSQ = nullptr;
            CommunicateMSQ = nullptr;
            UserIDMSQ = nullptr;

            SMTFactory Factory;
            SMTSolver Solver = Factory.createSMTSolver();
            std::string Message;
            while (true) {
                if (!Incremental.getValue()) {
                    if (-1 == SlaveMSQ->recvMessage(Message, 1)) {
                        perror("Slave fails to recv: ");
                        abort();
                    }
                    DEBUG_WITH_TYPE("smtd-slave", errs() << "[Slave " << ChildPid << "] get msg (1): " << Message << "\n");

                    SMTExpr Expr = Factory.parseSMTLib2String(Message);
                    Solver.add(Expr);
                    auto Result = Solver.check();
                    if (-1 == SlaveMSQ->sendMessage(std::to_string((int)Result), 2)) {
                        perror("Slave fails to send: ");
                        abort();
                    }
                    Solver.reset();
                } else {
                    // TODO incremental methods should have more complex protocol
                }
            }
        }
        break;
        default: { // Parent process
            UserWorkerMap[UserId] = {ChildPid, ChildMSQKey};
            // Here we need one guarantee
            // 1. client has got the sent msg
            if (-1 == CommunicateMSQ->recvMessage(CtrlMsg, 11)) {
                llvm_unreachable("[Master] Fail to receive confirmation!");
            }
            DEBUG(assert(CtrlMsg == std::to_string(UserId) + ":got"));
            // This has been copied to subprocess, and is not necessary here.
            delete SlaveMSQ;
        }
        break;
        }
    }

    llvm_unreachable("Currently, only can exit by interruption (Ctrl + C)!");
    return 1;
}
