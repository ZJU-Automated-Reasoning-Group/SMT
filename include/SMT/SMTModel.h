/**
 * Authors: Qingkai & Andy
 */

#ifndef SMT_SMTMODEL_H
#define SMT_SMTMODEL_H

#include <string>

#include "z3++.h"
#include "SMTObject.h"

class SMTFactory;

class SMTModel : public SMTObject {
private:
	z3::model Model;

	SMTModel(SMTFactory* F, z3::model Z3Model);

public:

	SMTModel(SMTModel const & M);

	virtual ~SMTModel();

	SMTModel& operator=(const SMTModel& M);

	unsigned size();

	std::pair<std::string, std::string> getModelDbgInfo(int Index);

	friend class SMTSolver;
};

#endif
