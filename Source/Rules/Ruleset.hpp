#ifndef RULESET_HPP_INCLUDED
#define RULESET_HPP_INCLUDED

#include "Setups.hpp" //constructor needs SetupClass & SetupEnums defined
#include "Pieces.hpp" //need to map SwitchName for Promotions. Should be transitively included by Setups anyway.
#include "GlobalVars.hpp" //class definition references "isDEBUG"

#include <set>
#include <initializer_list>
#include <map>
#include <string>
#include <iostream> //class definition calls std::cerr

class Move;

//This class should probably be a subclass of "RuleSet"
class PromotionRules
{
public:
	struct Trigger
	{
		enum Criteria
		{
			EndOfColumn,
			InsideTerritory,
			AtHeight, //seperate promotion-line from territory. When sq's dist board-edge == height, promote.
			OnThrone, //move onto a specific square/region of board to promote (king of the hill)
			OnCapture,
		};
		enum Modifier
		{
			NonOptional, //Player must select a promotion.
			Forced,

			/* //For the "To_" modifiers, triggers will make an insertion into the piece's promotionArray, but won't modify their "canPromote" flag, so normally un-promoteable pieces won't be affected without the "Forced" enum.
			If the "Forced" enum is included, the piece will immediately be promoted into the generated/selected type without user-selection, ignoring promotion thresholds/availability, and will even affect pieces which can't normally promote. */

			ToRandom, //any type of piece (that's included in ruleset and can't normally promote)
			ToRandom_Legal, //selecting from ONLY the types that piece would normally promote to. Doesn't affect pieces that can't promote
			ToRandom_Choice, //replaces current promotionArray with 3/4 random choices. Prompt occurs even with "Forced"; which acts the same as "NonOptional" for this modifier. If neither is included, the player can decline promotion (and potentially reroll next move)
			ToCaptured,
			IncludeCanPromote, //normally pawns/promotable pieces are excluded from possible promotion choices
		};

	/*
		//You may need a specific option for unpromoting-conditions.
		For example, in crazyhouse, capturing a promoted piece should add a held piece of the un-promoted type.
		In an "OnCapture - Forced ToRandom" ruleset, should capturing a promoted piece turn the capturing piece into the unpromoted type?
		You could create a variant centered around "OnThrone"/KotH-type, where pieces are demoted if they leave the throne/hill
	 */

		//this should really just be a multimap instead? Iterating through every modifier for a key would be annoying though

		Criteria criteria;
		std::set<Modifier> modifiers;

		//technically, < is the only operator overload we need for std::set to function properly
		bool operator< (const Trigger& rhs) const { return (criteria < rhs.criteria); };
		bool operator< (const Criteria& rhs) const { return (criteria < rhs); };
		//bool operator<= (const Trigger& rhs) const { return (criteria <= rhs.criteria); };
		//bool operator== (const Trigger& rhs) const { return (criteria == rhs.criteria); };

		Trigger(Criteria C, std::initializer_list<Modifier> M = {})
		{
			criteria = C;
			modifiers = M;
		}
	};


	std::set<Trigger> triggers;
	bool isTerritoryPromote{false}; //placeholder; just check if triggers.contains(insideTerritory)
//bool EndOfColumnPromote{true}; //eventually, split the functionality of PromotionRank variables with this
	//int wPromoteRank; //sort of placeholders, eventually these will be used for triggering promotions seperate from territory/EndOfColumn
	//int bPromoteRank;

	struct Thresholds
	{
		std::set<SwitchName> lastRowOnly{wQueen, bQueen};
		std::set<SwitchName> illegal{wPawn, bPawn, wHoplite, bHoplite}; //don't promote to any promote-able piece
		std::map<int, std::set<SwitchName>> minDist{{1, {wArchbishop, bArchbishop, wChancellor, bChancellor}}};
	};

	Thresholds thresholds;

	std::set<Trigger> GetTriggered (Piece&, Move&);
	std::set<SwitchName> HandleTriggers(Piece&, Move&, std::set<Trigger>&);

	//updates this class's variables based on contents of triggers
	//TERRITORY SIZE/SCALING must be accounted/recalculated for BEFORE YOU CALL THIS FUNCTION
	void SetRankVars();

	bool contains(Trigger::Criteria C)
	{ return (triggers.contains(C)); }

	void AddRule(Trigger::Criteria T, std::initializer_list<Trigger::Modifier> M = {})
	{
		if (triggers.contains(T))
		{
			if (isDEBUG) std::cerr << "PromotionRules already contained entry for criteria: " << T << '\n';
			triggers.erase(T);
		} //if you try an insert or emplace a duplicate into a set, it will keep the original instead of replacing it.

		triggers.emplace(T, M);
		SetRankVars();
		return;
	}

	void RemoveRule(Trigger::Criteria T)
	{
		triggers.erase(T);
		SetRankVars();
		return;
	}


	//constructors
	PromotionRules(Trigger::Criteria C, std::initializer_list<Trigger::Modifier> M = {})
	{
		triggers.emplace(C, M);
		SetRankVars();
	}

	PromotionRules()
		: PromotionRules(Trigger::Criteria::EndOfColumn, std::initializer_list{Trigger::Modifier::NonOptional})
	{
	}

	//To reset PromotionRules, you can call: currentRuleset.promoteRules = PromotionRules();

};

class Ruleset
{
public:
	enum Mode
	{
		mode_vanilla,
		mode_gothic,
		mode_horde,
		mode_spartan,
		mode_shogi,
		mode_crazyhouse,
		mode_FischerRandom, //unique castling rules, number of pieces/pawns restricted to
		mode_moreRandom, // these exist to allow for a ruleset for these setups
		mode_superRandom, //
		mode_scotch,
		mode_circe,
		mode_alice,
		mode_custom,
		mode_botMatch, //AI plays against itself
		mode_RTS, //See design doc for more info
		mode_stressTest, //initial position is all queens of alternating colors //This should be a setup instead?
	};
	enum Options
	{
		limitKing,
		designateRoyals,
		hasPieceDrops,
		isCaptureToHeld,
		hasProgressiveTurns,
		hasCirceCaptures,
		timeLimit,
		noDraw,
		randomEvents,
		botMatchAutostart, //currently unimplemented
	};

	enum Castle_Rules
	{
		//these options are additive, except for no_castling, obviously
		//for example, including both unlimited_dist and static_dist would perform calculations for both types of castlings

		no_castling,
		unlimited_dist, //king moves as far as it needs to reach the rook
		static_dist, //MAX distance the king moves when castling (determined by distCastle), rook will move as far as it needs to reach the king
		static_destinations, //Interprets distCastle {left, right} as {column1 , column2} destinations (for King)
	//should rookflip be disabled for static_destinations?

	onMove_loseCanCastle, //any move will disable (SPECIFICALLY) canCastle for that piece
	onMove_loseCastleWith, //any move will disable castlewith for that piece
	//if these aren't included, then pieces only lose the flag when they actually castle/castledwith

	//These will preserve the flags even if onMove_loseCastle/loseCastleWith is included, basically acting as exceptions
	onCastle_preserveCastling, //when a piece castles, it keeps it's canCastle and/or canCastleWith statuses (the piece it castled with loses them unless the following option is also set)
	onCastleWith_preserveCastling, //when a piece is castled with, it keeps it's canCastle and canCastleWith status
	onRookFlip_preserveCastleWith, //a Piece making a "RookFlip" move will not lose it's canCastle or canCastleWith status (the piece it's flipping over still does)
	onRookFlip_preserveCastling, //the piece the Rook is flipping over does not have it's castling disabled (the rook still does)
	//the rookflip options are specific exceptions to the global castling rules (if they would not normally be preserved)
	//Otherwise, they're redundant; Rookflip is considered canCastleWith, and the piece it's flipping over is considered castling; they are affected by the two global options

	pieceDrop_noCastle, //dropped pieces will have canCastle and canCastleWith disabled

	};
	enum GameEventlist //random events that can occur during gameplay
	{
		pieceShuffle, //each piece of the same color swaps their positions randomly. Probably should prevent shuffling into check
		transmogrify, //some number of pieces are randomly promoted/de-promoted?
		movesetShuffle, //pieces of a certain type have their movesets swapped with another type
		boardExpand,
		boardShrink,
	};

public:
	Mode gameMode{mode_vanilla}; //basically defines the initial values for all these variables (most importantly pieceSets, boardSize, TerritoryScale)
	PromotionRules promoteRules{};

	//should this also be moved to Setupclass?
	std::pair<int,int> defaultBoardsize{8, 8};
	std::pair<double,double> territoryRatio{3, 3}; //ratio white, ratio black (numRows/ratio)
	std::pair<bool,bool> isTerritoryScaled{true,true}; //if false, territoryRatio is interpreted as a static number of rows, regardless of boardsize. (can create overlap) //pair is for white territory / black territory

	struct castleRulesStruct
	{
		//bool unlimitedDist{true}; //limit the distance the king can move for castling (to distCastle), the rook will move as far as it needs to instead
		//bool staticCastling{false}; //enabling this causes the distCastle values to be interpreted as destination columns, rather than relative distances from the King's starting position (960 castling)
		std::pair<int, int> distCastle{2, 2}; //distance moved Left(queenside),Right(kingside). Set this to {0,0} to disable castling altogether
		//bool addStaticSquares{false}; //gives you the static-distance castling options IN ADDITION to the dynamic (unlimited-dist) squares
		//bool loseCastleWith{true}; //you can only use a piece as a castling target once (moving still erases it)
		//bool loseCanCastle{true}; //you can only castle with a piece once (moving still erases it)
		std::set<Ruleset::Castle_Rules> CastleOptionsList{};
	};

	// castleRules go here
	//castleRulesStruct nineSixty{false, true, {3, 7}, false}; //snaps the King to C & G files when castling

	//These will all be turned into RulesetEnums
	int m_limitKing{-1}; //limits the number of royal pieces per player - promotions and piecedrops that create more than this will become illegal. -1 is unlimited//should limitKing also apply to setups?
	bool m_designateRoyals{false}; //if true, royalty is manually (or randomly) assigned to a number of pieces (== limitKing)
	bool m_hasPieceDrops{false};	//Can pieces be dropped onto the board? This doesn't determine whether (or how) you get held pieces.
	bool m_isCaptureToHeld{false}; //usually the same value as pieceDrops, but in some cases players will start with held pieces instead of gaining them.
	bool m_hasProgressiveTurns{false};
	bool m_hasCirceCaptures{false};
	//bool mustCaptureAll{false}; //Instead, set limitKing=0 and designateRoyals=true : 0 pieces will be designated as royal.

	//void getDefaultRules(); //member function to set defaults for each gameMode
	//void setGamemodeRules(); //for creating a custom ruleset

	//move to main (setups will be altered by GUI)
	//void changeSetuptype(); //I need something to change the initialPositiontype independently of changeGameMode.

	bool changeGamemode(std::string = "");	//for switching to a premade ruleset. Returns true if the entry was valid/not cancelled
	void setRankVars(); //sets territory/promotion ranks based on ruleset

	Ruleset (Mode baseMode)
	{
		gameMode = baseMode;

		if(default_promoteInTerritory)
			promoteRules.AddRule(PromotionRules::Trigger::InsideTerritory);
		else
			promoteRules.RemoveRule(PromotionRules::Trigger::InsideTerritory);

		//modes that don't specify a defaultBoardsize shouldn't change the current size on switch
		//defaultBoardsize = {numColumns, numRows};
		//better workaround added to changeGameMode() to only resize if the default is larger than current

		switch(baseMode)
		{
		case mode_custom:
		currentSetup.positionType = Setup::Preset::allPieces;
		break;

		default:
			std::cerr << "Ruleset has no entry for this mode!\n";
			[[fallthrough]];
		case mode_vanilla:
		currentSetup.positionType = Setup::Preset::vanilla;
		defaultBoardsize ={8, 8};
		break;

		case mode_horde:
		currentSetup.positionType = Setup::Preset::horde;
		isTerritoryScaled ={true, false};
		//territoryRatio = {((numRows/5) + (numRows/4) + (numRows/3) - 3), 2};
		//the problem is numRows is only evaluated when you change rulesets, not when you just change boardsize
		territoryRatio ={2.1, 2};
		break;

		case mode_spartan:
		currentSetup.positionType = Setup::Preset::spartan;
		break;

		case mode_gothic:
		defaultBoardsize ={10, 8};
		currentSetup.positionType = Setup::Preset::gothic;
		break;

		case mode_stressTest:
		defaultBoardsize ={26, 26};
		currentSetup.positionType = Setup::Preset::stressTest;
		break;

		case mode_FischerRandom:
		currentSetup.positionType = Setup::Preset::FischerRandom;
		defaultBoardsize = {8, 8};
		//force boardsize 8x8?
		break;

		case mode_moreRandom:
		currentSetup.positionType = Setup::Preset::moreRandom;
		break;

		case mode_superRandom:
		currentSetup.positionType = Setup::Preset::superRandom;
		break;

		case mode_shogi:
		defaultBoardsize ={9, 9};
		currentSetup.positionType = Setup::Preset::shogi;
		m_hasPieceDrops = true;
		m_isCaptureToHeld = true;
		promoteRules.AddRule(PromotionRules::Trigger::Criteria::InsideTerritory);
		break;

	 	/*case mode_botMatch:
			currentSetup.positionType = Setup::Preset::custom;
		break; */

		} //END GAMEMODE SWITCH

		//this is a constructor, nothing should get executed just from creating a ruleset
		/*numColumns = defaultBoardsize.first;
		numRows = defaultBoardsize.second;*/



	} //END RULESET CONSTRUCTOR

}; //END RULESET CLASS//

//maybe just use constructors instead? Or just predefine Rulesets for each mode
extern Ruleset rules; //to store the user's current ruleset






#endif
