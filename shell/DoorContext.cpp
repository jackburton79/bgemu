/*
 * DoorContext.cpp
 *
 *  Created on: 08/ott/2012
 *      Author: stefano
 */

#include "DoorContext.h"

DoorContext::DoorContext(ShellContext* parent, Door* door)
	:
	ShellContext(parent),
	fDoor(door)
{
}


DoorContext::~DoorContext()
{
}


void
DoorContext::PrintPrompt()
{

}


void
DoorContext::ListCommands()
{

}


ShellContext*
DoorContext::HandleCommand(std::string line)
{
	return fParentContext;
}
