#include "TextStorage.hpp"

#include <cassert>
#include <iostream>

//sf::Font Text_Storage::dejaVu; //static member
sf::Font Text_Storage::testFont; //static member

// DO NOT REMOVE the duplicated "loadFromFile" statements
// They're necessary because the "assert" statements get removed in the release-build, which would cause the fonts to never get loaded

void InitFPS_Text()
{
	assert(Text_Storage::testFont.loadFromFile("Resource/Fonts/dejavu/DejaVuSerifCondensed.ttf") && "failed to load test font");
	Text_Storage::testFont.loadFromFile("Resource/Fonts/dejavu/DejaVuSerifCondensed.ttf");
	fpsText.setFont(Text_Storage::testFont);
	fpsText.setString("calculating... \n");
	fpsText.setFillColor(sf::Color::Green);
	fpsText.setOutlineColor(sf::Color::Black);
	fpsText.setOutlineThickness(3);
	fpsText.setCharacterSize(16);
	return;
}

void Text_Storage::LoadStaticFonts() //doesn't really do anything
{
	assert(Text_Storage::testFont.loadFromFile("Resource/Fonts/dejavu/DejaVuSerifCondensed.ttf") && "failed to load test font");
	Text_Storage::testFont.loadFromFile("Resource/Fonts/dejavu/DejaVuSerifCondensed.ttf");
}

Text_Storage::Text_Storage()
{
	assert(dejaVu.loadFromFile("Resource/Fonts/dejavu/DejaVuSerifCondensed.ttf") && "failed to load dejaVu font");
	dejaVu.loadFromFile("Resource/Fonts/dejavu/DejaVuSerifCondensed.ttf");
	//assert(testFont.loadFromFile("Resource/Fonts/dejavu/DejaVuSerifCondensed.ttf") && "failed to load test font"); //segfault
	testFont = dejaVu; //prevents segfaults, but it doesn't seem to work properly. Must reload file in Main (by calling InitFPS_Text, for example) before it works.

	// sf::Font::loadFromFile cannot be called on a static member from outside of main(); segfaults.
	// I think it's because the function ends before the loading is done?
	// from the SFML font reference: "SFML cannot preload all the font data in this function, so the file has to remain accessible until the sf::Font object loads a new font or is destroyed."

	//InitFPS_Text(); //for some reason this doesn't work here. Segfaults unless the "testFont = dejaVu" line is present

	// Debug text showing each square's ID-number
	DEBUG_SqID.setFont(dejaVu);
	DEBUG_SqID.setCharacterSize(48);
	DEBUG_SqID.setFillColor(sf::Color(0, 0, 255));
	DEBUG_SqID.setPosition(0, 0);
	DEBUG_SqID.setStyle(sf::Text::Bold);

	DEBUG_piecehover.setFont(dejaVu); //for displaying the piecename of the piece that you're hovering over (or have selected)
	DEBUG_piecehover.setCharacterSize(48);
	DEBUG_piecehover.setFillColor(sf::Color(255, 0, 0));
	DEBUG_piecehover.setOutlineColor(sf::Color(0, 0, 0));
	DEBUG_piecehover.setOutlineThickness(1);
	DEBUG_piecehover.setStyle(sf::Text::Bold);

	DEBUG_mousecoord.setFont(dejaVu);
	DEBUG_mousecoord.setCharacterSize(64); // in pixels, not points.
	DEBUG_mousecoord.setFillColor(sf::Color(0, 200, 0));
	DEBUG_mousecoord.setOutlineThickness(4);
	DEBUG_mousecoord.setOutlineColor(sf::Color::Black);
	DEBUG_mousecoord.setString("a1");

	//This one is for drawing the names above pieces in setupwindow
	setup_piecename = DEBUG_mousecoord;
	setup_piecename.setCharacterSize(24);
	setup_piecename.setFillColor(sf::Color(255, 125, 0));
	setup_piecename.setStyle(sf::Text::Bold);
	setup_piecename.setOutlineColor(sf::Color(0, 0, 0));
	setup_piecename.setOutlineThickness(4);
	setup_piecename.setStyle(sf::Text::Italic);

	//This is for displaying the name of the mousepiece in the boardwindow during setupmode
	setup_mousename = setup_piecename;
	setup_mousename.setOrigin(30, -30);
	setup_mousename.setFillColor(sf::Color(75, 75, 255));
	setup_mousename.setOutlineColor(sf::Color::Blue);

	gameover.setFont(dejaVu);
	gameover.setCharacterSize(128);
	gameover.setFillColor(sf::Color(255, 0, 0));
	gameover.setOutlineColor(sf::Color(0, 0, 0));
	gameover.setOutlineThickness(5);
	gameover.setStyle(sf::Text::Bold | sf::Text::Italic);
	gameover.setString("Game Over");
	gameover.setOrigin(480, 48); // sets origin at center of text (96px font * 5 ,  96/2)
	//gameover.setOrigin(640, 64); // it should be this now that character size is 128?????

	checkmark = sf::Text("!", dejaVu, 64);
	checkmark.setFillColor(sf::Color(255, 0, 0));
	checkmark.setOutlineColor(sf::Color(0, 0, 0));
	checkmark.setOutlineThickness(3);
	checkmark.setRotation(-10);


	return;
}

