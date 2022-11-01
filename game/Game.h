/*
 * Game.h
 *
 *  Created on: 29/mar/2015
 *      Author: Stefano Ceccherini
 */

#ifndef GAME_H_
#define GAME_H_

#include "IETypes.h"

#define AI_UPDATE_FREQ 15
#define ROUND_DURATION_SEC 6

class Actor;
class DialogHandler;
class Party;
class Game {
public:
	static Game* Get();
	void Loop(bool noNewGame = false, bool executeScripts = true);
	void CreateParty();
	void InitiateDialog(Actor* actor, Actor* target);
	bool InDialogMode() const;
	void TerminateDialog();
	DialogHandler* Dialog();

	::Party* Party();

	void LoadStartingArea();
	void ToggleDayNight();

	bool Load(const char* name);
	bool Save(const char* name);

	// TODO: we need this to keep track of actor moving between areas
	// until we have proper support for saving areas data
	class TempState {
	public:
		std::vector<Actor*> actors;
	};
	TempState* GetTempState();

	void SetTestMode(bool value);
	bool TestMode() const;


private:
	Game();
	~Game();

	DialogHandler* fDialog;

	::Party* fParty;
	TempState* fTempState;

	uint32 fDelay;
	bool fTestMode;
};

#endif /* GAME_H_ */
