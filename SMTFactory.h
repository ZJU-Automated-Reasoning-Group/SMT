/**
 * Authors: Qingkai & Andy
 */

#ifndef UTILS_SMT_SMTFACTORY_H
#define UTILS_SMT_SMTFACTORY_H

#include <z3++.h>
#include <stdio.h>
#include <string>
#include <mutex>
#include <set>
#include <unordered_map>
#include <map>

#include "SMTExpr.h"
#include "SMTSolver.h"

class SMTExprPruner {
public:
	/// See SMTFactory::translate for details
	virtual bool shouldPrune(const SMTExpr&) = 0;

	virtual ~SMTExprPruner() {
	}
};

/**
 * When a SMTFactory is deconstructed, the SMTExpr, SMTExprVec,
 * SMTModel, SMTSolver created by the factory become invalid.
 * Thus, please deconstruct them before deconstructing
 * the factory.
 *
 * Constraints built by the same SMTFactory instance cannot be
 * accessed concurrently. SMTFactory provides a FactoryLock
 * for concurrency issues.
 */
class SMTFactory {
private:
	z3::context Ctx;

	std::mutex FactoryLock;

	unsigned TempSMTVaraibleIndex;

public:
	SMTFactory() :
			TempSMTVaraibleIndex(0) {
	}

	~SMTFactory() {
	}

	SMTSolver createSMTSolver();

	SMTExpr createEmptySMTExpr();

	SMTExprVec createEmptySMTExprVec();

	SMTExprVec createBoolSMTExprVec(bool, size_t);

	SMTExpr createRealConst(const std::string& name);

	SMTExpr createRealVal(const std::string& name);

	SMTExpr createBitVecConst(const std::string& name, uint64_t sz);

	SMTExpr createBoolConst(const std::string& name);

	SMTExpr createTemporaryBitVecConst(uint64_t sz);

	SMTExpr createBitVecVal(const std::string& name, uint64_t sz);

	SMTExpr createBitVecVal(uint64_t val, uint64_t sz);

	SMTExpr createIntVal(int i);

	SMTExpr createSelect(SMTExpr& vec, SMTExpr index);

	SMTExpr createStore(SMTExpr& vec, SMTExpr index, SMTExpr& val2insert);

	SMTExpr createIntRealArrayConstFromStringSymbol(const std::string& name);

	SMTExpr createIntBvArrayConstFromStringSymbol(const std::string& name, uint64_t sz);

	SMTExpr createIntDomainConstantArray(SMTExpr& ElmtExpr);

	SMTExpr createBoolVal(bool b);
	SMTExpr createBoolVal(const std::string&);

	/// This function translate an SMTExprVec (the 1st parameter) created
	/// by other SMTFactory to the context of this SMTFactory.
	SMTExprVec translate(const SMTExprVec &);

	/// The variables in the constraints will be renamed using a suffix
	/// (the 2nd parameter).
	/// Since the symbols of the variables are renamed, we record the
	/// <old symbol, new expr> map in the 3rd parameter.
	///
	/// The last parameter is optional, which indicates when a expr should
	/// be pruned in the translated constraints. For examples, the original
	/// constraint is as following:
	///                 "x + y > z && (a || b) && !c",
	/// and suppose that "x" is the variable be pruned (in other words, we
	/// do not want to see the variable "x" in the translated constraint).
	/// In this example, "x + y > z" will be replaced by "true".
	///
	/// This function returns a std::pair, in which the first is the translated
	/// constraint, and the second indicates if some variables are pruned.
	std::pair<SMTExprVec, bool> rename(const SMTExprVec&, const std::string&, std::unordered_map<std::string, SMTExpr>&, SMTExprPruner* = nullptr);

	std::mutex& getFactoryLock() {
		return FactoryLock;
	}

	SMTExprVec parseSMTLib2File(const std::string& FileName);

private:
	typedef struct RenamingUtility {
		bool WillBePruned;
		SMTExpr AfterBeingPruned;
		SMTExprVec ToPrune;
		std::unordered_map<std::string, SMTExpr> SymbolMapping;
	} RenamingUtility;

	std::map<SMTExpr, RenamingUtility, SMTExprComparator> ExprRenamingCache;

	/// Utility for public function translate
	/// It visits all exprs in a ``big" expr.
	bool visit(SMTExpr&, std::unordered_map<std::string, SMTExpr>&, SMTExprVec&, std::map<SMTExpr, bool, SMTExprComparator>&, SMTExprPruner*);
};

#endif
