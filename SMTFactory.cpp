/**
 * Authors: Qingkai
 */

#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <llvm/Support/Debug.h>
#include <llvm/Support/CommandLine.h>

#include "SMTFactory.h"

#define DEBUG_TYPE "smt-fctry"

static llvm::cl::opt<int> SolverTimeOut("solver-time-out", llvm::cl::init(-1), llvm::cl::desc("Set the timeout (ms) of the smt solver."));

SMTExprVec SMTFactory::translate(const SMTExprVec & Exprs) {
	SMTExprVec RetExprVec = this->createEmptySMTExprVec();
	for (unsigned ExprIdx = 0; ExprIdx < Exprs.size(); ExprIdx++) {
		SMTExpr OrigExpr = Exprs[ExprIdx];
		SMTExpr Ret(to_expr(ctx, Z3_translate(OrigExpr.z3_expr.ctx(), OrigExpr.z3_expr, ctx)));
		RetExprVec.push_back(Ret);
	}
	return RetExprVec;
}

std::pair<SMTExprVec, bool> SMTFactory::rename(const SMTExprVec& Exprs, const std::string& RenamingSuffix,
		std::unordered_map<std::string, SMTExpr>& Mapping, SMTExprPruner* Pruner) {

	DEBUG(llvm::dbgs() << "Start translating and pruning ...\n");

	SMTExprVec RetExprVec = this->createEmptySMTExprVec();
	bool RetBool = false; // the constraint is pruned?

	for (unsigned ExprIdx = 0; ExprIdx < Exprs.size(); ExprIdx++) {
		SMTExpr Ret = Exprs[ExprIdx];

		std::unordered_map<std::string, SMTExpr> LocalMapping;

		auto It = ExprRenamingCache.find(Ret);
		if (It != ExprRenamingCache.end()) {
			auto & Cache = ExprRenamingCache.at(Ret);
			if (Cache.WillBePruned) {
				RetBool = true;
				Ret = Cache.AfterBeingPruned;

				if (Ret.isTrue()) {
					continue;
				} else {
					LocalMapping = Cache.SymbolMapping;
				}
			} else {
				LocalMapping = Cache.SymbolMapping;
			}
		} else {
			SMTExprVec ToPrune = this->createEmptySMTExprVec();
			std::map<SMTExpr, bool, SMTExprComparator> Visited;

			bool AllPruned = visit(Ret, LocalMapping, ToPrune, Visited, Pruner);
			if (AllPruned) {
				RenamingUtility RU { true, createBoolVal(true), createEmptySMTExprVec(), LocalMapping };
				ExprRenamingCache.insert(std::make_pair(Ret, RU));

				RetBool = true;
				continue;
			} else if (ToPrune.size()) {
				SMTExprVec TrueVec = this->createBoolSMTExprVec(true, ToPrune.size());
				assert(ToPrune.size() == TrueVec.size());
				SMTExpr AfterSubstitution = Ret.substitute(ToPrune, TrueVec);
				RenamingUtility RU { true, AfterSubstitution, ToPrune, LocalMapping };
				ExprRenamingCache.insert(std::make_pair(Ret, RU));

				RetBool = true;
				Ret = AfterSubstitution;
			} else {
				RenamingUtility RU { false, createBoolVal(true), createEmptySMTExprVec(), LocalMapping };
				ExprRenamingCache.insert(std::make_pair(Ret, RU));
			}
		}

		// renaming
		assert(RenamingSuffix.find(' ') == std::string::npos);
		if (RenamingSuffix != "") {
			// Utility for replacement
			SMTExprVec From = createEmptySMTExprVec(), To = createEmptySMTExprVec();

			// Mapping.string + suffix --> new string + Mapping.expr.sort -> new expr
			for (auto& It : LocalMapping) {
				std::string OldSymbol = It.first;
				SMTExpr OldExpr = It.second;

				std::string NewSymbol = OldSymbol + RenamingSuffix;
				SMTExpr NewExpr(z3::expr(ctx, Z3_mk_const(ctx, Z3_mk_string_symbol(ctx, NewSymbol.c_str()), OldExpr.z3_expr.get_sort())));
				Mapping.insert(std::pair<std::string, SMTExpr>(OldSymbol, NewExpr));

				From.push_back(OldExpr);
				To.push_back(NewExpr);
			}

			if (From.size()) {
				assert(From.size() == To.size());
				Ret = Ret.substitute(From, To);
			}
		}

		RetExprVec.push_back(Ret);
	}

	DEBUG(llvm::dbgs() << "End translating and pruning ...\n");

	return std::make_pair(RetExprVec, RetBool);
}

bool SMTFactory::visit(SMTExpr& Expr2Visit, std::unordered_map<std::string, SMTExpr>& Mapping, SMTExprVec& ToPrune,
		std::map<SMTExpr, bool, SMTExprComparator>& Visited, SMTExprPruner* Pruner) {
	assert(Expr2Visit.isApp() && "Must be an app-only constraints.");

	if (Visited.count(Expr2Visit)) {
		return Visited[Expr2Visit];
	} else {
		auto It = ExprRenamingCache.find(Expr2Visit);
		if (It != ExprRenamingCache.end()) {
			auto & Cache = ExprRenamingCache.at(Expr2Visit);
			Mapping.insert(Cache.SymbolMapping.begin(), Cache.SymbolMapping.end());

			if (Cache.WillBePruned) {
				if (Cache.AfterBeingPruned.isTrue()) {
					Visited[Expr2Visit] = true;
					return true;
				} else {
					ToPrune.mergeWithAnd(Cache.ToPrune);
					Visited[Expr2Visit] = false;
					return false;
				}
			} else {
				Visited[Expr2Visit] = false;
				return false;
			}
		}

		unsigned NumArgs = Expr2Visit.numArgs();
		std::vector<bool> Arg2Prune;
		bool All2Prune = true;
		bool One2Prune = false;
		for (unsigned I = 0; I < NumArgs; I++) {
			SMTExpr Arg = Expr2Visit.getArg(I);
			bool WillPrune = visit(Arg, Mapping, ToPrune, Visited, Pruner);
			Arg2Prune.push_back(WillPrune);
			if (!WillPrune && All2Prune) {
				All2Prune = false;
			} else if (WillPrune && !One2Prune) {
				One2Prune = true;
			}
		}

		if (Expr2Visit.isConst() && !Expr2Visit.isNumeral()) {
			if (Pruner && Pruner->shouldPrune(Expr2Visit)) {
				Visited[Expr2Visit] = true;
				return true;
			} else {
				// If the node do not need to prune, we record it
				if (!Expr2Visit.isTrue() && !Expr2Visit.isFalse()) {
					std::string Symbol = Expr2Visit.getSymbol();
					assert(Symbol != "true" && Symbol != "false");
					Mapping.insert(std::pair<std::string, SMTExpr>(Symbol, Expr2Visit));
				}
			}
		} else if (Expr2Visit.isLogicAnd()) {
			if (All2Prune) {
				Visited[Expr2Visit] = true;
				return true;
			} else {
				// recording the expr to prune
				for (unsigned I = 0; I < NumArgs; I++) {
					if (Arg2Prune[I]) {
						ToPrune.push_back(Expr2Visit.getArg(I));
					}
				}
			}
		} else {
			if (One2Prune) {
				Visited[Expr2Visit] = true;
				return true;
			}
		}

		Visited[Expr2Visit] = false;
		return false;
	}
}

SMTSolver SMTFactory::createSMTSolver() {
	// z3::tactic t = z3::tactic(ctx, "simplify") & z3::tactic(ctx, "smt");
	// z3::solver ret = t.mk_solver();
	z3::solver ret(ctx);

	if (SolverTimeOut.getValue() > 0) {
		// FIXME still do not know why this
		// causes crashes in concurrent executions.
		z3::params p(ctx);
		// the unit is ms
		p.set("timeout", (unsigned) SolverTimeOut.getValue());
		ret.set(p);
	}
	return SMTSolver(ret);
}

SMTExprVec SMTFactory::createBoolSMTExprVec(bool Content, size_t Size) {
	SMTExprVec Ret = createEmptySMTExprVec();
	for (unsigned J = 0; J < Size; J++) {
		Ret.push_back(createBoolVal(Content));
	}
	return Ret;
}
