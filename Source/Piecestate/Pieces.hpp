#ifndef CHESSPIECES_OLD_HPP_INCLUDED
#define CHESSPIECES_OLD_HPP_INCLUDED

#include "Moves.hpp"

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <Thor/Resources.hpp>

#include <vector>
#include <map> //Piece::valuetable
#include <unordered_map> //mapping strings to pieceEnums
#include <tuple> //getIndicatorInfo uses a big group of variables. Could probably just pass the piece
#include <memory> //unique_ptr (PieceArrays)
#include <string>
#include <string_view>
#include <set> //used in PieceQueue
#include <unordered_set>

enum SwitchName // piecenames for switch statements, because they can't take strings, apparently.
{
//White Vanilla Pieces
	wRook,
	wKnight,
	wBishop,
	wQueen,
	wKing,
	wPawn,
	wPieceEnd,
//Black Vanilla Pieces
	bRook,
	bKnight,
	bBishop,
	bQueen,
	bKing,
	bPawn,
	bPieceEnd,
//Gothic Pieces
	wArchbishop,
	bArchbishop,
	wChancellor,
	bChancellor,
// Fairy Leapers
	wUnicorn,
	bUnicorn,
	wHawk,
	bHawk,
	wElephant,
	bElephant,
	wDragon,
	bDragon,
// Spartan pieces start here; strategos/warlord is NOT ROYAL; spartans use regular kings
	wLieutenant,
	bLieutenant, //Alfir
	wStrategos,
	bStrategos, //shogi dragonKing
	wHoplite,
	bHoplite,
	wCaptain,
	bCaptain,
	wWarlord, //officially, Spartan-chess uses an archbishop, but my version is different
	bWarlord,
// End of Spartan-Pieces; random stuff now
	wFerz,
	bFerz,
	wAlfir,
	bAlfir,
	wLeopard,
	bLeopard,
	wSpider,
	bSpider,
	wPrincess,
	bPrincess,
	wPrince,
	bPrince,
	wFortress,
	bFortress,
	wPaladin,
	bPaladin,
	wGriffon,
	bGriffon,
	wWyvern,
	bWyvern,
	wLance,
	bLance,
	wPhoenix,
	bPhoenix,
	wTower,
	bTower,
	wEmperor,
	bEmperor,
//wSamurai,
//bSamurai,
	wTestPiece,
	bTestPiece,
	fairyPieceEnd, //sentinel for the maximum value of the enum. Don't put anything (legitimate) after this, unless you feel like implementing fakePiece.
	//fakePiece, //this is for fake/invalid pieces. Also for deleting pieces in setup mode?

/*Octopus,
Basilisk,
Ashling,
//Shogi pieces,
DoubleRook,
DoubleBishop,
DoubleQueen,
TripleRook,
TripleBishop,
TripleQueen,
*/

};
/* The Textures for Hoplites, Captains, & Strategos seem to be slightly mis-sized (by a couple pixels).
Additionally, it seems that colored transparent pixels isn't supported, so exporting when exporting as .png from gimp uncheck every option except "save resolution".
*/

SwitchName& operator++(SwitchName&); //you can't define these in the header at all. Linker error; multiple definitions
SwitchName& operator--(SwitchName&);

inline bool EnumIsWhite(SwitchName N) { return ((N <= wPieceEnd) || (((N > bPieceEnd) && ((N % 2) == 0)))); }

const std::map<SwitchName, int>& GetPieceValuetable(); //currently unused, needs to be rewritten

class Move;

class Piece
{
private:
	static const std::unordered_map<SwitchName, const std::string> NameMap; //must store std::strings, not std::string_views (because std::string_views are not null terminated and are dumb)
	static thor::ResourceHolder<sf::Texture, SwitchName, thor::Resources::CentralOwner> Textures;
	static const std::string GetTexturePath(SwitchName);

public:
	SwitchName m_PieceType;
	std::string_view m_PieceName;
	sf::Sprite m_Sprite;
	int m_Column; //legal boardsquares start at 1,1
	int m_Row;
	int m_SqID;
	std::string_view m_AlgCoord;

//Oddly, it seems that these bools are actually the variables that are critical to initialize.
//This is because built-in types remain uninitialized by default.
	bool isCaptured{false}; //Don't screw with the initializers here
	bool isWhite{true};
	bool canCastle{false};
	bool canCastleWith{false};
	bool isEnpassantPiece{false};
	bool isGivingCheck{false}; //if this was removed then piece-states could be easily stored
	bool hasPromotions{false};
	bool isRoyal{false};
	bool isInCheck{false};

	int centipawnValue;

	std::set<SwitchName> m_Promotions;
	std::vector<Move> Movetable;
	std::unordered_set<int> m_OverlappingMoves; //If castling and non-castling moves share a destination

	//returning std::string& instead of string_view. //this is much less efficient; but operator+ can't work with string_view, and string_view can't be implicitly converted to a string.
	//it's inefficient because the compiler can't guarantee that the underlying string referenced won't be modified; so it has to re-fetch the content every time (still better than making a copy every time). And also it has to iterate through the entire string to find the null-terminator to determine the length. //but it should be able to tell if the map stores const std::strings, right?
	const std::string& GetName_Str() { return NameMap.at(m_PieceType); }
	static const std::string& GetName_Str(SwitchName N) { return NameMap.at(N); }

	// returning string_view. This should be much more efficient, because it doesn't require any memory allocation.
	// Passing string_views by copy is more efficient than using a reference, because it's very lightweight and doesn't need to be dereferenced. It also allows for aliasing; the compiler can statically determine that no other code can modify the length/data pointers, allowing it to "cache" the string across function calls. (It can't do this with a reference; like GetName_Str)
	// These are somewhat redundant; you could just use the returned-value of "GetName_Str" to initialize a std::string_view? I guess technically that would create a single copy of the reference
	const std::string_view GetName() { return m_PieceName; } //you should verify that the std::string_view isn't empty/nullptr
	static const std::string_view GetName(SwitchName N) { return NameMap.at(N); }
	static const sf::Texture& GetTexture(SwitchName T) { return Textures[T]; }
	static void LoadAllTextures();
	static sf::Sprite CreateSprite(SwitchName S) { return sf::Sprite{GetTexture(S)}; } //unused??
	static const std::unordered_map<SwitchName, sf::Sprite> CreateSprites(SwitchName first, SwitchName last, bool includeLast = false); //I guess this is unused?? It's intended for SetupWindow

	void SetAttributes(); //add isWhite check to this?
	void SetPromotes();
	void Promote(SwitchName); //change to the piece specified in promoteSet{}
	virtual void GenerateMovetable(bool includeCastles = true);
	void PrintAllMoves(); //Incomplete implementation
	virtual void MakeMove(const Move& theMove, bool isRecursive = false);

	inline const std::tuple<bool, bool, bool, bool, sf::Vector2f> GetIndicatorInfo() { return {isRoyal,canCastle,canCastleWith,hasPromotions,m_Sprite.getPosition()}; } //This must return a copy, returning a reference to a locally-constructed std::tuple segfaults.

	Piece() = default;
	Piece(SwitchName, int X, int Y);
	Piece(SwitchName, std::string Nonstandard_Location_Name); //For pieces in unusual places (placeholder piece, mousePiece, etc)

	virtual ~Piece(); // custom destructor; in Debug-mode, alerts that the piece was destroyed

//declaring a destructor prevents the automatic generation of copy, move, and destructor operations for the class.
	Piece(const Piece&) = default; // explicitly re-adding the copy constructor
	Piece(Piece&&) = default; //move constructor
	Piece& operator=(const Piece&) = default; //copy assignment
	Piece& operator=(Piece&&) = default; //move assignment
// r-value references (temporaries) are mostly used in container operations (push_back/insert/emplace?/swap), and by constructors and std::move

};

using PieceArray = std::vector<std::unique_ptr<Piece>>;
extern std::vector<std::unique_ptr<Piece>> wPieceArray;
extern std::vector<std::unique_ptr<Piece>> bPieceArray;

std::string_view CreateLocationName(const std::string); //stores it in the NonstandardLocationName struct

void EraseCapturedPieces(std::vector<std::unique_ptr<Piece>>& thePieceArray);
void EraseCapturedPieces(); //checks both piece-arrays
void RegenMovetables(std::vector<std::unique_ptr<Piece>>& pieceArray, bool isYourTurn);
Piece* PlacePiece(SwitchName PieceType, int X, int Y); //Minimum 1,1
Piece* PlacePiece(std::unique_ptr<Piece>& preConstructed, int X = -1, int Y = -1);

struct PieceQueue
{
	std::vector<std::unique_ptr<Piece>> preConstructed;
	std::set<std::pair<int,int>> taken; //coords already used by some piece added to the queue

	void AddToQueue(SwitchName Name, int X, int Y)
	{
		std::unique_ptr<Piece>& ptr1 = preConstructed.emplace_back(new Piece(Name, "Placement Queue"));
		ptr1->m_Column = X; ptr1->m_Row = Y;
		taken.insert(std::pair{X, Y});
	}

	//This overload will destroy the original queue vector! (because copying a unique_ptr causes it to release ownership and become nullptr)
	void AddToQueue(std::vector<std::unique_ptr<Piece>>& P_array)
	{
		for(std::unique_ptr<Piece>& U : P_array)
		{
			preConstructed.emplace_back(U.release());
		}

		//go through them and add their coords to "taken"
		//should change their algCoord to "Placement Queue"?
	}
};





#endif
