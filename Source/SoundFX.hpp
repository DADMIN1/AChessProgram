#ifndef SOUNDFX_HPP_INCLUDED
#define SOUNDFX_HPP_INCLUDED

#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp> //this include seems to be required for Thor/Resources

#include <Thor/Resources.hpp>

#include <vector>
#include <map>
#include <string>

enum class SFX
{
	capture,
	move,
	check,
	erase,
};

class SFX_Controller
{
	public:
	using enum SFX; //this is only legal in C++20, with g++ version 11 or higher

private:
/* 	std::vector<sf::Sound> m_sounds;
	std::vector<sf::SoundBuffer> m_buffers;
	//sf::SoundBuffer holds the data of a sound, which is an array of audio samples.
	//sf::Sound is the object that controls the playback of the data held in a soundbuffer. Each sf::sound must be bound to a buffer; multiple sounds can use the same sound buffer at the same time.
*/
	std::string file_path_prefix{"Resource/Sounds/"};
	std::string file_extension{".ogg"};
	std::string create_path(std::string); //this is a seperate function because thor::ResourceLoader requires a function as a parameter.

	std::map<SFX, std::string> m_buffer_filepaths; //associating SFX buffer identifiers / purpose with the filename it needs to load.

	//better to use than seperate vectors/maps
	//static thor::ResourceHolder<sf::SoundBuffer, SFX> m_buffer_holder;
	thor::ResourceHolder<sf::SoundBuffer, SFX> m_buffer_holder; //if it's static, it has some fucky interactions with ResourceLoader definitions?

	std::map<SFX, sf::Sound> m_SFX_map; //we're assigning a sf::Sound to each 'SFX' enum; to be used as an identifier. We may have multiple sf::Sounds loaded for alternate playback effects/mappings, but we'll only have one assigned to each usage (check, erase, etc.) at a time. sf::Sound already keeps a reference to the buffer it's using.
		//we have to store these to preserve their settings/properties.

	bool useAltSFX{false};

	void SetBufferPaths();

public:

	SFX_Controller();
	void ReloadSound(SFX); //sets properties like volume, pitch for the specified sound; creates the sf::sound object and adds it to m_SFX_map if it doesn't already exist, or just updates it.

	void Toggle_Alt_SFX(); //not yet implemented

	void Play(SFX);

};

#endif
