/*
 * Game.h
 *
 *  Created on: 29/mar/2015
 *      Author: stefano
 */

#ifndef GAME_H_
#define GAME_H_

class Actor;
class Party;
class Game {
public:
	static Game* Get();
	void Loop(bool executeScripts);
	::Party* Party();

	bool Load(const char* name);
	bool Save(const char* name);

private:
	Game();
	~Game();

	::Party* fParty;
};

#endif /* GAME_H_ */
