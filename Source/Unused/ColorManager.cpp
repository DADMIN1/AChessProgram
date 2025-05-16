#include "ColorManager.hpp"

//Initializing Square-color variables
std::vector<sf::Color> ColorManager::s_tileColors{ sf::Color(0x644B3BFF) , sf::Color(0xC89678FF) };
std::vector<sf::Color> ColorManager::s_coordColors{ ColorManager::s_tileColors[1] , ColorManager::s_tileColors[0] }; //opposite of square colors
uint ColorManager::s_coordSize{ 16 }; 
std::vector<sf::Color> ColorManager::s_outlineColors{ sf::Color(0x000000FF) , sf::Color(0x000000FF) };
float ColorManager::s_outlineWidth{ 1.f };

//Highlight colors
std::vector<sf::Color> ColorManager::s_moveHighlightColors{ sf::Color(200,200,80,75) , sf::Color(200,200,80,75) }; //starting square, destination
std::vector<sf::Color> ColorManager::s_selectionHighlightColors{ sf::Color(0xFFFFFF7D) , sf::Color(255,218,0,175) }; //white for selected square, gold for selected piece (in setup window)

//Color-linking flags
bool ColorManager::s_isCoordColorLinked{ true };
bool ColorManager::s_isOutlineColorLinked{ false }; //Outline normally just stays black
bool ColorManager::s_isLSD_MODE{ false };


sf::Font ColorManager::s_coordFont; //this definition is required




//Class functions//

void ColorManager::ActivateLSD(bool activate)
{
	ColorManager::s_isLSD_MODE = activate;
	//you should probably just an RGB/LSD shader rather than changing around all the colors every frame.
	return;
}


void ColorManager::SetLastMoveColors(sf::Color startColor, sf::Color destColor, uint halfmoves)
{
//halfmoves must be at least 1!!!

	ColorManager::s_moveHighlightColors[0] = startColor;
	ColorManager::s_moveHighlightColors[1] = destColor;
	ColorManager::s_moveHighlightColors.resize(halfmoves * 2); //because two highlights are used on each halfmove
//I'm assuming that the resize filled the vector with startColor or something?

/* 	sf::Color rgbDiff = startColor - destColor;

	for (int A{ 1 }; A < halfmoves * 2; A++)
	{
		rgbDiff += sf::Color(A * 16,A * 16,A * 16, A*16); //every additional highlight gets lighter and more transparent
	} */

	return;
}

void ColorManager::LoadHighlightShader()
{
	//This isn't necessary for highlights though?
	return;
}