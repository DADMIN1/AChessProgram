#ifndef STORED_PIECE_STATES_HPP_INCLUDED
#define STORED_PIECE_STATES_HPP_INCLUDED

#include "Pieces.hpp"
#include "GameEvent.hpp"
//#include "BobbyFish.hpp"

#include <unordered_map>
#include <map>
#include <vector>

//needs to be a template
struct StateChange_Diff
{
	// all parts of the piece that have changed
	//

};

class PieceState
{
	Piece* currentState;
	std::unordered_map<GameEvent, StateChange_Diff> changeTrigger; //if a GameEvent causes a change in this Piece's State, it points to the stored/calculated differences. If that event is known to not affect this Piece/State, it's mapped to nullptr. If there's no entry for an Event, it needs to be calculated.
};

class PieceStateStorage
{
	std::map<Piece*, std::vector<PieceState*>> m_StateMap; //maps a piece (from/in PieceArray) to it's StateHistory

};

#endif
