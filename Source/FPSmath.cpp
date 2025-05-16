#include "FPSmath.hpp"

sf::Text fpsText{};
bool isShowFPS{false};
sf::Clock fpsTimer;
unsigned int frameCount{0}; //frame counter
bool isFPScapped{true};

std::string singleFrametime{"16.67 ms \n"};
std::string FPStextBeforeSum;
std::string FPScalcRemainder;
std::string FPStextTrueSum;

std::vector<sf::Time> timerLedger{};
sf::Time sumOfLedger{};

void printTotalTime(CodeTimer& timer)
{
	std::cout << " ran " << timer.m_lapTimes.size() << " times over " << timer.totalLapTime() << " seconds\n";
	return;
}

//The manual calculation gives us a fractional FPS output, but it's wildly inaccurate above 60FPS, and crashes when average frame time < 1ms (because the intPart rounds down to 0, which is used as a divisor)
//So it's all commented out because I'm not going to waste time on it, just use the basic
void calcFPS()
{	
	timerLedger[frameCount] = fpsTimer.getElapsedTime();
	sumOfLedger += timerLedger[frameCount]; //add the last-frame time each call, instead of summing all at once
	frameCount += 1;

	//Calculate the averages only after some amount of frames
	if (frameCount == timerLedger.size())
	{
		/*we're splitting the number into two parts because there's no other way to truncate the number to a specific decimal place
		If we try and display the singleFrametime as milliseconds normally, it rounds to an integer (16), and if we use microseconds,it'll show as 16234 or something, without a decimal place.
		Converting to float to get a decimal place changes it to NINE significant digits, and is much less efficient, and isn't very accurate*/

		/* 		unsigned int intPart; //part of number before decimal place
				unsigned int decPart; //part of number after decimal place

				intPart = (sumOfLedger.asMilliseconds()/frameCount);
				decPart = ((sumOfLedger.asMicroseconds() / frameCount) - ((sumOfLedger.asMilliseconds() / frameCount * 1000))); */

				//if you have an FPS > 1000, then the average will be < 1 millisecond, which intPart will round down to 0 (as it should, since we would want to display "0.xxx ms"). Unfortunately, this will crash because we divide 1000 by intPart to get the whole-number of ms.
		 		/*if (intPart == 0)
				{
					std::cerr << "\nsum of timerLedger rounded down to 0!!!! Incoming segfault!\n";
		
					for (int I{0}; I < timerLedger.size(); ++I)
					{
						std::cerr << "timerLedger[" << I << "] = " << timerLedger[I].asMicroseconds() << '\n';
					}
		
				} */

		//unsigned int wholePart{((intPart > 0)? (1000 / intPart) : (frameCount))};

		//setting strings
		// singleFrametime = (std::to_string(intPart) + "." + std::to_string(decPart) + " ms \n"); //top string
		singleFrametime = (std::to_string(sumOfLedger.asMilliseconds()/static_cast<double>(frameCount)) + " ms \n"); //top string

	/*our singleFrametime is supposed to be a millisecond amount, but it's actually two numbers: the part before the decimal
	is the whole number result of a conversion to milliseconds, and the number after is the remainder in microseconds (the decimal is fake,
	so it's 1000x larger than it appears) we have to do it this way to prevent a floating-point conversion; no decimals allowed.
	Therefore, when calculating the FPS from these averages, the first division we do (1000/intPart) is essentially roundeding down to the nearest MS,
	so the result is an overestimate. The difference must be found by SUBTRACTING the result of the decPart.
	*/

	/* if(isDEBUG)
	{
		FPStextBeforeSum = ("~" + std::to_string(1000 / intPart) + "FPS \n");
		FPScalcRemainder = ("-" + std::to_string(1000 / decPart) + "." + std::to_string((1000000 / decPart) - ((1000 / decPart) * 1000)) + "FPS \n"); //this should be "ms", not "FPS"??? It used to be.
	} */

	//The decPart+1 is because any subtracting any nonzero amount from a whole (division of intpart) will leave you with whole-1 and some remainder
//So really there is an exception if decPart is exactly 0.000 , but I don't feel like checking that.

		// FPStextTrueSum = (std::to_string((1000 / intPart) - ((1000 / decPart)+1)) + "." + std::to_string((1000 - ((1000000 / decPart) - ((1000 / decPart) * 1000)))) + "FPS");

		FPStextTrueSum = (std::to_string(1000000/(sumOfLedger.asMicroseconds() / frameCount)) + "FPS");

		/* 	if(isDEBUG)
			{
				fpsText.setString(singleFrametime + FPStextBeforeSum + FPScalcRemainder + FPStextTrueSum);
			}
			else */
		fpsText.setString(singleFrametime + FPStextTrueSum);

		sumOfLedger = sf::microseconds(0);
		frameCount = 0;
	}

	fpsTimer.restart();
	return;
}
