#ifndef JOBCONTROLLER_H
#define JOBCONTROLLER_H

#include <object.h>

#include <list>
#include <memory>

class JobController
{
public:
  JobController(pid_t pid) noexcept;
 ~JobController(void) noexcept;

private:
  struct node_t;
  std::shared_ptr<node_t> m_jobtree;
};

#endif // JOBCONTROLLER_H
