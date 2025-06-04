#include "Setups.hpp"

#include "GameState.hpp"
#include "SquareStorage.hpp" //part of initialPieceSetup is to add them to SquareStorage
#include "Board.hpp"

#include <effolkronium/random.hpp> //effolkronium, used by random setups (like 960)

#include <initializer_list> //required by createBimap (normally they don't take initializer lists)
#include <ranges> //used in the validate functions
#include <sstream>
#include <cassert>
#include <cctype> //Type-checking for extracted characters/strings
#include <list> //loadFEN stores the splitByRows-strings in a std::list
#include <cmath> //for "floor()" used in InitialPieceSetup(), and abs(), only used there

std::string savedFEN{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/ w KQkq - 0 1"};

//Boost-Bimaps don't support initializer lists natively, that's why we have this template.
template <typename L, typename R>
boost::bimap<L, R>
createBimap(std::initializer_list<typename boost::bimap<L, R>::value_type> list)
{
	return boost::bimap<L, R>(list.begin(), list.end());
}

const boost::bimap<SwitchName, char> Setup::Default_FEN_MAP = createBimap<SwitchName, char>({
		{bPawn, 'p'},
		{bKing, 'k'},
		{bRook, 'r'},
		{bKnight, 'n'},
		{bBishop, 'b'},
		{bQueen, 'q'},
		{wPawn, 'P'},
		{wKing, 'K'},
		{wRook, 'R'},
		{wKnight, 'N'},
		{wBishop, 'B'},
		{wQueen, 'Q'},
		{wArchbishop, 'A'},
		{bArchbishop, 'a'},
		{wChancellor, 'C'},
		{bChancellor, 'c'},
		{wUnicorn, 'U'},
		{bUnicorn, 'u'},
		{wHawk, 'I'},
		{bHawk, 'i'},
		{wElephant, 'E'},
		{bElephant, 'e'},
		{wDragon, 'D'},
		{bDragon, 'd'},
		{wLieutenant, 'L'},
		{bLieutenant, 'l'},
		{wStrategos, 'G'},
		{bStrategos, 'g'},
		{wHoplite, 'H'},
		{bHoplite, 'h'},
		{wCaptain, 'M'},
		{bCaptain, 'm'},
		{wWarlord, 'W'},
		{bWarlord, 'w'},
		{wFerz, 'F'},
		{bFerz, 'f'},
		{wAlfir, 'V'},
		{bAlfir, 'v'},
		{wLeopard, 'O'},
		{bLeopard, 'o'},
		{wSpider, 'S'},
		{bSpider, 's'},
		{wPrincess, 'X'},
		{bPrincess, 'x'},
		{wPrince, 'Y'},
		{bPrince, 'y'},
		{wFortress, '>'}, //ran out of alphabetical characters
		{bFortress, '<'},
		{wPaladin, '$'},
		{bPaladin, '%'},
		{wEmperor, 'Z'},
		{bEmperor, 'z'},
		{wTower, 'T'},
		{bTower, 't'},
		{wLance, 'J'},
		{bLance, 'j'},
	});

Setup currentSetup{Setup::Preset::vanilla}; //Must be initialized AFTER bimap template/Default_FEN_MAP definitions

bool Validate1(std::string& section) //FEN
{
	return std::ranges::all_of(section, [](char C)
	{ return (std::isalnum(C) || std::ispunct(C)); });
}

bool Validate2(std::string& section) //sideToMove
{
	if (section.length() > 1) { return false; } //redundant length check

	char C = std::tolower(section.front());
	return ((C == 'w') || (C == 'b'));
}

bool Validate3(std::string& section) //castling
{
	if ((section.length() == 1) && (section.front() == '-')) //allow a dash for no-castling
	{ return true; }

	return std::ranges::all_of(section, [](char C)
	{ return ((std::toupper(C) == 'K') || (std::toupper(C) == 'Q')); });
}

bool Validate4(std::string& section) //enPassant
{
	if ((section.length() == 1) && (section.front() == '-')) { return true; }
	else if (section.length() < 2) { return false; } //enpassant coord must be at least 2 chars: a1
	else if (!std::islower(section.front())) { return false; }

	std::string temp = section.substr(1); //copy every char except first(0)
	return std::ranges::all_of(temp, [](char C)
	{ return (std::isdigit(C)); });
}

bool Validate5(std::string& section)
{
	return std::ranges::all_of(section, [](char C)
	{ return (std::isdigit(C)); });
}

std::string GetCastleStrings(SquareStorage& SS)
{
	std::string castlerights{};
	if (!SS.m_canCastle.empty())
	{
		int castlerSqID = SS.m_canCastle.begin()->first;
		bool KSC {false};
		bool QSC {false};
		for (auto& CWsq : SS.m_canCastleWith)
		{
			if (CWsq.first > castlerSqID) { KSC = true; }
			else if (CWsq.first < castlerSqID) { QSC = true; }
		}

		if (KSC) { castlerights.push_back((SS.m_isWhite)? 'K' : 'k'); }
		if (QSC) { castlerights.push_back((SS.m_isWhite)? 'Q' : 'q'); }
	}

	return castlerights;
}

// erases entries from squareStorage and sets canCastle/canCastleWith bools based on FEN castle-flags. (Setups from FEN default all pieces' castle-flags to true)
void SetCastleFlags(SquareStorage& SS, bool CKS, bool CQS)
{
	bool hasCastles{(CKS || CQS)};
	if (SS.m_canCastle.empty()) { hasCastles = false; }

	if (!hasCastles)
	{
		for (auto& CC : SS.m_canCastle)
		{ CC.second->canCastle = false; }

		for (auto& CW : SS.m_canCastleWith)
		{ CW.second->canCastleWith = false; }

		SS.m_canCastle.clear();
		SS.m_canCastleWith.clear();
		return;
	}

	if (CKS && CQS) { return; }
	//we need to iterate through "canCastle" pieces only if we have one disabled

	int castleSq = SS.m_canCastle.begin()->first;

	//you must use an iterator here
	for (auto CW {SS.m_canCastleWith.begin()}; CW != SS.m_canCastleWith.end();)
	{
		if (((CW->first > castleSq) ? !CKS : !CQS))
		{
			CW->second->canCastleWith = false;
			CW = SS.m_canCastleWith.erase(CW); //sets iterator to next
		}
		else { ++CW; } //don't increase the iterator if you erased
	}

	return;
}


FENstruct LoadFEN(std::string FEN, boost::bimap<SwitchName, char> FEN_MAP)
{
	assert(!FEN.empty() && "Can't load empty FEN");
	assert(!FEN_MAP.empty() && "Passed FEN_MAP is empty!");

	std::istringstream ParsingStream{FEN};

	FENstruct parsedFEN{ParsingStream};
	parsedFEN.Printall();

	std::list<std::string> splitByRows;

	std::istringstream positionSection{parsedFEN.position};
	for (std::string R; std::getline(positionSection, R, '/');)
	{
		splitByRows.push_back(R);
	}

	std::ostringstream FEN_Readback;
	FEN_Readback << '\n';
	int maxColumns = 0;
	int Y = splitByRows.size(); //numRows, we're starting on top row
	int X = 1;
	PieceQueue queue;

	for (std::string& R : splitByRows)
	{
		std::istringstream SS{R};
		while (SS)
		{
			auto nextChar = SS.peek();

			if (std::isalpha(nextChar))
			{
				char C;
				SS >> C;
				queue.AddToQueue(FEN_MAP.right.at(C), X, Y);
				FEN_Readback << C;
				++X;
			}
			else if (std::isdigit(nextChar))
			{
				int I;
				SS >> I; //This way we can have multi-digit numbers in the FEN string
				X += I;
				FEN_Readback << I;
			}
			//SS.peek(); //unnecessary
			if (X > 27) { std::cerr << "Loaded FEN's Row#" << Y << " has > 26 columns! Truncating.\n"; X = 27; break; }
		}

		FEN_Readback << '\n';
		if (X > (maxColumns+1)) { maxColumns = (X-1); }
		--Y;
		X = 1;
	}

	std::cout << FEN_Readback.str() << '\n';

	numColumns = maxColumns;
	numRows = splitByRows.size();
	GenerateBoard(true);

	for (std::unique_ptr<Piece>& P : queue.preConstructed)
	{
		if(PlacePiece(P)->isCaptured) //it'll remain captured if it's got an invalid coordinate
		{
			 P.reset(); //this shouldn't actually be necessary; they'll all get deallocated after queue goes out of scope (when this function ends)
			//the overload of PlacePiece that we called has already added it to SquareStorage
			 continue;
		}

		if (P->isWhite) { wPieceArray.emplace_back(P.release()); }
		else { bPieceArray.emplace_back(P.release()); }
	}
	//the "PlacePiece" call in the above loop sets (default) Piece-attributes and adds an entry to SquareStorage

	// now setting castle-rights
	auto castleRightsContains = [&parsedFEN](char F)->bool {return (parsedFEN.castleRights.find(F) != parsedFEN.castleRights.npos); };

	SetCastleFlags(wSquareStorage, castleRightsContains('K'), castleRightsContains('Q'));
	SetCastleFlags(bSquareStorage, castleRightsContains('k'), castleRightsContains('q'));

	//Still need to set/alternate turn (to generate the movetables)

	return parsedFEN;
}


std::string WriteFEN(std::vector<Boardsquare>& SqRef)
{
	assert(!SqRef.empty() && "Cannot write FEN for empty SquareTable");

	squaretable_matrix_t RowMatrix = GetBoardRows(SqRef);
	assert(!RowMatrix.empty() && "Cannot write FEN with empty RowMatrix!");

	auto mapFEN = currentSetup.GetFENmap(); //eventually we will need to check if FENmap is default or is custom, and export it
	assert(!mapFEN.empty() && "GetFENmap returned empty!");
	std::stringstream FEN;

	static int emptyCounter = 0;
	for (auto R = RowMatrix.rbegin(); R != RowMatrix.rend(); ++R)
	{
		for (Boardsquare& B : *R)
		{
			//It shouldn't be possible to increment emptyCounter twice in a single iteration
			//if (emptyCounter == 9) { FEN << emptyCounter; emptyCounter = 0; } //We can't have two-digit numbers in the FEN

			if (B.isOccupied)
			{
				if (B.occupyingPiece == nullptr)
				{
					// if (isDEBUG) { std::cerr << B.algebraicName << " is occupied but pointer is null!\n"; }
					std::cerr << B.m_algCoord << " is occupied but pointer is null!\n";
					B.isOccupied = false;
					++emptyCounter;
				}
				else if (B.occupyingPiece->isCaptured)
				{
					// if (isDEBUG) { std::cerr << B.algebraicName << " is occupied but occupyingPiece is captured!\n"; }
					std::cerr << B.m_algCoord << " is occupied but occupyingPiece is captured!\n";
					B.isOccupied = false;
					B.occupyingPiece = nullptr; //Shouldn't do this anymore with shared pointer?
					++emptyCounter;
				}
				else
				{
					if (emptyCounter > 0) { FEN << emptyCounter; emptyCounter = 0; }
					FEN << mapFEN.left.at(B.occupyingPiece->m_PieceType);
				}
					//We don't break/continue because we still need to add something to the fen string.
			}
			else //if it was INITIALLY unoccupied
			{
				++emptyCounter;
			}
		}

		if (emptyCounter > 0) { FEN << emptyCounter;  emptyCounter = 0; }
		FEN << '/'; //FEN doesn't usually end with a slash
	}

	std::string castleString{GetCastleStrings(wSquareStorage) + GetCastleStrings(bSquareStorage)};
	if (castleString.empty()) { castleString.push_back('-'); }

	SquareStorage& EnemySquareStorage = ((isWhiteTurn)? wSquareStorage : bSquareStorage);
	//seperated by spaces: side-to-move, castling-rights, en-passant coord, halfmove-clock (50-move rule), turn-count
	FEN << ' ' << ((isWhiteTurn)? 'w' : 'b') << ' ' << castleString << ' ' << ((EnemySquareStorage.m_EnPassantSq.empty())? "-" : std::string{SquareTable[EnemySquareStorage.m_EnPassantSq.begin()->first].m_algCoord}) << ' ' << 0 << ' ' << 1;

	//prints whole FEN when it's done.
	std::cout << "Generated FEN: " << FEN.str() << '\n';
	return FEN.str();
}


//if no parameters are passed, it defaults to the currentRuleset setuptype
void initialPieceSetup(Setup::Preset SetupType)
{
	isGameOver = false;

	ClearBoard();
	GenerateBoard(false); // otherwise the squares remain marked as "occupied". Doesn't change boardsize

	bool isSetupWhite{true};
	bool isSetupBlack{true};

	if (currentSetup.Options.contains(Setup::Option::asymmetric))
	{
		isSetupBlack = false;
	}
	
	// pawn sets for superRandom and horde
	const std::set<SwitchName>  pawnTypes{wPawn, bPawn, wHoplite, bHoplite, wFerz, bFerz};
	const std::set<SwitchName> wPawnTypes{wPawn, wHoplite, wFerz};
	const std::set<SwitchName> bPawnTypes{bPawn, bHoplite, bFerz};

SetupAgain:

	//Check if the setuptype is dynamically generated or not
	switch (SetupType)
	{
	default: //fallthrough to vanilla
	case Setup::Preset::vanilla:
	{
		//draw a column of pawns on the 2nd rank (from each side).
		for (int s{1}; s <= numColumns; ++s)
		{
			PlacePiece(wPawn, s, 2);
			PlacePiece(bPawn, s, numRows - 1);

			int columnMinusOne = (s-1); //loop used to start at 0
				if ((columnMinusOne % 7) < 5) // RNBQK
				{
					PlacePiece(SwitchName((columnMinusOne % 7)), s, 1);
					PlacePiece(SwitchName((columnMinusOne % 7) + 7), s, numRows);
				}
				else // BN, and then it repeats
				{
					PlacePiece(SwitchName(7 - (columnMinusOne % 7)), s, 1);
					PlacePiece(SwitchName((7 - (columnMinusOne % 7)) + 7), s, numRows);
				}
		}
		break;
	} // end vanilla piece setup
	break;

	case Setup::Preset::spartan:
	{
		//draw a column of pawns on the 2nd rank (from each side).
		for (int s{1}; s <= numColumns; ++s)
		{
			int spi{ 0 }; //spartanPieceIndex
			int columnMinusOne = (s-1); //loop used to start at 0

			PlacePiece(wPawn, s, 2);
			PlacePiece(bHoplite, s, numRows - 1);

			if ((columnMinusOne % 7) == 4) //E file
			{
				PlacePiece(wKing, s, 1);
				PlacePiece(bCaptain, s, numRows); // formula works for 4
				// spi = 2; //maybe we need this anyway, because the else statements only trigger if this is false?
			}
			else if ((columnMinusOne % 7) < 4) // RNBQK
			{	//A through D files
				PlacePiece(SwitchName(columnMinusOne % 7), s, 1); //White pieces will be the same

				spi = (columnMinusOne % 7);
				//Spartan piece Order is: Lieutenant,Strategos,King,Captain,Captain,King,Warlord,Lieutenant
				if (spi == 2) PlacePiece(bKing, s, numRows);
				else { PlacePiece(SwitchName(bLieutenant + (2 * spi)), s, numRows); }
			}
			else //E, F, G files (H is treated as A file) (does this actually run for s%7 == 4?)
			{
				PlacePiece(SwitchName(7 - (columnMinusOne % 7)), s, 1); //white

				spi = ((columnMinusOne % 7) % 3);
				if (spi == 2) PlacePiece(bKing, s, numRows);
				else { PlacePiece(SwitchName(bLieutenant + abs((4 - spi) * 2)), s, numRows); }
			}
		}
	} //end spartan piece setup
	break;

	case Setup::Preset::horde:
	{
		const bool isRandomPawns = !isSetupBlack; // controlled by asymmetric setup-option
		auto aPawn = [&wPawnTypes, isRandomPawns](int rerolls=3) -> SwitchName {
			SwitchName nextPawn {*effolkronium::random_static::get(wPawnTypes)};
			while ((rerolls-- > 0) && (nextPawn != wPawn)) // rerolling to favor normal pawns
				nextPawn = *effolkronium::random_static::get(wPawnTypes);
			return (isRandomPawns? nextPawn : wPawn);
		};
		
		for (int s{1}; s <= numColumns; ++s)
		{
			int columnMinusOne = (s-1); //loop used to start at 0

			//placing black pieces
			PlacePiece(bPawn, s, numRows - 1);

			if ((columnMinusOne % 7) < 5) // RNBQK
			{ PlacePiece(SwitchName((columnMinusOne % 7) + 7), s, numRows); }
			else // BN, and then it repeats
			{ PlacePiece(SwitchName((7 - (columnMinusOne % 7)) + 7), s, numRows); }

			// WHITE PAWNS new formula
			int R{ 1 };
			while ((R <= wTerritory + 1) && (R < numRows - 3)) //always one rank above territory (numRows/2.6)
			{
				SwitchName nextPawn = aPawn(std::min(R-3,3)); //reroll more often on higher rows; capping at 3
				// mirroring the pawns for symmetry
				if (columnMinusOne < (numColumns/2)) {
					PlacePiece(nextPawn, s, R);
					PlacePiece(nextPawn,numColumns - columnMinusOne, R);
				} else { PlacePiece(aPawn(), s, R); }
				R += 1;
			}

			std::set<int> excludesLayer1{};
			std::set<int> nextLayerExcludes{};

			//EXTRA ROWS FORMULA FOR EVEN# OF COLUMNS
			if ((numColumns % 2) == 0)
			{
				excludesLayer1.insert(0);

				if (numColumns == 8)
				{ excludesLayer1.insert(3); }
				else //if (numColumns != 8)
				{
					if (floor(numColumns / 2.6) == floor(numColumns / 3)) //why? conversion to int normally rounds up???
					{ excludesLayer1.insert(numColumns / 2.6); } //starting at 12 columns, you get groups of 3 on the sides
					else
					{ excludesLayer1.insert(numColumns / 3); }
				}

				nextLayerExcludes = excludesLayer1;

				while ((R <= (numRows - 3)) && (nextLayerExcludes.size() < std::size_t(numColumns + 2))) //+2 because a completely-empty row will include firstColumn-1 and lastColumn+1
				{
					if ((!nextLayerExcludes.contains(columnMinusOne)) && (columnMinusOne < (numColumns/2)))
					{
						SwitchName nextPawn = aPawn(); // only roll once for symmetry
						PlacePiece(nextPawn, s, R);
						PlacePiece(nextPawn,numColumns - columnMinusOne, R);
					}

					for (auto e : excludesLayer1)
					{ //next layer shrinks by 1 on both sides, so it builds a pyramid
						if((e >= 0) && (e < (numColumns/2)))
						{
							nextLayerExcludes.insert(e + 1);
							nextLayerExcludes.insert(e - 1);
						}
					}
					excludesLayer1 = nextLayerExcludes;

					R += 1;
				}
			}

			else //if ((numColumns % 2) != 0) //EXTRA ROWS FORMULA FOR ODD# OF COLUMNS
			{
				//Odd setups always have pawns on both ends
				//std::set<int> excludesLayer1{}; //does not include 0 by default

				excludesLayer1.insert(numColumns / 4);
				nextLayerExcludes = excludesLayer1;

				while ((R <= (numRows - 3)) && (nextLayerExcludes.size() < std::size_t(numColumns + 2)))
				{
					if ((!nextLayerExcludes.contains(columnMinusOne)) && (columnMinusOne <= (numColumns/2)))
					{
						SwitchName nextPawn = aPawn();
						PlacePiece(nextPawn, s, R);
						PlacePiece(nextPawn,numColumns - columnMinusOne, R);
					}

					//don't do this until AFTER the first iteration, otherwise you won't get the pawns on the edges
					nextLayerExcludes.insert(0); //otherwise it doesn't close in from the edges

					for (auto e : excludesLayer1)
					{ //next layer shrinks by 1 on both sides, so it builds a pyramid
						if ((e >= 0) && (e < (numColumns/2)))
						{
							nextLayerExcludes.insert(e + 1);
							nextLayerExcludes.insert(e - 1);
						}
					}
					excludesLayer1 = nextLayerExcludes;

					R += 1;
				}
			}
		}
	} //End HORDE piece setup
	break;

	case Setup::Preset::stressTest:
	for (auto& Sq : SquareTable)
	{
		PlacePiece(((Sq.isDark)? wQueen : bQueen), Sq.column, Sq.row);
	}
	break;

	case Setup::Preset::gothic:
	{
		for (int x{1}; x <= numColumns; ++x)
		{
			int LC = ((x-1) % 9); //this was originally written with the loop starting at 0, so x needs to be decremented by 1
			PlacePiece(bPawn, x, numRows - 1);
			PlacePiece(wPawn, x, 2);

			switch (LC)
			{
			case 0:
			case 9: //in case you want to repeat rooks, just set LC to MOD10 instead of 9
				PlacePiece(wRook, x, 1);
				PlacePiece(bRook, x, numRows);
				break;

			case 1:
			case 8:
				PlacePiece(wKnight, x, 1);
				PlacePiece(bKnight, x, numRows);
				break;

			case 2:
			case 7:
				PlacePiece(wBishop, x, 1);
				PlacePiece(bBishop, x, numRows);
				break;

			case 3:
				PlacePiece(wQueen, x, 1);
				PlacePiece(bQueen, x, numRows);
				break;

			case 4:
				PlacePiece(wChancellor, x, 1);
				PlacePiece(bChancellor, x, numRows);
				break;

			case 5:
				PlacePiece(wKing, x, 1);
				PlacePiece(bKing, x, numRows);
				break;

			case 6:
				PlacePiece(wArchbishop, x, 1);
				PlacePiece(bArchbishop, x, numRows);
				break;
			}
		}
	}	   //End gothic initialsetup
	break; //belongs to gothic switch

	case Setup::Preset::FischerRandom:
	{
		PieceQueue queue;

		//TODO: If we're using a board smaller than 8x8, we'll need to restrict this range.
		int positionID{effolkronium::random_static::get(1, 960)};

		if (currentSetup.isSetupFromID)
		{
			int stored = ((isSetupWhite)? currentSetup.storedID : currentSetup.storedID_Black);

			if (stored < 1) { std::cerr << "Illegal storedID! " << stored << " - below minimum (1)\n"; currentSetup.isSetupFromID = false; }
			else if (stored > 960) { std::cerr << "Illegal storedID! " << stored << " - above maximum (960)\n"; currentSetup.isSetupFromID = false; }
			else //if storedID was illegal, positionID will retain it's randomly-generated value
			{ positionID = stored; }

			//don't do this here, this variable is needed in Main after setup is complete
			//currentSetup.isSetupFromID = false;

			//in the cases that storedID is invalid, isSetupFromID is set to false to prevent Main from saving the FEN of whatever random position gets generated instead.
		}
		else
		{
			if(isSetupWhite)
				currentSetup.storedID = positionID;

			//if (isSetupBlack) //in case it IS symmetric, we need to copy this over
			currentSetup.storedID_Black = positionID;
		}
		
		auto PadNumber = [](int positionID) {
			if (positionID < 99) std::cout << " ";
			if (positionID <  9) std::cout << " ";
			return positionID;
		};
		
		if (currentSetup.Options.contains(Setup::Option::asymmetric) && isSetupBlack)
		{
			std::cout << "FischerRandom Position#: "
				<< "[Black: " << PadNumber(currentSetup.storedID_Black)  << "] "
				<< "[White: " << PadNumber(currentSetup.storedID) << ']' << '\n';
		}
		else if (isSetupBlack)
		std::cout << "FischerRandom Position#: " << PadNumber(positionID) << "\n";


		int magicNumber = (positionID - 1); //The formula for generating setups requires a range of [0,959]
		queue.AddToQueue(wBishop, (((magicNumber % 4) + 1)*2), 1); //lightsq bishop
		magicNumber /= 4;
		queue.AddToQueue(wBishop, (((magicNumber % 4)*2) + 1), 1); //darksq bishop
		magicNumber /= 4;

		int queenSquare{((magicNumber%6)+1)};
		for (auto& T : queue.taken)
		{
			if (T.first <= queenSquare) { queenSquare += 1; }
		}
		queue.AddToQueue(wQueen, queenSquare, 1);
		magicNumber /= 6;

		//magicNumber is between 0-9 at this point
		//these are the number of empty squares to skip before placing
		int firstKnight = (magicNumber/4);
		if ((magicNumber == 7) || (magicNumber == 9)) { firstKnight += 1; }
		int secondKnight = (firstKnight + (((magicNumber%9)%7)%4) + 1);
		firstKnight += 1; //x-coords start at 1, but we needed the [0-3] range for the formula to get secondKnight.
		secondKnight += 1;

		std::set<int> notTaken; //there will be three squares left, after the knights

		for (int X{1}; X <= 8; ++X)
		{
			if (queue.taken.contains({X,1}))
			{
				firstKnight += 1;
				secondKnight += 1;
				continue;
			}
			else if (X == firstKnight)
			{ queue.AddToQueue(wKnight, firstKnight, 1); }
			else if (X == secondKnight)
			{ queue.AddToQueue(wKnight, secondKnight, 1); }
			else { notTaken.insert(X); }
		}

		//rooks on the outside, king between them
		queue.AddToQueue(wRook, *notTaken.begin(), 1);
		queue.AddToQueue(wRook, *notTaken.rbegin(), 1);
		queue.AddToQueue(wKing, *(++notTaken.begin()), 1);

		// TODO: handle nonstandard boardsizes
		PieceArray tempWhite;
		tempWhite.swap(queue.preConstructed); //because we're going to add black pieces to the queue
		for (auto& ptr : tempWhite)
		{
			if (isSetupWhite) { queue.AddToQueue(wPawn, ptr->m_Column, 2); }
			if (isSetupBlack) {
				queue.AddToQueue(bPawn, ptr->m_Column, numRows-1);
				queue.AddToQueue(SwitchName(ptr->m_PieceType+7), ptr->m_Column, numRows);
			}
		}

		queue.AddToQueue(tempWhite); //recombining into queue. This overload basically destroys tempWhite

		//placing everything in queue
		for (auto& P : queue.preConstructed)
		{
			if (!PlacePiece(P)->isCaptured) //it'll remain captured if it's got an invalid coordinate
			{
				//the overload of PlacePiece that we called has already added it to SquareStorage
			/* 	if ((P->isWhite) && isSetupWhite) { wPieceArray.push_back(P); wSquareStorage.AddPiece(P.get()); }
				else if ((!P->isWhite) && isSetupBlack) { bPieceArray.push_back(P); bSquareStorage.AddPiece(P.get()); } */
				if ((P->isWhite) && isSetupWhite) { wPieceArray.emplace_back(P.release()); }
				else if ((!P->isWhite) && isSetupBlack) { bPieceArray.emplace_back(P.release()); }
			}
			else { P.reset(); } //this shouldn't actually be necessary; they'll all get deallocated after queue goes out of scope (when this function ends)
		}

		if (!isSetupBlack)
		{
			isSetupWhite = false;
			isSetupBlack = true;

			goto SetupAgain;
		}

	}// end of FischerRandom
	break;

	//Uses only the vanilla piece-set. No limits/minimums on piece-duplicates enforced. Symmetric.
	case Setup::Preset::moreRandom:
	{
		for (int x{1}; x <= numColumns; ++x)
		{
			if (isSetupBlack) PlacePiece(bPawn, x, numRows - 1);
			if (isSetupWhite) PlacePiece(wPawn, x, 2);

			int randPiece = effolkronium::random_static::get(0, static_cast<int>(wPieceEnd-1));

			while (randPiece == wPawn)
			{
				randPiece = effolkronium::random_static::get(0, static_cast<int>(wPieceEnd-1));
			}

			if (isSetupWhite) PlacePiece(SwitchName(randPiece), x, 1);
			if (isSetupBlack) PlacePiece(SwitchName(randPiece + 7), x, numRows);
		}

		if (!isSetupBlack)
		{
			isSetupWhite = false;
			isSetupBlack = true;

			goto SetupAgain;
		}
	} //end of moreRandom
	break;

	//all pieces, random pawns
	case Setup::Preset::superRandom:
	{
		const static std::set<SwitchName> illegalNames{wPieceEnd, bPieceEnd, fairyPieceEnd, wTestPiece, bTestPiece};

		for (int x{1}; x <= numColumns; ++x)
		{
			if (isSetupWhite && isSetupBlack) {
				const static std::set<std::pair<SwitchName, SwitchName>> pawnTypes{{wPawn, bPawn}, {wHoplite, bHoplite}, {wFerz, bFerz}};
				const auto& [wRandPawn, bRandPawn] = *effolkronium::random_static::get(pawnTypes);
				PlacePiece(wRandPawn, x, 2); PlacePiece(bRandPawn, x, numRows - 1);
			} else {
				if (isSetupWhite) PlacePiece(*effolkronium::random_static::get(wPawnTypes), x, 2);
				if (isSetupBlack) PlacePiece(*effolkronium::random_static::get(bPawnTypes), x, numRows - 1);
			}

			SwitchName randPiece = SwitchName(effolkronium::random_static::get(0, static_cast<int>(fairyPieceEnd)));
			while (illegalNames.contains(SwitchName(randPiece)) || pawnTypes.contains(SwitchName(randPiece)))
				randPiece = SwitchName(effolkronium::random_static::get(0, static_cast<int>(fairyPieceEnd)));

			if (randPiece < wPieceEnd) //The sections for standard-pieces have a different layout from the fairy pieces
			{
				if (isSetupWhite) PlacePiece(SwitchName(randPiece), x, 1);
				if (isSetupBlack) PlacePiece(SwitchName(randPiece + 7), x, numRows); //add seven to get the matching piece for black
			}
			else if (randPiece < bPieceEnd)
			{
				if (isSetupWhite) PlacePiece(SwitchName(randPiece - 7), x, 1);
				if (isSetupBlack) PlacePiece(SwitchName(randPiece), x, numRows);
			}
			else if ((randPiece % 2) == 0) //for the rest of the enum, if the piece is even, it's white
			{
				if (isSetupWhite) PlacePiece(SwitchName(randPiece), x, 1);
				if (isSetupBlack) PlacePiece(SwitchName(randPiece + 1), x, numRows);
			}
			else if ((randPiece % 2) != 0)
			{
				if (isSetupWhite) PlacePiece(SwitchName(randPiece - 1), x, 1);
				if (isSetupBlack) PlacePiece(SwitchName(randPiece), x, numRows);
			}
			else
			{
				std::cerr << "Random Setup: randPiece didn't match any section of SwitchName, somehow. ID=" << randPiece << '\n';
			}

		}

		if (!isSetupBlack)
		{
			isSetupWhite = false;
			isSetupBlack = true;

			goto SetupAgain;
		}

	} //end of superRandom
	break;


	} // end ruleset switch

	isWhiteTurn = false;
	AlternateTurn();
	//SetRankVars????

	return;
} //End initialPieceSetup


int FindMagicNumber(int magic, int remainder, int modulus) //returns -1 if it can't be found
{
	int minPossible = magic*modulus;
	// int maxPossible = minPossible + magic - 1;
	int maxPossible = minPossible + modulus;

	int match{-1};
	for (int M{minPossible}; M < maxPossible; ++M)
	{
		if ((M % modulus) == remainder)
		{
			match = M;
			std::cout << "found magic number: " << M << '\n';
			break;
		}
	}

	return match;
}


int GetFischerRandomID (std::string rowFEN)
{
	int theID{0};
	const int possibleCombinations{959};

	if (rowFEN.empty())
	{
		//create it from wPieceArray
		std::cerr << "GetFischerRandomID can't fetch the rowFEN itself yet. Either pass a non-empty string or implement this feature \n";
		return theID;
	}

	std::set<int> BishopColumns{};
	std::set<int> KnightColumn{};
	int QueenColumn{};
	int LightBishopColumn{};

	//std::cout << "passed row-FEN: " << rowFEN << '\n';
	for (std::size_t column{0}; column < rowFEN.length(); ++column)
	{
		switch (rowFEN.at(column))
		{
			case 'B':
			case 'b':
			{
				BishopColumns.insert(column);
				bool isDarkBishop = ((column % 2) == 0);
				if (!isDarkBishop) { LightBishopColumn = column; }
				break;
			}

			case 'Q':
			case 'q':
				QueenColumn = column;
				break;

			case 'N':
			case 'n':
				KnightColumn.insert(column);
				break;

			default: break;
		}
	}

	struct Remainders
	{
		int BishopLight; //[0-3]
		int BishopDark; //[0-3]
		int Queen; //[0-5]
		int Knights; //[0-9]

		Remainders() = default;
		Remainders(int Max)
		{
			BishopLight = (Max); //959
			BishopDark = (BishopLight/4); //239
			Queen = (BishopDark/6); //59
			Knights = (Queen/6); //9
		}
	};

	Remainders MaximumID(possibleCombinations); //stores the highest possible magicNumber for that step
	Remainders DerivedRemainder{}; //stores the remainder we've reverse-engineered for that step

	DerivedRemainder.Queen = QueenColumn;
	for (int I : BishopColumns)
	{
		if (I < QueenColumn)
		{
			--DerivedRemainder.Queen;
		}
	}

	std::set<int> taken;
	taken = BishopColumns; //operator= completely overwrites the set
	taken.insert(QueenColumn);

	int KnightFirst_Burned{*KnightColumn.begin()}; //number of unoccupied squares skipped before first knight was placed
	int KnightSecond_Burned{((*KnightColumn.rbegin()) - (*KnightColumn.begin()) - 1)}; //empty squares between the two knights
	for (auto I : taken)
	{
		if (I < (*KnightColumn.begin()))
		{
			--KnightFirst_Burned;
		}
		else if ((I > (*KnightColumn.begin())) && (I < (*KnightColumn.rbegin())))
		{
			--KnightSecond_Burned;
		}
	}

	DerivedRemainder.Knights = (KnightFirst_Burned * (5-KnightFirst_Burned)) + KnightSecond_Burned;
	if (KnightFirst_Burned >= 2) { DerivedRemainder.Knights += 1; }

	std::cout << "\nKnights burned: " << KnightFirst_Burned << " , " << KnightSecond_Burned << '\n';

	//We need to know difference between light/darksq bishop!!!
	//if the column is uneven, it's the light-square //only on bottom row
	DerivedRemainder.BishopLight = LightBishopColumn/2;
	DerivedRemainder.BishopDark = (((*BishopColumns.begin() == LightBishopColumn)? (*BishopColumns.rbegin()) : (*BishopColumns.begin()))/2);

	std::cout << "Remainders: \n"
		<< " BishopLight = " << DerivedRemainder.BishopLight << '\n'
		<< " BishopDark = " << DerivedRemainder.BishopDark << '\n'
		<< " Queen = " << DerivedRemainder.Queen << '\n'
		<< " Knights = " << DerivedRemainder.Knights << '\n';

	int KnightMagic = DerivedRemainder.Knights;
	int QueenMagic = FindMagicNumber(KnightMagic, DerivedRemainder.Queen, 6);
	int BishopDarkMagic = FindMagicNumber(QueenMagic, DerivedRemainder.BishopDark, 4);
	int initialMagic = FindMagicNumber(BishopDarkMagic, DerivedRemainder.BishopLight, 4);

	theID = initialMagic + 1;

	return theID;
}



