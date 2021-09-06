# Contributing

## Code
Contributing code is very easy to do once you get the hang of it. We use GitHub for our online repository and issue tracker, so you'll need an account first. After you've done that and are comfortable with how the platform and git itself works, read on.

### Set Up Your Repository
1. Fork [LibreSprite](https://github.com/LibreSprite/LibreSprite) so you have a personal repository to push to.
2. Clone your personal repository with `git clone --recursive https://github.com/YOUR-USERNAME/LibreSprite`. Remember to change the URL to your repository's. The `--recursive` is used to download the third party libraries for building.
3. Move into the directory, on Linux this is done with `cd LibreSprite`.
4. Follow the [instructions](INSTALL.md) to create a build and make sure your local copy is working.
5. Add an upstream remote so you can get other developer's updates with `git remote add upstream https://github.com/LibreSprite/LibreSprite`.

### Updating Your Repository
These instructions will update both your local repository and online fork.
1. `git fetch upstream` Will download any new changes from the official LibreSprite repository.
2. `git checkout master` Will switch to your master branch.
3. `git merge upstream/master` Will merge or fast-forward your local master branch so it contains all the updates.
4. `git push origin master` Will update your online repository's master branch, it's a good idea to keep it up to date.

### Making Changes
To make a contribution you will need to make a feature branch. This greatly simplifies adding your code to the official repository, and it ensures the master branch is always working. As a heads up, it's best to let us know what you're working on by commenting on or creating an [issue](https://github.com/LibreSprite/LibreSprite/issues), this way we all save time by working on different things.
1. Call `git checkout master`, always make sure you are on master before making a feature branch.
2. Call `git checkout -b name-of-your-feature` This will create a feature branch and switch to it. Try to be specific in your feature name, this helps track it down in the future if we need to.
3. Make a meaningful change, you don't want to implement the whole feature in one shot generally. Try to break your task into meaningful (and revertible) chunks, though sometimes one chunk is all it takes like with small bug fixes.
4. To create a commit, start by verifying with `git status` that only files you wanted to change are effected. If there are unexpected changes, please resolve them before continuing.
5. Stage all of your changes with `git add -A`.
6. Create your commit with `git commit -m "Type a precise description of only the changes made with this commit."` Try to describe the changes like it's a change log (hint, it is). Messages like "Convert 'bool' members in ui::Widget to flags" are what we're after.
7. Repeat steps 2-5 until the feature is complete.

### Pushing Changes
You are ready for your contribution to be reviewed and merged. There are a few things you can do to make this easier for maintainers to review your code, so please follow all steps.
1. Follow the instructions for Updating Your Repository.
2. Switch back to your feature branch with `git checkout name-of-your-feature`.
3. Rebase your branch with `git rebase master`, this will make sure there are no conflicts with other people's contributions.
4. Rebuild your project to make sure everything still works. If things are now broken, resolve them, making commits along the way. Once resolved, return to step 1 as more changes could have been made in the meantime.
5. Push your branch to your online fork with `git push origin name-of-your-feature`.
6. On GitHub, create a pull request for your feature branch.
