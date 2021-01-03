#include <rendercat/util/gl_perfquery.hpp>
#include <rendercat/util/gl_debug.hpp>
#include <glbinding/gl/types.h>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/functions.h>
#include <cassert>
#include <utility>
#include <fmt/core.h>

using namespace gl45core;
using namespace rc;

PerfQuery::PerfQuery()
{
	for(int i = 0; i < query_count; ++i) {
		glCreateQueries(GL_TIME_ELAPSED, 1, m_query[i].get());
		rcObjectLabel(m_query[i], fmt::format("perfquery[{}]", i));

		m_state[i] = QueryState::ResultAvailable;
	}
}

void PerfQuery::begin()
{
	int qidx = next_query();
	m_state[qidx] = QueryState::Began;
	glBeginQuery(GL_TIME_ELAPSED, *m_query[qidx]);
	current_query = qidx;
}

void PerfQuery::end()
{
	assert((m_state[current_query] == QueryState::Began) && "Must begin() query before calling end()");
	m_state[current_query] = QueryState::Ended;
	glEndQuery(GL_TIME_ELAPSED);
}

float PerfQuery::get()
{
	assert((m_state[current_query] == QueryState::Ended) && "Must end() query before calling get()");
	uint64_t time_elapsed = 0;
	glGetQueryObjectui64v(*m_query[current_query], GL_QUERY_RESULT, &time_elapsed);
	if(!time_elapsed)
		return -1.0f;

	float gpu_time = (double)time_elapsed / 1000000.0;
	return gpu_time;
}


void PerfQuery::collect()
{
	for(int i = 0; i < query_count; ++i) {
		if(m_state[i] != QueryState::Ended)
			continue;

		uint64_t time_elapsed = 0;
		glGetQueryObjectui64v(*m_query[i], GL_QUERY_RESULT_NO_WAIT, &time_elapsed);
		if(!time_elapsed)
			continue;

		m_state[i] = QueryState::ResultAvailable;
		float gpu_time = (double)time_elapsed / 1000000.0;
		push_time(gpu_time, i);
	}

}

void PerfQuery::push_time(float t, int q)
{
	time_avg = 0.0f;
	time_last = t;
	query_num = q;

	for(unsigned i = 0; i < std::size(times)-1; ++i) {
		times[i] = times[i+1];
		time_avg += times[i+1];
	}
	times[std::size(times)-1] = t;
	time_avg += t;
	time_avg /= std::size(times);

}

int PerfQuery::next_query()
{
	for(int i = 0; i < query_count; ++i) {
		if(m_state[i] == QueryState::ResultAvailable)
			return i;
	}
	assert(false && "no vacant queries left, increase count!");
	return -1;
}


