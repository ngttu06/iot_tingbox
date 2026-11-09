#pragma once

// Trial default. Build with -DAUDIO_USE_DFPLAYER=0 to return to MAX98357A.
#ifndef AUDIO_USE_DFPLAYER
#define AUDIO_USE_DFPLAYER 1
#endif
#if AUDIO_USE_DFPLAYER != 0 && AUDIO_USE_DFPLAYER != 1
#error "AUDIO_USE_DFPLAYER must be 0 or 1"
#endif