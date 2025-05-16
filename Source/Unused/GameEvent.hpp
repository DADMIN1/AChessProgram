#ifndef GAME_EVENT_HPP_INCLUDED
#define GAME_EVENT_HPP_INCLUDED

//Record of a single, simple change to the boardstate that is easily applied/reversed
//Theoretically, you should be able to detect transpositions by seeing if all the same (persistent) BoardEvents occur, even if they're in a different order. Also, you'll be able to see if a piece is doing a dumb repetition, regardless of when it happened?
// Should make printing moves/lines much easier


struct GameEvent //union?
{
	enum GameEvent_Type
	{
		PIECE_MOVED,
		PIECE_CAPTURED,
		PIECE_CASTLED,
		PIECE_CASTLEDWITH,

		PIECE_PROMOTED,
		PIECE_DROPPED,
		PIECE_TOHAND,

		//non-persistent?
		ENPASSANT_CREATED,
		ENPASSANT_REMOVED,
		CASTLEDEST_CREATED,
		CASTLEDEST_REMOVED,
		CHECK_CREATED,
		CHECK_REMOVED,
		LOST_CASTLERIGHTS,

		GAMEOVER_CHECKMATE,
		GAMEOVER_NOPIECES, //One side had all their pieces captured
		GAMEOVER_STALEMATE,
		GAMEOVER_THREEFOLD_REPETITION,
		GAMEOVER_MOVELIMIT, //50-moves without a pawn-move or capture

		NULLMOVE,
	};

	GameEvent_Type m_EventType;

	//optional (union), dependant on Type of object
	//Move* m_moveMade;
	//Piece* m_moved;

};

#endif
