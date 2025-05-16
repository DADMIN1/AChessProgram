#ifndef TEXT_STORAGE_HPP_INCLUDED
#define TEXT_STORAGE_HPP_INCLUDED

#include <SFML/Graphics.hpp>

#include <vector>

struct Text_Storage
{
	sf::Font dejaVu; //for some reason loading a font segfaults if it's static.
	static sf::Font testFont;

	//from the top of main
	sf::Text DEBUG_SqID; // This has been changed to display each square's SquareID at all times, instead of the selected square
	sf::Text DEBUG_piecehover; // Shows the name of the piece hovered over
	sf::Text setup_piecename; //This one is for drawing the names above pieces in setupwindow
	sf::Text setup_mousename; //This is for displaying the name of the mousepiece in the boardwindow during setupmode

	// inside main
	std::vector<sf::Text> DEBUG_SqIDstorage; //stores DEBUG/SqText for each square. Handled in main
	sf::Text DEBUG_mousecoord; //This displays the algCoord, Column, and Row of the square your mouse is hovering over
	sf::Text gameover;

	sf::Text checkmark;

	// colorWindowText;

	static void LoadStaticFonts(); //must be called inside of Main // STILL doesn't work properly (no segfault, the fonts still don't show up anywhere)
	Text_Storage();
};

//eventually, you should replace all this garbage with a thor::resourceLoader instead

extern sf::Text fpsText; //in FPSmath

void InitFPS_Text(); //must be called from Main.

#endif
