#include "Ruleset.hpp"

#include "Board.hpp" //for generateBoard
#include "GlobalVars.hpp"

//maybe just use constructors instead? Or just predefine Rulesets for each mode
Ruleset rules{ Ruleset::Mode::mode_vanilla }; //to store the user's current ruleset

//Returns each Trigger with criteria fufilled by Piece/Move, if any (also sets the "willPromote" variable)
std::set<PromotionRules::Trigger> PromotionRules::GetTriggered (Piece& thePiece, Move& theMove)
{
	std::set<PromotionRules::Trigger> activated;
	Boardsquare& targetSq = SquareTable[theMove.targetSqID];

	using Criteria = PromotionRules::Trigger::Criteria;
	using Modifier = PromotionRules::Trigger::Modifier;
	
	for (auto T : triggers)
	{
		if ((!thePiece.hasPromotions) && (!T.modifiers.contains(Modifier::Forced)))
		{ continue; }

		switch (T.criteria)
		{
			case Criteria::EndOfColumn:
			{
				if (((thePiece.isWhite)? (targetSq.edgeDist.Top) : (targetSq.edgeDist.Bottom)) == 0)
				{
					std::cout << "piece is at the end of column\n";
					activated.insert(T);
				}
				break;
			}

			case Criteria::InsideTerritory:
			{
				int enemyTerritory = ((thePiece.isWhite)? bTerritory : wTerritory);

				if ((thePiece.isWhite)? (targetSq.row >= enemyTerritory) : (targetSq.row <= enemyTerritory))
				{
					activated.insert(T);
				}
				break;
			}
			
			default:
				break;
		}
	}

	if (!activated.empty())
	{
		theMove.willPromote = true;
		//although, you should do some checks to ensure it will have at least one valid promotion?
	}

	return activated;
}

//this has to be called before the move is actually made, because it might access the piece-to-be-captured
std::set<SwitchName> PromotionRules::HandleTriggers(Piece& thePiece, Move& theMove, std::set<PromotionRules::Trigger>& triggered)
{
	using Criteria = PromotionRules::Trigger::Criteria;
	using Modifier = PromotionRules::Trigger::Modifier;
	
	std::set<SwitchName> selections;

	Boardsquare& targetSq = SquareTable[theMove.targetSqID];
	int enemyTerritory = ((thePiece.isWhite)? bTerritory : wTerritory);
	int distInside = ((thePiece.isWhite)? (targetSq.row - enemyTerritory) : (enemyTerritory - targetSq.row));
	//iterate through triggered here, if the modifiers include forced, or random, or OnCapture, manipulate the promotions here?
	for (auto T : triggers)
	{
		if (T.modifiers.contains(Modifier::Forced)) { ; } // placeholder to avoid 'unused' warning on 'using Modifier' declaration
	}
	
	for (SwitchName S : thePiece.m_Promotions)
	{
		if (thresholds.illegal.contains(S))
		{ continue; }
		else if (triggered.contains(Criteria::EndOfColumn))
		{ selections.insert(S); }
		else if (triggered.contains(Criteria::InsideTerritory))
		{
			if(!thresholds.lastRowOnly.contains(S)) //I don't know if this logic is correct
			selections.insert(S);
			for (auto P : thresholds.minDist)
			{
				if (P.first <= distInside) { continue; }
				else
				{
					if (P.second.contains(S))
					{
						selections.erase(S); //because it may have been inserted on an earlier iteration
						break;
					}
				}
			}
		}
	}

	return selections;
}

//TERRITORY SIZE/SCALING must be accounted/recalculated for BEFORE YOU CALL THIS FUNCTION
void PromotionRules::SetRankVars()
{
	isTerritoryPromote = triggers.contains(Trigger::Criteria::InsideTerritory);

	/* if (isTerritoryPromote)
	{
		wPromoteRank = bTerritory;
		bPromoteRank = wTerritory;
	}
	else
	{
		wPromoteRank = numRows;
		bPromoteRank = 1;
	} */
	
	return;
}


void Ruleset::setRankVars()
{	
	if (isTerritoryScaled.first)
	{//Territory ratios
		wTerritory = floor(numRows / territoryRatio.first); //This formula also works for Shogi. It's the best
	}
	else
	{	//static territory amounts. 0 means no territory
		wTerritory = (territoryRatio.first);
	}

	if (isTerritoryScaled.second)
	{
		bTerritory = (numRows - floor(numRows / territoryRatio.second)) + 1; //Add one otherwise it's not symmetric
	}
	else
	{
		bTerritory = (numRows - (territoryRatio.second) + 1);
	}

	promoteRules.SetRankVars();

	return;
}

//you're only ever supposed to use this function on currentRuleset
bool Ruleset::changeGamemode(std::string passedStr)
{
	bool isRulesetValid{ false };

	//random and superRandom can just be setup types, but 960 should be it's own game mode (castling rules, limited pieceset)
	std::map<Mode, std::string> mapRules {
		{mode_custom,        "custom"}, //All other rulesets will be static, and are just copied into "CurrentRuleset"
		{mode_vanilla,       "vanilla"},
		{mode_horde,         "horde"},
		{mode_gothic,        "gothic"},
		{mode_spartan,       "spartan"},
		{mode_FischerRandom, "FischerRandom"},
		{mode_moreRandom,    "moreRandom"},
		{mode_superRandom,   "superRandom"},
		{mode_shogi,         "shogi"},
		{mode_crazyhouse,    "crazyhouse"},
		{mode_circe,         "circe"},
		{mode_scotch,        "scotch"},
		{mode_horde,         "horde"},
		{mode_botMatch,      "botmatch"},
		{mode_stressTest,    "stresstest"},
	};

	static std::string ruleInput{"cancel"};
	std::stringstream passedStrStream;
	
	if (!passedStr.empty())
	{
		if (isDEBUG)
		std::cout << "PassedString = " << passedStr << '\n';
		passedStrStream.str(passedStr);
		passedStrStream >> ruleInput; //this doesn't actually remove the characters from stringstream
		
		if (isDEBUG) {
			std::cout << "ruleInput now contains: " << ruleInput << '\n';
			std::cout << "after extraction, passedStrStream contains: " << ((passedStrStream.str().empty())? "(empty)" : passedStrStream.str()) << '\n';
			std::cout << "Skipped Input!\n";
		}
		goto SkipInput;
	}

	std::cout << "\n Enter name of ruleset \n"
		<< "(enter 'cancel' to cancel, or 'list' for the valid ruleset names) \n";
	std::cout << "your current ruleset is: " << mapRules[gameMode]  << '\v' << '\n';
	
InputAgain:
	//ruleInput.clear();

	std::cin.putback('\n');
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); //this doesn't really do anything
	std::cin.putback('\n');

	//std::getline(std::cin, ruleInput);
	while (!ruleInput.empty())
	{
		//std::cin >> ruleInput;
		std::getline(std::cin, ruleInput);
		//std::cin.putback('\n');
	}
	
	std::cin >> ruleInput;
	
	// accept '960' and 'random' as 'FischerRandom'
	if (ruleInput == "960") ruleInput = "FischerRandom";
	if (ruleInput.ends_with("random") || ruleInput.ends_with("Random")) { // correcting capitalization
		if (ruleInput.starts_with("fischer")) ruleInput = "FischerRandom"; else
		ruleInput = std::string{ruleInput, 0, (ruleInput.length()-6)}.append("Random");
		if (ruleInput == "Random") ruleInput = "FischerRandom";
	}
	
	if (ruleInput.starts_with("cancel") || ruleInput.starts_with("Cancel") || ruleInput.starts_with("CANCEL")) {
		[[maybe_unused]] int status = std::system("clear"); //supress the compiler's "unused variable" warning
		
		std::cout << "\nRuleset change cancelled \v \n";
		isRulesetValid = false;

		return false;
	}
	
	//if user entered "l", print the mode names
	if ((ruleInput == "l") || (ruleInput == "L") || ruleInput.starts_with("List") || ruleInput.starts_with("LIST")) ruleInput = "list";
	if (ruleInput == "list")
	{
		isRulesetValid = false;
		std::cout << "\nGame Modes: \n";
		for (const auto& [mode, modeName]: mapRules) {
			std::cout << "  " << modeName << '\n';
		}
		std::cout << "\nEnter ruleset: \n";
		goto InputAgain;
	}

SkipInput:
	//iterating through list and either printing them out, or setting it as the current gamemode
	for (const auto& M : mapRules)
	{
		Ruleset::Mode R = M.first;
		if (M.second == ruleInput)
		{//RULESET-SPECIFIC ACTIONS PERFORMED HERE//
			//gameMode = gametype(r); //will be set by next two lines
			isRulesetValid = true;
			rules = Ruleset{ R }; //getting the default values for the new ruleset

			//There needs to be a distinction made between mode switches that require a resize and those that don't
			//	We want to avoid resetting the board if we don't have to
			//only resizes if the default is larger than the current
			bool shouldResize{ false };
			if (defaultBoardsize.first > numColumns)
			{
				numColumns = defaultBoardsize.first; shouldResize = true;
			}

			if (defaultBoardsize.second > numRows)
			{
				numRows = defaultBoardsize.second; shouldResize = true;
			}

			GenerateBoard(shouldResize); //must do this seperately in case board size has changed. (initialPieceSetup always passes false)
			setRankVars();
			//getDefaultRules(); //sets values relevant to the gameMode
			//Currently this changes the setupType as well

			if (passedStr.empty()) //if we actually passed a string, then std::cin will not contain anything (because goto skipInput), and calling "peek" or "get" will block the thread until it can steal something from std::cin
			{
				bool firstLoop{true};
				while ((R == mode_FischerRandom) && (std::cin.peek() == ' '))
				{
					std::cin.get(); //get the space out of the way

					int specifiedPositionID{0};
					if (std::isdigit(std::cin.peek())) //to prevent blocking if there's no numbers after the space
					{ std::cin >> specifiedPositionID; }
					else
					{
						std::cerr << "invalid parameter for FischerRandom!\n" << "\n Enter ruleset: \n";
						goto InputAgain;
					}

					if (specifiedPositionID > 0) //if the position was specified
					{
						currentSetup.isSetupFromID = true;
						if (!firstLoop) { currentSetup.Options.insert(Setup::Option::asymmetric); }
						((firstLoop)? currentSetup.storedID : currentSetup.storedID_Black) = specifiedPositionID;
					}

					if (firstLoop)
					{
						currentSetup.Options.erase(Setup::Option::asymmetric);
						currentSetup.storedID_Black = currentSetup.storedID;
						firstLoop = false;
					}
					
				}
			}
			initialPieceSetup();

			break;
		}
	}

	if (!isRulesetValid)
	{
		std::cout << "invalid ruleset name: " << ruleInput << "\n Enter another: \n";
		goto InputAgain;
	}

	std::cout << "Ruleset changed to: " << mapRules[gameMode] << '\n';
	return isRulesetValid;
}




