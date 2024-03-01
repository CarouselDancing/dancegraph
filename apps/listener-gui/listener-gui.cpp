#define _WINSOCKAPI_    // stops windows.h including winsock.h
#define ENET_IMPLEMENTATION



#include <string>
#include <filesystem>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>
#include "imgui_util.h"

#include <nlohmann/json.hpp>

#include <core/common/config.h>
#include <core/net/net.h>
#include <core/net/listener.h>
#include <core/net/config.h>

#include <WS2tcpip.h>
#include <windows.h>
#include <commdlg.h>

#include "listener-gui.h"



//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Listener, server_address, server_ip, server_port, server_config)


using namespace nlohmann;
using json = nlohmann::json;

static ExampleAppLog g_Log;


NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(net::Address, ip, port);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(net::UserRoleConfig, user_signals, env_signals);

/*
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(net::SceneConfig, user_roles);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(net::ClientListenerCommonConfig, name, address, server_address, scene, adapter, env_signals, user_signals);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(net::ListenerConfig, common, signals, clients);
*/

//------- Utilities -------

// example flt: "Image Files\0*.jpg;*.jpeg;*.png;\0"
// example flt: "JSON files\0*.json;\0"
std::string OpenFileDialog(const char * flt = "JSON files\0*.json;\0")
{
    OPENFILENAME ofn;
    char szFile[512];

    // open a file name
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = flt;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    GetOpenFileName(&ofn);
    return szFile;
}

std::string ReadAllText(const std::string_view fname)
{
    std::stringstream buffer;
    std::ifstream ifsdb(fname.data());
    buffer << ifsdb.rdbuf();
    return buffer.str();
}

void WriteAllBytes(const char* fname, const void* data, size_t elementSize, size_t elementNum)
{
    FILE* fp = fopen(fname, "wb");
    if (fp != nullptr)
    {
        fwrite(data, elementSize, elementNum, fp);
        fclose(fp);
    }
    else
        printf("File could not be opened for writing: %s\n", fname);
}

void WriteAllText(const std::string_view fname, const std::string& text)
{
    WriteAllBytes(fname.data(), text.data(), 1, text.size());
}

//~~~~~~ Utilities -------

void Listener::Init(int argc, char** argv)
{
    net::initialize();

    config::ConfigServer configServer;
    auto jsConfig = configServer.get_net_cfg(listener_config_name);
    listenerConfig = net::loadListenerConfig(jsConfig);
    jsConfig.at("signals").get_to(signalList);
}

void Listener::OnGui()
{
    if (is_listener_running) {        
        listenerState.tick();
    }
	static const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    ImGui::SetNextWindowPos({0,0});
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	if (ImGui::Begin("main",nullptr, flags))
	{
		if (ImGui::BeginTable("table", 2))
		{
			ImGui::TableNextColumn();

            if (!is_listener_running)
            {
                ImGui::InputText("IP", &listenerConfig.common.server_address.ip, ImGuiInputTextFlags_CharsDecimal);
                // Port
                ImGui::DragInt("Port", &listenerConfig.common.server_address.port, 1, 0, 65536);

                if (ImGui::Button("Start listener"))
                    Start();
            }
            else
            {
                // TODO: run server state!
                ImGui::LabelText("Address", "%s:%d", listenerConfig.common.server_address.ip.c_str(), listenerConfig.common.server_address.port);
            }

			// Put the log here
			ImGui::TableNextColumn();
            g_Log.Draw();

			ImGui::EndTable();
		}
		ImGui::End();
	}
}

void Listener::Start()
{
    // validate ip
    IN_ADDR addr;
    if (inet_pton(AF_INET, listenerConfig.common.server_address.ip.c_str(), &addr) != 1)
    {
        g_Log.AddLog("Error: Invalid IP address: %s\n", listenerConfig.common.server_address.ip.c_str());
        return;
    }

    // Save config to listener-gui.json
    //WriteAllText( app_config_filename,  json(*this).dump(4));

    
    //WriteAllText(app_config_filename, json(listenerConfig.common.server_address).dump(4));

    // hook the network log to our gui log
    set_log_callback([](const char* s) { g_Log.AddLog("%s", s); });

    // Start the listener
    g_Log.AddLog("Starting listener\n");    
    try {
        //auto jsConfig = configServer.get_net_cfg(listener_config);
        
        json addendum = json({ {"server_address", {
                {"ip", listenerConfig.common.server_address.ip},
                {"port", listenerConfig.common.server_address.port }}}});


        std::cout << "Starting with added json: " << addendum.dump(4) << "\n";
        
        listenerState.initialize(listenerConfig);
        is_listener_running = true;
    }
    catch (std::filesystem::filesystem_error e) {
        g_Log.AddLog("Exception (filesystem): %s\n", e.what());
    }
    catch (json::exception e) {
        g_Log.AddLog("Exception (json): %s\n", e.what());
    }
    catch (std::exception e) {
        g_Log.AddLog("Exception: %s\n", e.what());
    }
}