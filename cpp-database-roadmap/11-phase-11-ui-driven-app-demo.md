# Phase 11 — UI-Driven Application Demo

## 1. The Pain Point

Once a real Dear ImGui window exists, continuing to dump the application walkthrough to `std::cout` wastes the UI layer we just introduced.

The current approach has two problems:

- application demonstrations execute eagerly during startup instead of being explored interactively;
- the window is not the primary surface for understanding the system's behavior.

## 2. Objective & Architecture

Move the phase-oriented application walkthrough into the `views` layer so the executable becomes a composition root that prepares data and launches a persistent UI.

```text
app composition root
   |
   +--> builds AppDemoData
           |
           +--> views::run_app_demo(...)
                   |
                   +--> Dear ImGui window
                           +--> phase buttons
                           +--> phase result panel
```

This phase keeps business logic in the application/infrastructure layers while moving presentation and interaction flow into `libs/views`.

## 3. Technical Specifications

- Keep `app/main.cpp` responsible for composition and demo-data preparation only.
- Add a dedicated `views::run_app_demo` entry point for the persistent application window.
- Replace console-first phase walkthroughs with ImGui controls and result panels where practical.
- Keep the window open until the user closes it.
- Represent phase output as view data structures rather than direct `std::cout` calls.

## 4. File Structure

```text
libs/views/
├── include/views/app_demo_view.h
└── src/app_demo_view.cpp
app/
└── main.cpp
cpp-database-roadmap/
└── 11-phase-11-ui-driven-app-demo.md
scripts/
└── phase-11-script.md
```

## 5. Verification Criteria

**Pass** when:

1. Running the app opens a persistent window that stays open until manually closed.
2. Phase demonstrations are visible in the UI instead of being printed as the primary output path.
3. `app/main.cpp` prepares data and launches the view layer without owning widget logic.
4. A user can inspect phase outputs interactively through the window.
