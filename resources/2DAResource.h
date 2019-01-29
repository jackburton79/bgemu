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

	std::string ValueFor(const char* row, const char* column = NULL) const;
	int32 IntegerValueFor(const char* row, const char* column = NULL) const;
	std::string ValueAt(int rowIndex, int columnIndex) const;
	int32 IntegerValueAt(int rowIndex, int columnIndex) const;

	int32 CountRows() const;
	int32 CountColumns() const;

private:
	virtual ~TWODAResource();

	void _HandleHeadersRow(char* string);
	void _HandleContentRow(char* string);

	typedef std::vector<std::string> StringList;
	typedef std::map<std::pair<std::string, std::string>, std::string> StringMap;

	StringMap fMap;
	std::string fDefaultValue;

	StringList fColumnHeaders;
	StringList fRowHeaders;

	std::vector<StringList> fTable;
};

#endif /* TWODARESOURCE_H_ */
