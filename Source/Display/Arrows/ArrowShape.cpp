#include "ArrowShape.hpp"

//#include <thread>

ArrowShape::ArrowShape()
	: thor::Arrow{
		sf::Vector2f(480, 480),
		sf::Vector2f(96, 96),
		// sf::Color::Blue,
		sf::Color(0x0000FF88),
		20.f} //origin(position), vector defining arrow, color, thickness
{
	//m_fromSq = nullptr;
	//m_toSq = nullptr;
	return;
}
