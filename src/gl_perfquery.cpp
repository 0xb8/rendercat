#include <rendercat/gl_perfquery.hpp>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/functions.h>

using namespace gl45core;

PerfQuery::PerfQuery()
{
	glCreateQueries(GL_TIME_ELAPSED, 1, &m_gpu_time_query);
}

PerfQuery::~PerfQuery()
{
	glDeleteQueries(1, &m_gpu_time_query);
}

void PerfQuery::begin()
{
	glBeginQuery(GL_TIME_ELAPSED, m_gpu_time_query);
}

void PerfQuery::end()
{
	glEndQuery(GL_TIME_ELAPSED);
}

void PerfQuery::collect()
{
	uint64_t time_elapsed = 0;
	glGetQueryObjectui64v(m_gpu_time_query, GL_QUERY_RESULT_NO_WAIT, &time_elapsed);
	float gpu_time = (double)time_elapsed / 1000000.0;
	time_avg = 0.0f;

	for(unsigned i = 0; i < std::size(times)-1; ++i) {
		times[i] = times[i+1];
		time_avg += times[i+1];
	}
	times[std::size(times)-1] = gpu_time;
	time_avg += gpu_time;
	time_avg /= std::size(times);
	time_last = gpu_time;
}


