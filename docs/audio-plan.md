# Audio: state and plan

## Where it stands

- `SoundEngine` (libjgame/audio): one SDL device with a ring buffer for MVE
  movie audio, plus a pool of 8 one-shot devices (`PlaySample()`, raw PCM).
- `resources/ACMDecoder.*` decodes Interplay ACM (WAVC resources, bare `.acm`
  files) to 16 bit PCM; `WAVResource::DecodePCM()` covers plain RIFF and WAVC.
  Before it existed every voice file failed to play (a warning) - only the few
  plain-PCM effect sounds worked.
- `Core::PlaySound(resref)` = decode + `PlaySample()`. Users so far: the
  PLAYSOUND/VERBALCONSTANT actions, effect 174, loot window open/close, and
  dialogs (the TLK entry of a line names its recording).
- Music stream (step 1 of the music plan, done): `AudioStream` (libjgame) is a
  source of streamed PCM; `SoundEngine::PlayStream()/StopStream()` play one on a
  device of its own (format = the stream's) with a bus volume and a fade in/out,
  replacing the previous one; `IsStreamPlaying()` turns false when the track
  ends. `game/ACMStream` streams a loose `.acm` (read into memory, decoded on
  demand by the audio thread), `game/GameFiles` finds files below the game
  directory case-insensitively (BG1's `Music/Bc1/Bc1a1.ACM`).
- Tests: `Assert-Sound`, `Assert-LastSound`, `Dump-Sound <res>,<file.wav>`;
  `Play-Music-File`, `Stop-Music [fadeMs]`, `Print-Music`, `Assert-Music`,
  `Assert-MusicPosition`, `Wait-Audio <ms>` (real time), `Dump-Music-File`.

## Steps, in order

1. **A mixer** (replaces the one-device-per-sound pool). One SDL device, a
   callback that sums N voices (16 bit, stereo, 44100 or the device rate,
   sources resampled) with per-voice volume/pan and named buses: voice, effects,
   music, ambient. Streaming voices pull PCM from a decoder on demand (music is
   several minutes long; decode it in the callback thread's buffer, not up
   front). Master/bus volumes from `baldur.ini` ("Volume Music", "Volume
   Voices", "Volume Ambients", "Volume SFX").
2. **Soundset lines.** The CRE's 100 soundset strrefs (`CREResource::
   SoundSetStringRef`, slots per SNDSLOT.IDS): selection ("Ready", "Yes sir?"),
   order acknowledgment (move/attack/talk), battle cry, hurt, dying, "party
   member is hungry" ... Plays on `ClickedOn`/selection/move orders, one voice
   per actor at a time (a new line cuts the old one), never overlapping the
   same slot twice within ~1 s.
3. **Music.** ARE header's song references (day/night/win/battle/lose) index
   `SONGLIST.2DA` -> a `.mus` playlist in `<game>/music/`. A `.mus` file is a
   subfolder name, a count and lines `<track> [<loop target>] [@TAG <interrupt>]`
   (see docs/iesdp-gh-pages/file_formats/ie_formats/mus.htm); tracks are
   `<folder><track>.acm`, bare ACM streamed through `ACMDecoder`. Needed:
   `MusicPlayer` (playlist state, end-of-list loop, fade out/in, interrupt
   tags), area start/leave/night switch, combat music while any enemy is
   fighting the party and back afterwards, actions SETMUSIC/MUSICLIST-style
   (PlaySong, StartSong, "Song" action variants, in ACTION.IDS). BG1 keeps its
   music on the CDs - `music/` may be missing: fall back to silence.
4. **Ambients.** ARE ambient table (name of up to 10 WAVs, position/radius,
   volume, interval and jitter, day/night/time-of-day mask, looping or random
   one-shot, "area-wide" flag): the `AreaRoom` keeps a list, each ticks its own
   timer and plays through the ambient bus attenuated by the distance from the
   party leader. Area-wide ambient loops are the base sound of a room.
5. **World effects.** Footsteps (`walksnd.2da`: per animation id and terrain
   material of the search map; GemRB `Actor::PlayWalkSound`), weapon and hit
   sounds (`itemsnd.2da`, `defsound.2da`; `Actor::PlayHitSound`), door and
   container sounds (containers already), spell casting (SPL header sound),
   projectiles and the `sound` fields of VVC effects.
6. **UI sounds.** Button clicks (GAM_01..), inventory pick/drop (per item
   type sound), scroll and page-turn, level up jingle, gold, "journal updated".
7. **Movies' audio and the mixer.** Route `MoviePlayer` through the mixer so a
   cutscene and its music/ambients duck each other correctly.

Each step gets a console assert (`Assert-LastSound`-style, or a mixer state
dump, e.g. `Print-Voices`) so it can be checked headless; the actual audibility
is a manual check.

## Formats worth knowing

- WAVC: 28 byte header (`WAVC`, version, uncompressed size, compressed size,
  data offset, channels?...) then an ACM stream: signature 0x01032897, samples
  (total, all channels), channels, rate, levels/subblocks. Decoded in
  `ACMDecoder` (a port of GemRB's ACM reader).
- TLK entry: flags (bit 1 text, bit 2 sound, bit 3 token), sound resref (8),
  volume, pitch, text offset, length. `TLKEntry::sound_ref`.
- `SONGLIST.2DA` (bg1/bg2): row = song id, column = `.mus` name (and per-game
  extras); `SNDSLOT.IDS` names the CRE soundset slots.
- GemRB references: `core/Audio.cpp`, `plugins/OpenALAudio` (mixer, ambients),
  `core/MusicMgr.cpp` + `plugins/MUSImporter`, `core/Ambient.cpp`,
  `GameScript/Actions.cpp` (Song/PlaySound actions).
