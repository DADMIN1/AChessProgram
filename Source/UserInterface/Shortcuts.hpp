#ifndef PRINTSHORTCUTS_HPP_INCLUDED
#define PRINTSHORTCUTS_HPP_INCLUDED

#include <iostream>
#include <iomanip> //formatting output buffer/cout

// these shortcut entries are not automatically generated, make sure they are accurate
void printShorcuts()
{
	std::cout << "\x1B[2J" << "\x1B[H"; // '\x1B' is the escape character // the "[2J" command "clears" the entire screen, and "[H" resets the cursor position to 0,0
	// This combination of commands is basically Equivalent to the Ctrl+L shortcut; it does what '\f' is SUPPOSED to do.

	//auto prevfmt = std::cout.setf(std::ios::left, std::ios::adjustfield);
	// int oldWidth = std::cout.width(5);

	std::cout << "Keyboard Shortcuts:\n"
		<< "Q: Exits the program\n"
		<< "F1: print these shortcuts\n"
		<< "Right-click: deselect a piece, erases a piece (setup mode)\n"
		<< "Tab: SetupMode\n"
		<< "PageUp: Debug Mode\n"
		<< "PageDown: Disables turn switching\n"
		<< "Insert: Resize the board dimensions, input is through the shell.\n"
		<< "Delete: Clear the board of all pieces.\n"
		<< "Home: Dyamically generate an initial position\n"
		<< "Shift+Home: Enter a starting-position number for FischerRandom\n" //The starting position will be written to savedFEN
		<< "End: Alternates White/Black to move\n"
		<< "Spacebar: flip the board\n"
		<< "T: Toggles whether promotions are triggered by territory or final-row\n"
		<< "M: Reverse-engineers the FischerRandom setup-ID of the current position\n" //there's no error checking, so feeding it an invalid position may segfault
		<< "S: Reshape-mode\n"
		<< "  Shift+S: restores all erased squares (Reshape-mode)\n"

		<< "MouseWheel up/down: cycle through piece selection (setup mode)\n"
		//Moving / windowHax script disabled because it's a problem
		<< "Up/Down arrowkeys: show/hide piece selection window (setup mode)\n"
			<< "  Up arrowkey (outside setup mode): display board at fullsize\n"
			//<< "   this will (attempt to) break the window outside desktop limits when numRows>8 or numColumns>16\n"
		<< "F2: switch gametype, input is through the shell.\n" //you can specify an initial-position ID X for 960: "FischerRandom X"
		<< "F3: disable board flipping\n"
		<< "F4: Test function; engine search/eval\n"
		//<< "F5: Tallies material and prints legal moves (CURRENTLY BROKEN)\n"
		<< "F5: Test function; clears arrow overlay\n"
		<< "F6: prints all entries in SquareStorage structs\n"
		<< "F7: opens/closes Color-changing window\n"
			<< "  Space: Switch color (Color-Window shortcut)\n"
			<< "  R: Reset all sliders (Color-Window shortcut)\n"
			<< "  I: Inverts outline-thickness (Color-Window shortcut)\n"
			<< "  L: LSD-Mode (Color-Window shortcut)\n"
		<< "Numpad 0: stores a FEN-string (through terminal) into savedFEN\n"
			<< "  please ensure the FEN-string ends with a forwards slash\n"
		<< "Pluskey: store current position in savedFEN\n"
		<< "Minuskey: load position from savedFEN (auto-resizes board to fit FEN)\n"
		<< "Right-Alt: alternate sound effects\n"
		<< "Tilde: display FPS\n"
			<< "  Shift+Tilde: Unlock FPS\n"
		<< "NumPad 1: Draw territory lines\n"
		<< "NumPad 2: Draw castling indicators\n"
		<< "NumPad 3: Draw royal-piece indicators\n"
		<< "NumPad 4: Draw promotion indicators\n"
		<< "NumPad 5: Draw all indicators (including territory lines)\n"
		<< "NumPad 6: Display all/none move-highlights on mouse hover\n"
		<< "NumPad 7: Display only the opponent's move-highlights on mouse hover\n"
		<< "NumPad 8: Draw the Danger Map\n"
		<< "Numpad 9: Toggles arrow-overlay visibility\n"
		<< "Pause-Break: clears input buffer, clears terminal\n"
		;

	std::cout<< "\n";
	//std::cout.width(oldWidth);
	return;
}

void PrintTest()
{
	//Doesn't work; just inserts a newline
	//std::cout << '\f'; //FF (form-feed) or NP (new-page) = '\x0C'

/*	//NONE OF THESE WORK
	std::cout << "\x1B[108";
	std::cout << "\x1B[76";
	std::cout << "\x1B[0;38";
	std::cout << "\x1B[^L";
	std::cout << "\x1B[\f"; */

	std::cout << "\x1B[2J" << "\x1B[H"; // '\x1B' is the escape character // the "[2J" command "clears" the entire screen, and "[H" resets the cursor position to 0,0
	// This combination of commands is Equivalent to the Ctrl+L shortcut; it does what '\f' is SUPPOSED to do.

	std::cout << "Beginning PrintTest:\n";
	int lineCount{0};
	bool addSpacerLines{false}; //prints "empty" lines in between each call to "print", in order to prevent interference between lines

	auto print = [&lineCount, &addSpacerLines](std::string str, bool addNewline = true)->std::ostream&
	{
		++lineCount;

		if (addSpacerLines) { std::cout << "\t--[#" << lineCount << "]--\n"; }

		std::cout << "Line#" << lineCount << ": ";
		return (addNewline ? (std::cout << str << '\n') : (std::cout << str));
	};

	auto printPrefix = [&print](auto strPrefix, std::string str, bool addNewline = true)->std::ostream&
	{
		std::cout << strPrefix;
		return (print(str, addNewline));
	};

	auto insertChars = [](char toInsert)->std::string
	{
		std::string baseStr{"0123456789"};
		for (auto it{baseStr.begin()}; ++it < baseStr.end(); ++it)
		{
			baseStr.insert(it, toInsert);
			++it; //yes, we want to increment it twice, because deleting a char (inserting backspace) shifts the position back by one
		}
		return (baseStr);
	};

	print("FirstLine");
	print("backspaces: " + insertChars('\b')); //BS = \x08
/*	//these do nothing
	print("deletes: " + insertChars('\x7F')); //DEL
	print("cancels: " + insertChars('\x18')); //CAN */

	//addSpacerLines = true;
	print("ending with carriage-return only \r", false); //gets partially overwritten
/* 	print("backspace after newline") << '\b';
	print("backspace after inline newline\n\b", false); //both seem to do nothing */
	print("ending with CR+LF\r");
	print("ending with LF+CR\n\r", false);

	printPrefix('\n', "Now Testing with prefixes:");
	printPrefix('\r', "Prefix = CR, End = LF");
	printPrefix('\r', "Prefix = CR, End = CR\r", false);
	printPrefix("\r\n", "Prefix = CR+LF, End = CR\r", false); //9
	printPrefix('\b', "Prefix = BS, End = CR+BS\r\b", false);
	printPrefix("\r\b", "Prefix = CR+BS, End = CR+BS\r\b", false); //11
	printPrefix("\b\n", "Prefix = BS+LF, End = BS+LF\b\n", false);
	printPrefix('\b', "Prefix = BS, End = CR+BS\r\b", false); //13
	printPrefix("\b\r", "Prefix = BS+CR, End = BS+CR\b\r", false); //14

	print("FinalLine"); //15

	//If spacer-lines remain disabled, it looks like the backspace prefixes/suffixes actually do something?

	std::cout << "\nFinished PrintTest\n";
}

#endif
