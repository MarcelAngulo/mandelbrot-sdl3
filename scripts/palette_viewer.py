#!/usr/bin/env python3
import sys
import argparse
import math
from pathlib import Path
from PIL import Image

def positive_int(value):
    """Custom validator to ensure n > 0"""
    ivalue = int(value)
    if ivalue <= 0:
        raise argparse.ArgumentTypeError(
                f"{value} must be a positive integer greater than 0.")
    return ivalue

def hex_to_rgb(hex_color):
    """Utility to convert a hex string like 'FF0000' to a tuple (255, 0, 0)."""
    hex_color = hex_color.lstrip('#')
    return tuple(int(hex_color[i:i+2],16) for i in (0, 2, 4))

def main():
    parser = argparse.ArgumentParser(
        description="Generate a smooth Mandelbrot color palette PNG.")

    parser.add_argument("-n", "--steps", type=positive_int,
                    required=True, help="Number of steps (width of the image) for the color transition")

    parser.add_argument("-c", "--color", nargs='*', default=[],
                    help="List of hex colors (e.g., FF0000 00FF00)")

    args = parser.parse_args()
    colors_input = args.color

    # Handle piped inputs (Bash style)
    if not sys.stdin.isatty():
        piped_input = sys.stdin.read().split()
        colors_input.extend(piped_input)

    # Validation
    if len(colors_input) < 2:
        parser.error(
        "You must provide at least two colors to create a transition.")

    # Convert the string inputs ('FF0000') into usable math tuples
    # ((255,0,0))
    rgb_colors = [hex_to_rgb(c) for c in colors_input]
    print(f"Generating transition across {args.steps} steps using colors: {colors_input}")

    n = args.steps
    section_width = n / (len(rgb_colors) - 1)
    # Create a blank image: width = steps, height = 50 pixels
    # (so it looks like a ribbon/strip)
    img = Image.new('RGB', (n, 50))
    pixels = img.load()

    for x in range(n):
        i = int(x // section_width)
        p = (math.cos((x%section_width) / section_width * math.pi)+1)/2
        print('----> %i %.3f'%(i, p))
        r1,g1,b1 = rgb_colors[i]
        r2,g2,b2 = rgb_colors[i+1]
        r = r2 - (r2-r1)*p
        g = g2 - (g2-g1)*p
        b = b2 - (b2-b1)*p
        print("%+ 3i %+ 3i %+ 3i"%(r,g,b))

        for y in range(50):
            pixels[x,y] = (int(r), int(g), int(b))

    # Get script absolute directory
    script_dir = Path(__file__).resolve().parent
    # Generate the directory which will contain the image
    output_dir = script_dir.parent / "output"
    # Create directory if necessary
    output_dir.mkdir(parents=True, exist_ok=True)
    output_filename = output_dir / "palette_preview.png"
    img.save(output_filename)
    print(f"Successfully saved to {output_filename}")

if __name__ == "__main__":
    main()
