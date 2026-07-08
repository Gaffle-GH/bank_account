#include "class.h"

#include <algorithm>
#include <filesystem>

account::account(string name, double currentbal) : name(name), currentbal(currentbal) {}

string account::getName() const { return name; }

double account::getBalance() const { return currentbal; }

const vector<string>& account::getHistory() const { return history; }

void account::setDataDir(const string& dir) { dataDir = dir; }

string account::getDataDir() const { return dataDir; }

string account::getFilePath() const
{
    const string fileName = name + "_accountINFOCARD.txt";
    if (dataDir.empty())
    {
        return fileName;
    }
    return (std::filesystem::path(dataDir) / fileName).string();
}

void clearScreen()
{
    cout << "\033[2J\033[1;1H";
}

string trim(const string& str)
{
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

bool account::saveFile()
{
    if (!dataDir.empty())
    {
        std::error_code ec;
        std::filesystem::create_directories(dataDir, ec);
    }

    const string path = getFilePath();
    ofstream file(path);
    if (!file.is_open())
    {
        cout << "Error: Unable to Save File" << endl;
        return false;
    }

    file << "Account Info: " << endl;
    file << setfill('-') << setw(20) << "-" << endl;
    file << "Account Name: " << name << endl;
    file << "Current Balance: $" << currentbal << fixed << setprecision(2) << endl;
    file << endl;
    file << "Action History: " << endl;

    for (const string& entry : history)
    {
        if (!entry.empty())
        {
            file << entry << endl;
        }
    }

    file << endl;
    file << "##END OF TRANSCRIPT##" << endl;
    file.close();

    cout << "Account Information Saved to " << path << endl;
    return true;
}

bool account::loadFile()
{
    ifstream file(getFilePath());
    if (!file.is_open())
    {
        return false;
    }

    string line;
    getline(file, line);
    getline(file, line);
    getline(file, line);
    getline(file, line);
    currentbal = stod(line.substr(line.find("$") + 1));
    getline(file, line);
    getline(file, line);
    history.clear();
    while (getline(file, line))
    {
        if (line == "##END OF TRANSCRIPT##")
        {
            break;
        }
        string trimmedLine = trim(line);
        if (!trimmedLine.empty())
        {
            history.push_back(trimmedLine);
        }
    }
    file.close();
    return true;
}

bool account::deposit(double amount)
{
    if (amount <= 0)
    {
        return false;
    }

    currentbal += amount;

    stringstream ss;
    ss << fixed << setprecision(2) << "Deposit: +$" << amount;
    history.push_back(ss.str());
    return true;
}

bool account::withdraw(double amount, string& errorMsg)
{
    if (amount <= 0)
    {
        errorMsg = "Amount must be greater than zero";
        return false;
    }

    if (amount > currentbal)
    {
        errorMsg = "Insufficient funds";
        return false;
    }

    currentbal -= amount;

    stringstream ss;
    ss << fixed << setprecision(2) << "Withdraw: -$" << amount;
    history.push_back(ss.str());
    return true;
}

void account::display()
{
    int optioninput;
    clearScreen();

    cout << "Account Display for " << name << ": " << endl;
    cout << endl;
    cout << "Current Balance: $" << setprecision(2) << fixed << currentbal << endl;
    cout << endl;

    cout << "Options: " << endl;
    cout << "1. Deposit" << endl;
    cout << "2. Withdraw" << endl;
    cout << "3. History" << endl;
    cout << "4. Save and Exit" << endl;
    cout << endl;

    cout << "Enter Option: ";
    cin >> optioninput;
    cout << endl;

    option(optioninput);
}

void account::bankHistory()
{
    const int pageSize = 10;
    int start = 0;
    char input;

    while (true)
    {
        clearScreen();
        cout << "Account History Selected: " << endl;
        cout << "Action History (Displaying: MAX 10)\n\n";

        int end = min(start + pageSize, (int)history.size());

        for (int i = start; i < end; ++i)
        {
            if (!history[i].empty())
            {
                cout << history[i] << endl;
            }
        }

        cout << "\nDisplaying [" << start + 1 << "-" << end << "] of " << history.size() << endl;

        cout << "[Next Page] (N) | [Previous Page] (P) | [Exit] (E): ";
        cin >> input;

        if (input == 'n' || input == 'N')
        {
            if (end >= (int)history.size())
            {
                cout << "\nNo more history.\n";
                cin.ignore();
                cin.get();
            }
            else
            {
                start += pageSize;
            }
        }
        else if (input == 'p' || input == 'P')
        {
            if (start == 0)
            {
                cin.ignore();
                cin.get();
            }
            else
            {
                start -= pageSize;
            }
        }
        else if (input == 'e' || input == 'E')
        {
            display();
            return;
        }
        else
        {
            cout << "\nInvalid option.\n";
            cin.ignore();
            cin.get();
        }
    }
}

void account::option(int optioninput)
{
    switch (optioninput)
    {
    case 1:
        clearScreen();
        cout << "Deposit Selected: " << endl << endl;
        depositPrompt();
        break;
    case 2:
        clearScreen();
        cout << "Withdraw Selected: " << endl << endl;
        withdrawPrompt();
        break;
    case 3:
        clearScreen();
        cout << "History Selected: " << endl << endl;
        bankHistory();
        break;
    case 4:
        clearScreen();
        cout << "Saving..." << endl;
        saveFile();
        break;
    default:
        clearScreen();
        cout << "Error: Invalid Option" << endl;
        cout << "Shutting Down.." << endl << endl;
        break;
    }
}

void account::depositPrompt()
{
    double amount = 0;

    cout << "Current Balance: $" << currentbal << setprecision(2) << fixed << endl;
    cout << "Enter Deposit Amount: $";
    cin >> amount;
    cout << endl;

    if (deposit(amount))
    {
        cout << setfill('-') << setw(20) << "-" << endl;
        cout << "Deposit successful!" << endl;
    }
    else
    {
        cout << "ERROR: Invalid deposit amount" << endl;
    }

    display();
}

void account::withdrawPrompt()
{
    double amount = 0;

    cout << "Current Balance: $" << currentbal << setprecision(2) << fixed << endl;
    cout << "Enter Withdraw Amount: $";
    cin >> amount;
    cout << endl;

    string errorMsg;
    if (withdraw(amount, errorMsg))
    {
        cout << setfill('-') << setw(20) << "-" << endl;
        cout << "Withdrawal successful!" << endl;
    }
    else
    {
        cout << endl;
        cout << "ERROR: " << errorMsg << endl << endl;
        cout << setfill('-') << setw(20) << "-" << endl << endl;
    }

    display();
}
