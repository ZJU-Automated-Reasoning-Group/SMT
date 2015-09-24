/**
 * Authors: Qingkai
 */

#include <vector>
#include <set>

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>

#include "SMTExpr.h"

SMTExpr SMTExpr::substitute(SMTExprVec& From, SMTExprVec& To) {
	return SMTExpr(z3_expr.substitute(From.z3_expr_vec, To.z3_expr_vec));
}

SMTExpr SMTExpr::localSimplify() {
	return z3_expr.simplify();
}

SMTExpr SMTExpr::dilligSimplify(SMTExpr N, z3::solver& Solver4Sim, z3::context& Ctx) {
	if (!N.isLogicAnd() && !N.isLogicOr()) {
		// A leaf
		Solver4Sim.push();
		Solver4Sim.add(N.z3_expr);
		if (Solver4Sim.check() == z3::check_result::unsat) {
			Solver4Sim.pop();
			return SMTExpr(Ctx.bool_val(false));
		}
		Solver4Sim.pop();
		Solver4Sim.push();
		Solver4Sim.add(!N.z3_expr);
		if (Solver4Sim.check() == z3::check_result::unsat) {
			Solver4Sim.pop();
			return SMTExpr(Ctx.bool_val(true));
		}
		Solver4Sim.pop();
		return N;
	} else {
		// A connective (AND or OR)
		assert(N.isLogicAnd() || N.isLogicOr());

		std::vector<SMTExpr> C;
		std::set<SMTExpr, SMTExprComparator> CSet;
		for (unsigned I = 0, E = N.numArgs(); I < E; I++) {
			if (N.getArg(I).equals(Ctx.bool_val(true))) {
				if (N.isLogicAnd()) {
					continue;
				} else if (N.isLogicOr()) {
					return Ctx.bool_val(true);
				}
			} else if (N.getArg(I).equals(Ctx.bool_val(false))) {
				if (N.isLogicAnd()) {
					return Ctx.bool_val(false);
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
						Args[J] = !(*Candidate).z3_expr;
					} else {
						Args[J] = (*Candidate).z3_expr;
					}
				}
				SMTExpr Alpha(to_expr(Ctx, Z3_mk_and(Ctx, C.size() - 1, Args)));
				delete[] Args;

				SMTExpr& Ci = C[I];

				Solver4Sim.push();
				Solver4Sim.add(Alpha.z3_expr);
				SMTExpr NewCi = dilligSimplify(Ci, Solver4Sim, Ctx);
				Solver4Sim.pop();

				if (!z3::eq(Ci.z3_expr, NewCi.z3_expr)) {
					if (FixedPoint)
						FixedPoint = false;
					C[I] = NewCi;
				}

				if (NewCi.equals(Ctx.bool_val(true)) && N.isLogicOr()) {
					return Ctx.bool_val(true);
				} else if (NewCi.equals(Ctx.bool_val(false)) && N.isLogicAnd()) {
					return Ctx.bool_val(false);
				}
			}

			// FIXME
			FixedPoint = true;
		}

		if (N.isLogicAnd()) {
			Z3_ast* Args = new Z3_ast[C.size()];
			size_t j = 0;
			for (size_t i = 0; i < C.size(); i++) {
				if (C[i].equals(Ctx.bool_val(true))) {
					continue;
				}
				Args[j++] = C[i].z3_expr;
			}

			if (j == 1) {
				SMTExpr Ret(to_expr(Ctx, Args[0]));
				delete[] Args;
				return Ret;
			}

			if (j == 0) {
				delete[] Args;
				return Ctx.bool_val(true);
			}

			SMTExpr Ret(to_expr(Ctx, Z3_mk_and(Ctx, j, Args)));
			delete[] Args;

			return Ret;
		} else {
			// is logic OR
			Z3_ast* Args = new Z3_ast[C.size()];
			size_t j = 0;
			for (size_t i = 0; i < C.size(); i++) {
				if (C[i].equals(Ctx.bool_val(false))) {
					continue;
				}
				Args[j++] = C[i].z3_expr;
			}

			if (j == 1) {
				SMTExpr Ret(to_expr(Ctx, Args[0]));
				delete[] Args;
				return Ret;
			}

			if (j == 0) {
				delete[] Args;
				return Ctx.bool_val(false);
			}

			SMTExpr Ret(to_expr(Ctx, Z3_mk_or(Ctx, j, Args)));
			delete[] Args;

			return Ret;
		}
	}
}

SMTExpr SMTExpr::dilligSimplify() {
	z3::context& Ctx = z3_expr.ctx();
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

SMTExpr operator!(SMTExpr const & a) {
	if (a.isLogicNot()) {
		assert(a.numArgs() == 1);
		return a.getArg(0);
	} else if (a.equals(a.z3_expr.ctx().bool_val(true))) {
		return a.z3_expr.ctx().bool_val(false);
	} else if (a.equals(a.z3_expr.ctx().bool_val(false))) {
		return a.z3_expr.ctx().bool_val(true);
	} else {
		return !a.z3_expr;
	}
}

SMTExpr SMTExprVec::toAndExpr() const {
	if (z3_expr_vec.empty()) {
		return z3_expr_vec.ctx().bool_val(true);
	}

	z3::expr t = z3_expr_vec.ctx().bool_val(true), f = z3_expr_vec.ctx().bool_val(false);

	Z3_ast* Args = new Z3_ast[z3_expr_vec.size()];
	unsigned ActualSz = 0, Index = 0;
	for (unsigned I = 0, E = z3_expr_vec.size(); I < E; I++) {
		z3::expr e = z3_expr_vec[I];
		if (z3::eq(e, t)) {
			continue;
		} else if (z3::eq(e, f)) {
			delete[] Args;
			return f;
		}
		Args[ActualSz++] = z3_expr_vec[I];
		Index = I;
	}

	if (ActualSz == 1) {
		delete[] Args;
		return z3_expr_vec[Index];
	}

	SMTExpr Ret(to_expr(z3_expr_vec.ctx(), Z3_mk_and(z3_expr_vec.ctx(), ActualSz, Args)));
	delete[] Args;

	return Ret;
}

SMTExpr SMTExprVec::toOrExpr() const {
	if (z3_expr_vec.empty()) {
		return z3_expr_vec.ctx().bool_val(true);
	}

	z3::expr t = z3_expr_vec.ctx().bool_val(true), f = z3_expr_vec.ctx().bool_val(false);

	Z3_ast* Args = new Z3_ast[z3_expr_vec.size()];
	unsigned ActualSz = 0, Index = 0;
	for (unsigned I = 0, E = z3_expr_vec.size(); I < E; I++) {
		z3::expr e = z3_expr_vec[I];
		if (z3::eq(e, f)) {
			continue;
		} else if (z3::eq(e, t)) {
			delete[] Args;
			return t;
		}
		Args[ActualSz++] = z3_expr_vec[I];
		Index = I;
	}

	if (ActualSz == 1) {
		delete[] Args;
		return z3_expr_vec[Index];
	}

	SMTExpr Ret(to_expr(z3_expr_vec.ctx(), Z3_mk_or(z3_expr_vec.ctx(), ActualSz, Args)));
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
