#ifndef GLOBAL_VARS_HPP_INCLUDED
#define GLOBAL_VARS_HPP_INCLUDED

/*
If you change the initial value of these from 8, the scaling of icons and territory lines and stuff gets screwed up
when you resize the board, which isn't the case if you start with 8x8. My guess is that there are some initial values assumed to be
120, or that they get a size from SqWidth/SqHeight and then never update it.
*/
extern int numColumns;
extern int numRows;
extern float winWidth;
extern float winHeight;
extern float SqWidth; // These are the dimensions of the squares in pixels.
extern float SqHeight;  // (8/numColumns) should give the same result, but it doesn't work for some reason.
extern bool SqSizeChanged; //Basically a flag to resize/rescale all visual elements. Unimplemented. Probably should default to true.

extern bool isDrawTerritory;
extern bool isDrawCastleIndicators;
extern bool isDrawRoyalIndicators;
extern bool isDrawPromotionIndicators;
extern bool isIndicatorsEnabled; //master-switch for all indicators (including territory lines)
//end copied from CustomRendering.hpp

//MOVED FROM BOARD.H
constexpr int MIN_BOARD_DIMENSIONS{3};
constexpr int MAX_BOARD_DIMENSIONS{26};
extern float HorizontalScale; // These are the global scaling factors for Sprites, Squares, Text, etc.
extern float VerticalScale;	  // Calculated as (SqWidth/120) or (SqHeight/(960?/numRows)), the denominator should be 120? If you change the window width from 960, you'll just get 1 every time?

extern bool isDEBUG;
extern bool DEBUG_DisableTurns;

extern int wTerritory; //row that white's territory ends on.
extern int bTerritory; //territory determines when a pawn can double-move, and where pieces can be placed

extern float SqOffsetx; //basically unused now, repurposed for Flipboard calculations
extern float SqOffsety;
extern bool isFlipboard;
extern bool isFlipPerTurn;

extern bool isPieceSelected;
extern bool isSquareSelected;

extern bool default_promoteInTerritory;

#endif
