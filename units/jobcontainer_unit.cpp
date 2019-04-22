#include <climits>

#include <put/cxxutils/posix_helpers.h>

#include <put/application.h>
#include <put/object.h>

#include <put/specialized/eventbackend.h>
#include <put/specialized/processevent.h>
#include <put/specialized/mountpoints.h>


#include "../jobcontainer.h"
#include "../string_helpers.h"

int main(int argc, char *argv[]) noexcept
{
// data
  std::string exit_type("HaltServices");
  std::list<std::string> services =
  {
    "unittest/demo"
  };
  std::unordered_map<std::string, std::string> options =
  {
    { "/Process/Executable", "process_control_unit.elf" },
    { "/Process/User", "unittest" },
    { "/Process/Group", "unittest" },
  };
  std::string stop_signal("SIGQUIT");

// program
  std::string arguments;
  posix::Signal::EId stop_signal_id = decode_signal_name(stop_signal);

  Application app;
  JobContainer job("demo");

  arguments.append(stop_signal);
  for(auto& service : services)
  {
    arguments.push_back(' ');
    arguments.append(service);
  }

  options["/Process/Arguments"] = arguments;

  Object::connect(job.startFailure, []() noexcept { exit(EXIT_FAILURE); });
  Object::connect(job.stopFailure , []() noexcept { exit(EXIT_FAILURE); });

  Object::connect(job.startSuccess,
                  [&job, &services, stop_signal_id, &exit_type]() noexcept
                  { job.stop(seconds(1), services, stop_signal_id, exit_type); });

  Object::connect(job.stopSuccess, []() noexcept { Application::quit(EXIT_SUCCESS); });

  job.start(seconds(1),
             services,
             options);

  return app.exec();
}
