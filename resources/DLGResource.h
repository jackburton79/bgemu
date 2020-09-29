/*
 * DLGResource.h
 *
 *  Created on: 04/ott/2013
 *      Author: stefano
 */

#ifndef DLGRESOURCE_H_
#define DLGRESOURCE_H_

#include "Resource.h"

struct dlg_state {
	int32 text_ref;
	int32 transition_first;
	int32 transitions_num;
	int32 trigger;
};

class DLGResource : public Resource {
public:
	DLGResource(const res_ref& name);

	virtual bool Load(Archive* archive, uint32 key);

	void StartDialog();

	static Resource* Create(const res_ref& name);
private:
	virtual ~DLGResource();

	struct dlg_state _GetStateAt(int index);
	std::string _GetStateTrigger(int triggerIndex);

	uint32 fNumStates;
	uint32 fStateTableOffset;
	uint32 fNumTransitions;
	uint32 fTransitionsTableOffset;
	uint32 fStateTriggersTableOffset;
	uint32 fStateTriggersNum;
	uint32 fTransitionTriggersTableOffset;
	uint32 fTransitionTriggersNum;
	uint32 fActionsTableOffset;
	uint32 fActionsNum;
};

#endif /* DLGRESOURCE_H_ */
