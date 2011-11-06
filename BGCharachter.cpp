#include "BGCharachter.h"
#include "Utils.h"

#include "FileStream.h"

BGCharachter::BGCharachter()
{
}


void
BGCharachter::Load(const char *fileName)
{
	TFileStream inFile(fileName);
	char array[5];
	inFile.Read(array, 4);
	array[4] = '\0';
	
	if (CheckSignature(array, CHR_SIGNATURE)) {
		inFile.Read(array, 4);
		array[4] = '\0';
		CheckVersion(array);
		inFile.Read(fName, CHR_NAME_LENGTH);
		printf("Player's name: %s\n", fName);
		inFile >> fCREOffset;	
		printf("CRE Offset: %ld\n", fCREOffset);
		inFile >> fCRELength;
		printf("CRE Length: %ld\n", fCRELength);
		inFile.Seek(fCREOffset, SEEK_SET);
		GetCreatureInfo(inFile);
	}
}

