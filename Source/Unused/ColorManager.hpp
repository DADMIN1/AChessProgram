#ifndef COLORMANAGER_HPP_INCLUDED
#define COLORMANAGER_HPP_INCLUDED

//3rd party headers
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>

//std libs
#include <vector>


//Should we actually have a non-static ColorManager object to color two boards differently?
	//Chessboard-class should have a ColorManager member?
class ColorManager
{

public:
	//the size of each vector = floor(#TileEdges / 2)
		static std::vector<sf::Color> s_tileColors; //the first two are darkest and lightest square colors (a1 is always dark)
		static std::vector<sf::Color> s_coordColors; //coord colors are opposite of square color by default
		static sf::Font s_coordFont;
		static uint s_coordSize; //sf::Text takes a uint, not a float
		static std::vector<sf::Color> s_outlineColors; //each square-color can have a unique outline color
		static float s_outlineWidth; //If this changes we need to shift the tiles' position to prevent overlap.

		//highlights
		static std::vector<sf::Color> s_moveHighlightColors; //first is origin square, second is destination
			//lastMove highlights will iterate two positions in the vector each halfmove; expand the vector to enable movement-trails.
		static std::vector<sf::Color> s_selectionHighlightColors; //first is selected square highlight, second is highlight for selection in setupWindow

		//Color-linking flags
		static bool s_isCoordColorLinked; //if true, modifying square-colors will also modify their associated coordcolor.
		static bool s_isOutlineColorLinked;
		static bool s_isLSD_MODE;
		
	//all functions should be static, so I don't actually have to make a ColorManager object to invoke them.
	//however, static functions cannot touch non-static variables
		
		static void SetLastMoveColors(sf::Color startColor, sf::Color destColor, uint halfmoves=2); //interpolates between start and end colors.

		//void saveCurrentColors(std::string presetName);
		//void loadFromFile(std::string presetName);
		static void LoadHighlightShader(); //update the shader with values of highlight-colors
			//do we actually need a shader? We can just draw a transparent color to a rendertexture and overlay it?
		static void ActivateLSD(bool); //this definitely should be done with a shader

};




#endif
