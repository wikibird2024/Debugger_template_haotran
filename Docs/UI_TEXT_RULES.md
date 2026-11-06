# UI text rules

Author: Hao Tran. Rules for every word on screen: labels, tooltips, messages and
the log. Taken from `plasma_debugger`. Keep the rules and make the word list from
your own machine's words.

## Rules

| Rule | Example | From |
|---|---|---|
| As short as possible without losing meaning | "Line status", not "Get HW status (0x30): what the line PC reads" | KDE, GNOME |
| One word per idea, everywhere | Word list below | NN/g heuristic 4 |
| Buttons start with a verb | "Start move", not "Move"; a state word as a choice is fine | KDE, GNOME |
| Sentence case everywhere | "Read version" | Own choice, see below |
| Capitals only for STOP ALL | "Failed", not "FAILED" | Own choice |
| `…` only when more input comes before the action runs | "Browse…" | KDE, GNOME |
| No `!`, no contractions | "Cannot open the file." | Qt wiki |
| Plurals are right | "1 frame", "2 frames", not "frame(s)" | Qt wiki |
| Errors say what failed, why and what to do | "Not connected. Press Connect." | NN/g errors |
| Opcodes and part numbers are for diagnosis | Tooltips and the log, not buttons | NN/g errors, heuristic 2 |
| Confirm actions that are hard to undo, and say what happens | Update firmware | NN/g heuristic 5 |
| Color is never the only signal | Off dot hollow, fault text beside the red | NN/g errors |
| A value on its own starts with a capital | "Idle", "None" | Own choice |
| Long notes go in a tooltip, not an intro paragraph | | NN/g heuristic 8 |

**Why sentence case:** KDE, GNOME and the Qt wiki want title case on buttons. Our
tools were already consistent in sentence case, and it reads the same as the
labels beside it. This is a deliberate difference.

## Word list

These fit every tool. Add your machine's words below them.

| Use | Do not use |
|---|---|
| board | device, mainboard |
| page | tab |
| update firmware | upgrade |
| Live | Live status, tick |
| `0x20` (short hex, capital digits: `0x0A`) | `0x0020`, `0x0a` |

## Page pattern

```
Page title
Connection hint (shared, only when the page cannot show anything yet)
One short line: what this page does
Boxes: most used first
Log bar: every result, on every page
```

A result goes to the log bar, not to a label on the page. An error or warning
also shows a toast. An error says "Error:" in the log, because the log has no color.

## Sources

- [KDE HIG: Text and labels](https://develop.kde.org/hig/text_and_labels/)
- [GNOME HIG: Writing style](https://developer.gnome.org/hig/guidelines/writing-style.html)
- [NN/g: Error-message guidelines](https://www.nngroup.com/articles/error-message-guidelines/)
- [NN/g: 10 usability heuristics](https://www.nngroup.com/articles/ten-usability-heuristics/)
- [Qt wiki: UI text conventions](https://wiki.qt.io/UI_Text_Conventions)

These sources were cited by the plasma rules and were not read again here. ISA-101
is a paid standard and was not read.
