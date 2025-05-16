#ifndef BOBBY_FISH_HPP_INCLUDED
#define BOBBY_FISH_HPP_INCLUDED

//you may be able to move some of these to .cpp file
#include "SquareStorage.hpp"
#include "TurnHistory.hpp"
#include "Pieces.hpp" //already included by SquareStorage
#include "Moves.hpp" //already included by SquareStorage
#include "Board.hpp" //already included by SquareStorage
#include "GameEvent.hpp"

#include "FPSmath.hpp" //for CodeTimer

#include <unordered_map>
#include <vector>
#include <queue>

constexpr int MAX_MOVES = 256;
constexpr int MAX_PLY = 246;

namespace BobbyFishEngine{

	enum Value : int
	{
		VALUE_ZERO = 0,
		VALUE_DRAW = 0,
		VALUE_KNOWN_WIN = 10000,
		VALUE_MATE = 32000,
		VALUE_INFINITE = 32001,
		VALUE_NONE = 32002,

		//what?
		VALUE_MATE_IN_MAX_PLY = VALUE_MATE - 2 * MAX_PLY,
		VALUE_MATED_IN_MAX_PLY = -VALUE_MATE + 2 * MAX_PLY,

		// Mg/Eg are middlegame and endgame values
		// Stockfish stores both values in a single "Score"-enum, with the least-significant 16 bits storing the middlegame value, and the upper 16 bits go to the endgame value. Then it has to do some wizardry to perform operations on/with them.
		PawnValueMg = 128, PawnValueEg = 213,
		KnightValueMg = 781, KnightValueEg = 854,
		BishopValueMg = 825, BishopValueEg = 915,
		RookValueMg = 1276, RookValueEg = 1380,
		QueenValueMg = 2538, QueenValueEg = 2682,

		MidgameLimit = 15258, EndgameLimit = 3915 //cutoffs where the line is assumed to be nonsense?
	};

	using Turn = std::vector<GameEvent>; //std::queue instead?

	struct SavedGameState
	{
		// Repetition counter (record of positions)
		// 50/100-move rule

		// Side-to-move
		// Position-Hash (without side-to-move)

		// std::vector<std::queue<Piece*>> pieceStates; //useless unless we can somehow reuse the top pieceStates across lines
		std::vector<Piece*> pieceStates;
		std::vector<Move*> allMoves;
		//auto legalMoves = P->Movetable | std::views::filter([&](Move& M) { return M.isLegal; });
		//auto captures = legalMoves | std::views::filter([&](Move M) { return M.isCapture; });
	};

	/*
	Node types:
		PV-nodes: score is within bounds of alpha and beta. These nodes must have all moves searched, and value returned is exact, which propagates up to the root, along with a principal-variation.

		Cut-nodes (fail-high): beta-cutoff was performed. A minimum of one move needs to be searched at these nodes. The score returned is a lower bound (might be greater) on the exact score of the node. The child of a cut-node is always an "All-node", and the parent is either a PV- or All-node. The ply-distance of a cut-node to it's PV-ancestor is always odd?

		All-nodes (fail-low): no move at this node had a score exceeding alpha. Every move is searched, and the score returned is an upper bound (the exact score might be less). It's children are all cut-nodes, and the parent is a cut-node. The ply-distance of an all-node to it's PV-ancestor is even.
	*/
	struct Node
	{
		int alpha; //lower bound: minimum score a node must reach to change the value of the previous node; represents the minimum score that the maximizing player is assured
		int beta; //upper bound: if a node's value exceeds this, it means that node will be avoided by the opponent, because his guaranteed score (alpha of the parent node) is already greater. Thus, Beta represents the best-score the minimizing player could achieve so far. (in a Negamax framework from the max-player's POV; where beta is just negative alpha of parent node).

		u_int m_depth; //what happens when we transpose into this node at a much later point in some line? Or if we find that another line can enter this position at a lower depth?
		int m_lazyEval; //simple eval based on material balance, in centipawns. This is just for move-ordering purposes.
		int* m_eval; //complex eval, updated/points to the final eval of the depth search. centipawns

		std::string hash;
		SavedGameState m_state; //we need a full savestate here because otherwise we have to regenerate every movetable every time? Also, we need the movetables to stay valid forever?
		std::unordered_map<Move*, Turn> searchedMoves; //necessary because we might cut off the node before searching everything, then return to it later? //Move is already stored in GameEvent/Turn, so why do we need this additional map for it?

		//sort by depth of parent node?
		std::unordered_map<Node*, Turn*> m_parents; //We may need multiple parent pointers for transpositions
		std::unordered_map<Turn*, Node*> m_children;

		std::string GetHashAfterTurn(Turn*);
		//std::unordered_map<Move, Node*> resultingPositions; //for some reason this ruins the default constructor
		void SortMoves();

		void CreateChildren()
		{
			//for all moves in m_state (sorted)
				//create or copy the current boardstate into the children
				//makeMove, (BUT DON'T RE-GENERATE MOVETABLES) creating a Turn-object from each move, create an entry in m_children (Node is nullptr) //actually you don't have to makeMove to create a Turn-object (except to create the positionHash)
				//if that Turn would transpose into an already-existing Node, link the child-ptr to the node stored in transposition_table, add this node to that child's parent-list, then
			return;
		}

		void UndoToParent(Node*) //we don't need to store the turn if we're storing the state???????
		{
			//finds the associated turn in m_parent
			//undo the turn
			return;
		}
	};

	typedef std::vector<Node*> Variation;

	/// RootMove struct is used for moves at the root of the tree. For each root move
/// we store a score and a PV (really a refutation in the case of moves which
/// fail low). Score is normally set at -VALUE_INFINITE for all non-pv moves.
	struct RootMove : public Node
	{
		bool operator<(const RootMove& m) const
		{ // Sort in descending order
			return m.score != score ? m.score < score
				: m.previousScore < previousScore;
		}

		Value score{-VALUE_INFINITE};
		Value previousScore{-VALUE_INFINITE};
		int bestMoveCount = 0;
		std::vector<Move> pv; //principal variation

		std::vector<Node*> m_Lines;
		Variation m_BestLine;
	};



	//If you get a position from earlier in the line (local transposition), it's a repetition.
	//All repetitions can be evaluated as draws, because the only outcomes are either threefold-draw, or one side deviates and it transposes into the line that would have occured if you just hadn't repeated. Will the eval be handled properly if the opponent has a better outcome than a draw, but you don't (in a lost position)?

	//In an alpha-beta search, you will need to exhaustively search a single line down to the max depth before you can actually start pruning any variations, because otherwise you don't have the correct bounds. Move-ordering is crucial, since evaluating the best move first will allow for the most pruning. That's why iterative deepening is a good idea.
	class SearchTree
	{
		//split SearchTree into threads, each needs a copy of rootState/moves?
	private:
		SavedGameState m_rootState;
		std::vector<RootMove> m_rootMoves; //each of these will be it's own thread or something?

	public:
		std::unordered_map<std::string, Node*> transpositionTable; //hashTable has to be per-thread?



		int SearchNode(Node&); //return eval?????


	};

	/*
	Also, allow for null-moves, to do "null-move pruning".

	Late-move Reductions reduce the search depth of moves that are likely to fail-low (best outcome is worse than current lower-bound). Typically, the first few moves(3-4) are searched at full depth, and if none fail-high, many of the remaining moves are reduced. Do not reduce the depth of:
		Tactical moves (captures and promotions)
		Moves while in check
		Moves which give check
	*/


	class BobbyFish
	{
		SavedGameState m_rootState;
		std::vector<RootMove> m_rootMoves; //sorted by how good they are
		SearchTree m_searchTree;
		bool m_isSeaching{false};
		bool m_searchHalted{false}; //cancelled by timelimit or user (in the case of pondering, a move by the opponent)

	public:
		int m_targetDepthPly{3};
		double m_timeLimit{0}; //doesn't limit search time if <= 0

		void FillRootMoves();
		void SortRootMoves();
		void StartSearch(); //needs to copy state/moves/pieces, clear searchtree, etc

		enum PrintFormat
		{
			BestLine,
			PerPiece,
			ForPiece,
			ForMove,
		};

		void PrintEvals(PrintFormat);
	};
}; //end namespace fish

#endif
