module;

#include <cmath>
extern "C" {
#include <libavutil/time.h>
}

module utils.Clock;

void Clock::Set(double pts, int serial)
{
	SetAt(pts, serial, av_gettime_relative() / AVClockBase);
}

double Clock::Get() const
{
	if (m_queue_serial && *m_queue_serial != m_serial)
		return NAN;

	if (m_paused)
		return m_pts;

	double const time = av_gettime_relative() / AVClockBase;
	return m_pts_drift + time - (time - m_last_updated) * (1.0 - m_speed);
}

void Clock::SetSpeed(double speed)
{
	Set(Get(), m_serial);
	m_speed = speed;
}
