/*
 * UserIDAllocator.cpp
 *
 *  Created on: 20/12/2015
 *      Author: Qingkai
 */

#include <llvm/Support/ErrorHandling.h>

#include "UserIDAllocator.h"

static llvm::ManagedStatic<UserIDAllocator> Allocator;

UserIDAllocator* UserIDAllocator::getUserAllocator() {
    return &(*Allocator);
}

long UserIDAllocator::allocate() {
    static long ID = 100;

    if (FreeUserIDs.empty()) {
        bool Overflow = false;
        for (; ; ++ID) {
            if (ID > 0 && !ExistingUsers.count(ID)) {
                ExistingUsers.insert(ID);
                return ID;
            } else if (ID <= 0) { // overflow
                if (!Overflow) {
                    Overflow = true;
                    ID = 100;
                } else {
                    llvm_unreachable("Too many users and no available ID can be assigned.");
                }
            }
        }
        llvm_unreachable("Too many users and no available ID can be assigned.");
        return ID;
    } else {
        long Ret = FreeUserIDs.back();
        FreeUserIDs.pop_back();

        ExistingUsers.insert(Ret);
        return Ret;
    }
}

void UserIDAllocator::recycle(long ID) {
    auto It = ExistingUsers.find(ID);
    assert(It != ExistingUsers.end());

    FreeUserIDs.push_back(ID);
    ExistingUsers.erase(It);
}
