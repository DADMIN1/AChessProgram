#ifndef CHESSPIECE_STACK_HPP_INCLUDED
#define CHESSPIECE_STACK_HPP_INCLUDED

#include "Pieces.hpp"

#include <SFML/Graphics.hpp>

#include <deque> //maybe list would be better? Especially if we're cycling/merging/re-arranging

class PieceStack : public virtual Piece
{ //Virtual inheritance prevents derived classes from inheriting duplicate instances of the base-class
	using stack_basetype = std::deque<std::unique_ptr<Piece>>;
	
	bool hasMonarch{false};
	stack_basetype m_stack;
	sf::RenderTexture m_Texture; //draw all the constituent piece-sprites here
	//m_Sprite already defined as a member of Piece-class
	
	public:
	//if any Piece-function/data is accessed through this
	//point it towards the top member instead, for the most part.
	

	const sf::Texture& GetTexture() {return {m_Texture.getTexture()};}
	const sf::Sprite SetTexture(stack_basetype&); //draw the Stack of Pieces to m_Texture, then return a Sprite. Passing a variable to enable drawing to/from substacks.
	//will Piece::CreateSprite() work correctly? We can't override it because it's static

	void MakeMove(Move& theMove, bool isRecursive = false) override; //needs to apply the changes to all Pieces in the Stack
	void GenerateMovetable(bool includeCastles = true) override; // We technically don't need to override this for reductive-type-Stacks, because only the movetable of the leading (top-most) Piece is used.

	PieceStack& Split(); //returns the remaining (left-behind) substack. If either stack only has one piece remaining, it should just return to being a Piece instead.
	PieceStack& Merge(PieceStack& , bool underOver); //merges with another stack; std::move or std::swap the newly created (returned) stack into this one?
	
	Piece* operator->() {};
};



#endif
