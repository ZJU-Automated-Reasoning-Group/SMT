/*
 * Message queue for IPC
 *
 * Author: Qingkai
 *
 */

#ifndef SUPPORT_MESSAGEQUEUE_H
#define SUPPORT_MESSAGEQUEUE_H

#define IPC_MSQ_BUFF_SIZE 2048
typedef struct MessageType {
	long int MessageTypeID;
	char Data[IPC_MSQ_BUFF_SIZE];
} MessageType;

class MessageQueue {
private:
	/// The ID of the message queue
	int MSQId;

	/// Message block
	MessageType Message;

public:
	/// Create a connect to the message queue using key \p Key
	///
	/// \p New indicates if the it will allocate a brand new
	/// message queue or reuse an existing one.
	MessageQueue(key_t Key, bool New = false);

	/// The destructor will not close the message queue in the
	/// system. You can call MessageQueue:destroy to do so.
	~MessageQueue();

	/// Close the message queue. This means that other processes connected
	/// to it will fail to receive and send.
	void destroy();

	/// This function appends a copy of the message \p MessageRef to the message queue
	///
	/// If sufficient space is available in the queue, it succeeds immediately.
	///
	/// If insufficient space is available in  the  queue, then its behavior is to
	/// block until space becomes available.
	///
	/// \p MessageTypeId should be a value greater than 0, to mark the message type.
	///
	/// It returns -1 when some error happens, and returns 0 otherwise.
	int sendMessage(const std::string& MessageRef, long MessageTypeId = 1);

	/// This function receives a message from the queue
	///
	/// The argument \p MessageTypeId specifies the type of message requested as follows:
	///     * If \p MessageTypeId is 0, then the first message in the queue is read.
	///     * If \p MessageTypeId is greater than 0, then the first message in the queue
	///         of type \p MessageTypeId is read
	///     * If \p MessageTypeId is less than 0, then the first message in the queue with
	///         the lowest type less than or  equal  to  the  absolute value of
	///         \p MessageTypeId will be read.
	///
	/// If no qualified message can be read from the queue, it will be blocked.
	///
	/// It returns -1 when some error happens, and returns 0 otherwise.
	int recvMessage(std::string& MessageRef, long MessageTypeId = 0);
};

#endif
