#include "Actor.h"
#include "Animation.h"
#include "BamResource.h"
#include "CreResource.h"
#include "Graphics.h"
#include "IDSResource.h"
#include "RectUtils.h"
#include "ResManager.h"
#include "World.h"

#include <string>

Actor::Actor(::actor &actor)
	:
	fActor(&actor),
	fAnimation(NULL),
	fOwnsActor(false)
{
	fCRE = gResManager->GetCRE(fActor->cre);
	try {
		fAnimation = new Animation(this);
	} catch (...) {
		delete fAnimation;
		fAnimation = NULL;
	}
}


Actor::Actor(const char* creName, point position, int face)
	:
	fActor(NULL),
	fAnimation(NULL),
	fOwnsActor(true)
{
	fActor = new actor;
	fActor->cre = creName;
	fActor->orientation = face;
	fActor->position = position;

	fCRE = gResManager->GetCRE(creName);

	strcpy(fActor->name, fCRE->KitStr());

	try {
		fAnimation = new Animation(this);
	} catch (...) {
		delete fAnimation;
		fAnimation = NULL;
	}
}


Actor::~Actor()
{
	if (fOwnsActor)
		delete fActor;
	gResManager->ReleaseResource(fCRE);
	delete fAnimation;
}


const char *
Actor::Name() const
{
	return (const char *)fActor->name;
}


void
Actor::Draw(SDL_Surface *surface, SDL_Rect area)
{
	if (fAnimation == NULL)
		return;

	Frame frame = fAnimation->NextFrame();

	SDL_Surface *image = frame.surface;
	if (image == NULL)
		return;

	point center = offset_point(Position(), -frame.rect.w / 2,
						-frame.rect.h / 2);

	SDL_Rect rect = { center.x, center.y, image->w, image->h };

	rect = offset_rect(rect, -frame.rect.x, -frame.rect.y);

	if (rects_intersect(area, rect)) {
		rect = offset_rect(rect, -area.x, -area.y);
		SDL_BlitSurface(image, NULL, surface, &rect);
	}
	SDL_FreeSurface(image);
}


point
Actor::Position() const
{
	return fActor->position;
}


orientation
Actor::Orientation() const
{
	return (orientation)fActor->orientation;
}


point
Actor::Destination() const
{
	return fActor->destination;
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
	printf("General %d: *%s*\n", general, string);
	if (!strcmp(string, "CHARACTER"))
		c = 'C';
	/*else if (!strcmp(string, "MONSTER"))
		c = 'M';*/
	//else
		//throw -1;
	return c;
}


static char
RaceToLetter(uint8 race)
{
	const char *stringRace = RacesIDS()->ValueFor(race);
	printf("Race %d: *%s*\n", race, stringRace);
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
ClassToLetter(uint8 cclass)
{
	const char *stringClass = ClassesIDS()->ValueFor(cclass);
	printf("Class: *%s*\n", stringClass);
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


static
int
AnimationType(int value)
{
	//if (value > 0x70 && value < 0xd5)
		return 1;

	//return 2;
}



static
const char *
WeirdMonsterCode(uint8 code)
{
	switch (code) {
		case 0x01:
			return "MMIN";
		case 0x02:
			return "MBEH";
		case 0x03:
			return "MIMP";
		case 0x09:
			return "MSAH";
		case 0x0c:
			return "MKUO";
		case 0x11:
			return "MUMB";
		case 0x14:
			return "MGIT";
		case 0x15:
			return "MBES";
		case 0x16:
			return "AMOO";
		case 0x17:
			return "ARAB";
		case 0x18:
			return "ADER";
		case 0x20:
			return "AGRO";
		case 0x21:
			return "APHE";
		case 0x27:
			return "MDRO";
		case 0x28:
			return "MKUL";
		//case 0x2c:
		//	return ""
		default:
			printf("unknown code 0x%x\n", code);
			return "";
	}
}


/* static */
res_ref
Actor::AnimationFor(Actor &actor)
{
	CREResource *creature = actor.CRE();
	const uint16 animationID = creature->AnimationID();

	res_ref nameRef;
	uint8 high = (animationID & 0xFF00) >> 8;
	uint8 low = (animationID & 0x00FF);
	// TODO: Seems like animation type could be told by
	// a mask here: monsters, characters, "objects", etc.
	if (AnimationType(high) == 1) {
		std::string baseName = "";
		switch (high) {
			case 0x74:
				baseName = "MDOG";
				break;
			case 0x7a:
				baseName = "MSPI";
				break;
			case 0x7f:
				baseName = WeirdMonsterCode(low);
				break;
			/*case 0x80:
				baseName = "";
				break;*/
			case 0xca:
			{
				if (low == 0x10)
					baseName = "NNOWL";
				else
					baseName = "NNOML";
				break;
			}
			case 0xb0:
				baseName = "ACOW";
				break;
			case 0xc1:
				baseName = "ACAT";
				break;
			case 0xc2:
				baseName = "ACHK";
				break;
			case 0xc3:
				baseName = "ARAT";
				break;
			case 0xc4:
				baseName = "ASQU";
				break;
			case 0xc5:
				baseName = "ABAT";
				break;
			case 0xc6:
				baseName = "NBEGH";
				break;
			case 0xc7:
			{
				if (low == 0x10)
					baseName = "NGRLL";
				else
					baseName = "NBOYL";
				break;
			}
			case 0xc8:
			{
				if (low == 0x10)
					baseName = "NFAWH";
				else
					baseName = "NFAMH";
				break;
			}
			/*case 0xc9:
			{
				if (low == 0x10)
					baseName = "NFAWH";
				else
					baseName = "NFAMH";
				break;
			}*/
			// Birds
			case 0xd0:
				baseName = "AEAG";
				break;
			case 0xd1:
				baseName = "AGUL";
				break;
			case 0xd2:
				baseName = "AVUL";
				break;
			case 0xd3:
				baseName = "ABIR";
				break;
			default:
				break;
		}

		if (baseName.empty()) {
			printf("unknown code 0x%x (%s)\n", animationID,
						AnimateIDS()->ValueFor(animationID));
			throw -1;
		}

		baseName.append("G1");
		if (baseName[0] == 'N') {
			if (actor.Orientation() >= ORIENTATION_NE
				&& actor.Orientation() <= ORIENTATION_SE)
			baseName.append("E");
		}
		strcpy(nameRef.name, baseName.c_str());
	} else {
		char name[8];
		// These fields are required
		name[0] = GeneralToLetter(creature->General());
		name[1] = RaceToLetter(creature->Race());
		name[2] = low == 0x10 ? 'F' : 'M';
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

		memcpy(nameRef.name, name, sizeof(name));
	}


	//printf("nameRef: %s\n", (const char *)nameRef);
	return nameRef;
}
