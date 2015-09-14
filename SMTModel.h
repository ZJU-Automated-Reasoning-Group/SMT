/**
 * Authors: Qingkai & Andy
 */

#ifndef UTILS_SMT_SMTMODEL_H
#define UTILS_SMT_SMTMODEL_H

#include <z3++.h>

class SMTModel {
private:
	z3::model z3_model;

	SMTModel(z3::model m) :
			z3_model(m) {
	}

public:

	SMTModel(SMTModel const & m) :
			z3_model(m.z3_model) {
	}

	~SMTModel() {
	}

	SMTModel& operator=(const SMTModel& M) {
		if (this != &M) {
			this->z3_model = M.z3_model;
		}
		return *this;
	}

	unsigned size() {
		return z3_model.size();
	}

	std::pair<std::string, std::string> getModelDbgInfo(int Index) {
		auto Item = z3_model[Index];

		if (Item.name().kind() == Z3_STRING_SYMBOL) {
			z3::expr E = z3_model.get_const_interp(Item);
			std::ostringstream OstrExpr;
			OstrExpr << E;

			std::ostringstream OstrTy;
			OstrTy << E.get_sort();

			return std::pair<std::string, std::string>(OstrExpr.str(), OstrTy.str());
		} else {
			return std::pair<std::string, std::string>("", "");
		}
	}

	friend class SMTSolver;
};

#endif
