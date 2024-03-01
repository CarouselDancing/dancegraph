#include "server.h"

#include <inttypes.h>

#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h> 
#include <core/common/utility.h>
#include <core/net/stats.h>
#include <core/net/config_master.h>

#include "log.h"
#include "common.h"

#include <chrono>

//#define SIGGRAPH_DEMO
#define TELEMETRY_BTN
 


// Static, so that we can set it as a callback to the general net library logger
static ExampleAppLog g_Log;

namespace imgui
{
    void ThroughputStats(const char * label, const net::ThroughputStats& stats, double elapsedSec)
    {
        auto total = stats.bytesIn + stats.bytesOut;
        uint64_t perSec = elapsedSec > 0 ? uint64_t(total / elapsedSec) : 0;
        ImGui::LabelText(label, "%I64u|%I64u = IN:%I64u + OUT:%I64u", perSec, total, stats.bytesIn, stats.bytesOut);
    }

    void VecThroughputStats(const std::vector<net::ThroughputStats>& vec, const char * name, double elapsedSec)
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

    
    /* make_world_summary(const WorldState& ws, const std::vector<ClientDataInServer>& clients)
    {
        int numConnected = 0;
        for (const auto& client : clients)
            if (client.is_connected())
                ++numConnected;
        auto s = fmt::format("WorldState clients: {} registered / {} connected:\n", clients.size(), numConnected);
        assert(clients.size() == ws.clients.size());
        auto it_client = clients.begin();
        for (const auto& client : ws.clients)
        {
            auto q = fmt::format("\t{} at {}, last seen at {}{}\n", client.name, client.address, sig::time_point_as_string(it_client->last_seen_time_point), it_client->is_connected() ? "" : " (DISCONNECTED)");;
            s += q;
            ++it_client;
        }
        //s += fmt::format("Environment: {}\n", ws.environment.to_string());
        return s;
    }
    */

}

namespace net
{
    void DumpWorldState(const char* label, const net::WorldState& ws, const std::vector<net::ClientDataInServer>& clients, const ServerState& serverState)
    {
        int numConnected = 0;
        for (const auto& client : clients)
            if (client.is_connected())
                ++numConnected;

        auto s = fmt::format("WorldState clients: {} registered / {} connected:\n", clients.size(), numConnected);
        ImGui::LabelText(label, s.c_str());

        assert(clients.size() == ws.clients.size());
        auto it_client = clients.begin();

        int i = 0;
        for (const auto& client : ws.clients)
        {
            auto client_to_server_time_offset = client.time_offset_ms_ntp - serverState.get_time_offset_ms_ntp();
            auto q = fmt::format("\t#{}: {} @ {}, time offset {}, last seen {}{}\n", i++, client.name, client.address, client_to_server_time_offset, sig::time_point_as_string(it_client->last_seen_time_point), it_client->is_connected() ? "" : " (DISCONNECTED)");;
            ImGui::PushID(i);
            ImGui::LabelText(label, q.c_str());
            ++it_client;
            ImGui::PopID();
        }
    }

    ServerGui::ServerGui(const char* application_name)
        :application_name(application_name)
    { 
        spdlog::default_logger()->sinks().emplace_back(std::make_shared<spdlog::sinks::callback_sink_mt>([](const spdlog::details::log_msg& msg) {
            g_Log.AddLog("[%s] %s\n", magic_enum::enum_name(msg.level).data(), std::string(std::string_view(msg.payload.data(), msg.payload.size())).c_str());
            }));
        auto logFile = fmt::format("{}/log_{}.txt", net::cfg::DanceGraphAppDataPath(), application_name);
        spdlog::default_logger()->sinks().emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile));

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

    void ServerGui::init_gui()
    {


        ImGui::InputText("IP", &config.address.ip, ImGuiInputTextFlags_CharsDecimal);
        // Port
        ImGui::DragInt("Port", &config.address.port, 1, 0, 65536);

        ImGui::InputText("Scene", &config.scene);

        if (ImGui::Button("Start server"))
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

    void ServerGui::run_gui()
    {
        auto now = sig::time_now();

#ifdef SIGGRAPH_DEMO
        auto& state = world.environment.musicState;
        auto& is_music_on = state.mBody.isPlaying;
        if (ImGui::Button(fmt::format("Turn music {}", is_music_on ? "OFF" : "ON").c_str()))
        {
            is_music_on = !is_music_on;
            static time_point time_of_first_start = sig::time_now(); // first time it WILL be on
            static time_point time_of_last_start;
            time_point time_now = sig::time_now();
            if (is_music_on) // when we turn it on, just play from the current musicTime
            {
                time_of_last_start = time_now;
            }
            else // when we turn it off, add the elapsed time since last start to calculate the new "on-resume" music time
            {
                state.mBody.musicTime += std::chrono::duration_cast<std::chrono::microseconds>(time_now - time_of_last_start).count();
            }
            
            for (size_t userIdx = 0; userIdx < clients.size(); ++userIdx)
            {
                if (clients[userIdx].is_connected())
                {
                    net::send_env_msg<EnvMusicState>(clients[userIdx].peer_per_channel[(int)SignalType::Environment], state, (int16_t)userIdx, time_now, &stats);
                }
            }
        }
#endif

#ifdef TELEMETRY_BTN
        if (ImGui::Button("Telemetry capture"))
        {
            auto server_start_time = start_time_point;
            request_telemetry([server_start_time](const net::TelemetryData& tdLatest) {
                g_Log.AddLog("Logging telemetry\n");
                static FILE* fp = nullptr;
                if (fp == nullptr)
                {
                    const char * filename = std::filesystem::exists("e:/dancegraph") ? "e:/dancegraph/dancegraph-telemetry.csv" : "dancegraph-telemetry.csv";
                    fp = fopen(filename, "a");
                    fprintf(fp, "SigType, SigIdx, UserIdx, PacketId, TelemetryCapturePoint, TimePoint, EpochReps\n");
                }
                
                if (fp != nullptr)
                    for (const auto& [key, value] : tdLatest)
                        fprintf(fp, "%d, %d, %d, %d, %s, %s, %" PRId64 "\n", key.sigType, key.sigIdx, key.userIdx, key.packetId, TelemetryCapturePointString(key.telemetryCapturePoint).data(), sig::time_point_as_string(value).c_str(), (value- server_start_time).count());
                
            });
        }
#endif
        


        static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
        {
            ImGuiTabItemFlags tab_item_flags = 0;


            if (ImGui::BeginTabItem("Connections", nullptr, tab_item_flags))
            {
                if (ImGui::TreeNode("Server Status"))
                {
                    DumpWorldState("", world, clients, *this);
                    ImGui::TreePop();
                }

                for (size_t i = 0; i < clients.size(); ++i)
                {
                    ImGui::PushID(i); // this is to allow us to have multiple controls inside this look with the same name
                    auto& client = clients[i];


                    if (ImGui::TreeNode(client.name.c_str()))
                    {
                        // Sync info
                        if(!client.sync_info.synced)
                            ImGui::LabelText("Delay (ms)","Calculating...(%d left)", client.sync_info.times_left);
                        else
                            ImGui::LabelText("Delay (ms)", "%I64u", client.sync_info.delay_ms);
                        // Last seen
                        auto last_seen_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - client.last_seen_time_point).count();
                        ImGui::LabelText("Last seen", "%I64 ms ago", last_seen_elapsed_ms);

                        ImGui::LabelText("NTP offset", "%f ms", client.time_offset_ms_ntp);

                        // server stats
                        imgui::ServerStats(stats);
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    void ServerGui::on_gui()
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