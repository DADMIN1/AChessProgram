#ifndef CHESS_CONFIG_READER_HPP
#define CHESS_CONFIG_READER_HPP

#include <string>
#include <functional>
#include <unordered_map>
#include <string_view>
#include <iostream> //for fakeFunction

//Configs are read from a hardcoded filepath!!!!
//Make sure the file is named "Test.conf"

extern bool isSetupFromCustomFEN; //this needs to be declared so that main knows how to generate the board
extern std::string initSetupFEN;

inline bool fakeFunction()
{
	std::cout << "InitOptions Works!\n";
	return false;
}

class InitOptions
{
public:
	/* std::unordered_map< std::string_view, std::string_view > loadedValues{
		//{"SetupFromCustomFEN" , "true"},
	}; */

	std::unordered_map< std::string_view, std::string > loadedValues{};

	std::unordered_map< std::string_view, std::function<void()> > FunctionMap{
		{"TestKey" , fakeFunction},
		//{"InitialSavedFEN" , fakeFunction},
	};

	void ApplyOption(std::string_view)
	{
		;
	}

};

std::string ExtractValueFromString(std::string& ConfigLine);
InitOptions SetConfigValues();

#endif
