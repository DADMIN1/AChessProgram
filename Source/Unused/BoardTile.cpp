#include "BoardTile.hpp"


void BoardTile::FetchColors()
{
	setFillColor(ColorManager::s_tileColors[m_colorIndex]);
	setOutlineColor(ColorManager::s_outlineColors[m_colorIndex]);
	setOutlineThickness(ColorManager::s_outlineWidth);

	m_coord.setFillColor(ColorManager::s_coordColors[m_colorIndex]);
	//m_coord.setCharacterSize(ColorManager::s_coordSize); //This is done in the constructor
}

void CustomTile::FetchColors()
{
	sf::ConvexShape::setFillColor(ColorManager::s_tileColors[m_colorIndex]);
	sf::ConvexShape::setOutlineColor(ColorManager::s_outlineColors[m_colorIndex]);
	sf::ConvexShape::setOutlineThickness(ColorManager::s_outlineWidth);

	m_coord.setFillColor(ColorManager::s_coordColors[m_colorIndex]);
	//m_coord.setCharacterSize(ColorManager::s_coordSize); //This is done in the constructor
}

void BoardTile::Rotate(float angle)
{
	rotate(angle);
	m_coord.rotate(angle);
	return;
}

void CustomTile::Rotate(float angle)
{
	rotate(angle);
	m_coord.rotate(angle);
	return;
}

void BoardTile::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	target.draw(sf::CircleShape{*this}, states);
	target.draw(m_coord, states);
	return;
}

void CustomTile::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	target.draw(sf::ConvexShape{*this}, states);
	target.draw(m_coord, states);
	return;
}

