#pragma once
#include "EngineTypes.h"
#include <memory>
namespace rastro
{
std::unique_ptr<Sample> decodeMp3(const void *, size_t);
}
