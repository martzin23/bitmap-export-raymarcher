# Description

Software ray-marching renderer that is an improved version of [another one I made](https://github.com/martzin23/simple-console-raymarcher). It takes text files that describe a scene and renders it to a bitmap image. Created in November 2023.

# Usage

1. Compile the code
2. Run it with a scene file as standard input:
    `$ bitmap_export_raymarcher.exe < 1_scene.txt`
3. Open `render_000.bmp` to see the result

- The format of `scene.txt` files is explained in `0_instructions_scene.txt`.
- It can render animations as an image sequence if `frameMax` is higher than `frameMin` in the scene file.
    - There are no animated elements by default so every frame would look the same.
    - You can use the `sceneSettings.frameCount` variable to get the frame number and use it in the code to add animated elements.
    - You need to use other software to convert the rendered image sequence to a video file.

# Renders

![render_025](/showcase/render_025.png)

![render_025](/showcase/render_023.png)

![render_041](/showcase/render_041.png)

![render_043](/showcase/render_043.png)

![ray_march_animation2_0001-0180.gif](/showcase/ray_march_animation2_0001-0180.gif)
