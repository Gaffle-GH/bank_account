#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

class account
{
public:
    account(string name, double currentbal);

    // --- Core API (shared by console and GUI) ---
    string getName() const;
    double getBalance() const;
    const vector<string>& getHistory() const;

    // Folder where the account file is read from / written to.
    // Empty means the current working directory.
    void setDataDir(const string& dir);
    string getDataDir() const;
    string getFilePath() const;

    bool deposit(double amount);
    bool withdraw(double amount, string& errorMsg);

    bool saveFile();
    bool loadFile();

    // --- Console-only UI loop ---
    void display();
    void option(int optioninput);
    void depositPrompt();
    void withdrawPrompt();
    void bankHistory();

private:
    string name;
    double currentbal;
    vector<string> history;
    string dataDir;
};

void clearScreen();
string trim(const string& str);
