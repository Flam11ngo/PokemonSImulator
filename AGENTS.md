# Project Instructions

This file provides context for AI assistants working on this project.

## Project Type: Pokemon Battle Simulator

A Backend implementation of a Pokemon single battle simulator, designed to mimic the mechanics of the Pokemon Showdown battle simulator. The project is structured to allow for easy extension and maintenance, with a focus on simplifying the battle main loop and ensuring that all move, ability, and item logic is properly implemented.

Current goal is to implement more moves,abilities and items, and to simplify the battle main loop by dropping the logic of the moves,abilities and items in the battle main loop, so that it can be easily extended in the future.

### Documentation
Documentation is located in the `docs/` directory. Please refer to it for detailed information about the project structure, APIs, and usage.

Keep update `README.md` with any new features or changes to existing functionality.

### Decision Log
- **2026-05-17**: Item implementation is frozen. All remaining 152 "battle items" in data/items.json are evolution items, experience/money modifiers, PP restoration, contest scarves, EV-training weights, or breeding triggers — none affect singles PvP battle mechanics. The 145 items already registered in the engine cover every competitively relevant held item. Future work focuses on abilities and moves only.

### Version Control
This project uses Git. See .gitignore for excluded files.


## Guidelines

- Follow existing code style and patterns
- Write tests for new functionality
- Keep changes focused and atomic
- Document public APIs

## Important Notes

- Try your best to simplify the code in battle main loop.
- Make sure to drop the logic of the moves,abilities and items in the battle main loop, so that it can be easily extended in the future.
- Make sure to implement every move,ability and items logic, don't leave it with a TODO.
- Remember to update the README.md file with any new features or changes to existing functionality.
- Always test your changes locally before pushing to the repository.
- If you have any questions or need clarification, please don't hesitate to ask for help from the team. We are here to support each other and ensure the success of the project.
- Don't guess the logic of the moves,abilities and items, try to find out the exact logic by looking at the source code of the Pokemon Showdown or by testing it in the battle simulator.
- Don't guess when u have no idea to complete this feature, u can ask me.