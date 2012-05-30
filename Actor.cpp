#include "Actor.h"
#include "Animation.h"
#include "BamResource.h"
#include "BCSResource.h"
#include "Bitmap.h"
#include "Core.h"
#include "CreResource.h"
#include "GraphicsEngine.h"
#include "IDSResource.h"
#include "RectUtils.h"
#include "ResManager.h"
#include "Script.h"


#include <assert.h>
#include <string>


std::vector<Actor*> Actor::sActors;

Actor::Actor(::actor &actor)
	:
	fActor(&actor),
	fAnimation(NULL),
	fCRE(NULL),
	fBCSResource(NULL),
	fOwnsActor(false)
{
	_Init();
}


Actor::Actor(const char* creName, point position, int face)
	:
	fActor(NULL),
	fAnimation(NULL),
	fCRE(NULL),
	fBCSResource(NULL),
	fOwnsActor(true)
{
	fActor = new actor;
	fActor->cre = creName;
	strcpy(fActor->name, fActor->cre);
	//fActor->orientation = std::min(face, 7);
	fActor->orientation = 0;
	fActor->position = position;

	_Init();
}


void
Actor::_Init()
{
	fCRE = gResManager->GetCRE(fActor->cre);

	fActor->Print();

	printf("%d\n", fCRE->Colors().major);
	// TODO: Get all scripts ? or just the specific one ?

	if (fCRE == NULL)
		throw "error!!!";
	//fBCSResource = gResManager->GetBCS(fCRE->DefaultScriptName());
	//gResManager->GetBCS(fCRE->OverrideScriptName());
	//gResManager->GetBCS(fCRE->RaceScriptName());
	//printf("ScriptName: %s\n", (const char*)fCRE->GeneralScriptName());
	gResManager->GetBCS(fCRE->GeneralScriptName());
	//gResManager->GetBCS(fCRE->ClassScriptName());


	if (fBCSResource != NULL)
		fBCSResource->GetScript()->fActor = this;

	try {
		fAnimation = new Animation(this);
	} catch (...) {
		printf("Actor::Actor(): cannot instantiate Animation\n");
		//delete fAnimation;
		fAnimation = NULL;
	}

	//if (fAnimation != NULL)
		//fAnimation->fBAM->DumpFrames("/home/stefano/dumps");
}


Actor::~Actor()
{
	if (fOwnsActor)
		delete fActor;
	gResManager->ReleaseResource(fCRE);
	gResManager->ReleaseResource(fBCSResource);
	delete fAnimation;
}


/* static */
void
Actor::Add(Actor* actor)
{
	sActors.push_back(actor);
}


/* static */
void
Actor::Remove(const char* name)
{

}


/* static */
Actor*
Actor::GetByIndex(uint32 i)
{
	return sActors[i];
}


/* static */
Actor*
Actor::GetByName(const char* name)
{
	return NULL;
}


/* static */
std::vector<Actor*>&
Actor::List()
{
	return sActors;
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

	Bitmap *image = frame.bitmap;
	if (image == NULL)
		return;

	point center = offset_point(Position(), -frame.rect.w / 2,
						-frame.rect.h / 2);

	//printf("center: %d %d\n", center.x, center.y);
	SDL_Rect rect = { center.x, center.y, image->Width(), image->Height() };

	rect = offset_rect(rect, -frame.rect.x, -frame.rect.y);

	if (rects_intersect(area, rect)) {
		rect = offset_rect(rect, -area.x, -area.y);
		SDL_BlitSurface(image->Surface(), NULL, surface, &rect);
	}
	GraphicsEngine::DeleteBitmap(image);
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


void
Actor::SetDestination(const point& point)
{
	fActor->destination = point;
}


CREResource *
Actor::CRE()
{
	return fCRE;
}


::Script *
Actor::Script()
{
	assert (fBCSResource != NULL);
	return fBCSResource->GetScript();
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
	else if (!strcmp(stringRace, "ORC"))
		c = 'O';
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
	else if (!strcmp(stringClass, "INNOCENT"))
		c = 'B';

	return c;
}


void
Actor::UpdateMove()
{
	if (fActor->position.x != fActor->destination.x)
		printf("MOVING!!!!\n");

	if (fActor->position.x < fActor->destination.x)
		fActor->position.x++;
	else if (fActor->position.x > fActor->destination.x)
		fActor->position.x--;

	if (fActor->position.y < fActor->destination.y)
		fActor->position.y++;
	else if (fActor->position.y > fActor->destination.y)
		fActor->position.y--;
}


static
int
AnimationType(int value)
{
	int masked = (value & 0xFF00) >> 8;
	if ((masked >= 0x61 && masked <= 0x63)
		|| masked == 0x50)
		return 2;
	return 1;
}


/* static */
res_ref
Actor::AnimationFor(Actor &actor)
{
	CREResource *creature = actor.CRE();
	const uint16 animationID = creature->AnimationID();
	printf("%s\n", 	AnimateIDS()->ValueFor(animationID));
	res_ref nameRef;
	uint8 high = (animationID & 0xFF00) >> 8;
	uint8 low = (animationID & 0x00FF);
	// TODO: Seems like animation type could be told by
	// a mask here: monsters, characters, "objects", etc.
	if (AnimationType(animationID) == 1) {
		std::string baseName = "";
		switch (animationID) {
			case 0x7400:
				baseName = "MDOG";
				break;
			case 0x7a00:
				baseName = "MSPI";
				break;
			case 0x7e00:
				baseName = "MWER";
				break;
			case 0x7f01:
				baseName = "MMIN";
				break;
			case 0x7f02:
				baseName = "MBEH";
				break;
			case 0x7f03:
				baseName = "MIMP";
				break;
			case 0x7f09:
				baseName = "MSAH";
				break;
			case 0x7f0c:
				baseName = "MKUO";
				break;
			case 0x7f11:
				baseName = "MUMB";
				break;
			case 0x7f14:
				baseName = "MGIT";
				break;
			case 0x7f15:
				baseName = "MBES";
				break;
			case 0x7f16:
				baseName = "AMOO";
				break;
			case 0x7f17:
				baseName = "ARAB";
				break;
			case 0x7f18:
				baseName = "ADER";
				break;
			case 0x7f20:
				baseName = "AGRO";
				break;
			case 0x7f21:
				baseName = "APHE";
				break;
			case 0x7f27:
				baseName = "MDRO";
				break;
			case 0x7f28:
				baseName = "MKUL";
				break;
			/*case 0x8000:
				baseName = "";
				break;*/
			case 0xca00:
				baseName = "NNOML";
				break;
			case 0xca10:
				baseName = "NNOWL";
				break;
			case 0xb000:
				baseName = "ACOW";
				break;
			case 0xc100:
				baseName = "ACAT";
				break;
			case 0xc200:
				baseName = "ACHK";
				break;
			case 0xc300:
				baseName = "ARAT";
				break;
			case 0xc400:
				baseName = "ASQU";
				break;
			case 0xc500:
				baseName = "ABAT";
				break;
			case 0xc600:
				baseName = "NBEGH";
				break;
			case 0xc700:
				baseName = "NBOYL";
				break;
			case 0xc710:
				baseName = "NGRLL";
				break;
			case 0xc800:
				baseName = "NFAMH";
				break;
			case 0xc810:
				baseName = "NFAWH";
				break;
			/*case 0xc9:
			{
				if (low == 0x10)
					baseName = "NFAWH";
				else
					baseName = "NFAMH";
				break;
			}*/
			// Birds
			case 0xd000:
				baseName = "AEAG";
				break;
			case 0xd100:
				baseName = "AGUL";
				break;
			case 0xd200:
				baseName = "AVUL";
				break;
			case 0xd300:
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
		// TODO: Implement for real
		char name[8];
		// These fields are required
		name[0] = GeneralToLetter(creature->General());
		name[1] = RaceToLetter(creature->Race());
		name[2] = low == 0x10 ? 'F' : 'M';
		name[3] = ClassToLetter(creature->Class());
		if (name[3] != 'F' && name[3] != 'C')
			name[4] = '2';
		else
			name[4] = '4'; //ArmorToLetter();
		name[5] = 'A'; // Action
		if (name[5] == 'A' || name[5] == 'G')
			name[6] = '1'; // Detail

		strcpy(nameRef.name, name);
		printf("nameRef: %s\n", (const char *)nameRef);
	}


	//printf("nameRef: %s\n", (const char *)nameRef);
	return nameRef;
}
