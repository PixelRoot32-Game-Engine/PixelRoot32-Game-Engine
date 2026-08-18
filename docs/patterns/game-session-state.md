# Pattern: Game Session State (Score / Lives / Game-Over)

> **Status:** deliberate exclusion, not an oversight and not a TODO. There is no
> `GameState`, `ScoreTracker`, or `LivesSystem` class in this engine, and
> Gameplay Framework Phase 2 (`docs/architecture/memory-system.md`) does not
> add one. This page documents why, and shows the pattern to use instead.

If you are looking for the built-in class that tracks score, lives, and
game-over — it does not exist, and this is the answer to "why not", not a
placeholder for "not yet".

---

## Why there is no engine class

At first glance several examples look like they repeat the same trio — `int
score; int lives; bool gameOver;` — which reads as an obvious candidate for a
small reusable class. Checked against all 13 examples under `examples/`, that
impression does not survive: the overlap is much thinner than it looks, and
the shapes that do exist are actively incompatible with each other.

### `score`, `lives`, and `gameOver` are not one recurring shape — they are three

| Field | Examples that have it | Count |
|---|---|---|
| `score` | `2048`, `flappy_bird`, `midway_clone` | 3 of 13 |
| `lives` | `bomberbot`, `midway_clone` | **2 of 13** |
| Full triad (`score` + `lives` + `gameOver`) | none | **0 of 13** |

Two examples out of thirteen is not a pattern worth an engine class — and
those two (`bomberbot`, a bomberman-style game, and `midway_clone`, a
shoot-'em-up) share only `lives`, not the trio. No example at all carries the
full triad.
`metroidvania`, the platformer with the most gameplay logic of any example,
has **none** of the three fields: no `score`, no `lives`, no `gameOver`.
A health/lives concept doesn't even apply the same way to every genre.

### `gameOver` itself has three mutually incompatible shapes

1. **A bare `bool`.** `2048/src/Game2048Logic.h` declares
   `bool gameOver = false;` directly. Binary: the game either has ended or it
   hasn't.

2. **One value inside a 3-state flow enum.** `flappy_bird` has no bare
   `gameOver` field at all. Instead, `GameState` in
   `examples/flappy_bird/src/FlappyBirdConstants.h:12-16` is:

   ```cpp
   enum class GameState {
       WAITING,   ///< Pre-start, waiting for first jump
       RUNNING,   ///< Active gameplay
       GAME_OVER  ///< Bird collided
   };
   ```

   Game-over here is one of three *mutually exclusive session phases*, not a
   flag layered on top of "playing."

3. **Two distinct endings flattened into one predicate.**
   `examples/midway_clone/src/MidwayScene.h:149` declares:

   ```cpp
   bool isTerminal() const { return lives_ <= 0 || stageComplete_; }
   ```

   Died and finished-the-stage are opposite outcomes, and this collapses them
   into a single bool because nothing downstream needs to tell them apart yet.
   A shared type would have had to choose: expose one terminal flag, or expose
   the outcome. Either choice is wrong for one of the games above.

   The same tension shows up *inside* a single example.
   `examples/2048/src/Game2048Logic.h:43-44` declares both:

   ```cpp
   bool gameOver = false;
   bool won = false;
   ```

   Two independent representations of "is this session over" coexisting
   unreconciled in one header — a board can be won and still playable, or over
   and unwon. That is itself the argument against a single shared type: had a
   `GameState`/`ScoreTracker` class existed when this was written, would it
   have modeled "is it over" or "how did it end"? There is no single correct
   answer, because they are not the same question.

None of these three shapes is more "correct" than the others — each is
correct **for its own game**. A shared type would have to pick one shape,
and by definition would be wrong for at least two of the three.

### Even the trivially-compatible case doesn't fit

`2048` tracks both `score` and `gameOver`, but neither field lives on the
`Scene`. Both are members of `Game2048Logic`
(`examples/2048/src/Game2048Logic.h:41,43`) — a plain domain-logic class the
scene owns and queries via `getScore()` / `isGameOver()`. A `Scene`-attached
tracker — the most natural shape for such a class — would not even have
anywhere to attach in this example, because the scene is deliberately a thin
presentation layer over logic that owns its own state. This is the fourth
data point, and it points the same direction as the other three: session
state belongs with the game's own logic, shaped by that game's own rules,
not bolted onto every `Scene` uniformly.

---

## Industry precedent

This isn't just a local observation — it matches how established engines
have handled the same question.

| Engine | Ships a score/lives/game-over type? | What it ships instead |
|---|---|---|
| **Unity** | No | `PlayerPrefs` — generic key-value persistence. Score/lives are always user-defined `MonoBehaviour` fields or a project-specific `GameManager` singleton. |
| **Godot** | No | Documents a project-defined **autoload singleton** (e.g. a `GameState.gd`) as *the* recommended pattern for cross-scene session data — explicitly leaving its shape to the game. |
| **Bevy** (ECS) | No | A plain `Resource` the game defines itself; ECS engines have no opinion on what constitutes "session state" because that's domain data, not engine data. |
| **Unreal Engine** | **Yes** — `Score` lives on `APlayerState` | But `PlayerState` exists as **network-replication infrastructure**: it is Unreal's per-connected-player container, replicated to every client so each peer knows every other player's score. The `Score` field is a side effect of that container already existing for multiplayer, not evidence that "score" deserves first-class engine status on its own. PixelRoot32 has no equivalent — it ships no networking at all today; the ESP-NOW module is a roadmap item (`README.md`, Roadmap). Without replication, there is no structural reason to centralize `Score` the way `APlayerState` does. |
| **GameMaker Studio** | **Shipped one, then deprecated it** | GameMaker is the closest analogue to this project — 2D, sample/tutorial-driven, aimed at exactly this kind of arcade game. It shipped built-in global `score`, `lives`, and `health` variables from its earliest versions. Despite costing next to nothing in memory, they were deprecated in later versions precisely because they baked in an arcade session model (one score, one life counter, one health value, globally) that does not fit every game built with the engine — a puzzle game, a turn-based game, or anything with per-entity rather than per-player health had to work around built-ins that assumed the wrong shape. |

The GameMaker case is the most directly instructive: this is not a
hypothetical risk. A real engine shipped almost exactly the type §5.9
proposes, at effectively zero memory cost, and walked it back once enough
different game shapes proved the built-in model wrong for them.

## The point that actually ties this together

The genuinely universal primitive that established engines *do* provide is
not a score/lives type — it's **persistence**: Unity's `PlayerPrefs`,
Godot's `ConfigFile`. Every one of those precedent engines converges on the
same thing: give the game a place to durably store *whatever shape of
session data it has*, and let the game define that shape itself.

PixelRoot32 already has this on its roadmap as **§5.8 — a generic key-value
persistence system**, scheduled for Gameplay Framework Phase 4. That is the
capability that actually generalizes across every example's ad hoc
`score`/`lives`/`gameOver` trio: not a shared shape for the data, but a
shared mechanism for making whatever shape a given game chooses survive a
reset or a power cycle. §5.9 was reaching for something real — the
duplication is real, engineers should not be reinventing high-score
persistence per example — but the fix is one level down the stack from a
shared struct.

---

## The pattern to use instead

Keep session state as a plain member of your own `Scene`, or — better, for
anything beyond a single flag and a counter — as a small domain-logic class
your scene owns and delegates to. This is not a compromise; it's what every
precedent engine above actually recommends, and it's what the best existing
example in this codebase already does.

**Simple case — session state as `Scene` members**, when the shape really is
just a `score`/`lives`/`gameOver` triple for that one game:

```cpp
class BrickBreakerScene : public pixelroot32::core::Scene {
    // ...
private:
    int score;
    int lives;
    bool gameOver;

public:
    void addScore(int points) { score += points; /* ... */ }
};
```

This is exactly what `examples/midway_clone/src/MidwayScene.h` already does,
with its `score_`, `lives_` and `isTerminal()` sitting directly on the scene. Nothing about
this is a workaround — it is the right amount of structure for two fields
and a flag that are meaningful only to that one game.

**Richer case — a small domain-logic class the scene owns**, when session
state has real rules (win/merge logic, multiple derived values, or state
that shouldn't live in a rendering-facing class):

```cpp
// Game2048Logic owns score + gameOver; the Scene queries it, never
// duplicates it.
class Game2048Logic {
public:
    int getScore() const { return score; }
    bool isGameOver() const { return gameOver; }
    void checkGameOver();

private:
    int score = 0;
    bool gameOver = false;
};

class Game2048Scene : public pixelroot32::core::Scene {
    // ...
private:
    Game2048Logic gameLogic;
    // scoreLabel etc. read from gameLogic, they don't own the value
};
```

This is `examples/2048/src/Game2048Logic.h` and
`examples/2048/src/Game2048Scene.h` as they exist today, and it's the
pattern to reach for once "is the game over" stops being a single boolean
question — 2048's own answer depends on whether any tile can still merge,
which is exactly the kind of per-game rule a shared engine class could never
have anticipated.

**For a flow-state game** (a title screen, a running phase, a game-over
screen, maybe a win screen), model the flow explicitly as your own enum —
as `flappy_bird` already does — rather than layering a
`bool` on top of a `Scene` that's also trying to represent "not started
yet." If your game already needs a state machine for player or enemy
behavior, Gameplay Framework Phase 2's `pixelroot32::gameplay::StateMachine`
(`include/gameplay/StateMachine.h`) is a reasonable, general-purpose place to
put a session-flow enum too — but that's a choice for the specific game,
not something the engine should decide on your behalf.

### What this buys you that a shared class couldn't

- **The shape matches the game.** A binary `gameOver`, a three-phase flow,
  and a four-way round outcome are three different data models; forcing them
  through one shared type would have made at least two of the three worse.
- **No dependency on engine internals for domain rules.** `checkGameOver()`
  in `Game2048Logic` encodes 2048-specific merge rules. That logic has no
  business living in `include/gameplay/`, where it would be compiled into
  every game whether or not it uses that rule.
- **Persistence, when you need it, is a separate concern.** Once §5.8 lands,
  saving/loading whatever session shape you chose becomes a serialization
  problem against a key-value store, not a redesign of your session type.

---

## Summary

- §5.9's premise — that games under this engine duplicate `score`/`lives`/
  `gameOver` handling — is real but overstated: no example shares the full
  triad, only two share even `lives`, and the `gameOver` concept alone takes
  three structurally incompatible shapes across the examples that have it at
  all.
- Unity, Godot, and Bevy ship no such type; Unreal's `Score` is a side
  effect of network-replication infrastructure this engine doesn't have;
  GameMaker shipped one and deprecated it once it proved to fit the wrong
  games.
- The real, generalizable need underneath §5.9 is persistence, already
  tracked as roadmap item §5.8 for Gameplay Framework Phase 4.
- Keep session state as `Scene` members for the simple case, or as a small
  owned domain-logic class (`Game2048Logic` is the working precedent) once
  the rules get non-trivial. This is not a workaround for a missing engine
  feature — it is the correct, final design for domain data that is
  specific to your game.
