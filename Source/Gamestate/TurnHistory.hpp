#ifndef TURN_HISTORY_HPP_INCLUDED
#define TURN_HISTORY_HPP_INCLUDED

#include "Moves.hpp"
#include "Pieces.hpp"

#include <vector>
#include <map>
#include <string_view>
#include <memory> //for smart_ptr

namespace Inferior { //we'll replace this with something better
struct Turn
{
	std::size_t m_halfMove{0}; //why is this even necessary, really?
	bool m_isWhite{true}; //this is redundant, we'll store each colors' turns in seperate arrays //or not?
	Move m_Move; //this should be a reference instead of a copy, because it's already keeping a copy of the movetable
	std::string_view m_turnName; //stores the Nonstandard location-name generated for the turn. Used to set the stored pieces' m_algcoord in turn's destructor; so that their destructor shows "Turn_X_Storage" instead of their saved AlgName.

	//these are pointers to the actual piece (in the PieceArray) that was moved; if that Piece ever gets erased from it's PieceArray, accessing these will cause a segfault.
	Piece* movedPiecePtr;
	Piece* affectedPiecePtr{nullptr}; //captured piece, or castling-with piece.

	//These need to be shared across all instances of turns created with the same movedPiece/affectedPiece, because currently the engine is making a copy of each piece for each of it's moves
	Piece movedPieceState; //A copy of the moved-piece as it was on that turn. Needed for restoring enPassant/castle-flags
	std::shared_ptr<Piece> affectedPieceState{nullptr};

	//If captured by en-passant, we'd have no record of what square it was actually on
	std::map<int, Piece*> EnPassantSqs; //For the enemy. Side-to-move shouldn't have any.
//50-move-counter;
//repetition-count;

	Turn(std::size_t, const Move& M, Piece* P); //pointer to the moving piece
	~Turn();

	//remember, declaring a destructor prevents the automatic generation of copy, move, and destructor operations for the class.
	Turn(const Turn&) = default; //copy constructor
	Turn(Turn&&) = default; //move constructor
	Turn& operator=(const Turn&) = default; //copy assignment
	Turn& operator=(Turn&&) = default; //move assignment
};
}

using Turn = Inferior::Turn;

class TurnHistory //this is inferior too
{
	public:
	std::size_t turnNumber{0}; //halfmoves. decrement this when you're undoing moves (so that you can also redo them)
	std::vector<Turn> m_history; //this should be a tree, then you could branch history whenever a turn is over-written instead

	bool isRedoingMove{false}; //indicates that we're simply traversing the move-history, not overwriting it (affects CreateTurn) //"UndoLastMove()" doesn't call Piece::MakeMove(), so we don't need a variable for that
	bool isMakingTurn{false}; //set to true by CreateTurn, false by AddTurn. It prevents overlapping calls to "CreateTurn", from things like recursive "MakeMove" (castling).

	Turn& CreateTurn(std::size_t, const Move, Piece*);
	// void AddTurn(Turn& T);
	void AddTurn();

	void AltUndoLastMove(); //meant to be used by engine, more efficient, but possibly less reliable
	Turn* UndoLastMove(); //returns previous-previous turn (so moveHighlights can be restored), or nullptr.
	Turn* RedoMove(); //make the next move in m_history. Returns that turn, or nullptr.

	void Clear();
};

extern TurnHistory turnhistory;



#endif
