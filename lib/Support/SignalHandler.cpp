/*
 * SignalHandler.cpp
 *
 *  Created on: 17/07/2015
 *      Author: andyzhou
 *
 *  Modified on: 30/09/2015
 *      Author: Qingkai
 */

#include <llvm/Support/Signals.h>
#include <vector>

#include "Support/SignalHandler.h"

static std::vector<std::function<void()>> InterruptSigHandlers;
static std::vector<std::function<void()>> ErrorSigHandlers;

static void OnInterruptSignal() {
	for(auto Handler : InterruptSigHandlers) {
		Handler();
	}
	exit(1);
}

static void OnErrorSignal(void*) {
	for(auto Handler : ErrorSigHandlers) {
		Handler();
	}
	exit(1);
}

void RegisterSignalHandler() {
	// error signal handlers
	llvm::sys::PrintStackTraceOnErrorSignal();
	llvm::sys::AddSignalHandler(OnErrorSignal, 0);

	// interrupt signal handlers
	llvm::sys::SetInterruptFunction(OnInterruptSignal);
}

void AddInterruptSigHandler(std::function<void()>& SigHandler) {
	InterruptSigHandlers.push_back(SigHandler);
}

void AddErrorSigHandler(std::function<void()>& SigHandler) {
	ErrorSigHandlers.push_back(SigHandler);
}

