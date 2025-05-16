#include "Moves.hpp"
#include "Board.hpp" //edgeDist stuff, constructor needs access to SquareTable
#include "SquareStorage.hpp"
#include "GlobalVars.hpp" //numRows/numColumns/territory,etc.

#include <iostream> //determineTransforms calls cerr

std::vector<Move> tempMovetable{}; //This is an array for storing each move that you generate

// MAKE SURE THE VARIABLES ARE INITIALIZED IN THE CORRECT ORDER!!!
Move::Move(moveType T, int SqID) //current squareID
	: m_moveType{T}, currentSqID{SqID}, currentAlgCoord{SquareTable[SqID].m_algCoord}
{
	/* m_moveType = T;
	currentSqID = SqID;
	currentAlgCoord = SquareTable[SqID].algebraicName; */

	switch (T)
	{
		case scootLeftCastle:
		case scootRightCastle:
			isLegal = false; //Intentional fallthrough
		case castleMoveLeft:
		case castleMoveRight:
		case rookFlipLeft:
		case rookFlipRight:
			isTypeCastle = true;
			isPacifist = true;
			break;

			//NOT castle-scoots
		case scootLeft:
		case scootRight:
			isPacifist = true; break;

		case knightMove:
			isJump = true; break;

		case bPawnMove:
		case wPawnMove:
			isPacifist = true; break;

		case wHopliteMove:
		case bHopliteMove:
			isForcedCapture = true; break;

		default: break;
	}

}

std::string GetNotation(const Move& M)
{
	std::string storedNotation{'(' + std::string{M.currentAlgCoord} + ')' + ((M.isCapture) ? " x " : " -> ") + std::string{M.targetAlgCoord}};
	return (M.isLegal ? storedNotation : (storedNotation.append(" (ILLEGAL!)")));
}

//just calculates a transformation of SquareIDs
int calcTransform(int locationID,int dX,int dY) //NOT a part of the Move class
{
	return (locationID + (numRows * dX) - dY);
}

Move& Move::calcTransform(int dX,int dY)
{
	/*just from using the custom class constructor, the Move already has legitimate values for
	m_moveType and currentSqID generated. The targetSqID and algCoords(target and current)need to be set.*/
	targetSqID = (currentSqID + (numRows * dX) - dY);
	targetAlgCoord = SquareTable[targetSqID].m_algCoord;
	isDestOccupied = SquareTable[targetSqID].isOccupied;

	//kind of jank, but we need to break the move-generation loop somehow
	if (SquareTable[targetSqID].isExcluded)
	{
		isDestOccupied = true;
	}

	return *this;
}

std::vector<Move> determineTransforms(moveType thisType,int startSq) //placeholder is startingSqID
{
	Move tempMove{ thisType, startSq }; //constructs a "Move" object, assigns movetype, and startingSq

	bool isSpaceUp{ SquareTable[startSq].edgeDist.Top > 0 };
	bool isSpaceDown{ SquareTable[startSq].edgeDist.Bottom > 0 };
	bool isSpaceLeft{ SquareTable[startSq].edgeDist.Left > 0 };
	bool isSpaceRight{ SquareTable[startSq].edgeDist.Right > 0 };
	// for clever optimizations

	//compiler warning that DIST_MIN is unused?
	//int DIST_MIN{ 1 }; //(usually) used for limited-range pieces
	int DIST_MAX{ 1 };

	/////////////////////////////////////////////////////
	switch (thisType)
	{
	case infOrthogonal:
		((numColumns > numRows) ? DIST_MAX = numColumns : DIST_MAX = numRows);
		for (int i{ 1 }; i < DIST_MAX; i++)
		{
			isSpaceUp ? isSpaceUp = (i <= SquareTable[startSq].edgeDist.Top) : false; //if isSpaceUp is false, it stays false. Otherwise, it checks if it's below the distance limit.
			// for some reason you have to specify "isSpaceUp =" for the true/false check, but not after the colon? (You still can after the colon)
			isSpaceDown ? isSpaceDown = (i <= SquareTable[startSq].edgeDist.Bottom) : false;
			isSpaceLeft ? isSpaceLeft = (i <= SquareTable[startSq].edgeDist.Left) : false;
			isSpaceRight ? isSpaceRight = (i <= SquareTable[startSq].edgeDist.Right) : false;

			if (isSpaceUp)
			{
				tempMovetable.emplace_back(tempMove.calcTransform(0, i)); // sets the targetSqID and isDestOccupied
				if (tempMove.isDestOccupied)
					isSpaceUp = false;
			}

			if (isSpaceDown)
			{
				tempMovetable.emplace_back(tempMove.calcTransform(0,-i));
				if (tempMove.isDestOccupied)
					isSpaceDown = false;
			}

			if (isSpaceLeft)
			{
				tempMovetable.emplace_back(tempMove.calcTransform(-i,0)); // sets the targetSqID and isDestOccupied
				if (tempMove.isDestOccupied)
					isSpaceLeft = false;
			}

			if (isSpaceRight)
			{
				tempMovetable.emplace_back(tempMove.calcTransform(i,0));
				if (tempMove.isDestOccupied)
					isSpaceRight = false;
			}

			if ((isSpaceUp || isSpaceDown || isSpaceLeft || isSpaceRight) == false) //if there is no space left in any direction
				break;
		}
		break;

	case infDiagonal:
	{
		((SquareTable[startSq].edgeDist.Top < SquareTable[startSq].edgeDist.Right) ? DIST_MAX = SquareTable[startSq].edgeDist.Top : DIST_MAX = SquareTable[startSq].edgeDist.Right);
		for (int i{ 1 }; i <= DIST_MAX; i++)
		{
			tempMovetable.emplace_back(tempMove.calcTransform(i,i));
			if (tempMove.isDestOccupied)
				break;
		}

		((SquareTable[startSq].edgeDist.Bottom < SquareTable[startSq].edgeDist.Left) ? DIST_MAX = SquareTable[startSq].edgeDist.Bottom : DIST_MAX = SquareTable[startSq].edgeDist.Left);
		for (int i{ 1 }; i <= DIST_MAX; i++)
		{
			tempMovetable.emplace_back(tempMove.calcTransform(-i,-i));
			if (tempMove.isDestOccupied)
				break;
		}

		((SquareTable[startSq].edgeDist.Top < SquareTable[startSq].edgeDist.Left) ? DIST_MAX = SquareTable[startSq].edgeDist.Top : DIST_MAX = SquareTable[startSq].edgeDist.Left);
		for (int i{ 1 }; i <= DIST_MAX; i++)
		{
			tempMovetable.emplace_back(tempMove.calcTransform(-i,i)); // sets the targetSqID and isDestOccupied
			if (tempMove.isDestOccupied)
				break;
		}

		((SquareTable[startSq].edgeDist.Bottom < SquareTable[startSq].edgeDist.Right) ? DIST_MAX = SquareTable[startSq].edgeDist.Bottom : DIST_MAX = SquareTable[startSq].edgeDist.Right);
		for (int i{ 1 }; i <= DIST_MAX; i++)
		{
			tempMovetable.emplace_back(tempMove.calcTransform(i,-i));
			if (tempMove.isDestOccupied)
				break;
		}

		break;
	} //END INFDIAGONAL
	break;


	case wPawnMove:
	{
		if (isSpaceUp)
		{
			//Forward move
			tempMovetable.emplace_back(tempMove.calcTransform(0,1));
			if ((tempMove.isDestOccupied == false) && (SquareTable[startSq].edgeDist.Bottom < wTerritory))
				tempMovetable.emplace_back(tempMove.calcTransform(0,2));	//double move

			//Diagonals
			tempMove.isPacifist = false;
			tempMove.isForcedCapture = true;
			if (isSpaceRight)
			{
				tempMovetable.emplace_back(tempMove.calcTransform(1,1));//Diagonals
			}
			if (isSpaceLeft)
			{
				tempMovetable.emplace_back(tempMove.calcTransform(-1,1));
			}
		}
		break;
	}

	case bPawnMove:
	{
		if (isSpaceDown)
		{
			tempMove.isPacifist = true;
			tempMovetable.emplace_back(tempMove.calcTransform(0,-1)); //forward move
			if ((tempMove.isDestOccupied == false) && (SquareTable[(startSq)].row >= bTerritory))
				tempMovetable.emplace_back(tempMove.calcTransform(0,-2)); //double move

			//Diagonals
			tempMove.isPacifist = false;
			tempMove.isForcedCapture = true;
			if (isSpaceRight)
			{
				tempMovetable.emplace_back(tempMove.calcTransform(1,-1));
			}
			if (isSpaceLeft)
			{
				tempMovetable.emplace_back(tempMove.calcTransform(-1,-1));
			}
		}
		break;
	}

	case wHopliteMove:
	{
		if (isSpaceUp)
		{
			//Captures straight forward
			tempMovetable.emplace_back(tempMove.calcTransform(0,1));

			//Moves diagonally without capturing
			tempMove.isForcedCapture = false;
			tempMove.isPacifist = true;

			if (isSpaceRight)
			{
				tempMovetable.emplace_back(tempMove.calcTransform(1,1));//up-right Diagonal
				if ((SquareTable[startSq].edgeDist.Right > 1) && (SquareTable[startSq].edgeDist.Bottom < wTerritory)) //If the white pawn is on or below the 2nd rank
				{
					tempMove.isJump = true;
					tempMovetable.emplace_back(tempMove.calcTransform(2,2)); //double-diagonal move
				}
			}
			if (isSpaceLeft)
			{
				tempMove.isJump = false; //otherwise it can carry over from the double-right move
				tempMovetable.emplace_back(tempMove.calcTransform(-1,1));
				if ((SquareTable[startSq].edgeDist.Left > 1) && (SquareTable[startSq].edgeDist.Bottom < wTerritory)) //If the white pawn is on or below the 2nd rank
				{
					tempMove.isJump = true;
					tempMovetable.emplace_back(tempMove.calcTransform(-2,2)); //double-diagonal move
				}
			}
		}
		break;
	}

	case bHopliteMove:
	{
		if (isSpaceDown)
		{
			//Captures straight forward
			tempMovetable.emplace_back(tempMove.calcTransform(0,-1));

			//Moves diagonally without capturing
			tempMove.isPacifist = true;
			tempMove.isForcedCapture = false;
			if (isSpaceRight)
			{
				tempMovetable.emplace_back(tempMove.calcTransform(1,-1));				   //up-right Diagonal
				if ((SquareTable[startSq].edgeDist.Right > 1) && (SquareTable[startSq].row >= bTerritory)) //If the white pawn is on or below the 2nd rank
				{
					tempMove.isJump = true;
					tempMovetable.emplace_back(tempMove.calcTransform(2,-2)); //double-diagonal move
				}
			}
			if (isSpaceLeft)
			{
				tempMove.isJump = false; //otherwise it can carry over from the double-right move
				tempMovetable.emplace_back(tempMove.calcTransform(-1,-1));
				if ((SquareTable[startSq].edgeDist.Left > 1) && (SquareTable[startSq].row >= bTerritory)) //If the white pawn is on or below the 2nd rank
				{
					tempMove.isJump = true;
					tempMovetable.emplace_back(tempMove.calcTransform(-2,-2)); //double-diagonal move
				}
			}
		}
		break;
	}

	case rookFlipLeft:
		if (SquareTable[startSq].edgeDist.Left >= 2)
		{tempMovetable.emplace_back(tempMove.calcTransform(-2,0));}
		break;

	case rookFlipRight:
		if (SquareTable[startSq].edgeDist.Right >= 2)
		{ tempMovetable.emplace_back(tempMove.calcTransform(2,0)); } break;

	case scootLeft:
	case scootLeftCastle:
		if (isSpaceLeft) //at least 1 square to left edge
		{  tempMovetable.emplace_back(tempMove.calcTransform(-1,0)); }
		break;

	case scootRight:
	case scootRightCastle:
		if (isSpaceRight) //at least 1 square to right edge
			{tempMovetable.emplace_back( tempMove.calcTransform(1, 0));}
		break;

	case wLancemove:
		for (int i{ 1 }; i <= SquareTable[startSq].edgeDist.Top; i++)
		{
			tempMovetable.emplace_back(tempMove.calcTransform(0,i)); // sets the targetSqID and isDestOccupied
			if (tempMove.isDestOccupied)
				break;
		}
		break;

	case bLancemove:
		for (int i{ 1 }; i <= SquareTable[startSq].edgeDist.Bottom; i++)
		{
			tempMovetable.emplace_back(tempMove.calcTransform(0,-i)); // sets the targetSqID and isDestOccupied
			if (tempMove.isDestOccupied)
				break;
		}
		break;

	default:
		std::cerr << "DetermineTransforms(1) has no case label for Movetype: " << thisType << '\n';
		break;

	} //END SWITCH
	return tempMovetable;
} //END STATIC/INF PIECE TRANSFORMS


// The overload for pieces that have limit to their range
//For orthogonals/diagonals, check if limit_MIN == 1 (it normally is), it's not a jump, if it's 0, it is.
std::vector<Move> determineTransforms(moveType thisType,int startSq,int limit_MIN,int limit_MAX)
{
	Move tempMove{ thisType, startSq }; //constructs a "Move" object, assigns movetype, and startingSq

	bool isSpaceUp{ SquareTable[startSq].edgeDist.Top > 0 };
	bool isSpaceDown{ SquareTable[startSq].edgeDist.Bottom > 0 };
	bool isSpaceLeft{ SquareTable[startSq].edgeDist.Left > 0 };
	bool isSpaceRight{ SquareTable[startSq].edgeDist.Right > 0 };
	// for clever optimizations

	//int DIST_MIN{ limit_MIN }; //(usually) used for limited-range pieces
	int DIST_MAX{ limit_MAX };

	//////////////////////////////////////////////////////////
	switch (thisType)
	{
	case limOrthogonal:

		if ((limit_MIN > 1) && (limit_MAX == limit_MIN))
			tempMove.isJump = true; //For hawks, captains

		for (int i{ limit_MIN }; i <= limit_MAX; i++)
		{
			isSpaceUp ? isSpaceUp = (i <= SquareTable[startSq].edgeDist.Top) : false; //if isSpaceUp is false, it stays false. Otherwise, it checks if it's below the distance limit.
			isSpaceDown ? isSpaceDown = (i <= SquareTable[startSq].edgeDist.Bottom) : false;
			isSpaceLeft ? isSpaceLeft = (i <= SquareTable[startSq].edgeDist.Left) : false;
			isSpaceRight ? isSpaceRight = (i <= SquareTable[startSq].edgeDist.Right) : false;

			if (isSpaceUp)
			{
				tempMovetable.emplace_back(tempMove.calcTransform(0,i)); // sets the targetSqID and isDestOccupied
				if (tempMove.isDestOccupied)
					isSpaceUp = false;
			}

			if (isSpaceDown)
			{
				tempMovetable.emplace_back(tempMove.calcTransform(0,-i));
				if (tempMove.isDestOccupied)
					isSpaceDown = false;
			}

			if (isSpaceLeft)
			{
				tempMovetable.emplace_back(tempMove.calcTransform(-i,0)); // sets the targetSqID and isDestOccupied
				if (tempMove.isDestOccupied)
					isSpaceLeft = false;
			}

			if (isSpaceRight)
			{
				tempMovetable.emplace_back(tempMove.calcTransform(i,0));
				if (tempMove.isDestOccupied)
					isSpaceRight = false;
			}

			if ((isSpaceUp || isSpaceDown || isSpaceLeft || isSpaceRight) == false) //if there is no space left in any direction
				break;
		}
		break;

	case limDiagonal:
	{
		if ((limit_MIN > 1) && (limit_MAX == limit_MIN))
			tempMove.isJump = true; //For lieutenants,

		((SquareTable[startSq].edgeDist.Top < SquareTable[startSq].edgeDist.Right) ? DIST_MAX = SquareTable[startSq].edgeDist.Top : DIST_MAX = SquareTable[startSq].edgeDist.Right);
		if (DIST_MAX > limit_MAX)
			DIST_MAX = limit_MAX;
		for (int i{ limit_MIN }; i <= DIST_MAX; i++)
		{
			tempMovetable.emplace_back(tempMove.calcTransform(i,i));
			if (tempMove.isDestOccupied)
				break;
		}

		((SquareTable[startSq].edgeDist.Bottom < SquareTable[startSq].edgeDist.Left) ? DIST_MAX = SquareTable[startSq].edgeDist.Bottom : DIST_MAX = SquareTable[startSq].edgeDist.Left);
		if (DIST_MAX > limit_MAX)
			DIST_MAX = limit_MAX;
		for (int i{ limit_MIN }; i <= DIST_MAX; i++)
		{
			tempMovetable.emplace_back(tempMove.calcTransform(-i,-i));
			if (tempMove.isDestOccupied)
				break;
		}

		((SquareTable[startSq].edgeDist.Top < SquareTable[startSq].edgeDist.Left) ? DIST_MAX = SquareTable[startSq].edgeDist.Top : DIST_MAX = SquareTable[startSq].edgeDist.Left);
		if (DIST_MAX > limit_MAX)
			DIST_MAX = limit_MAX;
		for (int i{ limit_MIN }; i <= DIST_MAX; i++)
		{
			tempMovetable.emplace_back(tempMove.calcTransform(-i,i)); // sets the targetSqID and isDestOccupied
			if (tempMove.isDestOccupied)
				break;
		}

		((SquareTable[startSq].edgeDist.Bottom < SquareTable[startSq].edgeDist.Right) ? DIST_MAX = SquareTable[startSq].edgeDist.Bottom : DIST_MAX = SquareTable[startSq].edgeDist.Right);
		if (DIST_MAX > limit_MAX)
			DIST_MAX = limit_MAX;
		for (int i{ limit_MIN }; i <= DIST_MAX; i++)
		{
			tempMovetable.emplace_back(tempMove.calcTransform(i,-i));
			if (tempMove.isDestOccupied)
				break;
		}

		break;
	} //END LIMDIAGONAL
	break;

	case knightMove:
		if (SquareTable[startSq].edgeDist.Top < limit_MIN)
			isSpaceUp = false;
		if (SquareTable[startSq].edgeDist.Right < limit_MIN)
			isSpaceRight = false;
		if (SquareTable[startSq].edgeDist.Left < limit_MIN)
			isSpaceLeft = false;
		if (SquareTable[startSq].edgeDist.Bottom < limit_MIN)
			isSpaceDown = false;

		if ((isSpaceRight == true) && (SquareTable[startSq].edgeDist.Top >= limit_MAX))
			tempMovetable.emplace_back(tempMove.calcTransform(limit_MIN,limit_MAX)); //wait does it matter? // 1,2
		if ((isSpaceRight == true) && (SquareTable[startSq].edgeDist.Bottom >= limit_MAX))
			tempMovetable.emplace_back(tempMove.calcTransform(limit_MIN,-1 * limit_MAX)); // 1,-2

		if ((isSpaceLeft == true) && (SquareTable[startSq].edgeDist.Top >= limit_MAX))
			tempMovetable.emplace_back(tempMove.calcTransform(-1 * limit_MIN,limit_MAX)); //-1, 2
		if ((isSpaceLeft == true) && (SquareTable[startSq].edgeDist.Bottom >= limit_MAX))
			tempMovetable.emplace_back(tempMove.calcTransform(-1 * limit_MIN,-1 * limit_MAX)); //-1,-2

		if ((isSpaceUp == true) && (SquareTable[startSq].edgeDist.Right >= limit_MAX))
			tempMovetable.emplace_back(tempMove.calcTransform(limit_MAX,limit_MIN));
		if ((isSpaceUp == true) && (SquareTable[startSq].edgeDist.Left >= limit_MAX))
			tempMovetable.emplace_back(tempMove.calcTransform(-1 * limit_MAX,limit_MIN));

		if ((isSpaceDown == true) && (SquareTable[startSq].edgeDist.Right >= limit_MAX))
			tempMovetable.emplace_back(tempMove.calcTransform(limit_MAX,-1 * limit_MIN));
		if ((isSpaceDown == true) && (SquareTable[startSq].edgeDist.Left >= limit_MAX))
			tempMovetable.emplace_back(tempMove.calcTransform(-1 * limit_MAX,-1 * limit_MIN));

		break;

	case castleMoveLeft: //this is in limited so that we can restrict castling to a certain # of squares
	{
		//(limit_MIN == 1)//White
		SquareStorage& friendStorage{ ((limit_MIN == 1) ? wSquareStorage : bSquareStorage) };
		SquareStorage& enemyStorage{ ((limit_MIN == 1) ? bSquareStorage : wSquareStorage) };

		if (friendStorage.CastleDestR.empty()) { break; }
			else if (friendStorage.CastleDestR.begin()->first > startSq) { break; } //if the lowest sqID in the set isn't less, there are no castlesquares on the left side.
			//you could also check if the rows match the starting square (sqID mod numRows)

		bool isParentRoyal{ friendStorage.m_Royal.contains(startSq) };
		if (isParentRoyal && (enemyStorage.m_givingCheck.contains(startSq))) { break; }

		DIST_MAX = SquareTable[startSq].edgeDist.Left; //Otherwise it's 960 castling rules
		if (limit_MAX > 0) DIST_MAX = limit_MAX; //If you want to limit how far the king can move to castle

		for (int i{ 1 }; i <= DIST_MAX; i++) //Castling-in-place is performed by the rook, so we don't need to start at 0
		{
			if (isParentRoyal && (enemyStorage.Attacking.contains(calcTransform(startSq, -i, 0))))
			{ break; } //can't castle through check
			else if (SquareTable[calcTransform(startSq, -i, 0)].isOccupied) //if there's a piece in our way, don't go any further.
			{
				if (friendStorage.m_canCastleWith.contains(calcTransform(startSq, -i, 0))) { tempMovetable.emplace_back( tempMove.calcTransform(-i, 0)); } //side of rook
				break;
			}
			else if (friendStorage.CastleDestR.contains(calcTransform(startSq, -i, 0)))
			{
				tempMovetable.emplace_back( tempMove.calcTransform(-i, 0));	//side of rook
				tempMovetable.emplace_back( tempMove.calcTransform(-(i + 1), 0)); //we want to put the move on top of the rook
				break;
			}
		}
	}
	break;


	case castleMoveRight:
	{
		//(limit_MIN == 1)//White
		SquareStorage& friendStorage{ ((limit_MIN == 1) ? wSquareStorage : bSquareStorage) };
		SquareStorage& enemyStorage{ ((limit_MIN == 1) ? bSquareStorage : wSquareStorage) };

			if (friendStorage.CastleDestL.empty()) { break; }
			else if (friendStorage.CastleDestL.rbegin()->first < startSq) { break; } //if the highest sqID in the set isn't more, there are no castlesquares on the right side.
			//you could also check if the rows match the starting square (sqID mod numRows)

		bool isParentRoyal{ friendStorage.m_Royal.contains(startSq) };
		if (isParentRoyal && (enemyStorage.m_givingCheck.contains(startSq))) { break; }

		DIST_MAX = SquareTable[startSq].edgeDist.Right; //Otherwise it's 960 castling rules
		if (limit_MAX > 0) DIST_MAX = limit_MAX; //If you want to limit how far the king can move to castle

		for (int i{ 1 }; i <= DIST_MAX; i++) //Castling-in-place is performed by the rook, so we don't need to start at 0
		{
			if (isParentRoyal && (enemyStorage.Attacking.contains(calcTransform(startSq, i, 0))))
			{ break; } //can't castle through check
			else if (SquareTable[calcTransform(startSq, i, 0)].isOccupied) //if there's a piece in our way, don't go any further.
			{
				if (friendStorage.m_canCastleWith.contains(calcTransform(startSq, i, 0))) { tempMovetable.emplace_back( tempMove.calcTransform(i, 0)); } //side of rook
				break;
			}
			else if (friendStorage.CastleDestL.contains(calcTransform(startSq, i, 0)))
			{
				tempMovetable.emplace_back( tempMove.calcTransform(i, 0));	//side of rook
				tempMovetable.emplace_back( tempMove.calcTransform((i + 1), 0)); //we want to put the move on top of the rook
				break;
			}
		}
	}
	break;

	case hawkMove:
		for (int i{ limit_MIN }; i <= limit_MAX; i++)
		{ //these will take care of the board edges/illegal move logic
			tempMovetable = determineTransforms(limOrthogonal,startSq,i,i);
			tempMovetable = determineTransforms(limDiagonal,startSq,i,i);
		}
		break;

	case lieutenantMove: //limit_MAX = diagonal distance, limit_MIN = scoot distance?
		tempMovetable = determineTransforms(limDiagonal,startSq,1,1);
		tempMovetable = determineTransforms(limDiagonal,startSq,2,2);
		tempMovetable = determineTransforms(scootRight,startSq); //pacifist, NOT a castling move
		tempMovetable = determineTransforms(scootLeft,startSq);
		break;

	default:
		std::cerr << "DetermineTransforms(2) has no case label for Movetype: " << thisType << '\n';
		break;

	} //END PIECETYPE SWITCH

	return tempMovetable;
} // END LIMITED/RANGED MOVEMENT GENERATION


//This overload is for making very specific, asymmetric movesets
std::vector<Move> determineTransforms(moveType thisType,int startSq,moveOptions Opts)
{
	Move tempMove{ thisType, startSq }; //constructs a "Move" object, assigns movetype, and startingSq

	tempMove.isPacifist = Opts.opt_Pacifist;
	tempMove.isJump = Opts.opt_Jump;
	tempMove.isForcedCapture = Opts.opt_onlyCapture;

	bool isSpaceUp{ SquareTable[startSq].edgeDist.Top > 0 };
	bool isSpaceDown{ SquareTable[startSq].edgeDist.Bottom > 0 };
	bool isSpaceLeft{ SquareTable[startSq].edgeDist.Left > 0 };
	bool isSpaceRight{ SquareTable[startSq].edgeDist.Right > 0 };
	// for clever optimizations

	int limit_MIN{ 1 };
	int limit_MAX{ 1 };

	//	if (Opts.dist_INF == false)
	{
		limit_MIN = std::get<0>(Opts.dist_LIM);
		limit_MAX = std::get<1>(Opts.dist_LIM);
	}

	//int DIST_MIN{ limit_MIN }; //(usually) used for limited-range pieces
	int DIST_MAX{ limit_MAX };

	switch (thisType)
	{
	case knightMove:
		//You can reverse the length order (2,1 instead of 1,2) to get the corner-moves (like the right-up move, without right-down)
		if (SquareTable[startSq].edgeDist.Top < limit_MIN)
			isSpaceUp = false;
		if (SquareTable[startSq].edgeDist.Right < limit_MIN)
			isSpaceRight = false;
		if (SquareTable[startSq].edgeDist.Left < limit_MIN)
			isSpaceLeft = false;
		if (SquareTable[startSq].edgeDist.Bottom < limit_MIN)
			isSpaceDown = false;

		if (Opts.dir_UP)
		{
			if ((isSpaceRight == true) && (SquareTable[startSq].edgeDist.Top >= limit_MAX))
				tempMovetable.emplace_back(tempMove.calcTransform(limit_MIN,limit_MAX)); //wait does it matter? // 1,2
			if ((isSpaceLeft == true) && (SquareTable[startSq].edgeDist.Top >= limit_MAX))
				tempMovetable.emplace_back(tempMove.calcTransform(-1 * limit_MIN,limit_MAX)); //-1, 2
		}

		if (Opts.dir_LEFT)
		{
			if ((isSpaceUp == true) && (SquareTable[startSq].edgeDist.Left >= limit_MAX))
				tempMovetable.emplace_back(tempMove.calcTransform(-1 * limit_MAX,limit_MIN));
			if ((isSpaceDown == true) && (SquareTable[startSq].edgeDist.Left >= limit_MAX))
				tempMovetable.emplace_back(tempMove.calcTransform(-1 * limit_MAX,-1 * limit_MIN));
		}

		if (Opts.dir_DOWN)
		{
			if ((isSpaceLeft == true) && (SquareTable[startSq].edgeDist.Bottom >= limit_MAX))
				tempMovetable.emplace_back(tempMove.calcTransform(-1 * limit_MIN,-1 * limit_MAX)); //-1,-2
			if ((isSpaceRight == true) && (SquareTable[startSq].edgeDist.Bottom >= limit_MAX))
				tempMovetable.emplace_back(tempMove.calcTransform(limit_MIN,-1 * limit_MAX)); // 1,-2
		}

		if (Opts.dir_RIGHT)
		{
			if ((isSpaceDown == true) && (SquareTable[startSq].edgeDist.Right >= limit_MAX))
				tempMovetable.emplace_back(tempMove.calcTransform(limit_MAX,-1 * limit_MIN));
			if ((isSpaceUp == true) && (SquareTable[startSq].edgeDist.Right >= limit_MAX))
				tempMovetable.emplace_back(tempMove.calcTransform(limit_MAX,limit_MIN));
		}
		break;
		//END KNIGHTMOVES

	case limOrthogonal:

		/*		if ((limit_MIN > 1) && (limit_MAX == limit_MIN))
			tempMove.isJump = true; //For hawks, captains*/

		for (int i{ limit_MIN }; i <= limit_MAX; i++)
		{
			isSpaceUp ? isSpaceUp = (i <= SquareTable[startSq].edgeDist.Top) : false; //if isSpaceUp is false, it stays false. Otherwise, it checks if it's below the distance limit.
			isSpaceDown ? isSpaceDown = (i <= SquareTable[startSq].edgeDist.Bottom) : false;
			isSpaceLeft ? isSpaceLeft = (i <= SquareTable[startSq].edgeDist.Left) : false;
			isSpaceRight ? isSpaceRight = (i <= SquareTable[startSq].edgeDist.Right) : false;

			if ((isSpaceUp) && (Opts.dir_UP))
			{
				tempMovetable.emplace_back(tempMove.calcTransform(0,i)); // sets the targetSqID and isDestOccupied
				if ((tempMove.isDestOccupied) && (Opts.opt_Jump == false))
					isSpaceUp = false;
			}

			if ((isSpaceDown) && (Opts.dir_DOWN))
			{
				tempMovetable.emplace_back(tempMove.calcTransform(0,-i));
				if ((tempMove.isDestOccupied) && (Opts.opt_Jump == false))
					isSpaceDown = false;
			}

			if ((isSpaceLeft) && (Opts.dir_LEFT))
			{
				tempMovetable.emplace_back(tempMove.calcTransform(-i,0)); // sets the targetSqID and isDestOccupied
				if ((tempMove.isDestOccupied) && (Opts.opt_Jump == false))
					isSpaceLeft = false;
			}

			if ((isSpaceRight) && (Opts.dir_RIGHT))
			{
				tempMovetable.emplace_back(tempMove.calcTransform(i,0));
				if ((tempMove.isDestOccupied) && (Opts.opt_Jump == false))
					isSpaceRight = false;
			}

			if ((isSpaceUp || isSpaceDown || isSpaceLeft || isSpaceRight) == false) //if there is no space left in any direction
				break;
		}
		break;

	case limDiagonal:
	{
		/*		if ((limit_MIN > 1) && (limit_MAX == limit_MIN))
			tempMove.isJump = true; */

		((SquareTable[startSq].edgeDist.Top < SquareTable[startSq].edgeDist.Right) ? DIST_MAX = SquareTable[startSq].edgeDist.Top : DIST_MAX = SquareTable[startSq].edgeDist.Right);
		if (DIST_MAX > limit_MAX)
			DIST_MAX = limit_MAX;

		for (int i{ limit_MIN }; i <= DIST_MAX; i++)
		{
			tempMovetable.emplace_back(tempMove.calcTransform(i,i));
			if ((tempMove.isDestOccupied) && (Opts.opt_Jump == false))
				break;
		}

		((SquareTable[startSq].edgeDist.Bottom < SquareTable[startSq].edgeDist.Left) ? DIST_MAX = SquareTable[startSq].edgeDist.Bottom : DIST_MAX = SquareTable[startSq].edgeDist.Left);
		if (DIST_MAX > limit_MAX)
			DIST_MAX = limit_MAX;
		for (int i{ limit_MIN }; i <= DIST_MAX; i++)
		{
			tempMovetable.emplace_back(tempMove.calcTransform(-i,-i));
			if ((tempMove.isDestOccupied) && (Opts.opt_Jump == false))
				break;
		}

		((SquareTable[startSq].edgeDist.Top < SquareTable[startSq].edgeDist.Left) ? DIST_MAX = SquareTable[startSq].edgeDist.Top : DIST_MAX = SquareTable[startSq].edgeDist.Left);
		if (DIST_MAX > limit_MAX)
			DIST_MAX = limit_MAX;
		for (int i{ limit_MIN }; i <= DIST_MAX; i++)
		{
			tempMovetable.emplace_back(tempMove.calcTransform(-i,i)); // sets the targetSqID and isDestOccupied
			if ((tempMove.isDestOccupied) && (Opts.opt_Jump == false))
				break;
		}

		((SquareTable[startSq].edgeDist.Bottom < SquareTable[startSq].edgeDist.Right) ? DIST_MAX = SquareTable[startSq].edgeDist.Bottom : DIST_MAX = SquareTable[startSq].edgeDist.Right);
		if (DIST_MAX > limit_MAX)
			DIST_MAX = limit_MAX;
		for (int i{ limit_MIN }; i <= DIST_MAX; i++)
		{
			tempMovetable.emplace_back(tempMove.calcTransform(i,-i));
			if ((tempMove.isDestOccupied) && (Opts.opt_Jump == false))
				break;
		}

		break;
	} //END LIMDIAGONAL
	break;

	//unused in this overload
	/* case infOrthogonal:
		((numColumns > numRows) ? DIST_MAX = numColumns : DIST_MAX = numRows);
		for (int i{ 1 }; i < DIST_MAX; i++)
		{
			isSpaceUp ? isSpaceUp = (i <= SquareTable[startSq].edgeDist.Top) : false; //if isSpaceUp is false, it stays false. Otherwise, it checks if it's below the distance limit.
			// for some reason you have to specify "isSpaceUp =" for the true/false check, but not after the colon? (You still can after the colon)
			isSpaceDown ? isSpaceDown = (i <= SquareTable[startSq].edgeDist.Bottom) : false;
			isSpaceLeft ? isSpaceLeft = (i <= SquareTable[startSq].edgeDist.Left) : false;
			isSpaceRight ? isSpaceRight = (i <= SquareTable[startSq].edgeDist.Right) : false;

			if ((isSpaceUp) && (Opts.dir_UP))
			{
				tempMovetable.emplace_back(tempMove.calcTransform(0,i)); // sets the targetSqID and isDestOccupied
				if ((tempMove.isDestOccupied) && (Opts.opt_Jump == false))
					isSpaceUp = false;
			}

			if ((isSpaceDown) && (Opts.dir_DOWN))
			{
				tempMovetable.emplace_back(tempMove.calcTransform(0,-i));
				if ((tempMove.isDestOccupied) && (Opts.opt_Jump == false))
					isSpaceDown = false;
			}

			if ((isSpaceLeft) && (Opts.dir_LEFT))
			{
				tempMovetable.emplace_back(tempMove.calcTransform(-i,0)); // sets the targetSqID and isDestOccupied
				if ((tempMove.isDestOccupied) && (Opts.opt_Jump == false))
					isSpaceLeft = false;
			}

			if ((isSpaceRight) && (Opts.dir_RIGHT))
			{
				tempMovetable.emplace_back(tempMove.calcTransform(i,0));
				if ((tempMove.isDestOccupied) && (Opts.opt_Jump == false))
					isSpaceRight = false;
			}

			if ((isSpaceUp || isSpaceDown || isSpaceLeft || isSpaceRight) == false) //if there is no space left in any direction
				break;
		}
		break; //END INFORTHOGONAL */

	default:
		std::cerr << "DetermineTransforms(3) has no case label for Movetype: " << thisType << '\n';
		break;
	} //END MOVETYPE SWITCH

	return tempMovetable;
} //END ASYMMETRIC MOVE GENERATION
