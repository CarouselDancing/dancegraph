#include "client.h"

#include <spdlog/sinks/callback_sink.h>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h> 
#include <core/common/utility.h>
#include <core/net/stats.h>
#include <core/net/config_master.h>

#include "log.h"
#include "common.h"

// Static, so that we can set it as a callback to the general net library logger
static ExampleAppLog g_Log;

namespace imgui
{
    void ThroughputStats(const char* label, const net::ThroughputStats& stats, double elapsedSec)
    {
        auto total = stats.bytesIn + stats.bytesOut;
        uint64_t perSec = elapsedSec > 0 ? uint64_t(total / elapsedSec) : 0;
        ImGui::LabelText(label, "%I64u|%I64u = IN:%I64u + OUT:%I64u", perSec, total, stats.bytesIn, stats.bytesOut);
    }

    void VecThroughputStats(const std::vector<net::ThroughputStats>& vec, const char* name, double elapsedSec)
    {
        if (ImGui::TreeNode(name))
        {
            for (size_t i = 0; i < vec.size(); ++i)
                ThroughputStats(fmt::format("{} #{}", name, i).c_str(), vec[i], elapsedSec);
            ImGui::TreePop();
        }
    }

    void ServerStats(net::ServerStats& stats)
    {
        static sig::time_point timeBase = sig::time_now();
        if (ImGui::Button("Reset"))
        {
            stats.ResetThroughput();
            timeBase = sig::time_now();
        }
        auto elapsedSec = std::chrono::duration_cast<std::chrono::microseconds>(sig::time_now() - timeBase).count() * 0.000001;
        ThroughputStats("total", stats.throughputTotal, elapsedSec);
        VecThroughputStats(stats.throughputPerClient, "Client", elapsedSec);
        VecThroughputStats(stats.throughputPerListener, "Listener", elapsedSec);
        VecThroughputStats(stats.throughputPerClientSignal, "Client sig", elapsedSec);
        VecThroughputStats(stats.throughputPerEnvSignal, "Env sig", elapsedSec);
    }
}

namespace net
{
    ClientGui::ClientGui(const char* application_name)
        :application_name(application_name)
    {
        spdlog::default_logger()->sinks().emplace_back(std::make_shared<spdlog::sinks::callback_sink_mt>([](const spdlog::details::log_msg& msg) {
            g_Log.AddLog("[%s] %s\n", magic_enum::enum_name(msg.level).data(), std::string(std::string_view(msg.payload.data(), msg.payload.size())).c_str());
            }));

        if (!net::initialize())
            exit(-1);

        if (!cfg::Root::load())
        {
            spdlog::error("Failed loading dancegraph.json");
            exit(-1);
        }

        auto text = dancenet::readTextFile(std::string(application_name) + ".json");
        if (!text.empty())
        {
            nlohmann::json j = nlohmann::json::parse(text);
            config = j;
        }
    }

    void ClientGui::init_gui()
    {
        ImGui::InputText("IP", &config.address.ip, ImGuiInputTextFlags_CharsDecimal);
        // Port
        ImGui::DragInt("Port", &config.address.port, 1, 0, 65536);

        ImGui::InputText("Scene", &config.scene);

        if (ImGui::Button("Start client"))
        {
            // validate ip
            IN_ADDR addr;
            if (inet_pton(AF_INET, config.address.ip.c_str(), &addr) != 1)
            {
                g_Log.AddLog("Error: Invalid IP address: %s\n", config.address.ip.c_str());
                return;
            }

            // Save config to server-gui.json
            dancenet::writeTextFile(std::string(application_name) + ".json", nlohmann::json(config).dump(4));

            // hook the network log to our gui log
            set_log_callback([](const char* s) { g_Log.AddLog("%s", s); });

            // Start the server
            g_Log.AddLog("Starting server\n");
            bool ok = initialize(config);
        }
    }

    void ClientGui::run_gui()
    {
        auto now = sig::time_now();
        if (is_initialized)
            tick();

        static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
        {

            ImGuiTabItemFlags tab_item_flags = 0;
            if (ImGui::BeginTabItem("Connections", nullptr, tab_item_flags))
            {
#if 0
                for (size_t i = 0; i < clients.size(); ++i)
                {
                    ImGui::PushID(i); // this is to allow us to have multiple controls inside this look with the same name
                    auto& client = clients[i];
                    if (ImGui::TreeNode(client.name.c_str()))
                    {
                        // Sync info
                        if (!client.sync_info.synced)
                            ImGui::LabelText("Delay (ms)", "Calculating...(%d left)", client.sync_info.times_left);
                        else
                            ImGui::LabelText("Delay (ms)", "%I64u", client.sync_info.delay_ms);
                        // Last seen
                        auto last_seen_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - client.last_seen_time_point).count();
                        ImGui::LabelText("Last seen", "%I64 ms ago", last_seen_elapsed_ms);
                        // server stats
                        imgui::ServerStats(stats);
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
#endif
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    void ClientGui::on_gui()
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