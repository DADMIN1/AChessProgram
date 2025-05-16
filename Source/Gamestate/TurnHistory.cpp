#include "TurnHistory.hpp"
#include "GlobalVars.hpp" //isDEBUG and all that
#include "GameState.hpp" //for AlternateTurn
#include "Board.hpp" //"UndoLastMove" accesses/modifies squaretable
#include "SquareStorage.hpp"

#include <iostream>

Turn::Turn(std::size_t moveNum, const Move& M, Piece* P)
	: m_halfMove{moveNum}, m_Move{M}, m_turnName{CreateLocationName("Turn_#" + std::to_string(moveNum))}, movedPiecePtr{P}, movedPieceState{*P}
	//attempting copy the shared_ptr in the initializer list will cause a segfault (creates a new ptr to the resource instead of copying, which will cause a double-deallocation)
{
	if (isDEBUG) { std::cout << "Constructing " << m_turnName << '\n'; }

	m_isWhite = P->isWhite;
	affectedPiecePtr = nullptr;
	return;
}

Turn::~Turn()
{
	 /* changing the stored piece-states' algcoord to match the turn#, so that their destructors indicate that they were nonstandard-location pieces, not actual pieces
	 otherwise their algcoord would either be blank (if they were created manually from some kind of default constructor), or an actual algcoord from the piece they were copied from.
	 Either way it makes the destructor message confusing.
	*/
	// We can only do this because the lifetime of the piecestates is dependent on this object alone. If they're changed to be (potentially) stored elsewhere, we can't do this here, because they may not be destroyed, and then their algCoord will be invalid (and it might get copied elsewhere)
	if (isDEBUG)
	{
		std::cout << "Destroying " << m_turnName << '\n';
		std::string_view nameForPieces = CreateLocationName(std::string{m_turnName} + " SavedState");

		movedPieceState.m_AlgCoord = nameForPieces;
		if (affectedPieceState.get() != nullptr) {affectedPieceState->m_AlgCoord = nameForPieces;}
	}
}

// void TurnHistory::AddTurn(Turn& T)
void TurnHistory::AddTurn()
{
	//std::cout << "Adding turn#: " << T.m_halfMove << '\n';
	//std::cout << T.m_halfMove << ": " << T.movedPiecePtr->GetName() << T.m_Move.GetNotation() << '\n';

	/* if (turnNumber != T.m_halfMove)
	{
		std::cerr << "Added Turn mismatches current turnNumber!\n";
	} */

	turnNumber += 1;
	isRedoingMove = false;
	isMakingTurn = false;
	return;
}

Turn& TurnHistory::CreateTurn(std::size_t num, const Move withMove, Piece* movedPiece)
{
	if (num < m_history.size())
	{
		if ((isRedoingMove) || (isMakingTurn))
			return m_history[num];
		else if (!isRedoingMove)
		{
			if(isDEBUG)
			std::cout << "Overwriting movehistory!\n";
			while ((!m_history.empty()) && (m_history.back().m_halfMove >= num))
				{
					m_history.pop_back();
				}
		}
	}

	isMakingTurn = true;

	if (num != turnNumber)
	{ std::cerr << "Warning: creating a Turn with a number that doesn't match it's index\n"; }

	Turn& T = m_history.emplace_back(num, withMove, movedPiece);
	T.EnPassantSqs = ((T.m_isWhite)? bSquareStorage.m_EnPassantSq : wSquareStorage.m_EnPassantSq);

	// return m_history.emplace_back(num, withMove, movedPiece);
	return T;
}


//RedoMove doesn't necessarily need valid SquareStorage-entries, but the player does, if they're trying to select a different move.
	//Castling is the only move-generation that relies on SquareStorage.
void TurnHistory::AltUndoLastMove()
{
	//AddTurn increments the move#, so this will be > 0 after the engine makes a move
	if (turnNumber == 0) //no moves made yet
		return;

	Turn& lastTurn = m_history[turnNumber-1];
	SquareStorage& friendStorage{((lastTurn.m_isWhite) ? wSquareStorage : bSquareStorage)};
	SquareStorage& enemyStorage{((lastTurn.m_isWhite) ? bSquareStorage : wSquareStorage)};

	Piece* MovedPiece = lastTurn.movedPiecePtr;
	SquareTable[lastTurn.m_Move.targetSqID].occupyingPiece = nullptr;
	SquareTable[lastTurn.m_Move.targetSqID].isOccupied = false;
	friendStorage.RemovePiece(MovedPiece);

	if (lastTurn.affectedPiecePtr != nullptr)
	{
		Piece* AffectedPiece = lastTurn.affectedPiecePtr;  //affectedPiecePtr points to the actual piece IN THE PIECEARRAY. affectedPieceState is a pointer to a dynamically-allocated copy of that piece as it was before the move. We cannot simply use the pointer in affectedPieceState because it's completely seperate from the one in the array, and it'll be deallocated.
		bool isFriendly{(AffectedPiece->isWhite == MovedPiece->isWhite)};

		if (isFriendly) //if the last move was castling, for example
		{
			SquareTable[AffectedPiece->m_SqID].isOccupied = false;
			SquareTable[AffectedPiece->m_SqID].occupyingPiece = nullptr;
			friendStorage.RemovePiece(AffectedPiece);
		}
		//if the affected piece was not friendly, that means it was captured, so we don't need to remove it from enemystorage.

		*AffectedPiece = *lastTurn.affectedPieceState; //copying the stored Piece-state into the actual Piece in xPieceArray
		SquareTable[AffectedPiece->m_SqID].isOccupied = true;
		SquareTable[AffectedPiece->m_SqID].occupyingPiece = AffectedPiece; //it should be okay to set these, because it's pointing to the piece in xPieceArray
		((isFriendly)? friendStorage : enemyStorage).AddPiece(AffectedPiece); //ADDPIECE DOES NOT CREATE ENTRIES FOR CASTLEDESTS!!!!
	}

	// *lastTurn.movedPiecePtr = lastTurn.movedPieceState;
	*MovedPiece = lastTurn.movedPieceState;
	SquareTable[lastTurn.m_Move.currentSqID].occupyingPiece = lastTurn.movedPiecePtr;
	SquareTable[lastTurn.m_Move.currentSqID].isOccupied = true;
	friendStorage.AddPiece(MovedPiece); //ADDPIECE DOES NOT CREATE ENTRIES FOR CASTLEDESTS!!!!

	//for some reason removePiece didn't seem to take care of this, so we have to clear it manually?
	friendStorage.m_EnPassantSq.clear(); //squarestorage removepiece will actually take care of this, so will AlternateTurn.

	enemyStorage.m_EnPassantSq = lastTurn.EnPassantSqs;

	//isWhiteTurn = !lastTurn.m_isWhite;
	//AlternateTurn(); //regenerate Movetables and refill SquareStorage

	//We're going to copy/restore volatile sets instead of recalculating them for better efficiency
	//WE HAVE TO CALL "FILLCASTLEDESTS" IF WE'RE SKIPPING ALTERNATETURN!!!
	isWhiteTurn = lastTurn.m_isWhite;

	assert((turnNumber > 0) && "AltUndoMove would result in a negative move number!");
	turnNumber -= 1;
	return;
}


Turn* TurnHistory::UndoLastMove()
{
	if (turnNumber == 0) //no moves made yet
		return nullptr;

	Turn& lastTurn = m_history[turnNumber-1];
	//isWhiteTurn = lastTurn.m_isWhite; //don't suddenly change the side-to-move without updating movetables/squarestorage

	wSquareStorage.ClearAll();
	bSquareStorage.ClearAll();
	//This will cause AlternateTurn to refill everything except for EnPassant

	SquareTable[lastTurn.movedPiecePtr->m_SqID].isOccupied = false;
	SquareTable[lastTurn.movedPiecePtr->m_SqID].occupyingPiece = nullptr;
	*lastTurn.movedPiecePtr = lastTurn.movedPieceState;

	if (lastTurn.affectedPiecePtr != nullptr)
	{
		SquareTable[lastTurn.affectedPiecePtr->m_SqID].isOccupied = false;
		SquareTable[lastTurn.affectedPiecePtr->m_SqID].occupyingPiece = nullptr;
		*lastTurn.affectedPiecePtr = *lastTurn.affectedPieceState;

		SquareTable[lastTurn.affectedPiecePtr->m_SqID].isOccupied = true;
		SquareTable[lastTurn.affectedPiecePtr->m_SqID].occupyingPiece = lastTurn.affectedPiecePtr;
	}

	SquareTable[lastTurn.movedPiecePtr->m_SqID].isOccupied = true;
	SquareTable[lastTurn.movedPiecePtr->m_SqID].occupyingPiece = lastTurn.movedPiecePtr;

	((lastTurn.m_isWhite)? bSquareStorage : wSquareStorage).m_EnPassantSq = lastTurn.EnPassantSqs;

	isWhiteTurn = !lastTurn.m_isWhite;
	AlternateTurn(); //refill SquareStorage and regenerate Movetables
	turnNumber -= 1;
	if (turnNumber == 0) //no moves made yet
		return nullptr;

	return &m_history[turnNumber-1];
}

Turn* TurnHistory::RedoMove()
{
	if ((turnNumber >= m_history.size()) || (turnNumber > m_history.back().m_halfMove))
		return nullptr;

	isRedoingMove = true;

	Turn& nextTurn = m_history[turnNumber];
	isWhiteTurn = nextTurn.m_isWhite; //need to alternateTurn here instead?

	//if we simply restore the turn::pieces to their previous state, then squareStorage is invalid.
		//Actually SquareStorage should be valid, because we had to UndoLastMove to get here.

	//The problem is that MakeMove searches SquareStorage for pointers, but some maps aren't properly restored by UndoLastMove?
	//Restore Piece-states here? If UndoLastMove works properly, the Pieces must be in their previous states anyway. Just skip the SquareStorage checks? But then SquareStorage will be invalid for the next move; you'll have to clear/refill anyway

	nextTurn.movedPiecePtr->MakeMove(nextTurn.m_Move);

	AlternateTurn();
	//turnNumber += 1; //AddTurn (called in MakeMove) already increments turn number

	isRedoingMove = false; //this is redundant, it's already reset by AddTurn (called in MakeMove)
	//return &m_history[turnNumber-1];
	return &nextTurn;
}

void TurnHistory::Clear()
{
	if (isDEBUG)
	{ std::cout << "Clearing turnhistory!\n"; }

	turnNumber = 0;
	isRedoingMove = false;
	isMakingTurn = false;
	m_history.clear();
	m_history.reserve(200); //it's very important that you prevent the vector from ever resizing

	return;
}

TurnHistory turnhistory{};



