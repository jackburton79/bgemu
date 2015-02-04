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
	Reference(T* target)
		:
		fTarget(target)
	{
		fTarget->Acquire();
	}
	
	Reference(const Reference& ref);
	~Reference() {
		Unset();
	};

	T* Target() {
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
	
	Reference& operator=(const Reference& ref) {
		SetTo(ref.Target());

		return *this;
	};
	
	Reference& operator=(T* other) {
		SetTo(other);
		return *this;
	}
	
	bool operator==(const T* other) {
		return fTarget == other;
	}
private:
	T* fTarget;
};


/*
Reference::Reference(const Reference& ref)
	:
	fTarget(NULL)
{
	fTarget = const_cast<Reference&>(ref).Target();
	fTarget->Acquire();
}
*/





#endif /* REFERENCE_H_ */
