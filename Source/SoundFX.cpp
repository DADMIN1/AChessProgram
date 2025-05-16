#include "SoundFX.hpp"

#include <cassert>
#include <iostream>

bool altSFX{false};

//this is a static member
//thor::ResourceHolder<sf::SoundBuffer, SFX> SFX_Controller::m_buffer_holder;

void SFX_Controller::SetBufferPaths()
{
	if (useAltSFX == false)
	{
	m_buffer_filepaths = {
			{ capture, "CaptureStandard" },
			{ move, "MoveStandard" },
			{ check, "BerserkOther" },
			{ erase, "SelectStandard" },
		};
	}
	else if (useAltSFX)
	{
		m_buffer_filepaths = {
			{ capture, "ExplosionStandard" }, // (explosion)
			// { move, "Explosion" }, // (gunshot)
			// { move, "Gunshot_Short" }, //edited version of above; shortened
			{ move, "Gunshot_Short_PitchDown" }, //pitch lowered by 25% //also fadeout applied
			{ check, "pingOther" },
			{ erase, "SelectStandard" },
		};
	}

	for (auto& P : m_buffer_filepaths)
	{
		std::string fullpath = file_path_prefix + P.second + file_extension;
		m_buffer_holder.acquire(P.first, thor::Resources::fromFile<sf::SoundBuffer>({fullpath}), thor::Resources::KnownIdStrategy::Reload);

		ReloadSound(P.first); //this creates and inserts a sf::sound object into m_SFX_map
	}
	
	return;
}

SFX_Controller::SFX_Controller()
{
	SetBufferPaths();
	//we need a list of every filename in the directory, seperate from the binding here.
	
	return;
}

void SFX_Controller::ReloadSound(SFX ID)
{
	sf::Sound& theSound{m_SFX_map[ID]};
	theSound.setBuffer(m_buffer_holder[ID]);

	switch (ID)
	{
		case check:
			theSound.setPitch(2.75);
			theSound.setVolume(15);
			if (useAltSFX)
			{
				theSound.setPitch(0.75);
			}
			break;

		case erase:
			theSound.setPitch(0.6);
			theSound.setVolume(20);
			break;

		case move:
			theSound.setVolume((useAltSFX)? 30 : 120);
			//theSound.setPitch((useAltSFX)? 0.9 : 1.0); //pitch/playback-speed are the same
			break;
			
		default:
			break;
	}

	return;
}

void SFX_Controller::Play(SFX soundType)
{
	assert(m_SFX_map.contains(soundType) && "SFX_map didn't contain passed SFX-type!");
	m_SFX_map[soundType].play();
	return;
}

void SFX_Controller::Toggle_Alt_SFX()
{
	((useAltSFX == true) ? useAltSFX = false : useAltSFX = true);

	SetBufferPaths(); //this also calls "ReloadSound" for every SFX
	/* ideally we would just rebind m_SFX_map or change the internal buffer reference of the sf::Sound
		but we can't do the former because there's we don't have any way to store/access sf::Sounds seperately,
		we're basically doing the latter, but it's easier this way instead of adding alt enums/buffers for everything.

		Maybe an entirely seperate instance / object of SFX_Controller for alt_SFX would be more straightforward.
	*/

	std::cout << "Alt_SFX " << ((useAltSFX) ? "enabled\n" : "disabled\n");

	return;
}
