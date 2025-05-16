#include "BoardTile.hpp"


void BoardTile::FetchColors()
{
	setFillColor(ColorManager::s_tileColors[m_colorIndex]);
	setOutlineThickness(ColorManager::s_outlineWidth);
	setOutlineColor(ColorManager::s_coordColors[m_colorIndex]);

	m_coord.setFillColor(ColorManager::s_coordColors[m_colorIndex]);
	//m_coord.setCharacterSize(ColorManager::s_coordSize); //This is done in the constructor
}

void CustomTile::FetchColors()
{
	sf::ConvexShape::setFillColor(ColorManager::s_tileColors[m_colorIndex]);
	sf::ConvexShape::setOutlineThickness(ColorManager::s_outlineWidth);
	sf::ConvexShape::setOutlineColor(ColorManager::s_coordColors[m_colorIndex]);

	m_coord.setFillColor(ColorManager::s_coordColors[m_colorIndex]);
	//m_coord.setCharacterSize(ColorManager::s_coordSize); //This is done in the constructor
}

/*void FetchColors(sf::Shape& D, std::size_t m_colorIndex)
{
	D.setFillColor(ColorManager::s_tileColors[m_colorIndex]);
	D.setOutlineThickness(ColorManager::s_outlineWidth);
	D.setOutlineColor(ColorManager::s_coordColors[m_colorIndex]);

	m_coord.setFillColor(ColorManager::s_coordColors[m_colorIndex]);
	//m_coord.setCharacterSize(ColorManager::s_coordSize); //This is done in the constructor
}
*/
