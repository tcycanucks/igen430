# Notes for platformio.ini
`pio run`: will run main.cpp and compile it

# How to use git

`git status`: show you all untracked and tracked files

`git add <file path>`: to add file to be committed 
    `git add .`: adds all files to be committed (to the queue)
`git commit -m "<message>"`: to commit (now this is a version of the code that is 'solidified'/saved"
`git push origin main`: to push your updates to your github repo (uploading)

`git pull origin main`: pulls git repo
`git remote -v`: shows origin of which it fetches and pushes to
`git restore`: to previous version of github repo
`git fetch --all --prune`: syncs everything



1. The "Setup" Commands
Use these if you are starting a new folder or fixing a broken connection.
    `git init`: Turns the current folder into a Git repo.
    `git remote add origin <URL>`: Connects your Mac to the GitHub website.
    `git remote set-url origin <URL>`: Updates the link if you renamed the repo.
    `git remote -v`: Shows you exactly which website link your folder is "talking" to.

2. The "Daily Work" Cycle
Do these in order every time you finish a task.
    `git status`: Shows you which files you changed (they will be red).
    `git add .`: Groups all changed files together ("Staging").
    `git commit -m "fixed the layout"`: Saves that group with a name.
    `git push origin <branch-name>`: Uploads your work to GitHub.

3. Branching (Working with Teammates)
This is how you work on your own "tab" so you don't overwrite your friend's code.
    `git branch`: Lists all branches (the one with the * is where you are now).
    `git branch -m new-name-here`: modifies or moves branch
    `git checkout -b <new-name>`: Creates a new branch and switches to it.
    `git checkout <name>`: Switches back to an existing branch (e.g., git checkout main).
    `git pull origin main`: Downloads your teammate's latest work into your current folder.

4. The "I'm Stuck" Rescue Commands
    `git checkout .`: (Careful!) This deletes all your unsaved changes and resets the file to how it looked at the last commit.
    `git log --oneline`: Shows a simple list of your recent saves.
    `Control + C`: If the terminal seems "frozen" or stuck in a loop, this kills the process.
    `q`: If your terminal opens a weird text screen you can't type in, press q to quit that view.