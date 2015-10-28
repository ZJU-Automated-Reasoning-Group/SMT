/**
 * Authors: Qingkai & Andy
 */

#ifndef UTILS_SMT_SMTEXPR_H
#define UTILS_SMT_SMTEXPR_H

#include <z3++.h>
#include <map>
#include <llvm/Support/raw_ostream.h>

class SMTExprVec;
class SMTExprComparator;

class SMTExpr {
private:
	z3::expr Expr;

	SMTExpr(z3::expr e) :
			Expr(e) {
	}

public:

	SMTExpr(SMTExpr const & e) :
			Expr(e.Expr) {
	}

	SMTExpr& operator=(const SMTExpr& e) {
		if (this != &e) {
			this->Expr = e.Expr;
		}
		return *this;
	}

	friend SMTExpr operator!(SMTExpr const & a);

	friend SMTExpr operator||(SMTExpr const & a, SMTExpr const & b) {
		if (a.isTrue() || b.isTrue()) {
			return a.Expr.ctx().bool_val(true);
		} else if (a.isFalse()) {
			return b;
		} else if (b.isFalse()) {
			return a;
		}

		return a.Expr || b.Expr;
	}

	friend SMTExpr operator||(SMTExpr const & a, bool b) {
		return a || SMTExpr(a.Expr.ctx().bool_val(b));
	}

	friend SMTExpr operator||(bool a, SMTExpr const & b) {
		return b || a;
	}

	friend SMTExpr operator&&(SMTExpr const & a, SMTExpr const & b) {
		if (a.isFalse() || b.isFalse()) {
			return a.Expr.ctx().bool_val(false);
		} else if (a.isTrue()) {
			return b;
		} else if (b.isTrue()) {
			return a;
		}

		return a.Expr && b.Expr;
	}

	friend SMTExpr operator&&(SMTExpr const & a, bool b) {
		return a && SMTExpr(a.Expr.ctx().bool_val(b));
	}

	friend SMTExpr operator&&(bool a, SMTExpr const & b) {
		return b && a;
	}

	friend SMTExpr operator==(SMTExpr const & a, SMTExpr const & b) {
		assert(a.isSameSort(b));
		return a.Expr == b.Expr;
	}

	friend SMTExpr operator==(SMTExpr const & a, int b) {
		return a.Expr == b;
	}

	friend SMTExpr operator==(int a, SMTExpr const & b) {
		return a == b.Expr;
	}

	friend SMTExpr operator!=(SMTExpr const & a, SMTExpr const & b) {
		return a.Expr != b.Expr;
	}

	friend SMTExpr operator!=(SMTExpr const & a, int b) {
		return a.Expr != b;
	}

	friend SMTExpr operator!=(int a, SMTExpr const & b) {
		return a != b.Expr;
	}

	friend SMTExpr operator+(SMTExpr const & a, SMTExpr const & b) {
		return a.Expr + b.Expr;
	}

	friend SMTExpr operator+(SMTExpr const & a, int b) {
		return a.Expr + b;
	}

	friend SMTExpr operator+(int a, SMTExpr const & b) {
		return a + b.Expr;
	}

	friend SMTExpr operator*(SMTExpr const & a, SMTExpr const & b) {
		return a.Expr * b.Expr;
	}

	friend SMTExpr operator*(SMTExpr const & a, int b) {
		return a.Expr * b;
	}

	friend SMTExpr operator*(int a, SMTExpr const & b) {
		return a * b.Expr;
	}

	friend SMTExpr operator/(SMTExpr const & a, SMTExpr const & b) {
		return a.Expr / b.Expr;
	}

	friend SMTExpr operator/(SMTExpr const & a, int b) {
		return a.Expr / b;
	}

	friend SMTExpr operator/(int a, SMTExpr const & b) {
		return a / b.Expr;
	}

	friend SMTExpr operator-(SMTExpr const & a, SMTExpr const & b) {
		return a.Expr - b.Expr;
	}

	friend SMTExpr operator-(SMTExpr const & a, int b) {
		return a.Expr - b;
	}

	friend SMTExpr operator-(int a, SMTExpr const & b) {
		return a - b.Expr;
	}

	friend SMTExpr operator-(SMTExpr const & a) {
		return -a.Expr;
	}

	friend SMTExpr operator<=(SMTExpr const & a, SMTExpr const & b) {
		return a.Expr <= b.Expr;
	}

	friend SMTExpr operator<=(SMTExpr const & a, int b) {
		return a.Expr <= b;
	}

	friend SMTExpr operator<=(int a, SMTExpr const & b) {
		return a <= b.Expr;
	}

	friend SMTExpr operator<(SMTExpr const & a, SMTExpr const & b) {
		return a.Expr < b.Expr;
	}

	friend SMTExpr operator<(SMTExpr const & a, int b) {
		return a.Expr < b;
	}

	friend SMTExpr operator<(int a, SMTExpr const & b) {
		return a < b.Expr;
	}

	friend SMTExpr operator>=(SMTExpr const & a, SMTExpr const & b) {
		return a.Expr >= b.Expr;
	}

	friend SMTExpr operator>=(SMTExpr const & a, int b) {
		return a.Expr >= b;
	}

	friend SMTExpr operator>=(int a, SMTExpr const & b) {
		return a >= b.Expr;
	}

	friend SMTExpr operator>(SMTExpr const & a, SMTExpr const & b) {
		return a.Expr > b.Expr;
	}

	friend SMTExpr operator>(SMTExpr const & a, int b) {
		return a.Expr > b;
	}

	friend SMTExpr operator>(int a, SMTExpr const & b) {
		return a > b.Expr;
	}

	friend SMTExpr operator&(SMTExpr const & a, SMTExpr const & b) {
		return a.Expr & b.Expr;
	}

	friend SMTExpr operator&(SMTExpr const & a, int b) {
		return a.Expr & b;
	}

	friend SMTExpr operator&(int a, SMTExpr const & b) {
		return a & b.Expr;
	}

	friend SMTExpr operator^(SMTExpr const & a, SMTExpr const & b) {
		return a.Expr ^ b.Expr;
	}

	friend SMTExpr operator^(SMTExpr const & a, int b) {
		return a.Expr ^ b;
	}

	friend SMTExpr operator^(int a, SMTExpr const & b) {
		return a ^ b.Expr;
	}

	friend SMTExpr operator|(SMTExpr const & a, SMTExpr const & b) {
		return a.Expr | b.Expr;
	}

	friend SMTExpr operator|(SMTExpr const & a, int b) {
		return a.Expr | b;
	}

	friend SMTExpr operator|(int a, SMTExpr const & b) {
		return a | b.Expr;
	}

	friend SMTExpr operator~(SMTExpr const & a) {
		return ~a.Expr;
	}

	friend llvm::raw_ostream&
	operator<<(llvm::raw_ostream& out, SMTExpr n);

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

	SMTExpr getQuantifierBody() const {
		return Expr.body();
	}

	bool isVar() const {
		return Expr.is_var();
	}

	unsigned numArgs() const {
		return Expr.num_args();
	}

	SMTExpr getArg(unsigned i) const {
		return Expr.arg(i);
	}

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

	SMTExpr bv12bool() {
		if (Expr.is_array()) {
			assert(Expr.get_sort().array_range().is_bv());
			unsigned bvSz = Expr.get_sort().array_range().bv_size();
			assert(bvSz == 1);
			auto func = (Expr.ctx().bv_val("0", 1) == Expr.ctx().bv_val("0", 1)).decl();

			z3::expr const_bv1 = const_array(Expr.get_sort().array_domain(), Expr.ctx().bv_val("1", 1));
			Z3_ast mapargs[2] = { Expr, const_bv1 };

			return z3::expr(Expr.ctx(), Z3_mk_map(Expr.ctx(), func, 2, mapargs));
		} else {
			assert(Expr.is_bv() && Expr.get_sort().bv_size() == 1);
			return Expr == Expr.ctx().bv_val(1, 1);
		}
	}

	SMTExpr bool2bv1() {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array()) {
			assert(Expr.get_sort().array_range().is_bool());
			auto func = ite(ctx.bool_val(false), ctx.bv_val(1, 1), ctx.bv_val(0, 1)).decl();

			z3::expr const_bv0 = const_array(Expr.get_sort().array_domain(), ctx.bv_val(0, 1));
			z3::expr const_bv1 = const_array(Expr.get_sort().array_domain(), ctx.bv_val(1, 1));

			Z3_ast mapargs[3] = { Expr, const_bv0, const_bv1 };

			z3::expr bvret(ctx, Z3_mk_map(ctx, func, 3, mapargs));
			return bvret;
		} else {
			assert(Expr.is_bool());
			return ite(Expr, ctx.bv_val(1, 1), ctx.bv_val(0, 1));
		}
	}

	SMTExpr real2int() {
		z3::context & ctx = Expr.ctx();
		if (Expr.is_array()) {
			assert(Expr.get_sort().array_range().is_real());
			auto func = z3::expr(ctx, Z3_mk_real2int(ctx, ctx.real_val("0.0"))).decl();

			Z3_ast mapargs[1] = { Expr };
			return z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs));
		} else {
			assert(Expr.is_real());
			return z3::expr(ctx, Z3_mk_real2int(ctx, Expr));
		}
	}

	SMTExpr int2real() {
		z3::context & ctx = Expr.ctx();
		if (Expr.is_array()) {
			assert(Expr.get_sort().array_range().is_int());

			auto func = to_real(Expr).decl();
			Z3_ast mapargs[1] = { Expr };
			return z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs));
		} else {
			assert(Expr.is_int());
			return to_real(Expr);
		}
	}

	SMTExpr int2bv(unsigned sz) {
		z3::context & ctx = Expr.ctx();
		if (Expr.is_array()) {
			assert(Expr.get_sort().array_range().is_int());
			auto func = z3::expr(ctx, Z3_mk_int2bv(ctx, sz, ctx.int_val("0"))).decl();

			Z3_ast mapargs[1] = { Expr };
			return z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs));
		} else {
			assert(Expr.is_int());
			return z3::expr(ctx, Z3_mk_int2bv(ctx, sz, Expr));
		}
	}

	SMTExpr bv2int(bool isSigned) {
		z3::context & ctx = Expr.ctx();
		if (Expr.is_array()) {
			assert(Expr.get_sort().array_range().is_bv());
			unsigned bvSz = Expr.get_sort().array_range().bv_size();
			auto func = z3::expr(ctx, Z3_mk_bv2int(ctx, ctx.bv_val("0", bvSz), isSigned)).decl();

			Z3_ast mapargs[1] = { Expr };
			return z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs));
		} else {
			assert(Expr.is_bv());
			return z3::expr(ctx, Z3_mk_bv2int(ctx, Expr, isSigned));
		}
	}

	SMTExpr basic_zext(unsigned sz) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array()) {
			assert(Expr.get_sort().array_range().is_bv());
			unsigned bvSz = Expr.get_sort().array_range().bv_size();
			auto func = z3::expr(ctx, Z3_mk_zero_ext(ctx, sz, ctx.bv_val("10", bvSz))).decl();
			Z3_ast mapargs[1] = { Expr };
			return z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs));
		} else {
			assert(Expr.is_bv());
			return z3::expr(ctx, Z3_mk_zero_ext(ctx, sz, Expr));
		}
	}

	SMTExpr basic_add(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) + ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") + ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (Expr + b.Expr);
		}
	}

	SMTExpr basic_sub(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) - ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") - ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (Expr - b.Expr);
		}
	}

	SMTExpr basic_mul(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) * ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") * ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (Expr * b.Expr);
		}
	}

	SMTExpr basic_udiv(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvudiv(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return z3::expr(ctx, Z3_mk_bvudiv(ctx, Expr, b.Expr));
		}
	}

	SMTExpr basic_sdiv(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) / ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") / ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (Expr / b.Expr);
		}
	}

	SMTExpr basic_urem(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvurem(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return z3::expr(ctx, Z3_mk_bvurem(ctx, Expr, b.Expr));
		}
	}

	SMTExpr basic_srem(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvsrem(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else if (Expr.is_real() && b.Expr.is_real()) {
			// z3 only accept rem between integers
			z3::expr ch1ToInt = z3::expr(ctx, Z3_mk_real2int(ctx, Expr));
			z3::expr ch2ToInt = z3::expr(ctx, Z3_mk_real2int(ctx, b.Expr));
			z3::expr remResult = z3::expr(ctx, Z3_mk_rem(ctx, ch1ToInt, ch2ToInt));
			z3::expr remResult2Real = z3::expr(ctx, Z3_mk_int2real(ctx, remResult));
			return remResult2Real;
		} else {
			return z3::expr(ctx, Z3_mk_bvsrem(ctx, Expr, b.Expr));
		}
	}

	SMTExpr basic_shl(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvshl(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return z3::expr(ctx, Z3_mk_bvshl(ctx, Expr, b.Expr));
		}
	}

	SMTExpr basic_ashr(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvashr(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return z3::expr(ctx, Z3_mk_bvashr(ctx, Expr, b.Expr));
		}
	}

	SMTExpr basic_lshr(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvlshr(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return z3::expr(ctx, Z3_mk_bvlshr(ctx, Expr, b.Expr));
		}
	}

	SMTExpr basic_and(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) & ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") & ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (Expr & b.Expr);
		}
	}

	SMTExpr basic_or(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) | ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") | ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (Expr | b.Expr);
		}
	}

	SMTExpr basic_xor(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) ^ ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") ^ ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (Expr ^ b.Expr);
		}
	}

	SMTExpr basic_eq(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) == ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") == ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (Expr == b.Expr);
		}
	}

	SMTExpr basic_ne(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) != ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") != ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (Expr != b.Expr);
		}
	}

	SMTExpr basic_ugt(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvugt(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return z3::expr(ctx, Z3_mk_bvugt(ctx, Expr, b.Expr));
		}
	}

	SMTExpr basic_uge(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvuge(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return z3::expr(ctx, Z3_mk_bvuge(ctx, Expr, b.Expr));
		}
	}

	SMTExpr basic_sgt(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) > ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") > ctx.real_val("0.0")).decl();
			}

			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (Expr > b.Expr);
		}
	}

	SMTExpr basic_sge(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) >= ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") >= ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (Expr >= b.Expr);
		}
	}

	SMTExpr basic_ult(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvult(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return z3::expr(ctx, Z3_mk_bvult(ctx, Expr, b.Expr));
		}
	}

	SMTExpr basic_ule(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvule(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return z3::expr(ctx, Z3_mk_bvule(ctx, Expr, b.Expr));
		}
	}

	SMTExpr basic_slt(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) < ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") < ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (Expr < b.Expr);
		}
	}

	SMTExpr basic_sle(SMTExpr &b) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array() && b.Expr.is_array()) {
			Z3_ast const mapargs[2] = { Expr, b.Expr };
			z3::func_decl func(ctx);
			if (Expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = Expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) <= ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") <= ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (Expr <= b.Expr);
		}
	}

	SMTExpr basic_extract(unsigned high, unsigned low) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array()) {
			assert(Expr.get_sort().array_range().is_bv());
			unsigned bvSz = Expr.get_sort().array_range().bv_size();
			auto func = z3::expr(ctx, Z3_mk_extract(ctx, high, low, ctx.bv_val("10", bvSz))).decl();
			Z3_ast mapargs[1] = { Expr };
			return z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs));
		} else {
			assert(Expr.is_bv());
			return z3::expr(ctx, Z3_mk_extract(ctx, high, low, Expr));
		}
	}

	SMTExpr basic_sext(unsigned sz) {
		z3::context& ctx = Expr.ctx();
		if (Expr.is_array()) {
			assert(Expr.get_sort().array_range().is_bv());
			unsigned bvSz = Expr.get_sort().array_range().bv_size();
			auto func = z3::expr(ctx, Z3_mk_sign_ext(ctx, sz, ctx.bv_val("10", bvSz))).decl();
			Z3_ast mapargs[1] = { Expr };
			return z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs));
		} else {
			assert(Expr.is_bv());
			return z3::expr(ctx, Z3_mk_sign_ext(ctx, sz, Expr));
		}
	}

	SMTExpr basic_ite(SMTExpr& TBValue, SMTExpr& FBValue) {
		z3::context& ctx = Expr.ctx();
		z3::expr& condition = Expr;
		if (condition.is_array()) {
			assert(TBValue.Expr.is_array() && FBValue.Expr.is_array());

			Z3_ast mapargs[3] = { condition, TBValue.Expr, FBValue.Expr };
			z3::func_decl func(ctx);
			if (TBValue.Expr.get_sort().array_range().is_bv()) {
				assert(FBValue.Expr.get_sort().array_range().is_bv());
				unsigned bvSz = TBValue.Expr.get_sort().array_range().bv_size();
				func = ite(ctx.bool_val(true), ctx.bv_val(1, bvSz), ctx.bv_val(0, bvSz)).decl();
			} else {
				assert(TBValue.Expr.get_sort().array_range().is_real());
				assert(FBValue.Expr.get_sort().array_range().is_real());
				func = ite(ctx.bool_val(true), ctx.real_val("0.0"), ctx.real_val("0.0")).decl();
			}

			z3::expr mapped(ctx, Z3_mk_map(ctx, func, 3, mapargs));
			return z3::expr(ctx, Z3_mk_map(ctx, func, 3, mapargs));
		} else {
			return ite(condition, TBValue.Expr, FBValue.Expr);
		}
	}

	SMTExpr localSimplify();

	SMTExpr dilligSimplify();

	unsigned size(std::map<SMTExpr, unsigned, SMTExprComparator>&);

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

class SMTExprVec {
private:
	z3::expr_vector ExprVec;

	SMTExprVec(z3::expr_vector Vec) :
			ExprVec(Vec) {
	}

public:
	SMTExprVec(SMTExprVec const &Vec) :
			ExprVec(Vec.ExprVec) {
	}

	SMTExprVec& operator=(const SMTExprVec& Vec) {
		if (this != &Vec) {
			this->ExprVec = Vec.ExprVec;
		}
		return *this;
	}

	unsigned size() const {
		return ExprVec.size();
	}

	unsigned constraintSize() const;

	void push_back(SMTExpr e) {
		if (e.isTrue()) {
			return;
		}
		ExprVec.push_back(e.Expr);
	}

	SMTExpr operator[](int i) const {
		return ExprVec[i];
	}

	void clear() {
		ExprVec = z3::expr_vector(ExprVec.ctx());
	}

	bool empty() const {
		return ExprVec.empty();
	}

	SMTExprVec copy() {
		z3::expr_vector Ret = z3::expr_vector(ExprVec.ctx());
		Ret.resize(ExprVec.size());
		for (unsigned Idx = 0; Idx < ExprVec.size(); Idx++) {
			Z3_ast_vector_set(ExprVec.ctx(), Ret, Idx, ExprVec[Idx]);
		}
		return Ret;
	}

	/// *this = *this && v2
	void mergeWithAnd(const SMTExprVec& v2) {
		for (size_t i = 0; i < v2.size(); i++) {
			ExprVec.push_back(v2.ExprVec[i]);
		}
	}

	static SMTExprVec merge(SMTExprVec v1, SMTExprVec v2) {
		if(v1.size() < v2.size()) {
			v2.mergeWithAnd(v1);
			return v2;
		} else {
			v1.mergeWithAnd(v2);
			return v1;
		}
	}

	/// *this = *this || v2
	void mergeWithOr(const SMTExprVec& v2) {
		if (v2.empty())
			return;

		if (empty()) {
			ExprVec = v2.ExprVec;
			return;
		}

		SMTExpr e1 = this->toAndExpr();
		SMTExpr e2 = v2.toAndExpr();

		z3::expr_vector ret(ExprVec.ctx());
		ret.push_back(e1.Expr || e2.Expr);
		ExprVec = ret;
	}

	/// From the vector create one expr to represent the AND result.
	/// Create  bool expr to represent this vector
	SMTExpr toAndExpr() const;

	SMTExpr toOrExpr() const;

	friend class SMTFactory;
	friend class SMTSolver;
	friend class SMTExpr;

	friend llvm::raw_ostream &
	operator<<(llvm::raw_ostream& out, SMTExprVec vec);

	friend std::ostream & operator<<(std::ostream & out, SMTExprVec vec) {
		out << vec.ExprVec;
		return out;
	}
};

#endif
