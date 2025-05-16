#include "SquareStorage.hpp"

#include "Pieces.hpp"
#include "Board.hpp" //Printall() accesses Squaretable's algebraicName, ClearBoard() obviously calls clear
#include "TurnHistory.hpp" //we want ClearBoard to clear the gameHistory
#include "Setups.hpp" //for FENmap

#include <iostream>
#include <list>

SquareStorage wSquareStorage(true, wPieceArray);
SquareStorage bSquareStorage(false, bPieceArray);

//FROM FISH//
/*while iterating through the whole legal-move-list, if the move belongs to the same piece as the previous move,
we can reuse the piecename instead of calling findPiece() again.*/
bool isPreviousPiece{false};
int lastSqID{-2};
std::string lastPieceName{"bTestPiece"};

std::list<Move> legalMoveList{};	 //must be list for random-selection to work?

//can't work because nothing adds moves to legalMoveList
void PrintLegalMovelist()
{
	//Printing all moves in legalmovelist and their proclivity
	std::cout << "\nLegal Movelist: \n";

	//resetting these variables
	lastSqID = -2; //Reset this for the first piece
	lastPieceName = "lastPieceName";
	isPreviousPiece = false; //These variables are all for printing out the moves

	for (Move& M : legalMoveList)
	{
		if (M.currentSqID == lastSqID)
		{
			isPreviousPiece = true;
			std::cout << " , "; //spacing between moves
		}
		else
		{
			isPreviousPiece = false;
			std::cout << '\n'; //we have to do this here instead of at the end
		}
		//If it's the same piece, we don't need to update these
		//This should always be true for the first piece
		if (isPreviousPiece == false)
		{
			lastPieceName = ((wSquareStorage.m_Occupied.contains(M.currentSqID) ? wSquareStorage.m_Occupied[M.currentSqID]->GetName() : bSquareStorage.m_Occupied[M.currentSqID]->GetName()));
			lastSqID = M.currentSqID;
		}

		if (M.isDestOccupied)
			std::cout << lastPieceName << '(' << M.currentAlgCoord << ')' << 'x' << M.targetAlgCoord; //Print 'x' for captures
		else
			std::cout << lastPieceName << '(' << M.currentAlgCoord << ')' << "->" << M.targetAlgCoord; //Otherwise -> for normal moves

		//We actually have to do the newline when isPreviousPiece==false TWICE! (otherwise it always prints a newline after a piece's first move)
	}
	std::cout << std::noshowpos; //Otherwise every number printed afterwards will show positive
	std::cout << '\n';
	std::cout << '\n';

	return;
}

std::string SquareStorage::GetHash(bool isSideToMove)
{
	//because this is called by the engine after MakeMove, but before AlternateTurn, it should actually be flipped.
	//this could create false hash-collisions if we try to check/generate hashes at different stages in the move-making process
	std::string positionHash{((isSideToMove)? "TRUE " : "FALSE ")};

	for (auto& P : m_Occupied)
	{
		positionHash.append(std::to_string(P.first));
		positionHash.push_back(currentSetup.GetFENchar(P.second->m_PieceType));
	}

	return positionHash;
}

int TallyMaterial()
{
	//This function will be used by the search function

		// default centipawn value is now set by PlacePiece function
	int wTotal{0};
	//std::cout << '\n';
	for (std::unique_ptr<Piece>& w : wPieceArray) //must use references to actually set the values(also much more efficient than making copies)
	{
		//FISH_setPieceValue(w); //Re-evaluate with positional considerations
		if (!w->isCaptured)
			wTotal += w->centipawnValue; //If it does work, you can merge the two lines
	}
	if (isDEBUG)
		std::cout << "\nWhite total material = " << wTotal << '\n';

	int bTotal{0};
	for (std::unique_ptr<Piece>& b : bPieceArray)
	{
		//FISH_setPieceValue(b);
		if (!b->isCaptured)
			bTotal += b->centipawnValue;
	}
	if (isDEBUG)
		std::cout << "Black total material = " << bTotal << '\n';

	return (wTotal + bTotal);
}

void CentipawnValueTest()
{
	std::cout << "Testing if centipawn value sticks: \n";
	std::cout << "White array: \n";
	for (std::unique_ptr<Piece>& v : wPieceArray)
		std::cout << v->GetName() << " = " << v->centipawnValue << '\n';

	std::cout << "Black array: \n";
	for (std::unique_ptr<Piece>& b : bPieceArray)
		std::cout << b->GetName() << " = " << b->centipawnValue << '\n';

	return;
}

void ClearSquareStorage()
{
	wSquareStorage.ClearAll();
	bSquareStorage.ClearAll();
	return;
}

void ClearBoard()
{
	turnhistory.Clear();
	ClearSquareStorage();
	wPieceArray.clear();
	bPieceArray.clear();
	SquareTable.clear();

	return;
}

void SquareStorage::ClearAll()
{
	m_Occupied.clear();
	m_Royal.clear();
	m_canCastle.clear();
	m_canCastleWith.clear();
	CastleDestR.clear();
	CastleDestL.clear();
	m_EnPassantSq.clear();

	Attacking.clear();
	Captures.clear();
	//Defended.clear();
	m_givingCheck.clear();

	m_wasEmptied = true;

	return;
}

void SquareStorage::ClearVolatileSets()
{
	Attacking.clear();
	Captures.clear();
	//Defended.clear();
	m_givingCheck.clear();

	return;
}

void SquareStorage::RemovePiece(Piece* P)
{
	int SqID{ P->m_SqID };
	m_Occupied.erase(SqID);

	if (P->isRoyal) { m_Royal.erase(SqID); }
	if (P->canCastle) { m_canCastle.erase(SqID); }

	if (P->canCastleWith)
	{
		//Even if the squareIDs are invalid, nothing bad will happen.
		m_canCastleWith.erase(SqID);
		CastleDestR.erase(SqID+numRows);
		CastleDestL.erase(SqID-numRows);
	}

	if (P->isEnpassantPiece)
	{
		if ((m_EnPassantSq.size() < 2))
		{m_EnPassantSq.clear();}
		else
		{
			std::cerr << "Warning: EnPassant is storing multiple values!\n";
			for (auto& [key, ptr] : m_EnPassantSq) //we have to search manually because we don't know the EnPassant SqID
			{
				std::cerr << key;
				if (ptr == P) //if the Piece-pointer matches
				{ std::cerr << "[match]"; m_EnPassantSq.erase(key); }
				std::cerr << ' ';
			}
			std::cerr << '\n';
		}
	}

	return;
}

void SquareStorage::AddPiece(Piece* P)
{
	int SqID{ P->m_SqID };

	m_Occupied.insert_or_assign(SqID, P);
	if (P->isRoyal) { m_Royal.insert_or_assign(SqID, P); }
	if (P->canCastle) { m_canCastle.insert_or_assign(SqID, P); }
	if (P->canCastleWith) { m_canCastleWith.insert_or_assign(SqID, P); } //if you were to insert a key that already exists, it will simply fail, and not update the value.

//if P->isEnpassantPiece, find a way to re-insert the EnPassant-Squares
	if (P->isEnpassantPiece) {
		//assert((P->GetName_Str() == std::string{"Pawn"}) && "non-pawn marked as enPassantPiece");
		assert((P->GetName() == "Pawn") && "non-pawn marked as enPassantPiece");
		int enpassant_sq = (P->isWhite? SqID+1: SqID-1);
		m_EnPassantSq.insert_or_assign(enpassant_sq, P);
	}

	return;
}

void SquareStorage::FillStableSets(std::vector<std::unique_ptr<Piece>>& friendlyArray)
{
	for (std::unique_ptr<Piece>& P : friendlyArray)
	{
		if (P->isCaptured) {continue;}

		AddPiece(P.get());
//can't generate castleDest here because we need the opponent's Volatile sets filled first
	}

	m_wasEmptied = false;
	return;
}

//This is supposed to be the only place where VolatileSets are filled, but it's much more convenient to fill them as move's legality is being checked.
void SquareStorage::FillVolatileSets(std::vector<std::unique_ptr<Piece>>& friendlyArray)
{ //This assumes you have all the stable-sets filled!

	if (m_wasEmptied) { std::cerr << "Can't fill volatile sets while stable sets are invalid!\n"; return; }

//Iterate through movesets. Legality should be determined elsewhere.
	for (std::unique_ptr<Piece>& P : friendlyArray)
	{
		if (P->isCaptured) {continue;}

		for (Move& M : P->Movetable)
		{
			if (!M.isLegal)
			{
				if ((M.isForcedCapture) && (!M.isDestOccupied)) { Attacking.emplace(M.targetSqID, P.get()); }

				continue;
			}

			if (M.isCapture)
			{
				Captures.emplace(M.targetSqID, P.get());
				if (M.isCapturingRoyal) { m_givingCheck.insert_or_assign(M.targetSqID,P.get()); }
			}
			else if (!M.isPacifist) { Attacking.emplace(M.targetSqID, P.get()); }
		}
	}

	return;
}


void SquareStorage::FillCastleDests()
{
	for (auto& M : m_canCastleWith)
	{
		if (SquareTable[M.first].edgeDist.Left > 0)
		{
			int targetID{ (M.first - numRows) };
			 if ((!m_Occupied.contains(targetID)) || (m_canCastle.contains(targetID)))
			{
				{ CastleDestL.insert_or_assign(targetID,M.second); }
			}
		}

		if (SquareTable[M.first].edgeDist.Right > 0)
		{
			int targetID{ (M.first + numRows) };
			if ((!m_Occupied.contains(targetID)) || (m_canCastle.contains(targetID)))
			{
				{ CastleDestR.insert_or_assign(targetID,M.second); }
			}
		}
	}

	return;
}

void SquareStorage::PrintAll(bool isWhiteStorage)
	{
		//Don't need this in here, put it elsewhere.
		((isWhiteStorage) ? (std::cout<<"\nWhiteSqStorage: \n") : (std::cout<<"\nBlackSqStorage: \n"));

		//should print "empty" if the container is empty
		//should these iterator loops use refs instead?
		if (m_wasEmptied)
		{
			std::cerr << "SquareStorage's StableLists hasn't been refilled! \n";
			//return;
		}

		std::cout << "Royal SquareList =  ";
		for (auto& l : m_Royal)
		{
			std::cout << l.first << "(" << SquareTable[(l.first)].m_algCoord << "), ";
		}
		std::cout << '\n';

		std::cout << "Occupied SquareList =  ";
		for (auto& l : m_Occupied)
		{
			std::cout << l.first << "(" << SquareTable[(l.first)].m_algCoord << "), ";
		}
		std::cout << '\n';

		std::cout << "CanCastle SquareList =  ";
		for (auto& l : m_canCastle)
		{
			std::cout << l.first << "(" << SquareTable[(l.first)].m_algCoord << "), ";
		}
		std::cout << '\n';

		std::cout << "CanCastleWith SquareList =  ";
		for (auto& l : m_canCastleWith)
		{
			std::cout << l.first << "(" << SquareTable[(l.first)].m_algCoord << "), ";
		}
		std::cout << '\n';

		std::cout << "CastleDestRight Destination SquareList =  ";
		for (auto& l : CastleDestR)
		{
			std::cout << l.first << "(" << SquareTable[(l.first)].m_algCoord << "), ";
		}
		std::cout << '\n';

		std::cout << "CastleDestLeft Destination SquareList =  ";
		for (auto& l : CastleDestL)
		{
			std::cout << l.first << "(" << SquareTable[(l.first)].m_algCoord << "), ";
		}
		std::cout << '\n';

		std::cout << "EnPassant SquareList =  ";
		for (auto& l : m_EnPassantSq)
		{
			std::cout << l.first << "(" << SquareTable[(l.first)].m_algCoord << "), ";
		}
		std::cout << '\n';

		//Volatile Sets here
		std::cout << "Attacking SquareList =  ";
		for (auto& l : Attacking)
		{
			std::cout << l.first << "(" << SquareTable[(l.first)].m_algCoord << "), ";
		}
		std::cout << '\n';

		std::cout << "Captures SquareList =  ";
		for (auto& l : Captures)
		{
			std::cout << l.first << "(" << SquareTable[(l.first)].m_algCoord << "), ";
		}
		std::cout << '\n';

/* 		std::cout << "Defended SquareList =  ";
		for (int d : Defended)
		{
			std::cout << d << "(" << SquareTable[(d)].algebraicName << "), ";
		}
		std::cout << '\n'; */

		std::cout << '\n';
		return;
	}

void SquareStorage::RevokeAllCastleRights()
{
	CastleDestR.clear();
	CastleDestL.clear();

	for (auto& L : m_canCastleWith)
	{
		L.second->canCastleWith = false;
		L.second->GenerateMovetable(false); //regenerates movetable without any castle-moves
	}
	m_canCastleWith.clear();

	for (auto& L : m_canCastle)
	{
		L.second->canCastle = false;
		L.second->GenerateMovetable(false);
	}
	m_canCastle.clear();

	return;
}

void SquareStorage::RestoreAllCastleRights()
{
	for (auto& L : m_Occupied)
	{
		L.second->SetAttributes(); //also resets promotions, Piecename, and CentipawnValue

		if (L.second->canCastle || L.second->canCastleWith)
		{
			AddPiece(L.second); //adds entry to m_canCastle and m_canCastleWith
		}
	}

	FillCastleDests(); //fills castleDestL/R. Both m_canCastleWith and m_canCastle must be filled before calling this.

	//GenerateMovetable can only be called after CastleDests have been filled.
	for (auto& P : m_canCastle)
	{
		P.second->GenerateMovetable(true);
	}
	for (auto& P : m_canCastleWith)
	{
		P.second->GenerateMovetable(true);
	}

	return;
}
