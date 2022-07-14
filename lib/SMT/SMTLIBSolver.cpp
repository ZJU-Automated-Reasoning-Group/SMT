/**
 * Authors: rainoftime
 */

#include "SMT/SMTLIBSolver.h"
#include "SMT/SMTExceptions.h"

#include "SMT/SMTConfigure.h"

#include <assert.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <algorithm>
#include <iostream>
#include <exception>
#include <cstdio>

SmtlibSmtSolver::SmtlibSmtSolver(std::string path,
		std::vector<std::string> cmdArgs) :
		path(path), cmdLineArgs(cmdArgs), debug(true), contextLevel(0) {
	processIdOfSolver = 0;
	init();
}

SmtlibSmtSolver::~SmtlibSmtSolver() {
	writeCommand("( exit )"); // TOOD: perhaps some solvers do not suppor this command
	// do not wait for success because it does not matter at this point and may cause problems if the solver is not running properly
	if (processIdOfSolver != 0) {
		// Since the process has been opened successfully, it means that we have to close our fds
		close(fromSolver);
		close(toSolver);
		kill(processIdOfSolver, SIGTERM);
		waitpid(processIdOfSolver, nullptr, 0); // make sure the process has exited
	}
}

void SmtlibSmtSolver::init() {

	signal(SIGPIPE, SIG_IGN);

	// get the pipes started
	int pipeIn[2];
	int pipeOut[2];
	const int READ = 0;
	const int WRITE = 1;
	assert(pipe(pipeIn) == 0);
	assert(pipe(pipeOut) == 0);

	// now start the child process, i.e., the solver
	pid_t pid = fork();
	assert(pid >= 0);
	if (pid == 0) {
		// Child process
		// duplicate the fd so that standard input and output will be send to our pipes
		dup2(pipeIn[READ], STDIN_FILENO);
		dup2(pipeOut[WRITE], STDOUT_FILENO);
		dup2(pipeOut[WRITE], STDERR_FILENO);
		// we can now close everything since our child process will use std in and out to address the pipes
		close(pipeIn[READ]);
		close(pipeIn[WRITE]);
		close(pipeOut[READ]);
		close(pipeOut[WRITE]);

		const char **argv = new const char*[cmdLineArgs.size() + 2];
		argv[0] = path.c_str();
		for (int i = 1; i <= cmdLineArgs.size(); i++) {
			argv[i] = cmdLineArgs[i - 1].c_str();
		}
		argv[cmdLineArgs.size() + 1] = NULL;

		execv(path.c_str(), (char**) argv);  //"-smt2 -in"
		// if we reach this point, execl was not successful
		assert("Could not execute the solver correctly");
	}
	// Parent Process
	toSolver = pipeIn[WRITE];
	fromSolver = pipeOut[READ];
	close(pipeOut[WRITE]);
	close(pipeIn[READ]);
	processIdOfSolver = pid;

	// some initial commands
	// writeCommand("( set-option :print-success true )");
	// writeCommand("( set-logic ALL )");
	// writeCommand("( get-info :name )");
}

void SmtlibSmtSolver::add(std::string cmd) {
	writeCommand(cmd);
        // if (debug) CmdTraces.push_back(cmd);
}

SMTLIBSolverResult SmtlibSmtSolver::check() {
        // if (debug) CmdTraces.push_back("(check-sat)\n");
	writeCommand("(check-sat)\n");
	auto solverOutput = readSolverOutput();
	auto errorRes = checkForErrorMessage(solverOutput);
	if (errorRes != SMTLIBSolverResult::SMTRT_TBD) // unknown or error
		return errorRes;

	if (solverOutput.find("unsat") != std::string::npos) {
		return SMTLIBSolverResult::SMTRT_Unsat;
	} else {
		// TODO: other cases?
		return SMTLIBSolverResult::SMTRT_Sat;
	}
}

void SmtlibSmtSolver::setLogic(std::string logic) {
	writeCommand("(set-logic " + logic + ")\n");
        // if (debug) CmdTraces.push_back("(set-logic " + logic + ")\n");
}

void SmtlibSmtSolver::push(unsigned num) {
	writeCommand("(push " + std::to_string(num) + ")\n");
	contextLevel += num;
        // if (debug) CmdTraces.push_back("(push " + std::to_string(num) + ")\n");
}

void SmtlibSmtSolver::pop(unsigned num) {
	writeCommand("(pop " + std::to_string(num) + ")\n");
	contextLevel -= num;
        // if (debug) CmdTraces.push_back("(pop " + std::to_string(num) + ")\n");
}

// TOOD: use reset or reset-assertions
void SmtlibSmtSolver::reset() {
        writeCommand("(reset-assertions)");
        // writeCommand("(reset)");
}

unsigned SmtlibSmtSolver::getContextLevel() const {
	return contextLevel;
}

SMTLIBSolverResult SmtlibSmtSolver::solveWholeFormula(std::string query) {
	writeCommand(query);
	auto solverOutput = readSolverOutput();
	auto errorRes = checkForErrorMessage(solverOutput);
	if (errorRes != SMTLIBSolverResult::SMTRT_TBD) // unknown or error
		return errorRes;

	if (solverOutput.find("unsat") != std::string::npos) {
		return SMTLIBSolverResult::SMTRT_Unsat;
	} else {
		// TODO: other cases?
		return SMTLIBSolverResult::SMTRT_Sat;
	}
}

void SmtlibSmtSolver::writeCommand(std::string smt2Command) {
	if (processIdOfSolver != 0) {
		if (write(toSolver, (smt2Command + "\n").c_str(),
				smt2Command.length() + 1) < 0) {
			assert("Was not able to write cmd");
		}
	}
}

std::string SmtlibSmtSolver::readSolverOutput(bool waitForOutput) {
	if (processIdOfSolver == 0) {
		std::cout<< "failed to read solver output as the solver is not running\n";
		return "error"; // is this OK?
	}
	int bytesReadable;
	if (waitForOutput) {
		bytesReadable = 1;          // just assume that there are bytes readable
	} else if (ioctl(fromSolver, FIONREAD, &bytesReadable) < 0) { // actually obtain the readable bytes
		std::cout << "Could not check if the solver has output\n";
		return "error"; // is this OK?
	}
	std::string solverOutput = "";
	const ssize_t MAX_CHUNK_SIZE = 256;
	char chunk[MAX_CHUNK_SIZE];
	while (bytesReadable > 0) {
		ssize_t chunkSize = read(fromSolver, chunk, MAX_CHUNK_SIZE);
		assert(chunkSize >= 0);
		solverOutput += std::string(chunk, chunkSize);
		if (ioctl(fromSolver, FIONREAD, &bytesReadable) < 0) { // obtain the new amount of readable bytes
			std::cout << "Could not check if the solver has output\n";
			return "error";
		}
		if (bytesReadable == 0 && solverOutput.back() != '\n') {
			bytesReadable = 1;  // we expect more output!
		}
		if (bytesReadable > 0) {
			pid_t w = waitpid(processIdOfSolver, nullptr, WNOHANG);
			if (w != 0) {
				std::cout
						<< "Error when checking whether the solver is still running. Will assume that solver has terminated.\n";
				std::cout
						<< "The solver exited unexpectedly when reading output: "
						<< solverOutput << "\n";
				solverOutput += "terminated";
				this->processIdOfSolver = 0;
				bytesReadable = 0;
			}
		}
	}
	return solverOutput;
}

SMTLIBSolverResult SmtlibSmtSolver::checkForErrorMessage(
		const std::string message) {
	// size_t errorOccurrance = message.find("error");
	// do not throw an exception for timeout or memout errors
	if (message.find("timeout") != std::string::npos) {
		if (debug) {
			std::cout << "SMT solver answered: '" << message
					<< "' and I am interpreting this as timeout\n";

		}
		this->processIdOfSolver = 0;
		return SMTLIBSolverResult::SMTRT_Unknown;
	} else if (message.find("memory") != std::string::npos) {
		if (debug) {
			std::cout << "SMT solver answered: '" << message
					<< "' and I am interpreting this as out of memory \n";
		}
		this->processIdOfSolver = 0;
		return SMTLIBSolverResult::SMTRT_Unknown;
	} else if (message.find("error") != std::string::npos) {
		if (debug) {
			size_t errorOccurrance = message.find("error");

			std::string errorMsg =
					"An error was detected while checking the solver output. ";
			std::cout << "Detected an error message in the solver response:\n"
					<< message << "\n";
			size_t firstQuoteSign = message.find('\"', errorOccurrance);
			if (firstQuoteSign != std::string::npos
					&& message.find("\\\"", firstQuoteSign - 1)
							!= firstQuoteSign - 1) {
				size_t secondQuoteSign = message.find('\"', firstQuoteSign + 1);
				while (secondQuoteSign != std::string::npos
						&& message.find("\\\"", secondQuoteSign - 1)
								== secondQuoteSign - 1) {
					secondQuoteSign = message.find('\"', secondQuoteSign + 1);
				}
				if (secondQuoteSign != std::string::npos) {
					errorMsg += "The error message was: <<"
							+ message.substr(errorOccurrance,
									secondQuoteSign - errorOccurrance + 1)
							+ ">>.";
					std::cout << errorMsg << "\n";
				}
			}
			errorMsg +=
					"The error message could not be parsed correctly. Snippet:\n"
							+ message.substr(errorOccurrance, 200);
			std::cout << errorMsg << "\n";
		}

		return SMTLIBSolverResult::SMTRT_Error;
	}

	return SMTLIBSolverResult::SMTRT_TBD;
}



// TODO: allow more options
SmtlibSmtSolver* createSMTLIBSolver() {
  	SmtlibSmtSolver* BinSolver = new SmtlibSmtSolver(SMTConfig::SMTLIBSolverPath, SMTConfig::SMTLIBSolverArgs);
	BinSolver->setLogic("QF_BV");
	//gs->set_timeout(1);
	return BinSolver;
}
