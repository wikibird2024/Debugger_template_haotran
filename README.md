# Debugger template

Author: Hao Tran. Our best debugger code and what we have learned, kept in one
place, so a new board tool starts from working code and known lessons.

```
Grader v2: safety rules ─┐
                         ├─> this template ─> copy it ─> your board's tool
Plasma: screen shell ────┘         ▲                          │
                                   └── new lessons come back ─┘
```

**Status:** runs and passes its tests against a fake board in memory. It has never
talked to real hardware.

| File | What it holds |
|---|---|
| `Docs/LESSONS.md` | Every rule, why it exists, where it came from, what proves it |
| `Docs/UI_TEXT_RULES.md` | How to write the words on screen |
| `src/board/` | One board thread, STOP ALL, reconnect, live poll, fake board |
| `src/logic/` | Rules with no threads: limits, judging a move, judging a stop |
| `src/ui/` | Shell (header, sidebar, log bar, toast, theme, safety input) and two example pages |

## Run it

Needs (Ubuntu): `cmake ninja-build qt6-base-dev`.

```sh
./test.sh                                                  # build and run all tests
./build-cmake/debug/debugger_template                      # try it with the fake board
QT_QPA_PLATFORM=offscreen ./build-cmake/debug/debugger_template --screenshot shots/
```

## Start a new tool

```
1 Copy this repo
2 BoardLink.h       your commands and status fields
3 YourLink.cpp      real serial or USB, next to FakeBoard
4 FakeBoard         same commands, plus the ways your board misbehaves
5 BoardProfile      your poll rate, retry time, limits
6 Session::stopAll  send every stop your machine needs
7 MainWindow        add your pages; copy MovePage for commands
8 MakeBoardLink.cpp return your link
```

Then read "For each new board" in `Docs/LESSONS.md`.

## Check on the bench before you trust a new tool

The tests prove the logic against a fake. These need the real machine, with the
hardware E-stop in reach:

| Check | Pass |
|---|---|
| STOP ALL while moving | Motion stops; the log says stopped |
| Esc with a list open and in a dialog, on a real keyboard | Motion stops |
| Unplug the cable while moving | Tool says link lost; write down what the board does |
| Plug it back | Reconnects; Live is off; nothing moves |
| Power off the board | Tool says link lost; window stays usable |
| Move outside the limits | Refused; nothing moves |
| Poll at full rate for an hour | No timeouts, no late replies |
