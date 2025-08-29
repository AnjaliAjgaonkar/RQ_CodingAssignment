#include<iostream>
#include<windows.h>
#include <string>
#include <cctype> 
using namespace std;

#define MYPIPE L"\\\\.\\pipe\\RQ_testPipe"

struct Database
{
    DWORD procID;
    int operation;
    int number;
};

struct OutputData
{
    int number;
    LONGLONG timestamp;
};

//Usage
void printUsage()
{
    cout << "\n\nSelect an option from following menu:";
    cout << "\n1. Insert a number";
    cout << "\n2. Delete a number";
    cout << "\n3. Print all numbers";
    cout << "\n4. Delete all numbers";
    cout << "\n5. Find a number";
    cout << "\n6. Exit";
}

//Insert number in store
bool Insert(DWORD dwProcID, int choice, int num)
{
    //Connect to server
    HANDLE hPipe = CreateFile(
        MYPIPE,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if (hPipe == INVALID_HANDLE_VALUE) {
        cout << "\nINSERT: Failed to connect to server. Error: " << GetLastError();
        return false;
    }

    Database dobj{};
    dobj.procID = dwProcID;
    dobj.operation = choice;
    dobj.number = num;

    BOOL bRet;
    DWORD dwBytesWritten = 0;

    //Sending procID, user selected option, number to be stored to the server
    //cout << "\nINSERT: Writing to pipe..";
    bRet = WriteFile(
        hPipe,
        (LPCVOID)&dobj,
        sizeof(dobj),
        &dwBytesWritten,
        NULL
    );
    if (bRet)
    {
        //cout << "\nINSERT: Number of bytes sent: " << dwBytesWritten;
    }
    else
    {
        cout << "\nINSERT: Failed to send data to server. Error: " << GetLastError();
        CloseHandle(hPipe);
        return false;
    }

    OutputData output{};
    DWORD dwBytesRead = 0;

    //Get inserted number details (timestamp) from server
    //cout << "\nINSERT: Reading from pipe..";
    bRet = ReadFile(
        hPipe,
        (LPVOID)&output,
        sizeof(output),
        &dwBytesRead,
        NULL
    );
    if (bRet)
    {
        //Print inserted number details
        cout << "\nInputted number details from server :";
        cout << "\nNumber: " << output.number;
        cout << "\nTimestamp: " << output.timestamp;
    }
    else
    {
        cout << "\nINSERT: Failed to get inserted number details from server. Error: " << GetLastError();
        CloseHandle(hPipe);
        return false;
    }
        
    CloseHandle(hPipe);
    return true;
  }

//Delete single number from store
bool Delete(DWORD dwProcID, int choice, int num)
{
    //Connect to server
    HANDLE hPipe = CreateFile(
        MYPIPE,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        cout << "\nDELETE: Failed to connect to server. Error: " << GetLastError();
        return false;
    }

    Database dobj{};
    dobj.procID = dwProcID;
    dobj.operation = choice;
    dobj.number = num;

    BOOL bRet;
    DWORD dwBytesWritten = 0;

    //Sending procID, user selected option, number to be deleted from the server
    //cout << "\nDELETE: Writing to pipe..";
    bRet = WriteFile(
        hPipe,
        (LPCVOID)&dobj,
        sizeof(dobj),
        &dwBytesWritten,
        NULL
    );
    if (bRet)
    {
        //cout << "\nDELETE: Number of bytes sent: " << dwBytesWritten;
    }
    else
    {
        cout << "\nDELETE: Failed to send data to server. Error: " << GetLastError();
        CloseHandle(hPipe);
        return false;
    }

    //Get timestamp of input number from server
    OutputData output{};
    DWORD dwBytesRead = 0;

    //cout << "\nDELETE: Reading from pipe..";
    bRet = ReadFile(
        hPipe,
        (LPVOID)&output,
        sizeof(output),
        &dwBytesRead,
        NULL
    );
    if (bRet)
    {
        //Check for dummy number
        //If dummy number found, it means number not present in store
        if (output.number != -1)
        {
            //Print input number details from server before deletion
            cout << "\nNumber details to be deleted from server :";
            cout << "\nNumber: " << output.number;
            cout << "\nTimestamp: " << output.timestamp;
        }
        else
            cout << "\nNumber not found in store";
    }
    else
    {
        cout << "\nDELETE: Failed to get deleted number details from server. Error: " << GetLastError();
        CloseHandle(hPipe);
        return false;
    }
        
    CloseHandle(hPipe);
    return true;
}

//Print elements from store
bool Print(DWORD dwProcID, int choice)
{
    //Connect to server
    HANDLE hPipe = CreateFile(
        MYPIPE,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        cout << "\nPRINT: Failed to connect to server. Error: " << GetLastError();
        return false;
    }

    Database dobj;
    dobj.procID = dwProcID;
    dobj.operation = choice;
    dobj.number = -1; //Blank

    BOOL bRet;
    DWORD dwBytesWritten = 0;

    //Sending procID, user selected option to the server
    //cout << "\nPRINT: Writing to pipe..";
    bRet = WriteFile(
        hPipe,
        (LPCVOID)&dobj,
        sizeof(dobj),
        &dwBytesWritten,
        NULL
    );
    if (bRet)
    {
        //cout << "\nPRINT: Number of bytes sent: " << dwBytesWritten;
    }
    else
    {
        cout << "\nPRINT: Failed to send data. Error: " << GetLastError();
        CloseHandle(hPipe);
        return false;
    }
        
    DWORD dwBytesRead = 0;
    int size = 0;

    //Get total number of elements from server
    //cout << "\nPRINT: Reading from pipe..";
    bRet = ReadFile(
        hPipe,
        (LPVOID)&size,
        sizeof(int),
        &dwBytesRead,
        NULL
    );
    if (bRet)
    {
        //cout << "\nPRINT: Number of bytes read: " << dwBytesRead;
        cout << "\nPRINT: Total number of elements in store: " << size;

        pair<int, LONGLONG> numPair;
        for (int i = 0; i < size; i++)
        {
            //Get number and timestamp from server
            bRet = ReadFile(
                hPipe,
                (LPVOID)&numPair,
                sizeof(numPair),
                &dwBytesRead,
                NULL
            );
            if (bRet)
            {
                cout << "\nNumber: " << numPair.first << " TimeStamp: " << numPair.second;
            }
            else
            {
                cout << "\nPRINT: Failed to get number and timestamp from server. Error: " << GetLastError();
                CloseHandle(hPipe);
                return false;
            }
        }
    }
    else
    {
        cout << "\nPRINT: Failed to get total number of elements. Error: " << GetLastError();
        CloseHandle(hPipe);
        return false;
    }
        
    CloseHandle(hPipe);
    return true;
}

//Delete all from store
bool DeleteAll(DWORD dwProcID, int choice)
{
    //Connect to server
    HANDLE hPipe = CreateFile(
        MYPIPE,
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        cout << "\nDELETEALL: Failed to connect to server. Error: " << GetLastError();
        return false;
    }

    Database dobj{};
    dobj.procID = dwProcID;
    dobj.operation = choice;
    dobj.number = -1; //Dummy value

    BOOL bRet;
    DWORD dwBytesWritten = 0;

    //cout << "\nDELETEALL: Writing to pipe..";
    bRet = WriteFile(
        hPipe,
        (LPCVOID)&dobj,
        sizeof(dobj),
        &dwBytesWritten,
        NULL
    );
    if (bRet)
    {
        //cout << "\nDELETEALL: Number of bytes sent: " << dwBytesWritten;
    }
    else
    {
        cout << "\nDELETEALL: Failed to send data to server. Error: " << GetLastError();
        CloseHandle(hPipe);
        return false;
    }

    CloseHandle(hPipe);
    return true;
}

//Find number from store
bool Find(DWORD dwProcID, int choice, int num, bool *res)
{
    //Connect to server
    HANDLE hPipe = CreateFile(
        MYPIPE,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        cout << "\nFIND: Failed to connect to server. Error: " << GetLastError();
        return false;
    }

    Database dobj;
    dobj.procID = dwProcID;
    dobj.operation = choice;
    dobj.number = num;

    BOOL bRet;
    DWORD dwBytesWritten = 0;

    //Sending procID, user selected option, number to be searched to the server
    //cout << "\nFIND: Writing to pipe..";
    bRet = WriteFile(
        hPipe,
        (LPCVOID)&dobj,
        sizeof(dobj),
        &dwBytesWritten,
        NULL
    );
    if (bRet)
    {
        //cout << "\nFIND: Number of bytes sent: " << dwBytesWritten;
    }
    else
    {
        cout << "\nFIND: Failed to send data to server. Error: " << GetLastError();
        CloseHandle(hPipe);
        return false;
    }
        
    bool result = false;
    DWORD dwBytesRead = 0;
    
    //Check if value is present or not from server 
    //cout << "\nFIND: Reading from pipe..";
    bRet = ReadFile(
        hPipe,
        (LPVOID)&result,
        sizeof(result),
        &dwBytesRead,
        NULL
    );
    if (bRet)
    {
        *res = result; //store result
        //cout << "\nFIND: Number of bytes read: " << dwBytesRead;
    }
    else
    {
        cout << "\nFIND: Failed to read data from server. Error: " << GetLastError();
        CloseHandle(hPipe);
        return false;
    }

    CloseHandle(hPipe);
    return true;
}

//Function to check if input number is positive and numeric
bool IsValidInput(const string& input)
{
    if (input.empty())
        return false;

    //check if every char in input string is digit or not
    for (char ch : input)
    {
        if(!isdigit(static_cast<unsigned char>(ch)))
            return false;
    }

    //check for positive integer
    int num = stoi(input);
    if (num < 0)
        return false;

    return true;
}

//server main
int main()
{
    int choice;
    int num;
    string input;
    bool ret = false;
    DWORD dwProcID = GetCurrentProcessId();

    while (true)
    {
        printUsage();

        cout << "\nEnter your choice: ";
        cin >> input;

        if (!IsValidInput(input))
        {
            cout << "\nInvalid input. Please select valid option.";
            continue;
        }

        choice = stoi(input);

        bool result;
        switch (choice)
        {

        //Insert number in store
        case 1:
            cout << "\nEnter number to be inserted: ";
            cin >> input;

            if (!IsValidInput(input))
            {
                cout << "\nInvalid input. Please enter a positive integer: ";
                break;
            }

            num = stoi(input);
            ret = Insert(dwProcID, choice, num);
            if (ret == false)
                cout << "\nInsert function failed..";
            //else
            //    cout << "\nInsert function success..";

            break;

        //Delete number from store
        case 2:
            cout << "\nEnter number to be deleted: ";
            cin >> input;

            if (!IsValidInput(input))
            {
                cout << "\nInvalid input. Please enter a positive integer: ";
                break;
            }

            num = stoi(input);
            ret = Delete(dwProcID, choice, num);
            if (ret == false)
            {
                cout << "\nDelete function failed..";
            }     
            //else
            //   cout << "\nDelete function success..";

            break;

        //Print all numbers from store
        case 3:
            cout << "\nPrinting all numbers";

            ret = Print(dwProcID, choice);
            if (ret == false)
                cout << "\nPrint function failed..";
            //else
            //    cout << "\nPrint function success..";

            break;

        //Delete all numbers from store
        case 4:
            cout << "\nDeleting all numbers from store.. ";

            ret = DeleteAll(dwProcID, choice);
            if (ret == false)
                cout << "\nDelete All function failed..";
            //else
            //    cout << "\nDelete All function success..";

            break;

        case 5:
            cout << "\nEnter number to check in map: ";
            cin >> input;

            if (!IsValidInput(input))
            {
                cout << "\nInvalid input. Please enter a positive integer: ";
                break;
            }

            num = stoi(input);
            ret = Find(dwProcID, choice, num, &result);
            if (ret == false)
                cout << "\nFind function failed..";
            else
            {
                //cout << "\nFind function success..";

                if (result == true)
                    cout << "\n" <<num << " found in store.\n";
                else
                    cout << "\n" << num << " not found in store.\n";
            }
                
            break;

        case 6:
            cout << "\nExiting..";
            return 0;

        default:
            cout << "\nInvalid option.";
            break;
        }
    }

	return 0;
}