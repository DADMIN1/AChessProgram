#ifndef SQUARESTORAGE_HPP_INCLUDED
#define SQUARESTORAGE_HPP_INCLUDED

#include <vector>
#include <map>
#include <memory> //for std::unique_ptr
#include <string>

#include "CoordLookupTable.hpp"

class Piece;

struct InfoStruct //for Captures, EnPassant, and GivingCheck lists
{
	using SquareHash = u_int32_t;
	SquareHash location;

	using PieceHash = u_int32_t;
	PieceHash parent;


};

class SquareStorage
{
public:
	const bool m_isWhite;

	//Stable-lists, can persist through turns with only minor updates
	std::map<int, Piece*> m_Occupied{};
	std::map<int, Piece*> m_Royal{};
	std::map<int, Piece*> m_canCastle{};
	std::map<int, Piece*> m_canCastleWith{};

	//These are half-stable, they'll get cleared on every one of your turns, though ideally castleDest would last forever.
	std::map<int, Piece*> CastleDestR{}; //Squares next to canCastleWith pieces. Right-side of parent piece
	std::map<int, Piece*> CastleDestL{}; //The beauty of this is that even if two pieces both generate a CastleDest on the same square, they'll be stored in the other array!
	std::map<int, Piece*> m_EnPassantSq{}; //First int is square to capture on
//allow multiple entries for scotch, and for variants/fairy pieces where pawns can move > 2sq

	//Volatile, these really need to be cleared and re-generated every HALF-turn, mostly because they change too unpredictably. Also, erasing a value from a multiset erases all instances of it.//All of these list the TARGET SqID.
	std::multimap<int, Piece*> Attacking{}; //NON-CAPTURE attacks (legal non-pacifist, or illegal forcedCapture)
	std::multimap<int, Piece*> Captures{}; //Target squares for LEGAL captures
	//std::multimap<int, InfoStruct> Detailed_Captures{};
	//std::multimap<int, VolatileInfoStruct> Captures_EnPassant{};
	//std::multiset<int> Defended{}; //Pieces that are "attacked" by friendly pieces //useful for moveCircles?
	std::map<int, Piece*> m_givingCheck{}; //SqID of targeted Royal, Piece* of attacking piece

	bool m_wasEmptied{ true }; //Flag indicating that the Stable-lists need to be mass-refilled


	void ClearVolatileSets();
	void ClearAll();

	void RemovePiece(Piece*); //When moving a piece, remove it's current values from the sets
	void AddPiece(Piece*); //Adds a single piece's values to the stable sets, avoids a complete clear and recalculation.
	//ADDPIECE DOES NOT CREATE ENTRIES FOR CASTLEDESTS!!!! (RemovePiece does erase them, though)

	void FillStableSets(std::vector<std::unique_ptr<Piece>>& friendlyArray);
	void FillVolatileSets(std::vector<std::unique_ptr<Piece>>& friendlyArray); //Both sides must have their stable-sets filled first
	void FillCastleDests(); //Assumes the enemy's volatile sets have been updated
	void RevokeAllCastleRights();
	void RestoreAllCastleRights();


	std::string GetHash(bool isSideToMove);

	void PrintAll(bool isWhiteStorage);

	SquareStorage(bool isW, std::vector<std::unique_ptr<Piece>>& friendArray)
		:m_isWhite{isW}
	{
		FillStableSets(friendArray);
	}

};

extern SquareStorage wSquareStorage;
extern SquareStorage bSquareStorage;

void ClearBoard(); //Clears everything off the board
void ClearSquareStorage(); //calls "ClearAll" for both squarestorage objects

//These shouldn't really be here
void CentipawnValueTest();
int TallyMaterial();
void PrintLegalMovelist();


#endif
