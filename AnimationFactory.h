/*
 * AnimationFactory.h
 *
 *  Created on: 20/mag/2013
 *      Author: stefano
 */

#ifndef __ANIMATIONFACTORY_H_
#define __ANIMATIONFACTORY_H_

#include <string>
#include <vector>

class Animation;
class AnimationFactory {
public:
	AnimationFactory(const char* baseName);
	~AnimationFactory();

	Animation* AnimationFor(int action, IE::orientation o);

private:
	bool _HasEastBams() const;
	std::vector<std::string> fList;
};

#endif /* ANIMATIONFACTORY_H_ */
