#include <iostream>
#include "sqlite3.h"
#include <ctime>
#include <iomanip>
#include <chrono>
#include <string>

// Menu constants
const char MENU_WRITE = 'W';
const char MENU_DELETE = 'D';
const char MENU_READ = 'R';
const char MENU_QUIT = 'Q';
const char MENU_EXIT = 'E';

// Other constants
const std::string DATABASE_NAME = "diarydatabase.db";
const std::string DATE_FORMAT = "%Y-%m-%d";
const std::string ERROR_LOCAL_TIME = "Failed to get local time";
const std::string ERROR_STRFTIME = "strftime failed";
const std::string ERROR_SQL = "SQL error: ";
const std::string ERROR_DATABASE = "Can't open database: ";
const std::string ERROR_DELETE_FAILED = "Delete failed: ";
const std::string SUCCESS_ENTRY_DELETED = "Diary entry deleted successfully.";
const std::string SUCCESS_ENTRY_ADDED = "Diary entry added successfully.";
const std::string ENTRY_EXISTS = "Entry already exists.";
const std::string NO_ENTRY_FOUND = "No entry found for the specified date.";
const std::string ENTRY_DOES_NOT_EXIST = "Diary entry does not exist.";

// Function to get the current date in ISO format
std::string getCurrentDate() 
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t in_time_t = std::chrono::system_clock::to_time_t(now);
    struct tm time_info;

    if (localtime_s(&time_info, &in_time_t) != 0)
    {
        throw std::runtime_error(ERROR_LOCAL_TIME);
    }

    char date[11];
    if (std::strftime(date, sizeof(date), DATE_FORMAT.c_str(), &time_info) == 0)
    {
        throw std::runtime_error(ERROR_STRFTIME);
    }

    return date;
}

// Function for checking if entry exists for a date
bool doesEntryExist(sqlite3* db, const std::string& date) 
{
    const char* checkEntrySQL = "SELECT COUNT(*) FROM Diary WHERE Date = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, checkEntrySQL, -1, &stmt, 0) == SQLITE_OK) 
    {
        sqlite3_bind_text(stmt, 1, date.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) 
        {
            int count = sqlite3_column_int(stmt, 0);
            return count > 0;
        }

        sqlite3_finalize(stmt);
    }

    return false;
}


// Function for printing all entry dates
void printAllEntryDates(sqlite3* db) 
{
    const char* selectDatesSQL = "SELECT Date FROM Diary;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, selectDatesSQL, -1, &stmt, 0) == SQLITE_OK) 
    {
        std::cout << "Dates of Diary Entries:\n";

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const char* date = (const char*)sqlite3_column_text(stmt, 0);
            std::cout << date << "\n";
        }

        sqlite3_finalize(stmt);
    }

    std::cout << std::endl;
}

// Function for deleting entry for specific date
void deleteDiaryEntry(sqlite3* db, const std::string& date)
{
    if (doesEntryExist(db, date))
    {
        const char* deleteSQL = "DELETE FROM Diary WHERE Date = ?;";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, deleteSQL, -1, &stmt, 0) == SQLITE_OK) 
        {
            sqlite3_bind_text(stmt, 1, date.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) != SQLITE_DONE) 
            {
                std::cerr << ERROR_DELETE_FAILED << sqlite3_errmsg(db) << std::endl;
                return;
            }

            sqlite3_finalize(stmt);
        }
        std::cout << SUCCESS_ENTRY_DELETED << std::endl;
    }

    else 
    {
        std::cout << ENTRY_DOES_NOT_EXIST << std::endl;
    }
}


// Function for reading a specific entry
void viewDiaryEntry(sqlite3* db, const std::string& date) 
{
    const char* selectSQL = "SELECT Entry FROM Diary WHERE Date = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, selectSQL, -1, &stmt, 0) == SQLITE_OK) 
    {
        sqlite3_bind_text(stmt, 1, date.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* entry = (const char*)sqlite3_column_text(stmt, 0);
            std::cout << "Date: " << date << "\nEntry: " << entry << "\n";
        }

        else 
        {
            std::cout << NO_ENTRY_FOUND << std::endl;
        }

        sqlite3_finalize(stmt);
    }
}

// Functin for adding a new diary entry to database
void addDiaryEntry(sqlite3* db, const std::string& date, std::string& entry)
{
    const char* insertSQL = "INSERT INTO Diary (Date, Entry) VALUES (?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, insertSQL, -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, date.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, entry.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) 
        {
            std::cerr << "Insert failed: " << sqlite3_errmsg(db) << std::endl;
        }

        sqlite3_finalize(stmt);
    }
    std::cout << SUCCESS_ENTRY_ADDED << std::endl;
}



int main()
{
    // Welcome text
    std::cout << "Diary++" << std::endl << std::endl;

    // Database file open
    sqlite3* db;
    int rc = sqlite3_open(DATABASE_NAME.c_str(), &db);

    if (rc) {
        std::cerr << ERROR_DATABASE << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return 1;
    }
    // Create table for diary entries if does not exist
    std::string createTable = "CREATE TABLE IF NOT EXISTS Diary (Date TEXT PRIMARY KEY, Entry TEXT);";
    rc = sqlite3_exec(db, createTable.c_str(), 0, 0, 0);

    if (rc != SQLITE_OK) {
        std::cerr << ERROR_SQL << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return 1;
    }

    // boolean for exiting application
    bool quit = false;

    // Main diary loop
    while (!quit)
    {
        char menu = 'l';

        // Main menu, user can write entry for today, delete old entries, read old entries or quit
        while (menu != MENU_WRITE && menu != MENU_READ && menu != MENU_DELETE && menu != MENU_QUIT)
        {
            std::string input;
            std::cout << "Write entry (" << MENU_WRITE <<"), delete entry (" << MENU_DELETE << "), read entries (" << MENU_READ <<") or quit (" << MENU_QUIT <<")?!\n";
            std::cin >> input;
            menu = toupper(input[0]);
            std::cout << std::endl;
        }

        // Writing new entry
        if (menu == MENU_WRITE)
        {
            std::string date = getCurrentDate();

            if (doesEntryExist(db, date)) 
            {
                std::cout << "Entry already exists. " << std::endl << std::endl;
                continue;
            }

            std::cout << "Enter your diary entry (and save with ENTER): " << std::endl;
            std::string entry;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::getline(std::cin, entry);
            addDiaryEntry(db, date, entry);
        }

        // Deleting old entry
        else if (menu == MENU_DELETE)
        {
            printAllEntryDates(db);
            std::cout << "Write date of entry to delete (YYYY-MM-DD) or exit (" << MENU_EXIT <<")!\n";
            std::string date;
            std::cin >> date;

            if (date != "" + MENU_EXIT)
            {
                deleteDiaryEntry(db, date);
            }
        }

        // Read old entry
        else if (menu == MENU_READ)
        {
            printAllEntryDates(db);
            std::cout << "Write date of entry to read (YYYY-MM-DD) or exit (" << MENU_EXIT <<")!\n";
            std::string date;
            std::cin >> date;

            if (date !=  "" + MENU_EXIT)
            {
                viewDiaryEntry(db, date);
            }
        }

        // Quitting
        else if (menu == MENU_QUIT)
        {
            quit = true;
        }

        std::cout << std::endl;
    }

    sqlite3_close(db);
    return 0;
}