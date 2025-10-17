#include "stdafx.h"
#include "SubsStatsMgr.h"
#include "App.h"
#include "imgui.h"
#include "TopicRW.h"
#include "SharedData.h"
#include "NetworkDefs.h"

namespace DdsPerfTest
{
	SubsStatsMgr::SubsStatsMgr(App* app)
	: _app(app)
	{
		int participant = app->GetParticipant(0);
		_subsStatRW = std::make_shared<TopicRW>(participant, "SubsStats", &Net_SubsStats_desc, DDS_RELIABILITY_RELIABLE, DDS_DURABILITY_TRANSIENT_LOCAL, DDS_HISTORY_KEEP_LAST, 1);
	}

	void SubsStatsMgr::Tick()
	{
		ReadSubsStats();
	}

    void SubsStatsMgr::DrawUI()
    {
		DrawSubsStats();
    }

	void SubsStatsMgr::DrawSubsStats()
	{
		// show the stats in ImGui table (new style table) - increased column count to 8 for computer name
		if (ImGui::BeginTable("SubsStats", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_SortMulti | ImGuiTableFlags_SortTristate))
		{
			// sort stats
			std::vector<SubsStats> stats;
			stats.reserve(_subsStats.size());
			for (auto& kv : _subsStats) stats.push_back(kv.second);
			std::sort(stats.begin(), stats.end(), [](const SubsStats& a, const SubsStats& b) {
				if (a.Key.MsgName != b.Key.MsgName) return a.Key.MsgName < b.Key.MsgName;
				if (a.Key.AppIndex != b.Key.AppIndex) return a.Key.AppIndex < b.Key.AppIndex;
				if (a.Key.InAppIndex != b.Key.InAppIndex) return a.Key.InAppIndex < b.Key.InAppIndex;
				return false;
				});

			// header - added Partition and Computer columns
			ImGui::TableSetupColumn("Msg");
			ImGui::TableSetupColumn("Computer");
			ImGui::TableSetupColumn("App");
			ImGui::TableSetupColumn("Idx");
			ImGui::TableSetupColumn("Partition");  // NEW: Partition column
			ImGui::TableSetupColumn("Recv");
			ImGui::TableSetupColumn("Rate");
			ImGui::TableSetupColumn("Lost");
			ImGui::TableHeadersRow();

			for (auto& s : stats)
			{
				const auto& apps = _app->GetApps();
				std::string computerName = "[unknown]";
				if (s.Key.AppIndex >= 0 && s.Key.AppIndex < (int)apps.size())
				{
					computerName = apps[s.Key.AppIndex].ComputerName;
				}

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("%s", s.Key.MsgName.c_str());
				ImGui::TableNextColumn();
				ImGui::Text("%s", computerName.c_str());
				ImGui::TableNextColumn();
				ImGui::Text("%d", s.Key.AppIndex);
				ImGui::TableNextColumn();
				ImGui::Text("%d", s.Key.InAppIndex);
				ImGui::TableNextColumn();
				ImGui::Text("%s", s.PartitionName.c_str());  // NEW: Display partition name
				ImGui::TableNextColumn();
				ImGui::Text("%d", s.Received);
				ImGui::TableNextColumn();
				ImGui::Text("%d", s.Rate);
				ImGui::TableNextColumn();
				ImGui::Text("%d", s.Lost);
			}
			ImGui::EndTable();
		}

	}

	void SubsStatsMgr::ReadSubsStats()
	{
		const int MAX_SAMPLES = 10;
		Net_SubsStats* samples[MAX_SAMPLES] = { 0 }; // we want DDS to allocate memory for us (we do not need to care about freeing it)
		dds_sample_info_t infos[MAX_SAMPLES];

		int num = dds_take(_subsStatRW->GetReader(), (void**)samples, infos, MAX_SAMPLES, MAX_SAMPLES);
		if (num == DDS_RETCODE_NO_DATA)
			return;
		if (num < 0)
			DDS_FATAL("dds_read(SubsStats)): %s\n", dds_strretcode(-num));

		for (int i = 0; i < num; i++)
		{
			Net_SubsStats* sample = (Net_SubsStats*)samples[i];

			PubSubKey key;
			key.AppIndex = sample->AppIndex;
			key.MsgName = sample->MsgClass;
			key.InAppIndex = sample->InAppIndex;

			// if disposed sample, remove from map
			if ((infos[i].instance_state & DDS_ALIVE_INSTANCE_STATE) == 0)
			{
				_subsStats.erase(key);
				continue;
			}
			else
			{
				SubsStats stats;
				stats.Key = key;
				stats.PartitionName = sample->PartitionName ? sample->PartitionName : "";  // NEW: Store partition name
				stats.Rate = sample->Rate;
				stats.Lost = sample->Lost;
				stats.Received = sample->Received;

				// store
				_subsStats[stats.Key] = stats;
			}
		}

	}
	int SubsStatsMgr::GetSubsStatsWriter() const { return _subsStatRW->GetWriter(); }

}