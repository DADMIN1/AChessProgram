#ifndef FISH_ENGINE_HPP_INCLUDED
#define FISH_ENGINE_HPP_INCLUDED

#include "ZobristHash.hpp"

#include <limits> //std::numeric_limits
#include <string>
#include <map>

//zobrist hashtable is global

//This was the prototype/test for the engine. Use BobbyFish instead.
namespace Fish {

extern bool isRunning; //if there's an instance of the engine already spawned
extern bool isActive; // false if the spawned instance is not actively calculating; finished or paused; waiting for input before exiting or continuing.

extern std::map<std::size_t, std::string> pieceHeaders; // like: "White Bishop {d1}:". Key is the matching index for PieceArray/savedMovetables (SavedVolatiles)

extern int maxSearchDepth;
extern int targetDepth;
extern int currentDepth;
extern int movesMadeCount;
extern int finalPositionsCount;
extern int transpositionCount;
extern int initEval;

extern bool displayRelativeEval; //displays evals as the difference between initial/final-material instead of the material balance of the final position

constexpr int NumLimitMax(bool isWturn) { return ((isWturn)? std::numeric_limits<int>::max() : std::numeric_limits<int>::min()); }
constexpr int NumLimitMin(bool isWturn) { return ((isWturn)? std::numeric_limits<int>::min() : std::numeric_limits<int>::max()); }

void Search_root();
void EngineTest();

}

#endif
