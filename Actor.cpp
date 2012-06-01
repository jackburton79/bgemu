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

Actor::Actor(IE::actor &actor)
	:
	fActor(&actor),
	fAnimation(NULL),
	fCRE(NULL),
	fBCSResource(NULL),
	fOwnsActor(false)
{
	_Init();
}


Actor::Actor(const char* creName, IE::point position, int face)
	:
	fActor(NULL),
	fAnimation(NULL),
	fCRE(NULL),
	fBCSResource(NULL),
	fOwnsActor(true)
{
	fActor = new IE::actor;
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
Actor::Draw(Bitmap *surface, GFX::rect area)
{
	if (fAnimation == NULL)
		return;

	Frame frame = fAnimation->NextFrame();

	Bitmap *image = frame.bitmap;
	if (image == NULL)
		return;

	IE::point center = offset_point(Position(), -frame.rect.w / 2,
						-frame.rect.h / 2);


	GFX::rect rect = { center.x, center.y, image->Width(), image->Height() };

	rect = offset_rect(rect, -frame.rect.x, -frame.rect.y);

	if (rects_intersect(area, rect)) {
		rect = offset_rect(rect, -area.x, -area.y);
		GraphicsEngine::Get()->BlitToScreen(image, NULL, &rect);
	}
	GraphicsEngine::DeleteBitmap(image);
}


IE::point
Actor::Position() const
{
	return fActor->position;
}


IE::orientation
Actor::Orientation() const
{
	return (IE::orientation)fActor->orientation;
}


IE::point
Actor::Destination() const
{
	return fActor->destination;
}


void
Actor::SetDestination(const IE::point& point)
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


static
const char*
IDToBaseName(const uint16 &id)
{
	switch (id) {
		case 0x2000: return "MSIR";
		case 0x7001: return "MOGR";
		case 0x7400: return "MDOG";
		case 0x7a00: return "MSPI";
		case 0x7e00: return "MWER";
		case 0x7f01: return "MMIN";
		case 0x7f02: return "MBEH";
		case 0x7f03: return "MIMP";
		case 0x7f09: return "MSAH";
		case 0x7f0c: return "MKUO";
		case 0x7f11: return "MUMB";
		case 0x7f14: return "MGIT";
		case 0x7f15: return "MBES";
		case 0x7f16: return "AMOO";
		case 0x7f17: return "ARAB";
		case 0x7f18: return "ADER";
		case 0x7f20: return "AGRO";
		case 0x7f21: return "APHE";
		case 0x7f27: return "MDRO";
		case 0x7f28: return "MKUL";
		case 0x8000: return "MGNL";
		case 0x8100: return "MHOB";
		case 0xb000: return "ACOW";
		case 0xca00: return "NNOML";
		case 0xca10: return "NNOWL";
		case 0xc100: return "ACAT";
		case 0xc200: return "ACHK";
		case 0xc300: return "ARAT";
		case 0xc400: return "ASQU";
		case 0xc500: return "ABAT";
		case 0xc600: return "NBEGH";
		case 0xc700: return "NBOYL";
		case 0xc710: return "NGRLL";
		case 0xc800: return "NFAMH";
		case 0xc810: return "NFAWH";
		/*case 0xc9:
		{
			if (low == 0x10)
				baseName = "NFAWH";
			else
				baseName = "NFAMH";
			break;
		}*/

		// Birds
		case 0xd000: return "AEAG";
		case 0xd100: return "AGUL";
		case 0xd200: return "AVUL";
		case 0xd300: return "ABIR";
		default: return "";
	}
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
		std::string baseName = IDToBaseName(animationID);

		if (baseName.empty()) {
			printf("unknown code 0x%x (%s)\n", animationID,
						AnimateIDS()->ValueFor(animationID));
			throw -1;
		}

		baseName.append("G1");
		if (baseName[0] == 'N') {
			if (actor.Orientation() >= IE::ORIENTATION_NE
				&& actor.Orientation() <= IE::ORIENTATION_SE)
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
