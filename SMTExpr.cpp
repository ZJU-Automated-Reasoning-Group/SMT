/**
 * Authors: Qingkai
 */

#include <vector>
#include <set>

#include <llvm/Support/Debug.h>

#include "SMTExpr.h"
#include "SMTFactory.h"

SMTExpr::SMTExpr(SMTFactory* F, z3::expr e) :
		Expr(e), Factory(F) {
}

SMTExpr::SMTExpr(SMTExpr const & e) :
		Expr(e.Expr), Factory(e.Factory) {
}

SMTExpr& SMTExpr::operator=(const SMTExpr& e) {
	if (this != &e) {
		this->Expr = e.Expr;
		this->Factory = e.Factory;
	}
	return *this;
}

SMTExpr SMTExpr::substitute(SMTExprVec& From, SMTExprVec& To) {
	return SMTExpr(Factory, Expr.substitute(From.ExprVec, To.ExprVec));
}

SMTExpr SMTExpr::localSimplify() {
	return SMTExpr(Factory, Expr.simplify());
}

SMTExpr SMTExpr::dilligSimplify(SMTExpr N, z3::solver& Solver4Sim, z3::context& Ctx) {
	if (!N.isLogicAnd() && !N.isLogicOr()) {
		// A leaf
		Solver4Sim.push();
		Solver4Sim.add(N.Expr);
		if (Solver4Sim.check() == z3::check_result::unsat) {
			Solver4Sim.pop();
			return SMTExpr(Factory, Ctx.bool_val(false));
		}
		Solver4Sim.pop();
		Solver4Sim.push();
		Solver4Sim.add(!N.Expr);
		if (Solver4Sim.check() == z3::check_result::unsat) {
			Solver4Sim.pop();
			return SMTExpr(Factory, Ctx.bool_val(true));
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
					return SMTExpr(Factory, Ctx.bool_val(true));
				}
			} else if (N.getArg(I).isFalse()) {
				if (N.isLogicAnd()) {
					return SMTExpr(Factory, Ctx.bool_val(false));
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
				SMTExpr Alpha(Factory, to_expr(Ctx, Z3_mk_and(Ctx, C.size() - 1, Args)));
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
					return SMTExpr(Factory, Ctx.bool_val(true));
				} else if (NewCi.isFalse() && N.isLogicAnd()) {
					return SMTExpr(Factory, Ctx.bool_val(false));
				}
			}

			// FIXME
			FixedPoint = true;
		}

		if (N.isLogicAnd()) {
			Z3_ast* Args = new Z3_ast[C.size()];
			size_t j = 0;
			for (size_t i = 0; i < C.size(); i++) {
				if (C[i].isTrue()) {
					continue;
				}
				Args[j++] = C[i].Expr;
			}

			if (j == 1) {
				SMTExpr Ret(Factory, to_expr(Ctx, Args[0]));
				delete[] Args;
				return Ret;
			}

			if (j == 0) {
				delete[] Args;
				return SMTExpr(Factory, Ctx.bool_val(true));
			}

			SMTExpr Ret(Factory, to_expr(Ctx, Z3_mk_and(Ctx, j, Args)));
			delete[] Args;

			return Ret;
		} else {
			// is logic OR
			Z3_ast* Args = new Z3_ast[C.size()];
			size_t j = 0;
			for (size_t i = 0; i < C.size(); i++) {
				if (C[i].isFalse()) {
					continue;
				}
				Args[j++] = C[i].Expr;
			}

			if (j == 1) {
				SMTExpr Ret(Factory, to_expr(Ctx, Args[0]));
				delete[] Args;
				return Ret;
			}

			if (j == 0) {
				delete[] Args;
				return SMTExpr(Factory, Ctx.bool_val(false));
			}

			SMTExpr Ret(Factory, to_expr(Ctx, Z3_mk_or(Ctx, j, Args)));
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
	return SMTExpr(Factory, Expr.body());
}

SMTExpr SMTExpr::getArg(unsigned i) const {
	return SMTExpr(Factory, Expr.arg(i));
}

SMTExpr SMTExpr::bv12bool() {
	if (Expr.is_array()) {
		assert(Expr.get_sort().array_range().is_bv());
		unsigned bvSz = Expr.get_sort().array_range().bv_size();
		assert(bvSz == 1);
		auto func = (Expr.ctx().bv_val("0", 1) == Expr.ctx().bv_val("0", 1)).decl();

		z3::expr const_bv1 = const_array(Expr.get_sort().array_domain(), Expr.ctx().bv_val("1", 1));
		Z3_ast mapargs[2] = { Expr, const_bv1 };

		return SMTExpr(Factory, z3::expr(Expr.ctx(), Z3_mk_map(Expr.ctx(), func, 2, mapargs)));
	} else {
		assert(Expr.is_bv() && Expr.get_sort().bv_size() == 1);
		return SMTExpr(Factory, Expr == Expr.ctx().bv_val(1, 1));
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
		return SMTExpr(Factory, bvret);
	} else {
		assert(Expr.is_bool());
		return SMTExpr(Factory, ite(Expr, ctx.bv_val(1, 1), ctx.bv_val(0, 1)));
	}
}

SMTExpr SMTExpr::real2int() {
	z3::context & ctx = Expr.ctx();
	if (Expr.is_array()) {
		assert(Expr.get_sort().array_range().is_real());
		auto func = z3::expr(ctx, Z3_mk_real2int(ctx, ctx.real_val("0.0"))).decl();

		Z3_ast mapargs[1] = { Expr };
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs)));
	} else {
		assert(Expr.is_real());
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_real2int(ctx, Expr)));
	}
}

SMTExpr SMTExpr::int2real() {
	z3::context & ctx = Expr.ctx();
	if (Expr.is_array()) {
		assert(Expr.get_sort().array_range().is_int());

		auto func = to_real(Expr).decl();
		Z3_ast mapargs[1] = { Expr };
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs)));
	} else {
		assert(Expr.is_int());
		return SMTExpr(Factory, to_real(Expr));
	}
}

SMTExpr SMTExpr::int2bv(unsigned sz) {
	z3::context & ctx = Expr.ctx();
	if (Expr.is_array()) {
		assert(Expr.get_sort().array_range().is_int());
		auto func = z3::expr(ctx, Z3_mk_int2bv(ctx, sz, ctx.int_val("0"))).decl();

		Z3_ast mapargs[1] = { Expr };
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs)));
	} else {
		assert(Expr.is_int());
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_int2bv(ctx, sz, Expr)));
	}
}

SMTExpr SMTExpr::bv2int(bool isSigned) {
	z3::context & ctx = Expr.ctx();
	if (Expr.is_array()) {
		assert(Expr.get_sort().array_range().is_bv());
		unsigned bvSz = Expr.get_sort().array_range().bv_size();
		auto func = z3::expr(ctx, Z3_mk_bv2int(ctx, ctx.bv_val("0", bvSz), isSigned)).decl();

		Z3_ast mapargs[1] = { Expr };
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs)));
	} else {
		assert(Expr.is_bv());
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_bv2int(ctx, Expr, isSigned)));
	}
}

SMTExpr SMTExpr::basic_zext(unsigned sz) {
	z3::context& ctx = Expr.ctx();
	if (Expr.is_array()) {
		assert(Expr.get_sort().array_range().is_bv());
		unsigned bvSz = Expr.get_sort().array_range().bv_size();
		auto func = z3::expr(ctx, Z3_mk_zero_ext(ctx, sz, ctx.bv_val("10", bvSz))).decl();
		Z3_ast mapargs[1] = { Expr };
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs)));
	} else {
		assert(Expr.is_bv());
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_zero_ext(ctx, sz, Expr)));
	}
}

SMTExpr SMTExpr::basic_add(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return *this + b;
	}
}

SMTExpr SMTExpr::basic_sub(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return *this - b;
	}
}

SMTExpr SMTExpr::basic_mul(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return *this * b;
	}
}

SMTExpr SMTExpr::basic_udiv(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_bvudiv(ctx, Expr, b.Expr)));
	}
}

SMTExpr SMTExpr::basic_sdiv(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return *this / b;
	}
}

SMTExpr SMTExpr::basic_urem(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_bvurem(ctx, Expr, b.Expr)));
	}
}

SMTExpr SMTExpr::basic_srem(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else if (Expr.is_real() && b.Expr.is_real()) {
		// z3 only accept rem between integers
		z3::expr ch1ToInt = z3::expr(ctx, Z3_mk_real2int(ctx, Expr));
		z3::expr ch2ToInt = z3::expr(ctx, Z3_mk_real2int(ctx, b.Expr));
		z3::expr remResult = z3::expr(ctx, Z3_mk_rem(ctx, ch1ToInt, ch2ToInt));
		z3::expr remResult2Real = z3::expr(ctx, Z3_mk_int2real(ctx, remResult));
		return SMTExpr(Factory, remResult2Real);
	} else {
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_bvsrem(ctx, Expr, b.Expr)));
	}
}

SMTExpr SMTExpr::basic_shl(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_bvshl(ctx, Expr, b.Expr)));
	}
}

SMTExpr SMTExpr::basic_ashr(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_bvashr(ctx, Expr, b.Expr)));
	}
}

SMTExpr SMTExpr::basic_lshr(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_bvlshr(ctx, Expr, b.Expr)));
	}
}

SMTExpr SMTExpr::basic_and(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return *this & b;
	}
}

SMTExpr SMTExpr::basic_or(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return *this | b;
	}
}

SMTExpr SMTExpr::basic_xor(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return *this ^ b;
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return *this != b;
	}
}

SMTExpr SMTExpr::basic_ugt(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_bvugt(ctx, Expr, b.Expr)));
	}
}

SMTExpr SMTExpr::basic_uge(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_bvuge(ctx, Expr, b.Expr)));
	}
}

SMTExpr SMTExpr::basic_sgt(SMTExpr &b) {
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

		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return *this > b;
	}
}

SMTExpr SMTExpr::basic_sge(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return *this >= b;
	}
}

SMTExpr SMTExpr::basic_ult(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_bvult(ctx, Expr, b.Expr)));
	}
}

SMTExpr SMTExpr::basic_ule(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_bvule(ctx, Expr, b.Expr)));
	}
}

SMTExpr SMTExpr::basic_slt(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return *this < b;
	}
}

SMTExpr SMTExpr::basic_sle(SMTExpr &b) {
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 2, mapargs)));
	} else {
		return *this <= b;
	}
}

SMTExpr SMTExpr::basic_extract(unsigned high, unsigned low) {
	z3::context& ctx = Expr.ctx();
	if (Expr.is_array()) {
		assert(Expr.get_sort().array_range().is_bv());
		unsigned bvSz = Expr.get_sort().array_range().bv_size();
		auto func = z3::expr(ctx, Z3_mk_extract(ctx, high, low, ctx.bv_val("10", bvSz))).decl();
		Z3_ast mapargs[1] = { Expr };
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs)));
	} else {
		assert(Expr.is_bv());
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_extract(ctx, high, low, Expr)));
	}
}

SMTExpr SMTExpr::basic_sext(unsigned sz) {
	z3::context& ctx = Expr.ctx();
	if (Expr.is_array()) {
		assert(Expr.get_sort().array_range().is_bv());
		unsigned bvSz = Expr.get_sort().array_range().bv_size();
		auto func = z3::expr(ctx, Z3_mk_sign_ext(ctx, sz, ctx.bv_val("10", bvSz))).decl();
		Z3_ast mapargs[1] = { Expr };
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 1, mapargs)));
	} else {
		assert(Expr.is_bv());
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_sign_ext(ctx, sz, Expr)));
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
		return SMTExpr(Factory, z3::expr(ctx, Z3_mk_map(ctx, func, 3, mapargs)));
	} else {
		return SMTExpr(Factory, ite(condition, TBValue.Expr, FBValue.Expr));
	}
}

SMTFactory& SMTExpr::getSMTFactory() {
	return *Factory;
}

SMTExpr operator!(SMTExpr const & a) {
	if (a.isLogicNot()) {
		assert(a.numArgs() == 1);
		return a.getArg(0);
	} else if (a.isTrue()) {
		return a.Factory->createBoolVal(false);
	} else if (a.isFalse()) {
		return a.Factory->createBoolVal(true);
	} else {
		return SMTExpr(a.Factory, !a.Expr);
	}
}

SMTExpr operator||(SMTExpr const & a, SMTExpr const & b) {
	if (a.isTrue() || b.isTrue()) {
		return a.Factory->createBoolVal(true);
	} else if (a.isFalse()) {
		return b;
	} else if (b.isFalse()) {
		return a;
	}

	return SMTExpr(a.Factory, a.Expr || b.Expr);
}

SMTExpr operator||(SMTExpr const & a, bool b) {
	return a || a.Factory->createBoolVal(b);
}

SMTExpr operator||(bool a, SMTExpr const & b) {
	return b || a;
}

SMTExpr operator&&(SMTExpr const & a, SMTExpr const & b) {
	if (a.isFalse() || b.isFalse()) {
		return a.Factory->createBoolVal(false);
	} else if (a.isTrue()) {
		return b;
	} else if (b.isTrue()) {
		return a;
	}

	return SMTExpr(a.Factory, a.Expr && b.Expr);
}

SMTExpr operator&&(SMTExpr const & a, bool b) {
	return a && a.Factory->createBoolVal(b);
}

SMTExpr operator&&(bool a, SMTExpr const & b) {
	return b && a;
}

SMTExpr operator==(SMTExpr const & a, SMTExpr const & b) {
	assert(a.isSameSort(b));
	return SMTExpr(a.Factory, a.Expr == b.Expr);
}

SMTExpr operator==(SMTExpr const & a, int b) {
	return SMTExpr(a.Factory, a.Expr == b);
}

SMTExpr operator==(int a, SMTExpr const & b) {
	return SMTExpr(b.Factory, a == b.Expr);
}

SMTExpr operator!=(SMTExpr const & a, SMTExpr const & b) {
	return SMTExpr(a.Factory, a.Expr != b.Expr);
}

SMTExpr operator!=(SMTExpr const & a, int b) {
	return SMTExpr(a.Factory, a.Expr != b);
}

SMTExpr operator!=(int a, SMTExpr const & b) {
	return SMTExpr(b.Factory, a != b.Expr);
}

SMTExpr operator+(SMTExpr const & a, SMTExpr const & b) {
	return SMTExpr(a.Factory, a.Expr + b.Expr);
}

SMTExpr operator+(SMTExpr const & a, int b) {
	return SMTExpr(a.Factory, a.Expr + b);
}

SMTExpr operator+(int a, SMTExpr const & b) {
	return SMTExpr(b.Factory, a + b.Expr);
}

SMTExpr operator*(SMTExpr const & a, SMTExpr const & b) {
	return SMTExpr(a.Factory, a.Expr * b.Expr);
}

SMTExpr operator*(SMTExpr const & a, int b) {
	return SMTExpr(a.Factory, a.Expr * b);
}

SMTExpr operator*(int a, SMTExpr const & b) {
	return SMTExpr(b.Factory, a * b.Expr);
}

SMTExpr operator/(SMTExpr const & a, SMTExpr const & b) {
	return SMTExpr(a.Factory, a.Expr / b.Expr);
}

SMTExpr operator/(SMTExpr const & a, int b) {
	return SMTExpr(a.Factory, a.Expr / b);
}

SMTExpr operator/(int a, SMTExpr const & b) {
	return SMTExpr(b.Factory, a / b.Expr);
}

SMTExpr operator-(SMTExpr const & a, SMTExpr const & b) {
	return SMTExpr(a.Factory, a.Expr - b.Expr);
}

SMTExpr operator-(SMTExpr const & a, int b) {
	return SMTExpr(a.Factory, a.Expr - b);
}

SMTExpr operator-(int a, SMTExpr const & b) {
	return SMTExpr(b.Factory, a - b.Expr);
}

SMTExpr operator-(SMTExpr const & a) {
	return SMTExpr(a.Factory, -a.Expr);
}

SMTExpr operator<=(SMTExpr const & a, SMTExpr const & b) {
	return SMTExpr(a.Factory, a.Expr <= b.Expr);
}

SMTExpr operator<=(SMTExpr const & a, int b) {
	return SMTExpr(a.Factory, a.Expr <= b);
}

SMTExpr operator<=(int a, SMTExpr const & b) {
	return SMTExpr(b.Factory, a <= b.Expr);
}

SMTExpr operator<(SMTExpr const & a, SMTExpr const & b) {
	return SMTExpr(a.Factory, a.Expr < b.Expr);
}

SMTExpr operator<(SMTExpr const & a, int b) {
	return SMTExpr(a.Factory, a.Expr < b);
}

SMTExpr operator<(int a, SMTExpr const & b) {
	return SMTExpr(b.Factory, a < b.Expr);
}

SMTExpr operator>=(SMTExpr const & a, SMTExpr const & b) {
	return SMTExpr(a.Factory, a.Expr >= b.Expr);
}

SMTExpr operator>=(SMTExpr const & a, int b) {
	return SMTExpr(a.Factory, a.Expr >= b);
}

SMTExpr operator>=(int a, SMTExpr const & b) {
	return SMTExpr(b.Factory, a >= b.Expr);
}

SMTExpr operator>(SMTExpr const & a, SMTExpr const & b) {
	return SMTExpr(a.Factory, a.Expr > b.Expr);
}

SMTExpr operator>(SMTExpr const & a, int b) {
	return SMTExpr(a.Factory, a.Expr > b);
}

SMTExpr operator>(int a, SMTExpr const & b) {
	return SMTExpr(b.Factory, a > b.Expr);
}

SMTExpr operator&(SMTExpr const & a, SMTExpr const & b) {
	return SMTExpr(a.Factory, a.Expr & b.Expr);
}

SMTExpr operator&(SMTExpr const & a, int b) {
	return SMTExpr(a.Factory, a.Expr & b);
}

SMTExpr operator&(int a, SMTExpr const & b) {
	return SMTExpr(b.Factory, a & b.Expr);
}

SMTExpr operator^(SMTExpr const & a, SMTExpr const & b) {
	return SMTExpr(a.Factory, a.Expr ^ b.Expr);
}

SMTExpr operator^(SMTExpr const & a, int b) {
	return SMTExpr(a.Factory, a.Expr ^ b);
}

SMTExpr operator^(int a, SMTExpr const & b) {
	return SMTExpr(b.Factory, a ^ b.Expr);
}

SMTExpr operator|(SMTExpr const & a, SMTExpr const & b) {
	return SMTExpr(a.Factory, a.Expr | b.Expr);
}

SMTExpr operator|(SMTExpr const & a, int b) {
	return SMTExpr(a.Factory, a.Expr | b);
}

SMTExpr operator|(int a, SMTExpr const & b) {
	return SMTExpr(b.Factory, a | b.Expr);
}

SMTExpr operator~(SMTExpr const & a) {
	return SMTExpr(a.Factory, ~a.Expr);
}

SMTExprVec::SMTExprVec(SMTFactory* F, z3::expr_vector Vec) :
		ExprVec(Vec), Factory(F) {
}

SMTExprVec::SMTExprVec(SMTExprVec const &Vec) :
		ExprVec(Vec.ExprVec), Factory(Vec.Factory) {
}

SMTExprVec& SMTExprVec::operator=(const SMTExprVec& Vec) {
	if (this != &Vec) {
		this->ExprVec = Vec.ExprVec;
		this->Factory = Vec.Factory;
	}
	return *this;
}

unsigned SMTExprVec::size() const {
	return ExprVec.size();
}

void SMTExprVec::push_back(SMTExpr e) {
	if (e.isTrue()) {
		return;
	}
	ExprVec.push_back(e.Expr);
}

SMTExpr SMTExprVec::operator[](int i) const {
	return SMTExpr(Factory, ExprVec[i]);
}

bool SMTExprVec::empty() const {
	return ExprVec.empty();
}

SMTExprVec SMTExprVec::copy() {
	z3::expr_vector Ret = z3::expr_vector(ExprVec.ctx());
	Ret.resize(ExprVec.size());
	for (unsigned Idx = 0; Idx < ExprVec.size(); Idx++) {
		Z3_ast_vector_set(ExprVec.ctx(), Ret, Idx, ExprVec[Idx]);
	}
	return SMTExprVec(Factory, Ret);
}

/// *this = *this && v2
void SMTExprVec::mergeWithAnd(const SMTExprVec& v2) {
	for (size_t i = 0; i < v2.size(); i++) {
		ExprVec.push_back(v2.ExprVec[i]);
	}
}

SMTExprVec SMTExprVec::merge(SMTExprVec v1, SMTExprVec v2) {
	if (v1.size() < v2.size()) {
		v2.mergeWithAnd(v1);
		return v2;
	} else {
		v1.mergeWithAnd(v2);
		return v1;
	}
}

/// *this = *this || v2
void SMTExprVec::mergeWithOr(const SMTExprVec& v2) {
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

SMTExpr SMTExprVec::toAndExpr() const {
	if (ExprVec.empty()) {
		return Factory->createBoolVal(true);
	}

	z3::expr t = ExprVec.ctx().bool_val(true), f = ExprVec.ctx().bool_val(false);

	Z3_ast* Args = new Z3_ast[ExprVec.size()];
	unsigned ActualSz = 0, Index = 0;
	for (unsigned I = 0, E = ExprVec.size(); I < E; I++) {
		z3::expr e = ExprVec[I];
		if (z3::eq(e, t)) {
			continue;
		} else if (z3::eq(e, f)) {
			delete[] Args;
			return Factory->createBoolVal(false);
		}
		Args[ActualSz++] = ExprVec[I];
		Index = I;
	}

	if (ActualSz == 1) {
		delete[] Args;
		return SMTExpr(Factory, ExprVec[Index]);
	}

	SMTExpr Ret(Factory, to_expr(ExprVec.ctx(), Z3_mk_and(ExprVec.ctx(), ActualSz, Args)));
	delete[] Args;

	return Ret;
}

SMTExpr SMTExprVec::toOrExpr() const {
	if (ExprVec.empty()) {
		return Factory->createBoolVal(true);
	}

	z3::expr t = ExprVec.ctx().bool_val(true), f = ExprVec.ctx().bool_val(false);

	Z3_ast* Args = new Z3_ast[ExprVec.size()];
	unsigned ActualSz = 0, Index = 0;
	for (unsigned I = 0, E = ExprVec.size(); I < E; I++) {
		z3::expr e = ExprVec[I];
		if (z3::eq(e, f)) {
			continue;
		} else if (z3::eq(e, t)) {
			delete[] Args;
			return SMTExpr(Factory, t);
		}
		Args[ActualSz++] = ExprVec[I];
		Index = I;
	}

	if (ActualSz == 1) {
		delete[] Args;
		return SMTExpr(Factory, ExprVec[Index]);
	}

	SMTExpr Ret(Factory, to_expr(ExprVec.ctx(), Z3_mk_or(ExprVec.ctx(), ActualSz, Args)));
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

SMTFactory& SMTExprVec::getSMTFactory() {
	return *Factory;
}

llvm::raw_ostream& operator<<(llvm::raw_ostream& out, SMTExpr n) {
	z3::expr& expr = n.Expr;
	out << Z3_ast_to_string(expr.ctx(), expr);
	return out;
}

llvm::raw_ostream& operator<<(llvm::raw_ostream& out, SMTExprVec vec) {
	z3::expr_vector& ExprVec = vec.ExprVec;
	out << Z3_ast_vector_to_string(ExprVec.ctx(), ExprVec);
	return out;
}
