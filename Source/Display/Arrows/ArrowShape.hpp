#ifndef ARROWSHAPE_HPP_INCLUDED
#define ARROWSHAPE_HPP_INCLUDED

//#include "Board.hpp" //arrowshape keeps a pointer to it's start/end squares
//#include "Pieces.hpp" //arrowshape_smart
//#include "Moves.hpp"//arrowshape_smart
#include "GlobalVars.hpp"

class Boardsquare;
class Piece;
class Move;

#include <Thor/Shapes.hpp>

#include <vector>

class ArrowShape : public thor::Arrow
{
protected: //allows derived classes to access these members
	Boardsquare* m_fromSq{nullptr};
	Boardsquare* m_toSq{nullptr};

	//bool m_isBeingDrawn{true}; //flag to follow the mouse; start/endSq isn't set valid while this is true.
	//bool m_deleteMe{false}; //flag indicating the arrow is invalid, removed, or was cancelled while drawing.

public:
	ArrowShape();

	friend class Arrow_Creator;
};


// Smart ArrowShapes are intended to illustrate calculated lines, variations, or move-history.
// they're also a replacement/alternative to movecircles
// They automatically change shape and color to match the piece and move, and indicate extra information like whether the move was a capture or promotion.
class ArrowShape_smart : public ArrowShape
{
protected:
	//both the represented piece and move will need to be copied/saved somewhere so their pointers don't get invalidated.
	Piece* m_RepPiece{nullptr};
	Move* m_RepMove{nullptr};
	//the base-class's Boardsquare pointers could be found through m_RepMove (which stores current/target sqID)

public:
	ArrowShape_smart()
		: ArrowShape()
	{
		return;
	}

};
//It's not a good idea to use the smart arrows to draw manually, because the Piece/Move pointers will become invalidated after a single move. It's also very difficult to keep track of which piece/move the arrow refers to, after the first.
	//We'd have to map arrows/pieces to squares, so that chains of arrows know where the pieces would be (and then it'd somehow need to know what that piece's new movetable would be)


#endif
