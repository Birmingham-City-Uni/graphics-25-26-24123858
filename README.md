[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/6itnWvSn)
# CMP5359 Computer Graphics 

## Rasteriser
### Features Implemented

**Z buffering (depth testing):**
A depth buffer was implemented to ensure objects are rendered in the correct order
based on their distance from the camera. Each pixel stores the closest depth value 
rendered so far, and any pixels that are further away are discarded. This prevents 
objects from drawing over each other incorrectly and removes hindden surfaces from the scene.

**Texture mapping:**
Texture mapping was implemented to apply png textures to 3d models using uv
coordinates imported from the mesh. UV coords are perspective correctly 
interpolated across each triangle and used to sample the texture for every pixel.
Gamma correction is also applied, with textures converted from sRGB into linear 
colour space before lighting calculations and converted back before being written 
to the final image. Objects without textures can also be rendered using a flat 
colour material.

**Multiple light types (including polymorphism):**
Renderer supports multiple light types through object oriented lighting system.
A common Light base class is used by the renderer and AmbientLight,
PointLight, DirectionalLight and SpotLight are implemented as derived classes. Each light 
calculates its own direction and intensity behaviour, which allows different light types to be 
used together within the scene. This design uses inheritance and polymorphism,
enabling new light types to be added without modifying the rendering pipeline.
My final scene uses both ambient and spotlight lighting, with other lighting implemented but not used.

**Blinn-Phong and Phong shading:**
Both Phong and Blinn-Phong shading models were added to simulate realistic 
lighting on object surfaces. Diffuse lighting is calculated using Lambers cosine 
law and specular highlights can be created using either Phong reflection model or 
Blinn-Phong halfway vector approach (more effectve)
The active shading model can be set through the renderer, and I chose to use
Blinn-Phong in my final sccene.

**OOP/c++ paradigms:**
Object oriented programming principles were incluses and used to keep the code reusable,
modular and easier to maintain. The new Scene class manages rendering, objects, 
lights, cameras and buffers. SceneObject stores mesh, texture and material 
information for each assets. Inheritance and polymorphism are used throughout 
the lighting system, and pointers are used to manage memory automatically 
without manual deletion.
The project uses encapsulation through the Scene and SceneObject classes, inheritance
through the Light hierarchy, abstraction through the Light base class interface and 
polymorphism through lighting functions.

**Backface culling:**
Implemented to avoid rendering triangles that face away from the camera
Triangles with negative signed screenspace area are skipped before being
rasterised to avoid rendering rear facing geometry

**Perspective correct interpolation:**
Used when calculating UV coordinates, world positions and surface normals across triangles
This prevents distortion and texture warping (occurs with screenspace interpolation) and 
makes sure attributes stay accuratw no matter viewing angle or depth

**Alpha blending/translucency (glass obj):**
Transparency system was implemented to support the frosted glass panel within 
the scene. The glass texture acts as an opacity mask, where darker areas become 
more transparent and lighter areas remain visible (frosted). Transparent pixels are blended 
with the existing framebuffer colour using alpha blending, so the geometry behind 
the glass will be visible while having the appearance of a translucent surface.
Non transparent objects are rendered first, followed by transparent objects to ensure correct 
blending results, otherwise blending will result in black

**Normal mapping (button, cube, gun):**
Normal mapping was implemented to add additional surface detail without increasing mesh complexity
Normal maps store perpixel surface directions which are transformed from tangent space into world
space using the TBN matrix generated for each triangle
These normals are then used during the lighting calculations, allowing surface details to affect 
diffuse and specular lighting even though the actual geometry doesnt change

**Specular mapping (button, cube, gun):**
Was implemented to vary shininess of a model (on a perpixel basis)
The renderer samples a specular texture and uses its brightness to control the 
strength/sharpness of specular highlights, allowing different parts of the same 
object to appear glossy/metallic/matte
This creates more realistic materials than using a single specular value across the entire surface

**Emissive mapping (button, cube, door, sign):**
Emissive mapping was implemented to allow selected areas of a texture to appear 
illuminated 
Emissive textures are sampled independently of the scene lighting and added directly to the final 
shaded result, allowing objects to appear as though they are glowing, even when no light source 
is directly affecting them
Intensity of the effect can be adjusted through a configurable emissive strength value, to vary 
across models
black pixels= no emission (normal shading), coloured/bright pixels = glowing
This is how game engines make neon signs/glowing screens/LEDs look lit up


## Raytracer 
### Features Implemented



## Learning Resources and References
See REFERENCES.md for rasteriser and raytracer learning and asset resources





## Coursework and Lab Starter Repo
### Using this repo

This repo should be used for all of your work for the CMP5359 Computer Graphics module. It is where you should work on the labs, and also submit your final coursework by creating a GitHub release.

Please remember you are marked on regularly committing to this repository - this will form an important part of your Milestone marks!

### Structure

The `Labs` folder contains starter code for your lab activities. These can be compiled using CMake as detailed below.

The `Coursework` folder contains folders and starter code for your three coursework tasks, the Pathtracer, Raytracer and Rasteriser. You may choose to use this starter code, or write your own. See the Submitting and CMake sections below for how to format your submission, and how to compile the code.

### Compiling with CMake

Each of the subfolders, e.g. `Labs/Week1` or `Coursework/Rasteriser` contains a CMake project - note each contains a `CMakeLists.txt` file. To compile each, open up the CMake-GUI application. If you're working on your own machine, you can get CMake here https://cmake.org/download/ . Get the Binary version (probably `Windows x64 Installer`, but versions for Mac and Linux are available).

There are two boxes at the top where you enter the *source* and *build* directories. 

In the "Where is the source code" box at the top, enter the full path to the folder containing `CMakeLists.txt`. You can also use the "Browse Source" button to find the folder in a GUI window. For example, this might be `C:\Users\student\Documents\GitHub\CMP5359\Labs\Week1`.

In the "Where to build the binaries" directory, select a folder called "build" within the source directory. For example, this might be `C:\Users\student\Documents\GitHub\CMP5359\Labs\Week1\build`

**Very Important**: You *must* select a build folder within each project as the "Where to build the binaries" option. If you select the source directory or any other directory you will run into multiple issues, including the app being unable to find files, and clogging up your GitHub with lots of unnecessary files. 

Click "Configure", "Generate" and then open in Visual Studio, either using the third button below, or by going into the build folder and opening the `.sln` file.

When running, remember that you will likely need to select a project in the Solution Explorer on the right, and then right click and "Set as Startup Project" before running and debugging.

### Submitting

Your submitted coursework code should be contained in the `Coursework` directory, as follows:

* `Coursework/Rendered_Images`: Should contained 4 image files - your reference screenshot, and rendered images from your 3 rendering approaches (rasteriser, raytracer, pathtracer).
* `Coursework/Rasteriser`: C++ Code for your rasteriser.
* `Coursework/Raytracer`: C++ Code for your raytracer.
* `Coursework/Pathtracer`: Blender file (.blend format) containing your path-traced scene. I recommend leaving this outside the repo until you are ready, to avoid saving multiple versions of this large binary file.

I must be able to compile and run your code on the lab machines. I recommend building on the starter code and using CMake, but use of Visual Studio is acceptable (if you do, make sure you're using relative directories, i.e. `$(SolutionDir)` when setting up include directories so it will compile on other computers).
