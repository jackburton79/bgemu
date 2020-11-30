/*
 * Game.h
 *
 *  Created on: 29/mar/2015
 *      Author: stefano
 */

#ifndef GAME_H_
#define GAME_H_


#define AI_UPDATE_FREQ 15

class Actor;
class DialogState;
class Party;
class Game {
public:
	static Game* Get();
	void Loop(bool noNewGame = false, bool executeScripts = true);

	void InitiateDialog(Actor* actor, Actor* target);
	bool InDialogMode() const;
	void TerminateDialog();
	DialogState* Dialog();

	::Party* Party();

	void LoadStartingArea();

	bool Load(const char* name);
	bool Save(const char* name);

	void SetTestMode(bool value);
	bool TestMode() const;
private:
	Game();
	~Game();

	DialogState* fDialog;

	::Party* fParty;
	bool fTestMode;
};

#endif /* GAME_H_ */
