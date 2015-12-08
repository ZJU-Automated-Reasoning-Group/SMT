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

	SMTExpr bv12bool();

	SMTExpr bool2bv1();

	SMTExpr real2int();

	SMTExpr int2real();

	SMTExpr int2bv(unsigned sz);

	SMTExpr bv2int(bool isSigned);

	SMTExpr basic_zext(unsigned sz);

	SMTExpr basic_add(SMTExpr &b);

	SMTExpr basic_sub(SMTExpr &b);

	SMTExpr basic_mul(SMTExpr &b);

	SMTExpr basic_udiv(SMTExpr &b);

	SMTExpr basic_sdiv(SMTExpr &b);

	SMTExpr basic_urem(SMTExpr &b);

	SMTExpr basic_srem(SMTExpr &b);

	SMTExpr basic_shl(SMTExpr &b);

	SMTExpr basic_ashr(SMTExpr &b);

	SMTExpr basic_lshr(SMTExpr &b);

	SMTExpr basic_and(SMTExpr &b);

	SMTExpr basic_or(SMTExpr &b);

	SMTExpr basic_xor(SMTExpr &b);

	SMTExpr basic_eq(SMTExpr &b);

	SMTExpr basic_ne(SMTExpr &b);

	SMTExpr basic_ugt(SMTExpr &b);

	SMTExpr basic_uge(SMTExpr &b);

	SMTExpr basic_sgt(SMTExpr &b);

	SMTExpr basic_sge(SMTExpr &b);

	SMTExpr basic_ult(SMTExpr &b);

	SMTExpr basic_ule(SMTExpr &b);

	SMTExpr basic_slt(SMTExpr &b);

	SMTExpr basic_sle(SMTExpr &b);

	SMTExpr basic_extract(unsigned high, unsigned low);

	SMTExpr basic_sext(unsigned sz);

	SMTExpr basic_ite(SMTExpr& TBValue, SMTExpr& FBValue);

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

	void push_back(SMTExpr e);

	SMTExpr operator[](unsigned i) const;

	bool empty() const;

	SMTExprVec copy();

	/// *this = *this && v2
	void mergeWithAnd(const SMTExprVec& v2);

	static SMTExprVec merge(SMTExprVec v1, SMTExprVec v2);

	/// *this = *this || v2
	void mergeWithOr(const SMTExprVec& v2);

	/// From the vector create one expr to represent the AND result.
	/// Create  bool expr to represent this vector
	SMTExpr toAndExpr() const;

	SMTExpr toOrExpr() const;

	SMTFactory& getSMTFactory();

	friend class SMTFactory;
	friend class SMTSolver;
	friend class SMTExpr;

	friend llvm::raw_ostream & operator<<(llvm::raw_ostream& out, SMTExprVec vec);

	friend std::ostream & operator<<(std::ostream & out, SMTExprVec vec) {
		out << *vec.ExprVec;
		return out;
	}
};

#endif
