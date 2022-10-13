// optoma_connect_sample.cpp : 此檔案包含 'main' 函式。程式會於該處開始執行及結束執行。
//

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <wlanapi.h>
#include <objbase.h>
#include <wtypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <chrono>
#include <functional>
#include<string>
#include <fstream>
#include <iostream>
#include<vector>
#include<map>
#include "optoma_connect.h"
#import"C:\Users\hy728\桌面\CsharpTest\CsharpTest\bin\Debug\CsharpTest.tlb" no_namespace
#pragma comment(lib, "wlanapi.lib")

using namespace std;
#define N 100


void triggerscan();							//Triggers a scan on each wireless network interface
void wlanInit(HANDLE& wlanHandle, PWLAN_INTERFACE_INFO_LIST& interfaces);			//Function to open the Wlan API handle and gets interface info
void wlanCallback(WLAN_NOTIFICATION_DATA* scanNotificationData, PVOID myContext);	//Function to receive callback notifications for the wireless network scanning

//Context to pass along with callbacks
typedef struct _WLAN_CALLBACK_INFO {
	GUID interfaceGUID;
	HANDLE scanEvent;
	DWORD callbackReason;
} WLAN_CALLBACK_INFO;

vector<string> GetAvalWifiName(vector<string> devices)
{
	devices.clear();
	HRESULT result = 0;								//HRESULT to store the result of Wlan API calls
	HANDLE wlanHandle = NULL;						//HANDLE to the Wlan API
	PWLAN_INTERFACE_INFO_LIST interfaces = nullptr;	//PWLAN_INTERFACE_INFO_LIST pointer for the interface data returned by the Wlan API

	//Get the Wlan API handle and interface info
	wlanInit(wlanHandle, interfaces);
	bool findWPS1111 = false;
	bool findWPS2222 = false;
	string WPS1111RSSI = "";
	string WPS2222RSSI = "";

	//For each interface on the system, we'll print the name and number.
	for (ULONG i = 0; i < interfaces->dwNumberOfItems; i++)
	{

		PWLAN_AVAILABLE_NETWORK_LIST networks = nullptr;				//PWLAN_AVAILABLE_NETWORK_LIST pointer for the data returned by the Wlan API

		//Get the list of visible wireless networks
		result = WlanGetAvailableNetworkList(
			wlanHandle,													//The HANDLE returned by WlanOpenHandle
			&(interfaces->InterfaceInfo[i].InterfaceGuid),				//The wireless network interface to get network data from
			WLAN_AVAILABLE_NETWORK_INCLUDE_ALL_ADHOC_PROFILES |			//Include all ad hoc network profiles in the available network list, including profiles that are not visible
			WLAN_AVAILABLE_NETWORK_INCLUDE_ALL_MANUAL_HIDDEN_PROFILES,	//And include all hidden network profiles in the available network list, including profiles that are not visible
			NULL,														//Reserved
			&networks													//Address of the pointer to store the location to the network data in
		);

		//Print a clean header, tabbed in and spaced out

		//Loop over the visible networks returned by the Wlan API
		for (ULONG num = 0; num < networks->dwNumberOfItems; num++)
		{

			// The network name is an unsigned char, but we need a wchar_t to print it
			//So for each character in the SSID, let's convert the unsigned char a wchar_t and store it in networkSSID[]
			string wifiName = "";
			int iRSSI = 0;

			for (ULONG a = 0; a < networks->Network[num].dot11Ssid.uSSIDLength; a++)
			{
				wifiName = wifiName + char(networks->Network[num].dot11Ssid.ucSSID[a]);
			}

			//if (!findWPS1111 && wifiName == "WPS1111")
			//{
			//	if (networks->Network[num].wlanSignalQuality == 0)
			//		iRSSI = -100;
			//	else if (networks->Network[num].wlanSignalQuality == 100)
			//		iRSSI = -50;
			//	else
			//		iRSSI = -100 + (networks->Network[num].wlanSignalQuality / 2);

			//	WPS1111RSSI = wifiName + "," + to_string(iRSSI);
			//	findWPS1111 = true;
			//}
			//else if (!findWPS2222 && wifiName == "WPS2222")
			//{
			//	if (networks->Network[num].wlanSignalQuality == 0)
			//		iRSSI = -100;
			//	else if (networks->Network[num].wlanSignalQuality == 100)
			//		iRSSI = -50;
			//	else
			//		iRSSI = -100 + (networks->Network[num].wlanSignalQuality / 2);

			//	WPS2222RSSI = wifiName + "," + to_string(iRSSI);
			//	findWPS2222 = true;
			//}
			//else if (findWPS1111 && findWPS2222)
			//{
			//	WlanFreeMemory(networks);
			//	WlanFreeMemory(interfaces);						//Pointer to the PWLAN_WLAN_INTERFACE_INFO_LIST data
			//	WlanCloseHandle(wlanHandle, NULL);				//The Wlan HANDLE and a Reserved value
			//	triggerscan();
			//	devices.push_back(WPS1111RSSI);
			//	devices.push_back(WPS2222RSSI);
			//	return devices;
			//}
			devices.push_back(wifiName);
		}


		//And free the memory the Wlan API allocated for us
		WlanFreeMemory(networks);

	}

	wcout << endl;
	//Let's free the memory the Wlan API allocated for us and close the handle we opened
	WlanFreeMemory(interfaces);						//Pointer to the PWLAN_WLAN_INTERFACE_INFO_LIST data
	WlanCloseHandle(wlanHandle, NULL);				//The Wlan HANDLE and a Reserved value
	triggerscan();
	return devices;
}


struct timed_execution {
	typedef void (*func_type)(void);
	timed_execution(func_type func, const std::chrono::milliseconds period)
		: func_(func)
		, period_(period)
		, thread_(std::bind(&timed_execution::threadFunc, this))
	{
	}
private:
	void threadFunc() {
		while (true) {
			std::this_thread::sleep_for(period_);
			func_();
		}
	}
	func_type func_;
	const std::chrono::milliseconds period_;
	std::thread thread_;
};


//void exeFunInPer()
//{
//	timed_execution t(shownetworks, std::chrono::milliseconds(N));
//	std::this_thread::sleep_for(std::chrono::seconds(1000));
//}

void wlanInit(HANDLE& wlanHandle, PWLAN_INTERFACE_INFO_LIST& interfaces)
{
	HRESULT result = 0;								//HRESULT to store the return value from Wlan API calls
	DWORD negotiatedVersion = 0;					//DWORD for the Wlan API to store the negotiated API version in

	//Open a handle to the Wlan API
	result = WlanOpenHandle(
		WLAN_API_VERSION_2_0,						//Request API version 2.0
		NULL,										//Reserved
		&negotiatedVersion,							//Address of the DWORD to store the negotiated version
		&wlanHandle									//Address of the HANDLE to store the Wlan handle
	);


	//Enumerate the wireless network interfaces
	result = WlanEnumInterfaces(
		wlanHandle,									//The HANDLE returned by WlanOpenHandle
		NULL,										//Reserved
		&interfaces									//Address of the pointer to store the location to the interface data in
	);



	return;

}


void wlanCallback(WLAN_NOTIFICATION_DATA* scanNotificationData, PVOID myContext)
{
	//Get the data from my struct. If it's null, nothing to do
	WLAN_CALLBACK_INFO* callbackInfo = (WLAN_CALLBACK_INFO*)myContext;
	if (callbackInfo == nullptr)
	{
		return;
	}

	//Check the GUID in the struct against the GUID in the notification data, return if they don't match
	if (memcmp(&callbackInfo->interfaceGUID, &scanNotificationData->InterfaceGuid, sizeof(GUID)) != 0)
	{
		return;
	}

	//If the notification was for a scan complete or failure then we need to set the event
	if ((scanNotificationData->NotificationCode == wlan_notification_acm_scan_complete) || (scanNotificationData->NotificationCode == wlan_notification_acm_scan_fail))
	{
		//Set the notification code as the callbackReason
		callbackInfo->callbackReason = scanNotificationData->NotificationCode;

		//Set the event
		SetEvent(callbackInfo->scanEvent);
	}

	return;
}


void triggerscan()
{
	HRESULT result = 0;								//HRESULT to store the result of Wlan API calls
	HANDLE wlanHandle = NULL;						//HANDLE to the WLAN api
	PWLAN_INTERFACE_INFO_LIST interfaces = nullptr;	//PWLAN_INTERFACE_INFO_LIST pointer for the interface data returned by the Wlan API

	wlanInit(wlanHandle, interfaces);				//Get the Wlan API handle and interface info

	//For each interface on the system, we'll print the name and number.
	for (ULONG i = 0; i < interfaces->dwNumberOfItems; i++)
	{

		//Declare the callback parameter struct
		WLAN_CALLBACK_INFO callbackInfo = { 0 };
		callbackInfo.interfaceGUID = interfaces->InterfaceInfo[i].InterfaceGuid;

		//Create an event to be triggered in the scan case
		callbackInfo.scanEvent = CreateEvent(
			nullptr,
			FALSE,
			FALSE,
			nullptr);


		//Register for wlan scan notifications
		WlanRegisterNotification(wlanHandle,
			WLAN_NOTIFICATION_SOURCE_ALL,
			TRUE,
			(WLAN_NOTIFICATION_CALLBACK)wlanCallback,
			(PVOID)&callbackInfo,
			NULL,
			NULL);


		//Start a scan. If the WlanScan call fails, log the error
		WlanScan(wlanHandle, &(interfaces->InterfaceInfo[i].InterfaceGuid), NULL, NULL, NULL);
		if (GetLastError() != ERROR_SUCCESS)
		{
			wcout << "Error triggering scan on interface " << i + 1 << ". Error: " << GetLastError() << endl;
			continue;
		}
		else
		{
			//Scan request successfully sent
			/*wcout << "Scan request sent. Waiting for reply." << endl;*/
		}


		//Wait for the event to be signaled, or an error to occur. Don't wait longer than 15 seconds.
		DWORD waitResult = WaitForSingleObject(callbackInfo.scanEvent, 15000);

		//Check how we got here, via callback or timeout
		if (waitResult == WAIT_OBJECT_0)
		{
			if (callbackInfo.callbackReason == wlan_notification_acm_scan_complete)
			{
				/*wcout << "Reply: The scan for networks has completed." << endl;*/
			}
			else if (callbackInfo.callbackReason == wlan_notification_acm_scan_fail)
			{
				//wcout << "Reply: The scan for connectable networks failed." << endl;
			}


		}
		else if (waitResult == WAIT_TIMEOUT)
		{
			wcout << "Error: No response was received after 15 seconds." << endl;
			wcout << "\nWindows Logo certified wireless drivers are required to complete scans\n"
				<< "in under four seconds, so there may be something wrong." << endl << endl;
		}
		else
		{
			wcout << "\nUnknown error waiting for response. Error Code: " << waitResult << endl << endl;
		}

	}

	//Let's free the memory the Wlan API allocated for us and close the handle we opened
	WlanFreeMemory(interfaces);						//Pointer to the PWLAN_WLAN_INTERFACE_INFO_LIST data
	WlanCloseHandle(wlanHandle, NULL);				//The Wlan HANDLE and a Reserved value
	return;
}

map<string, string> shownetworks(map<string, string> wifiNameMap) 
{
	HRESULT result = 0;								//HRESULT to store the result of Wlan API calls
	HANDLE wlanHandle = NULL;						//HANDLE to the Wlan API
	PWLAN_INTERFACE_INFO_LIST interfaces = nullptr;	//PWLAN_INTERFACE_INFO_LIST pointer for the interface data returned by the Wlan API

	//Get the Wlan API handle and interface info
	wlanInit(wlanHandle, interfaces);
	bool findWPS1111 = false;
	bool findWPS2222 = false;
	string WPS1111RSSI = "";
	string WPS2222RSSI = "";

	//For each interface on the system, we'll print the name and number.
	for (ULONG i = 0; i < interfaces->dwNumberOfItems; i++)
	{

		PWLAN_AVAILABLE_NETWORK_LIST networks = nullptr;				//PWLAN_AVAILABLE_NETWORK_LIST pointer for the data returned by the Wlan API

		//Get the list of visible wireless networks
		result = WlanGetAvailableNetworkList(
			wlanHandle,													//The HANDLE returned by WlanOpenHandle
			&(interfaces->InterfaceInfo[i].InterfaceGuid),				//The wireless network interface to get network data from
			WLAN_AVAILABLE_NETWORK_INCLUDE_ALL_ADHOC_PROFILES |			//Include all ad hoc network profiles in the available network list, including profiles that are not visible
			WLAN_AVAILABLE_NETWORK_INCLUDE_ALL_MANUAL_HIDDEN_PROFILES,	//And include all hidden network profiles in the available network list, including profiles that are not visible
			NULL,														//Reserved
			&networks													//Address of the pointer to store the location to the network data in
		);

		//Print a clean header, tabbed in and spaced out

		//Loop over the visible networks returned by the Wlan API
		for (ULONG num = 0; num < networks->dwNumberOfItems; num++)
		{

			// The network name is an unsigned char, but we need a wchar_t to print it
			//So for each character in the SSID, let's convert the unsigned char a wchar_t and store it in networkSSID[]
			string wifiName = "";
			int iRSSI = 0;

			for (ULONG a = 0; a < networks->Network[num].dot11Ssid.uSSIDLength; a++)
			{
				wifiName = wifiName + char(networks->Network[num].dot11Ssid.ucSSID[a]);
			}


			if (wifiName == "WPS1111" && wifiNameMap.find("WPS1111") == wifiNameMap.end())
			{
				if (networks->Network[num].wlanSignalQuality == 0)
					iRSSI = -100;
				else if (networks->Network[num].wlanSignalQuality == 100)
					iRSSI = -50;
				else
					iRSSI = -100 + (networks->Network[num].wlanSignalQuality / 2);
				WPS1111RSSI = to_string(iRSSI);
				wifiNameMap["WPS1111"] = WPS1111RSSI;
			}
			else if (wifiName == "WPS2222" && wifiNameMap.find("WPS2222") == wifiNameMap.end())
			{
				if (networks->Network[num].wlanSignalQuality == 0)
					iRSSI = -100;
				else if (networks->Network[num].wlanSignalQuality == 100)
					iRSSI = -50;
				else
					iRSSI = -100 + (networks->Network[num].wlanSignalQuality / 2);
				WPS2222RSSI = to_string(iRSSI);
				findWPS2222 = true;
				wifiNameMap["WPS2222"] = WPS2222RSSI;
			}


		}


		//And free the memory the Wlan API allocated for us
		WlanFreeMemory(networks);

	}

	//Let's free the memory the Wlan API allocated for us and close the handle we opened
	WlanFreeMemory(interfaces);						//Pointer to the PWLAN_WLAN_INTERFACE_INFO_LIST data
	WlanCloseHandle(wlanHandle, NULL);				//The Wlan HANDLE and a Reserved value
	triggerscan();
	return wifiNameMap; 
}



int main()
{
	try
	{
		// initialize the library
		initial();
		std::string s;
		do
		{
			std::cout << "======================================\n";
			std::cout << "1 : List Devices\n";
			std::cout << "2 : Connect\n";
			std::cout << "3 : Disconnect\n";
			std::cout << "4 : Searching for wifi\n"; //自己加的
			std::cout << "5 : Searching for bluetooth\n"; //自己加的
			std::cout << "0 : Exit\n";
			std::cout << "Enter a command : \n";
			std::getline(std::cin, s);
			char* command = new char[s.size() + 1];
			memset(command, 0, s.size() + 1);
			memcpy_s(command, s.size() + 1, s.c_str(), s.size());
			char* content(nullptr);
			auto t = strtok_s(command, " ", &content);
			if (!t)
			{
				break;
			}
			switch (*t)
			{
			case '1':
			{
				// get wifi device
				auto devices = get_device_list();
				devices = GetAvalWifiName(get_device_list());
				std::cout << "總共有以下 " << devices.size() << " 設備\n\n";

				for (size_t index(0); index < devices.size(); ++index)
				{
					cout << "SSID" << index + 1 << ": " << devices[index].c_str() << "\n";
				}
				break;
			}
			case '2':
				// connect the specific wifi
				t = strtok_s(NULL, " ", &content);
				if (t)
				{
					connect(t);
				}
				break;
			case '3':
				// disconnect
				disconnect();
				break;
			case '4': //自己加的
			{
				//exeFunInPer();
				while (true)
				{
					cout << "Scanning..." << endl;
					vector<map<string, string>> wifiNameList;
					for (int i = 0; i < 5; i++)
					{
						wifiNameList.push_back(shownetworks({}));
					}

					
					cout << "WPS1111: ";
					for (int i = 0; i < 4; i++)
					{
						if (wifiNameList[i].find("WPS1111") != wifiNameList[i].end())
						{
							cout << wifiNameList[i]["WPS1111"] << ",";
						}			
					}
					if (wifiNameList[4].find("WPS1111") != wifiNameList[4].end())
					{
						cout << wifiNameList[4]["WPS1111"];
					}
					cout << endl;

					cout << "WPS2222: ";
					for (int i = 0; i < 4; i++)
					{
						if (wifiNameList[i].find("WPS2222") != wifiNameList[i].end())
						{
							cout << wifiNameList[i]["WPS2222"] << ",";
						}
					}
					if (wifiNameList[4].find("WPS2222") != wifiNameList[4].end())
					{
						cout << wifiNameList[4]["WPS2222"];
					}
					cout << endl;

				}
				break;
			}

			case '0':
			default:
				break;
			}
			delete[] command;
			printf("\n");
		} while (s[0] != '0');
		// de-initialize the library
		deinitial();
	}
	catch (std::exception& e)
	{
		// please let me know the error code
		std::cout << e.what() << std::endl;
		system("pause");
		return -1;
	}
	return 0;
}

// 執行程式: Ctrl + F5 或 [偵錯] > [啟動但不偵錯] 功能表
// 偵錯程式: F5 或 [偵錯] > [啟動偵錯] 功能表

// 開始使用的提示: 
//   1. 使用 [方案總管] 視窗，新增/管理檔案
//   2. 使用 [Team Explorer] 視窗，連線到原始檔控制
//   3. 使用 [輸出] 視窗，參閱組建輸出與其他訊息
//   4. 使用 [錯誤清單] 視窗，檢視錯誤
//   5. 前往 [專案] > [新增項目]，建立新的程式碼檔案，或是前往 [專案] > [新增現有項目]，將現有程式碼檔案新增至專案
//   6. 之後要再次開啟此專案時，請前往 [檔案] > [開啟] > [專案]，然後選取 .sln 檔案
