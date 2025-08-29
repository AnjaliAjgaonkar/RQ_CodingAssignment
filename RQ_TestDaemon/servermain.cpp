#include<windows.h>
#include<iostream>
#include<map>
#include<unordered_map>
#include<mutex>
using namespace std;

#define MYPIPE L"\\\\.\\pipe\\RQ_testPipe"

//numbers are stored in map according to process ID
unordered_map<DWORD, map<int, LONGLONG>> numStore;

//mutex to synchronize access on numStore and prevent data races 
mutex numStoreMtx;

struct Database
{
	DWORD procID;	//client process ID
	int operation;	//user selected operation choice
	int number;		//input number
};

struct OutputData
{
	int number;
	LONGLONG timestamp;
};

//Convert systemtime to unix time
LONGLONG GetSystemTimeAsUnixTime()
{
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);

	// Convert FILETIME to LONGLONG (100-nanosecond intervals since Jan 1, 1601)
	LONGLONG ll = static_cast<LONGLONG>(ft.dwLowDateTime) | (static_cast<LONGLONG>(ft.dwHighDateTime) << 32);

	// Subtract the 100-nanosecond intervals between Jan 1, 1601 and Jan 1, 1970
	// (116444736000000000LL is the magic number for this difference)
	ll -= 116444736000000000LL;

	// Convert to seconds by dividing by 10,000,000 (100-nanosecond intervals per second)
	LONGLONG unixTime = ll / 10000000LL;

	return unixTime;
}

//Insert number in map
bool Insert(Database dobj, HANDLE hPipe)
{
	LONGLONG unixTime = GetSystemTimeAsUnixTime();

	std::lock_guard<std::mutex> lock(numStoreMtx);
	numStore[dobj.procID][dobj.number] = unixTime;

	cout << "\nINSERT: Successfully inserted " << dobj.number << " in store.";

	//check if current client ID exists in map
	auto clientIDItr = numStore.find(dobj.procID);
	if (clientIDItr != numStore.end())
	{
		//get map for our client ID
		auto& ClientIDMap = clientIDItr->second;

		auto InputNumItr = ClientIDMap.find(dobj.number);
		if (InputNumItr != ClientIDMap.end())
		{
			cout << "\nInputted number: "<< InputNumItr->first;
			cout << "\nTimestamp: " << InputNumItr->second;

			OutputData output{};
			output.number = InputNumItr->first;
			output.timestamp = InputNumItr->second;

			BOOL bRet;
			DWORD dwBytesWritten;

			//Send number and timestamp to client
			bRet = WriteFile(
				hPipe,
				(LPVOID)&output,
				sizeof(output),
				&dwBytesWritten,
				NULL
			);
			if (bRet)
				cout << "\nINSERT: Number of bytes written: " << dwBytesWritten;
			else
			{
				cout << "\nINSERT: Failed to write data. Error: " << GetLastError();
				return false;
			}
		}
		else
		{
			cout << "\n " << dobj.number << " not inserted in map.";
			return false;
		}
	}
	else
	{
		cout << "\nINSERT: No entry found for Client process ID: " << dobj.procID;
		return false;
	}

	return true;
}

//Delete number from map
bool Delete(Database dobj, HANDLE hPipe)
{
	std::lock_guard<std::mutex> lock(numStoreMtx);

	//Before deletion, check if the number exists
	//If yes, we need to send the timestamp to client
	
	//Check if current client ID exists in map
	auto clientIDItr = numStore.find(dobj.procID);
	if (clientIDItr != numStore.end())
	{
		//get map for our client ID
		auto& ClientIDMap = clientIDItr->second;

		auto InputNumItr = ClientIDMap.find(dobj.number);
		if (InputNumItr != ClientIDMap.end())
		{
			cout << "\nInputted number: " << InputNumItr->first;
			cout << "\nTimestamp: " << InputNumItr->second;

			//Send buffer to client
			OutputData output{};
			output.number = InputNumItr->first;
			output.timestamp = InputNumItr->second;

			BOOL bRet;
			DWORD dwBytesWritten;

			//Send number and timestamp to client
			bRet = WriteFile(
				hPipe,
				(LPVOID)&output,
				sizeof(output),
				&dwBytesWritten,
				NULL
			);
			if (bRet)
				cout << "\nDELETE: Number of bytes written: " << dwBytesWritten;
			else
			{
				cout << "\nDELETE: Failed to write data. Error: " << GetLastError();
				return false;
			}
		}
		else
		{
			cout << "\n " << dobj.number << " not present in map.";

			//Sending dummy number in case number not present
			//Send buffer to client
			OutputData output{};
			output.number = -1;
			output.timestamp = -1;

			BOOL bRet;
			DWORD dwBytesWritten;

			//Send number and timestamp to client
			bRet = WriteFile(
				hPipe,
				(LPVOID)&output,
				sizeof(output),
				&dwBytesWritten,
				NULL
			);
			if (bRet)
				cout << "\nDELETE: Number of bytes written: " << dwBytesWritten;
			else
			{
				cout << "\nDELETE: Failed to write data. Error: " << GetLastError();
				return false;
			}

			return true;
		}
	}
	else
	{
		cout << "\nDELETE: No entry found for Client process ID: " << dobj.procID;

		//Sending dummy number in case number not present
		//Send buffer to client
		OutputData output{};
		output.number = -1;
		output.timestamp = -1;

		BOOL bRet;
		DWORD dwBytesWritten;

		//Send number and timestamp to client
		bRet = WriteFile(
			hPipe,
			(LPVOID)&output,
			sizeof(output),
			&dwBytesWritten,
			NULL
		);
		if (bRet)
			cout << "\nDELETE: Number of bytes written: " << dwBytesWritten;
		else
		{
			cout << "\nDELETE: Failed to write data. Error: " << GetLastError();
			return false;
		}

		return true;
	}

	//Delete number from map
	if (numStore[dobj.procID].erase(dobj.number) == 1)
		cout << "\nDELETE: Successfully deleted " << dobj.number << " from store.";
	else
	{
		cout << "\nDELETE: Deletion failed";
		return false;
	}
	return true;
}

//Delete all elements from map
void DeleteAll(Database dobj)
{
	std::lock_guard<std::mutex> lock(numStoreMtx);
	numStore[dobj.procID].clear();
	cout << "\nDELETEALL: Successfully deleted all entries. Store size: " << numStore[dobj.procID].size();
}

bool Print(Database dobj, HANDLE hPipe)
{
	int size;
	BOOL bRet;
	DWORD dwBytesWritten;
	
	// First send the total number of elements in store to client
	std::lock_guard<std::mutex> lock(numStoreMtx);
	size = static_cast<int>(numStore[dobj.procID].size());

	bRet = WriteFile(
		hPipe,
		(LPVOID)&size,
		sizeof(int),
		&dwBytesWritten,
		NULL
	);
	if (bRet)
		cout << "\nPRINT: Number of bytes written: " << dwBytesWritten;
	else
	{
		cout << "\nPRINT: Failed to write data. Error: " << GetLastError();
		return false;
	}
		
	if (size != 0)
	{
		//Iterate map for this client ID and send values one by one
		for (const auto& pair : numStore[dobj.procID])
		{
			bRet = WriteFile(
				hPipe,
				(LPVOID)&pair,
				sizeof(pair),
				&dwBytesWritten,
				NULL
			);

			if (bRet)
			{
				cout << "\nPRINT: Number of bytes written: " << dwBytesWritten;
			}
			else
			{
				cout << "\nPRINT: Failed to write data. Error: " << GetLastError();
				return false;
			}
		}
	}
	return true;

}

//Find number from map
bool Find(Database dobj)
{
	std::lock_guard<std::mutex> lock(numStoreMtx);
	if ((numStore[dobj.procID].find(dobj.number)) != numStore[dobj.procID].end())
	{
		cout <<"\n "<< dobj.number << " found in map.";
		return true;
	}
	else
	{
		cout <<"\n " << dobj.number << " not found in map.";
		return false;
	}
}

int main()
{
	cout << "\nStarting server..";
	HANDLE hPipe = CreateNamedPipe(
		MYPIPE,
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		1,
		sizeof(Database),
		sizeof(Database),
		0,
		NULL);
	if (hPipe == NULL || hPipe == INVALID_HANDLE_VALUE) {
		cout << "\nPipe creation failed. Error: %d." << GetLastError();
		return 1;
	}

	while (true)
	{
		cout << "\n\nWaiting for a client to connect to the pipe...";
		BOOL result = ConnectNamedPipe(hPipe, NULL);
		if (!result) {
			cout << "\nFailed to make connection on named pipe. Error: " << GetLastError();
			CloseHandle(hPipe);
			return 1;
		}
		else
			cout << "\nClient connected..";

		BOOL bRet;
		Database dobj{};
		DWORD dwBytesRead;
		DWORD dwBytesWritten;

		//Get procID, user selected option, number from client
		bRet = ReadFile(
			hPipe,
			(LPVOID)&dobj,
			sizeof(dobj),
			&dwBytesRead,
			NULL
		);
		if (bRet)
		{
			cout << "\nNumber of bytes read: " << dwBytesRead;

			cout << "\nProcessID: " << dobj.procID;
			cout << "\nUser selected operation: " << dobj.operation;
			cout << "\nInput Number: " << dobj.number;
		}
		else
		{
			cout << "\nFailed to get data from client. Error: " << GetLastError();
			//CloseHandle(hPipe);
			//return 1;
		}
			
		bool ret;
		bool res;

		switch (dobj.operation)
		{
		case 1:
			ret = Insert(dobj, hPipe);
			if (!ret)
			{
				cout << "\nInsert function failed.";
				//CloseHandle(hPipe);
				//return 1;
			}
			break;

		case 2:
			ret = Delete(dobj, hPipe);
			if (!ret)
			{
				cout << "\nDelete function failed.";
				//CloseHandle(hPipe);
				//return 1;
			}
			break;

		case 3:
			ret = Print(dobj, hPipe);
			if (!ret)
			{
				cout << "\nPrint function failed.";
				//CloseHandle(hPipe);
				//return 1;
			}
			break;

		case 4:
			DeleteAll(dobj);
			break;

		case 5:
			res = Find(dobj);

			//Send find results to client
			bRet = WriteFile(
				hPipe,
				(LPVOID)&res,
				sizeof(bool),
				&dwBytesWritten,
				NULL
			);
			if (bRet)
			{
				cout << "\nNumber of bytes written: " << dwBytesWritten;
			}
			else
			{
				cout << "\nFailed to write data. Error: " << GetLastError();
				//CloseHandle(hPipe);
				//return 1;
			}
			break;

		default:
			cout << "\nInvalid option";
		}

		// After communication, disconnect the pipe
		DisconnectNamedPipe(hPipe);

	}
	
	cout << "\nServer stopping..";
	CloseHandle(hPipe);
	return 0;
}

