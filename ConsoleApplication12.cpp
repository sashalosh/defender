
#define NOMINMAX
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <filesystem> 
#include <commdlg.h> 

#pragma comment(lib, "Comdlg32.lib")

namespace fs = std::filesystem;
using namespace std;


const string catalogDir = "music/catalog/";
const string lyricsDir = "music/add/";


const WORD COLOR_DEFAULT = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
const WORD COLOR_TITLE = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
const WORD COLOR_HIGHLIGH = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;

void setColor(WORD color) {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}


void pause() {
	cout << "\nPress Enter to continue...";
	cin.ignore(numeric_limits<streamsize>::max(), '\n');
}


void createDirectories() {
	if (!fs::exists("music")) {
		fs::create_directories("music");
	}
	if (!fs::exists(catalogDir)) {
		fs::create_directories(catalogDir);
	}
	if (!fs::exists(lyricsDir)) {
		fs::create_directories(lyricsDir);
	}
}

string toFilename(const string& title) {
	string result = title;
	for (char& c : result) {
		if (c == ' ') c = '_';
	}
	return result;
}


bool readFileToString(const string& filePath, string& outContent) {
	ifstream file(filePath, ios::binary);
	if (!file) return false;
	vector<char> buffer((istreambuf_iterator<char>(file)), {});

	if (buffer.size() >= 3 &&
		(unsigned char)buffer[0] == 0xEF &&
		(unsigned char)buffer[1] == 0xBB &&
		(unsigned char)buffer[2] == 0xBF) {
		outContent.assign(buffer.begin() + 3, buffer.end());
	}
	else {
		outContent.assign(buffer.begin(), buffer.end());
	}
	return true;
}


bool writeStringToFile(const string& filePath, const string& content) {
	ofstream file(filePath, ios::binary);
	if (!file) return false;

	unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
	file.write(reinterpret_cast<char*>(bom), 3);
	file << content;
	return true;
}


void addSong() {
	string title, author, lyrics, line;
	int year;
	cout << "Enter the song title:";
	getline(cin, title);
	cout << "Enter the author of the text:";
	getline(cin, author);
	cout << "Enter year of creation (0 if unknown):";
	cin >> year;
	cin.ignore(numeric_limits<streamsize>::max(), '\n');

	cout << "Enter the lyrics (a blank line ends the entry):\n";
	while (true) {
		getline(cin, line);
		if (line.empty()) break;
		lyrics += line + "\n";
	}

	string filename = toFilename(title);
	string metaPath = catalogDir + filename + ".txt";
	string lyricsPath = lyricsDir + filename + "_log.txt";

	string metaContent = "Name:" + title + "\n"
		+ "Author:" + author + "\n"
		+ "Year:" + (year == 0 ? "Unknown" : to_string(year)) + "\n";

	if (!writeStringToFile(metaPath, metaContent)) {
		cerr << "Error creating metadata file.\n";
		pause();
		return;
	}
	if (!writeStringToFile(lyricsPath, lyrics)) {
		cerr << "Error creating text file.\n";
		pause();
		return;
	}

	setColor(COLOR_HIGHLIGH);
	cout << "Song successfully added!\n";
	setColor(COLOR_DEFAULT);
	pause();
}


void loadSong() {
	string title, author;
	int year;

	cout << "Enter song title: ";
	getline(cin, title);
	cout << "Enter the author of the text: ";
	getline(cin, author);
	cout << "Enter year of creation (0 if unknown): ";
	cin >> year;
	cin.ignore(numeric_limits<streamsize>::max(), '\n');


	OPENFILENAMEW ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = L"Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0";
	ofn.lpstrFile = new WCHAR[MAX_PATH];
	ofn.lpstrFile[0] = L'\0';
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.lpstrTitle = L"Select a file with song lyrics";


	if (GetOpenFileNameW(&ofn) != TRUE) {
		cout << "File not selected.\n";
		delete[] ofn.lpstrFile;
		return;
	}

	int size_needed = WideCharToMultiByte(CP_UTF8, 0, ofn.lpstrFile, -1, NULL, 0, NULL, NULL);
	string selectedPath(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, ofn.lpstrFile, -1, &selectedPath[0], size_needed, NULL, NULL);
	delete[] ofn.lpstrFile;

	if (!selectedPath.empty() && selectedPath.back() == '\0') {
		selectedPath.pop_back();
	}


	string lyricsContent;
	if (!readFileToString(selectedPath, lyricsContent)) {
		cerr << "Error reading text file.\n";
		return;
	}

	string filename = toFilename(title);
	string metaPath = catalogDir + filename + ".txt";
	string lyricsPath = lyricsDir + filename + "_log.txt";

	string metaContent = "Name: " + title + "\n"
		+ "Author: " + author + "\n"
		+ "Year: " + (year == 0 ? "Unknown" : to_string(year)) + "\n";

	if (!writeStringToFile(metaPath, metaContent)) {
		cerr << "Error creating metadata file.\n";
		pause();
		return;
	}
	if (!writeStringToFile(lyricsPath, lyricsContent)) {
		cerr << "Error creating text file.\n";
		pause();
		return;
	}

	setColor(COLOR_HIGHLIGH);
	cout << "Song successfully downloaded from file!\n";
	setColor(COLOR_DEFAULT);
	pause();
}


void listSongs(vector<string>& songs) {
	songs.clear();
	for (const auto& entry : fs::directory_iterator(catalogDir)) {
		if (entry.path().extension() == ".txt") {
			songs.push_back(entry.path().stem().string());
		}
	}
	if (songs.empty()) {
		cout << "The song catalog is empty.\n";
	}
	else {
		cout << "\nList of songs:\n";
		for (size_t i = 0; i < songs.size(); ++i) {
			cout << i + 1 << ") " << songs[i] << "\n";
		}
	}
}


void viewSong() {
	vector<string> songs;
	listSongs(songs);
	if (songs.empty()) return;
	cout << "\nEnter song number to view: ";
	int index; cin >> index;
	cin.ignore(numeric_limits<streamsize>::max(), '\n');
	if (index < 1 || index >(int)songs.size()) {
		cout << "Wrong choice.\n";
		return;
	}

	string filename = toFilename(songs[index - 1]);
	string metaPath = catalogDir + filename + ".txt";
	string lyricsPath = lyricsDir + filename + "_log.txt";

	cout << "\nMetadata:\n";
	string metaContent;
	if (readFileToString(metaPath, metaContent)) {
		cout << metaContent;
	}
	else {
		cout << "Failed to open metadata file.\n";
	}

	cout << "\nSong lyrics:\n";
	string lyricsContent;
	if (readFileToString(lyricsPath, lyricsContent)) {
		cout << lyricsContent;
	}
	else {
		cout << "Failed to open text file.\n";
	}
	pause();
}


void deleteSong() {
	vector<string> songs;
	listSongs(songs);
	if (songs.empty()) return;
	cout << "\nEnter the song number to delete: ";
	int index; cin >> index;
	cin.ignore(numeric_limits<streamsize>::max(), '\n');
	if (index < 1 || index >(int)songs.size()) {
		cout << "Wrong choice.\n";
		return;
	}
	string filename = toFilename(songs[index - 1]);
	fs::remove(catalogDir + filename + ".txt");
	fs::remove(lyricsDir + filename + "_log.txt");
	setColor(COLOR_HIGHLIGH);
	cout << "The song has been removed.\n";
	setColor(COLOR_DEFAULT);
	pause();
}


void searchByAuthor() {
	cout << "Enter the author's name: ";
	string author; getline(cin, author);
	bool found = false;
	for (const auto& entry : fs::directory_iterator(catalogDir)) {
		ifstream file(entry.path(), ios::binary);
		string line;
		while (getline(file, line)) {
			if (line.find("Author:") != string::npos && line.find(author) != string::npos) {
				cout << "Song found: " << entry.path().stem().string() << "\n";
				found = true;
				break;
			}
		}
	}
	if (!found) {
		cout << "No songs found by this author.\n";
	}
	pause();
}


void searchByWord() {
	cout << "Enter a word to search: ";
	string word; getline(cin, word);
	bool found = false;
	for (const auto& entry : fs::directory_iterator(lyricsDir)) {
		ifstream file(entry.path(), ios::binary);
		string content((istreambuf_iterator<char>(file)), {});
		if (content.find(word) != string::npos) {

			string stem = entry.path().stem().string();
			if (stem.size() > 4 && stem.substr(stem.size() - 4) == "_log") {
				stem = stem.substr(0, stem.size() - 4);
			}
			cout << "The word is found in the song:" << stem << "\n";
			found = true;
		}
	}
	if (!found) {
		cout << "No songs found with this word.\n";
	}
	pause();
}


void menu() {

	int choice;
	do {

#ifdef _WIN32
		system("cls");
#else
		system("clear");
#endif

		setColor(COLOR_TITLE);
		cout << "+================================+\n";
		cout << "I       Song Lyrics Catalog      I\n";
		cout << "+================================+\n";
		setColor(COLOR_DEFAULT);

		cout << "1. Add a song manually\n";
		cout << "2. Load song from file\n";
		cout << "3. View song\n";
		cout << "4. Delete song\n";
		cout << "5. Search by author\n";
		cout << "6. Search by word\n";
		cout << "0. Exit\n";
		cout << "Choice: ";
		cin >> choice; cin.ignore(numeric_limits<streamsize>::max(), '\n');

		switch (choice) {
		case 1: addSong(); break;
		case 2: loadSong(); break;
		case 3: viewSong(); break;
		case 4: deleteSong(); break;
		case 5: searchByAuthor(); break;
		case 6: searchByWord(); break;
		case 0: cout << "Exit...\n"; break;
		default:
			cout << "Неверный выбор.\n";
			pause();
		}
	} while (choice != 0);
}

int main() {

	setlocale(LC_ALL, "ru_RU.UTF-8");
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	createDirectories();
	menu();
	return 0;
}