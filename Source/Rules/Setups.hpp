#ifndef SETUPS_HPP_INCLUDED
#define SETUPS_HPP_INCLUDED

#include "GlobalVars.hpp" //for isDEBUG
#include "Pieces.hpp" //Required because we need SwitchName

#include <boost/bimap.hpp> //FEN_MAP

#include <iostream> //cerr is used in FENstruct constructor
#include <sstream>
#include <set> //Setup Options
#include <list>
#include <vector>
#include <string>

extern std::string savedFEN;

class Boardsquare; //must be declared for WriteFEN (parameter is a SquareTable-ref)

//struct FENstruct; //defined below Setup, needs to be declared for Setup

//SETUPCLASS CURRENTLY UNIMPLEMENTED// (basically, it just stores the positionType for initialPieceSetup)
class Setup
{
private:
	static const boost::bimap<SwitchName, char> Default_FEN_MAP;
	boost::bimap<SwitchName, char> FEN_MAP{Default_FEN_MAP};

public:
	enum Preset
	{
		vanilla,
		gothic,
		spartan,
		shogi,
		horde,
		FischerRandom, //because these are actually unique setups
		moreRandom, //selects from pawns/piece sets, but isn't a shuffle (no default position)
		superRandom, //puts any piece from the set on any square in the territory

		knightmare, //general setup of all one type?
		stressTest,
		allPieces,
		FROM_SAVEDFEN, //currently unused
		EMPTY,
		custom,
	};
	enum Option
	{
		singleRoyal, //exactly one royal piece will be placed, no more, no less
		asymmetric,		//the setups will not be mirrored across colors, (wSetup and bSetup are different, instead of )
		noRepeat,	//The piece pattern is not repeated (not generated dynamically, which is the default)
		doubleEndpiece, //on each repeat, duplicate the last piece of the setup (normally the 1st piece would be skipped if it's the same type)
		row_shuffle, //shuffles the pieces around (in the same row)
		position_shuffle, //shuffles all piece's positions (pieces on different rows can be swapped)
		territory_shuffle, //piece's positions are completely randomized to be anywhere within the territory
		//mustPlace, //for random setups, each piece in the set must be placed before the set can be repeated
		centered, //if the setup doesn't repeat, it will be moved to the center of the board
		topTerritory, //the setup will be moved to the top of it's territory instead of staying at the bottom row
	};

	Preset positionType{vanilla};	//ONLY determines how we'll generate the initial position we'll use. Can be set independently of gameMode
	std::set<Option> Options{}; // Modifiers to positionType

	int storedID{0}; //for FischerRandom. It's entered and stored in the range of [1,960], but shifted down to [0, 959]
	int storedID_Black{0};
	bool isSetupFromID{false}; //FischerRandom sets up from storedID instead of generating a random number.

	//If you specify the FischerRandom positionID, the generated position will get stored here and can be copied to savedFEN
	// FENstruct lastGeneratedFEN; //FENstruct is overkill, the only fields that will matter are position and (maybe) castleRights.
	//std::string lastGeneratedFEN;

	//Modify_FENmap()
	//Load_FENmap()
	const boost::bimaps::bimap<SwitchName, char>& GetFENmap() { return FEN_MAP; }
	const char& GetFENchar(const SwitchName& S) { return FEN_MAP.left.at(S); }

	Setup(Preset t)
	{
		positionType = t;

	}

};

extern Setup currentSetup; //It will live in currentRuleset instead.

//For FENstruct
bool Validate1(std::string&);
bool Validate2(std::string&);
bool Validate3(std::string&);
bool Validate4(std::string&);
bool Validate5(std::string&);
struct FENstruct
{
	//std::stringstream m_FEN;
	std::string sourceFEN{"defaultSourceFEN"}; //we keep this, because it might leave fields unspecified (our writeFEN output won't match)
	std::list<std::string> extractions{};

	std::string position{"8/8/8/8/8/8/8/8/"};
	char sideToMove{'-'};
	std::string castleRights{"KQkq"};
	std::string enPassantCoord{"-"};
	int fiftyMoveCounter{0}; //actually halfmoves
	int turnCount{0};


	template <typename T>
	bool ExtractNextField(T& Member, long unsigned int maxLength, auto validChars)
	{
		auto numRemoved = extractions.remove_if([](std::string S) { return (S.empty()); });

		if(isDEBUG)
		if (numRemoved > 0) { std::cout << "removed " << numRemoved << " strings from extractions (reason: empty)\n"; }

		for (auto& insert : extractions) //very important that this is a std::string&
		{
			if (insert.length() <= maxLength)
			{
				//std::cout << "maxlength = " << maxLength << ' ';
				//std::cout << "insertion = (" << insert << ") , length: " << insert.length() << '\n';

				if (!validChars(insert)) { std::cerr << "extracted string contains invalid characters for this field!\n";  continue; }

				std::istringstream middleMan{insert};
				T tempMem;
				middleMan >> tempMem;

				if (middleMan.fail())
				{ std::cerr << "extraction failed!\n"; continue; }
				else
				{
					middleMan.peek(); //Because extracting to chars won't set the EOF flag unless we read the (next) EOF-character.
					//We have to do this after, because reading the EOF-character sets the fail-flag.
				}

				if (middleMan.eof())
				{
					Member = tempMem;
					insert.clear(); //so it will be removed from the list of possible extractions
					extractions.remove_if([](std::string S) { return (S.empty()); }); //if you don't do this here, the last extracted string will be left empty, but not removed from the list, triggering the "unread characters remaining" warning in the constructor.
					return true;
				}
				else
				{
					std::cerr << "incomplete extraction!\n";
				}

			}
		}

		extractions.remove_if([](std::string S) { return (S.empty()); });
		return false;
	}

	FENstruct(std::istringstream& toParse)
	{
		toParse >> std::ws; //get rid of leading whitespace

		sourceFEN = toParse.str();
		extractions.clear();

		if (isDEBUG)
		{ std::cout << "\nsourceFEN = (" << sourceFEN << ")\n"; }

			for (std::string section; std::getline(toParse >> std::ws, section, ' '); )
			{
				if(isDEBUG){ std::cout << "section = (" << section << ")\n"; }
				extractions.emplace_back(section);
			}

		ExtractNextField<std::string>(position, 999, &Validate1);
		if (position.back() != '/') { position.push_back('/'); }
		if (position.front() == '/') { position = position.substr(1); } //remove the first slash

		ExtractNextField<char>(sideToMove, 1, &Validate2);
		if (std::isupper(sideToMove)) { sideToMove = std::tolower(sideToMove); }

		//Castle-rights Could be longer, if we're not using standard FEN-notation
		if (!ExtractNextField<std::string>(castleRights, 4, &Validate3))
		{ //if you failed to extract castlerights, set to none, instead of defaulting to all (KQkq)
			castleRights = '-';
		}

		//note that setting an enPassantCoord without specifying a side-to-move may segfault?
		ExtractNextField<std::string>(enPassantCoord, 3, &Validate4); //Some coords can have two-digit numbers "z26"
		ExtractNextField<int>(fiftyMoveCounter, 2, &Validate5);
		ExtractNextField<int>(turnCount, 3, &Validate5);

		//auto numRemoved = extractions.remove_if([](std::string S) { return (S.empty()); }); //ExtractNextField might leave the last extracted string empty, but not remove it. You can do the additional "remove_if" here, or in "ExtractNextField" (before returning)
		/* if(isDEBUG)
		if (numRemoved > 0) { std::cout << "removed " << numRemoved << " strings from extractions (reason: empty)\n"; } */

		if (!extractions.empty())
		{
			std::cerr << "Warning: Unread remaining characters in parsed FEN: ";
			for (auto& S : extractions)
			{
				std::cerr << '(' << S << ')' << ", ";
			}
			std::cerr << '\n';
		}

	}

	void Printall()
	{
		std::cout << "position: " << position << '\n';
		std::cout << "sideToMove: " << sideToMove << '\n';
		std::cout << "castleRights: " << castleRights << '\n';
		std::cout << "enPassantCoord: " << enPassantCoord << '\n';
		std::cout << "fiftyMoveCounter: " << fiftyMoveCounter << '\n';
		std::cout << "turnCount: " << turnCount << '\n';
		std::cout << '\n';
	}
};

//if no parameters are passed, it defaults to the currentRuleset setuptype
void initialPieceSetup(Setup::Preset SetupType = currentSetup.positionType);

FENstruct LoadFEN(std::string passedFEN, boost::bimap<SwitchName, char> FEN_MAP = currentSetup.GetFENmap());
std::string WriteFEN(std::vector<Boardsquare>& SqRef); //We can pass an old, saved SquareTable to generate a FEN for it.

//Get the FischerRandom-ID of a setup. Pass the FEN string of the top or bottom row. If you don't pass anything, it'll create one from the bottom row (white-pieces). It doesn't differentiate between upper/lowercase (or, black/white pieces).
int GetFischerRandomID(std::string rowFEN = "");

#endif
