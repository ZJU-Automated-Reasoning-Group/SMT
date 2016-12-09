/**
 * Authors: Qingkai
 */

#include <llvm/Support/Debug.h>
#include <llvm/Support/CommandLine.h>

#include "SMTFactory.h"

#define DEBUG_TYPE "smt-fctry"

SMTExprVec SMTFactory::translate(const SMTExprVec & Exprs) {
	if (Exprs.empty()) {
		return this->createEmptySMTExprVec();
	}

	std::lock_guard<std::mutex> L(Exprs.getSMTFactory().getFactoryLock());

	std::shared_ptr<z3::expr_vector> Vec(new z3::expr_vector(z3::expr_vector(Ctx, Z3_ast_vector_translate(Exprs.ExprVec->ctx(), *Exprs.ExprVec, Ctx))));
	SMTExprVec Ret(this, Vec);
	return Ret;
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
				SMTExpr NewExpr(this, z3::expr(Ctx, Z3_mk_const(Ctx, Z3_mk_string_symbol(Ctx, NewSymbol.c_str()), OldExpr.Expr.get_sort())));
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
	z3::solver Ret(Ctx);
	return SMTSolver(this, Ret);
}

SMTExprVec SMTFactory::createBoolSMTExprVec(bool Content, size_t Size) {
	SMTExprVec Ret = createEmptySMTExprVec();
	for (unsigned J = 0; J < Size; J++) {
		Ret.push_back(createBoolVal(Content), true);
	}
	return Ret;
}

SMTExprVec SMTFactory::createSMTExprVec(const std::vector<SMTExpr>& ExprVec) {
	SMTExprVec Ret = createEmptySMTExprVec();
	for (size_t J = 0; J < ExprVec.size(); J++) {
		const SMTExpr& Ex = ExprVec[J];
		assert(&Ret.getSMTFactory() == &Ex.getSMTFactory() && "Contexts are not compatible!");
		Ret.push_back(Ex);
	}
	return Ret;
}

SMTExprVec SMTFactory::parseSMTLib2File(const std::string& FileName) {
	Z3_ast Ast = Z3_parse_smtlib2_file(Ctx, FileName.c_str(), 0, 0, 0, 0, 0, 0);
	z3::expr Whole(Ctx, Ast);
	SMTExprVec Assertions = createEmptySMTExprVec();
	for (unsigned int I = 0; I < Whole.num_args(); I++) {
		Assertions.push_back(SMTExpr(this, Whole.arg(I)));
	}
	return Assertions;
}

SMTExpr SMTFactory::parseSMTLib2String(const std::string& Raw) {
    Z3_ast Ast = Z3_parse_smtlib2_string(Ctx, Raw.c_str(), 0, 0, 0, 0, 0, 0);
    z3::expr Whole(Ctx, Ast);
    return SMTExpr(this, Whole);
}

SMTExpr SMTFactory::createEmptySMTExpr() {
	return SMTExpr(this, z3::expr(Ctx));
}

SMTExprVec SMTFactory::createEmptySMTExprVec() {
	std::shared_ptr<z3::expr_vector> Vec(nullptr);
	return SMTExprVec(this, Vec);
}

SMTExpr SMTFactory::createRealConst(const std::string& Name) {
	return SMTExpr(this, Ctx.real_const(Name.c_str()));
}

SMTExpr SMTFactory::createRealVal(const std::string& ValStr) {
	return SMTExpr(this, Ctx.real_val(ValStr.c_str()));
}

SMTExpr SMTFactory::createBitVecConst(const std::string& Name, uint64_t Sz) {
	return SMTExpr(this, Ctx.bv_const(Name.c_str(), Sz));
}

SMTExpr SMTFactory::createTemporaryBitVecConst(uint64_t Sz) {
	std::string Symbol("temp_");
	Symbol.append(std::to_string(TempSMTVaraibleIndex++));
	return SMTExpr(this, Ctx.bv_const(Symbol.c_str(), Sz));
}

SMTExpr SMTFactory::createBitVecVal(const std::string& ValStr, uint64_t Sz) {
	return SMTExpr(this, Ctx.bv_val(ValStr.c_str(), Sz));
}

SMTExpr SMTFactory::createBoolConst(const std::string& Name) {
	return SMTExpr(this, Ctx.bool_const(Name.c_str()));
}

SMTExpr SMTFactory::createBitVecVal(uint64_t Val, uint64_t Sz) {
	return SMTExpr(this, Ctx.bv_val((__uint64 ) Val, Sz));
}

SMTExpr SMTFactory::createIntVal(int Val) {
	return SMTExpr(this, Ctx.int_val(Val));
}

SMTExpr SMTFactory::createSelect(SMTExpr& Vec, SMTExpr Index) {
	return SMTExpr(this, z3::expr(Ctx, Z3_mk_select(Ctx, Vec.Expr, Index.Expr)));
}

SMTExpr SMTFactory::createStore(SMTExpr& Vec, SMTExpr Index, SMTExpr& Val2Store) {
	return SMTExpr(this, z3::expr(Ctx, Z3_mk_store(Ctx, Vec.Expr, Index.Expr, Val2Store.Expr)));
}


SMTExpr SMTFactory::createIntRealArrayConstFromStringSymbol(const std::string& Name) {
	Z3_sort ArraySort = Ctx.array_sort(Ctx.int_sort(), Ctx.real_sort());
	return SMTExpr(this, z3::expr(Ctx, Z3_mk_const(Ctx, Z3_mk_string_symbol(Ctx, Name.c_str()), ArraySort)));
}

SMTExpr SMTFactory::createIntBvArrayConstFromStringSymbol(const std::string& Name, uint64_t Sz) {
	Z3_sort ArraySort = Ctx.array_sort(Ctx.int_sort(), Ctx.bv_sort(Sz));
	return SMTExpr(this, z3::expr(Ctx, Z3_mk_const(Ctx, Z3_mk_string_symbol(Ctx, Name.c_str()), ArraySort)));
}

SMTExpr SMTFactory::createIntDomainConstantArray(SMTExpr& ElmtExpr) {
	return SMTExpr(this, z3::expr(Ctx, Z3_mk_const_array(Ctx, Ctx.int_sort(), ElmtExpr.Expr)));
}

SMTExpr SMTFactory::createBoolVal(bool B) {
	return SMTExpr(this, Ctx.bool_val(B));
}
