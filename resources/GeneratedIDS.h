/*
 * GeneratedIDS.h
 *
 *  Created on: 11/mag/2013
 *      Author: stefano
 */

#ifndef __GENERATEDIDS_H_
#define __GENERATEDIDS_H_

#include "IETypes.h"

class IDSResource;
class WriteIDSResource;
class GeneratedIDS {
public:
	static IDSResource* CreateIDSResource(const res_ref& name);
private:
	static void FillAniSnd(WriteIDSResource* res);
};

#endif /* GENERATEDIDS_H_ */
