#include "jobcontroller.h"

#include <specialized/ProcessEvent.h>
#include <specialized/procstat.h>

#include <cassert>

struct JobController::node_t
{
  ProcessEvent proc;

  std::shared_ptr<node_t> parent;
  std::list<std::shared_ptr<node_t>> children;

  node_t(node_t* _parent, pid_t pid)
    : proc(pid, ProcessEvent::Fork | ProcessEvent::Exit), parent(_parent)
  {
    Object::connect(proc.forked,
                    Object::fslot_t<void, pid_t, pid_t>
                      ([this](pid_t _parent, pid_t _child) noexcept { add(_parent, _child); }));

    Object::connect(proc.exited,
                    Object::fslot_t<void, pid_t, pid_t>
                      ([this](pid_t _pid, posix::error_t) noexcept { remove(_pid); }));
  }

  void add(pid_t parent_pid, pid_t child_pid)
  {
    process_state_t data;
    if(::procstat(child_pid, data) != posix::error_response && // if the process (still) exists AND
       proc.pid() == parent_pid) // this is the parent process
      children.emplace_back(new node_t(this, child_pid)); // add as child
  }

  void remove(pid_t target_pid)
  {
    auto self = std::shared_ptr<node_t>(this); // preserve this node until going out of scope!
    if(proc.pid() == target_pid && // if this is the process to remove AND
       parent != nullptr) // it's not the root process
    {
      std::shared_ptr<node_t> root(parent);
      while(root->parent != nullptr) // if not root node
        root = root->parent; // become parent node

      parent->children.remove(self); // remove this node from the parent node
      while(!children.empty()) // while there are still children
      {
        std::shared_ptr<node_t> old_child = children.front(); // preserve child (for scope)
        node_t* new_child = new node_t(root.get(), old_child->proc.pid()); // create new child
        new_child->children = old_child->children; // copy over children
        root->children.emplace_back(new_child); // add new child
      }
    }
  }
};

JobController::JobController(pid_t pid) noexcept
{
  m_jobtree = std::make_shared<node_t>(nullptr, ::getpid()); // current process serves as root
  Object::disconnect(m_jobtree->proc.exited); // don't track current process
  Object::disconnect(m_jobtree->proc.forked);
  m_jobtree->add(::getpid(), pid); // add process to track
}

JobController::~JobController(void) noexcept
{

}
