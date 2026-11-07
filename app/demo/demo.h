#pragma once

class QObject;

// The internal demo (agent.md, "Demo"): Emi Hartmann's practice from the shared demo world
// (demo/world.json, section `dojo`) and the landing-page screenshots. Built only with
// -DMITORI_DEMO=ON, never shipped; demo/start.sh runs it.
//
//   demo/start.sh ──▶ scratch folders, no videos, no network sources
//        │
//        ├─ Demo::prepare()   settings (welcomed, style, language), the data once, the course calendar
//        ├─ the window
//        └─ Demo::shoot()     with SHOT_PLAN and SHOT_DIR: each shot's steps, <name>.png, quit
namespace Demo {

// Before the language and the window: from MITORI_DEMO_WORLD, DEMO_LANG and DEMO_STYLE.
void prepare();

// After the window opened: the shots of SHOT_PLAN into SHOT_DIR, then the app quits.
// Without a plan nothing happens (the demo to look at).
void shoot(QObject *window);

} // namespace Demo
