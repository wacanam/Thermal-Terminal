// get _real_time_temp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <stdio.h>
#include <string>
#include <iostream>
//#include <curl\curl.h>
#include "Windows.h"
#include "HCNetSDK.h"
using namespace std;

//Macro Definition of temporal resolution
#define GET_YEAR(_time_)      (((_time_)>>26) + 2000) 
#define GET_MONTH(_time_)     (((_time_)>>22) & 15)
#define GET_DAY(_time_)       (((_time_)>>17) & 31)
#define GET_HOUR(_time_)      (((_time_)>>12) & 31) 
#define GET_MINUTE(_time_)    (((_time_)>>6)  & 63)
#define GET_SECOND(_time_)    (((_time_)>>0)  & 63)

int iNum = 0;
#define ISAPI_OUT_LEN	3 * 1024 * 1024
#define ISAPI_STATUS_LEN  8*1024



void CALLBACK GetThermInfoCallback(DWORD dwType, void* lpBuffer, DWORD dwBufLen, void* pUserData)
{
	if (dwType == NET_SDK_CALLBACK_TYPE_DATA)
	{
		LPNET_DVR_THERMOMETRY_UPLOAD lpThermometry = new NET_DVR_THERMOMETRY_UPLOAD;
		memcpy(lpThermometry, lpBuffer, sizeof(*lpThermometry));

		NET_DVR_TIME struAbsTime = { 0 };
		struAbsTime.dwYear = GET_YEAR(lpThermometry->dwAbsTime);
		struAbsTime.dwMonth = GET_MONTH(lpThermometry->dwAbsTime);
		struAbsTime.dwDay = GET_DAY(lpThermometry->dwAbsTime);
		struAbsTime.dwHour = GET_HOUR(lpThermometry->dwAbsTime);
		struAbsTime.dwMinute = GET_MINUTE(lpThermometry->dwAbsTime);
		struAbsTime.dwSecond = GET_SECOND(lpThermometry->dwAbsTime);

		printf("Real-time temperature measurement result: byRuleID[%d]wPresetNo[%d]byRuleCalibType[%d]byThermometryUnit[d%]byDataType[d%]"
			"dwAbsTime[%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d]\n", lpThermometry->byRuleID, lpThermometry->wPresetNo,
			lpThermometry->byRuleCalibType, lpThermometry->byThermometryUnit, lpThermometry->byDataType,
			struAbsTime.dwYear, struAbsTime.dwMonth, struAbsTime.dwDay,
			struAbsTime.dwHour, struAbsTime.dwMinute, struAbsTime.dwSecond);

		if (lpThermometry->byRuleCalibType == 0) //Measure temperature by point
		{
			printf("Information of Measuring Temperature by Point: fTemperature[%d]\n", lpThermometry->struPointThermCfg.fTemperature);
		}

		if ((lpThermometry->byRuleCalibType == 1) || (lpThermometry->byRuleCalibType == 2)) //Measure temperature by frame or line
		{
			printf("Information of Measuring Temperature by Frame or Line: fMaxTemperature[%d]fMinTemperature[%d]fAverageTemperature[%d]fTemperatureDiff[%d]\n",
				lpThermometry->struLinePolygonThermCfg.fMaxTemperature, lpThermometry->struLinePolygonThermCfg.fMinTemperature,
				lpThermometry->struLinePolygonThermCfg.fAverageTemperature, lpThermometry->struLinePolygonThermCfg.fTemperatureDiff);
		}

		if (lpThermometry != NULL)
		{
			delete lpThermometry;
			lpThermometry = NULL;
		}
	}
	else if (dwType == NET_SDK_CALLBACK_TYPE_STATUS)
	{
		DWORD dwStatus = *(DWORD*)lpBuffer;
		if (dwStatus == NET_SDK_CALLBACK_STATUS_SUCCESS)
		{
			printf("dwStatus:NET_SDK_CALLBACK_STATUS_SUCCESS\n");
		}
		else if (dwStatus == NET_SDK_CALLBACK_STATUS_FAILED)
		{
			DWORD dwErrCode = *(DWORD*)((char *)lpBuffer + 4);
			printf("NET_DVR_GET_MANUALTHERM_INFO failed, Error code %d\n", dwErrCode);
		}
	}
}

void main()
{
	//curl_post_data("http://postit.example.com/moo.cgi", "", "name=daniel&project=curl");
	DWORD dwChannel = 2;  //Thermal imaging channel

	//---------------------------------------
	//Initialize
	NET_DVR_Init();

	//Set connected time and reconnected time
	NET_DVR_SetConnectTime(2000, 1);
	NET_DVR_SetReconnect(10000, true);

	//---------------------------------------
	//Register device (it is not required when listening alarm)
	LONG lUserID;
	NET_DVR_DEVICEINFO_V40 struDeviceInfo;
	//NET_DVR_USER_LOGIN_INFO struUserLoginInfo;
	lUserID = 1;
	//lUserID = NET_DVR_Login_V0("172.28.7.95", 8000, "admin", "1234567.", &struDeviceInfo);

	//NET_DVR_USER_LOGIN_INFO struUserLoginInfo = { 0 };
	//struUserLoginInfo.sDeviceAddress[NET_DVR_DEV_ADDRESS_MAX_LEN]  "172.28.7.95";

	//lUserID = NET_DVR_Login_V40(&struUserLoginInfo, &struDeviceInfo);
	if (lUserID < 0)
	{
		printf("Login error, %d\n", NET_DVR_GetLastError());
		NET_DVR_Cleanup();
		return;
	}

	//Enable real-time temperature measurement
	NET_DVR_REALTIME_THERMOMETRY_COND struThermCond = { 0 };
	struThermCond.dwSize = sizeof(struThermCond);
	struThermCond.byRuleID = 1;       //Rule ID, 0-Get All Rules, the ID starts from 1.
	struThermCond.dwChan = dwChannel; //Start from 1, 0xffffffff- Get All Channels

	LONG lHandle = NET_DVR_StartRemoteConfig(lUserID, NET_DVR_GET_REALTIME_THERMOMETRY, &struThermCond, sizeof(struThermCond), GetThermInfoCallback, NULL);
	if (lHandle >= 0)
	{
		printf("NET_DVR_GET_REALTIME_THERMOMETRY failed, error code: %d\n", NET_DVR_GetLastError());
	}
	else
	{
		printf("NET_DVR_GET_REALTIME_THERMOMETRY is successful!");
	}

	Sleep(5000);  //Wait for receiving real-time temperature measurement result

	//Close the handle created by long connection configuration API, and release resource.
	if (!NET_DVR_StopRemoteConfig(lHandle))
	{
		printf("NET_DVR_StopRemoteConfig failed, error code: %d\n", NET_DVR_GetLastError());
	}

	//User logout, if the user is not login, skip this step.
	NET_DVR_Logout(lUserID);

	//Release SDK resource
	NET_DVR_Cleanup();

	cin.get();
}