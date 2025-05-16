#include "Pieces.hpp"
#include "GlobalVars.hpp" //for isDEBUG
#include "GameState.hpp"
#include "CoordLookupTable.hpp"
#include "Board.hpp"
#include "SquareStorage.hpp"
#include "TurnHistory.hpp"

#include <iostream> //piece destructor uses cout
#include <string> //iostream actually does not include std::string
#include <cassert>

struct NonstandardPieceLocation
{
	std::unordered_set<std::string> LocationNames;
	std::string_view operator[](const std::string S)
	{
		if (LocationNames.contains(S)) { return std::string_view{*LocationNames.find(S)}; }
		return std::string_view{*LocationNames.emplace(S).first}; //emplace returns pair; second is bool
	}
} NonstandardLocation; // This exists as a workaround for the third Piece-constructor overload; otherwise the Piece's m_AlgCoord std::string_view would be referring to a temporary string, causing segfaults.
// It would be much more efficient to construct a std::string_view once at the caller's location instead, and just pass that to a constructor (change the overload to only accept string_view), but then this struct needs to be declared globally, and two lines of code need to be added/modified at each call to the constructor.
// Whereas calling this from the constructor doesn't require any other code to be changed.

std::string_view CreateLocationName(const std::string S)
{ return NonstandardLocation[S]; }

Piece::~Piece()
{
	if (isDEBUG)
	{
		std::cout << GetName() << '(' << (isWhite ? 'w' : 'b')  <<") @ " << '[' << m_AlgCoord << ']' << " destroyed!\n";
	}
}

//defining static members
thor::ResourceHolder<sf::Texture, SwitchName, thor::Resources::CentralOwner> Piece::Textures;
const std::unordered_map<SwitchName, const std::string> Piece::NameMap
{
		{wKing, "King"},
		{wPawn, "Pawn"},
		{wRook, "Rook"},
		{wBishop, "Bishop"},
		{wKnight, "Knight"},
		{wQueen, "Queen"},
		{wStrategos, "Strategos"},
		{wWarlord, "Warlord"},
		{wArchbishop, "Archbishop"},
		{wChancellor, "Chancellor"},
		{wUnicorn, "Unicorn"},
		{wHawk, "Hawk"},
		{wElephant, "Elephant"},
		{wDragon, "Dragon"},
		{wCaptain, "Captain"},
		{wLieutenant, "Lieutenant"},
		{wAlfir, "Alfir"},
		{wLeopard, "Leopard"},
		{wSpider, "Spider"},
		{wFerz, "Ferz"},
		{wHoplite, "Hoplite"},
		{wPrince, "Prince"},
		{wPrincess, "Princess"},
		{wGriffon, "Griffon"},
		{wPaladin, "Paladin"},
		{wFortress, "Fortress"},
		{wGriffon, "Griffon"},
		{wWyvern, "Wyvern"},
		{wLance, "Lance"},
		{wPhoenix, "Phoenix"},
		{wTower, "SeigeTower"}, //Note that the Enum and String don't match
		{wEmperor, "Emperor"},

		{bKing, "King"},
		{bPawn, "Pawn"},
		{bRook, "Rook"},
		{bBishop, "Bishop"},
		{bKnight, "Knight"},
		{bQueen, "Queen"},
		{bStrategos, "Strategos"},
		{bWarlord, "Warlord"},
		{bArchbishop, "Archbishop"},
		{bChancellor, "Chancellor"},
		{bUnicorn, "Unicorn"},
		{bHawk, "Hawk"},
		{bElephant, "Elephant"},
		{bDragon, "Dragon"},
		{bCaptain, "Captain"},
		{bLieutenant, "Lieutenant"},
		{bAlfir, "Alfir"},
		{bLeopard, "Leopard"},
		{bSpider, "Spider"},
		{bFerz, "Ferz"},
		{bHoplite, "Hoplite"},
		{bPrince, "Prince"},
		{bPrincess, "Princess"},
		{bPaladin, "Paladin"},
		{bFortress, "Fortress"},
		{bPhoenix, "Phoenix"},
		{bGriffon, "Griffon"},
		{bWyvern, "Wyvern"},
		{bLance, "Lance"},
		{bTower, "SeigeTower"}, //Note that the Enum and String don't match
		{bEmperor, "Emperor"},

		{wTestPiece, "TestPiece"}, //Fake Pieces here
		{bTestPiece, "TestPiece"},
		{wPieceEnd, "invalidPiece"},
		{bPieceEnd, "invalidPiece"},
		{fairyPieceEnd, "invalidPiece"},
};


//I'm not sure this function really does anything //especially with asserts disabled
constexpr bool isSwitchNameLegal(SwitchName S, auto inc = 0, auto dec = 0) // we're also checking if the modification we're planning to make to the switchname will fall within the enum's range
{
	// using Basetype = std::underlying_type<SwitchName>::type;
	using Basetype = std::underlying_type_t<SwitchName>;
	Basetype upper_limit{std::numeric_limits<Basetype>::max()}; // upper limit should = 2^x - 1 for unsigned int; half that for signed int. //Where 2^x is the smallest power of two that's larger the highest value enum. /* "Values of unscoped enumeration type are implicitly-convertible to integral types. If the underlying type is not fixed, the value is convertible to the first type from the following list able to hold their entire value range: int, unsigned int, long, unsigned long, long long, or unsigned long long" https://en.cppreference.com/w/cpp/language/enum */

	// Bounds-checking inc / dec to ensure they'll be valid after being cast into the enum's base type
	assert(((inc >= 0)&&(dec >= 0)) && "SwitchName increment / decrement operators cannot accept negative values!"); //these can't be negative because the enum's base type is probably unsigned
	assert(((inc <= upper_limit)&&(dec <= upper_limit)) && "SwitchName increment / decrement operators were greater than the upper limit of enum's type_cast "); //Otherwise they'll overflow, and may appear valid after the cast (and possibly negative)

	assert(((S >= 0) && (S <= fairyPieceEnd)) && "SwitchName had an illegal value! (outside of enum range)"); // Checking that current SwitchName's value corresponds to a valid enumerator

	Basetype inc_cast = static_cast<Basetype>(inc);
	Basetype dec_cast = static_cast<Basetype>(dec);

	assert(((S+inc_cast >= S) && (S-dec_cast <= S)) && "SwitchName increment/decrement will trigger an overflow!"); // Checking for overflow or underflow

	/* Theoretically, we don't need to check that 'S' is still within the numerical/enum limits from the opposite direction:
	that {(S+inc) >= 0} and {(S-dec) <= fairyPieceEnd} still hold true after the increment/decrement operations.
	This is because the initial value of 'S' was within these bounds, and therefore incrementing 'S' cannot possibly violate the lower-bound unless the operation overflowed, or 'inc' was negative; both of which have already been accounted for (both before and after the cast). The inverse applies to 'dec'/upper-limit. */

	return (((S+inc_cast <= fairyPieceEnd) && (S-dec_cast >= 0))); // Verifying that the resulting SwitchName will still correspond to a valid enumerator after the operation
}

//for some reason you can't define the "isSwitchNameLegal" function in this header, and thus, you can't globally define the overloads that actually do bounds-checking
	//this version screws up the loop in Zobrist_Hashtable's "fillhashtable", because it has to include "fairyPieceEnd", but it must increment past it before that evals to false.
/* SwitchName& operator++(SwitchName& S) { assert(isSwitchNameLegal(S, 1, 0)); S = SwitchName(S + 1); return S; }
SwitchName& operator--(SwitchName& S) { assert(isSwitchNameLegal(S, 0, 1)); S = SwitchName(S - 1); return S; } */
SwitchName& operator++(SwitchName& S) { S = SwitchName(S + 1); return S; }
SwitchName& operator--(SwitchName& S) { S = SwitchName(S - 1); return S; }

const std::string Piece::GetTexturePath(SwitchName N)
{
	std::string Path{((EnumIsWhite(N)) ? "Resource/Graphics/w" : "Resource/Graphics/b") + GetName_Str(N) + ".png"};

	//invalid-piece texture doesn't have a 'w' or 'b' prefix
	switch (N)
	{
		case wPieceEnd:
		case bPieceEnd:
		case fairyPieceEnd:
			Path = "Resource/Graphics/invalidPiece.png"; break;

		default: break;
	}

	return Path;
}

void Piece::LoadAllTextures()
{
	static bool firstLoad{true};
	assert(firstLoad && "Piece::LoadAllTextures() should only be called once! (during startup)");
	if (!firstLoad) { std::cerr << "Piece::LoadAllTextures() should only be called once! (during startup)\n"; return; }

	for (SwitchName N = SwitchName(0); N < fairyPieceEnd; ++N)
	{
		Textures.acquire(N, thor::Resources::fromFile<sf::Texture>({GetTexturePath(N)})).setSmooth(true);
	}

	firstLoad = false;
	return;
}

const std::unordered_map<SwitchName, sf::Sprite> Piece::CreateSprites(SwitchName first, SwitchName last, bool includeLast)
{
	std::unordered_map<SwitchName, sf::Sprite> SpriteMap;
	for (SwitchName N{first}; N < last; ++N)
	{
		SpriteMap.emplace(N, sf::Sprite(Textures[N]));
	}
	if (includeLast) { SpriteMap.emplace(last, sf::Sprite(Textures[last])); }

	return SpriteMap;
}


std::vector<std::unique_ptr<Piece>> wPieceArray; // dynamic Array that contains objects of class piece.
std::vector<std::unique_ptr<Piece>> bPieceArray;

void Piece::SetAttributes()
{
	//isWhite is set by the constructor, should it be set here instead?
	m_PieceName = GetName(m_PieceType);

	switch (m_PieceType)
	{
		case wPawn:
		case bPawn:
		case wHoplite:
		case bHoplite:
		case wFerz:
		case bFerz:
		case wLance:
		case bLance:
			hasPromotions = true;
			SetPromotes();
			break;

		case wFortress:
		case bFortress:
			canCastle = true; [[fallthrough]];
		case wRook: //Intentional fallthrough
		case bRook:
		case wTower:
		case bTower:
			canCastleWith = true;
			break;

		case wPrince:
		case bPrince:
		case wPrincess:
		case bPrincess:
			hasPromotions = true;
			SetPromotes(); [[fallthrough]];
		case wKing: //Intentional fallthrough
		case bKing:
		case wEmperor:
		case bEmperor:
			isRoyal = true;
			canCastle = true;
			break;

		default:
			break;
	}

	const auto& valueMap = GetPieceValuetable();
	if (!valueMap.contains(m_PieceType))
	{
		std::cerr << "No centipawn value for type: " << m_PieceType << '\n';
	}
	else
	{
		centipawnValue = valueMap.at(m_PieceType);
		if (!isWhite) { centipawnValue *= -1; }
	}

	return;
}

Piece::Piece(SwitchName PieceType, int X, int Y) :
	m_PieceType{PieceType}, m_PieceName{GetName(PieceType)}, m_Sprite{GetTexture(PieceType)},
	m_Column{X}, m_Row{Y}, m_SqID{LookupTable.GetID(X,Y)}, m_AlgCoord{LookupTable.GetStrView(X,Y)}, isWhite{EnumIsWhite(PieceType)}
{
	if ((X < 1) || (Y < 1)) { m_AlgCoord = NonstandardLocation["Out of Bounds"]; isCaptured = true; }

	SetAttributes();
	Movetable.reserve(32);
}

//For pieces in unusual places (placeholder piece, mousePiece, etc)
Piece::Piece(SwitchName PieceType, std::string Location) :
	m_PieceType{PieceType}, m_PieceName{GetName(PieceType)}, m_Sprite{GetTexture(PieceType)},
	m_Column{-1}, m_Row{-1}, m_SqID{-1}, isCaptured{true}, isWhite{EnumIsWhite(PieceType)}
{
	if (isDEBUG) { std::cout << "Piece constructed at nonstandard-location:" << Location << '\n'; }
	m_AlgCoord = NonstandardLocation[Location];
}


//Incomplete implementation
void Piece::PrintAllMoves()
{
	std::cout << '\n';
	for (Move& M : Movetable)
	{
		std::cout << GetName() << GetNotation(M) << '\t';
	}
	return;
}


void EraseCapturedPiecesOld(std::vector<std::unique_ptr<Piece>>& thePieceArray) //now unused
{
	if (isDEBUG) { std::cout << "Checking for Captured Pieces... \n"; }
	auto newEnd = std::remove_if(thePieceArray.begin(), thePieceArray.end(), [&thePieceArray](auto& it)->bool { return(it->isCaptured); }); //actually, why does this lambda capture the array? it shouldn't need it.
	//remove_if basically just zeroes the "removed" and shifts them to the end of the array. It returns the iterator to the first invalid entry.
	//the lambda has to capture the iterator by reference because unique pointers can't be copied.
	//any iterator past the initial position of the first captured piece will be invalidated.

	/* std::remove_if actually uses move-assignment to shift the unremoved entries downwards, rather than calling std::move on the removed entries.
		therefore, the state of the "removed" objects is the leftover state of the (unremoved) entry that used to occupy that slot after a move-assignment.
		Thus, the state of the "removed" objects is not preserved in any way (unless they happen to be near the end, avoiding being overwritten); they are likely invalid.
	*/

//It looks like unique_ptrs release/delete their held ptr when they're overwritten; they're printing the destructor message before this gets output.
	if (isDEBUG && (newEnd != thePieceArray.end())) { std::cout << "Erased Captured Pieces!\n"; }

//We need to actually remove the invalid entries now, or it'll segfault.
/* 	while (newEnd != thePieceArray.end())
	{ newEnd = thePieceArray.erase(newEnd); } */
// remove all the elements at once, so that the vector isn't resorted after every erase.
	thePieceArray.erase(newEnd, thePieceArray.end());

	return;
}

//overload this with a version that actually preserves/returns the captured Pieces.
void EraseCapturedPieces(std::vector<std::unique_ptr<Piece>>& thePieceArray)
{
	if (isDEBUG)
	{
		std::cout << "Checking for Captured Pieces... \n";

		// std::cout << "Checking for Captured Pieces... " << "\x1B[s" << '\n'; //saves cursor position
		// the problem is that the cursor-position is relative to the line; when the position is restored, it doesn't adjust for any newlines that were printed since it was saved. It'd be better to just go up one line in the event that no pieces were erased than it would be to try and keep track of the cursor's position. Also, because you can only save one position at a time, there would be no way to get back to the bottom of the terminal after editing it.
	}

	auto numErased = std::erase_if(thePieceArray, [](auto& it)->bool { return(it->isCaptured); });

	if (isDEBUG)
	{
		if (numErased > 0)
		{
			std::cout << "  Erased " << numErased << " Captured Pieces!\n";
		}
		else
		{
			//std::cout << "\x1B[u"; //restores cursor position
			std::cout << "\x1B[1A"; //moves cursor up one line
			std::cout << "\x1B[33G"; //moves cursor to column 33
			std::cout << "None found\n"; //we have to repeat the whole line because the previous command
		}
	}

	return;
}

void EraseCapturedPieces()
{
	if (isDEBUG) {std::cout << "Checking for Captured Pieces... \n";}

	auto numErased = std::erase_if(wPieceArray, [](auto& it)->bool { return(it->isCaptured); });
	numErased += std::erase_if(bPieceArray, [](auto& it)->bool { return(it->isCaptured); });

	if (isDEBUG)
	{
		if (numErased > 0)
		{std::cout << "  Erased " << numErased << " Captured Pieces!\n";}
		else
		{
			std::cout << "\x1B[1A"; //moves cursor up one line
			std::cout << "\x1B[33G"; //moves cursor to column 33
			std::cout << "None found\n"; //we have to repeat the whole line because the previous command
		}
}

	return;
}

Piece* PlacePiece(SwitchName PieceType, int X, int Y)
{
	std::unique_ptr<Piece>& ptr = (((EnumIsWhite(PieceType)) ? (wPieceArray.emplace_back(new Piece(PieceType, X, Y))) : (bPieceArray.emplace_back(new Piece(PieceType, X, Y)))));

	Piece* ptr1 = ptr.get();

	ptr->m_AlgCoord = NonstandardLocation["Failed PlacePiece"];

Boardsquare& TargetSq = SquareTable[LookupTable.GetID(X, Y)];
	if (TargetSq.isOccupied) { ptr1->isCaptured = true; return ptr1; }
if (TargetSq.isExcluded) { ptr1->isCaptured = true; return ptr1; }

TargetSq.isOccupied = true;
	TargetSq.occupyingPiece = ptr1;
	ptr->m_AlgCoord = LookupTable.GetStrView(X, Y);

	ptr->m_Sprite.setPosition(TargetSq.getPosition()); //Without this, the piece will default to 0,0 for 1 frame.

	if (ptr->isWhite) { wSquareStorage.AddPiece(ptr1); }
	else { bSquareStorage.AddPiece(ptr1); }

return ptr1;
}

// remember that X, Y defaults to -1,-1 for this overload
Piece* PlacePiece(std::unique_ptr<Piece>& preConstructed, int X, int Y)
{
	preConstructed->isCaptured = true;
	preConstructed->m_AlgCoord = NonstandardLocation["preconstructed"];

	//These parameters default to -1, so it'll use the piece's coords if they're not specified
	if (X < 1) { X = preConstructed->m_Column; }
	else { preConstructed->m_Column = X; }
	if (Y < 1) { Y = preConstructed->m_Row; }
	else { preConstructed->m_Row = Y; }

	//assert(((X != -1) && (Y != -1)) && "Ptr-Overload of PlacePiece has invalid coords");
	if (X < 1 || Y < 1 || X > numColumns || Y > numRows)
	{
		std::cerr << "PlacePiece(preConstructed-overload) failed to get valid coords\n";
		preConstructed->isCaptured = true;
		return preConstructed.get(); //should we nullify it?
	}

	Boardsquare& TargetSq = SquareTable[LookupTable.GetID(X, Y)];
	if (TargetSq.isOccupied) { preConstructed->isCaptured = true; return preConstructed.get(); }
	if (TargetSq.isExcluded) { preConstructed->isCaptured = true; return preConstructed.get(); }

	preConstructed->isCaptured = false; //This one's set to true by the nonstandard-location constructor
	preConstructed->m_SqID = LookupTable.GetID(X, Y);
	preConstructed->m_AlgCoord = LookupTable.GetStrView(X, Y);

	TargetSq.isOccupied = true;
	TargetSq.occupyingPiece = preConstructed.get();

	preConstructed->m_Sprite.setPosition(TargetSq.getPosition()); //Without this, the piece will default to 0,0 for 1 frame.

	preConstructed->SetAttributes();

	if (preConstructed->isWhite) { wSquareStorage.AddPiece(preConstructed.get()); }
	else { bSquareStorage.AddPiece(preConstructed.get()); }

	return preConstructed.get();
}



//This function just updates the piece's ID and texture to the values stored in promotionArray
//The values in promotionArray are generated by function SetPromotes()
void Piece::Promote(SwitchName promoteTo)
{
	if (m_PieceType == promoteTo) { return; }

	m_PieceType = promoteTo;
	m_Sprite.setTexture(GetTexture(promoteTo));

	m_Promotions.clear();
	hasPromotions = false;

	SetAttributes(); //this doesn't set the "isWhite" bool
	//it does update the m_PieceName

	//update centipawn value
	//clear movetable?

	return;
}



//determineTransforms should just directly operate on the Piece's movetable instead of creating/returning a tempMovetable
void Piece::GenerateMovetable(bool includeCastles)
{

	if (isCaptured) //If the piece is captured, don't bother calculating anything
	{
		std::cerr << "attempted to generate movetable for captured piece! \n";
		return;
	}

	Movetable.clear();	   //clear out all the moves from last turn no matter what
	tempMovetable.clear(); //don't do this here
	m_OverlappingMoves.clear();
	Movetable.reserve(36); //this can be really wasteful
	tempMovetable.reserve(36);

	if (includeCastles)
	{//Don't bother generating castle moves during opponent's turn
		if (canCastle)
		{
			Movetable = determineTransforms(castleMoveLeft,m_SqID,isWhite,0); //unlimited-distance castling
			Movetable = determineTransforms(castleMoveRight,m_SqID,isWhite,0);
		}

		if (canCastleWith)
		{
			Movetable = determineTransforms(rookFlipLeft, m_SqID);
			Movetable = determineTransforms(rookFlipRight,m_SqID);
			//Movetable = determineTransforms(scootLeftCastle,locationSquareID); //Only generate DURING castling
			//Movetable = determineTransforms(scootRightCastle,locationSquareID);
		}
	}

	switch (m_PieceType)
	{
	case wBishop:
	case bBishop:
		Movetable = determineTransforms(infDiagonal,m_SqID);
		break;

	case bRook:
	case wRook:
		Movetable = determineTransforms(infOrthogonal,m_SqID);
		break;

	case wQueen:
	case bQueen:
		Movetable = determineTransforms(infOrthogonal,m_SqID);
		Movetable = determineTransforms(infDiagonal,m_SqID);
		break;

	case bKing:
	case wKing:
		Movetable = determineTransforms(limOrthogonal,m_SqID,1,1); //Min, Max of 1
		Movetable = determineTransforms(limDiagonal,m_SqID,1,1);
		break;

	case bPawn:
		Movetable = determineTransforms(bPawnMove,m_SqID);
		break;

	case wPawn:
		Movetable = determineTransforms(wPawnMove,m_SqID);
		break;

	case bKnight:
	case wKnight:
		Movetable = determineTransforms(knightMove,m_SqID,1,2); //Instead of MIN/MAX, knightmove parameters describe the coordinate transform
		break;

	case wArchbishop:
	case bArchbishop:
		Movetable = determineTransforms(knightMove,m_SqID,1,2);
		Movetable = determineTransforms(infDiagonal,m_SqID);
		break;

	case wChancellor:
	case bChancellor:
		Movetable = determineTransforms(knightMove,m_SqID,1,2);
		Movetable = determineTransforms(infOrthogonal,m_SqID);
		break;

	case wUnicorn:
	case bUnicorn:
		Movetable = determineTransforms(knightMove,m_SqID,1,2);
		Movetable = determineTransforms(knightMove,m_SqID,1,3); //longKnight.
		break;

	case bHawk:
	case wHawk:
		Movetable = determineTransforms(hawkMove,m_SqID,2,3);
		break;

	case wElephant:
	case bElephant:
		Movetable = determineTransforms(limOrthogonal,m_SqID,1,2);
		Movetable = determineTransforms(limDiagonal,m_SqID,1,2);
		break;

	case wDragon:
	case bDragon:
		Movetable = determineTransforms(infOrthogonal,m_SqID);
		Movetable = determineTransforms(infDiagonal,m_SqID);
		Movetable = determineTransforms(knightMove,m_SqID,1,2);
		break;

	case wStrategos:
	case bStrategos:
		Movetable = determineTransforms(infOrthogonal,m_SqID);
		Movetable = determineTransforms(limDiagonal,m_SqID,1,1);
		break;


	case wCaptain:
	case bCaptain:
		Movetable = determineTransforms(limOrthogonal,m_SqID,1,1);
		Movetable = determineTransforms(limOrthogonal,m_SqID,2,2);
		break;

	case wLieutenant:
	case bLieutenant:
		Movetable = determineTransforms(limDiagonal,m_SqID,1,1);
		Movetable = determineTransforms(limDiagonal,m_SqID,2,2);
		Movetable = determineTransforms(scootRight,m_SqID); //pacifist, NOT a castling move
		Movetable = determineTransforms(scootLeft,m_SqID);
		break;

	case wHoplite:
		Movetable = determineTransforms(wHopliteMove,m_SqID);
		break;

	case bHoplite:
		Movetable = determineTransforms(bHopliteMove,m_SqID);
		break;

	case bFerz:
	case wFerz: //Promotes to Alfir
		Movetable = determineTransforms(limDiagonal,m_SqID,1,1);
		break;

	case bAlfir:
	case wAlfir: //Promoted Shogi-Bishop
		Movetable = determineTransforms(limOrthogonal,m_SqID,1,1);
		Movetable = determineTransforms(infDiagonal,m_SqID);
		break;

	case wLeopard:
	case bLeopard:
		Movetable = determineTransforms(limDiagonal,m_SqID,1,2); //no jumps
		Movetable = determineTransforms(knightMove,m_SqID,1,2);
		break;

	case wSpider:
	case bSpider: //novel piece, not musketeer spider; for that, see Fortress
		Movetable = determineTransforms(knightMove,m_SqID,1,2);
		Movetable = determineTransforms(limOrthogonal,m_SqID,1,1);
		Movetable = determineTransforms(limOrthogonal,m_SqID,3,3);

		break;

	case wPrincess:
	case bPrincess:
		Movetable = determineTransforms(limDiagonal,m_SqID,1,1);
		Movetable = determineTransforms(limOrthogonal,m_SqID,2,2);
		/*if(isWhite)
		Movetable = determineTransforms(wHopliteMove,locationSquareID);
		else
		Movetable = determineTransforms(bHopliteMove,locationSquareID);*/
		break;


	case wPrince:
	case bPrince:
	{
		moveOptions tempOptions{ {1, 2}, true, true, true, true, true, true }; //pacifist knight moves in all directions
		Movetable = determineTransforms(limOrthogonal,m_SqID,1,1);
		Movetable = determineTransforms(knightMove,m_SqID,tempOptions);

/*				if(isWhite)
					Movetable = determineTransforms(wPawnMove,locationSquareID);
				else
					Movetable = determineTransforms(bPawnMove,locationSquareID);*/
	}
	break;

	case wTower: //old fortress moveset
	case bTower: //old fortress moveset
	{
		moveOptions tempOptions{ {1, 4}, false, false, true, true, true, true, true };
		//	Movetable = determineTransforms(limOrthogonal, locationSquareID, tempOptions); //1-sq orthogonal pacifist
		Movetable = determineTransforms(limDiagonal,m_SqID,tempOptions); //should be capture-only

		Movetable = determineTransforms(limOrthogonal,m_SqID,2,2);
		Movetable = determineTransforms(knightMove,m_SqID,1,2);
		//Movetable = determineTransforms(limDiagonal, locationSquareID, 1, 4); //should be capture-only
	}
	break;

	case wFortress:
	case bFortress:
	{
		moveOptions tempOptions{ {1, 2}, false, false, true, true, true, true, false };
		Movetable = determineTransforms(limDiagonal,m_SqID,tempOptions);
		if (((isWhite) && (m_Row <= wTerritory)) || ((!isWhite) && (m_Row >= bTerritory))) //if the piece is in it's territory
		{
			Movetable = determineTransforms(limOrthogonal,m_SqID,1,3);
			Movetable = determineTransforms(knightMove,m_SqID,1,2); //knight moves should be territory-only
			Movetable = determineTransforms(knightMove,m_SqID,1,3);
		}
		else
		{
			tempOptions.dir_UP = false;
			tempOptions.dir_DOWN = false;
			Movetable = determineTransforms(knightMove,m_SqID,tempOptions); //normal 1,2 knight
			tempOptions.dist_LIM = { 1, 3 };
			Movetable = determineTransforms(knightMove,m_SqID,tempOptions); //longknight
			tempOptions.opt_Jump = false;
			Movetable = determineTransforms(limOrthogonal,m_SqID,tempOptions);
		}
	}
	break;

	case wPaladin:
	case bPaladin:
	{
		//Movetable = determineTransforms(limDiagonal, locationSquareID, 1, 1);
		Movetable = determineTransforms(scootLeft,m_SqID);
		Movetable = determineTransforms(scootRight,m_SqID);
		Movetable = determineTransforms(limDiagonal,m_SqID,2,2);
		Movetable = determineTransforms(knightMove,m_SqID,1,3); //longknight
		if (isWhite)
			Movetable = determineTransforms(wLancemove,m_SqID);
		else
			Movetable = determineTransforms(bLancemove,m_SqID);
	}
	break;

	case wGriffon:
	case bGriffon:
		Movetable = determineTransforms(limOrthogonal,m_SqID,1,3); //hawk orthogonals only
		Movetable = determineTransforms(limDiagonal,m_SqID,1,1);
		Movetable = determineTransforms(knightMove,m_SqID,1,3); //longknight
		break;

	case wWyvern:
	case bWyvern:
		/*			Movetable = determineTransforms(limOrthogonal, locationSquareID, 1, 1);
			Movetable = determineTransforms(limOrthogonal, locationSquareID, 2, 2);*/
		Movetable = determineTransforms(limDiagonal,m_SqID,1,4);
		Movetable = determineTransforms(knightMove,m_SqID,1,2); //these knight moves should be capture-only
		Movetable = determineTransforms(knightMove,m_SqID,2,3); //diagonal from knight move
		break;

	case wLance:
	case bLance:
		Movetable = determineTransforms(scootLeft,m_SqID);
		Movetable = determineTransforms(scootRight,m_SqID);
		if (isWhite)
			Movetable = determineTransforms(wLancemove,m_SqID);
		else
			Movetable = determineTransforms(bLancemove,m_SqID);
		break;

	case wPhoenix:
	case bPhoenix:
		Movetable = determineTransforms(limDiagonal,m_SqID,1,1);
		Movetable = determineTransforms(limOrthogonal,m_SqID,2,2); //princess moves
	/*		if (isWhite)
			{moveOptions //just so ctrl+F will find this block
				Movetable = determineTransforms(wHopliteMove, locationSquareID);
				Opts.dir_UP = true;
			}
			else
			{
				Movetable = determineTransforms(bHopliteMove, locationSquareID);
				Opts.dir_DOWN = true;
			}
			Opts.dist_LIM = {1, 2};
			Movetable = determineTransforms(knightMove, locationSquareID, Opts); //generating the 4 forward-knight moves, is this color-dependant?
			Opts.dist_LIM = {2, 1};
			Movetable = determineTransforms(knightMove, locationSquareID, Opts);*/
		break;

	case wEmperor:
	case bEmperor:
//We don't want this piece to be strictly better than a King, because otherwise the promotion choice between King and Emperor is meaningless
//Currently thinking Captian+Lieutenant+Knight moves, but the Captain+Lieutenant moves are territory-only
//If we make Knightmoves territory-only or pacifist instead, the Captain+Lieutenant moves will be strictly better than King
		Movetable = determineTransforms(limOrthogonal,m_SqID,1,1);
		Movetable = determineTransforms(limDiagonal,m_SqID,1,1);
		Movetable = determineTransforms(knightMove,m_SqID,1,2);
		break;

	case wWarlord:
	case bWarlord:
		Movetable = determineTransforms(hawkMove,m_SqID,1,2);
		if (isWhite)
			Movetable = determineTransforms(wLancemove,m_SqID);
		else
			Movetable = determineTransforms(bLancemove,m_SqID);
		break;

	/*case wSamurai:
	case bSamurai:
		{
			Movetable = determineTransforms(limOrthogonal, locationSquareID, 1, 1);
			moveOptions tempOptions{{1, 2}, true, true, true, true, true, true}; //pacifist knight moves in all directions
			Movetable = determineTransforms(knightMove, locationSquareID, tempOptions);
		}
			if (isWhite)
				Movetable = determineTransforms(wPawnMove, locationSquareID);
			else
				Movetable = determineTransforms(bPawnMove, locationSquareID);

			break;*/

	case bTestPiece:
		break;


	case wTestPiece:
		break;

	default:
	{
		std::cerr << "failed to generate movetable, invalid m_PieceType: " << m_PieceType << GetName() << '\n';
		return;
	}
	break;

	} // END OF m_PieceType switch

	SquareStorage& friendStorage{ ((isWhite) ? wSquareStorage : bSquareStorage) };
	SquareStorage& enemyStorage{ ((isWhite) ? bSquareStorage : wSquareStorage) };
	std::unordered_set<int> castlesTo;

	//should Moves be illegal by default instead?
	for (Move& M : Movetable)
	{
		Boardsquare& targ = SquareTable[M.targetSqID];
		if (targ.isExcluded)
		{
			M.isLegal = false;
		}


		if (!M.isLegal)
		{ //Moves like castle-scooting are not meant to be played manually, so they'll be illegal by default.
			//if (isDEBUG) { std::cerr << GetName() << M.GetNotation() << " is already illegal! \n"; }
			continue;
		}

		if (M.targetSqID == M.currentSqID)
		{ //If the piece is capturing itself, make it illegal
			std::cerr << "Move" << GetName() << "x" << M.targetAlgCoord << "moves to it's current SqID! \n";
			M.isLegal = false;
			continue;
		}

		if ((M.isDestOccupied))
		{
			if (M.isPacifist)
			{
				//if (isDEBUG) { std::cout << "\nCapturing pacifist move: " << GetName() << '(' << m_AlgCoord << ')' << "->" << M.targetAlgCoord << '\t'; }

					if ((M.m_moveType == castleMoveRight) || (M.m_moveType == castleMoveLeft))
					{
						//if (isDEBUG) { std::cout << "Is Castling.\t"; }
						if (friendStorage.m_canCastleWith.contains(M.targetSqID))
						{
							//if (isDEBUG) { std::cout << "Found CastleWith-sqID in SquareStorage!\n"; }

							//You need to check if this Piece is royal before checking this. Also you should be looking at the "attacking" maps
							if (enemyStorage.Captures.contains(M.targetSqID) || enemyStorage.m_givingCheck.contains(m_SqID))
							{
								//if (isDEBUG) { std::cout << "But one of the pieces is in check!\n"; }
								M.isLegal = false;
							}

							continue;
						}
						else
						{// Should only happen if the canCastleWith piece is an enemy, which shouldn't actually happen lol.
							//if (isDEBUG) { std::cout << "Could not find CastleWith sqID in SquareStorage! Illegal!\n"; }
							M.isLegal = false; continue;
						}
					}
					else
					{
						//if (isDEBUG) { std::cout << "Not castling. Illegal!\n"; }
						M.isLegal = false; continue;
					} //Scoots should never be legal, Rookflip should not be legal if target is occupied
			}
			//Destination occupied, and not pacifist
			else if (enemyStorage.m_Occupied.contains(M.targetSqID))
			{
				M.isCapture = true;

				if (enemyStorage.m_Royal.contains(M.targetSqID))
				{
					M.isCapturingRoyal = true;
					isGivingCheck = true;
					//enemyInCheck = true

					continue; //It's fine to continue here, right?
				}
			}
			else if (friendStorage.m_Occupied.contains(M.targetSqID))
			{
				M.isLegal = false;
				continue;
			}
			else
			{
				std::cerr << GetName() << GetNotation(M) << ": destination is occupied but neither Storage contains targetSq! \n";
				M.isLegal = false;
				continue;
			}

		} //END if(isDestOccupied)
		else //if (M.isDestOccupied == false)
		{
			if (M.isPacifist)
			{
				if (M.isTypeCastle)
				castlesTo.insert(M.targetSqID);

				if ((M.m_moveType == rookFlipRight)||((M.m_moveType == rookFlipLeft)))
				{
					//You need to check if the flipped-over Piece is royal before checking this.
					int flipOverSq = ((M.m_moveType == rookFlipRight)? (m_SqID + numRows): (m_SqID - numRows));
					if (!(friendStorage.m_canCastle.contains(flipOverSq) && (!enemyStorage.m_givingCheck.contains(flipOverSq))))
					{M.isLegal = false;}
				}

				continue; //all normal pacifist moves will remain legal, and CastleScoots will remain illegal.
			}

			//handle EnPassant AFTER the "isOccupied" block, so that situations where the EnPassantSq is also occupied aren't also considered EnPassant-captures.
			else if (M.isForcedCapture)
			{
				M.isLegal = false;

				if (((M.m_moveType == wPawnMove) || (M.m_moveType == bPawnMove)) && (enemyStorage.m_EnPassantSq.contains(M.targetSqID)))
				{
					M.isEnpassant = true;
					M.isCapture = true; //isDestOccupied is still false!
					M.isLegal = true;
				}
			}
			else if (castlesTo.contains(M.targetSqID)) //checking if a legal move overlaps with a castle-move
			{
					m_OverlappingMoves.insert(M.targetSqID);
			}

		}

	} //END MOVETABLE ITERATION LOOP

	return;
} // END OF generateMovetable


void RegenMovetables(std::vector<std::unique_ptr<Piece>>& pieceArray, bool isYourTurn)
{
	for (std::unique_ptr<Piece>& P : pieceArray)
	{
		if (P->isCaptured)
		{
			//You really shouldn't be filling captured piece list here anyway
			//capturedPieceList.push_back({ P->m_PieceType,P->isWhite }); //Don't do this if you're not clearing the captured pieces
			continue;
		}

		if (isYourTurn) { P->isEnpassantPiece = false; }
		P->isGivingCheck = false;
		P->GenerateMovetable(isYourTurn); //excludes castling if it's not your turn
	}
	//EraseCapturedPieces(pieceArray); //Every time you do this, it invalidates the stored pointers of every piece after it in the array.

	return;
}


void Piece::MakeMove(const Move& theMove, bool isRecursive)
{
	SquareStorage& friendStorage{ ((isWhite) ? wSquareStorage : bSquareStorage) };
	SquareStorage& enemyStorage{ ((isWhite) ? bSquareStorage : wSquareStorage) };

	//Vacating the square we're leaving
	// auto MyPtr = SquareTable[m_SqID].occupyingPiece; //This shared_ptr is the same as the ones stored in PieceArrays, right?
	Piece* MyPtr = this;
	Turn& newTurn = turnhistory.CreateTurn(turnhistory.turnNumber, theMove, MyPtr); //if CreateTurn has already been called this turn, it will return the already-existing one instead of overwriting it.

	SquareTable[(m_SqID)].isOccupied = false;
	SquareTable[m_SqID].occupyingPiece = nullptr;
	friendStorage.RemovePiece(this);

	//Capture logic
	if (theMove.isCapture)
	{
//If you're redoing the move, you can (and should) simply use the "affectedPiece" instead of searching squareStorage (which is color-dependant). Though, if UndoLastMove works properly, it will work fine as is.

		Piece* targetPiece{((theMove.isEnpassant) ? enemyStorage.m_EnPassantSq.at(theMove.targetSqID) : enemyStorage.m_Occupied.at(theMove.targetSqID))};

		if (!turnhistory.isRedoingMove)
		{
			newTurn.affectedPiecePtr = SquareTable[targetPiece->m_SqID].occupyingPiece;

			/* //Prevent these from being needlessly copied! They'll always get overwritten anyway!
			newTurn.affectedPiecePtr->Movetable.clear();
			newTurn.affectedPiecePtr->m_OverlappingMoves.clear(); */

			newTurn.affectedPieceState = std::shared_ptr<Piece>{new Piece(*newTurn.affectedPiecePtr)};
			//*newTurn.affectedPieceState = *newTurn.affectedPiecePtr;
		}

		if (theMove.isEnpassant)
		{ //Normally these would just get overwritten because we'd be moving to the tile we're capturing on
			SquareTable[targetPiece->m_SqID].isOccupied = false;
			SquareTable[targetPiece->m_SqID].occupyingPiece = nullptr;
		}
		targetPiece->isCaptured = true;
		enemyStorage.RemovePiece(targetPiece); //This also takes care of EnPassant squares
	}

	//Creating EnPassant Squares
	if ((GetName() == "Pawn") && (theMove.isPacifist))
	{ //Notice this distance calculation only works with straight-vertical movements
		int DistanceTravled{ theMove.targetSqID - m_SqID };
			if (std::abs(DistanceTravled) > 1)
			{
				isEnpassantPiece = true;

				if (DistanceTravled > 0)
				{ //Traveling down the board - black pawns
					for (int I{ m_SqID+1 }; I < theMove.targetSqID; ++I)
					{
						if(isDEBUG) {std::cout << "enPassant (black) created on SqID: " << I << '\n';}
						friendStorage.m_EnPassantSq.insert_or_assign(I, this);
					}
				}
				else
				{ //Traveling up the board - white pawns
					for (int I{ theMove.targetSqID+1 }; I < m_SqID; ++I)
					{
						if (isDEBUG) { std::cout << "enPassant (white) created on SqID: " << I << '\n'; }
						friendStorage.m_EnPassantSq.insert_or_assign(I, this);
					}
				}
			}
	}


	if (theMove.isTypeCastle && !isRecursive)
	{
		//If this move was triggered by being castled with, the canCastleWith flag has already been set to false. Otherwise, the castling piece wouldn't yet have a valid entry in friendStorage.canCastle, and you would segfault.
		if (canCastleWith && ((theMove.m_moveType == rookFlipLeft) || (theMove.m_moveType == rookFlipRight)))
		{
			Piece* flippedOver{((theMove.m_moveType == rookFlipLeft) ? friendStorage.m_canCastle.at(m_SqID - numRows) : friendStorage.m_canCastle.at(m_SqID + numRows))};

			if (!turnhistory.isRedoingMove)
			{
				newTurn.affectedPiecePtr = SquareTable[flippedOver->m_SqID].occupyingPiece;
				newTurn.affectedPieceState = std::shared_ptr<Piece>{new Piece(*newTurn.affectedPiecePtr)};
				//*newTurn.affectedPieceState = *newTurn.affectedPiecePtr;
			}

			flippedOver->canCastle = false;
		}
		else if((theMove.m_moveType == castleMoveLeft) || (theMove.m_moveType == castleMoveRight)) //if you don't specify (plain else), it crashes
		{
			if (friendStorage.m_canCastleWith.contains(theMove.targetSqID))
			{
				Piece* partner = friendStorage.m_canCastleWith.at(theMove.targetSqID);

				if (!turnhistory.isRedoingMove)
				{
					newTurn.affectedPiecePtr = SquareTable[partner->m_SqID].occupyingPiece;
					newTurn.affectedPieceState = std::shared_ptr<Piece>{new Piece(*newTurn.affectedPiecePtr)};
					//*newTurn.affectedPieceState = *newTurn.affectedPiecePtr;
				}

				//Insert the Castle-Scoot into the rook's movetable, move it there
				//The Enums are laid out so that the appropriate scoot is always +2 from a given castleMove
				partner->Movetable = determineTransforms(static_cast<moveType>(theMove.m_moveType + 2), partner->m_SqID);
				partner->MakeMove(partner->Movetable.back(), true);
			}
			else //if it's targeting a castleDest instead
			{
				auto& destContains{ ((friendStorage.CastleDestL.contains(theMove.targetSqID)) ? friendStorage.CastleDestL : friendStorage.CastleDestR) };
				Piece* partner = destContains.at(theMove.targetSqID);

				if (!turnhistory.isRedoingMove)
				{
					newTurn.affectedPiecePtr = SquareTable[partner->m_SqID].occupyingPiece;
					newTurn.affectedPieceState = std::shared_ptr<Piece>{new Piece(*newTurn.affectedPiecePtr)};
					//*newTurn.affectedPieceState = *newTurn.affectedPiecePtr;
				}

				//The Enums are laid out so that the appropriate rookflip is always +4 from a given castleMove
				partner->Movetable = determineTransforms(static_cast<moveType>(theMove.m_moveType + 4), partner->m_SqID);
				partner->canCastleWith = false; //This prevents the rookFlip from looking for a flippedOver piece (which would segfault, because our location hasn't been updated yet)
				partner->MakeMove(partner->Movetable.back(), true);
			}
		}

	}


	//There should be a specific option for this?
	canCastle = false;
	canCastleWith = false; //These need to go before "AddPiece"

//These must be updated AFTER castling, otherwise castling on-top of the rook would erase the king
	m_SqID = theMove.targetSqID;
	m_AlgCoord = SquareTable[m_SqID].m_algCoord;
	m_Column = SquareTable[(m_SqID)].column;
	m_Row = SquareTable[(m_SqID)].row; //These also need to go before "AddPiece"


	SquareTable[m_SqID].isOccupied = true;
	SquareTable[m_SqID].occupyingPiece = MyPtr;
	friendStorage.AddPiece(this);

	if(!isRecursive) //prevent Rooks from incrementing the Turn# twice during castling
	turnhistory.AddTurn();

	return;
}

void Piece::SetPromotes()
{
	m_Promotions.clear();

	//get range of piece values in promotion table
	//if value of type is > 500, set it to be a "restricted" promotion

	switch (m_PieceType)
	{
		//white pieces
	case wPawn:
		m_Promotions.insert(wRook);
		m_Promotions.insert(wKnight);
		m_Promotions.insert(wBishop);
	[[fallthrough]];
	case wPrincess:
		m_Promotions.insert(wQueen);
		m_Promotions.insert(wArchbishop);
		m_Promotions.insert(wChancellor);
		break;

	case wPrince:
		m_Promotions.insert(wEmperor);
	[[fallthrough]];
	case wHoplite:
		m_Promotions.insert(wStrategos);
		m_Promotions.insert(wKing);
		m_Promotions.insert(wWarlord);
		break;

	case wFerz:
		m_Promotions.insert(wAlfir);
		break;

	case wLance:
		m_Promotions.insert(wRook);
		break;

	//black pieces
	case bPawn:
		m_Promotions.insert(bRook);
		m_Promotions.insert(bKnight);
		m_Promotions.insert(bBishop);
	[[fallthrough]];
	case bPrincess:
		m_Promotions.insert(bQueen);
		m_Promotions.insert(bArchbishop);
		m_Promotions.insert(bChancellor);
		break;

	case bPrince:
		m_Promotions.insert(bEmperor);
	[[fallthrough]];
	case bHoplite:
		m_Promotions.insert(bStrategos);
		m_Promotions.insert(bKing);
		m_Promotions.insert(bWarlord);
		break;

	case bFerz:
		m_Promotions.insert(bAlfir);
		break;

	case bLance:
		m_Promotions.insert(bRook);
		break;

	default:
		m_Promotions.clear();
		hasPromotions = false;
		break;
	}


	return;
}


const std::map<SwitchName, int>& GetPieceValuetable()
{
	//We'll probably want the evaluations to be assigned per-piece, so this is probably just going to be the base value table?
	//Cannot make any positional considerations here, because the function doesn't have access to Board/ruleset info. Move to FISH.

		//static (non-constexpr) local variables are initialized only the first time they are encountered.
	static std::map<SwitchName, int> Valuetable;
	int V{1}; //centipawn value for piece

	// Even as static, and with const declared, Valuetable still gets inserted into everytime without this
	if (Valuetable.empty())
	{
		std::cout << "Generating Piece-Valuetable... \n";

		for (int E{0}; E < fairyPieceEnd; E++)
		{
			//If Piece is the only royal on the field, it should be worth infinity
			//Maybe assign a royal bonus or something?

				//std::cout << "inserting into Valuetable \n";

			switch (E)
			{
				default:
				case wPawn:
				case bPawn:
				case wHoplite:
				case bHoplite:
					V = 100;
					break;

				case wKnight:
				case bKnight:
				case wBishop:
				case bBishop:
					V = 350;
					break;

				case wQueen:
				case bQueen:
					V = 975;
					break;

				case wKing:
				case bKing:
					V = 450;
					break;

				case wRook:
				case bRook:
					V = 500;
					break;

				case wArchbishop:
				case bArchbishop:
					V = 725; //in Spartan-chess
					//V = 650; //Gothic
					break;

				case wChancellor:
				case bChancellor:
					V = 825;
					break;

				case wCaptain:
				case bCaptain:
					V = 325;
					break;

				case wLieutenant:
				case bLieutenant:
					V = 375;
					break;

				case wWarlord:
				case bWarlord:
					V = 875;
					break;

				case wStrategos:
				case bStrategos:
					V = 650;
					break;
			}

			Valuetable.insert({SwitchName(E), V});
		}
	}

	return Valuetable;
}






