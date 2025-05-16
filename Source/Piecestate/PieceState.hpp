#ifndef PIECE_STATE_HPP_INCLUDED
#define PIECE_STATE_HPP_INCLUDED


struct RecordedPieceState  // basically a minimal version of a Piece; convertable to/from a Piece
{ //needs to be independent of side-to-move
	struct StateHash {} m_hash;

	struct DiffHash : public virtual StateHash // the result of comparing two PieceStates/hashes. You can convert a piece-state into/from another (or rather, calculate the resulting PieceStateHash) with the DiffHash.
	{

	};
	/* Ideally, it would be possible to construct a DiffHash from a Move (and the current PieceState) alone.
	The idea is that PieceStateStorage/GameHistory/Engine doesn't have to necessarily store the full PieceStateHash, and also they can simply look up a resulting piecestate rather than recalculating the state (re-doing the same move) over and over.
		If you can create a DiffHash from a move, then you can calculate the resulting hash and see if that move would put the piece into a known state (PieceStateHash is already recorded/mapped in storage).

	This would be very beneficial to the Engine, for detecting transpositions/repetitions, and understanding alternative move-orders, and avoiding unnecessary recalculation of piece states/moves.
		For example, it would be able to understand that moving a rook twice (on the same orthogonal) results in the same state as simply moving it there once. Thus, it can prioritize calculating moves that create "unqiue" states.

	A huge benefit is being able to understand the differences and similarities between similar positions (make a diffhash-matrix for the whole board/every piece), and being able to compare arbitrary components of the position (only observing the difference for a single piece, or for all pieces of one color, or ignoring/flipping side-to-move)

	This doesn't replace the design for the "PieceState" stacks in the Engine (but it synergizes very well with it); The DiffHash (created from a Move) won't predict the resulting state of any other piece. However, you can look at the resulting states of every piece after the move and record the differences with their previous state. This allows you to easily associate the resulting boardstate-changes with that move, which was the planned implementation of the stack-system.
	Having persistent state-records also solves the problem of unnecessary duplication/copying of piece-states, and also minimizes redundant movetable/state re-generation. (Instead of dealing with the messy logic that requires consideration of the whole game-state to determine when you can/can't use the piece-stacks, you can always use a stored state retrieved by hash)
		//note that the association of moves->resulting piece-stacks/boardstate is still per-boardstate/per-stack; it can't be associated with the retrieved piece-state directly because the rest of the boardstate is probably incompatible? //Actually, I thought I was going to associate the outcome of moves (who requires updating/recalculation) to a specific state/entry of each other piece; which should theoretically work correctly here???
	*/

	StateHash GetDiffHash(StateHash); //template this to compare two hashes/two RecordedPieceStates/one of each

};

struct PieceStateStorage // maps RecordedPieceStates to their hashes; maps history of piecestates to the actual objects in the piece-arrays, so you can see how each piece has changed individually. records hash-tables of each entire piecearray, to allow for easy color-swapping/transposition/group-movement, etc.
{

};

#endif
