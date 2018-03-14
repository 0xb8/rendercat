#pragma once
#include <rendercat/util/gl_unique_handle.hpp>

namespace rc {

struct PerfQuery
{
	PerfQuery();
	~PerfQuery() = default;

	PerfQuery(const PerfQuery&) = delete;
	PerfQuery& operator =(const PerfQuery&) = delete;

	PerfQuery(PerfQuery&& o) noexcept = default;
	PerfQuery& operator =(PerfQuery&& o) noexcept = default;

	static constexpr unsigned num_samples = 64u;

	float times[num_samples] = {0.0f};
	float time_avg = 0.0f;
	float time_last = 0.0f;
	int   query_num = 0;

	void begin();
	void end();

	void collect();

private:
	static constexpr int query_count = 8;
	enum class QueryState {
		Began,
		Ended,
		ResultAvailable
	};

	void push_time(float t, int q);
	int  next_query();
	int  current_query = 0;

	query_handle m_query[query_count];
	QueryState   m_state[query_count];
};

} // namespace rc
