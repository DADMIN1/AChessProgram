#include "HighlightsIndicators.hpp"
#include "Colorsliders.hpp" //to access lightsqcolor/sqoutline etc.
#include "GlobalVars.hpp" //sqHeight, SquareTable, isGameOver
#include "Moves.hpp" //need Move Class, moveType enums, targetPxOffset.
#include "Board.hpp" //need to access Squaretable to get each square's center

#include <unordered_set>

bool MoveCircles::use_altcolors {false};

MoveCircles::MoveCircles(std::size_t index)
{
	legal = sf::CircleShape(SqWidth / 4);	// Displays legal moves
	legal.setFillColor(sf::Color(50, 200, 255, 175)); //light blue
	legal.setOrigin(legal.getRadius(), legal.getRadius());
	legal.setOutlineThickness(6);
	legal.setOutlineColor(sf::Color(0, 0, 0, 0)); //transparent, defining this just so that the inherting circles get an outline
	
	// alternate colors
	switch(index) {
		default:
		case 0: break;
		case 1: legal.setOutlineColor(sf::Color(50, 200, 255, 175)); break;
		case 2: legal.setOutlineColor(sf::Color(175, 75, 150, 175)); break;
		case 3: legal.setOutlineColor(sf::Color(125, 255, 50, 175)); break;
	}
	if (index > 0) legal.setFillColor(legal.getOutlineColor() * sf::Color::Yellow);
	
	capture = (legal);
	capture.setFillColor(sf::Color(255, 0, 0, 150));
	capture.setRadius(40);
	capture.setOrigin(capture.getRadius(), capture.getRadius());

	illegal = (legal);
	illegal.setFillColor(sf::Color(125, 125, 125, 175)); //grey
	illegal.setRadius(30);
	illegal.setOrigin(illegal.getRadius(), illegal.getRadius());

	pacifist = (legal);
	pacifist.setRadius(35);
	pacifist.setFillColor(sf::Color(0, 0, 0, 0));
	pacifist.setOutlineColor(sf::Color(50, 225, 50, 125)); //green
	pacifist.setOrigin(pacifist.getRadius(), pacifist.getRadius());

	illegal_Hollow = (pacifist);
	illegal_Hollow.setOutlineColor(sf::Color(125, 125, 125, 175)); //grey

	capture_forced = (pacifist);
	capture_forced.setOutlineColor(sf::Color(255, 0, 0, 125)); //red instead of green

	castle = (pacifist);
	castle.setOutlineColor(sf::Color(255, 195, 0, 175)); //gold

	priority = (legal);
	priority.setOutlineColor(legal.getFillColor());
	priority.setOutlineThickness(-10);

	priority_hollow = (castle);
	priority_hollow.setOutlineThickness(10);

	jump_texture.loadFromFile("Resource/Graphics/jumpArrow.png");

	jump_indicator.setTexture(jump_texture);
	jump_indicator.setOrigin(60, 60);

}

void MoveCircles::RescaleAll(float H_scale, float V_scale)
{
	legal.setScale(H_scale, V_scale);
	illegal.setScale(H_scale, V_scale);
	pacifist.setScale(H_scale, V_scale);
	capture.setScale(H_scale, V_scale);
	castle.setScale(H_scale, V_scale);
	capture_forced.setScale(H_scale, V_scale);
	illegal_Hollow.setScale(H_scale, V_scale);
	priority.setScale(H_scale, V_scale);
	priority_hollow.setScale(H_scale, V_scale);

	jump_indicator.setScale(H_scale, V_scale);
	return;
}

MoveCircles moveCircles{};
std::array<MoveCircles, 3> altCircles{MoveCircles(1), MoveCircles(2), MoveCircles(3)};
sf::RectangleShape dangermapHighlight{};

void MoveCircles::RescaleAlt(float H_scale, float V_scale)
{
	for (MoveCircles& MC: altCircles) MC.RescaleAll(H_scale, V_scale);
	return;
}

//Moved from Main so that Fish.hpp could access these
sf::RectangleShape moveHighlight1{sf::Vector2f(120,120)}; //Highlights starting square of the last piece moved
sf::RectangleShape moveHighlight2{sf::Vector2f(120,120)}; //Highlights ending square of last piece moved
int moveHighlight1SqID{0};
int moveHighlight2SqID{0};

sf::VideoMode desktopSize = sf::VideoMode::getDesktopMode();
int yAtBottom{ -27 }; //When the window is at this position, the bottom of window is at the bottom of the screen
int yAtTop{ -24 }; //Basically just the titlebar offset
bool windowAtTop{ false }; //Is the top of the window at the top of screen? (Titlebar is OFFSCREEN) (Or lower?)
bool windowAtBottom{ false }; //The bottom of the Window at the bottom of the screen?

//remember that sf::Texture is just a shortcut for sf::Image, which is why Texture.loadfromfile creates a new image every time it's called.
sf::Image RoyalIndicator{}; //This has to be loaded in main()?
sf::Image canCastleIndicator{};
sf::Image CastlewithIndicator{};
sf::Image canPromoteIndicator{};
//sf::Image isPromotedIndicator;

sf::RenderTexture IndicatorOverlay{};

void DrawMovecircles(sf::RenderTexture& buffer, std::vector<Move>& refMoveTable, bool isOppTurn, std::unordered_set<int>& overlaps)
{
	moveType prevMovetype{fakeMove};
	MoveCircles* MCptr{&moveCircles};
	int MC_altindex = 0; // selects from altCircles

	//if (isGameOver) { return; } //don't draw circles when game is over

	//if movecircles are disabled, instantly break the loop
	if (((isOppTurn) && (!isDrawOppMovecircles)) || (!isDrawMovecircles))
		return;

	for (const Move& refMove : refMoveTable)
	{
		if (SquareTable[refMove.targetSqID].isExcluded)
		{
			continue;
		}

		// sf::Vector2f targetPxPosition = (SquareTable[refMove.targetSqID].getPosition() + sf::Vector2f{SqWidth/2, SqHeight/2});
		const sf::Vector2f& targetPxPosition = SquareTable[refMove.targetSqID].getCenter(); //storing as a const reference will prolong the lifetime of the temporary
		
		if (MoveCircles::use_altcolors)
		{
			if ((prevMovetype != refMove.m_moveType) && (refMove.isLegal))
			{
				if (prevMovetype != fakeMove) {
					MCptr = &altCircles[MC_altindex];
					MC_altindex = ((MC_altindex+1) % altCircles.size());
				}
				prevMovetype = refMove.m_moveType;
			}
		}

		if (refMove.isLegal)
		{
			if ((isOppTurn) && !(refMove.isCapture || refMove.isPacifist))
			//if ((isOppTurn) && !refMove.isCapture)
			{ //if it's the other player's turn, all movecircles should be greyed out except captures (except hollow-move circles, like pacifist and forced-captures)
				MCptr->illegal.setPosition(targetPxPosition);
				buffer.draw(MCptr->illegal);
			}
			else if ((refMove.isCapture) || (refMove.isEnpassant)) //isEnpassant is only true if it's an actual, legal enPassant capture
			{
				MCptr->capture.setPosition(targetPxPosition);
				buffer.draw(MCptr->capture);
			}
			else if (refMove.isTypeCastle)
			{
				MCptr->castle.setPosition(targetPxPosition);
				buffer.draw(MCptr->castle);

				if (!isCastlePriority && overlaps.contains(refMove.targetSqID))
				{
					MCptr->illegal_Hollow.setPosition(targetPxPosition); //grey out the non-castle move
					buffer.draw(MCptr->illegal_Hollow);
					buffer.draw(MCptr->illegal_Hollow);
				}
				else if (isCastlePriority && overlaps.contains(refMove.targetSqID))
				{
					MCptr->priority_hollow.setPosition(targetPxPosition);
					buffer.draw(MCptr->priority_hollow);
				}
			}
			else if (refMove.isPacifist)
			{
				if (isOppTurn)
				{
					MCptr->illegal_Hollow.setPosition(targetPxPosition);
					buffer.draw(MCptr->illegal_Hollow);
				}
				else
				{
					MCptr->pacifist.setPosition(targetPxPosition);
					buffer.draw(MCptr->pacifist);
				}
			}
			else
			{
				MCptr->legal.setPosition(targetPxPosition);
				buffer.draw(MCptr->legal);

				if (isCastlePriority && overlaps.contains(refMove.targetSqID))
				{
					MCptr->illegal.setPosition(targetPxPosition); //grey out the non-castle move
					buffer.draw(MCptr->illegal);
					buffer.draw(MCptr->illegal);
				}
				else if (!isCastlePriority && overlaps.contains(refMove.targetSqID))
				{
					MCptr->priority.setPosition(targetPxPosition);
					buffer.draw(MCptr->priority);
				}
			}

		} //end of if(isLegal) block

			//These sections are not exclusive! They can be drawn alongside the previous (legal-only) movecircles!
			// If it's a forced capture, you'll want to draw it on your turn, even if it's illegal (it will be most of the time), unless it's illegal because it's capturing a friendly piece? (but not the opponent's piece)
			// If it's the opponent's turn, you'll only want to draw it if it's a legal capture (otherwise it's just represeted as a grey circle?)

			if (refMove.isForcedCapture)
			{
				if (!refMove.isLegal) //we don't want to grey out legal captures on opponent's turn
				{
					if (isOppTurn)
					{
						MCptr->illegal.setPosition(targetPxPosition);
						buffer.draw(MCptr->illegal);
					}
					else if (refMove.isDestOccupied) //if the dest is occupied but it isn't legal, it must be capturing a friendly piece
					{
						MCptr->illegal_Hollow.setPosition(targetPxPosition);
						buffer.draw(MCptr->illegal_Hollow);
					}
					else //if it's your turn, and forced-capture is illegal because the dest isn't occupied
					{
						MCptr->capture_forced.setPosition(targetPxPosition);
						buffer.draw(MCptr->capture_forced);
					}
				}
				else
				{
					MCptr->capture_forced.setPosition(targetPxPosition);
					buffer.draw(MCptr->capture_forced);
				}
			}
			if (refMove.isJump)
			{
				MCptr->jump_indicator.setPosition(targetPxPosition);
				buffer.draw(MCptr->jump_indicator);
			}

			if (!refMove.isLegal)
			{
				if (refMove.isTypeCastle || refMove.isForcedCapture) { continue; }

				if (refMove.isPacifist)
				{
					MCptr->illegal_Hollow.setPosition(targetPxPosition);
					buffer.draw(MCptr->illegal_Hollow);
				}
				// else if (!(refMove.isTypeCastle || refMove.isForcedCapture))
				else
				{
					MCptr->illegal.setPosition(targetPxPosition);
					buffer.draw(MCptr->illegal);
				}
			}
	}

	return;
}

//sf::Sprite DrawPieceIndicators(bool i_isRoyal,bool i_canCastle,bool i_canCastleWith,bool i_canPromote,sf::Vector2f i_pxPosition)
sf::Sprite DrawPieceIndicators(const std::tuple<bool,bool,bool,bool,sf::Vector2f>& pieceInfo)
{
	//re-creating the overlay is incredibly memory-intensive, apparently
	//IndicatorOverlay.create(SqWidth, SqHeight); //if you move to main, you'll need to recreate it after any resize
	IndicatorOverlay.clear(sf::Color(0,0,0,0)); //transparent

//Piece-Indicator variable order
	bool i_isRoyal;
	bool i_canCastle;
	bool i_canCastleWith;
	bool i_canPromote;
	sf::Vector2f i_pxPosition; //(from Piece::m_Sprite.getPosition());

	//doesn't actually copy, I think.
	std::tie(i_isRoyal,i_canCastle,i_canCastleWith,i_canPromote,i_pxPosition) = pieceInfo;

	//You should be able to use the global ones instead
/* 	float HorizontalScale{ SqWidth / 120 };
	float VerticalScale{ SqHeight / 120 }; */


	sf::Vector2f cornerOffset(5,SqHeight / 4.5); //size of sprites is 30x30, so they'll overlap a bit
	int R{ 1 }; //number of indicators in the bottom-left

	if ((isDrawPromotionIndicators && i_canPromote))
	{
		sf::Texture canPromoteTexture;
		canPromoteTexture.loadFromImage(canPromoteIndicator); //the image is loaded in main, before frameloop
		sf::Sprite canPromoteIndicatorSprite(canPromoteTexture);
		canPromoteIndicatorSprite.setScale(HorizontalScale,VerticalScale);
		canPromoteIndicatorSprite.setPosition(cornerOffset.x,cornerOffset.y * R); //starting Top-right
		R += 1;

		IndicatorOverlay.draw(canPromoteIndicatorSprite);
	}

	if ((isDrawRoyalIndicators) && (i_isRoyal))
	{
		sf::Texture RoyalIndicatorTexture;
		RoyalIndicatorTexture.loadFromImage(RoyalIndicator); //the image is loaded in main, before frameloop
		sf::Sprite RoyalIndicatorSprite(RoyalIndicatorTexture);
		RoyalIndicatorSprite.setScale(HorizontalScale,VerticalScale);
		RoyalIndicatorSprite.setPosition(cornerOffset.x,cornerOffset.y * R); //bottom-right
		R += 1;

		IndicatorOverlay.draw(RoyalIndicatorSprite);
	}

	if ((isDrawCastleIndicators) && i_canCastle)
	{
		sf::Texture canCastleTexture;
		canCastleTexture.loadFromImage(canCastleIndicator); //the image is loaded in main, before frameloop
		sf::Sprite canCastleIndicatorSprite(canCastleTexture);
		canCastleIndicatorSprite.setScale(HorizontalScale,VerticalScale);
		canCastleIndicatorSprite.setPosition(cornerOffset.x,cornerOffset.y * R); //starting Bottom-left
		R += 1;

		IndicatorOverlay.draw(canCastleIndicatorSprite);
	}

	if ((isDrawCastleIndicators) && i_canCastleWith)
	{
		sf::Texture CastlewithTexture;
		CastlewithTexture.loadFromImage(CastlewithIndicator); //the image is loaded in main, before frameloop
		sf::Sprite CastlewithIndicatorSprite(CastlewithTexture);
		CastlewithIndicatorSprite.setScale(HorizontalScale,VerticalScale);
		CastlewithIndicatorSprite.setPosition(cornerOffset.x,cornerOffset.y * R); //starting Bottom-left
		R += 1;

		IndicatorOverlay.draw(CastlewithIndicatorSprite);
	}


	IndicatorOverlay.display();
	sf::Sprite returnsprite(IndicatorOverlay.getTexture());
	returnsprite.setPosition(i_pxPosition);
	return returnsprite;
}

