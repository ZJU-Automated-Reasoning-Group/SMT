/**
 * Authors: Qingkai
 */

#ifndef SMT_SMTOBJECT_H
#define SMT_SMTOBJECT_H

class SMTFactory;

class SMTObject {
private:
	SMTFactory* Context;

protected:
	SMTObject(SMTFactory* Ctx) : Context(Ctx) {
	}

	SMTObject(const SMTObject& Obj) : Context(Obj.Context) {
	}

	SMTObject& operator=(const SMTObject& Obj) {
		if (this != &Obj) {
			this->Context = Obj.Context;
		}
		return *this;
	}

public:
	virtual ~SMTObject() = 0;

	SMTFactory& getSMTFactory() const {
		return *Context;
	}
};

#endif
