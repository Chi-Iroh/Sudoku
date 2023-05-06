#define NOMINMAX

#include <iostream>
#include <fstream>
#include <random>
#include <optional>
#include <filesystem>
#include <string>
#include <array>
#include <chrono>
#include <limits>
#include <set>
#include <memory>
#include <Windows.h>

constexpr std::size_t gridSize{ 9 };
constexpr std::size_t nBoxes{ gridSize * gridSize };
constexpr char maxColumn{ 'A' + gridSize };
constexpr char realMaxColumn{ maxColumn - 1 };

struct Box;

using grid_t = std::array<Box, nBoxes>;

using box_t = std::pair<char, std::size_t>;
constexpr box_t cancellingBoxInputCode{ 'Z', 0 };

constexpr box_t toGridIndexes(std::size_t arrayIndex) {
	return { static_cast<char>('A' + arrayIndex / gridSize), arrayIndex % gridSize};
}

constexpr std::size_t toArrayIndex(char column, std::size_t line) {
	return line * gridSize + (column - 'A');
}

struct Box {
	bool isConstant{};
	std::optional<std::size_t> value{};

	constexpr Box(bool isConstant = false, std::optional<std::size_t> value = std::nullopt) { // intentionally not marked as 'explicit', so that array can default-initialize Boxes
		this->isConstant = isConstant;
		this->value = value;
	}

	constexpr bool isEmpty() const {
		return !value.has_value();
	}

	constexpr char get() const {
		return isEmpty() ? ' ' : static_cast<char>('0' + value.value());
	}
};

enum class Colour {
	Black,
	DarkBlue,
	LightBlue,
	DarkGreen,
	DarkRed,
	DarkPurple,
	Orange,
	White,
	Grey,
	Blue,
	Green,
	LighterBlue,
	Red,
	Purple,
	Yellow,
	BrightWhite
};

namespace defaultColours {
	constexpr Colour constantBoxes{ Colour::DarkRed };
	constexpr Colour commands{ Colour::Blue };
	constexpr Colour boxEdges{ Colour::DarkBlue };
	constexpr Colour editableBoxes{ Colour::White };
	constexpr Colour defaultConsoleColour{ Colour::White };
}

struct Colours {
	Colour constantBoxes{ defaultColours::constantBoxes };
	Colour commands{ defaultColours::commands };
	Colour boxEdges{ defaultColours::boxEdges };
	Colour editableBoxes{ defaultColours::editableBoxes };

	Colour& operator[](std::size_t index) {
		switch (index) {
		case 0:
			return constantBoxes;

		case 1:
			return commands;

		case 2:
			return boxEdges;

		case 3:
			return editableBoxes;

		default:
			throw std::out_of_range("");
		}
	}
};

void changeColour(Colour colour) {
	static HANDLE console{ GetStdHandle(STD_OUTPUT_HANDLE) };
	SetConsoleTextAttribute(console, static_cast<WORD>(colour));
}

inline void resetConsoleColour() {
	changeColour(defaultColours::defaultConsoleColour);
}

void displayGrid(const std::array<Box, nBoxes>& grid, const Colours& colours) {
	static const std::array<std::string, gridSize> commands{
		"1 - Edit",
		"2 - Erase",
		"3 - Retry",
		"4 - Save",
		"5 - Load",
		"6 - Change colours",
		"7 - Restore default colours",
		"8 - Solution",
		"9 - Exit"
	};

	resetConsoleColour();
	std::cout << ' ';
	for (char column{ 'A' }; column < maxColumn; column++) {
		std::cout << "      " << column;
	}

	changeColour(colours.commands);
	std::cout << "             Commands :" << std::endl << "    ";
	
	changeColour(colours.boxEdges);
	for (std::size_t i{}; i < gridSize; i++) {
		std::cout << "+------";
	}
	std::cout << '+' << std::endl;

	for (std::size_t line{}; line < gridSize; line++) {
		resetConsoleColour();
		std::cout << line << "   ";


		for (std::size_t column{}; column < gridSize; column++) {
			resetConsoleColour();
			if (column == 0 || column == 3 || column == 6) {
				changeColour(colours.boxEdges);
			}
			std::cout << "|  ";

			const auto& box{ grid[toArrayIndex(static_cast<char>('A' + column), line)]};
			if (box.isConstant) {
				changeColour(colours.constantBoxes);
			}
			else {
				changeColour(colours.editableBoxes);
			}
			std::cout << box.get() << "   ";
		}

		changeColour(colours.boxEdges);
		std::cout << "|        ";

		changeColour(colours.commands);
		std::cout << commands[line] << std::endl;

		changeColour(colours.boxEdges);
		std::cout << "    +";
		for (std::size_t i{}; i < gridSize; i++) {
			resetConsoleColour();
			if (line == 2 || line == 5 || line == 8) {
				changeColour(colours.boxEdges);
			}
			std::cout << "------";
			if ((line != 2 && line != 5 && line != 8) && (i == 2 || i == 5 || i == 8)) {
				changeColour(colours.boxEdges);
			}
			std::cout << '+';
		}
		std::cout << std::endl;
	}
	resetConsoleColour();
}

std::string currentFormattedPath() {
	std::string currentPath{ std::filesystem::current_path().string() };
	std::replace(currentPath.begin(), currentPath.end(), '\\', '/');
	return currentPath;
}

std::size_t countLevels() {
	using namespace std::string_literals;

	std::size_t nLevels{};
	std::filesystem::recursive_directory_iterator iterator{ currentFormattedPath() + "/Levels"};
	for (const auto& element : iterator) {
		nLevels += element.path().extension() == ".txt";
	}
	return nLevels;
}

std::size_t randomLevel() {
	static const auto firstCallTime{ std::chrono::system_clock::now() };
	std::random_device device{};
	std::mt19937::result_type seed{ static_cast<std::mt19937::result_type>(device() ^ std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - firstCallTime).count()) };
	std::mt19937 generator{ seed };
	return std::uniform_int_distribution<std::mt19937::result_type>(1, countLevels())(generator);
}

std::array<Box, nBoxes> openLevel(std::size_t levelID) {
	std::array<Box, nBoxes> levelGrid{};
	std::ifstream level{ currentFormattedPath() + "/Levels/" + std::to_string(levelID) + ".txt" };
	for (std::size_t lineID{}; lineID < gridSize; lineID++) {
		std::string line{};
		std::getline(level, line);
		for (std::size_t columnID{}; char c : line) {
			const Box box{ Box(c != ' ', (c != ' ') ? (c - '0') : std::optional<std::size_t>()) };
			levelGrid[toArrayIndex(static_cast<char>('A' + columnID), lineID)] = box;
			columnID++;
		}
	}
	return levelGrid;
}

void clearCin() {
	std::cin.clear();
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

template<typename T>
// lowerBound <= input <= upperBound
T securedInput(const std::string& message, T lowerBound, T upperBound) {
	T value{};
	while (true) {
		std::cout << message << std::endl;
		std::cin >> value;

		if (lowerBound <= value && value <= upperBound) {
			return value;
		}
		clearCin();
	}
}

bool yesNoInput(const std::string & message) {
	char input{};
	do {
		std::cout << message << " [y/n]" << std::endl;
		if (!(std::cin >> input)) {
			clearCin();
		}
	} while (input != 'y' && input != 'n');
	return input == 'y';
}

box_t securedInputBox(const std::string& action) {
	std::string boxID{};
	clearCin();
	while (true) {
		std::cout << "Which box " << action << " ? (0 to cancel)" << std::endl;
		std::getline(std::cin, boxID);
		if (boxID == "0") {
			return cancellingBoxInputCode;
		}

		if (boxID.size() != 2) {
			std::cerr << "Two characters only ! (e.g A1, B4...) " << std::endl;
			continue;
		}

		if ('A' <= boxID[0] && boxID[0] <= realMaxColumn) {
			if ('0' <= boxID[1] && boxID[1] <= '8') {
				return { boxID[0], boxID[1] - '0' };
			}
			std::cerr << "Line must be included between 1 and 8 !" << std::endl;
		}
		std::cerr << "Column must be included between A and " << realMaxColumn << std::endl;
	}
}

void editBox(grid_t& grid) {
	while (true) {
		const auto [column, line] = securedInputBox("edit");
		if (box_t{ column, line } == cancellingBoxInputCode) {
			return;
		}
		if (grid[toArrayIndex(column, line)].isConstant) {
			std::cerr << "Cannot edit " << column << line << " ! (constant box)" << std::endl;
			continue;
		}

		grid[toArrayIndex(column, line)].value = securedInput("New value ? [1-9]", 1, 9);
		break;
	}
	return;
}

void eraseBox(grid_t& grid) {
	while (true) {
		const auto [column, line] = securedInputBox("erase");
		if (box_t{ column, line } == cancellingBoxInputCode) {
			return;
		}
		if (grid[toArrayIndex(column, line)].isConstant) {
			std::cerr << "Cannot erase " << column << line << " ! (constant box)" << std::endl;
			continue;
		}

		grid[toArrayIndex(column, line)].value = std::nullopt;
		break;
	}
	return;
}

void retryLevel(grid_t& grid) {
	if (!yesNoInput("Retry ? (All your unsaved changes will be erased)")) {
		return;
	}
	for (auto& box : grid) {
		if (!box.isConstant) {
			box.value = std::nullopt;
		}
	}
}

void saveState(const grid_t& grid, const Colours& colours) {
	const std::size_t saveLocation{ securedInput<std::size_t>("Which location use for saving ? [1-3]", 1, 3) };
	std::ofstream saveFile{ currentFormattedPath() + "/Saves/" + static_cast<char>('0' + saveLocation) + ".txt" };

	for (std::size_t line{}; line < gridSize; line++) {
		for (char column{ 'A' }; column < maxColumn; column++) {
			const Box& box{ grid[toArrayIndex(column, line)] };
			if (box.isConstant) {
				saveFile << '(' << box.value.value() << ')';
			}
			else {
				saveFile << box.get();
			}
		}
		saveFile << std::endl;
	}
	saveFile << static_cast<std::size_t>(colours.boxEdges) << ' ' << static_cast<std::size_t>(colours.commands) << ' ';
	saveFile << static_cast<std::size_t>(colours.constantBoxes) << ' ' << static_cast<std::size_t>(colours.editableBoxes);
}

std::set<std::size_t> availableSaves() {
	std::set<std::size_t> saves{};
	std::filesystem::recursive_directory_iterator iterator{ currentFormattedPath() + "/Saves" };
	for (const auto& element : iterator) {
		if (element.path().extension() == ".txt") {
			saves.insert(std::stoul(element.path().stem().string()));
		}
	}
	return saves;
}

void loadFileData(const std::string& filePath, grid_t& grid, Colours& colours) {
	const bool isSolutionFile{ filePath.contains("Solutions") };
	std::ifstream file{ filePath };
	for (std::size_t lineID{}; lineID < gridSize; lineID++) {
		std::string line{};
		std::getline(file, line);
		char column{ 'A' };
		for (std::size_t i{}; i < line.size(); i++) {
			Box& currentBox{ grid[toArrayIndex(column++, lineID)] };
			if (line[i] == ' ') {
				currentBox.value = std::nullopt;
				currentBox.isConstant = false;
			}
			else if (std::isdigit(line[i])) {
				currentBox.value = line[i] - '0';
				currentBox.isConstant = isSolutionFile; // no parenthesises in solution files
			}
			else if (line[i] == '(') {
				currentBox.value = line[(++i)++] - '0'; // (4) : i points to '('; ++i points to '4'; (++i)++ will point to ')'; the the 'i++' of the loop will point right after the ')'
				currentBox.isConstant = true;
			}
			else {
				throw std::invalid_argument("Error while parsing file \"" + filePath + "\" at line " + std::to_string(lineID) + " (unexpected character : '" + line[i] + "') !");
			}
		}
	}
	if (isSolutionFile) { // no colours specified in solution files
		return;
	}
	std::array<std::size_t, 4> inputColours{};
	file >> inputColours[0] >> inputColours[1] >> inputColours[2] >> inputColours[3];
	colours.boxEdges = static_cast<Colour>(inputColours[0]);
	colours.commands = static_cast<Colour>(inputColours[1]);
	colours.constantBoxes = static_cast<Colour>(inputColours[2]);
	colours.editableBoxes = static_cast<Colour>(inputColours[3]);
}

void loadBackup(grid_t& grid, Colours& colours) {
	const std::set<std::size_t> saves{ availableSaves() };
	if (saves.size() == 0) {
		std::cerr << "No saves found !" << std::endl;
		return;
	}
	std::size_t saveID{};
	while (true) {
		std::cout << "Choose a save [";
		for (auto iter{ saves.cbegin() }; iter != saves.cend(); iter++) {
			if (iter != saves.cbegin()) {
				std::cout << '/';
			}
			std::cout << *iter;
		}
		std::cout << ']' << std::endl;

		if (!(std::cin >> saveID)) {
			clearCin();
			continue;
		}

		if (!saves.contains(saveID)) {
			std::cerr << "Save " << saveID << " doesn't exist !" << std::endl;
			continue;
		}

		break;
	}

	loadFileData(currentFormattedPath() + "/Saves/" + std::to_string(saveID) + ".txt", grid, colours);
}

void changeColours(Colours& colours) {
	std::array<std::string, 4> coloursText{
		"Box edges",
		"Commands",
		"Constant boxes",
		"Editable boxes"
	};
	for (std::size_t colour{}; colour < 4; colour++) {
		resetConsoleColour();
		std::cout << coloursText[colour] << " colour ? [1-15] (0 to cancel)" << std::endl;
		for (std::size_t i{ 1 }; i < 0x10; i++) {
			changeColour(static_cast<Colour>(i));
			std::cout << i << ' ';
		}
		std::cout << std::endl;

		std::size_t newColour{ 0x10 };
		while (true) {
			if (std::cin >> newColour) {
				if (newColour < 0x10) {
					break;
				}
			}
			else {
				clearCin();
			}
			std::cout << coloursText[colour] << " colour ? [1-15] (0 to leave unchanged)" << std::endl;
		}
		if (newColour != 0) {
			colours[colour] = static_cast<Colour>(newColour);
		}
	}
}

void restoreDefaultColours(Colours& colours) {
	if (!yesNoInput("Restore default colours ? (Unsaved colour preferences will be erased)")) {
		return;
	}
	colours.boxEdges = defaultColours::boxEdges;
	colours.commands = defaultColours::commands;
	colours.constantBoxes = defaultColours::constantBoxes;
	colours.editableBoxes = defaultColours::editableBoxes;
}

void loadSolution(grid_t& grid, std::size_t levelID) {
	Colours unusedColours{}; // I don't give the main colours to 'loadFileData' in order to leave the user-set colours unchanged
	loadFileData(currentFormattedPath() + "/Solutions/" + std::to_string(levelID) + ".txt", grid, unusedColours);
}

int main() {
	bool appOpened{ true };
	resetConsoleColour();
	const std::size_t levelID{ randomLevel() };
	std::array<Box, nBoxes> grid{ openLevel(levelID) };
	Colours colours{};
	
	do {
		displayGrid(grid, colours);
		const std::size_t input{ securedInput<std::size_t>("Command ? [1-9]", 1, 9) };

		switch (input) {
		case 1:
			editBox(grid);
			break;

		case 2:
			eraseBox(grid);
			break;

		case 3:
			retryLevel(grid);
			break;

		case 4:
			saveState(grid, colours);
			break;

		case 5:
			loadBackup(grid, colours);
			break;

		case 6:
			changeColours(colours);
			break;

		case 7:
			restoreDefaultColours(colours);
			break;

		case 8:
			loadSolution(grid, levelID);
			break;

		case 9:
			appOpened = false;
			break;
		}

		std::system("cls");
	} while (appOpened);
}