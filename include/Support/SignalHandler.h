/*
 * SignalHandler.h
 *
 *  Created on: 19/12/2015
 *      Author: Qingkai
 */

#ifndef SUPPORT_SIGNALHANDLER_H
#define SUPPORT_SIGNALHANDLER_H

#include <functional>
#include <llvm/Support/raw_ostream.h>

/// Register signal handlers
void RegisterSignalHandler();

/// Add signal handler for interrupt signals
/// Handlers will be executed in order until
/// the last one or one of them exits
void AddInterruptSigHandler(std::function<void()>& SigHandler);

/// Add signal handler for error signals
/// Handlers will be executed in order until
/// the last one or one of them exits
void AddErrorSigHandler(std::function<void()>& SigHandler);

#endif /* SUPPORT_SIGNALHANDLER_H */
