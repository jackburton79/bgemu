/*
 * TextEdit.h
 *
 *  Created on: 11/nov/2012
 *      Author: stefano
 */

#ifndef TEXTEDIT_H_
#define TEXTEDIT_H_

#include "Control.h"

class TextEdit: public Control {
public:
	TextEdit(IE::text_edit* textEdit);
	virtual ~TextEdit();
};

#endif /* TEXTEDIT_H_ */
