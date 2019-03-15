# Trello2Monthly
<h4 align="center">A command line program to build Monthly Status Report.</h4>

Trello2Monthly is a cross-platform command line program to generate PDF and Word files from Trello. Trello2Monthly uses <a href="https://github.com/Microsoft/cpprestsdk">cpprestsdk</a> and <a href="https://github.com/gabime/spdlog">spdlog</a>.


## Installation
[FR]: https://github.com/akashnimare/foco/releases


### Windows
1. Install cpprestsdk and spdlog with Microsoft's <a href="https://github.com/Microsoft/vcpkg">vcpkg</a>.
2. Install <a href="https://miktex.org/">MiKTeX</a>
3. It's recommended to compile the Template.tex before using Trello2Monthly so that all the necessary packages are installed correctly.
4. Get Developer Key and Token from Trello.
5. Prepare a `Secrets.h` file with content:
```
#pragma once
static const utility::string_t trello_secrect(U("?key={YOUR KEY}&token={YOUR TOKEN}"));
```
### Linux

Coming soon