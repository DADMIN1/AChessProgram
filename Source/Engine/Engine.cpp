#include "Engine.hpp"
#include "EngineCommand.hpp"

#include "ZobristHash.hpp"
#include "CoordLookupTable.hpp"
#include "Pieces.hpp" //both are required for zobrist hashing
#include "SquareStorage.hpp"
#include "Setups.hpp" //to get the FENmap
#include "GameState.hpp" //for AlternateTurn and isWhiteTurn
#include "TurnHistory.hpp"
#include "FPSmath.hpp" //for CodeTimer

#include <unordered_map>

#include <ranges>
#include <cassert>
#include <iostream>
#include <sstream>
#include <array>
#include <set> //for hashtable
#include <unordered_set>
#include <algorithm> // for std::sort
#include <optional>
#include <iterator> // for std::move_iterator ; used in sorting moves

#include <effolkronium/random.hpp>


#define ENGINE_SUBSCRIPT_OVERLOAD //overloads transposition-table's operator[] to call the custom (Record) constructor, deletes default constructor. //without this, it's a pain in the ass because you have to manually set the hash for every newly-constructed record.

//#define DEBUG_ENGINE_SUB_OPERATOR //very bad idea if you're searching past depth 1 //prints stuff every time you call the subscript overloads

//#define ENGINE_DONT_HASH_TURNS

//#define ENGINE_INITIAL_PRINT

#define ENGINE_USEALT_UNDOLASTMOVE // use 'turnhistory.AltUndoLastMove()' instead of 'turnhistory.UndoLastMove()'

//nested "ifdef / endif" statements do work correctly, don't worry about it.

namespace Fish {
bool isRunning{false};
bool isActive{false};
std::map<std::size_t, std::string> pieceHeaders{}; // like: "White Bishop {d1}:"
};
struct Transposition_Record
{
	zhash_t hash;
	int depth_startFrom{0}; //depth the position was created at/evaluated from. (minimum it can be transposed into)
	int depth_searchedTo{0};
	bool hasEval{false}; //this position's search hasn't been fully explored to the max-depth, meaning it's eval can't be used.
	bool isRootMove{false}; //flag to prevent it from ever being overwritten (root-moves will be the only ones that keep starting-depth 0; which would otherwise be an indication that the record was a temporary/default-constructed)

	enum class result {
		WhiteWin,
		BlackWin,
		Draw,
		Stalemate,
	};

	std::optional<result> gameEnd; //indicates that the eval represents a game-over (win/loss/draw); in which case the depth-

	int eval{0}; // relative (difference from initial-position eval), not absolute. That way, the default, 0, is a reasonable value instead of a potential best score. Most of the time, transpositions without known evals will be encountered due to repetitions (probably a draw) anyway.

#ifdef ENGINE_SUBSCRIPT_OVERLOAD
	Transposition_Record() = delete;
#else
	Transposition_Record() = default;
#endif

	Transposition_Record(const zhash_t& H) : hash{H}
	{ assert((hash == H) && "transposition_record constructor did not copying hash!\n"); }

	Transposition_Record(const Transposition_Record&) = default; //copy constructor
	//Transposition_Record(Transposition_Record&&) = default; //move constructor
	//Transposition_Record& operator=(const Transposition_Record&) = default; //copy assignment
	//Transposition_Record& operator=(Transposition_Record&&) = default; //move assignment
};




//The Transposition_Record stored in "transpositionTable" does NOT!!! share a state with the base-class of this object!!!
//That is intentional; so that we can keep the previous depth's evals, depths, and "hasEval" state in the transpositionTable, while we copy and recursive_search with fresh copies of init-rootmoves each iteration.
struct Rootmove_t : public Transposition_Record
{
	//using Rec = Transposition_Record;

	std::size_t accessNum; //the index used to access the related Piece* in SavedVolatiles.savedMovetables[]. Also pieceHeaders
	Move m_move;
	bool isWhite;

	char m_FENchar;
	std::stringstream m_stream{};

	std::string displayEval()
	{
		if (gameEnd) //assumes checkmate
		{
			//using result = Transposition_Record::result;
			// std::string rStr{((rec.gameEnd == result::WhiteWin)? "WhiteWin!" : "BlackWins!")};
			return std::string{std::string((gameEnd == result::WhiteWin)? "(+\u221E)" : "(-\u221E)") + " #" + std::to_string(depth_searchedTo)};
		}


		int objectiveEval = eval + Fish::initEval;
		if (Fish::displayRelativeEval) { objectiveEval = eval; }

		char prefix = ((objectiveEval >= 0)? '+' : '-'); //std::showpos doesn't work anymore, so we have to do this instead. This causes double negative signs though.

		return std::string{prefix + std::to_string(objectiveEval)};
	}

	auto GetStream()
	{
		m_stream.str(""); //erases the current stream
		//for some reason this doesn't show '+' on positive numbers
		m_stream << "  [" << std::showpos << displayEval() << std::noshowpos << "] " << m_FENchar << ((m_move.isCapture) ? " x " : " -> ") << m_move.targetAlgCoord << "  //(@Depth:" << depth_searchedTo << ')' << '\n';

		return m_stream.view();
	}

	//index num is used to access the
	Rootmove_t(const zhash_t& H, const std::size_t& index, const Move& c, const bool& d, const char& f)
		: Transposition_Record(H), accessNum{index}, m_move{c}, isWhite{d}, m_FENchar{f}
	{
		isRootMove = true;
		depth_startFrom = 0;
		//m_stream.str("");
	}

	Rootmove_t(const Rootmove_t& RR)
		: Transposition_Record(RR),
		accessNum{RR.accessNum},
		m_move{RR.m_move},
		isWhite{RR.isWhite},
		m_FENchar{RR.m_FENchar}
		//m_stream{RR.m_stream.str()}
	{
		isRootMove = true;
		depth_startFrom = 0;
		//m_stream.str("");
	}



	/* Rootmove_t& operator=(const Rootmove_t& RR) //must be defined for std::sort
	{
		Rootmove_t tmp{RR};
		std::swap(*this, tmp);
		//accessNum = RR.accessNum;


		return *this;
	} */
};

#ifdef ENGINE_SUBSCRIPT_OVERLOAD
// you can only create a map with a type that isn't default-constructable if you overload the subscript operator to always call a custom constructor. (at least, the subscript-operator requires it to be default-constructable; it probably wouldn't be a requirement if you didn't use it. But using the "emplace" or "insert" functions are a pain in the ass)
// operator overloads to make the subscript operator actually call the Transposition_Record constructor
template<>
std::unordered_map<zhash_t, Transposition_Record>::mapped_type& std::unordered_map<zhash_t, Transposition_Record>::operator[](const zhash_t& Z)
{
#ifdef DEBUG_ENGINE_SUB_OPERATOR
	std::cout << "custom subscript overload called! \n";
#endif
	return (emplace(std::make_pair(Z, Transposition_Record(Z))).first)->second;
};

//there's also an overload for r-ref //probably unnecessary???
template<>
std::unordered_map<zhash_t, Transposition_Record>::mapped_type& std::unordered_map<zhash_t, Transposition_Record>::operator[](zhash_t&& ZZ)
{
	const zhash_t Z = ZZ; //there's no point in doing this; it anything it's worse because this local variable is being fed to the constructor (which constructs the hash from a reference)
#ifdef DEBUG_ENGINE_SUB_OPERATOR
	std::cout << "custom r-ref subscript overload called! passed value: " << Z << '\n';
#endif
	return (emplace(std::make_pair(Z, Transposition_Record(Z))).first)->second;
};
// You must define these overloads in the global scope, for some reason? (Literally not allowed elsewhere; probably required to be in the same scope as the std namespace)

#endif //end of "engine subscript overload" macro

/* template<>
std::unordered_map<zhash_t, Transposition_Record>::mapped_type& std::unordered_map<zhash_t, Transposition_Record>::operator[](Rootmove_t& R)
{
	return (emplace(std::make_pair(Z, Transposition_Record(R))).first)->second;
}; */


namespace Fish { //must be inside namespace to use "currentDepth", etc

struct Transpose_Flags // indicates whether a movehash (transposition_Record) returned by operator[] should be treated as new, and whether it's eval should be used or not
{
	bool shouldWrite{true};
	bool shouldUseEval{false};
	bool shouldTranspose{false}; // basically indicates that you should quit the search immediately (even if you can't use the eval); the node will be searched from a different line instead.

	//bool isCheck{false}; //for quiesence_search
	//bool isPromote{false}; //indicates that we also need to create the other hashes
	//bool isLocalTransposition{}; //the transposition is a repetition

	void SetRecordFlags(Transposition_Record& Rec, const Move& M, const bool isWhite)
	{
		Rec.depth_startFrom = currentDepth;
		Rec.depth_searchedTo = currentDepth;

		if (M.isCapturingRoyal) //if checkmate
		{
			Rec.hasEval = true;
			Rec.gameEnd = ((isWhite)? Transposition_Record::result::WhiteWin : Transposition_Record::result::BlackWin);

			shouldWrite = false;
			shouldUseEval = true;
			shouldTranspose = false;
		}

		Rec.eval = M.proclivity;

		return;
	}

	Transpose_Flags(Transposition_Record& Rec, const Move& M, const bool isWhite)
	{
		if (Rec.isRootMove) { shouldWrite = false; shouldUseEval = Rec.hasEval; shouldTranspose = true; return; }
		if (Rec.gameEnd) { shouldWrite = false; shouldUseEval = true; shouldTranspose = true; return; } //do not overwrite regardless of the stored depths. //actually we should update the depths to show the distance-to-mate?? //no, because we also want to show draws.

		if (Rec.hasEval) // false by default
		{
			if (Rec.depth_startFrom < currentDepth)
			{
				shouldWrite = false;
				shouldUseEval = true;
				shouldTranspose = true;
			}

			if (Rec.depth_startFrom > currentDepth) //overwrite the entry
			{
				Rec.hasEval = false;
			}

			// if the record is out of date (and starts at this depth), update/overwrite it
			if ((Rec.depth_startFrom == currentDepth) && ((targetDepth - Rec.depth_searchedTo) >= (Rec.depth_searchedTo - Rec.depth_startFrom)))
			{
				Rec.hasEval = false;
				//Rec.depth_searchedTo = currentDepth;
			}
		}
		else if ((!Rec.hasEval) && (Rec.depth_startFrom < currentDepth) && (Rec.depth_startFrom > 0) && (!Rec.isRootMove)) //REMEMBER, IF IT'S A NEW NODE, THE DEPTH VARIABLES HAVEN'T BEEN SET YET! THEY'RE STILL 0!!!
		{ //this should basically only happen if there's a repetition in the local line or something
			//++transpositionCount;
			shouldWrite = false;
			shouldUseEval = false;
			shouldTranspose = true;
		}

		if (shouldWrite)
		{
			SetRecordFlags(Rec, M, isWhite);
		}
	}
};
}


// DOES apply the "side-to-move" hash  // also sets move-proclivity (if it's a capture)
// You should pass the stored enemy squarestorage (const ref), or find the target piece yourself beforehand
// zhash_t HashMoveCapture(Move& M, const SwitchName Ptype, const bool whiteToMove, const zhash_t fromHash, const SquareStorage& enemystorage)
zhash_t HashMove(Move& M, const SwitchName Ptype, const bool whiteToMove, const zhash_t fromHash)
{
	assert((M.isLegal) && "Illegal Move in Engine Search! (HashMove)");

	zhash_t resultHash = fromHash;
	resultHash xor_eq zhashtable[M.currentSqID][Ptype]; // xor-ing out the moving piece
	// does the order of xor operations matter??? I don't think so?
	if (M.isCapturingRoyal)
	{
		M.proclivity = ((whiteToMove)? 99999 : -99999); //not integer limits because that's prone to overflow
		auto& target = ((whiteToMove? bSquareStorage : wSquareStorage).m_Royal.at(M.targetSqID));
		resultHash xor_eq zhashtable[target->m_SqID][target->m_PieceType];
	}
	else if (M.isEnpassant)
	{
		M.proclivity += ((whiteToMove)? 100 : -100);
		auto& target = ((whiteToMove? bSquareStorage : wSquareStorage).m_EnPassantSq.at(M.targetSqID));
		resultHash xor_eq zhashtable[target->m_SqID][target->m_PieceType];
	}
	else if (M.isCapture)
	{
		auto& target = ((whiteToMove? bSquareStorage : wSquareStorage).m_Occupied.at(M.targetSqID));
		M.proclivity -= target->centipawnValue; //-= because centipawn value for black pieces is negative
		resultHash xor_eq zhashtable[target->m_SqID][target->m_PieceType];
	}
	resultHash xor_eq zhashtable[M.targetSqID][Ptype]; // xor-ing IN the moving piece

#ifndef ENGINE_DONT_HASH_TURNS
	resultHash xor_eq zhashtable.turnhash(isWhiteTurn); // xoring OUT the current side-to-move flag
	resultHash xor_eq zhashtable.turnhash(!isWhiteTurn); // xoring IN the next side-to-move flag
	//xor the castle/en-passant hashes here
#endif

	return resultHash;
}

/* zhash_t HashMove(Move& M, const SwitchName Ptype, const bool whiteToMove, const zhash_t fromHash)
{
	assert((M.isLegal && (!M.isCapture)) && "Invalid Move in HashMove! (illegal or capture)");
	zhash_t resultHash = fromHash;
	resultHash xor_eq zhashtable[M.currentSqID][Ptype]; // xor-ing out the moving piece
	resultHash xor_eq zhashtable[M.targetSqID][Ptype]; // xor-ing IN the moving piece

#ifndef ENGINE_DONT_HASH_TURNS
	resultHash xor_eq zhashtable.turnhash(isWhiteTurn); // xoring OUT the current side-to-move flag
	resultHash xor_eq zhashtable.turnhash(!isWhiteTurn); // xoring IN the next side-to-move flag
	//xor the castle/en-passant hashes here
#endif

	return resultHash;
} */


namespace Fish{
int maxSearchDepth{6};
int targetDepth{3};
int currentDepth{0};
bool displayRelativeEval{true};

int movesMadeCount{0};
int finalPositionsCount{0};
int transpositionCount{0};
int initEval{0};

std::unordered_map<zhash_t, Transposition_Record> transpositionTable{}; //using the zobrist hashtable

bool isBetterEval(const int eval, const int currentBest, const bool isWhiteEval)
{ return ((isWhiteEval) ? (eval >= currentBest) : (eval <= currentBest)); } //should be more efficient to return false for equality. But for some reason this prevents checkmates from being set as bestmoves (because they overflow?)

bool isStrictlyBetterEval(const int eval, const int currentBest, const bool isWhiteEval)
{ return ((isWhiteEval) ? (eval > currentBest) : (eval < currentBest)); }

bool isGoodOutcome(const Transposition_Record& outcome, const bool isWhiteEval)
{
	using result = Transposition_Record::result;
	int materialcount = outcome.eval;

	if (!outcome.gameEnd)
	{
		// return ((isWhiteEval)? (materialcount > 0) : (materialcount <= 0));
		return ((isWhiteEval)? (materialcount > 0) : (materialcount < 0));
	}

	switch (*outcome.gameEnd)
	{
		case result::WhiteWin:
		{ return isWhiteEval; } break;
		case result::BlackWin:
		{ return !isWhiteEval; } break;
		case result::Draw:
		case result::Stalemate:
		{
			// return ((isWhiteEval)? (materialcount < 0) : (materialcount >= 0));
			return ((isWhiteEval)? (materialcount < 0) : (materialcount > 0));
		}
		default:
			break;
	}
	return false;
}

// For some reason distance-to-mate is depending on the order of the PieceArray. // right-to-left placement works, left-to-right does not evaluate the second move as checkmate
// I assume this has something to do with the "shouldTranspose" and "hasEval" flags?? //because the root-move checkmate hasn't been fully evaluated when it's transposed into by a deeper line
// that should naturally get fixed by iterative deepening
bool isBetterEval(const Transposition_Record& eval, const Transposition_Record& currentBest, const bool isWhiteEval)
{
	auto comp = (isGoodOutcome(eval, isWhiteEval) <=> isGoodOutcome(currentBest, isWhiteEval));
	if (comp != 0) //if one was good and the other was bad
	{
		return (comp > 0); //true if the eval was better than currentBest
	}
	// otherwise, if they're both good or both bad

	if (eval.gameEnd)
	{
		bool outcomeGood = isGoodOutcome(eval, isWhiteEval); //doesn't matter which one we evaluate here, because we know their outcome is equivalent (otherwise comp would have returned 0)

		//if there's a mismatch in their depths, pick the shortest or longest distance-to-mate
		if ((eval.depth_searchedTo - eval.depth_startFrom) != (currentBest.depth_searchedTo - currentBest.depth_startFrom))
		{
			if (outcomeGood)
			{
				//find shortest distance to checkmate if you're winning
				return ((eval.depth_searchedTo - eval.depth_startFrom) < (currentBest.depth_searchedTo - currentBest.depth_startFrom)); //we need to favor the newly-evaluated (therefore always local) move, otherwise it will prefer transpositions (to checkmate), causing the distance-to-mate to increase
			}
			else //if both bad
			{ //find longest distance to checkmate
				return ((eval.depth_searchedTo - eval.depth_startFrom) > (currentBest.depth_searchedTo - currentBest.depth_startFrom));
			}
		}
		// otherwise, if there's no difference in number of moves in each line, pick whichever one starts sooner (because that will actually have a shorter distance-to-mate)
		else
		{
			if (outcomeGood) return (eval.depth_startFrom < currentBest.depth_startFrom);
			else if (!outcomeGood) return (eval.depth_startFrom > currentBest.depth_startFrom);
		}
		// if the distance-to-mate is exactly the same, then the default comparison (material) will be returned
	}

	//otherwise, if not checkmate
	return isStrictlyBetterEval(eval.eval, currentBest.eval, isWhiteEval); //just compare material
}

//altUndoMove will take care of squaretable pointers and the stable sets in squarestorage (including enpassant and canCastle/With)
//but we want to avoid re-generating movetables for all the pieces, but it's hard to figure out what needs to be recalculated, so we store the volatile sets and the movetables here.
struct SavedVolatiles
{
	std::multimap<int, Piece*> Attacking;
	std::multimap<int, Piece*> Captures;
	std::map<int, Piece*> m_givingCheck;

	//WE HAVE TO CALL "FILLCASTLEDESTS" IF WE'RE SKIPPING ALTERNATETURN!!!?
	std::map<int, Piece*> CastleDestR; //These may be invalid because they aren't normally cleared/updated on opponent's turn?
	std::map<int, Piece*> CastleDestL;

	//add enPassant here!

	//instead of pointers, you should store indecies into the piecearray instead?
	//we'll need a map of int -> Pieceref in global(engine) scope.
	std::vector< std::pair< std::unique_ptr<Piece>&, std::vector<Move> > > savedMovetables;

	//SavedVolatiles() = default;

	SavedVolatiles(SquareStorage& theStorage, std::vector<std::unique_ptr<Piece>>& theArray)
		: Attacking{theStorage.Attacking}, Captures{theStorage.Captures}, m_givingCheck{theStorage.m_givingCheck}, CastleDestR{theStorage.CastleDestR}, CastleDestL{theStorage.CastleDestL}
	{
		savedMovetables.reserve(theArray.size());
		for (auto& P : theArray)
		{
			if (!P->isCaptured)
				savedMovetables.emplace_back(P, P->Movetable);

			//You can't actually exclude the illegal moves here; they include things like the pawn captures.
			/* if (P->isCaptured) continue;
			auto legalMoves = P->Movetable | std::views::filter([](Move& M)->bool { return M.isLegal; }) | std::views::reverse;
			savedMovetables.emplace_back(P, std::vector<Move>{legalMoves.begin(), legalMoves.end()}); */
		}
		return;
	}

};

void RestoreVolatiles(const SavedVolatiles& W, const SavedVolatiles& B)
{
	wSquareStorage.Attacking = W.Attacking;
	wSquareStorage.Captures = W.Captures;
	wSquareStorage.m_givingCheck = W.m_givingCheck;
	wSquareStorage.CastleDestR = W.CastleDestR;
	wSquareStorage.CastleDestL = W.CastleDestL;

//add enPassant here!

	for (auto& PM : W.savedMovetables)
	{
		PM.first->Movetable = PM.second;
	}

	bSquareStorage.Attacking = B.Attacking;
	bSquareStorage.Captures = B.Captures;
	bSquareStorage.m_givingCheck = B.m_givingCheck;
	bSquareStorage.CastleDestR = B.CastleDestR;
	bSquareStorage.CastleDestL = B.CastleDestL;

	for (auto& PM : B.savedMovetables)
	{
		PM.first->Movetable = PM.second;
	}

	return;
}



void PrintWithHeaders_All(SavedVolatiles& currentTeam, std::vector<Rootmove_t> storedAll)
{
	for (auto& H : pieceHeaders)
	{
		std::size_t I = H.first;
		assert((!currentTeam.savedMovetables.at(I).first->isCaptured) && "PrintWithHeadersAll included a captured piece somehow");

		auto matching = storedAll | std::views::filter([I](Rootmove_t& T)->bool { return T.accessNum == I;});
		if (matching.begin() == matching.end()) { continue; } //no legal moves

		std::cout << H.second << '\n'; //pieceheader
		for (auto M{matching.begin()}; M != matching.end(); ++M) { std::cout << M->GetStream(); }
		std::cout << '\n';
	}
	return;
}

// these should not be using savedvolatiles. Just iterate through PieceHeaders instead, it's key is the index into savedMovetables (which matches accessnum in rootmove)
void PrintWithHeaders(SavedVolatiles& currentTeam, auto moveRange)
{
	for (std::size_t I{0}; I < currentTeam.savedMovetables.size(); ++I)
	{
		auto matching = moveRange | std::views::filter([I](Rootmove_t& T)->bool { return T.accessNum == I;});

		if (matching.begin() == matching.end())
		{ continue; } //no legal moves

		if (currentTeam.savedMovetables.at(I).first->isCaptured)
		{ continue; }

		std::cout << pieceHeaders.at(I) << '\n'; //pieceheader

		for (auto M{matching.begin()}; M != matching.end(); ++M)
		{
			std::cout << M->GetStream();
		}

		std::cout << '\n';
	}
	return;
}

/* void PrintWithHeaders_Best(SavedVolatiles& currentTeam, std::vector<Rootmove_t>& storedAll)
{
	std::sort (storedAll.begin(), storedAll.end(), [](const auto& lh, const auto& rh)->bool {return lh.eval > rh.eval; });

	for (auto& M : storedAll)
	{

	}
} */


void Search_recursive(Transposition_Record& parentNode, int Min, const int Max); //forward declaration

void Search_root()
{
	assert (zhashtable.VerifyHashTable() && "zhashtable was not legit when Search_Root was called!");
	currentDepth = 0;
	movesMadeCount = 0;
	finalPositionsCount = 0;
	transpositionCount = 0;

	initEval = TallyMaterial();
	pieceHeaders.clear();
	//transpositionTable is cleared in the "EngineTest" function

	auto& FENmap{currentSetup.GetFENmap().left};

	SavedVolatiles init_wVolatiles {wSquareStorage, wPieceArray};
	SavedVolatiles init_bVolatiles {bSquareStorage, bPieceArray};
	SavedVolatiles& currentTeam{((isWhiteTurn) ? init_wVolatiles : init_bVolatiles)};
	PieceArray& friendArray((isWhiteTurn)? wPieceArray : bPieceArray);
	//const SquareStorage& enemyStorage{((isWhiteTurn)? bSquareStorage : wSquareStorage)}; //unused now?

	// store these to allow arbitrary search/print order; and allow printing/updating between searches.
	// I'm an idiot; these end up being duplicated, and desynced between here and transpositionTable!!!
	std::vector<Rootmove_t> init_Rootmoves;

#ifdef ENGINGE_DONT_HASH_TURNS
	const zhash_t rootHash{HashPieceArray(wPieceArray,bPieceArray)};
#else
	const zhash_t rootHash{HashPieceArray(wPieceArray,bPieceArray) xor zhashtable.turnhash(isWhiteTurn)};
#endif

	assert(transpositionTable.empty() && "transpositionTable wasn't cleared!!!");
	transpositionTable[rootHash].isRootMove = true; //prevent the root-position from ever being overwritten

#ifndef ENGINE_SUBSCRIPT_OVERLOAD
	transpositionTable[rootHash].hash = rootHash;
#endif

	//creating hashes / stored stringstreams for every piece's every move, to be searched/printed later
	//we want to create an entry for every rootmove before we start searching any move, to prevent useless searches (because rootmoves will always be the minimum depth, any node created by transposing into them from any other line would always get overwritten later)

	std::size_t offsetNum{0};
	for (auto& SMTpair : currentTeam.savedMovetables)
	{
		std::unique_ptr<Piece>& P = SMTpair.first;
		if (P->isCaptured)
		{
			++offsetNum;
			continue;
		} //there shouldn't be any captured pieces in the saved volatiles

		char FENchar{FENmap.at(P->m_PieceType)};
		pieceHeaders[offsetNum] = {((P->isWhite) ? "White " : "Black ") + std::string{P->GetName()} + " {" + std::string{P->m_AlgCoord} + "}"};

		auto legalMoves = SMTpair.second | std::views::filter([](Move& M)->bool { return M.isLegal; }) | std::views::reverse; //there isn't really any need to enforce an order here, just sort the rootmoves later
		// for (Move M : SMTpair.second)
		for (Move M : legalMoves)
		{
			//assert((M.isLegal) && "illegal move somehow copied into init_wVolatiles");
			if (!M.isLegal) continue;

			zhash_t H = HashMove(M, P->m_PieceType, P->isWhite, rootHash); //updates "proclivity" as well+
			auto& placed = init_Rootmoves.emplace_back(H, offsetNum, M, isWhiteTurn, FENchar);
			placed.eval = M.proclivity; //we're setting this initial eval because it's normally set by "HashMove", but that gets skipped for the rootmoves when we call "SearchMove". NEVER SET THE EVAL AGAIN!!!!! Copy from the init-Rootmoves at every search

			//the only entry before this in the table is the initial position, so it will get inserted.
			transpositionTable[H] = placed;
			//transpositionTable.emplace(placed.hash, Transposition_Record{placed}); //copies the base class of transposition_Record, which has "isRootMove" flag, and the initial eval (hasEval still equals false)

#ifndef ENGINE_SUBSCRIPT_OVERLOAD
			transpositionTable[H].hash = H;
#endif
		}
		++offsetNum;
	}

	//we WANT this to modify Rootmoves, so that we can save their evals and stuff
	auto SearchMove = [&](Rootmove_t& stored, int toDepth = targetDepth, int minscore = NumLimitMin(isWhiteTurn)) -> void
	{
		int oldDepth = targetDepth;
		targetDepth = toDepth;

		assert((currentDepth == 0) && "rootmove did not start at depth 0");
		friendArray[stored.accessNum]->MakeMove(stored.m_move);
		AlternateTurn();
		Search_recursive(stored, minscore, NumLimitMax(isWhiteTurn));// alpha (bestScore) is passed to beta (min). root-moves will always have an alpha (max) of infinity, because the opponent can't prevent it.

//THAT MODIFIED THE BASE OF ROOTMOVE, BUT NOT THE ENTRY IN TRANSPOSITIONTABLE!!!!

#ifdef ENGINE_USEALT_UNDOLASTMOVE
		turnhistory.AltUndoLastMove();
#else
		turnhistory.UndoLastMove();
#endif
		RestoreVolatiles(init_wVolatiles, init_bVolatiles);
		assert((currentDepth == 0) && "rootmove did not return to depth 0");

		targetDepth = oldDepth;
		return;
	};

	//initial rootmove search //to get the base evals, and see if there's a mate in one?
	/* for (Rootmove_t& T : init_Rootmoves)
	{
		SearchMove(T, 0);
		//T.rec.hasEval = true; //already set by SearchMove
	} */

#ifdef ENGINE_INITIAL_PRINT
	PrintWithHeaders_All(currentTeam, init_Rootmoves);
	auto captures = init_Rootmoves | std::views::filter([](const Rootmove_t& S)->bool {return S.m_move.isCapture;});
	std::cout << "Captures: \n";
	PrintWithHeaders(currentTeam, captures);
#endif

	std::vector<std::vector<Rootmove_t>> pastSearchStorage; //stores a copy of every rootmove that was searched at a given (target) depth, AFTER calling searchmove. Each index into the outer vector corresponds to a target-depth. This is mostly for debug/printing purposes?

	std::unordered_map<zhash_t, Rootmove_t> current_Rootmoves; //keeps an up-to-date copy of every single rootmove. This is important for being able to print stuff, and for move ordering. Also, if a search is interrupted partway through, this will preserve the last completed search's results.

	//iterative deepening
	pastSearchStorage.reserve(targetDepth+1);
	int finalTarget = targetDepth;
	targetDepth = 0;

RestartSearch:
	EngineState.Tick({Tickpoint::iterativeDeepeningLoop, Tickpoint::before});

	while (targetDepth <= finalTarget)
	{
		std::cout << "searching to depth: " << targetDepth << '\n';

		int bestScore = NumLimitMin(isWhiteTurn); //this will have to be reset every depth/iteration
		zhash_t bestHash{0};
		// pastSearchStorage.emplace_back(std::vector<Rootmove_t>{});
		pastSearchStorage.emplace_back();

		//The transposition_records from last depth's search are still in the transpositionTable!!! They retain their old values until we overwrite them here.


		//EngineState.Tick({Tickpoint::moveSearch, Tickpoint::before, 0});
		EngineState.Tick({Tickpoint::iterativeDeepeningLoop, Tickpoint::inloop});

		//we're making fresh copies here
		for (Rootmove_t T : init_Rootmoves)
		{
			EngineState.Tick({Tickpoint::moveSearch, Tickpoint::inloop, 0});

			SearchMove(T, targetDepth, bestScore);
			assert(T.hasEval && "Rootmove didn't return with a setEval after SearchMove!");
			//T.hasEval = true; //should have been set by SearchMove, but whatever.
			//T.depth_searchedTo = targetDepth;

			transpositionTable[T.hash] = T; //updating this is required; sets hasEval, eval, gameEnd, depth, etc.
			//current_Rootmoves.insert_or_assign(T.hash, T); //literally just cannot do this
			current_Rootmoves.erase(T.hash);
			current_Rootmoves.emplace(std::make_pair(T.hash, T));

			if (((bestHash != 0) && (isBetterEval(T, current_Rootmoves.at(bestHash), isWhiteTurn))) || (bestHash == 0))
			{//if besthash exists, then it should be in current_Rootmoves, because it must have been set by a move searched earlier this turn.
				bestHash = T.hash;
				bestScore = T.eval;
			}

			pastSearchStorage.at(targetDepth).push_back(T);
		}
		EngineState.Tick({Tickpoint::moveSearch, Tickpoint::after, 0});

		++targetDepth;
		/* std::sort(init_Rootmoves.begin(), init_Rootmoves.end(), [](const Rootmove_t& lh, const Rootmove_t& rh)->bool {return isBetterEval(lh.rec, rh.rec, isWhiteTurn); }); */
		/* std::sort(std::begin(init_Rootmoves), std::end(init_Rootmoves), [](const auto& lh, const auto& rh)->bool {return lh.rec.eval > rh.rec.eval; }); */
		/* std::sort(init_Rootmoves.begin(), init_Rootmoves.end(), [](const auto& lh, const auto& rh)->bool {return lh.rec.eval > rh.rec.eval; }); */
		//PrintWithHeaders_All(currentTeam, stored_Rootmoves);
	}

	EngineState.Tick(Tickpoint::searchCompleted);

	if (EngineState.GetCommand() == EngineState::Command::restart)
	{
		//targetDepth += 1; //leave unchanged
		finalTarget += 1;
		goto RestartSearch;
	}
	if (EngineState.GetCommand() == EngineState::Command::reset)
	{
		targetDepth = 0;
		pastSearchStorage.clear();
		current_Rootmoves.clear();
		transpositionTable.clear();
		goto RestartSearch;
	}
	if (EngineState.GetCommand() == EngineState::Command::exit)
	{ return; }
	//if ignore, you just continue as usual and print the moves

	std::cout << "search completed!\n";
	targetDepth = finalTarget; //because targetDepth just incremented past finalDepth, if we leave it, the next search will use the higher depth.

	std::vector<Rootmove_t> final_Rootmoves{};
	std::ranges::for_each(current_Rootmoves.begin(), current_Rootmoves.end(), [&](auto E)->void { final_Rootmoves.push_back(E.second); });

	PrintWithHeaders_All(currentTeam, final_Rootmoves);

	/* std::cout << "\nbestmoves: \n";
	//auto testRange = std::ranges::ref_view(final_Rootmoves);
	auto topmoves = testRange | std::views::take(5); //not sure if this works properly when there's less than five legal moves
	PrintWithHeaders(currentTeam, topmoves); */

	return;
}

//parentNode isn't a very accurate name, it's more like currentNode or searchingNode
void Search_recursive(Transposition_Record& parentNode, int Min, const int Max)
{
	int BaseDepth = currentDepth;
	Transposition_Record* bestMove{nullptr};

	//for some reason you can't end it here, or it still complains about bestmove being nullptr. I'm guessing it requires some function to set the move proclivity or something?? //or the eval of parentNode?
	/* if (wSquareStorage.m_Occupied.empty())
	{
		parentNode.gameEnd = Transposition_Record::result::BlackWin;
		parentNode.hasEval = true;
		return parentNode;
	}
	else if (bSquareStorage.m_Occupied.empty())
	{
		parentNode.gameEnd = Transposition_Record::result::WhiteWin;
		parentNode.hasEval = true;
		return parentNode;
	} */

	++currentDepth;

	assert((maxSearchDepth >= targetDepth) && "max search-depth is below BaseDepth!!!");
	assert((currentDepth <= maxSearchDepth) && "Engine's current search depth is beyond maxdepth!!!");

	SavedVolatiles wVolatiles{wSquareStorage, wPieceArray};
	SavedVolatiles bVolatiles{bSquareStorage, bPieceArray};
	SavedVolatiles& currentTeam{((isWhiteTurn) ? wVolatiles : bVolatiles)};

	//The idea was that we could avoid copying movetables un-necessarily if we just use the moves stored in the volatiles. After all, we've already copied them into the volatiles' "saved_movetables" variable. You can't iterate through the Piece's current movetable without making a copy because the iterators will get invalidated after MakeMove/AlternateTurn/UndoMove.
	//The movetables in the local volatiles should have their "proclivity" values updated based on the returned eval of their recursive search, and when you eventually store the volatiles per-node, you can use that for move-ordering. //But we use them to restore the pieces to their original state???? So we should never change them.
	//The moves stored in Rootmoves need to be copies, however.
	for (auto& SMTpair : currentTeam.savedMovetables)
	{
		auto& P = SMTpair.first; //std::unique_ptr<Piece>&

		if (P->isCaptured)
			continue;

		EngineState.Tick(Tickpoint::pieceLoop);

		auto legalMoves = SMTpair.second | std::views::filter([](Move& M) { return M.isLegal; }) | std::views::reverse;
		//std::vector<Move> copiedMovetable{legalMoves.begin(), legalMoves.end()};

		for (Move& M : legalMoves) //should this be a copy???????
		{
			if (!M.isLegal) continue;

			EngineState.Tick(Tickpoint::moveSearch);
			//M.proclivity = 0; //we need to reset it every time to prevent Hashmove from continuously growing the eval.

#ifdef ENGINE_SUBSCRIPT_OVERLOAD
			Transposition_Record& movehash = transpositionTable[HashMove(M, P->m_PieceType, P->isWhite, parentNode.hash)];
			//REMINDER THAT TRANSPOSITION TABLE IS NOT SYNCED WITH ROOTMOVES!!!!
			//You may need to handle root-moves specially; they'll be handled correctly UNLESS their "hasEval" is still false?? (I think they'll still be handled correctly?? But they might get overwritten/given evals when they really shouldn't) Which shouldn't be the case with iterative deepening (because it's impossible to transpose into them until depth 4)
#endif
#ifndef ENGINE_SUBSCRIPT_OVERLOAD
			zhash_t HHH = HashMove(M, P->m_PieceType, P->isWhite, parentNode.hash);
			Transposition_Record& movehash = transpositionTable[HHH];
			movehash.hash = HHH;
#endif
			assert((movehash.hash != 0) && "movehash created emtpy!\n");
			assert((movehash.hash != parentNode.hash) && "move did not change position hash!!\n");

			if (M.isCapture && (((P->isWhite)? bSquareStorage : wSquareStorage).m_Occupied.size() == 1))
			{ //capturing the final piece of the opponent is an instant win
				movehash.hasEval = true;
				movehash.gameEnd = ((P->isWhite)? Transposition_Record::result::WhiteWin : Transposition_Record::result::BlackWin);
				//goto EndMoveSearch; //technically we should check to see if movehash is a transposition into a rootmove
			}

			Transpose_Flags moveflags{movehash, M, P->isWhite}; //this sets the initial values of "startFrom / searchedTo" variables

			/* if ((movehash.isRootMove) && (!movehash.hasEval))
			{ //if it does have an eval, it can be handled like any other transposition
				++transpositionCount;
				continue;
			}
			else if (!movehash.isRootMove) //actually I'm not sure it's necessary to exclude this? it should evaluate to false for the next two statements???
			{ */
				if ((currentDepth >= targetDepth) || (moveflags.shouldTranspose && moveflags.shouldUseEval) || movehash.gameEnd)
				{
					++finalPositionsCount;

					if (!(moveflags.shouldTranspose && moveflags.shouldUseEval))
					{
						movehash.eval = M.proclivity; //set by HashMove(), it's the relative change in eval (from this move to end of line, if we're at max depth it's simply equal to the value of a capture, or 0 if not a capture)
					}
					else if (moveflags.shouldTranspose) { ++transpositionCount; } //if we're transposing into a position with a known eval instead, then movehash already has it's eval set.

					if(!moveflags.shouldTranspose)
					movehash.hasEval = true;
				}
				else if (moveflags.shouldTranspose) //implying shouldn't use eval
				{
					++transpositionCount;
					continue; // do not search further (the line will be continued elsewhere), do not return/update an eval (because we don't have one for this move)
				}

				//search all the way to max depth
				if ((currentDepth < targetDepth) && (moveflags.shouldWrite) && (!movehash.hasEval))
				{
					++movesMadeCount;
					P->MakeMove(M);
					AlternateTurn();
					Search_recursive(movehash, Max, Min); //We're feeding Max->Min and Min->Max because
					// that recursive call updates the score of movehash, since each call sets the value of parentNode to bestScoreLocal // also sets "hasEval" to true //it also updates the searchedTo_depth
					#ifdef ENGINE_USEALT_UNDOLASTMOVE
						turnhistory.AltUndoLastMove();
					#else
						turnhistory.UndoLastMove();
					#endif
					RestoreVolatiles(wVolatiles, bVolatiles);
				}
			//}

			//MOVEHASH MIGHT BE A ROOTMOVE HERE?!?!?! (If we transposed into it)
			if (movehash.hasEval)
			{
				if ((bestMove == nullptr) || isBetterEval(movehash, *bestMove, P->isWhite))
				{
					bestMove = &movehash;

					if (isStrictlyBetterEval(bestMove->eval, Max, isWhiteTurn))
					{
						goto EndMoveSearch;
						//parentNode's eval gets set at the end of this function
					}
					else
					{
						Min = bestMove->eval; // bestLocalScore, gets fed to all subsequent Search_recursive calls
						// Max never gets updated; it's the bestLocalScore (Min) of the OPPONENT'S (previous) turn, and therefore doesn't change on your turn.
					}
				}

				/* if (movehash.depth_searchedTo > parentNode.depth_searchedTo)
				{ parentNode.depth_searchedTo = movehash.depth_searchedTo; } */
				// DON'T DO THIS HERE!!! if a line ends in checkmate, the value of it's "searchedTo" depth is significant, but it would be overwritten here.

				//if bestScore is above max, instantly end the search
				//DON'T end search if it's checkmate, those get min/maxed like everything else
			}
		}
	}
EndMoveSearch:
	EngineState.Tick(Tickpoint::searchCompleted);

	--currentDepth;
	assert((currentDepth == BaseDepth) && "Search_recursive did not return to basedepth!!!");

	if (bestMove == nullptr)
	{
		std::cerr << "bestMove was nullptr! (searchmove recursive), depth: " << currentDepth << '\n';
		// parentNode.hasEval = false;
		// parentNode.hasEval = true; //we shouldn't set it to true, but we CAN'T set it to false. Because then any node that transposes into a repetition / unsearched root node on every move, will not have an eval, even if it had other legitimate results. // That's wrong though, because each call to this function searches every possible response; there can't be any other
		return;
	}
	assert((bestMove != nullptr) && "bestMove was nullptr! (searchmove recursive)");

	assert((bestMove->hasEval) && "bestMove did not have an eval!!!\n");

	//MOVEHASH MIGHT BE A ROOTMOVE HERE?!?!?! (If we transposed into it)
	if (bestMove->gameEnd)
	{
		parentNode.gameEnd = bestMove->gameEnd; //this flag is more to indicate that the node should never be overwritten / and it's score should be interpreted as #mate-in-X or a draw, rather than a relative material count.
		//parentNode.eval = bestMove->eval;
		parentNode.eval += bestMove->eval;
	}

	//I think this is bullshit if it's a rootmove????

	parentNode.eval += bestMove->eval; //additive, because if multiple pieces were captured in the line, the overall (relative) eval would be the total value of the captured pieces. It's okay that the objective eval (pieces captured earlier than parentNode's depth) isn't passed the line, because the eval stored in transposition table represents the relative material change from that depth onwards; any use of it should always give the same result (because any transpositions into that line will always start with the same material balance)
	parentNode.hasEval = true;

	//if (bestMove->gameEnd)
		// Note that the bestMove->depth_searchedTo might not be set yet, especially if it's a transposition/root move
		// I'm pretty sure this is just wrong
	{ parentNode.depth_searchedTo = parentNode.depth_startFrom + (bestMove->depth_searchedTo - bestMove->depth_startFrom) + 1; } //so that the "Mate in X" count gets set correctly

	// Note that the bestMove->depth_searchedTo might not be set yet, especially if it's a transposition/root move
/* 	if (bestMove->depth_searchedTo <= bestMove->depth_startFrom ) //basically, if we're transposing into a line of unknown length
	{ parentNode.depth_searchedTo = parentNode.depth_startFrom + targetDepth; }
	else
	{
		parentNode.depth_searchedTo = parentNode.depth_startFrom + (bestMove->depth_searchedTo - bestMove->depth_startFrom);
	} */

	return; //propagate bestScore backwards
}


void EngineTest()
{
	if (Fish::isRunning)
	{
		std::cerr << "fish is already running!";
		return;
	}

	Fish::isRunning = true;

	transpositionTable.clear();
	transpositionTable.reserve(10000000);

	maxSearchDepth = targetDepth + 1;
	std::cout << "\nEngine is searching... (target/max Depths: " << targetDepth << '/' << maxSearchDepth << " ply)\n";
	CodeTimer engineTimer{};

	Search_root();

	std::cout << "\nTotal Time elapsed: " << engineTimer.elapsed() << " seconds\n";

	std::cout << movesMadeCount << " Moves made!\n";
	std::cout << finalPositionsCount << " Unique lines!\n";
	std::cout << transpositionTable.size() << " Stored Hashes!\n";
	std::cout << transpositionCount << " Transpositions!\n";
	transpositionTable.clear();

	Fish::isRunning = false;

	return;
}

} //end of Fish-namespace
