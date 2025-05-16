#include "Colorsliders.hpp"

//RGBA 0-255; 0=black
//alpha channel is fully transparent at 255

//{0xBEA091FF}
sf::Color lightsqColor{ 190, 160, 145 };
//{0x644B3CFF}
sf::Color darksqColor{ 100, 75, 60 };

//should these really be seperate?
sf::Color outlinecolorLight{ 0, 0, 0 };
sf::Color outlinecolorDark{ 0, 0, 0 };
//sf::Color darksqOutlinecolor(0x000000FF);
float sqOutlineThicknessD{1.f};
float sqOutlineThicknessL{1.f};
int sqOutlineSignDark{-1}; //either 1 or -1, used to invert outline-thickness (grows into square instead of overlapping other squares)
int sqOutlineSignLight{-1};

sf::Color coordcolorLight{ lightsqColor };
sf::Color coordcolorDark{ darksqColor };
int coordTextsize{16};
std::vector<sf::Text> coordText; //previously in main
std::vector<int> darkSqIDs;
std::vector<int> lightSqIDs;
sequenced_set wasModified;

bool isLSDmode{false};
//lastmoveHighlights colors?

bool isEditingLightColors{ true };	//determines which slider set is active
sf::FloatRect colorWindowSwitchButton{ 171, 7, 59, 17 }; //bounding box for "Switch (colors)"

//These aren't really implemented right now.
bool isOutlineColorShared{true}; //does each color square have it's own outline color? Shared color by default
bool isCoordtextColorlinked{ true }; //is the color of the inlaid coordinates linked to the color of the opposite square?
sf::FloatRect SqoutlinecolorTogglebox{ 227, 231, 20, 20 };
sf::FloatRect CoordtextcolorTogglebox{ 228, 129, 20, 20 }; //bounding boxes

sf::Text colorWindowText{}; //tells you which color you're editing

std::vector<sliderClass> colorWindowSliders{};
sliderClass* selectedSlider{nullptr};

//the variables associated with various SliderIDs
std::map<std::pair<SliderID,subSliderID>,sf::Uint8&> getColorAssociationMap()
{
	return std::map<std::pair<SliderID,subSliderID>,sf::Uint8&>{
		{ {sqColorlight, R}, lightsqColor.r },
		{ {sqColorlight, G}, lightsqColor.g },
		{ {sqColorlight, B}, lightsqColor.b },
		{ {sqColordark, R}, darksqColor.r },
		{ {sqColordark, G}, darksqColor.g },
		{ {sqColordark, B}, darksqColor.b },
		{ {outlineLight, R}, outlinecolorLight.r },
		{ {outlineLight, G}, outlinecolorLight.g },
		{ {outlineLight, B}, outlinecolorLight.b },
		{ {outlineDark, R}, outlinecolorDark.r },
		{ {outlineDark, G}, outlinecolorDark.g },
		{ {outlineDark, B}, outlinecolorDark.b },
		{ {coordtextLight, R}, coordcolorLight.r },
		{ {coordtextLight, G}, coordcolorLight.g },
		{ {coordtextLight, B}, coordcolorLight.b },
		{ {coordtextDark, R}, coordcolorDark.r },
		{ {coordtextDark, G}, coordcolorDark.g },
		{ {coordtextDark, B}, coordcolorDark.b },
	};
	//Insert/remove/swap appropriate associative values based on the Linked variables
}

void sliderClass::getValueFromAssociated()
{
	std::map<std::pair<SliderID,subSliderID>,sf::Uint8&> CAmap{ getColorAssociationMap() };
/* 	if (isCoordtextColorlinked)
	{
		for (auto P : CAmap) //NOT REF!
		{
			if (P.first.first == coordtextLight)
			{
				CAmap.erase(P.first);
				CAmap.insert_or_assign(P.first, (CAmap.at({sqColorlight, P.first.second})));
			}
			else if (P.first.first == coordtextDark)
			{
				CAmap.erase(P.first);
				CAmap.insert_or_assign(P.first, (CAmap.at({sqColordark, P.first.second})));
			}
		}
	} */

	if (sliderName == outlineThicknessD)
	{ currentValue = (sqOutlineThicknessD * 10); }
	else if (sliderName == outlineThicknessL)
	{ currentValue = (sqOutlineThicknessL * 10); }
	else if (sliderName == coordSize)
		currentValue = (coordTextsize * 5);
	else
		currentValue = (CAmap.find({ sliderName, subName })->second);

	currentValueFloat = currentValue;

	sliderObj.setPosition(currentValue + minXoffset, sliderObj.getPosition().y);
	sliderBoundingbox = sliderObj.getGlobalBounds();

	return;
}

//Sets sliders' values/positions to match their associated variable (use when variable was independently modified)
void SetSlidersFromAssociated(SliderID ID)
{
	int accessNum{0};
	if (ID == outlineThicknessD) { accessNum = 18; }
	else if (ID == outlineThicknessL) { accessNum = 19; }
	else if (ID == coordSize) { accessNum = 20; }

	if ((ID == outlineThicknessD) || (ID == outlineThicknessL) || (ID == coordSize))
	{ colorWindowSliders[accessNum].getValueFromAssociated(); }
	else
	{
		for (int Sub{0}; Sub < 3; ++Sub) //for R,G,B
		{
			accessNum = ((static_cast<int>(ID)*3) + Sub);
			colorWindowSliders[accessNum].getValueFromAssociated();
		}

	}


	return;
}

void sliderClass::updateAssociated()
{
	std::map<std::pair<SliderID, subSliderID>, sf::Uint8&> CAmap{getColorAssociationMap()};

	if (CAmap.contains({sliderName, subName})) //some sliders do NOT exist in the map, like outlineThickness
		CAmap.find({ sliderName, subName })->second = currentValue;

	switch (sliderName)
	{
		case outlineThicknessD:
		sqOutlineThicknessD = (currentValue / 10); //range of [0-25.5]
		break;

		case outlineThicknessL:
			sqOutlineThicknessL = (currentValue / 10); //range of [0-25.5]
			break;

	case coordSize:
	{
		coordTextsize = (currentValue / 5); //range of [0-51]
		for (auto& T : coordText)
		{
			T.setCharacterSize(coordTextsize);
		}
		break;
	}

	case coordtextLight:
	{
		for (auto& I : darkSqIDs)
		{ coordText[I].setFillColor(coordcolorLight); }
		break;
	}

	case coordtextDark:
	{
		for (auto& I : lightSqIDs)
		{ coordText[I].setFillColor(coordcolorDark); }
		break;
	}

	default: break;
	}

	return;
}


void sliderClass::Reset()
{
	std::map<std::pair<SliderID, subSliderID>, sf::Uint8&> CAmap{getColorAssociationMap()};

	currentValue = defaultValue;
	currentValueFloat = currentValue;

	sliderObj.setPosition(currentValue + minXoffset, sliderObj.getPosition().y);
	sliderBoundingbox = sliderObj.getGlobalBounds();

	updateAssociated();

	return;
}

//this resets the values of all the variables/sliders associated with a SliderID
void resetSlider(SliderID ID)
{
	wasModified.push_back(ID);

	switch (ID)
	{
		case outlineThicknessD:
			colorWindowSliders[18].Reset();
			break;

		case outlineThicknessL:
			colorWindowSliders[19].Reset();
			break;

		case coordSize:
			colorWindowSliders[20].Reset();
			break;

		case resetall:
		{
			for (std::size_t m{0}; m < colorWindowSliders.size(); ++m)
			{
				colorWindowSliders[m].Reset();
				wasModified.push_back(colorWindowSliders[m].sliderName);
			}
		}
		break;

		default:
		{
			for (int Sub{0}; Sub < 3; ++Sub) //for R,G,B
			{
				int accessNum = ((static_cast<int>(ID)*3) + Sub);
				colorWindowSliders[accessNum].Reset();
			}
		}
		break;
	}

	return;
};

std::vector<sf::FloatRect> sliderResetbuttons{
	{256, 30, 39, 16},	//SqColor
	{256, 129, 39, 16}, //CoordColor
	{256, 231, 39, 16}, //OutlineColor
	{256, 331, 39, 16}, //OutlineThickness
	{256, 395, 39, 16}, //CoordTextSize
	{236, 7, 59, 17},	//ResetAll
};

//this function handles the bounding-box checks for all buttons in the colorwindow
bool checkColorwindowButtons(sf::Vector2f clickLocation)
{
	bool isClickingAButton{false};

	if (SqoutlinecolorTogglebox.contains(clickLocation))
	{
		((isOutlineColorShared) ? isOutlineColorShared = false : isOutlineColorShared = true);
		isClickingAButton = true;
	}
	else if (CoordtextcolorTogglebox.contains(clickLocation))
	{
		((isCoordtextColorlinked) ? isCoordtextColorlinked = false : isCoordtextColorlinked = true);
		isClickingAButton = true;
	}
	else if (colorWindowSwitchButton.contains(clickLocation))
	{
		isEditingLightColors = (!isEditingLightColors);
		colorWindowText.setString(((isEditingLightColors)? "Light colors" : "Dark colors"));
		colorWindowText.setFillColor(((isEditingLightColors)? sf::Color::White : sf::Color::Black));

		for (auto& Slider : colorWindowSliders)
		{
			if (drawSliderForColor(Slider.sliderName))
			{
				Slider.getValueFromAssociated();
			}
		}

		isClickingAButton = true;
	}

	//checking the reset buttons
	for (std::size_t R{0}; R < sliderResetbuttons.size(); ++R)
	{
		if (sliderResetbuttons[R].contains(clickLocation))
		{
			isClickingAButton = true;

			switch (R)
			{
			case 0: //Sqcolor
				((isEditingLightColors) ? resetSlider(sqColorlight) : resetSlider(sqColordark));
				if (isCoordtextColorlinked)
				{		//lightsquares use dark coordtext, so reset the opposite textcolor (to the sqcolor being edited)
					((isEditingLightColors) ? resetSlider(coordtextDark) : resetSlider(coordtextLight));
				}
				break;

			case 1: //Coordcolor
				if (isCoordtextColorlinked) { isCoordtextColorlinked = false; } //otherwise the value doesn't reset
				((isEditingLightColors) ? resetSlider(coordtextDark) : resetSlider(coordtextLight));
				break;

			case 2: //outline color
				if (isOutlineColorShared)
				{
					resetSlider(outlineLight);
					resetSlider(outlineDark);
				}
				else
					((isEditingLightColors) ? resetSlider(outlineLight) : resetSlider(outlineDark));
				break;

			case 3: //OutlineThickness
				if ((isOutlineColorShared) || (isEditingLightColors))
				{ resetSlider(outlineThicknessL); }
				if ((isOutlineColorShared) || (!isEditingLightColors))
				{ resetSlider(outlineThicknessD); }
				break;

			case 4: //CoordTextSize
				resetSlider(coordSize);
				break;

			case 5: //resetall
				resetSlider(resetall);
				break;

			default: break;

			}

			break; //out of the for loop
		}
	}

	return (isClickingAButton);
}

void loadAllSliders() //loads the sliders into the colorWindowSliders vector above
{
	//getAssociationMap

	//create a slider for each rgb component in variable
	for (int s{ 0 }; s < SliderID(outlineThicknessD); ++s)
	{
		for (int B{ 0 }; B < 3; ++B)
		{
			colorWindowSliders.insert(colorWindowSliders.end(),{ SliderID(s), subSliderID(B) });
		}
	}
	colorWindowSliders.insert(colorWindowSliders.end(), {outlineThicknessD});
	colorWindowSliders.insert(colorWindowSliders.end(), {outlineThicknessL});
	colorWindowSliders.insert(colorWindowSliders.end(), {coordSize});
	//std::cout << "loaded sliders. \n"; //removed so I don't have to include iostream
	return;
}

bool drawSliderForColor(SliderID ID)
{
	bool shouldDraw{ false };

	switch (ID)
	{
	case coordSize:
		shouldDraw = true;
		break;

	case sqColorlight:
	case outlineLight:
	case coordtextDark: //it's more intuitive that the slider should affect the text on the same color as the squares you're editing. It's more informative too; otherwise you'd just have two sets of identical sliders.
	case outlineThicknessL:
		shouldDraw = isEditingLightColors;
		break;

	case sqColordark:
	case outlineDark:
	case coordtextLight:
	case outlineThicknessD:
		shouldDraw = !isEditingLightColors;
		break;

	default: break;
	}

	return shouldDraw;
}

