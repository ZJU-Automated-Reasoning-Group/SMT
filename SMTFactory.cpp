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

std::pair<SMTExprVec, bool> SMTFactory::rename(const SMTExprVec& Exprs, const std::string& RenamingSuffix, std::map<std::string, SMTExpr>& Mapping,
		SMTExprPruner* Pruner) {

	DEBUG(llvm::dbgs() << "Start translating and pruning ...\n");

	SMTExprVec RetExprVec = this->createEmptySMTExprVec();
	bool RetBool = false; // the constraint is pruned?

	for (unsigned ExprIdx = 0; ExprIdx < Exprs.size(); ExprIdx++) {
		SMTExpr Ret = Exprs[ExprIdx];

		SMTExprVec ToPrune = this->createEmptySMTExprVec();
		std::map<std::string, SMTExpr> LocalMapping;
		std::map<SMTExpr, bool, SMTExprComparator> Visited; // an ast node in a constraint should be pruned?
		if (visit(Ret, LocalMapping, ToPrune, Visited, Pruner)) {
			RetBool = true;
			continue;
		} else {
			// prune
			if (ToPrune.size()) {
				SMTExprVec TrueVec = this->createBoolSMTExprVec(true, ToPrune.size());
				assert(ToPrune.size() == TrueVec.size());
				Ret = Ret.substitute(ToPrune, TrueVec);
				RetBool = true;
			}
		}

		if (z3::eq(Ret.z3_expr, this->createBoolVal(true).z3_expr)) {
			continue;
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

bool SMTFactory::visit(SMTExpr& Expr2Visit, std::map<std::string, SMTExpr>& Mapping, SMTExprVec& ToPrune,
		std::map<SMTExpr, bool, SMTExprComparator>& Visited, SMTExprPruner* Pruner) {
	assert(Expr2Visit.isApp() && "Must be an app-only constraints.");

	if (Visited.count(Expr2Visit)) {
		return Visited[Expr2Visit];
	} else {
		unsigned NumArgs = Expr2Visit.numArgs();
		std::vector<bool> Arg2Prune(NumArgs);
		bool All2Prune = true;
		bool One2Prune = false;
		for (unsigned I = 0; I < NumArgs; I++) {
			SMTExpr Arg = Expr2Visit.getArg(I);
			Arg2Prune.push_back(visit(Arg, Mapping, ToPrune, Visited, Pruner));
			if (!Arg2Prune[I] && All2Prune) {
				All2Prune = false;
			} else if (Arg2Prune[I] && !One2Prune) {
				One2Prune = true;
			}
		}

		if (Expr2Visit.isConst() && !Expr2Visit.isNumeral()) {
			if (Pruner && Pruner->shouldPrune(Expr2Visit)) {
				Visited[Expr2Visit] = true;
				return true;
			} else {
				// If the node do not need to prune, we record it
				std::string symbol = Expr2Visit.getSymbol();
				if (symbol != "true" && symbol != "false") { // two special symbols
					Mapping.insert(std::pair<std::string, SMTExpr>(symbol, Expr2Visit));
				}
			}
		} else if (Expr2Visit.isLogicAnd()) {
			// if all should cut, just return true
			if (All2Prune) {
				Visited[Expr2Visit] = true;
				return true;
			} else {
				// recording the expr to prune
				for (unsigned I = 0; I < NumArgs; I++) {
					if (Arg2Prune[I])
						ToPrune.push_back(Expr2Visit.getArg(I));
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
	z3::tactic t = z3::tactic(ctx, "simplify") & z3::tactic(ctx, "smt");
	z3::solver ret = t.mk_solver();

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
