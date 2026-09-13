/*
 * TextEdit.h
 *
 *  Created on: 11/nov/2012
 *      Author: stefano
 */

#pragma once

#include "Control.h"

class TextEdit: public Control {
public:
	TextEdit(IE::text_edit* textEdit);
	virtual ~TextEdit();
};
