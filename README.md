# Trello2Monthly
<h4 align="center">A command line program to build Monthly Status Report.</h4>

Trello2Monthly is a command line program to generate PDF and Word files from Trello. Trello2Monthly uses <a href="https://github.com/Microsoft/cpprestsdk">cpprestsdk</a>, <a href="https://github.com/gabime/spdlog">spdlog</a> and <a href="https://github.com/skystrife/cpptoml">cpptoml</a>.

## Installation
### Windows
1. Install cpprestsdk, spdlog and cpptoml with Microsoft's <a href="https://github.com/Microsoft/vcpkg">vcpkg</a>.
    * `vcpkg install cpprestsdk`
    * `vcpkg install spdlog`
    * `vcpkg install cpptoml`
2. Clone the project: `git clone https://github.com/phuongtran7/Trello_2_Monthly_CMake.git`.
3. Go into the project folder: `cd CMake_Trello2Monthly`.
4. Create `build` folder: `mkdir build`.
5. Go into `build` folder: `cd build`.
6. Build project: `cmake .. -DCMAKE_TOOLCHAIN_FILE="{VCPKG_ROOT}\scripts\buildsystems\vcpkg.cmake"`, with VCPKG_ROOT is the installation folder of vcpkg.
7. Open `.sln` file and start compiling the executable.

### Linux
1. Install cpprestsdk, spdlog and cpptoml with Microsoft's <a href="https://github.com/Microsoft/vcpkg">vcpkg</a>.
    * `vcpkg install cpprestsdk`
    * `vcpkg install spdlog`
    * `vcpkg install cpptoml`
2. Clone the project: `git clone https://github.com/phuongtran7/Trello_2_Monthly_CMake.git`.
3. Go into the project folder: `cd CMake_Trello2Monthly`.
4. Create `build` folder: `mkdir build`.
5. Go into `build` folder: `cd build`.
6. Build project: `cmake .. -DCMAKE_TOOLCHAIN_FILE="{VCPKG_ROOT}\scripts\buildsystems\vcpkg.cmake"`, with VCPKG_ROOT is the installation folder of vcpkg.
7. Compile the executable by using `make`.

## Preparing Trello
Trello2Monthly expects a certain way of using Trello to successfully generate a Monthly Status. There are four important components in Trello: `Board`, `List`, `Card` and `Label`.

Trello2Monthly expects a `Board` to represent the Month for the status report. Everything should be included in a Board for that month. The `Board` name should contain the month and year for the report. For example `Monthly Status Report - August 1997`. 

A `Board` can contain many `Lists`, each of these `Lists` represents a week in the month. The name of the `List` should be something like `8/24/1997 -- 8/30/1997` for easy integration with the LaTEX template.

In the `List`, there can be as many `Cards` as required. These `Cards` represents the task that is completed in that week. Each of the `Cards`(Tasks) can also use the `Description` box to explain more about that particular task. The `Description` will appear as a subitem under the `Card` name in the generated PFD file.

Due to the nature of this work, there are many projects can run parallel with each other, therefore the `Labels` are used to mark which task belongs to which projects. A task can have multiple `Labels` and it will appear at multiple sections in the final PDF file. 

There are two ways to report the work hours for each week. The first one is to use `Label`. Create a new `Label` called `Hour Breakdown` and then tag the cards that contain the work hour. For example: a `Card` named `Boeing 737: 10 Hours` with the tag `Hour Breakdown` will be put into the Hour Breakdown section of the PDF file. There can be as many `Cards` that tagged with `Hour Breakdown` as needed. Each of the `Cards` can represent a different project for that week.

The second way of reporting the hours is to manually specify the work hour on each of the tasks (`Cards`). To achieve this, we have to take advantage of the Power-Ups in Trello. Each of free user can have one Power-Up per board and we can use it to install `Custom Field Power-Ups`. This Power-Up allow the user to create a filed inside a `Card` and attach a value to each. In this case, we can create a `Custom Field` named `Work Hour` for the board. Then in each of the `Cards` in the week, the user can add the work hour for that particular `Card`. When generate the report, the program will combine all work hours of the `Cards` that have the same `Label`. For example, there are two `Cards` represent two complete tasks, these two `Cards` have the same label `Boeing 737`. The first `Card` has the custom field `Work Hour` with value of 6 and second one has the value of 4. When compiling the report, the program will add these two together and put in the `Hour Breakdown` section `Boeing 737: 10 Hours`. This allows the user to have a more fine-grained control over the task work hour.

The user can mix the two ways of reporting hour in a same `Board`, however, the user should commit to use only one way for the `List`. For example, in a `Board` there are four `Lists`, represent four weeks in that month. The user can use the general reporting hour by using `Label` for three of the `Lists` and using the more fine-grained control for the last one.

## Usage
### Windows
1. Install <a href="https://miktex.org/">MiKTeX</a>
2. It's recommended to compile the Template.tex before using Trello2Monthly so that all the necessary packages are installed correctly. Otherwise, during the generation PDF process, MiKTeX will interupt and ask to install these packages.
3. Get API key and Token from <a href="https://developers.trello.com/docs/api-introduction">Trello</a>.
4. Prepare a `config.toml` file with content:
```
[Configuration]
key = "DEVELOPER KEY"
token = "TOKEN"
author = "AUTHOR NAME"
```
and then put it next to the compiled executable.

5. Start the executable

### Linux
1. Install TexLive by using: `sudo apt-get install texlive-latex-base`.
2. Install addition packages by using : `sudo apt-get install texlive-latex-extra`.
3. Get API key and Token from <a href="https://developers.trello.com/docs/api-introduction">Trello</a>.
4. Prepare a `config.toml` file with content:
```
[Configuration]
key = "DEVELOPER KEY"
token = "TOKEN"
author = "AUTHOR NAME"
```
and then put it next to the compiled executable.

5. Run the executable by `./CMake_Trello2Monthly`

## Related Projects
1. <a href="https://github.com/phuongtran7/Trello2Monthly">Trello2Monthly</a>: Windows specific project that has auto update feature.
2. <a href="https://github.com/phuongtran7/Trello2Monthly_Service">Trello2Monthly_Service</a>: A client and server microservice for Trello2Monthly project. Instead of installing MiKTeX and Pandoc on each of the computer. The user can choose to install them once on the server machine. The client, which runs on other machines, can send the data over to the server to compile the PFD and Word document and then receive it back.