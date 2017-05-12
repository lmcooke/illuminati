# Progressive and Expressive Photon

By Nellie Robinson, Emily Reif, Luci Cooke, and Kenji Endo

The goal of this project was to use photon beam mapping in order to render art directed volumetric lighting.

---

### Overview

Rendering participating media can be slow and memory heavy, in addition to being difficult for artists to control and art direct. However, volumetric effects and lighting can create magical and fantastical effects. For this project, we wanted to create a system to allow artists to control physical and artistic parameters in volumetric lighting.

The primary art-directable feature in this project is the ability to curve photon beams. We allow for the user to specify curves in Maya, and then export them into a specific file format that can then be used in G3D scenes.

---

### Reference Papers

We primarily referenced two papers:
1. "Progressive Photon Beams" (Wojciech Jarosz, Derek Nowrouzezahrai, Robert Thomas, Peter-Pike Sloan, Matthias Zwicker. Proceedings of SIGGRAPH Asia, 30(6), December 2011);
2. "A Programmable System for Artistic Volumetric Lighting" (Derek Nowrouzezahrai, Jared Johnson, Andrew Selle, Dylan Lacewell, Michael Kaschalk, Wojciech Jarosz. Proceedings of ACM SIGGRAPH Transactions on Graphics, 30(4):29:1 - 29:8, August 2011).

---

### Implementation Details

This project employs two parts in rendering: pre-processing and rendering. The pre-processing step entails building a photon map, in which we used a technique similar to traditional photon mapping. However, instead of storing photons as points in our photon map, we stored photon beams. Additionally, we ray-march in our pre-processing step in order to take into account the user-specified scattering, transmission, and extinction properties of the participating media. This also entails ray-marching along and scattering off of curved photon beams, so that there will be indirect illumination due to the curvy beams.

In order to specify curves, we created a file type that lists the coordinates, radius, and color for each CV. Our implementation then reads and interpolates between these points.

The rendering step took a combined CPU/GPU approach. The atmospheric illumination is rendered on the GPU, as we created a 2D billboard from each photon beam which is perpendicular to the camera. We then render these "splatted" beams on the GPU. This allows for the atmospheric illumination to run in real-time, and allows for the user to adjust their settings and see results immediately. We also render the atmospheric lighting progressively. This allows us to jitter the interpolation of each curved beam so that the beams will converge over time.

Surface illumination is dependent upon upon the photon map built in the pre-processing step and so it is rendered on the CPU in the background. At each frame, we then composite the surface and volumetric illumination together on the GPU. The surface illumination is also rendered progressively, so that multiple passes will improve image quality.

---

### To Use

This project uses the G3D library, and can be compiled from the command line from the /src directory using "icompile --opt --run". 

In order to create photon beams from a curve in Maya:
1. Run the 'makeSpline.py' script in the script editor against the curve in Maya, and create a .lit file from the text it outputs.
2. Reference the .lit file in the Scene.Any file which describes the scene. For an example, see src/data-files/scene/two_splines.Scene.Any.
=======


# Git Workflow

### Best Practices
1. Only develop in your personal branch (below I call this my-branch)
2. Write helpful commit messages. "Added support for shooting rays" as opposed to "Update #3
3. Make sure you pull and merge before you push (step 5 below)
4. Whenever you pull or merge, check to see if it says "MERGE CONFLICT". Resolve them asap. Running `git status` will remind you where the conflicts are.

---

### Overview
1. Get a local copy of the most up to date version of origin/master (cloud copy)
2. Make a branch locally, my-branch
3. Do your development in your local branch, my-branch
4. Periodically update your local copy of master so that it matches origin/master, then merge your local copy of master into my-branch to resolve merge conflicts over time instead of all at once when you need to push
5. When it's time to push your code up:
  1. Do step 4 (update local master, merge into my-branch, resolve all conflicts)
  2. Make sure that everything still runs how you want (no crashes/bugs)
  3. Merge your local branch back into your local master
  4. Make sure that nothing has changed in origin/master
  5. Push your code up.

---

### General Commands
`git status` // check if you have any uncomitted changes

`git branch` // check what branch you're currently on

`git branch my-branch` // creates a new branch called my-branch

`git checkout my-branch` // switch to my-branch

`git add .` // add all unstaged changes (unstaged just means you haven't added it yet/told git "I wanna commit this!")

`git add ./path/to/file/filename.cpp` // add a specific unstaged file

`git commit -m "My message here"` // commit all staged changes

`git pull origin master` // make pull changes from origin/master (cloud copy) into your local copy of master. This automatically does the merge/rebase for you.

---

### Specific Workflow Commands
1. Get a local copy of the most up to date version of origin/master (cloud copy)

  `git clone url.goes.here`
2. Make a branch locally, my-branch

  `git branch my-branch`  // Make the new branch
  
  `git checkout my-branch`  // Switch to the new branch
3. Do your development in your local branch, my-branch

  work work work
  
  `git status`
  
  `git branch` // Just to make sure you're on my-branch
  
  `git add .` // Will add all of the unstaged files
  
  `git commit -m "Very informative message here"`
4. Periodically update your local copy of master so that it matches origin/master, then merge your local copy of master into my-branch to resolve merge conflicts over time instead of all at once when you need to push

  `git pull origin master` // Make your local version of master match origin/master (cloud copy). This automically does the merge/rebase for you.
  
  Check for merge conflicts (look at what is printed out)
  
  `git branch` // Check what branch you're currently on
  
  `git checkout my-branch` // If you aren't already on your personal branch
  
  `git merge master` // Merge master into my-branch.
  
  Check for merge conflicts and resolve.
  
  Make sure that everything compiles and runs w/o crashers, etc.
5. When it's time to push your code up:
  1. Do step 4 (update local master, merge into my-branch, resolve all conflicts)
  2. Make sure that nothing has changed in origin/master. Why? What if someone else pushed something up while you were resolving merge conflicts or something?! Soooo...
  
    `git checkout master` // Get on the master branch
    
    `git pull origin master` // Update your local copy of master again
    
    `git diff my-branch master` // This makes sure that there's no difference. If there is a difference, go back to the top of step 4. If there isn't, then...
  3. Merge your local branch back into your local master. 
  
    `git merge my-branch` // Merge my-branch into your local copy of master
    
  4. Pat yourself on the back. Woohoo!
  5. Push your code up.
  
    `git branch` // Make sure you're on master. Run `git checkout master` if you aren't.
    
    `git push`
    
---

### .gitignore
After changes have been made to the gitignore (additions or deletions), you may need to flush cached git files to kick the gitignore into gear.

First, commit any changes you've made, update local master and merge local master into personal branch.

  `git rm -r --cached . // removes cached files`
  
  `git add . // adds everything back`
  
  `git commit -m \"fixed untracked files\"`
  
Then, merge local into master, and push as normal.
>>>>>>> 6ed38933356ddb25f6830c46703b77a8a777f1df
