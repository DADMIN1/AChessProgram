#ifndef ARROW_OVERLAY_HPP_INCLUDED
#define ARROW_OVERLAY_HPP_INCLUDED

#include "GlobalVars.hpp"
#include "ArrowShape.hpp"
#include "DrawArrow.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp> //Graphics and Window are actually seperate modules.


class ArrowOverlay : public sf::RenderTexture, public sf::Sprite
{
public:
	std::vector<ArrowShape*> m_arrows;
	bool m_isVisible{true};
	bool m_isFlipped{false};

	//sf::RenderWindow* mp_parentWindow {nullptr};
	static ArrowOverlay* activeOverlay; //points to the overlay being used to draw
	Arrow_Creator arrowCreator{};

public:
	ArrowOverlay();

	void AddArrow(ArrowShape* theArrow); //add a finished arrow to the overlay
	void RotateFlip(bool shouldFlip); //pass "isFlipboard" to this function.
	void ClearAll();

	void RedrawArrows(); //You can't ever call this function; it'll screw up every arrow that wasn't drawn with the current window size.
	void UpdateToWindow(sf::RenderWindow* window); //unimplemented
};


#endif
