#include "ConfigReader.hpp"

#include "GlobalVars.hpp" //numColumns/numRows

#include <fstream> //file i/o operations
#include <iostream>
//#include <string_view>
#include <sstream>


std::string initSetupFEN{""};

bool isSetupFromCustomFEN{false};

std::string ExtractValueFromString(std::string& ConfigLine)
{
	if (ConfigLine.empty())
	{
		std::cerr << "ExtractValueFromString was passed empty string \n";
			return "";
	}

	auto splitAt = ConfigLine.find('=');
	//Prints first half of the line
	std::cout << "Loading Config: " << ConfigLine.substr(0,splitAt) << ": "; //Does this need a newline?
	++splitAt; //Moving past the equals-sign

	//Iterating through any whitespace space that should follow the '='
	while ((ConfigLine.at(splitAt) == ' ') || (ConfigLine.at(splitAt) == '\t'))
	{
		++splitAt;
	}

	//Returns second half
	return ConfigLine.substr(splitAt);
}


InitOptions SetConfigValues()
{
	InitOptions loadedOptions;

	std::ifstream configFile{"Config/Test.conf"};
	if (!configFile) { std::cerr << "Unable to open config file!!!\n"; return loadedOptions; }

	while (configFile) //ifstream returns false when it reaches the end-of-file marker
	{
		std::string inputLine;
		std::getline(configFile,inputLine);
		std::stringstream extractedValue; //stringstreams understand how to convert chars into ints/floats/etc

		//ignore comment-lines in the config file
		if (inputLine.starts_with('#'))
		{
			continue;
		}

		if (!inputLine.empty()) //for some reason it always passes an empty line on the last loop
		{
			extractedValue << ExtractValueFromString(inputLine);
			if (extractedValue.str().empty())
			{
				std::cerr << "Config's value is empty\n";
				continue;
			}
			else { std::cout << extractedValue.str() << '\n'; }

			//if we read in a true/false string, convert it to a bool/number
			//for some reason, using .clear() instead of .str("") causes this to fail.
			if (extractedValue.str() == "true") { extractedValue.str(""); extractedValue << true; }
			if (extractedValue.str() == "false") { extractedValue.str(""); extractedValue << false; }
		}
		else
			{continue;} //don't do any checks if inputLine was empty

		if (inputLine.starts_with("DefaultGameMode"))
		{
			loadedOptions.loadedValues.insert_or_assign("DefaultGameMode", extractedValue.str());
		}

		//Applying Config values
		if (inputLine.starts_with("SetupFromCustomFEN"))
		{
			extractedValue >> isSetupFromCustomFEN;

			//bool conversion tests
/* 			if (isSetupFromCustomFEN == true)
			{std::cout << "CustomFEN set to true! \n";}
			if (isSetupFromCustomFEN == false)
			{std::cout << "CustomFEN set to false! \n";} */
		}

		if (inputLine.starts_with("InitialSavedFEN"))
		{
			std::cout << "Using Custom_FEN overrides the board-size. \n";
			initSetupFEN = extractedValue.str(); //get the whole line, using the extraction operator will not extract the side-to-move, castling, etc.
			//this should actually be from "Override_initial_FEN option"
		}
		
		//Unfortunately, even if the values are unused, the extraction function prints them out.
		//Also, it requires that the Config file lists variables in a certain order
		if ((inputLine.starts_with("InitialBoardSize")) && (!isSetupFromCustomFEN))
		{
			extractedValue >> numColumns;
			char garbageChar; //this exists so we can throw away the '/' between the two numbers
			extractedValue >> garbageChar;
			extractedValue >> numRows;

			//extraction test
			//std::cout << "Config set boardsize to: " << numColumns << 'x' << numRows << '\n';
		}

		if ((inputLine.starts_with("Init_DEBUG_Toggle")))
		{
			extractedValue >> isDEBUG;
		}

		if ((inputLine.starts_with("Default_PromoteInTerritory")))
		{
			extractedValue >> default_promoteInTerritory;
		}

	}

	return loadedOptions;
}
