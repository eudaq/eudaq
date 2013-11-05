eudaq
=====

eudaq Data Acquisition Framework main repository


Compiling and Installing:
------------------------
cd build
cmake ..
make install

will build main library, executables and gui (if Qt is found) and install into the source directory (./lib and ./bin). The install prefix can be changed by setting INSTALL_PREFIX option, e.g.:

cmake -DINSTALL_PREFIX=/usr/local ..


The main library (libEUDAQ.so) is always build, while the rest of the
package is optional. Defaults are to build the main executables and
(if Qt is found) the GUI application. Disable this behavior by setting
e.g. BUILD_main=OFF (disabling main executables) or enable
e.g. producers using BUILD_tlu=ON to enable build of tlu producer and
executables.

Example:

cd build
cmake -D BUILD_tlu=ON ..
make install

Variables thus set are cached in CMakeCache.txt and will again be taken into account at the next cmake run.


Development:
-----------

If you would like to contribute your code back into the main repository, please follow the 'fork & pull request' strategy:

* create a user account on github, log in
* 'fork' the (main) project on github (using the button on the page of the main repo)

* either: clone from the newly forked project and add 'upstream' repository to local clone (change user names in URLs accordingly):

git clone https://github.com/hperrey/eudaq eudaq
cd eudaq
git remote add upstream https://github.com/eudaq/eudaq.git

_OR_ if edits were made to a previous checkout of upstream: rename origin to upstream, add fork as new origin:

cd eudaq
git remote rename origin upstream
git remote add origin https://github.com/hperrey/eudaq
git remote -v show

* optional: edit away on your local clone!

* push the edits to origin (our fork)
git push
(defaults to 'git push origin master' where origin is the repo and master the branch)

* verify that your changes made it to your github fork and then click there on the 'compare & pull request' button

* summarize your changes and click on 'send' 
