#ifndef DRAW_ARROW_HPP_INCLUDED
#define DRAW_ARROW_HPP_INCLUDED

#include "ArrowShape.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp> //Graphics and Window are actually seperate modules.

#include <Thor/Input.hpp> //for actionmaps/callback

#include <string>

//struct for callbacks?
void OnMouseDrag(thor::ActionContext<std::string> context);

// Needed? Functions:
// GetHoveredSq
// GetHoveredPiece
// InsertIntoOverlay //when you're finished drawing

//class responsible for creating/drawing new arrows (with the mouse)
class Arrow_Creator
{	
public:
	static bool isDrawingMode; //if this is false, the class/object is basically inactive
	static ArrowShape m_arrow; //must be static to be called by "OnMouseDrag"
	
	//sf::RenderWindow* mp_parentWindow;
	static sf::Vector2i m_mousePosition;

	//these must be static to operate on m_arrow
	static void EnterDrawingState(thor::ActionContext<std::string> context);
	static void ExitDrawingState(thor::ActionContext<std::string> context); //can't return anything other than void if it's used as a callback
	static void UpdateArrow(sf::Vector2i& mousePosition);

	thor::ActionMap<std::string> m_actionMap; //this could use enums instead
	thor::ActionMap<std::string>::CallbackSystem m_callbacks;
	//actionmap maps identifiers (strings/enums) to triggers (thor::Action, made out of sf::Events or other thor::Actions)
	//CallbackSystem maps the IDs from actionmap to a callback-function

	// Arrow_Creator(sf::RenderWindow* parentWindow);
	Arrow_Creator();
	void SendCallback(sf::Window*);
	friend void OnMouseDrag(thor::ActionContext<std::string> context);

};

#endif
