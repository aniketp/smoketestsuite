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

#include <fstream>
#include <iostream>
#include <cstdlib>
#include "utils.h"

void
utils::opt_def::insert_opts()
{
  // Option definitions.
  opt_rel h_def;        // '-h'
  h_def.type = 's';
  h_def.value = "h";
  h_def.keyword = "help";

  opt_rel v_def;        // '-v'
  v_def.type = 's';
  v_def.value = "v";
  v_def.keyword = "version";

  // `opt_map` contains all the options
  // which can be "easily" tested.
  opt_map.insert(std::make_pair<std::string, opt_rel>
                          ("h", (opt_rel)h_def));  // NOTE: Explicit typecast required here.
  opt_map.insert(std::make_pair<std::string, opt_rel>
                          ("v", (opt_rel)v_def));
};

std::list<utils::opt_rel*>
utils::opt_def::check_opts(std::string utility) {
  std::string line;                    // An individual line in a man-page.
  std::string opt_name;                // Name of the option.
  std::string opt_ident = ".It Fl";    // Identifier for an option in man page.
  std::string buffer;                  // Option description extracted from man-page.
  std::string opt_string;              // Identified option names.
  int opt_pos;                         // Starting index of the (identified) option.
  int space_index;                     // First occurrence of space character
                                       // in a multi-word option definition.
  std::list<opt_rel*> ident_opt_list;  // List of identified option relations (opt_rel's).

  // Generate the hashmap opt_map.
  insert_opts();

  // TODO: Section number cannot be hardcoded.
  std::ifstream infile("groff/" + utility + ".1");

  // Search for all the options accepted by the
  // utility and collect those present in `opt_map`.
  while (std::getline(infile, line)) {
    if ((opt_pos = line.find(opt_ident)) != std::string::npos) {
      opt_pos += opt_ident.length() + 1;    // Locate the position of option name.

      if (opt_pos > line.length()) {
        // This condition will trigger when a utility
        // supports an empty argument, e.g. tset(issue #9)
        continue;
      }

      // Check for long options ; While here, also sanitize
      // multi-word option definitions in a man page to properly
      // extract short options from option definitions such as:
      // .It Fl r Ar seconds (taken from date(1)).
      if ((space_index = line.find(" ", opt_pos + 1, 1))
                      != std::string::npos)
        opt_name = line.substr(opt_pos, space_index - opt_pos);
      else
        opt_name = line.substr(opt_pos);

      // Check if the identified option matches the identifier.
      // `opt_list.back()` is the previously checked option, the
      // description of which is now stored in `buffer`.
      if (!opt_list.empty() &&
          (opt_map_iter = opt_map.find(opt_list.back()))
                       != opt_map.end() &&
          buffer.find((opt_map_iter->second).keyword) != std::string::npos) {
        ident_opt_list.push_back(&(opt_map_iter->second));

        // Since the usage of the option under test
        // is known, we remove it from `opt_list`.
        opt_list.pop_back();
      }

      // Update the list of valid options.
      opt_list.push_back(opt_name);

      // Empty the buffer for next option's description.
      buffer.clear();
    }
    else {
      // Collect the option description until next
      // valid option definition is encountered.
      buffer.append(line);
    }
  }

  return ident_opt_list;
}