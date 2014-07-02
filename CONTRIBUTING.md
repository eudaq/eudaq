If you would like to get involved in the EUDAQ development - all
contributions are highly welcome! :)

# Getting Started
* make sure you have a user account on [GitHub](https://github.com/signup/free) and are logged in
* 'fork' the (main) project on GitHub [using the button on the page of the main repo](https://github.com/eudaq/eudaq/fork)

# General guidelines
* make atomic commits (aka commit often)
* for anything but trivial changes (e.g. affecting documentation only)
  please do not work on the ```master``` branch, but create a feature
  branch (see below for instructions) and commit only changes that
  affect this particular feature
* use descriptive commit messages -- for an example, see below
* follow the style (indentation, naming schemes for variables, ...) of the existing code, where possible

# Making Changes
Have you fixed a bug already or would like to contribute your own
processor into the main repository? Then please fork the project and
issue a pull request using these instructions:

* now add the newly forked repository as a git remote to your local EUDAQ installation and rename the original repository to 'upstream':
```
git remote rename origin upstream
git remote add origin https://github.com/[YOUR GITHUB USER HERE]/eudaq
git remote -v show
```
* create and checkout a new branch for the feature you are developing based on the ```master``` branch:
```
git branch my_feature_branch master
git checkout my_feature_branch
```

* now edit away on your local clone! But keep in sync with the development in the upstream repository by running
```
git pull upstream master
```
on a regular basis. Replace ```master``` by the appropriate branch if you work on a separate one *within the main repository*.

* Commit often using a descriptive commit message in the form of
```
This is a concrete and condensed headline for your commit

After the headline followed by an empty line, add a more thorough
description of what you have changed and why.  Refer to any issues on
GitHub affected by the commit by writing something along the lines of
e.g. 'this addresses issue eudaq/eudaq#1' to refer to the
first issue in the main repository.  
```

* push the edits to origin (your fork)
```
git push origin my_feature_branch
```

* verify that your changes made it to your GitHub fork and then click there on the 'compare & pull request' button

* summarize your changes and click on 'send'

* now we will review your changes and comment on anything unclear; any
  further commits you will make before the pull request is merged will
  also end up in the same pull request, so you can easily add or
  correct parts of your contribution while the review is ongoing.

# Additional Resources
[Using GitHub Pull Requests](https://help.github.com/articles/using-pull-requests)

[Using mark-down e.g. in issues, comments or the wiki](https://help.github.com/articles/markdown-basics)

[Some GitHub mark-down specialties](https://help.github.com/articles/github-flavored-markdown)

[General help on GitHub](https://help.github.com/)

