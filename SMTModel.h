/**
 * Authors: Qingkai & Andy
 */

#ifndef UTILS_SMT_SMTMODEL_H
#define UTILS_SMT_SMTMODEL_H

#include <z3++.h>

class SMTModel {
private:
	z3::model Model;

	SMTModel(z3::model m) :
			Model(m) {
	}

public:

	SMTModel(SMTModel const & m) :
			Model(m.Model) {
	}

	~SMTModel() {
	}

	SMTModel& operator=(const SMTModel& M) {
		if (this != &M) {
			this->Model = M.Model;
		}
		return *this;
	}

	unsigned size() {
		return Model.size();
	}

	std::pair<std::string, std::string> getModelDbgInfo(int Index) {
		auto Item = Model[Index];

		if (Item.name().kind() == Z3_STRING_SYMBOL) {
			z3::expr E = Model.get_const_interp(Item);
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
