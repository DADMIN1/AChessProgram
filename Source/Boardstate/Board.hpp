#ifndef BOARD_OLD_HPP_INCLUDED
#define BOARD_OLD_HPP_INCLUDED

#include "GlobalVars.hpp" //numRows/numColumns, etc.
#include "Colorsliders.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
//#include <SFML/Graphics/Color.hpp> //included by RectangleShape

#include <vector>	// SquareTable declaration
//#include <memory> //for the occupyingPiece std::unique_ptr
#include <map>
#include <set>
#include <utility> //std::pair
#include <string_view>

class Piece; //Going to store pointers to piece occupying the square.

extern bool isTriggerResize; //indicates that the size of board/squares/window need to be recalculated

struct Dist //stores the distance to each edge of the board for each square.
{
	int Top;
	int Bottom;
	int Left;
	int Right;
};

extern std::map<int, int> columnHeight; //how many rows to REMOVE from each column
extern std::multimap<int, int> excluded_C; //coords of excluded, mapped as column,row
extern std::multimap<int, int> excluded_R; //coords of excluded, mapped as row, column
extern std::set<std::pair<int, int>> manual_excludes; //the user has excluded these during reshape-mode

class Boardsquare : public sf::RectangleShape
{
public:
	static sf::Color lightSqColor;
	static sf::Color darkSqColor;
	
/* 	static std::multimap<int, int> excluded_C; //coords of excluded, mapped as column,row
	static std::multimap<int, int> excluded_R; //coords of excluded, mapped as row, column */

public:
	int m_ID; //Unique identifier; such that SquareTable[m_ID] will access this object
	int column;	//minimum column is now also 1
	int row; //minimum row is 1
	std::string_view m_algCoord;
	bool isDark;
	bool isOccupied;
	Piece* occupyingPiece;
	//if we want to store a piece here, we need to use a smart_ptr, because references can't be changed, and regular pointers are invalidated if the PieceArray ever reallocates memory, which often happens when it's size changes.
	//also, if we accidentally leave a non-null pointer here after clearing the PieceArray, smart_ptrs won't segfault when we access it, but regular pointers to deallocated memory will.
	//but SquareStorage uses normal Piece* and it works just fine? Is that just because the PieceArrays never get resized/reallocated after the pointers are stored?
	//can we use std::reference_wrapper here???

	bool isExcluded;
/* 	bool isSoftTop;
	bool isSoftBottom; */

	Dist edgeDist;

	void setSize(auto x, auto y) { sf::RectangleShape::setSize(sf::Vector2f{x,y}); } //this overload lets us call the function without manually typing out a Vector2f constructor
	// const sf::Vector2f& getCenter() { return (getPosition() + sf::Vector2f(SqWidth/2, SqHeight/2)); } //storing the returned vector2f as a const reference will prolong it's lifetime
	const sf::Vector2f getCenter() { return (getPosition() + sf::Vector2f(SqWidth/2, SqHeight/2)); }

	//unnecessary
	//auto getGlobalBounds() { return sf::IntRect(sf::Shape::getGlobalBounds()); }
	//auto getLocalBounds() { return sf::IntRect(sf::Shape::getLocalBounds()); } //don't EVER use getLocalBounds; it ignores rotations, scaling, and movements (pretends shape is still at 0,0) that have been applied to the shape

	template<typename V>
	bool contains(sf::Vector2<V> point) { return ((sf::Rect<V>(sf::Shape::getGlobalBounds())).contains(point)); }
	//Mouse::getPosition() always returns IntRect, but Shape::getGlobalBounds() always returns FloatRect, and for some reason contains() cannot convert between the two. This also allows us to call .contains directly, instead of chaining the two.

	void RecalcEdgeDist();
	void PrintEdgeDists();

	Boardsquare(int x, int y, int IDnum);

};

extern std::vector<Boardsquare> SquareTable;

using squaretable_matrix_t = std::vector<std::vector<std::reference_wrapper<Boardsquare>>>;
squaretable_matrix_t GetBoardRows(std::vector<Boardsquare>& sourceTable = SquareTable);

void GenerateBoard(bool isResizingBoard);
void SetBoardDimensions(); //User inputs new boardsize through command line
void FlipBoard();

#endif
