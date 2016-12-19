/*
 * This is a testing client for smtd.
 *
 * Author: Qingkai
 */

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/FileSystem.h>

#include "Support/MessageQueue.h"
#include "SMT/SMTFactory.h"

#define DEBUG_TYPE "smtd"

using namespace llvm;

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

void test(int Key) {
    MessageQueue* MasterCommandMSQ = new MessageQueue(Key);
    MessageQueue* MasterCommunicateMSQ = new MessageQueue(1 + Key);

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
