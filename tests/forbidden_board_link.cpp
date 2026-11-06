#include <cstdio>
#include <cstdlib>

#include "MakeBoardLink.h"

/* Stands in for a real board link in check_screenshot_run.sh: if main.cpp ever
   makes it, the program dies and the check fails. */
std::unique_ptr<BoardLink> makeBoardLink()
{
    std::fputs("The real board link was made.\n", stderr);
    std::abort();
}
