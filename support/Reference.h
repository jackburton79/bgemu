/*
 * Reference.h
 *
 *  Created on: 17/nov/2014
 *      Author: stefano
 */

#ifndef REFERENCE_H_
#define REFERENCE_H_

class Referenceable;
template <typename T>
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
		if (fTarget->Release())
			delete fTarget;
	};

	T* Target() {
		return fTarget;
	};

	Reference& operator=(const Reference& ref) {
		fTarget = const_cast<Reference&>(ref).Target();
		fTarget->Acquire();

		return *this;
	};
	
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
