#pragma once

struct PerfQuery
{
	PerfQuery();
	~PerfQuery();

	PerfQuery(const PerfQuery&) = delete;
	PerfQuery& operator =(const PerfQuery&) = delete;

	PerfQuery(PerfQuery&& o) noexcept;
	PerfQuery& operator =(PerfQuery&& o) noexcept;

	static constexpr unsigned num_samples = 64u;

	float times[num_samples] = {0.0f};
	float time_avg = 0.0f;
	float time_last = 0.0f;

	void begin();
	void end();

	void collect();

private:
	unsigned m_gpu_time_query;
	unsigned m_state;
};
