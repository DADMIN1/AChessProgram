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
	std::map<Mode,std::string> mapRules;
	mapRules[mode_custom] = "custom"; //All other rulesets will be static, and are just copied into "CurrentRuleset"
	mapRules[mode_vanilla] = "vanilla";
	mapRules[mode_horde] = "horde";
	mapRules[mode_gothic] = "gothic";
	mapRules[mode_spartan] = "spartan";
	mapRules[mode_FischerRandom] = "FischerRandom";
	mapRules[mode_moreRandom] = "moreRandom";
	mapRules[mode_superRandom] = "superRandom";
	mapRules[mode_shogi] = "shogi";
	mapRules[mode_crazyhouse] = "crazyhouse";
	mapRules[mode_circe] = "circe";
	mapRules[mode_scotch] = "scotch";
	mapRules[mode_horde] = "horde";
	mapRules[mode_botMatch] = "botmatch";

	mapRules[mode_stressTest] = "stresstest";
	//End mapRules

	// static std::string ruleInput;
	static std::string ruleInput{"cancel"};
	std::stringstream passedStrStream;
	
	if (!passedStr.empty())
	{
		std::cout << "PassedString = " << passedStr << '\n';
		passedStrStream.str(passedStr);
		passedStrStream >> ruleInput; //this doesn't actually remove the characters from stringstream

		std::cout << "ruleInput now contains: " << ruleInput << '\n';
		std::cout << "after extraction, passedStrStream contains: " << ((passedStrStream.str().empty())? "(empty)" : passedStrStream.str()) << '\n';
		

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
	
	if (ruleInput == "cancel")
	{
		std::ignore = std::system("clear"); //supress the compiler's "unused variable" warning
		
		std::cout << "\nRuleset change cancelled \v \n";
		isRulesetValid = false;

		return false;
	}

SkipInput:
	std::cout << "Skipped Input!\n";
	
	//iterating through list and either printing them out, or setting it as the current gamemode
	for (auto& M : mapRules)
	{
		Ruleset::Mode R = M.first;
		
		if (ruleInput == "list") //if user entered "l", print the mode name
		{
			std::cout << mapRules[R] << '\n';
			continue;
		}
		else if (mapRules[R] == ruleInput)
		{//RULESET-SPECIFIC ACTIONS PERFORMED HERE//
			//gameMode = gametype(r); //will be set by next two lines
			isRulesetValid = true;
			rules = Ruleset{ R }; //getting the default values for the new ruleset

//There needs to be a distinction made between mode switches that require a resize and those that don't
			//We want to avoid restting the board if we don't have to
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

	if (ruleInput == "list")
	{
		isRulesetValid = false;
		std::cout << '\v' << '\n';

		goto InputAgain;
	}

	if (!isRulesetValid)
	{
		std::cout << "invalid ruleset name: " << ruleInput << "\n Enter another: \n";
		goto InputAgain;
	}

	std::cout << "Ruleset changed to: " << mapRules[gameMode] << '\n';
	return isRulesetValid;
}




