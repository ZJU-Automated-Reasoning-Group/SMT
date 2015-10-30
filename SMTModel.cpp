/**
 * Authors: Qingkai & Andy
 */

#include "SMTModel.h"
#include "SMTFactory.h"

SMTModel::SMTModel(SMTFactory* F, z3::model m) :
		Model(m), Factory(F) {
}

SMTModel::SMTModel(SMTModel const & m) :
		Model(m.Model), Factory(m.Factory) {
}

SMTModel::~SMTModel() {
}

SMTModel& SMTModel::operator=(const SMTModel& M) {
	if (this != &M) {
		this->Model = M.Model;
		this->Factory = M.Factory;
	}
	return *this;
}

unsigned SMTModel::size() {
	return Model.size();
}

std::pair<std::string, std::string> SMTModel::getModelDbgInfo(int Index) {
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
