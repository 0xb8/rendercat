#pragma once

#include <glbinding/gl/types.h>

struct PerfQuery
{
	PerfQuery();
	~PerfQuery();

	PerfQuery(const PerfQuery&) = delete;
	PerfQuery& operator =(const PerfQuery&) = delete;

	PerfQuery(PerfQuery&& o) noexcept
	{
		this->operator=(std::move(o));
	}

	PerfQuery& operator =(PerfQuery&& o) noexcept
	{
		std::swap(m_gpu_time_query, o.m_gpu_time_query);
		time_avg = o.time_avg;
		time_last = o.time_last;
		memcpy(times, o.times, sizeof(times));
		return *this;
	}

	static constexpr size_t num_samples = 64u;


	float times[num_samples] = {0.0f};
	float time_avg = 0.0f;
	float time_last = 0.0f;

	void begin();
	void end();

	void collect();


private:
	gl::GLuint m_gpu_time_query;
};
