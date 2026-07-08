import tkinter as tk
from tkinter import filedialog
from PIL import Image, ImageTk

# ==========================
# Variabel Global
# ==========================

img_original = None
img_preview = None

# ==========================
# Browse Image
# ==========================

def browse_image():
    global img_original
    global img_preview

    filename = filedialog.askopenfilename(
        title="Pilih Gambar",
        filetypes=[
            ("Image Files", "*.png *.jpg *.jpeg *.bmp")
        ]
    )

    if not filename:
        return

    entryFile.delete(0, tk.END)
    entryFile.insert(0, filename)

    img_original = Image.open(filename)

    preview = img_original.copy()
    preview.thumbnail((220,220))

    img_preview = ImageTk.PhotoImage(preview)

    lblPreview.config(image=img_preview)

    lblStatus.config(text="Gambar berhasil dibuka")
# ==========================
# Resize Image
# ==========================

def resize_image():

    global img_original
    global img_preview

    if img_original is None:
        lblStatus.config(text="Belum ada gambar")
        return

    w = int(entryWidth.get())
    h = int(entryHeight.get())

    img = img_original.resize((w, h), Image.LANCZOS)

    preview = img.copy()

    preview = preview.resize((220,220), Image.NEAREST)

    img_preview = ImageTk.PhotoImage(preview)

    lblPreview.config(image=img_preview)

    lblStatus.config(text=f"Resize {w} x {h} berhasil")
    


# ==========================
# Generate Header
# ==========================

def generate_header():

    global img_original

    if img_original is None:
        lblStatus.config(text="Belum ada gambar")
        return

    array_name = entryArray.get().strip()

    width = int(entryWidth.get())
    height = int(entryHeight.get())

    filename = array_name + ".h"

    # Resize gambar
    img = img_original.resize((width, height), Image.LANCZOS)

    # Pastikan RGB
    img = img.convert("RGB")

    with open(filename, "w") as f:

        guard = array_name.upper() + "_H"

        f.write("#ifndef " + guard + "\n")
        f.write("#define " + guard + "\n\n")

        f.write("#include <Arduino.h>\n\n")

        f.write("#define LOGO_WIDTH " + str(width) + "\n")
        f.write("#define LOGO_HEIGHT " + str(height) + "\n\n")

        f.write("const uint16_t " + array_name + "[] PROGMEM = {\n")

        count = 0

        for y in range(height):

            for x in range(width):

                r, g, b = img.getpixel((x, y))

                rgb565 = ((r & 0xF8) << 8) | \
                         ((g & 0xFC) << 3) | \
                         (b >> 3)

                f.write("0x%04X," % rgb565)

                count += 1

                if count % 12 == 0:
                    f.write("\n")

        f.write("\n};\n\n")

        f.write("#endif\n")

    lblStatus.config(text="Header berhasil dibuat : " + filename)
    
    
# ==========================
# Window
# ==========================

root = tk.Tk()

root.title("CYD Image Converter")

root.geometry("820x720")

root.resizable(False,False)

# ==========================
# Judul
# ==========================

lblTitle = tk.Label(
    root,
    text="CYD IMAGE TO HEADER",
    font=("Arial",16,"bold")
)

lblTitle.pack(pady=10)

# ==========================
# File
# ==========================

frameFile = tk.Frame(root)
frameFile.pack(pady=5)

entryFile = tk.Entry(frameFile,width=55)
entryFile.pack(side=tk.LEFT,padx=5)

btnBrowse = tk.Button(
    frameFile,
    text="Browse",
    width=10,
    command=browse_image
    
  
)
btnResize = tk.Button(
    root,
    text="Resize Preview",
    width=25,
    height=2,
    command=resize_image
)

btnResize.pack(pady=5)


btnBrowse.pack(side=tk.LEFT)

# ==========================
# Size
# ==========================

frameSize = tk.Frame(root)
frameSize.pack(pady=10)

tk.Label(frameSize,text="Width").grid(row=0,column=0)

entryWidth = tk.Entry(frameSize,width=6)
entryWidth.insert(0,"40")
entryWidth.grid(row=0,column=1,padx=10)

tk.Label(frameSize,text="Height").grid(row=0,column=2)

entryHeight = tk.Entry(frameSize,width=6)
entryHeight.insert(0,"40")
entryHeight.grid(row=0,column=3,padx=10)

# ==========================
# Array Name
# ==========================

frameArray = tk.Frame(root)
frameArray.pack()

tk.Label(frameArray,text="Array Name").grid(row=0,column=0)

entryArray = tk.Entry(frameArray,width=20)
entryArray.insert(0,"logoBRIN")
entryArray.grid(row=0,column=1,padx=10)

# ==========================
# Pre52view
# ==========================

framePreview = tk.LabelFrame(
    root,
    text="Preview",
    width=250,
    height=250
)

framePreview.pack(pady=15)

framePreview.pack_propagate(False)

lblPreview = tk.Label(framePreview)
lblPreview.pack(expand=True)

# ==========================
# Generate
# ==========================

btnGenerate = tk.Button(
    root,
    text="Generate Header (.h)",
    width=25,
    height=2,
    command=generate_header
)

btnGenerate.pack(pady=10)

# ==========================
# Status
# ==========================

lblStatus = tk.Label(
    root,
    text="Ready",
    fg="blue"
)

lblStatus.pack()

root.mainloop()