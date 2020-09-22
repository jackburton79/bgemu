/*
 * Dialog.h
 *
 *  Created on: 22 set 2020
 *      Author: Stefano Ceccherini
 */

#ifndef DIALOG_H_d
#define DIALOG_H_

#include "IETypes.h"

class DLGResource;
class Dialog {
public:
	Dialog(const res_ref& name);
	virtual ~Dialog();

	void Start();
private:
	DLGResource* fDLG;
};

#endif /* DIALOG_H_ */
