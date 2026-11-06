#include "MakeBoardLink.h"

#include "board/FakeBoard.h"

std::unique_ptr<BoardLink> makeBoardLink()
{
    return std::make_unique<FakeBoard>();
}
