/**
 * Authors: Qingkai & Andy
 */

#ifndef UTILS_SMT_SMTMODEL_H
#define UTILS_SMT_SMTMODEL_H

#include <z3++.h>

class SMTFactory;

class SMTModel {
private:
	z3::model Model;
	SMTFactory* Factory;

	SMTModel(SMTFactory* F, z3::model Z3Model);

public:

	SMTModel(SMTModel const & M);

	~SMTModel();

	SMTModel& operator=(const SMTModel& M);

	unsigned size();

	std::pair<std::string, std::string> getModelDbgInfo(int Index);

	friend class SMTSolver;
};

#endif
