#pragma once

#include "IETypes.h"

class Bitmap;


// Helpers several screens (and the parts of Game that aren't screens yet)
// share.
namespace ScreenSupport {

// The spellbook-icon frame for a spell resref (SPL 0x3a -> BAM cycle 0
// frame 0). Caller owns the returned reference, or NULL.
Bitmap* MakeSpellIcon(const res_ref& spellName);

}
