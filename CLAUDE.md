# CLAUDE.md

## What this is

Our best debugger code and lessons, kept for later tools. Qt6/C++, Linux. Owner:
Hao Tran. It holds working code (`src/`) and written experience (`Docs/LESSONS.md`).
It runs against `FakeBoard` only and has never talked to real hardware: never call
anything here bench-checked.

## Build and test

```sh
./test.sh                                   # configure, build, ctest; no board, no display
cmake --preset asan && cmake --build --preset asan && ctest --preset asan
QT_QPA_PLATFORM=offscreen ./build-cmake/debug/debugger_template --screenshot shots/
```

Done means: builds with zero warnings (`-Werror`), `./test.sh` passes, and for a UI
change the screenshots were looked at. Format with `.clang-format`.

## Layers

```
src/ui      widgets; talks only to Session
src/board   Session -> BoardWorker -> BoardLink (FakeBoard or a real link); no widgets
src/logic   rules with no threads or widgets
```

## Rules that keep it safe

Never weaken these to save time; ask first. Why each exists: `Docs/LESSONS.md`.

- Every board exchange goes through `BoardWorker`. The GUI thread never touches a link.
- `Session::stopAll` ends loops, drops the queue, sends the stop next, then confirms
  from live status. A new loop must end on `stopAllRequested`.
- Every waiting flag on a page resets on `Session::workEnded`. Dropped jobs never answer.
- Moves go through `Session::moveTo`, which checks the limits. Never call
  `BoardLink::moveTo` from a page.
- A `BoardLink` never resends a command and never reopens by itself, and every call
  ends within its timeout. A long transfer is one job per exchange, or it calls
  `Session::setStopBlocked`.
- Judge a command from live status, not its reply. Refuse it while Live is off and
  end it when Live goes off.
- After a reconnect, Live stays off and nothing resumes.
- `--screenshot` always uses `FakeBoard`: it connects and turns Live on by itself.
- STOP ALL stays visible and enabled on every page. It is a software stop, not the
  E-stop, so its failure messages name the hardware E-stop.
- `SafetyInputFilter` stays on the application: Esc is STOP ALL everywhere, and the
  wheel only changes a clicked box.

## Conventions

- Plain names, units in names (`timeoutMs`), booleans `isX` / `hasX`. C++ uses Qt's
  camelCase with `m_` members; `kName` for constants.
- UI text follows `Docs/UI_TEXT_RULES.md`. Pages are built in code, with no `.ui` files.
- Colors come only from `Theme`. `tests/check_no_hard_coded_colors.sh` enforces it.
- New experience goes into `Docs/LESSONS.md` with its source. Mark what is proven
  by a test and what was seen on a bench; do not blur the two.
