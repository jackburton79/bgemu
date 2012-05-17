#ifndef __SCRIPT_H
#define __SCRIPT_H

#include <list>


struct script {
	std::list<trigger*> conditions;
	std::list<response*> responses;
};

#endif
