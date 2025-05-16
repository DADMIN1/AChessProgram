#include "ArrowOverlay.hpp"

//this is a static member
ArrowOverlay* ArrowOverlay::activeOverlay{nullptr};

ArrowOverlay::ArrowOverlay()
{
	//create(960, 960); //should be equal to board/window size
	sf::RenderTexture::create(winWidth, winHeight);
	setSmooth(true);
	//set scale?

	sf::Sprite::setTexture(sf::RenderTexture::getTexture(), true); //'resetRect=true' ; resizes the sprite to fit the new texture's dimensions

	activeOverlay = this;
}

void ArrowOverlay::AddArrow(ArrowShape* theArrow) //add a finished arrow to the overlay
{
	m_arrows.push_back(theArrow);
	sf::RenderTexture::draw(*theArrow);
	display();
}

//pass "isFlipboard" to this function.
void ArrowOverlay::RotateFlip(bool shouldFlip)
{
	if (m_isFlipped == shouldFlip)
	{
		if (shouldFlip == true)
		{
			setPosition(getGlobalBounds().width, getGlobalBounds().height);
			// we have to update this every frame when it's flipped; otherwise rescaling the window shifts the texture off-center // because the origin is still (0,0), which should now correspond to the bottom-right corner, so scaling will shrink/expand it in the wrong direction.
		}

		return;
	}

	m_isFlipped = shouldFlip;

	if (shouldFlip)
	{
		sf::View flippedview (ArrowOverlay::activeOverlay->getView());
		flippedview.setRotation(180);
		setView(flippedview);
		setRotation(180);
		// both view and sprite rotation need to match.

		setPosition(getGlobalBounds().width, getGlobalBounds().height); //has to be done AFTER the rotation/other transforms
		// we have to do this because SFML rotates shit in the stupidest way possible; about the origin instead of their center.
		// and if we change the origin instead, then the arrow-scaling gets fucked up.
	}
	else
	{
		sf::View unflippedview (ArrowOverlay::activeOverlay->getView());
		unflippedview.setRotation(0);
		setView(unflippedview);
		setRotation(0);
		setPosition(0, 0);
	}
	display(); //maybe unnecessary?
	return;
}

void ArrowOverlay::ClearAll()
{
	m_arrows.clear();
	RedrawArrows(); //clears the texture
	//sf::RenderTexture::create(winWidth, winHeight); //this is likely a bad idea; internally the "board size" is still 960x960. You should just set it to that and set the view/scale equal to the boardWindow instead.
	//sf::Sprite::setTexture(sf::RenderTexture::getTexture(), true); //'resetRect=true' ; resizes the sprite to fit the new texture's dimensions
}


//You can't ever call this function; it'll screw up every arrow that wasn't drawn with the current window size.
void ArrowOverlay::RedrawArrows()
{
	sf::RenderTexture::clear(sf::Color::Transparent);
	for (ArrowShape* arrow : m_arrows)
	{
		// reposition/resize here? if m_isBeingDrawn == false
		//if (A.m_isBeingDrawn == false) //need to point to ArrowShape base class?
		sf::RenderTexture::draw(*arrow);
	}
	display();
}

void ArrowOverlay::UpdateToWindow(sf::RenderWindow* window)
{
	// do not call any of these. It only works if you leave the sprite/texture as-is.
	/* sf::RenderTexture::create(winWidth, winHeight);
	sf::Sprite::setTexture(sf::RenderTexture::getTexture(), true); //'resetRect=true' ; resizes the sprite to fit the new texture's dimensions
	RedrawArrows(); */

	//Doesn't work.
	/* 		setView(window->getView());
			setScale(1.f, 1.f);
			// setView(window->getDefaultView());
			//setView(sf::RenderTexture::getDefaultView());
			RedrawArrows();

	//		setView(window->getView());
			setScale(HorizontalScale, VerticalScale);

	//		RedrawArrows(); */

	//the problem is that it's redrawing each arrow at the original default-view/scale.
	// which works perfectly if those arrows were created when the board's view was the default, but screws up any arrows that were created while the board had a non-default horizontal/vertical scale.
			// Basically, the current implementation works by drawing the arrow to a texture, then continuously scaling the sprite to match any resizes to the window. As soon as the window resizes, the stored arrows' coordinates become invalid.

	//literally just copied the constructor
	float savedRotation = getView().getRotation(); //I think recreating the rendertexture destroys the view?
	sf::RenderTexture::create(120*numColumns, 120*numRows); //internal resolution; NOT winWidth/winHeight
	//unfortunately, this clears the texture; it would be cool to keep the already-drawn arrows (they get scaled correctly)
	//RedrawArrows?

	sf::View copiedView(window->getView()); //external resolution; essentially mapping window coords/pixels to internal coordinates
	copiedView.setRotation(savedRotation);
	setView(copiedView);
	//without this, the arrows are drawn to the rendertexture with their window-coordinates; basically desyncs unless the internal resolution happens to match the window size (it won't).

	//sf::Sprite::setTexture(sf::RenderTexture::getTexture(), true); //'resetRect=true' ; resizes the sprite to fit the new texture's dimensions
	sf::Sprite::setTextureRect(sf::IntRect{sf::Vector2i{0,0}, sf::Vector2i{sf::RenderTexture::getSize()}}); //this seems to work too, but it's ugly.
	//if you don't resize the texture rectangle, the arrows will get cut off.
	
	activeOverlay = this;
}

