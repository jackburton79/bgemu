/*
 * TWODAResource.h
 *
 *  Created on: 01/ott/2012
 *      Author: stefano
 */

#ifndef TWODARESOURCE_H_
#define TWODARESOURCE_H_

#include "Resource.h"

#include <map>
#include <string>

class TWODAResource: public Resource {
public:
	TWODAResource(const res_ref& name);

	virtual bool Load(Archive *archive, uint32 key);
	virtual void Dump();

	std::string ValueFor(const char* row, const char* column = NULL);
	uint32 IntegerValueFor(const char* row, const char* column = NULL);
private:
	virtual ~TWODAResource();
	std::map<std::pair<std::string, std::string>, std::string> fMap;
};

#endif /* TWODARESOURCE_H_ */
