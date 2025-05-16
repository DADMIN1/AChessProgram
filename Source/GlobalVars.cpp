#include "GlobalVars.hpp"

int numColumns{8};
int numRows{ 8 };
//float SqWidth{ winWidth / numColumns }; // These are the dimensions of the squares in pixels.
//float SqHeight{ winHeight / numRows };  // (8/numColumns) should give the same result, but it doesn't work for some reason.
float SqWidth{ 120 }; // These are the dimensions of the squares in pixels.
float SqHeight{ 120 };  // (8/numColumns) should give the same result, but it doesn't work for some reason.
float winWidth{ SqWidth * numColumns };
float winHeight{ SqHeight * numRows };
bool SqSizeChanged{ false }; //Basically a flag to resize/rescale all visual elements. Unimplemented. Probably should default to true.

bool isDrawTerritory{ true };
bool isDrawCastleIndicators{ true };
bool isDrawRoyalIndicators{ true };
bool isDrawPromotionIndicators{ false };
bool isIndicatorsEnabled{ true }; //master-switch for all indicators (including territory lines)
//end copied from CustomRendering.hpp

//COPIED FROM BOARD.H
//int PromotionRankBlack{ 1 };
//int PromotionRankWhite{ numRows }; //moved to PromotionRules
float HorizontalScale{ 1 }; // These are the global scaling factors for Sprites, Squares, Text, etc.
float VerticalScale{ 1 };	  // Calculated as (SqWidth/120) or (SqHeight/(960?/numRows)), the denominator should be 120? If you change the window width from 960, you'll just get 1 every time?

bool isDEBUG{false};
bool DEBUG_DisableTurns{false};

//These are actually determined by the generateboard function. Ignore these values
int wTerritory{ 2 }; //row that white's territory ends on.
int bTerritory{ 7 }; //territory determines when a pawn can double-move, and where pieces can be placed

float SqOffsetx{}; //basically unused now, repurposed for Flipboard calculations
float SqOffsety{};
bool isFlipboard{false};
bool isFlipPerTurn{false}; //determines whether the board flips every turn

bool isPieceSelected{ false };
bool isSquareSelected{ false };

bool default_promoteInTerritory{false};
