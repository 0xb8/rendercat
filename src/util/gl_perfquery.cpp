#include <rendercat/util/gl_perfquery.hpp>
#include <glbinding/gl/types.h>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/functions.h>
#include <cassert>
#include <utility>

using namespace gl45core;

static const unsigned PQ_WAIT = 0;
static const unsigned PQ_RUN  = 1;

PerfQuery::PerfQuery()
{
	static_assert(sizeof(gl::GLuint) == sizeof(m_gpu_time_query));

	glCreateQueries(GL_TIME_ELAPSED, 1, &m_gpu_time_query);
}

PerfQuery::~PerfQuery()
{
	glDeleteQueries(1, &m_gpu_time_query);
}

PerfQuery::PerfQuery(PerfQuery&& o) noexcept :
	time_avg(o.time_avg),
	time_last(o.time_last),
	m_gpu_time_query(std::exchange(o.m_gpu_time_query, 0u)),
        m_state(o.m_state)

{
	for(unsigned i = 0; i < num_samples; ++i) // compiler should replace this with memcpy
		times[i] = o.times[i];
}

PerfQuery& PerfQuery::operator =(PerfQuery&& o) noexcept
{
	if(this != &o) {
		glDeleteQueries(1, &m_gpu_time_query);
		m_gpu_time_query = std::exchange(o.m_gpu_time_query, 0);
		m_state = o.m_state;
		time_avg = o.time_avg;
		time_last = o.time_last;
		for(unsigned i = 0; i < num_samples; ++i) // compiler should replace this with memcpy
			times[i] = o.times[i];
	}
	return *this;
}

void PerfQuery::begin()
{
	assert((m_state == PQ_WAIT) && "Must end() query before calling begin()");
	m_state = PQ_RUN;
	glBeginQuery(GL_TIME_ELAPSED, m_gpu_time_query);
}

void PerfQuery::end()
{
	assert((m_state == PQ_RUN) && "Must begin() query before calling end()");
	m_state = PQ_WAIT;
	glEndQuery(GL_TIME_ELAPSED);
}

void PerfQuery::collect()
{
	assert((m_state == PQ_WAIT) && "Must end() query before calling collect()");
	uint64_t time_elapsed = 0;
	glGetQueryObjectui64v(m_gpu_time_query, GL_QUERY_RESULT_NO_WAIT, &time_elapsed);
	if(!time_elapsed)
		return;

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


