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

