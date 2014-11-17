/*
 * Reference.h
 *
 *  Created on: 17/nov/2014
 *      Author: stefano
 */

#ifndef REFERENCE_H_
#define REFERENCE_H_

class Referenceable;
class Reference {
public:
	Reference(Referenceable* target);
	Reference(const Reference& ref);
	~Reference();

	Referenceable* Target();

	Reference& operator=(const Reference& ref);
private:
	Referenceable* fTarget;
};

#endif /* REFERENCE_H_ */
