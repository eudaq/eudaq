# HiDRA2
## Git workflow procedure

- Everyone works on its own branch
- When a new feature is ready it needs to be merged to master with a Pull Request:
   - Commit any relevant changes
   - `git switch master` and `git pull` to update local master branch
   - `git switch <your branch>` and `git rebase master` to include changes on master on your own branch
   - Solve any merge conflict and finally `git push` to push your changes to your remote branch
   - In the web browser go to pull request, select master as target branch and your branch as source and create the pull request
   - When request is ready to merge -----> merge it. 


