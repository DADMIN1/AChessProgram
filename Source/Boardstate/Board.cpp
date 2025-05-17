#include "Board.hpp"
#include "CoordLookupTable.hpp"
#include "SquareStorage.hpp" //generateBoard calls ClearBoard on SquareStorage

#include <ranges> //Generate ranges/views of SquareTable into Rows/Columns (SquareTable matrix)
#include <iostream>
#include <sstream> //generateBoard uses stringstream as a buffer to print the generation-info

sf::Color Boardsquare::lightSqColor{190, 160, 145};
sf::Color Boardsquare::darkSqColor{100, 75, 60};

//how many rows to remove from the top of each column
std::map<int, int> columnHeight{
	/* {1, 2},
	{2, 1},
	{4, 4},
	{7, 1},
	{8, 2}, */
};

std::multimap<int, int> excluded_C{}; //coords of excluded, mapped as column,row
std::multimap<int, int> excluded_R{}; //coords of excluded, mapped as row, column
std::set<std::pair<int, int>> manual_excludes{}; //the user has excluded these during reshape-mode

bool isTriggerResize{true}; //for the initial setup

Boardsquare::Boardsquare(int x, int y, int IDnum) : sf::RectangleShape({SqWidth, SqHeight}),
	m_ID{IDnum}, column{x}, row{y}, m_algCoord{LookupTable.GetStrView(column, row)},
	isDark{!(static_cast<bool>((column + row) % 2))}, isOccupied{false}, isExcluded{false},
	edgeDist{{(m_ID % numRows)}, {(numRows - (1 + edgeDist.Top))} , {(m_ID / numRows)} , {(numColumns - (1 + edgeDist.Left))}}
	//edgeDist{ {(m_ID % numRows) - columnHeight[x]}, {((numRows-columnHeight[x]) - (1 + edgeDist.Top))} , {(m_ID / numRows)} , {(numColumns - (1 + edgeDist.Left))} }
{
	if ((row > (numRows - columnHeight[x])) || manual_excludes.contains({column,row}))
	{
		//std::cout << m_algCoord << " has been excluded.\n";
		isExcluded = true;
		excluded_C.insert({x,y});
		excluded_R.insert({y,x});
	}

	setSize(SqWidth, SqHeight);
	setPosition((column - 1)* SqWidth, ((numRows - row)* SqHeight));
	// setFillColor((isDark) ? darkSqColor : lightSqColor);
	setFillColor((isDark) ? darksqColor : lightsqColor); //using global colors, not the static class-members
	setOutlineColor((isDark) ? outlinecolorDark : outlinecolorLight);
	setOutlineThickness((isDark)? (float(sqOutlineSignDark)*sqOutlineThicknessD) : (float(sqOutlineSignLight)*sqOutlineThicknessL));
}

std::vector<Boardsquare> SquareTable;

void Boardsquare::RecalcEdgeDist()
{
	if (excluded_C.contains(column))
	{
		for (std::multimap<int,int>::iterator R{excluded_C.lower_bound(column)}; R != excluded_C.upper_bound(column); ++R)
		{
			if ((R->second <= row) && (R->second >= (row - edgeDist.Bottom)))
			{ edgeDist.Bottom = row - R->second - 1; }

			if ((R->second >= row) && (R->second <= (row + edgeDist.Top)))
			{ edgeDist.Top = (R->second - row - 1); }
		}
	}

	if (excluded_R.contains(row))
	{
		for (std::multimap<int, int>::iterator C{excluded_R.lower_bound(row)}; C != excluded_R.upper_bound(row); ++C)
		{
			if ((C->second <= column) && (C->second >= (column - edgeDist.Left)))
			{ edgeDist.Left = column - C->second - 1; }

			if ((C->second >= column) && (C->second <= (column + edgeDist.Right)))
			{ edgeDist.Right = C->second - column - 1; }
		}
	}

	return;
}

void Boardsquare::PrintEdgeDists()
{
	std::cout << '\v' << m_algCoord << '\n';
	std::cout << "Edgedist Top = " << edgeDist.Top << '\n';
	std::cout << "Edgedist Bottom = " << edgeDist.Bottom << '\n';
	std::cout << "Edgedist Left = " << edgeDist.Left << '\n';
	std::cout << "Edgedist Right = " << edgeDist.Right << '\n';

	return;
}

squaretable_matrix_t GetBoardRows(std::vector<Boardsquare>& sourceTable)
{
	squaretable_matrix_t AutoVector;

	std::ranges::for_each(std::views::iota(1, (numRows+1)), [&AutoVector, &sourceTable](int Y) mutable{std::ranges::filter_view Temp{sourceTable | std::views::filter([Y](Boardsquare& B)->bool { return (B.row == Y); })}; AutoVector.emplace_back(Temp.begin(), Temp.end()); });

	return AutoVector;
}

void GenerateBoard(bool isResizingBoard) //totalSquares always equals numRows*numColumns. You can call GenerateBoard(SetBoardDimensions()); unless you don't want to be prompted for new dimensions;
{
	ClearBoard();
	SquareTable.clear();
	SquareTable.reserve(numColumns*numRows);

	LookupTable.Load_IDmap(numColumns, numRows);

/* 	excluded_C.clear();
	excluded_R.clear(); */

	int c = 1;
	int r = (numRows - 1); // Starting from the 2nd rank from the top (because columns will generate the first row)
	int sqNum = 0;		   // Number of squares generated.

//Resize window to make new dimensions look nice
	constexpr int WinHeightMax{ 1050 }; //titlebar screws this up, so it's not 1080
	constexpr int WinWidthMax{ 2560 };

	//Don't do this if we're just clearing/resetting the squares
	//Only when you actually change the board dimensions
	if (isResizingBoard)
	{
		winWidth = (960*numColumns/8);
		winHeight = (960*numRows/8);

		if (winHeight > WinHeightMax)
		{
			//reduce the other dimension in-proportion
			winWidth *= (WinHeightMax/winHeight);
			winHeight = WinHeightMax;
		}
		if (winWidth > WinWidthMax)
		{
			winHeight *= (WinWidthMax/winWidth);
			winWidth = WinWidthMax;
		}

	//This stuff is technically redundant because it'll get set by the resize-event anyway
		SqHeight = (winHeight / numRows);
		SqWidth = (winWidth / numColumns);

		HorizontalScale = (SqWidth / 120);
		VerticalScale = (SqHeight / 120);

	//You must trigger a sf::resize-event after this function
	//boardWindow.setSize(sf::Vector2u(winWidth,winHeight));
		isTriggerResize = true; //This will trigger a window-resize
	}

	std::cout << "\nGenerating Boardsize: " << numColumns << 'x' << numRows << '\n';

	while (c <= numColumns)
	{
		//push back creates a copy, emplace_back constructs the element in-place
		SquareTable.emplace_back(c,numRows,sqNum);

		//if (isDEBUG)
		{
			std::stringstream tempString;
			tempString << SquareTable.back().m_algCoord << '[' << sqNum << ']' << " || ";
			int prev = std::cout.width(11); //ID can be three-digit, so it's 11, not 10
			std::cout << tempString.str();

			/* if (SquareTable.back().isDark) { std::cout << ":D"; }
			else {std::cout << ":L";} */
			std::cout.width(prev);
		}
		++sqNum;

		while (r > 0)
		{
			SquareTable.emplace_back(c,r,sqNum);

			//if (isDEBUG)
			{
				std::stringstream tempString;
				tempString << SquareTable.back().m_algCoord << '[' << sqNum << ']' << " || ";
				int prev = std::cout.width(11);
				std::cout << tempString.str();

				/* if (SquareTable.back().isDark) { std::cout << ":D"; }
				else { std::cout << ":L"; } */
				std::cout.width(prev);
			}
			++sqNum;
			--r;
		}
		//if (isDEBUG)
		{ //puts a newline at the end of each completed column
			std::cout << '\n';
		}

		++c;
		r = (numRows - 1);
	}

	//unfortunately, we can't do this while generating the board, because they are only added to the excluded list after they have been generated.
	/* for (Boardsquare& Sq : SquareTable)
	{
		Sq.RecalcEdgeDist();
	} */
	//The problem is that move-generation assumes that edgedist actually refers to the end of the board, so diagonals and jump-moves won't be generated when the pieces are next to an excluded square inside of the board.

	std::cout << "\v\n";
	return;
}

void SetBoardDimensions()
{
	std::cout << "\nCurrent Boardsize: " << numColumns << 'x' << numRows << '\n';
	int numColumnsStored = numColumns; //Reset to these if something goes wrong
	int numRowsStored = numRows;

	if ((numColumns < MIN_BOARD_DIMENSIONS) || (numColumns > MAX_BOARD_DIMENSIONS) || (numRows < MIN_BOARD_DIMENSIONS) || (numRows > MAX_BOARD_DIMENSIONS))
	{
		std::cout << "Current dimensions invalid, resetting to 8x8 \n";
		numColumns = 8;
		numRows = 8;
		numColumnsStored = 8;
		numRowsStored = 8;
	}

	std::cout << "Enter board dimensions between " << MIN_BOARD_DIMENSIONS << " and " << MAX_BOARD_DIMENSIONS << '\n';
	std::cout << "(Enter 0 to cancel) \n";

	int userInputC = 0;
	int userInputR = 0;
	std::string throwawayBuffer{}; // std::cin won't put anything into userInputC if it's not an integer
	
	if (std::cin >> userInputC) {
		if (userInputC != 0) numColumns = userInputC;
		else {
			std::cout << "board resize cancelled \n";
			numColumns = numColumnsStored;
			numRows = numRowsStored;
			return;
		}
	} else {
		std::cin.clear();
		if (std::cin >> throwawayBuffer) {
			std::cout << "bad input: ";
			std::cout << '[' << throwawayBuffer << ']' << '\n';
		}
		std::cout << "board resize cancelled \n";
		numColumns = numColumnsStored;
		numRows = numRowsStored;
		return;
	}
	
	// checking if two numbers were given on same line
	// extract and discard space or 'x' between the numbers if they are
	if (char maybeMiddle(std::cin.peek()); maybeMiddle != std::cin.eof()) {
		if ((maybeMiddle == 'x') || (maybeMiddle == ' ')) std::cin.get();
		else {
			std::cin.clear();
			std::cout << userInputC << " by ";
		}
	} else {
		std::cin.clear();
		std::cout << userInputC << " by ";
	}
	
	if (std::cin >> userInputR) {
		if (userInputR != 0) numRows = userInputR;
		else {
			std::cout << "board resize cancelled \n";
			numColumns = numColumnsStored;
			numRows = numRowsStored;
			return;
		}
	} else {
		std::cin.clear();
		if (std::cin >> throwawayBuffer) {
			std::cout << "bad input: ";
			std::cout << '[' << throwawayBuffer << ']' << '\n';
		}
		std::cout << "board resize cancelled \n";
		numColumns = numColumnsStored;
		numRows = numRowsStored;
		return;
	}

	// if the given values violate the min/max values, reset it to 8x8 and tell them about it.
	if ((numColumns < MIN_BOARD_DIMENSIONS) || (numColumns > MAX_BOARD_DIMENSIONS) || (numRows < MIN_BOARD_DIMENSIONS) || (numRows > MAX_BOARD_DIMENSIONS))
	{
		std::cout << "Invalid input: " << numColumns << " , " << numRows << "; cancelling \n ";
		numColumns = numColumnsStored;
		numRows = numRowsStored;
		return;
	}

	//It's actually important to clear the board no matter what
	//if((numColumns != numColumnsStored) && (numRows != numRowsStored))
	GenerateBoard(true);

	return;
}

void FlipBoard()
{
//RECALCULATES ALL BOARDSQUARE POSITIONS BASED ON isBoardFlipped()

	for (int i{ 0 }; i < (numRows * numColumns); i++)
	{
		if (isFlipboard == true)
		{//inverted offsets
			SqOffsetx = ((numColumns - SquareTable[i].column) * SqWidth);
			SqOffsety = ((SquareTable[i].row - 1) * SqHeight);
			SquareTable[i].setPosition(SqOffsetx,SqOffsety);
		}
		if (isFlipboard == false)
		{//defaults
			SqOffsetx = ((SquareTable[i].column - 1) * SqWidth);
			SqOffsety = ((numRows - SquareTable[i].row) * SqHeight);
			SquareTable[i].setPosition(SqOffsetx,SqOffsety);
		}

	}
	return;
}


