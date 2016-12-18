/*
 * Message queue for IPC
 *
 * Author: Qingkai
 *
 */

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <error.h>
#include <string>

#include "Support/MessageQueue.h"

#define DEBUG_TYPE "msq"

using namespace llvm;

// Two magic string, both length is MSG_MAGICLEN, i.e.,7.
#define MSG_CONTINUE "X033[0m"
#define MSG_FINISHED "X033[1m"
#define MSG_MAGICLEN 7

#define IPC_MSQ_MARGIN 20

MessageQueue::MessageQueue(key_t Key, bool New) {
	MSQId = msgget(Key, 0666 | IPC_CREAT | (New ? IPC_EXCL : 0));
	if (MSQId == -1) {
		perror("Fail to create");
		llvm_unreachable("Fail to create message queue.");
	}

	assert(IPC_MSQ_BUFF_SIZE > IPC_MSQ_MARGIN);
	assert(MSG_MAGICLEN < IPC_MSQ_MARGIN);
}

MessageQueue::~MessageQueue() {
}

void MessageQueue::destroy() {
	if (msgctl(MSQId, IPC_RMID, 0) == -1) {
		perror("Fail to close: ");
		llvm_unreachable("Fail to close message queue, you can try to use `ipcrm` to close manually!");
	}
}

int MessageQueue::sendMessage(const std::string& MessageRef, long MessageTypeId) {
	size_t MessageLen = MessageRef.length();

	size_t SegmentLen = IPC_MSQ_BUFF_SIZE - IPC_MSQ_MARGIN;
	size_t SegmentNum = MessageLen / SegmentLen + 1;

	const char* CStr = MessageRef.c_str();
	Message.MessageTypeID = MessageTypeId;

	size_t Counter = 0;
	while (Counter < SegmentNum) {
		strncpy(Message.Data, &(CStr[Counter * SegmentLen]), SegmentLen);
		Message.Data[SegmentLen] = '\0';
		strcat(Message.Data, Counter != SegmentNum - 1 ? MSG_CONTINUE : MSG_FINISHED);

		DEBUG(errs() << "Sending: " << Message.Data << "\n");

		if (msgsnd(MSQId, &Message, IPC_MSQ_BUFF_SIZE, 0) == -1) {
			// perror("Fail to send message: ");
			// llvm_unreachable("Fail to send message.");
			return -1;
		}

		Counter++;
	}
	return 0;
}

int MessageQueue::recvMessage(std::string& MessageRef, long MessageTypeId) {
	MessageRef.clear(); // Original memory space in MessageRef will be reused.
	while(true) {
		int NumBytes = msgrcv(MSQId, &Message, IPC_MSQ_BUFF_SIZE, MessageTypeId, 0);
		if (NumBytes == -1) {
			// errs() << this << " fails to recv message: " << strerror(errno) << "\n";
			// llvm_unreachable((std::string("Fail to recv message. ") + std::to_string((long)this) + " " + std::to_string(Key)).c_str());
			return -1;
		}

		char* Data = Message.Data;
		char* Tail = Data + (strlen(Data) - MSG_MAGICLEN);

		DEBUG(errs() << "Recved: " << Data << "\n");
		DEBUG(errs() << "Recved Tail: " << Tail << "\n");

		if (strcmp(Tail, MSG_FINISHED) == 0) {
			*Tail = '\0';
			DEBUG(errs() << "Recved after Rm Tail: " << Data << "\n");
			MessageRef.append(Data);

			break;
		} else if (strcmp(Tail, MSG_CONTINUE) == 0) {
			// okay
		} else {
			errs() << Tail << "\n";
			assert(false && "Collapsed message received!");
		}

		*Tail = '\0';
		DEBUG(errs() << "Recved after Rm Tail: " << Data << "\n");
		MessageRef.append(Data);
	}

	return 0;
}
