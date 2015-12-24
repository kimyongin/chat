#pragma once
#include <Windows.h>
#include <netfw.h>

namespace Common
{
	namespace FireWall
	{
		HRESULT IsFireWallOn(OUT PBOOL isOn); // 윈도우기본방화벽 사용유무 확인
		HRESULT IsFireWallContainApp(IN LPCTSTR szProcessImageName, OUT PBOOL isOn);
		HRESULT AddFireWallApp(IN LPCTSTR szProcessImageName, IN LPCTSTR policyName);
		HRESULT TurnFireWallOn();
		HRESULT TurnFireWallOff();

		enum FW_ERROR_CODE
		{
			FW_NOERROR = 0,
			FW_ERR_INITIALIZED,					// Already initialized or doesn't call Initialize()
			FW_ERR_CREATE_SETTING_MANAGER,		// Can't create an instance of the firewall settings manager
			FW_ERR_LOCAL_POLICY,				// Can't get local firewall policy
			FW_ERR_PROFILE,						// Can't get the firewall profile
			FW_ERR_FIREWALL_IS_ENABLED,			// Can't get the firewall enable information
			FW_ERR_FIREWALL_ENABLED,			// Can't set the firewall enable option
			FW_ERR_INVALID_ARG,					// Invalid Arguments
			FW_ERR_AUTH_APPLICATIONS,			// Failed to get authorized application list
			FW_ERR_APP_ENABLED,					// Failed to get the application is enabled or not
			FW_ERR_CREATE_APP_INSTANCE,			// Failed to create an instance of an authorized application
			FW_ERR_SYS_ALLOC_STRING,			// Failed to alloc a memory for BSTR
			FW_ERR_PUT_PROCESS_IMAGE_NAME,		// Failed to put Process Image File Name to Authorized Application
			FW_ERR_PUT_REGISTER_NAME,			// Failed to put a registered name
			FW_ERR_ADD_TO_COLLECTION,			// Failed to add to the Firewall collection
			FW_ERR_REMOVE_FROM_COLLECTION,		// Failed to remove from the Firewall collection
			FW_ERR_GLOBAL_OPEN_PORTS,			// Failed to retrieve the globally open ports
			FW_ERR_PORT_IS_ENABLED,				// Can't get the firewall port enable information
			FW_ERR_PORT_ENABLED,				// Can't set the firewall port enable option
			FW_ERR_CREATE_PORT_INSTANCE,		// Failed to create an instance of an authorized port
			FW_ERR_SET_PORT_NUMBER,				// Failed to set port number
			FW_ERR_SET_IP_PROTOCOL,				// Failed to set IP Protocol
			FW_ERR_EXCEPTION_NOT_ALLOWED,		// Failed to get or put the exception not allowed
			FW_ERR_NOTIFICATION_DISABLED,		// Failed to get or put the notification disabled
			FW_ERR_UNICAST_MULTICAST,			// Failed to get or put the UnicastResponses To MulticastBroadcast Disabled Property 
		};

		class FireWallImpl
		{
		public:
			FireWallImpl(void);
			~FireWallImpl(void);

			// You should call after CoInitialize() is called
			FW_ERROR_CODE Initialize();

			// This function is automatically called by destructor, but should be called before CoUninitialize() is called
			FW_ERROR_CODE Uninitialize();

			FW_ERROR_CODE IsWindowsFirewallOn(BOOL& bOn);

			FW_ERROR_CODE TurnOnWindowsFirewall();
			FW_ERROR_CODE TurnOffWindowsFirewall();

			// lpszProcessImageFilaName: File path
			// lpszRegisterName: You can see this name throught the control panel
			FW_ERROR_CODE AddApplication(LPCTSTR lpszProcessImageFileName, LPCTSTR lpszRegisterName);
			FW_ERROR_CODE RemoveApplication(LPCTSTR lpszProcessImageFileName);
			FW_ERROR_CODE IsAppEnabled(LPCTSTR lpszProcessImageFileName, BOOL& bEnable);

			// lpszRegisterName: You can see this name throught the control panel
			FW_ERROR_CODE AddPorts(LONG lPortNumber, NET_FW_IP_PROTOCOL ipProtocol, LPCTSTR lpszRegisterName);
			FW_ERROR_CODE RemovePort(LONG lPortNumber, NET_FW_IP_PROTOCOL ipProtocol);
			FW_ERROR_CODE IsPortEnabled(LONG lPortNumber, NET_FW_IP_PROTOCOL ipProtocol, BOOL& bEnable);

			FW_ERROR_CODE IsExceptionNotAllowed(BOOL& bNotAllowed);
			FW_ERROR_CODE SetExceptionNotAllowed(BOOL bNotAllowed);

			FW_ERROR_CODE IsNotificationDiabled(BOOL& bDisabled);
			FW_ERROR_CODE SetNotificationDiabled(BOOL bDisabled);

			FW_ERROR_CODE IsUnicastResponsesToMulticastBroadcastDisabled(BOOL& bDisabled);
			FW_ERROR_CODE SetUnicastResponsesToMulticastBroadcastDisabled(BOOL bDisabled);
		protected:
			INetFwProfile* m_pFireWallProfile;
			INetFwProfile* m_pFireWallProfile_Domain;	// 도메인
			INetFwProfile* m_pFireWallProfile_Standard;	// 일반
			HRESULT m_comInit;
		};
	}
}