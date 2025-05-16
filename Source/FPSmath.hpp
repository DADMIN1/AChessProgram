#ifndef FPSMATH_HPP_INCLUDED
#define FPSMATH_HPP_INCLUDED

#include <SFML/System/Clock.hpp> //I assume this includes time?
#include <SFML/Graphics/Text.hpp>

#include <iostream> //don't need it anymore
#include <string>
#include <vector>
#include <chrono>
#include <numeric> //for std::accumulate to sum laptimes

class CodeTimer
{
private:
	// Type aliases to make accessing nested type easier
	using clock_type = std::chrono::steady_clock;
	using second_type = std::chrono::duration<double, std::ratio<1> >;

	std::chrono::time_point<clock_type> m_beg{clock_type::now()};

	bool isLapRunning{false};
	std::vector<double> m_lapTimes{};
	
public:
	void reset()
	{
		m_lapTimes.clear();
		isLapRunning = false;
		m_beg = clock_type::now();
	}

	double elapsed() const
	{
		return std::chrono::duration_cast<second_type>(clock_type::now() - m_beg).count();
	}

	void lapStart()
	{
		if (isLapRunning)
		{
			std::cerr << "CodeTimer is overlapping starts\n";
			return;
		}

		isLapRunning = true;
		m_beg = clock_type::now();
		return;
	}

	void lapEnd()
	{
		if (!isLapRunning)
		{
			std::cerr << "CodeTimer attempted to end lap but wasn't running\n";
			return;
		}

		isLapRunning = false;
		m_lapTimes.push_back(elapsed());
		return;
	}

	double totalLapTime() const
	{
		return std::accumulate(m_lapTimes.begin(), m_lapTimes.end(), double{0});
	}

	friend void printTotalTime(CodeTimer&);

};

extern bool isDEBUG; // in main

//These are for the old calcFPS function, use new CodeTimer class instead
extern sf::Text fpsText;
extern bool isShowFPS;
extern sf::Clock fpsTimer;
extern unsigned int frameCount;
extern bool isFPScapped;

extern std::string singleFrametime;
extern std::string FPStextBeforeSum;
extern std::string FPScalcRemainder;
extern std::string FPStextTrueSum;

extern std::vector<sf::Time> timerLedger;
extern sf::Time sumOfLedger;

void calcFPS();

#endif
