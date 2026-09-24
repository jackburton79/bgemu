#include "ScreenSupport.h"

#include "BamResource.h"
#include "Bitmap.h"
#include "ResManager.h"
#include "SPLResource.h"


Bitmap*
ScreenSupport::MakeSpellIcon(const res_ref& spellName)
{
	SPLResource* spl = gResManager->GetSPL(spellName);
	if (spl == NULL)
		return NULL;
	Bitmap* icon = NULL;
	BAMResource* bam = gResManager->GetBAM(spl->BookIcon());
	if (bam != NULL) {
		icon = bam->FrameForCycle(0, 0);
		gResManager->ReleaseResource(bam);
	}
	gResManager->ReleaseResource(spl);
	return icon;
}
