#include "stdafx.h"
#include "DataMgr.h"

namespace DdsPerfTest
{
	DataMgr::DataMgr( App* app, std::function<void(const SharedData&)> onReceived )
	{
        _app = app;
        _onReceived = onReceived;

        int participant = _app->GetParticipant(0);
        _sharedDataRW = std::make_shared<TopicRW>(participant, "MasterSettings", &Net_MasterSettings_desc, DDS_RELIABILITY_RELIABLE, DDS_DURABILITY_TRANSIENT_LOCAL, DDS_HISTORY_KEEP_LAST, 1);
    }

    void DataMgr::Tick()
    {
        ReadSettings();
    }

    void DataMgr::Send()
    {
		SendSettings(_local);
	}


    void DataMgr::ReadSettings()
    {
        const int MAX_SAMPLES = 10;
        Net_MasterSettings* samples[MAX_SAMPLES] = {}; // we want DDS to allocate memory for us (we do not need to care about freeing it)
        dds_sample_info_t infos[MAX_SAMPLES];

        int num = dds_take(_sharedDataRW->GetReader(), (void**)samples, infos, MAX_SAMPLES, MAX_SAMPLES);
        if (num == DDS_RETCODE_NO_DATA)
            return;
        if (num < 0)
            DDS_FATAL("dds_read(MasterSettings)): %s\n", dds_strretcode(-num));

        for (int i = 0; i < num; i++)
        {

            auto& sample = *samples[i];
            auto& info = infos[i];

            if (!(info.instance_state & DDS_ALIVE_INSTANCE_STATE))
                continue;

            SharedData shd;

            shd.Sender.ComputerName = sample.Sender.ComputerName;
            shd.Sender.ProcessId = sample.Sender.ProcessId;

            shd.Disabled = sample.Disabled;

            for (int j = 0; j < (int)sample.Apps._length; j++)
            {
                auto& app = sample.Apps._buffer[j];
                shd.Apps.push_back(AppId(app.ComputerName, app.ProcessId));
            }

            for (int j = 0; j < (int)sample.Msgs._length; j++)
            {
                auto& net = sample.Msgs._buffer[j];

                std::vector<int> subsCnt;
                for (int k = 0; k < (int)net.SubsCnt._length; k++)
                {
                    auto& elem = net.SubsCnt._buffer[k];
                    subsCnt.push_back(elem);
                }

                std::vector<int> publCnt;
                for (int k = 0; k < (int)net.PublCnt._length; k++)
                {
                    auto& elem = net.PublCnt._buffer[k];
                    publCnt.push_back(elem);
                }

                MsgSettings msg(net.Name, net.Disabled, net.Opened, net.Rate, net.Size, subsCnt, publCnt, net.AllSubsDisabled, net.AllPublDisabled);
                shd.Msgs[msg.Name] = msg;
            }

            _received = shd;

            _onReceived( shd );
        }
    }

    void DataMgr::SendSettings(SharedData& settings)
    {
        Net_MasterSettings sample = {};

        sample.Sender.ComputerName = (char*)_app->GetAppId().ComputerName.c_str();
        sample.Sender.ProcessId = _app->GetAppId().ProcessId;

        sample.Disabled = settings.Disabled;

        sample.Apps._length = (int)settings.Apps.size();
        sample.Apps._maximum = (int)settings.Apps.size();
        sample.Apps._buffer = new Net_AppId[settings.Apps.size()];
        for (int i = 0; i < (int)settings.Apps.size(); i++)
        {
            sample.Apps._buffer[i].ComputerName = (char*)settings.Apps[i].ComputerName.c_str();
            sample.Apps._buffer[i].ProcessId = settings.Apps[i].ProcessId;
        }

        for (int i = 0; i < (int)settings.Msgs.size(); i++)
        {
            sample.Msgs._length = (int)settings.Msgs.size();
            sample.Msgs._maximum = (int)settings.Msgs.size();
            sample.Msgs._buffer = new Net_MsgSpec[settings.Msgs.size()];
            int j = 0;
            for (auto& msg : settings.Msgs)
            {
                auto& p = sample.Msgs._buffer[j];
                auto& q = msg.second;
                p.Disabled = q.Disabled;
                p.Opened = q.Opened;
                p.Name = (char*)q.Name.c_str();
                p.Rate = q.Rate;
                p.Size = q.Size;

                p.SubsCnt._length = (int)q.SubsCnt.size();
                p.SubsCnt._maximum = (int)q.SubsCnt.size();
                p.SubsCnt._release = true;
                p.SubsCnt._buffer = new int[q.SubsCnt.size()];
                for (int k = 0; k < (int)q.SubsCnt.size(); k++)
                    p.SubsCnt._buffer[k] = q.SubsCnt[k];

                p.PublCnt._length = (int)q.PublCnt.size();
                p.PublCnt._maximum = (int)q.PublCnt.size();
                p.PublCnt._release = true;
                p.PublCnt._buffer = new int[q.PublCnt.size()];
                for (int k = 0; k < (int)q.PublCnt.size(); k++)
                    p.PublCnt._buffer[k] = q.PublCnt[k];

                p.AllSubsDisabled = q.AllSubsDisabled;
                p.AllPublDisabled = q.AllPublDisabled;

                j++;
            }
        }


        int res = dds_write(_sharedDataRW->GetWriter(), &sample);
        if (res < 0)
            DDS_FATAL("dds_write(MasterSettings): %s\n", dds_strretcode(-res));
    }

    void DataMgr::SaveSettings( const SharedData& settings )
    {
        FILE* f = fopen("settings.csv", "w");
        if (!f) return;
        fprintf(f, "Name,Disabled,Opened,Rate,Size,AllPublDisabled,AllSubsDisabled\n");
        for (auto& kv : settings.Msgs)
        {
            auto& msg = kv.second;
            fprintf(f, "%s,%d,%d,%d,%d,%d,%d,", msg.Name.c_str(), msg.Disabled, msg.Opened, msg.Rate, msg.Size, msg.AllPublDisabled, msg.AllSubsDisabled);

            for (int i = 0; i < MAX_APPS; i++)
            {
                if (i < (int)msg.PublCnt.size())
                    fprintf(f, "%d,", msg.PublCnt[i]);
                else
                    fprintf(f, "0,");
            }
            for (int i = 0; i < MAX_APPS; i++)
            {
                if (i < (int)msg.SubsCnt.size())
                    fprintf(f, "%d,", msg.SubsCnt[i]);
                else
                    fprintf(f, "0,");
            }

            fprintf(f, "\n");
        }
        fclose(f);
    }

    void DataMgr::RestoreSettings(SharedData& settings)
    {
        FILE* f = fopen("settings.csv", "r");
        if (!f) return;

        // read line by line
        settings.Msgs.clear();
        int lineNo = 0;
        while (!feof(f))
        {
            char line[1024];
            while (fgets(line, sizeof(line), f))
            {
                lineNo++;
                if (lineNo == 1) continue; // skip the header line

                MsgSettings msg;
                char name[1000] = {};
                int opened = 0;
                int publs[MAX_APPS] = {};
                int subss[MAX_APPS] = {};
                int allPublDisabled = 0;
                int allSubsDisabled = 0;

                int numScanned = sscanf(line, "%[^,],%d,%d,%d,%d,%d,%d,"
                    "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d," // MAX_APPS !!!
                    "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,",// MAX_APPS !!!
                    name, &msg.Disabled, &opened, &msg.Rate, &msg.Size, &allPublDisabled, &allSubsDisabled,
                    &publs[0], &publs[1], &publs[2], &publs[3], &publs[4], &publs[5], &publs[6], &publs[7], &publs[8], &publs[9],
                    &subss[0], &subss[1], &subss[2], &subss[3], &subss[4], &subss[5], &subss[6], &subss[7], &subss[8], &subss[9]
                );

                if (numScanned == 7 + MAX_APPS + MAX_APPS)
                {
                    msg.Name = name;
                    msg.Opened = opened;
                    msg.AllPublDisabled = allPublDisabled;
                    msg.AllSubsDisabled = allSubsDisabled;

                    for (int i = 0; i < MAX_APPS; i++)
                        msg.PublCnt.push_back(publs[i]);

                    for (int i = 0; i < MAX_APPS; i++)
                        msg.SubsCnt.push_back(subss[i]);

                    settings.Msgs[msg.Name] = msg;
                }
            }
        }
        fclose(f);

    }

    void DataMgr::SaveSettings()
    {
        SaveSettings( _local );
    }

    void DataMgr::RestoreSettings()
    {
        RestoreSettings( _local );
    }

    void DataMgr::SendAndSaveIfDirty()
    {
        if( _localDirty )
		{
			Send();
			SaveSettings();
			_localDirty = false;
		}
    }


}