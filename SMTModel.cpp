/**
 * Authors: Qingkai & Andy
 */

#include "SMTModel.h"
#include "SMTFactory.h"

SMTModel::SMTModel(SMTFactory* F, z3::model Z3Model) : SMTObject(F),
		Model(Z3Model) {
}

SMTModel::SMTModel(SMTModel const & M) : SMTObject(M),
		Model(M.Model) {
}

SMTModel::~SMTModel() {
}

SMTModel& SMTModel::operator=(const SMTModel& M) {
	SMTObject::operator =(M);
	if (this != &M) {
		this->Model = M.Model;
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
