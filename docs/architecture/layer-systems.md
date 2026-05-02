# Layer 3: System Layer

## Responsibility

Game engine subsystems that implement high-level functionality. These systems provide the core capabilities that game code builds upon.

---

## Subsystem Overview

| Subsystem | Document |
|-----------|----------|
| **Audio** | [Audio Subsystem](./audio-subsystem.md) |
| **Physics** | [Physics Subsystem](./physics-subsystem.md) |
| **Touch Input** | [Touch Input](./touch-input.md) |
| **Tile Animation** | [Tile Animation](./tile-animation.md) |
| **Resolution Scaling** | [Resolution Scaling](./resolution-scaling.md) |

---

## Architecture Diagrams

### Rendering Pipeline (Game Code → Display)

```mermaid
flowchart TB
    subgraph Game["Game Code"]
        A[Actor::draw] -->|"drawSprite()"| B[Renderer]
    end

    subgraph RendererLayer["Renderer"]
        B -->|"Clip"| C[Viewport Culling]
        C -->|"Transform"| D[World to Screen]
        D -->|"Scale"| E[Logical to Physical]
    end

    subgraph Surface["DrawSurface"]
        E --> F[DrawSurface Interface]
    end

    subgraph Driver["Driver Layer"]
        F --> G[TFT_eSPI]
        F --> H[U8G2]
        F --> I[SDL2]
    end

    subgraph Display["Display"]
        G --> J[LCD Panel]
        H --> K[OLED]
        I --> L[PC Monitor]
    end
```

### Logical vs Physical Resolution

```mermaid
flowchart LR
    subgraph Logical["Logical (128x128)"]
        L1["Pixel at (64, 64)"]
    end

    subgraph Physical["Physical (240x240)"]
        P1["Pixel at (120, 120)"]
    end

    L1 -->|"Scale 1.875x"| P1
```

### Indexed Color → RGB565

```mermaid
flowchart LR
    A[Indexed Color] -->|"Background Palette"| B[RGB565]
    C[Indexed Color] -->|"Sprite Palette"| B
```

### PC Keyboard/Mouse Mapping

```mermaid
flowchart LR
    subgraph Keyboard["Keyboard"]
        A[Arrow Keys]
        B[Z/X/C/V]
        C[Enter/Space]
    end

    subgraph Mapping["Engine Mapping"]
        A -->|maps to| D[UP/DOWN/LEFT/RIGHT]
        B -->|maps to| E[B/A/X/Y]
        C -->|maps to| F[START]
    end

    subgraph Mouse["Mouse"]
        G[Left Click] -->|maps to| H[Touch PRESS/CLICK]
        I[Movement] -->|maps to| J[Touch DRAG]
    end
```

### UI Composition

```mermaid
flowchart TB
    subgraph Scene["Scene"]
        E[Entity list addEntity]
        U[UIManager touch registry]
    end

    subgraph Layouts["Layout containers"]
        V[UIVerticalLayout]
        H[UIHorizontalLayout]
        G[UIGridLayout]
        A[UIAnchorLayout]
        P[UIPanel]
    end

    subgraph ClassicUI["Classic UI entities"]
        L[UILabel]
        B[UIButton]
        C[UICheckBox]
    end

    subgraph TouchUI["Touch widgets"]
        TB[UITouchButton]
        TC[UITouchCheckbox]
        TS[UITouchSlider]
    end

    E --> Layouts
    Layouts --> ClassicUI
    E --> TouchUI
    U --> TouchUI
```

### System Architecture

```mermaid
flowchart TB
    subgraph SystemLayer["System Layer"]
        R[Renderer]
        I[Input Manager]
        A[Audio Engine]
        P["Physics<br/>(Flat Solver)"]
        Merge(( ))
        R --> Merge
        I --> Merge
        A --> Merge
        P --> Merge
        Merge --> UIS[UI System]
        Merge --> PS[Particle System]
        Merge --> C2[Camera 2D]
        Merge --> TA[Tile Animation]
    end
    Scene["Scene Layer<br/>coordinates game objects"]
    SystemLayer --> Scene
```

---

## Data Flow

### Game Loop Flow

```mermaid
flowchart TB
    Init[Init] --> GameLoop[Game Loop] --> Exit[Exit]
    GameLoop --> InputPoll["Input<br/>Poll"]
    GameLoop --> UpdateLogic["Update<br/>Logic"]
    GameLoop --> DrawRender["Draw<br/>Render"]
    UpdateLogic --> AudioGen["Audio<br/>Generate"]
    UpdateLogic --> PhysicsUpd["Physics<br/>Update"]
    UpdateLogic --> UIDraw["UI<br/>Draw"]
```

### Audio Pipeline

```
Game Code
    │
    ▼ (submitCommand)
AudioCommandQueue (Thread-Safe)
    │
    ▼ (processCommands)
AudioScheduler
    │
    ├──▶ Pulse Channel
    ├──▶ Triangle Channel
    ├──▶ Noise Channel
    └──▶ Music Sequencer
    │
    ▼ (generateSamples)
Mixer (with LUT)
    │
    ▼
AudioBackend
    ├──▶ ESP32_I2S_AudioBackend
    ├──▶ ESP32_DAC_AudioBackend
    └──▶ SDL2_AudioBackend
```

---

## Modular Compilation Flags

| Subsystem | Flag | Default |
|-----------|------|---------|
| Audio | `PIXELROOT32_ENABLE_AUDIO` | Enabled |
| Physics | `PIXELROOT32_ENABLE_PHYSICS` | Enabled |
| UI System | `PIXELROOT32_ENABLE_UI_SYSTEM` | Enabled |
| Particles | `PIXELROOT32_ENABLE_PARTICLES` | Enabled |
| Touch Input | `PIXELROOT32_ENABLE_TOUCH` | Disabled |
| Tile Animations | `PIXELROOT32_ENABLE_TILE_ANIMATIONS` | Enabled |
| Static tilemap framebuffer cache (4bpp) | `PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE` | Enabled |

---

## Related Documentation

| Subsystem | Document |
|-----------|----------|
| Audio | [Audio Subsystem](./audio-subsystem.md) |
| Physics | [Physics Subsystem](./physics-subsystem.md) |
| Touch Input | [Touch Input](./touch-input.md) |
| Tile Animation | [Tile Animation](./tile-animation.md) |
| Resolution Scaling | [Resolution Scaling](./resolution-scaling.md) |
| Memory | [Memory System](./memory-system.md) |
| API Reference | [API Index](../api/index.md) |