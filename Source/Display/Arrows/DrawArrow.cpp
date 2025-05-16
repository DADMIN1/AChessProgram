#include "DrawArrow.hpp"
#include "ArrowOverlay.hpp"

#include <iostream>
//#include <functional>

thor::Action mouseClickR(sf::Mouse::Right, thor::Action::PressOnce);
thor::Action mouseHoldR(sf::Mouse::Right, thor::Action::Hold);
thor::Action mouseMove(sf::Event::MouseMoved);
thor::Action mouseDragR = {mouseHoldR && mouseMove};
// mouseRelease to finish drawing
// any click/mouse_button to cancel drawing

ArrowShape Arrow_Creator::m_arrow{}; //defining static member variable
bool Arrow_Creator::isDrawingMode{false};
sf::Vector2i Arrow_Creator::m_mousePosition;

void OnMouseDrag(thor::ActionContext<std::string> context)
{
	sf::Vector2i& mousePosition = Arrow_Creator::m_mousePosition;
	// context.window is a pointer to a sf::Window. It can be used for relative mouse input as follows:
	mousePosition = sf::Mouse::getPosition(*context.window);
	// Make sure the mouse is still within the window!! //Do we actually need to?

	if (isDEBUG)
		std::cout << "mousedrag callback! {" << mousePosition.x << ' ' << mousePosition.y << "}\n";

	if (Arrow_Creator::isDrawingMode)
	{
		sf::RenderWindow* realWindow = static_cast<sf::RenderWindow*> (context.window);
		Arrow_Creator::UpdateArrow(mousePosition);
		realWindow->draw(Arrow_Creator::m_arrow);
	}
	
	return;
}


void Arrow_Creator::EnterDrawingState(thor::ActionContext<std::string> context)
{
	if (isDEBUG)
		std::cout << "Entering Drawing State\n";

	if (isDrawingMode)
	{
		std::cerr << "Error: Drawing-State was already active!\n";
		ExitDrawingState(context);
		return;
	}

	m_mousePosition = sf::Mouse::getPosition(*context.window);
	//if (!isDrawingMode) //this shouldn't be conditional
	{
		isDrawingMode = true;
		m_arrow.setPosition(sf::Vector2f{m_mousePosition});
		m_arrow.setDirection(0,0);
		m_arrow.setThickness(20.f * HorizontalScale); //ideally this would be set in the constructor instead (doesn't work there because we're just re-using the same arrow)
	}
	return;
}

void Arrow_Creator::ExitDrawingState(thor::ActionContext<std::string> context)
{
	if (isDEBUG)
		std::cout << "Exiting Drawing State\n";

	if (isDrawingMode)
	{
		m_mousePosition = sf::Mouse::getPosition(*context.window);
		UpdateArrow(m_mousePosition);
		ArrowOverlay::activeOverlay->AddArrow(&m_arrow);
	}
	isDrawingMode = false;
	return;
}

//you could just make Arrow_Creator a friend class
void Arrow_Creator::UpdateArrow(sf::Vector2i& mousePosition)
{
	m_arrow.setDirection(sf::Vector2f{mousePosition} - m_arrow.getPosition());
}

/* Arrow_Creator::Arrow_Creator(sf::RenderWindow* parentWindow)
	: mp_parentWindow{parentWindow} */
Arrow_Creator::Arrow_Creator()
{
	m_actionMap["mouseDragR"] = mouseDragR;
	m_actionMap["mouseHoldR"] = (thor::Action(sf::Mouse::Right, thor::Action::Hold)); // m_actionMap["mouseHoldR"] = mouseHoldR;
	m_actionMap["mouseReleaseR"] = (thor::Action(sf::Mouse::Right, thor::Action::ReleaseOnce));
	m_actionMap["mouseClickR"] = mouseClickR;

	// m_callbacks.connect("mouseDragR", &OnMouseDrag); //for some reason this rarely triggers; maybe 1-3 times per second?
	 m_callbacks.connect("mouseHoldR", &OnMouseDrag);
	 // thor callbacks cannot accept nonstatic member functions.

	 m_callbacks.connect("mouseClickR", &EnterDrawingState);
	 m_callbacks.connect("mouseReleaseR", &ExitDrawingState);
}

void Arrow_Creator::SendCallback(sf::Window* W)
{
	//m_actionMap.update(*mp_parentWindow); //pass whichever window the events happened in
	// m_actionMap.invokeCallbacks(m_callbacks, mp_parentWindow);
	m_actionMap.invokeCallbacks(m_callbacks, W);
	m_actionMap.clearEvents(); //if we don't call this the single click-events/actions keep triggering every frame and the arrow's origin moves to follow the mouse.

	//m_actionMap.update seems to be interfering with the event processing; selecting pieces sometimes doesn't work because it's somehow eating the right-click hold/release state? Quickly tapping right and left click on top of a piece can cause the piece to become stuck attatched to the mouse. Another glitch; Right-clicking a piece, then left-clicking another piece will prevent it from being selected.
		//This is because it erases and repolls window events; to avoid this, just pushEvents to the map from the boardWindow's pollevent switch in Main. That fixed both of these glitches

	//May need to define custom events for entering/exiting drawing mode.
	//sf::Event::EventType customEvent;
	//customEvent = static_cast<sf::Event::EventType>(6969);

	return;
}
