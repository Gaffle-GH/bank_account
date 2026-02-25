#include <string>
#include <iostream>
#include <iomanip>
#include "class.h"

using namespace std;

int main(){
    string name;
    clearScreen();
    
    cout << "Account Program:" << endl;
    cout << "Insert Name for the Account: "; 
    cin >> name;

    account user(name, 0);

    if(user.loadFile()){
        cout << "File Found..." << endl;
        cout << "Loaded Existing Account Information." << endl << endl;
        cout << setfill('-') << setw(20) << "-" << endl;
    } else if (!user.loadFile()){
        cout << "ERROR: No Existing Information found" << endl;
        cout << "Creating New Account..." << endl << endl;
        cout << setfill('-') << setw(20) << "-" << endl;
    }
    user.display();
    return 0;
}