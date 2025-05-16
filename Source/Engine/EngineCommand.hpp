#ifndef FISH_COMMAND_HPP_INCLUDED
#define FISH_COMMAND_HPP_INCLUDED

#include "Engine.hpp"

#include <SFML/System.hpp>
#include <SFML/Window.hpp> // for keyboard key enum

#include <deque>
#include <cassert>

namespace Fish{

	// indicates the position of Engine's execution; used like breakpoints
	struct Tickpoint
	{
		enum TPtype {
			any,
			pieceLoop,
			moveSearch,
			iterativeDeepeningLoop,
			searchCompleted, //meaning the function will return
		} m_type;
		enum TPmod {
			none,
			before,
			after,
			inloop, //before/after imply outside of loop
		} m_modifier;

		int atDepth; //-1 represents any depth (when used as a target breakpoint)

		Tickpoint(TPtype t = TPtype::any, TPmod m = TPmod::none, int d = currentDepth)
			: m_type{t}, m_modifier{m}, atDepth{d}
		{
			;
		}
	};

	// Tells the engine (in the search loop thread) how to proceed
	struct Directive
	{
		enum Directive_type {
			none,
			continueTill, //keep going until you hit the specified breakpoint
			skipTill, //skip execution (searching) until you hit breakpoint
			jumpTo, //(potentially) aborts current execution. Used to jump execution to breakpoints outside of the search-loops, especially when they would not normally be reachable (restarting / quitting).

			print,
			exit,
		} m_type;

		Tickpoint m_from;
		Tickpoint m_breakpoint;

		Directive(Directive_type t) : m_type{t}
		{ ; }
	};
}

namespace EngineState {

	enum class Command {
		ignore,
		pause,
		resume,
		setFlag,
		pushCommand, //Queues a sequence to be executed AFTER EXITING THE INTERRUPT STATE!
		handleInput,

		interrupt, //halts the search (at current step), performs a queued command, then pushes another interrupt or enters the "waiting" state. (Each step-command halts the search, pushes an "interrupt", executes, returns, and then execution is immediately broken by the next "interrupt" command, which waits.) (Also useful for command chains; for example, calling 'restart' needs to set the "stopped" flag, end the current search, clear variables, and begin a new search with new state flags)

		step, //perform or undo one move, pause again.
		stepInto, //continues current line to target depth, then pauses again when it returns to move at the NEXT depth (iterate through child nodes of the current move (all responses at currentdepth+1))
		stepPast, //skips all child nodes for the move at the line's current depth, goes to the next move at the same depth
		stepOut, //skips everything in the line at the current depth, returns to move at previous depth. If done at the root node, returns from the search of the current target-depth

		clear, //erases most stored variables except the initial ones (including rootmoves). Doesn't necessarily exit the function or even the current search
		restart, //performs iterative-deeping search again, does not exit function (engine continues running)
		reset, //exits and re-starts the "search_rootmoves" function, resetting all stored variables

		exit, //shut down engine thread
	};

	enum class Flag {
		nullstate,
		searching,
		waiting, //expecting input; either completed search to targetDepth, or waiting for next command (for stepping and such)
		paused,
		stopped, //halted by user, not automatically. Aborts search, but doesn't exit function

		baseState, //return to normal execution
		interrupt, //halt normal execution, then execute the queued interrupt(s)
		resolvedState, //indicates completion of an "interrupt"; the current state's "onExitFlags" are passed back to
		//execution of the base-state resumes.Any queued interrupts tied to the resoultion of this state are triggered.
	};

	struct InterruptBase_t
	{
		Command commandType{Command::ignore}; //command(s) the interrupt was created with
		//Fish::Directive m_directive{Fish::Directive::none}; //don't put this here

		virtual void TriggerInit(); // performs all actions required by the interrupt when it's first handled/added
		virtual void Tick();
		virtual void TriggerOnResolve();

		virtual Command GetCommand(Fish::Tickpoint);
	};

	template<Command C = Command::ignore>
	struct Interrupt_t
	{ //commands to execute, including other interrupts
		const Command m_command;

		Flag exitflag{Flag::nullstate}; //execution flag to set (in globalstate) when this state is resolved

		Interrupt_t() : m_command{ C }
		{ ; }
	};

	template<>
	struct Interrupt_t<Command::handleInput>
	{
		const sf::Keyboard::Key m_input;
		bool wasSeen{false}; // main waits until this is set to true
		bool wasUsed{false}; // indicates whether main should throw away the input

		Interrupt_t(const sf::Keyboard::Key K)
			: m_input{ K }
		{
			;
		}
	};

}


namespace Fish {

	//currentDepth should be in here, so that it can be passed as a default parameter to Directive's constructor (for m_fromDepth and m_targetDepth)
	class EngineState_t
	{
		EngineState::Flag executionFlag{EngineState::Flag::baseState};
		std::deque<EngineState::Command> commandStack; //remove this; you only need interruptStack, and each interrupt can get it's own commandStack.
		EngineState::InterruptBase_t* activeInterrupt; //remove this; just use top-most interrupt
		std::deque<EngineState::InterruptBase_t*> interruptStack; //should be priority_queue instead

		Tickpoint lastTick;

		// Every interrupt has it's own "Directive", which indicates that it's command is considered "active" until the next


		// I don't remember if these have to be public declarations
		friend struct EngineState::InterruptBase_t;

		//template<typename A>
		//using Fren = EngineState::Interrupt_t<A>;
		//friend struct Fren;

	public:

		// pushes an interrupt onto the stack (top or bottom based on priority, potentially ignored if current interrupt has super-high priority (like exit command))
		void PushInterrupt(EngineState::InterruptBase_t*)
		{
			commandStack.push_front(EngineState::Command::interrupt);
			// now push interrupt onto interrupt_stack


			return;
		}
		// This should be delegated to "activeInterrupt"; it decides how the new interrupt is handled (if at all; it might get ignored)

		void Tick(Tickpoint execPoint);


		void Init()
		{
			executionFlag = EngineState::Flag::baseState;
			//stateStack.clear();
			return;
		}


		// tells the Search-function how to continue execution
		EngineState::Command GetCommand()
		{
			if (!interruptStack.empty())
			{
				return interruptStack.front()->GetCommand(lastTick);
			}

			return EngineState::Command::ignore;
		}


		bool TakeInput(sf::Keyboard::Key K);
		EngineState::Command HandleInput(sf::Keyboard::Key passedKey);
	};

	extern EngineState_t EngineState;
}


#endif
