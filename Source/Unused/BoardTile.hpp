#ifndef BOARDTILE_HPP_INCLUDED
#define BOARDTILE_HPP_INCLUDED

//project headers
#include "ColorManager.hpp"
#include "Board.hpp"

//3rd party headers
#include <SFML/Graphics.hpp> //for sf::Shape class

//std libs
#include <cmath> //std::sin and std::cos
#include <vector>

//when you inherit from sf::Shape, you need to override getPointCount() and getPoint()
	//is this still true for convexshape? I don't think so.
// also, sf::Shape::update() must be called whenever any of the shape's points change

//Relevant: https://www.sfml-dev.org/tutorials/2.5/graphics-shape.php
//Also Relevant: https://www.sfml-dev.org/tutorials/2.5/graphics-vertex-array.php

// do NOT inherit from sf::Transformable / sf::Drawable; they're inaccessible when inheriting Boardsquare (apparently they're ambiguous, even when virtual)
// inherits Boardsquare from 'Board.hpp', not the class below (BoardTile)
class CustomTile : public sf::ConvexShape //, public Boardsquare
{
	//The shape gets inscribed in a minimal enclosing circle, given a side_length
	float side_length;
	float interior_angle;
	float radius; // calculated based on the other properties
	// if you just set radius = side_length instead, that's identical to 'BoardTile' below
	
private:
	const int m_ID; //if we're chaning the board size during gameplay, this will probably have to not be const?
	const int m_colorIndex; //index ColorManger's tileColors with this number to get the color.
	sf::Text m_coord;
	bool m_isOccupied;
	
	//sf::VertexArray m_vertices{};
	//sf::Texture m_texture{};
	
	/*virtual void draw(sf::RenderTarget& target,sf::RenderStates states) const
	{
		// apply the entity's transform -- combine it with the one that was passed by the caller
		// 'base class: sf::Transformable is ambiguous' <-- what the hell??? somehow calling it from Boardsquare isn't???
		states.transform *= getTransform(); // getTransform() is defined by sf::Transformable

		// apply the texture
		states.texture = &m_texture;

		// you may also override states.shader or states.blendMode if you want

		// draw the vertex array
		target.draw(m_vertices,states);
	}*/

public:

	// implementing the SFML 'draw' function for this class
	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
	void FetchColors(); //fetch all values from ColorManager. This includes coord-text.
	void Rotate(float angle);
	
	virtual sf::Vector2f getPoint(std::size_t index) const
	{
		static const float pi = 3.141592654f;

		float angle = index * 2 * pi / getPointCount() - pi / 2;
		float x = std::cos(angle) * radius;
		float y = std::sin(angle) * radius;

		return sf::Vector2f((radius + x), (radius + y));
	}

	CustomTile(int id, std::string nameString, sf::Vector2f position, int colorIndex, float width = 96, u_long sides = 4)
		: sf::ConvexShape(sides), //Boardsquare::Boardsquare(id,1,id),
		side_length{width},
		radius{width},
		m_ID{ id },
		m_colorIndex{ colorIndex },
		m_coord{ nameString , ColorManager::s_coordFont , ColorManager::s_coordSize },
		m_isOccupied{ false }
	{
		//colorindex will always = (id % (sides/2)), because that's how many unique colors we need?
			//we'll have to modify the color-array so that the lightsq-color is always last, instead of second
		
		setOrigin(width,width);
		setRotation(180 / sides); //orientate it so that orthogonals are always up //only if it has even number of sides
		setPosition(position);
		//move(width,width);
		move(60,60); // half of 120 (real default squaresize)

		FetchColors();
		m_coord.move(position);
		m_coord.setOrigin(48,16);
		m_coord.move(48,16);
	}
};


class BoardTile : virtual public sf::CircleShape //I don't want to deal with setting points. Instead we'll have to set rotation.
{
private:
	const int m_ID; //if we're chaning the board size during gameplay, this will probably have to not be const?
	const int m_colorIndex; //index ColorManger's tileColors with this number to get the color.
	sf::Text m_coord;
	bool m_isOccupied;

public:

	// implementing the SFML 'draw' function for this class
	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
	void FetchColors(); //fetch all values from ColorManager. This includes coord-text.
	void Rotate(float angle);

	BoardTile(int id, std::string nameString, sf::Vector2f position, int colorIndex, float radius = 96, u_long sides = 4)
		: //sf::ConvexShape{ sides },
		sf::CircleShape{ radius, sides }, //tile size should actually be (radius-outlineThickness)
		m_ID{ id },
		m_colorIndex{ colorIndex },
		m_coord{ nameString , ColorManager::s_coordFont , ColorManager::s_coordSize },
		m_isOccupied{ false }
	{
		//colorindex will always = (id % (sides/2)), because that's how many unique colors we need?
			//we'll have to modify the color-array so that the lightsq-color is always last, instead of second
		setOrigin(radius,radius);
		setRotation(180 / sides); //orientate it so that orthogonals are always up //only if it has even number of sides
		setPosition(position);
		//move(radius,radius);
		move(60,60); // half of 120 (real default squaresize)

		FetchColors();
		m_coord.move(position);
		m_coord.move(radius,radius);
		m_coord.setOrigin(32,16);
	}
};

#endif
