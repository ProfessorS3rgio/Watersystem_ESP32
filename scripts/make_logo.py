from PIL import Image
import sys

# Configuration: target printer width and max height
# TARGET_W = 320   # large logo width (must be multiple of 8)
# MAX_H = 160      # large logo height

TARGET_W = 256   # even larger logo width (must be multiple of 8)
MAX_H = 96       # even larger logo height

INFILE = sys.argv[1] if len(sys.argv) > 1 else "stii.png"
OUTFILE = "output.txt"

img = Image.open(INFILE).convert("L")
orig_w, orig_h = img.size

# Compute scale to fit within TARGET_W x MAX_H while preserving aspect ratio
scale = min(TARGET_W / orig_w, MAX_H / orig_h)
new_w = max(1, int(orig_w * scale))
new_h = max(1, int(orig_h * scale))

resized = img.resize((new_w, new_h), Image.LANCZOS)

# Create a white canvas of width TARGET_W and height new_h, paste centered
canvas = Image.new('L', (TARGET_W, new_h), 255)
paste_x = (TARGET_W - new_w) // 2
canvas.paste(resized, (paste_x, 0))

# Convert to 1-bit (monochrome). Black pixels become 0, white 255 in 'L' mode,
# but in '1' mode getpixel returns 0 or 255; we'll treat 0 as black.
bw = canvas.convert('1')
w, h = bw.size
if w % 8 != 0:
    raise SystemExit("Resulting image width must be multiple of 8 (got %d)" % w)

bytes_out = []
for y in range(h):
    for bx in range(0, w, 8):
        byte = 0
        for bit in range(8):
            px = bw.getpixel((bx+bit, y))
            # In '1' mode white is 255, black is 0. We want 1 for black.
            bitval = 1 if px == 0 else 0
            byte = (byte << 1) | bitval
        bytes_out.append(byte)

# Write C array to output file instead of printing to stdout
OUTPUT_FILE = "output.txt"
with open(OUTPUT_FILE, "w") as f:
    f.write("const uint8_t logo[] PROGMEM = {\n")
    for i in range(0, len(bytes_out), 12):
        line = bytes_out[i:i+12]
        f.write("  " + ", ".join("0x%02X" % b for b in line) + ",\n")
    f.write("};\n")
    f.write("// width=%d height=%d\n" % (w,h))

with open(OUTFILE, "a") as f:
    f.write("// source=%s original=%dx%d resized=%dx%d final=%dx%d\n" % (INFILE, orig_w, orig_h, new_w, new_h, w, h))

print("Wrote logo array to %s (width=%d height=%d)" % (OUTFILE, w, h))

# mark TODO complete