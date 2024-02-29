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
#include <csignal> // kill
#include <unistd.h> // fork

#include <map>
#include <set>

#include "Support/SignalHandler.h"
#include "Support/MessageQueue.h"
#include "UserIDAllocator.h"
#include "SMT/SMTFactory.h"

#define DEBUG_TYPE "smtd"

using namespace llvm;

static cl::opt<int> MSQKey("smtd-key", cl::desc("Indicate the key of the master's message queue."), cl::init(1234));

static cl::opt<bool> Incremental("smtd-incremental", cl::desc("Enable incremental SMT solving."), cl::init(false));

static cl::opt<bool> RunTestClient("smtd-test", cl::desc("Run a testing client."), cl::init(false), cl::ReallyHidden);

// a testing client
extern void test(int Key);

static MessageQueue* SlaveMSQ = nullptr;

static MessageQueue* CommandMSQ = nullptr;

static MessageQueue* CommunicateMSQ = nullptr;

static pid_t MainProcessID;

static void RegisterSigHandler() {
    // Signal handlers
    MainProcessID = getpid();
    RegisterSignalHandler();
    std::function<void()> ExitHandler = []() {
        // close all child processes
        if (MainProcessID == getpid()) {
            CommandMSQ->destroy();
            CommunicateMSQ->destroy();
            delete CommunicateMSQ;
            delete CommandMSQ;

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
        SlaveMSQ = nullptr;
    };
    AddInterruptSigHandler(ExitHandler);
    AddErrorSigHandler(ExitHandler);
}

int main(int argc, char **argv) {
    // Print stack trace when crash occurs
    llvm::PrettyStackTraceProgram X(argc, argv);

    // Enable debug stream buffering.
    llvm::EnableDebugBuffering = true;

    // Call llvm_shutdown() on exit by calling destructor.
    llvm::llvm_shutdown_obj Y;

    // Parse command line options
    llvm::cl::ParseCommandLineOptions(argc, argv, "SMT solving service.\n");

    // Register signal handlers
    RegisterSigHandler();

    if (RunTestClient.getValue()) {
        test(MSQKey.getValue());
        return 0;
    }

    if (Incremental.getValue()) {
        llvm_unreachable("Incremental mode has not been supported yet!");
        abort();
    }

    outs() << "*******************************\n"
           << "Please run your applications with -solver-enable-smtd=" << MSQKey.getValue()
           << " -solver-enable-smtd-incremental=" << (Incremental.getValue() ? "true" : "false") << "\n"
           << "*******************************\n";

    UserIDAllocator* IDAllocator = UserIDAllocator::getUserAllocator();
    std::vector<std::pair<pid_t, int>> FreeMSQs;
    std::map<long, std::pair<pid_t, int>> UserWorkerMap;

    int Counter = 0;

    CommandMSQ = new MessageQueue(MSQKey.getValue(), true);
    CommunicateMSQ = new MessageQueue(MSQKey.getValue() + ++Counter, true);

    std::string Command, CtrlMsg; // buffer that can be reused
    while (true) {
        if (-1 == CommandMSQ->recvMessage(Command)) {
            llvm_unreachable("[Master] fail to recv command");
        }
        DEBUG(errs() << "[Master] get msg (0): " << Command << "\n";);

        long UserID = -1;
        try {
            if (Command == "requestid") {
                UserID = IDAllocator->allocate();
                CommunicateMSQ->sendMessage(std::to_string(UserID), 12);
                continue;
            }

            size_t M = Command.find_first_of(':');
            if (M == 0 || M == std::string::npos || M + 1 == Command.length()) {
                throw std::runtime_error(": is not found or the last character!\n");
            }

            // request message: "user_id:request_content";
            UserID = std::stol(Command.substr(0, M));
            if (UserID < 100) {
                throw std::runtime_error("[Master] UserID cannot be less than 100!");
            }

            std::string UserRequest = Command.substr(M + 1);
            DEBUG(errs() << "[Master] get request: " << UserRequest << "\n";);
            if (UserRequest == "open") {
                auto It = UserWorkerMap.find(UserID);
                if (It == UserWorkerMap.end()) {
                    if (!FreeMSQs.empty()) {
                        // try to reuse
                        auto& Worker = FreeMSQs.back();

                        DEBUG(errs() << "[Master] (open) Reusing and sending to " << UserID << ": " << Worker.second << "...");
                        if (CommunicateMSQ->sendMessage(std::to_string(Worker.second), UserID) == -1) {
                            throw std::runtime_error("[Master] Fail to send slave id to user after open with reuse!");
                        }
                        DEBUG(errs() << "Done!\n");

                        UserWorkerMap[UserID] = Worker;
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
                auto It = UserWorkerMap.find(UserID);
                if (It != UserWorkerMap.end()) {
                    UserWorkerMap.erase(It);
                    FreeMSQs.push_back(It->second);
                    IDAllocator->recycle(UserID);
                    continue;
                } else {
                    throw std::runtime_error("[Master] Non-existent user requests close!");
                }
            } else if (UserRequest == "reopen") {
                // reopen means the client detects the exception of
                // the slave, and request a new one.
                auto It = UserWorkerMap.find(UserID);
                if (It != UserWorkerMap.end()) {
                    auto ChildPID = It->second.first;
                    UserWorkerMap.erase(It);
                    kill(ChildPID, SIGINT);
                    waitpid(ChildPID, 0, 0);

                    // try reuse
                    if (!FreeMSQs.empty()) {
                        auto& Worker = FreeMSQs.back();
                        DEBUG(errs() << "[Master] (reopen) Reusing and sending to " << UserID << ": " << Worker.second << "...");
                        if (CommunicateMSQ->sendMessage(std::to_string(Worker.second), UserID) == -1) {
                            throw std::runtime_error("[Master] Fail to send slave id to user after reopen!");
                        }
                        DEBUG(errs() << "Done!\n");

                        UserWorkerMap[UserID] = Worker;
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
        DEBUG(errs() << "[Master] Sending to " << UserID << ": " << ChildMSQKey << "...");
        if (CommunicateMSQ->sendMessage(std::to_string(ChildMSQKey), UserID) == -1) {
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
            CommandMSQ = nullptr;
            CommunicateMSQ = nullptr;

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
            UserWorkerMap[UserID] = {ChildPid, ChildMSQKey};
            // Here we need one guarantee
            // 1. client has got the sent msg
            if (-1 == CommunicateMSQ->recvMessage(CtrlMsg, 11)) {
                llvm_unreachable("[Master] Fail to receive confirmation!");
            }
            DEBUG(assert(CtrlMsg == std::to_string(UserID) + ":got"));
            // This has been copied to subprocess, and is not necessary here.
            delete SlaveMSQ;
        }
        break;
        }
    }

    llvm_unreachable("Currently, only can exit by interruption (Ctrl + C)!");
    return 1;
}
