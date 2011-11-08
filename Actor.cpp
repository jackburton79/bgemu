#include "Actor.h"
#include "CreResource.h"
#include "IDSResource.h"
#include "ResManager.h"
#include "World.h"

Actor::Actor(::actor &actor)
	:
	fActor(&actor)
{
	fCRE = gResManager->GetCRE(fActor->cre);
}


Actor::~Actor()
{
	gResManager->ReleaseResource(fCRE);
}


point
Actor::Position() const
{
	return fActor->position;
}


const char *
Actor::Name() const
{
	return (const char *)fActor->name;
}


CREResource *
Actor::CRE()
{
	return fCRE;
}


static char
GeneralToLetter(uint8 general)
{
	const char *string = GeneralIDS()->ValueFor(general);
	char c = 'C';
	printf("General type %d: *%s*\n", general, string);
	if (!strcmp(string, "CHARACTER"))
		c = 'C';
	/*else if (!strcmp(string, "MONSTER"))
		c = 'M';*/

	return c;
}


static char
RaceToLetter(uint8 race)
{
	const char *stringRace = RacesIDS()->ValueFor(race);
	printf("stringRace %d: *%s*\n", race, stringRace);
	char c = 'H';
	if (!strcmp(stringRace, "HUMAN"))
		c = 'H';
	else if (!strcmp(stringRace, "ELF")
		|| !strcmp(stringRace, "HALF_ELF"))
		c = 'E';
	else if (!strcmp(stringRace, "DWARF")
			|| !strcmp(stringRace, "GNOME"))
		c = 'D';
	else if (!strcmp(stringRace, "HALFLING"))
		c = 'I';
	/*else if (!strcmp(stringRace, "LYCANTHROPE"))
		c = 'L';
	else if (!strcmp(stringRace, "BASILISK"))
		c = 'B';
	else if (!strcmp(stringRace, "ELEMENTAL"))
		c = 'E';
*/
	return c;
}


static char
GenderToLetter(uint8 gender)
{
	const char *stringGender = GendersIDS()->ValueFor(gender);
	char c = 'M';
	printf("stringGender: *%s*\n", stringGender);
	if (!strcmp(stringGender, "MALE"))
		c = 'M';
	if (!strcmp(stringGender, "FEMALE"))
		c = 'F';
	/*else if (!strcmp(stringGender, "ILLUSIONARY"))
		c = 'I';*/

	return c;
}


static char
ClassToLetter(uint8 cclass)
{
	const char *stringClass = ClassesIDS()->ValueFor(cclass);
	printf("stringClass: *%s*\n", stringClass);
	char c = 'F';
	if (!strncmp(stringClass, "FIGHTER", strlen("FIGHTER"))
		|| !strncmp(stringClass, "RANGER", strlen("RANGER"))
		|| !strncmp(stringClass, "PALADIN", strlen("PALADIN")))
		c = 'F';
	else if (!strncmp(stringClass, "THIEF", strlen("THIEF"))
		|| !strncmp(stringClass, "BARD", strlen("BARD")))
		c = 'T';
	else if (!strncmp(stringClass, "CLERIC", strlen("CLERIC")))
		c = 'C';
	else if (!strncmp(stringClass, "MAGE", strlen("MAGE")))
		c = 'W';
	else if (!strcmp(stringClass, "MONK"))
		c = 'M';
	/*else if (!strcmp(stringClass, "INNOCENT"))
		c = 'B';
	else if (!strcmp(stringClass, "WEREWOLF"))
		c = 'Z';
	else if (!strncmp(stringClass, "BASILISK_GREATER",
			strlen("BASILISK_GREATER")))
		c = 'G';
	else if (!strncmp(stringClass, "ELEMENTAL", strlen("ELEMENTAL")))
		c = 'T';
*/
	return c;
}


/* static */
res_ref
Actor::AnimationFor(Actor &actor)
{
	CREResource *creature = actor.CRE();
	char name[8];
	// These fields are required
	name[0] = GeneralToLetter(creature->General());
	name[1] = RaceToLetter(creature->Race());
	name[2] = GenderToLetter(creature->Gender());
	name[3] = ClassToLetter(creature->Class());

	// these could be optional
	char *namePtr = &name[4];
	if (name[3] == 'F' || name[3] == 'C')
		*namePtr++ = '4';
	else if (name[0] != 'M')
		*namePtr++ = '2';

	*namePtr++ = 'G'; // action
	*namePtr++ = '1'; // weapon or nothing
	if (namePtr != &name[7])
		*namePtr = '\0';

	res_ref nameRef;
	memcpy(nameRef.name, name, sizeof(name));

	return nameRef;
}
