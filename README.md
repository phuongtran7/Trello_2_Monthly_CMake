# Trello2Monthly
<h4 align="center">A command line program to build Monthly Status Report.</h4>

Trello2Monthly is a command line program to generate PDF and Word files from Trello. Trello2Monthly uses <a href="https://github.com/Microsoft/cpprestsdk">cpprestsdk</a> and <a href="https://github.com/gabime/spdlog">spdlog</a>.

## Installation
### Windows
1. Install cpprestsdk and spdlog with Microsoft's <a href="https://github.com/Microsoft/vcpkg">vcpkg</a>.
2. Install <a href="https://miktex.org/">MiKTeX</a>
3. It's recommended to compile the Template.tex before using Trello2Monthly so that all the necessary packages are installed correctly. Otherwise, during the generation PDF process, MiKTeX will interupt and ask to install these packages.
4. Get API key and Token from <a href="https://developers.trello.com/docs/api-introduction">Trello</a>.
5. Prepare a `Tokens.txt` file with content:
```
{API Key}
{Token}
```

### Linux
Coming soon

## Preparing Trello
Trello2Monthly expects a certain way of using Trello to successfully generate a Monthly Status. There are four important components in Trello: Board, List, Card and Label.

Trello2Monthly expects a `Board` to represent the Month for the status report. Everything should be included in a Board for that month.

A `Board` can contain many `Lists`, each of these `Lists` represents a week in a month. The name of the `List` should be something like `8/24/1997 -- 8/30/1997` for easy integration with the LaTEX 
template.

In the `List`, there can be as many `Cards` as required. Each of these `Cards` represents the task that is completed in that week.

And finally, due to the nature of this work, there are many projects can run parallel with each other, therefore the `Labels` are used to marked which task belongs to which projects. A task can have multiple `Label` and it will appear at multiple section in the final PDF file.