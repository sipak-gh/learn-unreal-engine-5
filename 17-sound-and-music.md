# Chapter 17 — Sound and Music

GridSnake looks finished but plays in silence. This chapter covers Unreal's audio pipeline from
the ground up: sound assets and how to import them, fire-and-forget 2D sounds, audio components
that you keep and control, MetaSounds (Epic's procedural audio system) with parameters set from
C++, and sound classes and mixes for volume control. GridSnake gains sound effects for eating a
gem, dying, the exit opening and finishing a level, a music track that keeps playing across level
loads and opens up as you collect gems, and Music and SFX volume sliders on the Options screen.

Previous: [Chapter 16](16-menus-pause-and-input-modes.md). Next:
[Chapter 18](18-saving-progress.md) (saving progress).

## Audio in Unreal

Every playable sound asset in Unreal derives from one base class, `USoundBase`. Code that only
needs "something to play" stores a `USoundBase*`, and the designer can then pick any of the
concrete kinds in the Details panel:

| Asset | Class | What it is |
|---|---|---|
| Sound Wave | `USoundWave` | One imported audio file (WAV, and a few other formats). The raw material. |
| Sound Cue | `USoundCue` | A node graph that combines waves: random choice, modulation, concatenation. The older system; still supported and fine for simple variation. |
| MetaSound Source | `UMetaSoundSource` | A procedural audio graph that runs sample by sample on the audio thread, with typed inputs you can set while it plays. The modern system. |

A `UPROPERTY` of type `TObjectPtr<USoundBase>` accepts all three. That is why every sound
property in GridSnake uses the base type: you can start with a plain wave and later swap in a
Sound Cue or a MetaSound without touching code (exercise 4 does exactly that).

Two more ideas run through the chapter:

- **Sound classes** (`USoundClass`) are categories such as "Music" or "SFX". Every sound can
  belong to one. **Sound mixes** (`USoundMix`) adjust whole classes at once, for example "music at
  50%". This is how volume sliders work.
- **2D or spatialized.** A spatialized (3D) sound has a position in the world. Unreal pans it
  between speakers and makes it quieter with distance, using a **Sound Attenuation** setting. A 2D
  sound has no position: it plays at the same volume, centred, wherever the listener is.

For GridSnake the choice is easy. The camera never moves, everything is on one screen, and the
listener sits at the camera, 500 units above the board. With spatialization, a gem at the edge of
the board would sound quieter and off-centre compared with one in the middle, which is noise, not
information. A top-down, single-screen 2D game should play its sounds in 2D. Attenuation is worth
learning when you make a game with a scrolling or 3D world; the Sources list links the page.

Unreal's audio engine (the *Audio Mixer*) mixes all active sounds on its own audio thread; game
code only asks for sounds to start, stop and change. The MetaSound plugin (`MetasoundEngine` and friends) is enabled by default in
new 5.8 projects, so nothing needs to be turned on. None of the C++ in this chapter needs a module
beyond `Core`, `CoreUObject` and `Engine`: `UGameplayStatics`, `UAudioComponent`, `USoundClass` and
`USoundMix` all live in `Engine`.

## Importing sounds

The course's art generator also writes the sounds. If you ran it in chapter 4, the `.wav` files
are already in `RawAssets/Audio`. If not, run it now from the project folder (the folder with
`GridSnake.uproject`). These are the audio lines of a real run; the texture lines are trimmed:

```console
$ python3 Tools/make_placeholder_art.py
...
wrote Audio/SW_Eat.wav (0.12s)
wrote Audio/SW_Die.wav (0.60s)
wrote Audio/SW_ExitOpen.wav (0.36s)
wrote Audio/SW_LevelComplete.wav (0.84s)
wrote Audio/SW_UIClick.wav (0.04s)
wrote Audio/SW_Music.wav (8.00s)
```

The files are deliberately plain. Python's `wave` module reports them as 16-bit mono PCM at
44.1 kHz, the safest format to import:

```console
$ python3 -c "
import wave, glob, os
for p in sorted(glob.glob('RawAssets/Audio/*.wav')):
    w = wave.open(p)
    print('%-22s %d ch, %d-bit, %d Hz, %d frames' % (os.path.basename(p), w.getnchannels(),
          w.getsampwidth() * 8, w.getframerate(), w.getnframes()))
"
SW_Die.wav             1 ch, 16-bit, 44100 Hz, 26460 frames
SW_Eat.wav             1 ch, 16-bit, 44100 Hz, 5292 frames
SW_ExitOpen.wav        1 ch, 16-bit, 44100 Hz, 15876 frames
SW_LevelComplete.wav   1 ch, 16-bit, 44100 Hz, 37044 frames
SW_Music.wav           1 ch, 16-bit, 44100 Hz, 352800 frames
SW_UIClick.wav         1 ch, 16-bit, 44100 Hz, 1764 frames
```

`SW_Music` is exactly eight seconds: 16 beats at 120 beats per minute, so its end joins its start
without a gap or a click when it loops. A loop that is a fraction of a beat too long or short
stumbles audibly at the seam.

Import them:

1. In the **Content Browser**, create the folder `Content/GridSnake/Audio` (right-click >
   **New Folder**).
2. Drag the six `.wav` files from your file manager into that folder. Unreal creates one
   **Sound Wave** asset per file and names it after the file: `SW_Eat`, `SW_Die`, `SW_ExitOpen`,
   `SW_LevelComplete`, `SW_UIClick` and `SW_Music`.
3. Double-click `SW_Music`. Under **Sound** in its Details, tick **Looping** and save. The
   MetaSound will loop it anyway, but the flag documents its purpose and lets you preview the loop.
4. Hover over a sound in the Content Browser and click the play button on its thumbnail to hear it.

Two Sound Wave settings matter in this chapter:

- **Looping**: plays forever instead of once.
- **Class** (in the **Sound** category; the C++ property is `SoundClassObject`): the sound class
  the wave belongs to. You will set it after creating the classes below.

Sound waves are compressed when the game is cooked; the defaults are fine for a game this size.

## Fire-and-forget sounds

Most game sounds are one-shots: something happens, a sound plays, nobody needs to refer to it
again. `UGameplayStatics::PlaySound2D` does exactly that:

```cpp
// Kismet/GameplayStatics.h
static void PlaySound2D(const UObject* WorldContextObject, USoundBase* Sound,
    float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f,
    USoundConcurrency* ConcurrencySettings = nullptr, const AActor* OwningActor = nullptr,
    bool bIsUISound = true);
```

- `WorldContextObject` is any object that can find the world: `this` in an actor or component.
  You met this pattern with `GetPlayerPawn(this, 0)` in chapter 7.
- `VolumeMultiplier` and `PitchMultiplier` scale the asset's own settings. A pitch multiplier of
  1.1 plays 10% higher *and* 10% faster, like speeding up a tape.
- `StartTime` skips into the sound.
- It returns nothing: no handle, so no stopping or changing it later. That is the point.
- `bIsUISound` (default `true`) means the sound keeps playing while the game is paused.

Its spatialized sibling is `PlaySoundAtLocation(this, Sound, Location)`, which you would use in a
3D or scrolling game together with an attenuation setting.

The first example plays a sound once per second at a slightly different pitch each time.

`examples/ch17/ExSoundPlayer.h`

```cpp
// Example for chapter 17 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExSoundPlayer.generated.h"

class USoundBase;

/**
 * Plays a sound in 2D once per second, each time at a slightly different pitch.
 * Drop it in a level, pick any Sound Wave for Sound, and press Play.
 */
UCLASS()
class AExSoundPlayer : public AActor
{
	GENERATED_BODY()

public:
	/** Any sound asset: a Sound Wave, a Sound Cue or a MetaSound Source. */
	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundBase> Sound;

	/** How many times BeginPlay's timer plays the sound. */
	UPROPERTY(EditAnywhere, Category = "Sound", meta = (ClampMin = "0"))
	int32 Repeats = 3;

	/** Plays Sound once at a random pitch between 0.9 and 1.1. */
	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayVaried();

protected:
	virtual void BeginPlay() override;

private:
	void HandleTimer();

	FTimerHandle RepeatTimer;
	int32 PlayedCount = 0;
};
```

`examples/ch17/ExSoundPlayer.cpp`

```cpp
// Example for chapter 17 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExSoundPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

void AExSoundPlayer::BeginPlay()
{
	Super::BeginPlay();

	if (!Sound)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: set Sound in the Details panel"), *GetName());
		return;
	}

	GetWorldTimerManager().SetTimer(RepeatTimer, this, &AExSoundPlayer::HandleTimer, /*InRate=*/1.0f, /*InbLoop=*/true);
}

void AExSoundPlayer::HandleTimer()
{
	PlayVaried();

	if (++PlayedCount >= Repeats)
	{
		GetWorldTimerManager().ClearTimer(RepeatTimer);
	}
}

void AExSoundPlayer::PlayVaried()
{
	if (!Sound)
	{
		return;
	}

	// A small random pitch change stops a repeated sound from sounding mechanical.
	const float Pitch = FMath::FRandRange(0.9f, 1.1f);
	UGameplayStatics::PlaySound2D(this, Sound, /*VolumeMultiplier=*/1.0f, /*PitchMultiplier=*/Pitch);

	UE_LOG(LogTemp, Log, TEXT("Played %s at pitch %.2f"), *Sound->GetName(), Pitch);
}
```

To try it:

1. Copy both files into your project's `Source/<Module>/` folder and build (or use Live Coding,
   **Ctrl+Alt+F11**, if you only changed a `.cpp`; a new class needs a full build, as chapter 2
   explained).
2. Find `ExSoundPlayer` in the **Place Actors** panel (or the Content Browser's **C++ Classes**
   folder) and drag it into any level.
3. In its Details panel, set **Sound > Sound** to `SW_Eat` (or any Sound Wave).
4. Press **Play**. You hear three "blips", one per second, each slightly different.

Expected Output Log (filter: `LogTemp`; the pitches are random):

```
LogTemp: Played SW_Eat at pitch <0.90..1.10>
LogTemp: Played SW_Eat at pitch <0.90..1.10>
LogTemp: Played SW_Eat at pitch <0.90..1.10>
```

- The timer gives a gap between plays. Three overlapping copies of the same sound would all play
  at once: `PlaySound2D` happily starts a new instance every call. Unreal limits runaway overlap
  with **Sound Concurrency** settings (a Concurrency asset or the per-sound settings), which can
  cap how many instances of a sound play at once and what happens to the oldest.
- `FMath::FRandRange(Min, Max)` returns a random float in the range. A pitch spread of ±10% is
  enough to stop a repeated sound from sounding like a machine gun, and small enough that it still
  sounds like the same event. GridSnake's exercise 1 applies it to the gem sound.
- The `Sound` property is `USoundBase`, so the actor plays waves, cues and MetaSounds alike.
- The most common mistake is a missing include. Without `Kismet/GameplayStatics.h`, the compiler
  reports `UGameplayStatics` as undeclared. Without `Sound/SoundBase.h`, `Sound->GetName()` fails
  with "use of undefined type" / "incomplete type": a forward declaration can store a pointer but
  not call through it. The header forward-declares, the `.cpp` includes (chapter 5).

## Audio components and persistent music

A one-shot sound cannot be stopped, faded or changed. For anything you want to control after it
starts, such as music, an engine hum or a looping alarm, you want an **audio component**:
`UAudioComponent`. It is a component that owns one playing sound and gives you a handle with
methods such as:

| Method | Effect |
|---|---|
| `Play(StartTime = 0)` | Starts the sound. |
| `Stop()` | Stops it at once. |
| `FadeIn(Duration, VolumeLevel = 1, StartTime = 0)` | Starts playing at volume 0 and ramps up. |
| `FadeOut(Duration, VolumeLevel)` | Ramps to `VolumeLevel`; at 0 the sound stops when the fade ends. |
| `AdjustVolume(Duration, VolumeLevel)` | Ramps the volume of a playing sound without stopping it. |
| `IsPlaying()` | True while the component's sound is active. |
| `SetFloatParameter(Name, Value)` | Sets a named input on the sound (MetaSounds; covered below). |
| `SetVolumeMultiplier`, `SetPitchMultiplier` | Change the multipliers directly. |

You can add an audio component to an actor like any other component (`CreateDefaultSubobject`,
chapter 5). For sounds started at runtime, `UGameplayStatics` has two helpers:

- `SpawnSound2D(...)` creates a component and plays it immediately.
- `CreateSound2D(...)` creates it *without* playing it, so you can set it up first.

```cpp
// Kismet/GameplayStatics.h
static UAudioComponent* CreateSound2D(const UObject* WorldContextObject, USoundBase* Sound,
    float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f,
    USoundConcurrency* ConcurrencySettings = nullptr, bool bPersistAcrossLevelTransition = false,
    bool bAutoDestroy = true);
```

The last two parameters decide the component's lifetime:

- **`bAutoDestroy`**: with the default `true`, the component destroys itself when the sound
  finishes. That is right for a one-off sound you only want to stop early. For music you want to
  fade, restart or query, pass `false` and keep the pointer.
- **`bPersistAcrossLevelTransition`**: normally the component belongs to the current world and
  dies with it when a new map opens (chapter 14: a level change destroys everything in the
  world). With `true`, the component is created for the audio device instead of the world, so the
  sound keeps playing while the old map unloads and the new one loads.

The function can return `nullptr`, for example when the game runs without audio (a dedicated
server, or the `-nosound` command-line switch). Always check.

The second example creates a component, fades it in, sets a MetaSound parameter, waits, fades out
and reports `IsPlaying()` at every step.

`examples/ch17/ExMusicFader.h`

```cpp
// Example for chapter 17 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExMusicFader.generated.h"

class UAudioComponent;
class USoundBase;

/**
 * Starts Music with a fade-in, sets one MetaSound input, and fades the music out
 * again after HoldSeconds. Logs IsPlaying() along the way.
 */
UCLASS()
class AExMusicFader : public AActor
{
	GENERATED_BODY()

public:
	/** Best with a looping sound: a looping Sound Wave or a MetaSound Source. */
	UPROPERTY(EditAnywhere, Category = "Music")
	TObjectPtr<USoundBase> Music;

	/** Name of a Float input on the MetaSound. Ignored by sounds without that input. */
	UPROPERTY(EditAnywhere, Category = "Music")
	FName ParameterName = TEXT("Intensity");

	UPROPERTY(EditAnywhere, Category = "Music")
	float ParameterValue = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Music", meta = (ClampMin = "0", Units = "s"))
	float FadeSeconds = 2.0f;

	/** Time from the start of the fade-in to the start of the fade-out. */
	UPROPERTY(EditAnywhere, Category = "Music", meta = (ClampMin = "0", Units = "s"))
	float HoldSeconds = 5.0f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void StartFadeOut();
	void ReportAfterFadeOut();

	/** bAutoDestroy is false, so we own the component; the UPROPERTY keeps it alive. */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicComponent;

	FTimerHandle FadeTimer;
};
```

`examples/ch17/ExMusicFader.cpp`

```cpp
// Example for chapter 17 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExMusicFader.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

void AExMusicFader::BeginPlay()
{
	Super::BeginPlay();

	if (!Music)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: set Music in the Details panel"), *GetName());
		return;
	}

	// Creates the component but does not play it yet.
	MusicComponent = UGameplayStatics::CreateSound2D(this, Music, /*VolumeMultiplier=*/1.0f, /*PitchMultiplier=*/1.0f,
		/*StartTime=*/0.0f, /*ConcurrencySettings=*/nullptr, /*bPersistAcrossLevelTransition=*/false, /*bAutoDestroy=*/false);
	if (!MusicComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateSound2D returned null (no audio device?)"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Created %s: IsPlaying=%s"), *Music->GetName(), MusicComponent->IsPlaying() ? TEXT("true") : TEXT("false"));

	// FadeIn starts playback at volume 0 and ramps up to 1.
	MusicComponent->FadeIn(FadeSeconds);
	UE_LOG(LogTemp, Log, TEXT("After FadeIn(%.1f): IsPlaying=%s"), FadeSeconds, MusicComponent->IsPlaying() ? TEXT("true") : TEXT("false"));

	// Sets a MetaSound input by name. Safe to call on any sound: unknown names are ignored.
	MusicComponent->SetFloatParameter(ParameterName, ParameterValue);
	UE_LOG(LogTemp, Log, TEXT("Set %s = %.2f"), *ParameterName.ToString(), ParameterValue);

	GetWorldTimerManager().SetTimer(FadeTimer, this, &AExMusicFader::StartFadeOut, HoldSeconds, /*InbLoop=*/false);
}

void AExMusicFader::StartFadeOut()
{
	// Fading to volume 0 stops the sound when the fade ends. Until then it still plays.
	MusicComponent->FadeOut(FadeSeconds, /*FadeVolumeLevel=*/0.0f);
	UE_LOG(LogTemp, Log, TEXT("After FadeOut(%.1f, 0): IsPlaying=%s"), FadeSeconds, MusicComponent->IsPlaying() ? TEXT("true") : TEXT("false"));

	GetWorldTimerManager().SetTimer(FadeTimer, this, &AExMusicFader::ReportAfterFadeOut, FadeSeconds + 0.5f, /*InbLoop=*/false);
}

void AExMusicFader::ReportAfterFadeOut()
{
	UE_LOG(LogTemp, Log, TEXT("%.1f s after the fade-out started: IsPlaying=%s"), FadeSeconds + 0.5f, MusicComponent->IsPlaying() ? TEXT("true") : TEXT("false"));
}

void AExMusicFader::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The component is not attached to this actor, so stop it explicitly.
	if (MusicComponent)
	{
		MusicComponent->Stop();
	}

	Super::EndPlay(EndPlayReason);
}
```

To try it: drop `ExMusicFader` into a level, set **Music > Music** to `SW_Music` (or, after the
MetaSounds section, `MSS_Music`) and press **Play**. The music swells in over two seconds, plays,
and fades away about five seconds after it started.

Expected Output Log (filter: `LogTemp`; times are approximate):

```
LogTemp: Created SW_Music: IsPlaying=false
LogTemp: After FadeIn(2.0): IsPlaying=true
LogTemp: Set Intensity = 1.00
                                                  (about 5 s later)
LogTemp: After FadeOut(2.0, 0): IsPlaying=true
                                                  (about 2.5 s later)
LogTemp: 2.5 s after the fade-out started: IsPlaying=false
```

- `CreateSound2D` returns an idle component: `IsPlaying()` is false until `Play` or `FadeIn`.
- `FadeOut` does not stop the sound immediately. It is still "playing" (just getting quieter)
  until the fade reaches 0, and only then does the component stop. If you need to know when it is
  really over, bind the component's `OnAudioFinished` delegate instead of guessing with a timer.
- With a plain Sound Wave, `SetFloatParameter` finds no input called `Intensity` and does nothing.
  That is deliberate behaviour you can rely on: the same code works for any sound.
- `bAutoDestroy` is `false`, so the component survives the end of the sound and the pointer stays
  valid. Hold such a pointer in a `UPROPERTY` (`TObjectPtr`), as chapter 3 explained, or the
  garbage collector may free the component under you.
- A component created by `CreateSound2D` with a world context but no owner is not attached to the
  actor, so destroying the actor would not stop it. `EndPlay` stops it explicitly. The same
  thinking appears in GridSnake's audio subsystem below.

### Music that survives a level change

GridSnake's music should start once, in the main menu, and keep playing through every level load
without restarting. Two things make that work:

1. **Who owns the component.** An actor or a game mode dies with its world. The only objects that
   live from the first map to the last are the GameInstance and its subsystems (chapter 14). So a
   new game instance subsystem, `UGSAudioSubsystem`, owns the music component.
2. **What the component belongs to.** `bPersistAcrossLevelTransition = true` creates the
   component for the audio device rather than for the current world. Without it, the music would
   stop as soon as the old world was torn down, even if the subsystem still held the pointer.

Every level's game mode tells the subsystem "a level started". The subsystem starts the music only
if it is not already playing:

```cpp
if (MusicComponent && MusicComponent->IsPlaying())
{
    return;
}
```

The first call (in `L_MainMenu`) starts it; later calls do nothing unless the music was stopped.

## MetaSounds

A Sound Wave is a recording. A **MetaSound** is a small program that *produces* audio: a graph of
nodes such as oscillators, wave players, filters, envelopes and maths, evaluated on the audio
thread for every block of samples. MetaSounds replace Sound Cues for new work, and they can do
things a Sound Cue cannot, because their nodes process the audio itself, not just choose and
adjust waves.

The feature this chapter needs is **inputs**: a MetaSound can declare named, typed inputs (Float,
Int32, Bool, Trigger, Wave Asset and more), and gameplay code can set them while the sound plays.
That is how you make *adaptive* music: one piece of music whose sound changes with the game state,
instead of hard cuts between separate tracks.

There are two MetaSound asset types:

- **MetaSound Source**: a playable sound (`UMetaSoundSource`, a `USoundBase`). This is what you
  play, and what GridSnake uses.
- **MetaSound Patch**: a reusable piece of graph that you place inside other MetaSounds, like a
  function. Useful once your graphs grow; not needed here.

### The music graph

`MSS_Music` plays the looping music wave through a low-pass filter. A low-pass filter lets low
frequencies through and cuts high ones. With a low cutoff (800 Hz), the music sounds muffled, as if
heard through a wall. With the cutoff near the top of human hearing (18 000 Hz), the filter does
nearly nothing and the music sounds bright and present. An `Intensity` input from 0 to 1 moves the
cutoff between those two values. Gems raise the intensity, so the music "opens up" as the player
closes in on the exit.

```
Inputs:  Intensity (Float, default 0)

On Play ──► Wave Player (Mono)  Wave Asset = SW_Music, Loop = true
                 │ Out Mono
                 ▼
           One-Pole Low Pass Filter ──► Out Mono (the source's mono output)
                 ▲ Cutoff Frequency
Intensity ──► Map Range (In 0..1 → Out 800..18000)
```

Node display names below are those of the 5.8 MetaSound editor's node search as far as the
documentation shows them; they may differ slightly in your version. If a name does not match,
type part of it ("Wave Player", "Low Pass", "Map Range") into the search and pick the closest
match.

1. In `Content/GridSnake/Audio`, right-click > **Audio > MetaSound Source**. Name it `MSS_Music`
   and double-click it. The MetaSound editor opens with an empty graph containing the source's
   built-in **On Play** input and its output (the label may differ slightly in your version).
2. Make the source mono, because the music wave is mono: select the source in the editor (or open
   its **Details**) and set **Output Format** to **Mono**. The graph's output node now has a single
   **Out Mono** audio pin.
3. Add the input: in the **Members** panel, next to **Inputs**, click **+**. Name the new input
   `Intensity`, set its type to **Float** and its default value to `0`. The name must match the
   C++ string `"Intensity"` exactly, including case. Drag the input from the Members panel into
   the graph.
4. Right-click the graph, search for **Wave Player (Mono)** and add it. In its pins (or the Details
   panel with the node selected) set **Wave Asset** = `SW_Music` and tick **Loop**.
5. Connect **On Play** to the Wave Player's **Play** trigger pin. Without this the wave never
   starts.
6. Add a **One-Pole Low Pass Filter**. Connect the Wave Player's **Out Mono** to the filter's audio
   **In**, and the filter's **Out** to the graph output's **Out Mono**.
7. Add a **Map Range** (Float) node. Set its input range to 0..1 and its output range to
   800..18000, and turn **Clamped** on if the node has that option. Connect **Intensity** to its
   **In** and its **Out** to the filter's **Cutoff Frequency**.
8. In the source's Details, set **Sound > Class** = `SC_Music`. (Create `SC_Music` first if you
   follow this chapter out of order; see the next section.)
9. Press **Play** in the MetaSound editor's toolbar. While it plays, select the `Intensity` input
   and drag its default value between 0 and 1: the music moves from muffled to bright.
10. Save.

A one-shot MetaSound wires the Wave Player's **On Finished** to the output's **On Finished** to
tell the engine it is done. A looping one never finishes; the audio component stops it.

Two hedged details to check if your input seems to do nothing:

- MetaSound inputs can be marked as **constructor** inputs (a checkbox in the input's details, the
  label may differ slightly). A constructor input is read only once, when the sound starts, so
  later `SetFloatParameter` calls have no effect. `Intensity` must not be a constructor input.
- Map Range's mapping is linear. Hearing is closer to logarithmic in frequency, so most of the
  audible change happens low in the range. That is fine for GridSnake's three gems.

### Setting the input from C++

`UAudioComponent` implements Unreal's audio parameter interface, so setting an input is one call:

```cpp
MusicComponent->SetFloatParameter(TEXT("Intensity"), 0.5f);
```

There are matching calls for the other input types, such as `SetIntParameter`,
`SetBoolParameter` and `SetTriggerParameter` (which fires a Trigger input). All of them take an
`FName`. A
misspelt name does not fail loudly; nothing happens. That is why GridSnake declares the name once
as a constant (`UGSAudioSubsystem::IntensityParameter`) instead of repeating a string literal.

The value travels to the audio thread and the graph picks it up at its next processing block, a
few milliseconds later. You can call it every frame if you need to; GridSnake only calls it when a
gem is collected and when a level starts.

## Sound classes and mixes

Volume sliders need a way to say "all music at 60%" without visiting every sound. Unreal has two
asset types for this:

- A **Sound Class** (`USoundClass`) is a named category with default properties (volume, pitch
  and more). Each sound points at one class with its **Class** setting. Classes can have child
  classes, so a "Master" class could contain "Music" and "SFX".
- A **Sound Mix** (`USoundMix`) is a set of *adjusters*: "class X: volume × 0.6, pitch × 1.0".
  A mix does nothing until it is **pushed**: the audio device keeps a stack of active mixes, and
  every active mix's adjusters apply to the sounds of the classes they name.

```
SW_Eat ──┐                             SMix_Volumes (pushed once, stays active)
SW_Die ──┼── Class = SC_SFX   ◄─────── adjuster: SC_SFX   volume × SfxVolume
...    ──┘
MSS_Music ── Class = SC_Music ◄─────── adjuster: SC_Music volume × MusicVolume
```

You can fill in a mix's adjusters in its Details panel (**Sound Classes > Sound Class Effects**,
the label may differ slightly in your version). For volume settings, though, the values change at
runtime, so GridSnake creates the mix *empty* and writes adjusters from C++:

```cpp
// Kismet/GameplayStatics.h
static void SetSoundMixClassOverride(const UObject* WorldContextObject, USoundMix* InSoundMixModifier,
    USoundClass* InSoundClass, float Volume = 1.0f, float Pitch = 1.0f, float FadeInTime = 1.0f,
    bool bApplyToChildren = true);
static void PushSoundMixModifier(const UObject* WorldContextObject, USoundMix* InSoundMixModifier);
static void PopSoundMixModifier(const UObject* WorldContextObject, USoundMix* InSoundMixModifier);
```

- `SetSoundMixClassOverride` overrides the adjuster for `InSoundClass` in the mix, adding one if
  the mix had none for that class. `FadeInTime` ramps to the new volume; 0 applies it at once,
  which is what a slider wants. `bApplyToChildren` makes the adjustment reach child classes too.
- The override only matters while the mix is active, so push it.
- `PushSoundMixModifier` on a mix that is already active does not stack a second copy; it adds a
  reference. `PopSoundMixModifier` removes one. `ClearSoundMixModifiers` removes them all.
- Active mixes belong to the audio device, not to an actor. Do not rely on exactly when the engine
  resets them, though: GridSnake simply re-applies its mix every time a level starts.

The third example is these calls in isolation.

`examples/ch17/ExVolumeControl.h`

```cpp
// Example for chapter 17 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExVolumeControl.generated.h"

class USoundClass;
class USoundMix;

/**
 * Sets the volume of one Sound Class through a Sound Mix: the same two calls
 * GridSnake's audio subsystem makes for its Music and SFX sliders.
 */
UCLASS()
class AExVolumeControl : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Volume")
	TObjectPtr<USoundMix> Mix;

	UPROPERTY(EditAnywhere, Category = "Volume")
	TObjectPtr<USoundClass> SoundClass;

	/** Applied in BeginPlay. 0 = silent, 1 = unchanged. */
	UPROPERTY(EditAnywhere, Category = "Volume", meta = (ClampMin = "0", ClampMax = "1"))
	float StartVolume = 0.25f;

	/** Scales every sound of InClass (and its child classes) by Volume while InMix is active. */
	UFUNCTION(BlueprintCallable, Category = "Volume")
	void SetClassVolume(USoundMix* InMix, USoundClass* InClass, float Volume);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
```

`examples/ch17/ExVolumeControl.cpp`

```cpp
// Example for chapter 17 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExVolumeControl.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

void AExVolumeControl::BeginPlay()
{
	Super::BeginPlay();

	SetClassVolume(Mix, SoundClass, StartVolume);
}

void AExVolumeControl::SetClassVolume(USoundMix* InMix, USoundClass* InClass, float Volume)
{
	if (!InMix || !InClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetClassVolume needs a Sound Mix and a Sound Class"));
		return;
	}

	// 1. Write (or add) the adjuster for this class into the mix.
	//    FadeInTime 0 applies it at once; bApplyToChildren reaches child classes too.
	UGameplayStatics::SetSoundMixClassOverride(this, InMix, InClass, Volume, /*Pitch=*/1.0f,
		/*FadeInTime=*/0.0f, /*bApplyToChildren=*/true);

	// 2. A mix only does something while it is on the stack of active mixes.
	UGameplayStatics::PushSoundMixModifier(this, InMix);

	UE_LOG(LogTemp, Log, TEXT("%s volume = %.2f through %s"), *InClass->GetName(), Volume, *InMix->GetName());
}

void AExVolumeControl::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// A pushed mix stays active until it is popped.
	if (Mix)
	{
		UGameplayStatics::PopSoundMixModifier(this, Mix);
		UE_LOG(LogTemp, Log, TEXT("Popped %s"), *Mix->GetName());
	}

	Super::EndPlay(EndPlayReason);
}
```

To try it, create the assets of the next steps first (`SC_Music`, `SC_SFX`, `SMix_Volumes`, and
`SW_Music` with **Class** = `SC_Music`). Then place both `ExMusicFader` (Music = `SW_Music`) and
`ExVolumeControl` (Mix = `SMix_Volumes`, Sound Class = `SC_Music`, Start Volume 0.25) in a level
and press **Play**. The music is clearly quieter than without the volume control. Set Start Volume
to 0 and it is silent.

Expected Output Log (filter: `LogTemp`), with the music fader's lines omitted:

```
LogTemp: SC_Music volume = 0.25 through SMix_Volumes
LogTemp: Popped SMix_Volumes                       (when you stop Play In Editor)
```

- If the music is *not* quieter, check the wave's **Class**. A sound with no class, or a different
  class, is not touched by the adjuster. This is the number one reason volume sliders "do nothing".
- Adjusters scale; they do not replace. `SC_Music`'s own Volume (1) × the sound's own volume ×
  the mix adjuster (0.25) gives the result.
- The example pops the mix in `EndPlay`. GridSnake never pops its volume mix: the player's volume
  settings should apply for the whole session, in every level and menu.

## Debugging audio

When a sound does not play, check in this order:

1. **Did the code run?** A `UE_LOG` next to the play call answers that. Chapter 19 adds more
   tools.
2. **Is the property set?** Every play call in GridSnake is guarded by `if (Sound)`, so a missing
   Blueprint assignment is silent. Open the Blueprint and look.
3. **Is the class muted?** A Sound Class volume of 0 or a mix adjuster of 0 silences a sound
   while it still "plays".
4. **Is the sound actually playing?** In Play In Editor, open the console with the backtick key
   (**`**) and type `stat sound` or `au.Debug`: autocomplete lists the audio stat groups (such as
   `stat SoundMixes`, which shows the active mixes) and debug variables. Their exact names vary
   between versions, so pick from the list rather than typing from memory.

Recent engine versions also include **Audio Insights**, a dedicated audio debugging window built
on Unreal Insights (chapter 19). If the plugin is enabled in your project, you find it under the
**Tools** menu (the label may differ slightly in your version).

## GridSnake: sound effects, music and volume

Everything that changed between `project/ch16` and `project/ch17` (no Build.cs change; all the
audio classes live in `Engine`):

| File | Change |
|---|---|
| `Config/DefaultGame.ini`, `Core/GSSettings.h` | Four soft-reference settings in an **Audio** category |
| `Core/GSAudioSubsystem.h/.cpp` | New: persistent music, intensity and volumes |
| `Core/GSGameMode.h/.cpp` | Level-complete sound; starts the music; raises the intensity per gem |
| `Core/GSMenuGameMode.cpp` | Starts the music in the main menu |
| `Gameplay/GSGem`, `Gameplay/GSExit`, `Snake/GSSnake` | Pickup, opening and death sounds |
| `UI/GSOptionsWidget.h/.cpp` | Music and SFX sliders |

### Settings: which assets to use

The audio subsystem is a plain C++ class with no Blueprint subclass, so nobody can set asset
properties on it in a Details panel. It reads them from the project settings class instead, the
same way the level list arrived in chapter 13. `Core/GSSettings.h` gains:

```cpp
	/** Looping music (MSS_Music), started once and kept playing across levels. */
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundBase> Music;

	/** The mix that applies the player's volume settings (SMix_Volumes). */
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundMix> VolumeMix;

	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundClass> MusicClass;

	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundClass> SfxClass;
```

and `Config/DefaultGame.ini` the matching lines at the end of the GridSnake section:

```ini
; GridSnake (ch17)
Music=/Game/GridSnake/Audio/MSS_Music.MSS_Music
VolumeMix=/Game/GridSnake/Audio/SMix_Volumes.SMix_Volumes
MusicClass=/Game/GridSnake/Audio/SC_Music.SC_Music
SfxClass=/Game/GridSnake/Audio/SC_SFX.SC_SFX
```

They are soft references (chapter 13): the settings object stores paths, and nothing loads until
the subsystem calls `LoadSynchronous()`. Picking the assets in **Project Settings > Game >
GridSnake > Audio** writes the same lines.

### The audio subsystem

`UGSAudioSubsystem` is a game instance subsystem (chapter 14), so it exists once for the whole
session and survives every level change. It owns three jobs: the persistent music, the music's
intensity, and the two volume values.

`Core/GSAudioSubsystem.h`

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GSAudioSubsystem.generated.h"

class UAudioComponent;

/**
 * Music that keeps playing across level loads, and the volume settings.
 * Sound effects are played by the actors themselves (UGameplayStatics::PlaySound2D);
 * their volume follows the SC_SFX sound class, which this subsystem controls.
 */
UCLASS()
class UGSAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/** Called by the game modes when a map starts: starts the music once and re-applies volumes. */
	void HandleLevelStarted(UWorld* World);

	/** 0 = calm, 1 = all gems collected. Drives the "Intensity" input of the music MetaSound. */
	void SetMusicIntensity(float Intensity);

	UFUNCTION(BlueprintCallable, Category = "GridSnake|Audio")
	void SetMusicVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "GridSnake|Audio")
	void SetSfxVolume(float Volume);

	UFUNCTION(BlueprintPure, Category = "GridSnake|Audio")
	float GetMusicVolume() const { return MusicVolume; }

	UFUNCTION(BlueprintPure, Category = "GridSnake|Audio")
	float GetSfxVolume() const { return SfxVolume; }

	/** Name of the float input on MSS_Music. */
	static const FName IntensityParameter;

private:
	/** Writes the volumes into the SMix_Volumes sound mix and makes sure it is active. */
	void ApplyVolumes();

	/**
	 * Created with bPersistAcrossLevelTransition, so it is not owned by any
	 * world and keeps playing while maps load. The UPROPERTY keeps it alive.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicComponent;

	float MusicVolume = 0.8f;
	float SfxVolume = 1.0f;
};
```

`Core/GSAudioSubsystem.cpp`

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSAudioSubsystem.h"
#include "GridSnake.h"
#include "Components/AudioComponent.h"
#include "Core/GSSettings.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

const FName UGSAudioSubsystem::IntensityParameter(TEXT("Intensity"));

void UGSAudioSubsystem::Deinitialize()
{
	// The music is not owned by a world, so nothing else would stop it
	// when Play In Editor ends.
	if (MusicComponent)
	{
		MusicComponent->Stop();
		MusicComponent = nullptr;
	}

	Super::Deinitialize();
}

void UGSAudioSubsystem::HandleLevelStarted(UWorld* World)
{
	ApplyVolumes();
	SetMusicIntensity(0.0f);

	if (MusicComponent && MusicComponent->IsPlaying())
	{
		return;
	}

	USoundBase* Music = UGSSettings::Get()->Music.LoadSynchronous();
	if (!Music)
	{
		return;
	}

	MusicComponent = UGameplayStatics::CreateSound2D(World, Music, /*VolumeMultiplier=*/1.0f, /*PitchMultiplier=*/1.0f,
		/*StartTime=*/0.0f, /*ConcurrencySettings=*/nullptr, /*bPersistAcrossLevelTransition=*/true, /*bAutoDestroy=*/false);
	if (MusicComponent)
	{
		MusicComponent->FadeIn(/*FadeInDuration=*/1.0f);
		UE_LOG(LogGridSnake, Log, TEXT("Music started: %s"), *Music->GetName());
	}
}

void UGSAudioSubsystem::SetMusicIntensity(float Intensity)
{
	if (MusicComponent)
	{
		// Does nothing if the sound has no input with this name (a plain Sound Wave).
		MusicComponent->SetFloatParameter(IntensityParameter, FMath::Clamp(Intensity, 0.0f, 1.0f));
	}
}

void UGSAudioSubsystem::SetMusicVolume(float Volume)
{
	MusicVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyVolumes();
}

void UGSAudioSubsystem::SetSfxVolume(float Volume)
{
	SfxVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyVolumes();
}

void UGSAudioSubsystem::ApplyVolumes()
{
	const UGSSettings* Settings = UGSSettings::Get();
	USoundMix* Mix = Settings->VolumeMix.LoadSynchronous();
	USoundClass* MusicClass = Settings->MusicClass.LoadSynchronous();
	USoundClass* SfxClass = Settings->SfxClass.LoadSynchronous();
	if (!Mix || !MusicClass || !SfxClass)
	{
		UE_LOG(LogGridSnake, Warning, TEXT("Volume settings need VolumeMix, MusicClass and SfxClass in Project Settings > GridSnake"));
		return;
	}

	// A sound mix scales the volume of every sound whose Sound Class it overrides.
	// Overrides only take effect while the mix is pushed (active).
	UGameInstance* GameInstance = GetGameInstance();
	UGameplayStatics::SetSoundMixClassOverride(GameInstance, Mix, MusicClass, MusicVolume, /*Pitch=*/1.0f, /*FadeInTime=*/0.0f, /*bApplyToChildren=*/true);
	UGameplayStatics::SetSoundMixClassOverride(GameInstance, Mix, SfxClass, SfxVolume, /*Pitch=*/1.0f, /*FadeInTime=*/0.0f, /*bApplyToChildren=*/true);
	// Pushing a mix that is already active only adds a reference; we never pop it.
	UGameplayStatics::PushSoundMixModifier(GameInstance, Mix);
}
```

In the order things happen:

- **`HandleLevelStarted`** runs at the start of every map. It re-applies the volumes and resets
  the intensity to 0 (the previous level may have ended at 1). If music is already playing it
  stops there. Otherwise it loads `MSS_Music`, creates a persistent, non-auto-destroying
  component and fades it in. `World` is only the world context `CreateSound2D` uses to find the
  audio device; the component does not belong to that world.
- **`SetMusicIntensity`** clamps to 0..1 and forwards to the MetaSound input. With no music set
  it does nothing, so gameplay never depends on audio.
- **`IntensityParameter`** is a `static const FName` defined once in the `.cpp`. Building an
  `FName` from a string costs a name-table lookup, so a constant is both faster and safer.
- **`ApplyVolumes`** writes both adjusters into `SMix_Volumes` and pushes it, using the game
  instance as the world context (a subsystem is not in a world). A missing setting produces a
  warning that names the fix, the habit from chapter 13.
- **`Deinitialize`** stops the music. When Play In Editor ends, the game instance and its
  subsystems are torn down, but a persistent sound has no world to die with. Without this
  `Stop()`, the music could outlive the session.
- The volumes live only in memory and reset when the game restarts. Chapter 18 saves them.

### Sounds in the actors

Each actor that makes a noise gets a `USoundBase` property and a guarded `PlaySound2D` call:

| Class | Property (category) | Asset | Played in |
|---|---|---|---|
| `AGSGem` | `PickupSound` (Gem) | `SW_Eat` | `OnSnakeEntered_Implementation`, just before `Destroy()` |
| `AGSExit` | `OpenSound` (Exit) | `SW_ExitOpen` | `SetOpen` |
| `AGSSnake` | `DeathSound` (Audio) | `SW_Die` | `Die`, after `PlayHeadAnim(EGSHeadAnim::Dead)` |
| `AGSGameMode` | `LevelCompleteSound` (Audio) | `SW_LevelComplete` | `OnExitReached`, after `SetPhase(Completed)` |

Every property has the same shape, and every call site too:

```cpp
	/** Played when the gem is collected (SW_Eat). */
	UPROPERTY(EditDefaultsOnly, Category = "Gem")
	TObjectPtr<USoundBase> PickupSound;
```

```cpp
	// 2D: no position, no attenuation. Right for a game seen from one fixed camera.
	if (PickupSound)
	{
		UGameplayStatics::PlaySound2D(this, PickupSound);
	}

	Destroy();
```

- The gem plays its sound and destroys itself in the same function. That is safe because
  `PlaySound2D` hands the sound to the audio device; the gem is only the world context. A sound on
  the gem's own audio component would be cut off.
- The exit's call has one extra condition, because `SetOpen` also runs in the editor when you
  tick **Is Open** on a placed exit (chapter 12):

  ```cpp
  	if (bIsOpen && OpenSound && HasActorBegunPlay())
  ```

- Each sound sits with the object that knows the event happened.

The game mode also drives the music. It gains a `GetAudio()` helper, mirroring `GetLevelFlow()`
from chapter 14, and two calls in `Core/GSGameMode.cpp`:

```cpp
	// StartPlay():
	LevelFlow->NotifyLevelStarted(GetWorld());

	Super::StartPlay();

	GetAudio()->HandleLevelStarted(GetWorld());
```

```cpp
	// OnGemCollected(), after AddGem and Grow:
	// The music gets busier with every gem.
	GetAudio()->SetMusicIntensity(float(State->GetGemsCollected()) / float(FMath::Max(State->GetGemsRequired(), 1)));
```

```cpp
UGSAudioSubsystem* AGSGameMode::GetAudio() const
{
	return GetGameInstance()->GetSubsystem<UGSAudioSubsystem>();
}
```

- The intensity is the fraction of gems collected (1/3, 2/3, 1). The `float` casts matter: `1 / 3`
  in integer arithmetic is 0. `FMath::Max(..., 1)` avoids dividing by zero.
- The game mode, not the gem, sets the intensity: the gem only reports "I was collected"
  (chapter 11), and the reaction lives with the rules.
- `Core/GSMenuGameMode.cpp` makes the same `HandleLevelStarted` call after its `Super::StartPlay()`,
  so the music starts in the menu and the first level continues it.

### Volume sliders

`UI/GSOptionsWidget.h` adds two `BindWidget` sliders, `MusicVolumeSlider` and `SfxVolumeSlider`
(type `USlider`, forward-declared), and two `UFUNCTION()` handlers. `UI/GSOptionsWidget.cpp`
includes `Components/Slider.h` and `Core/GSAudioSubsystem.h`, then:

```cpp
	// NativeOnInitialized():
	MusicVolumeSlider->OnValueChanged.AddDynamic(this, &UGSOptionsWidget::HandleMusicVolumeChanged);
	SfxVolumeSlider->OnValueChanged.AddDynamic(this, &UGSOptionsWidget::HandleSfxVolumeChanged);

	// Refresh(), before the focus call:
	const UGSAudioSubsystem* Audio = GetGameInstance()->GetSubsystem<UGSAudioSubsystem>();
	MusicVolumeSlider->SetValue(Audio->GetMusicVolume());
	SfxVolumeSlider->SetValue(Audio->GetSfxVolume());
```

```cpp
void UGSOptionsWidget::HandleMusicVolumeChanged(float Value)
{
	GetGameInstance()->GetSubsystem<UGSAudioSubsystem>()->SetMusicVolume(Value);
}

void UGSOptionsWidget::HandleSfxVolumeChanged(float Value)
{
	GetGameInstance()->GetSubsystem<UGSAudioSubsystem>()->SetSfxVolume(Value);
}
```

- `OnValueChanged` is a dynamic multicast delegate with one `float`, bound with `AddDynamic` like
  the check boxes in chapter 16.
- The widget stores no volume. The subsystem is the single source of truth: `Refresh` reads it
  whenever the panel opens and the handlers write it.
- The documentation does not promise whether `SetValue` also fires `OnValueChanged`. It does not
  matter here: if it fires, the handler writes the same value back. Handlers that are safe to call
  with an unchanged value make the question irrelevant.
- `FadeInTime` 0 in `ApplyVolumes` makes the change audible while the slider moves.

### Editor steps

These follow `project/ch17/ASSETS.md`. Build first (close the editor, build, reopen) so the new
properties and the subsystem exist.

1. **Import the sounds** into `Content/GridSnake/Audio` ("Importing sounds") and tick **Looping**
   on `SW_Music`.
2. **Create the classes and the mix** in the same folder: right-click > **Audio > Classes > Sound
   Class** twice, for `SC_Music` and `SC_SFX` (volume 1), and **Audio > Mix > Sound Mix** for
   `SMix_Volumes`. Leave the mix empty: C++ adds the adjusters. If the submenus differ in your
   version, type "Sound Class" or "Sound Mix" into the **Add** menu's search.
3. **Assign the effects to `SC_SFX`.** Set **Sound > Class** = `SC_SFX` on `SW_Eat`, `SW_Die`,
   `SW_ExitOpen`, `SW_LevelComplete` and `SW_UIClick`. To do all five at once, select them,
   right-click > **Asset Actions > Edit Selection in Property Matrix**. `SW_Music` needs no class:
   it plays only through `MSS_Music`, whose class counts.
4. **Create `MSS_Music`** ("The music graph") with **Sound > Class** = `SC_Music`.
5. **Settings:** **Edit > Project Settings > Game > GridSnake > Audio**: **Music** = `MSS_Music`,
   **Volume Mix** = `SMix_Volumes`, **Music Class** = `SC_Music`, **Sfx Class** = `SC_SFX`.
6. **Blueprints** (open each, **Class Defaults**): `BP_GSGem` **Gem > Pickup Sound** = `SW_Eat`;
   `BP_GSExit` **Exit > Open Sound** = `SW_ExitOpen`; `BP_GSSnake` **Audio > Death Sound** =
   `SW_Die`; `BP_GSGameMode` **Audio > Level Complete Sound** = `SW_LevelComplete`.
7. **`W_Options`:** under the check-box rows, add two **Horizontal Box** rows, each with a
   **Text** ("Music", "SFX") and a **Slider** named exactly **MusicVolumeSlider** /
   **SfxVolumeSlider**. Set **Min Value** 0, **Max Value** 1, **Step Size** 0.05, and give each
   slider width (slot **Size** = **Fill**, or a **Size Box**); a slider with no width is
   invisible. The widget will not compile until both `BindWidget` names exist.
8. **Click sounds:** for every button in `W_MainMenu`, `W_LevelSelect`, `W_LevelButton`,
   `W_Options`, `W_PauseMenu` and `W_LevelEnd`, set **Style > Pressed Sound** = `SW_UIClick`
   (under **Appearance > Style**). Slate plays it; no code. It is in `SC_SFX`, so the SFX slider
   controls it too.
9. **Test** from `L_MainMenu`: the music fades in, muffled. Press **Play**: the screen fades, the
   level loads, and the music continues without restarting. Each gem blips and brightens the
   music; the exit plays a fanfare as it opens; reaching it plays the jingle; crashing plays the
   death sound. In **Options**, the sliders change the volumes live.

Expected Output Log (filter: `LogGridSnake`; other lines omitted), playing from `L_MainMenu`,
pressing Play and collecting a gem:

```
LogGridSnake: Level flow subsystem initialised
LogGridSnake: Level L_MainMenu started (index -1, run score 0)
LogGridSnake: Music started: MSS_Music
LogGridSnake: Opening /Game/GridSnake/Maps/L_Level01.L_Level01
LogGridSnake: Level L_Level01 started (index 0, run score 0)
...
LogGridSnake: Gem 1/3
```

`Music started` appears once per session. A second one after `Opening ...` means the music did
not survive the level change: check `bPersistAcrossLevelTransition`. The warning `Volume settings
need VolumeMix, MusicClass and SfxClass in Project Settings > GridSnake` means step 5 is incomplete.

### The camera fade and the music

Chapter 14's level flow fades the camera with `bShouldFadeAudio = true`, in both directions:

```cpp
PlayerController->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, FadeSeconds, FLinearColor::Black,
    /*bShouldFadeAudio=*/true, /*bHoldWhenFinished=*/true);
```

With that flag the camera manager fades the game's audio with the picture, and now that includes
the persistent music: it dips while the screen is black and swells back as the next level fades
in. This interaction is the least documented part of the chapter, so check it by ear. If you
prefer music straight through transitions, or the music stays silent after a level change (for
example on a test map whose game mode never calls `NotifyLevelStarted`, which starts the fade-in),
pass `false` for `bShouldFadeAudio` in both `StartCameraFade` calls in `GSLevelFlowSubsystem.cpp`.

## Summary

- Every sound asset is a `USoundBase`: a Sound Wave (an imported file), a Sound Cue (a legacy
  graph of waves) or a MetaSound Source (a procedural graph with inputs). Store `USoundBase`
  properties so any of them can be used.
- A single-screen top-down game plays its sounds in 2D: no position, no attenuation.
- `PlaySound2D` is fire-and-forget: no handle, safe just before the caller destroys itself.
- `UAudioComponent` is the handle for sounds you control (`FadeIn`, `FadeOut`, `IsPlaying`,
  `SetFloatParameter`). `CreateSound2D` creates one without playing it; `bAutoDestroy = false`
  keeps it after the sound ends, and `bPersistAcrossLevelTransition = true` keeps it playing
  through map loads. Hold it in a `UPROPERTY`.
- Persistent music belongs in a game instance subsystem: started once, guarded with
  `IsPlaying()`, stopped in `Deinitialize`.
- MetaSound inputs are set by name; a wrong name is silently ignored, so keep names in one
  constant. Constructor inputs cannot change after the sound starts.
- Sound classes group sounds; a pushed sound mix scales whole classes.
  `SetSoundMixClassOverride` + `PushSoundMixModifier` is all a volume slider needs.

GridSnake now sounds like a game, but it forgets everything when it closes: the volumes, the
unlocked levels, the best scores. Chapter 18 fixes that.

## Exercises

1. **Vary the gem sound.** Make each pickup play at a slightly random pitch, with the spread
   editable in `BP_GSGem`.

   <details><summary>Solution</summary>

   In `Gameplay/GSGem.h`, next to `PickupSound`:

   ```cpp
   	/** Random pitch spread for PickupSound: 0.1 plays between 0.9 and 1.1. */
   	UPROPERTY(EditDefaultsOnly, Category = "Gem", meta = (ClampMin = "0", ClampMax = "0.5"))
   	float PickupPitchVariation = 0.1f;
   ```

   In `Gameplay/GSGem.cpp`, replace the play call:

   ```cpp
   	if (PickupSound)
   	{
   		const float Pitch = 1.0f + FMath::FRandRange(-PickupPitchVariation, PickupPitchVariation);
   		UGameplayStatics::PlaySound2D(this, PickupSound, /*VolumeMultiplier=*/1.0f, Pitch);
   	}
   ```

   A variation: raise the pitch with each gem (1.0, 1.12, 1.25) to tell the player how close the
   exit is. Only the game mode knows the count, so move the sound to `OnGemCollected`.
   </details>

2. **Duck the music while paused.** The music should sound distant while the pause menu is open.

   <details><summary>Solution</summary>

   Intensity 0 is the muffled sound, so reuse it. In `Core/GSPlayerController.cpp`, include
   `Core/GSAudioSubsystem.h`, then:

   ```cpp
   // PauseGame(), after SetPause(true) succeeded:
   	GetGameInstance()->GetSubsystem<UGSAudioSubsystem>()->SetMusicIntensity(0.0f);

   // ResumeGame(), after SetPause(false):
   	if (const AGSGameState* State = GetWorld()->GetGameState<AGSGameState>())
   	{
   		GetGameInstance()->GetSubsystem<UGSAudioSubsystem>()->SetMusicIntensity(
   			float(State->GetGemsCollected()) / float(FMath::Max(State->GetGemsRequired(), 1)));
   	}
   ```

   The music keeps running while paused because `CreateSound2D` makes it a UI sound. To lower
   the volume instead, create `SMix_Paused` with a **Sound Class Effects** entry for `SC_Music`
   at volume 0.3, and push it in `PauseGame` and pop it in `ResumeGame`
   (`PushSoundMixModifier` / `PopSoundMixModifier`). Restarting or quitting from the pause menu
   never calls `ResumeGame`, so also pop it in the controller's `EndPlay`.
   </details>

3. **A step tick.** Play a quiet tick on every snake step, so the player hears the speed rise.

   <details><summary>Solution</summary>

   In `Snake/GSSnake.h`:

   ```cpp
   	/** Played quietly on every step. */
   	UPROPERTY(EditDefaultsOnly, Category = "Audio")
   	TObjectPtr<USoundBase> StepSound;

   	UPROPERTY(EditDefaultsOnly, Category = "Audio", meta = (ClampMin = "0", ClampMax = "1"))
   	float StepVolume = 0.25f;
   ```

   In `AGSSnake::Step()`, just before the final `UpdateVisuals()` (after the death checks, so a
   fatal step plays only the death sound):

   ```cpp
   	if (StepSound)
   	{
   		UGameplayStatics::PlaySound2D(this, StepSound, StepVolume);
   	}
   ```

   Set it to `SW_UIClick` in `BP_GSSnake`. The ticks speed up with the step interval. If they
   overlap messily at high speed, give the sound a **Concurrency** setting with **Max Count** 1
   and **Stop Oldest** (the labels may differ slightly in your version).
   </details>

4. **Two eating sounds.** Replace `SW_Eat` with a Sound Cue that picks one of two waves at random.
   Change no code.

   <details><summary>Solution</summary>

   1. Duplicate `SW_Eat` as `SW_Eat2` and set its **Pitch** to 1.3 (or import another file).
   2. Right-click > **Audio > Sound Cue**; name it `SCue_EatRandom` (`SC_` is taken by sound
      classes).
   3. In the cue editor, drag both waves in (creating **Wave Player** nodes), add a **Random**
      node, connect both players to it and the Random node to the **Output**.
   4. Set the cue's **Sound > Class** = `SC_SFX`, so the SFX slider still applies.
   5. In `BP_GSGem`, set **Pickup Sound** to the cue.

   `PickupSound` is a `TObjectPtr<USoundBase>` and a Sound Cue is a `USoundBase`: this is the
   payoff of using the base type.
   </details>

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-17--sound-and-music)
