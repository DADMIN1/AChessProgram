#include "CoordLookupTable.hpp"

#include <effolkronium/random.hpp>

#include <iostream>
#include <sstream> // output buffer

constexpr int MAX{26}; //same as MAX_BOARD_DIMENSIONS in Board.cpp
constexpr int MIN{3}; //same as MIN_BOARD_DIMENSIONS

std::string GetAlgebraicName(int x, int y)
{ return std::string{(static_cast<char>(x + 96) + std::to_string(y))}; }

Coord_Lookup LookupTable{};

Coord_Lookup::NameTableType::NameTableType()
{
	for (int x{1}, y{1}; ((x<=MAX)); ((++y <= MAX) ? y : (++x, y = 1)))
	{
		this->NameTableType::insert(std::make_pair(x, y), GetAlgebraicName(x, y));

		//here's all the different ways to get an iterator
/* 		auto iter = NameTableType::insert(std::make_pair(x, y), GetAlgebraicName(x, y));

		auto wtf = iter.first;
		const std::pair<int, int>& a = wtf->left; //I think it's better to call "get_left"
		const std::string& gay = wtf->right;

		//auto asdt = this->by<Coord>().find(std::make_pair(1, 2));
		auto asdt = this->find(std::make_pair(1, 2)); //I'm pretty sure you can call find without "by"
		const std::pair<int, int>& p = asdt->get_left();
		const std::string& sdffsdfd = asdt->get_right();

		auto gheryt = this->by<AlgName>().find("lol");
		const std::pair<int, int>& pdffd = gheryt->get_left(); //still the same, for some reason.
		const std::string& sdd = gheryt->get_right();

		++wtf;
		++asdt; */
	}

}

Coord_Lookup::SqID_MapType::SqID_MapType(NameTableType& N, int numColumns, int numRows)
{
	boardSize = {numColumns, numRows};
	int ID{0};

	// SqIDs were generated top-to-bottom, for some reason. Many things rely on this.
	for (int x{1}, y{numRows}; ((x<=numColumns)); ((--y > 0) ? y : (++x, y = numRows)))
	{
		insert_or_assign(ID, N.GetCoordIter(x, y));
		reverseMapCoord.insert_or_assign(std::make_pair(x, y), ID);
		reverseMapStr.insert_or_assign(this->at(ID)->get_right(), ID);
		++ID;
	}
}

Coord_Lookup::Coord_Lookup()
	: NameTable{}, SqID_Map{NameTable, 8, 8}, BoardsizeMap{}
{
	//generating a SqID_Map for every possible boardsize
	for (int x{MIN}, y{MIN}; ((x<=MAX)); ((++y <= MAX) ? y : (++x, y = MIN)))
	{
		BoardsizeMap.insert_or_assign({x,y}, SqID_MapType{NameTable, x, y});
	}

}

// Test functions //
void Coord_Lookup::GetIterTest_IntPairs()
{
	std::cout << "Getting iterators of 10 random squares by coord\n";
	for (int a{10}; a>0; --a)
	{
		int x = effolkronium::random_static::get(1, 26);
		int y = effolkronium::random_static::get(1, 26);
		auto iter = NameTable.GetIter(std::make_pair(x, y));

		// std::cout << "Coord (" << x << ',' << y <<") has the name: " << iter->get_right() << '\n';
		std::cout << "Coord: " << iter->get_left() << " = " << iter->get_right() << '\n';
	}
	return;
}

void Coord_Lookup::IteratorTest(auto testIterator)
{
	std::cout << "testing iteration\n";
	for (int a{30}; a>0; --a)
	{
		//if (!(a%10)) { std::cout << '\n'; }
		std::cout << "{";
		std::cout << testIterator->get_left() << ", ";
		std::cout << testIterator->get_right() << "}, ";
		testIterator++;
	}
	std::cout << '\n';

	return;
}

void Coord_Lookup::PrintNameTable()
{
	std::cout << "Printing NameTable \n";
	std::stringstream out;

	for (const auto& p : NameTable.left) //p is the pair of elements; so p.first is the coord, and p.second is the string
	{
		out << " || " << '(' << p.first.first << ',' << p.first.second << ")=" << '[' << p.second << ']';
		if ((p.first.second == MAX/2) || (p.first.second == MAX)) { out << " || " << '\n'; } //print newline to split each column in half (too long to print all on a single line)
	}
	out << '\n'; //otherwise we don't get a final newline

	int prev = std::cout.width(13);
	std::cout << out.str() << '\n';
	std::cout.width(prev);

	std::cout << "Coord: {6,9} has the algebraic name: " << GetStrView(6, 9) << '\n';
	std::cout << "Testing Other Search: " << GetCoord("c4") << '\n';

	IteratorTest(NameTable.begin());
	IteratorTest(NameTable.by<AlgName>().begin()); //this doesn't quite print them in order, because the way it orders strings is by per char; therefore, "a10" is immediately after "a1", and all the "a11... a20... a26" are before "a2". Sorting from last character to first might work, but it might compare numbers to letters across 2/3 letter coords.
	GetIterTest_IntPairs();

	return;
}
// End test functions //
