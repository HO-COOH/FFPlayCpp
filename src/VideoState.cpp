module;

module ffplay;
import :VideoState;

double VideoState::getMasterClock() const
{
	switch (getMasterSyncType())
	{
		case SyncType::VideoMaster: return m_videoClock.Get();
		case SyncType::AudioMaster: return m_audioClock.Get();
		default: return m_externalClock.Get();
	}
}
