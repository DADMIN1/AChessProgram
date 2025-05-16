#include "ZobristHash.hpp"

Zobrist_Hashtable zhashtable{};

//creates a hash from a single piecearray, doesn't apply side-to-move hash (you should probably always do it on the matching turn, otherwise en-passant won't be the same)
zhash_t HashPieceArray(PieceArray& Arr)
{
	zhash_t hash{0}; //the initial xor should set it equal to that hash
	for (auto& P : Arr)
	{
		if (P->isCaptured) { continue; }
		zhash_t oldHash = hash;
		hash xor_eq zhashtable[P->m_SqID][P->m_PieceType];
		assert((hash != oldHash) && "HashPieceArray did not update the hash!!!!\n");
	}
	return hash;
}

zhash_t HashPieceArray(PieceArray& ArrL, PieceArray& ArrR)
{
	zhash_t hash{HashPieceArray(ArrL) xor HashPieceArray(ArrR)};

	//YOU SHOULD OPTIONALLY HASH TURNS HERE!!!

	return hash;
}
