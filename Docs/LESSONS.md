# Lessons

Author: Hao Tran. What our debugger tools have taught us, so the next tool starts
from it. Add a line when a tool or a bench session teaches something new.

**How far this is checked:** the template runs only against a fake board. A 🟢
below means a test in this repo proves the rule against that fake. No rule here
has been checked on a real board through this template.

**From** says where the lesson came from:

| Name | Path |
|---|---|
| Grader v2 | `/mnt/storage/HoangCuong/dekit_grader_v2-ORIGIN/dekit_grader_v2_debugger` |
| Plasma | `/mnt/storage/PhienNguyen/Plasma/plasma-fw-debugger` |
| Template | Found while building this repo |

## Stop and safety: in the code

| | Rule | Why | From | Test |
|---|---|---|---|---|
| 🟢 | STOP ALL drops the queue and goes next | A stop must never wait behind moves | Grader v2 | `test_worker`: stopThrowsAway... |
| 🟢 | A stop waits for at most one timeout | It cannot cut short the exchange in progress | Grader v2, Template | `test_worker`: stopWaitsForAtMostOneTimeout |
| 🟢 | Confirm the stop from live status | The board can answer OK and keep moving | Template | `test_session`: boardAnswersOkButKeepsMoving |
| 🟢 | Use a typed stop result | Grader v2 searches the reply text, which breaks when wording changes | Template | `test_logic`: judgeStop_everyOutcome |
| 🟢 | A stop not sent says so | Grader v2 returns silently when not connected or updating | Template | `test_session`: notConnected, blocked |
| 🟢 | Say STOP ALL is not the E-stop | It needs the link and working firmware; failure text points to the hardware E-stop | Template | `test_session`: noReply |
| 🟢 | Esc works everywhere | Measured offscreen: a window shortcut misses Esc in a modal dialog, which just closes. It did fire with a list open. Plasma's own test expects the dialog to close with no stop | Plasma, Template | `test_ui`: esc_... |
| 🟢 | No move without a limit check | Firmware may have no position limits | Grader v2 | `test_session`: outsideLimits |
| 🟢 | Waiting moves die on link loss | A move must not run by surprise after reconnect | Template | `test_session`: waitingMoveDoesNotRunAfterwards |
| 🟢 | Moves wait until the stop is confirmed | A new move would make the check wrong | Template | `test_session`: refusedWhileStopIsBeingConfirmed |

## Reading the board: in the code

| | Rule | Why | From | Test |
|---|---|---|---|---|
| 🟢 | Judge a command from status, not the reply | Firmware says OK when it ignores a command | Grader v2 | `test_ui`: okButIgnored |
| 🟢 | Done only after busy was seen | Not busy at the start proves nothing | Grader v2 | `test_logic`: moveWatch_... |
| 🟢 | Refuse a judged command with Live off | Nothing would ever judge it | Grader v2 | `test_ui`: refusedWithLiveOff |
| 🟢 | Live off shows "unknown" | An old value looks like a current one | Template | `test_ui`: liveOff_doesNotShowAnOldValue |
| 🟢 | Reconnect leaves Live off | Nothing restarts by itself | Plasma | `test_session`: liveStaysOff |
| 🟢 | Never stack reconnect attempts | Opening a port can take seconds | Plasma | `test_session`: neverStacksAttempts |
| 🟢 | A dropped retry must not stop reconnect | The template forgot this: a slow job that lost the link dropped the retry, and the tool stayed on Reconnecting with the board back | Plasma, Template | `test_session`: retryDroppedByLinkLoss |
| 🟢 | One read waits at a time | A slow link must not fill the queue | Plasma, Grader v2 | `test_session`: onlyOneReadWaits |

## Threads and waiting: in the code

| | Rule | Why | From | Test |
|---|---|---|---|---|
| 🟢 | One thread talks to the board, one job at a time | The window never freezes, and frames never mix | Both | `test_worker`: jobsRunInOrder |
| 🟢 | Dropped jobs never answer | A waiting flag stays stuck and the page locks | Grader v2 | `test_ui`: stopAllFreesTheButton |
| 🟢 | One signal resets all waiting (`workEnded`) | Grader v2 needs two signals per page, and one is easy to forget | Template | `test_session`: signalsEveryPageToReset |
| 🟢 | Ignore replies from before a stop | A late reply would restart the waiting | Template | `test_session`: replyFromBeforeStop |
| 🟢 | `done` only while its page lives | A closed page must not be called | Grader v2 | `test_worker`: doneDoesNotRun... |

## Screen: in the code

| | Rule | Why | From | Test |
|---|---|---|---|---|
| 🟢 | The wheel only changes a clicked box | Scrolling past a step box turned 1 mm into 10 mm | Plasma | `test_ui`: wheel_... |
| 🟢 | Guards sit on the whole application | A page or a later box cannot forget them | Template | `test_ui`: guardCoversABoxMadeLater |
| 🟢 | Color means state only, never alone | Off is hollow, a fault has text | Plasma | `test_ui`: faultShowsAsText |
| 🟢 | All colors come from one table | Colors written in pages break dark mode | Plasma, Template | `no_hard_coded_colors` |
| 🟢 | `--screenshot` works offscreen | Check a UI change with no display | Both | `test_ui`: screenshots_... |
| 🟢 | `--screenshot` uses the fake board only | That run connects and turns Live on by itself | Template | `screenshot_run` |
| 🟢 | `--screenshot` leaves settings alone | It must not change the user's setup | Grader v2 | `screenshot_run` |

## For each new board: not in the template

These depend on the firmware. Check each one when writing a real `BoardLink`.

| Lesson | Example | From |
|---|---|---|
| Does the firmware need a keep-alive? | Grader v2 sends 0x17 every 20 s: after 60 s with no frame the firmware turns lights off and power-cycles the cameras | Grader v2 |
| Does the firmware check ids? | Grader v2 refuses motor ids above 8: three commands do not check and would corrupt memory | Grader v2 |
| Does the firmware remember commands? | Grader v2 sends 0x34 only when idle: a start sent mid-clamp runs later, unwatched | Grader v2 |
| What must be read before a write? | Calibration Apply only after a good Read, or zeros reach the driver | Grader v2 |
| Which stops does the machine need? | Grader v2 sends three: speed test, clamp, flip | Grader v2 |
| Can anything be resent safely? | Grader v2 only resends firmware update pages: same page, same data | Grader v2 |
| Is any long job one exchange? | Split it, or call `setStopBlocked` so the screen says STOP is not active | Grader v2, Plasma |
| What runs on if the PC goes away? | A disconnect does not stop the board; only firmware can act on a lost link | Template |
| Where is the wire format written? | Firmware is the truth. Grader v2 changes firmware, tool, simulator and doc together | Grader v2 |
| Are late replies possible? | Grader v2 skips valid frames with another opcode and counts them | Grader v2 |
| What are this link's real numbers? | Do not copy poll or retry rates: Grader v2 runs at 38400 baud | Plasma brief |
| Are moves shorter than one poll? | They look like "never started": poll faster, or read a move counter | Template |

## Tools and setup

| Lesson | Detail | From |
|---|---|---|
| Keep `QT_ACCESSIBILITY=0` | Qt's accessibility bridge crashed Grader v2 on the bench (not reproduced here) | Grader v2 |
| Add X11 as a Wayland fallback | Wayland alone aborts when its display is gone | Plasma |
| Use the Fusion style | The sheet then looks the same on every desktop | Plasma |
| A worker link is made on its own thread | `QSerialPort` belongs to the thread that made it | Grader v2 |
| Link with `Q_INIT_RESOURCE` | A static library does not register its resources by itself | Template |
| Comments in `theme.qss` must not use `@` | The check for unfilled tokens fails on it | Template |
| In tests, wait until a move is accepted before stopping | STOP ALL drops a move still queued, so nothing was moving to stop | Template |
| Qt's test mode moves settings to `~/.qttest` | A check watching `XDG_CONFIG_HOME` saw nothing. Run such checks with a scratch `HOME`; `QSettings::setPath` is what actually keeps writes away | Template |
| Break a rule on purpose to prove its test | The first settings test passed while the run wrote settings | Template |

## Ideas not taken yet

| Idea | Why not yet | From |
|---|---|---|
| Python board simulator on a pty | Needs real firmware to copy. Stronger than the fake once a tool has one | Grader v2 |
| Packet trace page | Needs a real frame format | Plasma, Grader v2 |
| Pin map page | Board-specific | Plasma, Grader v2 |
| Input change list | Add with the first real board | Plasma |
| Warning strip when a page drops a limit | No page here drops one | Plasma |
