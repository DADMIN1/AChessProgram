#include "EngineCommand.hpp"
//#include "Engine.hpp" //required to include this here if you don't include it in EngineCommand.hpp

#include <iostream>

namespace Fish {

	EngineState_t EngineState{};

	using Command = EngineState::Command;
	using Flag = EngineState::Flag;
	using TPtype = Tickpoint::TPtype;
	using TPMod = Tickpoint::TPmod;

	void EngineState_t::Tick(Tickpoint execPoint)
	{
		// wait on rootmove searchCompleted
		if ((execPoint.atDepth == 0) && (execPoint.m_type == TPtype::searchCompleted))
		{
			std::cout << "Iterative-Deepeninng completed, how to proceed?\n";
			std::cout << "[Enter: search deeper], [R: repeat search], [Esc: Exit]\n";
			executionFlag = Flag::waiting;
		}

		while (!commandStack.empty())
		{
			// always execute/read the commands in commandStack (if there's something there like "exit", we need to handle it even if an interrupt / directive already exists)
			switch (commandStack.front())
			{
				case Command::interrupt:
				{
					// if we're already handling an interrupt, then switch to it only if the passed interrupt is higher-priority than the current one.
					if (executionFlag == Flag::interrupt)
					{
						//if(activeInterrupt.priority < commandStack.front.priority)
						//swap them
						//else
					}

					executionFlag = Flag::interrupt;
					//activeInterrupt = interruptStack.front
					//pop interruptStack.front
					//activeInterrupt.TriggerInit
				}
				break;

				default:
					break;
			}

			commandStack.pop_front();
		}

		// if state is interrupt, handle/call the top interrupt
		while (EngineState.executionFlag == Flag::interrupt)
		{
			activeInterrupt->Tick();
			activeInterrupt->TriggerOnResolve();
		}

		// otherwise,

		lastTick = execPoint;
		// return Command::ignore;
		return;
	}

	bool EngineState_t::TakeInput(sf::Keyboard::Key K)
	{
		assert((isRunning) && "Engine - TakeInput somehow called while engine wasn't running!");
		if (K == sf::Keyboard::Key::Unknown) { std::cerr << "suppressing 'unused parameter' warning"; }

		/* EngineState.executionFlag = Flag::interrupt;

		while (EngineState.executionFlag == Flag::interrupt)
		{ ; } */

		return false; //
	}

	Command EngineState_t::HandleInput(sf::Keyboard::Key passedKey)
	{
		assert((isRunning) && "Engine - HandleInput somehow called while engine wasn't running!");

		switch (passedKey)
		{
			case sf::Keyboard::Key::F4:
			{
				if (isActive)
				{
					executionFlag = Flag::stopped;
					isActive = false;
					return Command::pause;
				}

				if (executionFlag == Flag::paused) { return Command::resume; }
			}
			break;
			
			default: 
			/*{
				executionFlag = Flag::stopped;
				isActive = false;
			}*/
			break;
		}

		return Command::exit;
	}

}
