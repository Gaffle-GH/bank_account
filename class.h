#include <iomanip>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

using namespace std;

class account{
    public:
    account(string name, double currentbal);
    
    void display();
    void option(int optioninput);
    void deposit();
    void withdraw();

    void saveFile();
    bool loadFile();

    private:
    string name;
    double currentbal;
    vector<string> history;
};

account::account(string name, double currentbal) : name(name), currentbal(currentbal){}

void account::saveFile(){
    ofstream file(name + "_accountINFOCARD.txt");
    if (file.is_open()){
        file << "Account Info: " << endl;
        file << setfill('-') << setw(20) << "-" << endl;
        file << "Account Name: " << name << endl;
        file << "Current Balance: $" << currentbal << fixed << setprecision(2) << endl;
        file << endl;
        file << "Action History: " << endl;

        for (size_t i = 0; i < history.size(); i++){
            file << history[i] << endl;
        } 

        file << "##END OF TRANSCRIPT##" << endl;
        file.close();

        cout << "Account Information Saved to " << name << "_accountINFOCARD.txt" << endl;
        cout << endl;
        cout << "Shutting Down..." << endl << endl;

    }else{
        cout << "Error: Unable to Save File" << endl;

    }
}

bool account::loadFile(){
    ifstream file(name + "_accountINFOCARD.txt");
    if(file.is_open()){
        string line;
        getline(file, line);
        getline(file, line);
        getline(file, line);
        getline(file, line);
        currentbal = stod(line.substr(line.find("$") + 1));
        getline(file, line);
        getline(file, line);
        history.clear();
        while(getline(file, line)){
            if (line == "##END OF TRANSCRIPT##"){
                break;
            }
            history.push_back(line);
        }
        file.close();
    } else {
        return false;
    }
    return true;
};

void account::display(){
    int optioninput;
    cout << endl;
    cout << "Account Display for " << name << ": " << endl;
    cout << endl;
    cout << "Current Balance: $" << setprecision(2) << fixed << currentbal << endl;
    cout << endl;

    cout << "Options: " << endl;
    cout << "1. Deposit" << endl;
    cout << "2. Withdraw" << endl;
    cout << "3. Save and Exit" << endl;
    cout << endl;

    cout << "Enter Option: ";
    cin >> optioninput;
    cout << endl;

    return option(optioninput);
}

void account::option(int optioninput){
    switch (optioninput){
    case 1:
        cout << setfill('-') << setw(20) << "-" << endl << endl;
        cout << "Deposit Selected: " << endl << endl;
        deposit();
        break;
    case 2:
        cout << setfill('-') << setw(20) << "-" << endl << endl;
        cout << "Withdraw Selected: " << endl << endl;
        withdraw();
        break;
    case 3:
        cout << setfill('-') << setw(20) << "-" << endl << endl;
        cout << "Saving..." << endl;
        saveFile();
        break;
    default:
        cout << setfill('-') << setw(20) << "-" << endl << endl;
        cout << "Error: Invalid Option" << endl;
        cout << "Shutting Down.." << endl << endl;
        break;
    }
}

void account::deposit(){
    double deposit = 0;
    stringstream ss;

    cout << "Current Balance: $" << currentbal << setprecision(2) << fixed << endl;
    cout << "Enter Deposit Amount: $"; 
    cin >> deposit;
    cout << endl;  

    currentbal = currentbal + deposit;
    cout << setfill('-') << setw(20) << "-" << endl;

    ss << fixed << setprecision(2) << "Deposit: +$" << deposit;
    history.push_back(ss.str());
    display(); 
}

void account::withdraw(){
    double withdraw = 0;
    stringstream ss;

    cout << "Current Balance: $" << currentbal << setprecision(2) << fixed << endl;
    cout << "Enter Withdraw Amount: $";
    cin >> withdraw;
    cout << endl;

    currentbal = currentbal - withdraw;
    cout << setfill('-') << setw(20) << "-" << endl;

    if (currentbal < 0){
        currentbal = 0;
        cout << endl;
        cout << "ERROR: Insufficient Funds" << endl << endl;
        ss << fixed << "ERROR: Withdrawn Insufficient Funds";
        cout << setfill('-') << setw(20) << "-" << endl << endl;
        history.push_back(ss.str());
        saveFile();
        return;
    } 


    ss << fixed << setprecision(2) << "Withdraw: -$" << withdraw;
    history.push_back(ss.str());
    display();
}
