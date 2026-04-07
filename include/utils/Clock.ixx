export module utils.Clock;

export class Clock
{
    double m_pts{};           /* clock base */
    double m_pts_drift{};     /* clock base minus time at which we updated the clock */
    double m_last_updated{};
    double m_speed = 1.0;
    int m_serial{};           /* clock is based on a packet with this serial */
    bool m_paused{};
    int* m_queue_serial{};

public:
    constexpr static double AVClockBase = 1000000.0;
    constexpr void SetAt(double pts, int serial, double time)
    {
        m_pts = pts;
        m_last_updated = time;
        m_pts_drift = m_pts - time;
        m_serial = serial;
    }

    void Set(double pts, int serial);
    void SetSpeed(double speed);

    double Get() const;
};