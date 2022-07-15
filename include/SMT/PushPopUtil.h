/*
Author: rainoftime (copied from Util...)
*/

#ifndef SMT_PUSHPOPVEC_H
#define SMT_PUSHPOPVEC_H

#include <vector>
#include <cstddef>
#include <cassert>

template<typename T>
class PushPopVec {
protected:
	std::vector<size_t> CacheStack;
	std::vector<T> CacheVector;

	size_t LastSz = 0;

public:
	virtual ~PushPopVec() {
	}

	virtual void add(T N) {
		CacheVector.push_back(N);
	}

	void push_back(T N) {
		add(N);
	}

	template<typename VecTy>
	void addAll(VecTy &Vec) {
		for (unsigned I = 0; I < Vec.size(); I++) {
			push_back(Vec[I]);
		}
	}

	T& operator[](size_t Index) {
		assert(Index < size());
		return CacheVector[Index];
	}

	virtual void push() {
		CacheStack.push_back(CacheVector.size());
	}

	virtual void pop(unsigned N = 1) {
		size_t OrigSz = size();
		size_t TargetSz = OrigSz;
		while (N-- > 0) {
			assert(!CacheStack.empty());
			TargetSz = CacheStack.back();
			CacheStack.pop_back();
		}

		if (TargetSz != OrigSz) {
			assert(TargetSz < OrigSz);
			CacheVector.erase(CacheVector.begin() + TargetSz,
					CacheVector.end());
		}

		if (LastSz > CacheVector.size()) {
			LastSz = CacheVector.size();
		}
	}

	virtual void reset() {
		CacheStack.clear();
		CacheVector.clear();
		LastSz = 0;
	}

	size_t size() const {
		return CacheVector.size();
	}

	bool empty() const {
		return !size();
	}

	const std::vector<T>& getCacheVector() const {
		return CacheVector;
	}

	/// This function only gets the elements you have
	/// not got using this function.
	///
	/// The first call of this function returns the all
	/// the elements. A second call of the function
	/// with an argument \c false will return elements
	/// newly added. A second call of the function with
	/// an argument \c true will return all the elements.
	std::pair<typename std::vector<T>::iterator,
			typename std::vector<T>::iterator> getCacheVector(bool Restart) {
		if (Restart) {
			LastSz = 0;
		}

		size_t Start = LastSz;
		size_t End = CacheVector.size();

		LastSz = End;
		return std::make_pair(CacheVector.begin() + Start, CacheVector.end());
	}

};

#endif

