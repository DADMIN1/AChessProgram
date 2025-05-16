#ifndef CHESSPIECE_MOVES_HPP_INCLUDED
#define CHESSPIECE_MOVES_HPP_INCLUDED

#include <vector>	 // return type of determineTransforms()
#include <tuple> //just use pair instead? used by moveOptions class
#include <string>
#include <string_view>

enum moveType
{
	infOrthogonal, // rook move
	infDiagonal,   // bishop move
	limOrthogonal, // king's orthogonals are limited to 1
	limDiagonal,   // like vizier
	knightMove,	   // +2,+1 , etc.

	wPawnMove,
	bPawnMove,
	wHopliteMove,
	bHopliteMove,

//castlingMoves//
	castleMoveLeft,
	castleMoveRight,
//castleWithMoves//
	scootRightCastle,
	scootLeftCastle, //For adjusting rooks/kings 1sq after castling
	rookFlipRight,
	rookFlipLeft, //getting the rook to flip to the other side

	scootLeft, //pacifist 1sq left/right. NOT for castling
	scootRight,
	wLancemove,
	bLancemove, //does not include the scootLeft/scootRight

	hawkMove,
	lieutenantMove,
	captainMove,

	fakeMove, //placeholder
};

enum moveOpt
{
	dir_UP,
	dir_DOWN,
	dir_LEFT,
	dir_RIGHT,
	pacifist,
	jump,
	forceCapture,
	territoryOnly,
};

//We don't need a custom-offset option, instead of using startSq, feed the targetSqID of calcTransform() to determineTransforms()
struct moveOptions
{
	//bool dist_INF{true}; //if this is false, then the numbers in dist_LIM will be used
	std::tuple<int,int> dist_LIM{1, 1};

	bool opt_Pacifist{false};
	bool opt_Jump{false};

	//only UPwards part of the move
	bool dir_UP{false};
	bool dir_DOWN{false};
	bool dir_LEFT{false};
	bool dir_RIGHT{false};

	//	bool colorW{true}; //required for the following two
	bool opt_onlyCapture{false}; //move can ONLY be made if it's a capture, like pawn diagonals
	//	bool opt_InTerritory{false}; //this move can only be made inside the player's territory
};

class Move
{
	public:
	moveType m_moveType;
	int currentSqID;
	int targetSqID;
	std::string_view currentAlgCoord; //These are here for ease of printing out the moves.
	std::string_view targetAlgCoord;

	bool isJump{ false };
	bool isPacifist{ false }; //The move isn't allowed to capture (forward pawn moves)
	bool isForcedCapture{ false }; //Must capture to be legal
	bool isTypeCastle{ false }; //These moves are only intended to be played when castling
	bool isLegal{ true }; //lol

	bool isDestOccupied{ false };
	bool isCapture{ false }; //ONLY TRUE IF it's destination is occupied by a piece of the CORRECT color.
	bool isCapturingRoyal{ false };
	bool isEnpassant{ false };

	bool willPromote{ false }; //set by Ruleset::getTriggered

	int proclivity{0}; //Estimating importance, for engine search-ordering.

//Turn this into a constructor rather than applying it to tempMove every damn time.
	Move& calcTransform(int dX,int dY); // Calculates a transformation of X columns and Y rows to starting SqID, returns itself.

	Move(moveType T, int SqID); //current squareID //I think this is only used by tempMove.
	Move() = default; //apparently unnecessary?

	/* default copy/move constructors and assignment-operators should be generated
	as long as you don't define any of them, or a destructor. */
};

std::string GetNotation(const Move& M);

extern std::vector<Move> tempMovetable; //you should get rid of this

int calcTransform(int locationID, int dX, int dY); //just calculates a transformation of SquareIDs

std::vector<Move> determineTransforms(moveType thisType, int startSq); // get the thing to pass a piece as a parameter
std::vector<Move> determineTransforms(moveType thisType, int startSq, int limit_MIN, int limit_MAX); // overload for pieces that have limit to their range
std::vector<Move> determineTransforms(moveType thisType, int startSq, moveOptions Opts); //overload for making very specific, asymmetric movesets

#endif
