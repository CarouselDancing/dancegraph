#include "listener.h"

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h> 

#include <core/common/utility.h>

#include "log.h"
#include "common.h"

// Static, so that we can set it as a callback to the general net library logger
static ExampleAppLog g_Log;

using namespace nlohmann;

namespace net
{
    ListenerGui::ListenerGui(const char* application_name)
        :application_name(application_name)
    {
        auto text = dancenet::readTextFile(std::string(application_name) + ".json");
        if (!text.empty())
        {
            json j = json::parse(text);
            config = j;
        }
    }

    void ListenerGui::init_gui()
    {
        ImGui::InputText("Server IP", &config.server_address.ip, ImGuiInputTextFlags_CharsDecimal);
        // Port
        ImGui::DragInt("Server port", &config.server_address.port, 1, 0, 65536);

        if (ImGui::Button("Connect to server"))
        {
            // validate ip
            IN_ADDR addr;
            if (inet_pton(AF_INET, config.server_address.ip.c_str(), &addr) != 1)
            {
                g_Log.AddLog("Error: Invalid IP address: %s\n", config.server_address.ip.c_str());
                return;
            }

            // Save config to server-gui.json
            dancenet::writeTextFile(std::string(application_name) + ".json", json(config).dump(4));

            // hook the network log to our gui log
            set_log_callback([](const char* s) { g_Log.AddLog("%s", s); });

            // Start the listener
            g_Log.AddLog("Starting listener\n");
            initialize(config);
        }
    }

    void ListenerGui::on_gui()
    {
        static const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
        ImGui::SetNextWindowPos({ 0,0 });
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        if (ImGui::Begin("main", nullptr, flags))
        {
            if (ImGui::BeginTable("table", 2))
            {
                ImGui::TableNextColumn();
                imgui::GlobalLogLevel();
                if (!is_initialized)
                    init_gui();
                else
                    run_gui();
                ImGui::TableNextColumn();
                g_Log.Draw();
                ImGui::EndTable();
            }
            ImGui::End();
        }
    }
}