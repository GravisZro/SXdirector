#include <climits>

#include <put/cxxutils/posix_helpers.h>

#include <put/application.h>
#include <put/object.h>

#include <put/specialized/eventbackend.h>
#include <put/specialized/processevent.h>
#include <put/specialized/mountpoints.h>


#include "../jobcontainer.h"
#include "../string_helpers.h"

#define UNIT_NAME "jobcontainer_unit"

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

  Object::connect(job.startFailure,
                  []() noexcept
                  {
                    terminal::write("%s - %s: job did not start\n", UNIT_NAME, "FAILURE");
                    Application::quit(EXIT_FAILURE);
                  });

  Object::connect(job.stopFailure,
                  []() noexcept
                  {
                    terminal::write("%s - %s: job did not stop\n", UNIT_NAME, "FAILURE");
                    Application::quit(EXIT_FAILURE);
                  });

  Object::connect(job.startSuccess,
                  [&job, &services, stop_signal_id, &exit_type]() noexcept
                  { job.stop(seconds(1), services, stop_signal_id, exit_type); });

  Object::connect(job.stopSuccess,
                  []() noexcept
                  {
                    terminal::write("%s - %s\n", UNIT_NAME, "SUCCESS");
                    Application::quit(EXIT_SUCCESS);
                  });

  job.start(seconds(1),
             services,
             options);

  return app.exec();
}
