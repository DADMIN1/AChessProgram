#ifndef ZOBRIST_HASHTABLE_HPP_INCLUDED
#define ZOBRIST_HASHTABLE_HPP_INCLUDED

#include "Pieces.hpp" //switchnames and Piecearray (for hashPieceArray function)

#include <effolkronium/random.hpp>

#include <limits> //std::numeric_limits
#include <unordered_set>
#include <array>

//#define DEBUG_ENGINE_CHECKTABLE_COUT


//using zhash_t = u_int32_t;
using zhash_t = u_int64_t;
/* hash-collisions are expected to occur after evaluating sqrt(hash_size) # positions.
For 32-bit ints, this is ~65k (2^16); for 64-bit ints, it's ~4 billion (2^32).*/

class Zobrist_Hashtable {

	std::unordered_set<zhash_t> taken{0}; //ensure 0 is never used as a hash

	zhash_t getRandom()
	{
		assert(taken.size() < std::numeric_limits<zhash_t>::max()); //ensure that the map is not somehow full (all possible values have already been used)
		zhash_t R;
		do
		{
			R = effolkronium::random_static::get<zhash_t>();
		} while (taken.contains(R));
#ifdef DEBUG_ENGINE_CHECKTABLE_COUT
		std::cout << "\n getRandom Hash was equal to 0!!!!!\n";
#endif
		assert(((R != 0) && (!taken.contains(R))) && "getRandom hash was invalid!");
		taken.insert(R);
		return R;
	}

	// random number for every piece-type for each square-ID (26x26)
	std::array< std::array<zhash_t, (SwitchName::fairyPieceEnd+1) >, 676> m_hashtable;
	// fairyPieceEnd+1 to include that enum as well.

	zhash_t m_wTurn{getRandom()};
	zhash_t m_bTurn{getRandom()}; //these can be xor'd with the position hash to differentiate between the same position with a different side-to-move. Otherwise it's unspecified.
	// you'll need something similar for en-passant and castling rights
public:
	zhash_t turnhash(bool getW) const {return ((getW)? m_wTurn : m_bTurn);}
	const auto& operator[](const int& I) const { return m_hashtable[I]; };

	bool VerifyTurnHashes()
	{
		bool wasLegit{true};

		while ((m_wTurn == 0) || (m_bTurn == 0) || (m_wTurn == m_bTurn))
		{
			wasLegit = false;
			m_wTurn = getRandom();
			m_bTurn = getRandom();
#ifdef DEBUG_ENGINE_CHECKTABLE_COUT
			std::cout << "Side-to-move hashes were illegitimate!!\n";
#endif
		}
		return wasLegit;
	}

	bool VerifyHashTable()
	{
		bool wasTableLegit{true};

#ifdef DEBUG_ENGINE_CHECKTABLE_COUT
		std::cout << "Checking that Hash-table is legitimate!\n";
		int sqID_count{0};
		std::cout << "Checking SqID: \n";
#endif
		for (auto& SQ : m_hashtable)
		{
#ifdef DEBUG_ENGINE_CHECKTABLE_COUT
			std::cout << sqID_count << ", ";
#endif
			for (auto& A : SQ)
			{
				if (A == 0)
				{
					wasTableLegit = false;
					A = getRandom();
#ifdef DEBUG_ENGINE_CHECKTABLE_COUT
					std::cout << "\n Hash was equal to 0!!!!!\n";
#endif
				}
			}
#ifdef DEBUG_ENGINE_CHECKTABLE_COUT
			++sqID_count;
#endif
		}
#ifdef DEBUG_ENGINE_CHECKTABLE_COUT
		std::cout << '\n';
#endif
		return wasTableLegit;
	}

	Zobrist_Hashtable()
	{
		assert(VerifyTurnHashes() && "Turn hashes (Zobrist_Hashtable) were not valid!");

		taken.clear();
		taken.insert({0, m_wTurn, m_bTurn}); //ensure these aren't used

		for (int SQ{0}; SQ < 676; ++SQ)
		{
			for (SwitchName S{static_cast<SwitchName>(0)}; S <= fairyPieceEnd; ++S)
			{
				assert((S <= fairyPieceEnd) && "switchname is out of bounds!!!\n");
				m_hashtable[SQ][S] = getRandom();
			}
		}

		assert(VerifyHashTable() && "FillHashTable somehow did not work in Zobrist_Hashtable constructor.");
		//taken.clear(); //reducing size of the object; we shouldn't ever need it again.
	}

};

extern Zobrist_Hashtable zhashtable;

//creates a hash from a single piecearray, does NOT apply side-to-move hash (you should probably always do it on the matching turn, otherwise en-passant won't be the same)
zhash_t HashPieceArray(PieceArray& Arr);
zhash_t HashPieceArray(PieceArray& ArrL, PieceArray& ArrR);



#endif
