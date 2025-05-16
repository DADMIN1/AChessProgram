#ifndef HIGHLIGHTS_INDICATORS_HPP_INCLUDED
#define HIGHLIGHTS_INDICATORS_HPP_INCLUDED

#include <SFML/Graphics.hpp>

#include <vector>
#include <tuple> //getPieceIndicators
#include <unordered_set>

class Move; //passed to getMoveCircles

extern bool isDrawOppMovecircles;
extern bool isDrawMovecircles;
extern bool isCastlePriority; //defined in main

extern sf::RectangleShape moveHighlight1;
extern sf::RectangleShape moveHighlight2;
extern int moveHighlight1SqID; //defined in Main
extern int moveHighlight2SqID;

extern sf::VideoMode desktopSize;
extern int yAtBottom;
extern int yAtTop;
extern bool windowAtTop;
extern bool windowAtBottom;

struct MoveCircles
{
	sf::CircleShape legal; //this constructor relies on SqWidth
	sf::CircleShape illegal;
	sf::CircleShape illegal_Hollow;
	sf::CircleShape pacifist;
	sf::CircleShape capture;
	sf::CircleShape castle;
	sf::CircleShape capture_forced;

	sf::Texture jump_texture;
	sf::Sprite jump_indicator;

	//These two indicate whether castling will occur when moving the king a 1-square distance
	sf::CircleShape priority;
	sf::CircleShape priority_hollow;

	MoveCircles(std::size_t index=0);
	void RescaleAll(float H_scale, float V_scale);
	static void RescaleAlt(float H_scale, float V_scale);
	static bool use_altcolors; // cycle colors to more easily distinguish movement patterns
};

extern MoveCircles moveCircles; //this has to be defined globally for the "RescaleEverything" function
// the fact that MoveCircle's constructor relies on sqWidth, and also needs to be defined globally, potentially creates problems based on include-order
extern std::array<MoveCircles, 3> altCircles;

extern sf::RectangleShape dangermapHighlight;

extern sf::Image RoyalIndicator; //This has to be loaded in main()?
extern sf::Image canCastleIndicator;
extern sf::Image CastlewithIndicator;
extern sf::Image canPromoteIndicator;

extern sf::RenderTexture IndicatorOverlay;

// pass a piece's movetable and the movecircles will be drawn to the passed rendertexture.
void DrawMovecircles(sf::RenderTexture& move_Highlight_Buffer, std::vector<Move>& ref_Move_Table, bool is_Opp_Turn, std::unordered_set<int>& overlapping_Moves);
// the "moveHighlightBuffer" is defined in Main.

sf::Sprite DrawPieceIndicators(const std::tuple<bool, bool, bool, bool, sf::Vector2f>&); // draws to IndicatorOverlay
// uses a tuple so we don't need to include Piece.hpp



#endif
