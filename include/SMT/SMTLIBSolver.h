/**
 * Authors: rainoftime
 */

#ifndef SMTLIBSOLVER_SMTLIB_SOLVER_H_
#define SMTLIBSOLVER_SMTLIB_SOLVER_H_


#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


enum SMTLIBSolverResult {
	SMTRT_Unsat, SMTRT_Sat, SMTRT_Unknown, SMTRT_TBD, SMTRT_Error
};


class SmtlibSmtSolver {
public:

	SmtlibSmtSolver(std::string path, std::vector<std::string> cmdLineArgs);
	~SmtlibSmtSolver();
        
        std::vector<std::string> CmdTraces = { };

	void setLogic(std::string logic);

	void add(std::string fml);

	SMTLIBSolverResult check();

	SMTLIBSolverResult solveWholeFormula(std::string query);

	void push(unsigned num = 1);
	void pop(unsigned num = 1);
	unsigned getContextLevel() const;
  
        void reset();

	// path to the solver binary
	std::string path;

	// command line arguments for the binary
	std::vector<std::string> cmdLineArgs;

	bool debug;
        int queries = 0; // number of queries
        

protected:

	void init();

	void writeCommand(std::string smt2Command);

	/*!
	 * Reads from the solver. The output is checked for an error message and an exception is thrown in that case.
	 * @param waitForOutput if this is true and there is currently no output, we will wait until there is output.
	 * @return the output of the solver. Every entry of the vector corresponds to one output line
	 */
         std::string readSolverOutput(bool waitForOutput = true);

	/*!
	 * Checks if the given message contains an error message and throws an exception.
	 * More precisely, an exception is thrown whenever the word "error" is contained in the message.
	 * This function is directly called when reading the solver output via readSolverOutput()
	 * We will try to parse the message in the SMT-LIBv2 format, i.e.,
	 * ( error "this is the error message from the solver" ) to give some debug information
	 * However, the whole message is always written to the debug log (providing there is an error)
	 * @param message the considered message which should be an output of the solver.
	 */
        SMTLIBSolverResult checkForErrorMessage(const std::string message);

	// descriptors for the pipe from and to the solver
	int toSolver;
	int fromSolver;
	// A flag storing the Process ID of the solver. If this is zero, then the solver is not running

public:
	pid_t processIdOfSolver;

	// tracks the context level of the solver
	// (e.g., number of pushes - number of pops)
	unsigned contextLevel;

};


// The interface for cereating a solver
// TODO: add more options for controlling the behavior
SmtlibSmtSolver* createSMTLIBSolver();


#endif /* SMTLIBSOLVER_SMTLIB_SOLVER_H_ */
