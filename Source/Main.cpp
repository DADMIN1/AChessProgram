#include "GlobalVars.hpp"
#include "GameState.hpp"
#include "CoordLookupTable.hpp"
#include "Board.hpp"
#include "Colorsliders.hpp"
#include "ConfigReader.hpp"
#include "HighlightsIndicators.hpp"
#include "SoundFX.hpp"
#include "Pieces.hpp"
#include "Ruleset.hpp"
#include "Shortcuts.hpp"
#include "SquareStorage.hpp"
#include "FPSmath.hpp"
#include "TurnHistory.hpp"
#include "Engine.hpp"
#include "ArrowShape.hpp"
#include "DrawArrow.hpp"
#include "ArrowOverlay.hpp"
#include "TextStorage.hpp"
#include "EngineCommand.hpp"

//experimental polygonal board-tiles - visible in debug-mode
#define TILETEST_EXPERIMENT
#undef TILETEST_EXPERIMENT //comment this to enable

#ifdef TILETEST_EXPERIMENT
#include "BoardTile.hpp"
#endif

#include <effolkronium/random.hpp> //effolkronium, currently only used for LSDmode

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp> //Graphics and Window are actually seperate modules.

#include <iostream>
#include <vector>
#include <cctype> //for std::toupper / std::tolower
//#include <thread> //only required for detach
#include <tuple> //for std::ignore //lol

//uppercase-conversion for std::string (std::toupper only works with unsigned char)
std::string str_toupper(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
	return s;
}

Boardsquare* selectedSq{}; // Storage for a pointer to a selected Sq
Piece* selectedPiece{nullptr};
Boardsquare* hoveredSq{nullptr}; //Seems like this is only used to get hoveredPiece
Piece* hoveredPiece{nullptr}; //and this is only used to get moveCircles

bool isReshapeMode{false};
bool isSetupMode{false};
SwitchName SetupPiecetype{wPawn}; //stores the value of the piece you want to place in setup mode
sf::Sprite MousePiece_Setup; // Piece sprite attached to mouse during setup mode
sf::Sprite MousePiece; //attached to mouse during click+drag
bool isDraggingPiece{false};

bool isCastlePriority{true}; //if a piece has castling and non-castling moves overlapping, it will choose the castle-move

bool setupSpritesLoaded{false}; //This one is implemented
bool isBoardloaded{false}; //indicates that we need to reposition/redraw the squares, coordText, etc

bool isDrawHighlight{false};	//determines whether you draw the last-move highlights, NOT the selected-piece highlight
bool isDrawMovecircles{true};	 // global toggle for movecircles
bool isDrawOppMovecircles{true}; //specifically enables/disables Opponent's movecircles only
bool isDrawDangermap{false};	 //toggles drawing red square highlights on all squares in w/bPieceSquareStorage.AttackedSquares

Text_Storage textStorage{}; //this has to be declared above ChangePieceSetup to make setup_MouseName accessible.
//Or you could just declare it as a friend function????

//currently this only increments the pieceID by 1, instead of the given amount.
void ChangeSetupPiece(int amount, SwitchName targetType = SetupPiecetype)
{
	//if (!isSetupMode) { return; }

	static const std::set<SwitchName> illegalTypes{wPieceEnd, bPieceEnd, fairyPieceEnd};

	if (amount == 0) { SetupPiecetype = targetType; }
	else if (amount < 0) //going back
		{
			if (SetupPiecetype == 0) { SetupPiecetype = fairyPieceEnd; } //wraps around to the end

			--SetupPiecetype;
			if (illegalTypes.contains(SetupPiecetype)) { --SetupPiecetype; }
		}

		else if(amount > 0) //going to next piece
		{
			if(SetupPiecetype >= (fairyPieceEnd - 1)) {SetupPiecetype = SwitchName(0);} //wraps around to the start
			else { ++SetupPiecetype; } //this statement has to be exclusive to the first, otherwise you can't ever get 0

			if (illegalTypes.contains(SetupPiecetype)) { ++SetupPiecetype; }
		}

		MousePiece_Setup.setTexture(Piece::GetTexture(SetupPiecetype));
		textStorage.setup_mousename.setString(Piece::GetName_Str(SetupPiecetype));

	return;
}

void RescaleEverything()
{
	FlipBoard(); //recalculate positions of all squares, necessary.
	// it doesn't actually flip the board, it recalculates the positions of every square, based on the "isBoardFlipped" state.

	//if (SqSizeChanged)
	{ //not getting any better performance by disabling this, oddly
		//rechecking all piece sprites like a fucking noob
		//completely necessary. Do not remove these loops
		for (std::unique_ptr<Piece>& WP : wPieceArray)
		{
			if (!WP->isCaptured)
			{
				WP->m_Sprite.setScale(HorizontalScale, VerticalScale);
				WP->m_Sprite.setPosition(SquareTable[WP->m_SqID].getPosition());
			}
		}
		for (std::unique_ptr<Piece>& BP : bPieceArray)
		{
			if (!BP->isCaptured)
			{
				BP->m_Sprite.setScale(HorizontalScale, VerticalScale);
				BP->m_Sprite.setPosition(SquareTable[BP->m_SqID].getPosition());
			}
		}

		moveCircles.RescaleAll(HorizontalScale, VerticalScale);
		moveCircles.RescaleAlt(HorizontalScale, VerticalScale);

		moveHighlight1.setScale(HorizontalScale, VerticalScale);
		moveHighlight2.setScale(HorizontalScale, VerticalScale);

		dangermapHighlight.setScale(HorizontalScale, VerticalScale);

		ArrowOverlay::activeOverlay->setScale(HorizontalScale, VerticalScale); //Need to also keep the texture's view synched to boardWindow (taken care of in sf::Event::Resized).
			// This is actually a hack solution; it relies on continuously scaling the sprite, while never redrawing the old arrows to the render-texture. If you ever try and redraw the arrows, they'll all get screwed up.
		//The other (related) problem is that already-drawn arrows scale with the window; they get stretched or squished. So the size of arrows starts mismatching when you draw them before/after resizing the window.

		ArrowOverlay::activeOverlay->RotateFlip(isFlipboard); //must be called after the setScale call.

	} //END OF RESCALE BLOCK

	return;
}

SwitchName OverlayPromotionSelect(Piece& promotingPiece, sf::Vector2f targetSq, std::set<SwitchName> selections, sf::RenderWindow& passedWindow)
{
	if (selections.empty()) { std::cerr << "No legal options for promotion! \n"; return promotingPiece.m_PieceType; }

	float xPosition = targetSq.x;
	float yPosition = targetSq.y;

	std::vector<sf::Sprite> overlaySprites;
	int iterCount{1};
	for (SwitchName S : selections)
	{
		sf::Sprite& n = overlaySprites.emplace_back(sf::Sprite(Piece::GetTexture(S)));
		n.setScale(HorizontalScale, VerticalScale);
		n.setPosition(xPosition, yPosition + (iterCount*SqHeight*((promotingPiece.isWhite)? 1 : -1)));
		++iterCount;
	}

	iterCount -= 1; //because we started at 1, not 0
	sf::RectangleShape backDrop(sf::Vector2f(SqWidth, SqHeight*((promotingPiece.isWhite)? iterCount : -iterCount)));
	backDrop.setFillColor(sf::Color(196, 196, 196, 128));
	backDrop.setOutlineColor(sf::Color(255,255,255,200));
	backDrop.setOutlineThickness(-3);
	backDrop.setPosition(xPosition, yPosition);
	if (promotingPiece.isWhite) { backDrop.move(0, SqHeight); }

	sf::RenderTexture theOverlay;
	theOverlay.create(passedWindow.getSize().x, passedWindow.getSize().y);
	//theOverlay.create(backDrop.getSize().x, std::abs(backDrop.getSize().y)); //if we do it this way, each element (sprites/backDrop) would need to be placed relative to (0,0) instead of their absolute-position. This would be simpler, but there's no way to check mouse-position relative to the overlay, and the sprites' bounding-box would not match their drawn location.
	theOverlay.clear(sf::Color::Transparent);

	//we only have to draw/dislay once because the frameloop is paused until this function ends
	theOverlay.draw(backDrop);
	for (auto& S : overlaySprites)
	{ theOverlay.draw(S); }
	theOverlay.display();

	//sf::Texture freshOverlay = theOverlay.getTexture();

	sf::Sprite theOverlaySprite;
	theOverlaySprite.setTexture(theOverlay.getTexture());

	passedWindow.draw(theOverlaySprite);
	passedWindow.display();

	SwitchName choice{promotingPiece.m_PieceType};

	sf::Event E;
	sf::Sprite* selectedSprite{nullptr};
	SwitchName selectedType{promotingPiece.m_PieceType}; //defaults to no-change if no type gets selected
	sf::Vector2f mousePosition = sf::Vector2f(sf::Mouse::getPosition(passedWindow));

	while (choice == promotingPiece.m_PieceType)
	{ //this could (and probably should) be a while(true) loop instead. Then we don't need the redundant selectedType/choice.
		passedWindow.pollEvent(E);

		switch(E.type)
		{
			case sf::Event::MouseMoved:
			{
				//this needs to be updated every time, because the left-mouse-pressed event uses it
				mousePosition = sf::Vector2f(sf::Mouse::getPosition(passedWindow));

				if (selectedSprite)
				{
					if (selectedSprite->getGlobalBounds().contains(mousePosition))
					{ break; }
					else
					{
						//remove the gold highlight
						selectedSprite = nullptr;
					}
				}
			}
			break;

			case sf::Event::MouseButtonPressed:
			{
				//decline promotion
				if (E.mouseButton.button == sf::Mouse::Right)
				{
					return promotingPiece.m_PieceType;
				}

				if (E.mouseButton.button == sf::Mouse::Left)
				{
					if (!selectedSprite)
					{
						for (std::set<SwitchName>::iterator iter{selections.begin()}; auto& S : overlaySprites)
						{
							if (S.getGlobalBounds().contains(mousePosition))
							{
								selectedSprite = &S;
								selectedType = *iter;
								//set gold highlight
								break;
							}

							++iter;
						}
					}
				}
			}
			break;

			case sf::Event::MouseButtonReleased:
			{
				if (E.mouseButton.button == sf::Mouse::Left)
				{
					if (selectedSprite)
					{ //only set it if we're still hovering the sprite we clicked on
						choice = selectedType;
					}
				}
			}
			break;

			case sf::Event::KeyPressed:
			{
				//cancel the move entirely!
				if (E.key.code == sf::Keyboard::Escape)
				{
					return promotingPiece.m_PieceType;
				}
			}
			break;

			case sf::Event::MouseWheelScrolled:
			{
				//shift the overlay and all sprites up or down
				//for instances when you have too many promotions to fit on the screen.
				//we could probably just add more columns to the overlay in those cases? But there could be cases where the piece has too many promotions and the board is actually too small to draw them all.
			}
			break;

			default:
				break;
		}
	}

	return choice;
}


int main()
{
	//std::system("pwd");
	std::ignore = std::system("clear");
	//auto system("clear"); //bash command. Pretending to use return-value to prevent the useless warning. For some reason, std::system can't pretend to use return-value with 'auto' or 'int', or even void-cast.
		//I think that using "auto system()" causes the compiler to optimize-out the line, or it turns it into a string declaration instead? It seems to break the windowHax-code

		//Load config file
	InitOptions initOptions = SetConfigValues();
	std::cout << "calling initOptions: " << initOptions.FunctionMap.begin()->first << '\n';
	initOptions.FunctionMap.begin()->second();

	const unsigned int textureSizeMax{ sf::Texture::getMaximumSize() };
	std::cout << "\nGraphics card has a max texture size of: " << textureSizeMax << " pixels per dimension \n\n";
		//returns 16384px = 128^2
		//Maximum texture-dimensions is 16384x16384,

	// read config file to determine internal resolution. (high=128px/tile, medium=96px, low=64px)
		/*internal resolution can be maintained for all board sizes, all sprites will be drawn to rendertextures,
		which can then be scaled up/down to fit the window.*/

	Piece::LoadAllTextures();

/* 	sf::RenderTexture testSpriteSheet;
	testSpriteSheet.create(960, 960);
	testSpriteSheet.clear(sf::Color::Transparent);

	for (auto S : Piece::CreateSprites(SwitchName(0), fairyPieceEnd))
	{
		static int increasingOffset{0};
		S.second.setPosition((increasingOffset % 960), (96 * int(increasingOffset/960)));
		testSpriteSheet.draw(S.second);
		increasingOffset += 96;
	}
	testSpriteSheet.display(); */

	sf::Vector2i windowPositionCheck{}; //we'll use this to see if the window has moved
	//this is used to determine if the "windowHax" script should be activated

	timerLedger.reserve(2000);
	timerLedger.resize(120);

	//Text_Storage::testFont.loadFromFile("Resource/Fonts/dejavu/DejaVuSerifCondensed.ttf"); // doesn't segfault inside main
	InitFPS_Text();

	sf::RectangleShape selectedPieceHighlight(sf::Vector2f(120,120));	// Highlight for selected piece on board
	selectedPieceHighlight.setFillColor(sf::Color(255,255,255,125)); //white

	sf::RectangleShape setupSelectionhighlight(sf::Vector2f(120,120)); //highlight square for selected piece in setup window
	setupSelectionhighlight.setFillColor(sf::Color(255,218,0,175));	//gold

	moveHighlight1.setFillColor(sf::Color(200,200,80,75));
	moveHighlight2.setFillColor(sf::Color(200,200,80,75));

	// sf::RectangleShape dangermapHighlight(sf::Vector2f(SqWidth, SqHeight));
	dangermapHighlight.setSize(sf::Vector2f(SqWidth, SqHeight));
	//transparent looks kind of jank because of the board colors
	//	dangermapHighlight.setFillColor(sf::Color(255,0,0,75));
	dangermapHighlight.setFillColor(sf::Color(255, 0, 0, 50));

	//setting antialiasingLevel to anything above 0 causes the creation to fail? It has some weird side-effects, like turning the FPS-timer invisible unless you're hovering a piece.
	//sf::ContextSettings  graphicSettings (0, 0, 0, 1, 1, sf::ContextSettings::Default, false);
/* 	graphicSettings.depthBits = 0;
	graphicSettings.stencilBits = 0;
	graphicSettings.antialiasingLevel = 0;
	graphicSettings.majorVersion = 1;
	graphicSettings.minorVersion = 1;
	graphicSettings.attributeFlags = graphicSettings.Default;
	graphicSettings.sRgbCapable = false; */

	//There's literally no point in even trying to set the anti-aliasing level of a renderTexture. sfml antialiasing only applies to shapes/verticies/primitives, not textures. //It's not even supposed to support it anyway, and causes all sorts of problems.
	sf::RenderTexture moveHighlightBuffer; //buffer to draw movecircles to
	moveHighlightBuffer.create(960,960);
	moveHighlightBuffer.setSmooth(true);

	//code for drawing piece bounding boxes generalize this. Or can I remove this?
	sf::RectangleShape visualPieceBox(sf::Vector2f(SqWidth,SqHeight));
	visualPieceBox.setOutlineColor(sf::Color(255,0,0));
	visualPieceBox.setOutlineThickness(2);
	visualPieceBox.setFillColor(sf::Color::Transparent);

	//960/8 = 120
	sf::VideoMode videoModeDefault(960,960); // Defining a variable to hold resolutions for switching between window sizes.
	//sf::VideoMode videoModeDefault(1024, 1024);
//Don't change to this until you're ready to switch from 96px/tile to 128px
//96dpi is a standard, though

	sf::ContextSettings  mainWindowContextSettings(0, 0, 16, 1, 1, sf::ContextSettings::Default, false);
	//mainWindowContextSettings.antialiasingLevel = 16;
	//sf::RenderWindow boardWindow(sf::VideoMode(1024,1024), "Chess_V2", sf::Style::Default, mainWindowContextSettings); // 1024 = 128x8 , and it fits within 1080 height.
	sf::RenderWindow boardWindow(videoModeDefault, "Chess_V2", sf::Style::Default, mainWindowContextSettings); // 1024 = 128x8 , and it fits within 1080 height.


	//sf::RenderWindow boardWindow(sf::VideoMode(videoModeDefault), "Board window");
	boardWindow.setFramerateLimit(60);
	//boardWindow.setPosition(sf::Vector2i(1980, 0)); //opens on right monitor
	//boardWindow.setPosition(sf::Vector2i(0,0));

//actually why don't you just use the winWidth/Height variables in this?
	sf::View StandardView(sf::FloatRect(0,0,960,960));

	//sf::View SetupModeView(sf::FloatRect(0, 0, 720, 240));
	sf::View SetupModeView(sf::FloatRect(0,0,1440,1080));
	boardWindow.setView(StandardView);

	sf::VideoMode smallLong(720,240); // size for normal pieceset
	//sf::VideoMode maxPieceSelect(1440, 1080);
	sf::VideoMode maxPieceSelect(720,480);
	sf::RenderWindow SetupWindow(maxPieceSelect,"Piece Selection",sf::Style::Titlebar); //don't allow setupwindow to be resized
	//I don't think the videomode set here matters, only the one that's set when you re-open it later.
	SetupWindow.setView(SetupModeView);
	SetupWindow.setFramerateLimit(60); //the main window's framerate is throttled if you set this any lower
	SetupWindow.setVisible(false);
	SetupWindow.setPosition(sf::Vector2i(boardWindow.getSize().x, 0) + boardWindow.getPosition()); //trying to open it to the right of the boardwindow

	sf::Texture setupBackdropTileTex;
	setupBackdropTileTex.loadFromFile("Resource/Graphics/cheatTile.png");
	setupBackdropTileTex.setRepeated(true);
	setupBackdropTileTex.setSmooth(true);

	sf::Sprite BackdropTile(setupBackdropTileTex);
	BackdropTile.setTextureRect(sf::IntRect(sf::Vector2i(0,0),sf::Vector2i(SetupModeView.getSize())));
	std::vector<Piece> selectionSprites{};
	selectionSprites.reserve(static_cast<int>(fairyPieceEnd));

	SFX_Controller SFX_Controls;

	sf::RectangleShape wTerritoryLine(sf::Vector2f(winWidth,4));
	wTerritoryLine.setFillColor(sf::Color(255,255,255,125));
	wTerritoryLine.setOrigin(0,2);
	sf::RectangleShape bTerritoryLine(wTerritoryLine);
	bTerritoryLine.setFillColor(sf::Color(0,0,0,125));

	MousePiece_Setup.setOrigin(sf::Vector2f(SqWidth / 2, SqWidth / 2));
	MousePiece.setOrigin(sf::Vector2f(SqWidth / 2, SqWidth / 2));

	//setting a scale is better than drawing to a renderTexture and resizing that because the bounding boxes are tied to the actual sprite
	sf::Vector2f setupwindowScale(0.5, 0.5); //custom scale to apply to everything in setupWindow
	// isn't it possible to get around that using an sf::view?

	//should this be put before or after IndicatorOverlay.create?
	//declared in CustomRendering.hpp. Have to do this here otherwise it goes out of scope?
	RoyalIndicator.loadFromFile("Resource/Graphics/RoyalIndicator.png");
	canCastleIndicator.loadFromFile("Resource/Graphics/canCastleIndicator.png");
	CastlewithIndicator.loadFromFile("Resource/Graphics/CastlewithIndicator.png");
	canPromoteIndicator.loadFromFile("Resource/Graphics/canPromoteIndicator.png");
	//isPromotedIndicator.loadFromFile("Resource/Graphics/isPromotedIndicator.png"); //not implemented yet

	IndicatorOverlay.create(SqWidth,SqHeight); //if you do this here it won't scale with different square sizes //seems fine??

	wPieceArray.reserve(numColumns*numRows);
	bPieceArray.reserve(numColumns*numRows);
	SquareTable.reserve(numColumns*numRows);

	isWhiteTurn = true;

	if (initOptions.loadedValues.contains("DefaultGameMode"))
	{
		rules.changeGamemode(initOptions.loadedValues.at("DefaultGameMode"));
	}

	if (default_promoteInTerritory)
	{
		rules.promoteRules.isTerritoryPromote = true;
		rules.promoteRules.AddRule(PromotionRules::Trigger::Criteria::InsideTerritory);
	}
	
	if (initSetupFEN.length() > 0)
	{
		std::cout << "storing initial FEN (loaded from config): \n";
		std::cout << initSetupFEN << '\n';
		savedFEN = initSetupFEN;
	}
	
	if (isSetupFromCustomFEN)
	{
		assert((initSetupFEN.length() > 0) && "cannot setup from FEN: initSetupFEN is empty!");
		//thor::create "LoadSetupFromFEN"(sf::Keyboard::Key::Subtract) event
		FENstruct parsedFEN = LoadFEN(initSetupFEN);

		if (parsedFEN.sideToMove != '-')
		{ isWhiteTurn = !(parsedFEN.sideToMove == 'w'); }
		else { isWhiteTurn = !isWhiteTurn; } //if not specified, side-to-move will be the same after alternateTurn

		rules.setRankVars(); //otherwise movetables aren't calculated with nonstandard territory sizes
		AlternateTurn(); //Generate movetables for all pieces
	}
	else
	{
		GenerateBoard(true);
		initialPieceSetup();
	}

	//to properly scale the initial board/window, you need to call this after "GenerateBoard(true)" has been called (it is called by LoadFEN)
	//GenerateBoard(true) rescales sqWidth,winWidth,Horizontal-scale, etc, and sets "isTriggerResize" to true
//I think the problem is that I relocated the "isTriggerResize" and "RescaleEverything" calls from the top of the frameloop to after the PollEvent code.
	boardWindow.setSize(sf::Vector2u(winWidth, winHeight));

	rules.setRankVars();
	printShorcuts(); //lists the keyboard shortcuts for the program

	//window for color picking sliders
	sf::RenderWindow colorWindow(sf::VideoMode(306,459),"Colors");
	//colorWindow.setFramerateLimit(60); //framerate doesn't matter because it won't get preserved through closing/reopening
	colorWindow.setPosition(sf::Vector2i(boardWindow.getSize().x,0) + boardWindow.getPosition());
	//colorWindow.setVisible(false); //close instead
	colorWindow.close(); //we close the window because otherwise it throttles the framerate

	sf::Texture colorPickingTexture;
	colorPickingTexture.loadFromFile("Resource/Graphics/colorWindow.png");
	sf::Sprite colorPickingMenu(colorPickingTexture);
	sf::Texture greenCheckboxTexture;
	greenCheckboxTexture.loadFromFile("Resource/Graphics/FilledCheckbox.png");
	sf::Sprite filledCheckbox(greenCheckboxTexture);

	//sf::Text colorWindowText("Light colors", dejaVu); //tells you which color you're editing
	colorWindowText.setString("Light colors"); //needs a string to show after window is opened
	colorWindowText.setFont(textStorage.dejaVu);
	colorWindowText.setFillColor(sf::Color::White);
	colorWindowText.setOutlineColor(sf::Color::Black);
	colorWindowText.setOutlineThickness(2);
	colorWindowText.setStyle(sf::Text::Bold);
	colorWindowText.setCharacterSize(18);
	colorWindowText.setPosition(4,4);

	loadAllSliders();

	#ifdef TILETEST_EXPERIMENT
	std::cout << "creating testTiles \n";
	std::vector<BoardTile> testTiles;
	std::vector<CustomTile> customTiles;
	for (int s{ 0 }; (s <= 7); ++s)
	{
		//id, algCoord, px-position, colorindex, radius, sides //last two are optional
		testTiles.push_back(BoardTile(s,"placeholder",sf::Vector2f(120*s,0),(s%2),68.f,(s+3)));
		customTiles.push_back(CustomTile(s,"placeholder",sf::Vector2f(120*s,120),(s%2),68.f,(s+3)));
	}
	#endif
	
	ArrowOverlay arrowOverlay{};
	arrowOverlay.UpdateToWindow(&boardWindow);

	// 	START OF BOARDWINDOW FRAMELOOP
	while(boardWindow.isOpen())
	{
		// while(colorWindow.isOpen() && colorWindow.hasFocus()) //LSD-mode happens in this loop, so you can't use the "hasFocus" requirement
		while(colorWindow.isOpen())
		{
			colorWindow.clear();
			colorWindow.draw(colorPickingMenu);
			colorWindow.draw(colorWindowText);

			sf::Event CWevent;
			while(colorWindow.pollEvent(CWevent))
			{
				if(CWevent.type == sf::Event::Closed)
					colorWindow.close();

				if(CWevent.type == sf::Event::KeyPressed)
				{
					switch(CWevent.key.code)
					{
						case sf::Keyboard::F7:
						{
							isLSDmode = false; //since the updates only occur when the window is open anyway
							colorWindow.close();
						}
						break;
				// save values for color you're editing

				// save values for both colors

				// Enable LSD mode
					case sf::Keyboard::L:
					{
						((isLSDmode) ? isLSDmode = false : isLSDmode = true);

						//during LSD-mode, these have different functions:
						isCoordtextColorlinked = false; //outlines have their opacity changed
						isOutlineColorShared = true; //square outline thickness is shared between colors

						selectedSlider = nullptr;
					}
					break;

					case sf::Keyboard::Key::R:
						{resetSlider(resetall);}
						break;

					case sf::Keyboard::Space:
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

						selectedSlider = nullptr;
					}
					break;

					case sf::Keyboard::I:
					{
						((isEditingLightColors)? sqOutlineSignLight : sqOutlineSignDark) *= -1;
						wasModified.push_back(outlineThicknessD);
						wasModified.push_back(outlineThicknessL);
						std::cout <<  ((isEditingLightColors)? "LightSq" : "DarkSq")  << " outline-thickness sign set to " << ((isEditingLightColors)? sqOutlineSignLight : sqOutlineSignDark) << '\n';

						selectedSlider = nullptr;
					}
					break;

					default:
					break;
					}
				}

				if(CWevent.type == sf::Event::MouseButtonPressed)
				{
					sf::Vector2f clickLocation{sf::Mouse::getPosition(colorWindow)};

					if (checkColorwindowButtons(clickLocation)) //if you did not click a button, this returns false
					{
						selectedSlider = nullptr;
					}
					else
					{
						selectedSlider = nullptr;

						for (std::size_t SS{0}; SS < colorWindowSliders.size(); ++SS)
						{
							if (!drawSliderForColor(colorWindowSliders[SS].sliderName))
							{ continue; }

							if (colorWindowSliders[SS].sliderBoundingbox.contains(sf::Vector2f(sf::Mouse::getPosition(colorWindow))))
							{
								selectedSlider = &colorWindowSliders[SS];
								break;
							}
						}
					}
				}

				if (CWevent.type == sf::Event::MouseButtonReleased)
				{
					selectedSlider = nullptr;
				}
			}

			//Drawing checkboxes if their setting is enabled
			if(isOutlineColorShared)
			{
				filledCheckbox.setPosition(226,222);
				colorWindow.draw(filledCheckbox);
			}

			if(isCoordtextColorlinked)
			{
				filledCheckbox.setPosition(227,120);
				colorWindow.draw(filledCheckbox);
			}

			// if ((isSliderSelected) && sf::Mouse::isButtonPressed(sf::Mouse::Left))
			if ((selectedSlider) && sf::Mouse::isButtonPressed(sf::Mouse::Left))
			{
				if (selectedSlider->sliderGuidebounds.contains(sf::Vector2f(sf::Mouse::getPosition(colorWindow).x, selectedSlider->sliderObj.getPosition().y)))
				{
					selectedSlider->sliderObj.setPosition((sf::Mouse::getPosition(colorWindow).x), selectedSlider->sliderObj.getPosition().y);
					selectedSlider->currentValue = sf::Mouse::getPosition(colorWindow).x - selectedSlider->minXoffset;
					selectedSlider->currentValueFloat = selectedSlider->currentValue;
					selectedSlider->sliderBoundingbox = selectedSlider->sliderObj.getGlobalBounds();

					if (isDEBUG)
						std::cout << selectedSlider->currentValue << "\n";

					selectedSlider->updateAssociated();
				}

				wasModified.push_back(selectedSlider->sliderName);
			}

			if (isLSDmode && isOutlineColorShared)
			{
				//18 = darkOutline, 19 = lightOutline
				int dependent{((isEditingLightColors)? 18 : 19)};
				int control{((isEditingLightColors)? 19 : 18)};

				colorWindowSliders[dependent].currentValue = colorWindowSliders[control].currentValue;
				colorWindowSliders[dependent].currentValueFloat = colorWindowSliders[control].currentValueFloat;
				colorWindowSliders[dependent].changeRate = colorWindowSliders[control].changeRate;

			}

			//draw loop, also updating values during LSD mode
			for (auto& SS : colorWindowSliders)
			{
				//LSDmode code here
				if (isLSDmode)
				{
					if (SS.changeRate == 0)
					{
						SS.changeRate = effolkronium::random_static::get(-1.0, 1.0);
					}

					if (SS.currentValueFloat >= 255)
					{
						SS.currentValueFloat = 255;
						SS.changeRate = effolkronium::random_static::get(-1.0, -0.1);
					}

					if (SS.currentValueFloat <= 0)
					{
						if (((SS.sliderName == outlineThicknessD)) && effolkronium::random_static::get<bool>(0.75))
							{
								// ((SS.sliderName == outlineThicknessD)? sqOutlineSignDark : sqOutlineSignLight) *= -1;
							sqOutlineSignDark *= -1;
						}
						else if (((SS.sliderName == outlineThicknessL)) && effolkronium::random_static::get<bool>(0.75))
						{sqOutlineSignLight *= -1;}

						SS.currentValueFloat = 0;
						SS.changeRate = effolkronium::random_static::get(0.1, 1.0);
					}

					SS.currentValueFloat += SS.changeRate;
					SS.currentValue = static_cast<int>(SS.currentValueFloat);
					SS.sliderObj.move(SS.changeRate, 0); //this isn't necessary, it's just cool to see them move
					SS.updateAssociated();
					SS.sliderBoundingbox = SS.sliderObj.getGlobalBounds();
				}

				//if the slider is associated with the square(light/dark) we're editing, draw it
				if (drawSliderForColor(SS.sliderName))
					{colorWindow.draw(SS.sliderObj);}
			}

			if (!wasModified.empty())
			{
				if (wasModified.front() == resetall) //otherwise the outline colors won't get reset?
				{
					std::cout << "RESETALL!\n";
					isCoordtextColorlinked = true;
					isOutlineColorShared = true;
					//wasModified.clear();
				}

				for (SliderID S_ID : wasModified)
					{
					switch(S_ID)
					{
						case sqColordark:
						{
							for (auto& I : darkSqIDs)
							{
								SquareTable[I].setFillColor(darksqColor);
							}
							if (isCoordtextColorlinked)
							{
								coordcolorDark = darksqColor;
								wasModified.push_back(coordtextDark);
							}
						}
						break;

						case sqColorlight:
						{
							for (auto& I : lightSqIDs)
							{
								SquareTable[I].setFillColor(lightsqColor);
							}
							if (isCoordtextColorlinked)
							{
								coordcolorLight = lightsqColor;
								wasModified.push_back(coordtextLight);
							}
						}
						break;

						case coordtextLight:
						{
							for (auto& I : darkSqIDs)
							{ coordText[I].setFillColor(coordcolorLight); }
						}
						break;

						case coordtextDark:
						{
							for (auto& I : lightSqIDs)
							{ coordText[I].setFillColor(coordcolorDark); }
						}
						break;

						case outlineThicknessD:
						{
							for (int& I : darkSqIDs)
							{
								SquareTable[I].setOutlineThickness((sqOutlineSignDark*sqOutlineThicknessD));
							}

							if (isOutlineColorShared)
							{
								sqOutlineThicknessL = sqOutlineThicknessD;
								wasModified.push_back(outlineThicknessL); //not an infinite loop because std::set doesn't allow duplicates
							}
						}
						break;

						case outlineThicknessL:
						{
							for (int& I : lightSqIDs)
							{
								SquareTable[I].setOutlineThickness((sqOutlineSignLight*sqOutlineThicknessL));
							}

							if (isOutlineColorShared)
							{
								sqOutlineThicknessD = sqOutlineThicknessL;
								wasModified.push_back(outlineThicknessD); //This doesn't work because this enum is a lesser value, so we've already traversed past this in the for-loop
							}
						}
						break;

						case outlineLight:
						case outlineDark:
						{
							if (isOutlineColorShared)
							{
								//You can't use "isEditingLightColors" because then resetall won't update one of them!
								// if (isEditingLightColors) { outlinecolorDark = outlinecolorLight; }
								if (S_ID == outlineLight) { outlinecolorDark = outlinecolorLight; }
								else { outlinecolorLight = outlinecolorDark; }

								for (auto& Sq : SquareTable)
								{ Sq.setOutlineColor(outlinecolorLight); } //doesn't matter which we choose if it's shared
							}
							else
							{
								//You can't use "isEditingLightColors" because then resetall won't update one of them!
								// for (int& I : ((isEditingLightColors)? lightSqIDs : darkSqIDs))
								// { SquareTable[I].setOutlineColor(((isEditingLightColors)? outlinecolorLight : outlinecolorDark)); }
								for (int& I : ((S_ID == outlineLight)? lightSqIDs : darkSqIDs))
								{ SquareTable[I].setOutlineColor(((S_ID == outlineLight)? outlinecolorLight : outlinecolorDark)); }
							}
						}
						break;

						default:
							break;
					}

					SetSlidersFromAssociated(S_ID); //update the sliders to match the new, modified values
				} //end of for loop

				wasModified.clear();
			}

			if (isLSDmode)
			{
				if (isCoordtextColorlinked)
				{
					// outlinecolorDark.a = ((outlinecolorDark.r + outlinecolorDark.g + outlinecolorDark.b) / 3);
				outlinecolorDark.a = ((darksqColor.r + darksqColor.g + darksqColor.b) / 3);
				// outlinecolorLight.a = ((outlinecolorLight.r + outlinecolorLight.g + outlinecolorLight.b) / 3  );
				outlinecolorLight.a = ((lightsqColor.r + lightsqColor.g + lightsqColor.b) / 3  );
				}

				for (auto& Sq : SquareTable)
				{
					if (Sq.isDark)
					{
						Sq.setFillColor(darksqColor);
						Sq.setOutlineThickness(sqOutlineSignDark * sqOutlineThicknessD);
						Sq.setOutlineColor(outlinecolorDark);
					}
					else
					{
						Sq.setFillColor(lightsqColor);
						Sq.setOutlineThickness(sqOutlineSignLight * sqOutlineThicknessL);
						Sq.setOutlineColor(outlinecolorLight);
					}

					/* if (Sq.isOccupied)
					{
						// Sq.occupyingPiece->m_Sprite.setColor(Sq.getFillColor() - sf::Color(1,1,1,0));
						if(isCoordtextColorlinked)
							Sq.occupyingPiece->m_Sprite.setColor(sf::Color(255, 255, 255, 255));
						else
							Sq.occupyingPiece->m_Sprite.setColor(sf::Color(255, 255, 255, 255));
					} */
				}
			}

			colorWindow.display();

			//boardWindow.requestFocus();
			break; //why are we using a while-loop if we always break?
		} //END OF COLORWINDOW FRAMELOOP//

		//START OF SETUPWINDOW FRAMELOOP//
		//isOpen status no longer changes; add a isVisible() check.
		//Maybe use hasFocus()? (SFML doesn't have an isVisible function)
		while(SetupWindow.isOpen() && isSetupMode)
		{
			SetupWindow.clear();

			//Start of SETUPWINDOW EVENTS
			sf::Event SetupWindowEvent;
			while(SetupWindow.pollEvent(SetupWindowEvent))
			{

				if(SetupWindowEvent.type == sf::Event::Closed)
					SetupWindow.setVisible(false);
				//SetupWindow.close(); //There doesn't seem to be any performance difference between hiding the window and having it closed
				//And it would be preferrable to not close it so that we don't have to re-create the pieces every time, and re-set the framerate and other options

				if(SetupWindowEvent.type == sf::Event::KeyPressed)
				{
					// Inner switch - START OF SETUPWINDOW KEYBOARD SHORTCUTS
					switch(SetupWindowEvent.key.code)
					{
						case sf::Keyboard::Q:
						case sf::Keyboard::Tab: //toggle setup mode/window
						{
							if (isSetupMode)
							{
								isSetupMode = false;

								ClearSquareStorage();
								EraseCapturedPieces();
								turnhistory.Clear();

								(isWhiteTurn) ? isWhiteTurn = false : isWhiteTurn = true;
								AlternateTurn(); //This refills SquareStorage

								SetupWindow.setVisible(false);
							}
						}
						break;

						/*	case sf::Keyboard::Up:
						SetupWindow.setPosition(sf::Vector2i(0, 0));
						SetupWindow.setVisible(true);
						break;*/

						case sf::Keyboard::Down: //Doesn't toggle setupMode though
						{ SetupWindow.setVisible(false); }
						break;

						default:
							break;
					} //END KEYCODE SWITCH
				}	  //END SETUPWINDOW KEYPRESS EVENTS

				if(SetupWindowEvent.type == sf::Event::Resized)
				{
					sf::FloatRect visibleArea(0.f,0.f,SetupWindowEvent.size.width,SetupWindowEvent.size.height);
					SetupWindow.setView(sf::View(visibleArea));
					//break; //Should I break here? it's not in a switch, it's breaking the while loop.
				}

				//this doesn't work anymore because Window-Manger ignores focus requests, for some reason
				/* if(SetupWindowEvent.type == sf::Event::MouseEntered)
				{
					SetupWindow.requestFocus();
				}

				if(SetupWindowEvent.type == sf::Event::MouseLeft)
				{
					boardWindow.requestFocus();
				} */

				//changing scroll event may have broken it
				if((SetupWindowEvent.type == sf::Event::MouseWheelScrolled) && (isSetupMode))
				{
					if(SetupWindowEvent.mouseWheelScroll.delta > 0)
					{ //If you scroll up, select the previous m_PieceType
						ChangeSetupPiece(-1);
					}
					else if(SetupWindowEvent.mouseWheelScroll.delta < 0)
					{ //If you scroll down, select the next m_PieceType
						ChangeSetupPiece(1);
					}
				}

				if(SetupWindowEvent.type == sf::Event::MouseButtonPressed)
				{
					//after selected a piece by clicking, you probably want to go back to the board window?
					if (SetupWindowEvent.mouseButton.button == sf::Mouse::Left)
					{
						for(std::size_t j{0}; j < selectionSprites.size(); ++j)
						{
							if(selectionSprites[j].m_Sprite.getGlobalBounds().contains(sf::Vector2f(sf::Mouse::getPosition(SetupWindow))))
							{
								ChangeSetupPiece(0, selectionSprites[j].m_PieceType);
								break;
							}
						}
					}
				}
			} //END of SetupWindow event polling loop

			//SetupWindow loading pieces

			if(setupSpritesLoaded == false)
			{
				selectionSprites.clear(); //because we're not overwriting them
				selectionSprites.reserve(static_cast<int>(fairyPieceEnd));
				int r{0}; //keeps track of what row we're on, for pieces
				float setupwindowTilewidth{120 * setupwindowScale.x};
				float setupwindowTileheight{120 * setupwindowScale.y};

				std::stringstream outPut;
				if (isDEBUG) { std::cout << "loading SetupWindow Sprites...\n"; }

				for (int a{0}; a < fairyPieceEnd; ++a)
				{
					//Because we don't touch the array at wPieceEnd(6),bPieceEnd(13),fairyPieceEnd(), they remain invalid pieces (and get put in the top-left)
					//for invalid pieces
					if((a == wPieceEnd) || (a == bPieceEnd) || (a == fairyPieceEnd))
					{
						if (isDEBUG) { outPut << "skipping sprite#" << a << '\t'; }
						continue;
					}

					Piece& spriteJustPlaced = selectionSprites.emplace_back(SwitchName(a), "Setup_Window");
					if (isDEBUG) { outPut << "sprite#" << a << ": " << ((spriteJustPlaced.isWhite)? 'w' : 'b') << spriteJustPlaced.GetName() << '\t'; }

					if (a < wPieceEnd)
					{spriteJustPlaced.m_Sprite.setPosition(sf::Vector2f((a * setupwindowTilewidth),0));}

					if((a > wPieceEnd) && (a < bPieceEnd))
					{spriteJustPlaced.m_Sprite.setPosition(sf::Vector2f(((a - wPieceEnd - 1) * setupwindowTilewidth),setupwindowTileheight));}

					if((a > bPieceEnd) && (a < wPaladin)) //currently 50 is wPaladin. Need to make new columns for those
					{
						int b = (a - 2);			  //because we skip wPieceEnd and bPieceEnd
						if((b % 12 == 0) && (a > 0)) //6 pieces per row
							{r += 2;}

						if((a % 2) == 0)
							{spriteJustPlaced.m_Sprite.setPosition(sf::Vector2f((((b % 12) / 2) * setupwindowTilewidth),setupwindowTileheight * r));}
						else if((a % 2) > 0)
							{spriteJustPlaced.m_Sprite.setPosition(sf::Vector2f((((b % 12 / 2)) * setupwindowTilewidth),setupwindowTileheight * (r + 1)));}
					}

					if(a == wPaladin) //really it's 48(4 rows x 2 colors) + 2 (wPieceEnd, bPieceEnd)
					{
						r = 0;
						spriteJustPlaced.m_Sprite.setPosition(sf::Vector2f(6 * setupwindowTilewidth,0));
					}
					if((a > wPaladin) && (a < fairyPieceEnd)) //currently 50 is wPaladin
					{
						int b = (a - 2);			  //because we skip wPieceEnd and bPieceEnd
						if((b % 12 == 0) && (a > 0)) //6 pieces per row
							{r += 2;}

						if((a % 2) == 0)
							{spriteJustPlaced.m_Sprite.setPosition(sf::Vector2f((((b % 12) / 2) * setupwindowTilewidth) + 6 * setupwindowTilewidth,setupwindowTileheight * r));} //add 720 for starting on the 7th column
						else if((a % 2) > 0)
							{spriteJustPlaced.m_Sprite.setPosition(sf::Vector2f((((b % 12 / 2)) * setupwindowTilewidth) + 6 * setupwindowTilewidth,setupwindowTileheight * (r + 1)));}
					}

					spriteJustPlaced.m_Sprite.setScale(setupwindowScale);
				}

				setupSpritesLoaded = true;
				if (isDEBUG) { outPut << " Done.\n"; std::cout << outPut.str(); }
			} //end of setupwindow sprite loading

			// Setupwindow drawing stuff

			// don't draw unless if (SetupWindow.isVisible()) //This entire loop requires that the Window is open and in SetupMode.
			BackdropTile.setScale(setupwindowScale);
			textStorage.setup_piecename.setScale(setupwindowScale);

			SetupWindow.draw(BackdropTile);

			//SetupWindow.draw(setupSelectionhighlight);

			for (Piece& J : selectionSprites)
			{
				//Moved to left-click event so it doesn't have to be checked every frame
				//Moved back because it doesn't update the highlight position after you scroll to re-select a piece, and it shows the highlight before you've selected anything (and it's 4 tiles large)
				if (J.m_PieceType == SetupPiecetype)
				{
					setupSelectionhighlight.setPosition(J.m_Sprite.getPosition());
					setupSelectionhighlight.setScale(sf::Vector2f(setupwindowScale));
					SetupWindow.draw(setupSelectionhighlight);
				}

				textStorage.setup_piecename.setString(J.GetName_Str()); //This is probably the biggest performance impact, copying strings is expensive
				textStorage.setup_piecename.setPosition(J.m_Sprite.getPosition() + sf::Vector2f(0, -3));
				SetupWindow.draw(J.m_Sprite); //Drawing the piece sprites in the setupwindow
				SetupWindow.draw(textStorage.setup_piecename);
			}

			SetupWindow.display();

			break; //why are we using a while-loop if we're just going to break?
		} //END OF SETUPWINDOW FRAMELOOP


		sf::Event boardWindowEvent;
		while (boardWindow.pollEvent(boardWindowEvent))
		{
			if (!(isSetupMode || isGameOver || isReshapeMode || isPieceSelected || Fish::isRunning))
			{ // we exclude the piece-selected state because we want to use right-click to cancel a selection without drawing anything.
				arrowOverlay.arrowCreator.m_actionMap.pushEvent(boardWindowEvent); //do this instead of .update
			}
			else
			{//if we're drawing an arrow during one of the excluded states, cancel drawing mode.
				arrowOverlay.arrowCreator.isDrawingMode = false;
			}

			if ((boardWindowEvent.type == sf::Event::Closed) || ((boardWindowEvent.type == sf::Event::KeyPressed) && (boardWindowEvent.key.code == sf::Keyboard::Q)))
			{
				colorWindow.close();
				SetupWindow.close(); //They'll take care of their own piece pointers

				turnhistory.Clear();
				ClearSquareStorage();
				EraseCapturedPieces();
				wPieceArray.clear(); //this is just to make sure their destructors are called before their sting_view (ref) gets deallocated
				bPieceArray.clear();

				boardWindow.close();
				break;
			}

			if(boardWindowEvent.type == sf::Event::Resized) // Note that maximizing a window does not count as a resize.
			{
				isSquareSelected = false;
				isPieceSelected = false;
				isDraggingPiece = false;

				boardWindow.setView(sf::View(sf::FloatRect(0, 0, boardWindowEvent.size.width, boardWindowEvent.size.height)));
				winWidth = boardWindow.getSize().x;
				winHeight = boardWindow.getSize().y;

				SqWidth = (winWidth / numColumns);
				SqHeight = (winHeight / numRows); // Scale them like this instead of .setscale(); for some reason that doesn't work well.
				HorizontalScale = (SqWidth / 120);
				VerticalScale = (SqHeight / 120);

				moveHighlight1.setScale(HorizontalScale,VerticalScale);
				moveHighlight2.setScale(HorizontalScale,VerticalScale);

				moveHighlightBuffer.create(winWidth, winHeight);
				moveHighlightBuffer.setSmooth(true); //seems like this has no effect?

				//arrowOverlay.UpdateToWindow(&boardWindow);
				float savedRotation = arrowOverlay.getView().getRotation();
				sf::View copiedView(boardWindow.getView());
				copiedView.setRotation(savedRotation);
				arrowOverlay.setView(copiedView);
			} // end resize code


			if (Fish::isRunning)
			{
				if ((boardWindowEvent.type != sf::Event::KeyPressed))
				{ continue; }

				/* auto returnedState = Fish::HandleInput(boardWindowEvent.key.code);
				if (returnedState != EngineState::Flag::nullstate)
				{ continue; } */
				if (Fish::EngineState.TakeInput(boardWindowEvent.key.code))
				{
					continue;
				}

				bool shouldIgnore{true};

				//whitelisted keys
				switch (boardWindowEvent.key.code)
				{
					//Q has already been checked
					case sf::Keyboard::F4: //start/stop engine
					case sf::Keyboard::PageUp: //debug-mode
					case sf::Keyboard::F1: //print-shortcuts
					case sf::Keyboard::Space: //flipboard
					case sf::Keyboard::F3: //flip-board per turn (probably epilepsy-inducing)
					case sf::Keyboard::F5: //clear arrow-overlay
					case sf::Keyboard::F7: //colorwindow
					case sf::Keyboard::Tilde: //FPS
					//case sf::Keyboard::E: //pointless, because each iteration resets targetDepth?
					case sf::Keyboard::J: //eval-display relative/objective toggle
						shouldIgnore = false;
						[[fallthrough]];
					default:
						break;
				}

				//toggles various displays/overlays. Not numpad0 though; that tries to get a fen string
				if ((boardWindowEvent.key.code >= sf::Keyboard::Numpad1) && (boardWindowEvent.key.code <= sf::Keyboard::Numpad9))
				{ shouldIgnore = false; }

				if (shouldIgnore) { continue; } //skips current pollevent loop
			}

			if (boardWindowEvent.type == sf::Event::KeyPressed)
			{
				//any time we press a key, assume we're changing something that makes it these pointers invalid
				hoveredPiece = nullptr;
				hoveredSq = nullptr;
				isPieceSelected = false;
				isSquareSelected = false;
				isDraggingPiece = false;

				//cancelling reshape mode
				if (isReshapeMode && (boardWindowEvent.key.code != sf::Keyboard::S) && (boardWindowEvent.key.code != sf::Keyboard::LShift))
				{
					isReshapeMode = false;

					ClearSquareStorage();
					EraseCapturedPieces();
					turnhistory.Clear();

					isWhiteTurn = !isWhiteTurn;
					AlternateTurn();
				}

				// Inner switch - START OF ALL KEYBOARD SHORTCUTS
				switch(boardWindowEvent.key.code)
				{
				/* case sf::Keyboard::Q:
				{
					colorWindow.close();
					SetupWindow.close();
					boardWindow.close(); //this does NOT push an "sf::Event::Closed" event; therefore this does NOT have equivalent behavior that block.
				}
				break; */

				case sf::Keyboard::Insert:
				{
					isBoardloaded = false;

					isSetupMode = false; //maybe you don't need to cancel setupmode?
					isSquareSelected = false;
					//SetupWindow.close();
					SetupWindow.setVisible(false);

					isGameOver = false;

					ClearBoard();
					SetBoardDimensions(); //GenerateBoard is now called by this function

					//boardWindow.setSize(boardWindow.getSize()+sf::Vector2u(1,1)); //triggering the resize-event
					//boardWindow.setSize(sf::Vector2u(winWidth, winHeight)); //triggering the resize-event
						//let the resize trigger happen later
					rules.setRankVars();

					initialPieceSetup(); //Doing this manually every time is annoying

					isDrawHighlight = false;
					moveHighlight1.setScale(HorizontalScale, VerticalScale);
					moveHighlight2.setScale(HorizontalScale, VerticalScale);
					textStorage.checkmark.setScale(HorizontalScale, VerticalScale);

					moveHighlightBuffer.create(winWidth, winHeight);
					moveHighlightBuffer.setSmooth(true); //seems like this has no effect?

					arrowOverlay.UpdateToWindow(&boardWindow);

					isWhiteTurn = false;
					AlternateTurn();
				}
				break;

				case sf::Keyboard::Delete:
				{
					isSquareSelected = false;
					isPieceSelected = false;
					isDraggingPiece = false;
					isDrawHighlight = false;

					isGameOver = false;

					ClearBoard();
					GenerateBoard(false); //otherwise the squares remain "occupied"
						//prevent it from resizing window/recalculating territory. It shouldn't need to

					isWhiteTurn = false;
					AlternateTurn();
				}
				break;

				case sf::Keyboard::Home:
				{
					isSquareSelected = false;
					isPieceSelected = false;
					isDraggingPiece = false;
					isDrawHighlight = false;

					isGameOver = false;

					if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
					{
						currentSetup.isSetupFromID = true;

						std::cout << "enter initial-position ID (range:[1, 960]): \n";
						std::cin >> currentSetup.storedID; //if this is invalid, it gets overwritten with a valid random number

						if ((std::cin.peek() == ' '))
						{
							std::cin.get(); //get the space out of the way

							if (std::isdigit(std::cin.peek())) //to prevent blocking if there's no numbers after the space
							{
								std::cin >> currentSetup.storedID_Black;
								currentSetup.Options.insert(Setup::Option::asymmetric);
							}
						}
						else //if there was only a single number specified
						{
							currentSetup.Options.erase(Setup::Option::asymmetric);
							currentSetup.storedID_Black = currentSetup.storedID;
						}
					}

					ClearBoard();
					initialPieceSetup(); //initialPieceSetup clears the arrays for you
					rules.setRankVars(); //this shortcut could possibly resize the board, if the initSetup was from FEN, but default position wasn't changed

					//save FEN only if setup-ID was specified (FischerRandom only)
					if (currentSetup.isSetupFromID)
					{
						savedFEN = WriteFEN(SquareTable);
						currentSetup.isSetupFromID = false;
					}
				}
				break;

				case sf::Keyboard::End: //just alternates whose turn it is
				{
					isPieceSelected = false;
					isDraggingPiece = false;
					isSquareSelected = false;
					if (DEBUG_DisableTurns)
					{ //manually alternate turns if turns are disabled.
						((isWhiteTurn == true) ? isWhiteTurn = false : isWhiteTurn = true);
					}

					AlternateTurn();
				}
				break;

				//Piece Setup mode toggle
				case sf::Keyboard::Tab:
				{
					isSquareSelected = false;
					isPieceSelected = false;
					isDraggingPiece = false;
					isDrawHighlight = false;

					isGameOver = false;

					// this causes the setupwindow to open on the left monitor
					//	sf::Mouse::setPosition(sf::Vector2i(1910, 480));

					ClearSquareStorage();
					EraseCapturedPieces();
					turnhistory.Clear();

					if (isSetupMode)
					{
						isSetupMode = false;
						SetupWindow.setVisible(false);

						((isWhiteTurn == true) ? isWhiteTurn = false : isWhiteTurn = true);
						AlternateTurn(); //force all pieces (including the ones we just placed) to re-generate their Movetables.
					}
					else
					{
						isSetupMode = true;
						SetupWindow.setVisible(true);
						((isFPScapped) ? SetupWindow.setFramerateLimit(60) : SetupWindow.setFramerateLimit(0));

						ChangeSetupPiece(0); //Sets all the info required for the mousepiece
						MousePiece_Setup.setScale(HorizontalScale, VerticalScale);

						textStorage.setup_mousename.setScale(HorizontalScale, VerticalScale);
						//You don't need to reposition the origin; setScale handles it properly

						SetupWindow.setPosition(sf::Vector2i(boardWindow.getSize().x, 0) + boardWindow.getPosition()); //trying to open it to the right of the boardwindow
					}
				}
				break;

			// DebugMode Toggles
				case sf::Keyboard::PageUp:
				{((isDEBUG) ? isDEBUG = false : isDEBUG = true);}
				break;

				case sf::Keyboard::PageDown:
				{
					if (DEBUG_DisableTurns)
					{
						std::cout << "Automatic turn switching enabled \n";
						DEBUG_DisableTurns = false;
					}
					else
					{
						std::cout << "Automatic turn switching disabled \n";
						DEBUG_DisableTurns = true;
					}
				}
				break;

				case sf::Keyboard::F1:
				{printShorcuts();}
				break;

				case sf::Keyboard::F2:
				{
					isBoardloaded = false;

					if (rules.changeGamemode()) //returns true if input is valid //also, sets rankvars
					{	//Maybe put changeGamemode() into a different header, so that you can do this initial setup stuff inside the function
						isSquareSelected = false;
						isPieceSelected = false;
						isDraggingPiece = false;
						isDrawHighlight = false;

						//changing game mode sometimes changes the board size, so we have to
						textStorage.checkmark.setScale(HorizontalScale, VerticalScale);

						isGameOver = false;

						//save FEN only if setup-ID was specified (FischerRandom only)
						if (currentSetup.isSetupFromID)
						{
							savedFEN = WriteFEN(SquareTable);
							currentSetup.isSetupFromID = false;
						}
					}
				}
				break;

				case sf::Keyboard::Add:
				{
					std::cout << "SAVING to savedFEN...\n";
					savedFEN = WriteFEN(SquareTable);
					std::cout << '\n';
				}
				break;

				case sf::Keyboard::Subtract: //load from FEN
				{
					isBoardloaded = false;

					std::cout << "LOADING from savedFEN...\n";
					isSquareSelected = false;
					isPieceSelected = false;
					isDraggingPiece = false;
					isDrawHighlight = false;

					isGameOver = false;

					ClearBoard();
					FENstruct parsedFEN = LoadFEN(savedFEN);

					rules.setRankVars();
					if (parsedFEN.sideToMove != '-')
					{ isWhiteTurn = !(parsedFEN.sideToMove == 'w'); }
					else { isWhiteTurn = !isWhiteTurn; } //if not specified, side-to-move will be the same after alternateTurn

					AlternateTurn(); //Generate movetables for all pieces,

					std::cout << '\n';
				}
				break;

				case sf::Keyboard::Numpad0:
				{
					std::string newFEN{};
					std::cout << "Enter FEN:\n";
					if (std::cin >> newFEN) {
						std::cout << "saved!\n";
						savedFEN = newFEN;
					} else {
						std::cin.clear();
						/*std::cin.ignore(INT64_MAX,'\n');*/ //ignores all characters in cin until it reaches newline
						std::cout << "bad input: " << '[' << newFEN << ']' << '\n';
					}
				}
				break;

				case sf::Keyboard::Space:
				{
					((isFlipboard == true) ? isFlipboard = false : isFlipboard = true);
					FlipBoard();
					//you don't even need to do this here. It won't do anything.
					//the piece sprites only update when you resize the window.
					//or recalculate their position every frame like a noob.
				}
				break;

				case sf::Keyboard::RAlt: //becaue I use leftAlt for a lot of shorcuts
				{
					SFX_Controls.Toggle_Alt_SFX();
				}
				break;

				case sf::Keyboard::F3:
				{
					((isFlipPerTurn) ? isFlipPerTurn = false : isFlipPerTurn = true);
					((isFlipPerTurn) ? std::cout << "Enabled automaticboard flipping \n" : std::cout << "Diabled automaticboard flipping \n");
					((isWhiteTurn) ? isFlipboard = false : isFlipboard = true); //Enabling make the board flip immediately; disabling during Black's turn will not un-flip the board.
				}
				break;

				case sf::Keyboard::F4:
				{
					if (isSetupMode || isReshapeMode || isGameOver || wPieceArray.empty() || bPieceArray.empty())
					{
						std::cerr << "Can't start engine; invalid state!";
						break;
					}

					//std::thread(Fish::EngineTest).detach();
					Fish::EngineTest();
				}
				break;

				case sf::Keyboard::F5:
				{
					/* for (auto& P : wPieceArray)
					{
						P->PrintAllMoves();
					} */

					/* std::cout << "FISH eval: " << TallyMaterial() << '\n';
					PrintLegalMovelist(); */

					//UndoRedoTest();

					/* for (int M{0}; M <= 9; ++M)
					{
					// int magicNumber{effolkronium::random_static::get(0,9)};
						int magicNumber{M};
						std::cout << "magicNumber = " << magicNumber << '\n';

						//these are the number of empty squares to skip before placing
						int firstKnight = (magicNumber/4);
						if ((magicNumber == 7) || (magicNumber == 9)) { firstKnight += 1; }
						int secondKnight = (firstKnight + (((magicNumber%9)%7)%4) + 1);

						std::cout << "\t firstKnight: " << firstKnight << '\n';
						std::cout << "\t secondKnight: " << secondKnight << '\n' << '\n';
					}
					return 0; */

					std::cout << "Arrow Overlay cleared! \n";
					ArrowOverlay::activeOverlay->ClearAll();

					//PrintTest();

					//LookupTable.PrintNameTable();
				}
				break;

				case sf::Keyboard::F6:
				{
					wSquareStorage.PrintAll(true);
					bSquareStorage.PrintAll(false);
				}
				break;

				case sf::Keyboard::F7:
				{
					((colorWindow.isOpen()) ? colorWindow.close() : colorWindow.create(sf::VideoMode(306,459),"Colors",sf::Style::Titlebar), //does not allow the window to be resized
					 colorWindow.setPosition(boardWindow.getPosition() - sf::Vector2i(306,0)));	//set to left of boardwindow
				}
				break;

				case sf::Keyboard::Tilde:
				{
					if ((sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))))
					{
						fpsTimer.restart();
						fpsText.setString("calculating... \n");
						frameCount = 0;
						sumOfLedger = sf::microseconds(0);

						//If you're doing the manual FPS-calculation, you cannot set the framelimit above 1000 (or unlimited), because then your average will be < 1ms, which will round to 0 and crash on division
						((isFPScapped) ? (std::cout << "FPS limit disabled \n", boardWindow.setFramerateLimit(0), isFPScapped = false) : (std::cout << "FPS limit restored (60) \n", boardWindow.setFramerateLimit(60), isFPScapped = true));

						((isFPScapped) ? timerLedger.resize(120) : timerLedger.resize(2000));

						isShowFPS = true;
						break;
					}

					((isShowFPS) ? isShowFPS = false : isShowFPS = true);
					fpsTimer.restart();

					sumOfLedger = sf::microseconds(0);
					frameCount = 0;
					fpsText.setString("calculating... \n");
				}
				break;

				case sf::Keyboard::Up:
				{
					if (isSetupMode)
					{
						SetupWindow.setVisible(true);
						//SetupWindow.setSize(sf::Vector2u(720, 540));
						SetupWindow.setPosition(sf::Vector2i(boardWindow.getSize().x, 0) + boardWindow.getPosition()); //trying to open it to the right of the boardwindow
					}
					else
					{
						desktopSize = sf::VideoMode::getDesktopMode();
						std::cout << desktopSize.width << "x" << desktopSize.height << '\n';

						//Unfortunately, windows without titlebars cannot be moved above the screen, even with windowHax.

						//THESE MAY NEED TO BE RECALCULATED, if you changed the squaresize from 120
						if (numRows > 8) //with the titlebar, even 9 squares goes off the screen
						{
							boardWindow.setPosition(sf::Vector2i(boardWindow.getPosition().x, 0));
							//std::ignore = std::system("./windowHax up"); //We have to get the window out-of-bounds first, otherwise the resize gets blocked (fucking window manager)
						}
						if (numColumns > 16) //16*120 = 1920
						{
							boardWindow.setPosition(sf::Vector2i(0, boardWindow.getPosition().y));
							//std::ignore = std::system("./windowHax left"); //we have to break the screen horizontally too, otherwise the width will get bounded
						}
						boardWindow.setSize(sf::Vector2u(numColumns * 120, numRows * 120));

						//The validity of video modes is only relevant when using fullscreen windows; otherwise any video mode can be used with no restriction.
					}

					//boardWindow.requestFocus();
				}
				break;

				case sf::Keyboard::Down:
				{
					if (isSetupMode)
					{
						SetupWindow.setVisible(false);
					}
					else
					{
						boardWindow.setSize(sf::Vector2u(960, 960));
					}
				}
				break;

				case sf::Keyboard::Left:
				{
					isGameOver = false;

					Turn* nowPrevious = turnhistory.UndoLastMove();
					if (nowPrevious == nullptr)
						{ isDrawHighlight = false; }
					else
					{
						moveHighlight1.setPosition(SquareTable[nowPrevious->m_Move.currentSqID].getPosition());
						moveHighlight1SqID = nowPrevious->m_Move.currentSqID;
						moveHighlight2.setPosition(SquareTable[nowPrevious->m_Move.targetSqID].getPosition());
						moveHighlight2SqID = nowPrevious->m_Move.targetSqID;

						if (nowPrevious->m_Move.isCapture) { SFX_Controls.Play(SFX::capture); }
						else { SFX_Controls.Play(SFX::move); }
					}
				}
				break;

				case sf::Keyboard::Right:
				{
					Turn* nowPrevious = turnhistory.RedoMove();
					if (nowPrevious == nullptr)
						break;
					else
					{
						isDrawHighlight = true;
						moveHighlight1.setPosition(SquareTable[nowPrevious->m_Move.currentSqID].getPosition());
						moveHighlight1SqID = nowPrevious->m_Move.currentSqID;
						moveHighlight2.setPosition(SquareTable[nowPrevious->m_Move.targetSqID].getPosition());
						moveHighlight2SqID = nowPrevious->m_Move.targetSqID;

						if (nowPrevious->m_Move.isCapture) { SFX_Controls.Play(SFX::capture); }
						else { SFX_Controls.Play(SFX::move); }
					}
				}
				break;

				case sf::Keyboard::Numpad1:
				{
					((isDrawTerritory) ? (std::cout << "Territory lines disabled \n", isDrawTerritory = false) : (std::cout << "Territory lines enabled \n", isDrawTerritory = true));
				}
				break;

				case sf::Keyboard::Numpad2:
				{
					((isDrawCastleIndicators) ? (std::cout << "Castling indicators disabled \n", isDrawCastleIndicators = false) : (std::cout << "Castling indicators enabled \n", isDrawCastleIndicators = true));
				}
				break;

				case sf::Keyboard::Numpad3:
				{
					((isDrawRoyalIndicators) ? (std::cout << "Royal indicators disabled \n", isDrawRoyalIndicators = false) : (std::cout << "Royal indicators enabled \n", isDrawRoyalIndicators = true));
				}
				break;

				case sf::Keyboard::Numpad4:
				{
					((isDrawPromotionIndicators) ? (std::cout << "Promotion indicators disabled \n", isDrawPromotionIndicators = false) : (std::cout << "Promotion indicators enabled \n", isDrawPromotionIndicators = true));
				}
				break;

				case sf::Keyboard::Numpad5:
				{
					if (isIndicatorsEnabled)
					{
						std::cout << "All indicators disabled \n";
						isDrawTerritory = false;
						isDrawCastleIndicators = false;
						isDrawRoyalIndicators = false;
						isDrawPromotionIndicators = false;
						isIndicatorsEnabled = false;
					}
					else
					{
						std::cout << "All indicators enabled \n";
						isDrawTerritory = true;
						isDrawCastleIndicators = true;
						isDrawRoyalIndicators = true;
						isDrawPromotionIndicators = true;
						isIndicatorsEnabled = true;
					}
				}
				break;

				case sf::Keyboard::Numpad6:
				{
					if ((sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)))) {
						MoveCircles::use_altcolors = !MoveCircles::use_altcolors;
						if (MoveCircles::use_altcolors) {
							isDrawMovecircles = true;
							std::cout << "alt-colors enabled for movetable highlights \n";
						} else std::cout << "alt-colors disabled for movetable highlights \n";
						break;
					}
					((isDrawMovecircles) ? (std::cout << "all Movetable highlights disabled \n", isDrawMovecircles = false) : (std::cout << "all Movetable highlights enabled \n", isDrawMovecircles = true));
				}
				break;

				case sf::Keyboard::Numpad7:
				{
					if (isDrawMovecircles)
					{
						((isDrawOppMovecircles) ? (std::cout << "Opponent movetable highlights disabled \n", isDrawOppMovecircles = false) : (std::cout << "Opponent movetable highlights enabled \n", isDrawOppMovecircles = true));
					}
				}
				break;

				case sf::Keyboard::Numpad8:
				{
					((isDrawDangermap) ? (std::cout << "Dangermap disabled \n", isDrawDangermap = false) : (std::cout << "Dangermap enabled \n", isDrawDangermap = true));
				}
				break;

				case sf::Keyboard::Numpad9:
				{
					ArrowOverlay::activeOverlay->m_isVisible = !ArrowOverlay::activeOverlay->m_isVisible;
					std::cout << "Arrow Overlay " << ((ArrowOverlay::activeOverlay->m_isVisible)? "enabled" : "disabled") << '\n';
				}
				break;

				case sf::Keyboard::Pause: //pause break
				{
					std::ignore = std::system("clear");
					//std::cin.unsetf(std::ios::skipws);
					
					if(std::cin.fail()) std::cout << "STDIN: FAIL\n";
					if (std::cin.bad()) std::cout << "STDIN: BAD \n";
					if (std::cin.eof()) std::cout << "STDIN: EOF \n"; //EOF not possible in stdin?
					std::cin.clear(); // unsetting error flags
					
					//surrounding our string with newlines seperates it from any other input
					std::cin.putback('\n');
					for (char C:"NIDTS_DNE") //"END_STDIN" backwards
					{ std::cin.putback(C); } assert(std::cin.get() == '\000'); // terminator inserted by string-literal - now at the start of the string
					std::cin.putback('\n'); //trailing newline is required for extraction without blocking
					//reading stdin always blocks when empty - no exemptions
					//this is why stdin never returns EOF; reads always wait for input (blocking) instead of returning EOF when empty.
					//when stdin has any error-flag set, all operations will fail silently. beware of infinite loops failing to extract characters forever
					std::cin.clear();// unsetting error flags
					
					// TODO: 'putback' inserts text at the end, not the start
					// so it's the first thing extracted every time, not the last
					// so this just doesn't solve the problem (can't read without blocking)
					// and the input buffer isn't flushed until some kind of read is performed.
					// even 'std::system("reset")' doesn't clear input buffer. This is just impossible?
					// tellg does nothing (always -1). seekg always segfaults/does nothing
					// gcount is always 1, unless 'putback' was called, then 0
					
					int loopcount = 0;
					std::string throwawayBuffer{"NIDTS_DNE"};
					// leading whitespace is ignored by extraction (>>)
					while ((std::cin >> throwawayBuffer) && (throwawayBuffer != "END_STDIN"))
					{
						std::cin.ignore(INT64_MAX,'\n'); //ignores all characters in stdin until it reaches newline
						// 		^ '.ignore' is also blocking when called on an emtpy stdin
						if(!std::cin.good()) {
							std::cout << "STDIN: NOT GOOD\n";
							exit(1);
						}
						
						// if there was anything else in the buffer, print it out.
						if (loopcount == 01) std::cout << "clearing input buffer: \n";
						if (loopcount++ > 0) std::cout << '[' << throwawayBuffer << ']' << '\n';
						// nothing prints when stdin is empty because loop ends on first cycle
						//if (loopcount > 2) break;
						
						// debug - printing each character value
						/*for (char C: throwawayBuffer)
						std::cout << C << " (" << int(C) << "), ";
						std::cout << '\n';*/
					}
					
					std::cout << "[TERMINAL CLEARED]" << '\n'; //<< loopcount << '\n';
					//std::cin.setf(std::ios::skipws);
					std::cin.clear();
					
				}
				break;

				//this is kind of a placeholder
				case sf::Keyboard::Key::T:
				{
					((rules.promoteRules.isTerritoryPromote)? rules.promoteRules.RemoveRule(PromotionRules::Trigger::Criteria::InsideTerritory) : rules.promoteRules.AddRule(PromotionRules::Trigger::Criteria::InsideTerritory));
					default_promoteInTerritory = rules.promoteRules.isTerritoryPromote; //otherwise changing gamemodes will override this

					std::cout << "Territory-based promotions " << ((rules.promoteRules.isTerritoryPromote)? "enabled" : "disabled") << '\n';
				}
				break;

				case sf::Keyboard::Key::M:
				{
					std::cout << "\v \n";
					std::string currentFEN = WriteFEN(SquareTable);

					auto endRowTop = currentFEN.find_first_of('/'); //doesn't include the ending '/'
					std::string black_rowFEN = currentFEN.substr(0, endRowTop);

					auto endRowBottom = currentFEN.find_last_of('/');
					std::string positionStr = currentFEN.substr(0, endRowBottom); //doesn't include the ending '/'
					auto beginRowBottom = positionStr.find_last_of('/');
					++beginRowBottom; //don't include the slash as first character
					std::string white_rowFEN = positionStr.substr(beginRowBottom);

				/* 	std::cout << "black-FEN: " << black_rowFEN << '\n';
					std::cout << "white-FEN: " << white_rowFEN << '\n'; */

					int whiteSetupID = GetFischerRandomID(white_rowFEN);

					bool isSetupSymmetric{true};
					if ((white_rowFEN.compare(str_toupper(black_rowFEN))) != 0)
					{
						//std::cout << "rowFENs are not symmetric!!! \v \n";
						isSetupSymmetric = false;
					}

					if(isSetupSymmetric)
					{ std::cout << "Setup ID (range:[1, 960]): " << whiteSetupID << '\n'; }
					else
					{
						int blackSetupID = GetFischerRandomID(black_rowFEN); //We have to use this before doing a comparison between the two rowFEN-strings, because str_toupper() is a permanent conversion, I think.

						std::cout << "Setup-IDs (range:[1, 960]): \t"
							<< "Black: " << blackSetupID << " , "
							<< "White: " << whiteSetupID << '\n';
					}

					std::cout << "\v \n";
				}
				break;

				case sf::Keyboard::Key::C:
				{
					isCastlePriority = !isCastlePriority;
					std::cout << "Castling " << ((isCastlePriority)? "prioritized\n" : "de-prioritized\n");
				}
				break;

				case sf::Keyboard::Key::D:
				{ //removes or adds castling/canCastleWith status to all (appropriate) pieces
					static bool SetCastlingStatusTo{false};

					if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
					{ SetCastlingStatusTo = !SetCastlingStatusTo; }

					if (SetCastlingStatusTo == true)
					{
						wSquareStorage.RestoreAllCastleRights(); //shouldn't matter which order we do this in
						bSquareStorage.RestoreAllCastleRights();
					}
					else if (SetCastlingStatusTo == false)
					{
						wSquareStorage.RevokeAllCastleRights();
						bSquareStorage.RevokeAllCastleRights();
					}

					std::cout << "Castling rights " << ((SetCastlingStatusTo)? "restored\n" : "revoked\n");
					turnhistory.Clear();
				}
				break;

				case sf::Keyboard::Key::A:
				{
					if (currentSetup.Options.contains(Setup::Option::asymmetric))
					{
						currentSetup.Options.erase(Setup::Option::asymmetric);
						std::cout << "Setup is now symmetric\n";
					}
					else
					{
						currentSetup.Options.insert(Setup::Option::asymmetric);
						std::cout << "Setup is now asymmetric\n";
					}
				}
				break;

				case sf::Keyboard::Key::S:
				{
					if (boardWindowEvent.key.shift)
					{
						if (!isReshapeMode) { break; }

						std::cout << "Resetting board-shape\n";
						manual_excludes.clear();
						excluded_C.clear();
						excluded_R.clear();

						for (auto& Sq : SquareTable)
						{
							Sq.isExcluded = false;
						}

						break;
					}

					isReshapeMode = !isReshapeMode;
					std::cout << "Reshape-Mode " << ((isReshapeMode)? "Enabled\n" : "Disabled\n");

					if (isReshapeMode)
					{
						isSetupMode = false;
						SetupWindow.setVisible(false);
						isDrawHighlight = false;
						isGameOver = false;
					}
					else
					{
						ClearSquareStorage();
						EraseCapturedPieces();

						turnhistory.Clear();

						isWhiteTurn = !isWhiteTurn;
						AlternateTurn();
					}
				}
				break;

				case sf::Keyboard::Key::E:
				{
					std::cout << '\n';
					std::cout << "Enter max search depth for engine (ply): \n";

					int tempInt; //in case insertion fails, we don't want to directly affect the real variable
					std::cin >> tempInt;
					if ((tempInt <= 0) || (!tempInt))
					{
						std::cerr << "failed to set search depth! defaulting to 3.\n";
						Fish::targetDepth = 3;
						void* dumb;
						std::cin >> dumb;
						break;
					}
					Fish::targetDepth = tempInt;
					std::cout << "search depth set to: " << tempInt << '\n';
				}
				break;

				case sf::Keyboard::Key::J:
				{
					Fish::displayRelativeEval = (!Fish::displayRelativeEval);
					std::cout << "Engine eval display set to: " << ((Fish::displayRelativeEval)? "relative" : "objective") << '\n';
				}
				break;

				default:
				break;

				} // END OF ALL KEYBOARD SHORTCUTS SWITCH
			}	  // END OF ALL BOARDWINDOW_EVENTPOLLING_KEYBOARD

			/* if(boardWindowEvent.type == sf::Event::MouseEntered)
			{
				boardWindow.requestFocus();
			} */

			if(boardWindowEvent.type == sf::Event::MouseMoved)
			{
				//This event only triggers if the mouse is still inside the window
				//std::cout << "mouse moved: " << boardWindowEvent.mouseMove.x << "," << boardWindowEvent.mouseMove.y << '\n';
				if (!(hoveredSq && hoveredSq->contains(sf::Mouse::getPosition(boardWindow))))
				{ //if current hoveredSq is NOT valid, or has changed
					hoveredSq = nullptr;
					hoveredPiece = nullptr;

					//find the hovered square //You should not do this if the mouse isn't inside the window.
					for (auto& Sq : SquareTable)
					{
						if (Sq.contains(sf::Mouse::getPosition(boardWindow)))
						{
							hoveredSq = &Sq;
							hoveredPiece = ((Sq.isOccupied)? Sq.occupyingPiece : nullptr);
							break;
						}
					}

					if (hoveredSq && hoveredPiece) //hoveredSq is still nullptr if a square wasn't found.
					{
						textStorage.DEBUG_piecehover.setString(hoveredPiece->GetName_Str());
					}
					else
					{
						textStorage.DEBUG_piecehover.setString("empty Square");
					}
				}

				if (isSetupMode)
				{
					MousePiece_Setup.setPosition(sf::Vector2f(sf::Mouse::getPosition(boardWindow)));
					//MousePiece_Setup.setPosition(sf::Vector2f(boardWindowEvent.mouseMove.x, boardWindowEvent.mouseMove.y)); // I imagine that setting the position this way is less efficient; two calls instead of one. It also risks a crash (accessing members of a union when it's not the matching type)
					//boardWindow.draw(MousePiece_Setup);
				}
				else if (isDraggingPiece) //isDraggingPiece is only set to true if you click to select a piece
				{
					MousePiece.setPosition(sf::Vector2f(sf::Mouse::getPosition(boardWindow)));
					//MousePiece.setPosition(sf::Vector2f(boardWindowEvent.mouseMove.x, boardWindowEvent.mouseMove.y));
					//boardWindow.draw(MousePiece);
				}// what is the point in repositioning these here, if they have to get repositioned again, before they're drawn??? Somehow it helps make the motion more smooth. Be warned, you can't call "display" here.
				//the mousepiece/setup piece might not be valid yet??? If they require hoveredSq first.
				//also, if you draw it here, it'll duplicate the sprite on promotion-event
			}

			if(boardWindowEvent.type == sf::Event::MouseWheelScrolled)
			{
				//it would be better to manipulate the center of a view around, but using views fucks up the position of bounding boxes and there's no way to fix that

// Use of the WindowHax script is currently disabled, because it's dangerous (mostly just annoying) when any other window is open; there's no garauntee that the script will actually manipulate the game's window.
	// It wasn't working properly anyway (scrolling limits not working properly, top should not be allowed below top-of-screen if the limits are broken)
	// This wasn't mentioned anywhere, but the script intentionally doesn't break the window outside of the boundaries unless the board dimensions are beyond a certain size.
// Also my mouse doesn't have horizontal buttons anymore.
				switch (boardWindowEvent.mouseWheelScroll.wheel)
				{
				/* case sf::Mouse::HorizontalWheel:
				{
					if(boardWindowEvent.mouseWheelScroll.delta > 0) //Right
					{
						boardWindow.setPosition(boardWindow.getPosition() + sf::Vector2i(160,0)); //right

						if(boardWindow.getPosition().x == windowPositionCheck.x) //if the window hasn't moved to the right after that, we must be at the right edge
						{
							//std::cout << "window at right edge: " << boardWindow.getPosition().x << '\n';
							std::ignore = std::system("./windowHax right");
							boardWindow.setPosition(boardWindow.getPosition() + sf::Vector2i(160,0)); //right
						}
						windowPositionCheck = boardWindow.getPosition();
						//std::cout << "windowPositionCheck updated to: " << windowPositionCheck.x << '\n';
					}

					if(boardWindowEvent.mouseWheelScroll.delta < 0) //Left
					{
						if(boardWindow.getPosition().x == 0)
							std::ignore = std::system("./windowHax left"); //left edge should actually use the windowPositionCheck as well, since it can stick on the

						boardWindow.setPosition(boardWindow.getPosition() + sf::Vector2i(-160,0)); //left
					}
				}
				break; */
//END OF HORIZONTAL SCROLLWHELL CASE//

				case sf::Mouse::VerticalWheel:
				{
					if(isSetupMode)
					{
						if(boardWindowEvent.mouseWheelScroll.delta > 0)
						{ //If you scroll up, select the previous m_PieceType
							ChangeSetupPiece(-1);
						}
						else if(boardWindowEvent.mouseWheelScroll.delta < 0)
						{ //If you scroll down, select the next m_PieceType
							ChangeSetupPiece(1);
						}
					} //End of setupMode scrollwheel

/* 					else
					{
						if (boardWindow.getPosition().y != yAtBottom) { yAtBottom = false; }
						else { yAtBottom = true; }

						//	sf::Mouse::setPosition(boardWindow.getPosition() + sf::Vector2i(boardWindow.getSize().x / 2, boardWindow.getSize().y / 2));
						//instead, save the mouse position relative to window, then reset the position after the scroll (windowHax will snap the mouse to the center of the window)
						if(boardWindowEvent.mouseWheelScroll.delta > 0)
						{ //scrolling up
							if(boardWindow.getPosition().y == 0)
								{std::ignore = std::system("./windowHax up");}

							if(!windowAtBottom)
								boardWindow.setPosition(boardWindow.getPosition() + sf::Vector2i(0,-80)); //up, actually

							if((windowPositionCheck.y < yAtBottom) && (windowPositionCheck.y != -27)) //Lock the window to the
							{
								boardWindow.setPosition(boardWindow.getPosition() + sf::Vector2i(0,-27));
								windowAtBottom = true;
							}

							windowPositionCheck = boardWindow.getPosition();
						}

						if(boardWindowEvent.mouseWheelScroll.delta < 0)
						{ //scrolling Down
							if(boardWindow.getPosition().y == boardWindow.getSize().y)
							{
								std::cout << "window at bottom edge \n";
								std::ignore = std::system("./windowHax down");
							}
							boardWindow.setPosition(boardWindow.getPosition() + sf::Vector2i(0,80)); //down
						}
					} */
				}
				break;

				default:
					break;
				}
			}

			//Piece/Square selection logic
			//This does need to be on both press and release, otherwise you can't drag and drop pieces (forced to click twice)
			if ((boardWindowEvent.type == sf::Event::MouseButtonPressed || boardWindowEvent.type == sf::Event::MouseButtonReleased) && hoveredSq && (!isReshapeMode))
			{
				if (boardWindowEvent.type == sf::Event::MouseButtonReleased)
				{
					isDraggingPiece = false;
					//hoveredSq->PrintEdgeDists();
				}

				if (boardWindowEvent.mouseButton.button == sf::Mouse::Left)
				{
					if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && (!isSetupMode)) // don't allow mouse-released to select a piece
					{
							if ((hoveredSq->isOccupied) && (hoveredSq->occupyingPiece != nullptr) && (!hoveredSq->occupyingPiece->isCaptured) && (hoveredSq->occupyingPiece->isWhite == isWhiteTurn))
							{
								isPieceSelected = true;
								selectedPiece = hoveredSq->occupyingPiece; //potentially nullptr
								isDraggingPiece = true;

								MousePiece.setTexture(Piece::GetTexture(selectedPiece->m_PieceType));
								MousePiece.setScale(HorizontalScale, VerticalScale);
								MousePiece.setPosition(sf::Vector2f(sf::Mouse::getPosition(boardWindow)));

								selectedPieceHighlight.setSize(sf::Vector2f(SqWidth, SqHeight));
								selectedPieceHighlight.setPosition(selectedPiece->m_Sprite.getPosition());
							}

							if (selectedPiece == nullptr)
							{
								isPieceSelected = false;
								isDraggingPiece = false;
							}
					}
					else if(isSetupMode)
					{
						isPieceSelected = false;
						isSquareSelected = true;
						isDraggingPiece = false;
					}

					if (hoveredSq->contains(sf::Mouse::getPosition(boardWindow)))
					{
						//I don't know why this has to go up here instead of at the bottom
							isSquareSelected = true;
							selectedSq = hoveredSq;

						if ((isSquareSelected) && (isPieceSelected) && (selectedPiece) && (!isSetupMode))
						{
							if(selectedSq->m_ID != selectedPiece->m_SqID) //prevent unnecessary searches through movetable
							for (auto& MOVE : selectedPiece->Movetable)
								{
									if ((MOVE.targetSqID == selectedSq->m_ID) && (MOVE.isLegal))
									{
										if (MOVE.isTypeCastle && !isCastlePriority && selectedPiece->m_OverlappingMoves.contains(MOVE.targetSqID))
										{ continue; }

										isDrawHighlight = true;
										//Sets last-move highlight for the piece's starting position
										moveHighlight1.setPosition(SquareTable[selectedPiece->m_SqID].getPosition());
										moveHighlight1SqID = selectedPiece->m_SqID;

										//highlight for where the piece ended up.
										moveHighlight2.setPosition(selectedSq->getPosition());
										moveHighlight2SqID = selectedSq->m_ID;

										std::set<PromotionRules::Trigger> triggered = rules.promoteRules.GetTriggered(*selectedPiece, MOVE);
										SwitchName selectedPromote{selectedPiece->m_PieceType};

										if (!triggered.empty())
										{
											if (boardWindowEvent.type == sf::Event::MouseButtonPressed)
											{ selectedPiece->m_Sprite.setPosition(selectedSq->getPosition()); } //I think you should be using mousepiece here instead. Then you wouldn't even have to check if it's mousepressed/released.

											boardWindow.draw(selectedPiece->m_Sprite); //I think you should be using mousepiece here instead
											selectedPieceHighlight.setPosition(selectedSq->getPosition());
											boardWindow.draw(selectedPieceHighlight); //we're intentionally layering the highlight OVER the Piece-sprite; so that it's visually seperate (and less prominent) from the promotion-display.

											//put OverlayPromotionSelect into HandleTriggers so it can properly handle if a promotion is forced/non-optional, and if the selection-overlay should be shown at all.
											std::set<SwitchName> promoteTo = rules.promoteRules.HandleTriggers(*selectedPiece, MOVE, triggered);

											selectedPromote = OverlayPromotionSelect(*selectedPiece, selectedSq->getPosition(), promoteTo, boardWindow);

											if (selectedPromote == selectedPiece->m_PieceType)
											{
												MOVE.willPromote = false;
												//differentiate between cancelling the move and declining promotion!
											}
										}

										//Turn newTurn(turnhistory.turnNumber, MOVE, selectedPiece);
										selectedPiece->MakeMove(MOVE);
										if (MOVE.willPromote)
										{
											selectedPiece->Promote(selectedPromote);
										}

										if (MOVE.isCapture) { SFX_Controls.Play(SFX::capture); }
										else { SFX_Controls.Play(SFX::move); }

										auto& enemyStorage = ((isWhiteTurn)? bSquareStorage : wSquareStorage);
										if (((MOVE.isCapturingRoyal) && (enemyStorage.m_Royal.empty()))
											|| (MOVE.isCapture && enemyStorage.m_Occupied.empty()))
										{
											isGameOver = true;
											textStorage.gameover.setString((isWhiteTurn)? "White Wins!!!" : "Black Wins!!!");
										}

										AlternateTurn();

										if (selectedPiece->isGivingCheck)
										{ SFX_Controls.Play(SFX::check); } //have to set this manually if not resetting every turn


										isPieceSelected = false;
										isSquareSelected = false;
										isDraggingPiece = false;

										break; //out of movetable search
									}
							}//end movetable search

							break;
						}
						/* else
						{
							isSquareSelected = true;
							selectedSq = hoveredSq;
						} */
					}

				} //End of mousebuttonLEFT (excludes mousebuttonright)

				if ((boardWindowEvent.mouseButton.button == sf::Mouse::Right) && (isPieceSelected)) //otherwise it may try and move invalid selectedPieces
				{
/* 					if(selectedPiece->isCaptured == false)
						{selectedPiece->m_Sprite.setPosition(SquareTable[(selectedPiece->m_SqID)].getPosition());} //reset the position of selected Piece */
					isPieceSelected = false;
					isSquareSelected = false; //probably unnecessary
					isDraggingPiece = false;
				}

				if (boardWindowEvent.type == sf::Event::MouseButtonReleased)
				{
					isDraggingPiece = false;

					if (isSetupMode)
					{
						if (isDEBUG)
							savedFEN = WriteFEN(SquareTable);
					}
				}

			} // End of if boardWindowEvent == mouseButtonPressed/Released
		} // End of boardWindowEvents

		//CLEAR/REDRAW AFTER EVENTS!//
		//MOVED FROM TOP OF FRAMELOOP
//calcFPS moved to end of frameloop

		boardWindow.clear(); // clear the previous frame.
		moveHighlightBuffer.clear(sf::Color(0, 0, 0, 0));

		if (isTriggerResize)
		{
			if (isDEBUG)
			{
				std::cout << "resize triggered! \n";
			}
			boardWindow.setSize(sf::Vector2u(winWidth, winHeight));
			isTriggerResize = false;
		}

		RescaleEverything();
		//MOVED FROM TOP OF FRAMELOOP//


		//setup mode erasing pieces & placing pieces
		if ((isSetupMode) && (boardWindow.hasFocus()) && (hoveredSq) && (sf::Mouse::isButtonPressed(sf::Mouse::Right) || sf::Mouse::isButtonPressed(sf::Mouse::Left)))
		{
			//static int lastChanged{-1}; //unused
			//int targetSq = hoveredSq->m_ID; //also unused

			if ((hoveredSq->isOccupied) && ((hoveredSq->occupyingPiece->m_PieceType != SetupPiecetype) || sf::Mouse::isButtonPressed(sf::Mouse::Right)))
			{
				//we don't need to remove it from squarestorage, right?
				hoveredSq->occupyingPiece->isCaptured = true;
				hoveredSq->isOccupied = false;
				hoveredSq->occupyingPiece = nullptr;

				SFX_Controls.Play(SFX::erase);
			}

			if ((!hoveredSq->isOccupied) && (sf::Mouse::isButtonPressed(sf::Mouse::Left)))
			{
				PlacePiece(SetupPiecetype, hoveredSq->column, hoveredSq->row)->m_Sprite.setScale(HorizontalScale, VerticalScale);
				SFX_Controls.Play(SFX::move);
			}

			//lastChanged = targetSq;
		}


		//reshaping board
		//this needs to affect "columnHeight" map, otherwise generate-board will undo this
		if ((isReshapeMode) && (boardWindow.hasFocus()) && (hoveredSq) && (sf::Mouse::isButtonPressed(sf::Mouse::Right) || sf::Mouse::isButtonPressed(sf::Mouse::Left)))
		{

			if (sf::Mouse::isButtonPressed(sf::Mouse::Right) && !hoveredSq->isExcluded)
			{
				//we don't need to remove it from squarestorage, right?
				if(hoveredSq->isOccupied)
				{
					hoveredSq->occupyingPiece->isCaptured = true; //you need to check if it's occupied first
				}

				hoveredSq->isOccupied = false;
				hoveredSq->occupyingPiece = nullptr;
				hoveredSq->isExcluded = true;
				excluded_C.insert({hoveredSq->column, hoveredSq->row});
				excluded_R.insert({hoveredSq->row, hoveredSq->column});
				manual_excludes.insert({hoveredSq->column, hoveredSq->row});

				SFX_Controls.Play(SFX::erase);
			}

			if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && hoveredSq->isExcluded)
			{
				hoveredSq->isExcluded = false;
				manual_excludes.erase({hoveredSq->column, hoveredSq->row});

				for (std::multimap<int, int>::iterator R{excluded_C.lower_bound(hoveredSq->column)}; R != excluded_C.upper_bound(hoveredSq->column); ++R)
				{
					if (R->second == hoveredSq->row)
					{
						excluded_C.erase(R);
						break;
					}
				}

				for (std::multimap<int, int>::iterator C{excluded_R.lower_bound(hoveredSq->row)}; C != excluded_R.upper_bound(hoveredSq->row); ++C)
				{
					if (C->second == hoveredSq->column)
					{
						excluded_R.erase(C);
						break;
					}
				}

				SFX_Controls.Play(SFX::move);
			}

		}

		static bool isCoordTextLoaded{false};
		static bool isSqTextDEBUGLoaded{false};

		if (!isBoardloaded)
		{
			//SetRankVars() //why?
			isCoordTextLoaded = false;
			isSqTextDEBUGLoaded = false;
		}

		for (auto& Sq : SquareTable)
		{
			Sq.setSize(SqWidth, SqHeight);
			//Sq.setScale(HorizontalScale, VerticalScale); //setScale screws up if they're smaller than normal

			if(!Sq.isExcluded)
			boardWindow.draw(Sq);
		}

		//RescaleEverything();

		//if the board-dimensions were changed, we'll need to generate all the coords/text for the board
		if (!isCoordTextLoaded)
		{
			coordText.clear();
			darkSqIDs.clear();
			lightSqIDs.clear();

			coordText.reserve(SquareTable.size());
			darkSqIDs.reserve(SquareTable.size());
			lightSqIDs.reserve(SquareTable.size());

			for (auto& Sq : SquareTable)
			{
				auto& text = coordText.emplace_back(std::string{Sq.m_algCoord}, textStorage.dejaVu, coordTextsize);
				text.setFillColor((Sq.isDark)? coordcolorLight : coordcolorDark);

				//If the outline is negative (growing inwards), move the coordText inward.
				float offset{((Sq.getOutlineThickness() < 0)? Sq.getOutlineThickness() : 0)};
				text.setPosition(Sq.getPosition() - sf::Vector2f{offset, offset});
				text.setScale(HorizontalScale, VerticalScale);

				((Sq.isDark)? darkSqIDs : lightSqIDs).emplace_back(Sq.m_ID);
			}
			isCoordTextLoaded = true;
		}

		if (!isSqTextDEBUGLoaded)
		{
			textStorage.DEBUG_SqIDstorage.clear();
			textStorage.DEBUG_SqIDstorage.reserve(SquareTable.size());
			textStorage.DEBUG_SqID.setScale(HorizontalScale, VerticalScale);

			for (auto& Sq : SquareTable)
			{
				auto& text = textStorage.DEBUG_SqIDstorage.emplace_back(textStorage.DEBUG_SqID);
				text.setString(std::to_string(Sq.m_ID));
				text.setPosition(Sq.getPosition());
			}
			isSqTextDEBUGLoaded = true;
		}

		isBoardloaded = true;

		//DRAWING STUFF//
		//Ideally we would only do this repositioning/rescaling if !isBoardLoaded, but it's not working properly (probably because everything else is rescaled at the start of frameloop instead of bottom)
		for (std::size_t I{0}; I < SquareTable.size(); ++I) //we have to reposition/resize text every frame, otherwise resizing looks super janky
		{
			if (SquareTable[I].isExcluded)
				continue;

			//If the outline is negative (growing inwards), move the coordText inward.
			float offset{((SquareTable[I].getOutlineThickness() < 0)? SquareTable[I].getOutlineThickness() : 0)};
			coordText[I].setPosition(SquareTable[I].getPosition() - sf::Vector2f{offset, offset});
			coordText[I].setScale(HorizontalScale, VerticalScale);
			boardWindow.draw(coordText[I]);
			if (isDEBUG)
			{
				textStorage.DEBUG_SqIDstorage[I].setPosition(SquareTable[I].getPosition());
				textStorage.DEBUG_SqIDstorage[I].setScale(HorizontalScale, VerticalScale);
				boardWindow.draw(textStorage.DEBUG_SqIDstorage[I]);
			}
		}

		wTerritoryLine.setPosition(0, ((!isFlipboard)?(numRows - wTerritory):(wTerritory))* SqHeight);
		bTerritoryLine.setPosition(0, ((!isFlipboard)?(numRows - (bTerritory-1)):(bTerritory-1))* SqHeight);
		wTerritoryLine.setSize(sf::Vector2f(winWidth, 4));
		bTerritoryLine.setSize(sf::Vector2f(winWidth, 4));

		if (isDrawTerritory)
		{
			boardWindow.draw(wTerritoryLine);
			boardWindow.draw(bTerritoryLine);
		}

		if (isDrawDangermap)
		{
			SquareStorage& enemyStorage = ((isWhiteTurn)? bSquareStorage : wSquareStorage);

			for (auto& thePair : enemyStorage.Attacking)
			{
				int SqID = thePair.first;
				dangermapHighlight.setPosition(SquareTable[SqID].getPosition());
				boardWindow.draw(dangermapHighlight);
			}
			for (auto& thePair : enemyStorage.Captures)
			{
				int SqID = thePair.first;
				dangermapHighlight.setPosition(SquareTable[SqID].getPosition());
				boardWindow.draw(dangermapHighlight);
			}
		}

		if(isDrawHighlight)
		{
			moveHighlight1.setPosition(SquareTable[moveHighlight1SqID].getPosition());
			moveHighlight2.setPosition(SquareTable[moveHighlight2SqID].getPosition());
			boardWindow.draw(moveHighlight1);
			boardWindow.draw(moveHighlight2);
		}

		//This should not be dependent on the "isDrawHighlight" variable; it's meant to determine whether the last-move highlights are legitimate, so it's false until a move has been made.
		if ((isPieceSelected) && (selectedPiece) && (selectedPiece->isCaptured == false))
		{
			selectedPieceHighlight.setSize(sf::Vector2f(SqWidth, SqHeight));
			selectedPieceHighlight.setPosition(selectedPiece->m_Sprite.getPosition());
			boardWindow.draw(selectedPieceHighlight);
		}
		//we want to draw the highlight before (under) the selected piece

		if (isDEBUG)
		{
			if (hoveredSq)
			{
				int MouseTextOffsetX{96}; //set origin to right side instead
					int MouseTextOffsetY{48};

					if(sf::Mouse::getPosition(boardWindow).y < 48) //if it's at the top of the screen
						MouseTextOffsetY = 0;
					if(sf::Mouse::getPosition(boardWindow).y > (int(boardWindow.getSize().y) - 144)) //if it's at the bottom of the screen
						MouseTextOffsetY = 144;
					if(sf::Mouse::getPosition(boardWindow).x < 192) //if it's at the left of the screen
						MouseTextOffsetX = 0;
					//if (sf::Mouse::getPosition(boardWindow).x > boardWindow.getSize().x-128)
					//	MouseTextOffsetX = 192;

					textStorage.DEBUG_mousecoord.setString(std::string{hoveredSq->m_algCoord} + '\n' + std::to_string(hoveredSq->column) + ',' + std::to_string(hoveredSq->row));
					textStorage.DEBUG_mousecoord.setOrigin(MouseTextOffsetX,MouseTextOffsetY);
					textStorage.DEBUG_mousecoord.setPosition(sf::Vector2f(sf::Mouse::getPosition(boardWindow)));
					//text.DEBUG_MouseCoord.setCharacterSize(144/(VerticalScale+HorizontalScale)); //144 is twice the normal size, to average it out
					textStorage.DEBUG_mousecoord.setScale(HorizontalScale,VerticalScale); //it loooks nicer if it's scaled, but it screws up the offsets?

					//	boardWindow.draw(text.DEBUG_MouseCoord); //if you draw it here, it'll end up underneath all the pieces
			}
		}

		// function to draw all Pieces loaded into the piece arrays
		// this inside the (top level) while BoardwindowisOpen loop
		for(std::size_t d{0}; d < wPieceArray.size(); d++)
		{
			// we don't delete captured pieces completely because we want to keep track of captured material, and also because it's a pain in the ass
			if(wPieceArray[d]->isCaptured)
			{
				continue;
			}
			else if(wPieceArray[d]->isCaptured == false)
			{ //if it's not captured, and not the selected piece
				boardWindow.draw(wPieceArray[d]->m_Sprite);

				if(isIndicatorsEnabled && (wPieceArray[d]->canCastle || wPieceArray[d]->canCastleWith || wPieceArray[d]->hasPromotions || wPieceArray[d]->isRoyal))
					boardWindow.draw(DrawPieceIndicators(wPieceArray[d]->GetIndicatorInfo())); //should add a condition to prevent this from being done every damn frame


				//drawing debug visual bounding boxes
				if(isDEBUG)
				{
					visualPieceBox.setSize(sf::Vector2f(wPieceArray[d]->m_Sprite.getGlobalBounds().width,wPieceArray[d]->m_Sprite.getGlobalBounds().height));
					visualPieceBox.setPosition(wPieceArray[d]->m_Sprite.getPosition());
					boardWindow.draw(visualPieceBox); //bounding boxes for piece sprites
				}
				//drawing checkmark if the piece is giving check
				if(wPieceArray[d]->isGivingCheck)
				{
					textStorage.checkmark.setPosition(wPieceArray[d]->m_Sprite.getGlobalBounds().left + (SqWidth * 0.6),wPieceArray[d]->m_Sprite.getGlobalBounds().top);
					boardWindow.draw(textStorage.checkmark);
				}
			}
		}

		// Drawing black pieces
		for(std::size_t d{0}; d < bPieceArray.size(); d++)
		{
			if(bPieceArray[d]->isCaptured)
			{
				continue;
			}
			else if(bPieceArray[d]->isCaptured == false)
			{
				boardWindow.draw(bPieceArray[d]->m_Sprite);

				if (isIndicatorsEnabled && (bPieceArray[d]->canCastle || bPieceArray[d]->canCastleWith || bPieceArray[d]->hasPromotions || bPieceArray[d]->isRoyal))
					{boardWindow.draw(DrawPieceIndicators(bPieceArray[d]->GetIndicatorInfo()));} //should add a condition to prevent this from being done without reason

				if(isDEBUG)
				{
					visualPieceBox.setSize(sf::Vector2f(bPieceArray[d]->m_Sprite.getGlobalBounds().width,bPieceArray[d]->m_Sprite.getGlobalBounds().height));
					visualPieceBox.setPosition(bPieceArray[d]->m_Sprite.getPosition());
					boardWindow.draw(visualPieceBox); //bounding boxes for piece sprites
				}

				if(bPieceArray[d]->isGivingCheck)
				{
					textStorage.checkmark.setPosition(bPieceArray[d]->m_Sprite.getGlobalBounds().left + (SqWidth * 0.6),bPieceArray[d]->m_Sprite.getGlobalBounds().top);
					boardWindow.draw(textStorage.checkmark);
				}
			}
		}

		//Creating Movecircles for the hovered piece here. Don't do it if a piece is selected
		if((hoveredPiece && (!isPieceSelected)) && (!isSetupMode) && (!isReshapeMode) && (!isGameOver))
		{
			if (hoveredPiece->isCaptured)
			{
				hoveredPiece = nullptr;
			}
			else
			{
				bool isOpponentTurn{false};
				if (((hoveredPiece->isWhite) && (!isWhiteTurn)) || ((!hoveredPiece->isWhite) && (isWhiteTurn)))
					isOpponentTurn = true;

				DrawMovecircles(moveHighlightBuffer, hoveredPiece->Movetable, isOpponentTurn, hoveredPiece->m_OverlappingMoves);
			}
		}

		// Creating Movecircles in the case that a piece is selected
		if ((isPieceSelected) && (selectedPiece) && (selectedPiece->isCaptured == false) && (!isSetupMode) && (!isReshapeMode) && (!isGameOver))
		{
			DrawMovecircles(moveHighlightBuffer, selectedPiece->Movetable, false, selectedPiece->m_OverlappingMoves);
		}

		arrowOverlay.arrowCreator.SendCallback(&boardWindow); //this has to be under the pollevent switch, otherwise it eats all the events, it seems. //It also has to be after the window is cleared, and the board is drawn, otherwise the board will be drawn over the arrow.

		if (arrowOverlay.m_isVisible)
		{
			boardWindow.draw(arrowOverlay);
		} // draw the arrows before (under) the movecircle-buffer, and the mousepiece sprite

		moveHighlightBuffer.display();
		sf::Sprite highlightBufferSprite(moveHighlightBuffer.getTexture());
		boardWindow.draw(highlightBufferSprite);
//static sf::Sprite highlightBufferSprite(moveHighlightBuffer.getTexture()); //a static sprite doesn't resize to fit the texture, so you're screwed if the window-size changes.

		if (isDraggingPiece) //we might need to exclude the nonstandard game-states
		{
			boardWindow.draw(selectedPieceHighlight); //redrawing the highlight OVER the selected piece (which is the sprite on the STARTING square; seperate from the mouse-piece!)

			MousePiece.setPosition(sf::Vector2f(sf::Mouse::getPosition(boardWindow))); //this should already be updated by the mousemove-event, but we should probably update it per-frame.
			boardWindow.draw(MousePiece); //we have to wait until want after drawing movecircles to layer the dragged-piece on top
		}

		if (isDEBUG)
		{
			//draw name of piece you're hovering over
			boardWindow.draw(textStorage.DEBUG_piecehover);
		}

		// drawing a sprite attachted to the mouse in placement mode
		if (isSetupMode)
		{
			MousePiece_Setup.setPosition(sf::Vector2f(sf::Mouse::getPosition(boardWindow)));
			textStorage.setup_mousename.setPosition(MousePiece_Setup.getPosition());
			boardWindow.draw(MousePiece_Setup);
			boardWindow.draw(textStorage.setup_mousename);
		}

		if (isReshapeMode && hoveredSq)
		{
			sf::RectangleShape hoveredOutline{*hoveredSq};
			hoveredOutline.setFillColor(sf::Color::Transparent);
			hoveredOutline.setOutlineColor(sf::Color(255, 230, 150, 200));
			hoveredOutline.setOutlineThickness(-3);
			boardWindow.draw(hoveredOutline);
		}

		if (isGameOver)
		{
			//don't need to reset origin, setScale takes care of everything
			textStorage.gameover.setScale(HorizontalScale, VerticalScale);
			textStorage.gameover.setPosition((boardWindow.getSize().x / 2),(boardWindow.getSize().y / 2));
			boardWindow.draw(textStorage.gameover);
		}

		if (isDEBUG) { boardWindow.draw(textStorage.DEBUG_mousecoord); }

		if (isShowFPS)
		{
			calcFPS();
			boardWindow.draw(fpsText);
		}

		#ifdef TILETEST_EXPERIMENT
		//Drawing tiles with varying number of sides
 		if(isDEBUG)
		{
			for (auto& T : testTiles)
			{
				T.rotate(1);
				boardWindow.draw(T);
			}
			for (sf::ConvexShape& T : customTiles)
			{
				T.rotate(1);
				boardWindow.draw(T);
			}
		}
		#endif
		
		//boardWindow.draw(sf::Sprite(testSpriteSheet.getTexture()));
		boardWindow.display(); // end the current frame, show everything we just drew (to the backbuffer)

	}//End of boardwindow frameloop

	return 0;
} //End of Main
