/**
 * Authors: Qingkai & Andy
 */

#ifndef UTILS_SMT_SMTEXPR_H
#define UTILS_SMT_SMTEXPR_H

#include <z3++.h>
#include <map>

class SMTExprVec;
class SMTExprComparator;

class SMTExpr {
private:
	z3::expr z3_expr;

	SMTExpr(z3::expr e) :
			z3_expr(e) {
	}

public:

	SMTExpr(SMTExpr const & e) :
			z3_expr(e.z3_expr) {
	}

	SMTExpr& operator=(const SMTExpr& e) {
		if (this != &e) {
			this->z3_expr = e.z3_expr;
		}
		return *this;
	}

	friend SMTExpr operator!(SMTExpr const & a);

	friend SMTExpr operator||(SMTExpr const & a, SMTExpr const & b) {
		if (a.equals(a.z3_expr.ctx().bool_val(true)) || b.equals(a.z3_expr.ctx().bool_val(true))) {
			return a.z3_expr.ctx().bool_val(true);
		} else if (a.equals(a.z3_expr.ctx().bool_val(false))) {
			return b;
		} else if (b.equals(a.z3_expr.ctx().bool_val(false))) {
			return a;
		}

		return a.z3_expr || b.z3_expr;
	}

	friend SMTExpr operator||(SMTExpr const & a, bool b) {
		return a || SMTExpr(a.z3_expr.ctx().bool_val(b));
	}

	friend SMTExpr operator||(bool a, SMTExpr const & b) {
		return b || a;
	}

	friend SMTExpr operator&&(SMTExpr const & a, SMTExpr const & b) {
		if (a.equals(a.z3_expr.ctx().bool_val(false)) || b.equals(a.z3_expr.ctx().bool_val(false))) {
			return a.z3_expr.ctx().bool_val(false);
		} else if (a.equals(a.z3_expr.ctx().bool_val(true))) {
			return b;
		} else if (b.equals(a.z3_expr.ctx().bool_val(true))) {
			return a;
		}

		return a.z3_expr && b.z3_expr;
	}

	friend SMTExpr operator&&(SMTExpr const & a, bool b) {
		return a && SMTExpr(a.z3_expr.ctx().bool_val(b));
	}

	friend SMTExpr operator&&(bool a, SMTExpr const & b) {
		return b && a;
	}

	friend SMTExpr operator==(SMTExpr const & a, SMTExpr const & b) {
		assert(a.isSameSort(b));
		return a.z3_expr == b.z3_expr;
	}

	friend SMTExpr operator==(SMTExpr const & a, int b) {
		return a.z3_expr == b;
	}

	friend SMTExpr operator==(int a, SMTExpr const & b) {
		return a == b.z3_expr;
	}

	friend SMTExpr operator!=(SMTExpr const & a, SMTExpr const & b) {
		return a.z3_expr != b.z3_expr;
	}

	friend SMTExpr operator!=(SMTExpr const & a, int b) {
		return a.z3_expr != b;
	}

	friend SMTExpr operator!=(int a, SMTExpr const & b) {
		return a != b.z3_expr;
	}

	friend SMTExpr operator+(SMTExpr const & a, SMTExpr const & b) {
		return a.z3_expr + b.z3_expr;
	}

	friend SMTExpr operator+(SMTExpr const & a, int b) {
		return a.z3_expr + b;
	}

	friend SMTExpr operator+(int a, SMTExpr const & b) {
		return a + b.z3_expr;
	}

	friend SMTExpr operator*(SMTExpr const & a, SMTExpr const & b) {
		return a.z3_expr * b.z3_expr;
	}

	friend SMTExpr operator*(SMTExpr const & a, int b) {
		return a.z3_expr * b;
	}

	friend SMTExpr operator*(int a, SMTExpr const & b) {
		return a * b.z3_expr;
	}

	friend SMTExpr operator/(SMTExpr const & a, SMTExpr const & b) {
		return a.z3_expr / b.z3_expr;
	}

	friend SMTExpr operator/(SMTExpr const & a, int b) {
		return a.z3_expr / b;
	}

	friend SMTExpr operator/(int a, SMTExpr const & b) {
		return a / b.z3_expr;
	}

	friend SMTExpr operator-(SMTExpr const & a, SMTExpr const & b) {
		return a.z3_expr - b.z3_expr;
	}

	friend SMTExpr operator-(SMTExpr const & a, int b) {
		return a.z3_expr - b;
	}

	friend SMTExpr operator-(int a, SMTExpr const & b) {
		return a - b.z3_expr;
	}

	friend SMTExpr operator-(SMTExpr const & a) {
		return -a.z3_expr;
	}

	friend SMTExpr operator<=(SMTExpr const & a, SMTExpr const & b) {
		return a.z3_expr <= b.z3_expr;
	}

	friend SMTExpr operator<=(SMTExpr const & a, int b) {
		return a.z3_expr <= b;
	}

	friend SMTExpr operator<=(int a, SMTExpr const & b) {
		return a <= b.z3_expr;
	}

	friend SMTExpr operator<(SMTExpr const & a, SMTExpr const & b) {
		return a.z3_expr < b.z3_expr;
	}

	friend SMTExpr operator<(SMTExpr const & a, int b) {
		return a.z3_expr < b;
	}

	friend SMTExpr operator<(int a, SMTExpr const & b) {
		return a < b.z3_expr;
	}

	friend SMTExpr operator>=(SMTExpr const & a, SMTExpr const & b) {
		return a.z3_expr >= b.z3_expr;
	}

	friend SMTExpr operator>=(SMTExpr const & a, int b) {
		return a.z3_expr >= b;
	}

	friend SMTExpr operator>=(int a, SMTExpr const & b) {
		return a >= b.z3_expr;
	}

	friend SMTExpr operator>(SMTExpr const & a, SMTExpr const & b) {
		return a.z3_expr > b.z3_expr;
	}

	friend SMTExpr operator>(SMTExpr const & a, int b) {
		return a.z3_expr > b;
	}

	friend SMTExpr operator>(int a, SMTExpr const & b) {
		return a > b.z3_expr;
	}

	friend SMTExpr operator&(SMTExpr const & a, SMTExpr const & b) {
		return a.z3_expr & b.z3_expr;
	}

	friend SMTExpr operator&(SMTExpr const & a, int b) {
		return a.z3_expr & b;
	}

	friend SMTExpr operator&(int a, SMTExpr const & b) {
		return a & b.z3_expr;
	}

	friend SMTExpr operator^(SMTExpr const & a, SMTExpr const & b) {
		return a.z3_expr ^ b.z3_expr;
	}

	friend SMTExpr operator^(SMTExpr const & a, int b) {
		return a.z3_expr ^ b;
	}

	friend SMTExpr operator^(int a, SMTExpr const & b) {
		return a ^ b.z3_expr;
	}

	friend SMTExpr operator|(SMTExpr const & a, SMTExpr const & b) {
		return a.z3_expr | b.z3_expr;
	}

	friend SMTExpr operator|(SMTExpr const & a, int b) {
		return a.z3_expr | b;
	}

	friend SMTExpr operator|(int a, SMTExpr const & b) {
		return a | b.z3_expr;
	}

	friend SMTExpr operator~(SMTExpr const & a) {
		return ~a.z3_expr;
	}

	friend std::ostream & operator<<(std::ostream & out, SMTExpr const & n) {
		out << n.z3_expr;
		return out;
	}

	bool isSameSort(SMTExpr const & b) const {
		return z3::eq(z3_expr.get_sort(), b.z3_expr.get_sort());
	}

	bool isArray() {
		return z3_expr.is_array();
	}

	bool isBitVector() {
		return z3_expr.is_bv();
	}

	bool isReal() {
		return z3_expr.is_real();
	}

	bool isBool() {
		return z3_expr.is_bool();
	}

	bool isBvArray() {
		if (z3_expr.is_array()) {
			return z3_expr.get_sort().array_range().is_bv();
		}
		return false;
	}

	bool isConst() const {
		return z3_expr.is_const();
	}

	bool isApp() const {
		return z3_expr.is_app();
	}

	bool isNumeral() const {
		return z3_expr.is_numeral();
	}

	bool isQuantifier() const {
		return z3_expr.is_quantifier();
	}

	SMTExpr getQuantifierBody() const {
		return z3_expr.body();
	}

	bool isVar() const {
		return z3_expr.is_var();
	}

	unsigned numArgs() const {
		return z3_expr.num_args();
	}

	SMTExpr getArg(unsigned i) const {
		return z3_expr.arg(i);
	}

	unsigned getBitVecSize() {
		return z3_expr.get_sort().bv_size();
	}

	bool isLogicAnd() const {
		z3::expr z = z3_expr.ctx().bool_val(true);
		return z3::eq(z3_expr.decl(), (z && z).decl());
	}

	bool isLogicOr() const {
		z3::expr z = z3_expr.ctx().bool_val(true);
		return z3::eq(z3_expr.decl(), (z || z).decl());
	}

	bool isLogicNot() const {
		z3::expr z = z3_expr.ctx().bool_val(true);
		return z3::eq(z3_expr.decl(), (!z).decl());
	}

	bool equals(const SMTExpr& e) const {
		return z3::eq(z3_expr, e.z3_expr);
	}

	std::string getSymbol() const {
		return Z3_ast_to_string(z3_expr.ctx(), z3_expr);
	}

	SMTExpr substitute(SMTExprVec& From, SMTExprVec& To);

	SMTExpr bv12bool() {
		if (z3_expr.is_array()) {
			assert(z3_expr.get_sort().array_range().is_bv());
			unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
			assert(bvSz == 1);
			auto func = (z3_expr.ctx().bv_val("0", 1) == z3_expr.ctx().bv_val("0", 1)).decl();

			z3::expr const_bv1 = const_array(z3_expr.get_sort().array_domain(), z3_expr.ctx().bv_val("1", 1));
			Z3_ast mapargs[2] = { z3_expr, const_bv1 };

			return z3::expr(z3_expr.ctx(), Z3_mk_map(z3_expr.ctx(), func, 2, mapargs));
		} else {
			assert(z3_expr.is_bv() && z3_expr.get_sort().bv_size() == 1);
			return z3_expr == z3_expr.ctx().bv_val(1, 1);
		}
	}

	SMTExpr bool2bv1() {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array()) {
			assert(z3_expr.get_sort().array_range().is_bool());
			auto func = ite(ctx.bool_val(false), ctx.bv_val(1, 1), ctx.bv_val(0, 1)).decl();

			z3::expr const_bv0 = const_array(z3_expr.get_sort().array_domain(), ctx.bv_val(0, 1));
			z3::expr const_bv1 = const_array(z3_expr.get_sort().array_domain(), ctx.bv_val(1, 1));

			Z3_ast mapargs[3] = { z3_expr, const_bv0, const_bv1 };

			z3::expr bvret(ctx, Z3_mk_map(ctx, func, 3, mapargs));
			return bvret;
		} else {
			assert(z3_expr.is_bool());
			return ite(z3_expr, ctx.bv_val(1, 1), ctx.bv_val(0, 1));
		}
	}

	SMTExpr real2int() {
		z3::context & ctx = z3_expr.ctx();
		if (z3_expr.is_array()) {
			assert(z3_expr.get_sort().array_range().is_real());
			auto func = z3::expr(ctx, Z3_mk_real2int(ctx, ctx.real_val("0.0"))).decl();

			Z3_ast mapargs[1] = { z3_expr };
			return z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs));
		} else {
			assert(z3_expr.is_real());
			return z3::expr(ctx, Z3_mk_real2int(ctx, z3_expr));
		}
	}

	SMTExpr int2real() {
		z3::context & ctx = z3_expr.ctx();
		if (z3_expr.is_array()) {
			assert(z3_expr.get_sort().array_range().is_int());

			auto func = to_real(z3_expr).decl();
			Z3_ast mapargs[1] = { z3_expr };
			return z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs));
		} else {
			assert(z3_expr.is_int());
			return to_real(z3_expr);
		}
	}

	SMTExpr int2bv(unsigned sz) {
		z3::context & ctx = z3_expr.ctx();
		if (z3_expr.is_array()) {
			assert(z3_expr.get_sort().array_range().is_int());
			auto func = z3::expr(ctx, Z3_mk_int2bv(ctx, sz, ctx.int_val("0"))).decl();

			Z3_ast mapargs[1] = { z3_expr };
			return z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs));
		} else {
			assert(z3_expr.is_int());
			return z3::expr(ctx, Z3_mk_int2bv(ctx, sz, z3_expr));
		}
	}

	SMTExpr bv2int(bool isSigned) {
		z3::context & ctx = z3_expr.ctx();
		if (z3_expr.is_array()) {
			assert(z3_expr.get_sort().array_range().is_bv());
			unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
			auto func = z3::expr(ctx, Z3_mk_bv2int(ctx, ctx.bv_val("0", bvSz), isSigned)).decl();

			Z3_ast mapargs[1] = { z3_expr };
			return z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs));
		} else {
			assert(z3_expr.is_bv());
			return z3::expr(ctx, Z3_mk_bv2int(ctx, z3_expr, isSigned));
		}
	}

	SMTExpr basic_zext(unsigned sz) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array()) {
			assert(z3_expr.get_sort().array_range().is_bv());
			unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
			auto func = z3::expr(ctx, Z3_mk_zero_ext(ctx, sz, ctx.bv_val("10", bvSz))).decl();
			Z3_ast mapargs[1] = { z3_expr };
			return z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs));
		} else {
			assert(z3_expr.is_bv());
			return z3::expr(ctx, Z3_mk_zero_ext(ctx, sz, z3_expr));
		}
	}

	SMTExpr basic_add(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) + ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") + ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (z3_expr + b.z3_expr);
		}
	}

	SMTExpr basic_sub(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) - ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") - ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (z3_expr - b.z3_expr);
		}
	}

	SMTExpr basic_mul(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) * ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") * ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (z3_expr * b.z3_expr);
		}
	}

	SMTExpr basic_udiv(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvudiv(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return z3::expr(ctx, Z3_mk_bvudiv(ctx, z3_expr, b.z3_expr));
		}
	}

	SMTExpr basic_sdiv(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) / ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") / ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (z3_expr / b.z3_expr);
		}
	}

	SMTExpr basic_urem(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvurem(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return z3::expr(ctx, Z3_mk_bvurem(ctx, z3_expr, b.z3_expr));
		}
	}

	SMTExpr basic_srem(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvsrem(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else if (z3_expr.is_real() && b.z3_expr.is_real()) {
			// z3 only accept rem between integers
			z3::expr ch1ToInt = z3::expr(ctx, Z3_mk_real2int(ctx, z3_expr));
			z3::expr ch2ToInt = z3::expr(ctx, Z3_mk_real2int(ctx, b.z3_expr));
			z3::expr remResult = z3::expr(ctx, Z3_mk_rem(ctx, ch1ToInt, ch2ToInt));
			z3::expr remResult2Real = z3::expr(ctx, Z3_mk_int2real(ctx, remResult));
			return remResult2Real;
		} else {
			return z3::expr(ctx, Z3_mk_bvsrem(ctx, z3_expr, b.z3_expr));
		}
	}

	SMTExpr basic_shl(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvshl(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return z3::expr(ctx, Z3_mk_bvshl(ctx, z3_expr, b.z3_expr));
		}
	}

	SMTExpr basic_ashr(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvashr(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return z3::expr(ctx, Z3_mk_bvashr(ctx, z3_expr, b.z3_expr));
		}
	}

	SMTExpr basic_lshr(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvlshr(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return z3::expr(ctx, Z3_mk_bvlshr(ctx, z3_expr, b.z3_expr));
		}
	}

	SMTExpr basic_and(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) & ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") & ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (z3_expr & b.z3_expr);
		}
	}

	SMTExpr basic_or(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) | ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") | ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (z3_expr | b.z3_expr);
		}
	}

	SMTExpr basic_xor(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) ^ ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") ^ ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (z3_expr ^ b.z3_expr);
		}
	}

	SMTExpr basic_eq(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) == ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") == ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (z3_expr == b.z3_expr);
		}
	}

	SMTExpr basic_ne(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) != ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") != ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (z3_expr != b.z3_expr);
		}
	}

	SMTExpr basic_ugt(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvugt(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return z3::expr(ctx, Z3_mk_bvugt(ctx, z3_expr, b.z3_expr));
		}
	}

	SMTExpr basic_uge(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvuge(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return z3::expr(ctx, Z3_mk_bvuge(ctx, z3_expr, b.z3_expr));
		}
	}

	SMTExpr basic_sgt(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) > ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") > ctx.real_val("0.0")).decl();
			}

			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (z3_expr > b.z3_expr);
		}
	}

	SMTExpr basic_sge(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) >= ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") >= ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (z3_expr >= b.z3_expr);
		}
	}

	SMTExpr basic_ult(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvult(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return z3::expr(ctx, Z3_mk_bvult(ctx, z3_expr, b.z3_expr));
		}
	}

	SMTExpr basic_ule(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = z3::expr(ctx, Z3_mk_bvule(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
			} else {
				assert(false);
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return z3::expr(ctx, Z3_mk_bvule(ctx, z3_expr, b.z3_expr));
		}
	}

	SMTExpr basic_slt(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) < ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") < ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (z3_expr < b.z3_expr);
		}
	}

	SMTExpr basic_sle(SMTExpr &b) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array() && b.z3_expr.is_array()) {
			Z3_ast const mapargs[2] = { z3_expr, b.z3_expr };
			z3::func_decl func(ctx);
			if (z3_expr.get_sort().array_range().is_bv()) {
				unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
				func = (ctx.bv_val("0", bvSz) <= ctx.bv_val("0", bvSz)).decl();
			} else {
				func = (ctx.real_val("0.0") <= ctx.real_val("0.0")).decl();
			}
			return z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs));
		} else {
			return (z3_expr <= b.z3_expr);
		}
	}

	SMTExpr basic_extract(unsigned high, unsigned low) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array()) {
			assert(z3_expr.get_sort().array_range().is_bv());
			unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
			auto func = z3::expr(ctx, Z3_mk_extract(ctx, high, low, ctx.bv_val("10", bvSz))).decl();
			Z3_ast mapargs[1] = { z3_expr };
			return z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs));
		} else {
			assert(z3_expr.is_bv());
			return z3::expr(ctx, Z3_mk_extract(ctx, high, low, z3_expr));
		}
	}

	SMTExpr basic_sext(unsigned sz) {
		z3::context& ctx = z3_expr.ctx();
		if (z3_expr.is_array()) {
			assert(z3_expr.get_sort().array_range().is_bv());
			unsigned bvSz = z3_expr.get_sort().array_range().bv_size();
			auto func = z3::expr(ctx, Z3_mk_sign_ext(ctx, sz, ctx.bv_val("10", bvSz))).decl();
			Z3_ast mapargs[1] = { z3_expr };
			return z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs));
		} else {
			assert(z3_expr.is_bv());
			return z3::expr(ctx, Z3_mk_sign_ext(ctx, sz, z3_expr));
		}
	}

	SMTExpr basic_ite(SMTExpr& TBValue, SMTExpr& FBValue) {
		z3::context& ctx = z3_expr.ctx();
		z3::expr& condition = z3_expr;
		if (condition.is_array()) {
			assert(TBValue.z3_expr.is_array() && FBValue.z3_expr.is_array());

			Z3_ast mapargs[3] = { condition, TBValue.z3_expr, FBValue.z3_expr };
			z3::func_decl func(ctx);
			if (TBValue.z3_expr.get_sort().array_range().is_bv()) {
				assert(FBValue.z3_expr.get_sort().array_range().is_bv());
				unsigned bvSz = TBValue.z3_expr.get_sort().array_range().bv_size();
				func = ite(ctx.bool_val(true), ctx.bv_val(1, bvSz), ctx.bv_val(0, bvSz)).decl();
			} else {
				assert(TBValue.z3_expr.get_sort().array_range().is_real());
				assert(FBValue.z3_expr.get_sort().array_range().is_real());
				func = ite(ctx.bool_val(true), ctx.real_val("0.0"), ctx.real_val("0.0")).decl();
			}

			z3::expr mapped(ctx, Z3_mk_map(ctx, func, 3, mapargs));
			return z3::expr(ctx, Z3_mk_map(ctx, func, 3, mapargs));
		} else {
			return ite(condition, TBValue.z3_expr, FBValue.z3_expr);
		}
	}

	SMTExpr localSimplify();

	SMTExpr dilligSimplify();

	unsigned size();

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
	bool operator()(const SMTExpr& x, const SMTExpr& y) const {
		Z3_ast x_ast = (Z3_ast) x.z3_expr;
		Z3_ast y_ast = (Z3_ast) y.z3_expr;
		return x_ast < y_ast || (!(y_ast < x_ast) && x_ast < y_ast);
	}
};

class SMTExprVec {
private:
	z3::expr_vector z3_expr_vec;

	SMTExprVec(z3::expr_vector vec) :
			z3_expr_vec(vec) {
	}

public:
	SMTExprVec(SMTExprVec const &Vec) :
			z3_expr_vec(Vec.z3_expr_vec) {
	}

	SMTExprVec& operator=(const SMTExprVec& Vec) {
		if (this != &Vec) {
			this->z3_expr_vec = Vec.z3_expr_vec;
		}
		return *this;
	}

	unsigned size() const {
		return z3_expr_vec.size();
	}

	void push_back(SMTExpr e) {
		if(e.equals(e.z3_expr.ctx().bool_val(true))){
			return;
		}
		z3_expr_vec.push_back(e.z3_expr);
	}

	SMTExpr operator[](int i) const {
		return z3_expr_vec[i];
	}

	void clear() {
		z3_expr_vec = z3::expr_vector(z3_expr_vec.ctx());
	}

	bool empty() const {
		return z3_expr_vec.empty();
	}

	SMTExprVec copy() {
		z3::expr_vector Ret = z3::expr_vector(z3_expr_vec.ctx());
		for (unsigned Idx = 0; Idx < z3_expr_vec.size(); Idx++) {
			Ret.push_back(z3_expr_vec[Idx]);
		}
		return Ret;
	}

	/// *this = *this && v2
	void mergeWithAnd(const SMTExprVec& v2) {
		for (size_t i = 0; i < v2.size(); i++) {
			z3_expr_vec.push_back(v2.z3_expr_vec[i]);
		}
	}

	/// *this = *this || v2
	void mergeWithOr(const SMTExprVec& v2) {
		if (v2.empty())
			return;

		if (empty()) {
			z3_expr_vec = v2.z3_expr_vec;
			return;
		}

		SMTExpr e1 = this->toAndExpr();
		SMTExpr e2 = v2.toAndExpr();

		z3::expr_vector ret(z3_expr_vec.ctx());
		ret.push_back(e1.z3_expr || e2.z3_expr);
		z3_expr_vec = ret;
	}

	/// From the vector create one expr to represent the AND result.
	/// Create  bool expr to represent this vector
	SMTExpr toAndExpr() const;

	SMTExpr toOrExpr() const;

	friend class SMTFactory;
	friend class SMTSolver;
	friend class SMTExpr;

	friend std::ostream & operator<<(std::ostream & out, SMTExprVec vec) {
		out << vec.z3_expr_vec;
		return out;
	}
};

#endif
