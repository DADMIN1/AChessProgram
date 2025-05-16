#include "GameState.hpp"

#include "GlobalVars.hpp" //isDEBUG / disableTurns / isFlipBoard
#include "SquareStorage.hpp" //AlternateTurn directly accesses the SquareStorage structs
#include "Board.hpp"
#include "Pieces.hpp" //AlternateTurn directly accesses the Piece-Arrays.

bool isGameOver{false};
bool isWhiteTurn{true};

void AlternateTurn()
{
	if ((!DEBUG_DisableTurns) && (!isGameOver))
	{
		((isWhiteTurn) ? isWhiteTurn = false : isWhiteTurn = true);
	}

//if the piecearray is empty, don't give them a turn
//we actually need to be checking squarestorage
	if (wPieceArray.empty()) { isWhiteTurn = false; }
	if (bPieceArray.empty()) { isWhiteTurn = true; }

	SquareStorage& friendStorage{((isWhiteTurn) ? wSquareStorage : bSquareStorage)};
	SquareStorage& enemyStorage{((isWhiteTurn) ? bSquareStorage : wSquareStorage)};
	std::vector<std::unique_ptr<Piece>>& friendArray{((isWhiteTurn) ? wPieceArray : bPieceArray)};
	std::vector<std::unique_ptr<Piece>>& enemyArray{((isWhiteTurn) ? bPieceArray : wPieceArray)};

	friendStorage.m_EnPassantSq.clear(); //We should also be clearing this flag for each piece

//We're not erasing the captured pieces anymore!!!

	friendStorage.ClearVolatileSets();
	enemyStorage.ClearVolatileSets();

	if (friendStorage.m_wasEmptied) { friendStorage.FillStableSets(friendArray); }
	if (enemyStorage.m_wasEmptied) { enemyStorage.FillStableSets(enemyArray); }

	RegenMovetables(enemyArray, false);
	enemyStorage.FillVolatileSets(enemyArray); //Currently not implemented correctly

	friendStorage.FillCastleDests();
	RegenMovetables(friendArray, true);
	friendStorage.FillVolatileSets(friendArray);

	if (isFlipPerTurn)
	{
		//checking turn instead of current flip status because rule changes reset to white's turn
		((isWhiteTurn) ? isFlipboard = false : isFlipboard = true);
		FlipBoard();
	}


	return;
}
