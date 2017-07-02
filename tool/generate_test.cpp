#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/wait.h>
#include "generate_test.hpp"

void
add_testcase(string option,
             string keyword,
             string utility,
             string output,
             ofstream& test_script)
{
  string testcase_name;

  // Add testcase name.
  test_script << "atf_test_case ";
  testcase_name = option;
  testcase_name.append("_flag");
  test_script << testcase_name + "\n";

  if (output.find("usage:") == string::npos) {
    // Presence of the string "usage:" in `output` denotes
    // that the `option` is a known one and EXIT_SUCCESS
    // will be encountered when it is executed.

    // Add testcase description.
    test_script << testcase_name
                 + "_head()\n{\n\tatf_set \"descr\" "
                 + "\"Verify the behavior for the option '-"
                 + option + "'\"" + "\n}\n\n";

    // Add body of the testcase.
    test_script << testcase_name
                 + "_body()\n{\n\tatf_check -s exit:0 "
                 + utility + " -" + option
                 + "\n\tatf_check -s exit:0 -o match:\""
                 + keyword + "\" " + utility + " -" + option
                 + "\n}\n\n";
  } else {
    // Non-empty output denotes that stderr was not empty
    // and hence the command failed to execute. We add an
    // appropriate testcase to verify this behavior.

    // Add testcase description.
    test_script << testcase_name
                 + "_head()\n{\n\tatf_set \"descr\" "
                 + "\"Verify that the option \'-"
                 + option
                 + "\' produces a valid error message in case of an invalid usage\"\n}\n\n";

    // Add body of the testcase.
    test_script << testcase_name
                 + "_body()\n{\n\tatf_check -s exit:1 -e inline:\'"
                 + output + "\' " + utility + " -" + option + "\n}\n\n";
  }
}

string
exec(const char* cmd)
{
    array<char, 128> buffer;
    string result;
    shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != NULL)
            result += buffer.data();
    }

    return result;
}

void
generate_test()
{
  list<tool::opt_rel*> ident_opt_list;
  string test_file;
  string output;
  string testcase_list;
  string command;
  struct stat buffer;
  ofstream test_fstream;

  tool::opt_def f_opts;
  ident_opt_list = f_opts.check_opts();
  test_file = f_opts.utility + "_test.sh";

  // Check if the test file exists. In case it does, stop execution.
  if (stat (test_file.c_str(), &buffer) == 0) {
    cout << "Skipping: Test file already exists" << endl;
    return;
  }

  test_fstream.open(test_file, ios::out);

  // TODO Add license to the test script.

  // If a known option was encountered (i.e. `ident_opt_list` is
  // not empty), produce a testcase to check the validity of the
  // result of that option. If no known option was encountered,
  // produce testcases to verify the correct usage message
  // when using the valid options incorrectly.

  // Add testcases for known options.
  if (!ident_opt_list.empty()) {
    for (auto i = ident_opt_list.begin(); i != ident_opt_list.end(); i++) {
      add_testcase((*i)->value, (*i)->keyword,
                   f_opts.utility, output, test_fstream);
      testcase_list.append("atf_add_test_case " + (*i)->value + "_flag\n");
    }
  }

  // Add testcases for the options whose usage is unknown.
  for (int i = 0; i < f_opts.opt_list.length(); i++) {
    command = f_opts.utility + " -" + f_opts.opt_list.at(i)
                     + " 2>&1";
    output = exec(command.c_str());
    if (!output.empty()) {
      add_testcase(string(1, f_opts.opt_list.at(i)), "",
                   f_opts.utility, output, test_fstream);
      testcase_list.append("\tatf_add_test_case "
                           + string(1, f_opts.opt_list.at(i)) + "_flag\n");
    }
  }

  test_fstream << "atf_init_test_cases()\n{\n" + testcase_list + "}";
  test_fstream.close();
}

int
main()
{
  generate_test();
  return 0;
}
