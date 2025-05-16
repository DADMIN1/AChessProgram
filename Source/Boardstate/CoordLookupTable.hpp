#ifndef COORDLOOKUPTABLEHPP_INCLUDED
#define COORDLOOKUPTABLEHPP_INCLUDED

#include <map>
#include <utility> //std::pair
#include <string_view>
#include <string>

#include <boost/bimap.hpp>
/* #include <boost/bimap/support/iterator_type_by.hpp>
#include <boost/bimap/support/map_type_by.hpp>
#include <boost/bimap/support/key_type_by.hpp>
#include <boost/bimap/support/data_type_by.hpp>
#include <boost/bimap/support/map_by.hpp>
#include <boost/bimap/support/value_type_by.hpp> */

#include <ostream>
//overload to print arbitrary pairs
template <typename T, typename U>
std::ostream& operator<<(std::ostream& os, const std::pair<T, U>& pair)
{ return (os << '(' << pair.first << ',' << pair.second << ')'); };

//https://stackoverflow.com/questions/30704621/tagged-boostbimap-in-templates-do-they-work

//this class exists so that it's not a massive pain in the ass to call .insert / .find
template <typename K, typename V>
class BetterMap : public virtual boost::bimap<K, V, boost::bimaps::set_of_relation<> >
{ //Virtual inheritance prevents derived classes from inheriting duplicate instances of the base-class
public: //if you made these private, the derived classes (NameTableType) wouldn't be able to use them.
	using mapType = typename boost::bimap<K, V, boost::bimaps::set_of_relation<> >;
	using insertType = typename mapType::value_type;
	using keyL = typename K::tag; //Coord (the arbitrary struct passed as a tag)
	using keyR = typename V::tag; //AlgName
	using valueType_L = typename mapType::left_value_type::left_value_type; //const std::pair<int,int>
	using valueType_R = typename mapType::left_value_type::right_value_type; //const std::string
	//valuetype R being "left::right" isn't a typo. "right::right" would give the mirrored layout; returning the int-pair.
	
	auto insert(const auto& l, const auto& r) //return type cannot be a reference (or const?)
	{ return this->mapType::insert(insertType(l, r)); }
/* 	auto insertNoRef(auto l, auto r)
	{ return this->mapType::insert(insertType(l, r)); } */

	const valueType_R& Search(const valueType_L& L) { return this->mapType::template by<keyL>().find(L)->get_right(); }
	const valueType_L& Search(const valueType_R& R) { return this->mapType::template by<keyR>().find(R)->get_left(); }

	//these cannot return a reference, for some reason
	auto GetIter(const valueType_L& val) { return this->mapType::template by<keyL>().find(val); }
	auto GetIter(const valueType_R& val) { return this->mapType::template by<keyR>().find(val); }
};

/* auto IteratorConverter(auto iter) { return std::make_pair(iter->get_left(), iter->get_right()); } */


class Coord_Lookup // Table storing precalculated associations for SquareIDs, Coordinates (X,Y), and Algebraic-Names
{
	template <typename A, typename B>
	using Tagged = boost::bimaps::tags::tagged<A, B>;

	class NameTableType : public virtual BetterMap<Tagged<std::pair<int, int>, struct Coord>, Tagged<std::string, struct AlgName>>
	{  //the structs in the "BetterMap" arguements are the tags that can be used to search the map
	public:
//alternate search function to accept two ints (instead of std::pair), and return std::string_view (instead of std::string&)
		std::string_view GetStrView(int a, int b)
		{ return this->mapType::template by<typename BetterMap::keyL>().find(std::make_pair(a, b))->get_right(); }
		
		auto GetCoordIter(int a, int b)
		{ return this->GetIter(std::make_pair(a,b)); }

		NameTableType();
	} NameTable; //table of 26x26, precalculated Algebraic-Names for every Coordinate; their mapping is not dependent on Board-dimensions. Whereas the mapping of SqIDs to coords/AlgCoord is dependent on Board-Dimensions / Shape(kind of).

	using IterType = boost::bimaps::detail::map_view_iterator<Coord, boost::bimaps::detail::bimap_core<Coord_Lookup::Tagged<std::pair<int, int>, Coord>, Coord_Lookup::Tagged<std::string, AlgName>, boost::bimaps::set_of_relation<>, mpl_::na, mpl_::na>>;
	// to be used as a key in a BetterMap/bimap, the type specifically needs a less-than operator, and must be assignable
	// also, this is a type of forward-iterator, so it can't be decremented

	//unfortunately we have to copy everything over, instead of storing iterators/pointers, because boost is fucking shit
	// class SqID_MapType : public virtual BetterMap<Tagged<int, struct SqID>, Tagged< std::pair<std::pair<int, int>, std::string>, struct Link>>
	class SqID_MapType : public std::map<int, IterType>
	{ //links Sq-ID to an entry in NameTable (pair containing info about both)
	public:
		std::pair<int, int> boardSize;

		//note that any Coord <-> AlgName lookups can be performed by NameTable instead
		std::map<std::pair<int, int>, int> reverseMapCoord;
		std::map<std::string, int> reverseMapStr; //cannot be a const ref, unfortunately. String refs/views must be created from the iterator, NOT the strings stored in this map.

		SqID_MapType(NameTableType&, int numColumns, int numRows);
	} SqID_Map;

	std::map<std::pair<int,int>, SqID_MapType> BoardsizeMap;

	//test functions
	void PrintNameTable();
	void IteratorTest(auto);
	void GetIterTest_IntPairs();

public:
	void Load_IDmap(int numColumns, int numRows)
	{
		if (SqID_Map.boardSize != std::make_pair(numColumns, numRows))
		{ SqID_Map = BoardsizeMap.at(std::make_pair(numColumns, numRows)); }
		return;
	}

	std::string_view GetStrView(int a, int b) { return NameTable.GetStrView(a, b); }
	const std::pair<int, int>& GetCoord(const std::string& str) { return NameTable.Search(str); }
	const std::string& GetStrRef(int a, int b) { return NameTable.GetCoordIter(a,b)->get_right(); }
	
	const int& GetID(int a, int b) { return SqID_Map.reverseMapCoord.at(std::make_pair(a, b)); }
	const int& GetID(const std::string& s) { return SqID_Map.reverseMapStr.at(s); }
	
	/* const IterType& Lookup(int ID) { return SqID_Map.at(ID); } //we don't want to allow anyone to modify the iterator/pointer (it'll change the table?) */
	std::string_view GetStrView(int ID) { return SqID_Map.at(ID)->get_right(); } //it's important that all string_views are created from the original string stored in NameTable! (the iter returns a const reference to it)
	const std::string& GetStrRef(int ID) { return SqID_Map.at(ID)->get_right(); } // it should be okay to make a string_view out of the returned ref, right? so the "GetStrView" function is redundant?
	const std::pair<int, int>& GetCoord(int ID) { return SqID_Map.at(ID)->get_left(); }


	Coord_Lookup();
};

extern Coord_Lookup LookupTable;

#endif
