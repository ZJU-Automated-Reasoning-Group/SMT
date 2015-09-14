/**
 * Authors: Qingkai & Andy
 */

#ifndef UTILS_SMT_SMTFACTORY_H
#define UTILS_SMT_SMTFACTORY_H

#include <z3++.h>

#include "SMTExpr.h"
#include "SMTModel.h"
#include "SMTSolver.h"
#include "Utils/stringutil.h"

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
 */
class SMTFactory {
private:
	z3::context ctx;

	unsigned TempSMTVaraibleIndex;

	std::map<SMTExpr, std::map<std::string, SMTExpr>, SMTExprComparator> CachedSymbolConstMap;

	std::map<SMTExpr, SMTExpr, SMTExprComparator> CachedPrunedExprMap;

public:
	SMTFactory() :
			TempSMTVaraibleIndex(0) {
	}

	~SMTFactory() {
	}

	SMTSolver createSMTSolver();

	SMTExpr createEmptySMTExpr() {
		return (SMTExpr) z3::expr(ctx);
	}

	SMTExprVec createEmptySMTExprVec() {
		return (SMTExprVec) z3::expr_vector(ctx);
	}

	SMTExprVec createBoolSMTExprVec(bool, size_t);

	SMTExpr createRealConst(std::string name) {
		return ctx.real_const(name.c_str());
	}

	SMTExpr createRealVal(std::string name) {
		return ctx.real_val(name.c_str());
	}

	SMTExpr createBitVecConst(std::string name, uint64_t sz) {
		return ctx.bv_const(name.c_str(), sz);
	}

	SMTExpr createTemporaryBitVecConst(uint64_t sz) {
		std::string symbol;
		format_str(symbol, "temp_%u", TempSMTVaraibleIndex++);
		return ctx.bv_const(symbol.c_str(), sz);
	}

	SMTExpr createBitVecVal(std::string name, uint64_t sz) {
		return ctx.bv_val(name.c_str(), sz);
	}

	SMTExpr createBitVecVal(uint64_t val, uint64_t sz) {
		return ctx.bv_val((__uint64 ) val, sz);
	}

	SMTExpr createIntVal(int i) {
		return ctx.int_val(i);
	}

	SMTExpr createSelect(SMTExpr& vec, SMTExpr index) {
		return z3::expr(ctx, Z3_mk_select(ctx, vec.z3_expr, index.z3_expr));
	}

	SMTExpr createStore(SMTExpr& vec, SMTExpr index, SMTExpr& val2insert) {
		return z3::expr(ctx, Z3_mk_store(ctx, vec.z3_expr, index.z3_expr, val2insert.z3_expr));
	}

	SMTExpr createIntRealArrayConstFromStringSymbol(std::string& name) {
		Z3_sort array_sort = ctx.array_sort(ctx.int_sort(), ctx.real_sort());
		return z3::expr(ctx, Z3_mk_const(ctx, Z3_mk_string_symbol(ctx, name.c_str()), array_sort));
	}

	SMTExpr createIntBvArrayConstFromStringSymbol(std::string& name, uint64_t sz) {
		Z3_sort array_sort = ctx.array_sort(ctx.int_sort(), ctx.bv_sort(sz));
		return z3::expr(ctx, Z3_mk_const(ctx, Z3_mk_string_symbol(ctx, name.c_str()), array_sort));
	}

	SMTExpr createIntDomainConstantArray(SMTExpr& ElmtExpr) {
		return z3::expr(ctx, Z3_mk_const_array(ctx, ctx.int_sort(), ElmtExpr.z3_expr));
	}

	SMTExpr createBoolVal(bool b) {
		return ctx.bool_val(b);
	}

	void releaseCache() {
		CachedSymbolConstMap.clear();
		CachedPrunedExprMap.clear();
	}

	/// This function translate an SMTExprVec (the 1st parameter) created
	/// by other SMTFactory to the context of this SMTFactory. The variables
	/// in the constraints will be renamed using a suffix (the 2nd parameter).
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
	std::pair<SMTExprVec, bool> translate(const SMTExprVec&, const std::string&, std::map<std::string, SMTExpr>&, SMTExprPruner* = nullptr);

private:
	/// Utility for public function translate
	/// It visits all exprs in a ``big" expr.
	bool visit(SMTExpr&, std::map<std::string, SMTExpr>&, SMTExprVec&, std::map<SMTExpr, bool, SMTExprComparator>&, SMTExprPruner*);
};

#endif
