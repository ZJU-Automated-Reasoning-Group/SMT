/**
 * Authors: Qingkai
 */

#include <vector>
#include <set>

#include <llvm/Support/Debug.h>

#include "SMT/SMTExpr.h"
#include "SMT/SMTFactory.h"

SMTExpr::SMTExpr(SMTFactory* F, z3::expr Z3Expr) : SMTObject(F),
		Expr(Z3Expr) {
}

SMTExpr::SMTExpr(SMTExpr const & E) : SMTObject(E),
		Expr(E.Expr) {
}

SMTExpr& SMTExpr::operator=(const SMTExpr& E) {
	SMTObject::operator =(E);
	if (this != &E) {
		this->Expr = E.Expr;
	}
	return *this;
}

SMTExpr SMTExpr::substitute(SMTExprVec& From, SMTExprVec& To) {
	assert(From.size() == To.size());
	if (From.empty()) {
		return *this;
	}
	return SMTExpr(&getSMTFactory(), Expr.substitute(*From.ExprVec, *To.ExprVec));
}

SMTExpr SMTExpr::localSimplify() {
	return SMTExpr(&getSMTFactory(), Expr.simplify());
}

SMTExpr SMTExpr::dilligSimplify(SMTExpr N, z3::solver& Solver4Sim, z3::context& Ctx) {
	if (!N.isLogicAnd() && !N.isLogicOr()) {
		// A leaf
		Solver4Sim.push();
		Solver4Sim.add(N.Expr);
		if (Solver4Sim.check() == z3::check_result::unsat) {
			Solver4Sim.pop();
			return SMTExpr(&getSMTFactory(), Ctx.bool_val(false));
		}
		Solver4Sim.pop();
		Solver4Sim.push();
		Solver4Sim.add(!N.Expr);
		if (Solver4Sim.check() == z3::check_result::unsat) {
			Solver4Sim.pop();
			return SMTExpr(&getSMTFactory(), Ctx.bool_val(true));
		}
		Solver4Sim.pop();
		return N;
	} else {
		// A connective (AND or OR)
		assert(N.isLogicAnd() || N.isLogicOr());

		std::vector<SMTExpr> C;
		std::set<SMTExpr, SMTExprComparator> CSet;
		for (unsigned I = 0, E = N.numArgs(); I < E; I++) {
			if (N.getArg(I).isTrue()) {
				if (N.isLogicAnd()) {
					continue;
				} else if (N.isLogicOr()) {
					return SMTExpr(&getSMTFactory(), Ctx.bool_val(true));
				}
			} else if (N.getArg(I).isFalse()) {
				if (N.isLogicAnd()) {
					return SMTExpr(&getSMTFactory(), Ctx.bool_val(false));
				} else if (N.isLogicOr()) {
					continue;
				}
			}

			if (!CSet.count(N.getArg(I))) {
				C.push_back(N.getArg(I));
				CSet.insert(N.getArg(I));
			}
		}

		bool FixedPoint = false;
		while (!FixedPoint) {
			//std::cout << "............." << (void*) (Z3_ast) N.z3_expr << "\n\n";
			FixedPoint = true;

			for (size_t I = 0, E = C.size(); I < E; ++I) {
				Z3_ast* Args = new Z3_ast[C.size() - 1];
				for (size_t J = 0; J < C.size() - 1; J++) {
					SMTExpr *Candidate = nullptr;
					if (J < I) {
						Candidate = &C[J];
					} else {
						assert(J + 1 < C.size());
						Candidate = &C[J + 1];
					}

					if (N.isLogicOr()) {
						Args[J] = !(*Candidate).Expr;
					} else {
						Args[J] = (*Candidate).Expr;
					}
				}
				SMTExpr Alpha(&getSMTFactory(), to_expr(Ctx, Z3_mk_and(Ctx, C.size() - 1, Args)));
				delete[] Args;

				SMTExpr& Ci = C[I];

				Solver4Sim.push();
				Solver4Sim.add(Alpha.Expr);
				SMTExpr NewCi = dilligSimplify(Ci, Solver4Sim, Ctx);
				Solver4Sim.pop();

				if (!z3::eq(Ci.Expr, NewCi.Expr)) {
					if (FixedPoint)
						FixedPoint = false;
					C[I] = NewCi;
				}

				if (NewCi.isTrue() && N.isLogicOr()) {
					return SMTExpr(&getSMTFactory(), Ctx.bool_val(true));
				} else if (NewCi.isFalse() && N.isLogicAnd()) {
					return SMTExpr(&getSMTFactory(), Ctx.bool_val(false));
				}
			}

			// FIXME
			FixedPoint = true;
		}

		if (N.isLogicAnd()) {
			Z3_ast* Args = new Z3_ast[C.size()];
			size_t J = 0;
			for (size_t I = 0; I < C.size(); I++) {
				if (C[I].isTrue()) {
					continue;
				}
				Args[J++] = C[I].Expr;
			}

			if (J == 1) {
				SMTExpr Ret(&getSMTFactory(), to_expr(Ctx, Args[0]));
				delete[] Args;
				return Ret;
			}

			if (J == 0) {
				delete[] Args;
				return SMTExpr(&getSMTFactory(), Ctx.bool_val(true));
			}

			SMTExpr Ret(&getSMTFactory(), to_expr(Ctx, Z3_mk_and(Ctx, J, Args)));
			delete[] Args;

			return Ret;
		} else {
			// is logic OR
			Z3_ast* Args = new Z3_ast[C.size()];
			size_t J = 0;
			for (size_t I = 0; I < C.size(); I++) {
				if (C[I].isFalse()) {
					continue;
				}
				Args[J++] = C[I].Expr;
			}

			if (J == 1) {
				SMTExpr Ret(&getSMTFactory(), to_expr(Ctx, Args[0]));
				delete[] Args;
				return Ret;
			}

			if (J == 0) {
				delete[] Args;
				return SMTExpr(&getSMTFactory(), Ctx.bool_val(false));
			}

			SMTExpr Ret(&getSMTFactory(), to_expr(Ctx, Z3_mk_or(Ctx, J, Args)));
			delete[] Args;

			return Ret;
		}
	}
}

SMTExpr SMTExpr::dilligSimplify() {
	z3::context& Ctx = Expr.ctx();
	z3::solver Solver4Sim(Ctx);
	Solver4Sim.add(Ctx.bool_val(true));

	//	std::cout << "\nstart...." << this->size() << "\n";
	SMTExpr AftSim = dilligSimplify(*this, Solver4Sim, Ctx);
	//	std::cout << "end..." << AftSim.size() << "\n";

	return AftSim;
}

unsigned SMTExpr::size(std::map<SMTExpr, unsigned, SMTExprComparator>& SizeCache) {
	if (!this->isLogicAnd() && !this->isLogicOr()) {
		if (isLogicNot()) {
			return this->getArg(0).size(SizeCache);
		} else {
			return 1;
		}
	} else {
		if (SizeCache.count(*this)) {
			return 0;
		}

		unsigned Sz = 0;
		for (unsigned I = 0, E = this->numArgs(); I < E; I++) {
			Sz += this->getArg(I).size(SizeCache);
		}
		SizeCache.insert(std::make_pair(*this, Sz));
		return Sz;
	}
}

SMTExpr SMTExpr::getQuantifierBody() const {
	return SMTExpr(&getSMTFactory(), Expr.body());
}

SMTExpr SMTExpr::getArg(unsigned I) const {
	return SMTExpr(&getSMTFactory(), Expr.arg(I));
}

SMTExpr SMTExpr::bv12bool() {
	if (Expr.is_array()) {
		assert(Expr.get_sort().array_range().is_bv());
		unsigned bvSz = Expr.get_sort().array_range().bv_size();
		assert(bvSz == 1);
		auto func = (Expr.ctx().bv_val("0", 1) == Expr.ctx().bv_val("0", 1)).decl();

		z3::expr const_bv1 = const_array(Expr.get_sort().array_domain(), Expr.ctx().bv_val("1", 1));
		Z3_ast mapargs[2] = { Expr, const_bv1 };

		return SMTExpr(&getSMTFactory(), z3::expr(Expr.ctx(), Z3_mk_map(Expr.ctx(), func, 2, mapargs)));
	} else {
		assert(Expr.is_bv() && Expr.get_sort().bv_size() == 1);
		return SMTExpr(&getSMTFactory(), Expr == Expr.ctx().bv_val(1, 1));
	}
}

SMTExpr SMTExpr::bool2bv1() {
	z3::context& ctx = Expr.ctx();
	if (Expr.is_array()) {
		assert(Expr.get_sort().array_range().is_bool());
		auto func = ite(ctx.bool_val(false), ctx.bv_val(1, 1), ctx.bv_val(0, 1)).decl();

		z3::expr const_bv0 = const_array(Expr.get_sort().array_domain(), ctx.bv_val(0, 1));
		z3::expr const_bv1 = const_array(Expr.get_sort().array_domain(), ctx.bv_val(1, 1));

		Z3_ast mapargs[3] = { Expr, const_bv0, const_bv1 };

		z3::expr bvret(ctx, Z3_mk_map(ctx, func, 3, mapargs));
		return SMTExpr(&getSMTFactory(), bvret);
	} else {
		assert(Expr.is_bool());
		return SMTExpr(&getSMTFactory(), ite(Expr, ctx.bv_val(1, 1), ctx.bv_val(0, 1)));
	}
}

SMTExpr SMTExpr::real2int() {
	z3::context & ctx = Expr.ctx();
	if (Expr.is_array()) {
		assert(Expr.get_sort().array_range().is_real());
		auto func = z3::expr(ctx, Z3_mk_real2int(ctx, ctx.real_val("0.0"))).decl();

		Z3_ast mapargs[1] = { Expr };
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs)));
	} else {
		assert(Expr.is_real());
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_real2int(ctx, Expr)));
	}
}

SMTExpr SMTExpr::int2real() {
	z3::context & ctx = Expr.ctx();
	if (Expr.is_array()) {
		assert(Expr.get_sort().array_range().is_int());

		auto func = to_real(Expr).decl();
		Z3_ast mapargs[1] = { Expr };
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs)));
	} else {
		assert(Expr.is_int());
		return SMTExpr(&getSMTFactory(), to_real(Expr));
	}
}

SMTExpr SMTExpr::int2bv(unsigned sz) {
	z3::context & ctx = Expr.ctx();
	if (Expr.is_array()) {
		assert(Expr.get_sort().array_range().is_int());
		auto func = z3::expr(ctx, Z3_mk_int2bv(ctx, sz, ctx.int_val("0"))).decl();

		Z3_ast mapargs[1] = { Expr };
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs)));
	} else {
		assert(Expr.is_int());
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_int2bv(ctx, sz, Expr)));
	}
}

SMTExpr SMTExpr::bv2int(bool isSigned) {
	z3::context & ctx = Expr.ctx();
	if (Expr.is_array()) {
		assert(Expr.get_sort().array_range().is_bv());
		unsigned bvSz = Expr.get_sort().array_range().bv_size();
		auto func = z3::expr(ctx, Z3_mk_bv2int(ctx, ctx.bv_val("0", bvSz), isSigned)).decl();

		Z3_ast mapargs[1] = { Expr };
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs)));
	} else {
		assert(Expr.is_bv());
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_bv2int(ctx, Expr, isSigned)));
	}
}

#define BINARY_OPERATION(X) \
SMTExpr SMTExpr::basic_##X(SMTExpr &b) { \
	z3::context& ctx = Expr.ctx(); \
	if (Expr.is_array() && b.Expr.is_array()) { \
		Z3_ast const mapargs[2] = { Expr, b.Expr };\
		z3::func_decl func(ctx);\
		if (Expr.get_sort().array_range().is_bv()) {\
			unsigned bvSz = Expr.get_sort().array_range().bv_size();\
			func = z3::expr(ctx, Z3_mk_bv##X(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();\
		} else {\
			assert(false); \
		}\
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));\
	} else {\
		assert(isBitVector() && b.isBitVector()); \
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_bv##X(ctx, Expr, b.Expr)));\
	}\
}\

BINARY_OPERATION(add)
BINARY_OPERATION(sub)
BINARY_OPERATION(mul)
BINARY_OPERATION(udiv)
BINARY_OPERATION(sdiv)
BINARY_OPERATION(urem)
BINARY_OPERATION(srem)
BINARY_OPERATION(shl)
BINARY_OPERATION(ashr)
BINARY_OPERATION(lshr)
BINARY_OPERATION(and)
BINARY_OPERATION(or)
BINARY_OPERATION(xor)

BINARY_OPERATION(ugt)
BINARY_OPERATION(uge)
BINARY_OPERATION(sgt)
BINARY_OPERATION(sge)
BINARY_OPERATION(ult)
BINARY_OPERATION(ule)
BINARY_OPERATION(sle)
BINARY_OPERATION(slt)

SMTExpr SMTExpr::basic_concat(SMTExpr &b) {
	z3::context& ctx = Expr.ctx();
	if (Expr.is_array() && b.Expr.is_array()) {
		Z3_ast const mapargs[2] = { Expr, b.Expr };
		z3::func_decl func(ctx);
		if (Expr.get_sort().array_range().is_bv()) {
			unsigned bvSz = Expr.get_sort().array_range().bv_size();
			func = z3::expr(ctx, Z3_mk_concat(ctx, ctx.bv_val("0", bvSz), ctx.bv_val("0", bvSz))).decl();
		} else {
			assert(false);
		}
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_concat(ctx, Expr, b.Expr)));
	}
}

SMTExpr SMTExpr::basic_eq(SMTExpr &b) {
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
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return *this == b;
	}
}

SMTExpr SMTExpr::basic_ne(SMTExpr &b) {
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
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return *this != b;
	}
}

SMTExpr SMTExpr::basic_extract(unsigned high, unsigned low) {
	z3::context& ctx = Expr.ctx();
	if (Expr.is_array()) {
		assert(Expr.get_sort().array_range().is_bv());
		unsigned bvSz = Expr.get_sort().array_range().bv_size();
		auto func = z3::expr(ctx, Z3_mk_extract(ctx, high, low, ctx.bv_val("10", bvSz))).decl();
		Z3_ast mapargs[1] = { Expr };
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs)));
	} else {
		assert(Expr.is_bv());
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_extract(ctx, high, low, Expr)));
	}
}

SMTExpr SMTExpr::basic_sext(unsigned sz) {
	z3::context& ctx = Expr.ctx();
	if (Expr.is_array()) {
		assert(Expr.get_sort().array_range().is_bv());
		unsigned bvSz = Expr.get_sort().array_range().bv_size();
		auto func = z3::expr(ctx, Z3_mk_sign_ext(ctx, sz, ctx.bv_val("10", bvSz))).decl();
		Z3_ast mapargs[1] = { Expr };
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs)));
	} else {
		assert(Expr.is_bv());
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_sign_ext(ctx, sz, Expr)));
	}
}

SMTExpr SMTExpr::basic_zext(unsigned sz) {
	z3::context& ctx = Expr.ctx();
	if (Expr.is_array()) {
		assert(Expr.get_sort().array_range().is_bv());
		unsigned bvSz = Expr.get_sort().array_range().bv_size();
		auto func = z3::expr(ctx, Z3_mk_zero_ext(ctx, sz, ctx.bv_val("10", bvSz))).decl();
		Z3_ast mapargs[1] = { Expr };
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs)));
	} else {
		assert(Expr.is_bv());
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_zero_ext(ctx, sz, Expr)));
	}
}

SMTExpr SMTExpr::basic_ite(SMTExpr& TBValue, SMTExpr& FBValue) {
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
		return SMTExpr(&getSMTFactory(), z3::expr(ctx, Z3_mk_map(ctx, func, 3, mapargs)));
	} else {
		return SMTExpr(&getSMTFactory(), ite(condition, TBValue.Expr, FBValue.Expr));
	}
}

SMTExpr SMTExpr::array_elmt(unsigned ElmtNum, unsigned Index) {
	assert(ElmtNum > Index);
	unsigned TotalSz = getBitVecSize();
	unsigned EachSz = TotalSz / ElmtNum;
	unsigned High = TotalSz - Index * EachSz - 1;
	unsigned Low = TotalSz - (Index + 1) * EachSz;
	return basic_extract(High, Low);
}

#define ARRAY_CMP_OPERATION(X) \
SMTExpr SMTExpr::array_##X(SMTExpr &BvAsArray, unsigned ElmtNum) { \
	assert(ElmtNum > 0); \
	if (ElmtNum == 1) { \
		return basic_##X(BvAsArray).bool2bv1(); \
	} else { \
		SMTExpr Op2 = BvAsArray.array_elmt(ElmtNum, 0); \
		SMTExpr Ret = array_elmt(ElmtNum, 0).basic_##X(Op2).bool2bv1(); \
		for (unsigned I = 1; I < ElmtNum; I++) { \
			Op2 = BvAsArray.array_elmt(ElmtNum, I); \
			SMTExpr NextRet = array_elmt(ElmtNum, I).basic_##X(Op2).bool2bv1(); \
			Ret = Ret.basic_concat(NextRet); \
		} \
		return Ret; \
	} \
} \

ARRAY_CMP_OPERATION(sgt)
ARRAY_CMP_OPERATION(sge)
ARRAY_CMP_OPERATION(ugt)
ARRAY_CMP_OPERATION(uge)
ARRAY_CMP_OPERATION(slt)
ARRAY_CMP_OPERATION(sle)
ARRAY_CMP_OPERATION(ule)
ARRAY_CMP_OPERATION(ult)
ARRAY_CMP_OPERATION(eq)
ARRAY_CMP_OPERATION(ne)

#define ARRAY_BIN_OPERATION(X) \
SMTExpr SMTExpr::array_##X(SMTExpr &BvAsArray, unsigned ElmtNum) { \
	assert(ElmtNum > 0); \
	if (ElmtNum == 1) { \
		return basic_##X(BvAsArray); \
	} else { \
		SMTExpr Op2 = BvAsArray.array_elmt(ElmtNum, 0); \
		SMTExpr Ret = array_elmt(ElmtNum, 0).basic_##X(Op2); \
		for (unsigned I = 1; I < ElmtNum; I++) { \
			Op2 = BvAsArray.array_elmt(ElmtNum, I); \
			SMTExpr Next = array_elmt(ElmtNum, I).basic_##X(Op2); \
			Ret = Ret.basic_concat(Next); \
		} \
		return Ret; \
	} \
} \

ARRAY_BIN_OPERATION(add)
ARRAY_BIN_OPERATION(sub)
ARRAY_BIN_OPERATION(mul)
ARRAY_BIN_OPERATION(udiv)
ARRAY_BIN_OPERATION(sdiv)
ARRAY_BIN_OPERATION(urem)
ARRAY_BIN_OPERATION(srem)
ARRAY_BIN_OPERATION(shl)
ARRAY_BIN_OPERATION(ashr)
ARRAY_BIN_OPERATION(lshr)
ARRAY_BIN_OPERATION(and)
ARRAY_BIN_OPERATION(or)
ARRAY_BIN_OPERATION(xor)

SMTExpr SMTExpr::array_ite(SMTExpr& TBValue, SMTExpr& FBValue, unsigned ElmtNum) {
	assert(ElmtNum > 0);
	if (ElmtNum == 1) {
		return this->bv12bool().basic_ite(TBValue, FBValue);
	} else {
		SMTExpr T = TBValue.array_elmt(ElmtNum, 0);
		SMTExpr F = FBValue.array_elmt(ElmtNum, 0);
		SMTExpr Ret = array_elmt(ElmtNum, 0).bv12bool().basic_ite(T, F);

		for (unsigned I = 1; I < ElmtNum; I++) {
			T = TBValue.array_elmt(ElmtNum, I);
			F = FBValue.array_elmt(ElmtNum, I);
			SMTExpr Next = array_elmt(ElmtNum, I).bv12bool().basic_ite(T, F);
			Ret = Ret.basic_concat(Next);
		}
		return Ret;
	}
}

#define ARRAY_EXT_OPERATION(X) \
SMTExpr SMTExpr::array_##X(unsigned Sz, unsigned ElmtNum) { \
	assert(ElmtNum > 0); \
	if (ElmtNum == 1) { \
		return basic_##X(Sz); \
	} else { \
		unsigned ElmtExtSz = Sz / ElmtNum; \
		SMTExpr Ret = array_elmt(ElmtNum, 0).basic_##X(ElmtExtSz); \
		for (unsigned I = 1; I < ElmtNum; I++) { \
			SMTExpr Next = array_elmt(ElmtNum, I).basic_##X(ElmtExtSz); \
			Ret = Ret.basic_concat(Next); \
		} \
		return Ret; \
	} \
}

ARRAY_EXT_OPERATION(zext)
ARRAY_EXT_OPERATION(sext)

SMTExpr SMTExpr::array_trunc(unsigned Sz, unsigned ElmtNum) {
	assert(ElmtNum > 0);
	if (ElmtNum == 1) {
		return basic_extract(getBitVecSize() - Sz - 1, 0);
	} else {
		unsigned ElmtSz = getBitVecSize() / ElmtNum;
		unsigned ElmtTruncSz = Sz / ElmtNum;

		assert(ElmtSz > ElmtTruncSz);

		unsigned High = ElmtSz - ElmtTruncSz - 1;
		SMTExpr Ret = array_elmt(ElmtNum, 0).basic_extract(High, 0);
		for (unsigned I = 1; I < ElmtNum; I++) {
			SMTExpr Next = array_elmt(ElmtNum, I).basic_extract(High, 0);
			Ret = Ret.basic_concat(Next);
		}
		return Ret;
	}
}

SMTExpr operator!(SMTExpr const & A) {
	if (A.isLogicNot()) {
		assert(A.numArgs() == 1);
		return A.getArg(0);
	} else if (A.isTrue()) {
		return A.getSMTFactory().createBoolVal(false);
	} else if (A.isFalse()) {
		return A.getSMTFactory().createBoolVal(true);
	} else {
		return SMTExpr(&A.getSMTFactory(), !A.Expr);
	}
}

SMTExpr operator||(SMTExpr const & A, SMTExpr const & B) {
	if (A.isFalse() || B.isTrue()) {
		return B;
	} else if (A.isTrue() || B.isFalse()) {
		return A;
	}

	return SMTExpr(&A.getSMTFactory(), A.Expr || B.Expr);
}

SMTExpr operator||(SMTExpr const & A, bool B) {
	return A || A.getSMTFactory().createBoolVal(B);
}

SMTExpr operator||(bool A, SMTExpr const & B) {
	return B || A;
}

SMTExpr operator&&(SMTExpr const & A, SMTExpr const & B) {
	if (A.isTrue() || B.isFalse()) {
		return B;
	} else if (B.isTrue() || A.isFalse()) {
		return A;
	}

	return SMTExpr(&A.getSMTFactory(), A.Expr && B.Expr);
}

SMTExpr operator&&(SMTExpr const & A, bool B) {
	return A && A.getSMTFactory().createBoolVal(B);
}

SMTExpr operator&&(bool A, SMTExpr const & B) {
	return B && A;
}

#define UNARY_OPERATION_EXPR(X) \
SMTExpr operator X(SMTExpr const & A) { \
	return SMTExpr(&A.getSMTFactory(), X(A.Expr)); \
}

UNARY_OPERATION_EXPR(-)
UNARY_OPERATION_EXPR(~)

#define BINARY_OPERATION_EXPR_EXPR(X) \
SMTExpr operator X(SMTExpr const & A, SMTExpr const & B) { \
	assert(A.isSameSort(B)); \
	return SMTExpr(&A.getSMTFactory(), A.Expr X B.Expr); \
}

BINARY_OPERATION_EXPR_EXPR(|)
BINARY_OPERATION_EXPR_EXPR(^)
BINARY_OPERATION_EXPR_EXPR(&)
BINARY_OPERATION_EXPR_EXPR(>)
BINARY_OPERATION_EXPR_EXPR(<)
BINARY_OPERATION_EXPR_EXPR(>=)
BINARY_OPERATION_EXPR_EXPR(<=)
BINARY_OPERATION_EXPR_EXPR(!=)
BINARY_OPERATION_EXPR_EXPR(==)
BINARY_OPERATION_EXPR_EXPR(+)
BINARY_OPERATION_EXPR_EXPR(-)
BINARY_OPERATION_EXPR_EXPR(*)
BINARY_OPERATION_EXPR_EXPR(/)

#define BINARY_OPERATION_EXPR_INT(X) \
SMTExpr operator X(SMTExpr const & A, int B) { \
	return SMTExpr(&A.getSMTFactory(), A.Expr X B); \
}

BINARY_OPERATION_EXPR_INT(|)
BINARY_OPERATION_EXPR_INT(^)
BINARY_OPERATION_EXPR_INT(&)
BINARY_OPERATION_EXPR_INT(>)
BINARY_OPERATION_EXPR_INT(<)
BINARY_OPERATION_EXPR_INT(>=)
BINARY_OPERATION_EXPR_INT(<=)
BINARY_OPERATION_EXPR_INT(!=)
BINARY_OPERATION_EXPR_INT(==)
BINARY_OPERATION_EXPR_INT(+)
BINARY_OPERATION_EXPR_INT(-)
BINARY_OPERATION_EXPR_INT(*)
BINARY_OPERATION_EXPR_INT(/)

#define BINARY_OPERATION_INT_EXPR(X) \
SMTExpr operator X(int A, SMTExpr const & B) { \
	return SMTExpr(&B.getSMTFactory(), A X B.Expr); \
}

BINARY_OPERATION_INT_EXPR(|)
BINARY_OPERATION_INT_EXPR(^)
BINARY_OPERATION_INT_EXPR(&)
BINARY_OPERATION_INT_EXPR(>)
BINARY_OPERATION_INT_EXPR(<)
BINARY_OPERATION_INT_EXPR(>=)
BINARY_OPERATION_INT_EXPR(<=)
BINARY_OPERATION_INT_EXPR(!=)
BINARY_OPERATION_INT_EXPR(==)
BINARY_OPERATION_INT_EXPR(+)
BINARY_OPERATION_INT_EXPR(-)
BINARY_OPERATION_INT_EXPR(*)
BINARY_OPERATION_INT_EXPR(/)

llvm::raw_ostream& operator<<(llvm::raw_ostream& Out, SMTExpr E) {
	z3::expr& Z3Expr = E.Expr;
	Out << Z3_ast_to_string(Z3Expr.ctx(), Z3Expr);
	return Out;
}

std::ostream & operator<<(std::ostream& Out, SMTExpr const & N) {
	Out << N.Expr;
	return Out;
}

/*==-- SMTExprVec --==*/

SMTExprVec::SMTExprVec(SMTFactory* F, std::shared_ptr<z3::expr_vector> Vec) : SMTObject(F),
		ExprVec(Vec) {
}

SMTExprVec::SMTExprVec(const SMTExprVec& Vec) : SMTObject(Vec),
		ExprVec(Vec.ExprVec) {
}

SMTExprVec& SMTExprVec::operator=(const SMTExprVec& Vec) {
	SMTObject::operator =(Vec);
	if (this != &Vec) {
		this->ExprVec = Vec.ExprVec;
	}
	return *this;
}

unsigned SMTExprVec::size() const {
	if (ExprVec.get() == nullptr) {
		return 0;
	}
	return ExprVec->size();
}

void SMTExprVec::push_back(SMTExpr E, bool Enforce) {
	if (E.isTrue() && !Enforce) {
		return;
	}
	if (ExprVec.get() == nullptr) {
		ExprVec = std::make_shared<z3::expr_vector>(E.Expr.ctx());
	}
	ExprVec->push_back(E.Expr);
}

SMTExpr SMTExprVec::operator[](unsigned I) const {
	assert(I < size());
	return SMTExpr(&getSMTFactory(), (*ExprVec)[I]);
}

bool SMTExprVec::empty() const {
	return size() == 0;
}

SMTExprVec SMTExprVec::copy() {
	if (size() == 0) {
		// std::shared_ptr<z3::expr_vector> Ret(nullptr);
		assert(!ExprVec.get());
		return *this;
	}

	std::shared_ptr<z3::expr_vector> Ret = std::make_shared<z3::expr_vector>(ExprVec->ctx());
	Ret->resize(ExprVec->size());
	for (unsigned Idx = 0; Idx < ExprVec->size(); Idx++) {
		Z3_ast_vector_set(ExprVec->ctx(), *Ret, Idx, (*ExprVec)[Idx]);
	}
	return SMTExprVec(&getSMTFactory(), Ret);
}

/// *this = *this && v2
void SMTExprVec::mergeWithAnd(const SMTExprVec& Vec) {
	if (Vec.size() == 0) {
		return;
	}

	if (ExprVec.get() == nullptr) {
		ExprVec = std::make_shared<z3::expr_vector>(Vec.ExprVec->ctx());
	}

	for (size_t I = 0; I < Vec.size(); I++) {
		ExprVec->push_back((*Vec.ExprVec)[I]);
	}
}

SMTExprVec SMTExprVec::merge(SMTExprVec Vec1, SMTExprVec Vec2) {
	if (Vec1.size() < Vec2.size()) {
		Vec2.mergeWithAnd(Vec1);
		return Vec2;
	} else {
		Vec1.mergeWithAnd(Vec2);
		return Vec1;
	}
}

/// *this = *this || v2
void SMTExprVec::mergeWithOr(const SMTExprVec& Vec) {
	SMTExprVec Ret = getSMTFactory().createEmptySMTExprVec();
	if (Vec.empty() || empty()) {
		Ret.push_back(getSMTFactory().createBoolVal(true));
		*this = Ret;
		return;
	}

	SMTExpr E1 = this->toAndExpr();
	SMTExpr E2 = Vec.toAndExpr();
	Ret.ExprVec->push_back(E1.Expr || E2.Expr);
	*this = Ret;
	return;
}

SMTExpr SMTExprVec::toAndExpr() const {
	if (empty()) {
		return getSMTFactory().createBoolVal(true);
	}

	z3::expr t = ExprVec->ctx().bool_val(true), f = ExprVec->ctx().bool_val(false);

	Z3_ast* Args = new Z3_ast[ExprVec->size()];
	unsigned ActualSz = 0, Index = 0;
	for (unsigned I = 0, E = ExprVec->size(); I < E; I++) {
		z3::expr e = (*ExprVec)[I];
		if (z3::eq(e, t)) {
			continue;
		} else if (z3::eq(e, f)) {
			delete[] Args;
			return getSMTFactory().createBoolVal(false);
		}
		Args[ActualSz++] = (*ExprVec)[I];
		Index = I;
	}

	if (ActualSz == 1) {
		delete[] Args;
		return SMTExpr(&getSMTFactory(), (*ExprVec)[Index]);
	}

	SMTExpr Ret(&getSMTFactory(), to_expr(ExprVec->ctx(), Z3_mk_and(ExprVec->ctx(), ActualSz, Args)));
	delete[] Args;

	return Ret;
}

SMTExpr SMTExprVec::toOrExpr() const {
	if (empty()) {
		return getSMTFactory().createBoolVal(true);
	}

	z3::expr t = ExprVec->ctx().bool_val(true), f = ExprVec->ctx().bool_val(false);

	Z3_ast* Args = new Z3_ast[ExprVec->size()];
	unsigned ActualSz = 0, Index = 0;
	for (unsigned I = 0, E = ExprVec->size(); I < E; I++) {
		z3::expr e = (*ExprVec)[I];
		if (z3::eq(e, f)) {
			continue;
		} else if (z3::eq(e, t)) {
			delete[] Args;
			return SMTExpr(&getSMTFactory(), t);
		}
		Args[ActualSz++] = (*ExprVec)[I];
		Index = I;
	}

	if (ActualSz == 1) {
		delete[] Args;
		return SMTExpr(&getSMTFactory(), (*ExprVec)[Index]);
	}

	SMTExpr Ret(&getSMTFactory(), to_expr(ExprVec->ctx(), Z3_mk_or(ExprVec->ctx(), ActualSz, Args)));
	delete[] Args;
	return Ret;
}

unsigned SMTExprVec::constraintSize() const {
	unsigned Ret = 0;
	std::map<SMTExpr, unsigned, SMTExprComparator> Cache;
	for (unsigned I = 0; I < this->size(); I++) {
		Ret += (*this)[I].size(Cache);
	}
	return Ret;
}

llvm::raw_ostream& operator<<(llvm::raw_ostream& Out, SMTExprVec Vec) {
	if (Vec.ExprVec.get() == nullptr) {
		Out << "(empty vector)";
		return Out;
	}
	Out << Z3_ast_vector_to_string(Vec.ExprVec->ctx(), *Vec.ExprVec);
	return Out;
}

std::ostream & operator<<(std::ostream& Out, SMTExprVec Vec) {
	if (Vec.ExprVec.get() == nullptr) {
		Out << "(empty vector)";
		return Out;
	}
	Out << *Vec.ExprVec;
	return Out;
}
