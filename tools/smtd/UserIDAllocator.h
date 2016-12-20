/*
 * UserIDAllocator.h
 *
 *  Created on: 20/12/2015
 *      Author: Qingkai
 */

#ifndef TOOLS_SMTD_USERIDALLOCATOR_H
#define TOOLS_SMTD_USERIDALLOCATOR_H

#include <llvm/Support/ManagedStatic.h>

#include <set>
#include <vector>

/// Allocate user ids in smtd
/// This class is not thread-safe.
class UserIDAllocator {
private:
    std::set<long> ExistingUsers;
    std::vector<long> FreeUserIDs;

    /// Make sure the class has only one instance using \c llvm::ManagedStatic.
    /// @{
    UserIDAllocator() {
    }

    friend void* llvm::object_creator<UserIDAllocator>();
    /// @}

public:
    ~UserIDAllocator() {}

    long allocate();

    void recycle(long ID);

public:
    static UserIDAllocator* getUserAllocator();
};

#endif /* TOOLS_SMTD_USERIDALLOCATOR_H */
