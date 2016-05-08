/**
 * Authors: Qingkai & Andy
 */

#ifndef UTILS_SMT_SMTEXPR_H
#define UTILS_SMT_SMTEXPR_H

#include <z3++.h>
#include <map>
#include <iostream>
#include <memory>
#include <llvm/Support/raw_ostream.h>

class SMTFactory;
class SMTExprVec;
class SMTExprComparator;

class SMTExpr {
private:
	z3::expr Expr;
	SMTFactory* Factory;

	SMTExpr(SMTFactory* F, z3::expr e);

public:

	SMTExpr(SMTExpr const & e);

	SMTExpr& operator=(const SMTExpr& e);

	friend SMTExpr operator!(SMTExpr const & a);

	friend SMTExpr operator||(SMTExpr const & a, SMTExpr const & b);

	friend SMTExpr operator||(SMTExpr const & a, bool b);

	friend SMTExpr operator||(bool a, SMTExpr const & b);

	friend SMTExpr operator&&(SMTExpr const & a, SMTExpr const & b);

	friend SMTExpr operator&&(SMTExpr const & a, bool b);

	friend SMTExpr operator&&(bool a, SMTExpr const & b);

	friend SMTExpr operator==(SMTExpr const & a, SMTExpr const & b);

	friend SMTExpr operator==(SMTExpr const & a, int b);

	friend SMTExpr operator==(int a, SMTExpr const & b);

	friend SMTExpr operator!=(SMTExpr const & a, SMTExpr const & b);

	friend SMTExpr operator!=(SMTExpr const & a, int b);

	friend SMTExpr operator!=(int a, SMTExpr const & b);

	friend SMTExpr operator+(SMTExpr const & a, SMTExpr const & b);

	friend SMTExpr operator+(SMTExpr const & a, int b);

	friend SMTExpr operator+(int a, SMTExpr const & b);

	friend SMTExpr operator*(SMTExpr const & a, SMTExpr const & b);

	friend SMTExpr operator*(SMTExpr const & a, int b);

	friend SMTExpr operator*(int a, SMTExpr const & b);

	friend SMTExpr operator/(SMTExpr const & a, SMTExpr const & b);

	friend SMTExpr operator/(SMTExpr const & a, int b);

	friend SMTExpr operator/(int a, SMTExpr const & b);

	friend SMTExpr operator-(SMTExpr const & a, SMTExpr const & b);

	friend SMTExpr operator-(SMTExpr const & a, int b);

	friend SMTExpr operator-(int a, SMTExpr const & b);

	friend SMTExpr operator-(SMTExpr const & a);

	friend SMTExpr operator<=(SMTExpr const & a, SMTExpr const & b);

	friend SMTExpr operator<=(SMTExpr const & a, int b);

	friend SMTExpr operator<=(int a, SMTExpr const & b);

	friend SMTExpr operator<(SMTExpr const & a, SMTExpr const & b);

	friend SMTExpr operator<(SMTExpr const & a, int b);

	friend SMTExpr operator<(int a, SMTExpr const & b);

	friend SMTExpr operator>=(SMTExpr const & a, SMTExpr const & b);

	friend SMTExpr operator>=(SMTExpr const & a, int b);

	friend SMTExpr operator>=(int a, SMTExpr const & b);

	friend SMTExpr operator>(SMTExpr const & a, SMTExpr const & b);

	friend SMTExpr operator>(SMTExpr const & a, int b);

	friend SMTExpr operator>(int a, SMTExpr const & b);

	friend SMTExpr operator&(SMTExpr const & a, SMTExpr const & b);

	friend SMTExpr operator&(SMTExpr const & a, int b);

	friend SMTExpr operator&(int a, SMTExpr const & b);

	friend SMTExpr operator^(SMTExpr const & a, SMTExpr const & b);

	friend SMTExpr operator^(SMTExpr const & a, int b);

	friend SMTExpr operator^(int a, SMTExpr const & b);

	friend SMTExpr operator|(SMTExpr const & a, SMTExpr const & b);

	friend SMTExpr operator|(SMTExpr const & a, int b);

	friend SMTExpr operator|(int a, SMTExpr const & b);

	friend SMTExpr operator~(SMTExpr const & a);

	friend llvm::raw_ostream& operator<<(llvm::raw_ostream& out, SMTExpr n);

	friend std::ostream & operator<<(std::ostream & out, SMTExpr const & n) {
		out << n.Expr;
		return out;
	}

	bool isSameSort(SMTExpr const & b) const {
		return z3::eq(Expr.get_sort(), b.Expr.get_sort());
	}

	bool isArray() {
		return Expr.is_array();
	}

	bool isBitVector() {
		return Expr.is_bv();
	}

	bool isReal() {
		return Expr.is_real();
	}

	bool isBool() {
		return Expr.is_bool();
	}

	bool isBvArray() {
		if (Expr.is_array()) {
			return Expr.get_sort().array_range().is_bv();
		}
		return false;
	}

	bool isConst() const {
		return Expr.is_const();
	}

	bool isApp() const {
		return Expr.is_app();
	}

	bool isNumeral() const {
		return Expr.is_numeral();
	}

	bool isQuantifier() const {
		return Expr.is_quantifier();
	}

	SMTExpr getQuantifierBody() const;

	bool isVar() const {
		return Expr.is_var();
	}

	unsigned numArgs() const {
		return Expr.num_args();
	}

	SMTExpr getArg(unsigned i) const;

	unsigned getBitVecSize() {
		return Expr.get_sort().bv_size();
	}

	bool isLogicAnd() const {
		return Expr.decl().decl_kind() == Z3_OP_AND;
	}

	bool isLogicOr() const {
		return Expr.decl().decl_kind() == Z3_OP_OR;
	}

	bool isLogicNot() const {
		return Expr.decl().decl_kind() == Z3_OP_NOT;
	}

	bool isTrue() const {
		return Expr.decl().decl_kind() == Z3_OP_TRUE;
	}

	bool isFalse() const {
		return Expr.decl().decl_kind() == Z3_OP_FALSE;
	}

	bool equals(const SMTExpr& e) const {
		return z3::eq(Expr, e.Expr);
	}

	std::string getSymbol() const {
		return Z3_ast_to_string(Expr.ctx(), Expr);
	}

	SMTExpr substitute(SMTExprVec& From, SMTExprVec& To);

	/*==-- basic operations --==*/

	SMTExpr bv12bool();

	SMTExpr bool2bv1();

	SMTExpr real2int();

	SMTExpr int2real();

	SMTExpr int2bv(unsigned Sz);

	SMTExpr bv2int(bool IsSigned);

#define DECLARE_BASIC_OPERATION(X) \
	SMTExpr basic_##X(SMTExpr &Expr);

	DECLARE_BASIC_OPERATION(add)
	DECLARE_BASIC_OPERATION(sub)
	DECLARE_BASIC_OPERATION(mul)
	DECLARE_BASIC_OPERATION(udiv)
	DECLARE_BASIC_OPERATION(sdiv)
	DECLARE_BASIC_OPERATION(urem)
	DECLARE_BASIC_OPERATION(srem)
	DECLARE_BASIC_OPERATION(shl)
	DECLARE_BASIC_OPERATION(ashr)
	DECLARE_BASIC_OPERATION(lshr)
	DECLARE_BASIC_OPERATION(and)
	DECLARE_BASIC_OPERATION(or)
	DECLARE_BASIC_OPERATION(xor)
	DECLARE_BASIC_OPERATION(concat)

	DECLARE_BASIC_OPERATION(eq)
	DECLARE_BASIC_OPERATION(ne)
	DECLARE_BASIC_OPERATION(ugt)
	DECLARE_BASIC_OPERATION(uge)
	DECLARE_BASIC_OPERATION(sgt)
	DECLARE_BASIC_OPERATION(sge)
	DECLARE_BASIC_OPERATION(ult)
	DECLARE_BASIC_OPERATION(ule)
	DECLARE_BASIC_OPERATION(slt)
	DECLARE_BASIC_OPERATION(sle)

	SMTExpr basic_extract(unsigned High, unsigned Low);

	SMTExpr basic_zext(unsigned Sz);

	SMTExpr basic_sext(unsigned Sz);

	SMTExpr basic_ite(SMTExpr& TBValue, SMTExpr& FBValue);

	/*==-- array operations --==*/
	SMTExpr array_elmt(unsigned ElmtNum, unsigned Index);

#define DECLARE_ARRAY_OPERATION(X) \
	SMTExpr array_##X(SMTExpr &BvAsArray, unsigned ElmtNum);

	DECLARE_ARRAY_OPERATION(sgt)
	DECLARE_ARRAY_OPERATION(sge)
	DECLARE_ARRAY_OPERATION(ugt)
	DECLARE_ARRAY_OPERATION(uge)
	DECLARE_ARRAY_OPERATION(slt)
	DECLARE_ARRAY_OPERATION(sle)
	DECLARE_ARRAY_OPERATION(ule)
	DECLARE_ARRAY_OPERATION(ult)
	DECLARE_ARRAY_OPERATION(eq)
	DECLARE_ARRAY_OPERATION(ne)

	DECLARE_ARRAY_OPERATION(add)
	DECLARE_ARRAY_OPERATION(sub)
	DECLARE_ARRAY_OPERATION(mul)
	DECLARE_ARRAY_OPERATION(udiv)
	DECLARE_ARRAY_OPERATION(sdiv)
	DECLARE_ARRAY_OPERATION(urem)
	DECLARE_ARRAY_OPERATION(srem)
	DECLARE_ARRAY_OPERATION(shl)
	DECLARE_ARRAY_OPERATION(ashr)
	DECLARE_ARRAY_OPERATION(lshr)
	DECLARE_ARRAY_OPERATION(and)
	DECLARE_ARRAY_OPERATION(or)
	DECLARE_ARRAY_OPERATION(xor)

	SMTExpr array_ite(SMTExpr& TBValue, SMTExpr& FBValue, unsigned ElmtNum);

	// Sz: size to extend
	SMTExpr array_sext(unsigned Sz, unsigned ElmtNum);

	// Sz: size to extend
	SMTExpr array_zext(unsigned Sz, unsigned ElmtNum);

	// Sz: size to trunc
	SMTExpr array_trunc(unsigned Sz, unsigned ElmtNum);

	/*==-- simplifications --==*/
	SMTExpr localSimplify();

	SMTExpr dilligSimplify();

	unsigned size(std::map<SMTExpr, unsigned, SMTExprComparator>&);

	SMTFactory& getSMTFactory();

	friend class SMTFactory;
	friend class SMTSolver;
	friend class SMTExprVec;
	friend class SimplificationUtil;
	friend class SMTExprComparator;

private:
	SMTExpr dilligSimplify(SMTExpr N, z3::solver& Solver4Sim, z3::context& Ctx);

};

// This can be used as the comparator of a stl container,
// e.g. SMTExprComparator comparator; map<SMTExpr, value, comparator> ...;
class SMTExprComparator {
public:
	bool operator()(const SMTExpr& X, const SMTExpr& Y) const {
		unsigned XId = Z3_get_ast_id(X.Expr.ctx(), X.Expr);
		unsigned YId = Z3_get_ast_id(Y.Expr.ctx(), Y.Expr);
		return XId < YId;
	}
};

/**
 * NOTE: when SMTExprVec.empty(), the copy constructor
 * and = operator have the copy semantics. Otherwise,
 * they have move semantics.
 */
class SMTExprVec {
private:
	SMTFactory* Factory;
	std::shared_ptr<z3::expr_vector> ExprVec;

	SMTExprVec(SMTFactory* F, std::shared_ptr<z3::expr_vector> Vec);

public:
	SMTExprVec(SMTExprVec const &Vec);

	SMTExprVec& operator=(const SMTExprVec& Vec);

	unsigned size() const;

	unsigned constraintSize() const;

	// If Enforce is set, all exprs will be added to the vector,
	// otherwise, "true" will be filtered
	void push_back(SMTExpr Expr, bool Enforce = false);

	SMTExpr operator[](unsigned I) const;

	bool empty() const;

	SMTExprVec copy();

	/// *this = *this && v2
	void mergeWithAnd(const SMTExprVec& Vec);

	static SMTExprVec merge(SMTExprVec Vec1, SMTExprVec Vec2);

	/// *this = *this || v2
	void mergeWithOr(const SMTExprVec& Vec);

	/// From the vector create one expr to represent the AND result.
	/// Create  bool expr to represent this vector
	SMTExpr toAndExpr() const;

	SMTExpr toOrExpr() const;

	SMTFactory& getSMTFactory() const;

	friend class SMTFactory;
	friend class SMTSolver;
	friend class SMTExpr;

	friend llvm::raw_ostream & operator<<(llvm::raw_ostream& Out, SMTExprVec Vec);

	friend std::ostream & operator<<(std::ostream& Out, SMTExprVec Vec) {
		if (Vec.ExprVec.get() == nullptr) {
			Out << "(empty vector)";
			return Out;
		}
		Out << *Vec.ExprVec;
		return Out;
	}
};

#endif
