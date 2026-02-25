#include <iomanip>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

using namespace std;

class account
{
public:
    account(string name, double currentbal);

    void display();
    void option(int optioninput);
    void deposit();
    void withdraw();
    void bankHistory();

    void saveFile();
    bool loadFile();

private:
    string name;
    double currentbal;
    vector<string> history;
};

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

account::account(string name, double currentbal) : name(name), currentbal(currentbal) {}

void account::saveFile()
{
    ofstream file(name + "_accountINFOCARD.txt");
    if (file.is_open())
    {
        file << "Account Info: " << endl;
        file << setfill('-') << setw(20) << "-" << endl;
        file << "Account Name: " << name << endl;
        file << "Current Balance: $" << currentbal << fixed << setprecision(2) << endl;
        file << endl;
        file << "Action History: " << endl;

        int start = 0;

        int end = (int)history.size();

        // Display entries
        for (int i = start; i < end; ++i)
        {
            if (!history[i].empty())
            {
                file << history[i] << endl;
            }
        }

        file << endl;
        file << "##END OF TRANSCRIPT##" << endl;
        file.close();

        cout << "Account Information Saved to " << name << "_accountINFOCARD.txt" << endl;
    }
    else
    {
        cout << "Error: Unable to Save File" << endl;
    }
}

bool account::loadFile()
{
    ifstream file(name + "_accountINFOCARD.txt");
    if (file.is_open())
    {
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
    }
    else
    {
        return false;
    }
    return true;
};

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

    return option(optioninput);
}

void account::bankHistory()
{
    clearScreen();

    const int pageSize = 10;
    int start = 0; // first index of current page
    char input;

    while (true)
    {
        clearScreen();
        cout << "Account History Selected: " << endl;
        cout << "Action History (Displaying: MAX 10)\n\n";

        int end = min(start + pageSize, (int)history.size());

        // Display entries
        for (int i = start; i < end; ++i)
        {
            if (!history[i].empty())
            {
                cout << history[i] << endl;
            }
        }

        // Page info
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
        cout << "Deposit Selected: " << endl
             << endl;
        deposit();
        break;
    case 2:
        clearScreen();
        cout << "Withdraw Selected: " << endl
             << endl;
        withdraw();
        break;
    case 3:
        clearScreen();
        cout << "History Selected: " << endl
             << endl;
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
        cout << "Shutting Down.." << endl
             << endl;
        break;
    }
}

void account::deposit()
{
    double deposit = 0;

    cout << "Current Balance: $" << currentbal << setprecision(2) << fixed << endl;
    cout << "Enter Deposit Amount: $";
    cin >> deposit;
    cout << endl;

    currentbal = currentbal + deposit;
    cout << setfill('-') << setw(20) << "-" << endl;

    stringstream ss;
    ss << fixed << setprecision(2) << "Deposit: +$" << deposit;
    history.push_back(ss.str());
    cout << "Deposit successful!" << endl;
    display();
}

void account::withdraw()
{
    double withdraw = 0;

    cout << "Current Balance: $" << currentbal << setprecision(2) << fixed << endl;
    cout << "Enter Withdraw Amount: $";
    cin >> withdraw;
    cout << endl;

    currentbal = currentbal - withdraw;
    cout << setfill('-') << setw(20) << "-" << endl;

    if (currentbal < 0)
    {
        currentbal = 0;
        cout << endl;
        cout << "ERROR: Insufficient Funds" << endl
             << endl;
        cout << setfill('-') << setw(20) << "-" << endl
             << endl;
        exit(0);
    }

    stringstream ss;
    ss << fixed << setprecision(2) << "Withdraw: -$" << withdraw;
    history.push_back(ss.str());
    display();
}
