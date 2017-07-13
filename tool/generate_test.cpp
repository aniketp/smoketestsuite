//
// Copyright 2017 Shivansh Rai
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//
// $FreeBSD$

#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/wait.h>
#include "generate_test.h"

void
add_known_testcase(string option,
                   string utility,
                   string descr,
                   string output,
                   ofstream& test_script)
{
  string testcase_name;

  // Add testcase name.
  test_script << "atf_test_case ";

  if (!option.empty()) {
    testcase_name = option;
    testcase_name.append("_flag");
  }
  else
    testcase_name = "no_arguments";

  test_script << testcase_name + "\n";

  // Add testcase description.
  test_script << testcase_name
               + "_head()\n{\n\tatf_set \"descr\" ";

  if (!descr.empty())
    test_script << descr;
  else
    test_script << "\"Verify the usage of option \'" + option + "\'\"";

  test_script << "\n}\n\n";

  // Add body of the testcase.
  test_script << testcase_name
               + "_body()\n{\n\tatf_check -s exit:0 -o ";

  // Match the usage output if generated.
  if (!output.empty())
    test_script << "inline:\'" + output + "\' ";
  else
    test_script << "empty ";

  test_script << utility;

  if (!option.empty())
    test_script << " -" + option;

  test_script << "\n}\n\n";
}

void
add_unknown_testcase(string option,
                     string utility,
                     string output,
                     string& testcase_buffer)
{
  testcase_buffer.append("\n\tatf_check -s exit:1 -e ");

  if (!output.compare(0, 6, "usage:"))
    testcase_buffer.append("inline:\"$usage_output\" ");
  else if (!output.empty())
    testcase_buffer.append("inline:\'" + output + "\' ");
  else
    testcase_buffer.append("empty ");

  testcase_buffer.append(utility);

  if (!option.empty())
    testcase_buffer.append(" -" + option);
}

pair<string, int>
exec(const char* cmd)
{
  array<char, 128> buffer;
  string usage_output;
  FILE* pipe = popen(cmd, "r");

  if (!pipe) throw runtime_error("popen() failed!");
  try {
    while (!feof(pipe)) {
        if (fgets(buffer.data(), 128, pipe) != NULL)
            usage_output += buffer.data();
    }
  }
  catch(...) {
    pclose(pipe);
    throw;
  }

  return make_pair<string, int>
         ((string)usage_output, WEXITSTATUS(pclose(pipe)));
}

void
generate_test(string utility)
{
  list<utils::opt_rel*> ident_opt_list;  // List of identified option relations.
  string test_file;             // atf-sh test name.
  string testcase_list;         // List of testcases.
  string command;               // Command to be executed in shell.
  string descr;                 // Testcase description.
  string testcase_buffer;       // Buffer for holding data for a testcase.
  struct stat buffer;
  ofstream test_fstream;        // Output stream for the atf-sh test.
  ifstream license_fstream;     // Input stream for license.
  pair<string, int> output;     // Return value type for `exec()`.

  utils::opt_def f_opts;
  ident_opt_list = f_opts.check_opts(utility);
  test_file = "generated_tests/" + utility + "_test.sh";

  // Check if the test file exists. In case it does, stop execution.
  if (stat (test_file.c_str(), &buffer) == 0) {
    cout << "Skipping: Test file already exists" << endl;
    return;
  }

  test_fstream.open(test_file, ios::out);
  license_fstream.open("license", ios::in);

  test_fstream << license_fstream.rdbuf();
  license_fstream.close();

  // If a known option was encountered (i.e. `ident_opt_list` is
  // not empty), produce a testcase to check the validity of the
  // result of that option. If no known option was encountered,
  // produce testcases to verify the correct usage message
  // when using the valid options incorrectly.

  // Add testcases for known options.
  if (!ident_opt_list.empty()) {
    for (auto i = ident_opt_list.begin(); i != ident_opt_list.end(); i++) {
      command = utility + " -" + (*i)->value + " 2>&1";
      output = exec(command.c_str());
      if (!output.first.compare(0, 6, "usage:")) {
        add_known_testcase((*i)->value, utility, descr,
                           output.first, test_fstream);
      }
      else {
        // We failed to guess the correct usage.
        add_unknown_testcase((*i)->value, utility, output.first, testcase_buffer);
      }
      testcase_list.append("\tatf_add_test_case " + (*i)->value + "_flag\n");
    }
  }

  // Add testcases for the options whose usage is not known (yet).
  if (!f_opts.opt_list.empty()) {
    // Add the "$usage_output" variable.
    command = utility + " -" + f_opts.opt_list[1] + " 2>&1";
    output = exec(command.c_str());

    if (!output.first.compare(0, 6, "usage:") && output.second)
      test_fstream << "usage_output=\'" + output.first + "\'\n\n";

    // Execute the utility with options and add tests accordingly.
    for (int i = 0; i < f_opts.opt_list.length(); i++) {
      command = utility + " -" + f_opts.opt_list[i] + " 2>&1";
      output = exec(command.c_str());

      if (output.second) {
        // Error is generated.
        add_unknown_testcase(string(1, f_opts.opt_list[i]),
                             utility, output.first, testcase_buffer);
      }
      else {
        // Output is generated.
        // Also, we guessed a correct usage.
        add_known_testcase(string(1, f_opts.opt_list[i]), utility,
                           "", output.first, test_fstream);
        testcase_list.append(string("\tatf_add_test_case ")
                            + f_opts.opt_list[i] + "_flag\n");
      }
    }

    testcase_list.append("\tatf_add_test_case invalid_usage\n");
    test_fstream << string("atf_test_case invalid_usage\ninvalid_usage_head()\n")
                  + "{\n\tatf_set \"descr\" \"Verify that an invalid usage "
                  + "with a supported option produces a valid error message"
                  + "\"\n}\n\ninvalid_usage_body()\n{";

    test_fstream << testcase_buffer + "\n}\n\n";
  }

  // If the invocation of utility under test without any option
  // fails, we add a relevant test under "no_arguments" testcase.
  command = utility + " 2>&1";
  output = exec(command.c_str());

  if (output.second) {
    // An error was encountered.
    test_fstream << string("atf_test_case no_arguments\n")
                  + "no_arguments_head()\n{\n\tatf_set \"descr\" ";
    if (!output.first.empty()) {
      // We expect a usage message to be generated in this case.
      descr = "\"Verify that " + utility
            + " fails and generates a valid output"
            + " when no arguments are supplied\"";

      if (!output.first.compare(0, 6, "usage:"))
        test_fstream << descr + "\n}\n\nno_arguments_body()\n{"
                      + "\n\tatf_check -s exit:1 -e inline:\"$usage_output\" "
                      + utility;
      else
        test_fstream << descr + "\n}\n\nno_arguments_body()\n{"
                      + "\n\tatf_check -s exit:1 -e inline:\'"
                      + output.first + "\' " + utility;
    }

    else {
      descr = "\"Verify that " + utility + "fails silently when no arguments are supplied\"" ;
      test_fstream << descr + "\n}\n\nno_arguments_body()\n{"
                    + "\n\tatf_check -s exit:1 -e empty " + utility;
    }

    test_fstream << "\n}\n\n";
  }

  else {
    // The command ran successfully, hence we guessed
    // a correct usage for the utility under test.
    if (!output.first.empty()) {
      descr = "\"Verify that " + utility
            + " executes successfully and produces a valid"
            + " output when invoked without any arguments\"";
      add_known_testcase("", utility, descr, output.first, test_fstream);
    }
    else {
      descr = "\"Verify that " + utility
            + " executes successfully and silently"
            + " when invoked without any arguments\"";
      add_known_testcase("", utility, descr, output.first, test_fstream);
    }
  }

  testcase_list.append("\tatf_add_test_case no_arguments\n");
  test_fstream << "atf_init_test_cases()\n{\n" + testcase_list + "}\n";
  test_fstream.close();
}

int
main()
{
  list<string> utility_list = {"date", "ln", "stdbuf"};
  for (const auto& i : utility_list) {
    generate_test(i);
  }

  return 0;
}
