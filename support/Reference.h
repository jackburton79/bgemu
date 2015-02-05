/*
 * Copyright 2004-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2014-2015, Stefano Ceccherini
 * Distributed under the terms of the MIT License.
 */
 
#ifndef REFERENCE_H_
#define REFERENCE_H_

class Referenceable;
template <typename T = Referenceable>
class Reference {
public:
	Reference() {
		fTarget = NULL;
	}
	Reference(T* target)
		:
		fTarget(target)
	{
		if (fTarget != NULL)
			fTarget->Acquire();
	}
	
	Reference(const Reference<T>& ref)
		:
		fTarget(NULL)
	{
		SetTo(ref.Target());	
	};
	
	~Reference() {
		Unset();
	};

	T* Target() const {
		return fTarget;
	};

	void SetTo(T* target) {
		if (target != NULL)
			target->Acquire();
		
		Unset();
	
		fTarget = target;
	}
	
	void Unset() {
		if (fTarget != NULL) {
			fTarget->Release();
			fTarget = NULL;
		}
	}
	
	T* operator*() {
		return *fTarget;
	}
	
	Reference& operator=(const Reference<T>& ref) {
		SetTo(ref.Target());

		return *this;
	};
	
	Reference& operator=(T* other) {
		SetTo(other);
		return *this;
	}
	
	template <typename O>
	Reference& operator=(O* other) {
		SetTo(other->Target());	
	}
	
	bool operator==(const Reference<T>& other) const
	{
		return fTarget == other.fTarget;
	}

	bool operator==(const T* other) const
	{
		return fTarget == other;
	}

	bool operator!=(const Reference<T>& other) const
	{
		return fTarget != other.fTarget;
	}

	bool operator!=(const T* other) const
	{
		return fTarget != other;
	}

private:
	T* fTarget;
};





#endif /* REFERENCE_H_ */
