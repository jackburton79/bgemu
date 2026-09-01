#include "Actions.h"

#include "Action.h"
#include "Actor.h"
#include "Animation.h"
#include "Core.h"
#include "Game.h"
#include "IDSResource.h"
#include "Object.h"
#include "ResManager.h"
#include "Script.h"
#include "SpellEffect.h"
#include "SPLResource.h"
#include "Timer.h"
#include "Variables.h"

#include <iostream>
#include <unordered_map>


// ---- Native action implementations (proof of concept: one stateless, one
// with simple state, one with more involved state + resource access) ----

// SETGLOBAL(S:NAME*,S:AREA*,I:VALUE*) - stateless, completes immediately.
static void
RunActionSetGlobal(Object* sender, action_params* params, action_state& state)
{
	std::string variableScope;
	std::string variableName;
	Variables::GetNameAndScope(params->string1, variableScope, variableName);
	if (variableScope.compare("LOCALS") == 0) {
		if (sender != NULL)
			sender->SetVariable(variableName.c_str(), params->integer1);
	} else {
		// TODO: Check for AREA variables
		Core::Get()->Vars().Set(params->string1, params->integer1);
	}
	state.completed = true;
}


// WAIT(I:TIME*) - simple state: a single tick countdown, reusing
// action_state::counter.
static void
RunActionWait(Object* sender, action_params* params, action_state& state)
{
	if (!state.initiated) {
		state.counter = params->integer1 * AI_UPDATE_FREQ;
		state.initiated = true;
	}
	if (--state.counter <= 0)
		state.completed = true;
}


// FORCESPELL(O:TARGET,I:SPELL*SPELL) - more involved state: resource
// lookups done once (on first tick), a tick countdown derived from the
// spell's casting time, and a start timestamp kept only for the diagnostic
// print at the end (mirrors the original ActionForceSpell::operator()()).
static void
RunActionForceSpell(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(sender);
	if (actor == NULL) {
		std::cerr << "ForceSpell: NO sender Actor" << std::endl;
		state.completed = true;
		return;
	}

	if (!state.initiated) {
		IDSResource* spellIDS = gResManager->GetIDS("SPELL");
		std::string spellName = spellIDS->StringForID(params->integer1).c_str();
		std::string spellResourceName = SPLResource::GetSpellResourceName(params->integer1);
		gResManager->ReleaseResource(spellIDS);
		std::cout << "spell: " << spellName << std::endl;

		SPLResource* spellResource = gResManager->GetSPL(spellResourceName.c_str());
		uint16 castTime = spellResource->CastingTime();
		// TODO: Not sure if it's correct. CastingTime is 1/10 of round.
		// Round takes ROUND_DURATION_SEC seconds; AI updates AI_UPDATE_FREQ
		// times per second.
		state.counter = castTime * AI_UPDATE_FREQ * ROUND_DURATION_SEC / 10;
		std::cout << "casting time:" << state.counter << std::endl;
		gResManager->ReleaseResource(spellResource);

		actor->SetAnimationAction(ACT_CAST_SPELL_PREPARE);
		state.startTick = Timer::Ticks();
		state.initiated = true;
	}

	if (state.counter-- == 0) {
		IDSResource* spellIDS = gResManager->GetIDS("SPELL");
		std::string spellName = spellIDS->StringForID(params->integer1).c_str();
		gResManager->ReleaseResource(spellIDS);
		std::cout << "Spell " << spellName << " finished" << std::endl;

		actor->SetAnimationAction(ACT_CAST_SPELL_RELEASE);
		Object* target = Script::GetTargetObject(sender, params);
		if (target == NULL)
			target = sender;
		if (target != NULL) {
			std::cout << "target: " << target->Name() << std::endl;
			std::cout << "spell name: " << spellName << std::endl;
			SpellEffect* dummy = new SpellEffect(spellName);
			target->AddSpellEffect(dummy);
		}
		state.completed = true;
		std::cout << "duration:" << (Timer::Ticks() - state.startTick) << std::endl;
	}
}


// ---- Transitional adapter for every action id not yet migrated ----

void
RunLegacyAction(Object* sender, action_params* params, action_state& state)
{
	if (state.legacy == NULL) {
		// id==1 (ACTIONOVERRIDE), id==36 (CONTINUE) and id==127 (CUTSCENEID)
		// are script-flow markers intercepted in Script::_HandleAction()
		// before an action is ever queued, so isContinue is always false by
		// the time GetAction() is reached from here.
		bool isContinue = false;
		state.legacy = Script::GetAction(sender, params, isContinue);
		if (state.legacy == NULL) {
			// Not implemented by the legacy switch either: nothing to run,
			// mark it done so the queue moves on instead of stalling.
			state.completed = true;
			return;
		}
	}

	(*state.legacy)();

	if (state.legacy->Completed()) {
		delete state.legacy;
		state.legacy = NULL;
		state.completed = true;
	}
}


static const ActionDescriptor kActionsTable[] = {
		{ 0, "NOACTION", NULL },
		{ 1, "ACTIONOVERRIDE", NULL },
		{ 2, "ADDWAYPOINT", NULL },
		{ 3, "ATTACK", NULL },
		{ 5, "BACKSTAB", NULL },
		{ 7, "CREATECREATURE", NULL },
		{ 8, "DIALOG", NULL },
		{ 9, "DROPITEM", NULL },
		{ 10, "ENEMY", NULL },
		{ 11, "EQUIPITEM", NULL },
		{ 13, "FINDTRAPS", NULL },
		{ 14, "GETITEM", NULL },
		{ 15, "GIVEITEM", NULL },
		{ 16, "GIVEORDER", NULL },
		{ 17, "HELP", NULL },
		{ 18, "HIDE", NULL },
		{ 19, "JOINPARTY", NULL },
		{ 20, "LAYHANDS", NULL },
		{ 21, "LEAVEPARTY", NULL },
		{ 22, "MOVETOOBJECT", NULL },
		{ 23, "MOVETOPOINT", NULL },
		{ 24, "PANIC", NULL },
		{ 25, "PICKPOCKETS", NULL },
		{ 26, "PLAYSOUND", NULL },
		{ 27, "PROTECTPOINT", NULL },
		{ 28, "REMOVETRAPS", NULL },
		{ 29, "RUNAWAYFROM", NULL },
		{ 30, "SETGLOBAL", RunActionSetGlobal },
		{ 31, "SPELL", NULL },
		{ 33, "TURN", NULL },
		{ 34, "USEITEMSLOT", NULL },
		{ 36, "CONTINUE", NULL },
		{ 37, "FOLLOWPATH", NULL },
		{ 38, "SWING", NULL },
		{ 39, "RECOIL", NULL },
		{ 40, "PLAYDEAD", NULL },
		{ 47, "FORMATION", NULL },
		{ 48, "JUMPTOPOINT", NULL },
		{ 49, "MOVEVIEWPOINT", NULL },
		{ 50, "MOVEVIEWOBJECT", NULL },
		{ 51, "CLICKLBUTTONPOINT", NULL },
		{ 52, "CLICKLBUTTONOBJECT", NULL },
		{ 53, "CLICKRBUTTONPOINT", NULL },
		{ 54, "CLICKRBUTTONOBJECT", NULL },
		{ 55, "DOUBLECLICKLBUTTONPOINT", NULL },
		{ 56, "DOUBLECLICKLBUTTONOBJECT", NULL },
		{ 57, "DOUBLECLICKRBUTTONPOINT", NULL },
		{ 58, "DOUBLECLICKRBUTTONOBJECT", NULL },
		{ 59, "MOVECURSORPOINT", NULL },
		{ 60, "CHANGEAISCRIPT", NULL },
		{ 61, "STARTTIMER", NULL },
		{ 62, "SENDTRIGGER", NULL },
		{ 63, "WAIT", RunActionWait },
		{ 64, "UNDOEXPLORE", NULL },
		{ 65, "EXPLORE", NULL },
		{ 66, "DAYNIGHT", NULL },
		{ 67, "WEATHER", NULL },
		{ 68, "CALLLIGHTNING", NULL },
		{ 69, "VEQUIP", NULL },
		{ 70, "NIDSPECIAL1", NULL },
		{ 71, "NIDSPECIAL2", NULL },
		{ 72, "NIDSPECIAL3", NULL },
		{ 73, "NIDSPECIAL4", NULL },
		{ 74, "NIDSPECIAL5", NULL },
		{ 75, "NIDSPECIAL6", NULL },
		{ 76, "NIDSPECIAL7", NULL },
		{ 77, "NIDSPECIAL8", NULL },
		{ 78, "NIDSPECIAL9", NULL },
		{ 79, "NIDSPECIAL10", NULL },
		{ 80, "NIDSPECIAL11", NULL },
		{ 81, "NIDSPECIAL12", NULL },
		{ 82, "CREATEITEM", NULL },
		{ 83, "SMALLWAIT", NULL },
		{ 84, "FACE", NULL },
		{ 85, "RANDOMWALK", NULL },
		{ 86, "SETINTERRUPT", NULL },
		{ 87, "PROTECTOBJECT", NULL },
		{ 88, "LEADER", NULL },
		{ 89, "FOLLOW", NULL },
		{ 90, "MOVETOPOINTNORECTICLE", NULL },
		{ 91, "LEAVEAREA", NULL },
		{ 92, "SELECTWEAPONABILITY", NULL },
		{ 94, "GROUPATTACK", NULL },
		{ 95, "SPELLPOINT", NULL },
		{ 96, "REST", NULL },
		{ 97, "USEITEMPOINTSLOT", NULL },
		{ 98, "ATTACKNOSOUND", NULL },
		{ 100, "RANDOMFLY", NULL },
		{ 102, "MORALESET", NULL },
		{ 103, "MORALEINC", NULL },
		{ 104, "MORALEDEC", NULL },
		{ 105, "ATTACKONEROUND", NULL },
		{ 106, "SHOUT", NULL },
		{ 107, "MOVETOOFFSET", NULL },
		{ 108, "ESCAPEAREA", NULL },
		{ 108, "ESCAPEAREAMOVE", NULL },
		{ 109, "INCREMENTGLOBAL", NULL },
		{ 110, "LEAVEAREALUA", NULL },
		{ 111, "DESTROYSELF", NULL },
		{ 112, "USECONTAINER", NULL },
		{ 113, "FORCESPELL", RunActionForceSpell },
		{ 114, "FORCESPELLPOINT", NULL },
		{ 115, "SETGLOBALTIMER", NULL },
		{ 116, "TAKEPARTYITEM", NULL },
		{ 117, "TAKEPARTYGOLD", NULL },
		{ 118, "GIVEPARTYGOLD", NULL },
		{ 119, "DROPINVENTORY", NULL },
		{ 120, "STARTCUTSCENE", NULL },
		{ 121, "STARTCUTSCENEMODE", NULL },
		{ 122, "ENDCUTSCENEMODE", NULL },
		{ 123, "CLEARALLACTIONS", NULL },
		{ 124, "BERSERK", NULL },
		{ 125, "DEACTIVATE", NULL },
		{ 126, "ACTIVATE", NULL },
		{ 127, "CUTSCENEID", NULL },
		{ 128, "ANKHEGEMERGE", NULL },
		{ 129, "ANKHEGHIDE", NULL },
		{ 130, "RANDOMTURN", NULL },
		{ 131, "KILL", NULL },
		{ 132, "VERBALCONSTANT", NULL },
		{ 133, "CLEARACTIONS", NULL },
		{ 134, "ATTACKREEVALUATE", NULL },
		{ 135, "LOCKSCROLL", NULL },
		{ 136, "UNLOCKSCROLL", NULL },
		{ 137, "STARTDIALOGUE", NULL },
		{ 138, "SETDIALOGUE", NULL },
		{ 139, "PLAYERDIALOGUE", NULL },
		{ 140, "GIVEITEMCREATE", NULL },
		{ 141, "GIVEPARTYGOLDGLOBAL", NULL },
		{ 142, "USEDOOR", NULL },
		{ 143, "OPENDOOR", NULL },
		{ 144, "CLOSEDOOR", NULL },
		{ 145, "PICKLOCK", NULL },
		{ 146, "POLYMORPH", NULL },
		{ 147, "REMOVESPELL", NULL },
		{ 148, "BASHDOOR", NULL },
		{ 149, "EQUIPMOSTDAMAGINGMELEE", NULL },
		{ 150, "STARTSTORE", NULL },
		{ 151, "DISPLAYSTRING", NULL },
		{ 152, "CHANGEAITYPE", NULL },
		{ 153, "CHANGEENEMYALLY", NULL },
		{ 154, "CHANGEGENERAL", NULL },
		{ 155, "CHANGERACE", NULL },
		{ 156, "CHANGECLASS", NULL },
		{ 157, "CHANGESPECIFICS", NULL },
		{ 158, "CHANGEGENDER", NULL },
		{ 159, "CHANGEALIGNMENT", NULL },
		{ 160, "APPLYSPELL", NULL },
		{ 161, "INCREMENTCHAPTER", NULL },
		{ 162, "REPUTATIONSET", NULL },
		{ 163, "REPUTATIONINC", NULL },
		{ 164, "ADDEXPERIENCEPARTY", NULL },
		{ 165, "ADDEXPERIENCEPARTYGLOBAL", NULL },
		{ 166, "SETNUMTIMESTALKEDTO", NULL },
		{ 167, "STARTMOVIE", NULL },
		{ 168, "INTERACT", NULL },
		{ 169, "DESTROYITEM", NULL },
		{ 170, "REVEALAREAONMAP", NULL },
		{ 171, "GIVEGOLDFORCE", NULL },
		{ 172, "CHANGETILESTATE", NULL },
		{ 173, "ADDJOURNALENTRY", NULL },
		{ 174, "EQUIPRANGED", NULL },
		{ 175, "SETLEAVEPARTYDIALOGUEFILE", NULL },
		{ 176, "ESCAPEAREADESTROY", NULL },
		{ 177, "TRIGGERACTIVATION", NULL },
		{ 178, "BREAKINSTANTS", NULL },
		{ 179, "DIALOGUEINTERRUPT", NULL },
		{ 180, "MOVETOOBJECTFOLLOW", NULL },
		{ 181, "REALLYFORCESPELL", NULL },
		{ 182, "MAKEUNSELECTABLE", NULL },
		{ 183, "MULTIPLAYERSYNC", NULL },
		{ 184, "RUNAWAYFROMNOINTERRUPT", NULL },
		{ 185, "SETMASTERAREA", NULL },
		{ 186, "ENDCREDITS", NULL },
		{ 187, "STARTMUSIC", NULL },
		{ 188, "TAKEPARTYITEMALL", NULL },
		{ 189, "LEAVEAREALUAPANIC", NULL },
		{ 190, "SAVEGAME", NULL },
		{ 191, "SPELLNODEC", NULL },
		{ 192, "SPELLPOINTNODEC", NULL },
		{ 193, "TAKEPARTYITEMRANGE", NULL },
		{ 194, "CHANGEANIMATION", NULL },
		{ 195, "LOCK", NULL },
		{ 196, "UNLOCK", NULL },
		{ 197, "MOVEGLOBAL", NULL },
		{ 198, "STARTDIALOGNOSET", NULL },
		{ 199, "TEXTSCREEN", NULL },
		{ 200, "RANDOMWALKCONTINUOUS", NULL },
		{ 201, "DETECTSECRETDOOR", NULL },
		{ 202, "FADETOCOLOR", NULL },
		{ 203, "FADEFROMCOLOR", NULL },
		{ 204, "TAKEPARTYITEMNUM", NULL },
		{ 207, "MOVETOPOINTNOINTERRUPT", NULL },
		{ 208, "MOVETOOBJECTNOINTERRUPT", NULL },
		{ 209, "SPAWNPTACTIVATE", NULL },
		{ 210, "SPAWNPTDEACTIVATE", NULL },
		{ 211, "SPAWNPTSPAWN", NULL },
		{ 212, "GLOBALSHOUT", NULL },
		{ 213, "STATICSTART", NULL },
		{ 214, "STATICSTOP", NULL },
		{ 215, "FOLLOWOBJECTFORMATION", NULL },
		{ 216, "ADDFAMILIAR", NULL },
		{ 217, "REMOVEFAMILIAR", NULL },
		{ 218, "PAUSEGAME", NULL },
		{ 219, "CHANGEANIMATIONNOEFFECT", NULL },
		{ 220, "TAKEITEMLISTPARTY", NULL },
		{ 221, "SETMORALEAI", NULL },
		{ 222, "INCMORALEAI", NULL },
		{ 223, "DESTROYALLEQUIPMENT", NULL },
		{ 224, "GIVEPARTYALLEQUIPMENT", NULL },
		{ 225, "MOVEBETWEENAREASEFFECT", NULL },
		{ 226, "TAKEITEMLISTPARTYNUM", NULL },
		{ 227, "CREATECREATUREOBJECTEFFECT", NULL },
		{ 228, "CREATECREATUREIMPASSABLE", NULL },
		{ 229, "FACEOBJECT", NULL },
		{ 230, "RESTPARTY", NULL },
		{ 231, "CREATECREATUREDOOR", NULL },
		{ 232, "CREATECREATUREOBJECTDOOR", NULL },
		{ 233, "CREATECREATUREOBJECTOFFSCREEN", NULL },
		{ 234, "MOVEGLOBALOBJECTOFFSCREEN", NULL },
		{ 235, "SETQUESTDONE", NULL },
		{ 236, "STOREPARTYLOCATIONS", NULL },
		{ 237, "RESTOREPARTYLOCATIONS", NULL },
		{ 238, "CREATECREATUREOFFSCREEN", NULL },
		{ 239, "MOVETOCENTEROFSCREEN", NULL },
		{ 240, "REALLYFORCESPELLDEAD", NULL },
		{ 241, "CALM", NULL },
		{ 242, "ALLY", NULL },
		{ 243, "RESTNOSPELLS", NULL },
		{ 244, "SAVELOCATION", NULL },
		{ 245, "SAVEOBJECTLOCATION", NULL },
		{ 246, "CREATECREATUREATLOCATION", NULL },
		{ 247, "SETTOKEN", NULL },
		{ 248, "SETTOKENOBJECT", NULL },
		{ 249, "SETGABBER", NULL },
		{ 250, "CREATECREATUREOBJECTCOPYEFFECT", NULL },
		{ 251, "HIDEAREAONMAP", NULL },
		{ 252, "CREATECREATUREOBJECTOFFSET", NULL },
		{ 253, "CONTAINERENABLE", NULL },
		{ 254, "SCREENSHAKE", NULL },
		{ 255, "ADDGLOBALS", NULL },
		{ 256, "CREATEITEMGLOBAL", NULL },
		{ 257, "PICKUPITEM", NULL },
		{ 258, "FILLSLOT", NULL },
		{ 259, "ADDXPOBJECT", NULL },
		{ 260, "DESTROYGOLD", NULL },
		{ 261, "SETHOMELOCATION", NULL },
		{ 262, "DISPLAYSTRINGNONAME", NULL },
		{ 263, "ERASEJOURNALENTRY", NULL },
		{ 264, "COPYGROUNDPILESTO", NULL },
		{ 265, "DIALOGFORCEINTERRUPT", NULL },
		{ 266, "STARTDIALOGUEINTERRUPT", NULL },
		{ 267, "STARTDIALOGNOSETINTERRUPT", NULL },
		{ 268, "REALSETGLOBALTIMER", NULL },
		{ 269, "DISPLAYSTRINGHEAD", NULL },
		{ 270, "POLYMORPHCOPY", NULL },
		{ 271, "VERBALCONSTANTHEAD", NULL },
		{ 272, "CREATEVISUALEFFECT", NULL },
		{ 273, "CREATEVISUALEFFECTOBJECT", NULL },
		{ 274, "ADDKIT", NULL },
		{ 275, "STARTCOMBATCOUNTER", NULL },
		{ 276, "ESCAPEAREANOSEE", NULL },
		{ 277, "ESCAPEAREAOBJECTMOVE", NULL },
		{ 278, "TAKEITEMREPLACE", NULL },
		{ 279, "ADDSPECIALABILITY", NULL },
		{ 280, "DESTROYALLDESTRUCTABLEEQUIPMENT", NULL },
		{ 281, "REMOVEPALADINHOOD", NULL },
		{ 282, "REMOVERANGERHOOD", NULL },
		{ 283, "REGAINPALADINHOOD", NULL },
		{ 284, "REGAINRANGERHOOD", NULL },
		{ 285, "POLYMORPHCOPYBASE", NULL },
		{ 286, "HIDEGUI", NULL },
		{ 287, "UNHIDEGUI", NULL },
		{ 288, "SETNAME", NULL },
		{ 289, "ADDSUPERKIT", NULL },
		{ 290, "PLAYDEADINTERRUPTIBLE", NULL },
		{ 291, "MOVEGLOBALOBJECT", NULL },
		{ 292, "DISPLAYSTRINGHEADOWNER", NULL },
		{ 293, "STARTDIALOGOVERRIDE", NULL },
		{ 294, "STARTDIALOGOVERRIDEINTERRUPT", NULL },
		{ 295, "CREATECREATURECOPYPOINT", NULL },
		{ 296, "BATTLESONG", NULL },
		{ 297, "MOVETOSAVEDLOCATIONN", NULL },
		{ 298, "APPLYDAMAGE", NULL },
		{ 299, "BANTERBLOCKTIME", NULL },
		{ 300, "BANTERBLOCKFLAG", NULL },
		{ 301, "AMBIENTACTIVATE", NULL },
		{ 302, "ATTACHTRANSITIONTODOOR", NULL },
		{ 303, "DEATHMATCHPOSITIONGLOBAL", NULL },
		{ 304, "DEATHMATCHPOSITIONAREA", NULL },
		{ 305, "DEATHMATCHPOSITIONLOCAL", NULL },
		{ 306, "APPLYDAMAGEPERCENT", NULL },
		{ 307, "SG", NULL },
		{ 308, "ADDMAPNOTE", NULL },
		{ 309, "DEMOEND", NULL },
		{ 310, "MOVEGLOBALSTO", NULL },
		{ 311, "DISPLAYSTRINGWAIT", NULL },
		{ 312, "STATEOVERRIDETIME", NULL },
		{ 313, "STATEOVERRIDEFLAG", NULL },
		{ 314, "SETRESTENCOUNTERPROBABILITYDAY", NULL },
		{ 315, "SETRESTENCOUNTERPROBABILITYNIGHT", NULL },
		{ 316, "SOUNDACTIVATE", NULL },
		{ 317, "PLAYSONG", NULL },
		{ 318, "FORCESPELLRANGE", NULL },
		{ 319, "FORCESPELLPOINTRANGE", NULL },
		{ 320, "SETPLAYERSOUND", NULL },
		{ 321, "SETAREARESTFLAG", NULL },
		{ 322, "FAKEEFFECTEXPIRYCHECK", NULL },
		{ 323, "CREATECREATUREIMPASSABLEALLOWOVERLAP", NULL },
		{ 324, "SETBEENINPARTYFLAGS", NULL },
};


std::string
GetActionName(int32 id)
{
	for (auto action: kActionsTable) {
		if (action.id == id)
			return std::string(action.name);
	}
	return "";
}


int32
GetActionID(std::string name)
{
	for (auto action: kActionsTable) {
		if (::strcasecmp(action.name, name.c_str()) == 0) {
			std::cout << "in: " << name << ", found: " << action.name << ", id: " << action.id << std::endl;
			return action.id;
		}
	}
	return -1;
}


const ActionDescriptor*
GetActionDescriptor(int32 id)
{
	static const std::unordered_map<int32, const ActionDescriptor*> sById = [] {
		std::unordered_map<int32, const ActionDescriptor*> map;
		for (const auto& descriptor : kActionsTable)
			map.emplace(descriptor.id, &descriptor);
		return map;
	}();

	auto found = sById.find(id);
	return found != sById.end() ? found->second : NULL;
}


bool
IsInstantAction(int32 id)
{
	// Same caching rationale as GetActionDescriptor(): whether an action id
	// is "instant" never changes at runtime, so avoid loading/scanning the
	// IDS resource on every single call.
	static std::unordered_map<int32, bool> sInstantCache;

	auto cached = sInstantCache.find(id);
	if (cached != sInstantCache.end())
		return cached->second;

	bool isInstant = false;
	IDSResource* instants = gResManager->GetIDS("INSTANT");
	if (instants != NULL) {
		isInstant = instants->StringForID(id) != "";
		gResManager->ReleaseResource(instants);
	}

	sInstantCache[id] = isInstant;
	return isInstant;
}

