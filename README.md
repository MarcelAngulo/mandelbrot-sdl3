
# Mandelbrot Fractal Explorer

A high-performance, fully interactive Mandelbrot fractal explorer written in pure C. Utilizing SDL3 for hardware-accelerated rendering and OpenMP for CPU multithreading, it allows real-time exploration and manipulation of the fractal plane. It also includes an auxiliary Python script to assist in visualizing custom color palettes.

![Beautiful Images](./assets/tornasol.png)
![](./assets/tornasol2.png)
![Spiral](./assets/espiral.png)
![](./assets/bonito2.png)

## Features

*   **Written in pure C** for maximum efficiency and speed.
*   **Multithreaded Rendering** powered by OpenMP to calculate the fractal set across all CPU cores rapidly.
*   **Dynamic Color Palettes:** Define custom interpolating escape gradients and set interior colors.
*   **Interactive Exploration:** Pan, zoom, and modify maximum iterations on the fly using keyboard and mouse inputs.
*   **Built-in Screenshot Tool:** Save your current beautiful discoveries directly to a `.png` file.
*   **UI Overlays:** Toggleable on-screen information panel and a center crosshair pointer to see exactly where you are zooming.
*   **Customizable Power:** Go beyond the standard Mandelbrot set by modifying the power $p$ to render generalized Multibrot sets using the formula $z_{n+1} = z_{n}^p + c$.
*   **Command Line & Script Support:** Read configurations from scripts or command-line arguments to jump straight into specific coordinates and visual styles.
*   **Palette Visualizer:** Includes a lightweight Python script to preview colors.

## Dependencies

To compile and run this project, you will need:

*   A C compiler (e.g., [GCC](https://gcc.gnu.org/) or Clang)
*   [Make](https://www.gnu.org/software/make/)
*   [SDL3](https://libsdl.org/) (Core library)
*   [SDL3_image](https://github.com/libsdl-org/SDL_image) (For saving screenshots)
*   [SDL3_ttf](https://github.com/libsdl-org/SDL_ttf) (For rendering on-screen text)
*   [OpenMP](https://www.openmp.org/) (For CPU multithreading)
*   [Pillow](https://python-pillow.org/) (Optional, only required for the Python script)

## Building

### Step 1: Install Dependencies

**Arch Linux:**
```bash
sudo pacman -S base-devel gcc make sdl3 sdl3_image sdl3_ttf
```
**Debian / Ubuntu:**
```bash
sudo apt update
sudo apt install gcc make libomp-dev
# Note: Since SDL3 is relatively new, you may need to build it from source 
# or use a third-party PPA depending on your Linux distribution.
```

**macOS (Using Homebrew):**
```bash
brew install gcc make sdl3 sdl3_image sdl3_ttf libomp
```

### Step 2: Compile the Project
Navigate to the root directory of the project and run `make`:
```bash
make
```

### Step 3: Run the Program
Once compiled, the binary is generated in the `bin` directory.
```bash
./bin/mandelbrot
```

## Command Line Arguments & Commands

You can pass these commands as arguments when launching the program, enter them in the interactive command mode, or place them in a text file to read as a script. Both `--flag` and `flag` formats are supported.

| Command | Shortcut | Arguments | Description |
| :--- | :--- | :--- | :--- |
| `center` | `c` | `[double re] [double im]` | Sets the center coordinates of the view. |
| `range` | `r` | `[double range]` | Sets the horizontal zoom range. |
| `window` | `w` | `[int width] [int height]` | Resizes the application window. |
| `maxiter` | `i` | `[int > 0]` | Sets the maximum escape iterations (detail level). |
| `power` | `p` | `[int]` | Sets the power $p$ for the fractal calculation. |
| `outputpath` | `o` | `[string directory]` | Sets the default directory for saved screenshots. |
| `colors` | `s` | `[set] [rng] [N] [C...]` | Defines the color palette (interior, gradient range, size, and hex colors). |
| `showpointer` | | `true` or `false` | Toggles the center crosshair pointer. |
| `showinfo` | | `true` or `false` | Toggles the text information overlay. |
| `screenshot` | | `[filename]` or `--default`| Saves a PNG of the current view. |
| `read` | | `[filename]` | Loads configuration commands from a text file. |
| `write` | | `[filename]` or `--default`| Saves the current state/location to a text file. |
| `exit` | | | Safely quits the application. |

## Usage

### Basic CLI Usage
Launch the program with a specific window size, a custom maximum iteration count, and jump straight to interesting coordinates:
```bash
./bin/mandelbrot window 1280 720 center -0.743643 0.131825 maxiter 1500 range 0.00005
```

### Key Bindings (Explore Mode)
When in Explore mode, use the following keys to interact with the fractal:

*   `z` or `F1`: Toggle help menu
*   `x`: Return to last update/backup
*   `a`: Zoom out
*   `s`: Zoom in
*   `q` or `ESCAPE`: Quit application
*   `o` or `-`: Decrease max iterations
*   `p` or `+`: Increase max iterations
*   `g`: Save screenshot
*   `h` or `LEFT`: Move left
*   `l` or `RIGHT`: Move right
*   `j` or `DOWN`: Move down
*   `k` or `UP`: Move up
*   `i`: Toggle info overlay
*   `c`: Toggle pointer overlay
*   `f`: Toggle fullscreen

Press `n` to enter **Command Mode**. A text bar will appear at the bottom where you can type commands like `:center -0.5 0.3` or `:colors 000000 100 5 0d0221 7f00ff ff007f ffaa00 00ffff`. Press `RETURN` to execute or `ESCAPE` to return to Explore mode.

### Loading Scripts (`read` command)

You can save commands into text files to act as preset locations or color themes. Create a file named `cyberpunk.txt` with the following content:
```text
colors 000000 100 5 0d0221 7f00ff ff007f ffaa00 00ffff
center -0.743643 0.131825
range 0.00005
maxiter 1000
```
Then, you can load this exact state by passing it via the terminal:
```bash
./bin/mandelbrot read cyberpunk.txt
```
Or by typing `:read cyberpunk.txt` in the program's Command Mode!

Here is another script example you could save as `multibrot_fire.txt`:
```text
power 3
colors 0a0000 80 5 000000 7a0000 ff3300 ffcc00 ffffff
center 0 0
range 3.5
```
