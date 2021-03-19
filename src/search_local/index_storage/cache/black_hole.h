/*
 * =====================================================================================
 *
 *       Filename:  black_hole.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/08/2020 10:02:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Norton, yangshuang68@jd.com
 *        Company:  JD.com, Inc.
 *
 * =====================================================================================
 */
#include <task_request.h>

class BlackHole : public TaskDispatcher<TaskRequest>
{
public:
	BlackHole(PollThread *o) : TaskDispatcher<TaskRequest>(o), output(o){};
	virtual ~BlackHole(void);
	void bind_dispatcher(TaskDispatcher<TaskRequest> *p) { output.bind_dispatcher(p); }

private:
	RequestOutput<TaskRequest> output;
	virtual void task_notify(TaskRequest *);
};
