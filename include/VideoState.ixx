export module ffplay:VideoState;

import :SyncType;
import :ReadThread;
import utils.Clock;
import utils.PacketQueue;
import utils.FrameQueue;

export class VideoState
{
public:

private:
	constexpr static auto VIDEO_PICTURE_QUEUE_SIZE = 3;
	constexpr static auto SUBPICTURE_QUEUE_SIZE = 16;
	constexpr static auto SAMPLE_QUEUE_SIZE = 9;


	SyncType m_avSyncType{ SyncType::AudioMaster };

	Clock m_audioClock;
	Clock m_videoClock;
	Clock m_externalClock;

	PacketQueue videoq;
	PacketQueue audioq;
	PacketQueue subtitleq;

	FrameQueue<VIDEO_PICTURE_QUEUE_SIZE> pictq{ videoq, true };
	FrameQueue<SUBPICTURE_QUEUE_SIZE> subpq{ subtitleq, false };
	FrameQueue<SAMPLE_QUEUE_SIZE> sampq{ audioq, true };

	void readThreadProc();

	void audioThreadProc();
	void videoThreadProc();
	void subtitleThreadProc();



	void openStreamComponent(int streamIndex);


	//constexpr SyncType getMasterSyncType() const
	//{
	//	if (m_avSyncType == SyncType::VideoMaster)
	//	{
	//		if (m_videoStream)
	//			return SyncType::VideoMaster;
	//		else
	//			return SyncType::AudioMaster;
	//	}
	//	else if (m_avSyncType == SyncType::AudioMaster)
	//	{
	//		if (m_audioStream)
	//			return SyncType::AudioMaster;
	//		else
	//			return SyncType::External;
	//	}
	//	else
	//		return SyncType::External;
	//}

	double getMasterClock() const;
};