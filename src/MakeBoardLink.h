#ifndef MAKEBOARDLINK_H
#define MAKEBOARDLINK_H

#include <memory>

#include "board/BoardLink.h"

/* The link this tool uses. A new tool returns its real link here. It runs on the
   worker thread, so a link built on QSerialPort lives there. --screenshot never
   calls it: that run connects and turns Live on by itself. */
std::unique_ptr<BoardLink> makeBoardLink();

#endif // MAKEBOARDLINK_H
