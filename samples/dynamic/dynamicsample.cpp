
#include <gainput/gainput.h>

#include "../samplefw/SampleFramework.h"

#if defined(GAINPUT_PLATFORM_LINUX) || defined(GAINPUT_PLATFORM_WIN)
#define ENABLE_FILEIO
#include <iostream>
#include <fstream>
#include <sstream>
#endif


enum Button
{
	ButtonReset,
	ButtonLoad,
	ButtonSave,
	ButtonTest
};


void SampleMain()
{
	SfwOpenWindow("Gainput: Dynamic sample");

	gainput::InputManager manager;

	const gainput::DeviceId mouseId = manager.CreateDevice<gainput::InputDeviceMouse>();
	const gainput::DeviceId touchId = manager.CreateDevice<gainput::InputDeviceTouch>();
	const gainput::DeviceId keyboardId = manager.CreateDevice<gainput::InputDeviceKeyboard>();
	const gainput::DeviceId padId = manager.CreateDevice<gainput::InputDevicePad>();

#if defined(GAINPUT_PLATFORM_LINUX)
	manager.SetXDisplay(SfwGetXDisplay(), SfwGetWidth(), SfwGetHeight());
#elif defined(GAINPUT_PLATFORM_WIN)
	manager.SetDisplaySize(SfwGetWidth(), SfwGetHeight());
#endif

	SfwSetInputManager(&manager);

	gainput::InputMap map(manager);
	map.MapBool(ButtonReset, keyboardId, gainput::KEY_ESCAPE);
	map.MapBool(ButtonSave, keyboardId, gainput::KEY_F1);
	map.MapBool(ButtonLoad, keyboardId, gainput::KEY_F2);

	gainput::DeviceButtonSpec anyButton[32];
	bool mapped = false;

	SFW_LOG("No button mapped, please press any button.\n");
	SFW_LOG("Press ESC to reset.\n");


	while (!SfwIsDone())
	{
		manager.Update();

#if defined(GAINPUT_PLATFORM_WIN)
		MSG msg;
		while (PeekMessage(&msg, SfwGetHWnd(),  0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			manager.HandleMessage(msg);
		}
#endif

		SfwUpdate();

		if (map.GetBoolWasDown(ButtonReset))
		{
			SFW_LOG("Mapping reset. Press any button.\n");
			mapped = false;
			map.Unmap(ButtonTest);
		}

		if (map.GetBoolWasDown(ButtonSave))
		{
			SFW_LOG("Saving...\n");
#ifdef ENABLE_FILEIO
			std::ofstream of;
			of.open("mappings.txt");
#endif
			const size_t maxCount = 32;
			gainput::DeviceButtonSpec buttons[maxCount];
			for (int i = ButtonReset; i <= ButtonTest; ++i)
			{
				if (!map.IsMapped(i))
					continue;
				const size_t count = map.GetMappings(gainput::UserButtonId(i), buttons, maxCount);
				for (size_t b = 0; b < count; ++b)
				{
					const gainput::InputDevice* device = manager.GetDevice(buttons[b].deviceId);
					char name[maxCount];
					device->GetButtonName(buttons[b].buttonId, name, maxCount);
					const gainput::ButtonType buttonType = device->GetButtonType(buttons[b].buttonId);
					SFW_LOG("Btn %d: %d:%d (%s%d:%s) type=%d\n", i, buttons[b].deviceId, buttons[b].buttonId, device->GetTypeName(), device->GetIndex(), name, buttonType);
#ifdef ENABLE_FILEIO
					of << i << " " << device->GetTypeName() << " " << device->GetIndex() << " " << name << " " << buttonType << std::endl;
#endif
				}
			}
#ifdef ENABLE_FILEIO
			of.close();
#endif
		}

		if (map.GetBoolWasDown(ButtonLoad))
		{
			SFW_LOG("Loading...\n");
#ifdef ENABLE_FILEIO
			std::ifstream ifs("mappings.txt");
			if(ifs.is_open())
			{
				map.Clear();
				std::string line;
				int i;
				std::string typeName;
				unsigned index;
				std::string name;
				int buttonType;
				while (ifs.good())
				{
					getline(ifs, line);
					if (!ifs.good())
						break;
					std::istringstream iss(line);
					iss >> i;
					iss >> typeName;
					iss >> index;
					iss >> name;
					iss >> buttonType;
					gainput::DeviceId deviceId = manager.FindDeviceId(typeName.c_str(), index);
					gainput::InputDevice* device = manager.GetDevice(deviceId);
					gainput::DeviceButtonId button = device->GetButtonByName(name.c_str());
					SFW_LOG("Btn %d: %d:%d (%s%d:%s) type=%d\n", i, deviceId, button, typeName.c_str(), index, name.c_str(), buttonType);
					if (buttonType == gainput::BT_BOOL)
					{
						map.MapBool(i, deviceId, button);
					}
					else if (buttonType == gainput::BT_FLOAT)
					{
						map.MapFloat(i, deviceId, button);
					}
				}
				ifs.close();
			}
#endif
		}


		if (!mapped)
		{
			const size_t anyCount = manager.GetAnyButtonDown(anyButton, 32);
			for (size_t i = 0; i < anyCount; ++i)
			{
				// Filter the returned buttons as needed.
				const gainput::InputDevice* device = manager.GetDevice(anyButton[i].deviceId);
				if (device->GetButtonType(anyButton[i].buttonId) == gainput::BT_BOOL
					&& map.GetUserButtonId(anyButton[i].deviceId, anyButton[i].buttonId) == gainput::InvalidDeviceButtonId)
				{
					SFW_LOG("Mapping to: %d:%d\n", anyButton[i].deviceId, anyButton[i].buttonId);
					map.MapBool(ButtonTest, anyButton[i].deviceId, anyButton[i].buttonId);
					mapped = true;
					break;
				}
			}
		}
		else
		{
			if (map.GetBoolWasDown(ButtonTest))
			{
				SFW_LOG("Button was down!\n");
			}
		}
	}

	SfwCloseWindow();
}


