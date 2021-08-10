/*
 * Game.h
 *
 *  Created on: 29/mar/2015
 *      Author: stefano
 */

#ifndef GAME_H_
#define GAME_H_

#include "IETypes.h"

#define AI_UPDATE_FREQ 15

class Actor;
class DialogHandler;
class Party;
class Game {
public:
	static Game* Get();
	void Loop(bool noNewGame = false, bool executeScripts = true);

	void InitiateDialog(Actor* actor, Actor* target);
	bool InDialogMode() const;
	void TerminateDialog();
	DialogHandler* Dialog();

	void HandleDialog();

	::Party* Party();

	void LoadStartingArea();
	void ToggleDayNight();

	bool Load(const char* name);
	bool Save(const char* name);

	void SetTestMode(bool value);
	bool TestMode() const;

private:
	Game();
	~Game();

	DialogHandler* fDialog;

	::Party* fParty;
	bool fTestMode;
};

#endif /* GAME_H_ */
