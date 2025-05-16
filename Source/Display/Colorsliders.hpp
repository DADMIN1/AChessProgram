#ifndef COLORSLIDERS_HPP_INCLUDED
#define COLORSLIDERS_HPP_INCLUDED

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Text.hpp>

#include <vector>
#include <map> //getColorAssociationMap() returns a map

//these are for a custom container that's basically a std::unordered_set except it maintains insertion order (use push_back)
// it's used for must be defined here because it's for "wasModified" list, which is used in "resetSlider"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/indexed_by.hpp>

//RGBA 0-255; 0=black
//alpha channel is fully transparent at 255

extern sf::Color lightsqColor;
extern sf::Color darksqColor;

extern sf::Color outlinecolorLight;
extern sf::Color outlinecolorDark;

extern float sqOutlineThicknessD;
extern float sqOutlineThicknessL;
extern int sqOutlineSignDark; //either 1 or -1, used to invert outline-thickness (grows into square instead of overlapping other squares)
extern int sqOutlineSignLight;

extern sf::Color coordcolorLight;
extern sf::Color coordcolorDark;
extern int coordTextsize;

extern std::vector<sf::Text> coordText;
extern std::vector<int> darkSqIDs; //There's no way to tell a square's color from it's ID alone
extern std::vector<int> lightSqIDs; //These are mostly for setting appropriate color to coordText(coordText[x] belongs to Squaretable[x])

extern bool isLSDmode;
//lastmoveHighlights colors?

extern bool isEditingLightColors; //determines which slider set is active/drawn

extern bool isOutlineColorShared; //does each color square have it's own outline color? Shared color by default
extern bool isCoordtextColorlinked; //is the color of the inlaid coordinates linked to the color of the opposite square?
extern sf::FloatRect SqoutlinecolorTogglebox;
extern sf::FloatRect CoordtextcolorTogglebox; //bounding boxes
extern sf::FloatRect colorWindowSwitchButton; //bounding box for "Switch (colors)"

extern sf::Text colorWindowText; //tells you which color you're editing

enum SliderID
{
	sqColorlight,
	sqColordark,
	outlineLight,
	outlineDark,
	coordtextLight,
	coordtextDark,

	outlineThicknessD, //not split into RGB
	outlineThicknessL,
	coordSize,

	resetall, //fake, exists to call resetSlider() with "resetall"
};

enum subSliderID
{
	R,
	G,
	B,
};

//the variables associated with various SliderIDs
std::map<std::pair<SliderID,subSliderID>,sf::Uint8&> getColorAssociationMap();

class sliderClass
{
	public:
	SliderID sliderName;
	subSliderID subName;
	sf::RectangleShape sliderObj{sf::Vector2f(10, 25)};
	sf::FloatRect sliderBoundingbox;
	sf::FloatRect sliderGuidebounds; //actually a bounding box for the whole range of valid positions, not just the small rectangle
	int minXoffset{0};	//The value that has to be subtracted from x-coord to get the slider's value (this will be set to non-zero)
	int defaultValue{0};
	int currentValue{0}; //The slider's current position - minXoffset

	//these two are for LSD mode
	float currentValueFloat{0};
	float changeRate{0};

	void updateAssociated(); //finds and updates the colors/other sliders related to the slider's current value
	void getValueFromAssociated(); //updates the slider's position to the variable's current position. Useful for initialization/reset
	void Reset(); //NEW

	sliderClass(SliderID initID,subSliderID sub = R)
	{
		sliderName = initID;
		subName = sub;

		sliderObj.setOutlineThickness(2);
		sliderObj.setOutlineColor(sf::Color::Black);
		sliderObj.setFillColor(sf::Color(169,169,169));
		sliderObj.setOrigin(5,12.5);

		switch(initID)
		{
		case sqColordark:
		case sqColorlight:
		sliderObj.setPosition(36,58);
		break;

		case outlineLight:
		case outlineDark:
		sliderObj.setPosition(31,258);
		break;

		case coordtextLight:
		case coordtextDark:
		sliderObj.setPosition(35,161);
		break;

		case outlineThicknessD:
		case outlineThicknessL:
			sliderObj.setPosition(27, 366);
		break;

		case coordSize:
		sliderObj.setPosition(27,430);
		break;

		default:
		break;
		}

		//because R (the default) is 0, sliders will not be moved unless they are the 2nd or 3rd of their type
		sliderObj.move(0,(26 * subSliderID(sub)));
		minXoffset = sliderObj.getPosition().x;
		sliderBoundingbox = sliderObj.getGlobalBounds();

		//sliderGuidebounds = sliderObj.getGlobalBounds();
		sliderGuidebounds.width = 255;
		sliderGuidebounds.height = 100;
		sliderGuidebounds.top = sliderObj.getPosition().y - (sliderGuidebounds.height / 2); //assuming it's around the (adjusted) origin, not the top-left
		sliderGuidebounds.left = sliderObj.getPosition().x;

		getValueFromAssociated();
		defaultValue = currentValue;

	} //END OF CONSTRUCTOR//
};

//has to be unordered set, otherwise when we insert stuff in an iteration loop (in main), it might be inserted before the current iterator
typedef boost::multi_index_container<SliderID,
	boost::multi_index::indexed_by<
	boost::multi_index::sequenced<>,
	boost::multi_index::ordered_unique<boost::multi_index::identity<SliderID>>
	>
> sequenced_set;

extern sequenced_set wasModified; //tracks when multiple color/associated values need to be re-applied (in main, somewhere with access to Squaretable). For example, reset buttons can modify the associated values of every color, without selecting a slider. The alternative would be to re-apply color/size/thickness variables on every frame, but this is more efficient and allows linked/shared values (linked coordText being opposite SqColor) to be properly controlled.
//LSD-mode does re-apply each variable on every frame, because it's meant to modify everything independently.

extern sliderClass* selectedSlider;

extern std::vector<sliderClass> colorWindowSliders;

void SetSlidersFromAssociated(SliderID); //basically same as getValueFromAssociated, but searches for all (3) sliders with matching SliderID

//this resets the values of all the variables/sliders associated with a SliderID
void resetSlider(SliderID C);

extern std::vector<sf::FloatRect> sliderResetbuttons;

bool checkColorwindowButtons(sf::Vector2f clickLocation);

void loadAllSliders(); //loads the sliders into the colorWindowSliders vector above

bool drawSliderForColor(SliderID D);

#endif
